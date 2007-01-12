#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "log.h"
#include "waveformfile_au.hpp"
#include "pcmbuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ Private methods
int WaveformFile_Au::LoadSamples(PcmBuffer *buf, bool loop)
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

#if B_BIG_ENDIAN != 1
	if (readbytes > 0 && buf->GetBytesPerSample() == 2) {
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
//}}}

//{{{ Public methods
WaveformFile_Au::WaveformFile_Au(JFAudFile *tfh)
	: isvalid(false), isusable(false), WaveformFile(), fh(NULL),
	  samplerate(-1), channels(-1), bytespersample(-1),
	  datastart(-1), datalen(-1), datapos(-1)
{
	char id[4];

	int l;
	long len;
	short s;

	if (!tfh) return;

	len = tfh->Seek(0, JFAudFile::End);
	tfh->Rewind();

	if (tfh->Read(4, id) != 4) return;
	if (id[0] != '.' || id[1] != 's' || id[2] != 'n' || id[3] != 'd') return;
	isvalid = true;		// passed the magic test, so we're dealing with a .snd file
				// now see whether the contents are acceptable to us

	if (!tfh->ReadLong(&l, true) || l < 24) return; else datastart = l;
	if (!tfh->ReadLong(&l, true)) return; else datalen = l;
	if (datalen < 0 || len-datastart < datalen) datalen = len-datastart;
	if (datalen < 0) return;
	if (!tfh->ReadLong(&l, true)) return;
	switch (l) {
		case fmt_pcm8:  bytespersample = 1; break;
		case fmt_pcm16: bytespersample = 2; break;
		default: return;
	}
	if (!tfh->ReadLong(&l, true)) return; else samplerate = l;
	if (!tfh->ReadLong(&l, true) || (l != 1 && l != 2)) return; else channels = l;
	
	tfh->Seek(datastart, JFAudFile::Set);

	fh = tfh;
	isusable = true;	// file passed all the tests, so we can play it
}

WaveformFile_Au::~WaveformFile_Au()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

PcmBuffer *WaveformFile_Au::ReadSamples(unsigned int nsamps, bool loop)
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

