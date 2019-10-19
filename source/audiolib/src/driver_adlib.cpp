/**
 * Adlib MIDI output
 */

#include "driver_adlib.h"

#include "al_midi.h"
#include "compat.h"
#include "midifuncs.h"
#include "midi.h"
#include "_multivc.h"

enum {
   AdlibErr_Warning = -2,
   AdlibErr_Error   = -1,
   AdlibErr_Ok      = 0,
   AdlibErr_Uninitialised,
};

static int ErrorCode = AdlibErr_Ok;

int AdlibDrv_GetError(void)
{
    return ErrorCode;
}

const char *AdlibDrv_ErrorString( int ErrorNumber )
{
    const char *ErrorString;
    
    switch( ErrorNumber )
    {
        case AdlibErr_Warning :
        case AdlibErr_Error :
            ErrorString = AdlibDrv_ErrorString( ErrorCode );
            break;

        case AdlibErr_Ok :
            ErrorString = "Adlib ok.";
            break;
            
        case AdlibErr_Uninitialised:
            ErrorString = "Adlib uninitialised.";
            break;

        default:
            ErrorString = "Unknown Adlib error.";
            break;
    }
        
    return ErrorString;
}

int AdlibDrv_MIDI_Init(midifuncs *funcs)
{
    AdlibDrv_MIDI_Shutdown();
    Bmemset(funcs, 0, sizeof(midifuncs));

    funcs->NoteOff           = OPLMusic::AL_NoteOff;
    funcs->NoteOn            = OPLMusic::AL_NoteOn;
    funcs->PolyAftertouch    = NULL;
    funcs->ControlChange     = OPLMusic::AL_ControlChange;
    funcs->ProgramChange     = OPLMusic::AL_ProgramChange;
    funcs->ChannelAftertouch = NULL;
    funcs->PitchBend         = OPLMusic::AL_SetPitchBend;
    
    return AdlibErr_Ok;
}

void AdlibDrv_MIDI_Shutdown(void)
{
    AdlibDrv_MIDI_HaltPlayback();
}

int AdlibDrv_MIDI_StartPlayback(void (*service)(void))
{
    AdlibDrv_MIDI_HaltPlayback();

    OPLMusic::AL_Init(MV_MixRate);
    MV_HookMusicRoutine(service);

    return MIDI_Ok;
}

void AdlibDrv_MIDI_HaltPlayback(void)
{
    MV_UnhookMusicRoutine();
}

void AdlibDrv_MIDI_SetTempo(int tempo, int division)
{
    MV_MIDIRenderTempo = (tempo * division)/60;
    MV_MIDIRenderTimer = 0;
}

void AdlibDrv_MIDI_Lock(void)
{
}

void AdlibDrv_MIDI_Unlock(void)
{
}

