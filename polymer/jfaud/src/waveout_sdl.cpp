#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "log.h"
#include "waveout_sdl.hpp"
#include "softwaremixer.hpp"
#if defined __APPLE__
# include <SDL/SDL.h>
#else
# include "SDL.h"
#endif

//using namespace std;

#define CallbackRate 40

void WaveOut_SDL::sdlcallback(WaveOut_SDL *dev, void *buffer, int len)
{
	dev->owner->MixSome(buffer, len);
}

WaveOut_SDL::WaveOut_SDL(SoftwareMixer *mix)
	: owner(mix), inited(false),
	  WaveOut(mix),
	  samplerate(0), channels(0), bits(0), buflen(0)
{
}

WaveOut_SDL::~WaveOut_SDL()
{
	if (inited) SDL_CloseAudio();
}

bool WaveOut_SDL::Init(int samplerate, int channels, int bits)
{
	SDL_AudioSpec desired, achieved;
	int targetlen, buflen, i;
	
	if (inited) return false;
	
	if (!SDL_WasInit(SDL_INIT_AUDIO)) return false;
	
	// find the nearest power-of-two buffer size that will give
	// at least a callback rate of CallbackRate
	targetlen = samplerate / CallbackRate;
	for (i=1; i<=targetlen; i+=i) buflen = i;

	desired.freq     = samplerate;
	switch (bits) {
		case 8: desired.format = AUDIO_S8; break;
		case 16: desired.format = AUDIO_S16SYS; break;	// the mixer shall mix in native byte order
		default: return false;
	}
	desired.channels = channels;
	desired.samples  = buflen;
	desired.callback = (void(*)(void*,Uint8*,int))sdlcallback;
	desired.userdata = (void*)this;
	
	_JFAud_LogMsg("WaveOut_SDL::Init(): asked for %dHz %d-format %d-channel with %d-sample buffer\n",
				  samplerate, AUDIO_S16SYS, channels, buflen);
	
	if (SDL_OpenAudio(&desired, &achieved)) return false;	// failure
	
	_JFAud_LogMsg("WaveOut_SDL::Init(): got %dHz %d-format %d-channel with %d-sample buffer\n",
				  achieved.freq, achieved.format, achieved.channels, achieved.samples);

	this->samplerate = achieved.freq;
	this->channels   = achieved.channels;
	switch (achieved.format) {
		case AUDIO_S8: this->bits = 8; break;
		case AUDIO_S16SYS: this->bits = 16; break;
		default:
			SDL_CloseAudio();
			return false;
	}
	this->buflen = achieved.samples;

	inited = true;
	return true;
}

int WaveOut_SDL::GetSampleRate(void) const { return inited ? samplerate : -1; }
int WaveOut_SDL::GetChannels(void) const { return inited ? channels : -1; }
int WaveOut_SDL::GetBitsPerSample(void) const { return inited ? bits : -1; }
int WaveOut_SDL::GetMixBufferLen(void) const { return inited ? buflen : -1; }

bool WaveOut_SDL::Pause(bool onf)
{
	if (!inited) return false;
	SDL_PauseAudio(onf);
	return true;
}

bool WaveOut_SDL::Lock(void)
{
	if (!inited) return false;
	SDL_LockAudio();
	return true;
}

bool WaveOut_SDL::Unlock(void)
{
	if (!inited) return false;
	SDL_UnlockAudio();
	return true;
}

bool WaveOut_SDL::SetVolume(float vol)
{
	return false;
}

float WaveOut_SDL::GetVolume(void) const
{
	return 0;
}
