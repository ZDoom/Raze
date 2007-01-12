#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstring"
# include "watcomhax/cstdlib"
#else
# include <cstring>
# include <cstdlib>
#endif

#include "log.h"
#include "file.hpp"
#include "soundcache.hpp"
#include "crc32.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ SoundCacheFile
SoundCacheFile::SoundCacheFile(SoundCacheItem *o)
	: posn(0), owner(o)
{
}

SoundCacheFile::~SoundCacheFile()
{
	owner->Release();
}

long SoundCacheFile::Read(long nbytes, void *buf)
{
	long toread = owner->length - posn;
	if (toread > nbytes) toread = nbytes;
	memcpy(buf, (void *)((intptr_t)owner->data + posn), toread);
	posn += toread;
	return toread;
}

long SoundCacheFile::Seek(long pos, SeekFrom where)
{
	if (where == JFAudFile::Set) {
		posn = pos;
	} else if (where == JFAudFile::Cur) {
		posn = posn + pos;
	} else if (where == JFAudFile::End) {
		posn = owner->length + pos;
	} else return -1;

	if (posn < 0) posn = 0;
	else if (posn > owner->length) posn = owner->length;

	return posn;
}

long SoundCacheFile::Tell(void) const
{
	return posn;
}

long SoundCacheFile::Length(void) const
{
	return owner->length;
}
//}}}

//{{{ SoundCacheItem
void SoundCacheItem::Release()
{
	if (lock == 0) {
		_JFAud_LogMsg("SoundCacheItem::Release() called on 0 lock count!\n");
		return;
	}
	if (--lock == 0) age = 0;
}

void SoundCacheItem::Retain()
{
	lock++;
}

SoundCacheItem::SoundCacheItem(uint64_t h, void *d, long l)
	: hashval(h), data(d), length(l),
	  lock(0), usage(0), age(0)
{
}

SoundCacheItem::~SoundCacheItem()
{
	free(data);
}

SoundCacheFile * SoundCacheItem::SpawnFile(void)
{
	SoundCacheFile *f = new SoundCacheFile(this);
	if (f) {
		Retain();
		usage++;
	}
	return f;
}
//}}}

//{{{ SoundCache
uint64_t SoundCache::HashName(const char *filename, const char *subfilename) const
{
	uint32_t hash = 0, subhash = 0;
	CRC32 crc;

	if (filename) {
		crc.Reset();
		crc.ProcessBlock(filename, strlen(filename));
		hash = crc.Value();
	}
	if (subfilename) {
		crc.Reset();
		crc.ProcessBlock(subfilename, strlen(subfilename));
		subhash = crc.Value();
	}
	return (uint64_t)hash | ((uint64_t)subhash << 32);
}

bool SoundCache::ReclaimSpace(unsigned bytes)
{
	SoundCacheItem *p, *option;
	void *optionp;
	int optioni, i;

	while (maxcachesize - currentsize < bytes) {
		option = NULL;
		for (i = 0; i < sizeof(hashtable)/sizeof(hashtable[0]); i++) {
			for (hashtable[i].Beginning(); hashtable[i].Valid(); hashtable[i].Next()) {
				p = hashtable[i].CurrentItem();
				if (p->GetLockCount() > 0) continue;
				if (option) {
					if (p->GetUsageCount() < option->GetUsageCount()) continue;
						// popular items are more likely to be kept around
					if (p->GetAge() < option->GetAge()) continue;
						// younger items are kept around longer
				}
				option = p;
				optionp = hashtable[i].GetCurrentPointer();
				optioni = i;
			}
		}

		if (option == NULL) return false;	// all space locked up

#ifdef DEBUG
		_JFAud_LogMsg("SoundCache::ReclaimSpace(): reclaiming %d bytes from %016llx age:%d usage:%d\n",
				option->GetLength(), option->GetHashValue(), option->GetAge(), option->GetUsageCount());
#endif
		
		currentsize -= option->GetLength();
		delete option;
		hashtable[optioni].SetCurrentPointer(optionp);
		hashtable[optioni].DeleteCurrent();
	}

	return true;
}

SoundCache::SoundCache()
	: maxcachesize(1048576), maxobjectsize(1048576), maxage(1024),
	  currentsize(0)
{
}

SoundCache::~SoundCache()
{
	FlushCache();
}

bool SoundCache::SetCacheSize(unsigned cache, unsigned object)
{
	maxcachesize = cache;
	maxobjectsize = object;
	return true;
}


bool SoundCache::SetMaxItemAge(unsigned age)
{
	maxage = age;
	return true;
}

bool SoundCache::FlushCache(void)
{
	SoundCacheItem *p;
	int i;

	for (i = 0; i < sizeof(hashtable)/sizeof(hashtable[0]); i++) {
		for (hashtable[i].Beginning(); hashtable[i].Valid(); hashtable[i].Next()) {
			p = hashtable[i].CurrentItem();
			if (p->GetLockCount() > 0) continue;
			currentsize -= p->GetLength();
			delete p;
			hashtable[i].DeleteCurrent();
		}
	}

	return true;
}

JFAudFile* SoundCache::CheckCache(const char *filename, const char *subfilename)
{
	SoundCacheItem *p;
	int i;
	uint64_t hash;

	hash = HashName(filename, subfilename);
	i = hash % (sizeof(hashtable)/sizeof(hashtable[0]));
	for (hashtable[i].Beginning(); hashtable[i].Valid(); hashtable[i].Next()) {
		p = hashtable[i].CurrentItem();
		if (p->GetHashValue() == hash)
			return static_cast<JFAudFile*>( p->SpawnFile() );
	}
	return NULL;
}

JFAudFile* SoundCache::CacheFile(JFAudFile *file, const char *filename, const char *subfilename)
{
	void *data;
	long len;
	int i;
	uint64_t hash;
	SoundCacheItem *p;

	if (file->Length() > maxobjectsize) {
#ifdef DEBUG
		_JFAud_LogMsg("%s(%s) too big for caching: %d vs %d\n", filename, subfilename, file->Length(), maxobjectsize);
#endif
		return NULL;	// object too big for cache
	}
	if (!ReclaimSpace(file->Length())) {
#ifdef DEBUG
		_JFAud_LogMsg("%s(%s) too big for available cache space: %d vs %d\n", filename, subfilename, file->Length(), maxcachesize - currentsize);
#endif
		return NULL;		// not enough cache space for object
	}

	file->Rewind();
	data = malloc(file->Length());
	if (!data) return false;

	len = file->Read(file->Length(), data);

	hash = HashName(filename, subfilename);
	i = hash % (sizeof(hashtable)/sizeof(hashtable[0]));

	p = new SoundCacheItem(hash, data, len);
	if (!p) {
		free(data);
		return NULL;
	}
	
	hashtable[i].Beginning();
	hashtable[i].InsertBeforeCurrent(p);
	currentsize += len;

	return static_cast<JFAudFile*>( p->SpawnFile() );
}

bool SoundCache::Update(void)
{
	SoundCacheItem *p;
	int i;

#if 0 //def DEBUG
	_JFAud_LogMsg("Beginning cache state dump (%d bytes total used)...\n", currentsize);
#endif

	for (i = 0; i < sizeof(hashtable)/sizeof(hashtable[0]); i++) {
		for (hashtable[i].Beginning(); hashtable[i].Valid(); hashtable[i].Next()) {
			p = hashtable[i].CurrentItem();
			if (p->GetLockCount() > 0) continue;

			p->IncrementAge();
#if 0 //def DEBUG
			_JFAud_LogMsg("  %016llx  Lock:%d Used:%d Age:%d\n",
					p->GetHashValue(), p->GetLockCount(), p->GetUsageCount(), p->GetAge());
#endif
			if (p->GetAge() < maxage) continue;

			currentsize -= p->GetLength();
			delete p;
			hashtable[i].DeleteCurrent();
		}
	}

#if 0 //def DEBUG
	_JFAud_LogMsg("End cache state dump...\n");
#endif

	return true;
}
//}}}

//{{{ SoundCacheItemListNode
SoundCacheItemListNode::SoundCacheItemListNode(SoundCacheItem* i) :
	next(NULL),
	prev(NULL),
	item(i)
{ }
SoundCacheItemListNode::~SoundCacheItemListNode() { }
//}}}

//{{{ SoundCacheItemList
SoundCacheItemList::SoundCacheItemList() :
		head(NULL),
		tail(NULL),
		cur(NULL),
		next(NULL),
		prev(NULL)
{ }

SoundCacheItemList::~SoundCacheItemList()
{
	for (; head; head = cur) {
		cur = head->next;
		delete head;
	}
}

bool SoundCacheItemList::Beginning()
{
	cur = head;
	if (!cur) return false;
	next = head->next;
	prev = (SoundCacheItemListNode*)0;
	return true;
}

bool SoundCacheItemList::End()
{
	cur = tail;
	if (!cur) return false;
	next = (SoundCacheItemListNode*)0;
	prev = tail->prev;
	return true;
}
	
bool SoundCacheItemList::Next()
{
	cur = next;
	if (!cur) return false;
	next = cur->next;
	prev = cur->prev;
	return true;
}

bool SoundCacheItemList::Prev()
{
	cur = prev;
	if (!cur) return false;
	next = cur->next;
	prev = cur->prev;
	return true;
}

SoundCacheItem* SoundCacheItemList::CurrentItem() const
{
	if (cur) return cur->item;
	return (SoundCacheItem*)0;
}

bool SoundCacheItemList::Valid() const { return !!cur; }

bool SoundCacheItemList::DeleteCurrent()
{
	if (!cur) return false;
	if (cur == head) {
		head = head->next;
		if (head) head->prev = (SoundCacheItemListNode*)0;
		delete cur;
	} else if (cur == tail) {
		tail = tail->prev;
		if (tail) tail->next = (SoundCacheItemListNode*)0;
		delete cur;
	} else {
		cur->next->prev = cur->prev;
		cur->prev->next = cur->next;
		delete cur;
	}
	cur = (SoundCacheItemListNode*)0;
	return true;
}

bool SoundCacheItemList::InsertBeforeCurrent(SoundCacheItem* i)
{
	SoundCacheItemListNode *x;
	x = new SoundCacheItemListNode(i);
	if (!head) {
		head = tail = cur = x;
	} else if (cur == head) {
		x->next = head;
		head->prev = x;
		head = x;
	} else {
		x->next = cur;
		x->prev = cur->prev;
	}
	prev = cur->prev;
	return true;
}

bool SoundCacheItemList::InsertAfterCurrent(SoundCacheItem* i)
{
	SoundCacheItemListNode *x;
	x = new SoundCacheItemListNode(i);
	if (!head) {
		head = tail = cur = x;
	} else if (cur == tail) {
		x->prev = tail;
		tail->next = x;
		tail = x;
	} else {
		x->prev = cur;
		x->next = cur->next;
	}
	next = cur->next;
	return true;
}

bool SoundCacheItemList::AtBeginning() const { return !cur ? false : (cur == head); }
bool SoundCacheItemList::AtEnd() const { return !cur ? false : (cur == tail); }

void *SoundCacheItemList::GetCurrentPointer() const { return (void*)cur; }
void SoundCacheItemList::SetCurrentPointer(void *p)
{
	cur = (SoundCacheItemListNode *)p;
	next = cur->next;
	prev = cur->prev;
}
//}}}
