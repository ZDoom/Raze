#define JFAUD_INTERNAL
// format description: http://www.borg.com/~jglatt/tech/aiff.htm

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "log.h"
#include "waveformfile_aiff.hpp"
#include "pcmbuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ Private methods
bool WaveformFile_Aiff::ReadChunkHead(char type[4], int *len)
{
	if (fh->Read(4, type) != 4 || !fh->ReadLong(len, true))
		return false;
	return true;
}

bool WaveformFile_Aiff::FindChunk(const char *type, int *start, int *len)
{
	char id[4];

	fh->Seek(4+4+4, JFAudFile::Set);

	while (ReadChunkHead(id,len)) {
		if (id[0] == type[0] && id[1] == type[1] && id[2] == type[2] && id[3] == type[3]) {
			*start = fh->Tell();
			return true;
		}

		fh->Seek(*len, JFAudFile::Cur);
	}
	
	*start = *len = -1;

	return false;
}

int WaveformFile_Aiff::LoadSamples(PcmBuffer *buf, bool loop)
{
	int bytes;
	unsigned int totalbytes = buf->GetMaxSamples() * buf->GetBlockSize(), readbytes = 0;
	char *sp = (char*)buf->GetData();
	bool breakloop = false;

	while (totalbytes > 0) {
		bytes = datalen - datapos;
		if (bytes > totalbytes) bytes = totalbytes;
		
		bytes = fh->Read(bytes, sp);

		if (bytes < 0) return -1;	// read failure
		else if (bytes == 0) {
			if (loop) {
				if (breakloop) break;	// to prevent endless loops

				fh->Seek(datastart, JFAudFile::Set);
				datapos = 0;
				breakloop = true;
			} else {
				break;
			}
		} else {
			sp         += bytes;
			datapos    += bytes;
			readbytes  += bytes;
			totalbytes -= bytes;
			breakloop = false;
		}
	}

	// AIFF uses signed chars, but we use unsigned
	if (readbytes > 0 && buf->GetBytesPerSample() == 1) {
		unsigned char *spp = (unsigned char*)buf->GetData();
		for (bytes=0; bytes<readbytes; bytes++)
			*spp = (unsigned char)((int)(*(signed char *)spp) + 128);
	}
#if B_BIG_ENDIAN != 1
	else if (readbytes > 0 && buf->GetBytesPerSample() == 2) {
		unsigned short *spp = (unsigned short *)buf->GetData();
		for (bytes=0; bytes<readbytes/2; bytes++) {
			*spp = B_BIG16(*spp);
			spp++;
		}
	}
#endif

	buf->SetNumSamples(readbytes / buf->GetBlockSize());

	return 0;
}

// courtesy of Ken Silverman
int WaveformFile_Aiff::tbyte2int_bigendian(unsigned char *t)
{
	int i;
	uint64_t q;
	for(i=0,q=0;i<8;i++) q = (q<<8)+t[i+2];
	return((int)(q>>(16446-(t[0]<<8)-t[1])));
}
//}}}

//{{{ Public methods
WaveformFile_Aiff::WaveformFile_Aiff(JFAudFile *tfh)
	: isvalid(false), isusable(false), WaveformFile(), fh(NULL),
	  samplerate(-1), channels(-1), bytespersample(-1),
	  datastart(-1), datalen(-1), datapos(-1)
{
	int l,len;
	char id[10];
	short s;

	if (!tfh) return;

	fh = tfh;
	fh->Rewind();

	if (!ReadChunkHead(id,&len)) return;
	if (id[0] != 'F' || id[1] != 'O' || id[2] != 'R' || id[3] != 'M') return;
	if (fh->Read(4, id) != 4) return;
	if (id[0] != 'A' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F') return;
	isvalid = true;

	// read the format chunk
	if (!FindChunk("COMM",&l,&len) || len < (2+4+2+10)) return;
	if (!fh->ReadShort(&s, true)) return; else channels = s;
	if (!fh->ReadLong(&l, true)) return; // skip numsampleframes
	if (!fh->ReadShort(&s, true) || (s != 8 && s != 16)) return; else bytespersample = s/8;
	if (fh->Read(10, id) != 10) return; else samplerate = tbyte2int_bigendian((unsigned char *)id);

	// locate the data chunk and keep its properties
	if (!FindChunk("SSND",(int*)&datastart,(int*)&datalen)) return;
	if (datastart < 0 || datalen < 0) return;

	if (!fh->ReadLong(&l, true)) return;	// read offset
	datalen -= 4+4 + l;
	datastart += 4+4 + l;
	fh->Seek(datastart, JFAudFile::Cur);

	datapos = 0;

	isusable = true;
}

WaveformFile_Aiff::~WaveformFile_Aiff()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

PcmBuffer *WaveformFile_Aiff::ReadSamples(unsigned int nsamps, bool loop)
{
	PcmBuffer *buf;
	
	if (!isvalid) return NULL;

	buf = new PcmBuffer();
	if (!buf) return NULL;

	if (nsamps == 0) {
		nsamps = TotalSamples();
		loop = false;
	}

	if (!buf->Allocate(nsamps, samplerate, channels, bytespersample) ||
	    LoadSamples(buf, loop) < 0 ||
	    buf->GetNumSamples() == 0) {
		delete buf;
		return NULL;
	}

	return buf;
}
//}}}

// vim:fdm=marker:

