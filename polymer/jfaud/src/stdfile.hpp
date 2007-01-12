#ifndef __stdfile_hpp__
#define __stdfile_hpp__

#include "file.hpp"

#ifdef DEBUG
#include "log.h"
#endif

class StdFile : public JFAudFile {
	int fh;
#ifdef DEBUG
	char fn[32];
#endif

public:
	StdFile(const char *filename, const char *subfilename = (const char*)0);
	virtual ~StdFile();
	virtual bool IsOpen(void) const;

	virtual long Read(long nbytes, void *buf);
	virtual long Seek(long pos, SeekFrom where);
	virtual long Tell(void) const;
	virtual long Length(void) const;
};

#endif
