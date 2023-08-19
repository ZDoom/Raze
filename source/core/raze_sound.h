#pragma once
#include "s_soundinternal.h"
#include "m_fixed.h"
#include "vectors.h"
#include "build.h"

inline FVector3 GetSoundPos(const DVector3& pos)
{
    return { float(pos.X), float(-pos.Z), float(-pos.Y) };
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
int S_ReserveSoundSlot(const char* logicalname, int slotnum, int limit = 6);
void S_CacheAllSounds(void);

class RazeSoundEngine : public SoundEngine
{
public:
	RazeSoundEngine()
	{
		// add the empty sound right now.
		AddSoundLump("no_sound", fileSystem.CheckNumForFullName("engine/dsempty.lmp"), 0);
	}
	virtual bool SourceIsActor(FSoundChan* chan) { return chan->SourceType == SOURCE_Actor; }
	virtual int SoundSourceIndex(FSoundChan* chan) { return 0; }
	virtual void SetSource(FSoundChan* chan, int index) {}
	std::vector<uint8_t> ReadSound(int lumpnum) override;
};
