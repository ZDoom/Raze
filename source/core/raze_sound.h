#pragma once
#include "s_soundinternal.h"

enum
{
	SOURCE_Actor = SOURCE_None+1,		// Sound is coming from an actor.
	SOURCE_Ambient,		// Sound is coming from a blood ambient definition.
	SOURCE_Player,		// SW player sound (player in SW maintains its own position separately from the sprite so needs to be special.)
	SOURCE_Swirly,		// Special stuff for Exhumed. (local sound with custom panning)
	SOURCE_EXBoss,		// Another special case for Exhumed.
};


inline void FX_StopAllSounds(void) 
{ 
	soundEngine->StopAllChannels();
}

void FX_SetReverb(int strength);

inline void FX_SetReverbDelay(int delay) 
{ 
}

inline int S_FindSoundByResID(int ndx)
{
	return soundEngine->FindSoundByResID(ndx);
}

inline int S_FindSound(const char* name)
{
	return soundEngine->FindSound(name);
}

int S_LookupSound(const char* fn);
class FSerializer;
void S_SerializeSounds(FSerializer& arc);
