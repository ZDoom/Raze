#ifndef __waveformfile_raw_hpp__
#define __waveformfile_raw_hpp__

#include "waveformfile.hpp"

class WaveformFile_Raw : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid;

	int samplerate;
	int channels;
	int bytespersample;
	bool bigendian;

	long datalen, datapos;

	unsigned int TotalSamples() const { return datalen / (channels*bytespersample); }
	int LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_Raw(JFAudFile *, int samplerate, int channels, int bytespersample, bool bigendian);
	virtual ~WaveformFile_Raw();
	virtual bool IsValid() const { return isvalid; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_RAW; }
	static const char *GetClassFormatName() { return "Raw PCM"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_RAW : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "Raw PCM" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isvalid ? (float)TotalSamples() / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return !isvalid ? 0 : TotalSamples(); }
};

#endif

