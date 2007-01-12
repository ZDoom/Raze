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
#include "waveformfile_voc.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ Private methods
int WaveformFile_Voc::LoadSamples(PcmBuffer *buf, bool loop)
{
	int bytes;
	unsigned int totalbytes = buf->GetMaxSamples() * buf->GetBlockSize(), readbytes = 0;
	char *sp = (char*)buf->GetData();
	bool breakloop = false;

	while (totalbytes > 0) {
		bytes = datalen - datapos;
		if (bytes > totalbytes) bytes = totalbytes;
		
		bytes = ReadBlockData(bytes, sp);

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

int WaveformFile_Voc::ReadBlockData(int bytes, char *ptr)
{
	int bytesread = 0, skip;
	while (bytes > 0) {
		if (blockleft == 0) {
			if (!ReadBlockHead() || block.type == blk_terminate) break;
			skip = 0;
			switch (block.type) {
				case blk_sounddata:
					blockleft = block.info.sounddata.len;
					break;
				case blk_soundcont:
					blockleft = block.info.soundcont.len;
					break;
				case blk_type9:
					blockleft = block.info.type9.len;
					break;

				case blk_asciiz:
					skip = block.info.asciiz.len;
					break;

				default: break;
			}
			if (skip > 0) fh->Seek(skip, JFAudFile::Cur);
		} else {
			skip = fh->Read(min(bytes, blockleft), ptr);
			if (skip < 0) return -1;
			else if (skip == 0) { blockleft = 0; break; }	// short read, truncated block

			ptr += skip;
			bytesread += skip;
			bytes -= skip;
			blockleft -= skip;
		}
	}
	return bytesread;
}

int WaveformFile_Voc::ReadBlockHead()
{
	int len = 0;
	int l;
	unsigned short s;
	unsigned char c;

	if (!fh->ReadByte(&block.type)) goto failure;

	if (block.type == blk_terminate) return 1;
	
	if (fh->Read(3, &len) != 3) goto failure;
	len = B_LITTLE32(len);

	switch (block.type) {
		case blk_sounddata:
			if (len < 2) goto failure;
			block.info.sounddata.len = len - 2;
			if (!fh->ReadByte((char*)&c)) goto failure; block.info.sounddata.samplerate = 1000000/(256-c);
			if (!fh->ReadByte(&block.info.sounddata.format)) goto failure;
			break;
		case blk_soundcont:
			block.info.soundcont.len = len;
			break;
		case blk_silence:
			if (len != 3) goto failure;
			if (!fh->ReadShort((short*)&s, false)) goto failure; block.info.silence.len = (int)s+1;
			if (!fh->ReadByte((char*)&c)) goto failure; block.info.silence.samplerate = 1000000/(256-c);
			break;
		case blk_marker:
			if (len != 2) goto failure;
			if (!fh->ReadShort((short*)&block.info.marker.value)) goto failure;
			break;
		case blk_asciiz:
			block.info.asciiz.len = len;
			break;
		case blk_repeat:
			if (len != 2) goto failure;
			if (!fh->ReadShort((short*)&block.info.repeat.count)) goto failure;
			break;
		case blk_extended:
			if (len != 4) goto failure;
			if (!fh->ReadShort((short*)&s)) goto failure;
			if (!fh->ReadByte((char*)&block.info.extended.format)) goto failure;
			if (!fh->ReadByte((char*)&c)) goto failure; block.info.extended.channels = c+1;
			block.info.extended.samplerate = 256000000/((c+1)*(65536-s));
			break;
		case blk_type9:
			if (len < 12) goto failure;
			block.info.type9.len = len-12;
			if (!fh->ReadLong(&block.info.type9.samplerate)) goto failure;
			if (!fh->ReadByte((char*)&block.info.type9.bitspersample)) goto failure;
			if (!fh->ReadByte((char*)&block.info.type9.channels)) goto failure;
			if (!fh->ReadShort((short*)&block.info.type9.format)) goto failure;
			if (!fh->ReadLong(&l)) goto failure;
			break;
		default: goto failure;
	}

	return 1;

failure:
	block.type = blk_terminate;
	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_Voc::WaveformFile_Voc(JFAudFile *tfh)
	: isvalid(false), isusable(false), WaveformFile(), fh(NULL),
	  samplerate(-1), channels(-1), bytespersample(-1),
	  datastart(-1), datalen(-1), datapos(-1), blockleft(0)
{
	char id[20];

	int l,len;
	short s,s2;

	int srate = -1, skip;
	short chans = -1, format = -1, bps = -1;

	if (!tfh) return;

	tfh->Rewind();

	// header
	if (tfh->Read(20, id) != 20) return;
	if (memcmp(id, "Creative Voice File\x1a", 20)) return;
	isvalid = true;	// file passed the magic test
	
	if (!tfh->ReadShort(&s, false) || s < 26) return; else datastart = s;
	if (!tfh->ReadShort(&s, false)) return;
	if (!tfh->ReadShort(&s2, false) || ~s+0x1234 != s2) return;

	// scan the blocks and work out how long the sound is
	tfh->Seek(datastart, JFAudFile::Set);
	fh = tfh;	// ReadBlockHead needs this

	datalen = datapos = blockleft = 0;
	while (ReadBlockHead() && block.type != blk_terminate) {
		skip = 0;
		switch (block.type) {
			case blk_sounddata:
				if (srate < 0) {
					srate = block.info.sounddata.samplerate;
					chans = 1;
					format = block.info.sounddata.format;
				} else if (srate != block.info.sounddata.samplerate ||
					format != block.info.sounddata.format) return;	// inconsistent format
				skip = block.info.sounddata.len;
				datalen += skip;
				break;
			case blk_soundcont:
				if (srate < 0) return;	// invalid format: can't continue sound with no format
				skip = block.info.soundcont.len;
				datalen += skip;
				break;
			case blk_extended:
				if (srate < 0) {
					srate = block.info.extended.samplerate;
					chans = block.info.extended.channels;
					format = block.info.extended.format;
				} else if (srate != block.info.type9.samplerate ||
					chans != block.info.type9.channels ||
					format != block.info.type9.format) return;	// inconsistent format
				break;
			case blk_type9:
				if (srate < 0) {
					srate = block.info.type9.samplerate;
					bps = block.info.type9.bitspersample;
					chans = block.info.type9.channels;
					format = block.info.type9.format;
				} else if (srate != block.info.type9.samplerate ||
					chans != block.info.type9.channels ||
					format != block.info.type9.format) return;	// inconsistent format
				skip = block.info.type9.len;
				datalen += skip;
				break;

			case blk_asciiz:
				skip = block.info.asciiz.len;
				break;

			default: break;
		}
		if (skip > 0) fh->Seek(skip, JFAudFile::Cur);
	}

	if (srate < 0) return;	// file has no sound blocks in it

	samplerate = srate;
	switch (format) {
		case fmt_pcm8:  if (bps >= 0 && bps != 8) return; bytespersample = 1; break;
		case fmt_pcm16: if (bps >= 0 && bps != 16) return; bytespersample = 2; break;
		default: return;	// unsupported format
	}
	channels = chans;

	fh->Seek(datastart, JFAudFile::Set);

	isusable = true;
}

WaveformFile_Voc::~WaveformFile_Voc()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

PcmBuffer *WaveformFile_Voc::ReadSamples(unsigned int nsamps, bool loop)
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

