#include "ns.h"
#include "wh.h"
#include "raze_sound.h"

BEGIN_WH_NS

class WHSoundEngine : public SoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan *channel) override;
    TArray<uint8_t> ReadSound(int lumpnum) override;

public:
    WHSoundEngine()
    {
        S_Rolloff.RolloffType = ROLLOFF_Doom;
        S_Rolloff.MinDistance = 170;            // these are the numbers I got when uncrunching the original sound code.
        S_Rolloff.MaxDistance = 850;
    }

    void StopChannel(FSoundChan* chan) override
    {
        if (chan && chan->SysChannel != NULL && !(chan->ChanFlags & CHANF_EVICTED) && chan->SourceType == SOURCE_Actor)
        {
            chan->Source = NULL;
            chan->SourceType = SOURCE_Unattached;
        }
        SoundEngine::StopChannel(chan);
    }



};

void sfxInit(void)
{
    soundEngine = new WHSoundEngine;
}

void sfxTerm()
{
}

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> WHSoundEngine::ReadSound(int lumpnum)
{
    auto wlump = fileSystem.OpenFileReader(lumpnum);
    return wlump.Read();
}

void WHSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan *)
{
#if 0
    if (pos != nullptr && type != SOURCE_None)
    {
        FVector3 camera;
        
        if (gMe && gMe->pSprite) camera = GetSoundPos(&gMe->pSprite->pos);
        else camera = { 0, 0, 0 }; // don't crash if there is no player.

        if (vel) vel->Zero();

        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = pt[1];
            pos->Z = pt[2];
        }
        else if (type == SOURCE_Actor)
        {
            auto actor = (spritetype*)source;
            assert(actor != nullptr);
            size_t index = actor - sprite;
            // Engine expects velocity in units per second, not units per tic.
            if (vel) *vel = { xvel[index] * (30 / 65536.f), zvel[index] * (-30 / 65536.f), yvel[index] * (-30 / 65536.f) };
            *pos = GetSoundPos(&actor->pos);
        }
        else if (type == SOURCE_Ambient)
        {
            *pos = camera; // just to be safe. Ambient sounds are in the world but unpositioned
        }
        if ((chanflags & CHANF_LISTENERZ))
        {
            pos->Y = camera.Y;
        }
    }
#endif
}


END_WH_NS