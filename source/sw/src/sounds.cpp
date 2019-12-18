//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "compat.h"
#include "build.h"


#include "keys.h"

#include "names2.h"
#include "mytypes.h"
#include "gamedefs.h"
#include "config.h"


#include "panel.h"
#include "game.h"
#include "sounds.h"
#include "ai.h"
#include "network.h"

#include "cache.h"
#include "text.h"
#include "rts.h"
#include "menus.h"
#include "config.h"
#include "menu/menu.h"
#include "z_music.h"
#include "sound/s_soundinternal.h"
#include "filesystem/filesystem.h"

BEGIN_SW_NS

enum EChanExFlags
{
    CHANEXF_AMBIENT = 0x40000000,
    CHANEXF_NODOPPLER = 0x20000000,
    CHANEXF_DONTPAN = 0x10000000,
    CHANEXF_INTERMIT = 0x08000000,

};

// Parentally locked sounds list
int PLocked_Sounds[] =
{
    483,328,334,335,338,478,450,454,452,453,456,457,458,455,460,462,
    461,464,466,465,467,463,342,371,254,347,350,432,488,489,490,76,339,
    499,500,506,479,480,481,482,78,600,467,548,547,544,546,545,542,541,540,
    539,536,529,525,522,521,515,516,612,611,589,625,570,569,567,565,
    558,557
};

//
// Includes digi.h to build the table
//

#define DIGI_TABLE
VOC_INFO voc[] =
{
#include "digi.h"
};

#undef  DIGI_TABLE

//
// Includes ambient.h to build the table of ambient sounds for game
//

#define AMBIENT_TABLE
AMB_INFO ambarray[] =
{
#include "ambient.h"
};
#undef  AMBIENT_TABLE
#define MAX_AMBIENT_SOUNDS 82


class SWSoundEngine : public SoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan) override;
    TArray<uint8_t> ReadSound(int lumpnum);

public:
    SWSoundEngine()
    {
        S_Rolloff.RolloffType = ROLLOFF_Doom;  
        S_Rolloff.MinDistance = 0;            // These are the default values, SW uses a few different rolloff settings.
        S_Rolloff.MaxDistance = 1187;
    }
};

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> SWSoundEngine::ReadSound(int lumpnum)
{
    auto wlump = fileSystem.OpenFileReader(lumpnum);
    return wlump.Read();
}


//==========================================================================
//
//
//
//==========================================================================

void InitFX(void)
{
    if (soundEngine) return; // just in case.
    soundEngine = new SWSoundEngine;

    auto &S_sfx = soundEngine->GetSounds();
    S_sfx.Resize(countof(voc));
    for (auto& sfx : S_sfx) { sfx.Clear(); sfx.lumpnum = sfx_empty; }
    for (size_t i = 1; i < countof(voc); i++)
    {
        auto& entry = voc[i];
        auto lump = fileSystem.FindFile(entry.name);
        if (lump > 0)
        {
            auto& newsfx = S_sfx[i];
            newsfx.name = entry.name;
            newsfx.lumpnum = lump;
            newsfx.NearLimit = 6;
        }
    }
    soundEngine->HashSounds();
    for (auto& sfx : S_sfx)
    {
        soundEngine->CacheSound(&sfx);
    }
}


//==========================================================================
//
// Sound Distance Calculation
//
//==========================================================================

enum
{
    MAXLEVLDIST = 19000,   // The higher the number, the further away you can hear sound
    DECAY_CONST = 4000
};

short SoundDist(int x, int y, int z, int basedist)
{
    double tx, ty, tz;
    double sqrdist, retval;
    extern short screenpeek;

    tx = fabs(Player[screenpeek].posx - x);
    ty = fabs(Player[screenpeek].posy - y);
    tz = fabs((Player[screenpeek].posz - z) >> 4);

    // Use the Pythagreon Theorem to compute the magnitude of a 3D vector
    sqrdist = fabs(tx * tx + ty * ty + tz * tz);
    retval = sqrt(sqrdist);

    if (basedist < 0) // if basedist is negative
    {
        double decayshift = 2;
        int decay = labs(basedist) / DECAY_CONST;

        for (int i = 0; i < decay; i++)
            decayshift *= 2;

        if (fabs(double(basedist) / decayshift) >= retval)
            retval = 0;
        else
            retval *= decay;
    }
    else
    {
        if (basedist > retval)
            retval = 0;
        else
            retval -= basedist;
    }

    retval = retval * 256 / MAXLEVLDIST;

    if (retval < 0) retval = 0;
    if (retval > 255) retval = 255;

    return retval;
}

//==========================================================================
//
// Calculate rolloff info. 
//
//==========================================================================

FRolloffInfo GetRolloff(int basedist)
{
    FRolloffInfo info;

    if (basedist < 0) // if basedist is negative
    {
        double decayshift = 2;
        int decay = labs(basedist) / DECAY_CONST;

        for (int i = 0; i < decay; i++)
            decayshift *= 2;

        info.RolloffType = ROLLOFF_Doom;
        info.MinDistance = (float)(-basedist / decayshift / 16.);
        info.MaxDistance = MAXLEVLDIST / 16.f / decay;
    }
    else
    {
        info.RolloffType = ROLLOFF_Doom;
        info.MinDistance = basedist / 16.f;
        info.MaxDistance = info.MinDistance + MAXLEVLDIST / 16.f;
    }
    return info;
}


//==========================================================================
//
//
//
//==========================================================================

int RandomizeAmbientSpecials(int handle)
{
#define MAXRNDAMB 12
    int ambrand[] =
    {
        56,57,58,59,60,61,62,63,64,65,66,67
    };
    int i;

    // If ambient sound is found in the array, randomly pick a new sound
    for (i = 0; i < MAXRNDAMB; i++)
    {
        if (handle == ambarray[ambrand[i]].diginame)
            return ambrand[STD_RANDOM_RANGE(MAXRNDAMB - 1)];
    }

    return -1;
}

//==========================================================================
//
//
//
//==========================================================================


void DoTimedSound(FSoundChan *chan)
{
    chan->UserData[1] += synctics;  // This got called 5x a second, incrementing by 3 each time, meaning that 15 units are one second. Now it gets called 40x a second.

    if (chan->UserData[1] >= chan->UserData[0] * 8) // account for the 8x higher update frequency.
    {
        if (chan->ChanFlags & CHANF_EVICTED)
        {
            // Check for special case ambient sounds. Since the sound is stopped and doesn't occupy a real channel at this time we can just swap out the sound ID before restarting it.
            int ambid = RandomizeAmbientSpecials(chan->SoundID);
            if (ambid != -1)
            {
                chan->SoundID = FSoundID(ambarray[ambid].maxtics);
                chan->UserData[0] = STD_RANDOM_RANGE(ambarray[ambid].maxtics);
            }

            soundEngine->RestartChannel(chan);
        }

        chan->UserData[1] = 0;
    }
}


//==========================================================================
//
// 
//
//==========================================================================

void SWSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan)
{
    if (pos != nullptr)
    {
        PLAYERp pp = Player + screenpeek;
        FVector3 campos = GetSoundPos((vec3_t*)pp);

        //S_GetCamera(&campos, nullptr, &camsect);
        if (vel) vel->Zero();

        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = pt[1];
            pos->Z = pt[2];
        }
        else if (type == SOURCE_Actor || type == SOURCE_Player)
        {
            vec3_t* vpos = type == SOURCE_Actor ? &((SPRITEp)source)->pos : (vec3_t*)&((PLAYERp)source)->posx;
            FVector3 npos = GetSoundPos(vpos);

            // Can the ambient sound see the player?  If not, tone it down some.
            if ((chanflags & CHANF_LOOP) && (chanflags & CHANEXF_AMBIENT) && type == SOURCE_Actor)
            {
                auto sp = (SPRITEp)source;
                //MONO_PRINT("PlaySound:Checking sound cansee");
                if (!FAFcansee(vpos->x, vpos->y, vpos->z, sp->sectnum, pp->posx, pp->posy, pp->posz, pp->cursectnum))
                {
                    //MONO_PRINT("PlaySound:Reducing sound distance");
                    auto distvec = npos - campos;
                    // Special Cases
                    npos = campos + distvec * 1.75f;  // Play more quietly
                    chanflags |= CHANEXF_NODOPPLER; // Ambient sounds should be stationary, but let's make sure that no doppler gets applied after messing up the position.
                }
            }

            if (chanflags & CHANEXF_DONTPAN)
            {
                // For unpanned sounds the volume must be set directly and the position taken from the listener.
                *pos = campos;
                auto sdist = SoundDist(vpos->x, vpos->y, vpos->z, voc[chanSound].voc_distance);
                SetVolume(chan, (255 - sdist) * (1 / 255.f));
            }
            else
            {
                *pos = npos;
                // Doppler only makes sense for sounds that are actually positioned in the world (i.e. not in combination with DONTPAN)
                if (!(chanflags & CHANEXF_NODOPPLER) && vel)
                {
                    // Hack alert. Velocity may only be set if a) the sound is already running and b) an actual sound channel is modified.
                    // It remains to be seen if this is actually workable. I have my doubts. The velocity should be taken from a stable source.
                    if (chan && !(chanflags & (CHANF_JUSTSTARTED|CHANF_EVICTED)))
                    {
                        *vel = (npos - FVector3(pt[0], pt[1], pt[2])) * 40; // SW ticks 40 times a second.
                        chan->Point[0] = npos.X;
                        chan->Point[1] = npos.Y;
                        chan->Point[2] = npos.Z;
                    }
                }
            }
        }
        if ((chanflags & CHANF_LISTENERZ) && campos != nullptr && type != SOURCE_None)
        {
            pos->Y = campos.Y;
        }
    }
}

//==========================================================================
//
// Main function to update 3D sound array
//
//==========================================================================

void DoUpdateSounds(void)
{
    soundEngine->EnumerateChannels([](FSoundChan* chan)
        {
            if (chan->ChanFlags & EChanFlags::FromInt(CHANEXF_INTERMIT))
            {
                DoTimedSound(chan);
            }

            if (chan->SoundID == DIGI_WHIPME && chan->SourceType == SOURCE_Actor)
            {
                auto sp = (SPRITEp)chan->Source;
                PLAYERp pp = Player + screenpeek;
                if (!FAFcansee(sp->pos.x, sp->pos.y, sp->pos.z, sp->sectnum, pp->posx, pp->posy, pp->posz, pp->cursectnum))
                {
                    soundEngine->EvictChannel(chan);
                }
                else if (chan->ChanFlags & CHANF_EVICTED)
                {
                    soundEngine->RestartChannel(chan);
                }
            }
            return false;
        });
    soundEngine->UpdateSounds((int)totalclock);
}

//==========================================================================
//
// 
//
//==========================================================================

float S_ConvertPitch(int lpitch)
{
    return pow(2, lpitch / 1200.);
}

//==========================================================================
//
// Play a sound
//
//==========================================================================

int _PlaySound(int num, SPRITEp sp, PLAYERp pp, vec3_t *pos, Voc3D_Flags flags, int channel)
{
    VOC_INFOp vp;
    int pitch = 0;
    short angle, sound_dist;
    int tx, ty, tz;
    uint8_t priority;
    int maxtics = 0;
    EChanFlags cflags = channel == 8 ?CHANF_OVERLAP : CHANF_NONE;  // for the default channel we do not want to have sounds stopping each other.

    // Weed out parental lock sounds if PLock is active
    if (adult_lockout || Global_PLock)
    {
        unsigned i;

        for (i=0; i<sizeof(PLocked_Sounds); i++)
        {
            if (num == PLocked_Sounds[i])
                return -1;
        }
    }

    if (Prediction || !SoundEnabled())
        return -1;

    if (TEST(flags,v3df_ambient) && !TEST(flags,v3df_nolookup))  // Look for invalid ambient numbers
    {
        // Ambient sounds need special treatment
        if (!snd_ambience) return -1;
        if (num < 0 || num > MAX_AMBIENT_SOUNDS)
        {
            sprintf(ds,"Invalid or out of range ambient sound number %d\n",num);
            PutStringInfo(Player+screenpeek, ds);
            return -1;
        }
        maxtics = STD_RANDOM_RANGE(ambarray[num].maxtics);

        // If the ambient flag is set, do a name conversion to point to actual
        // digital sound entry.
        flags |= ambarray[num].ambient_flags;   // Add to flags if any
        num = ambarray[num].diginame;
        cflags |= EChanFlags::FromInt(CHANEXF_AMBIENT); // flag the sound as being an ambient sound.
    }
    //else
    {
        if (!soundEngine->isValidSoundId(num))
        {
            return -1;
        }

        vp = &voc[num];
        int sourcetype = SOURCE_None;
        void* source = nullptr;
        // If the sound is not supposd to be positioned, it may not be linked to the launching actor.
        if (!(flags & v3df_follow))
        {
            if (sp && !pos)
            {
                pos = &sp->pos;
                sp = nullptr;
            }
            else if (pp && !pos)
            {
                pos = (vec3_t*)&pp->posx;
                pp = nullptr;
            }
        }

        if (pos != nullptr)
        {
            sourcetype = SOURCE_Unattached;
        }
        else if (sp != nullptr)
        {
            source = sp;
            sourcetype = SOURCE_Actor;
        }
        else if (pp != nullptr)
        {
            source = pp;
            sourcetype = SOURCE_Player;
        }
        // Otherwise it's an unpositioned sound.

        if (!(flags & v3df_doppler))
        {
            cflags |= EChanFlags::FromInt(CHANEXF_NODOPPLER);    // this must ensure that CalcPosVel always zeros the velocity.
        }
        if (flags & v3df_dontpan)
        {
            cflags |= EChanFlags::FromInt(CHANEXF_DONTPAN);      // beware of hackery to emulate this. 
        }
        /*
        if (flags & v3df_init)
        {
            // this only gets used for starting looped sounds that are outside hearing range - something the sound engine handles transparently.
        }
        */
        if (vp->voc_flags & vf_loop)
        {
            cflags |= CHANF_LOOP;                               // with the new sound engine these can just be started and don't have to be stopped ever.
        }

        if (vp->pitch_hi <= vp->pitch_lo)
            pitch = vp->pitch_lo;
        else if (vp->pitch_hi != vp->pitch_lo)
            pitch = vp->pitch_lo + (STD_RANDOM_RANGE(vp->pitch_hi - vp->pitch_lo));

        float fpitch = S_ConvertPitch(pitch);

        auto rolloff = GetRolloff(vp->voc_distance);
        auto spos = GetSoundPos(pos);
        auto chan = soundEngine->StartSound(sourcetype, source, &spos, CHAN_BODY, cflags, num, 1.f, ATTN_NORM, &rolloff, fpitch);
        if (chan)
        {
            if (flags & v3df_intermit)
            {
                chan->ChanFlags |= CHANF_VIRTUAL | EChanFlags::FromInt(CHANEXF_INTERMIT);   // for intermittent sounds this must be set after starting the sound so that it actually plays.
            }
            chan->UserData[0] = maxtics; // counter for intermittent delay.
            chan->UserData[1] = 0;
        }
    }

    return 1;
}

//==========================================================================
//
//
//
//==========================================================================

void PlaySoundRTS(int rts_num)
{
    if (!adult_lockout && SoundEnabled() && RTS_IsInitialized() && snd_speech)
    {
        auto sid = RTS_GetSoundID(rts_num - 1);
        if (sid != -1)
        {
            soundEngine->StartSound(SOURCE_Unattached, nullptr, nullptr, CHAN_VOICE, 0, sid, 0.8f, ATTN_NONE);
        }
    }
}

//==========================================================================
//
//
//
//==========================================================================

void COVER_SetReverb(int amt)
{
    FX_SetReverb(amt);
}

//==========================================================================
//
// Deletes vocs in the 3D sound queue with no owners
//
//==========================================================================

void DeleteNoSoundOwner(short spritenum)
{
    SPRITEp sp = &sprite[spritenum];

    soundEngine->EnumerateChannels([=](FSoundChan* chan)
        {
            if (chan->Source == sp && chan->ChanFlags & CHANF_LOOP)
            {
                soundEngine->StopChannel(chan);
            }
            return false;
        });
}

//==========================================================================
//
// This is called from KillSprite to kill a follow sound with no valid sprite owner
// Stops any active sound with the follow bit set, even play once sounds.
//
//==========================================================================

void DeleteNoFollowSoundOwner(short spritenum)
{
    SPRITEp sp = &sprite[spritenum];
    soundEngine->StopSound(SOURCE_Actor, sp, -1); // all non-follow sounds are SOURCE_Unattached
}

//==========================================================================
//
//
//
//==========================================================================

void StopAmbientSound(void)
{
    soundEngine->EnumerateChannels([](FSoundChan* chan)
        {
            if (chan->ChanFlags & EChanFlags::FromInt(CHANEXF_AMBIENT))
            {
                soundEngine->StopChannel(chan);
            }
            return false;
        });
}


//==========================================================================
//
//
//
//==========================================================================

void StartAmbientSound(void)
{
    short i, nexti;
    extern SWBOOL InMenuLevel;

    if (InMenuLevel) return; // Don't restart ambience if no level is active! Will crash game.

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_AMBIENT], i, nexti)
    {
        SPRITEp sp = &sprite[i];

        PlaySound(sp->lotag, sp, v3df_ambient | v3df_init | v3df_doppler | v3df_follow);
    }
}

//==========================================================================
//
// Terminate the sounds list
//
//==========================================================================

void Terminate3DSounds(void)
{
    soundEngine->EnumerateChannels([](FSoundChan* chan)
        {
            if (chan->SourceType == SOURCE_Actor || chan->SourceType == SOURCE_Unattached ||
                (chan->SourceType == SOURCE_Player && chan->EntChannel != CHAN_VOICE))
            {
                soundEngine->StopChannel(chan);
            }
            return false;
        });
}


//==========================================================================
//
// no longer needed, only left to avoid changing the game code
//
//==========================================================================

void Set3DSoundOwner(short spritenum)
{
    
}

//==========================================================================
//
// Play a sound using special sprite setup
//
//==========================================================================

void PlaySpriteSound(short spritenum, int attrib_ndx, Voc3D_Flags flags)
{
    SPRITEp sp = &sprite[spritenum];
    USERp u = User[spritenum];

    ASSERT(u);

    PlaySound(u->Attrib->Sounds[attrib_ndx], sp, flags);
}

//==========================================================================
//
//
//
//==========================================================================

int _PlayerSound(int num, PLAYERp pp)
{
    int handle;
    VOC_INFOp vp;

    if (Prediction)
        return 0;

    if (pp < Player || pp >= Player + MAX_SW_PLAYERS)
    {
        return 0;
    }

    if (num < 0 || num >= DIGI_MAX || !soundEngine->isValidSoundId(num))
        return 0;

    if (TEST(pp->Flags, PF_DEAD)) return 0; // You're dead, no talking!

    // If this is a player voice and he's already yacking, forget it.
    vp = &voc[num];

    // Not a player voice, bail.
    if (vp->priority != PRI_PLAYERVOICE && vp->priority != PRI_PLAYERDEATH)
        return 0;

    // He wasn't talking, but he will be now.
    if (!soundEngine->IsSourcePlayingSomething(SOURCE_Player, pp, CHAN_VOICE))
    {
        soundEngine->StartSound(SOURCE_Player, pp, nullptr, CHAN_VOICE, 0, num, 1.f, ATTN_NORM);
    }

    return 0;
}

void StopPlayerSound(PLAYERp pp)
{
    soundEngine->StopSound(SOURCE_Player, pp, CHAN_VOICE);
}

bool SoundValidAndActive(SPRITEp spr, int channel)
{
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, spr, channel);
}


/*
============================================================================
=
= High level sound code (not directly engine related)
=
============================================================================
*/

int PlayerPainVocs[] =
{
    DIGI_PLAYERPAIN1,
    DIGI_PLAYERPAIN2,
    DIGI_PLAYERPAIN3,
    DIGI_PLAYERPAIN4,
    DIGI_PLAYERPAIN5
};

// Don't have these sounds yet
int PlayerLowHealthPainVocs[] =
{
    DIGI_HURTBAD1,
    DIGI_HURTBAD2,
    DIGI_HURTBAD3,
    DIGI_HURTBAD4,
    DIGI_HURTBAD5
};

int TauntAIVocs[] =
{
    DIGI_TAUNTAI1,
    DIGI_TAUNTAI2,
    DIGI_TAUNTAI3,
    DIGI_TAUNTAI4,
    DIGI_TAUNTAI5,
    DIGI_TAUNTAI6,
    DIGI_TAUNTAI7,
    DIGI_TAUNTAI8,
    DIGI_TAUNTAI9,
    DIGI_TAUNTAI10,
    DIGI_COWABUNGA,
    DIGI_NOCHARADE,
    DIGI_TIMETODIE,
    DIGI_EATTHIS,
    DIGI_FIRECRACKERUPASS,
    DIGI_HOLYCOW,
    DIGI_HAHA2,
    DIGI_HOLYPEICESOFCOW,
    DIGI_HOLYSHIT,
    DIGI_HOLYPEICESOFSHIT,
    DIGI_PAYINGATTENTION,
    DIGI_EVERYBODYDEAD,
    DIGI_KUNGFU,
    DIGI_HOWYOULIKEMOVE,
    DIGI_HAHA3,
    DIGI_NOMESSWITHWANG,
    DIGI_RAWREVENGE,
    DIGI_YOULOOKSTUPID,
    DIGI_TINYDICK,
    DIGI_NOTOURNAMENT,
    DIGI_WHOWANTSWANG,
    DIGI_MOVELIKEYAK,
    DIGI_ALLINREFLEXES
};

int PlayerGetItemVocs[] =
{
    DIGI_GOTITEM1,
    DIGI_HAHA1,
    DIGI_BANZAI,
    DIGI_COWABUNGA,
    DIGI_TIMETODIE
};

int PlayerYellVocs[] =
{
    DIGI_PLAYERYELL1,
    DIGI_PLAYERYELL2,
    DIGI_PLAYERYELL3
};


//==========================================================================
//
// PLays music
//
//==========================================================================

extern short Level;
CVAR(Bool, sw_nothememidi, false, CVAR_ARCHIVE)

SWBOOL PlaySong(const char* mapname, const char* song_file_name, int cdaudio_track, bool isThemeTrack) //(nullptr, nullptr, -1, false) starts the normal level music.
{
    if (mapname == nullptr && song_file_name == nullptr && cdaudio_track == -1)
    {
        // Get the music defined for the current level.

    }
    // Play  CD audio if enabled.
    if (cdaudio_track >= 0 && mus_redbook)
    {
        FStringf trackname("track%02d.ogg", cdaudio_track);
        if (!Mus_Play(nullptr, trackname, true))
        {
            buildprintf("Can't find CD track %i!\n", cdaudio_track);
        }
    }
    else if (isThemeTrack && sw_nothememidi) return false;   // The original SW source only used CD Audio for theme tracks, so this is optional.
    return Mus_Play(nullptr, song_file_name, true);
}

void StopSound(void)
{
    soundEngine->StopAllChannels();
    Mus_Stop();
}


END_SW_NS
