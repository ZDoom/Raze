#ifndef __waveformfile_au_hpp__
#define __waveformfile_au_hpp__

#include "waveformfile.hpp"

class WaveformFile_Au : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid, isusable;

	int samplerate;
	int channels;
	int bytespersample;

	long datastart, datalen, datapos;

	enum {
		fmt_pcm8   = 2,
		fmt_pcm16  = 3,
		fmt_pcm24  = 4,
		fmt_pcm32  = 5,
		fmt_ieee32 = 6,
		fmt_ieee64 = 7
	};

	unsigned int TotalSamples() const { return datalen / (channels*bytespersample); }
	int  LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_Au(JFAudFile *);
	virtual ~WaveformFile_Au();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_AU; }
	static const char *GetClassFormatName() { return "Sun AU"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_AU : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "Sun AU" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isvalid ? (float)TotalSamples() / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return !isvalid ? 0 : TotalSamples(); }
};

#endif
