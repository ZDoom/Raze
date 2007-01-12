#ifndef __pcmbuffer_hpp__
#define __pcmbuffer_hpp__

#include "buffer.hpp"

class PcmBuffer : public Buffer {
private:
	unsigned int numsamples, maxsamples;
	void *data;
	unsigned int samplerate;
	unsigned int channels;
	unsigned int bytespersample;
protected:
public:
	PcmBuffer() : data((void*)0) { }
	virtual ~PcmBuffer();

	virtual Type GetType() const { return TYPE_PCM; }

	unsigned int GetBlockSize() const        { return channels*bytespersample; }
	unsigned int GetMaxSamples() const       { return maxsamples; }
	void *       GetData() const             { return data; }
	void *       GetDataAt(int sample) const { return (void*)((char*)data + (GetBlockSize()*(sample%numsamples))); }

	unsigned int GetNumSamples() const     { return numsamples; }
	unsigned int GetSampleRate() const     { return samplerate; }
	unsigned int GetNumChannels() const    { return channels; }
	unsigned int GetBytesPerSample() const { return bytespersample; }
	unsigned int GetBitsPerSample() const  { return bytespersample * 8; }
	
	void SetNumSamples(unsigned int s) { if (s > maxsamples) s = maxsamples; numsamples = s; }
	
	bool Allocate(unsigned int nsamp, unsigned int srate, unsigned int chans, unsigned int bps);
	bool ConvertToNByte(int n);
};

#endif
