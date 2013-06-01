//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdio.h>
#include <string.h>

#include "fx_man.h"
#include "music.h"
#include "duke3d.h"
#include "osd.h"
#include "sounds.h"

#ifdef _WIN32
#include "winlayer.h"
#endif

int32_t g_numEnvSoundsPlaying,g_maxSoundPos = 0;

static int32_t MusicIsWaveform = 0;
static char *MusicPtr = 0;
static int32_t MusicLen = 0;
static int32_t MusicVoice = -1;
static int32_t MusicPaused = 0;

static mutex_t s_mutex;
static volatile uint32_t dq[128], dnum = 0;

void S_SoundStartup(void)
{
    int32_t fxdevicetype, i;
    void *initdata = 0;

    if (ud.config.FXDevice >= 0)
        fxdevicetype = ASS_AutoDetect;
    else return;

#ifdef RENDERTYPEWIN
    initdata = (void *) win_gethwnd(); // used for DirectSound
#endif

    initprintf("Initializing sound... ");

    if (FX_Init(fxdevicetype, ud.config.NumVoices, ud.config.NumChannels, ud.config.NumBits, ud.config.MixRate, initdata) != FX_Ok)
    {
        initprintf("failed! %s\n", FX_ErrorString(FX_Error));
        return;
    }

    initprintf("%d voices, %d channels, %d-bit %d Hz\n", ud.config.NumVoices, ud.config.NumChannels,
        ud.config.NumBits, ud.config.MixRate);

    for (i=g_maxSoundPos; i >= 0 ; i--)
    {
        int32_t j = MAXSOUNDINSTANCES-1;

        for (; j>=0; j--)
        {
            g_sounds[i].num = 0;
            g_sounds[i].SoundOwner[j].voice = 0;
            g_sounds[i].SoundOwner[j].ow = -1;
            g_sounds[i].SoundOwner[j].sndist = UINT32_MAX;
            g_sounds[i].SoundOwner[j].clock = 0;
        }

        g_soundlocks[i] = 199;
    }

    FX_SetVolume(ud.config.FXVolume);
    FX_SetReverseStereo(ud.config.ReverseStereo);
    FX_SetCallBack(S_Callback);
    FX_SetPrintf(initprintf);
    mutex_init(&s_mutex);
}

void S_SoundShutdown(void)
{
    if (ud.config.FXDevice < 0)
        return;

    if (MusicVoice >= 0)
        S_MusicShutdown();

    if (FX_Shutdown() != FX_Ok)
    {
        Bsprintf(tempbuf, "S_SoundShutdown(): error: %s", FX_ErrorString(FX_Error));
        G_GameExit(tempbuf);
    }
}

void S_MusicStartup(void)
{
    if (ud.config.MusicDevice < 0)
        return;

    initprintf("Initializing music...\n");

    if (MUSIC_Init(ud.config.MusicDevice, 0) == MUSIC_Ok || MUSIC_Init((ud.config.MusicDevice = 0), 0) == MUSIC_Ok)
    {
        MUSIC_SetVolume(ud.config.MusicVolume);
        return;
    }

    initprintf("S_MusicStartup(): failed initializing\n");
}

void S_MusicShutdown(void)
{
    if (ud.config.MusicDevice < 0)
        return;

    S_StopMusic();

    if (MUSIC_Shutdown() != MUSIC_Ok)
        initprintf("%s\n", MUSIC_ErrorString(MUSIC_ErrorCode));
}

void S_PauseMusic(int32_t onf)
{
    if (MusicPaused == onf || (MusicIsWaveform && MusicVoice < 0))
        return;

    MusicPaused = onf;

    if (MusicIsWaveform)
    {
        FX_PauseVoice(MusicVoice, onf);
        return;
    }

    if (onf)
        MUSIC_Pause();
    else
        MUSIC_Continue();
}

void S_MusicVolume(int32_t volume)
{
    if (MusicIsWaveform && MusicVoice >= 0)
        FX_SetPan(MusicVoice, volume, volume, volume);

    MUSIC_SetVolume(volume);
}

void S_MenuSound(void)
{
    static int32_t SoundNum=0;
    uint8_t menusnds[] =
    {
        LASERTRIP_EXPLODE,
        DUKE_GRUNT,
        DUKE_LAND_HURT,
        CHAINGUN_FIRE,
        SQUISHED,
        KICK_HIT,
        PISTOL_RICOCHET,
        PISTOL_BODYHIT,
        PISTOL_FIRE,
        SHOTGUN_FIRE,
        BOS1_WALK,
        RPG_EXPLODE,
        PIPEBOMB_BOUNCE,
        PIPEBOMB_EXPLODE,
        NITEVISION_ONOFF,
        RPG_SHOOT,
        SELECT_WEAPON,
    };

    S_PlaySound(menusnds[SoundNum++ % (sizeof(menusnds)/sizeof(menusnds[0]))]);
}

int32_t S_PlayMusic(const char *fn, const int32_t sel)
{
    char *ofn = (char *)fn, *testfn, *extension;
    int32_t fp;
    const char *alt = 0;

    if (ud.config.MusicToggle == 0) return 0;
    if (ud.config.MusicDevice < 0) return 0;

    if (MapInfo[sel].alt_musicfn != NULL)
        alt = fn = MapInfo[sel].alt_musicfn;

    testfn = (char *) Bmalloc(strlen(fn) + 6);
    strcpy(testfn, fn);
    extension = strrchr(testfn, '.');

    do
    {
        if (extension && !Bstrcasecmp(extension, ".mid"))
        {
            // we've been asked to load a .mid file, but first let's see
            // if there's a flac or an ogg with the same base name lying around
            strcpy(extension, ".flac");
            extension[5] = '\0';
            fp = kopen4loadfrommod(testfn, 0);
            if (fp >= 0)
            {
                Bfree(testfn);
                break;
            }
            strcpy(extension, ".ogg");
            extension[4] = '\0';
            fp = kopen4loadfrommod(testfn, 0);
            if (fp >= 0)
            {
                Bfree(testfn);
                break;
            }
        }

        Bfree(testfn);

        // just use what we've been given
        fp = kopen4loadfrommod((char *)fn, 0);

        if (alt && fp < 0)
            fp = kopen4loadfrommod(ofn, 0);
    }
    while (0);

    if (fp < 0)
    {
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: can't open \"%s\" for playback!\n",fn);
        return 0;
    }

    S_StopMusic();

    MusicPtr = (char *) Bmalloc((MusicLen = kfilelength(fp)));

    if ((g_musicSize = kread(fp, (char *)MusicPtr, MusicLen)) != MusicLen)
    {
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: read %d bytes from \"%s\", expected %d\n",g_musicSize, fn, MusicLen);
        kclose(fp);
        g_musicSize = 0;
        return 0;
    }

    kclose(fp);

    if (!Bmemcmp(MusicPtr, "MThd", 4))
    {
        MUSIC_PlaySong(MusicPtr, MUSIC_LoopSong);
        MusicIsWaveform = 0;
    }
    else
    {
        int32_t mvol = ud.config.MusicVolume;
        MusicVoice = FX_PlayLoopedAuto(MusicPtr, MusicLen, 0, 0,
                                       0, mvol, mvol, mvol,
                                       FX_MUSIC_PRIORITY, MUSIC_ID);
        if (MusicVoice > FX_Ok)
            MusicIsWaveform = 1;
    }
    return (alt != 0);
}

void S_StopMusic(void)
{
    MusicPaused = 0;

    if (MusicIsWaveform && MusicVoice >= 0)
    {
        FX_StopSound(MusicVoice);
        MusicVoice = -1;
        MusicIsWaveform = 0;
    }

    MUSIC_StopSong();

    if (MusicPtr)
    {
        Bfree(MusicPtr);
        MusicPtr = NULL;
        g_musicSize = MusicLen = 0;
    }
}

void S_Cleanup(void)
{
    // process from our own local copy of the delete queue so we don't hold the lock long
    uint32_t ldq[128], ldnum;

    mutex_lock(&s_mutex);

    if (!dnum)
    {
        mutex_unlock(&s_mutex);
        return;
    }

    ldnum = dnum;
    Bmemcpy(ldq, (void *)dq, ldnum*sizeof(int32_t));
    dnum = 0;

    mutex_unlock(&s_mutex);

    do
    {
        uint32_t num = ldq[--ldnum];

        // negative index is RTS playback
        if ((int32_t)num < 0)
        {
            if (rts_lumplockbyte[-(int32_t)num] >= 200)
                rts_lumplockbyte[-(int32_t)num]--;
            continue;
        }

        // num + (MAXSOUNDS*MAXSOUNDINSTANCES) is a sound played globally
        // for which there was no open slot to keep track of the voice
        if (num >= (MAXSOUNDS*MAXSOUNDINSTANCES))
        {
            g_soundlocks[num-(MAXSOUNDS*MAXSOUNDINSTANCES)]--;
            continue;
        }

        {
            int32_t j = num & (MAXSOUNDINSTANCES-1);
            int32_t i;

            num = (num - j) / MAXSOUNDINSTANCES;

            i = g_sounds[num].SoundOwner[j].ow;

            if (g_sounds[num].num > MAXSOUNDINSTANCES)
                OSD_Printf(OSD_ERROR "S_Cleanup(): num exceeds MAXSOUNDINSTANCES! g_sounds[%d].num %d wtf?\n", num, g_sounds[num].num);

            if (g_sounds[num].num > 0)
                g_sounds[num].num--;

            // MUSICANDSFX uses t_data[0] to control restarting the sound
            // ST_2_UNDERWATER
            if (i != -1 && S_IsAmbientSFX(i) && sector[sprite[i].sectnum].lotag < 3)
                actor[i].t_data[0] = 0;

            g_sounds[num].SoundOwner[j].ow = -1;
            g_sounds[num].SoundOwner[j].voice = 0;
            g_sounds[num].SoundOwner[j].sndist = UINT32_MAX;
            g_sounds[num].SoundOwner[j].clock = 0;
        }
        g_soundlocks[num]--;
    }
    while (ldnum);
}

// returns number of bytes read
int32_t S_LoadSound(uint32_t num)
{
    int32_t   fp = -1, l;

    if (num > (unsigned)g_maxSoundPos || ud.config.SoundToggle == 0 || ud.config.FXDevice < 0) return 0;

    if (g_sounds[num].filename == NULL && g_sounds[num].filename1 == NULL)
    {
        OSD_Printf(OSD_ERROR "Sound (#%d) not defined!\n",num);
        return 0;
    }

    if (g_sounds[num].filename1)
        fp = kopen4loadfrommod(g_sounds[num].filename1,g_loadFromGroupOnly);

    if (fp == -1)
    {
        fp = kopen4loadfrommod(g_sounds[num].filename,g_loadFromGroupOnly);

        if (fp == -1)
        {
            OSD_Printf(OSDTEXT_RED "Sound %s(#%d) not found!\n",g_sounds[num].filename,num);
            return 0;
        }
    }

    g_sounds[num].soundsiz = l = kfilelength(fp);

    g_soundlocks[num] = 200;

    allocache((intptr_t *)&g_sounds[num].ptr, l, (char *)&g_soundlocks[num]);
    l = kread(fp, g_sounds[num].ptr, l);
    kclose(fp);

    return l;
}


static int32_t S_GetPitch(int32_t num)
{
    int32_t j = klabs(g_sounds[num].pe-g_sounds[num].ps);

    if (j == 0)
        return g_sounds[num].ps;

    return min(g_sounds[num].ps, g_sounds[num].pe) + rand()%j;
}

static int32_t S_TakeSlot(int32_t num)
{
    uint32_t dist = 0, clock = UINT32_MAX;
    int32_t i = 0, j = 0;

    S_Cleanup();

    while (j < MAXSOUNDINSTANCES && g_sounds[num].SoundOwner[j].voice > 0)
    {
        if (g_sounds[num].SoundOwner[j].sndist > dist ||
            (g_sounds[num].SoundOwner[j].sndist == dist && g_sounds[num].SoundOwner[j].clock < clock))
        {
            clock = g_sounds[num].SoundOwner[j].clock;
            dist = g_sounds[num].SoundOwner[j].sndist;
            i = j;
        }

        j++;
    }

    if (j != MAXSOUNDINSTANCES)
        return j;

    if (FX_SoundActive(g_sounds[num].SoundOwner[i].voice))
        FX_StopSound(g_sounds[num].SoundOwner[i].voice);

    mutex_lock(&s_mutex);
    dq[dnum++] = (num * MAXSOUNDINSTANCES) + i;
    mutex_unlock(&s_mutex);
    S_Cleanup();

    return i;
}

static int32_t S_GetSlot(int32_t num)
{
    int32_t j = 0;

    while (j < MAXSOUNDINSTANCES && g_sounds[num].SoundOwner[j].voice > 0)
        j++;

    if (j == MAXSOUNDINSTANCES)
        j = S_TakeSlot(num);

    return j;
}

static inline int32_t S_GetAngle(int32_t camang, const vec3_t *cam, const vec3_t *pos)
{
    return (2048 + camang - getangle(cam->x-pos->x, cam->y-pos->y))&2047;
}

static int32_t S_CalcDistAndAng(int32_t i, int32_t num, int32_t camsect, int32_t camang,
                                const vec3_t *cam, const vec3_t *pos,
                                int32_t *sndistptr, int32_t *sndangptr)
{
    int32_t sndang, sndist;
    int32_t explosion = 0;

    if (PN == APLAYER && sprite[i].yvel == screenpeek)
    {
        sndang = sndist = 0;
        goto sound_further_processing;
    }

    sndang = S_GetAngle(camang, cam, pos);

    sndist = FindDistance3D(cam->x-pos->x, cam->y-pos->y, (cam->z-pos->z)>>4);

    if (g_fakeMultiMode==2)
    {
        // HACK for splitscreen mod: take the min of sound distances
        // to 1st and 2nd player.

        if (PN == APLAYER && sprite[i].yvel==1)
        {
            sndist = sndang = 0;
            goto sound_further_processing;
        }

        {
            const vec3_t *cam2 = &g_player[1].ps->pos;
            int32_t sndist2 = FindDistance3D(cam2->x-pos->x, cam2->y-pos->y, (cam2->z-pos->z)>>4);

            if (sndist2 < sndist)
            {
                cam = cam2;
                camsect = g_player[1].ps->cursectnum;
                camang = g_player[1].ps->ang;

                sndist = sndist2;
                sndang = S_GetAngle(camang, cam, pos);
            }
        }
    }

    if ((g_sounds[num].m&16) == 0 && S_IsAmbientSFX(i) && (sector[SECT].lotag&0xff) < 9)  // ST_9_SLIDING_ST_DOOR
        sndist = divscale14(sndist, SHT+1);

sound_further_processing:
    sndist += g_sounds[num].vo;
    if (sndist < 0)
        sndist = 0;

    if (camsect > -1 && sndist && PN != MUSICANDSFX &&
            !cansee(cam->x,cam->y,cam->z-(24<<8),camsect, SX,SY,SZ-(24<<8),SECT))
        sndist += sndist>>5;

    switch (DYNAMICSOUNDMAP(num))
    {
    case PIPEBOMB_EXPLODE__STATIC:
    case LASERTRIP_EXPLODE__STATIC:
    case RPG_EXPLODE__STATIC:
        explosion = 1;
        if (sndist > 6144)
            sndist = 6144;
        break;
    }

    if ((g_sounds[num].m&16) || sndist < ((255-LOUDESTVOLUME)<<6))
        sndist = ((255-LOUDESTVOLUME)<<6);

    *sndistptr = sndist;
    *sndangptr = sndang;

    return explosion;
}

int32_t S_PlaySound3D(int32_t num, int32_t i, const vec3_t *pos)
{
    int32_t j = 0;
    int32_t sndist, sndang, explosionp;
    int32_t voice, pitch;

    const DukePlayer_t *const myps = g_player[myconnectindex].ps;
    const DukePlayer_t *peekps;

    if (G_HaveEvent(EVENT_SOUND))
        num = VM_OnEvent(EVENT_SOUND, i, screenpeek, -1, num);

    if ((unsigned)num > (unsigned)g_maxSoundPos ||
            ud.config.FXDevice < 0 ||
            ((g_sounds[num].m&8) && ud.lockout) ||
            ud.config.SoundToggle == 0 ||
//            g_sounds[num].num >= MAXSOUNDINSTANCES ||
            (unsigned)i >= MAXSPRITES ||
            FX_VoiceAvailable(g_sounds[num].pr) == 0 ||
            (myps->timebeforeexit > 0 && myps->timebeforeexit <= GAMETICSPERSEC*3) ||
            (myps->gm&MODE_MENU))
        return -1;

    if (g_sounds[num].m&128)  // Duke-Tag sound
    {
        if ((voice = S_PlaySound(num)) <= FX_Ok)
            return -1;

        while (j < MAXSOUNDINSTANCES && g_sounds[num].SoundOwner[j].voice != voice)
            j++;

        if (j >= MAXSOUNDINSTANCES)
        {
            OSD_Printf(OSD_ERROR "%s %d: WTF?\n", __FILE__, __LINE__);
            return -1;
        }

        g_sounds[num].SoundOwner[j].ow = i;

        return voice;
    }

    // Duke talk
    if (g_sounds[num].m&4)
    {
        if ((g_netServer || ud.multimode > 1) && PN == APLAYER && sprite[i].yvel != screenpeek) // other player sound
        {
            if (!(ud.config.VoiceToggle&4))
                return -1;
        }
        else if (!(ud.config.VoiceToggle&1))
            return -1;

        // don't play if any Duke talk sounds are already playing
        for (j=g_maxSoundPos; j>=0; j--)
            if ((g_sounds[j].m&4) && g_sounds[j].num > 0)
                return -1;
    }

    explosionp = S_CalcDistAndAng(i, num, CAMERA(sect), CAMERA(ang), &CAMERA(pos), pos, &sndist, &sndang);

    pitch = S_GetPitch(num);
    peekps = g_player[screenpeek].ps;

    if (g_fakeMultiMode==2)
    {
        // splitscreen HACK
        if (g_player[1].ps->i == i)
            peekps = g_player[1].ps;
    }

    if (peekps->sound_pitch)
        pitch += peekps->sound_pitch;

    if (explosionp)
    {
        if (peekps->cursectnum > -1 && sector[peekps->cursectnum].lotag == ST_2_UNDERWATER)
            pitch -= 1024;
    }
    else
    {
        if (sndist > 32767 && PN != MUSICANDSFX && (g_sounds[num].m & 3) == 0)
            return -1;

        if (peekps->cursectnum > -1 && sector[peekps->cursectnum].lotag == ST_2_UNDERWATER && (g_sounds[num].m&4) == 0)
            pitch = -768;
    }

    if (g_sounds[num].num > 0 && PN != MUSICANDSFX)
        S_StopEnvSound(num, i);

    if (g_sounds[num].ptr == 0)
    {
        if (S_LoadSound(num) == 0)
            return -1;
    }
    else
    {
        if (g_soundlocks[num] < 200)
            g_soundlocks[num] = 200;
        else g_soundlocks[num]++;
    }

    j = S_GetSlot(num);

    if (j >= MAXSOUNDINSTANCES)
    {
        g_soundlocks[num]--;
        return -1;
    }

    {
        const int32_t repeatp = (g_sounds[num].m&1);
        const int32_t ambsfxp = S_IsAmbientSFX(i);

        if (repeatp && (g_sounds[num].m&32) && g_sounds[num].num > 0)
        {
            g_soundlocks[num]--;
            return -1;
        }

        if (repeatp && !ambsfxp)
        {
            voice = FX_PlayLoopedAuto(g_sounds[num].ptr, g_sounds[num].soundsiz, 0, -1,
                                      pitch, sndist>>6, sndist>>6, 0,  // XXX: why is 'right' 0?
                                      g_sounds[num].pr, (num * MAXSOUNDINSTANCES) + j);
        }
        else
        {
            // Ambient MUSICANDSFX always start playing using the 3D routines!
            voice = FX_PlayAuto3D(g_sounds[num].ptr, g_sounds[num].soundsiz,
                                  repeatp ? FX_LOOP : FX_ONESHOT,
                                  pitch, sndang>>4, sndist>>6,
                                  g_sounds[num].pr, (num * MAXSOUNDINSTANCES) + j);
        }
    }

    if (voice <= FX_Ok)
    {
        g_soundlocks[num]--;
        return -1;
    }

    g_sounds[num].num++;
    g_sounds[num].SoundOwner[j].ow = i;
    g_sounds[num].SoundOwner[j].voice = voice;
    g_sounds[num].SoundOwner[j].sndist = sndist>>6;
    g_sounds[num].SoundOwner[j].clock = totalclock;

    return voice;
}

int32_t S_PlaySound(int32_t num)
{
    int32_t pitch;
    int32_t voice, j;

    if (G_HaveEvent(EVENT_SOUND))
        num = VM_OnEvent(EVENT_SOUND, g_player[screenpeek].ps->i, screenpeek, -1, num);

    if (ud.config.FXDevice < 0) return -1;
    if (ud.config.SoundToggle==0) return -1;

    if ((unsigned)num > (unsigned)g_maxSoundPos || (g_sounds[num].filename == NULL && g_sounds[num].filename1 == NULL))
    {
        OSD_Printf("WARNING: invalid sound #%d\n",num);
        return -1;
    }

    if (!(ud.config.VoiceToggle&1) && (g_sounds[num].m&4)) return -1;
    if ((g_sounds[num].m&8) && ud.lockout) return -1;
    if (FX_VoiceAvailable(g_sounds[num].pr) == 0) return -1;

    pitch = S_GetPitch(num);

    if (g_sounds[num].ptr == 0)
    {
        if (S_LoadSound(num) == 0)
            return -1;
    }
    else
    {
        if (g_soundlocks[num] < 200)
            g_soundlocks[num] = 200;
        else g_soundlocks[num]++;
    }

    j = S_GetSlot(num);

    if (j >= MAXSOUNDINSTANCES)
    {
        g_soundlocks[num]--;
        return -1;
    }

    if (g_sounds[num].m&1)
        voice = FX_PlayLoopedAuto(g_sounds[num].ptr, g_sounds[num].soundsiz, 0, -1,
                                  pitch, LOUDESTVOLUME, LOUDESTVOLUME, LOUDESTVOLUME,
                                  g_sounds[num].soundsiz, (num * MAXSOUNDINSTANCES) + j);
    else
        voice = FX_PlayAuto3D(g_sounds[num].ptr, g_sounds[num].soundsiz, FX_ONESHOT,
                              pitch, 0, 255-LOUDESTVOLUME,
                              g_sounds[num].pr, (num * MAXSOUNDINSTANCES) + j);

    if (voice <= FX_Ok)
    {
        g_soundlocks[num]--;
        return -1;
    }

    g_sounds[num].num++;
    g_sounds[num].SoundOwner[j].ow = -1;
    g_sounds[num].SoundOwner[j].voice = voice;
    g_sounds[num].SoundOwner[j].sndist = 255-LOUDESTVOLUME;
    g_sounds[num].SoundOwner[j].clock = totalclock;
    return voice;
}

int32_t A_PlaySound(uint32_t num, int32_t i)
{
    if ((unsigned)num > (unsigned)g_maxSoundPos) return -1;

    return i < 0 ? S_PlaySound(num) : 
        S_PlaySound3D(num, i, (vec3_t *)&sprite[i]);
}

void S_StopEnvSound(int32_t num, int32_t i)
{
    int32_t j, iter = 0;

    if ((unsigned)num > (unsigned)g_maxSoundPos || g_sounds[num].num <= 0)
        return;

    do
    {
        if (iter++ > MAXSOUNDINSTANCES<<1)
        {
            initprintf(OSD_ERROR "S_StopEnvSound(): too many iterations! The following IDs are still active for sound %d:\n", num);
            for (j=MAXSOUNDINSTANCES-1; j>=0; j--)
                if (g_sounds[num].SoundOwner[j].ow == i)
                    initprintf(OSD_ERROR "slot %d, voice %d, sprite %d\n", j, g_sounds[num].SoundOwner[j].voice, g_sounds[num].SoundOwner[j].ow);
            return;
        }

        for (j=MAXSOUNDINSTANCES-1; j>=0; j--)
        {
            if ((i == -1 && g_sounds[num].SoundOwner[j].voice > FX_Ok) || (i != -1 && g_sounds[num].SoundOwner[j].ow == i))
            {
                if (i >= 0 && g_sounds[num].SoundOwner[j].voice <= FX_Ok)
                    initprintf(OSD_ERROR "S_StopEnvSound(): bad voice %d for sound ID %d index %d!\n", g_sounds[num].SoundOwner[j].voice, num, j);
                else if (g_sounds[num].SoundOwner[j].voice > FX_Ok)
                {
                    FX_StopSound(g_sounds[num].SoundOwner[j].voice);
                    return;
                }
            }
        }
    }
    while (j >= 0);
}

void S_ChangeSoundPitch(int32_t num, int32_t i, int32_t pitchoffset)
{
    int32_t j;

    if ((unsigned)num > (unsigned)g_maxSoundPos || g_sounds[num].num <= 0)
        return;

    for (j=MAXSOUNDINSTANCES-1; j>=0; j--)
    {
        int32_t voice = g_sounds[num].SoundOwner[j].voice;

        if ((i == -1 && voice > FX_Ok) || (i != -1 && g_sounds[num].SoundOwner[j].ow == i))
        {
            if (i >= 0 && voice <= FX_Ok)
                initprintf(OSD_ERROR "S_ChangeSoundPitch(): bad voice %d for sound ID %d index %d!\n", voice, num, j);
            else if (voice > FX_Ok && FX_SoundActive(voice))
                FX_SetPitch(voice, pitchoffset);
            break;
        }
    }
}

void S_Update(void)
{
    const vec3_t *c;
    int32_t ca,cs;
    int32_t num;  // the sound index...

    S_Cleanup();

    if ((g_player[myconnectindex].ps->gm & (MODE_GAME|MODE_DEMO)) == 0)
        return;

    g_numEnvSoundsPlaying = 0;

    if (ud.camerasprite == -1)
    {
        c = &CAMERA(pos);
        cs = CAMERA(sect);
        ca = CAMERA(ang);
    }
    else
    {
        c = (vec3_t *)&sprite[ud.camerasprite];
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    num = g_maxSoundPos;

    do
    {
        int32_t k;

        for (k=MAXSOUNDINSTANCES-1; k>=0; k--)
        {
            int32_t i = g_sounds[num].SoundOwner[k].ow;
            int32_t sndist, sndang;

            if ((unsigned)i >= MAXSPRITES || g_sounds[num].num == 0 || g_sounds[num].SoundOwner[k].voice <= FX_Ok)
                continue;

            if (!FX_SoundActive(g_sounds[num].SoundOwner[k].voice))
            {
                /*
                  OSD_Printf("S_Update(): stale voice %d from sound %d position %d sprite %d\n",
                  g_sounds[num].SoundOwner[k].voice, num, k, g_sounds[num].SoundOwner[k].ow);
                */
                continue;
            }

            S_CalcDistAndAng(i, num, cs, ca, c, (const vec3_t *)&sprite[i], &sndist, &sndang);

            if (S_IsAmbientSFX(i))
                g_numEnvSoundsPlaying++;

            // AMBIENT_SOUND
            FX_Pan3D(g_sounds[num].SoundOwner[k].voice, sndang>>4, sndist>>6);
            g_sounds[num].SoundOwner[k].sndist = sndist>>6;
        }
    }
    while (num--);
}

void S_Callback(uint32_t num)
{
    if ((int32_t)num == MUSIC_ID)
        return;

    mutex_lock(&s_mutex);
    dq[dnum++] = num;
    mutex_unlock(&s_mutex);
}

void S_ClearSoundLocks(void)
{
    int32_t i;

    for (i=g_maxSoundPos; i >= 0 ; i--)
        if (g_soundlocks[i] >= 200)
            g_soundlocks[i] = 199;

    for (i=0; i<11; i++)
        if (rts_lumplockbyte[i] >= 200)
            rts_lumplockbyte[i] = 199;
}

int32_t A_CheckSoundPlaying(int32_t i, int32_t num)
{
    if (num > g_maxSoundPos || num < 0) return 0;

    if (g_sounds[num].num > 0 && i >= 0)
    {
        int32_t j=MAXSOUNDINSTANCES-1;

        for (; j>=0; j--)
            if (g_sounds[num].SoundOwner[j].ow == i)
                return 1;
    }

    return (i == -1) ? g_sounds[num].num : 0;
}

// Check if actor <i> is playing any sound.
int32_t A_CheckAnySoundPlaying(int32_t i)
{
    int32_t j;

    for (j=g_maxSoundPos; j>=0; j--)
    {
        int32_t k;

        for (k=0; k<MAXSOUNDINSTANCES; k++)
            if (g_sounds[j].SoundOwner[k].ow == i)
                return 1;
    }

    return 0;
}

int32_t S_CheckSoundPlaying(int32_t i, int32_t num)
{
    if (num > g_maxSoundPos || num < 0) return 0;
    return (i == -1) ? (g_soundlocks[num] >= 200) : g_sounds[num].num;
}
