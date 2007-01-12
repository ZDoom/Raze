#ifndef __nullmixer_hpp__
#define __nullmixer_hpp__

#include "mixer.hpp"

class NullMixer;

class NullMixerChannel : public JFAudMixerChannel {
private:
public:
	NullMixerChannel();
	virtual ~NullMixerChannel();

	virtual bool SetMedia(WaveformFile *);

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
	virtual bool SetRolloff(float rolloff);
	virtual bool SetLoop(bool onf);
	virtual bool SetFollowListener(bool onf);

	virtual float GetGain(void) const;
	virtual float GetPitch(void) const;
	virtual void  GetPosition(float *x, float *y, float *z) const;
	virtual void  GetVelocity(float *x, float *y, float *z) const;
	virtual void  GetDirection(float *x, float *y, float *z) const;
	virtual float GetRefDist(void) const;
	virtual float GetRolloff(void) const;
	virtual bool  GetLoop(void) const;
	virtual bool  GetFollowListener(void) const;
};

class NullMixer : public JFAudMixer {
private:
public:
	NullMixer();
	virtual ~NullMixer();

	virtual JFAudMixerChannel *AcquireChannel(void);
	virtual bool ReleaseChannel(JFAudMixerChannel *);

	virtual bool Update();
	
	virtual bool SetListenerPosition(float x, float y, float z);
	virtual bool SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz);
	virtual bool SetListenerVelocity(float x, float y, float z);
	virtual bool SetListenerGain(float gain);

	virtual void  GetListenerPosition(float *x, float *y, float *z) const;
	virtual void  GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const;
	virtual void  GetListenerVelocity(float *x, float *y, float *z) const;
	virtual float GetListenerGain(void) const;
};

#endif
