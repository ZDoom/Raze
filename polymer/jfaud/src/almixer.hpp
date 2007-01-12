#ifndef __almixer_hpp__
#define __almixer_hpp__

#include "mixer.hpp"

#if defined _WIN32
# include "al.h"
# include "alc.h"
#elif defined __APPLE__
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

class ALMixer;

class ALMixerChannel : public JFAudMixerChannel {
private:
	ALMixer *owner;
	ALuint  source;
	ALuint  buffer[2];
	bool    bufferused[2];

	bool streamed, looped, shouldbeplaying;

	WaveformFile *media;
	bool laststate;

	ALint SourceState(void) const;
	bool  FillBuffer(int n);
	void  Cleanup();

public:
	ALMixerChannel(ALMixer *own, ALuint src);	// should only be called by ALMixer::AcquireChannel()!
	virtual ~ALMixerChannel();

	ALuint   GetALSource(void) const { return source; }	// you'd want to have a very good reason to call this
	ALMixer *GetALMixer(void) const  { return owner; }

	virtual bool SetMedia(WaveformFile *);

	virtual bool Play(void);
	virtual bool Pause(void);
	virtual bool Update(void);
	virtual bool IsPlaying(void) const;
	virtual bool IsPaused(void) const { return SourceState() == AL_PAUSED; }
	virtual bool IsStopped(void) const { ALint a = SourceState(); return a == AL_INITIAL || a == AL_STOPPED; }

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
};

class ALMixerChanList {
	public:
	class ALMixerChanList *next;
	ALMixerChannel *chan;
};
class ALMixer : public JFAudMixer {
private:
	ALCcontext *alctx;
	ALCdevice  *aldev;

	ALMixerChanList *chans;

public:
	ALMixer();
	virtual ~ALMixer();

	virtual bool Init(void);
	virtual bool Uninit(void);
	virtual char **Enumerate(char **def);
	
	virtual bool Open(const char *dev, int frequency, int *maxvoices = NULL);
	virtual bool Close(void);

	virtual JFAudMixerChannel *AcquireChannel(void);
	virtual bool ReleaseChannel(JFAudMixerChannel *);	// NOTE: this deletes its parameter

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
