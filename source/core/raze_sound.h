#pragma once
#include "s_soundinternal.h"
#include "m_fixed.h"
#include "vectors.h"
#include "build.h"

inline FVector3 GetSoundPos(const vec3_t *pos)
{
    // converts a Build coordinate to a sound system coordinate
    const float xmul = 1 / 16.f;
    const float ymul = -1 / 16.f;
    const float zmul = -1 / 256.f;
    return { pos->x* xmul, pos->z* zmul, pos->y* ymul };
}


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

int S_LookupSound(const char* fn);
class FSerializer;
void S_SerializeSounds(FSerializer& arc);
