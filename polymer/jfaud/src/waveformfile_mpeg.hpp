#ifndef __waveformfile_mpeg_hpp__
#define __waveformfile_mpeg_hpp__

#if USEMPADEC

#include "waveformfile.hpp"

#include "mpadec.h"

class WaveformFile_Mpeg : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid;

	enum { NoError = 0, Error = -1, NotMpeg = -2 };
	
	mpadec_t mpa;
	mpadec_info_t streaminfo;
	mp3tag_info_t taginfo;

	// decoder state etc.
	uint8_t inbuf[65536], outbuf[8*1152];
	uint32_t insiz, outsiz, outpos;
	long streamoffs;	// start of mpeg data (ID3v2 is skipped over on init)
	long streamlen;	// length of file following start
	long streampos;	// file position
	int InitDecoder(JFAudFile *fh);
	void UninitDecoder();
	
	int DecodeSome(int bytes, char *p);
	int LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_Mpeg(JFAudFile *);
	virtual ~WaveformFile_Mpeg();
	virtual bool IsValid() const { return isvalid; }

	static InitState Init();
	static bool Uninit();
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_MPEG; }
	static const char *GetClassFormatName() { return "MPEG Audio"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_MPEG : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "MPEG Audio" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const;
	virtual unsigned int GetPCMLength(void) const;
};

#endif //USEMPADEC

#endif


