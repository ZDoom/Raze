#ifndef __memfile_hpp__
#define __memfile_hpp__

#include <cstdlib>
#include <cstring>
#include "file.hpp"
#include "sysdefs.h"

class MemFile : public JFAudFile {
private:
	unsigned char *dataptr, *posn;
	long datalen;
	void (*disposalfunc)(void*);

public:
	MemFile(void *ptr, long len, void (*dispose)(void*))
	{
		dataptr = posn = (unsigned char *)ptr;
		datalen = len;
		disposalfunc = dispose;
	}
	virtual ~MemFile()
	{
		if (disposalfunc) disposalfunc((void*)dataptr);
		else free((void*)dataptr);
	}
	virtual bool IsOpen(void) const { return dataptr != NULL && datalen >= 0; }

	virtual long Read(long nbytes, void *buf)
	{
		long toread = datalen - ((intptr_t)posn - (intptr_t)dataptr);
		if (toread > nbytes) toread = nbytes;
		memcpy(buf, posn, toread);
		posn = (unsigned char *)((intptr_t)posn + toread);

		return toread;
	}
	virtual bool ReadByte(char *c)
	{
		if ((intptr_t)posn - (intptr_t)dataptr >= datalen) return false;
		*c = *(posn++);
		return true;
	}
	virtual bool ReadShort(short *s, bool big=false)
	{
		short ss = 0;
		if ((intptr_t)posn - (intptr_t)dataptr >= datalen-1) return false;
		if (B_LITTLE_ENDIAN == 1) {
			ss  = ((short)*(posn++));
			ss |= ((short)*(posn++)) << 8;
		} else {
			ss  = ((short)*(posn++)) << 8;
			ss |= ((short)*(posn++));
		}
		*s = ss;
		return true;
	}
	virtual bool ReadLong(int *l, bool big=false)
	{
		short ll = 0;
		if ((intptr_t)posn - (intptr_t)dataptr >= datalen-3) return false;
		if (B_LITTLE_ENDIAN == 1) {
			ll  = ((int)*(posn++));
			ll |= ((int)*(posn++)) << 8;
			ll |= ((int)*(posn++)) << 16;
			ll |= ((int)*(posn++)) << 24;
		} else {
			ll  = ((int)*(posn++)) << 24;
			ll |= ((int)*(posn++)) << 16;
			ll |= ((int)*(posn++)) << 8;
			ll |= ((int)*(posn++));
		}
		*l = ll;
		return true;
	}
	
	virtual long Seek(long pos, SeekFrom where)
	{
		if (where == JFAudFile::Set) {
			posn = (unsigned char *)((intptr_t)dataptr + pos);
		} else if (where == JFAudFile::Cur) {
			posn = (unsigned char *)((intptr_t)posn + pos);
		} else if (where == JFAudFile::End) {
			posn = (unsigned char *)((intptr_t)dataptr + datalen + pos);
		} else return -1;

		if ((intptr_t)posn < (intptr_t)dataptr)
			posn = dataptr;
		else if ((intptr_t)posn > ((intptr_t)dataptr + datalen))
			posn = (unsigned char *)((intptr_t)dataptr + datalen);

		return (intptr_t)posn - (intptr_t)dataptr;
	}
	
	virtual long Tell(void) const { return (intptr_t)posn - (intptr_t)dataptr; }
	virtual long Length(void) const { return datalen; }

	virtual int Rewind(void) { posn = dataptr; return 0; }
};

#endif
