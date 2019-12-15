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
#include "renderlayer.h" // for win_gethwnd()
#include "al_midi.h"
#include "openaudio.h"
#include "z_music.h"
#include "mapinfo.h"
#include "sound/s_soundinternal.h"
#include <atomic>

BEGIN_DUKE_NS


#define DQSIZE 256

int32_t g_numEnvSoundsPlaying, g_highestSoundIdx;

static char *MusicPtr;

static int32_t MusicIsWaveform;
static int32_t MusicVoice = -1;

static bool MusicPaused;
static bool SoundPaused;

static std::atomic<uint32_t> dnum, dq[DQSIZE];
static mutex_t m_callback;

static inline void S_SetProperties(assvoice_t *snd, int const owner, int const voice, int const dist, int const clock)
{
    snd->owner = owner;
    snd->id    = voice;
    snd->dist  = dist;
    snd->clock = clock;
}

void S_SoundStartup(void)
{
#ifdef _WIN32
    void *initdata = (void *) win_gethwnd(); // used for DirectSound
#else
    void *initdata = NULL;
#endif

    initprintf("Initializing sound... ");

    int status = FX_Init(snd_numvoices, snd_numchannels, snd_mixrate, initdata);
    if (status != FX_Ok)
    {
        initprintf("failed! %s\n", FX_ErrorString(status));
        return;
    }

	initprintf("%d voices, %d channels, 16-bit %d Hz\n", *snd_numvoices, *snd_numchannels, *snd_mixrate);

    for (int i = 0; i <= g_highestSoundIdx; ++i)
    {
        for (auto & voice : g_sounds[i].voices)
        {
            g_sounds[i].num = 0;
            S_SetProperties(&voice, -1, 0, UINT16_MAX, 0);
        }
    }

	snd_fxvolume.Callback();

	snd_reversestereo.Callback();
    //FX_SetCallBack(S_Callback);
    FX_SetPrintf(OSD_Printf);
}

void S_SoundShutdown(void)
{
    int status = FX_Shutdown();
    if (status != FX_Ok)
    {
        Bsprintf(tempbuf, "S_SoundShutdown(): error: %s", FX_ErrorString(status));
        G_GameExit(tempbuf);
    }
}

void S_PauseSounds(bool paused)
{
    if (SoundPaused == paused)
        return;

    SoundPaused = paused;

    for (int i = 0; i <= g_highestSoundIdx; ++i)
    {
        for (auto & voice : g_sounds[i].voices)
            if (voice.id > 0)
                FX_PauseVoice(voice.id, paused);
    }
}

// returns number of bytes read
int32_t S_LoadSound(int num)
{
    if ((unsigned)num > (unsigned)g_highestSoundIdx || EDUKE32_PREDICT_FALSE(g_sounds[num].filename == NULL))
        return 0;

    auto &snd = g_sounds[num];

    auto fp = S_OpenAudio(snd.filename, 0, 0);

    if (!fp.isOpen())
    {
        OSD_Printf(OSDTEXT_RED "Sound %s(#%d) not found!\n", snd.filename, num);
        return 0;
    }

	int32_t l = fp.GetLength();
    snd.siz = l;
    cacheAllocateBlock((intptr_t *)&snd.ptr, l, nullptr);
    l = fp.Read(snd.ptr, l);

    return l;
}

void cacheAllSounds(void)
{
    for (int i=0, j=0; i <= g_highestSoundIdx; ++i)
    {
        if (g_sounds[i].ptr == 0)
        {
            j++;
            if ((j&7) == 0)
                gameHandleEvents();

            S_LoadSound(i);
        }
    }
}

static inline int S_GetPitch(int num)
{
    auto const &snd   = g_sounds[num];
    int const   range = klabs(snd.pe - snd.ps);

    return (range == 0) ? snd.ps : min(snd.ps, snd.pe) + rand() % range;
}

//==========================================================================
//
// 
//
//==========================================================================

float S_ConvertPitch(int lpitch)
{
    return pow(2, lpitch / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
}

//==========================================================================
//
// 
//
//==========================================================================

static int S_CalcDistAndAng(int spriteNum, int soundNum, int sectNum, int angle,
                             const vec3_t *cam, const vec3_t *pos, int *distPtr, FVector3 *sndPos)
{
    // Todo: Some of this hackery really should be done using rolloff and attenuation instead of messing around with the sound origin.
    int orgsndist = 0, sndang = 0, sndist = 0, explosion = 0;
    int userflags = soundEngine->GetUserFlags(soundNum);
    int dist_adjust = soundEngine->GetUserData(soundNum);

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

void S_Update(void)
{
    if ((g_player[myconnectindex].ps->gm & (MODE_GAME | MODE_DEMO)) == 0)
        return;

    g_numEnvSoundsPlaying = 0;

    const vec3_t* c;
    int32_t ca, cs;

    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            c = &CAMERA(pos);
            cs = CAMERA(sect);
            ca = fix16_to_int(CAMERA(q16ang));
        }
        else
        {
            auto pPlayer = g_player[screenpeek].ps;
            c = &pPlayer->pos;
            cs = pPlayer->cursectnum;
            ca = fix16_to_int(pPlayer->q16ang);
        }
    }
    else
    {
        c = &sprite[ud.camerasprite].pos;
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    int       sndnum = 0;
    int const highest = g_highestSoundIdx;

    do
    {
        if (g_sounds[sndnum].num == 0)
            continue;

        for (auto& voice : g_sounds[sndnum].voices)
        {
            int const spriteNum = voice.owner;

            if ((unsigned)spriteNum >= MAXSPRITES || voice.id <= FX_Ok || !FX_SoundActive(voice.id))
                continue;

            int sndist, sndang;

            S_CalcDistAndAng(spriteNum, sndnum, cs, ca, c, (const vec3_t*)&sprite[spriteNum], &sndist, nullptr);

            if (S_IsAmbientSFX(spriteNum))
                g_numEnvSoundsPlaying++;

            // AMBIENT_SOUND
            //FX_Pan3D(voice.id, sndang >> 4, sndist >> 6);
            voice.dist = sndist >> 6;
            voice.clock++;
        }
    } while (++sndnum <= highest);
}


//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound3D(int num, int spriteNum, const vec3_t* pos)
{
    int32_t j = VM_OnEventWithReturn(EVENT_SOUND, spriteNum, screenpeek, num);

    int sndnum = VM_OnEventWithReturn(EVENT_SOUND, spriteNum, screenpeek, num);

    auto const pPlayer = g_player[myconnectindex].ps;
    if (!soundEngine->isValidSoundId(sndnum) || !SoundEnabled() || (unsigned)spriteNum >= MAXSPRITES || (pPlayer->gm & MODE_MENU) ||
        (pPlayer->timebeforeexit > 0 && pPlayer->timebeforeexit <= GAMETICSPERSEC * 3)) return -1;

    int userflags = soundEngine->GetUserFlags(sndnum);
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

        // don't play if any Duke talk sounds are already playing
        for (j = 0; j <= g_highestSoundIdx; ++j)
            if ((g_sounds[j].m & SF_TALK) && g_sounds[j].num > 0)
                return -1;
    }
    else if ((userflags & (SF_DTAG | SF_GLOBAL)) == SF_DTAG)  // Duke-Tag sound
    {
        return S_PlaySound(sndnum);

    }

    int32_t    sndist;
    FVector3 sndpos;    // this is in sound engine space.
    int const  explosionp = S_CalcDistAndAng(spriteNum, sndnum, CAMERA(sect), fix16_to_int(CAMERA(q16ang)), &CAMERA(pos), pos, &sndist, &sndpos);
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

    bool is_playing = soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, sndnum);
    if (is_playing &&  PN(spriteNum) != MUSICANDSFX)
        S_StopEnvSound(sndnum, spriteNum);

    int const repeatp = (userflags & SF_LOOP);

    if (repeatp && (userflags & SF_ONEINST_INTERNAL) && is_playing)
    {
        return -1;
    }

    // Now 
    auto chan = soundEngine->StartSound(SOURCE_Actor, &sprite[spriteNum], &sndpos, CHAN_AUTO, sndnum, 1.f, 1.f, nullptr, S_ConvertPitch(pitch));
    if (!chan) return -1;
    return 0;
}

//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound(int num)
{
    int sndnum = VM_OnEventWithReturn(EVENT_SOUND, g_player[screenpeek].ps->i, screenpeek, num);

    if (!soundEngine->isValidSoundId(sndnum) || !SoundEnabled()) return -1;

    int userflags = soundEngine->GetUserFlags(sndnum);
    if ((!(snd_speech & 1) && (userflags & SF_TALK)) || ((userflags & SF_ADULT) && adult_lockout))
        return -1;

    int const pitch = S_GetPitch(num);

    soundEngine->StartSound(SOURCE_None, nullptr, nullptr, (userflags & SF_LOOP)? CHAN_AUTO|CHAN_LOOP : CHAN_AUTO, sndnum, 1.f, ATTN_NONE, nullptr, S_ConvertPitch(pitch));
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

int A_PlaySound(int soundNum, int spriteNum)
{
    return (unsigned)spriteNum >= MAXSPRITES ? S_PlaySound(soundNum) :
        S_PlaySound3D(soundNum, spriteNum, &sprite[spriteNum].pos);
}

void S_StopEnvSound(int sndNum, int sprNum)
{
    if (sprNum < -1 || sprNum >= MAXSPRITES) return;

    if (sprNum == -1) soundEngine->StopSoundID(sndNum);
    else soundEngine->StopSound(SOURCE_Actor, &sprite[sprNum], -1, sndNum);
}

void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset)
{
    if (spriteNum < -1 || spriteNum >= MAXSPRITES) return;
    double expitch = pow(2, pitchoffset / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
    if (spriteNum == -1)
    {
        soundEngine->ChangeSoundPitch(SOURCE_Unattached, nullptr, CHAN_AUTO, expitch, soundNum);
    }
    else
    {
        soundEngine->ChangeSoundPitch(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, expitch, soundNum);
    }
}

//==========================================================================
//
//
//
//==========================================================================

int A_CheckSoundPlaying(int spriteNum, int soundNum)
{
    if (spriteNum == -1) return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum);
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, soundNum);
}

// Check if actor <i> is playing any sound.
int A_CheckAnySoundPlaying(int spriteNum)
{
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, 0);
}

int S_CheckSoundPlaying(int soundNum)
{
    return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum);
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
