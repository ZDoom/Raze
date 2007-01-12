#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstring"
# include "watcomhax/cstdio"
#else
# include <cstring>
# include <cstdio>
#endif
#include <sys/types.h>
#include <fcntl.h>
#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
#endif
#include "stdfile.hpp"


#ifndef SCREWED_UP_CPP
using namespace std;
#endif

StdFile::StdFile(const char *filename, const char *subfilename)
	: JFAudFile(filename, subfilename)
{
	fh = -1;
	if (subfilename && strlen(subfilename) > 0) return;	// stdio doesn't handle containers

	fh = open(filename, O_RDONLY|O_BINARY);

#ifdef DEBUG
	strncpy(fn, filename, sizeof(fn)-1);
	fn[sizeof(fn)-1] = 0;
	_JFAud_LogMsg("StdFile::StdFile: opened %s\n", fn);
#endif
}

StdFile::~StdFile()
{
	if (fh >= 0) {
		close(fh);
#ifdef DEBUG
		_JFAud_LogMsg("StdFile::StdFile: closed %s\n", fn);
#endif
	}
}

bool StdFile::IsOpen(void) const
{
	return fh >= 0;
}

long StdFile::Read(long nbytes, void *buf)
{
	if (fh < 0) return -1;
	return read(fh, buf, nbytes);
}

long StdFile::Seek(long pos, SeekFrom where)
{
	int whence;
	if (fh < 0) return -1;
	switch (where) {
		case JFAudFile::Cur: whence = SEEK_CUR; break;
		case JFAudFile::Set: whence = SEEK_SET; break;
		case JFAudFile::End: whence = SEEK_END; break;
		default: return -1;
	}
	return lseek(fh, pos, whence);
}

long StdFile::Tell(void) const
{
	if (fh < 0) return -1;
#ifdef _WIN32
	return tell(fh);
#else
	return (long)lseek(fh,0,SEEK_CUR);
#endif
}

long StdFile::Length(void) const
{
	if (fh < 0) return -1;
#ifdef _WIN32
	return filelength(fh);
#else
	{
		long x,y;

		x = lseek(fh, 0, SEEK_CUR);
		y = lseek(fh, 0, SEEK_END);
		lseek(fh, x, SEEK_SET);
		return y;
	}
#endif
}
