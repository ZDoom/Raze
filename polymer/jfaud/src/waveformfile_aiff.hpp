#ifndef __waveformfile_aiff_hpp__
#define __waveformfile_aiff_hpp__

#include "waveformfile.hpp"

class WaveformFile_Aiff : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid, isusable;

	int samplerate;
	int channels;
	int bytespersample;

	int datastart, datalen, datapos;

	bool ReadChunkHead(char type[4], int *len);
	bool FindChunk(const char *type, int *start, int *len);
	int tbyte2int_bigendian(unsigned char *t);
	unsigned int TotalSamples() const { return datalen / (channels*bytespersample); }
	int  LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_Aiff(JFAudFile *);
	virtual ~WaveformFile_Aiff();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_AIFF; }
	static const char *GetClassFormatName() { return "AIFF"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_AIFF : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "AIFF" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isvalid ? (float)TotalSamples() / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return !isvalid ? 0 : TotalSamples(); }
};

#endif
