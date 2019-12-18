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
#include "fx_man.h"
#include "music.h"
#include "al_midi.h"
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


//==========================================================================
//
//
//
//==========================================================================

void InitFX(void)
{
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





extern USERp User[MAXSPRITES];
void DumpSounds(void);



// Global vars used by ambient sounds to set spritenum of ambient sounds for later lookups in
// the sprite array so FAFcansee can know the sound sprite's current sector location
SWBOOL Use_SoundSpriteNum = FALSE;
int16_t SoundSpriteNum = -1;  // Always set this back to -1 for proper validity checking!


////////////////////////////////////////////////////////////////////////////
// Play a sound
////////////////////////////////////////////////////////////////////////////

int RandomizeAmbientSpecials(int handle)
{
#define MAXRNDAMB 12
    int ambrand[] =
    {
        56,57,58,59,60,61,62,63,64,65,66,67
    };
    short i;

    // If ambient sound is found in the array, randomly pick a new sound
    for (i = 0; i < MAXRNDAMB; i++)
    {
        if (handle == ambrand[i])
            return ambrand[STD_RANDOM_RANGE(MAXRNDAMB - 1)];
    }

    return handle;   // Give back the sound, no new one was found
}


void
DoTimedSound(VOC3D_INFOp p)
{
    p->tics += synctics;

    if (p->tics >= p->maxtics)
    {
        if (!FX_SoundValidAndActive(p->handle))
        {
            // Check for special case ambient sounds
            p->num = RandomizeAmbientSpecials(p->num);

            // Sound was bumped from active sounds list, try to play again.
            // Don't bother if voices are already maxed out.
            if (FX_SoundsPlaying() < snd_numvoices)
            {
                if (p->flags & v3df_follow)
                {
                    //PlaySound(p->num, p->x, p->y, p->z, p->flags); todo
                    p->deleted = TRUE;  // Mark old sound for deletion
                }
                else
                {
                    //PlaySound(p->num, &p->fx, &p->fy, &p->fz, p->flags); todo
                    p->deleted = TRUE;  // Mark old sound for deletion
                }
            }
        }

        p->tics = 0;
    }
}


///////////////////////////////////////////////
// Main function to update 3D sound array
///////////////////////////////////////////////
typedef struct
{
    VOC3D_INFOp p;
    short dist;
    uint8_t priority;
} TVOC_INFO, * TVOC_INFOp;

void
DoUpdateSounds(void)
{
    VOC3D_INFOp p;
    SWBOOL looping;
    int pitch = 0, pitchmax;
    int delta;
    short dist, angle;
    SWBOOL deletesound = FALSE;

    TVOC_INFO TmpVocArray[32];
    int i;

    // Zero out the temporary array
    // Zero out the temporary array
    //memset(&TmpVocArray[0],0,sizeof(TmpVocArray));
    for (i = 0; i < 32; i++)
    {
        TmpVocArray[i].p = NULL;
        TmpVocArray[i].dist = 0;
        TmpVocArray[i].priority = 0;
    }

    p = nullptr;// voc3dstart;

    while (p)
    {
        ASSERT(p->num >= 0 && p->num < DIGI_MAX);

        looping = p->vp->voc_flags & vf_loop;

        //      //DSPRINTF(ds,"sound %d FX_SoundActive = %d\n,",p->num,FX_SoundActive(p->handle));
        //      MONO_PRINT(ds);

                // If sprite owner is dead, kill this sound as long as it isn't ambient
        if (looping && p->owner == -1 && !TEST(p->flags, v3df_ambient))
        {
            SET(p->flags, v3df_kill);
        }

        // Is the sound slated for death? Kill it, otherwise play it.
        if (p->flags & v3df_kill)
        {
            if (FX_SoundValidAndActive(p->handle))
            {
                FX_StopSound(p->handle); // Make sure to stop active sounds
                p->handle = 0;
            }

            //DSPRINTF(ds,"%d had v3df_kill.\n",p->num);
            //MONO_PRINT(ds);
            p->deleted = TRUE;
        }
        else
        {
            if (!FX_SoundValidAndActive(p->handle) && !looping)
            {
                if (p->flags & v3df_intermit)
                {
                    DoTimedSound(p);
                }
                else
                    //if(p->owner == -1 && !TEST(p->flags,v3df_ambient))
                {
                    //DSPRINTF(ds,"%d is now inactive.\n",p->num);
                    //MONO_PRINT(ds);
                    p->deleted = TRUE;
                }
            }
            else if (FX_SoundValidAndActive(p->handle))
            {
                if (p->flags & v3df_follow)
                {
                    dist = SoundDist(*p->x, *p->y, *p->z, p->vp->voc_distance);
                    angle = SoundAngle(*p->x, *p->y);
                }
                else
                {
                    if (p->fx == 0 && p->fy == 0 && p->fz == 0)
                        dist = 0;
                    else
                        dist = SoundDist(p->fx, p->fy, p->fz, p->vp->voc_distance);
                    angle = SoundAngle(p->fx, p->fy);
                }

                // Can the ambient sound see the player?  If not, tone it down some.
                if ((p->vp->voc_flags & vf_loop) && p->owner != -1)
                {
                    PLAYERp pp = Player + screenpeek;
                    SPRITEp sp = &sprite[p->owner];

                    //MONO_PRINT("Checking sound cansee");
                    if (!FAFcansee(sp->x, sp->y, sp->z, sp->sectnum, pp->posx, pp->posy, pp->posz, pp->cursectnum))
                    {
                        //MONO_PRINT("Reducing sound distance");
                        dist += ((dist / 2) + (dist / 4));  // Play more quietly
                        if (dist > 255) dist = 255;

                        // Special cases
                        if (p->num == 76 && TEST(p->flags, v3df_ambient))
                        {
                            dist = 255; // Cut off whipping sound, it's secret
                        }

                    }
                }

                if (dist >= 255 && p->vp->voc_distance == DIST_NORMAL)
                {
                    FX_StopSound(p->handle);    // Make sure to stop active
                    p->handle = 0;
                    // sounds
                }
                else
                {
                    // Handle Panning Left and Right
                    if (!(p->flags & v3df_dontpan))
                        FX_Pan3D(p->handle, angle, dist);
                    else
                        FX_Pan3D(p->handle, 0, dist);

                    // Handle Doppler Effects
#define DOPPLERMAX  400
                    if (!(p->flags & v3df_doppler) && FX_SoundActive(p->handle))
                    {
                        pitch -= (dist - p->doplr_delta);

                        if (p->vp->pitch_lo != 0 && p->vp->pitch_hi != 0)
                        {
                            if (abs(p->vp->pitch_lo) > abs(p->vp->pitch_hi))
                                pitchmax = abs(p->vp->pitch_lo);
                            else
                                pitchmax = abs(p->vp->pitch_hi);

                        }
                        else
                            pitchmax = DOPPLERMAX;

                        if (pitch > pitchmax)
                            pitch = pitchmax;
                        if (pitch < -pitchmax)
                            pitch = -pitchmax;

                        p->doplr_delta = dist;  // Save new distance to
                        // struct
                        FX_SetPitch(p->handle, pitch);
                    }
                }
            }
            else if (!FX_SoundValidAndActive(p->handle) && looping)
            {
                if (p->flags & v3df_follow)
                {
                    dist = SoundDist(*p->x, *p->y, *p->z, p->vp->voc_distance);
                    angle = SoundAngle(*p->x, *p->y);
                }
                else
                {
                    dist = SoundDist(p->fx, p->fy, p->fz, p->vp->voc_distance);
                    angle = SoundAngle(p->fx, p->fy);
                }

                // Sound was bumped from active sounds list, try to play
                // again.
                // Don't bother if voices are already maxed out.
                // Sort looping vocs in order of priority and distance
                //if (FX_SoundsPlaying() < snd_numvoices && dist <= 255)
                if (dist <= 255)
                {
                    for (i = 0; i < min((int)SIZ(TmpVocArray), *snd_numvoices); i++)
                    {
                        if (p->priority >= TmpVocArray[i].priority)
                        {
                            if (!TmpVocArray[i].p || dist < TmpVocArray[i].dist)
                            {
                                ASSERT(p->num >= 0 && p->num < DIGI_MAX);
                                TmpVocArray[i].p = p;
                                TmpVocArray[i].dist = dist;
                                TmpVocArray[i].priority = p->priority;
                                break;
                            }
                        }
                    }
                }
            }                       // !FX_SoundActive
        }                           // if(p->flags & v3df_kill)

        p = p->next;
    }                               // while(p)

    // Process all the looping sounds that said they wanted to get back in
    // Only update these sounds 5x per second!  Woo hoo!, aren't we optimized now?
    //if(MoveSkip8==0)
    //    {
    for (i = 0; i < min((int)SIZ(TmpVocArray), *snd_numvoices); i++)
    {
        int handle;

        p = TmpVocArray[i].p;

        //if (FX_SoundsPlaying() >= snd_numvoices || !p) break;
        if (!p) break;

        ASSERT(p->num >= 0 && p->num < DIGI_MAX);

        if (p->flags & v3df_follow)
        {
            if (p->owner == -1)
            {
                // Terminate the sound without aborting.
                continue;
            }

            Use_SoundSpriteNum = TRUE;
            SoundSpriteNum = p->owner;

            //handle = PlaySound(p->num, p->x, p->y, p->z, p->flags); todo
            //if(handle >= 0 || TEST(p->flags,v3df_ambient)) // After a valid PlaySound, it's ok to use voc3dend
            //voc3dend->owner = p->owner; // todo Transfer the owner
            p->deleted = TRUE;

            Use_SoundSpriteNum = FALSE;
            SoundSpriteNum = -1;

            //MONO_PRINT("TmpVocArray playing a follow sound");
        }
        else
        {
            if (p->owner == -1)
            {
                // Terminate the sound without aborting.
                continue;
            }

            Use_SoundSpriteNum = TRUE;
            SoundSpriteNum = p->owner;

            //handle = PlaySound(p->num, &p->fx, &p->fy, &p->fz, p->flags); todo
            //if(handle >= 0 || TEST(p->flags,v3df_ambient))
            //voc3dend->owner = p->owner; // todo Transfer the owner
            p->deleted = TRUE;

            Use_SoundSpriteNum = FALSE;
            SoundSpriteNum = -1;
        }
    }

}


/*


    // Can the ambient sound see the player?  If not, tone it down some.
    if ((vp->voc_flags & vf_loop) && Use_SoundSpriteNum && SoundSpriteNum >= 0)
    {
        PLAYERp pp = Player+screenpeek;

        //MONO_PRINT("PlaySound:Checking sound cansee");
        if (!FAFcansee(tx, ty, tz, sp->sectnum,pp->posx, pp->posy, pp->posz, pp->cursectnum))
        {
            //MONO_PRINT("PlaySound:Reducing sound distance");
            sound_dist += ((sound_dist/2)+(sound_dist/4));  // Play more quietly
            if (sound_dist > 255) sound_dist = 255;

            // Special Cases
            if (num == DIGI_WHIPME) sound_dist = 255;
        }
    }
*/




////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

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
    VOC3D_INFOp v3p;
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
        if (flags & v3df_init)
        {
            cflags |= CHANF_VIRTUAL;                            // don't start right away but keep the channel around until explicitly deleted.
        }
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
            chan->UserData = maxtics; // counter for intermittent delay.
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
// Stops and active sound with the follow bit set, even play once sounds.
//
//==========================================================================

void DeleteNoFollowSoundOwner(short spritenum)
{
    SPRITEp sp = &sprite[spritenum];
    soundEngine->StopSound(SOURCE_Actor, sp, -1);
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
    VOC3D_INFOp p;
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
