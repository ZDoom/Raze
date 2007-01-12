#ifndef __waveout_dsound_hpp__
#define __waveout_dsound_hpp__

#include "waveout.hpp"

class WaveOut_DSound : public WaveOut {
private:
	SoftwareMixer *owner;
	bool inited, paused;
	int  samplerate, channels, bits, buflen;

	HWND hwnd;
	LPDIRECTSOUND dsound;
	LPDIRECTSOUNDBUFFER bufprim, bufsec;
	LPDIRECTSOUNDNOTIFY notify;
	HANDLE notifypoints[3], mixthread;	// notifypoint[0] is the shutdown signal for the mix thread
	CRITICAL_SECTION mutex;

	static DWORD WINAPI MixThread(class WaveOut_DSound *myself);
protected:
public:
	WaveOut_DSound(SoftwareMixer *);
	virtual ~WaveOut_DSound();
	
	virtual bool SetWindowHandle(HWND h);
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

