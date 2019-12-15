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

#include "ns.h"	// Must come before everything else!

#include "compat.h"

#include "duke3d.h"
#include "z_music.h"
#include "mapinfo.h"
#include "sound/s_soundinternal.h"

BEGIN_DUKE_NS

int32_t g_highestSoundIdx;


class DukeSoundEngine : public SoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel) override;
    TArray<uint8_t> ReadSound(int lumpnum);

public:
    DukeSoundEngine() = default;
};

//==========================================================================
//
// This is to avoid hardscoding the dependency on Wads into the sound engine
// 
//==========================================================================

TArray<uint8_t> DukeSoundEngine::ReadSound(int lumpnum)
{
    auto wlump = fileSystem.OpenFileReader(lumpnum);
    return wlump.Read();
}

//==========================================================================
//
// 
//
//==========================================================================

void S_PauseSounds(bool paused)
{
    soundEngine->SetPaused(paused);
}

//==========================================================================
//
// 
//
//==========================================================================

void cacheAllSounds(void)
{
    for (int i=0, j=0; i < MAXSOUNDS; ++i)
    {
        soundEngine->CacheSound(i);
        if ((i&31) == 0)
            gameHandleEvents();
    }
}

//==========================================================================
//
// 
//
//==========================================================================

static inline int S_GetPitch(int num)
{
    auto const* snd = (sound_t*)soundEngine->GetUserData(num);
    int const   range = abs(snd->pitchEnd - snd->pitchStart);

    return (range == 0) ? snd->pitchStart : min(snd->pitchStart, snd->pitchEnd) + rand() % range;
}

float S_ConvertPitch(int lpitch)
{
    return pow(2, lpitch / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
}

int S_GetUserFlags(int sndnum)
{
    return ((sound_t*)soundEngine->GetUserData(sndnum + 1))->flags;
}

//==========================================================================
//
// 
//
//==========================================================================

int S_DefineSound(unsigned index, const char *filename, int minpitch, int maxpitch, int priority, int type, int distance, float volume)
{
    if ((unsigned)index >= MAXSOUNDS)
        return -1;

    auto& S_sfx = soundEngine->GetSounds();
    index++;
    unsigned oldindex = S_sfx.Size();
    if (index >= S_sfx.Size())
    {
        S_sfx.Resize(index + 1);
        for (; oldindex <= index; oldindex++)
        {
            S_sfx[oldindex].Clear();
        }
    }
    auto sfx = &S_sfx[index];
    bool alreadydefined = !sfx->bTentative;
    sfx->UserData.Resize(sizeof(sound_t));
    auto sndinf = (sound_t*)sfx->UserData.Data();
    sndinf->flags = type & ~SF_ONEINST_INTERNAL;
    if (sndinf->flags & SF_LOOP)
        sndinf->flags |= SF_ONEINST_INTERNAL;

    sndinf->pitchStart = clamp(minpitch, INT16_MIN, INT16_MAX);
    sndinf->pitchEnd = clamp(maxpitch, INT16_MIN, INT16_MAX);
    sndinf->priority = priority & 255;
    sndinf->volAdjust = clamp(distance, INT16_MIN, INT16_MAX);
    sfx->Volume = volume;
    return 0;
}


//==========================================================================
//
// 
//
//==========================================================================

static int S_CalcDistAndAng(int spriteNum, int soundNum, int sectNum,
                             const vec3_t *cam, const vec3_t *pos, int *distPtr, FVector3 *sndPos)
{
    // Todo: Some of this hackery really should be done using rolloff and attenuation instead of messing around with the sound origin.
    int orgsndist = 0, sndang = 0, sndist = 0, explosion = 0;
    auto const* snd = (sound_t*)soundEngine->GetUserData(soundNum);
    int userflags = snd->flags;
    int dist_adjust = snd->volAdjust;

    if (PN(spriteNum) == APLAYER && P_Get(spriteNum) == screenpeek)
        goto sound_further_processing;

    orgsndist = sndist = FindDistance3D(cam->x-pos->x, cam->y-pos->y, (cam->z-pos->z));

    if ((userflags & (SF_GLOBAL|SF_DTAG)) != SF_GLOBAL && S_IsAmbientSFX(spriteNum) && (sector[SECT(spriteNum)].lotag&0xff) < 9)  // ST_9_SLIDING_ST_DOOR
        sndist = divscale14(sndist, SHT(spriteNum)+1);

sound_further_processing:
    sndist += dist_adjust;
    if (sndist < 0)
        sndist = 0;

    if (!FURY && sectNum > -1 && sndist && PN(spriteNum) != MUSICANDSFX
        && !cansee(cam->x, cam->y, cam->z - (24 << 8), sectNum, SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (24 << 8), SECT(spriteNum)))
        sndist += sndist>>5;

    if ((userflags & (SF_GLOBAL|SF_DTAG)) == (SF_GLOBAL|SF_DTAG))
    {
boost:
        int const sdist = dist_adjust ? dist_adjust : 6144;

        explosion = true;

        if (sndist > sdist)
            sndist = sdist;
    }
    else if (!FURY)
    {
        switch (DYNAMICSOUNDMAP(soundNum))
        {
            case PIPEBOMB_EXPLODE__STATIC:
            case LASERTRIP_EXPLODE__STATIC:
            case RPG_EXPLODE__STATIC:
                goto boost;
        }
    }

    if ((userflags & (SF_GLOBAL|SF_DTAG)) == SF_GLOBAL || sndist < ((255-LOUDESTVOLUME) << 6))
        sndist = ((255-LOUDESTVOLUME) << 6);

    if (distPtr)
    {
        *distPtr = sndist;
    }

    if (sndPos)
    {
        // Now calculate the position in sound system coordinates.
        FVector3 sndvec = { float(pos->x - cam->x), (pos->z - cam->z) / 16.f, float(pos->y - cam->y) };   // distance vector
        FVector3 campos = { float(pos->x), (cam->z) / 16.f, float(cam->y) };                              // camera position
        sndvec *= float(sndist) / orgsndist;                                                // adjust by what was calculated above;
        *sndPos = campos + sndvec;                                                          // final sound pos - still in Build fixed point coordinates.
        *sndPos *= (1.f / 16); sndPos->Z = -sndPos->Z;                                      // The sound engine works with Doom's coordinate system so do the necessary conversions
    }

    return explosion;
}

//==========================================================================
//
//
//
//==========================================================================

void S_GetCamera(vec3_t** c, int32_t* ca, int32_t* cs)
{
    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            if (c) *c = &CAMERA(pos);
            if (cs) *cs = CAMERA(sect);
            if (ca) *ca = fix16_to_int(CAMERA(q16ang));
        }
        else
        {
            auto pPlayer = g_player[screenpeek].ps;
            if (c) *c = &pPlayer->pos;
            if (cs) *cs = pPlayer->cursectnum;
            if (ca) *ca = fix16_to_int(pPlayer->q16ang);
        }
    }
    else
    {
        if (c) *c = &sprite[ud.camerasprite].pos;
        if (cs) *cs = sprite[ud.camerasprite].sectnum;
        if (ca) *ca = sprite[ud.camerasprite].ang;
    }
}

//=========================================================================
//
// CalcPosVel
//
// The game specific part of the sound updater.
//
//=========================================================================

void DukeSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel)
{
    if (pos != nullptr)
    {
        vec3_t* campos;
        int32_t camsect;

        S_GetCamera(&campos, nullptr, &camsect);
        if (vel) vel->Zero();

        // [BL] Moved this case out of the switch statement to make code easier
        //      on static analysis.
        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = campos && !(chanflags & CHAN_LISTENERZ) ? pt[1] : campos->z / 256.f;
            pos->Z = pt[2];
        }
        else
        {
            switch (type)
            {
            case SOURCE_None:
            default:
                break;

            case SOURCE_Actor:
            {
                auto actor = (spritetype*)source;
                assert(actor != nullptr);
                if (actor != nullptr)
                {
                    S_CalcDistAndAng(int(actor - sprite), chanSound - 1, camsect, campos, &actor->pos, nullptr, pos);
                    /*
                    if (vel)
                    {
                        vel->X = float(actor->Vel.X * TICRATE);
                        vel->Y = float(actor->Vel.Z * TICRATE);
                        vel->Z = float(actor->Vel.Y * TICRATE);
                    }
                    */
                }
                break;
            }

            }

            if ((chanflags & CHAN_LISTENERZ) && campos != nullptr)
            {
                pos->Y = campos->z / 256.f;
            }
        }
    }
}


//==========================================================================
//
//
//
//==========================================================================

void S_Update(void)
{
    SoundListener listener;
    vec3_t* c;
    int32_t ca, cs;

    S_GetCamera(&c, &ca, &cs);

    if (c != nullptr)
    {
        listener.angle = (float)ca * pi::pi() / 2048; // todo: Check value range for angle.
        listener.velocity.Zero();
        listener.position = { c->x / 16.f, c->z / 256.f, c->y / 16.f };
        listener.underwater = false; 
        // This should probably use a real environment instead of the pitch hacking in S_PlaySound3D.
        // listenactor->waterlevel == 3;
        //assert(primaryLevel->Zones.Size() > listenactor->Sector->ZoneNumber);
        listener.Environment = 0;// primaryLevel->Zones[listenactor->Sector->ZoneNumber].Environment;
        listener.valid = true;
    }
    else
    {
        listener.angle = 0;
        listener.position.Zero();
        listener.velocity.Zero();
        listener.underwater = false;
        listener.Environment = nullptr;
        listener.valid = false;
    }
    listener.ListenerObject = ud.camerasprite == -1 ? nullptr : &sprite[ud.camerasprite];
    soundEngine->SetListener(listener);
}


//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound3D(int num, int spriteNum, const vec3_t* pos, bool looped)
{
    int32_t j = VM_OnEventWithReturn(EVENT_SOUND, spriteNum, screenpeek, num);

    int sndnum = VM_OnEventWithReturn(EVENT_SOUND, spriteNum, screenpeek, num);

    auto const pPlayer = g_player[myconnectindex].ps;
    if (!soundEngine->isValidSoundId(sndnum) || !SoundEnabled() || (unsigned)spriteNum >= MAXSPRITES || (pPlayer->gm & MODE_MENU) ||
        (pPlayer->timebeforeexit > 0 && pPlayer->timebeforeexit <= GAMETICSPERSEC * 3)) return -1;

    int userflags = S_GetUserFlags(sndnum);
    if ((!(snd_speech & 1) && (userflags & SF_TALK)) || ((userflags & SF_ADULT) && adult_lockout))
        return -1;

    // Duke talk
    if (userflags & SF_TALK)
    {
        if ((g_netServer || ud.multimode > 1) && PN(spriteNum) == APLAYER && P_Get(spriteNum) != screenpeek) // other player sound
        {
            if ((snd_speech & 4) != 4)
                return -1;
        }
        else if ((snd_speech & 1) != 1)
            return -1;


        bool foundone =  soundEngine->EnumerateChannels([&](FSoundChan* chan)
            {
                auto sid = chan->SoundID;
                auto flags = ((sound_t*)soundEngine->GetUserData(sid))->flags;
                return !!(flags & SF_TALK);
            });
        // don't play if any Duke talk sounds are already playing
        if (foundone) return -1;
    }
    else if ((userflags & (SF_DTAG | SF_GLOBAL)) == SF_DTAG)  // Duke-Tag sound
    {
        return S_PlaySound(sndnum);

    }

    int32_t    sndist;
    FVector3 sndpos;    // this is in sound engine space.

    vec3_t* campos;
    int32_t camsect;

    S_GetCamera(&campos, nullptr, &camsect);
    int const  explosionp = S_CalcDistAndAng(spriteNum, sndnum, camsect, campos, pos, &sndist, &sndpos);
    int        pitch = S_GetPitch(sndnum);
    auto const pOther = g_player[screenpeek].ps;


    if (pOther->sound_pitch)
        pitch += pOther->sound_pitch;

    if (explosionp)
    {
        if (pOther->cursectnum > -1 && sector[pOther->cursectnum].lotag == ST_2_UNDERWATER)
            pitch -= 1024;
    }
    else
    {
        if (sndist > 32767 && PN(spriteNum) != MUSICANDSFX && (userflags & (SF_LOOP | SF_MSFX)) == 0)
            return -1;

        if (pOther->cursectnum > -1 && sector[pOther->cursectnum].lotag == ST_2_UNDERWATER
            && (userflags & SF_TALK) == 0)
            pitch = -768;
    }

    bool is_playing = soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, sndnum+1);
    if (is_playing &&  PN(spriteNum) != MUSICANDSFX)
        S_StopEnvSound(sndnum, spriteNum);

    int const repeatp = (userflags & SF_LOOP);

    if (repeatp && (userflags & SF_ONEINST_INTERNAL) && is_playing)
    {
        return -1;
    }

    // Now 
    auto chflg = ((userflags & SF_LOOP) || looped) ? CHAN_AUTO | CHAN_LOOP : CHAN_AUTO;
    auto chan = soundEngine->StartSound(SOURCE_Actor, &sprite[spriteNum], &sndpos, chflg, sndnum+1, 1.f, 1.f, nullptr, S_ConvertPitch(pitch));
    if (!chan) return -1;
    return 0;
}

//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound(int num, bool looped)
{
    int sndnum = VM_OnEventWithReturn(EVENT_SOUND, g_player[screenpeek].ps->i, screenpeek, num);

    if (!soundEngine->isValidSoundId(sndnum+1) || !SoundEnabled()) return -1;

    int userflags = S_GetUserFlags(sndnum);
    if ((!(snd_speech & 1) && (userflags & SF_TALK)) || ((userflags & SF_ADULT) && adult_lockout))
        return -1;

    int const pitch = S_GetPitch(sndnum);

    auto chflg = ((userflags & SF_LOOP) || looped) ? CHAN_AUTO | CHAN_LOOP : CHAN_AUTO;
    soundEngine->StartSound(SOURCE_None, nullptr, nullptr, chflg, sndnum + 1, 1.f, ATTN_NONE, nullptr, S_ConvertPitch(pitch));
    /* for reference. May still be needed for balancing later.
       : FX_Play3D(snd.ptr, snd.siz, FX_ONESHOT, pitch, 0, 255 - LOUDESTVOLUME, snd.pr, snd.volume,
        (num * MAXSOUNDINSTANCES) + sndnum);
    */
    return 0;
}

//==========================================================================
//
//
//
//==========================================================================

int A_PlaySound(int soundNum, int spriteNum, bool looped)
{
    return (unsigned)spriteNum >= MAXSPRITES ? S_PlaySound(soundNum, looped) :
        S_PlaySound3D(soundNum, spriteNum, &sprite[spriteNum].pos, looped);
}

void S_StopEnvSound(int sndNum, int sprNum)
{
    if (sprNum < -1 || sprNum >= MAXSPRITES) return;

    if (sprNum == -1) soundEngine->StopSoundID(sndNum+1);
    else soundEngine->StopSound(SOURCE_Actor, &sprite[sprNum], -1, sndNum+1);
}

void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset)
{
    if (spriteNum < -1 || spriteNum >= MAXSPRITES) return;
    double expitch = pow(2, pitchoffset / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
    if (spriteNum == -1)
    {
        soundEngine->ChangeSoundPitch(SOURCE_Unattached, nullptr, CHAN_AUTO, expitch, soundNum+1);
    }
    else
    {
        soundEngine->ChangeSoundPitch(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, expitch, soundNum+1);
    }
}

//==========================================================================
//
//
//
//==========================================================================

int A_CheckSoundPlaying(int spriteNum, int soundNum)
{
    if (spriteNum == -1) return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum+1);
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, soundNum+1);
}

// Check if actor <i> is playing any sound.
int A_CheckAnySoundPlaying(int spriteNum)
{
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, 0);
}

int S_CheckSoundPlaying(int soundNum)
{
    return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum+1);
}

//==========================================================================
//
//
//
//==========================================================================

void S_MenuSound(void)
{
    static int SoundNum;
    int const menusnds[] = {
        LASERTRIP_EXPLODE, DUKE_GRUNT,       DUKE_LAND_HURT,   CHAINGUN_FIRE, SQUISHED,      KICK_HIT,
        PISTOL_RICOCHET,   PISTOL_BODYHIT,   PISTOL_FIRE,      SHOTGUN_FIRE,  BOS1_WALK,     RPG_EXPLODE,
        PIPEBOMB_BOUNCE,   PIPEBOMB_EXPLODE, NITEVISION_ONOFF, RPG_SHOOT,     SELECT_WEAPON,
    };
    int s = VM_OnEventWithReturn(EVENT_OPENMENUSOUND, g_player[screenpeek].ps->i, screenpeek, FURY ? -1 : menusnds[SoundNum++ % ARRAY_SIZE(menusnds)]);
    if (s != -1)
        S_PlaySound(s);
}

//==========================================================================
//
// Music
//
//==========================================================================

static void S_SetMusicIndex(unsigned int m)
{
    ud.music_episode = m / MAXLEVELS;
    ud.music_level = m % MAXLEVELS;
}

void S_PlayLevelMusicOrNothing(unsigned int m)
{
    ud.returnvar[0] = m / MAXLEVELS;
    ud.returnvar[1] = m % MAXLEVELS;

    int retval = VM_OnEvent(EVENT_PLAYLEVELMUSICSLOT, g_player[myconnectindex].ps->i, myconnectindex);

    if (retval >= 0)
    {
        // Thanks to scripting that stupid slot hijack cannot be refactored - but we'll store the real data elsewhere anyway!
        auto& mr = m == USERMAPMUSICFAKESLOT ? userMapRecord : mapList[m];
        Mus_Play(mr.labelName, mr.music, true);
        S_SetMusicIndex(m);
    }
}

int S_TryPlaySpecialMusic(unsigned int m)
{
    auto& musicfn = mapList[m].music;
    if (musicfn.IsNotEmpty())
    {
        if (!Mus_Play(nullptr, musicfn, true))
        {
            S_SetMusicIndex(m);
            return 0;
        }
    }

    return 1;
}

void S_PlaySpecialMusicOrNothing(unsigned int m)
{
    if (S_TryPlaySpecialMusic(m))
    {
        S_SetMusicIndex(m);
    }
}

void S_ContinueLevelMusic(void)
{
    VM_OnEvent(EVENT_CONTINUELEVELMUSICSLOT, g_player[myconnectindex].ps->i, myconnectindex);
}


END_DUKE_NS
