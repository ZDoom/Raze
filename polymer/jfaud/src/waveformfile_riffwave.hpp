#ifndef __waveformfile_riffwave_hpp__
#define __waveformfile_riffwave_hpp__

#include "waveformfile.hpp"

class WaveformFile_RiffWave : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid, isusable;

	int samplerate;
	int channels;
	int bytespersample;

	int datastart, datalen, datapos;

	bool ReadChunkHead(char type[4], int *len);
	bool FindChunk(const char *type, int *start, int *len);
	unsigned int TotalSamples() const { return datalen / (channels*bytespersample); }
	int  LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_RiffWave(JFAudFile *);
	virtual ~WaveformFile_RiffWave();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_RIFFWAV; }
	static const char *GetClassFormatName() { return "RIFF WAVE"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_RIFFWAV : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "RIFF WAVE" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isvalid ? (float)TotalSamples() / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return !isvalid ? 0 : TotalSamples(); }
};

#endif
