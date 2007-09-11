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
#include "types.h"
#include "fx_man.h"
#include "music.h"
#include "duke3d.h"
#include "util_lib.h"
#include "osd.h"

#define LOUDESTVOLUME 150

long backflag,numenvsnds;

/*
===================
=
= SoundStartup
=
===================
*/

void SoundStartup(void)
{
    int32 status;

    // if they chose None lets return
    if (ud.config.FXDevice < 0) return;

    status = FX_Init(ud.config.FXDevice, ud.config.NumVoices, ud.config.NumChannels, ud.config.NumBits, ud.config.MixRate);
    if (status == FX_Ok)
    {
        FX_SetVolume(ud.config.FXVolume);
        if (ud.config.ReverseStereo == 1)
        {
            FX_SetReverseStereo(!FX_GetReverseStereo());
        }
        status = FX_SetCallBack(testcallback);
    }

    if (status != FX_Ok)
    {
        sprintf(tempbuf, "Sound startup error: %s", FX_ErrorString(FX_Error));
        gameexit(tempbuf);
    }
}

/*
===================
=
= SoundShutdown
=
===================
*/

void SoundShutdown(void)
{
    int32 status;

    // if they chose None lets return
    if (ud.config.FXDevice < 0)
        return;

    status = FX_Shutdown();
    if (status != FX_Ok)
    {
        sprintf(tempbuf, "Sound shutdown error: %s", FX_ErrorString(FX_Error));
        gameexit(tempbuf);
    }
}

/*
===================
=
= MusicStartup
=
===================
*/

void MusicStartup(void)
{
    int32 status;

    // if they chose None lets return
    if (ud.config.MusicDevice < 0)
        return;

    status = MUSIC_Init(ud.config.MusicDevice, 0);

    if (status == MUSIC_Ok)
    {
        MUSIC_SetVolume(ud.config.MusicVolume);
    }
    else
    {
        initprintf("Couldn't find selected sound card, or, error w/ sound card itself.\n");

        SoundShutdown();
        uninittimer();
        uninitengine();
        CONTROL_Shutdown();
        CONFIG_WriteSetup();
        KB_Shutdown();
        uninitgroupfile();
        //unlink("duke3d.tmp");
        exit(-1);
    }
}

/*
===================
=
= MusicShutdown
=
===================
*/

void MusicShutdown(void)
{
    int32 status;

    // if they chose None lets return
    if (ud.config.MusicDevice < 0)
        return;

    status = MUSIC_Shutdown();
    if (status != MUSIC_Ok)
    {
        Error(MUSIC_ErrorString(MUSIC_ErrorCode));
    }
}

void MusicUpdate(void)
{
    MUSIC_Update();
}

unsigned char menunum=0;

void intomenusounds(void)
{
    short menusnds[] =
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
    sound(menusnds[menunum++]);
    menunum %= 17;
}

void playmusic(const char *fn)
{
#if defined(_WIN32)
    short      fp;
    long        l;

    if (fn == NULL) return;

    if (ud.config.MusicToggle == 0) return;
    if (ud.config.MusicDevice < 0) return;

    fp = kopen4load((char *)fn,0);

    if (fp == -1) return;

    l = kfilelength(fp);
    if (l >= (signed long)sizeof(MusicPtr))
    {
        kclose(fp);
        return;
    }

    kread(fp, MusicPtr, l);
    kclose(fp);
    MUSIC_PlaySong((unsigned char *)MusicPtr, MUSIC_LoopSong);
#else
    void PlayMusic(char *_filename);

    if (fn == NULL) return;

    if (ud.config.MusicToggle == 0) return;
    if (ud.config.MusicDevice < 0) return;

    // FIXME: I need this to get the music volume initialized (not sure why) -- Jim Bentler
    MUSIC_SetVolume(ud.config.MusicVolume);
    PlayMusic((char *)fn);
#endif
}

int loadsound(unsigned int num)
{
    long   fp, l;

    if (num >= NUM_SOUNDS || ud.config.SoundToggle == 0) return 0;
    if (ud.config.FXDevice < 0) return 0;

    fp = kopen4load(g_sounds[num].filename,loadfromgrouponly);
    if (fp == -1)
    {
//        Bsprintf(fta_quotes[113],"Sound %s(#%d) not found.",sounds[num],num);
//        FTA(113,g_player[myconnectindex].ps);
        initprintf("Sound %s(#%d) not found.\n",g_sounds[num].filename,num);
        return 0;
    }

    l = kfilelength(fp);
    g_sounds[num].soundsiz = l;

    g_sounds[num].lock = 200;

    allocache((long *)&g_sounds[num].ptr,l,(char *)&g_sounds[num].lock);
    kread(fp, g_sounds[num].ptr , l);
    kclose(fp);
    return 1;
}

int xyzsound(int num,int i,long x,long y,long z)
{
    long sndist, cx, cy, cz, j,k;
    int pitche,pitchs,cs;
    int voice, sndang, ca, pitch;

    //    if(num != 358) return 0;

    if (num >= NUM_SOUNDS ||
            ud.config.FXDevice < 0 ||
            ((g_sounds[num].m&8) && ud.lockout) ||
            ud.config.SoundToggle == 0 ||
            g_sounds[num].num > 3 ||
            FX_VoiceAvailable(g_sounds[num].pr) == 0 ||
            (g_player[myconnectindex].ps->timebeforeexit > 0 && g_player[myconnectindex].ps->timebeforeexit <= 26*3) ||
            g_player[myconnectindex].ps->gm&MODE_MENU) return -1;

    if (g_sounds[num].m&128)
    {
        sound(num);
        return 0;
    }

    if (g_sounds[num].m&4)
    {
        if (ud.config.VoiceToggle==0)
            return -1;
        else if (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && ud.config.VoiceToggle!=2)
            return -1;
        for (j=0;j<NUM_SOUNDS;j++)
            for (k=0;k<g_sounds[j].num;k++)
                if ((g_sounds[j].num > 0) && (g_sounds[j].m&4))
                    return -1;
    }

    cx = g_player[screenpeek].ps->oposx;
    cy = g_player[screenpeek].ps->oposy;
    cz = g_player[screenpeek].ps->oposz;
    cs = g_player[screenpeek].ps->cursectnum;
    ca = g_player[screenpeek].ps->ang+g_player[screenpeek].ps->look_ang;

    sndist = FindDistance3D((cx-x),(cy-y),(cz-z)>>4);

    if (i >= 0 && (g_sounds[num].m&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9)
        sndist = divscale14(sndist,(SHT+1));

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

    sndist += g_sounds[num].vo;
    if (sndist < 0) sndist = 0;
    if (sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT))
        sndist += sndist>>5;

    switch (num)
    {
    case PIPEBOMB_EXPLODE:
    case LASERTRIP_EXPLODE:
    case RPG_EXPLODE:
        if (sndist > (6144))
            sndist = 6144;
        if (sector[g_player[screenpeek].ps->cursectnum].lotag == 2)
            pitch -= 1024;
        break;
    default:
        if (sector[g_player[screenpeek].ps->cursectnum].lotag == 2 && (g_sounds[num].m&4) == 0)
            pitch = -768;
        if (sndist > 31444 && PN != MUSICANDSFX)
            return -1;
        break;
    }

    if (g_player[screenpeek].ps->sound_pitch) pitch += g_player[screenpeek].ps->sound_pitch;

    if (g_sounds[num].num > 0 && PN != MUSICANDSFX)
    {
        if (g_sounds[num].SoundOwner[0].i == i) stopsound(num);
        else if (g_sounds[num].num > 1) stopsound(num);
        else if (badguy(&sprite[i]) && sprite[i].extra <= 0) stopsound(num);
    }

    if (PN == APLAYER && sprite[i].yvel == screenpeek)
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(cx-x,cy-y);
        sndang &= 2047;
    }

    if (g_sounds[num].ptr == 0)
    {
        if (loadsound(num) == 0) return 0;
    }
    else
    {
        if (g_sounds[num].lock < 200)
            g_sounds[num].lock = 200;
        else g_sounds[num].lock++;
    }

    if (g_sounds[num].m&16) sndist = 0;

    if (sndist < ((255-LOUDESTVOLUME)<<6))
        sndist = ((255-LOUDESTVOLUME)<<6);

    if (g_sounds[num].m&1)
    {
        unsigned short start;

        if (g_sounds[num].num > 0) return -1;

        start = *(unsigned short *)(g_sounds[num].ptr + 0x14);

        if (*g_sounds[num].ptr == 'C')
            voice = FX_PlayLoopedVOC(g_sounds[num].ptr, start, start + g_sounds[num].soundsiz,
                                     pitch,sndist>>6,sndist>>6,0,g_sounds[num].pr,num);
        else
            voice = FX_PlayLoopedWAV(g_sounds[num].ptr, start, start + g_sounds[num].soundsiz,
                                     pitch,sndist>>6,sndist>>6,0,g_sounds[num].pr,num);
    }
    else
    {
        if (*g_sounds[num].ptr == 'C')
            voice = FX_PlayVOC3D(g_sounds[ num ].ptr,pitch,sndang>>6,sndist>>6, g_sounds[num].pr, num);
        else
            voice = FX_PlayWAV3D(g_sounds[ num ].ptr,pitch,sndang>>6,sndist>>6, g_sounds[num].pr, num);
    }

    if (voice > FX_Ok)
    {
        g_sounds[num].SoundOwner[g_sounds[num].num].i = i;
        g_sounds[num].SoundOwner[g_sounds[num].num].voice = voice;
        g_sounds[num].num++;
    }
    else g_sounds[num].lock--;
    return (voice);
}

void sound(int num)
{
    int pitch,pitche,pitchs,cx;
    int voice;
    long start;

    if (ud.config.FXDevice < 0) return;
    if (ud.config.SoundToggle==0) return;
    if (ud.config.VoiceToggle==0 && (g_sounds[num].m&4)) return;
    if ((g_sounds[num].m&8) && ud.lockout) return;
    if (FX_VoiceAvailable(g_sounds[num].pr) == 0) return;
    if (num > NUM_SOUNDS-1 || !g_sounds[num].filename)
    {
        OSD_Printf("WARNING: invalid sound #%d\n",num);
        return;
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
        if (loadsound(num) == 0) return;
    }
    else
    {
        if (g_sounds[num].lock < 200)
            g_sounds[num].lock = 200;
        else g_sounds[num].lock++;
    }

    if (g_sounds[num].m&1)
    {
        if (*g_sounds[num].ptr == 'C')
        {
            start = (long)*(unsigned short *)(g_sounds[num].ptr + 0x14);
            voice = FX_PlayLoopedVOC(g_sounds[num].ptr, start, start + g_sounds[num].soundsiz,
                                     pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,g_sounds[num].pr,num);
        }
        else
        {
            start = (long)*(unsigned short *)(g_sounds[num].ptr + 0x14);
            voice = FX_PlayLoopedWAV(g_sounds[num].ptr, start, start + g_sounds[num].soundsiz,
                                     pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,g_sounds[num].pr,num);
        }
    }
    else
    {
        if (*g_sounds[num].ptr == 'C')
            voice = FX_PlayVOC3D(g_sounds[ num ].ptr, pitch,0,255-LOUDESTVOLUME,g_sounds[num].pr, num);
        else
            voice = FX_PlayWAV3D(g_sounds[ num ].ptr, pitch,0,255-LOUDESTVOLUME,g_sounds[num].pr, num);
    }

    if (voice > FX_Ok) return;
    g_sounds[num].lock--;
}

int spritesound(unsigned int num, int i)
{
    if (num >= NUM_SOUNDS) return -1;
    return xyzsound(num,i,SX,SY,SZ);
}

void stopspritesound(int num, int i)
{
    stopsound(num);
}

void stopsound(int num)
{
    if (g_sounds[num].num > 0)
    {
        FX_StopSound(g_sounds[num].SoundOwner[g_sounds[num].num-1].voice);
        testcallback(num);
    }
}

void stopenvsound(int num,int i)
{
    int j, k;

    if (g_sounds[num].num > 0)
    {
        k = g_sounds[num].num;
        for (j=0;j<k;j++)
            if (g_sounds[num].SoundOwner[j].i == i)
            {
                FX_StopSound(g_sounds[num].SoundOwner[j].voice);
                break;
            }
    }
}

void pan3dsound(void)
{
    long sndist, sx, sy, sz, cx, cy, cz;
    int sndang,ca,j,k,i,cs;

    numenvsnds = 0;

    if (ud.camerasprite == -1)
    {
        cx = g_player[screenpeek].ps->oposx;
        cy = g_player[screenpeek].ps->oposy;
        cz = g_player[screenpeek].ps->oposz;
        cs = g_player[screenpeek].ps->cursectnum;
        ca = g_player[screenpeek].ps->ang+g_player[screenpeek].ps->look_ang;
    }
    else
    {
        cx = sprite[ud.camerasprite].x;
        cy = sprite[ud.camerasprite].y;
        cz = sprite[ud.camerasprite].z;
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    for (j=0;j<NUM_SOUNDS;j++) for (k=0;k<g_sounds[j].num;k++)
        {
            i = g_sounds[j].SoundOwner[k].i;

            sx = sprite[i].x;
            sy = sprite[i].y;
            sz = sprite[i].z;

            if (PN == APLAYER && sprite[i].yvel == screenpeek)
            {
                sndang = 0;
                sndist = 0;
            }
            else
            {
                sndang = 2048 + ca - getangle(cx-sx,cy-sy);
                sndang &= 2047;
                sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
                if (i >= 0 && (g_sounds[j].m&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9)
                    sndist = divscale14(sndist,(SHT+1));
            }

            sndist += g_sounds[j].vo;
            if (sndist < 0) sndist = 0;

            if (sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT))
                sndist += sndist>>5;

            if (PN == MUSICANDSFX && SLT < 999)
                numenvsnds++;

            switch (j)
            {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if (sndist > (6144)) sndist = (6144);
                break;
            default:
                if (sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
            }

            if (g_sounds[j].ptr == 0 && loadsound(j) == 0) continue;
            if (g_sounds[j].m&16) sndist = 0;

            if (sndist < ((255-LOUDESTVOLUME)<<6))
                sndist = ((255-LOUDESTVOLUME)<<6);

            FX_Pan3D(g_sounds[j].SoundOwner[k].voice,sndang>>6,sndist>>6);
        }
}

void testcallback(unsigned long num)
{
    int tempi,tempj,tempk;

    if ((long)num < 0)
    {
        if (lumplockbyte[-num] >= 200)
            lumplockbyte[-num]--;
        return;
    }

    tempk = g_sounds[num].num;

    if (tempk > 0)
    {
        if ((g_sounds[num].m&16) == 0)
            for (tempj=0;tempj<tempk;tempj++)
            {
                tempi = g_sounds[num].SoundOwner[tempj].i;
                if (sprite[tempi].picnum == MUSICANDSFX && sector[sprite[tempi].sectnum].lotag < 3 && sprite[tempi].lotag < 999)
                {
                    hittype[tempi].temp_data[0] = 0;
                    if ((tempj + 1) < tempk)
                    {
                        g_sounds[num].SoundOwner[tempj].voice = g_sounds[num].SoundOwner[tempk-1].voice;
                        g_sounds[num].SoundOwner[tempj].i     = g_sounds[num].SoundOwner[tempk-1].i;
                    }
                    break;
                }
            }

        g_sounds[num].num--;
        g_sounds[num].SoundOwner[tempk-1].i = -1;
    }

    g_sounds[num].lock--;
}

void clearsoundlocks(void)
{
    long i;

    for (i=0;i<NUM_SOUNDS;i++)
        if (g_sounds[i].lock >= 200)
            g_sounds[i].lock = 199;

    for (i=0;i<11;i++)
        if (lumplockbyte[i] >= 200)
            lumplockbyte[i] = 199;
}

int isspritemakingsound(int i, int num)
{
    if (num < 0) num=0;	// FIXME
    return (g_sounds[num].num > 0);
}

int issoundplaying(int i, int num)
{
    if (i == -1)
    {
        if (g_sounds[num].lock == 200)
            return 1;
        return 0;
    }
    return(g_sounds[num].num);
}
