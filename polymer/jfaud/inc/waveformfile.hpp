#ifndef __waveformfile_hpp__
#define __waveformfile_hpp__

#include "soundfile.hpp"
#include "pcmbuffer.hpp"

class WaveformFile : public SoundFile {
public:
	typedef enum {
		FORMAT_UNKNOWN = -1,
		FORMAT_FIRST = 0,
		FORMAT_RAW = 0,
		FORMAT_RIFFWAV,
		FORMAT_AIFF,
		FORMAT_VOC,
		FORMAT_AU,
		FORMAT_OGGVORBIS,
		FORMAT_FLAC,
		FORMAT_MPEG,
		FORMAT_MOD,
		FORMAT_LAST = FORMAT_MOD
	} Format;

	WaveformFile() { }
	virtual ~WaveformFile() { }

	virtual Type GetType() const { return TYPE_WAVEFORM; }
	virtual const char *GetTypeName() const { return "waveform"; }
	virtual Format GetFormat() const = 0;
	virtual const char *GetFormatName() const = 0;

	// Sub-classes should implement these
	//static InitState Init() { return InitDisabled; }
	//static bool Uninit() { return false; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false) = 0;
	virtual float GetPlayTime(void) const = 0;
	virtual unsigned int GetPCMLength(void) const = 0;
		// return 0x7fffffff if length is unknown, which will force streaming
};

bool InitialiseWaveformReaders(void);
void UninitialiseWaveformReaders(void);
WaveformFile *IdentifyWaveformFile(JFAudFile *);

#endif
