#ifndef __mixer_hpp__
#define __mixer_hpp__

#include "waveformfile.hpp"

class JFAudMixerChannel {
private:
protected:
	void (*stopcallback)(int);
	int stopcallbackid;
public:
	typedef enum {
		FilterNearest = 0,
		FilterLinear  = 1,
		Filter4Point  = 2,
	} Filter;

	typedef enum {
		DistanceModelInverse = 0,
		DistanceModelLinear = 1,
	} DistanceModel;

	JFAudMixerChannel();
	virtual ~JFAudMixerChannel();

	virtual bool SetMedia(WaveformFile *) = 0;
		// The waveform file becomes the property of the channel
		// if it was successfully attached, and should be deleted
		// in the destructor or if another file is attached.
	virtual bool SetStopCallback( void (*cb)(int), int id);
		// If this function is passed a non-NULL cb, the function will be
		// called passing 'id' when the sound stops playing.
	virtual bool SetFilter(Filter which);
	virtual bool SetDistanceModel(DistanceModel which);
	
	virtual bool Play(void) = 0;
	virtual bool Pause(void) = 0;
	virtual bool Update(void) = 0;
	virtual bool IsPlaying(void) const = 0;
	virtual bool IsPaused(void) const = 0;
	virtual bool IsStopped(void) const = 0;

	virtual bool SetGain(float gain) = 0;
	virtual bool SetPitch(float pitch) = 0;
	virtual bool SetPosition(float x, float y, float z) = 0;
	virtual bool SetVelocity(float x, float y, float z) = 0;
	virtual bool SetDirection(float x, float y, float z) = 0;
	virtual bool SetRefDist(float refdist) = 0;
	virtual bool SetMaxDist(float maxdist) = 0;
	virtual bool SetRolloff(float rolloff) = 0;
	virtual bool SetLoop(bool onf) = 0;
	virtual bool SetFollowListener(bool onf) = 0;
	
	virtual float GetGain(void) const = 0;
	virtual float GetPitch(void) const = 0;
	virtual void  GetPosition(float *x, float *y, float *z) const = 0;
	virtual void  GetVelocity(float *x, float *y, float *z) const = 0;
	virtual void  GetDirection(float *x, float *y, float *z) const = 0;
	virtual float GetRefDist(void) const = 0;
	virtual float GetMaxDist(void) const = 0;
	virtual float GetRolloff(void) const = 0;
	virtual bool  GetLoop(void) const = 0;
	virtual bool  GetFollowListener(void) const = 0;
};

class JFAudMixer {
private:
protected:
public:
	JFAudMixer() { }
	virtual ~JFAudMixer() { }

	virtual JFAudMixerChannel *AcquireChannel(void) = 0;
	virtual bool ReleaseChannel(JFAudMixerChannel *) = 0;

	virtual bool Update() = 0;

	virtual bool SetListenerPosition(float x, float y, float z) = 0;
	virtual bool SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz) = 0;
	virtual bool SetListenerVelocity(float x, float y, float z) = 0;
	virtual bool SetListenerGain(float gain) = 0;

	virtual void  GetListenerPosition(float *x, float *y, float *z) const = 0;
	virtual void  GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const = 0;
	virtual void  GetListenerVelocity(float *x, float *y, float *z) const = 0;
	virtual float GetListenerGain(void) const = 0;
};

#endif
