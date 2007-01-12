#ifndef __softwaremixer_hpp__
#define __softwaremixer_hpp__

#include "sysdefs.h"
#include "mixer.hpp"
//#include "waveout.hpp"
#include "pcmbuffer.hpp"

class WaveOut;	// SoftwareMixer and WaveOut have a circular reference to each other
class SoftwareMixer;

class SoftwareMixerChannel : public JFAudMixerChannel {
private:
	SoftwareMixer *owner;
	WaveformFile *media;

	PcmBuffer *buffer0, *buffer, *buffer2;
	int oldsamplerate;
	int64_t samplepos;	// low 32b = subsample
	int64_t sampleinc;
	short *bufferp;
	int volumes[2];
	int changed;

		// set to the appropriate mixing function on a buffer change
	void (*mixfunc)(class SoftwareMixerChannel *, int**, int*, int, int);
		// set to the appropriate interpolation function
	int (*filterfunc)(class SoftwareMixerChannel *, int, int);
		// set to the appropriate distance model function
	float (*distfunc)(float, float, float, float);

	enum { Stopped = 0, Paused = 1, Playing = 2 } state;
	float gain, pitch;
	float posx, posy, posz;
	float refdist, maxdist, rolloff;
	bool streamed, loop, follow;
	
	static void  Mix1To1(class SoftwareMixerChannel *, int**, int*, int, int);
	static void  Mix1To2(class SoftwareMixerChannel *, int**, int*, int, int);
	static void  Mix2To1(class SoftwareMixerChannel *, int**, int*, int, int);
	static void  Mix2To2(class SoftwareMixerChannel *, int**, int*, int, int);
	static void  DownMix2To2(class SoftwareMixerChannel *, int**, int*, int, int);

	static int   InterpolateNearest(class SoftwareMixerChannel *, int offset, int stride);
	static int   InterpolateLinear(class SoftwareMixerChannel *, int offset, int stride);
	static int   Interpolate4Point(class SoftwareMixerChannel *, int offset, int stride);

	static float DistanceLinear(float distance, float refdist, float maxdist, float rolloff);
	static float DistanceInverse(float distance, float refdist, float maxdist, float rolloff);

	// functions called by SoftwareMixer since it's our friend
	virtual void MixSome(int *, int);
	virtual bool UpdateProps(void);
	virtual void Cleanup(void);
protected:
public:
	SoftwareMixerChannel(SoftwareMixer *);
	virtual ~SoftwareMixerChannel();
	
	virtual bool SetMedia(WaveformFile *);
	virtual bool SetFilter(Filter which);
	virtual bool SetDistanceModel(DistanceModel which);
	
	virtual bool Play(void);
	virtual bool Pause(void);
	virtual bool Update(void);
	virtual bool IsPlaying(void) const;
	virtual bool IsPaused(void) const;
	virtual bool IsStopped(void) const;
	
	virtual bool SetGain(float gain);
	virtual bool SetPitch(float pitch);
	virtual bool SetPosition(float x, float y, float z);
	virtual bool SetVelocity(float x, float y, float z);
	virtual bool SetDirection(float x, float y, float z);
	virtual bool SetRefDist(float refdist);
	virtual bool SetMaxDist(float maxdist);
	virtual bool SetRolloff(float rolloff);
	virtual bool SetLoop(bool onf);
	virtual bool SetFollowListener(bool onf);
	
	virtual float GetGain(void) const;
	virtual float GetPitch(void) const;
	virtual void  GetPosition(float *x, float *y, float *z) const;
	virtual void  GetVelocity(float *x, float *y, float *z) const;
	virtual void  GetDirection(float *x, float *y, float *z) const;
	virtual float GetRefDist(void) const;
	virtual float GetMaxDist(void) const;
	virtual float GetRolloff(void) const;
	virtual bool  GetLoop(void) const;
	virtual bool  GetFollowListener(void) const;
	
	friend class SoftwareMixer;
};

class SoftwareMixer : public JFAudMixer {
private:
	WaveOut *waveout;
	struct _voicestat {
		SoftwareMixerChannel *chan;
		bool used;
	} *voices;
	int nvoices;
	
	int frequency, bytesamp, chans;
	int *mixbuffer, mixbufferlen, mixbufferused, mixbufferpos;

	// listener properties
	float posx, posy, posz;
	float atx, aty, atz, upx, upy, upz;
	float velx, vely, velz;
	float gain;
	
	virtual void DoMix(void);
protected:
public:
	SoftwareMixer();
	virtual ~SoftwareMixer();
	
	virtual bool Setup(WaveOut *dev, int maxvoices);

	virtual JFAudMixerChannel *AcquireChannel(void);
	virtual bool ReleaseChannel(JFAudMixerChannel *);
	
	virtual bool Update();
	virtual void MixSome(void *, int);	// called by the output device when it's starved
	
	virtual bool SetListenerPosition(float x, float y, float z);
	virtual bool SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz);
	virtual bool SetListenerVelocity(float x, float y, float z);
	virtual bool SetListenerGain(float gain);
	
	virtual void  GetListenerPosition(float *x, float *y, float *z) const;
	virtual void  GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const;
	virtual void  GetListenerVelocity(float *x, float *y, float *z) const;
	virtual float GetListenerGain(void) const;
	
	friend class SoftwareMixerChannel;
};

#endif

