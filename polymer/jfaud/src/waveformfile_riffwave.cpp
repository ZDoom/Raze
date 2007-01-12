#define JFAUD_INTERNAL
// format description: http://www.borg.com/~jglatt/tech/wave.htm

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "log.h"
#include "waveformfile_riffwave.hpp"
#include "pcmbuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ Private methods
bool WaveformFile_RiffWave::ReadChunkHead(char type[4], int *len)
{
	if (fh->Read(4, type) != 4 || !fh->ReadLong(len, false))
		return false;
	return true;
}

bool WaveformFile_RiffWave::FindChunk(const char *type, int *start, int *len)
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

int WaveformFile_RiffWave::LoadSamples(PcmBuffer *buf, bool loop)
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

#if B_LITTLE_ENDIAN != 1
	if (readbytes > 0 && buf->GetBytesPerSample() == 2) {
		unsigned short *spp = (unsigned short *)buf->GetData();
		for (bytes=0; bytes<readbytes/2; bytes++) {
			*spp = B_LITTLE16(*spp);
			spp++;
		}
	}
#endif

	buf->SetNumSamples(readbytes / buf->GetBlockSize());

	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_RiffWave::WaveformFile_RiffWave(JFAudFile *tfh)
	: isvalid(false), isusable(false), WaveformFile(), fh(NULL),
	  samplerate(-1), channels(-1), bytespersample(-1),
	  datastart(-1), datalen(-1), datapos(-1)
{
	int l,len;
	char id[4];
	short s;

	if (!tfh) return;

	fh = tfh;
	fh->Rewind();

	if (!ReadChunkHead(id,&len)) return;
	if (id[0] != 'R' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F') return;
	if (fh->Read(4, id) != 4) return;
	if (id[0] != 'W' || id[1] != 'A' || id[2] != 'V' || id[3] != 'E') return;
	isvalid = true;

	// read the format chunk
	if (!FindChunk("fmt ",&l,&len) || len < (2+2+4+4+2+2)) return;
	if (!fh->ReadShort(&s, false) || (s != 1)) return;	// must be pcm
	if (!fh->ReadShort(&s, false)) return; else channels = s;
	if (!fh->ReadLong(&l, false)) return; else samplerate = l;
	if (!fh->ReadLong(&l, false) || !fh->ReadShort(&s, false)) return;	// skip avgbps and blkalign
	if (!fh->ReadShort(&s, false) || (s != 8 && s != 16)) return; else bytespersample = s/8;

	// locate the data chunk and keep its properties
	if (!FindChunk("data",(int*)&datastart,(int*)&datalen)) return;
	if (datastart < 0 || datalen < 0) return;
	datapos = 0;

	isusable = true;
}

WaveformFile_RiffWave::~WaveformFile_RiffWave()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

PcmBuffer *WaveformFile_RiffWave::ReadSamples(unsigned int nsamps, bool loop)
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

