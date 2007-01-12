#ifndef __soundcache_hpp__
#define __soundcache_hpp__

/*
 * The SoundCache holds the contents of files in a hashed pool. The file data
 * is stored in SoundCacheItems which maintain the basic data block and size
 * information. The data can be used in the form of a SoundCacheFile object
 * which are spawned as needed thereby increasing the usage and lock count for
 * the cache item, and deleted at will which in its destructor decreases the
 * lock count. Cache items with high usage counts have longer persistence than
 * those with low usage counts, and only when a cache item has a zero lock will
 * can it be freed.
 */

class SoundCacheItem;

class SoundCacheItemListNode {
	public:
	class SoundCacheItemListNode *next, *prev;
	SoundCacheItem* item;
	SoundCacheItemListNode(SoundCacheItem* i);
	~SoundCacheItemListNode();
};

class SoundCacheItemList
{
private:
	SoundCacheItemListNode *head, *tail, *cur, *next, *prev;
protected:
public:
	SoundCacheItemList();
	~SoundCacheItemList();

	bool Beginning();
	bool End();
	bool Next();
	bool Prev();
	SoundCacheItem* CurrentItem() const;
	bool Valid() const;
	bool DeleteCurrent();
	bool InsertBeforeCurrent(SoundCacheItem* i);
	bool InsertAfterCurrent(SoundCacheItem* i);
	bool AtBeginning() const;
	bool AtEnd() const;
	void *GetCurrentPointer() const;
	void SetCurrentPointer(void *p);
};

class SoundCacheFile : public JFAudFile {
private:
	long posn;
	class SoundCacheItem *owner;
protected:
public:
	SoundCacheFile(class SoundCacheItem *owner);
	virtual ~SoundCacheFile();
	virtual bool IsOpen(void) const { return true; }

	virtual long Read(long nbytes, void *buf);
	virtual long Seek(long pos, SeekFrom where);
	virtual long Tell(void) const;
	virtual long Length(void) const;
};

class SoundCacheItem {
private:
	void *data;
	long length;
	unsigned lock, usage, age;

	uint64_t hashval;

	void Release();
	void Retain();
protected:
public:
	SoundCacheItem(uint64_t, void *, long);
	~SoundCacheItem();

	SoundCacheFile * SpawnFile(void);

	unsigned GetLockCount(void) const { return lock; }
	unsigned GetUsageCount(void) const { return usage; }
	unsigned GetAge(void) const { return lock > 0 ? 0 : age; }
	long     GetLength(void) const { return length; }
	uint64_t GetHashValue(void) const { return hashval; }
	void     IncrementAge(void) { age++; }

	friend class SoundCacheFile;	// SoundCacheFile accesses data, length, Release(), and Retain()
};

class SoundCache {
private:
	unsigned maxcachesize, maxobjectsize, maxage;
	unsigned currentsize;

	SoundCacheItemList hashtable[234];

	uint64_t HashName(const char *filename, const char *subfilename) const;
	bool ReclaimSpace(unsigned bytes);
protected:
public:
	SoundCache();
	~SoundCache();

	bool SetCacheSize(unsigned cache, unsigned object);
		// sets the total cache and per object sound file cache sizes

	bool SetMaxItemAge(unsigned age);
		// sets the maximum age any unused item will grow to before being thrown out

	bool FlushCache(void);
		// forces all unlocked entries to be freed immediately

	JFAudFile* CheckCache(const char *filename, const char *subfilename);
		// probes the cache for the given file and if it is present, returns
		// a file object

	JFAudFile* CacheFile(JFAudFile *file, const char *filename, const char *subfilename);
		// attempts to load the contents of file into the cache. returns the cached
		// file object on success or NULL if unable to cache.

	bool Update(void);
		// ages the cache, etc
};

#endif
