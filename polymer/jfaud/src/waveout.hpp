#ifndef __waveout_hpp__
#define __waveout_hpp__

//#include "softwaremixer.hpp"

class SoftwareMixer;

class WaveOut {
private:
protected:
public:
	WaveOut(SoftwareMixer *);
	virtual ~WaveOut();
	
	virtual bool Init(int samplerate, int channels, int bits) = 0;
	virtual int  GetSampleRate(void) const = 0;
	virtual int  GetChannels(void) const = 0;
	virtual int  GetBitsPerSample(void) const = 0;
	virtual int  GetMixBufferLen(void) const = 0;		// returns bytes
	
	virtual bool Pause(bool onf) = 0;
	virtual bool Lock(void) = 0;
	virtual bool Unlock(void) = 0;
	virtual bool SetVolume(float vol) = 0;
	virtual float GetVolume(void) const = 0;
};

#endif

