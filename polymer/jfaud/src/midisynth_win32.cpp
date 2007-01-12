#define JFAUD_INTERNAL
#define WIN32_LEAN_AND_MEAN
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include <ctype.h>

#include <windows.h>
#include <mmsystem.h>

#include "midisynth_win32.hpp"
#include "log.h"

#ifndef MOD_WAVETABLE
#define MOD_WAVETABLE 6
#define MOD_SWSYNTH 7
#endif

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#define USE_THREAD

void MidiSynth_Win32::Lock()
{
#ifdef USE_THREAD
	EnterCriticalSection(&mutex);
#endif
}
void MidiSynth_Win32::Unlock()
{
#ifdef USE_THREAD
	LeaveCriticalSection(&mutex);
#endif
}

void CALLBACK MidiSynth_Win32::midiCallback(HMIDIOUT handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	LPMIDIHDR hdr;
	struct hdrtyp *hdrtyp;

        if (uMsg != MOM_DONE) return;

	hdr = (LPMIDIHDR)dwParam1;
	hdrtyp = (struct hdrtyp *)(hdr->dwUser);
	hdrtyp->synth->_BufferDone(hdrtyp->n);
}

DWORD WINAPI MidiSynth_Win32::BufThread(class MidiSynth_Win32 *myself)
{
	MSG msg;
	int i;

	while (1) {
		switch (GetMessage(&msg, NULL, 0, 0)) {
			case 0: return 0;	// WM_QUIT.. do threads get this?
			case -1: return 1;	// fatal error from GetMessage
			default: break;
		}
		if (msg.message == MM_MOM_OPEN) continue;
		else if (msg.message == MM_MOM_CLOSE) return 0;
		else if (msg.message == MM_MOM_DONE) {
			myself->Lock();
			for (i=0;i<2;i++) {
				if ((LPARAM)&myself->headers[i].header == msg.lParam) {
					myself->_BufferDone(i);
					break;
				}
			}
			if (i == 2) _JFAud_LogMsg("MidiSynth_Win32::BufThread(): MM_MOM_DONE gave us an unknown buffer!\n");
			if (myself->streamhnd && myself->seq) myself->_Update();
			myself->Unlock();
		}
	}
}

void MidiSynth_Win32::_BufferDone(int n)
{
	if (headers[n].header.dwFlags & MHDR_PREPARED) {
		midiOutUnprepareHeader((HMIDIOUT)streamhnd, &headers[n].header, sizeof(MIDIHDR));
		buffersplaying--;
		buffullness[n] = 0;
	}
}


MidiSynth_Win32::MidiSynth_Win32()
	: streamhnd(NULL), seq(NULL), threadhnd(NULL)
{
#ifdef USE_THREAD
	InitializeCriticalSection(&mutex);
#endif
	memset(headers, 0, sizeof(headers));
	memset(buffullness, 0, sizeof(buffullness));
	buffersplaying = 0;
	Reset();
}

MidiSynth_Win32::~MidiSynth_Win32()
{
	Close();
#ifdef USE_THREAD
	DeleteCriticalSection(&mutex);
#endif
}

bool MidiSynth_Win32::Open(const char *dev)
{
	MMRESULT result;
	int numdevs, i, j, matchdev = -1, matchchars = -1;
	MIDIOUTCAPS caps, matchcaps;
	const char *type;

	numdevs = midiOutGetNumDevs();
	if (!numdevs) {
		_JFAud_LogMsg("MidiSynth_Win32::Open(): no MIDI devices\n");
		return false;
	}

	// note: a zero-length string in dev will choose the first device
	if (!dev) dev = "";
        for (i=0; i<numdevs; i++) {
                if (midiOutGetDevCaps(i,&caps,sizeof(MIDIOUTCAPS)) != MMSYSERR_NOERROR) continue;

                for (j=0; caps.szPname[j] && dev[j] && toupper(caps.szPname[j]) == toupper(dev[j]); j++) ;
                if (j > matchchars) matchchars = j, matchdev = i, memcpy(&matchcaps,&caps, sizeof(MIDIOUTCAPS));
        }
	if (matchdev < 0) return false;	// shouldn't happen!

	switch (matchcaps.wTechnology) {
		case MOD_MIDIPORT:  type = "a hardware port";           devtype = MidiSequencer::DEV_GeneralMidi; break;
		case MOD_SYNTH:     type = "a generic synthesiser";     devtype = MidiSequencer::DEV_GeneralMidi; break;
		case MOD_SQSYNTH:   type = "a square-wave synthesiser"; devtype = MidiSequencer::DEV_SoundBlasterOpl; break;
		case MOD_FMSYNTH:   type = "an FM synthesiser";         devtype = MidiSequencer::DEV_SoundBlasterOpl; break;
		case MOD_WAVETABLE: type = "a wavetable synthesiser";   devtype = MidiSequencer::DEV_GeneralMidi; break;
		case MOD_SWSYNTH:   type = "a software synthesiser";    devtype = MidiSequencer::DEV_GeneralMidi; break;
		default:            type = "an unknown synthesiser";    devtype = MidiSequencer::DEV_GeneralMidi; break;
	}
#ifdef DEBUG
	_JFAud_LogMsg("MidiSynth_Win32::Open(): device %d is \"%s\", %s\n", matchdev, matchcaps.szPname, type);
#endif

#ifdef USE_THREAD
	threadhnd = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)BufThread, (LPVOID)this, 0, &threadid);
	if (!threadhnd) {
		_JFAud_LogMsg("MidiSynth_Win32::Open(): CreateThread() failed with error 0x%x\n", GetLastError());
		return false;
	} else _JFAud_LogMsg("MidiSynth_Win32::Open(): Spawned buffering thread %d\n", threadid);
	
        result = midiStreamOpen(&streamhnd, (LPUINT)&matchdev, 1, (DWORD)threadid, 0, CALLBACK_THREAD);
#else
        result = midiStreamOpen(&streamhnd, (LPUINT)&matchdev, 1, (DWORD)midiCallback, 0, CALLBACK_FUNCTION);
#endif
                // dwInstance would be the pointer to the stream info struct
        if (result != MMSYSERR_NOERROR) return false;

	return true;
}

bool MidiSynth_Win32::Close(void)
{
	if (!streamhnd) return false;

	Lock();
	if (!Reset()) { Unlock(); return false; }

	midiStreamClose(streamhnd);
	streamhnd = NULL;
	Unlock();

#ifdef USE_THREAD
	if (threadhnd) {
		switch (WaitForSingleObject(threadhnd, 1000)) {
			case WAIT_OBJECT_0: {
				DWORD d = 0;
				GetExitCodeThread(threadhnd, &d);
				_JFAud_LogMsg("MidiSynth_Win32::Close(): Buffering thread exited with code %d\n", d);
			} break;
			default: _JFAud_LogMsg("MidiSynth_Win32::Close(): Buffering thread failed to exit\n"); break;
		}
		CloseHandle(threadhnd);
		threadhnd = NULL;
	}
#endif

	return true;
}

bool MidiSynth_Win32::Reset(bool stop)
{
	int i;

	if (streamhnd) {
		midiStreamStop(streamhnd);
		/*for (i=0;i<2;i++)
			if (headers[i].header.dwFlags & MHDR_PREPARED)
                                midiOutUnprepareHeader((HMIDIOUT)streamhnd, &headers[i].header, sizeof(MIDIHDR)); */
		StopAllNotes();
	}

	if (stop) {
		if (seq) seq->Rewind();
	} else {
		if (seq) delete seq;
		seq = NULL;
		loop = false;
	}

	// the callback/thread will handle these when _BufferDone is called
	//memset(headers, 0, sizeof(headers));
	//memset(buffullness, 0, sizeof(buffullness));
	//buffersplaying = 0;
	memset(bufdata, 0, sizeof(bufdata));
	evtcommand = 0;
	paused = justpaused = false;
	
	return true;
}

void MidiSynth_Win32::StopAllNotes(void)
{
	int i;
	if (!streamhnd) return;
	for (i=0;i<16;i++) {
		midiOutShortMsg((HMIDIOUT)streamhnd, (0xB0+i) | (64<<8));
		midiOutShortMsg((HMIDIOUT)streamhnd, (0xB0+i) | (123<<8));
		midiOutShortMsg((HMIDIOUT)streamhnd, (0xB0+i) | (120<<8));
	}
}

bool MidiSynth_Win32::_Update()
{
	int i;
	for (i=0;i<2;i++) {
		if (buffullness[i] > 0) continue;
		switch (bufferdata(i)) {
			case 0: continue;
			case -1: return false;	// midi device error
			default: return true;		// eof or midi file error
		}
	}
	return true;
}
bool MidiSynth_Win32::Update(void)
{
#ifdef USE_THREAD
	return true;
#else
	bool r;

	if (!streamhnd) return false;
	if (!seq) return true;

	Lock();
	r = _Update();
	Unlock();
	return r;
#endif
}

char **MidiSynth_Win32::Enumerate(char **def)
{
	int numdevs, tnumdevs=0, i, devstrlen = 0;
	char **rl, **rp, *p;
	MIDIOUTCAPS caps;

	numdevs = midiOutGetNumDevs();

	for (i=0; i<numdevs; i++) {
                if (midiOutGetDevCaps(i,&caps,sizeof(MIDIOUTCAPS)) != MMSYSERR_NOERROR) continue;
		devstrlen += 1 + strlen(caps.szPname);
		tnumdevs++;
	}
	
	rl = (char**)calloc(1, sizeof(char*)*(tnumdevs+1)+devstrlen);
	if (!rl) return NULL;

	p = (char*)rl + sizeof(char*)*(tnumdevs+1);
	rp = rl;
        for (i=0; i<numdevs && tnumdevs>0; i++, tnumdevs--) {
                if (midiOutGetDevCaps(i,&caps,sizeof(MIDIOUTCAPS)) != MMSYSERR_NOERROR) continue;
		*(rp++) = p;
		if (def && i==0) *def = p;
		strcpy(p, caps.szPname);
		p += strlen(caps.szPname) + 1;
        }

	return rl;
}

bool MidiSynth_Win32::SetMedia(MidiSequencer *media)
{
	if (!media || !media->IsValid()) return false;

	Lock();
	if (!Reset()) { Unlock(); return false; }

	seq = media;
	seq->SetDevice(devtype);
	Unlock();

	return true;
}

bool MidiSynth_Win32::Play(void)
{
	MMRESULT result;
        MIDIPROPTIMEDIV tdiv; 
	
	if (!streamhnd || !seq) return false;

	Lock();
	tdiv.cbStruct = sizeof(MIDIPROPTIMEDIV);
        tdiv.dwTimeDiv = seq->GetPPQN();
        result = midiStreamProperty(streamhnd, (LPBYTE)&tdiv, MIDIPROP_SET|MIDIPROP_TIMEDIV);
        if (result != MMSYSERR_NOERROR) { Unlock(); return false; }

	paused = true;

	result = midiStreamRestart(streamhnd);
	if (result != MMSYSERR_NOERROR) { Unlock(); return false; }
	_Update();
	Unlock();

	return true;
}

bool MidiSynth_Win32::Stop(void)
{
	Lock();
	Reset(true);
	Unlock();
	return true;
}

bool MidiSynth_Win32::Pause(void)
{
	Lock();
	paused = justpaused = true;
	Unlock();
	return streamhnd && seq;
}

bool MidiSynth_Win32::Resume(void)
{
	Lock();
	paused = false;
	Unlock();
	return streamhnd && seq;
}

bool MidiSynth_Win32::SetLoop(bool onf)
{
	if (!seq || !streamhnd) return false;

	Lock();
	loop = onf;
	seq->SetLoop(onf);
	Unlock();
	return true;
}

bool MidiSynth_Win32::GetLoop(void) const
{
	return loop;
}


int MidiSynth_Win32::bufferdata(int num)
{
	MMRESULT result;
	
	// buffers some messages and queues them
	unsigned int tickamt;
	int i=0,j,k;
	
	buffullness[num] = 0;

	if (!evtcommand) {
		if (seq->GetEvent(&evtdelta, &evtcommand, &evtlength, &evtdata)) {
			evtcommand = 0;
			return 1;	// error or eof
		}
	}

	tickamt = seq->GetPPQN()/8;	// the div by eight increases buffering but pausing becomes more responsive
	
	if (paused) {
		if (justpaused) {
			// turn all notes off
			for (i=0;i<16;i++) {
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = MEVT_F_SHORT | (0xB0+i) | (64<<8);
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = MEVT_F_SHORT | (0xB0+i) | (123<<8);
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = 0;
				bufdata[num][ buffullness[num]++ ] = MEVT_F_SHORT | (0xB0+i) | (120<<8);
			}
			justpaused = false;
		}
		
		// queue a nop for tickamt
		bufdata[num][ buffullness[num]++ ] = tickamt;
		bufdata[num][ buffullness[num]++ ] = 0;
		bufdata[num][ buffullness[num]++ ] = (MEVT_NOP<<24);
		goto queueandret;
	}

	if (evtdelta > tickamt) {
		evtdelta -= tickamt;

		// queue a nop for tickamt
		bufdata[num][ buffullness[num]++ ] = tickamt;
		bufdata[num][ buffullness[num]++ ] = 0;
		bufdata[num][ buffullness[num]++ ] = (MEVT_NOP<<24);
		goto queueandret;
	}

	do {
		// queue the event. tempo events get inserted as MEVT_TEMPO
		tickamt -= evtdelta;

		bufdata[num][ buffullness[num]++ ] = evtdelta;
		bufdata[num][ buffullness[num]++ ] = 0;
		if (evtcommand == 0xFF && evtdata[0] == 0x51) {
			bufdata[num][ buffullness[num]++ ] = (MEVT_TEMPO<<24) | ((DWORD)evtdata[2]<<16) |
							     ((DWORD)evtdata[3]<<8) | ((DWORD)evtdata[4]);
		} else if (evtlength < 3) {
			bufdata[num][ buffullness[num] ] = MEVT_F_SHORT | evtcommand;
			switch (evtlength) {
				case 2: bufdata[num][ buffullness[num] ] |= (DWORD)evtdata[1] << 16;
				case 1: bufdata[num][ buffullness[num] ] |= (DWORD)evtdata[0] << 8;
				default: break;
			}
			buffullness[num]++;
		} else {
			bufdata[num][ buffullness[num]++ ] = MEVT_F_LONG | (1+evtlength);
			bufdata[num][ buffullness[num] ] = evtcommand;
			for(j=1,k=0;k<evtlength;k++) {
				bufdata[num][ buffullness[num] ] |= ((DWORD)evtdata[k] << (j*8));
				if (++j==4) {
					buffullness[num]++;
					j=0;
					bufdata[num][ buffullness[num] ] = 0;
				}
			}
			if (j>0) buffullness[num]++;	// ended on a partial?
		}

		i = seq->GetEvent(&evtdelta, &evtcommand, &evtlength, &evtdata);
		if (i>0) {
			evtcommand = 0;
			break;		// end of song
		} else if (i<0) {
			evtcommand = 0;
			return 1;	// error or eof
		}
	} while (evtdelta <= tickamt && STREAMBUFLEN-buffullness[num] > (evtlength+sizeof(MIDIEVENT)+3)/4);

queueandret:
	headers[num].n = num;
	headers[num].synth = this;
	
	// queue buffer
	headers[num].header.lpData = (LPSTR)&bufdata[num];
	headers[num].header.dwBufferLength = headers[num].header.dwBytesRecorded = buffullness[num]*4;
	headers[num].header.dwUser = (DWORD)&headers[num];
	headers[num].header.dwFlags = 0;
	result = midiOutPrepareHeader((HMIDIOUT)streamhnd, &headers[num].header, sizeof(MIDIHDR));
	if (result != MMSYSERR_NOERROR) return -1;
	result = midiStreamOut(streamhnd, &headers[num].header, sizeof(MIDIHDR));
	if (result != MMSYSERR_NOERROR) return -1;
	buffersplaying++;

	return i;
} 

