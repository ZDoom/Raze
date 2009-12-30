//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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

//#include <conio.h>
#include <stdio.h>
#include <string.h>

#include "fx_man.h"
#include "music.h"
#include "duke3d.h"
#include "util_lib.h"
#include "osd.h"

#define LOUDESTVOLUME 150

int32_t backflag,g_numEnvSoundsPlaying,g_maxSoundPos = 0;

#define MUSIC_ID  -65536

static int32_t MusicIsWaveform = 0;
static char * MusicPtr = 0;
static int32_t MusicLen = 0;
static int32_t MusicVoice = -1;
static int32_t MusicPaused = 0;

/*
===================
=
= SoundStartup
=
===================
*/

void S_SoundStartup(void)
{
    int32_t status;
    int32_t fxdevicetype;
    void * initdata = 0;

    if (ud.config.FXDevice >= 0)
        fxdevicetype = ASS_AutoDetect;
    else return;

#ifdef WIN32
    initdata = (void *) win_gethwnd();
#endif

    initprintf("Initializing sound...\n");

    status = FX_Init(fxdevicetype, ud.config.NumVoices, ud.config.NumChannels, ud.config.NumBits, ud.config.MixRate, initdata);

    if (status != FX_Ok)
    {
        sprintf(tempbuf, "Sound startup error: %s", FX_ErrorString(FX_Error));
        G_GameExit(tempbuf);
    }

    FX_SetVolume(ud.config.FXVolume);
    FX_SetReverseStereo(ud.config.ReverseStereo);
    status = FX_SetCallBack(S_TestSoundCallback);
}

/*
===================
=
= SoundShutdown
=
===================
*/

void S_SoundShutdown(void)
{
    int32_t status;

    // if they chose None lets return
    if (ud.config.FXDevice < 0)
        return;

    if (MusicVoice >= 0)
    {
        S_MusicShutdown();
    }

    status = FX_Shutdown();
    if (status != FX_Ok)
    {
        Bsprintf(tempbuf, "Sound shutdown error: %s", FX_ErrorString(FX_Error));
        G_GameExit(tempbuf);
    }
}

/*
===================
=
= MusicStartup
=
===================
*/

void S_MusicStartup(void)
{
    int32_t status;

    // if they chose None lets return
    if (ud.config.MusicDevice < 0)
        return;

    initprintf("Initializing music...\n");

    status = MUSIC_Init(ud.config.MusicDevice, 0);

    if (status == MUSIC_Ok)
    {
        MUSIC_SetVolume(ud.config.MusicVolume);
    }
    else
    {
        ud.config.MusicDevice = 0;

        status = MUSIC_Init(ud.config.MusicDevice, 0);

        if (status == MUSIC_Ok)
        {
            MUSIC_SetVolume(ud.config.MusicVolume);
        }
        
        initprintf("S_MusicStartup(): failed initializing\n");
    }
}

/*
===================
=
= MusicShutdown
=
===================
*/

void S_MusicShutdown(void)
{
    int32_t status;

    // if they chose None lets return
    if (ud.config.MusicDevice < 0)
        return;

    S_StopMusic();

    status = MUSIC_Shutdown();
    if (status != MUSIC_Ok)
    {
        Error(MUSIC_ErrorString(MUSIC_ErrorCode));
    }
}

void S_PauseMusic(int32_t onf)
{
    if (MusicPaused == onf || (MusicIsWaveform && MusicVoice < 0))
        return;

    if (MusicIsWaveform)
        FX_PauseVoice(MusicVoice, onf);
    else
    {
        if (onf)
            MUSIC_Pause();
        else
            MUSIC_Continue();
    }

    MusicPaused = onf;
}

void S_MusicVolume(int32_t volume)
{
    if (MusicIsWaveform && MusicVoice >= 0)
    {
        FX_SetPan(MusicVoice, volume, volume, volume);
    }
    else if (!MusicIsWaveform)
    {
        MUSIC_SetVolume(volume);
    }
}


void S_MenuSound(void)
{
    static int32_t SoundNum=0;
    static int16_t menusnds[] =
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
        SELECT_WEAPON
    };
    S_PlaySound(menusnds[SoundNum++]);
    SoundNum %= (sizeof(menusnds)/sizeof(menusnds[0]));
}

/*
void _playmusic(const char *fn)
{
    int32_t        fp, l, i;

    if (fn == NULL) return;

    if (ud.config.MusicToggle == 0) return;
    if (ud.config.MusicDevice < 0) return;

    fp = kopen4loadfrommod((char *)fn,0);

    if (fp == -1)
    {
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: can't open '%s' for playback!",fn);
        return;
    }

    l = kfilelength(fp);
    MUSIC_StopSong();

    MusicPtr = Brealloc(MusicPtr, l);
    if ((i = kread(fp, (char *)MusicPtr, l)) != l)
    {
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: read %d bytes from '%s', needed %d\n",i, fn, l);
        kclose(fp);
        return;
    }

    kclose(fp);

    g_musicSize=l;

    // FIXME: I need this to get the music volume initialized (not sure why) -- Jim Bentler
    MUSIC_SetVolume(ud.config.MusicVolume);
    MUSIC_PlaySong((char *)MusicPtr, MUSIC_LoopSong);
}

int32_t S_PlayMusic(const char *fn, const int32_t sel)
{
    g_musicSize=0;
    if (MapInfo[sel].alt_musicfn != NULL)
        _playmusic(MapInfo[sel].alt_musicfn);
    if (!g_musicSize)
    {
        _playmusic(fn);
        return 0;
    }
    return 1;
}
*/

int32_t S_PlayMusic(const char *fn, const int32_t sel)
{
    char *ofn = (char *)fn;
    int32_t fp;
    char * testfn, * extension;
    int32_t alt = 0;

    if (ud.config.MusicToggle == 0) return 0;
    if (ud.config.MusicDevice < 0) return 0;

    if (MapInfo[sel].alt_musicfn != NULL)
        alt = (int32_t)(fn = MapInfo[sel].alt_musicfn);
        

    testfn = (char *) Bmalloc(strlen(fn) + 5);
    strcpy(testfn, fn);
    extension = strrchr(testfn, '.');

    do
    {
        if (extension && !Bstrcasecmp(extension, ".mid"))
        {
            // we've been asked to load a .mid file, but first
            // let's see if there's an ogg with the same base name
            // lying around
            strcpy(extension, ".ogg");
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
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: can't open '%s' for playback!",fn);
        return 0;
    }

    S_StopMusic();

    MusicLen = kfilelength(fp);
    MusicPtr = (char *) Bmalloc(MusicLen);

    if ((g_musicSize = kread(fp, (char *)MusicPtr, MusicLen)) != MusicLen)
    {
        OSD_Printf(OSD_ERROR "S_PlayMusic(): error: read %d bytes from '%s', needed %d\n",g_musicSize, fn, MusicLen);
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
        if ((MusicVoice = FX_PlayLoopedAuto(MusicPtr, MusicLen, 0, 0, 0, ud.config.MusicVolume,
            ud.config.MusicVolume, ud.config.MusicVolume,
            FX_MUSIC_PRIORITY, MUSIC_ID)) >= FX_Ok)
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


int32_t S_LoadSound(uint32_t num)
{
    int32_t   fp = -1, l;

    if (num >= MAXSOUNDS || ud.config.SoundToggle == 0) return 0;
    if (ud.config.FXDevice < 0) return 0;

    if (g_sounds[num].filename == NULL)
    {
        OSD_Printf(OSD_ERROR "Sound (#%d) not defined!\n",num);
        return 0;
    }

    if (g_sounds[num].filename1) fp = kopen4loadfrommod(g_sounds[num].filename1,g_loadFromGroupOnly);
    if (fp == -1)fp = kopen4loadfrommod(g_sounds[num].filename,g_loadFromGroupOnly);
    if (fp == -1)
    {
//        Bsprintf(ScriptQuotes[113],"g_sounds %s(#%d) not found.",sounds[num],num);
//        P_DoQuote(113,g_player[myconnectindex].ps);
        OSD_Printf(OSDTEXT_RED "Sound %s(#%d) not found!\n",g_sounds[num].filename,num);
        return 0;
    }

    l = kfilelength(fp);
    g_sounds[num].soundsiz = l;

    g_soundlocks[num] = 200;

    allocache((intptr_t *)&g_sounds[num].ptr,l,(char *)&g_soundlocks[num]);
    kread(fp, g_sounds[num].ptr , l);
    kclose(fp);
    return 1;
}

int32_t S_PlaySound3D(int32_t num, int32_t i, const vec3_t *pos)
{
    vec3_t c;
    int32_t sndist,j,k;
    int32_t pitche,pitchs,cs;
    int32_t voice, sndang, ca, pitch;

    //    if(num != 358) return 0;

    if (num >= MAXSOUNDS ||
            ud.config.FXDevice < 0 ||
            ((g_sounds[num].m&8) && ud.lockout) ||
            ud.config.SoundToggle == 0 ||
            g_sounds[num].num >= SOUNDMAX ||
            FX_VoiceAvailable(g_sounds[num].pr) == 0 ||
            (g_player[myconnectindex].ps->timebeforeexit > 0 && g_player[myconnectindex].ps->timebeforeexit <= GAMETICSPERSEC*3) ||
            g_player[myconnectindex].ps->gm&MODE_MENU) return -1;

    if (g_sounds[num].m&128)
    {
        voice = S_PlaySound(num);

        if (voice >= FX_Ok && g_sounds[num].num < SOUNDMAX && i >= 0 && i < MAXSPRITES)
        {
            g_sounds[num].SoundOwner[g_sounds[num].num].i = i;
            g_sounds[num].SoundOwner[g_sounds[num].num].voice = voice;
            g_sounds[num].num++;
        }

        return voice;
    }

    if (g_sounds[num].m&4)
    {
        if ((net_server || ud.multimode > 1) && PN == APLAYER && sprite[i].yvel != screenpeek) // other player sound
        {
            if (!(ud.config.VoiceToggle&4))
                return -1;
        }
        else if (!(ud.config.VoiceToggle&1))
            return -1;
        for (j=g_maxSoundPos; j>=0; j--)
            for (k=0; k<g_sounds[j].num; k++)
                if ((g_sounds[j].num > 0) && (g_sounds[j].m&4))
                    return -1;
    }

    Bmemcpy(&c, &ud.camera, sizeof(vec3_t));
    cs = ud.camerasect;
    ca = ud.cameraang;

    sndist = FindDistance3D((c.x-pos->x),(c.y-pos->y),(c.z-pos->z)>>4);

    if (i >= 0 && (g_sounds[num].m&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9)
        sndist = divscale14(sndist,(SHT+1));

    sndist += g_sounds[num].vo;
    if (sndist < 0) sndist = 0;
    if (cs > -1 && sndist && PN != MUSICANDSFX && !cansee(c.x,c.y,c.z-(24<<8),cs,SX,SY,SZ-(24<<8),SECT))
        sndist += sndist>>5;


    pitchs = g_sounds[num].ps;
    pitche = g_sounds[num].pe;

    j = klabs(pitche-pitchs);

    if (j)
    {
        if (pitchs < pitche)
            pitch = pitchs + (rand()%j);
        else pitch = pitche + (rand()%j);
    }
    else pitch = pitchs;

    switch (num)
    {
    case PIPEBOMB_EXPLODE:
    case LASERTRIP_EXPLODE:
    case RPG_EXPLODE:
        if (sndist > (6144))
            sndist = 6144;
        if (g_player[screenpeek].ps->cursectnum > -1 && sector[g_player[screenpeek].ps->cursectnum].lotag == 2)
            pitch -= 1024;
        break;
    default:
        if (g_player[screenpeek].ps->cursectnum > -1 && sector[g_player[screenpeek].ps->cursectnum].lotag == 2 && (g_sounds[num].m&4) == 0)
            pitch = -768;
        /*
                if (sndist > 31444 && PN != MUSICANDSFX)
                    return -1;
        */
        break;
    }

    if (g_player[screenpeek].ps->sound_pitch) pitch += g_player[screenpeek].ps->sound_pitch;

    if (g_sounds[num].num > 0 && PN != MUSICANDSFX)
    {
        if (g_sounds[num].SoundOwner[0].i == i) S_StopSound(num);
        else if (g_sounds[num].num > 1) S_StopSound(num);
        else if (A_CheckEnemySprite(&sprite[i]) && sprite[i].extra <= 0) S_StopSound(num);
    }

    if (PN == APLAYER && sprite[i].yvel == screenpeek)
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(c.x-pos->x,c.y-pos->y);
        sndang &= 2047;
    }

    if (g_sounds[num].ptr == 0)
    {
        if (S_LoadSound(num) == 0) return 0;
    }
    else
    {
        if (g_soundlocks[num] < 200)
            g_soundlocks[num] = 200;
        else g_soundlocks[num]++;
    }

    if (g_sounds[num].m&16) sndist = 0;

    if (sndist < ((255-LOUDESTVOLUME)<<6))
        sndist = ((255-LOUDESTVOLUME)<<6);

    if (g_sounds[num].m&1)
    {
        if (g_sounds[num].num > 0) return -1;

        voice = FX_PlayLoopedAuto(g_sounds[num].ptr, g_sounds[num].soundsiz, 0, -1,
                                  pitch,sndist>>6,sndist>>6,0,g_sounds[num].pr,num);
    }
    else
    {
        voice = FX_PlayAuto3D(g_sounds[ num ].ptr, g_sounds[num].soundsiz, pitch,sndang>>4,sndist>>6, g_sounds[num].pr, num);
    }

    if (voice >= FX_Ok)
    {
        g_sounds[num].SoundOwner[g_sounds[num].num].i = i;
        g_sounds[num].SoundOwner[g_sounds[num].num].voice = voice;
        g_sounds[num].num++;
    }
    else g_soundlocks[num]--;
    return (voice);
}

int32_t S_PlaySound(int32_t num)
{
    int32_t pitch,pitche,pitchs,cx;
    int32_t voice;

    if (ud.config.FXDevice < 0) return -1;
    if (ud.config.SoundToggle==0) return -1;
    if (!(ud.config.VoiceToggle&1) && (g_sounds[num].m&4)) return -1;
    if ((g_sounds[num].m&8) && ud.lockout) return -1;
    if (FX_VoiceAvailable(g_sounds[num].pr) == 0) return -1;
    if (num > MAXSOUNDS-1 || !g_sounds[num].filename)
    {
        OSD_Printf("WARNING: invalid sound #%d\n",num);
        return -1;
    }

    pitchs = g_sounds[num].ps;
    pitche = g_sounds[num].pe;
    cx = klabs(pitche-pitchs);

    if (cx)
    {
        if (pitchs < pitche)
            pitch = pitchs + (rand()%cx);
        else pitch = pitche + (rand()%cx);
    }
    else pitch = pitchs;

    if (g_sounds[num].ptr == 0)
    {
        if (S_LoadSound(num) == 0) return -1;
    }
    else
    {
        if (g_soundlocks[num] < 200)
            g_soundlocks[num] = 200;
        else g_soundlocks[num]++;
    }

    if (g_sounds[num].m&1)
    {
        voice = FX_PlayLoopedAuto(g_sounds[num].ptr, g_sounds[num].soundsiz, 0, -1,
                                  pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,g_sounds[num].soundsiz,num);
    }
    else
    {
        voice = FX_PlayAuto3D(g_sounds[ num ].ptr, g_sounds[num].soundsiz, pitch,0,255-LOUDESTVOLUME,g_sounds[num].pr, num);
    }

    if (voice >= FX_Ok) return voice;
    g_soundlocks[num]--;
    return -1;
}

int32_t A_PlaySound(uint32_t num, int32_t i)
{
    if (num >= MAXSOUNDS) return -1;

    if (i < 0)
    {
        S_PlaySound(num);
        return 0;
    }

    return S_PlaySound3D(num, i, (vec3_t *)&sprite[i]);
}

void S_StopSound(int32_t num)
{
    if (num >= 0 && num < MAXSOUNDS)
    {
        if (g_sounds[num].num > 0)
        {
            int32_t j=g_sounds[num].num-1;

            for (; j>=0; j--)
            {
                FX_StopSound(g_sounds[num].SoundOwner[j].voice);
                //                    S_TestSoundCallback(num);
            }
        }
    }
}

void S_StopEnvSound(int32_t num,int32_t i)
{
    if (num >= 0 && num < MAXSOUNDS)
    {
        if (g_sounds[num].num > 0)
        {
            int32_t j=g_sounds[num].num-1;

            for (; j>=0; j--)
            {
                if (g_sounds[num].SoundOwner[j].i == i)
                {
                    FX_StopSound(g_sounds[num].SoundOwner[j].voice);
//                    S_TestSoundCallback(num);
                }
            }
        }
    }
}

void S_Pan3D(void)
{
    vec3_t s, c;
    int32_t sndist;
    int32_t sndang,ca,j,k,i,cs;

    g_numEnvSoundsPlaying = 0;

    if (ud.camerasprite == -1)
    {
        Bmemcpy(&c, &ud.camera, sizeof(vec3_t));
        cs = ud.camerasect;
        ca = ud.cameraang;
    }
    else
    {
        Bmemcpy(&c, &sprite[ud.camerasprite], sizeof(vec3_t));
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    j = g_maxSoundPos;

    do
    {
        for (k=g_sounds[j].num-1; k>=0; k--)
        {
            i = g_sounds[j].SoundOwner[k].i;

            if (i < 0 || i >= MAXSPRITES)
            {
                OSD_Printf(OSD_ERROR "S_Pan3D(): INTERNAL ERROR: invalid id %d!\n",i);
                k--;
                continue;
            }
            
            Bmemcpy(&s, &sprite[i], sizeof(vec3_t));

            if (PN == APLAYER && sprite[i].yvel == screenpeek)
            {
                sndang = 0;
                sndist = 0;
            }
            else
            {
                sndang = 2048 + ca - getangle(c.x-s.x,c.y-s.y);
                sndang &= 2047;
                sndist = FindDistance3D((c.x-s.x),(c.y-s.y),(c.z-s.z)>>4);
                if (i >= 0 && (g_sounds[j].m&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9)
                    sndist = divscale14(sndist,(SHT+1));
            }

            sndist += g_sounds[j].vo;
            if (sndist < 0) sndist = 0;

            if (cs > -1 && sndist && PN != MUSICANDSFX && !cansee(c.x,c.y,c.z-(24<<8),cs,s.x,s.y,s.z-(24<<8),SECT))
                sndist += sndist>>5;

            if (PN == MUSICANDSFX && SLT < 999)
                g_numEnvSoundsPlaying++;

            switch (j)
            {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if (sndist > (6144)) sndist = (6144);
                break;
                /*
                default:
                if (sndist > 31444 && PN != MUSICANDSFX)
                {
                S_StopSound(j);
                continue;
                }
                */
            }

            if (g_sounds[j].ptr == 0 && S_LoadSound(j) == 0) continue;
            if (g_sounds[j].m&16) sndist = 0;

            if (sndist < ((255-LOUDESTVOLUME)<<6))
                sndist = ((255-LOUDESTVOLUME)<<6);

            FX_Pan3D(g_sounds[j].SoundOwner[k].voice,sndang>>4,sndist>>6);
        }
    }
    while (j--);
}

void S_TestSoundCallback(uint32_t num)
{
    if ((int32_t)num == MUSIC_ID)
        return;

    if ((int32_t)num < 0)
    {
        if (lumplockbyte[-(int32_t)num] >= 200)
            lumplockbyte[-(int32_t)num]--;
        return;
    }

    {
        int32_t j = 0;

        while (j < g_sounds[num].num)
        {
            if (!FX_SoundActive(g_sounds[num].SoundOwner[j].voice))
            {
                int32_t i = g_sounds[num].SoundOwner[j].i;

                g_sounds[num].num--;
                //            OSD_Printf("removing sound %d from spr %d\n",num,i);
                if (sprite[i].picnum == MUSICANDSFX && sector[sprite[i].sectnum].lotag < 3 && sprite[i].lotag < 999)
                    ActorExtra[i].temp_data[0] = 0;
                Bmemmove(&g_sounds[num].SoundOwner[j], &g_sounds[num].SoundOwner[j+1], sizeof(SOUNDOWNER) * (SOUNDMAX-j-1));
                break;
            }
            j++;
        }
    }
    g_soundlocks[num]--;
}

void S_ClearSoundLocks(void)
{
    int32_t i;

    for (i=0; i<MAXSOUNDS; i++)
        if (g_soundlocks[i] >= 200)
            g_soundlocks[i] = 199;

    for (i=0; i<11; i++)
        if (lumplockbyte[i] >= 200)
            lumplockbyte[i] = 199;
}

int32_t A_CheckSoundPlaying(int32_t i, int32_t num)
{
    int32_t j;

    if (num >= MAXSOUNDS || num < 0) return 0;
    if (i == -1) return (g_sounds[num].num > 0);

    if (g_sounds[num].num > 0)
    {
        for (j=g_sounds[num].num-1; j>=0; j--)
            if (g_sounds[num].SoundOwner[j].i == i)
            {
                return 1;
            }
    }

    return 0;
}

int32_t S_CheckSoundPlaying(int32_t i, int32_t num)
{
    if (num >= MAXSOUNDS || num < 0) return 0;

    if (i == -1)
    {
        if (g_soundlocks[num] >= 200)
            return 1;
        return 0;
    }

    return(g_sounds[num].num);
}
