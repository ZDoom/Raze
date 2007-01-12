#define JFAUD_INTERNAL
#if USEAL

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
# include "watcomhax/cstring"
#else
# include <cstdio>
# include <cstring>
#endif
#include "almixer.hpp"
#include "log.h"

#if defined _WIN32
# include "al.h"
#elif defined __APPLE__
# include <OpenAL/al.h>
#else
# include <AL/al.h>
#endif

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#define STREAMCHUNK	22050		// samples. 1sec @ 22.05KHz

//{{{ Dynamic loading
#if !LINKAL
#include "dynlib.hpp"
#define GETDLSYM(sym,sig) do { if (( libsyms.sym = (sig) lib->Get( (const char *)#sym )) == NULL) return getdlsymerr( #sym ); } while (0)
#define GETDLSYMSOFT(sym,sig) do { libsyms.sym = (sig) lib->Get( (const char *)#sym ); } while (0)
static bool getdlsymerr(const char *sym) { _JFAud_LogMsg("  Symbol %s not found. OpenAL disabled.\n", sym); return false; }
static DynamicLibrary *lib = NULL;
static int refcount = 0;

static struct {
	ALboolean    (ALCAPIENTRY *alcIsExtensionPresent)(ALCdevice *device, ALubyte *extName);
	ALCdevice *  (ALCAPIENTRY *alcOpenDevice)(ALubyte *tokstr);
	ALCcontext * (ALCAPIENTRY *alcCreateContext)(ALCdevice *dev, ALint* attrlist);
	ALubyte*     (ALCAPIENTRY *alcGetString)(ALCdevice *deviceHandle, ALCenum token);
	ALCenum      (ALCAPIENTRY *alcGetError)(ALCdevice *dev);
	ALboolean    (ALCAPIENTRY *alcMakeContextCurrent)(ALCcontext *context);
	ALvoid       (ALCAPIENTRY *alcDestroyContext)(ALCcontext* context);
	void         (ALCAPIENTRY *alcCloseDevice)(ALCdevice *dev);

	ALboolean    (ALAPIENTRY  *alIsExtensionPresent)(const ALubyte* extname);
	ALubyte*     (ALAPIENTRY  *alGetString)(ALenum param);
	ALenum       (ALAPIENTRY  *alGetError)(ALvoid);
	void         (ALAPIENTRY  *alGenSources)(ALsizei n, ALuint* sources);
	void         (ALAPIENTRY  *alGenBuffers)(ALsizei n, ALuint* buffers);
	void         (ALAPIENTRY  *alDeleteSources)(ALsizei n, ALuint* sources);
	void         (ALAPIENTRY  *alDeleteBuffers)(ALsizei n, ALuint* buffers);
	void         (ALAPIENTRY  *alListenerf)(ALenum pname, ALfloat param);
	void         (ALAPIENTRY  *alListener3f)(ALenum pname, ALfloat f1, ALfloat f2, ALfloat f3);
	void         (ALAPIENTRY  *alListenerfv)(ALenum pname, ALfloat* param);
	void         (ALAPIENTRY  *alGetListenerf)( ALenum pname, ALfloat* value );
	void         (ALAPIENTRY  *alGetListenerfv)( ALenum pname, ALfloat* values );
	void         (ALAPIENTRY  *alSourcePlay)(ALuint sid);
	void         (ALAPIENTRY  *alSourcePause)(ALuint sid);
	void         (ALAPIENTRY  *alSourceStop)(ALuint sid);
	void         (ALAPIENTRY  *alSourceRewind)(ALuint sid);
	void         (ALAPIENTRY  *alSourcei)(ALuint sid, ALenum param, ALint value);
	void         (ALAPIENTRY  *alSourcef)(ALuint sid, ALenum param, ALfloat value);
	void         (ALAPIENTRY  *alSource3f)(ALuint sid, ALenum param, ALfloat f1, ALfloat f2, ALfloat f3);
	void         (ALAPIENTRY  *alGetSourcei)(ALuint sid, ALenum pname, ALint* value);
	void         (ALAPIENTRY  *alGetSourcef)(ALuint sid, ALenum pname, ALfloat* value);
	void         (ALAPIENTRY  *alGetSourcefv)(ALuint sid, ALenum pname, ALfloat* values );
	void         (ALAPIENTRY  *alSourceQueueBuffers)(ALuint sid, ALsizei numEntries, ALuint *bids);
	void         (ALAPIENTRY  *alSourceUnqueueBuffers)(ALuint sid, ALsizei numEntries, ALuint *bids);
	void         (ALAPIENTRY  *alBufferData)(ALuint buffer, ALenum format, ALvoid* data, ALsizei size, ALsizei freq);
} libsyms;

static bool getallsyms()
{
	GETDLSYMSOFT(alcIsExtensionPresent, ALboolean (ALCAPIENTRY *)(ALCdevice *, ALubyte *));
	GETDLSYM(alcOpenDevice,          ALCdevice *  (ALCAPIENTRY *)(ALubyte *));
	GETDLSYM(alcCreateContext,       ALCcontext * (ALCAPIENTRY *)(ALCdevice *, ALint* ));
	GETDLSYM(alcGetString,           ALubyte*     (ALCAPIENTRY *)(ALCdevice *, ALCenum ));
	GETDLSYM(alcGetError,            ALCenum      (ALCAPIENTRY *)(ALCdevice *));
	GETDLSYM(alcMakeContextCurrent,  ALboolean    (ALCAPIENTRY *)(ALCcontext *));
	GETDLSYM(alcDestroyContext,      ALvoid       (ALCAPIENTRY *)(ALCcontext *));
	GETDLSYM(alcCloseDevice,         void         (ALCAPIENTRY *)(ALCdevice *));
	GETDLSYMSOFT(alIsExtensionPresent, ALboolean  (ALAPIENTRY  *)(const ALubyte*));
	GETDLSYM(alGetString,            ALubyte*     (ALAPIENTRY  *)(ALenum ));
	GETDLSYM(alGetError,             ALenum       (ALAPIENTRY  *)(ALvoid));
	GETDLSYM(alGenSources,           void         (ALAPIENTRY  *)(ALsizei , ALuint* ));
	GETDLSYM(alGenBuffers,           void         (ALAPIENTRY  *)(ALsizei , ALuint* ));
	GETDLSYM(alDeleteSources,        void         (ALAPIENTRY  *)(ALsizei , ALuint* ));
	GETDLSYM(alDeleteBuffers,        void         (ALAPIENTRY  *)(ALsizei , ALuint* ));
	GETDLSYM(alListenerf,            void         (ALAPIENTRY  *)(ALenum , ALfloat ));
	GETDLSYM(alListener3f,           void         (ALAPIENTRY  *)(ALenum , ALfloat , ALfloat , ALfloat ));
	GETDLSYM(alListenerfv,           void         (ALAPIENTRY  *)(ALenum , ALfloat* ));
	GETDLSYM(alGetListenerf,         void         (ALAPIENTRY  *)(ALenum pname, ALfloat* value ));
	GETDLSYM(alGetListenerfv,        void         (ALAPIENTRY  *)(ALenum pname, ALfloat* values ));
	GETDLSYM(alSourcePlay,           void         (ALAPIENTRY  *)(ALuint ));
	GETDLSYM(alSourcePause,          void         (ALAPIENTRY  *)(ALuint ));
	GETDLSYM(alSourceStop,           void         (ALAPIENTRY  *)(ALuint ));
	GETDLSYM(alSourceRewind,         void         (ALAPIENTRY  *)(ALuint ));
	GETDLSYM(alSourcei,              void         (ALAPIENTRY  *)(ALuint , ALenum , ALint ));
	GETDLSYM(alSourcef,              void         (ALAPIENTRY  *)(ALuint , ALenum , ALfloat ));
	GETDLSYM(alSource3f,             void         (ALAPIENTRY  *)(ALuint , ALenum , ALfloat, ALfloat, ALfloat ));
	GETDLSYM(alGetSourcei,           void         (ALAPIENTRY  *)(ALuint , ALenum , ALint* ));
	GETDLSYM(alGetSourcef,           void         (ALAPIENTRY  *)(ALuint , ALenum , ALfloat* ));
	GETDLSYM(alGetSourcefv,          void         (ALAPIENTRY  *)(ALuint , ALenum , ALfloat* ));
	GETDLSYM(alSourceQueueBuffers,   void         (ALAPIENTRY  *)(ALuint , ALsizei , ALuint *));
	GETDLSYM(alSourceUnqueueBuffers, void         (ALAPIENTRY  *)(ALuint , ALsizei , ALuint *));
	GETDLSYM(alBufferData,           void         (ALAPIENTRY  *)(ALuint , ALenum , ALvoid* , ALsizei , ALsizei ));
	return true;
}

#define alcIsExtensionPresent libsyms.alcIsExtensionPresent
#define alcOpenDevice libsyms.alcOpenDevice
#define alcCreateContext libsyms.alcCreateContext
#define alcGetString libsyms.alcGetString
#define alcGetError libsyms.alcGetError
#define alcMakeContextCurrent libsyms.alcMakeContextCurrent
#define alcDestroyContext libsyms.alcDestroyContext
#define alcCloseDevice libsyms.alcCloseDevice
#define alIsExtensionPresent libsyms.alIsExtensionPresent
#define alGetString libsyms.alGetString
#define alGetError libsyms.alGetError
#define alGenSources libsyms.alGenSources
#define alGenBuffers libsyms.alGenBuffers
#define alDeleteSources libsyms.alDeleteSources
#define alDeleteBuffers libsyms.alDeleteBuffers
#define alListenerf libsyms.alListenerf
#define alListener3f libsyms.alListener3f
#define alListenerfv libsyms.alListenerfv
#define alGetListenerf libsyms.alGetListenerf
#define alGetListenerfv libsyms.alGetListenerfv
#define alSourcePlay libsyms.alSourcePlay
#define alSourcePause libsyms.alSourcePause
#define alSourceStop libsyms.alSourceStop
#define alSourceRewind libsyms.alSourceRewind
#define alSourcei libsyms.alSourcei
#define alSourcef libsyms.alSourcef
#define alSource3f libsyms.alSource3f
#define alGetSourcei libsyms.alGetSourcei
#define alGetSourcef libsyms.alGetSourcef
#define alGetSourcefv libsyms.alGetSourcefv
#define alSourceQueueBuffers libsyms.alSourceQueueBuffers
#define alSourceUnqueueBuffers libsyms.alSourceUnqueueBuffers
#define alBufferData libsyms.alBufferData

#endif
//}}}

//{{{ ALMixerChannel
ALMixerChannel::ALMixerChannel(ALMixer *own, ALuint src)
	: owner(own), source(src),
	  streamed(false), looped(false), shouldbeplaying(false), laststate(false),
	  media(NULL),
	  JFAudMixerChannel()
{
	buffer[0]     = buffer[1]     = 0;
	bufferused[0] = bufferused[1] = false;
}

ALMixerChannel::~ALMixerChannel()
{
#ifdef DEBUG
	_JFAud_LogMsg("ALMixerChannel::~ALMixerChannel: destructing %p\n",this);
#endif
	
	Cleanup();
	alDeleteSources(1,&source);
	
	if (media) delete media;
}

ALint ALMixerChannel::SourceState(void) const
{
	ALint state;

	alGetError();
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	return alGetError() == AL_NO_ERROR ? state : AL_FALSE;
}

bool ALMixerChannel::FillBuffer(int n)
{
	PcmBuffer *buf;
	int ch,by;
	ALenum format[2][2] = {
		{ AL_FORMAT_MONO8,   AL_FORMAT_MONO16   },
		{ AL_FORMAT_STEREO8, AL_FORMAT_STEREO16 }
	};

	if (!media) return false;

	buf = media->ReadSamples(streamed ? STREAMCHUNK : 0, looped);
	if (!buf) return false;

#ifdef DEBUG
	_JFAud_LogMsg("ALMixerChannel::FillBuffer: read %d samples\n", buf->GetNumSamples());
#endif

	switch (buf->GetNumChannels()) {
		case 1: ch = 0; break;
		case 2: ch = 1; break;
		default: delete buf; return false;
	}
	switch (buf->GetBytesPerSample()) {
		case 1: by = 0; break;
		case 2: by = 1; break;
		default: delete buf; return false;
	}

	alGetError();
	alBufferData(buffer[n], format[ch][by], buf->GetData(),
		buf->GetNumSamples()*buf->GetBlockSize(),
		buf->GetSampleRate());

	delete buf;

	if (alGetError() == AL_NO_ERROR) {
		if (streamed) {
			alSourceQueueBuffers(source, 1, &buffer[n]);
		} else {
			alSourcei(source, AL_BUFFER, buffer[n]);
		}
		if (alGetError() == AL_NO_ERROR) {
			bufferused[n] = true;
#ifdef DEBUG
			_JFAud_LogMsg("ALMixerChannel::FillBuffer: queued buffer on %p\n",this);
#endif
		}
	}

	return bufferused[n];
}

void ALMixerChannel::Cleanup()
{
	ALuint b;

	if (stopcallback && !IsPlaying() && laststate) {
		laststate = false;
		stopcallback(stopcallbackid);
	}
	
	alSourceStop(source);

	// unqueue and delete any buffers
	alSourcei(source, AL_BUFFER, 0);
	if (buffer[0]) alDeleteBuffers(1,&buffer[0]);
	if (buffer[1]) alDeleteBuffers(1,&buffer[1]);
	buffer[0]     = buffer[1]     = 0;
	bufferused[0] = bufferused[1] = false;
}

bool ALMixerChannel::SetMedia(WaveformFile *file)
{
	if (!file) return false;

	Cleanup();
	if (media) {
		// clean up after our predecessor
		delete media;
		media = NULL;
	}

	media = file;

	looped = false;
	alSourcei(source, AL_LOOPING, AL_FALSE);

	alGetError();
	if (file->GetPCMLength() >= (2*STREAMCHUNK)) {
		streamed = true;
		alGenBuffers(2, &buffer[0]);
	} else {
		streamed = false;
		alGenBuffers(1, &buffer[0]);
	}
	if (alGetError() != AL_NO_ERROR) { media = NULL; return false; }
	if (!FillBuffer(0)) { media = NULL; return false; }

	return true;
}

bool ALMixerChannel::Play(void)
{
	alGetError();
	alSourcePlay(source);
	shouldbeplaying = (alGetError() == AL_NO_ERROR);
	laststate = shouldbeplaying;
	return shouldbeplaying;
}

bool ALMixerChannel::Update(void)
{
	ALint nq = 0;
	ALuint b;
	bool retv = true;

	if (!streamed) {
		if (stopcallback && !IsPlaying() && laststate) {
			laststate = false;
			stopcallback(stopcallbackid);
		}
		return true;
	}

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &nq);
	for (; nq>0; nq--) {
		alSourceUnqueueBuffers(source, 1, &b);

		if (b == buffer[0]) bufferused[0] = false;
		else if (b == buffer[1]) bufferused[1] = false;
		//else  !?!
	}

	if (!bufferused[0] && !FillBuffer(0)) retv = false;
	if (!bufferused[1] && !FillBuffer(1)) retv = false;

	if (!retv) shouldbeplaying = false;	// end of file or error
	if (shouldbeplaying && SourceState() != AL_PLAYING)	// underran, so the source stopped
		alSourcePlay(source);

	if (stopcallback && !IsPlaying() && laststate) {
		laststate = false;
		stopcallback(stopcallbackid);
	}

	return retv;
}

bool ALMixerChannel::IsPlaying(void) const
{
	if (!streamed) return SourceState() == AL_PLAYING;

	if (shouldbeplaying) return true;
	return SourceState() == AL_PLAYING;
}

bool ALMixerChannel::Pause(void)
{
	alGetError();
	alSourcePause(source);
	shouldbeplaying = laststate = false;
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetGain(float gain)
{
	alGetError();
	alSourcef(source, AL_GAIN, gain);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetPitch(float pitch)
{
	if (pitch > 2.0) pitch = 2.0;
	alGetError();
	alSourcef(source, AL_PITCH, pitch);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetPosition(float x, float y, float z)
{
	alGetError();
	alSource3f(source, AL_POSITION, x,y,z);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetVelocity(float x, float y, float z)
{
	alGetError();
	alSource3f(source, AL_VELOCITY, x,y,z);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetDirection(float x, float y, float z)
{
	alGetError();
	alSource3f(source, AL_DIRECTION, x,y,z);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetRefDist(float refdist)
{
	alGetError();
	alSourcef(source, AL_REFERENCE_DISTANCE, refdist);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetMaxDist(float maxdist)
{
	alGetError();
	alSourcef(source, AL_MAX_DISTANCE, maxdist);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetRolloff(float rolloff)
{
	alGetError();
	alSourcef(source, AL_ROLLOFF_FACTOR, rolloff);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetLoop(bool onf)
{
	looped = onf;
	if (streamed) return true;	// streamed sources don't loop in AL. it's implicit in the feeding

	alGetError();
	alSourcei(source, AL_LOOPING, onf ? AL_TRUE : AL_FALSE);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixerChannel::SetFollowListener(bool onf)
{
	alGetError();
	alSourcei(source, AL_SOURCE_RELATIVE, onf ? AL_TRUE : AL_FALSE);
	return alGetError() == AL_NO_ERROR;
}

float ALMixerChannel::GetGain(void) const
{
	ALfloat f;
	alGetSourcef(source, AL_GAIN, &f);
	return f;
}

float ALMixerChannel::GetPitch(void) const
{
	ALfloat f;
	alGetSourcef(source, AL_PITCH, &f);
	return f;
}

void ALMixerChannel::GetPosition(float *x, float *y, float *z) const
{
	ALfloat v[3];
	alGetSourcefv(source, AL_POSITION, v);
	*x = v[0]; *y = v[1]; *z = v[2];
}

void ALMixerChannel::GetVelocity(float *x, float *y, float *z) const
{
	ALfloat v[3];
	alGetSourcefv(source, AL_VELOCITY, v);
	*x = v[0]; *y = v[1]; *z = v[2];
}

void ALMixerChannel::GetDirection(float *x, float *y, float *z) const
{
	ALfloat v[3];
	alGetSourcefv(source, AL_DIRECTION, v);
	*x = v[0]; *y = v[1]; *z = v[2];
}

float ALMixerChannel::GetRefDist(void) const
{
	ALfloat f;
	alGetSourcef(source, AL_REFERENCE_DISTANCE, &f);
	return f;
}

float ALMixerChannel::GetMaxDist(void) const
{
	ALfloat f;
	alGetSourcef(source, AL_MAX_DISTANCE, &f);
	return f;
}

float ALMixerChannel::GetRolloff(void) const
{
	ALfloat f;
	alGetSourcef(source, AL_ROLLOFF_FACTOR, &f);
	return f;
}

bool ALMixerChannel::GetLoop(void) const
{
	ALint i;
	alGetSourcei(source, AL_LOOPING, &i);
	return i == AL_TRUE;
}

bool ALMixerChannel::GetFollowListener(void) const
{
	ALint i;
	alGetSourcei(source, AL_SOURCE_RELATIVE, &i);
	return i == AL_TRUE;
}

//}}}
//{{{ ALMixer
bool ALMixer::SetListenerPosition(float x, float y, float z)
{
	alGetError();
	alListener3f(AL_POSITION, x,y,z);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixer::SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz)
{
	ALfloat v[6] = { atx,aty,atz,upx,upy,upz };
	alGetError();
	alListenerfv(AL_ORIENTATION, v);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixer::SetListenerVelocity(float x, float y, float z)
{
	alGetError();
	alListener3f(AL_VELOCITY, x,y,z);
	return alGetError() == AL_NO_ERROR;
}

bool ALMixer::SetListenerGain(float gain)
{
	alGetError();
	alListenerf(AL_GAIN, gain);
	return alGetError() == AL_NO_ERROR;
}

void ALMixer::GetListenerPosition(float *x, float *y, float *z) const
{
	ALfloat v[3];
	alGetListenerfv(AL_POSITION, v);
	*x = v[0]; *z = -v[1]; *y = v[2];
}

void ALMixer::GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const
{
	ALfloat v[6];
	alGetListenerfv(AL_ORIENTATION, v);
	*atx = v[0]; *aty = v[1]; *atz = v[2];
	*upx = v[3]; *upy = v[4]; *upz = v[5];
}

void ALMixer::GetListenerVelocity(float *x, float *y, float *z) const
{
	ALfloat v[3];
	alGetListenerfv(AL_VELOCITY, v);
	*x = v[0]; *y = v[1]; *z = v[2];
}

float ALMixer::GetListenerGain(void) const
{
	ALfloat f;
	alGetListenerf(AL_GAIN, &f);
	return f;
}


JFAudMixerChannel *ALMixer::AcquireChannel(void)
{
	ALuint src;
	ALMixerChannel *ch;
	ALMixerChanList *chanlist;

	alGetError();
	alGenSources(1,&src);
	if (alGetError() != AL_NO_ERROR) return NULL;

	ch = new ALMixerChannel(this, src);
	if (!ch) {
		alDeleteSources(1,&src);
		return NULL;
	}

	chanlist = new ALMixerChanList;
	if (chanlist) {
		chanlist->next = chans;
		chanlist->chan = ch;
		chans = chanlist;
	}
	
	return ch;
}

bool ALMixer::ReleaseChannel(JFAudMixerChannel *ch)
{
	ALMixerChannel *alch = static_cast<ALMixerChannel*>(ch);
	ALMixerChanList *chanlist, *chanbak;
	
	if (!ch) return false;
	if (alch->GetALMixer() != this) return false;	// not one of our channels
	
	// Remove the channel from the refresh list
	chanlist = chans;
	chanbak = NULL;
	while (chanlist) {
		if (chanlist->chan != alch) {
			chanbak = chanlist;
			chanlist = chanlist->next;
			continue;
		}
		
		if (!chanbak) {
			// the head of the list dies
			chans = chans->next;
			delete chanlist;
		} else {
			// a link dies
			chanbak->next = chanlist->next;
			delete chanlist;
		}
		delete alch;
		break;
	}

	return false;
}

bool ALMixer::Update()
{
	ALMixerChanList *chanlist, *next;

	// Walk the refresh list and poke each channel
	chanlist = chans;
	while (chanlist) {
		// save the next pointer before updating the channel because the
		// link we're on might evaporate if a stop callback triggered in
		// Update() releases the channel
		next = chanlist->next;
		
		chanlist->chan->Update();
		chanlist = next;
	}
	return true;
}

#include <stdlib.h>

ALMixer::ALMixer()
	: alctx(NULL), aldev(NULL), chans(NULL)
{
}

ALMixer::~ALMixer()
{
	Close();
	Uninit();
}

bool ALMixer::Init(void)
{
#if !LINKAL
	if (lib) { refcount++; return true; }
	
	_JFAud_LogMsg("Loading " ALDL "\n");
	lib = new DynamicLibrary(ALDL);
	if (!lib) return false;
	if (!lib->IsOpen()) {
		delete lib;
		lib = NULL;
		return false;
	}

	if (getallsyms()) refcount = 1;
	else {
		delete lib;
		lib = NULL;
		return false;
	}
#endif
	return true;
}

bool ALMixer::Uninit(void)
{
#if !LINKAL
	if (refcount > 1) { refcount--; return true; }
	if (refcount == 0 || !lib) return false;
	refcount = 0;
	delete lib;
	lib = NULL;
	memset(&libsyms,0,sizeof(libsyms));
#endif
	return true;
}

char **ALMixer::Enumerate(char **def)
{
	char *defdev, *devlist, *devp, *p;
	char **rl, **rp;
	int numdevs, devstrlen, i;

#if !LINKAL
	if (!alcIsExtensionPresent) return NULL;
#endif
	if (alcIsExtensionPresent(NULL,(ALubyte*)"ALC_ENUMERATION_EXT") != AL_TRUE) return NULL;

	defdev = (char*)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	devlist = (char*)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	if (!devlist) return NULL;

	for (numdevs = devstrlen = 0, devp = devlist; devp[0]; ) {
		i = strlen(devp)+1;
		devstrlen += i;
		devp += i;
		numdevs++;
	}
	if (!numdevs) return NULL;

	rl = (char**)calloc(1, sizeof(char*)*(numdevs+1)+devstrlen);
	if (!rl) return NULL;
	
	p = (char*)rl + sizeof(char*)*(numdevs+1);
	for (rp = rl, devp = devlist; devp[0]; devp += strlen(devp)+1) {
		*rp = p;
		strcpy(p, devp);
		p += strlen(devp) + 1;
		if (def && defdev && !strcmp(defdev, devp)) *def = *rp;
		rp++;
	}

	return rl;
}

bool ALMixer::Open(const char *dev, int frequency, int *maxvoices)
{
	ALint ctxattrs[] = {
		ALC_SYNC, AL_FALSE,
		ALC_FREQUENCY, frequency,
		ALC_INVALID
	};
	ALubyte *s;

	if (alctx) return true;

	aldev = alcOpenDevice((ALubyte*)dev);
	if (!aldev) return false;

	alctx = alcCreateContext(aldev, ctxattrs);
	if (!alctx) {
		alcCloseDevice(aldev);
		aldev = NULL;
		return false;
	}

	alcGetError(aldev);
	alcMakeContextCurrent(alctx);
	if (alcGetError(aldev) != ALC_NO_ERROR) {
		alcMakeContextCurrent(NULL);
		alcDestroyContext(alctx);
		alcCloseDevice(aldev);
		alctx = NULL;
		aldev = NULL;
		return false;
	}

	// If the user passes in a maximum voice count, adjust it to the maximum
	// we can actually take.
	if (maxvoices && *maxvoices > 0) {
		int i;
		ALuint *v = new ALuint[*maxvoices];

		alGetError();
		for (i=*maxvoices; i>0; i--) {
			alGenSources(i, v);
			if (alGetError() == AL_NO_ERROR) break;
		}
		if (i > 0) alDeleteSources(i, v);
		*maxvoices = i;

		delete v;
	}

	_JFAud_LogMsg("OpenAL device information\n");
	s = alGetString(AL_VENDOR);     _JFAud_LogMsg("   Vendor:     %s\n", s);
	s = alGetString(AL_VERSION);    _JFAud_LogMsg("   Version:    %s\n", s);
	s = alGetString(AL_RENDERER);   _JFAud_LogMsg("   Renderer:   %s\n", s);
	s = alGetString(AL_EXTENSIONS); _JFAud_LogMsg("   Extensions: %s\n", s);

	return true;
}

bool ALMixer::Close(void)
{
	if (!alctx) return true;

	alcMakeContextCurrent(NULL);
	alcDestroyContext(alctx);
	alcCloseDevice(aldev);
	alctx = NULL;
	aldev = NULL;

	return true;
}

//}}}

#endif

// vim:fdm=marker:
