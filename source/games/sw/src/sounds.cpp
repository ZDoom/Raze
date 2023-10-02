//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment
Copyright (C) 2019 Christoph Oelckers

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
#include "build.h"

#include "names2.h"

#include "panel.h"
#include "game.h"
#include "sounds.h"
#include "ai.h"
#include "network.h"

#include "misc.h"
#include "misc.h"
#include "rts.h"
#include "menus.h"
#include "razemenu.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "filesystem.h"
#include "serializer.h"
#include "gamecontrol.h"
#include "gamestate.h"
#include "vm.h"

BEGIN_SW_NS

enum EChanExFlags
{
    CHANEXF_NODOPPLER = 0x20000000,
    CHANEXF_DONTPAN = 0x40000000,
};

enum ESoundFlags
{
    SFLAG_PLAYERVOICE = 1,
    SFLAG_PLAYERSPEECH = 2,
    SFLAG_LOOP = 4
};


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

float S_ConvertPitch(int lpitch)
{
    return powf(2, lpitch / 1200.f);
}

//==========================================================================
//
// Sound Distance Calculation
//
//==========================================================================

enum
{
    DECAY_CONST = 4000
};

const int MAXLEVLDIST = 19000;   // The higher the number, the further away you can hear sound

short SoundDist(const DVector3& pos, double basedist)
{
    double tx, ty, tz;
    double sqrdist;
    extern short screenpeek;

    double distance = (getPlayer(screenpeek)->GetActor()->getPosWithOffsetZ() - pos).Length() * 16;

    if (basedist < 0) // if basedist is negative
    {
        double decayshift = 2;
        int decay = abs(int(basedist)) / DECAY_CONST;

        for (int i = 0; i < decay; i++)
            decayshift *= 2;

        if (fabs(basedist / decayshift) >= distance)
            distance = 0;
        else
            distance *= decay;
    }
    else
    {
        if (basedist > distance)
            distance = 0;
        else
            distance -= basedist;
    }

    distance = distance * (256. / MAXLEVLDIST);

    if (distance < 0) distance = 0;
    if (distance > 255) distance = 255;

    return short(distance);
}

//==========================================================================
//
//
// Ambient sounds
//
//
//==========================================================================

struct AmbientSound
{
    DSWActor* spot;
    int ambIndex;
    FSoundID vocIndex;
    int ChanFlags;
    int maxIndex;
    int curIndex;
    bool intermit;
};

static TArray<AmbientSound*> ambients;

//==========================================================================
//
//
//
//==========================================================================

void StopAmbientSound(void)
{
    for (auto amb : ambients)
    {
        soundEngine->StopSound(SOURCE_Ambient, amb, -1);
        delete amb;
    }
    ambients.Clear();
}


//==========================================================================
//
// Play a sound
//
//==========================================================================

void InitAmbient(int num, DSWActor* actor)
{
    int pitch = 0;
    short angle, sound_dist;
    int tx, ty, tz;
    uint8_t priority;
    int maxtics = 0;

    if (!snd_ambience) return;

    // Ambient sounds need special treatment
    if (num < 0 || num > MAX_AMBIENT_SOUNDS)
    {
        if (num != -1) // skip message for -1 to allow using it for silencing buggy ambient sound sprites (there is one in SW level 9.)
        {
            sprintf(ds, "Invalid or out of range ambient sound number %d\n", num);
            PutStringInfo(getPlayer(screenpeek), ds);
        }
        return;
    }
    auto vnum = soundEngine->FindSoundByResID(ambarray[num].diginame);
    if (!soundEngine->isValidSoundId(vnum))
    {
        return; // linked sound does not exist.
    }
    auto sfx = soundEngine->GetSfx(vnum);

    auto amb = new AmbientSound;
    amb->spot = actor;
    amb->ambIndex = num;
    amb->vocIndex = vnum;
    amb->ChanFlags = CHANF_TRANSIENT;
    if (ambarray[num].ambient_flags & v3df_dontpan) amb->ChanFlags |= EChanFlags::FromInt(CHANEXF_DONTPAN);
    if (sfx->UserVal & SFLAG_LOOP) amb->ChanFlags |= CHANF_LOOP;
    amb->maxIndex = ambarray[num].maxtics;
    amb->curIndex = 0;
    amb->intermit = !!(ambarray[num].ambient_flags & v3df_intermit);
    ambients.Push(amb);
}

//==========================================================================
//
//
//
//==========================================================================

void StartAmbientSound(void)
{
    if (!SoundEnabled()) return;

    SWStatIterator it(STAT_AMBIENT);
    while (auto actor = it.Next())
    {
        InitAmbient(actor->spr.lotag, actor);
    }
}

//==========================================================================
//
//
//
//==========================================================================

static void RestartAmbient(AmbientSound* amb)
{
    if (!SoundEnabled()) return;

    auto sfx = soundEngine->GetSfx(amb->vocIndex);
    amb->curIndex = PlayClock;

    if (!soundEngine->IsSourcePlayingSomething(SOURCE_Ambient, amb, CHAN_BODY, amb->vocIndex))
        soundEngine->StartSound(SOURCE_Ambient, amb, nullptr, CHAN_BODY, EChanFlags::FromInt(amb->ChanFlags), amb->vocIndex, 1.f, ATTN_NORM);
}
//==========================================================================
//
//
//
//==========================================================================

static int RandomizeAmbientSpecials(int handle)
{
#define MAXRNDAMB 12
    static const  uint8_t ambrand[] =
    {
        56,57,58,59,60,61,62,63,64,65,66,67
    };
    int i;

    // If ambient sound is found in the array, randomly pick a new sound
    for (i = 0; i < MAXRNDAMB; i++)
    {
        if (handle == ambrand[i])
            return ambrand[StdRandomRange(MAXRNDAMB - 1)];
    }

    return -1;
}

//==========================================================================
//
//
//
//==========================================================================


static void DoTimedSound(AmbientSound* amb)
{
    if (PlayClock >= amb->curIndex + amb->maxIndex)
    {
        if (!soundEngine->IsSourcePlayingSomething(SOURCE_Ambient, amb, CHAN_BODY))
        {
            // Check for special case ambient sounds. Since the sound is stopped and doesn't occupy a real channel at this time we can just swap out the sound ID before restarting it.
            int ambid = RandomizeAmbientSpecials(amb->ambIndex);
            if (ambid != -1)
            {
                amb->vocIndex = soundEngine->FindSoundByResID(ambarray[ambid].diginame);
                amb->maxIndex = StdRandomRange(ambarray[ambid].maxtics);
            }
            RestartAmbient(amb);
        }
    }
}

//==========================================================================
//
// 
//
//==========================================================================

static void UpdateAmbients()
{
    if (!SoundEnabled()) return;

    for (auto& amb : ambients)
    {
        auto spot = amb->spot;
        auto sfx = soundEngine->GetSfx(amb->vocIndex);
        auto sdist = SoundDist(spot->spr.pos, sfx->Rolloff.MinDistance * 16);

        if (sdist < 255 && sfx->ResourceId == DIGI_WHIPME)
        {
            SWPlayer* pp = getPlayer(screenpeek);
            if (!FAFcansee(spot->spr.pos, spot->sector(), pp->GetActor()->getPosWithOffsetZ(), pp->cursector))
            {
                sdist = 255;
            }
        }
        if (sdist < 255)
        {
            if (amb->intermit) DoTimedSound(amb);
            else RestartAmbient(amb);

        }
        else
        {
            soundEngine->StopSound(SOURCE_Ambient, amb, -1);
        }

    }
}

//==========================================================================
//
// end of ambient sounds
//
//==========================================================================




//==========================================================================
//
//
//
//==========================================================================

class SWSoundEngine : public RazeSoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan) override;

public:
    SWSoundEngine()
    {
        S_Rolloff.RolloffType = ROLLOFF_Doom;  
        S_Rolloff.MinDistance = 0;            // These are the default values, SW uses a few different rolloff settings.
        S_Rolloff.MaxDistance = 1187;
    }

    bool SourceIsActor(FSoundChan* chan) override 
    { 
        return chan->SourceType == SOURCE_Actor || chan->SourceType == SOURCE_Unattached; 
    }

    int SoundSourceIndex(FSoundChan* chan) override
    {
        if (chan->SourceType == SOURCE_Player) return int((SWPlayer*)(chan->Source) - *PlayerArray);
        return 0;
    }

    void SetSource(FSoundChan* chan, int index) override
    {
        if (chan->SourceType == SOURCE_Player)
        {
            if (index < 0 || index >= MAX_SW_PLAYERS_REG) index = 0;
            chan->Source = getPlayer(index);
        }
        else chan->Source = nullptr;
    }

    void StopChannel(FSoundChan* chan) override
    {
        if (chan && chan->SysChannel != nullptr && !(chan->ChanFlags & CHANF_EVICTED) && chan->SourceType == SOURCE_Actor)
        {
            chan->Source = nullptr;
            chan->SourceType = SOURCE_Unattached;
        }
        SoundEngine::StopChannel(chan);
    }

};

//==========================================================================
//
//
//
//==========================================================================
void GameInterface::StartSoundEngine()
{
    soundEngine = new SWSoundEngine;
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
        SWPlayer* pp = getPlayer(screenpeek);
        FVector3 campos = GetSoundPos(pp->GetActor() ? pp->GetActor()->getPosWithOffsetZ() : DVector3());
        DVector3 vPos = {};
        bool pancheck = false;

        if (vel) vel->Zero();

        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = pt[1];
            pos->Z = pt[2];
        }
        else if (type == SOURCE_Actor || type == SOURCE_Player)
        {
            if (type == SOURCE_Actor)
            {
                vPos = ((DSWActor*)source)->spr.pos;
            }
            else
            {
                auto act = ((SWPlayer*)source)->GetActor();
                if (act) vPos = act->getPosWithOffsetZ();
                else if (pp->GetActor())
                    vPos = pp->GetActor()->getPosWithOffsetZ();
            }
            pancheck = true;
            FVector3 npos = GetSoundPos(vPos);

            *pos = npos;
#if 0
            if (vel)
            {
                // We do not do doppler effects because none of these old games are set up for it.
                if (chan && !(chanflags & (CHANF_JUSTSTARTED | CHANF_EVICTED)))
                {
                    *vel = (npos - FVector3(pt[0], pt[1], pt[2])) * 40; // SW ticks 40 times a second.
                    chan->Point[0] = npos.X;
                    chan->Point[1] = npos.Y;
                    chan->Point[2] = npos.Z;
                }
            }
#endif
        }
        else if (type == SOURCE_Ambient)
        {
            auto spot = ((AmbientSound*)source)->spot;
            vPos = spot->spr.pos;
            FVector3 npos = GetSoundPos(vPos);
            pancheck = true;

            // Can the ambient sound see the player?  If not, tone it down some.
            if ((chanflags & CHANF_LOOP))
            {
                if (!FAFcansee(vPos, spot->sector(), pp->GetActor()->getPosWithOffsetZ(), pp->cursector))
                {
                    auto distvec = npos - campos;
                    npos = campos + distvec * 1.75f;  // Play more quietly
                }
            }
            *pos = npos;
        }

        if (pancheck && chanflags & CHANEXF_DONTPAN)
        {
            // For unpanned sounds the volume must be set directly and the position taken from the listener.
            *pos = campos;
            auto sfx = soundEngine->GetSfx(chanSound);
            auto sdist = SoundDist(vPos, sfx->Rolloff.MinDistance * 16);
            if (chan) SetVolume(chan, (255 - sdist) * (1 / 255.f));
        }


        if ((chanflags & CHANF_LISTENERZ) && type != SOURCE_None)
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

void GameInterface::UpdateSounds(void)
{
    SWPlayer* pp = getPlayer(screenpeek);
    SoundListener listener;

    DAngle tang;
    if (pp->sop_remote)
    {
        DSWActor* rsp = pp->remoteActor;
        if (rsp && TEST_BOOL1(rsp))
            tang = rsp->spr.Angles.Yaw;
        else
            tang = (pp->sop_remote->pmid.XY() - pp->GetActor()->spr.pos.XY()).Angle();
    }
    else tang = pp->GetActor() ? pp->GetActor()->spr.Angles.Yaw : nullAngle;

    listener.angle = float(-tang.Radians());
    listener.velocity.Zero();
    listener.position = GetSoundPos(pp->GetActor() ? pp->GetActor()->getPosWithOffsetZ() : DVector3());
    listener.underwater = false;
    // This should probably use a real environment instead of the pitch hacking in S_PlaySound3D.
    // listenactor->waterlevel == 3;
    //assert(primaryLevel->Zones.Size() > listenactor->Sector->ZoneNumber);
    listener.Environment = 0;// primaryLevel->Zones[listenactor->Sector->ZoneNumber].Environment;
    listener.valid = true;

    listener.ListenerObject = pp;
    soundEngine->SetListener(listener);

    UpdateAmbients();
}

//==========================================================================
//
// Play a sound
//
//==========================================================================

int _PlaySound(const FSoundID sndid, DSWActor* actor, SWPlayer* pp, const DVector3* const ppos, int flags, int channel, EChanFlags cflags)
{
    if (Prediction || !SoundEnabled() || !soundEngine->isValidSoundId(sndid))
        return -1;

    auto sps = actor;
    auto pos = ppos ? *ppos : DVector3(0, 0, 0);

    auto sfx = soundEngine->GetSfx(sndid);
    int sourcetype = SOURCE_None;
    cflags |= channel == 8 ? CHANF_OVERLAP : CHANF_NONE;  // for the default channel we do not want to have sounds stopping each other.
    void* source = nullptr;
	
    // If the sound is not supposed to be positioned, it may not be linked to the launching actor.
    if (!(flags & v3df_follow)) // use if this is so intermittent that using the flag would break 3D sound.
    {
        if (actor && !ppos)
        {
            pos = actor->spr.pos;
            actor = nullptr;
            sourcetype = SOURCE_Unattached;
        }
        else if (pp && !ppos)
        {
            pos = pp->GetActor()->getPosWithOffsetZ();
            pp = nullptr;
            sourcetype = SOURCE_Unattached;
        }
    }

    if (ppos != nullptr)
    {
        sourcetype = SOURCE_Unattached;
    }
    else if (actor != nullptr)
    {
        source = actor;
        sourcetype = SOURCE_Actor;
    }
    else if (pp != nullptr)
    {
        source = pp;
        sourcetype = SOURCE_Player;
    }
    // Otherwise it's an unpositioned sound.

    //if (flags & v3df_doppler) cflags |= EChanFlags::FromInt(CHANEXF_NODOPPLER);    // intentionally not implemented
    //if (flags & v3df_dontpan) cflags |= EChanFlags::FromInt(CHANEXF_DONTPAN);      // disabled due to poor use
    if (sfx->UserVal & SFLAG_LOOP) cflags |= CHANF_LOOP;                               // with the new sound engine these can just be started and don't have to be stopped ever.

    FVector3 spos = GetSoundPos(pos);
    auto chan = soundEngine->StartSound(sourcetype, source, &spos, channel, cflags, sndid, 1.f, ATTN_NORM);
    if (chan && sourcetype == SOURCE_Unattached) chan->Source = sps; // needed for sound termination.
    return 1;
}

//==========================================================================
//
//
//
//==========================================================================

void PlaySoundRTS(int rts_num)
{
    if (SoundEnabled() && RTS_IsInitialized() && snd_speech)
    {
        auto sid = RTS_GetSoundID(rts_num - 1);
        if (sid.isvalid())
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
    S_SetReverb(amt);
}

//==========================================================================
//
// Deletes vocs in the 3D sound queue with no owners
//
//==========================================================================

void DeleteNoSoundOwner(DSWActor* actor)
{
    if (!soundEngine) return;

    soundEngine->EnumerateChannels([=](FSoundChan* chan)
        {
            if (chan->Source == actor && chan->ChanFlags & CHANF_LOOP)
            {
                soundEngine->StopChannel(chan);
            }
            return false;
        });

    // also delete all ambients attached to this actor.
    for (int i = ambients.Size() - 1; i >= 0; i--)
    {
        auto amb = ambients[i];
        if (amb->spot == actor)
        {
            soundEngine->StopSound(SOURCE_Ambient, amb, -1);
            ambients.Delete(i);
        }
    }
}

//==========================================================================
//
// This is called from KillSprite to kill a follow sound with no valid sprite Owner
// Stops any active sound with the follow bit set, even play once sounds.
//
//==========================================================================

void DeleteNoFollowSoundOwner(DSWActor* actor)
{
    soundEngine->StopSound(SOURCE_Actor, actor, -1); // all non-follow sounds are SOURCE_Unattached
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
// Play a sound using special sprite setup
//
//==========================================================================

void PlaySpriteSound(DSWActor* actor, int attrib_ndx, int flags)
{
    if (actor->hasU() && attrib_ndx > attr_none && attrib_ndx < MAXATTRIBSNDS)
        PlaySound(actor->user.__legacyState.Attrib->Sounds[attrib_ndx - 1], actor, flags);
}

//==========================================================================
//
//
//
//==========================================================================

int _PlayerSound(int num, SWPlayer* pp)
{
    int handle;

    if (Prediction)
        return 0;

    if (pp < *PlayerArray || pp >= getPlayer(MAX_SW_PLAYERS))
    {
        return 0;
    }

    auto sndid = soundEngine->FindSoundByResID(num);
    if (!soundEngine->isValidSoundId(sndid) || !SoundEnabled())
        return 0;

    if (pp->Flags & (PF_DEAD)) return 0; // You're dead, no talking!

    auto sfx = soundEngine->GetSfx(sndid);
    // If this is a player voice and he's already yacking, forget it.

    // Not a player voice, bail.
    if (!(sfx->UserVal & (SFLAG_PLAYERSPEECH|SFLAG_PLAYERVOICE)))
        return 0;

    // Don't talk if not allowed to.
    if ((sfx->UserVal & SFLAG_PLAYERSPEECH) && !snd_speech)
        return 0;

    // The surfacing sound should not block other player speech.
    if (soundEngine->IsSourcePlayingSomething(SOURCE_Player, pp, CHAN_VOICE, soundEngine->FindSoundByResID(DIGI_SURFACE)))
    {
        soundEngine->StopSound(SOURCE_Player, pp, CHAN_VOICE);
    }

    // He wasn't talking, but he will be now.
    if (!soundEngine->IsSourcePlayingSomething(SOURCE_Player, pp, CHAN_VOICE))
    {
        soundEngine->StartSound(SOURCE_Player, pp, nullptr, CHAN_VOICE, 0, soundEngine->FindSoundByResID(num), 1.f, ATTN_NORM);
    }

    return 0;
}

void StopPlayerSound(SWPlayer* pp, int which)
{
    soundEngine->StopSound(SOURCE_Player, pp, CHAN_VOICE, soundEngine->FindSoundByResID(which));
}

bool SoundValidAndActive(DSWActor* spr, int channel)
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

bool PlaySong(const char* song_file_name, int cdaudio_track, bool isThemeTrack) //(nullptr, nullptr, -1, false) starts the normal level music.
{
    // Play  CD audio if enabled.
    if (cdaudio_track >= 0 && (mus_redbook || !song_file_name || *song_file_name == 0))
    {
        FStringf trackname("shadow%02d.ogg", cdaudio_track);
        if (!Mus_Play(trackname, true))
        {
            trackname.Format("track%02d.ogg", cdaudio_track);
            if (!Mus_Play(trackname, true))
            {
                Printf("Can't find CD track %i!\n", cdaudio_track);
            }
        }
        else return true;
    }
    if (!song_file_name || *song_file_name == 0)
    {
        return true;
    }
    if (!Mus_Play(song_file_name, true))
    {
        // try the CD track anyway if no MIDI could be found (the original game doesn't have any MIDI, it was CD Audio only, this avoids no music playing if mus_redbook is off.)
        FStringf trackname("shadow%02d.ogg", cdaudio_track);
        if (!Mus_Play(trackname, true))
        {
            trackname.Format("track%02d.ogg", cdaudio_track);
            if (!Mus_Play(trackname, true)) return false;
        }
    }
    return true;
}

void StopSound(void)
{
    // This gets also called on shutdown.
	StopAmbientSound();
    if (soundEngine) soundEngine->StopAllChannels();
    Mus_Stop();
}

void StopFX()
{
	StopAmbientSound();
    if (soundEngine) soundEngine->StopAllChannels();
}

DEFINE_ACTION_FUNCTION(_SW, PlaySound)
{
    PARAM_PROLOGUE;
    PARAM_SOUND(sound);
    PARAM_INT(vflags);
    PARAM_INT(channel);
    PARAM_INT(cflags);
    _PlaySound(sound, nullptr, nullptr, nullptr, vflags, channel, EChanFlags::FromInt(cflags));
    return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_SW, StopSound, StopSound)
{
    StopSound();
    return 0;
}

DEFINE_ACTION_FUNCTION(_SW, IsSoundPlaying)
{
    PARAM_PROLOGUE;
    PARAM_INT(channel);
    ACTION_RETURN_BOOL(soundEngine->IsSourcePlayingSomething(SOURCE_None, nullptr, channel));

}

DEFINE_ACTION_FUNCTION(_SW, PlaySong)
{
    PARAM_PROLOGUE;
    PARAM_INT(song);
    PlaySong(ThemeSongs[song], ThemeTrack[song], true);
    return 0;
}

END_SW_NS
