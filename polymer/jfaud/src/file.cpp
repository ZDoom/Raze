#define JFAUD_INTERNAL
#include "file.hpp"
#include "sysdefs.h"


JFAudFile::JFAudFile()
{
}

JFAudFile::JFAudFile(const char *filename, const char *subfilename)
{
}

JFAudFile::~JFAudFile()
{
}

bool JFAudFile::ReadByte(char *c)
{
	return (Read(1, c) == 1);
}

bool JFAudFile::ReadShort(short *s, bool big)
{
	short ss;
	if (Read(2, &ss) != 2) return false;
	if (big) ss = B_BIG16(ss); else ss = B_LITTLE16(ss);
	*s = ss;
	return true;
}

bool JFAudFile::ReadLong(int *l, bool big)
{
	int ll;
	if (Read(4, &ll) != 4) return false;
	if (big) ll = B_BIG32(ll); else ll = B_LITTLE32(ll);
	*l = ll;
	return true;
}
