#define JFAUD_INTERNAL
#if USEMPADEC

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "log.h"
#include "pcmbuffer.hpp"
#include "waveformfile_mpeg.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#if !LINKMPADEC
#include "dynlib.hpp"
#define GETDLSYM(sym,sig) if (( libsyms.sym = (sig) lib->Get( (const char *)#sym )) == NULL) return getdlsymerr( #sym )
static bool getdlsymerr(const char *sym) { _JFAud_LogMsg("  Symbol %s not found. MPEG disabled.\n", sym); return false; }
static DynamicLibrary *lib = NULL;
static int refcount = 0;

static struct {
	mpadec_t (MPADECAPI *mpadec_init)(void);
	int (MPADECAPI *mpadec_uninit)(mpadec_t mpadec);
	int (MPADECAPI *mpadec_reset)(mpadec_t mpadec);
	int (MPADECAPI *mpadec_configure)(mpadec_t mpadec, mpadec_config_t *cfg);
	int (MPADECAPI *mpadec_get_info)(mpadec_t mpadec, void *info, int info_type);
	int (MPADECAPI *mpadec_decode)(mpadec_t mpadec, uint8_t *srcbuf, uint32_t srcsize, uint8_t *dstbuf, uint32_t dstsize, uint32_t *srcused, uint32_t *dstused);
	char * (MPADECAPI *mpadec_error)(int code);
} libsyms;

static bool getallsyms()
{
	GETDLSYM(mpadec_init,      mpadec_t (MPADECAPI *)(void));
	GETDLSYM(mpadec_uninit,    int      (MPADECAPI *)(mpadec_t));
	GETDLSYM(mpadec_reset,     int      (MPADECAPI *)(mpadec_t));
	GETDLSYM(mpadec_configure, int      (MPADECAPI *)(mpadec_t, mpadec_config_t *));
	GETDLSYM(mpadec_get_info,  int      (MPADECAPI *)(mpadec_t, void *, int));
	GETDLSYM(mpadec_decode,    int      (MPADECAPI *)(mpadec_t, uint8_t *, uint32_t, uint8_t *, uint32_t, uint32_t *, uint32_t *));
	GETDLSYM(mpadec_error,     char *   (MPADECAPI *)(int));
	return true;
}

#define mpadec_init libsyms.mpadec_init
#define mpadec_uninit libsyms.mpadec_uninit
#define mpadec_reset libsyms.mpadec_reset
#define mpadec_configure libsyms.mpadec_configure
#define mpadec_get_info libsyms.mpadec_get_info
#define mpadec_decode libsyms.mpadec_decode
#define mpadec_error libsyms.mpadec_error

#endif

//{{{ Private methods
int WaveformFile_Mpeg::InitDecoder(JFAudFile *fh)
{
	int r, i;
	uint32_t siz, inpos;
	
	mpadec_config_t conf = {
		MPADEC_CONFIG_FULL_QUALITY, MPADEC_CONFIG_AUTO, MPADEC_CONFIG_16BIT,
		B_BIG_ENDIAN ? MPADEC_CONFIG_BIG_ENDIAN : MPADEC_CONFIG_LITTLE_ENDIAN,
		MPADEC_CONFIG_REPLAYGAIN_NONE, 1, 1, 1, 0.0
	};

	mpa = mpadec_init();
	if (!mpa) return Error;

	if ((r = mpadec_configure(mpa, &conf)) != MPADEC_RETCODE_OK) {
		_JFAud_LogMsg("mpadec_configure error: %s\n",mpadec_error(r));
		return Error;
	}
	
	// set up
	insiz = inpos = 0;
	outsiz = outpos = 0;
	streamoffs = streamlen = streampos = 0;

	streamlen = fh->Seek(0,JFAudFile::End);
	fh->Rewind();

	// check for an ID3v2 tag
	r = fh->Read(min(10,streamlen), inbuf);
	if (r < 0) return Error;
	if (r < 4) return NotMpeg;

	if (r == 10 &&
	    inbuf[0] == 'I' && inbuf[1] == 'D' && inbuf[2] == '3' &&	// magic number
	    inbuf[3] < 255  && inbuf[4] < 255  &&				// version
	    inbuf[6] < 128  && inbuf[7] < 128  && inbuf[8] < 128  && inbuf[9] < 128	// size
	   ) {
		// ID3v2 was found, so work out how big it is to skip it
		streamoffs  = (int)inbuf[6] << 21;
		streamoffs |= (int)inbuf[7] << 14;
		streamoffs |= (int)inbuf[8] << 7;
		streamoffs |= (int)inbuf[9];
		if (inbuf[5] & 16) streamoffs += 10;	// footer
		streamoffs += 10;	// length of id3v2 header

		fh->Seek(streamoffs,JFAudFile::Set);
		streampos = 0;
		streamlen -= streamoffs;
		insiz = 0;
	} else {
		// no tag, so the ten bytes we read are probably audio data
		insiz = streampos = r;
	}

	// top up the input buffer
	r = fh->Read(sizeof(inbuf) - insiz, inbuf + insiz);
	if (r < 0) return Error;
	insiz += r;
	streampos += r;
	if (insiz < 4) return NotMpeg;
	
	if ((r = mpadec_reset(mpa)) != MPADEC_RETCODE_OK) {
		_JFAud_LogMsg("mpadec_reset error: %s\n",mpadec_error(r));
		return Error;
	}
	
	// decode the header
	r = mpadec_decode(mpa, inbuf, insiz, NULL, 0, &inpos, NULL);
	if (r != MPADEC_RETCODE_OK) {
		_JFAud_LogMsg("mpadec_decode error: %s\n",mpadec_error(r));
		return NotMpeg;
	}

	if (inpos > 0) {
		insiz -= inpos;
		memmove(inbuf, inbuf + inpos, insiz);
	}

	// fetch stream format info and any xing/lame tags
	if ((r = mpadec_get_info(mpa, &streaminfo, MPADEC_INFO_STREAM)) != MPADEC_RETCODE_OK ||
	    (r = mpadec_get_info(mpa, &taginfo,    MPADEC_INFO_TAG))    != MPADEC_RETCODE_OK) {
		_JFAud_LogMsg("mpadec_get_info error: %s\n",mpadec_error(r));
		return NotMpeg;
	}

	// a tag might have supplied us with the proper length and frame count
	if ((taginfo.flags & 2) && streamlen > taginfo.bytes) streamlen = taginfo.bytes;
	if (taginfo.flags & 1) {
		// VBR files will trigger this with the Xing tag, unless it's missing
		streaminfo.frames = taginfo.frames;

		// we know the number of samples per frame, the length of the data,
		// the sampling rate, and now the number of frames, so calculate
		// the file's bitrate
		// 125 = 8 (bits per byte, streamlen) / 1000 (bits per kilobit, bitrate)
		if (streaminfo.frames > 0 && streaminfo.frame_samples) {
			streaminfo.bitrate = (int32_t)(( (int64_t)streamlen * streaminfo.frequency ) /
				( (int64_t)streaminfo.frames * streaminfo.frame_samples * 125 ));
		}
	} else if (streaminfo.frame_samples > 0 && streaminfo.bitrate > 0) {
		// knowing the length of the data, the sampling rate, the number of
		// samples per frame, and the bitrate, we determine the number
		// of frames in the file
		streaminfo.frames = (int32_t)(( (int64_t)streamlen * streaminfo.frequency ) /
			( (int64_t)streaminfo.frame_samples * streaminfo.bitrate * 125 ));
	}

#ifdef DEBUG
	{
		char *spec;
		if (streaminfo.frequency < 16000) spec = "2.5";
		else if (streaminfo.frequency < 32000) spec = "2";
		else spec = "1";
		_JFAud_LogMsg("File is MPEG %s Layer %d, %d chans, %d kbps, %d Hz, %d frames\n",
				spec, streaminfo.layer, streaminfo.channels,
				streaminfo.bitrate, streaminfo.frequency,
				streaminfo.frames);
	}
#endif

	return NoError;
}

void WaveformFile_Mpeg::UninitDecoder()
{
	if (mpa) mpadec_uninit(mpa);
	mpa = NULL;
}

int WaveformFile_Mpeg::DecodeSome(int bytes, char *p)
{
	int r, rb = 0;
	uint32_t inpos = 0, siz;

	while (bytes > 0) {
		if (outsiz > 0) {
			// consume data already decoded
			siz = min(outsiz, bytes);
			memcpy(p, outbuf + outpos, siz);
			outpos += siz;
			outsiz -= siz;
			bytes  -= siz;
			p      += siz;
			rb     += siz;
		} else {
			// top up the input buffer
			if (insiz < sizeof(inbuf)) {
				r = min(sizeof(inbuf) - insiz, streamlen - streampos);
				if (r > 0) {
					r = fh->Read(r, inbuf + insiz);
					if (r < 0) return -1;
				}
				insiz += r;
				streampos += r;
			}

			// couldn't fill the input buffer at all
			if (insiz == 0) break;
			
			// try and decode as much as possible direct into the user's buffer
			outsiz = outpos = inpos = 0;
			r = mpadec_decode(mpa, inbuf, insiz, (uint8_t*)p, bytes, &inpos, &siz);
			p     += siz;
			bytes -= siz;
			rb    += siz;
			if (r == MPADEC_RETCODE_BUFFER_TOO_SMALL) {
				if (siz == 0) {
					// decode some more into our spill area so we can pack up the user's buffer
					// as much as possible
					r = mpadec_decode(mpa, inbuf, insiz, outbuf, sizeof(outbuf), &inpos, &outsiz);
					if (r != MPADEC_RETCODE_OK &&
					    r != MPADEC_RETCODE_BUFFER_TOO_SMALL &&
					    r != MPADEC_RETCODE_NEED_MORE_DATA) {
						_JFAud_LogMsg("mpadec_decode(1) error: %s\n", mpadec_error(r));
						return -1;
					}
				}
			} else if (r == MPADEC_RETCODE_NEED_MORE_DATA) {
				// starved of data and unable to decode any more, so give up
				if (siz == 0) {
					insiz = 0;
					break;
				}
			} else if (r != MPADEC_RETCODE_OK) {
				_JFAud_LogMsg("mpadec_decode(2) error: %s\n", mpadec_error(r));
				return -1;
			}
			insiz -= inpos;
			if (inpos > 0) memmove(inbuf, inbuf + inpos, insiz);
		}
	}

	return rb;
}

int WaveformFile_Mpeg::LoadSamples(PcmBuffer *buf, bool loop)
{
	int bytes, bitstr;
	unsigned int totalbytes = buf->GetMaxSamples() * buf->GetBlockSize(), readbytes = 0;
	char *sp = (char*)buf->GetData();
	bool breakloop = false;

	while (totalbytes > 0) {
		bytes = DecodeSome(totalbytes, sp);

		if (bytes < 0) return -1;	// read failure
		else if (bytes == 0) {
			if (loop) {
				if (breakloop) break;	// to prevent endless loops

				fh->Seek(streamoffs, JFAudFile::Set);
				streampos = 0;

				breakloop = true;
			} else {
				break;
			}
		} else {
			sp         += bytes;
			readbytes  += bytes;
			totalbytes -= bytes;
			breakloop = false;
		}
	}

	buf->SetNumSamples(readbytes / buf->GetBlockSize());

	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_Mpeg::WaveformFile_Mpeg(JFAudFile *tfh)
	: isvalid(false), WaveformFile(), fh(NULL), mpa(NULL)
{
	int r;

	if (!tfh) return;
#if !LINKMPADEC
	if (!lib) return;
#endif

	if (InitDecoder(tfh) == NoError) {
		fh = tfh;
		isvalid = true;
	}
}

WaveformFile_Mpeg::~WaveformFile_Mpeg()
{
	UninitDecoder();
	if (fh) delete fh;
}

PcmBuffer *WaveformFile_Mpeg::ReadSamples(unsigned int nsamps, bool loop)
{
	PcmBuffer *buf;

	if (!isvalid) return NULL;

	buf = new PcmBuffer();
	if (!buf) return NULL;

	if (nsamps == 0) {
		nsamps = (unsigned int)(streaminfo.decoded_frame_samples * streaminfo.frames);
		loop = false;
	}

	if (!buf->Allocate(nsamps, streaminfo.decoded_frequency, streaminfo.decoded_channels, 2) ||
	    LoadSamples(buf, loop) < 0 ||
	    buf->GetNumSamples() == 0) {
		delete buf;
		return NULL;
	}

	return buf;
}

float WaveformFile_Mpeg::GetPlayTime(void) const
{
	if (!isvalid) return 0.0;
	return (float)(streaminfo.decoded_frame_samples * streaminfo.frames) / (float)streaminfo.decoded_frequency;
}

unsigned int WaveformFile_Mpeg::GetPCMLength(void) const
{
	if (!isvalid) return 0;
	return (unsigned int)(streaminfo.decoded_frame_samples * streaminfo.frames);
}

SoundFile::InitState WaveformFile_Mpeg::Init()
{
#if !LINKMPADEC
	if (lib) { refcount++; return SoundFile::InitOK; }
	
	_JFAud_LogMsg("Loading " MPADECDL "\n");
	lib = new DynamicLibrary(MPADECDL);
	if (!lib) return SoundFile::InitDisabled;
	if (!lib->IsOpen()) {
		delete lib;
		lib = NULL;
		return SoundFile::InitDisabled;
	}

	if (getallsyms()) refcount = 1;
	else {
		delete lib;
		lib = NULL;
		return SoundFile::InitDisabled;
	}
#endif
	return SoundFile::InitOK;
}

bool WaveformFile_Mpeg::Uninit()
{
#if !LINKMPADEC
	if (refcount > 1) { refcount--; return true; }
	if (refcount == 0 || !lib) return false;
	refcount = 0;
	delete lib;
	lib = NULL;
#endif
	return true;
}
//}}}

#endif

// vim:fdm=marker:

