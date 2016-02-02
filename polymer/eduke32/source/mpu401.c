//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "mpu401.h"
#include "compat.h"
#include "pragmas.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

static HMIDISTRM hmido = (HMIDISTRM)-1;
static MIDIOUTCAPS midicaps;
static DWORD mididevice = -1;

#define PAD(x) ((((x)+3)&(~3)))

#define BUFFERLEN (32*4*4)
#define NUMBUFFERS 6
static char eventbuf[NUMBUFFERS][BUFFERLEN];
static int32_t  eventcnt[NUMBUFFERS];
static MIDIHDR bufferheaders[NUMBUFFERS];
static int32_t  _MPU_CurrentBuffer = 0;
int32_t  _MPU_BuffersWaiting = 0;

extern uint32_t _MIDI_GlobalPositionInTicks;
static uint32_t _MPU_LastEvent=0;

#define MIDI_NOTE_OFF         0x80
#define MIDI_NOTE_ON          0x90
#define MIDI_POLY_AFTER_TCH   0xA0
#define MIDI_CONTROL_CHANGE   0xB0
#define MIDI_PROGRAM_CHANGE   0xC0
#define MIDI_AFTER_TOUCH      0xD0
#define MIDI_PITCH_BEND       0xE0
#define MIDI_META_EVENT       0xFF
#define MIDI_END_OF_TRACK     0x2F
#define MIDI_TEMPO_CHANGE     0x51
#define MIDI_MONO_MODE_ON     0x7E
#define MIDI_ALL_NOTES_OFF    0x7B

void MPU_FinishBuffer(int32_t buffer)
{
    if (!eventcnt[buffer]) return;
    ZeroMemory(&bufferheaders[buffer], sizeof(MIDIHDR));
    bufferheaders[buffer].lpData = eventbuf[buffer];
    bufferheaders[buffer].dwBufferLength =
        bufferheaders[buffer].dwBytesRecorded = eventcnt[buffer];
    midiOutPrepareHeader((HMIDIOUT)hmido, &bufferheaders[buffer], sizeof(MIDIHDR));
    midiStreamOut(hmido, &bufferheaders[buffer], sizeof(MIDIHDR));
//	printf("Sending %d bytes (buffer %d)\n",eventcnt[buffer],buffer);
    _MPU_BuffersWaiting++;
}

void MPU_BeginPlayback(void)
{
    _MPU_LastEvent = _MIDI_GlobalPositionInTicks;
    if (hmido != (HMIDISTRM)-1) midiStreamRestart(hmido);
}

void MPU_Pause(void)
{
    if (hmido != (HMIDISTRM)-1) midiStreamPause(hmido);
}

void MPU_Unpause(void)
{
    if (hmido != (HMIDISTRM)-1) midiStreamRestart(hmido);
}


void CALLBACK MPU_MIDICallback(HMIDIOUT handle, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    UNREFERENCED_PARAMETER(dwInstance);
    UNREFERENCED_PARAMETER(dwParam2);

    switch (uMsg)
    {
    case MOM_DONE:
        midiOutUnprepareHeader((HMIDIOUT)handle, (MIDIHDR *)dwParam1, sizeof(MIDIHDR));
        for (int i=0; i<NUMBUFFERS; i++)
        {
            if ((MIDIHDR *)dwParam1 == &bufferheaders[i])
            {
                eventcnt[i] = 0;	// marks the buffer as free
//					printf("Finished buffer %d\n",i);
                _MPU_BuffersWaiting--;
                break;
            }
        }
        return;

    default: return;
    }
}

int32_t MPU_GetNextBuffer(void)
{
    for (int i = 0; i < NUMBUFFERS; i++)
        if (eventcnt[i] == 0)
            return i;

    return -1;
}

static int32_t const masks[3] ={ 0x00ffffffl, 0x0000ffffl, 0x000000ffl };

void MPU_SendMidi(char *data, int32_t count)
{
    if (count <= 0) return;

    if (count <= 3)
    {
        if (eventcnt[_MPU_CurrentBuffer] + 12 > BUFFERLEN)
        {
            // buffer over-full
            int32_t nextbuffer = MPU_GetNextBuffer();
            if (nextbuffer < 0)
            {
//				printf("All buffers full!\n");
                return;
            }
            MPU_FinishBuffer(_MPU_CurrentBuffer);
            _MPU_CurrentBuffer = nextbuffer;
        }

        char *p = eventbuf[_MPU_CurrentBuffer] + eventcnt[_MPU_CurrentBuffer];
        ((int32_t *)p)[0] = _MIDI_GlobalPositionInTicks - _MPU_LastEvent;
        ((int32_t *)p)[1] = 0;
        ((int32_t *)p)[2] = (MEVT_SHORTMSG << 24) | ((*((int32_t *)data)) & masks[count-1]);
        eventcnt[_MPU_CurrentBuffer] += 12;
    }
    else
    {
        int32_t padded = PAD(count);
        if (eventcnt[_MPU_CurrentBuffer] + 12 + padded > BUFFERLEN)
        {
            // buffer over-full
            int32_t nextbuffer = MPU_GetNextBuffer();
            if (nextbuffer < 0)
            {
//				printf("All buffers full!\n");
                return;
            }
            MPU_FinishBuffer(_MPU_CurrentBuffer);
            _MPU_CurrentBuffer = nextbuffer;
        }

        char *p = eventbuf[_MPU_CurrentBuffer] + eventcnt[_MPU_CurrentBuffer];
        ((int32_t *)p)[0] = _MIDI_GlobalPositionInTicks - _MPU_LastEvent;
        ((int32_t *)p)[1] = 0;
        ((int32_t *)p)[2] = (MEVT_LONGMSG<<24) | (count & 0xffffffl);
        p+=12; eventcnt[_MPU_CurrentBuffer] += 12;
        for (; count>0; count--, padded--, eventcnt[_MPU_CurrentBuffer]++)
            *(p++) = *(data++);
        for (; padded>0; padded--, eventcnt[_MPU_CurrentBuffer]++)
            *(p++) = 0;
    }
    _MPU_LastEvent = _MIDI_GlobalPositionInTicks;
}

int32_t MPU_Reset(void)
{
    midiStreamStop(hmido);
    midiStreamClose(hmido);

    return MPU_Ok;
}

int32_t MPU_Init(int32_t addr)
{
    for (int i=0; i<NUMBUFFERS; i++) eventcnt[i]=0;

    mididevice = addr;

    if (midiOutGetDevCaps(mididevice, &midicaps, sizeof(MIDIOUTCAPS)) != MMSYSERR_NOERROR) return MPU_Error;

    if (midiStreamOpen(&hmido,(LPUINT)&mididevice,1,(DWORD_PTR)MPU_MIDICallback,0L,CALLBACK_FUNCTION) != MMSYSERR_NOERROR) return MPU_Error;

    return MPU_Ok;
}

void MPU_NoteOff(char channel, char key, char velocity)
{
    char msg[] = { (char)(MIDI_NOTE_OFF | channel), key, velocity };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_NoteOn(char channel, char key, char velocity)
{
    char msg[] = { (char)(MIDI_NOTE_ON | channel), key, velocity };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_PolyAftertouch(char channel, char key, char pressure)
{
    char msg[] = { (char) (MIDI_POLY_AFTER_TCH | channel), key, pressure };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_ControlChange(char channel, char number, char value)
{
    char msg[] = { (char) (MIDI_CONTROL_CHANGE | channel), number, value };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_ProgramChange(char channel, char program)
{
    char msg[] = { (char)(MIDI_PROGRAM_CHANGE | channel), program };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_ChannelAftertouch(char channel, char pressure)
{
    char msg[] = { (char)(MIDI_AFTER_TOUCH | channel), pressure };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_PitchBend(char channel, char lsb, char msb)
{
    char msg[] = { (char)(MIDI_PITCH_BEND | channel), lsb, msb };
    MPU_SendMidi(msg, sizeof(msg));
}

void MPU_SetTempo(int32_t tempo)
{
    MIDIPROPTEMPO prop;
    prop.cbStruct = sizeof(MIDIPROPTEMPO);
    prop.dwTempo = tabledivide32_noinline(60000000l, tempo);
    midiStreamProperty(hmido, (LPBYTE)&prop, MIDIPROP_SET|MIDIPROP_TEMPO);
}

void MPU_SetDivision(int32_t division)
{
    MIDIPROPTIMEDIV prop;
    prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
    prop.dwTimeDiv = division;
    midiStreamProperty(hmido, (LPBYTE)&prop, MIDIPROP_SET|MIDIPROP_TIMEDIV);
}

