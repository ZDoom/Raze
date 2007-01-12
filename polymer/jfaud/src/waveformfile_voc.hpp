#ifndef __waveformfile_voc_hpp__
#define __waveformfile_voc_hpp__

#include "waveformfile.hpp"

class WaveformFile_Voc : public WaveformFile {
private:
	JFAudFile *fh;
	bool isvalid, isusable;

	int samplerate;
	int channels;
	int bytespersample;

	int datastart, datalen, datapos;

	enum {
		blk_terminate,
		blk_sounddata,
		blk_soundcont,
		blk_silence,
		blk_marker,
		blk_asciiz,
		blk_repeat,
		blk_endrepeat,
		blk_extended,
		blk_type9
	};

	enum {
		fmt_pcm8,
		fmt_adpcm4_8,
		fmt_adpcm3_8,
		fmt_adpcm2_8,
		fmt_pcm16,
		fmt_alaw = 6,
		fmt_ulaw,
		fmt_adpcm4_16 = 0x2000
	};
	
	struct {
		char type;
		union {
			struct {
				int len;
				unsigned short samplerate;
				char format;
			} sounddata;
			struct {
				int len;
			} soundcont;
			struct {
				int len;
				unsigned short samplerate;
			} silence;
			struct {
				unsigned short value;
			} marker;
			struct {
				int len;
			} asciiz;
			struct {
				unsigned short count;
			} repeat;
			struct {
				unsigned short samplerate;
				char format;
				char channels;
			} extended;
			struct {
				int len;
				int samplerate;
				char bitspersample;
				char channels;
				short format;
			} type9;
		} info;
	} block;
	int blockleft;

	int ReadBlockHead();
	int ReadBlockData(int bytes, char *ptr);

	unsigned int TotalSamples() const { return datalen / (channels*bytespersample); }
	int  LoadSamples(PcmBuffer *buf, bool loop);

public:
	WaveformFile_Voc(JFAudFile *);
	virtual ~WaveformFile_Voc();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_VOC; }
	static const char *GetClassFormatName() { return "Creative VOC"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_VOC : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "Creative VOC" : NULL; }

	virtual PcmBuffer *ReadSamples(unsigned int nsamps = 0, bool loop = false);
	virtual float GetPlayTime(void) const { return isvalid ? (float)TotalSamples() / (float)samplerate : 0.0; }
	virtual unsigned int GetPCMLength(void) const { return !isvalid ? 0 : TotalSamples(); }
};

#endif
