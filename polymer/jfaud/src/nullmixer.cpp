#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
# include "watcomhax/cstring"
#else
# include <cstdio>
# include <cstring>
#endif
#include "nullmixer.hpp"
#include "log.h"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ NullMixerChannel
NullMixerChannel::NullMixerChannel() { }
NullMixerChannel::~NullMixerChannel() { }

bool NullMixerChannel::SetMedia(WaveformFile *) { return false; }

bool NullMixerChannel::Play(void) { return true; }
bool NullMixerChannel::Pause(void) { return true; }
bool NullMixerChannel::Update(void) { return true; }
bool NullMixerChannel::IsPlaying(void) const { return false; }
bool NullMixerChannel::IsPaused(void) const { return false; }
bool NullMixerChannel::IsStopped(void) const { return true; }

bool NullMixerChannel::SetGain(float gain) { return true; }
bool NullMixerChannel::SetPitch(float pitch) { return true; }
bool NullMixerChannel::SetPosition(float x, float y, float z) { return true; }
bool NullMixerChannel::SetVelocity(float x, float y, float z) { return true; }
bool NullMixerChannel::SetDirection(float x, float y, float z) { return true; }
bool NullMixerChannel::SetRefDist(float refdist) { return true; }
bool NullMixerChannel::SetRolloff(float rolloff) { return true; }
bool NullMixerChannel::SetLoop(bool onf) { return true; }
bool NullMixerChannel::SetFollowListener(bool onf) { return true; }

float NullMixerChannel::GetGain(void) const { return 0.0; }
float NullMixerChannel::GetPitch(void) const { return 1.0; }
void  NullMixerChannel::GetPosition(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
void  NullMixerChannel::GetVelocity(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
void  NullMixerChannel::GetDirection(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
float NullMixerChannel::GetRefDist(void) const { return 0.0; }
float NullMixerChannel::GetRolloff(void) const { return 0.0; }
bool  NullMixerChannel::GetLoop(void) const { return false; }
bool  NullMixerChannel::GetFollowListener(void) const { return false; }
//}}}
//{{{ NullMixer
NullMixer::NullMixer() { }
NullMixer::~NullMixer() { }
JFAudMixerChannel *NullMixer::AcquireChannel(void) { return NULL; }
bool NullMixer::ReleaseChannel(JFAudMixerChannel *ch) { return false; }
bool NullMixer::Update() { return true; }
bool NullMixer::SetListenerPosition(float x, float y, float z) { return true; }
bool NullMixer::SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz) { return true; }
bool NullMixer::SetListenerVelocity(float x, float y, float z) { return true; }
bool NullMixer::SetListenerGain(float gain) { return true; }
void NullMixer::GetListenerPosition(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
void NullMixer::GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const { *atx = *aty = *atz = *upx = *upy = *upz = 0.0; }
void NullMixer::GetListenerVelocity(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
float NullMixer::GetListenerGain(void) const { return 0.0; }
//}}}

// vim:fdm=marker:
