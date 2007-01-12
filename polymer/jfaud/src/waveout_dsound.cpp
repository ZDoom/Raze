#define JFAUD_INTERNAL
#define WIN32_LEAN_AND_MEAN
#define DIRECTSOUND_VERSION 0x0300
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
#else
# include <cstdio>
#endif

#include "log.h"
#include "waveout_dsound.hpp"
#include "softwaremixer.hpp"
#include "dynlib.hpp"

#define DSOUNDDL "dsound.dll"
#define CallbackRate 40

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

static DynamicLibrary *lib = NULL;
static int refcount = 0;
static HRESULT (WINAPI *libDirectSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN) = NULL;

static const char * const DSoundErr(HRESULT h);

WaveOut_DSound::WaveOut_DSound(SoftwareMixer *m)
	: WaveOut(m),
	  owner(m), inited(false), paused(true),
	  samplerate(0), channels(0), bits(0), buflen(0),
	  hwnd(0), dsound(NULL), bufprim(NULL), bufsec(NULL), notify(NULL)
{
	notifypoints[0] = notifypoints[1] = notifypoints[2] = NULL;

	if (lib) { refcount++; return; }
	
	_JFAud_LogMsg("Loading " DSOUNDDL "\n");
	lib = new DynamicLibrary(DSOUNDDL);
	if (!lib) return;
	if (!lib->IsOpen()) {
		delete lib;
		lib = NULL;
		return;
	}

	libDirectSoundCreate = (HRESULT(WINAPI*)(LPGUID,LPDIRECTSOUND*,LPUNKNOWN))lib->Get("DirectSoundCreate");

	if (libDirectSoundCreate) refcount = 1;
	else {
		delete lib;
		lib = NULL;
	}

	InitializeCriticalSection(&mutex);
}

WaveOut_DSound::~WaveOut_DSound()
{
	if (mixthread) {
		SetEvent(notifypoints[0]);
		switch (WaitForSingleObject(mixthread, 1000)) {
			case WAIT_OBJECT_0: {
				DWORD d = 0;
				GetExitCodeThread(mixthread, &d);
				_JFAud_LogMsg("WaveOut_DSound::~WaveOut_DSound(): Mixer thread exited with code %d\n", d);
			} break;
			default: _JFAud_LogMsg("WaveOut_DSound::~WaveOut_DSound(): Mixer thread failed to exit\n"); break;
		}
		CloseHandle(mixthread);
	}
	if (notify) { notify->Release(); }
	if (bufsec) { bufsec->Release(); }
	if (bufprim) { bufprim->Release(); }
	if (dsound) { dsound->Release(); }
	if (notifypoints[0]) { CloseHandle(notifypoints[0]); }
	if (notifypoints[1]) { CloseHandle(notifypoints[1]); }
	if (notifypoints[2]) { CloseHandle(notifypoints[2]); }

	DeleteCriticalSection(&mutex);
	
	if (refcount > 1) { refcount--; return; }
	if (refcount == 0 || !lib) return;
	refcount = 0;
	delete lib;
	lib = NULL;
	libDirectSoundCreate = NULL;
}

bool WaveOut_DSound::SetWindowHandle(HWND h)
{
	hwnd = h;
	return true;
}

bool WaveOut_DSound::Init(int samplerate, int channels, int bits)
{
	DSBUFFERDESC desc;
	DSBPOSITIONNOTIFY posn[2];
	WAVEFORMATEX wfx;
	HRESULT hr;
	DWORD threadid;
	int i;

	if (inited || !lib || !owner) return false;

	dsound = NULL;
	bufprim = NULL;
	bufsec = NULL;
	notify = NULL;
	notifypoints[0] = notifypoints[1] = notifypoints[2] = NULL;
	mixthread = NULL;
	paused = true;

	// 0. Create Windows objects needed later on
	notifypoints[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
	notifypoints[1] = CreateEvent(NULL,FALSE,FALSE,NULL);
	notifypoints[2] = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (!notifypoints[0] || !notifypoints[1] || !notifypoints[2]) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): CreateEvent() failed with error 0x%x\n", GetLastError());
		goto failure;
	}

	// 1. Create the DirectSound object
	if FAILED(hr = libDirectSoundCreate(NULL, &dsound, NULL)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): DirectSoundCreate() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 2. Set the cooperative level
	if (!hwnd) hwnd = GetForegroundWindow();
	if (!hwnd) hwnd = GetDesktopWindow();
	if FAILED(hr = dsound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))
	{
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSound::SetCooperativeLevel() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}
	
	// 3. Create the primary buffer
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if FAILED(hr = dsound->CreateSoundBuffer(&desc, &bufprim, NULL)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSound::CreateSoundBuffer() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 4. Set the primary buffer format to what is requested
	ZeroMemory(&wfx, sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = channels;
	wfx.nSamplesPerSec = samplerate;
	wfx.wBitsPerSample = bits;
	wfx.nBlockAlign = bits/8*channels;
	wfx.nAvgBytesPerSec = samplerate*wfx.nBlockAlign;
	if FAILED(hr = bufprim->SetFormat(&wfx)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSoundBuffer::SetFormat() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 5. Fetch back what DirectSound actually set up
	ZeroMemory(&wfx, sizeof(wfx));
	if FAILED(hr = bufprim->GetFormat(&wfx, sizeof(wfx), NULL)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSoundBuffer::GetFormat() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}
	this->samplerate = wfx.nSamplesPerSec;
	this->channels = wfx.nChannels;
	this->bits = wfx.wBitsPerSample;
	for (i=1; i <= wfx.nSamplesPerSec / CallbackRate; i+=i) this->buflen = i;

	_JFAud_LogMsg("WaveOut_DSound::Init(): got %dHz %d-bit %d-channel with %d-sample buffer\n",
				  this->samplerate, this->bits, this->channels, this->buflen);

	// 6. Create our secondary mixing buffer
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
	desc.dwBufferBytes = wfx.nBlockAlign * this->buflen * 2;
	desc.lpwfxFormat = &wfx;
	if FAILED(hr = dsound->CreateSoundBuffer(&desc, &bufsec, NULL)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSound::CreateSoundBuffer() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 7. Setup the playback notification objects
	if FAILED(hr = bufsec->QueryInterface(IID_IDirectSoundNotify, (VOID**)&notify)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSoundBuffer::QueryInterface() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// when notifypoints[1] is tripped, the cursor is beyond the half way mark and the first half gets filled	
	posn[0].dwOffset = wfx.nBlockAlign * this->buflen;
	posn[0].hEventNotify = notifypoints[1];
	posn[1].dwOffset = 0;
	posn[1].hEventNotify = notifypoints[2];

	if FAILED(hr = notify->SetNotificationPositions(2, posn)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSoundNotify::SetNotificationPositions() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 8. Set the primary buffer playing. Our mixing buffer is still paused
	if FAILED(hr = bufprim->Play(0,0,DSBPLAY_LOOPING)) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): IDirectSoundBuffer::Play() failed with error %s\n", DSoundErr(hr));
		goto failure;
	}

	// 9. Spawn the mix thread
	mixthread = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)MixThread, (LPVOID)this, 0, &threadid);
	if (!mixthread) {
		_JFAud_LogMsg("WaveOut_DSound::Init(): CreateThread() failed with error 0x%x\n", GetLastError());
		goto failure;
	} else _JFAud_LogMsg("WaveOut_DSound::Init(): Spawned mixer thread %d\n", threadid);

	inited = true;
	return true;

failure:
	if (notify) bufsec->Release(); notify = NULL;
	if (bufsec) bufsec->Release(); bufsec = NULL;
	if (bufprim) bufprim->Release(); bufprim = NULL;
	if (dsound) dsound->Release(); dsound = NULL;
	if (notifypoints[0]) CloseHandle(notifypoints[0]); notifypoints[0] = NULL;
	if (notifypoints[1]) CloseHandle(notifypoints[1]); notifypoints[1] = NULL;
	if (notifypoints[2]) CloseHandle(notifypoints[2]); notifypoints[2] = NULL;
	return false;
}

int WaveOut_DSound::GetSampleRate(void) const { return inited ? samplerate : -1; }
int WaveOut_DSound::GetChannels(void) const { return inited ? channels : -1; }
int WaveOut_DSound::GetBitsPerSample(void) const { return inited ? bits : -1; }
int WaveOut_DSound::GetMixBufferLen(void) const { return inited ? buflen : -1; }

bool WaveOut_DSound::Pause(bool onf)
{
	HRESULT hr;
	if (!inited) return false;
	if (paused && !onf) {
		if FAILED(hr = bufsec->Play(0,0,DSBPLAY_LOOPING)) {
			if (hr == DSERR_BUFFERLOST) {
				if (FAILED(hr = bufsec->Restore())) {
					_JFAud_LogMsg("WaveOut_DSound::Pause() failed to restore with error %s\n", DSoundErr(hr));
					return false;
				}

				hr = bufsec->Play(0,0,DSBPLAY_LOOPING);
			}
			if (FAILED(hr)) {
				_JFAud_LogMsg("WaveOut_DSound::Pause() failed to play with error %s\n", DSoundErr(hr));
				return false;
			}
		} else paused = false;
	} else if (!paused && onf) {
		if FAILED(hr = bufsec->Stop()) {
			_JFAud_LogMsg("WaveOut_DSound::Pause() failed to stop with error %s\n", DSoundErr(hr));
			return false;
		} else paused = true;
	}

	return true;
}

bool WaveOut_DSound::Lock(void)
{
	if (!inited) return false;
	EnterCriticalSection(&mutex);
	return true;
}

bool WaveOut_DSound::Unlock(void)
{
	if (!inited) return false;
	LeaveCriticalSection(&mutex);
	return true;
}

bool WaveOut_DSound::SetVolume(float vol)
{
	return true;
}

float WaveOut_DSound::GetVolume(void) const
{
	return 0.0;
}

DWORD WINAPI WaveOut_DSound::MixThread(WaveOut_DSound *myself)
{
	LPVOID lockptr, lockptr2;
	DWORD lockbytes, lockbytes2;
	DWORD rv;
	HRESULT hr;
	int blocksize = myself->buflen * (myself->bits / 8) * myself->channels;

	while (1) {
		rv = WaitForMultipleObjects(3, myself->notifypoints, FALSE, INFINITE);
		if (rv == WAIT_OBJECT_0) {
			// shutdown semaphore was signalled
			_JFAud_LogMsg("WaveOut_DSound::MixThread(): shutdown event signalled\n");
			return 0;
		} else if (rv > WAIT_OBJECT_0) {
			// a notification point was passed
			DWORD p = blocksize * (rv-WAIT_OBJECT_0-1);

			//_JFAud_LogMsg("WaveOut_DSound::MixThread(): notification point %d reached\n", rv-WAIT_OBJECT_0-1);
			EnterCriticalSection(&myself->mutex);

			lockptr = lockptr2 = NULL;
			lockbytes = lockbytes2 = 0;
			if FAILED(hr = myself->bufsec->Lock(p, blocksize, &lockptr, &lockbytes, &lockptr2, &lockbytes2, 0)) {
				if (hr == DSERR_BUFFERLOST) {
					if (FAILED(hr = myself->bufsec->Restore())) {
						_JFAud_LogMsg("WaveOut_DSound::MixThread() failed to restore with error %s\n",
								DSoundErr(hr));
						LeaveCriticalSection(&myself->mutex);
						continue;
					}

					hr = myself->bufsec->Lock(p, blocksize, &lockptr, &lockbytes, &lockptr2, &lockbytes2, 0);
				}
				if FAILED(hr) {
					_JFAud_LogMsg("WaveOut_DSound::MixThread() failed to lock buffer with error %s\n", DSoundErr(hr));
					LeaveCriticalSection(&myself->mutex);
					continue;
				}
			}


			myself->owner->MixSome(lockptr, lockbytes);
			if (lockptr2) {
				_JFAud_LogMsg("WaveOut_DSound::MixThread() got split buffer\n");
				myself->owner->MixSome(lockptr2, lockbytes2);
			}

			if FAILED(hr = myself->bufsec->Unlock(lockptr,lockbytes,lockptr2,lockbytes2)) {
				_JFAud_LogMsg("WaveOut_DSound::MixThread() failed to unlock buffer with error %s\n", DSoundErr(hr));
			}

			LeaveCriticalSection(&myself->mutex);
		} else if (rv == WAIT_FAILED) {
			_JFAud_LogMsg("WaveOut_DSound::MixThread(): wait failed\n");
			return 1;
		} else {
			_JFAud_LogMsg("WaveOut_DSound::MixThread(): other error\n");
			// WAIT_ABANDONED_x, WAIT_TIMEOUT
		}
	}
}

static const char * const DSoundErr(HRESULT h)
{
	static char e[32];
	switch (h) {
		case DS_OK: return "DS_OK";
#ifdef DS_NO_VIRTUALIZATION
		case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
#endif
		case DSERR_ALLOCATED: return "DSERR_ALLOCATED";
		case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
		case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
		case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
		case DSERR_GENERIC: return "DSERR_GENERIC";
		case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
		case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
		case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
		case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
		case DSERR_NODRIVER: return "DSERR_NODRIVER";
		case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
		case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
		case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
		case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
		case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
		case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
#ifdef DSERR_ACCESSDENIED
		case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
#endif
		default: sprintf(e,"%X",h); return e;
	}
}
