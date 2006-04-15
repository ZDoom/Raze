//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

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


#define LOUDESTVOLUME 150

long backflag,numenvsnds;

/*
===================
=
= SoundStartup
=
===================
*/

void SoundStartup( void )
{
   int32 status;

   // if they chose None lets return
   if (FXDevice < 0) return;

   status = FX_Init( FXDevice, NumVoices, NumChannels, NumBits, MixRate );
   if ( status == FX_Ok ) {
      FX_SetVolume( FXVolume );
      if (ReverseStereo == 1) {
         FX_SetReverseStereo(!FX_GetReverseStereo());
      }
	  status = FX_SetCallBack( testcallback );
  }

   if ( status != FX_Ok ) {
	  sprintf(tempbuf, "Sound startup error: %s", FX_ErrorString( FX_Error ));
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

void SoundShutdown( void )
{
   int32 status;

   // if they chose None lets return
   if (FXDevice < 0)
      return;

   status = FX_Shutdown();
   if ( status != FX_Ok ) {
	  sprintf(tempbuf, "Sound shutdown error: %s", FX_ErrorString( FX_Error ));
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

void MusicStartup( void )
{
    int32 status;

    // if they chose None lets return
    if (MusicDevice < 0)
        return;

    status = MUSIC_Init( MusicDevice, 0 );

    if ( status == MUSIC_Ok )
    {
        MUSIC_SetVolume( MusicVolume );
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

void MusicShutdown( void )
{
    int32 status;

    // if they chose None lets return
    if (MusicDevice < 0)
        return;

    status = MUSIC_Shutdown();
    if ( status != MUSIC_Ok )
    {
        Error( MUSIC_ErrorString( MUSIC_ErrorCode ));
    }
}

void MusicUpdate(void)
{
    MUSIC_Update();
}

int USRHOOKS_GetMem(char **ptr, unsigned long size )
{
    *ptr = malloc(size);

    if (*ptr == NULL)
        return(USRHOOKS_Error);

    return( USRHOOKS_Ok);

}

int USRHOOKS_FreeMem(char *ptr)
{
    free(ptr);
    return( USRHOOKS_Ok);
}

char menunum=0;

void intomenusounds(void)
{
    short i;
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

void playmusic(char *fn)
{
    short      fp;
    long        l;

    if(MusicToggle == 0) return;
    if(MusicDevice < 0) return;

    fp = kopen4load(fn,0);

    if(fp == -1) return;

    l = kfilelength( fp );
    if(l >= (signed long)sizeof(MusicPtr))
    {
        kclose(fp);
        return;
    }

    kread( fp, MusicPtr, l);
    kclose( fp );
    MUSIC_PlaySong( MusicPtr, MUSIC_LoopSong );
}

char loadsound(unsigned short num)
{
    long   fp, l;

    if(num >= NUM_SOUNDS || SoundToggle == 0) return 0;
    if (FXDevice < 0) return 0;

    fp = kopen4load(sounds[num],loadfromgrouponly);
    if(fp == -1)
    {
        sprintf(&fta_quotes[113][0],"Sound %s(#%d) not found.",sounds[num],num);
        FTA(113,&ps[myconnectindex]);
        return 0;
    }

    l = kfilelength( fp );
    soundsiz[num] = l;

    Sound[num].lock = 200;

    allocache((long *)&Sound[num].ptr,l,(char *)&Sound[num].lock);
    kread( fp, Sound[num].ptr , l);
    kclose( fp );
    return 1;
}

int xyzsound(short num,short i,long x,long y,long z)
{
    long sndist, cx, cy, cz, j,k;
    short pitche,pitchs,cs;
    int voice, sndang, ca, pitch;

    //    if(num != 358) return 0;

    if( num >= NUM_SOUNDS ||
            FXDevice < 0 ||
            ( (soundm[num]&8) && ud.lockout ) ||
            SoundToggle == 0 ||
            Sound[num].num > 3 ||
            FX_VoiceAvailable(soundpr[num]) == 0 ||
            (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
            ps[myconnectindex].gm&MODE_MENU) return -1;

    if( soundm[num]&128 )
    {
        sound(num);
        return 0;
    }

    if( soundm[num]&4 )
    {
        if(VoiceToggle==0)
			return -1;
        else if (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && (!(gametype_flags[ud.coop]&GAMETYPE_FLAG_COOPSOUND)?1:VoiceToggle!=2))
			return -1;
        for(j=0;j<NUM_SOUNDS;j++)
            for(k=0;k<Sound[j].num;k++)
                if( (Sound[j].num > 0) && (soundm[j]&4) )
                    return -1;
    }

    cx = ps[screenpeek].oposx;
    cy = ps[screenpeek].oposy;
    cz = ps[screenpeek].oposz;
    cs = ps[screenpeek].cursectnum;
    ca = ps[screenpeek].ang+ps[screenpeek].look_ang;

    sndist = FindDistance3D((cx-x),(cy-y),(cz-z)>>4);

    if( i >= 0 && (soundm[num]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
        sndist = divscale14(sndist,(SHT+1));

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
            pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

    sndist += soundvo[num];
    if(sndist < 0) sndist = 0;
    if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT) )
        sndist += sndist>>5;

    switch(num)
    {
    case PIPEBOMB_EXPLODE:
    case LASERTRIP_EXPLODE:
    case RPG_EXPLODE:
        if(sndist > (6144) )
            sndist = 6144;
        if(sector[ps[screenpeek].cursectnum].lotag == 2)
            pitch -= 1024;
        break;
    default:
        if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&4) == 0)
            pitch = -768;
        if( sndist > 31444 && PN != MUSICANDSFX)
            return -1;
        break;
    }

    if (ps[screenpeek].sound_pitch) pitch = ps[screenpeek].sound_pitch;

    if( Sound[num].num > 0 && PN != MUSICANDSFX )
    {
        if( SoundOwner[num][0].i == i ) stopsound(num);
        else if( Sound[num].num > 1 ) stopsound(num);
        else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
    }

    if( PN == APLAYER && sprite[i].yvel == screenpeek )
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(cx-x,cy-y);
        sndang &= 2047;
    }

    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return 0; }
    else
    {
        if (Sound[num].lock < 200)
            Sound[num].lock = 200;
        else Sound[num].lock++;
    }

    if( soundm[num]&16 ) sndist = 0;

    if(sndist < ((255-LOUDESTVOLUME)<<6) )
        sndist = ((255-LOUDESTVOLUME)<<6);

    if( soundm[num]&1 )
    {
        unsigned short start;

        if(Sound[num].num > 0) return -1;

        start = *(unsigned short *)(Sound[num].ptr + 0x14);

        if(*Sound[num].ptr == 'C')
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + soundsiz[num],
                                      pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
        else
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + soundsiz[num],
                                      pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
    }
    else
    {
        if( *Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, soundpr[num], num );
        else
            voice = FX_PlayWAV3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, soundpr[num], num );
    }

    if ( voice > FX_Ok )
    {
        SoundOwner[num][Sound[num].num].i = i;
        SoundOwner[num][Sound[num].num].voice = voice;
        Sound[num].num++;
    }
    else Sound[num].lock--;
    return (voice);
}

void sound(short num)
{
    short pitch,pitche,pitchs,cx;
    int voice;
    long start;

    if (FXDevice < 0) return;
    if(SoundToggle==0) return;
    if(VoiceToggle==0 && (soundm[num]&4) ) return;
    if( (soundm[num]&8) && ud.lockout ) return;
    if(FX_VoiceAvailable(soundpr[num]) == 0) return;

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
            pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return; }
    else
    {
        if (Sound[num].lock < 200)
            Sound[num].lock = 200;
        else Sound[num].lock++;
    }

    if( soundm[num]&1 )
    {
        if(*Sound[num].ptr == 'C')
        {
            start = (long)*(unsigned short *)(Sound[num].ptr + 0x14);
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + soundsiz[num],
                                      pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
        else
        {
            start = (long)*(unsigned short *)(Sound[num].ptr + 0x14);
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + soundsiz[num],
                                      pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
    }
    else
    {
        if(*Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
        else
            voice = FX_PlayWAV3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
    }

    if(voice > FX_Ok) return;
    Sound[num].lock--;
}

int spritesound(unsigned short num, short i)
{
    if(num >= NUM_SOUNDS) return -1;
    return xyzsound(num,i,SX,SY,SZ);
}

void stopspritesound(short num, short i)
{
	stopsound(num);
}

void stopsound(short num)
{
    if(Sound[num].num > 0)
    {
        FX_StopSound(SoundOwner[num][Sound[num].num-1].voice);
        testcallback(num);
    }
}

void stopenvsound(short num,short i)
{
    short j, k;

    if(Sound[num].num > 0)
    {
        k = Sound[num].num;
        for(j=0;j<k;j++)
            if(SoundOwner[num][j].i == i)
            {
                FX_StopSound(SoundOwner[num][j].voice);
                break;
            }
    }
}

void pan3dsound(void)
{
    long sndist, sx, sy, sz, cx, cy, cz;
    short sndang,ca,j,k,i,cs;

    numenvsnds = 0;

    if(ud.camerasprite == -1)
    {
        cx = ps[screenpeek].oposx;
        cy = ps[screenpeek].oposy;
        cz = ps[screenpeek].oposz;
        cs = ps[screenpeek].cursectnum;
        ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
    }
    else
    {
        cx = sprite[ud.camerasprite].x;
        cy = sprite[ud.camerasprite].y;
        cz = sprite[ud.camerasprite].z;
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    for(j=0;j<NUM_SOUNDS;j++) for(k=0;k<Sound[j].num;k++)
        {
            i = SoundOwner[j][k].i;

            sx = sprite[i].x;
            sy = sprite[i].y;
            sz = sprite[i].z;

            if( PN == APLAYER && sprite[i].yvel == screenpeek)
            {
                sndang = 0;
                sndist = 0;
            }
            else
            {
                sndang = 2048 + ca - getangle(cx-sx,cy-sy);
                sndang &= 2047;
                sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
                if( i >= 0 && (soundm[j]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
                    sndist = divscale14(sndist,(SHT+1));
            }

            sndist += soundvo[j];
            if(sndist < 0) sndist = 0;

            if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
                sndist += sndist>>5;

            if(PN == MUSICANDSFX && SLT < 999)
                numenvsnds++;

            switch(j)
            {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if(sndist > (6144)) sndist = (6144);
                break;
            default:
                if( sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
            }

            if(Sound[j].ptr == 0 && loadsound(j) == 0 ) continue;
            if( soundm[j]&16 ) sndist = 0;

            if(sndist < ((255-LOUDESTVOLUME)<<6) )
                sndist = ((255-LOUDESTVOLUME)<<6);

            FX_Pan3D(SoundOwner[j][k].voice,sndang>>6,sndist>>6);
        }
}

void testcallback(unsigned long num)
{
    short tempi,tempj,tempk;

    if((long)num < 0)
    {
        if(lumplockbyte[-num] >= 200)
            lumplockbyte[-num]--;
        return;
    }

    tempk = Sound[num].num;

    if(tempk > 0)
    {
        if( (soundm[num]&16) == 0)
            for(tempj=0;tempj<tempk;tempj++)
            {
                tempi = SoundOwner[num][tempj].i;
                if(sprite[tempi].picnum == MUSICANDSFX && sector[sprite[tempi].sectnum].lotag < 3 && sprite[tempi].lotag < 999)
                {
                    hittype[tempi].temp_data[0] = 0;
                    if( (tempj + 1) < tempk )
                    {
                        SoundOwner[num][tempj].voice = SoundOwner[num][tempk-1].voice;
                        SoundOwner[num][tempj].i     = SoundOwner[num][tempk-1].i;
                    }
                    break;
                }
            }

        Sound[num].num--;
        SoundOwner[num][tempk-1].i = -1;
    }

    Sound[num].lock--;
}

void clearsoundlocks(void)
{
    long i;

    for(i=0;i<NUM_SOUNDS;i++)
        if(Sound[i].lock >= 200)
            Sound[i].lock = 199;

    for(i=0;i<11;i++)
        if(lumplockbyte[i] >= 200)
            lumplockbyte[i] = 199;
}

int isspritemakingsound(short i, int num)
{
	if (num < 0) num=0;	// FIXME
	return issoundplaying(num) > 0;
}

int issoundplaying(int num)
{
	return Sound[num].num;
}
