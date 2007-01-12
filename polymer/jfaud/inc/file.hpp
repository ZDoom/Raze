#ifndef __file_hpp__
#define __file_hpp__

class JFAudFile {
public:
	typedef enum { Cur, Set, End } SeekFrom;

	JFAudFile();
	JFAudFile(const char *filename, const char *subfilename = (const char*)0);
	virtual ~JFAudFile();
	virtual bool IsOpen(void) const = 0;

	virtual long Read(long nbytes, void *buf) = 0;
	virtual bool ReadByte(char *);
	virtual bool ReadShort(short *, bool big=false);
	virtual bool ReadLong(int *, bool big=false);
	
	virtual long Seek(long pos, SeekFrom where) = 0;
	virtual long Tell(void) const = 0;
	virtual long Length(void) const = 0;

	virtual int Rewind(void) { return Seek(0,Set); }
};

#endif
