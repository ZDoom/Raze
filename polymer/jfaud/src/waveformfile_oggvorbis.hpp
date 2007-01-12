#ifndef __waveformfile_oggvorbis_hpp__
#define __waveformfile_oggvorbis_hpp__

#if USEVORBIS

#include "waveformfile.hpp"

#ifdef __APPLE__
# include <Vorbis/vorbisfile.h>
#else
# ifdef __WATCOMC__
#  define __int16 short
#  define __int32 int
# endif
# include "vorbis/vorbisfile.h"
#endif

class WaveformFile_OggVorbis : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid;

	OggVorbis_File vfile;

	int samplerate;
	int channels;

	int LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_OggVorbis(JFAudFile *);
	virtual ~WaveformFile_OggVorbis();
	virtual bool IsValid() const { return isvalid; }

	static InitState Init();
	static bool Uninit();
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_OGGVORBIS; }
	static const char *GetClassFormatName() { return "OggVorbis"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_OGGVORBIS : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "OggVorbis" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const;
	virtual unsigned int GetPCMLength(void) const;
};

#endif //USEVORBIS

#endif

