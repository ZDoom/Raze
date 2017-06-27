//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

/**********************************************************************
   module: MPU401.C

   author: James R. Dose
   date:   January 1, 1994

   Low level routines to support sending of MIDI data to MPU401
   compatible MIDI interfaces.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

// This object is shared by all Build games with MIDI playback!

#include "mpu401.h"
#include "compat.h"
#include "pragmas.h"

#define NEED_MMSYSTEM_H
#include "windows_inc.h"

static HMIDISTRM hmido = (HMIDISTRM)-1;
static MIDIOUTCAPS midicaps;
static DWORD mididevice = -1;

typedef struct
{
    int32_t time;
    int32_t stream;
    int32_t event;
}
MIDIEVENTHEAD;
#define PAD(x) ((((x)+3)&(~3)))

#define BUFFERLEN (32*4*4)
#define NUMBUFFERS 6
static char eventbuf[NUMBUFFERS][BUFFERLEN];
static int32_t  eventcnt[NUMBUFFERS];
static MIDIHDR bufferheaders[NUMBUFFERS];
int32_t  _MPU_CurrentBuffer = 0;
int32_t  _MPU_BuffersWaiting = 0;

extern uint32_t _MIDI_GlobalPositionInTicks;
uint32_t _MPU_LastEvent=0;

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


/**********************************************************************

   Memory locked functions:

**********************************************************************/


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
    int32_t i;

    UNREFERENCED_PARAMETER(dwInstance);
    UNREFERENCED_PARAMETER(dwParam2);

    switch (uMsg)
    {
    case MOM_DONE:
        midiOutUnprepareHeader((HMIDIOUT)handle, (MIDIHDR *)dwParam1, sizeof(MIDIHDR));
        for (i=0; i<NUMBUFFERS; i++)
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


/*---------------------------------------------------------------------
   Function: MPU_SendMidi

   Queues a MIDI message to the music device.
---------------------------------------------------------------------*/

int32_t MPU_GetNextBuffer(void)
{
    int32_t i;
    for (i=0; i<NUMBUFFERS; i++)
    {
        if (eventcnt[i] == 0) return i;
    }
    return -1;
}

void MPU_SendMidi(char *data, int32_t count)
{
    char *p;
    int32_t padded, nextbuffer;
    static int32_t masks[3] = { 0x000000ffl, 0x0000ffffl, 0x00ffffffl };

    if (count <= 0) return;
    if (count <= 3)
    {
        if (eventcnt[_MPU_CurrentBuffer] + 12 > BUFFERLEN)
        {
            // buffer over-full
            nextbuffer = MPU_GetNextBuffer();
            if (nextbuffer < 0)
            {
//				printf("All buffers full!\n");
                return;
            }
            MPU_FinishBuffer(_MPU_CurrentBuffer);
            _MPU_CurrentBuffer = nextbuffer;
        }

        p = eventbuf[_MPU_CurrentBuffer] + eventcnt[_MPU_CurrentBuffer];
        ((int32_t *)p)[0] = _MIDI_GlobalPositionInTicks - _MPU_LastEvent;
        ((int32_t *)p)[1] = 0;
        ((int32_t *)p)[2] = (MEVT_SHORTMSG << 24) | ((*((int32_t *)data)) & masks[count-1]);
        eventcnt[_MPU_CurrentBuffer] += 12;
    }
    else
    {
        padded = PAD(count);
        if (eventcnt[_MPU_CurrentBuffer] + 12 + padded > BUFFERLEN)
        {
            // buffer over-full
            nextbuffer = MPU_GetNextBuffer();
            if (nextbuffer < 0)
            {
//				printf("All buffers full!\n");
                return;
            }
            MPU_FinishBuffer(_MPU_CurrentBuffer);
            _MPU_CurrentBuffer = nextbuffer;
        }

        p = eventbuf[_MPU_CurrentBuffer] + eventcnt[_MPU_CurrentBuffer];
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


/*---------------------------------------------------------------------
   Function: MPU_SendMidiImmediate

   Sends a MIDI message immediately to the the music device.
---------------------------------------------------------------------*/
void MPU_SendMidiImmediate(char *data, int32_t count)
{
    MIDIHDR mhdr;
    static int32_t masks[3] = { 0x00ffffffl, 0x0000ffffl, 0x000000ffl };

    if (!count) return;
    if (count<=3) midiOutShortMsg((HMIDIOUT)hmido, (*((int32_t *)data)) & masks[count-1]);
    else
    {
        ZeroMemory(&mhdr, sizeof(mhdr));
        mhdr.lpData = data;
        mhdr.dwBufferLength = count;
        midiOutPrepareHeader((HMIDIOUT)hmido, &mhdr, sizeof(MIDIHDR));
        midiOutLongMsg((HMIDIOUT)hmido, &mhdr, sizeof(MIDIHDR));
        while (!(mhdr.dwFlags & MHDR_DONE)) ;
        midiOutUnprepareHeader((HMIDIOUT)hmido, &mhdr, sizeof(MIDIHDR));
    }
}


/*---------------------------------------------------------------------
   Function: MPU_Reset

   Resets the MPU401 card.
---------------------------------------------------------------------*/

int32_t MPU_Reset
(
    void
)

{
    midiStreamStop(hmido);
    midiStreamClose(hmido);

    return MPU_Ok;
}


/*---------------------------------------------------------------------
   Function: MPU_Init

   Detects and initializes the MPU401 card.
---------------------------------------------------------------------*/

int32_t MPU_Init
(
    int32_t addr
)

{
    int32_t i;

    for (i=0; i<NUMBUFFERS; i++) eventcnt[i]=0;

    mididevice = addr;

    if (midiOutGetDevCaps(mididevice, &midicaps, sizeof(MIDIOUTCAPS)) != MMSYSERR_NOERROR) return MPU_Error;

    if (midiStreamOpen(&hmido,(LPUINT)&mididevice,1,(DWORD_PTR)MPU_MIDICallback,0L,CALLBACK_FUNCTION) != MMSYSERR_NOERROR) return MPU_Error;

    return MPU_Ok;
}


/*---------------------------------------------------------------------
   Function: MPU_NoteOff

   Sends a full MIDI note off event out to the music device.
---------------------------------------------------------------------*/

void MPU_NoteOff
(
    int32_t channel,
    int32_t key,
    int32_t velocity
)

{
    char msg[3];
    msg[0] = (MIDI_NOTE_OFF | channel);
    msg[1] = (key);
    msg[2] = (velocity);
    MPU_SendMidi(msg, 3);
}


/*---------------------------------------------------------------------
   Function: MPU_NoteOn

   Sends a full MIDI note on event out to the music device.
---------------------------------------------------------------------*/

void MPU_NoteOn
(
    int32_t channel,
    int32_t key,
    int32_t velocity
)

{
    char msg[3];
    msg[0] = (MIDI_NOTE_ON | channel);
    msg[1] = (key);
    msg[2] = (velocity);
    MPU_SendMidi(msg, 3);
}


/*---------------------------------------------------------------------
   Function: MPU_PolyAftertouch

   Sends a full MIDI polyphonic aftertouch event out to the music device.
---------------------------------------------------------------------*/

void MPU_PolyAftertouch
(
    int32_t channel,
    int32_t key,
    int32_t pressure
)

{
    char msg[3];
    msg[0] = (MIDI_POLY_AFTER_TCH | channel);
    msg[1] = (key);
    msg[2] = (pressure);
    MPU_SendMidi(msg, 3);
}


/*---------------------------------------------------------------------
   Function: MPU_ControlChange

   Sends a full MIDI control change event out to the music device.
---------------------------------------------------------------------*/

void MPU_ControlChange
(
    int32_t channel,
    int32_t number,
    int32_t value
)

{
    char msg[3];
    msg[0] = (MIDI_CONTROL_CHANGE | channel);
    msg[1] = (number);
    msg[2] = (value);
    MPU_SendMidi(msg, 3);
}


/*---------------------------------------------------------------------
   Function: MPU_ProgramChange

   Sends a full MIDI program change event out to the music device.
---------------------------------------------------------------------*/

void MPU_ProgramChange
(
    int32_t channel,
    int32_t program
)

{
    char msg[2];
    msg[0] = (MIDI_PROGRAM_CHANGE | channel);
    msg[1] = (program);
    MPU_SendMidi(msg, 2);
}


/*---------------------------------------------------------------------
   Function: MPU_ChannelAftertouch

   Sends a full MIDI channel aftertouch event out to the music device.
---------------------------------------------------------------------*/

void MPU_ChannelAftertouch
(
    int32_t channel,
    int32_t pressure
)

{
    char msg[2];
    msg[0] = (MIDI_AFTER_TOUCH | channel);
    msg[1] = (pressure);
    MPU_SendMidi(msg, 2);
}


/*---------------------------------------------------------------------
   Function: MPU_PitchBend

   Sends a full MIDI pitch bend event out to the music device.
---------------------------------------------------------------------*/

void MPU_PitchBend
(
    int32_t channel,
    int32_t lsb,
    int32_t msb
)

{
    char msg[3];
    msg[0] = (MIDI_PITCH_BEND | channel);
    msg[1] = (lsb);
    msg[2] = (msb);
    MPU_SendMidi(msg, 3);
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

void MPU_SetVolume(int32_t volume)
{
    /*
    HMIXER hmixer;
    int32_t mixerid;
    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_UNSIGNED mxcdu;
    MMRESULT mme;

    if (mididevice < 0) return;

    mme = mixerOpen(&hmixer, mididevice, 0,0, MIXER_OBJECTF_MIDIOUT);
    if (mme) {
    	puts("Failed opening mixer");
    	return;
    }

    mixerGetID(hmixer, &mixerid, MIXER_OBJECTF_HMIXER);
    printf("mixerid=%d\n",mixerid);

    ZeroMemory(&mxcd,sizeof(mxcd));
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails = (LPVOID)&mxcdu;
    mxcdu.dwValue = (volume << 8) & 0xffff;

    printf("set %d\n",mixerSetControlDetails((HMIXEROBJ)mididevice, &mxcd,
    	MIXER_OBJECTF_MIDIOUT|MIXER_SETCONTROLDETAILSF_VALUE));

    mixerClose(hmixer);
    */
    UNREFERENCED_PARAMETER(volume);
}

int32_t MPU_GetVolume(void)
{
//    if (mididevice < 0) return 0;

    return 0;
}

