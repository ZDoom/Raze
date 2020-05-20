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

#define game_c_

#include "duke3d.h"
#include "compat.h"
#include "baselayer.h"
#include "osdcmds.h"
#include "net.h"
#include "menus.h"
#include "savegame.h"
#include "anim.h"
#include "demo.h"

#include "cheats.h"
#include "sbar.h"
#include "screens.h"
#include "cmdline.h"
#include "palette.h"
#include "gamecvars.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "filesystem.h"
#include "statistics.h"
#include "c_dispatch.h"
#include "mapinfo.h"
#include "v_video.h"
#include "glbackend/glbackend.h"

// Uncomment to prevent anything except mirrors from drawing. It is sensible to
// also uncomment ENGINE_CLEAR_SCREEN in build/src/engine_priv.h.
//#define DEBUG_MIRRORS_ONLY

#if KRANDDEBUG
# define GAME_INLINE
# define GAME_STATIC
#else
# define GAME_INLINE inline
# define GAME_STATIC static
#endif

BEGIN_DUKE_NS

void SetDispatcher();

int32_t g_quitDeadline = 0;

int32_t g_cameraDistance = 0, g_cameraClock = 0;
static int32_t g_quickExit;

char boardfilename[BMAX_PATH] = {0};

int32_t voting = -1;
int32_t vote_map = -1, vote_episode = -1;

int32_t g_Debug = 0;

const char *defaultrtsfilename[GAMECOUNT] = { "DUKE.RTS", "REDNECK.RTS", "REDNECK.RTS", "NAM.RTS", "NAPALM.RTS" };

int32_t g_Shareware = 0;

int32_t tempwallptr;

static int32_t nonsharedtimer;

int32_t ticrandomseed;

GAME_STATIC GAME_INLINE int32_t G_MoveLoop(void);

int32_t g_levelTextTime = 0;

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

const char *G_DefaultRtsFile(void)
{
    if (DUKE)
        return defaultrtsfilename[GAME_DUKE];
    else if (NAPALM)
    {
        if (!fileSystem.FileExists(defaultrtsfilename[GAME_NAPALM]) && fileSystem.FileExists(defaultrtsfilename[GAME_NAM]))
            return defaultrtsfilename[GAME_NAM]; // NAM/NAPALM Sharing
        else
            return defaultrtsfilename[GAME_NAPALM];
    }
    else if (NAM)
    {
        if (!fileSystem.FileExists(defaultrtsfilename[GAME_NAM]) && fileSystem.FileExists(defaultrtsfilename[GAME_NAPALM]))
            return defaultrtsfilename[GAME_NAPALM]; // NAM/NAPALM Sharing
        else
            return defaultrtsfilename[GAME_NAM];
    }
    else if (RR)
        return defaultrtsfilename[GAME_RR];

    return defaultrtsfilename[0];
}

enum gametokens
{
    T_INCLUDE = 0,
    T_INTERFACE = 0,
    T_LOADGRP = 1,
    T_MODE = 1,
    T_CACHESIZE = 2,
    T_ALLOW = 2,
    T_NOAUTOLOAD,
    T_INCLUDEDEFAULT,
    T_MUSIC,
    T_SOUND,
    T_FILE,
    T_CUTSCENE,
    T_ANIMSOUNDS,
    T_NOFLOORPALRANGE,
    T_ID,
    T_MINPITCH,
    T_MAXPITCH,
    T_PRIORITY,
    T_TYPE,
    T_DISTANCE,
    T_VOLUME,
    T_DELAY,
    T_RENAMEFILE,
    T_GLOBALGAMEFLAGS,
    T_ASPECT,
    T_FORCEFILTER,
    T_FORCENOFILTER,
    T_TEXTUREFILTER,
};

static void gameTimerHandler(void)
{
    S_Update();
    G_HandleSpecialKeys();
}

void G_HandleSpecialKeys(void)
{
    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    // only dispatch commands here when not in a game
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_DispatchQueued();

    if (g_quickExit == 0 && inputState.GetKeyStatus(sc_LeftControl) && inputState.GetKeyStatus(sc_LeftAlt) && (inputState.GetKeyStatus(sc_Delete)||inputState.GetKeyStatus(sc_End)))
    {
        g_quickExit = 1;
        G_GameExit("Quick Exit.");
    }
}

void G_GameQuit(void)
{
    if (numplayers < 2)
        G_GameExit(" ");

    if (g_gameQuit == 0)
    {
        g_gameQuit = 1;
        g_quitDeadline = (int32_t) totalclock+120;
        //g_netDisconnect = 1;
    }

    if ((totalclock > g_quitDeadline) && (g_gameQuit == 1))
        g_netDisconnect = 1;
        //G_GameExit("Timed out.");
}


void OnMotorcycle(DukePlayer_t *pPlayer, int spriteNum)
{
    if (!pPlayer->OnMotorcycle && !(sector[pPlayer->cursectnum].lotag == 2))
    {
        if (spriteNum)
        {
            pPlayer->pos.x = sprite[spriteNum].x;
            pPlayer->pos.y = sprite[spriteNum].y;
            pPlayer->q16ang = F16(sprite[spriteNum].ang);
            pPlayer->ammo_amount[MOTORCYCLE_WEAPON] = sprite[spriteNum].owner;
            A_DeleteSprite(spriteNum);
        }
        pPlayer->over_shoulder_on = 0;
        pPlayer->OnMotorcycle = 1;
        pPlayer->last_full_weapon = pPlayer->curr_weapon;
        pPlayer->curr_weapon = MOTORCYCLE_WEAPON;
        pPlayer->gotweapon.Set(MOTORCYCLE_WEAPON);
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->q16horiz = F16(100);
    }
    if (!A_CheckSoundPlaying(pPlayer->i,186))
        A_PlaySound(186, pPlayer->i);
}

void OffMotorcycle(DukePlayer_t *pPlayer)
{
    int j;
    if (pPlayer->OnMotorcycle)
    {
        if (A_CheckSoundPlaying(pPlayer->i,188))
            S_StopEnvSound(188,pPlayer->i);
        if (A_CheckSoundPlaying(pPlayer->i,187))
            S_StopEnvSound(187,pPlayer->i);
        if (A_CheckSoundPlaying(pPlayer->i,186))
            S_StopEnvSound(186,pPlayer->i);
        if (A_CheckSoundPlaying(pPlayer->i,214))
            S_StopEnvSound(214,pPlayer->i);
        if (!A_CheckSoundPlaying(pPlayer->i,42))
            A_PlaySound(42, pPlayer->i);
        pPlayer->OnMotorcycle = 0;
        pPlayer->gotweapon.Clear(MOTORCYCLE_WEAPON);
        pPlayer->curr_weapon = pPlayer->last_full_weapon;
        P_CheckWeapon(pPlayer);
        pPlayer->q16horiz = F16(100);
        pPlayer->moto_do_bump = 0;
        pPlayer->MotoSpeed = 0;
        pPlayer->TiltStatus = 0;
        pPlayer->moto_drink = 0;
        pPlayer->VBumpTarget = 0;
        pPlayer->VBumpNow = 0;
        pPlayer->TurbCount = 0;
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        pPlayer->moto_underwater = 0;
        j = fi.spawn(pPlayer->i, TILE_EMPTYBIKE);
        sprite[j].ang = fix16_to_int(pPlayer->q16ang);
        sprite[j].xvel += sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        sprite[j].yvel += sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        sprite[j].owner = pPlayer->ammo_amount[MOTORCYCLE_WEAPON];
    }
}

void OnBoat(DukePlayer_t *pPlayer, int spriteNum)
{
    if (!pPlayer->OnBoat)
    {
        if (spriteNum)
        {
            pPlayer->pos.x = sprite[spriteNum].x;
            pPlayer->pos.y = sprite[spriteNum].y;
            pPlayer->q16ang = F16(sprite[spriteNum].ang);
            pPlayer->ammo_amount[BOAT_WEAPON] = sprite[spriteNum].owner;
            deletesprite(spriteNum);
        }
        pPlayer->over_shoulder_on = 0;
        pPlayer->OnBoat = 1;
        pPlayer->last_full_weapon = pPlayer->curr_weapon;
        pPlayer->curr_weapon = BOAT_WEAPON;
        pPlayer->gotweapon.Set(BOAT_WEAPON);
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->q16horiz = F16(100);
    }
}

void OffBoat(DukePlayer_t *pPlayer)
{
    int j;
    if (pPlayer->OnBoat)
    {
        pPlayer->OnBoat = 0;
        pPlayer->gotweapon.Clear(BOAT_WEAPON);
        pPlayer->curr_weapon = pPlayer->last_full_weapon;
        P_CheckWeapon(pPlayer);
        pPlayer->q16horiz = F16(100);
        pPlayer->moto_do_bump = 0;
        pPlayer->MotoSpeed = 0;
        pPlayer->TiltStatus = 0;
        pPlayer->moto_drink = 0;
        pPlayer->VBumpTarget = 0;
        pPlayer->VBumpNow = 0;
        pPlayer->TurbCount = 0;
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        pPlayer->moto_underwater = 0;
        j = fi.spawn(pPlayer->i, TILE_EMPTYBOAT);
        sprite[j].ang = fix16_to_int(pPlayer->q16ang);
        sprite[j].xvel += sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        sprite[j].yvel += sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        sprite[j].owner = pPlayer->ammo_amount[BOAT_WEAPON];
    }
}



void G_GameExit(const char *msg)
{
    if (*msg != 0) g_player[myconnectindex].ps->palette = BASEPAL;

    if (ud.recstat == 1)
        G_CloseDemoWrite();
	else if (ud.recstat == 2)
	{
		delete g_demo_filePtr;
		g_demo_filePtr = nullptr;
	}
    // JBF: fixes crash on demo playback
    // PK: modified from original

    if (!g_quickExit)
    {
        if (g_mostConcurrentPlayers > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_SCORESHEET) && *msg == ' ')
        {
            G_BonusScreen(1);
        }

        // shareware and TEN screens
        if (*msg != 0 && *(msg+1) != 'V' && *(msg+1) != 'Y')
            G_DisplayExtraScreens();
    }

	if (*msg != 0)
	{
		if (!(msg[0] == ' ' && msg[1] == 0))
		{
			I_Error("%s", msg);
		}
	}
	throw CExitEvent(0);
}



static int32_t G_DoThirdPerson(const DukePlayer_t *pp, vec3_t *vect, int16_t *vsectnum, int16_t ang, int16_t horiz)
{
    spritetype *sp = &sprite[pp->i];
    int32_t i, hx, hy;
    int32_t bakcstat = sp->cstat;
    hitdata_t hit;

    vec3_t n = {
        sintable[(ang+1536)&2047]>>4,
        sintable[(ang+1024)&2047]>>4,
        (horiz-100) * 128
    };

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    sp->cstat &= ~0x101;
    hitscan(vect, *vsectnum, n.x,n.y,n.z, &hit, CLIPMASK1);
    sp->cstat = bakcstat;

    if (*vsectnum < 0)
        return -1;

    hx = hit.pos.x-(vect->x);
    hy = hit.pos.y-(vect->y);

    if (klabs(n.x)+klabs(n.y) > klabs(hx)+klabs(hy))
    {
        *vsectnum = hit.sect;

        if (hit.wall >= 0)
        {
            int32_t daang = getangle(wall[wall[hit.wall].point2].x-wall[hit.wall].x,
                             wall[wall[hit.wall].point2].y-wall[hit.wall].y);

            i = n.x*sintable[daang] + n.y*sintable[(daang+1536)&2047];

            if (klabs(n.x) > klabs(n.y))
                hx -= mulscale28(n.x,i);
            else hy -= mulscale28(n.y,i);
        }
        else if (hit.sprite < 0)
        {
            if (klabs(n.x) > klabs(n.y))
                hx -= (n.x>>5);
            else hy -= (n.y>>5);
        }

        if (klabs(n.x) > klabs(n.y))
            i = divscale16(hx,n.x);
        else i = divscale16(hy,n.y);

        if (i < CAMERADIST)
            CAMERADIST = i;
    }

    vect->x += mulscale16(n.x,CAMERADIST);
    vect->y += mulscale16(n.y,CAMERADIST);
    vect->z += mulscale16(n.z,CAMERADIST);

    CAMERADIST = min(CAMERADIST+(((int32_t) totalclock-CAMERACLOCK)<<10),65536);
    CAMERACLOCK = (int32_t) totalclock;

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    return 0;
}

int32_t SE150_TempSectorZ[MAXSECTORS];
int32_t SE150_TempSectorPicnum[MAXSECTORS];

static void G_SE150_Draw(int32_t spnum, int32_t x, int32_t y, int32_t z, int32_t a, int32_t h, int32_t smoothratio)
{
    int32_t i = 13, j, k = 0;
    int32_t floor1 = spnum, floor2 = 0, ok = 0, fofmode;
    int32_t offx, offy;

    if (sprite[spnum].ang != 512) return;

    tileDelete(13);
    if (!(gotpic[i >> 3] & (1 << (i & 7)))) return;
    gotpic[i >> 3] &= ~(1 << (i & 7));

    floor1 = spnum;

    if (sprite[spnum].lotag == 152) fofmode = 150;
    if (sprite[spnum].lotag == 153) fofmode = 151;
    if (sprite[spnum].lotag == 154) fofmode = 150;
    if (sprite[spnum].lotag == 155) fofmode = 151;

    // fofmode=sprite[spnum].lotag-2;

    // sectnum=sprite[j].sectnum;
    // sectnum=cursectnum;
    ok++;

    /*  recursive?
     for(j=0;j<MAXSPRITES;j++)
     {
      if(
         sprite[j].sectnum==sectnum &&
         sprite[j].picnum==1 &&
         sprite[j].lotag==110
        ) { DrawFloorOverFloor(j); break;}
     }
    */

    // if(ok==0) { Message("no fof",RED); return; }

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == fofmode &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor1 = j; fofmode = sprite[j].lotag; ok++; break;
        }
    }
    // if(ok==1) { Message("no floor1",RED); return; }

    if (fofmode == 150) k = 151; else k = 150;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == k &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor2 = j; ok++; break;
        }
    }

    // if(ok==2) { Message("no floor2",RED); return; }

    for (j = 0; j < MAXSPRITES; j++)  // raise ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == 150)
            {
                SE150_TempSectorZ[sprite[j].sectnum] = sector[sprite[j].sectnum].floorz;
                sector[sprite[j].sectnum].floorz += (((z - sector[sprite[j].sectnum].floorz) / 32768) + 1) * 32768;
                SE150_TempSectorPicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].floorpicnum;
                sector[sprite[j].sectnum].floorpicnum = 13;
            }
            else if (k == 151)
            {
                SE150_TempSectorZ[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingz;
                sector[sprite[j].sectnum].ceilingz += (((z - sector[sprite[j].sectnum].ceilingz) / 32768) - 1) * 32768;
                SE150_TempSectorPicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingpicnum;
                sector[sprite[j].sectnum].ceilingpicnum = 13;
            }
        }
    }

    i = floor1;
    offx = x - sprite[i].x;
    offy = y - sprite[i].y;
    i = floor2;
#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_setanimatesprites(G_DoSpriteAnimations, offx + sprite[i].x, offy + sprite[i].y, z, fix16_to_int(a), smoothratio);
#endif

    renderDrawRoomsQ16(offx + sprite[i].x, offy + sprite[i].y, z, a, h, sprite[i].sectnum);
    G_DoSpriteAnimations(offx + sprite[i].x, offy + sprite[i].y, z, fix16_to_int(a), smoothratio);
    renderDrawMasks();

    for (j = 0; j < MAXSPRITES; j++)  // restore ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == 150)
            {
                sector[sprite[j].sectnum].floorz = SE150_TempSectorZ[sprite[j].sectnum];
                sector[sprite[j].sectnum].floorpicnum = SE150_TempSectorPicnum[sprite[j].sectnum];
            }
            else if (k == 151)
            {
                sector[sprite[j].sectnum].ceilingz = SE150_TempSectorZ[sprite[j].sectnum];
                sector[sprite[j].sectnum].ceilingpicnum = SE150_TempSectorPicnum[sprite[j].sectnum];
            }
        }
    }
}
void G_SE150(int32_t x, int32_t y, int32_t z, int32_t a, int32_t h, int32_t smoothratio)
{
    for (int i = headspritestat[STAT_RAROR]; i >= 0; i = nextspritestat[i])
    {
        switch(sprite[i].lotag)
        {
//            case 40:
//            case 41:
//                SE40_Draw(i,x,y,a,smoothratio);
//                break;
            case 152:
            case 153:
            case 154:
            case 155:
                if(g_player[screenpeek].ps->cursectnum == sprite[i].sectnum)
                    G_SE150_Draw(i,x,y,z,a,h,smoothratio);
                break;
        }
    }
}

#ifdef LEGACY_ROR
char ror_protectedsectors[MAXSECTORS];
static int32_t drawing_ror = 0;
static int32_t ror_sprite = -1;

static void G_OROR_DupeSprites(const spritetype *sp)
{
    // dupe the sprites touching the portal to the other sector
    int32_t k;
    const spritetype *refsp;

    if ((unsigned)sp->yvel >= (unsigned)g_mostConcurrentPlayers)
        return;

    refsp = &sprite[sp->yvel];

    for (SPRITES_OF_SECT(sp->sectnum, k))
    {
        if (spritesortcnt >= maxspritesonscreen)
            break;

        if (sprite[k].picnum != TILE_SECTOREFFECTOR && sprite[k].z >= sp->z)
        {
            tspriteptr_t tsp = renderAddTSpriteFromSprite(k);

            tsp->x += (refsp->x - sp->x);
            tsp->y += (refsp->y - sp->y);
            tsp->z += -sp->z + actor[sp->yvel].ceilingz;
            tsp->sectnum = refsp->sectnum;

//            Printf("duped sprite of pic %d at %d %d %d\n",tsp->picnum,tsp->x,tsp->y,tsp->z);
        }
    }
}

static int16_t SE40backupStat[MAXSECTORS];
static int32_t SE40backupZ[MAXSECTORS];

static void G_SE40(int32_t smoothratio)
{
    if ((unsigned)ror_sprite < MAXSPRITES)
    {
        int32_t x, y, z;
        int16_t sect;
        int32_t level = 0;
        const spritetype *const sp = &sprite[ror_sprite];
        const int32_t sprite2 = sp->yvel;

        if ((unsigned)sprite2 >= MAXSPRITES)
            return;

        if (klabs(sector[sp->sectnum].floorz - sp->z) < klabs(sector[sprite[sprite2].sectnum].floorz - sprite[sprite2].z))
            level = 1;

        x = CAMERA(pos.x) - sp->x;
        y = CAMERA(pos.y) - sp->y;
        z = CAMERA(pos.z) - (level ? sector[sp->sectnum].floorz : sector[sp->sectnum].ceilingz);

        sect = sprite[sprite2].sectnum;
        updatesector(sprite[sprite2].x + x, sprite[sprite2].y + y, &sect);

        if (sect != -1)
        {
            int32_t renderz, picnum;
            // XXX: PK: too large stack allocation for my taste
            int32_t i;
            int32_t pix_diff, newz;
            //                Printf("drawing ror\n");

            if (level)
            {
                // renderz = sector[sprite[sprite2].sectnum].ceilingz;
                renderz = sprite[sprite2].z - (sprite[sprite2].yrepeat * tilesiz[sprite[sprite2].picnum].y<<1);
                picnum = sector[sprite[sprite2].sectnum].ceilingpicnum;
                sector[sprite[sprite2].sectnum].ceilingpicnum = 562;
				tileDelete(562);

                pix_diff = klabs(z) >> 8;
                newz = - ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    SE40backupStat[i] = sector[i].ceilingstat;
                    SE40backupZ[i] = sector[i].ceilingz;
                    if (!ror_protectedsectors[i] || sp->lotag == 41)
                    {
                        sector[i].ceilingstat = 1;
                        sector[i].ceilingz += newz;
                    }
                }
            }
            else
            {
                // renderz = sector[sprite[sprite2].sectnum].floorz;
                renderz = sprite[sprite2].z;
                picnum = sector[sprite[sprite2].sectnum].floorpicnum;
                sector[sprite[sprite2].sectnum].floorpicnum = 562;
				tileDelete(562);

                pix_diff = klabs(z) >> 8;
                newz = ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    SE40backupStat[i] = sector[i].floorstat;
                    SE40backupZ[i] = sector[i].floorz;
                    if (!ror_protectedsectors[i] || sp->lotag == 41)
                    {
                        sector[i].floorstat = 1;
                        sector[i].floorz = +newz;
                    }
                }
            }

#ifdef POLYMER
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), fix16_to_int(CAMERA(q16ang)), smoothratio);
#endif
            renderDrawRoomsQ16(sprite[sprite2].x + x, sprite[sprite2].y + y,
                      z + renderz, CAMERA(q16ang), CAMERA(q16horiz), sect);
            drawing_ror = 1 + level;

            if (drawing_ror == 2) // viewing from top
                G_OROR_DupeSprites(sp);

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothratio);
            renderDrawMasks();

            if (level)
            {
                sector[sprite[sprite2].sectnum].ceilingpicnum = picnum;
                for (i = 0; i < numsectors; i++)
                {
                    sector[i].ceilingstat = SE40backupStat[i];
                    sector[i].ceilingz = SE40backupZ[i];
                }
            }
            else
            {
                sector[sprite[sprite2].sectnum].floorpicnum = picnum;

                for (i = 0; i < numsectors; i++)
                {
                    sector[i].floorstat = SE40backupStat[i];
                    sector[i].floorz = SE40backupZ[i];
                }
            }
        }
    }
}
#endif

void G_HandleMirror(int32_t x, int32_t y, int32_t z, fix16_t a, fix16_t q16horiz, int32_t smoothratio)
{
    if ((gotpic[TILE_MIRROR>>3]&(1<<(TILE_MIRROR&7)))
#ifdef POLYMER
        && (videoGetRenderMode() != REND_POLYMER)
#endif
        )
    {
        if (g_mirrorCount == 0)
        {
            // NOTE: We can have g_mirrorCount==0 but gotpic'd TILE_MIRROR,
            // for example in LNGA2.
            gotpic[TILE_MIRROR>>3] &= ~(1<<(TILE_MIRROR&7));
            return;
        }

        int32_t i = 0, dst = INT32_MAX;

        for (bssize_t k=g_mirrorCount-1; k>=0; k--)
        {
            if (!wallvisible(x, y, g_mirrorWall[k]))
                continue;

            const int32_t j =
                klabs(wall[g_mirrorWall[k]].x - x) +
                klabs(wall[g_mirrorWall[k]].y - y);

            if (j < dst)
                dst = j, i = k;
        }

        if (wall[g_mirrorWall[i]].overpicnum != TILE_MIRROR)
        {
            // Try to find a new mirror wall in case the original one was broken.

            int32_t startwall = sector[g_mirrorSector[i]].wallptr;
            int32_t endwall = startwall + sector[g_mirrorSector[i]].wallnum;

            for (bssize_t k=startwall; k<endwall; k++)
            {
                int32_t j = wall[k].nextwall;
                if (j >= 0 && (wall[j].cstat&32) && wall[j].overpicnum==TILE_MIRROR)  // cmp. premap.c
                {
                    g_mirrorWall[i] = j;
                    break;
                }
            }
        }

        if (wall[g_mirrorWall[i]].overpicnum == TILE_MIRROR)
        {
            int32_t tposx, tposy;
            fix16_t tang;

            renderPrepareMirror(x, y, z, a, q16horiz, g_mirrorWall[i], &tposx, &tposy, &tang);

            int32_t j = g_visibility;
            g_visibility = (j>>1) + (j>>2);

            renderDrawRoomsQ16(tposx,tposy,z,tang,q16horiz,g_mirrorSector[i]+MAXSECTORS);
            display_mirror = 1;
            G_DoSpriteAnimations(tposx,tposy,z,fix16_to_int(tang),smoothratio);
            display_mirror = 0;

            renderDrawMasks();
            renderCompleteMirror();   //Reverse screen x-wise in this function
            g_visibility = j;
        }

#ifdef SPLITSCREEN_MOD_HACKS
        if (!g_fakeMultiMode)
#endif
        {
            // HACK for splitscreen mod: this is so that mirrors will be drawn
            // from showview commands. Ugly, because we'll attempt do draw mirrors
            // each frame then. But it's better than not drawing them, I guess.
            // XXX: fix the sequence of setting/clearing this bit. Right now,
            // we always draw one frame without drawing the mirror, after which
            // the bit gets set and drawn subsequently.
            gotpic[TILE_MIRROR>>3] &= ~(1<<(TILE_MIRROR&7));
        }
    }
}

void G_DrawRooms(int32_t playerNum, int32_t smoothRatio)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    int yxAspect     = yxaspect;
    int viewingRange = viewingrange;

    //if (g_networkMode == NET_DEDICATED_SERVER) return;

    totalclocklock = totalclock;

    if (pub > 0 || videoGetRenderMode() >= REND_POLYMOST) // JBF 20040101: redraw background always
    {
        //videoClearScreen(0);
#ifndef EDUKE32_TOUCH_DEVICES
        if (ud.screen_size >= 8)
#endif
            G_DrawBackground();
        pub = 0;
    }

    if (ud.overhead_on == 2 || (pPlayer->cursectnum == -1 && videoGetRenderMode() != REND_CLASSIC))
        return;

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        videoSetCorrectedAspect();
    }

    if (ud.pause_on || pPlayer->on_crane > -1)
        smoothRatio = 65536;
    else
        smoothRatio = calc_smoothratio(totalclock, ototalclock);

    if (RRRA && fogactive)
        pPlayer->visibility = ud.const_visibility;

    int const playerVis = pPlayer->visibility;
    g_visibility        = (playerVis <= 0) ? 0 : (int32_t)(playerVis * (numplayers > 1 ? 1.f : r_ambientlightrecip));

    CAMERA(sect) = pPlayer->cursectnum;

    G_DoInterpolations(smoothRatio);
    G_AnimateCamSprite(smoothRatio);

    if (ud.camerasprite >= 0)
    {
        spritetype *const pSprite = &sprite[ud.camerasprite];

        // XXX: what?
        if (pSprite->yvel < 0) pSprite->yvel = -100;
        else if (pSprite->yvel > 199) pSprite->yvel = 300;

        CAMERA(q16ang) = fix16_from_int(actor[ud.camerasprite].tempang
                                      + mulscale16(((pSprite->ang + 1024 - actor[ud.camerasprite].tempang) & 2047) - 1024, smoothRatio));

#ifdef LEGACY_ROR
        if (!RR)
            G_SE40(smoothRatio);
#endif
#ifdef POLYMER
        if (videoGetRenderMode() == REND_POLYMER)
            polymer_setanimatesprites(G_DoSpriteAnimations, pSprite->x, pSprite->y, pSprite->z, fix16_to_int(CAMERA(q16ang)), smoothRatio);
#endif
        yax_preparedrawrooms();
        renderDrawRoomsQ16(pSprite->x, pSprite->y, pSprite->z - ZOFFSET6, CAMERA(q16ang), fix16_from_int(pSprite->yvel), pSprite->sectnum);
        yax_drawrooms(G_DoSpriteAnimations, pSprite->sectnum, 0, smoothRatio);
        G_DoSpriteAnimations(pSprite->x, pSprite->y, pSprite->z, fix16_to_int(CAMERA(q16ang)), smoothRatio);
        renderDrawMasks();
    }
    else
    {
        int32_t floorZ, ceilZ;

        int const vr            = divscale22(1, RR ? 64 : (sprite[pPlayer->i].yrepeat + 28));

        viewingRange = Blrintf(float(vr) * tanf(r_fov * (PI/360.f)));

        if (!RRRA || !pPlayer->DrugMode)
        {
            if (!r_usenewaspect)
                renderSetAspect(viewingRange, yxaspect);
            else
            {
                yxAspect     = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);

                renderSetAspect(mulscale16(viewingRange,viewingrange), yxaspect);
            }
        }

        if (videoGetRenderMode() >= REND_POLYMOST && (ud.screen_tilting
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        ))
        {
#ifdef USE_OPENGL
            renderSetRollAngle(pPlayer->orotscrnang + mulscale16(((pPlayer->rotscrnang - pPlayer->orotscrnang + 1024)&2047)-1024, smoothRatio));
#endif
            pPlayer->orotscrnang = pPlayer->rotscrnang;
        }

        if (RRRA && pPlayer->DrugMode > 0)
        {
            while (pPlayer->drug_timer < totalclock && !(pPlayer->gm & MODE_MENU) && !ud.pause_on && !System_WantGuiCapture())
            {
                int aspect;
                if (pPlayer->drug_stat[0] == 0)
                {
                    pPlayer->drug_stat[1]++;
                    aspect = viewingRange + pPlayer->drug_stat[1] * 5000;
                    if (viewingRange * 3 < aspect)
                    {
                        pPlayer->drug_aspect = viewingRange * 3;
                        pPlayer->drug_stat[0] = 2;
                    }
                    else
                    {
                        pPlayer->drug_aspect = aspect;
                    }
                    P_UpdateScreenPal(pPlayer);
                }
                else if (pPlayer->drug_stat[0] == 3)
                {
                    pPlayer->drug_stat[1]--;
                    aspect = viewingRange + pPlayer->drug_stat[1] * 5000;
                    if (aspect < viewingRange)
                    {
                        pPlayer->DrugMode = 0;
                        pPlayer->drug_stat[0] = 0;
                        pPlayer->drug_stat[2] = 0;
                        pPlayer->drug_stat[1] = 0;
                        pPlayer->drug_aspect = viewingRange;
                    }
                    else
                    {
                        pPlayer->drug_aspect = aspect;
                    }
                    P_UpdateScreenPal(pPlayer);
                }
                else if (pPlayer->drug_stat[0] == 2)
                {
                    if (pPlayer->drug_stat[2] > 30)
                    {
                        pPlayer->drug_stat[0] = 1;
                    }
                    else
                    {
                        pPlayer->drug_stat[2]++;
                        aspect = pPlayer->drug_stat[2] * 500 + viewingRange * 3;
                        pPlayer->drug_aspect = aspect;
                        P_UpdateScreenPal(pPlayer);
                    }
                }
                else
                {
                    if (pPlayer->drug_stat[2] < 1)
                    {
                        pPlayer->drug_stat[0] = 2;
                        pPlayer->DrugMode--;
                        if (pPlayer->DrugMode == 1)
                            pPlayer->drug_stat[0] = 3;
                    }
                    else
                    {
                        pPlayer->drug_stat[2]--;
                        aspect = pPlayer->drug_stat[2] * 500 + viewingRange * 3;
                        pPlayer->drug_aspect = aspect;
                        P_UpdateScreenPal(pPlayer);
                    }
                }

                pPlayer->drug_timer += TICSPERFRAME / 2;
            }
            if (!r_usenewaspect)
                renderSetAspect(pPlayer->drug_aspect, yxaspect);
            else
            {
                viewingRange = pPlayer->drug_aspect;
                yxAspect = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);

                renderSetAspect(mulscale16(viewingRange, viewingrange), yxaspect);
            }
            P_UpdateScreenPal(pPlayer);
        }

        if (pPlayer->newowner < 0)
        {
            if (playerNum == myconnectindex && numplayers > 1)
            {
                vec3_t const camVect = { omypos.x + mulscale16(mypos.x - omypos.x, smoothRatio),
                                         omypos.y + mulscale16(mypos.y - omypos.y, smoothRatio),
                                         omypos.z + mulscale16(mypos.z - omypos.z, smoothRatio) };

                CAMERA(pos)      = camVect;
                CAMERA(q16ang)   = myang + fix16_from_int(pPlayer->look_ang);
                CAMERA(q16horiz) = myhoriz + myhorizoff;
                CAMERA(sect)     = mycursectnum;
            }
            else
            {
                vec3_t const camVect = { pPlayer->opos.x + mulscale16(pPlayer->pos.x - pPlayer->opos.x, smoothRatio),
                                         pPlayer->opos.y + mulscale16(pPlayer->pos.y - pPlayer->opos.y, smoothRatio),
                                         pPlayer->opos.z + mulscale16(pPlayer->pos.z - pPlayer->opos.z, smoothRatio) };

                CAMERA(pos)      = camVect;
                CAMERA(q16ang)   = pPlayer->q16ang + fix16_from_int(pPlayer->look_ang);
                CAMERA(q16horiz) = pPlayer->q16horiz + pPlayer->q16horizoff;
            }

            if (cl_viewbob)
            {
                int zAdd = (pPlayer->opyoff + mulscale16(pPlayer->pyoff-pPlayer->opyoff, smoothRatio));

                if (pPlayer->over_shoulder_on)
                    zAdd >>= 3;

                CAMERA(pos.z) += zAdd;
            }

            if (pPlayer->over_shoulder_on)
            {
                CAMERA(pos.z) -= 3072;

                if (G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), fix16_to_int(CAMERA(q16ang)), fix16_to_int(CAMERA(q16horiz))) < 0)
                {
                    CAMERA(pos.z) += 3072;
                    G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), fix16_to_int(CAMERA(q16ang)), fix16_to_int(CAMERA(q16horiz)));
                }
            }
        }
        else
        {
            vec3_t const camVect = G_GetCameraPosition(pPlayer->newowner, smoothRatio);

            // looking through viewscreen
            CAMERA(pos)      = camVect;
            CAMERA(q16ang)   = pPlayer->q16ang + fix16_from_int(pPlayer->look_ang);
            CAMERA(q16horiz) = fix16_from_int(100 + sprite[pPlayer->newowner].shade);
            CAMERA(sect)     = sprite[pPlayer->newowner].sectnum;
        }

        ceilZ  = actor[pPlayer->i].ceilingz;
        floorZ = actor[pPlayer->i].floorz;

        if (g_earthquakeTime > 0 && pPlayer->on_ground == 1)
        {
            CAMERA(pos.z) += 256 - (((g_earthquakeTime)&1) << 9);
            CAMERA(q16ang)   += fix16_from_int((2 - ((g_earthquakeTime)&2)) << 2);
        }

        if (sprite[pPlayer->i].pal == 1)
            CAMERA(pos.z) -= (18<<8);

        if (pPlayer->newowner < 0 && pPlayer->spritebridge == 0)
        {
            // NOTE: when shrunk, p->pos.z can be below the floor.  This puts the
            // camera into the sector again then.

            if (CAMERA(pos.z) < (pPlayer->truecz + ZOFFSET6))
                CAMERA(pos.z) = ceilZ + ZOFFSET6;
            else if (CAMERA(pos.z) > (pPlayer->truefz - ZOFFSET6))
                CAMERA(pos.z) = floorZ - ZOFFSET6;
        }

        while (CAMERA(sect) >= 0)  // if, really
        {
            getzsofslope(CAMERA(sect),CAMERA(pos.x),CAMERA(pos.y),&ceilZ,&floorZ);
#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_CEILING) >= 0)
            {
                if (CAMERA(pos.z) < ceilZ)
                {
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
                    break;  // since CAMERA(sect) might have been updated to -1
                    // NOTE: fist discovered in WGR2 SVN r134, til' death level 1
                    //  (Lochwood Hollow).  A problem REMAINS with Polymost, maybe classic!
                }
            }
            else
#endif
                if (CAMERA(pos.z) < ceilZ+ZOFFSET6)
                    CAMERA(pos.z) = ceilZ+ZOFFSET6;

#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_FLOOR) >= 0)
            {
                if (CAMERA(pos.z) > floorZ)
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
            }
            else
#endif
                if (CAMERA(pos.z) > floorZ-ZOFFSET6)
                    CAMERA(pos.z) = floorZ-ZOFFSET6;

            break;
        }

        CAMERA(q16horiz) = fix16_clamp(CAMERA(q16horiz), F16(HORIZ_MIN), F16(HORIZ_MAX));

        G_HandleMirror(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(q16ang), CAMERA(q16horiz), smoothRatio);
#ifdef LEGACY_ROR
        if (!RR)
            G_SE40(smoothRatio);
#endif
        if (RRRA)
            G_SE150(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(q16ang), CAMERA(q16horiz), smoothRatio);
#ifdef POLYMER
        if (videoGetRenderMode() == REND_POLYMER)
            polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);
#endif
        // for G_PrintCoords
        dr_viewingrange = viewingrange;
        dr_yxaspect = yxaspect;
#ifdef DEBUG_MIRRORS_ONLY
        gotpic[TILE_MIRROR>>3] |= (1<<(TILE_MIRROR&7));
#else
        if (RR && sector[CAMERA(sect)].lotag == 848)
        {
            yax_preparedrawrooms();
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),CAMERA(sect));
            yax_drawrooms(G_DoSpriteAnimations, CAMERA(sect), 0, smoothRatio);

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();

            int geoSector = 0;

            for (bsize_t gs = 0; gs < g_geoSectorCnt; gs++)
            {
                int spriteNum = headspritesect[g_geoSector[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, g_geoSectorWarp[gs]);
                    sprite[spriteNum].x -= g_geoSectorX[gs];
                    sprite[spriteNum].y -= g_geoSectorY[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
                if (CAMERA(sect) == g_geoSector[gs])
                    geoSector = gs;
            }

            CAMERA(pos.x) -= g_geoSectorX[geoSector];
            CAMERA(pos.y) -= g_geoSectorY[geoSector];
            yax_preparedrawrooms();
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),g_geoSectorWarp[geoSector]);
            yax_drawrooms(G_DoSpriteAnimations, g_geoSectorWarp[geoSector], 0, smoothRatio);
            CAMERA(pos.x) += g_geoSectorX[geoSector];
            CAMERA(pos.y) += g_geoSectorY[geoSector];
                
            for (bsize_t gs = 0; gs < g_geoSectorCnt; gs++)
            {
                int spriteNum = headspritesect[g_geoSectorWarp[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, g_geoSector[gs]);
                    sprite[spriteNum].x += g_geoSectorX[gs];
                    sprite[spriteNum].y += g_geoSectorY[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
            }

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();

            for (bsize_t gs = 0; gs < g_geoSectorCnt; gs++)
            {
                int spriteNum = headspritesect[g_geoSector[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, g_geoSectorWarp2[gs]);
                    sprite[spriteNum].x -= g_geoSectorX2[gs];
                    sprite[spriteNum].y -= g_geoSectorY2[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
                if (CAMERA(sect) == g_geoSector[gs])
                    geoSector = gs;
            }

            CAMERA(pos.x) -= g_geoSectorX2[geoSector];
            CAMERA(pos.y) -= g_geoSectorY2[geoSector];
            yax_preparedrawrooms();
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),g_geoSectorWarp2[geoSector]);
            yax_drawrooms(G_DoSpriteAnimations, g_geoSectorWarp2[geoSector], 0, smoothRatio);
            CAMERA(pos.x) += g_geoSectorX2[geoSector];
            CAMERA(pos.y) += g_geoSectorY2[geoSector];
                
            for (bsize_t gs = 0; gs < g_geoSectorCnt; gs++)
            {
                int spriteNum = headspritesect[g_geoSectorWarp2[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, g_geoSector[gs]);
                    sprite[spriteNum].x += g_geoSectorX2[gs];
                    sprite[spriteNum].y += g_geoSectorY2[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
            }

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();
        }
        else
        {
            yax_preparedrawrooms();
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),CAMERA(sect));
            yax_drawrooms(G_DoSpriteAnimations, CAMERA(sect), 0, smoothRatio);
#ifdef LEGACY_ROR
            if (!RR && (unsigned)ror_sprite < MAXSPRITES && drawing_ror == 1)  // viewing from bottom
                G_OROR_DupeSprites(&sprite[ror_sprite]);
#endif
            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);
        }
#ifdef LEGACY_ROR
        drawing_ror = 0;
#endif
        renderDrawMasks();
#endif
    }

    G_RestoreInterpolations();

    if (!RRRA || !fogactive)
    {
        // Totalclock count of last step of p->visibility converging towards
        // ud.const_visibility.
        static int32_t lastvist;
        const int32_t visdif = ud.const_visibility-pPlayer->visibility;

        // Check if totalclock was cleared (e.g. restarted game).
        if (totalclock < lastvist)
            lastvist = 0;

        // Every 2nd totalclock increment (each 1/60th second), ...
        while (totalclock >= lastvist+2)
        {
            // ... approximately three-quarter the difference between
            // p->visibility and ud.const_visibility.
            const int32_t visinc = visdif>>2;

            if (klabs(visinc) == 0)
            {
                pPlayer->visibility = ud.const_visibility;
                break;
            }

            pPlayer->visibility += visinc;
            lastvist = (int32_t) totalclock;
        }
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        renderSetAspect(viewingRange, yxAspect);
    }
}

bool GameInterface::GenerateSavePic()
{
    G_DrawRooms(myconnectindex, 65536);
    return true;
}


void G_DumpDebugInfo(void)
{
    int32_t j,x;
    //    FILE * fp=fopen("condebug.log","w");

    for (x=0; x<MAXSTATUS; x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            buildprint("Sprite ", j, " (", TrackerCast(sprite[j].x), ",", TrackerCast(sprite[j].y), ",", TrackerCast(sprite[j].z),
                ") (picnum: ", TrackerCast(sprite[j].picnum), ")\n");
            buildprint("\n");
            j = nextspritestat[j];
        }
    }
}

// if <set_movflag_uncond> is true, set the moveflag unconditionally,
// else only if it equals 0.
static int32_t G_InitActor(int32_t i, int32_t tilenum, int32_t set_movflag_uncond)
{
    if (actorinfo[tilenum].scriptaddress)
    {
        auto sa = &ScriptCode[actorinfo[tilenum].scriptaddress];
        SH(i) = sa[0];
        AC_ACTION_ID(actor[i].t_data) = sa[1];
        AC_MOVE_ID(actor[i].t_data) = sa[2];

        if (set_movflag_uncond || (sa[3] && SHT(i) == 0))  // AC_MOVFLAGS
            SHT(i) = sa[3];

        return 1;
    }

    return 0;
}

#ifdef YAX_ENABLE
void Yax_SetBunchZs(int32_t sectnum, int32_t cf, int32_t daz)
{
    int32_t i, bunchnum = yax_getbunch(sectnum, cf);

    if (bunchnum < 0 || bunchnum >= numyaxbunches)
        return;

    for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, i))
        SECTORFLD(i,z, YAX_CEILING) = daz;
    for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, i))
        SECTORFLD(i,z, YAX_FLOOR) = daz;
}

static void Yax_SetBunchInterpolation(int32_t sectnum, int32_t cf)
{
    int32_t i, bunchnum = yax_getbunch(sectnum, cf);

    if (bunchnum < 0 || bunchnum >= numyaxbunches)
        return;

    for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, i))
        G_SetInterpolation(&sector[i].ceilingz);
    for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, i))
        G_SetInterpolation(&sector[i].floorz);
}
#else
# define Yax_SetBunchInterpolation(sectnum, cf)
#endif

static int G_MaybeTakeOnFloorPal(tspritetype *pSprite, int sectNum)
{
    int const floorPal = sector[sectNum].floorpal;

    if (floorPal && !lookups.noFloorPal(floorPal) && !actorflag(pSprite->owner, SFLAG_NOPAL))
    {
        pSprite->pal = floorPal;
        return 1;
    }

    return 0;
}

template <int rotations>
static int getofs_viewtype(int angDiff)
{
    return ((((angDiff + 3072) & 2047) * rotations + 1024) >> 11) % rotations;
}

template <int rotations>
static int viewtype_mirror(uint16_t & cstat, int frameOffset)
{
    if (frameOffset > rotations / 2)
    {
        cstat |= 4;
        return rotations - frameOffset;
    }

    cstat &= ~4;
    return frameOffset;
}

template <int mirrored_rotations>
static int getofs_viewtype_mirrored(uint16_t & cstat, int angDiff)
{
    return viewtype_mirror<mirrored_rotations*2-2>(cstat, getofs_viewtype<mirrored_rotations*2-2>(angDiff));
}

// XXX: this fucking sucks and needs to be replaced with a SFLAG
static int G_CheckAdultTile(int tileNum)
{
    UNREFERENCED_PARAMETER(tileNum);
    switch (tileNum)
    {
        case FEM1__STATIC:
        case FEM2__STATIC:
        case FEM3__STATIC:
        case FEM4__STATIC:
        case FEM5__STATIC:
        case FEM6__STATIC:
        case FEM7__STATIC:
        case FEM8__STATIC:
        case FEM9__STATIC:
        case MAN__STATIC:
        case MAN2__STATIC:
        case WOMAN__STATIC:
        case PODFEM1__STATIC:
        case FEMPIC1__STATIC:
        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:
        case FEMPIC4__STATIC:
        case FEMPIC5__STATIC:
        case FEMPIC6__STATIC:
        case FEMPIC7__STATIC:
        case BLOODYPOLE__STATIC:
        case FEM6PAD__STATIC:
        case OOZ2__STATIC:
        case WALLBLOOD7__STATIC:
        case WALLBLOOD8__STATIC:
        case FETUS__STATIC:
        case FETUSJIB__STATIC:
        case FETUSBROKE__STATIC:
        case HOTMEAT__STATIC:
        case FOODOBJECT16__STATIC:
        case TAMPON__STATIC:
        case XXXSTACY__STATIC:
        case 4946:
        case 4947:
        case 693:
        case 2254:
        case 4560:
        case 4561:
        case 4562:
        case 4498:
        case 4957:
            if (RR) return 0;
            return 1;
        case FEM10__STATIC:
        case NAKED1__STATIC:
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
        case STATUE__STATIC:
        case STATUEFLASH__STATIC:
        case OOZ__STATIC:
        case WALLBLOOD1__STATIC:
        case WALLBLOOD2__STATIC:
        case WALLBLOOD3__STATIC:
        case WALLBLOOD4__STATIC:
        case WALLBLOOD5__STATIC:
        case SUSHIPLATE1__STATIC:
        case SUSHIPLATE2__STATIC:
        case SUSHIPLATE3__STATIC:
        case SUSHIPLATE4__STATIC:
        case DOLPHIN1__STATIC:
        case DOLPHIN2__STATIC:
        case TOUGHGAL__STATIC:
            return 1;
    }
    return 0;
}

void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t ourz, int32_t oura, int32_t smoothratio)
{
    UNREFERENCED_PARAMETER(ourz);
    int32_t j, frameOffset, playerNum;
    intptr_t l;

    if (spritesortcnt == 0)
    {
#ifdef DEBUGGINGAIDS
        g_spriteStat.numonscreen = 0;
#endif
        return;
    }
#ifdef LEGACY_ROR
    ror_sprite = -1;
#endif
    for (j=spritesortcnt-1; j>=0; j--)
    {
        auto const t = &tsprite[j];
        const int32_t i = t->owner;
        auto const s = &sprite[i];

        switch (DYNAMICTILEMAP(s->picnum))
        {
        case SECTOREFFECTOR__STATIC:
            if (!RR && (s->lotag == 40 || s->lotag == 41))
            {
                t->cstat = 32768;
#ifdef LEGACY_ROR
                if (ror_sprite == -1)
                    ror_sprite = i;
#endif
            }

            if (t->lotag == SE_27_DEMO_CAM && ud.recstat == 1)
            {
                t->picnum = 11+(((int) totalclock>>3)&1);
                t->cstat |= 128;
            }
            else
                t->xrepeat = t->yrepeat = 0;
            break;
        }
    }

    for (j=spritesortcnt-1; j>=0; j--)
    {
        auto const t = &tsprite[j];
        const int32_t i = t->owner;
        spritetype *const s = &sprite[i];

        if (t->picnum < TILE_GREENSLIME || t->picnum > TILE_GREENSLIME+7)
            switch (DYNAMICTILEMAP(t->picnum))
            {
            case PIG__STATICRR:
            case DOGRUN__STATICRR:
            case VIXEN__STATICRR:
            case CHEER__STATICRR:
                goto default_case1;
            case PUKE__STATIC:
                if (RR) goto default_case1;
                fallthrough__;
            case BLOODPOOL__STATIC:
            case FOOTPRINTS__STATIC:
            case FOOTPRINTS2__STATIC:
            case FOOTPRINTS3__STATIC:
            case FOOTPRINTS4__STATIC:
                if (t->shade == 127) continue;
                break;
            case RESPAWNMARKERRED__STATIC:
            case RESPAWNMARKERYELLOW__STATIC:
            case RESPAWNMARKERGREEN__STATIC:
                if (RR) goto default_case1;
                if (ud.marker == 0)
                    t->xrepeat = t->yrepeat = 0;
                continue;
            case CHAIR3__STATIC:
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    t->cstat &= ~4;
                    break;
                }
#endif
                frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);
                t->picnum = s->picnum+frameOffset;
                break;
            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:
                if (adult_lockout) t->xrepeat = t->yrepeat = 0;
                else if (t->pal == 6)
                {
                    t->shade = -127;
                    continue;
                }
                fallthrough__;
            case BULLETHOLE__STATIC:
            case CRACK1__STATIC:
            case CRACK2__STATIC:
            case CRACK3__STATIC:
            case CRACK4__STATIC:
                t->shade = 16;
                continue;
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:
                continue;
            case RRTILE1947__STATICRR:
            case RRTILE2859__STATICRR:
            case RRTILE3774__STATICRR:
            case RRTILE5088__STATICRR:
            case RRTILE8094__STATICRR:
            case RRTILE8096__STATICRR:
                if (!RRRA) goto default_case1;
                continue;
            default:
default_case1:
                // NOTE: wall-aligned sprites will never take on ceiling/floor shade...
                if ((t->cstat&16) || (A_CheckEnemySprite(t) &&
                    (unsigned)t->owner < MAXSPRITES && sprite[t->owner].extra > 0) || t->statnum == STAT_PLAYER)
                {
                    if (RR && shadedsector[s->sectnum] == 1)
                    {
                        s->shade = 16;
                        t->shade = 16;
                    }
                    continue;
                }
            }

        // ... since this is not reached:
        if (t->cstat&CSTAT_SPRITE_NOSHADE)
            l = sprite[t->owner].shade;
        else
        {
            if (sector[t->sectnum].ceilingstat&1)
            {
                if (RR)
                    l = s->shade;
                else
                    l = sector[t->sectnum].ceilingshade;
            }
            else
                l = sector[t->sectnum].floorshade;

            if (l < -127)
                l = -127;
        }

        t->shade = l;
    }

    for (j=spritesortcnt-1; j>=0; j--) //Between drawrooms() and drawmasks()
    {
        int32_t switchpic;
        int32_t curframe;
        int32_t scrofs_action;
        //is the perfect time to animate sprites
        auto const t = &tsprite[j];
        const int32_t i = t->owner;
        // XXX: what's up with the (i < 0) check?
        // NOTE: not const spritetype because set at SET_SPRITE_NOT_TSPRITE (see below).
        EDUKE32_STATIC_ASSERT(sizeof(uspritetype) == sizeof(tspritetype)); // see TSPRITE_SIZE
        uspritetype *const pSprite = (i < 0) ? (uspritetype *)&tsprite[j] : (uspritetype *)&sprite[i];

        if (adult_lockout &&  G_CheckAdultTile(DYNAMICTILEMAP(pSprite->picnum)))
        {
            t->xrepeat = t->yrepeat = 0;
            continue;
        }

        if (!RR && pSprite->picnum == TILE_NATURALLIGHTNING)
        {
            t->shade = -127;
            t->clipdist |= TSPR_FLAGS_NO_SHADOW;
        }

        if (t->statnum == TSPR_TEMP)
            continue;

        Bassert(i >= 0);

        const DukePlayer_t *const ps = (pSprite->statnum != STAT_ACTOR && pSprite->picnum == TILE_APLAYER && pSprite->owner >= 0) ? g_player[P_GetP(pSprite)].ps : NULL;
        if (ps && ps->newowner == -1)
        {
            t->x -= mulscale16(65536-smoothratio,ps->pos.x-ps->opos.x);
            t->y -= mulscale16(65536-smoothratio,ps->pos.y-ps->opos.y);
            t->z = ps->opos.z + mulscale16(smoothratio,ps->pos.z-ps->opos.z) + (40<<8);

            if (RR)
            {
                pSprite->xrepeat = 24;
                pSprite->yrepeat = 17;
            }
        }
        else if ((pSprite->statnum == STAT_DEFAULT && pSprite->picnum != TILE_CRANEPOLE) || pSprite->statnum == STAT_PLAYER ||
                 pSprite->statnum == STAT_STANDABLE || pSprite->statnum == STAT_PROJECTILE || pSprite->statnum == STAT_MISC || pSprite->statnum == STAT_ACTOR)
        {
            t->x -= mulscale16(65536-smoothratio,pSprite->x-actor[i].bpos.x);
            t->y -= mulscale16(65536-smoothratio,pSprite->y-actor[i].bpos.y);
            t->z -= mulscale16(65536-smoothratio,pSprite->z-actor[i].bpos.z);
        }

        const int32_t sect = pSprite->sectnum;

        curframe = AC_CURFRAME(actor[i].t_data);
        scrofs_action = AC_ACTION_ID(actor[i].t_data);
        switchpic = pSprite->picnum;
        // Some special cases because dynamictostatic system can't handle
        // addition to constants.
        if ((pSprite->picnum >= TILE_SCRAP6) && (pSprite->picnum<=TILE_SCRAP6+7))
            switchpic = TILE_SCRAP5;
        else if ((pSprite->picnum==TILE_MONEY+1) || (pSprite->picnum==TILE_MAIL+1) || (pSprite->picnum==TILE_PAPER+1))
            switchpic--;

        switch (DYNAMICTILEMAP(switchpic))
        {
        case RESPAWNMARKERRED__STATICRR:
        case RESPAWNMARKERYELLOW__STATICRR:
        case RESPAWNMARKERGREEN__STATICRR:
            if (!RR) goto default_case2;
            t->picnum = 861 + (((int32_t) totalclock >> 4) & 13);
            if (pSprite->picnum == TILE_RESPAWNMARKERRED)
                t->pal = 0;
            else if (pSprite->picnum == TILE_RESPAWNMARKERYELLOW)
                t->pal = 1;
            else
                t->pal = 2;
            if (ud.marker == 0)
                t->xrepeat = t->yrepeat = 0;
            break;
        case DUKELYINGDEAD__STATIC:
            if (RR)
            {
                pSprite->x = 24;
                pSprite->y = 17;
                if (pSprite->extra > 0)
                    t->z += (6<<8);
            }
            else
                t->z += (24<<8);
            break;
        case BLOODPOOL__STATIC:
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (t->pal == 6)
                t->shade = -127;
            fallthrough__;
        case PUKE__STATIC:
        case MONEY__STATIC:
            //case TILE_MONEY+1__STATIC:
        case MAIL__STATIC:
            //case TILE_MAIL+1__STATIC:
        case PAPER__STATIC:
            //case TILE_PAPER+1__STATIC:
            if (RR && (switchpic == TILE_PUKE || switchpic == TILE_MAIL || switchpic == TILE_PAPER)) goto default_case2;
            if (adult_lockout && pSprite->pal == 2)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            break;
        case TRIPBOMB__STATIC:
            if (RR) goto default_case2;
            continue;
        case TRIPBOMBSPRITE__STATIC:
            if (!RR) goto default_case2;
            continue;
        case FORCESPHERE__STATIC:
            if (t->statnum == STAT_MISC)
            {
                int16_t const sqa = getangle(sprite[pSprite->owner].x - g_player[screenpeek].ps->pos.x,
                                       sprite[pSprite->owner].y - g_player[screenpeek].ps->pos.y);
                int16_t const sqb = getangle(sprite[pSprite->owner].x - t->x, sprite[pSprite->owner].y - t->y);

                if (klabs(getincangle(sqa,sqb)) > 512)
                    if (ldist(&sprite[pSprite->owner],(const spritetype *)t) < ldist(&sprite[g_player[screenpeek].ps->i],&sprite[pSprite->owner]))
                        t->xrepeat = t->yrepeat = 0;
            }
            continue;
        case BURNING2__STATIC:
            if (RR) goto default_case2;
            fallthrough__;
        case BURNING__STATIC:
            if (sprite[pSprite->owner].statnum == STAT_PLAYER)
            {
                int const playerNum = P_Get(pSprite->owner);

                if (display_mirror == 0 && playerNum == screenpeek && g_player[playerNum].ps->over_shoulder_on == 0)
                    t->xrepeat = 0;
                else
                {
                    t->ang = getangle(ourx - t->x, oury - t->y);
                    t->x   = sprite[pSprite->owner].x + (sintable[(t->ang + 512) & 2047] >> 10);
                    t->y   = sprite[pSprite->owner].y + (sintable[t->ang & 2047] >> 10);
                }
            }
            break;

        case ATOMICHEALTH__STATIC:
            t->z -= ZOFFSET6;
            break;
        case CRYSTALAMMO__STATIC:
            t->shade = (sintable[((int32_t) totalclock<<4)&2047]>>10);
            if (RR) break;
            continue;
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
        {
            if (RR) goto default_case2;
            int const viewscrTile = TILE_VIEWSCR;

            if (g_curViewscreen >= 0 && actor[OW(i)].t_data[0] == 1)
            {
                t->picnum = TILE_STATIC;
                t->cstat |= (rand()&12);
                t->xrepeat += 10;
                t->yrepeat += 9;
            }
            else if (g_curViewscreen == i && display_mirror != 3)
            {
                // this exposes a sprite sorting issue which needs to be debugged further...

                t->picnum = viewscrTile;
            }

            break;
        }
        case SHRINKSPARK__STATIC:
            if (RR)
            {
                if (RRRA && (sprite[pSprite->owner].picnum == TILE_CHEER || sprite[pSprite->owner].picnum == TILE_CHEERSTAYPUT))
                {
                    t->picnum = TILE_CHEERBLADE + (((int32_t) totalclock >> 4) & 3);
                    t->shade = -127;
                }
                else
                    t->picnum = TILE_SHRINKSPARK + (((int32_t) totalclock >> 4) & 7);
            }
            else
                t->picnum = TILE_SHRINKSPARK+(((int32_t) totalclock>>4)&3);
            break;
        case CHEERBOMB__STATICRR:
            if (!RRRA) goto default_case2;
            t->picnum = TILE_CHEERBOMB+( ((int32_t) totalclock>>4)&3 );
            break;
        case GROWSPARK__STATIC:
            if (RR) goto default_case2;
            t->picnum = TILE_GROWSPARK+(((int32_t) totalclock>>4)&3);
            break;
        case SPIT__STATIC:
            if (!RR) goto default_case2;
            t->picnum = TILE_SPIT + (((int32_t) totalclock >> 4) & 3);
            if (RRRA)
            {
                if (sprite[pSprite->owner].picnum == TILE_MINION && sprite[pSprite->owner].pal == 8)
                    t->picnum = TILE_RRTILE3500 + (((int32_t) totalclock >> 4) % 6);
                else if (sprite[pSprite->owner].picnum == TILE_MINION && sprite[pSprite->owner].pal == 19)
                {
                    t->picnum = TILE_RRTILE5090 + (((int32_t) totalclock >> 4) & 3);
                    t->shade = -127;
                }
                else if (sprite[pSprite->owner].picnum == TILE_MAMA)
                {
#ifdef USE_OPENGL
                    if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0 &&
                        !(spriteext[i].flags & SPREXT_NOTMD))
                    {
                        int32_t v = getangle(t->xvel, t->zvel >> 4);

                        spriteext[i].pitch = (v > 1023 ? v - 2048 : v);
                        t->cstat &= ~4;
                        t->picnum = TILE_RRTILE7274;
                        break;
                    }
#endif
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));
                    t->picnum = TILE_RRTILE7274 + frameOffset;
                }
            }
            break;
        case EMPTYBIKE__STATICRR:
            if (!RRRA) goto default_case2;
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x - ourx, pSprite->y - oury));
            t->picnum = TILE_EMPTYBIKE + frameOffset;
            break;
        case EMPTYBOAT__STATICRR:
            if (!RRRA) goto default_case2;
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x - ourx, pSprite->y - oury));
            t->picnum = TILE_EMPTYBOAT + frameOffset;
            break;
        case RPG__STATIC:
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum,t->pal) >= 0 &&
                    !(spriteext[i].flags & SPREXT_NOTMD))
            {
                int32_t v = getangle(t->xvel, t->zvel>>4);

                spriteext[i].pitch = (v > 1023 ? v-2048 : v);
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));
            t->picnum = TILE_RPG+frameOffset;
            break;
        case RPG2__STATICRR:
            if (!RRRA) goto default_case2;
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum,t->pal) >= 0 &&
                    !(spriteext[i].flags & SPREXT_NOTMD))
            {
                int32_t v = getangle(t->xvel, t->zvel>>4);

                spriteext[i].pitch = (v > 1023 ? v-2048 : v);
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));
            t->picnum = TILE_RPG2+frameOffset;
            break;

        case RECON__STATIC:
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));

            // RECON_T4
            if (klabs(curframe) > 64)
                frameOffset += 7;  // tilted recon car

            t->picnum = TILE_RECON+frameOffset;

            break;
        case APLAYER__STATIC:
            playerNum = P_GetP(pSprite);

            if (t->pal == 1) t->z -= (18<<8);

            if (g_player[playerNum].ps->over_shoulder_on > 0 && g_player[playerNum].ps->newowner < 0)
            {
                t->ang = fix16_to_int(
                g_player[playerNum].ps->q16ang
                + mulscale16((((g_player[playerNum].ps->q16ang + 1024 - g_player[playerNum].ps->oq16ang) & 2047) - 1024), smoothratio));
#ifdef USE_OPENGL
                if (bpp > 8 && hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0)
                {
                    static int32_t targetang = 0;

                    if (g_player[playerNum].input->extbits&(1<<1))
                    {
                        if (g_player[playerNum].input->extbits&(1<<2))targetang += 16;
                        else if (g_player[playerNum].input->extbits&(1<<3)) targetang -= 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }
                    else
                    {
                        if (g_player[playerNum].input->extbits&(1<<2))targetang -= 16;
                        else if (g_player[playerNum].input->extbits&(1<<3)) targetang += 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }

                    targetang = clamp(targetang, -128, 128);
                    t->ang += targetang;
                }
                else
#endif
                    t->cstat |= 2;
                if (screenpeek == myconnectindex && numplayers >= 2)
                {
                    t->x = omypos.x+mulscale16(mypos.x-omypos.x,smoothratio);
                    t->y = omypos.y+mulscale16(mypos.y-omypos.y,smoothratio);
                    t->z = omypos.z+mulscale16(mypos.z-omypos.z,smoothratio)+(40<<8);
                    t->ang = fix16_to_int(omyang+mulscale16((fix16_to_int(myang+F16(1024)-omyang)&2047)-1024,smoothratio));
                    t->sectnum = mycursectnum;
                }

            }

            if ((g_netServer || ud.multimode > 1) && (display_mirror || screenpeek != playerNum || pSprite->owner == -1))
            {
                if (ud.showweapons && sprite[g_player[playerNum].ps->i].extra > 0 && g_player[playerNum].ps->curr_weapon > 0
                        && spritesortcnt < maxspritesonscreen)
                {
                    tspritetype *const newTspr       = &tsprite[spritesortcnt];
                    int const          currentWeapon = g_player[playerNum].ps->curr_weapon;

                    *newTspr         = *t;
                    newTspr->statnum = TSPR_TEMP;
                    newTspr->cstat   = 0;
                    newTspr->pal     = 0;
                    newTspr->picnum  = (currentWeapon == GROW_WEAPON ? TILE_GROWSPRITEICON : WeaponPickupSprites[currentWeapon]);
                    if (RR)
                    {
                        newTspr->picnum = 0;
                        switch(DYNAMICWEAPONMAP(g_player[playerNum].ps->curr_weapon))
                        {
                            case PISTOL_WEAPON__STATIC:      newTspr->picnum = TILE_FIRSTGUNSPRITE;       break;
                            case SHOTGUN_WEAPON__STATIC:     newTspr->picnum = TILE_SHOTGUNSPRITE;        break;
                            case CHAINGUN_WEAPON__STATIC:    newTspr->picnum = TILE_CHAINGUNSPRITE;       break;
                            case RPG_WEAPON__STATIC:         newTspr->picnum = TILE_RPGSPRITE;            break;
                            case CHICKEN_WEAPON__STATIC:     newTspr->picnum = TILE_RPGSPRITE;            break;
                            case HANDREMOTE_WEAPON__STATIC:
                            case HANDBOMB_WEAPON__STATIC:    newTspr->picnum = TILE_HEAVYHBOMB;           break;
                            case TRIPBOMB_WEAPON__STATIC:    newTspr->picnum = TILE_TRIPBOMBSPRITE;       break;
                            case BOWLINGBALL_WEAPON__STATIC: newTspr->picnum = TILE_BOWLINGBALLSPRITE;    break;
                            case SHRINKER_WEAPON__STATIC:    newTspr->picnum = TILE_SHRINKSPARK;          break;
                            case GROW_WEAPON__STATIC:        newTspr->picnum = TILE_SHRINKSPARK;          break;
                            case FREEZE_WEAPON__STATIC:      newTspr->picnum = TILE_DEVISTATORSPRITE;     break;
                            case DEVISTATOR_WEAPON__STATIC:  newTspr->picnum = TILE_FREEZESPRITE;         break;
                        }
                    }
                    newTspr->z       = (pSprite->owner >= 0) ? g_player[playerNum].ps->pos.z - ZOFFSET4 : pSprite->z - (51 << 8);
                    newTspr->xrepeat = (newTspr->picnum == TILE_HEAVYHBOMB) ? 10 : 16;
                    if (RRRA && (g_player[playerNum].ps->OnMotorcycle || g_player[playerNum].ps->OnBoat))
                        newTspr->xrepeat = 0;
                    newTspr->yrepeat = newTspr->xrepeat;

                    spritesortcnt++;
                }

                if (g_player[playerNum].input->extbits & (1 << 7) && !ud.pause_on && spritesortcnt < maxspritesonscreen)
                {
                    tspritetype *const playerTyping = t;

                    playerTyping->statnum = TSPR_TEMP;
                    playerTyping->cstat   = 0;
                    playerTyping->picnum  = TILE_RESPAWNMARKERGREEN;
                    playerTyping->z       = (pSprite->owner >= 0) ? (g_player[playerNum].ps->pos.z - (20 << 8)) : (pSprite->z - (96 << 8));
                    playerTyping->xrepeat = 32;
                    playerTyping->yrepeat = 32;
                    playerTyping->pal     = 20;

                    spritesortcnt++;
                }
            }

            if (pSprite->owner == -1)
            {
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    frameOffset = 0;
                    t->cstat &= ~4;
                }
                else
#endif
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - oura);

                if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER) frameOffset += 1795-1405;
                else if ((actor[i].floorz-pSprite->z) > (64<<8)) frameOffset += 60;

                t->picnum += frameOffset;
                t->pal = g_player[playerNum].ps->palookup;

                goto PALONLY;
            }

            if (g_player[playerNum].ps->on_crane == -1 && (sector[pSprite->sectnum].lotag&0x7ff) != 1)  // ST_1_ABOVE_WATER ?
            {
                l = pSprite->z-actor[g_player[playerNum].ps->i].floorz+(3<<8);
                // SET_SPRITE_NOT_TSPRITE
                if (l > 1024 && pSprite->yrepeat > 32 && pSprite->extra > 0)
                    pSprite->yoffset = (int8_t)tabledivide32_noinline(l, pSprite->yrepeat<<2);
                else pSprite->yoffset=0;
            }

            if (g_player[playerNum].ps->newowner > -1)
            {
                // Display TILE_APLAYER sprites with action PSTAND when viewed through
                // a camera.
                auto aplayer_scr = &ScriptCode[actorinfo[TILE_APLAYER].scriptaddress];
                // [0]=strength, [1]=actionofs, [2]=moveofs

                scrofs_action = aplayer_scr[1];
                curframe = 0;
            }

            if (ud.camerasprite == -1 && g_player[playerNum].ps->newowner == -1)
            {
                if (pSprite->owner >= 0 && display_mirror == 0 && g_player[playerNum].ps->over_shoulder_on == 0)
                {
                    if ((!g_netServer && ud.multimode < 2) || ((g_netServer || ud.multimode > 1) && playerNum == screenpeek))
                    {
                        if (videoGetRenderMode() == REND_POLYMER)
                            t->clipdist |= TSPR_FLAGS_INVISIBLE_WITH_SHADOW;
                        else
                        {
                            t->owner = -1;
                            t->xrepeat = t->yrepeat = 0;
                            continue;
                        }

#ifdef USE_OPENGL
                        if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                        {
                            frameOffset = 0;
                            t->cstat &= ~4;
                        }
                        else
#endif
                            frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - oura);

                        if (sector[t->sectnum].lotag == ST_2_UNDERWATER) frameOffset += 1795-1405;
                        else if ((actor[i].floorz-pSprite->z) > (64<<8)) frameOffset += 60;

                        t->picnum += frameOffset;
                        t->pal = g_player[playerNum].ps->palookup;
                    }
                }
            }
PALONLY:
            G_MaybeTakeOnFloorPal(t, sect);

            if (pSprite->owner == -1) continue;

            if (t->z > actor[i].floorz && t->xrepeat < 32)
                t->z = actor[i].floorz;

            if (RRRA)
            {
                if (g_player[playerNum].ps->OnMotorcycle && playerNum == screenpeek)
                {
                    t->picnum = TILE_RRTILE7219;
                    t->xrepeat = 18;
                    t->yrepeat = 18;
                    scrofs_action = 0;
                    curframe = 0;
                }
                else if (g_player[playerNum].ps->OnMotorcycle)
                {
                    t->xrepeat = 18;
                    t->yrepeat = 18;
                    scrofs_action = 0;
                    curframe = 0;
#ifdef USE_OPENGL
                    if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                    {
                        frameOffset = 0;
                        t->cstat &= ~4;
                        t->picnum = TILE_RRTILE7213;
                    }
                    else
#endif
                        frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - oura);

                    t->picnum = TILE_RRTILE7213 + frameOffset;
                }
                else if (g_player[playerNum].ps->OnBoat && playerNum == screenpeek)
                {
                    t->picnum = TILE_RRTILE7190;
                    t->xrepeat = 32;
                    t->yrepeat = 32;
                    scrofs_action = 0;
                    curframe = 0;
                }
                else if (g_player[playerNum].ps->OnBoat)
                {
                    t->xrepeat = 32;
                    t->yrepeat = 32;
                    scrofs_action = 0;
                    curframe = 0;
#ifdef USE_OPENGL
                    if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                    {
                        frameOffset = 0;
                        t->cstat &= ~4;
                        t->picnum = TILE_RRTILE7213;
                    }
                    else
#endif
                        frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - oura);

                    t->picnum = TILE_RRTILE7184 + frameOffset;
                }
            }

            break;
        case RRTILE2460__STATICRR:
        case RRTILE2465__STATICRR:
        case BIKEJIBA__STATICRR:
        case BIKEJIBB__STATICRR:
        case BIKEJIBC__STATICRR:
        case BIKERJIBA__STATICRR:
        case BIKERJIBB__STATICRR:
        case BIKERJIBC__STATICRR:
        case BIKERJIBD__STATICRR:
        case CHEERJIBA__STATICRR:
        case CHEERJIBB__STATICRR:
        case CHEERJIBC__STATICRR:
        case CHEERJIBD__STATICRR:
        case FBOATJIBA__STATICRR:
        case FBOATJIBB__STATICRR:
        case RABBITJIBA__STATICRR:
        case RABBITJIBB__STATICRR:
        case RABBITJIBC__STATICRR:
        case MAMAJIBA__STATICRR:
        case MAMAJIBB__STATICRR:
            if (!RRRA) goto default_case2;
            fallthrough__;
        case MINJIBA__STATICRR:
        case MINJIBB__STATICRR:
        case MINJIBC__STATICRR:
        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case HEADJIB1__STATIC:
        case LEGJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
        case DUKELEG__STATIC:
        case DUKEGUN__STATIC:
        case DUKETORSO__STATIC:
        case BILLYJIBA__STATICRR:
        case BILLYJIBB__STATICRR:
        case HULKJIBA__STATICRR:
        case HULKJIBB__STATICRR:
        case HULKJIBC__STATICRR:
        case COOTJIBA__STATICRR:
        case COOTJIBB__STATICRR:
        case COOTJIBC__STATICRR:
            if (RR)
            {
                if (switchpic == TILE_HEADJIB1 || switchpic == TILE_LEGJIB1 || switchpic == TILE_ARMJIB1
                    || switchpic == TILE_LIZMANHEAD1 || switchpic == TILE_LIZMANARM1 || switchpic == TILE_LIZMANLEG1)
                    goto default_case2;
            }
            if (RRRA && t->pal == 19 && (switchpic == TILE_MINJIBA || switchpic == TILE_MINJIBB || switchpic == TILE_MINJIBC))
                t->shade = -127;
            if (adult_lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            if (t->pal == 6)
                t->shade = -120;
            if (RR && shadedsector[pSprite->sectnum] == 1)
                t->shade = 16;
            fallthrough__;
        case SCRAP1__STATIC:
        case SCRAP2__STATIC:
        case SCRAP3__STATIC:
        case SCRAP4__STATIC:
        case SCRAP5__STATIC:
            if ((RR || actor[i].picnum == TILE_BLIMP) && t->picnum == TILE_SCRAP1 && pSprite->yvel >= 0)
                t->picnum = pSprite->yvel < MAXUSERTILES ? pSprite->yvel : 0;
            else t->picnum += T1(i);
            if (!RR)
                t->shade = -128+6 < t->shade ? t->shade-6 : -128; // effectively max(t->shade-6, -128) while avoiding (signed!) underflow

            G_MaybeTakeOnFloorPal(t, sect);
            break;
        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == TILE_FLOORSLIME)
            {
                t->pal = 7;
                break;
            }
            fallthrough__;
        default:
default_case2:
            G_MaybeTakeOnFloorPal(t, sect);
            break;
        }

        if (G_HaveActor(pSprite->picnum) && scrofs_action != 0 && (!RR || (t->cstat & 48) != 48))
        {
            if ((unsigned)scrofs_action + ACTION_VIEWTYPE >= (unsigned)g_scriptSize)
                goto skip;

            int viewtype = ScriptCode[scrofs_action + ACTION_VIEWTYPE];
            uint16_t const action_flags = ScriptCode[scrofs_action + ACTION_FLAGS];

            int const invertp = viewtype < 0;
            l = klabs(viewtype);

#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                frameOffset = 0;
                t->cstat &= ~4;
            }
            else
#endif
            {
                int const viewAng = ((l > 4 && l != 8) || action_flags & AF_VIEWPOINT) ? getangle(pSprite->x-ourx, pSprite->y-oury) : oura;
                int const angDiff = invertp ? viewAng - pSprite->ang : pSprite->ang - viewAng;

                switch (l)
                {
                case 2:
                    frameOffset = getofs_viewtype<8>(angDiff) & 1;
                    break;

                case 3:
                case 4:
                    frameOffset = viewtype_mirror<7>(t->cstat, getofs_viewtype<16>(angDiff) & 7);
                    break;

                case 5:
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, angDiff);
                    break;
                case 7:
                    frameOffset = getofs_viewtype_mirrored<7>(t->cstat, angDiff);
                    break;
                case 8:
                    frameOffset = getofs_viewtype<8>(angDiff);
                    t->cstat &= ~4;
                    break;
                /*case 9:
                    frameOffset = getofs_viewtype_mirrored<9>(t->cstat, angDiff);
                    break;
                case 12:
                    frameOffset = getofs_viewtype<12>(angDiff);
                    t->cstat &= ~4;
                    break;
                case 16:
                    frameOffset = getofs_viewtype<16>(angDiff);
                    t->cstat &= ~4;
                    break;*/
                default:
                    if (RR)
                    {
                        if (A_CheckEnemySprite(pSprite) && pSprite->statnum == STAT_ZOMBIEACTOR && pSprite->extra > 0)
                        {
                            int const angDiff = pSprite->ang-getangle(pSprite->x-ourx, pSprite->y-oury);
                            frameOffset = getofs_viewtype_mirrored<5>(t->cstat, angDiff);
                            break;
                        }
                    }
                    frameOffset = 0;
                    break;
                }
            }

            t->picnum += frameOffset + ScriptCode[scrofs_action + ACTION_STARTFRAME] + l*curframe;
            // XXX: t->picnum can be out-of-bounds by bad user code.

            if (l > 0)
                while (tilesiz[t->picnum].x == 0 && t->picnum > 0)
                    t->picnum -= l;       //Hack, for actors

            if (actor[i].dispicnum >= 0)
                actor[i].dispicnum = t->picnum;
        }
//        else if (display_mirror == 1)
//            t->cstat |= 4;
        /* completemirror() already reverses the drawn frame, so the above isn't necessary.
         * Even Polymost's and Polymer's mirror seems to function correctly this way. */

skip:
        // Night vision goggles tsprite tinting.
        // XXX: Currently, for the splitscreen mod, sprites will be pal6-colored iff the first
        // player has nightvision on.  We should pass stuff like "from which player is this view
        // supposed to be" as parameters ("drawing context") instead of relying on globals.
        if (!RR && g_player[screenpeek].ps->inv_amount[GET_HEATS] > 0 && g_player[screenpeek].ps->heat_on &&
                (A_CheckEnemySprite(pSprite) || actorflag(t->owner,SFLAG_NVG) || pSprite->picnum == TILE_APLAYER || pSprite->statnum == STAT_DUMMYPLAYER))
        {
            t->pal = 6;
            t->shade = 0;
        }

        if (RR && !RRRA && pSprite->picnum == TILE_SBMOVE)
            t->shade = -127;

        // Fake floor shadow, implemented by inserting a new tsprite.
        if (pSprite->statnum == STAT_DUMMYPLAYER || A_CheckEnemySprite(pSprite) || actorflag(t->owner,SFLAG_SHADOW) || (pSprite->picnum == TILE_APLAYER && pSprite->owner >= 0))
            if ((!RR || (pSprite->cstat&48) == 0) && t->statnum != TSPR_TEMP && pSprite->picnum != TILE_EXPLOSION2 && (RR || pSprite->picnum != TILE_HANGLIGHT) && pSprite->picnum != TILE_DOMELITE && (RR || pSprite->picnum != TILE_HOTMEAT)
                && (!RR || pSprite->picnum != TILE_TORNADO) && (!RR || pSprite->picnum != TILE_EXPLOSION3) && (!RR || RRRA || pSprite->picnum != TILE_SBMOVE))
            {
                if (actor[i].dispicnum < 0)
                {
#ifdef DEBUGGINGAIDS
                    // A negative actor[i].dispicnum used to mean 'no floor shadow please', but
                    // that was a bad hack since the value could propagate to sprite[].picnum.
                    Printf(TEXTCOLOR_RED "actor[%d].dispicnum = %d\n", i, actor[i].dispicnum);
#endif
                    actor[i].dispicnum=0;
                    continue;
                }

                if (actor[i].aflags & SFLAG_NOFLOORSHADOW)
                    continue;

                if (r_shadows && spritesortcnt < (maxspritesonscreen-2)
#ifdef POLYMER
                    && !(videoGetRenderMode() == REND_POLYMER && pr_lighting != 0)
#endif
                    )
                {
                    if (DEER && klabs(sector[sect].ceilingheinum - sector[sect].floorheinum) > 576) continue;
                    if (RRRA && sector[sect].lotag == 160) continue;
                    int const shadowZ = (DEER || (sector[sect].lotag & 0xff) > 2 || pSprite->statnum == STAT_PROJECTILE ||
                                   pSprite->statnum == STAT_MISC || pSprite->picnum == TILE_DRONE || (!RR && pSprite->picnum == TILE_COMMANDER))
                                  ? sector[sect].floorz
                                  : actor[i].floorz;

                    if ((pSprite->z-shadowZ) < ZOFFSET3 && g_player[screenpeek].ps->pos.z < shadowZ)
                    {
                        tspritetype *const tsprShadow = &tsprite[spritesortcnt];

                        *tsprShadow         = *t;
                        tsprShadow->statnum = TSPR_TEMP;
                        tsprShadow->yrepeat = (t->yrepeat >> 3);

                        if (t->yrepeat < 4)
                            t->yrepeat = 4;

                        tsprShadow->shade   = 127;
                        tsprShadow->cstat  |= 2;
                        tsprShadow->z       = shadowZ;
                        tsprShadow->pal     = ud.shadow_pal;


#ifdef USE_OPENGL
                        if (videoGetRenderMode() >= REND_POLYMOST)
                        {
                            if (hw_models && md_tilehasmodel(t->picnum,t->pal) >= 0)
                            {
                                tsprShadow->yrepeat = 0;
                                // 512:trans reverse
                                //1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
                                tsprShadow->clipdist |= TSPR_FLAGS_MDHACK;
                                tsprShadow->cstat |= 512;
                            }
                            else
                            {
                                int const ii
                                = getangle(tsprShadow->x - g_player[screenpeek].ps->pos.x, tsprShadow->y - g_player[screenpeek].ps->pos.y);

                                tsprShadow->x += sintable[(ii+2560)&2047]>>9;
                                tsprShadow->y += sintable[(ii+2048)&2047]>>9;
                            }
                        }
#endif
                        spritesortcnt++;
                    }
                }
            }
        
        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case LASERLINE__STATIC:
            if (RR) break;
            if (sector[t->sectnum].lotag == ST_2_UNDERWATER) t->pal = 8;
            t->z = sprite[pSprite->owner].z-(3<<8);
            if (lasermode == 2 && g_player[screenpeek].ps->heat_on == 0)
                t->yrepeat = 0;
            fallthrough__;
        case EXPLOSION2BOT__STATIC:
        case GROWSPARK__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case FLOORFLAME__STATIC:
            if (RR) break;
            fallthrough__;
        case FREEZEBLAST__STATIC:
        case ATOMICHEALTH__STATIC:
        case FIRELASER__STATIC:
        case SHRINKSPARK__STATIC:
        case CHAINGUN__STATIC:
        case RPG__STATIC:
        case EXPLOSION2__STATIC:
        case EXPLOSION3__STATICRR:
        case OWHIP__STATICRR:
        case UWHIP__STATICRR:
        case RPG2__STATICRR:
        case RRTILE1790__STATICRR:
rrcoolexplosion1:
            if (RR && !RRRA && (pSprite->picnum == TILE_RPG2 || pSprite->picnum == TILE_RRTILE1790)) break;
            if (t->picnum == TILE_EXPLOSION2)
            {
                g_player[screenpeek].ps->visibility = -127;
                //restorepalette = 1;   // JBF 20040101: why?
                if (RR)
                    t->pal = 0;
            }
            else if (RR && t->picnum == TILE_FIRELASER)
                t->picnum = TILE_FIRELASER+(((int32_t) totalclock>>2)&5);
            t->shade = -127;
            t->clipdist |= TSPR_FLAGS_DRAW_LAST | TSPR_FLAGS_NO_SHADOW;
            break;
        case UFOBEAM__STATICRR:
        case RRTILE3586__STATICRR:
        case RRTILE3587__STATICRR:
            t->cstat |= 32768;
            pSprite->cstat |= 32768;
            break;
        case DESTRUCTO__STATICRR:
            t->cstat |= 32768;
            break;
        case FIRE__STATIC:
        case FIRE2__STATIC:
            if (RR && pSprite->picnum == TILE_FIRE2) break;
            t->cstat |= 128;
            fallthrough__;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (RR && pSprite->picnum == TILE_BURNING2) break;
            if (sprite[pSprite->owner].picnum != TILE_TREE1 && sprite[pSprite->owner].picnum != TILE_TREE2)
                t->z = sector[t->sectnum].floorz;
            t->shade = -127;
            fallthrough__;
        case SMALLSMOKE__STATIC:
            if (RR) break;
            t->clipdist |= TSPR_FLAGS_DRAW_LAST | TSPR_FLAGS_NO_SHADOW;
            break;
        case COOLEXPLOSION1__STATIC:
            if (RR) goto rrcoolexplosion1;
            t->shade = -127;
            t->clipdist |= TSPR_FLAGS_DRAW_LAST | TSPR_FLAGS_NO_SHADOW;
            t->picnum += (pSprite->shade>>1);
            break;
        case WALLLIGHT3__STATIC:
        case WALLLIGHT1__STATIC:
            if (!RR) break;
            fallthrough__;
        case RRTILE3668__STATICRR:
        case RRTILE3795__STATICRR:
        case RRTILE5035__STATICRR:
        case RRTILE7505__STATICRR:
        case RRTILE7506__STATICRR:
        case RRTILE7533__STATICRR:
        case RRTILE8216__STATICRR:
        case RRTILE8218__STATICRR:
        case RRTILE8220__STATICRR:
            if (!RRRA) break;
            fallthrough__;
        case RRTILE1878__STATICRR:
        case RRTILE1952__STATICRR:
        case RRTILE1953__STATICRR:
        case RRTILE1990__STATICRR:
        case RRTILE2050__STATICRR:
        case RRTILE2056__STATICRR:
        case RRTILE2072__STATICRR:
        case RRTILE2075__STATICRR:
        case RRTILE2083__STATICRR:
        case RRTILE2097__STATICRR:
        case RRTILE2156__STATICRR:
        case RRTILE2157__STATICRR:
        case RRTILE2158__STATICRR:
        case RRTILE2159__STATICRR:
        case RRTILE2160__STATICRR:
        case RRTILE2161__STATICRR:
        case RRTILE2175__STATICRR:
        case RRTILE2176__STATICRR:
        case RRTILE2357__STATICRR:
        case RRTILE2564__STATICRR:
        case RRTILE2573__STATICRR:
        case RRTILE2574__STATICRR:
        case RRTILE2583__STATICRR:
        case RRTILE2604__STATICRR:
        case RRTILE2689__STATICRR:
        case RRTILE2893__STATICRR:
        case RRTILE2894__STATICRR:
        case RRTILE2915__STATICRR:
        case RRTILE2945__STATICRR:
        case RRTILE2946__STATICRR:
        case RRTILE2947__STATICRR:
        case RRTILE2948__STATICRR:
        case RRTILE2949__STATICRR:
        case RRTILE2977__STATICRR:
        case RRTILE2978__STATICRR:
        case RRTILE3116__STATICRR:
        case RRTILE3171__STATICRR:
        case RRTILE3216__STATICRR:
        case RRTILE3720__STATICRR:
            t->shade = -127;
            break;
        case CHEER__STATICRR:
            if (!RRRA) break;
            if (t->picnum >= TILE_CHEER + 102 && t->picnum <= TILE_CHEER + 151)
                t->shade = -127;
            break;
        case MINION__STATICRR:
            if (!RRRA) break;
            if (t->pal == 19)
                t->shade = -127;
            break;
        case BIKER__STATICRR:
            if (!RRRA) break;
            if (t->picnum >= TILE_BIKER + 54 && t->picnum <= TILE_BIKER + 58)
                t->shade = -127;
            else if (t->picnum >= TILE_BIKER + 84 && t->picnum <= TILE_BIKER + 88)
                t->shade = -127;
            break;
        case BILLYRAY__STATICRR:
        case BILLYRAYSTAYPUT__STATICRR:
            if (!RRRA) break;
            if (t->picnum >= TILE_BILLYRAY + 5 && t->picnum <= TILE_BILLYRAY + 9)
                t->shade = -127;
            break;
        case RRTILE2034__STATICRR:
            t->picnum = TILE_RRTILE2034 + ((int32_t) totalclock & 1);
            break;
        case RRTILE2944__STATICRR:
            t->shade = -127;
            t->picnum = TILE_RRTILE2944 + (((int32_t) totalclock >> 2) & 4);
            break;
        case PLAYERONWATER__STATIC:
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum,pSprite->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                frameOffset = 0;
                t->cstat &= ~4;
            }
            else
#endif
                frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);

            t->picnum = pSprite->picnum+frameOffset+((T1(i)<4)*5);
            t->shade = sprite[pSprite->owner].shade;

            break;

        case MUD__STATICRR:
            t->picnum = TILE_MUD+T2(i);
            break;
        case WATERSPLASH2__STATIC:
            // WATERSPLASH_T2
            t->picnum = TILE_WATERSPLASH2+T2(i);
            break;
        case SHELL__STATIC:
            t->picnum = pSprite->picnum+(T1(i)&1);
            fallthrough__;
        case SHOTGUNSHELL__STATIC:
            t->cstat |= 12;
            if (T1(i) > 2) t->cstat &= ~16;
            else if (T1(i) > 1) t->cstat &= ~4;
            break;
        case FRAMEEFFECT1__STATIC:
            if (pSprite->owner >= 0 && sprite[pSprite->owner].statnum < MAXSTATUS)
            {
                if (sprite[pSprite->owner].picnum == TILE_APLAYER)
                    if (ud.camerasprite == -1)
                        if (screenpeek == P_Get(pSprite->owner) && display_mirror == 0)
                        {
                            t->owner = -1;
                            break;
                        }
                if ((sprite[pSprite->owner].cstat&32768) == 0)
                {
                    if (!actor[pSprite->owner].dispicnum)
                        t->picnum = actor[i].t_data[1];
                    else t->picnum = actor[pSprite->owner].dispicnum;

                    if (RR && sprite[pSprite->owner].picnum == TILE_APLAYER)
                        t->picnum = TILE_SMALLSMOKE;

                    if (!G_MaybeTakeOnFloorPal(t, sect))
                        t->pal = sprite[pSprite->owner].pal;

                    t->shade = sprite[pSprite->owner].shade;
                    t->ang = sprite[pSprite->owner].ang;
                    t->cstat = 2|sprite[pSprite->owner].cstat;
                }
            }
            break;

        case CAMERA1__STATIC:
        case RAT__STATIC:
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pSprite->picnum,pSprite->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);
            t->picnum = pSprite->picnum+frameOffset;
            break;
        }

        actor[i].dispicnum = t->picnum;
#if 0
        // why?
        if (sector[t->sectnum].floorpicnum == TILE_MIRROR)
            t->xrepeat = t->yrepeat = 0;
#endif
    }

#ifdef DEBUGGINGAIDS
    g_spriteStat.numonscreen = spritesortcnt;
#endif
}

void G_InitTimer(int32_t ticspersec)
{
    if (g_timerTicsPerSecond != ticspersec)
    {
        timerUninit();
        timerInit(ticspersec);
        g_timerTicsPerSecond = ticspersec;
    }
}


static int32_t g_RTSPlaying;

// Returns: started playing?
int G_StartRTS(int lumpNum, int localPlayer)
{
    if (!adult_lockout && SoundEnabled() &&
        RTS_IsInitialized() && g_RTSPlaying == 0 && (snd_speech & (localPlayer ? 1 : 4)))
    {
        auto sid = RTS_GetSoundID(lumpNum - 1);
        if (sid != -1)
        {
            S_PlaySound(sid, CHAN_AUTO, CHANF_UI);
            g_RTSPlaying = 7;
            return 1;
        }
    }

    return 0;
}

// Trying to sanitize the mess of options and the mess of variables the mess was stored in. (Did I say this was a total mess before...? >) )
// Hopefully this is more comprehensible, at least it neatly stores everything useful in a single linear value...
bool GameInterface::validate_hud(int layout)
{
	if (layout <= (RR? 5: 6))	// Status bar with border
	{
		return !(ud.statusbarflags & STATUSBAR_NOSHRINK);
	}
	else if (layout <= 7) // Status bar fullscreen
	{
		return (!(ud.statusbarflags & STATUSBAR_NOFULL) || !(ud.statusbarflags & STATUSBAR_NOOVERLAY));
	}
	else if (layout == 8)	// Status bar overlay
	{
		return !(ud.statusbarflags & STATUSBAR_NOOVERLAY);
	}
	else if (layout == 9)	// Fullscreen HUD
	{
		return (!(ud.statusbarflags & STATUSBAR_NOMINI) || !(ud.statusbarflags & STATUSBAR_NOMODERN));
	}
	else if (layout == 10)
	{
		return !(ud.statusbarflags & STATUSBAR_NOMODERN);
	}
	else if (layout == 11)
	{
		return !(ud.statusbarflags & STATUSBAR_NONONE);
	}
	return false;
}

void GameInterface::set_hud_layout(int layout)
{
	static const uint8_t screen_size_vals[] = { 60, 54, 48, 40, 32, 24, 16, 8, 8, 4, 4, 0 };
	static const uint8_t screen_size_vals_rr[] = { 56, 48, 40, 32, 24, 16, 12, 8, 8, 4, 4, 0 };
	if (validate_hud(layout))
	{
		ud.screen_size = RR? screen_size_vals_rr[layout] : screen_size_vals[layout];
		ud.statusbarmode = layout >= 8;
		ud.althud = layout >= 10;
		G_UpdateScreenArea();
	}
}

void GameInterface::set_hud_scale(int scale)
{
    ud.statusbarscale = clamp(scale, 36, 100);
    G_UpdateScreenArea();
}

void G_HandleLocalKeys(void)
{
//    CONTROL_ProcessBinds();

    if (ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
    {
        if (inputState.UnboundKeyPressed(sc_F1) || inputState.UnboundKeyPressed(sc_F2) || cl_autovote)
        {
            G_AddUserQuote(GStrings("VoteCast"));
            Net_SendMapVote(inputState.UnboundKeyPressed(sc_F1) || cl_autovote ? cl_autovote-1 : 0);
            inputState.ClearKeyStatus(sc_F1);
            inputState.ClearKeyStatus(sc_F2);
        }
    }

    if (!ALT_IS_PRESSED && ud.overhead_on == 0 && (g_player[myconnectindex].ps->gm & MODE_TYPE) == 0)
    {
        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(1))
				{
					S_PlaySound(RR ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale + 4;
            }

            G_UpdateScreenArea();
        }

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(-1))
				{
					S_PlaySound(RR ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale - 4;
            }

            G_UpdateScreenArea();
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE)) || System_WantGuiCapture())
        return;

    if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_COOPVIEW) || ud.recstat == 2))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = 0;
        restorepalette = -1;
    }

    if ((g_netServer || ud.multimode > 1) && buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.config.ShowOpponentWeapons = ud.showweapons = 1-ud.showweapons;
        P_DoQuote(QUOTE_WEAPON_MODE_OFF-ud.showweapons,g_player[screenpeek].ps);
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
        P_DoQuote(QUOTE_CROSSHAIR_OFF-cl_crosshair,g_player[screenpeek].ps);
    }

    if (ud.overhead_on && buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
    {
        buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if (ud.scrollmode)
        {
            ud.folx = g_player[screenpeek].ps->opos.x;
            ud.foly = g_player[screenpeek].ps->opos.y;
            ud.fola = fix16_to_int(g_player[screenpeek].ps->oq16ang);
        }
        P_DoQuote(QUOTE_MAP_FOLLOW_OFF+ud.scrollmode,g_player[myconnectindex].ps);
    }

    if (inputState.UnboundKeyPressed(sc_ScrollLock))
    {
        inputState.ClearKeyStatus(sc_ScrollLock);

        switch (ud.recstat)
        {
        case 0:
            if (SHIFTS_IS_PRESSED)
                G_OpenDemoWrite();
            break;
        case 1:
            G_CloseDemoWrite();
            break;
        }
    }

    if (ud.recstat == 2)
    {
        if (inputState.GetKeyStatus(sc_Space))
        {
            inputState.ClearKeyStatus(sc_Space);

            g_demo_paused = !g_demo_paused;
            g_demo_rewind = 0;

            if (g_demo_paused)
                FX_StopAllSounds();
        }

        if (inputState.GetKeyStatus(sc_Tab))
        {
            inputState.ClearKeyStatus(sc_Tab);
            g_demo_showStats = !g_demo_showStats;
        }

#if 0
        if (inputState.GetKeyStatus(sc_kpad_Plus))
        {
            G_InitTimer(240);
        }
        else if (inputState.GetKeyStatus(sc_kpad_Minus))
        {
            G_InitTimer(60);
        }
        else if (g_timerTicsPerSecond != 120)
        {
            G_InitTimer(120);
        }
#endif

        if (inputState.GetKeyStatus(sc_kpad_6))
        {
            inputState.ClearKeyStatus(sc_kpad_6);

            int const fwdTics = (15 << (int)ALT_IS_PRESSED) << (2 * (int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt    = g_demo_paused ? g_demo_cnt + 1 : g_demo_cnt + REALGAMETICSPERSEC * fwdTics;
            g_demo_rewind     = 0;

            if (g_demo_goalCnt > g_demo_totalCnt)
                g_demo_goalCnt = 0;
            else
                Demo_PrepareWarp();
        }
        else if (inputState.GetKeyStatus(sc_kpad_4))
        {
            inputState.ClearKeyStatus(sc_kpad_4);

            int const rewindTics = (15 << (int)ALT_IS_PRESSED) << (2 * (int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt       = g_demo_paused ? g_demo_cnt - 1 : g_demo_cnt - REALGAMETICSPERSEC * rewindTics;
            g_demo_rewind        = 1;

            if (g_demo_goalCnt <= 0)
                g_demo_goalCnt = 1;

            Demo_PrepareWarp();
        }
    }

    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED || WIN_IS_PRESSED)
    {
        int ridiculeNum = 0;

        // NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
        for (bssize_t j=sc_F1; j<=sc_F10; j++)
            if (inputState.UnboundKeyPressed(j))
            {
                inputState.ClearKeyStatus(j);
                ridiculeNum = j - sc_F1 + 1;
                break;
            }

        if (ridiculeNum)
        {
            if (SHIFTS_IS_PRESSED)
            {
                G_AddUserQuote(*CombatMacros[ridiculeNum-1]);
				Net_SendTaunt(ridiculeNum);
				pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (G_StartRTS(ridiculeNum, 1))
            {
				Net_SendRTS(ridiculeNum);
				pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
        {
            buttonMap.ClearButton(gamefunc_Third_Person_View);

            if (!RRRA || (!g_player[myconnectindex].ps->OnMotorcycle && !g_player[myconnectindex].ps->OnBoat))
            {
                g_player[myconnectindex].ps->over_shoulder_on = !g_player[myconnectindex].ps->over_shoulder_on;

                CAMERADIST  = 0;
                CAMERACLOCK = (int32_t) totalclock;

                P_DoQuote(QUOTE_VIEW_MODE_OFF + g_player[myconnectindex].ps->over_shoulder_on, g_player[myconnectindex].ps);
            }
        }

        if (ud.overhead_on != 0)
        {
            int const timerOffset = ((int) totalclock - nonsharedtimer);
            nonsharedtimer += timerOffset;

            if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
                g_player[myconnectindex].ps->zoom += mulscale6(timerOffset, max<int>(g_player[myconnectindex].ps->zoom, 256));

            if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
                g_player[myconnectindex].ps->zoom -= mulscale6(timerOffset, max<int>(g_player[myconnectindex].ps->zoom, 256));

            g_player[myconnectindex].ps->zoom = clamp(g_player[myconnectindex].ps->zoom, 48, 2048);
        }
    }

#if 0 // fixme: We should not query Esc here, this needs to be done differently
    if (I_EscapeTrigger() && ud.overhead_on && g_player[myconnectindex].ps->newowner == -1)
    {
        I_EscapeTriggerClear();
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on   = 0;
        ud.scrollmode    = 0;
        G_UpdateScreenArea();
    }
#endif

    if (buttonMap.ButtonDown(gamefunc_Map))
    {
        buttonMap.ClearButton(gamefunc_Map);
        if (ud.last_overhead != ud.overhead_on && ud.last_overhead)
        {
            ud.overhead_on = ud.last_overhead;
            ud.last_overhead = 0;
        }
        else
        {
            ud.overhead_on++;
            if (ud.overhead_on == 3) ud.overhead_on = 0;
            ud.last_overhead = ud.overhead_on;
        }

#ifdef __ANDROID__
        if (ud.overhead_on == 1)
            ud.scrollmode = 0;
        else if (ud.overhead_on == 2)
        {
            ud.scrollmode = 1;
            ud.folx = g_player[screenpeek].ps->opos.x;
            ud.foly = g_player[screenpeek].ps->opos.y;
            ud.fola = g_player[screenpeek].ps->oang;
        }
#endif
        restorepalette = 1;
        G_UpdateScreenArea();
    }
}

// Returns:
//   0: all OK
//  -1: ID declaration was invalid:
static int32_t S_DefineMusic(const char *ID, const char *name)
{
    int32_t sel = MUS_FIRST_SPECIAL;

    Bassert(ID != NULL);

    if (!Bstrcmp(ID,"intro"))
    {
        // nothing
    }
    else if (!Bstrcmp(ID,"briefing"))
    {
        sel++;
    }
    else if (!Bstrcmp(ID,"loading"))
    {
        sel += 2;
    }
    else
    {
        sel = G_GetMusicIdx(ID);
        if (sel < 0)
            return -1;
    }
	mapList[sel].music = name;
	return 0;
}

static int parsedefinitions_game(scriptfile *, int);

static void parsedefinitions_game_include(const char *fileName, scriptfile *pScript, const char *cmdtokptr, int const firstPass)
{
    scriptfile *included = scriptfile_fromfile(fileName);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || pScript == NULL) // this is a bit overboard to prevent unused parameter warnings
            {
           // Printf("Warning: Failed including %s as module\n", fn);
            }
/*
        else
            {
            Printf("Warning: Failed including %s on line %s:%d\n",
                       fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
            }
*/
    }
    else
    {
        parsedefinitions_game(included, firstPass);
        scriptfile_close(included);
    }
}

static void parsedefinitions_game_animsounds(scriptfile *pScript, const char * blockEnd, char const * fileName, dukeanim_t * animPtr)
{
    size_t numPairs = 0, allocSize = 4;

    animPtr->Sounds.Clear();

    int defError = 1;
    uint16_t lastFrameNum = 1;

    while (pScript->textptr < blockEnd)
    {
        int32_t frameNum;
        int32_t soundNum;

        // HACK: we've reached the end of the list
        //  (hack because it relies on knowledge of
        //   how scriptfile_* preprocesses the text)
        if (blockEnd - pScript->textptr == 1)
            break;

        // would produce error when it encounters the closing '}'
        // without the above hack
        if (scriptfile_getnumber(pScript, &frameNum))
            break;

        defError = 1;

        if (scriptfile_getsymbol(pScript, &soundNum))
            break;

        // frame numbers start at 1 for us
        if (frameNum <= 0)
        {
            Printf("Error: frame number must be greater zero on line %s:%d\n", pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        if (frameNum < lastFrameNum)
        {
            Printf("Error: frame numbers must be in (not necessarily strictly)"
                       " ascending order (line %s:%d)\n",
                       pScript->filename, scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        lastFrameNum = frameNum;

        if ((unsigned)soundNum >= MAXSOUNDS && soundNum != -1)
        {
            Printf("Error: sound number #%d invalid on line %s:%d\n", soundNum, pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        defError = 0;

        animsound_t sound;
        sound.frame = frameNum;
        sound.sound = soundNum;
        animPtr->Sounds.Push(sound);

        ++numPairs;
    }

    if (!defError)
    {
        // Printf("Defined sound sequence for hi-anim \"%s\" with %d frame/sound pairs\n",
        //           hardcoded_anim_tokens[animnum].text, numpairs);
    }
    else
    {
        Printf("Failed defining sound sequence for anim \"%s\".\n", fileName);
    }
    animPtr->Sounds.ShrinkToFit();
}

static int parsedefinitions_game(scriptfile *pScript, int firstPass)
{
    int   token;
    char *pToken;

    static const tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "includedefault",  T_INCLUDEDEFAULT   },
        { "#includedefault", T_INCLUDEDEFAULT   },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautoload",      T_NOAUTOLOAD       },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },
        { "cutscene",        T_CUTSCENE         },
        { "animsounds",      T_ANIMSOUNDS       },
        { "renamefile",      T_RENAMEFILE       },
        { "globalgameflags", T_GLOBALGAMEFLAGS  },
    };

    static const tokenlist soundTokens[] =
    {
        { "id",       T_ID },
        { "file",     T_FILE },
        { "minpitch", T_MINPITCH },
        { "maxpitch", T_MAXPITCH },
        { "priority", T_PRIORITY },
        { "type",     T_TYPE },
        { "distance", T_DISTANCE },
        { "volume",   T_VOLUME },
    };

    static const tokenlist animTokens [] =
    {
        { "delay",         T_DELAY },
        { "aspect",        T_ASPECT },
        { "sounds",        T_SOUND },
        { "forcefilter",   T_FORCEFILTER },
        { "forcenofilter", T_FORCENOFILTER },
        { "texturefilter", T_TEXTUREFILTER },
    };

    do
    {
        token  = getatoken(pScript, tokens, ARRAY_SIZE(tokens));
        pToken = pScript->ltextptr;

        switch (token)
        {
        case T_LOADGRP:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript,&fileName) && firstPass)
            {
				fileSystem.AddAdditionalFile(fileName);
            }
        }
        break;
        case T_CACHESIZE:
        {
            int32_t cacheSize;

            if (scriptfile_getnumber(pScript, &cacheSize) || !firstPass)
                break;
        }
        break;
        case T_INCLUDE:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript, &fileName))
                parsedefinitions_game_include(fileName, pScript, pToken, firstPass);

            break;
        }
        case T_INCLUDEDEFAULT:
        {
            parsedefinitions_game_include(G_DefaultDefFile(), pScript, pToken, firstPass);
            break;
        }
        case T_NOAUTOLOAD:
            if (firstPass)
                gNoAutoLoad = 1;
            break;
        case T_MUSIC:
        {
            char *tokenPtr = pScript->ltextptr;
            char *musicID  = NULL;
            char *fileName = NULL;
            char *musicEnd;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID: scriptfile_getstring(pScript, &musicID); break;
                    case T_FILE: scriptfile_getstring(pScript, &fileName); break;
                }
            }

            if (!firstPass)
            {
                if (musicID==NULL)
                {
                    Printf("Error: missing ID for music definition near line %s:%d\n",
                               pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || fileSystem.FileExists(fileName))
                    break;

                if (S_DefineMusic(musicID, fileName) == -1)
                    Printf("Error: invalid music ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
            }
        }
        break;

        case T_CUTSCENE:
        {
            char *fileName = NULL;

            scriptfile_getstring(pScript, &fileName);

            char *animEnd;

            if (scriptfile_getbraces(pScript, &animEnd))
                break;

            if (!firstPass)
            {
                dukeanim_t *animPtr = Anim_Find(fileName);

                if (!animPtr)
                {
                    animPtr = Anim_Create(fileName);
                    animPtr->framedelay = 10;
                    animPtr->frameflags = 0;
                }

                int32_t temp;

                while (pScript->textptr < animEnd)
                {
                    switch (getatoken(pScript, animTokens, ARRAY_SIZE(animTokens)))
                    {
                        case T_DELAY:
                            scriptfile_getnumber(pScript, &temp);
                            animPtr->framedelay = temp;
                            break;
                        case T_ASPECT:
                        {
                            double dtemp, dtemp2;
                            scriptfile_getdouble(pScript, &dtemp);
                            scriptfile_getdouble(pScript, &dtemp2);
                            animPtr->frameaspect1 = dtemp;
                            animPtr->frameaspect2 = dtemp2;
                            break;
                        }
                        case T_SOUND:
                        {
                            char *animSoundsEnd = NULL;
                            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                                break;
                            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
                            break;
                        }
                        case T_FORCEFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCEFILTER;
                            break;
                        case T_FORCENOFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCENOFILTER;
                            break;
                        case T_TEXTUREFILTER:
                            animPtr->frameflags |= CUTSCENE_TEXTUREFILTER;
                            break;
                    }
                }
            }
            else
                pScript->textptr = animEnd;
        }
        break;
        case T_ANIMSOUNDS:
        {
            char *tokenPtr     = pScript->ltextptr;
            char *fileName     = NULL;

            scriptfile_getstring(pScript, &fileName);
            if (!fileName)
                break;

            char *animSoundsEnd = NULL;

            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                break;

            if (firstPass)
            {
                pScript->textptr = animSoundsEnd;
                break;
            }

            dukeanim_t *animPtr = Anim_Find(fileName);

            if (!animPtr)
            {
                Printf("Error: expected animation filename on line %s:%d\n",
                    pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
                break;
            }

            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
        }
        break;

        case T_SOUND:
        {
            char *tokenPtr = pScript->ltextptr;
            char *fileName = NULL;
            char *musicEnd;

            double volume = 1.0;

            int32_t soundNum = -1;
            int32_t maxpitch = 0;
            int32_t minpitch = 0;
            int32_t priority = 0;
            int32_t type     = 0;
            int32_t distance = 0;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID:       scriptfile_getsymbol(pScript, &soundNum); break;
                    case T_FILE:     scriptfile_getstring(pScript, &fileName); break;
                    case T_MINPITCH: scriptfile_getsymbol(pScript, &minpitch); break;
                    case T_MAXPITCH: scriptfile_getsymbol(pScript, &maxpitch); break;
                    case T_PRIORITY: scriptfile_getsymbol(pScript, &priority); break;
                    case T_TYPE:     scriptfile_getsymbol(pScript, &type);     break;
                    case T_DISTANCE: scriptfile_getsymbol(pScript, &distance); break;
                    case T_VOLUME:   scriptfile_getdouble(pScript, &volume);   break;
                }
            }

            if (!firstPass)
            {
                if (soundNum==-1)
                {
                    Printf("Error: missing ID for sound definition near line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || fileSystem.FileExists(fileName))
                    break;

                // maybe I should have just packed this into a sound_t and passed a reference...
                if (S_DefineSound(soundNum, fileName, minpitch, maxpitch, priority, type, distance, volume) == -1)
                    Printf("Error: invalid sound ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
            }
        }
        break;
        case T_GLOBALGAMEFLAGS: scriptfile_getnumber(pScript, &duke3d_globalflags); break;
        case T_EOF: return 0;
        default: break;
        }
    }
    while (1);

    return 0;
}

int loaddefinitions_game(const char *fileName, int32_t firstPass)
{
    scriptfile *pScript = scriptfile_fromfile(fileName);

    if (pScript)
        parsedefinitions_game(pScript, firstPass);

    if (userConfig.AddDefs) for (auto& m : *userConfig.AddDefs)
        parsedefinitions_game_include(m, NULL, "null", firstPass);

    if (pScript)
        scriptfile_close(pScript);

    scriptfile_clearsymbols();

    return 0;
}


static void G_Cleanup(void)
{
    int32_t i;

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Xfree(g_player[i].ps);
        Xfree(g_player[i].input);
    }

    if (label != (char *)&sprite[0]) Xfree(label);
    if (labelcode != (int32_t *)&sector[0]) Xfree(labelcode);
#if 0
    if (labeltype != (int32_t*)&wall[0]) Xfree(labeltype);
#endif

}

/*
===================
=
= G_Startup
=
===================
*/

static void G_CompileScripts(void)
{
    label     = (char *)&sprite[0];     // V8: 16384*44/64 = 11264  V7: 4096*44/64 = 2816
    labelcode = (int32_t *)&sector[0]; // V8: 4096*40/4 = 40960    V7: 1024*40/4 = 10240

    loadcons(G_ConFile());
	fi.initactorflags();

    if ((uint32_t)labelcnt > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        G_GameExit("Error: too many labels defined!");

    {
        char *newlabel;
        int32_t *newlabelcode;
        int32_t *newlabeltype;

        newlabel     = (char *)Xmalloc(labelcnt << 6);
        newlabelcode = (int32_t *)Xmalloc(labelcnt * sizeof(int32_t));
        newlabeltype = (int32_t *)Xmalloc(labelcnt * sizeof(int32_t));

        Bmemcpy(newlabel, label, labelcnt*64);
        Bmemcpy(newlabelcode, labelcode, labelcnt*sizeof(int32_t));
#if 0
        Bmemcpy(newlabeltype, labeltype, labelcnt*sizeof(int32_t));
#endif

        label = newlabel;
        labelcode = newlabelcode;
#if 0
        labeltype = newlabeltype;
#endif
    }

    Bmemset(sprite, 0, MAXSPRITES*sizeof(spritetype));
    Bmemset(sector, 0, MAXSECTORS*sizeof(sectortype));
    Bmemset(wall, 0, MAXWALLS*sizeof(walltype));

    VM_OnEvent(EVENT_INIT);
}

static inline void G_CheckGametype(void)
{
    m_coop = clamp(*m_coop, 0, g_gametypeCnt-1);
    Printf("%s\n",g_gametypeNames[m_coop]);
    if (g_gametypeFlags[m_coop] & GAMETYPE_ITEMRESPAWN)
        ud.m_respawn_items = ud.m_respawn_inventory = 1;
}

void G_PostCreateGameState(void)
{
    Net_SendClientInfo();
}

void InitFonts();

static void G_Startup(void)
{
    int32_t i;

    timerInit(TICRATE);
    timerSetCallback(gameTimerHandler);

    G_CompileScripts();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (engineInit())
        G_FatalEngineError();

    G_InitDynamicTiles();
    G_InitDynamicSounds();

    // These depend on having the dynamic tile and/or sound mappings set up:
    G_InitMultiPsky(TILE_CLOUDYOCEAN, TILE_MOONSKY1, TILE_BIGORBIT1, TILE_LA);
    G_PostCreateGameState();
    if (g_netServer || ud.multimode > 1) G_CheckGametype();

	if (userConfig.CommandMap.IsNotEmpty())
	{
		FString startupMap;
        if (VOLUMEONE)
        {
            Printf("The -map option is available in the registered version only!\n");
        }
        else
        {
			startupMap = userConfig.CommandMap;
			if (startupMap.IndexOfAny("/\\") < 0) startupMap.Insert(0, "/");
			DefaultExtension(startupMap, ".map");
			startupMap.Substitute("\\", "/");
			NormalizeFileName(startupMap);

			if (fileSystem.FileExists(startupMap))
			{
                Printf("Using level: \"%s\".\n",startupMap.GetChars());
            }
            else
            {
                Printf("Level \"%s\" not found.\n",startupMap.GetChars());
                boardfilename[0] = 0;
            }
        }
		strncpy(boardfilename, startupMap, BMAX_PATH);
    }

    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].playerreadyflag = 0;

    Net_GetPackets();

    if (numplayers > 1)
        Printf("Multiplayer initialized.\n");

    if (TileFiles.artLoadFiles("tiles%03i.art") < 0)
        G_GameExit("Failed loading art.");

    InitFonts();

    // Make the fullscreen nuke logo background non-fullbright.  Has to be
    // after dynamic tile remapping (from C_Compile) and loading tiles.
    picanm[TILE_LOADSCREEN].sf |= PICANM_NOFULLBRIGHT_BIT;

//    Printf("Loading palette/lookups...\n");
    G_LoadLookups();
    TileFiles.PostLoadSetup();

    screenpeek = myconnectindex;
}

static void P_SetupMiscInputSettings(void)
{
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    ps->aim_mode = in_mousemode;
    ps->auto_aim = cl_autoaim;
    ps->weaponswitch = cl_weaponswitch;
}

void G_UpdatePlayerFromMenu(void)
{
    if (ud.recstat != 0)
        return;

    if (numplayers > 1)
    {
        Net_SendClientInfo();
        if (sprite[g_player[myconnectindex].ps->i].picnum == TILE_APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        /*int32_t j = g_player[myconnectindex].ps->team;*/

        P_SetupMiscInputSettings();
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);

        g_player[myconnectindex].pteam = playerteam;

        if (sprite[g_player[myconnectindex].ps->i].picnum == TILE_APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
}

void G_BackToMenu(void)
{
    boardfilename[0] = 0;
    if (ud.recstat == 1) G_CloseDemoWrite();
    ud.warp_on = 0;
    g_player[myconnectindex].ps->gm = 0;
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	inputState.keyFlushChars();
}

static int G_EndOfLevel(void)
{
	STAT_Update(ud.eog || (currentLevel->flags & MI_FORCEEOG));
	P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
    P_UpdateScreenPal(g_player[myconnectindex].ps);

    if (g_player[myconnectindex].ps->gm&MODE_EOL)
    {
        G_CloseDemoWrite();

        ready2send = 0;

        if (g_player[myconnectindex].ps->player_par > 0 && (g_player[myconnectindex].ps->player_par < ud.playerbest || ud.playerbest < 0) &&
            ud.display_bonus_screen == 1)
            CONFIG_SetMapBestTime(g_loadedMapHack.md4, g_player[myconnectindex].ps->player_par);

        if (ud.display_bonus_screen == 1)
        {
            int32_t i = ud.screen_size;
            ud.screen_size = 0;
            G_UpdateScreenArea();
            ud.screen_size = i;

            if (!RRRA || (g_mostConcurrentPlayers > 1 && numplayers > 1))
                G_BonusScreen(0);
            else
                G_BonusScreenRRRA(0);
        }

        // Clear potentially loaded per-map ART only after the bonus screens.
        artClearMapArt();

        if (ud.eog || (currentLevel->flags & MI_FORCEEOG))
        {
            ud.eog = 0;
            if ((!g_netServer && ud.multimode < 2))
            {
                if (!VOLUMEALL)
                    G_DoOrderScreen();
                g_player[myconnectindex].ps->gm = 0;
				return 2;
            }
            else
            {
                m_level_number = 0;
                ud.level_number = 0;
            }
        }
    }

    ud.display_bonus_screen = 1;
    ready2send = 0;

    if (numplayers > 1)
        g_player[myconnectindex].ps->gm = MODE_GAME;

    if (G_EnterLevel(g_player[myconnectindex].ps->gm))
    {
        return 2;
    }

    Net_WaitForEverybody();
    return 1;
}

#if defined(_WIN32) && defined(DEBUGGINGAIDS)
// See FILENAME_CASE_CHECK in cache1d.c
static int32_t check_filename_casing(void)
{
    return !(g_player[myconnectindex].ps->gm&MODE_GAME);
}
#endif

void G_MaybeAllocPlayer(int32_t pnum)
{
    if (g_player[pnum].ps == NULL)
        g_player[pnum].ps = (DukePlayer_t *)Xcalloc(1, sizeof(DukePlayer_t));
    if (g_player[pnum].input == NULL)
        g_player[pnum].input = (input_t *)Xcalloc(1, sizeof(input_t));
}

void app_loop();

// TODO: reorder (net)actor_t to eliminate slop and update assertion
EDUKE32_STATIC_ASSERT(sizeof(actor_t)%4 == 0);

static const char* actions[] = {
    "Move_Forward",
    "Move_Backward",
    "Turn_Left",
    "Turn_Right",
    "Strafe",
    "Fire",
    "Open",
    "Run",
    "Alt_Fire",	// Duke3D", Blood
    "Jump",
    "Crouch",
    "Look_Up",
    "Look_Down",
    "Look_Left",
    "Look_Right",
    "Strafe_Left",
    "Strafe_Right",
    "Aim_Up",
    "Aim_Down",
    "Weapon_1",
    "Weapon_2",
    "Weapon_3",
    "Weapon_4",
    "Weapon_5",
    "Weapon_6",
    "Weapon_7",
    "Weapon_8",
    "Weapon_9",
    "Weapon_10",
    "Inventory",
    "Inventory_Left",
    "Inventory_Right",
    "Holo_Duke",			// Duke3D", RR
    "Jetpack",
    "NightVision",
    "MedKit",
    "TurnAround",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Center_View",
    "Holster_Weapon",
    "Show_Opponents_Weapon",
    "Map_Follow_Mode",
    "See_Coop_View",
    "Mouse_Aiming",
    "Toggle_Crosshair",
    "Steroids",
    "Quick_Kick",
    "Next_Weapon",
    "Previous_Weapon",
    "Dpad_Select",
    "Dpad_Aiming",
    "Last_Weapon",
    "Alt_Weapon",
    "Third_Person_View",
    "Show_DukeMatch_Scores",
    "Toggle_Crouch",	// This is the last one used by EDuke32.
};

void InitFonts();

int32_t SetDefaults(void)
{
    g_player[0].ps->aim_mode = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.automsg = 0;
    ud.camerasprite = -1;

    ud.camera_time = 0;//4;

    ud.screen_tilting = 1;
    ud.statusbarflags = 0;// STATUSBAR_NOSHRINK;
    playerteam = 0;
    ud.angleinterpolation = 0;

    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.screenfade = 1;
    ud.menubackground = 1;
    ud.slidebar_paldisabled = 1;
    ud.shadow_pal = 4;
    return 0;
}


int GameInterface::app_main()
{
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        for (int j = 0; j < 10; j++)    
        {
            const char* s = "3457860291";
            ud.wchoice[i][j] = s[j] - '0';
        }
    }

    SetDispatcher();
    buttonMap.SetButtons(actions, NUM_ACTIONS);
    playing_rr = 1;
    g_skillCnt = 4;
    ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;

    g_movesPerPacket = 1;
    bufferjitter = 1;
    initsynccrc();

    // This needs to happen before G_CheckCommandLine() because G_GameExit()
    // accesses g_player[0].
    G_MaybeAllocPlayer(0);

    G_CheckCommandLine();

    SetDefaults();


    hud_size.Callback();
    hud_scale.Callback();
    S_InitSound();

    
    if (RR)
    {
        g_cdTrack = -1;
    }

    G_SetupCheats();

    if (SHAREWARE)
        g_Shareware = 1;
    else
    {
		if (fileSystem.FileExists("DUKESW.BIN")) // JBF 20030810
        {
            g_Shareware = 1;
        }
    }

    // gotta set the proper title after we compile the CONs if this is the full version

    if (g_scriptDebug)
        Printf("CON debugging activated (level %d).\n",g_scriptDebug);

    numplayers = 1;
    g_mostConcurrentPlayers = ud.multimode;

    if (!g_fakeMultiMode)
    {
        connectpoint2[0] = -1;
    }
    else
    {
        for (bssize_t i=0; i<ud.multimode-1; i++)
            connectpoint2[i] = i+1;
        connectpoint2[ud.multimode-1] = -1;

        for (bssize_t i=1; i<ud.multimode; i++)
            g_player[i].playerquitflag = 1;
    }

    Net_GetPackets();

    for (bssize_t i=0; i<MAXPLAYERS; i++)
        G_MaybeAllocPlayer(i);

    G_Startup(); // a bunch of stuff including compiling cons

    g_player[0].playerquitflag = 1;

    g_player[myconnectindex].ps->palette = BASEPAL;

    for (int i=1, j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(g_player[j].user_name,"%s %d", GStrings("PLAYER"),j+1);
        g_player[j].ps->team = g_player[j].pteam = i;
        g_player[j].ps->weaponswitch = 3;
        g_player[j].ps->auto_aim = 0;
        i = 1-i;
    }

    Anim_Init();

    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        Printf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }
    loaddefinitions_game(defsfile, FALSE);

	userConfig.AddDefs.reset();

    enginePostInit();

    tileDelete(TILE_MIRROR);
    skiptile = TILE_W_FORCEFIELD + 1;

    if (RR)
        tileDelete(0);
    if (RRRA)
        tileDelete(13);

    if (numplayers == 1 && boardfilename[0] != 0)
    {
        m_level_number  = 7;
        ud.m_volume_number = 0;
        ud.warp_on         = 1;
    }

    // getnames();

    if (g_netServer || ud.multimode > 1)
    {
        if (ud.warp_on == 0)
        {
            ud.m_monsters_off = 1;
            ud.m_player_skill = 0;
        }
    }

    g_mostConcurrentPlayers = ud.multimode;  // XXX: redundant?

    ud.last_level = -1;
    registerosdcommands();

#ifdef HAVE_CLIPSHAPE_FEATURE
    int const clipMapError = engineLoadClipMaps();
    if (clipMapError > 0)
        Printf("There was an error loading the sprite clipping map (status %d).\n", clipMapError);

    g_clipMapFiles.Reset();
#endif

    videoInit();
    InitFonts();
    V_LoadTranslations();
    videoSetPalette(BASEPAL, 0);

    // check if the minifont will support lowercase letters (3136-3161)
    // there is room for them in tiles012.art between "[\]^_." and "{|}~"
    minitext_lowercase = 1;

    for (bssize_t i = TILE_MINIFONT + ('a'-'!'); minitext_lowercase && i < TILE_MINIFONT + ('z'-'!') + 1; ++i)
        minitext_lowercase &= (int)tileCheck(i);

    /*if (RRRA)
        playmve("REDINT.MVE"); still needs work on the audio side*/

    //if (g_networkMode != NET_DEDICATED_SERVER)
    {
        Menu_Init();
    }

    FX_StopAllSounds();
    S_ClearSoundLocks();
	app_loop();
	return 0;
}
	
void app_loop()
{
	auto &myplayer = g_player[myconnectindex].ps;

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    g_player[myconnectindex].ps->fta = 0;
    for (int & q : user_quote_time)
        q = 0;

    if (ud.warp_on == 1)
    {
        G_NewGame_EnterLevel();
        // may change ud.warp_on in an error condition
    }

    if (ud.warp_on == 0)
    {
        if ((g_netServer || ud.multimode > 1) && boardfilename[0] != 0)
        {
            m_level_number = 7;
            ud.m_volume_number = 0;

            if (ud.m_player_skill == 4)
                ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            for (bssize_t TRAVERSE_CONNECT(i))
            {
                P_ResetWeapons(i);
                P_ResetInventory(i);
            }

            G_NewGame_EnterLevel();

            Net_WaitForEverybody();
        }
        else// if (g_networkMode != NET_DEDICATED_SERVER)
            G_DisplayLogo();

        //if (g_networkMode != NET_DEDICATED_SERVER)
        {
			M_StartControlPanel(false);
			M_SetMenu(NAME_Mainmenu);
			if (G_PlaybackDemo())
            {
                FX_StopAllSounds();
                g_noLogoAnim = 1;
                goto MAIN_LOOP_RESTART;
            }
        }
    }
    else G_UpdateScreenArea();

//    G_GameExit(" "); ///

    ud.showweapons = ud.config.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = playerteam;

    if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_GetTeamPalette(g_player[myconnectindex].pteam);
    else
    {
        if (playercolor) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);
        else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;
    }

    ud.warp_on = 0;
	inputState.ClearKeyStatus(sc_Pause);   // JBF: I hate the pause key

    do //main loop
    {
		handleevents();
		if (g_player[myconnectindex].ps->gm == MODE_DEMO)
		{
			M_ClearMenus();
			goto MAIN_LOOP_RESTART;
		}

        Net_GetPackets();

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        inputState.SetBindsEnabled(!!(g_player[myconnectindex].ps->gm & (MODE_GAME|MODE_DEMO)));

#if 0
//#ifndef _WIN32
        // stdin -> OSD input for dedicated server
        if (g_networkMode == NET_DEDICATED_SERVER)
        {
        }
        else
#endif
        {
            G_HandleLocalKeys();
        }

        OSD_DispatchQueued();

        char gameUpdate = false;
        double const gameUpdateStartTime = timerGetHiTicks();
        
        while (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
        {
            ototalclock += TICSPERFRAME;

            if (RRRA && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (RRRA && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            // this is where we fill the input_t struct that is actually processed by P_ProcessInput()
            auto const pPlayer = g_player[myconnectindex].ps;
            auto const q16ang  = fix16_to_int(pPlayer->q16ang);
            auto &     input   = inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex];

            input = localInput;
            input.fvel = mulscale9(localInput.fvel, sintable[(q16ang + 2560) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 2048) & 2047]) +
                         pPlayer->fric.x;
            input.svel = mulscale9(localInput.fvel, sintable[(q16ang + 2048) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 1536) & 2047]) +
                         pPlayer->fric.y;
            localInput = {};

            g_player[myconnectindex].movefifoend++;

            if (((!System_WantGuiCapture() && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (g_netServer || ud.multimode > 1)) &&
                    (g_player[myconnectindex].ps->gm&MODE_GAME))
            {
                G_MoveLoop();
            }
        }

        gameUpdate = true;
        g_gameUpdateTime = timerGetHiTicks()-gameUpdateStartTime;
        if (g_gameUpdateAvgTime < 0.f)
            g_gameUpdateAvgTime = g_gameUpdateTime;
        g_gameUpdateAvgTime = ((GAMEUPDATEAVGTIMENUMSAMPLES-1.f)*g_gameUpdateAvgTime+g_gameUpdateTime)/((float) GAMEUPDATEAVGTIMENUMSAMPLES);

        G_DoCheats();

        if (g_player[myconnectindex].ps->gm & (MODE_EOL|MODE_RESTART))
        {
            switch (G_EndOfLevel())
            {
                case 1: continue;
                case 2: goto MAIN_LOOP_RESTART;
            }
        }

        /*if (g_networkMode == NET_DEDICATED_SERVER)
        {
            idle();
        }
        else */if (G_FPSLimit())
        {
            if (RRRA && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (RRRA && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

            G_DrawRooms(screenpeek, smoothRatio);
            if (videoGetRenderMode() >= REND_POLYMOST)
                G_DrawBackground();
            G_DisplayRest(smoothRatio);
            videoNextPage();

            if (gameUpdate)
            {
                g_gameUpdateAndDrawTime = g_beforeSwapTime/* timerGetHiTicks()*/ - gameUpdateStartTime;
            }

        }

        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);
}

GAME_STATIC GAME_INLINE int32_t G_MoveLoop()
{
    int i;

    if (numplayers > 1)
        while (predictfifoplc < g_player[myconnectindex].movefifoend) Net_DoPrediction();

    Net_GetPackets();

    if (numplayers < 2) bufferjitter = 0;
    while (g_player[myconnectindex].movefifoend-movefifoplc > bufferjitter)
    {
        for(TRAVERSE_CONNECT(i))
        {
            if (movefifoplc == g_player[i].movefifoend) break;
        }
        if (i >= 0) break;
        if (G_DoMoveThings()) return 1;
    }


    return 0;
}

int G_DoMoveThings(void)
{
    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    // Moved lower so it is restored correctly by demo diffs:
    //if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (g_RTSPlaying > 0)
        g_RTSPlaying--;

    for (int & i : user_quote_time)
    {
        if (i)
        {
            if (--i > hud_messagetime)
                i = hud_messagetime;
            if (!i) pub = NUMPAGES;
        }
    }

    // Name display when aiming at opponents
    if (cl_idplayers && (g_netServer || ud.multimode > 1)
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        )
    {
        hitdata_t hitData;
        DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan((vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                sintable[fix16_to_int(pPlayer->q16ang) & 2047], fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) << 11, &hitData,
                0xffff0030);

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hitData.sprite >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) &&
                sprite[hitData.sprite].picnum == TILE_APLAYER)
        {
            int const playerNum = P_Get(hitData.sprite);

            if (playerNum != screenpeek && g_player[playerNum].ps->dead_flag == 0)
            {
                if (pPlayer->fta == 0 || pPlayer->ftq == QUOTE_RESERVED3)
                {
                    if (ldist(&sprite[pPlayer->i], &sprite[hitData.sprite]) < 9216)
                    {
						quoteMgr.FormatQuote(QUOTE_RESERVED3, "%s", &g_player[playerNum].user_name[0]);
                        pPlayer->fta = 12, pPlayer->ftq = QUOTE_RESERVED3;
                    }
                }
                else if (pPlayer->fta > 2) pPlayer->fta -= 3;
            }
        }
    }

    if (g_showShareware > 0)
    {
        g_showShareware--;
        if (g_showShareware == 0)
        {
            pus = NUMPAGES;
            pub = NUMPAGES;
        }
    }

    // Moved lower so it is restored correctly by diffs:
//    everyothertime++;

    //if (g_netClient) // [75] The server should not overwrite its own randomseed
    //    randomseed = ticrandomseed;

    for (bssize_t TRAVERSE_CONNECT(i))
        Bmemcpy(g_player[i].input, &inputfifo[movefifoplc&(MOVEFIFOSIZ-1)][i], sizeof(input_t));

    movefifoplc++;

    G_UpdateInterpolations();

    /*
        j = -1;
        for (TRAVERSE_CONNECT(i))
        {
            if (g_player[i].playerquitflag == 0 || TEST_SYNC_KEY(g_player[i].sync->bits,SK_GAMEQUIT) == 0)
            {
                j = i;
                continue;
            }

            G_CloseDemoWrite();

            g_player[i].playerquitflag = 0;
        }
    */

    Net_GetSyncStat();

    g_moveThingsCount++;

    if (ud.recstat == 1) G_DemoRecord();

    everyothertime++;
    if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (ud.pause_on == 0)
    {
        g_globalRandom = krand2();
        movedummyplayers();//ST 13
    }

    for (bssize_t TRAVERSE_CONNECT(i))
    {
        if (g_player[i].input->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                actor[g_player[i].ps->i].picnum = TILE_APLAYERTOP;
                quickkill(g_player[i].ps);
            }
        }
        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = G_GetTeamPalette(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

           hud_input(i);

        if (ud.pause_on == 0)
        {
            // todo: take HUD timer stuff out of the main game loop
            auto p = &ps[i];
            if (p->pals.f > 0)
                p->pals.f--;

            if (p->fta > 0)
            {
                p->fta--;
                if (p->fta == 0)
                {
                    p->ftq = 0;
                }
            }
            if (g_levelTextTime > 0)
                g_levelTextTime--;


            //P_ProcessInput(i);
            fi.processinput(i);
            fi.checksectors(i);
        }
    }

    if (ud.pause_on == 0)
        G_MoveWorld();

    Net_CorrectPrediction();

    //if (g_netServer)
    //    Net_SendServerUpdates();

    if ((everyothertime&1) == 0)
    {
        {
            fi.animatewalls();
            movecyclers();
        }

        //if (g_netServer && (everyothertime % 10) == 0)
        //{
        //    Net_SendMapUpdate();
        //}
    }

    //if (g_netClient)   //Slave
    //    Net_SendClientUpdate();

    if (RR && ud.recstat == 0 && ud.multimode < 2)
        dotorch();

    return 0;
}

void GameInterface::FreeGameData()
{
    setmapfog(0);
    G_Cleanup();
}

void GameInterface::UpdateScreenSize()
{
    G_UpdateScreenArea();
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

// access wrappers that alias old names to current data.
psaccess ps;
actor_t* hittype = actor;

END_DUKE_NS
