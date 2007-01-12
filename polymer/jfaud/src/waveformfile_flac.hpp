#ifndef __waveformfile_flac_hpp__
#define __waveformfile_flac_hpp__

// FIXME: not entirely 64bit safe on bloody long files

#if USEFLAC

#include "waveformfile.hpp"
#ifdef __APPLE__
# include <FLAC/seekable_stream_decoder.h>
#else
# include "FLAC/seekable_stream_decoder.h"
#endif

class WaveformFile_Flac : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid, isusable;

	FLAC__SeekableStreamDecoder *ssd;
	FLAC__uint64 filelen, pcmlen;
	unsigned pcmleft, pcmalloc;
	short   *pcm;
	unsigned samplerate, channels, blocksize;

	int LoadSamples(PcmBuffer *buf, bool loop);

	static FLAC__SeekableStreamDecoderReadStatus readCb(
		const FLAC__SeekableStreamDecoder *decoder,
		FLAC__byte buffer[], unsigned *bytes, void *client_data);
	static FLAC__StreamDecoderWriteStatus writeCb(
		const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame,
		const FLAC__int32 *const buffer[], void *client_data);
	static FLAC__SeekableStreamDecoderSeekStatus seekCb(
		const FLAC__SeekableStreamDecoder *decoder,
		FLAC__uint64 absolute_byte_offset, void *client_data);
	static FLAC__SeekableStreamDecoderTellStatus tellCb(
		const FLAC__SeekableStreamDecoder *decoder,
		FLAC__uint64 *absolute_byte_offset, void *client_data);
	static FLAC__SeekableStreamDecoderLengthStatus lengthCb(
		const FLAC__SeekableStreamDecoder *decoder,
		FLAC__uint64 *stream_length, void *client_data);
	static FLAC__bool eofCb(
		const FLAC__SeekableStreamDecoder *decoder, void *client_data);
	static void metadataCb(
		const FLAC__SeekableStreamDecoder *decoder,
		const FLAC__StreamMetadata *metadata, void *client_data);
	static void errorCb(const FLAC__SeekableStreamDecoder *decoder,
		FLAC__StreamDecoderErrorStatus status, void *client_data);

public:
	WaveformFile_Flac(JFAudFile *);
	virtual ~WaveformFile_Flac();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init();
	static bool Uninit();

	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_FLAC; }
	static const char *GetClassFormatName() { return "FLAC"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_FLAC : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "FLAC" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isusable ? (float)pcmlen / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return isusable ? pcmlen : 0; }
};

#endif //USEFLAC

#endif
