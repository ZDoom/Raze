#ifndef __waveout_sdl_hpp__
#define __waveout_sdl_hpp__

#include "waveout.hpp"

class WaveOut_SDL : public WaveOut {
private:
	SoftwareMixer *owner;
	bool inited;
	int  samplerate, channels, bits, buflen;
	static void sdlcallback(class WaveOut_SDL *, void *, int);
protected:
public:
	WaveOut_SDL(SoftwareMixer *);
	virtual ~WaveOut_SDL();
	
	virtual bool Init(int samplerate, int channels, int bits);
	virtual int  GetSampleRate(void) const;
	virtual int  GetChannels(void) const;
	virtual int  GetBitsPerSample(void) const;
	virtual int  GetMixBufferLen(void) const;
	
	virtual bool Pause(bool onf);
	virtual bool Lock(void);
	virtual bool Unlock(void);
	virtual bool SetVolume(float vol);
	virtual float GetVolume(void) const;
};

#endif

