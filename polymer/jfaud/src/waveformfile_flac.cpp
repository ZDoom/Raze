#define JFAUD_INTERNAL
#if USEFLAC

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
# include "watcomhax/cstring"
#else
# include <cstdlib>
# include <cstring>
#endif

#include "log.h"
#include "waveformfile_flac.hpp"
#include "pcmbuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#if !LINKFLAC
#include "dynlib.hpp"
#define GETDLSYM(sym,sig) if (( libsyms.sym = (sig) lib->Get( (const char *)#sym )) == NULL) return getdlsymerr( #sym )
static bool getdlsymerr(const char *sym) { _JFAud_LogMsg("  Symbol %s not found. FLAC disabled.\n", sym); return false; }
static DynamicLibrary *lib = NULL;
static int refcount = 0;

static struct {
	FLAC__SeekableStreamDecoder *(*FLAC__seekable_stream_decoder_new)();
	void (*FLAC__seekable_stream_decoder_delete)(FLAC__SeekableStreamDecoder *decoder);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_client_data)(FLAC__SeekableStreamDecoder *decoder, void *value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_read_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_write_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_seek_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_tell_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_length_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_eof_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_metadata_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value);
	FLAC__bool (*FLAC__seekable_stream_decoder_set_error_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value);
	FLAC__SeekableStreamDecoderState (*FLAC__seekable_stream_decoder_init)(FLAC__SeekableStreamDecoder *decoder);
	FLAC__bool (*FLAC__seekable_stream_decoder_finish)(FLAC__SeekableStreamDecoder *decoder);
	FLAC__SeekableStreamDecoderState (*FLAC__seekable_stream_decoder_get_state)(const FLAC__SeekableStreamDecoder *decoder);
	const char *(*FLAC__seekable_stream_decoder_get_resolved_state_string)(const FLAC__SeekableStreamDecoder *decoder);
	FLAC__bool (*FLAC__seekable_stream_decoder_process_single)(FLAC__SeekableStreamDecoder *decoder);
	FLAC__bool (*FLAC__seekable_stream_decoder_process_until_end_of_metadata)(FLAC__SeekableStreamDecoder *decoder);
	FLAC__bool (*FLAC__seekable_stream_decoder_seek_absolute)(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

	const char * const *FLAC__SeekableStreamDecoderStateString;
} libsyms;

static bool getallsyms()
{
	GETDLSYM(FLAC__seekable_stream_decoder_new,                           FLAC__SeekableStreamDecoder *(*)());
	GETDLSYM(FLAC__seekable_stream_decoder_delete,                        void (*)(FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_set_client_data,               FLAC__bool (*)(FLAC__SeekableStreamDecoder *,void *));
	GETDLSYM(FLAC__seekable_stream_decoder_set_read_callback,             FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderReadCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_write_callback,            FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderWriteCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_seek_callback,             FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderSeekCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_tell_callback,             FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderTellCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_length_callback,           FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderLengthCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_eof_callback,              FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderEofCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_metadata_callback,         FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderMetadataCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_set_error_callback,            FLAC__bool (*)(FLAC__SeekableStreamDecoder *,FLAC__SeekableStreamDecoderErrorCallback));
	GETDLSYM(FLAC__seekable_stream_decoder_init,                          FLAC__SeekableStreamDecoderState (*)(FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_finish,                        FLAC__bool (*)(FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_get_state,                     FLAC__SeekableStreamDecoderState (*)(const FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_get_resolved_state_string,     const char *(*)(const FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_process_single,                FLAC__bool (*)(FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_process_until_end_of_metadata, FLAC__bool (*)(FLAC__SeekableStreamDecoder *));
	GETDLSYM(FLAC__seekable_stream_decoder_seek_absolute,                 FLAC__bool (*)(FLAC__SeekableStreamDecoder *, FLAC__uint64));
	GETDLSYM(FLAC__SeekableStreamDecoderStateString,                      const char * const *);
	return true;
}

#define FLAC__seekable_stream_decoder_new libsyms.FLAC__seekable_stream_decoder_new
#define FLAC__seekable_stream_decoder_delete libsyms.FLAC__seekable_stream_decoder_delete
#define FLAC__seekable_stream_decoder_set_client_data libsyms.FLAC__seekable_stream_decoder_set_client_data
#define FLAC__seekable_stream_decoder_set_read_callback libsyms.FLAC__seekable_stream_decoder_set_read_callback
#define FLAC__seekable_stream_decoder_set_write_callback libsyms.FLAC__seekable_stream_decoder_set_write_callback
#define FLAC__seekable_stream_decoder_set_seek_callback libsyms.FLAC__seekable_stream_decoder_set_seek_callback
#define FLAC__seekable_stream_decoder_set_tell_callback libsyms.FLAC__seekable_stream_decoder_set_tell_callback
#define FLAC__seekable_stream_decoder_set_length_callback libsyms.FLAC__seekable_stream_decoder_set_length_callback
#define FLAC__seekable_stream_decoder_set_eof_callback libsyms.FLAC__seekable_stream_decoder_set_eof_callback
#define FLAC__seekable_stream_decoder_set_metadata_callback libsyms.FLAC__seekable_stream_decoder_set_metadata_callback
#define FLAC__seekable_stream_decoder_set_error_callback libsyms.FLAC__seekable_stream_decoder_set_error_callback
#define FLAC__seekable_stream_decoder_init libsyms.FLAC__seekable_stream_decoder_init
#define FLAC__seekable_stream_decoder_finish libsyms.FLAC__seekable_stream_decoder_finish
#define FLAC__seekable_stream_decoder_get_state libsyms.FLAC__seekable_stream_decoder_get_state
#define FLAC__seekable_stream_decoder_get_resolved_state_string libsyms.FLAC__seekable_stream_decoder_get_resolved_state_string
#define FLAC__seekable_stream_decoder_process_single libsyms.FLAC__seekable_stream_decoder_process_single
#define FLAC__seekable_stream_decoder_process_until_end_of_metadata libsyms.FLAC__seekable_stream_decoder_process_until_end_of_metadata
#define FLAC__seekable_stream_decoder_seek_absolute libsyms.FLAC__seekable_stream_decoder_seek_absolute
#define FLAC__SeekableStreamDecoderStateString libsyms.FLAC__SeekableStreamDecoderStateString

#endif

//{{{ Callbacks
FLAC__SeekableStreamDecoderReadStatus WaveformFile_Flac::readCb(
	const FLAC__SeekableStreamDecoder *decoder,
	FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;
	int rb;

	rb = file->fh->Read(*bytes, buffer);
	if (rb < 0) return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;

	*bytes = (unsigned)rb;

	return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
}

FLAC__StreamDecoderWriteStatus WaveformFile_Flac::writeCb(
	const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame,
	const FLAC__int32 *const buffer[], void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;
	unsigned i,j;
	short *p;

	/*
	file->samplerate = frame->header.sample_rate;
	file->channels = frame->header.channels;
	file->blocksize = (int)frame->header.channels * 2;
	*/
	file->pcmleft = (int)frame->header.blocksize;
	if (file->pcmalloc < file->pcmleft) {
		p = (short *)realloc(file->pcm, file->pcmleft * file->blocksize);
		if (!p) {
			file->pcmleft = 0;
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
		file->pcm = p;
		file->pcmalloc = file->pcmleft;
	} else p = file->pcm;
	for (i = 0; i < frame->header.blocksize; i++)
		for (j = 0; j < frame->header.channels; j++)
			*(p++) = buffer[j][i];// >> 16;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__SeekableStreamDecoderSeekStatus WaveformFile_Flac::seekCb(
	const FLAC__SeekableStreamDecoder *decoder,
	FLAC__uint64 absolute_byte_offset, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;

	if (file->fh->Seek((int)absolute_byte_offset, JFAudFile::Set) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

	return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus WaveformFile_Flac::tellCb(
	const FLAC__SeekableStreamDecoder *decoder,
	FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;
	int rb;

	rb = file->fh->Tell();
	if (rb < 0) return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

	*absolute_byte_offset = (FLAC__uint64)rb;
	return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__SeekableStreamDecoderLengthStatus WaveformFile_Flac::lengthCb(
	const FLAC__SeekableStreamDecoder *decoder,
	FLAC__uint64 *stream_length, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;

	*stream_length = file->filelen;
	return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool WaveformFile_Flac::eofCb(
	const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;
        int rb;

        rb = file->fh->Tell();
        if (rb < 0) return 1;

        return ((FLAC__uint64)rb >= file->filelen);
	
}

void WaveformFile_Flac::metadataCb(
	const FLAC__SeekableStreamDecoder *decoder,
	const FLAC__StreamMetadata *metadata, void *client_data)
{
	WaveformFile_Flac *file = (WaveformFile_Flac *)client_data;
	switch (metadata->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			file->samplerate = metadata->data.stream_info.sample_rate;
			file->channels   = metadata->data.stream_info.channels;
			file->pcmlen     = metadata->data.stream_info.total_samples;;
			file->blocksize  = file->channels * 2;
			break;
		default: break;
	}
}

void WaveformFile_Flac::errorCb(const FLAC__SeekableStreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	_JFAud_LogMsg("flac error\n");
}
//}}}

//{{{ Private methods
int WaveformFile_Flac::LoadSamples(PcmBuffer *buf, bool loop)
{
	int bytes;
	unsigned int totalsamples = buf->GetMaxSamples(), readsamples = 0, samps;
	char *sp = (char *)buf->GetData();

	while (totalsamples > 0) {
		if (pcmleft == 0) {
			if (FLAC__seekable_stream_decoder_get_state(ssd) ==
					FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM) {
				if (loop) {
					if (!FLAC__seekable_stream_decoder_seek_absolute(ssd, 0))
						break;	// failed to seek, so end
					continue;
				} else break;
			} else if (!FLAC__seekable_stream_decoder_process_single(ssd))
				return -1;	// failed decoding, so fail
		} else {
			samps = pcmleft;
			if (samps > totalsamples) samps = totalsamples;

			memcpy(sp, pcm, samps * blocksize);
			pcmleft -= samps;
			if (pcmleft > 0)
				memmove(pcm, (char*)pcm + samps * blocksize, pcmleft * blocksize);
			sp           += samps * blocksize;
			readsamples  += samps;
			totalsamples -= samps;
		}
	}

	buf->SetNumSamples(readsamples);

	return 0;
}
//}}}

//{{{ Public methods
WaveformFile_Flac::WaveformFile_Flac(JFAudFile *tfh)
	: fh(NULL), isvalid(false), isusable(false),
	  ssd(NULL), filelen(0), pcmlen(0),
	  pcmleft(0), pcmalloc(0), pcm(NULL),
	  samplerate(0), channels(0), blocksize(0)
{
	char sig[4];
	FLAC__SeekableStreamDecoderState ssdstate;

	if (!tfh) return;
#if !LINKFLAC
	if (!lib) return;
#endif
	fh = tfh;

	filelen = fh->Length();
	fh->Rewind();
	
	if (fh->Read(4, sig) != 4) return;
	if (sig[0] != 'f' || sig[1] != 'L' || sig[2] != 'a' || sig[3] != 'C') return;
	fh->Rewind();
	isvalid = true;

	ssd = FLAC__seekable_stream_decoder_new();
	if (!ssd) return;

	if (!FLAC__seekable_stream_decoder_set_client_data(ssd, (void*)this) ||
	    !FLAC__seekable_stream_decoder_set_read_callback(ssd, readCb) ||
	    !FLAC__seekable_stream_decoder_set_write_callback(ssd, writeCb) ||
	    !FLAC__seekable_stream_decoder_set_seek_callback(ssd, seekCb) ||
	    !FLAC__seekable_stream_decoder_set_tell_callback(ssd, tellCb) ||
	    !FLAC__seekable_stream_decoder_set_length_callback(ssd, lengthCb) ||
	    !FLAC__seekable_stream_decoder_set_eof_callback(ssd, eofCb) ||
	    !FLAC__seekable_stream_decoder_set_metadata_callback(ssd, metadataCb) ||
	    !FLAC__seekable_stream_decoder_set_error_callback(ssd, errorCb)) {
		FLAC__seekable_stream_decoder_delete(ssd);
		return;
	}

	ssdstate = FLAC__seekable_stream_decoder_init(ssd);
	if (ssdstate != FLAC__SEEKABLE_STREAM_DECODER_OK) {
		_JFAud_LogMsg("flac error = %s\n", FLAC__SeekableStreamDecoderStateString[ssdstate]);
		return;
	}

	if (!FLAC__seekable_stream_decoder_process_until_end_of_metadata(ssd)) {
		_JFAud_LogMsg("flac error = %s",FLAC__seekable_stream_decoder_get_resolved_state_string(ssd));
		return;
	}

	isusable = true;
}

WaveformFile_Flac::~WaveformFile_Flac()
{
	if (ssd) {
		FLAC__seekable_stream_decoder_finish(ssd);
		FLAC__seekable_stream_decoder_delete(ssd);
	}
	if (pcm) free(pcm);

	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

PcmBuffer *WaveformFile_Flac::ReadSamples(unsigned int nsamps, bool loop)
{
	PcmBuffer *buf;

	if (!isusable) return NULL;

	buf = new PcmBuffer();
	if (!buf) return NULL;

	if (nsamps == 0) {
		nsamps = (unsigned int)pcmlen;
		loop = false;
	}

	if (!buf->Allocate(nsamps, samplerate, channels, 2) ||
	    LoadSamples(buf, loop) < 0 ||
	    buf->GetNumSamples() == 0) {
		delete buf;
		return NULL;
	}

	return buf;
}

SoundFile::InitState WaveformFile_Flac::Init()
{
#if !LINKFLAC
	if (lib) { refcount++; return SoundFile::InitOK; }
	
	_JFAud_LogMsg("Loading " FLACDL "\n");
	lib = new DynamicLibrary(FLACDL);
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

bool WaveformFile_Flac::Uninit()
{
#if !LINKFLAC
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

