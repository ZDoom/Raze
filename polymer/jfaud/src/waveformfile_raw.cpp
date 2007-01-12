#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
# include "watcomhax/cstring"
#else
# include <cstdlib>
# include <cstring>
#endif

#include "log.h"
#include "waveformfile_raw.hpp"
#include "pcmbuffer.hpp"

//using namespace std;

//{{{ Private methods
int WaveformFile_Raw::LoadSamples(PcmBuffer *buf, bool loop)
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

				fh->Rewind();
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

	if (readbytes > 0 && buf->GetBytesPerSample() == 2 && bigendian != B_BIG_ENDIAN) {
		unsigned short *spp = (unsigned short *)buf->GetData();
		for (bytes=0; bytes<readbytes/2; bytes++) {
			*spp = B_SWAP16(*spp);
			spp++;
		}
	}

	buf->SetNumSamples(readbytes / buf->GetBlockSize());

	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_Raw::WaveformFile_Raw(JFAudFile *tfh, int samplerate, int channels, int bytespersample, bool bigendian)
	: isvalid(false), WaveformFile(), fh(NULL),
	  samplerate(-1), channels(-1), bytespersample(-1), bigendian(false),
	  datalen(-1), datapos(-1)
{
	if (!tfh) return;

	if (samplerate <= 0) return;
	if (channels < 1 || channels > 2) return;
	if (bytespersample < 1 || bytespersample > 2) return;

	this->samplerate = samplerate;
	this->channels = channels;
	this->bytespersample = bytespersample;
	this->bigendian = bigendian;

	fh = tfh;
	this->datalen = fh->Seek(0, JFAudFile::End);
	fh->Rewind();
	
	isvalid = true;
}

WaveformFile_Raw::~WaveformFile_Raw()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh) delete fh;
}

PcmBuffer *WaveformFile_Raw::ReadSamples(unsigned int nsamps, bool loop)
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

