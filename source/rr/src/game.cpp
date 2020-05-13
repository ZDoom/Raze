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
#include "rendering/v_video.h"

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

BEGIN_RR_NS


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


int32_t A_CheckInventorySprite(spritetype *s)
{
    switch (DYNAMICTILEMAP(s->picnum))
    {
    case FIRSTAID__STATIC:
    case STEROIDS__STATIC:
    case HEATSENSOR__STATIC:
    case BOOTS__STATIC:
    case JETPACK__STATIC:
    case HOLODUKE__STATIC:
    case AIRTANK__STATIC:
        return 1;
    default:
        return 0;
    }
}

void G_OnMotorcycle(DukePlayer_t *pPlayer, int spriteNum)
{
    if (!pPlayer->on_motorcycle && !(sector[pPlayer->cursectnum].lotag == 2))
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
        pPlayer->on_motorcycle = 1;
        pPlayer->last_full_weapon = pPlayer->curr_weapon;
        pPlayer->curr_weapon = MOTORCYCLE_WEAPON;
        pPlayer->gotweapon |= (1 << MOTORCYCLE_WEAPON);
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->q16horiz = F16(100);
    }
    if (!A_CheckSoundPlaying(pPlayer->i,186))
        A_PlaySound(186, pPlayer->i);
}

void G_OffMotorcycle(DukePlayer_t *pPlayer)
{
    int j;
    if (pPlayer->on_motorcycle)
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
        pPlayer->on_motorcycle = 0;
        pPlayer->gotweapon &= ~(1<<MOTORCYCLE_WEAPON);
        pPlayer->curr_weapon = pPlayer->last_full_weapon;
        P_CheckWeapon(pPlayer);
        pPlayer->q16horiz = F16(100);
        pPlayer->moto_do_bump = 0;
        pPlayer->moto_speed = 0;
        pPlayer->tilt_status = 0;
        pPlayer->moto_drink = 0;
        pPlayer->moto_bump_target = 0;
        pPlayer->moto_bump = 0;
        pPlayer->moto_turb = 0;
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        pPlayer->moto_underwater = 0;
        j = A_Spawn(pPlayer->i, EMPTYBIKE);
        sprite[j].ang = fix16_to_int(pPlayer->q16ang);
        sprite[j].xvel += sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        sprite[j].yvel += sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        sprite[j].owner = pPlayer->ammo_amount[MOTORCYCLE_WEAPON];
    }
}

void G_OnBoat(DukePlayer_t *pPlayer, int spriteNum)
{
    if (!pPlayer->on_boat)
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
        pPlayer->on_boat = 1;
        pPlayer->last_full_weapon = pPlayer->curr_weapon;
        pPlayer->curr_weapon = BOAT_WEAPON;
        pPlayer->gotweapon |= (1<<BOAT_WEAPON);
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->q16horiz = F16(100);
    }
}

void G_OffBoat(DukePlayer_t *pPlayer)
{
    int j;
    if (pPlayer->on_boat)
    {
        pPlayer->on_boat = 0;
        pPlayer->gotweapon &= ~(1<<BOAT_WEAPON);
        pPlayer->curr_weapon = pPlayer->last_full_weapon;
        P_CheckWeapon(pPlayer);
        pPlayer->q16horiz = F16(100);
        pPlayer->moto_do_bump = 0;
        pPlayer->moto_speed = 0;
        pPlayer->tilt_status = 0;
        pPlayer->moto_drink = 0;
        pPlayer->moto_bump_target = 0;
        pPlayer->moto_bump = 0;
        pPlayer->moto_turb = 0;
        pPlayer->vel.x = 0;
        pPlayer->vel.y = 0;
        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
        pPlayer->moto_underwater = 0;
        j = A_Spawn(pPlayer->i, EMPTYBOAT);
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

        if (sprite[k].picnum != SECTOREFFECTOR && sprite[k].z >= sp->z)
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
    if ((gotpic[MIRROR>>3]&(1<<(MIRROR&7)))
#ifdef POLYMER
        && (videoGetRenderMode() != REND_POLYMER)
#endif
        )
    {
        if (g_mirrorCount == 0)
        {
            // NOTE: We can have g_mirrorCount==0 but gotpic'd MIRROR,
            // for example in LNGA2.
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
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

        if (wall[g_mirrorWall[i]].overpicnum != MIRROR)
        {
            // Try to find a new mirror wall in case the original one was broken.

            int32_t startwall = sector[g_mirrorSector[i]].wallptr;
            int32_t endwall = startwall + sector[g_mirrorSector[i]].wallnum;

            for (bssize_t k=startwall; k<endwall; k++)
            {
                int32_t j = wall[k].nextwall;
                if (j >= 0 && (wall[j].cstat&32) && wall[j].overpicnum==MIRROR)  // cmp. premap.c
                {
                    g_mirrorWall[i] = j;
                    break;
                }
            }
        }

        if (wall[g_mirrorWall[i]].overpicnum == MIRROR)
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
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
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
        videoClearScreen(0);
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

    if (RRRA && g_fogType)
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

        if (!RRRA || !pPlayer->drug_mode)
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
            renderSetRollAngle(fix16_to_float(pPlayer->q16rotscrnang));
#endif
            pPlayer->oq16rotscrnang = pPlayer->q16rotscrnang;
        }

        if (RRRA && pPlayer->drug_mode > 0)
        {
            while (pPlayer->drug_timer < totalclock && !(pPlayer->gm & MODE_MENU) && !ud.pause_on && !GUICapture)
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
                        pPlayer->drug_mode = 0;
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
                        pPlayer->drug_mode--;
                        if (pPlayer->drug_mode == 1)
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
                CAMERA(q16ang)   = myang + pPlayer->q16look_ang;
                CAMERA(q16horiz) = myhoriz + myhorizoff;
                CAMERA(sect)     = mycursectnum;
            }
            else
            {
                vec3_t const camVect = { pPlayer->opos.x + mulscale16(pPlayer->pos.x - pPlayer->opos.x, smoothRatio),
                                         pPlayer->opos.y + mulscale16(pPlayer->pos.y - pPlayer->opos.y, smoothRatio),
                                         pPlayer->opos.z + mulscale16(pPlayer->pos.z - pPlayer->opos.z, smoothRatio) };

                CAMERA(pos)          = camVect;

                if (pPlayer->wackedbyactor >= 0)
                {
                    CAMERA(q16ang)   = pPlayer->oq16ang
                                     + mulscale16(((pPlayer->q16ang + F16(1024) - pPlayer->oq16ang) & 0x7FFFFFF) - F16(1024), smoothRatio)
                                     + pPlayer->q16look_ang;
                    CAMERA(q16horiz) = pPlayer->oq16horiz + pPlayer->oq16horizoff
                                     + mulscale16((pPlayer->q16horiz + pPlayer->q16horizoff - pPlayer->oq16horiz - pPlayer->oq16horizoff), smoothRatio);
                }
                else
                {
                    CAMERA(q16ang)   = pPlayer->q16ang + pPlayer->q16look_ang;
                    CAMERA(q16horiz) = pPlayer->q16horiz + pPlayer->q16horizoff;
                }
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
            CAMERA(q16ang)   = pPlayer->q16ang + pPlayer->q16look_ang;
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

        screen->BeginScene();
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
        gotpic[MIRROR>>3] |= (1<<(MIRROR&7));
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
        screen->FinishScene();
    }

    G_RestoreInterpolations();

    if (!RRRA || !g_fogType)
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

    VM_ScriptInfo(insptr, 64);

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
    if (g_tile[tilenum].execPtr)
    {
        SH(i) = *(g_tile[tilenum].execPtr);
        AC_ACTION_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+1);
        AC_MOVE_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+2);

        if (set_movflag_uncond || (*(g_tile[tilenum].execPtr + 3) && SHT(i) == 0))  // AC_MOVFLAGS
            SHT(i) = *(g_tile[tilenum].execPtr+3);

        return 1;
    }

    return 0;
}

static actor_t NullActor;
static spriteext_t NullSprExt;
static spritesmooth_t NullSprSmooth;

int32_t A_InsertSprite(int16_t whatsect,int32_t s_x,int32_t s_y,int32_t s_z,int16_t s_pn,int8_t s_s,
                       uint8_t s_xr,uint8_t s_yr,int16_t s_a,int16_t s_ve,int16_t s_zv,int16_t s_ow,int16_t s_ss)
{
    if (RR && s_ow < 0)
        return 0;

    int32_t i = /*Net_IsRelevantStat(s_ss) ? Net_InsertSprite(whatsect, s_ss) : */insertsprite(whatsect, s_ss);

    if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXSPRITES))
    {
        G_DumpDebugInfo();
        Printf("Failed spawning pic %d spr from pic %d spr %d at x:%d,y:%d,z:%d,sect:%d\n",
                          s_pn,s_ow < 0 ? -1 : TrackerCast(sprite[s_ow].picnum),s_ow,s_x,s_y,s_z,whatsect);
        G_GameExit("Too many sprites spawned.");
    }

    uspritetype spr_temp = { s_x, s_y,      s_z,  0,   s_pn, s_s,  0, 0,    0, s_xr, s_yr, 0,
                             0,   whatsect, s_ss, s_a, s_ow, s_ve, 0, s_zv, 0, 0,    0 };

#ifdef DEBUGGINGAIDS
    g_spriteStat.numins++;
#endif

    spritetype *s = &sprite[i];
    *s = *(spritetype *)&spr_temp;
    actor[i] = NullActor;
    actor[i].bpos = *(vec3_t *)s;

    if ((unsigned)s_ow < MAXSPRITES)
    {
        actor[i].picnum = sprite[s_ow].picnum;
        actor[i].floorz = actor[s_ow].floorz;
        actor[i].ceilingz = actor[s_ow].ceilingz;
    }

    actor[i].actorstayput = actor[i].extra = -1;
#ifdef POLYMER
    actor[i].lightId = -1;
#endif
    actor[i].owner = s_ow;

    G_InitActor(i, s_pn, 1);

    spriteext[i] = NullSprExt;
    spritesmooth[i] = NullSprSmooth;

    return i;
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

// A_Spawn has two forms with arguments having different meaning:
//
// 1. spriteNum>=0: Spawn from parent sprite <spriteNum> with picnum <tileNum>
// 2. spriteNum<0: Spawn from already *existing* sprite <tileNum>
int A_Spawn(int spriteNum, int tileNum)
{
    int         newSprite;
    spritetype *pSprite;
    actor_t *   pActor;
    int         sectNum;


    if (spriteNum >= 0)
    {
        // spawn from parent sprite <j>
        newSprite = A_InsertSprite(sprite[spriteNum].sectnum,sprite[spriteNum].x,sprite[spriteNum].y,sprite[spriteNum].z,
                           tileNum,0,0,0,0,0,0,spriteNum,0);
        actor[newSprite].picnum = sprite[spriteNum].picnum;
    }
    else
    {
        // spawn from already existing sprite <pn>
        newSprite                 = tileNum;
        spritetype *const pSprite = &sprite[newSprite];
        actor_t *const    pActor  = &actor[newSprite];

        Bmemset(&actor[newSprite], 0, sizeof(actor_t));
        Bmemcpy(&pActor->bpos, &sprite[newSprite], sizeof(vec3_t));

        pActor->picnum = pSprite->picnum;

        if (pSprite->picnum == SECTOREFFECTOR && pSprite->lotag == 50)
            pActor->picnum = pSprite->owner;

        pSprite->owner = pActor->owner = newSprite;

        pActor->floorz   = sector[pSprite->sectnum].floorz;
        pActor->ceilingz = sector[pSprite->sectnum].ceilingz;

        pActor->actorstayput = pActor->extra = -1;

#ifdef POLYMER
        pActor->lightId = -1;
#endif

        if ((pSprite->cstat & 48)
            && (RR || (pSprite->picnum != SPEAKER
            && pSprite->picnum != LETTER
            && pSprite->picnum != DUCK
            && pSprite->picnum != TARGET
            && pSprite->picnum != TRIPBOMB
            && pSprite->picnum != VIEWSCREEN
            && pSprite->picnum != VIEWSCREEN2))
            && (!(pSprite->picnum >= CRACK1 && pSprite->picnum <= CRACK4)))
        {
            if (pSprite->shade == 127)
                goto SPAWN_END;

            if (A_CheckSwitchTile(newSprite) && (pSprite->cstat & 16))
            {
                if (pSprite->pal && pSprite->picnum != ACCESSSWITCH && pSprite->picnum != ACCESSSWITCH2)
                {
                    if (((!g_netServer && ud.multimode < 2)) || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_DMSWITCHES)))
                    {
                        pSprite->xrepeat = pSprite->yrepeat = 0;
                        pSprite->lotag = pSprite->hitag = 0;
                        pSprite->cstat = 32768;
                        goto SPAWN_END;
                    }
                }

                pSprite->cstat |= 257;

                if (pSprite->pal && pSprite->picnum != ACCESSSWITCH && pSprite->picnum != ACCESSSWITCH2)
                    pSprite->pal = 0;

                goto SPAWN_END;
            }

            if (pSprite->hitag)
            {
                changespritestat(newSprite, STAT_FALLER);
                pSprite->cstat |= 257;
                pSprite->extra = g_impactDamage;
                goto SPAWN_END;
            }
        }

        if (pSprite->cstat & 1)
            pSprite->cstat |= 256;

        if (!G_InitActor(newSprite, pSprite->picnum, 0))
            T2(newSprite) = T5(newSprite) = 0;  // AC_MOVE_ID, AC_ACTION_ID
    }

    pSprite = &sprite[newSprite];
    pActor  = &actor[newSprite];
    sectNum = pSprite->sectnum;

    //some special cases that can't be handled through the dynamictostatic system.

    if (pSprite->picnum >= CAMERA1 && pSprite->picnum <= CAMERA1 + 4)
        pSprite->picnum = CAMERA1;
    else if (pSprite->picnum >= BOLT1 && pSprite->picnum <= BOLT1 + 3)
        pSprite->picnum = BOLT1;
    else if (!RR && pSprite->picnum >= SIDEBOLT1 && pSprite->picnum <= SIDEBOLT1 + 3)
        pSprite->picnum = SIDEBOLT1;
    if (DEER && pSprite->picnum != APLAYER && pSprite->picnum != RRTILE7936)
    {
        goto default_case;
    }
    else if (RRRA && pSprite->picnum == PIG+11)
    {
        pSprite->xrepeat = 16;
        pSprite->yrepeat = 16;
        pSprite->clipdist = 0;
        pSprite->extra = 0;
        pSprite->cstat = 0;
        changespritestat(newSprite, 121);
    }
    else
        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        default:
default_case:
            if (G_HaveActor(pSprite->picnum))
            {
                if (spriteNum == -1 && pSprite->lotag > ud.player_skill)
                {
                    pSprite->xrepeat = pSprite->yrepeat = 0;
                    changespritestat(newSprite, STAT_MISC);
                    break;
                }

                //  Init the size
                if (pSprite->xrepeat == 0 || pSprite->yrepeat == 0)
                    pSprite->xrepeat = pSprite->yrepeat = 1;

                if (A_CheckSpriteFlags(newSprite, SFLAG_BADGUY))
                {
                    if (ud.monsters_off == 1)
                    {
                        pSprite->xrepeat = pSprite->yrepeat = 0;
                        changespritestat(newSprite, STAT_MISC);
                        break;
                    }

                    A_Fall(newSprite);

                    if (A_CheckSpriteFlags(newSprite, SFLAG_BADGUYSTAYPUT))
                        pActor->actorstayput = pSprite->sectnum;

                    if (!RR || A_CheckSpriteFlags(newSprite, SFLAG_KILLCOUNT))
                        g_player[myconnectindex].ps->max_actors_killed++;
                    pSprite->clipdist = 80;

                    if (spriteNum >= 0)
                    {
                        if (sprite[spriteNum].picnum == RESPAWN)
                            pActor->tempang = sprite[newSprite].pal = sprite[spriteNum].pal;

                        A_PlayAlertSound(newSprite);
                        changespritestat(newSprite, STAT_ACTOR);
                    }
                    else
                        changespritestat(newSprite, STAT_ZOMBIEACTOR);
                }
                else
                {
                    pSprite->clipdist = 40;
                    pSprite->owner    = newSprite;
                    changespritestat(newSprite, STAT_ACTOR);
                }

                pActor->timetosleep = 0;

                if (spriteNum >= 0)
                    pSprite->ang = sprite[spriteNum].ang;
            }
            break;
        case FOF__STATIC:
            if (RR) goto default_case;
            pSprite->xrepeat = pSprite->yrepeat = 0;
            changespritestat(newSprite, STAT_MISC);
            break;
        case RRTILE280__STATICRR:
        case RRTILE281__STATICRR:
        case RRTILE282__STATICRR:
        case RRTILE283__STATICRR:
        case RRTILE2025__STATICRR:
        case RRTILE2026__STATICRR:
        case RRTILE2027__STATICRR:
        case RRTILE2028__STATICRR:
            pSprite->cstat = 0;
            pSprite->cstat |= 32768;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            pSprite->clipdist = 0;
            pSprite->extra = 0;
            changespritestat(newSprite, 105);
            break;
        case RRTILE3410__STATICRR:
            pSprite->extra = 0;
            changespritestat(newSprite, 107);
            break;
        case RRTILE8450__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 64;
            pSprite->yrepeat = 64;
            pSprite->extra = pSprite->lotag;
            pSprite->cstat |= 257;
            changespritestat(newSprite, 116);
            break;
        case RRTILE8487__STATICRR:
        case RRTILE8489__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 32;
            pSprite->yrepeat = 32;
            pSprite->extra = 0;
            pSprite->cstat |= 257;
            pSprite->hitag = 0;
            changespritestat(newSprite, 117);
            break;
        case RRTILE7424__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->extra = 0;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            changespritestat(newSprite, 11);
            break;
        case RRTILE7936__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            G_SetFog(2);
            g_fogType = 1;
            break;
        case RRTILE6144__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            for (bssize_t TRAVERSE_CONNECT(playerNum))
                g_player[playerNum].ps->sea_sick_stat = 1;
            break;
        case RRTILE8448__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->lotag = 1;
            pSprite->clipdist = 0;
            break;
        case RRTILE8099__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->lotag = 5;
            pSprite->clipdist = 0;
            changespritestat(newSprite, 123);
            break;
        case RRTILE8704__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->lotag = 1;
            pSprite->clipdist = 0;
            break;
        case RRTILE8192__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            g_ufoSpawnMinion = 1;
            break;
        case RRTILE8193__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            g_pistonSound = 1;
            break;
        case RRTILE8165__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->lotag = 1;
            pSprite->clipdist = 0;
            pSprite->owner = newSprite;
            pSprite->extra = 0;
            changespritestat(newSprite, 115);
            break;
        case RRTILE8593__STATICRR:
            if (!RRRA) goto default_case;
            pSprite->lotag = 1;
            pSprite->clipdist = 0;
            pSprite->owner = newSprite;
            pSprite->extra = 0;
            changespritestat(newSprite, 122);
            break;
//#endif
        case RRTILE285__STATICRR:
        case RRTILE286__STATICRR:
        case RRTILE287__STATICRR:
        case RRTILE288__STATICRR:
        case RRTILE289__STATICRR:
        case RRTILE290__STATICRR:
        case RRTILE291__STATICRR:
        case RRTILE292__STATICRR:
        case RRTILE293__STATICRR:
            pSprite->cstat = 0;
            pSprite->cstat |= 32768;
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            pSprite->clipdist = 0;
            pSprite->lotag = 0;
            changespritestat(newSprite, 106);
            break;

        case WATERSPLASH2__STATIC:
        case MUD__STATICRR:
            if (spriteNum >= 0)
            {
                setsprite(newSprite, (vec3_t *)&sprite[spriteNum]);
                pSprite->xrepeat = pSprite->yrepeat = 8+(krand2()&7);
            }
            else pSprite->xrepeat = pSprite->yrepeat = 16+(krand2()&15);

            pSprite->shade = -16;
            pSprite->cstat |= 128;

            if (spriteNum >= 0)
            {
                if (sector[sprite[spriteNum].sectnum].lotag == ST_2_UNDERWATER)
                {
                    pSprite->z = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) + (16 << 8);
                    pSprite->cstat |= 8;
                }
                else if (sector[sprite[spriteNum].sectnum].lotag == ST_1_ABOVE_WATER)
                    pSprite->z = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            }

            if (sector[sectNum].floorpicnum == FLOORSLIME || sector[sectNum].ceilingpicnum == FLOORSLIME)
                pSprite->pal = 7;
            fallthrough__;
        case NEON1__STATIC:
        case NEON2__STATIC:
        case NEON3__STATIC:
        case NEON4__STATIC:
        case NEON5__STATIC:
        case NEON6__STATIC:
        case DOMELITE__STATIC:
            if (pSprite->picnum != WATERSPLASH2)
                pSprite->cstat |= 257;
            fallthrough__;
        case NUKEBUTTON__STATIC:
            if (RR && pSprite->picnum == NUKEBUTTON)
                goto default_case;
            if (pSprite->picnum == DOMELITE)
                pSprite->cstat |= 257;
            fallthrough__;
        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case DUKETORSO__STATIC:
        case DUKEGUN__STATIC:
        case DUKELEG__STATIC:
            if (RR && pSprite->picnum == JIBS6)
            {
                pSprite->xrepeat >>= 1;
                pSprite->yrepeat >>= 1;
            }
            changespritestat(newSprite, STAT_MISC);
            break;
        case HEADJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LEGJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
            if (RR) goto default_case;
            changespritestat(newSprite, STAT_MISC);
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
            if (!RRRA) goto default_case;
            if (pSprite->picnum == RABBITJIBA)
            {
                pSprite->xrepeat = 18;
                pSprite->yrepeat = 18;
            }
            else if (pSprite->picnum == RABBITJIBB)
            {
                pSprite->xrepeat = 36;
                pSprite->yrepeat = 36;
            }
            else if (pSprite->picnum == RABBITJIBC)
            {
                pSprite->xrepeat = 54;
                pSprite->yrepeat = 54;
            }
            fallthrough__;
        case BILLYJIBA__STATICRR:
        case BILLYJIBB__STATICRR:
        case HULKJIBA__STATICRR:
        case HULKJIBB__STATICRR:
        case HULKJIBC__STATICRR:
        case MINJIBA__STATICRR:
        case MINJIBB__STATICRR:
        case MINJIBC__STATICRR:
        case COOTJIBA__STATICRR:
        case COOTJIBB__STATICRR:
        case COOTJIBC__STATICRR:
            changespritestat(newSprite, STAT_MISC);
            break;
        case TONGUE__STATIC:
            if (spriteNum >= 0)
                pSprite->ang = sprite[spriteNum].ang;
            pSprite->z -= 38<<8;
            pSprite->zvel = 256-(krand2()&511);
            pSprite->xvel = 64-(krand2()&127);
            changespritestat(newSprite, STAT_PROJECTILE);
            break;
        case NATURALLIGHTNING__STATIC:
            if (RR) goto default_case;
            pSprite->cstat &= ~257;
            pSprite->cstat |= 32768;
            break;
        case TRANSPORTERSTAR__STATIC:
        case TRANSPORTERBEAM__STATIC:
            if (spriteNum == -1) break;
            if (pSprite->picnum == TRANSPORTERBEAM)
            {
                pSprite->xrepeat = 31;
                pSprite->yrepeat = 1;
                pSprite->z = sector[sprite[spriteNum].sectnum].floorz-PHEIGHT;
            }
            else
            {
                if (sprite[spriteNum].statnum == STAT_PROJECTILE)
                    pSprite->xrepeat = pSprite->yrepeat = 8;
                else
                {
                    pSprite->xrepeat = 48;
                    pSprite->yrepeat = 64;
                    if (sprite[spriteNum].statnum == STAT_PLAYER || A_CheckEnemySprite(&sprite[spriteNum]))
                        pSprite->z -= ZOFFSET5;
                }
            }

            pSprite->shade = -127;
            pSprite->cstat = 128|2;
            pSprite->ang = sprite[spriteNum].ang;

            pSprite->xvel = 128;
            changespritestat(newSprite, STAT_MISC);
            A_SetSprite(newSprite,CLIPMASK0);
            setsprite(newSprite,(vec3_t *)pSprite);
            break;

        case FRAMEEFFECT1__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                if (RR && sprite[spriteNum].picnum == APLAYER)
                    T2(newSprite) = SMALLSMOKE;
                else
                    T2(newSprite) = sprite[spriteNum].picnum;
            }
            else pSprite->xrepeat = pSprite->yrepeat = 0;

            changespritestat(newSprite, STAT_MISC);

            break;

        case LASERLINE__STATIC:
            if (RR) goto default_case;
            pSprite->yrepeat = 6;
            pSprite->xrepeat = 32;

            if (g_tripbombLaserMode == 1)
                pSprite->cstat = 16 + 2;
            else if (g_tripbombLaserMode == 0 || g_tripbombLaserMode == 2)
                pSprite->cstat = 16;
            else
            {
                pSprite->xrepeat = 0;
                pSprite->yrepeat = 0;
            }

            if (spriteNum >= 0) pSprite->ang = actor[spriteNum].t_data[5]+512;
            changespritestat(newSprite, STAT_MISC);
            break;

        case FORCESPHERE__STATIC:
            if (spriteNum == -1)
            {
                pSprite->cstat = 32768;
                changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }
            else
            {
                pSprite->xrepeat = pSprite->yrepeat = 1;
                changespritestat(newSprite, STAT_MISC);
            }
            break;

        case BLOOD__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = RR ? 4 : 16;
            pSprite->z -= (26<<8);
            if (!RR && spriteNum >= 0 && sprite[spriteNum].pal == 6)
                pSprite->pal = 6;
            changespritestat(newSprite, STAT_MISC);
            break;
        case BLOODPOOL__STATIC:
        case PUKE__STATIC:
        {
            if (RR && pSprite->picnum == PUKE) goto default_case;
            int16_t pukeSect = pSprite->sectnum;

            updatesector(pSprite->x + 108, pSprite->y + 108, &pukeSect);
            if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
            {
                updatesector(pSprite->x - 108, pSprite->y - 108, &pukeSect);
                if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
                {
                    updatesector(pSprite->x + 108, pSprite->y - 108, &pukeSect);
                    if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
                    {
                        updatesector(pSprite->x - 108, pSprite->y + 108, &pukeSect);
                        if (pukeSect >= 0 && sector[pukeSect].floorz != sector[pSprite->sectnum].floorz)
                            goto zero_puke;
                    }
                    else goto zero_puke;
                }
                else goto zero_puke;
            }
            else
            {
            zero_puke:
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            if (sector[sectNum].lotag == ST_1_ABOVE_WATER)
            {
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            if (spriteNum >= 0 && (RR || pSprite->picnum != PUKE))
            {
                if (sprite[spriteNum].pal == 1)
                    pSprite->pal = 1;
                else if (sprite[spriteNum].pal != 6 && sprite[spriteNum].picnum != NUKEBARREL && sprite[spriteNum].picnum != TIRE)
                    pSprite->pal = (!RR && sprite[spriteNum].picnum == FECES) ? 7 : 2;  // Brown or red
                else
                    pSprite->pal = 0;  // green

                if (sprite[spriteNum].picnum == TIRE)
                    pSprite->shade = 127;
            }
            pSprite->cstat |= 32;
            if (RR) goto rrbloodpool_fallthrough;
            fallthrough__;
        }
        case FECES__STATIC:
            if (RR) goto default_case;
            if (spriteNum >= 0)
                pSprite->xrepeat = pSprite->yrepeat = 1;
            changespritestat(newSprite, STAT_MISC);
            break;

        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
rrbloodpool_fallthrough:
            pSprite->cstat |= 16;
            pSprite->xrepeat = 7 + (krand2() & 7);
            pSprite->yrepeat = 7 + (krand2() & 7);
            pSprite->z -= ZOFFSET2;

            if (pSprite->picnum == BLOODPOOL)
                pSprite->cstat |= 32768;

            if (spriteNum >= 0 && sprite[spriteNum].pal == 6)
                pSprite->pal = 6;

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case TRIPBOMB__STATIC:
            if (RR) goto default_case;
            if (pSprite->lotag > ud.player_skill)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->xrepeat = 4;
            pSprite->yrepeat = 5;
            pSprite->hitag   = newSprite;
            pSprite->owner   = pSprite->hitag;
            pSprite->xvel    = 16;

            A_SetSprite(newSprite, CLIPMASK0);

            pActor->t_data[0] = 17;
            pActor->t_data[2] = 0;
            pActor->t_data[5] = pSprite->ang;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case SPACEMARINE__STATIC:
            if (RR) goto default_case;
            pSprite->extra = 20;
            pSprite->cstat |= 257;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case PANNEL1__STATIC:
        case PANNEL2__STATIC:
        case OCEANSPRITE1__STATIC:
        case OCEANSPRITE2__STATIC:
        case OCEANSPRITE3__STATIC:
        case OCEANSPRITE5__STATIC:
        case MONK__STATIC:
        case INDY__STATIC:
        case LUKE__STATIC:
        case JURYGUY__STATIC:
        case HANGLIGHT__STATIC:
        case FETUS__STATIC:
        case FETUSBROKE__STATIC:
        case TRIPODCAMERA__STATIC:
            if (RR) goto default_case;
            fallthrough__;
        case HYDRENT__STATIC:
        case SATELITE__STATIC:
        case FUELPOD__STATIC:
        case SOLARPANNEL__STATIC:
        case ANTENNA__STATIC:
        case CHAIR1__STATIC:
        case CHAIR2__STATIC:
        case CHAIR3__STATIC:
        case BOTTLE1__STATIC:
        case BOTTLE2__STATIC:
        case BOTTLE3__STATIC:
        case BOTTLE4__STATIC:
        case BOTTLE5__STATIC:
        case BOTTLE6__STATIC:
        case BOTTLE7__STATIC:
        case BOTTLE8__STATIC:
        case BOTTLE10__STATIC:
        case BOTTLE11__STATIC:
        case BOTTLE12__STATIC:
        case BOTTLE13__STATIC:
        case BOTTLE14__STATIC:
        case BOTTLE15__STATIC:
        case BOTTLE16__STATIC:
        case BOTTLE17__STATIC:
        case BOTTLE18__STATIC:
        case BOTTLE19__STATIC:
        case SCALE__STATIC:
        case VACUUM__STATIC:
        case CACTUS__STATIC:
        case CACTUSBROKE__STATIC:
        case CAMERALIGHT__STATIC:
        case MOVIECAMERA__STATIC:
        case IVUNIT__STATIC:
        case POT1__STATIC:
        case POT2__STATIC:
        case POT3__STATIC:
        case SUSHIPLATE1__STATIC:
        case SUSHIPLATE2__STATIC:
        case SUSHIPLATE3__STATIC:
        case SUSHIPLATE4__STATIC:
        case SUSHIPLATE5__STATIC:
        case WAITTOBESEATED__STATIC:
        case VASE__STATIC:
        case PIPE1__STATIC:
        case PIPE2__STATIC:
        case PIPE3__STATIC:
        case PIPE4__STATIC:
        case PIPE5__STATIC:
        case PIPE6__STATIC:
        case GRATE1__STATIC:
        case FANSPRITE__STATIC:
            pSprite->clipdist = 32;
            pSprite->cstat |= 257;
            fallthrough__;
        case OCEANSPRITE4__STATIC:
            if (RR && pSprite->picnum == OCEANSPRITE4) goto default_case;
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
            pSprite->cstat &= ~257;
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        case DUKETAG__STATIC:
        case SIGN1__STATIC:
        case SIGN2__STATIC:
            if (RR) goto default_case;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else pSprite->pal = 0;
            break;

        case MASKWALL1__STATIC:
        case MASKWALL2__STATIC:
        case MASKWALL3__STATIC:
        case MASKWALL4__STATIC:
        case MASKWALL5__STATIC:
        case MASKWALL6__STATIC:
        case MASKWALL8__STATIC:
        case MASKWALL9__STATIC:
        case MASKWALL10__STATIC:
        case MASKWALL11__STATIC:
        case MASKWALL12__STATIC:
        case MASKWALL13__STATIC:
        case MASKWALL14__STATIC:
        case MASKWALL15__STATIC:
            if (RR) goto default_case;
            fallthrough__;
        case MASKWALL7__STATIC:
        {
            int const j    = pSprite->cstat & SPAWN_PROTECT_CSTAT_MASK;
            pSprite->cstat = j | CSTAT_SPRITE_BLOCK;
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        }
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (spriteNum >= 0)
            {
                int16_t footSect = pSprite->sectnum;

                updatesector(pSprite->x + 84, pSprite->y + 84, &footSect);
                if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                {
                    updatesector(pSprite->x - 84, pSprite->y - 84, &footSect);
                    if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                    {
                        updatesector(pSprite->x + 84, pSprite->y - 84, &footSect);
                        if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                        {
                            updatesector(pSprite->x - 84, pSprite->y + 84, &footSect);
                            if (footSect >= 0 && sector[footSect].floorz != sector[pSprite->sectnum].floorz)
                            {
                                pSprite->xrepeat = pSprite->yrepeat = 0;
                                changespritestat(newSprite, STAT_MISC);
                                break;
                            }
                        }
                        else goto zero_footprint;
                    }
                    else goto zero_footprint;
                }
                else
                {
                zero_footprint:
                    pSprite->xrepeat = pSprite->yrepeat = 0;
                    break;
                }

                pSprite->cstat = 32 + ((g_player[P_Get(spriteNum)].ps->footprintcount & 1) << 2);
                pSprite->ang   = sprite[spriteNum].ang;
            }

            pSprite->z = sector[sectNum].floorz;

            if (sector[sectNum].lotag != ST_1_ABOVE_WATER && sector[sectNum].lotag != ST_2_UNDERWATER)
                pSprite->xrepeat = pSprite->yrepeat = 32;

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case PODFEM1__STATIC:
            if (RR) goto default_case;
            pSprite->extra <<= 1;
            fallthrough__;
        case FEM1__STATIC:
        case FEM2__STATIC:
        case FEM3__STATIC:
        case FEM4__STATIC:
        case FEM5__STATIC:
        case FEM6__STATIC:
        case FEM7__STATIC:
        case FEM8__STATIC:
        case FEM9__STATIC:
            if (RR) goto default_case;
            fallthrough__;
        case FEM10__STATIC:
        case NAKED1__STATIC:
        case STATUE__STATIC:
        case TOUGHGAL__STATIC:
            pSprite->yvel  = pSprite->hitag;
            pSprite->hitag = -1;
            fallthrough__;
        case BLOODYPOLE__STATIC:
            if (RR && pSprite->picnum == BLOODYPOLE) goto default_case;
            pSprite->cstat   |= 257;
            pSprite->clipdist = 32;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            pSprite->cstat    = 256;
            pSprite->clipdist = 8;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;
        case BOWLINGBALL__STATICRR:
            pSprite->cstat = 256;
            pSprite->clipdist = 64;
            pSprite->xrepeat = 11;
            pSprite->yrepeat = 9;
            changespritestat(newSprite, 2);
            break;
        case HENSTAND__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 48;
            pSprite->xrepeat = 21;
            pSprite->yrepeat = 15;
            changespritestat(newSprite, 2);
            break;
        case RRTILE295__STATICRR:
            pSprite->cstat |= 32768;
            changespritestat(newSprite, 107);
            break;
        case RRTILE296__STATICRR:
        case RRTILE297__STATICRR:
            pSprite->xrepeat = 64;
            pSprite->yrepeat = 64;
            pSprite->clipdist = 64;
            changespritestat(newSprite, 108);
            break;
        case RRTILE3190__STATICRR:
        case RRTILE3191__STATICRR:
        case RRTILE3192__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 8;
            pSprite->xrepeat = 32;
            pSprite->yrepeat = 26;
            pSprite->xvel = 32;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3120__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 8;
            pSprite->xrepeat = 12;
            pSprite->yrepeat = 10;
            pSprite->xvel = 32;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3122__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 2;
            pSprite->xrepeat = 8;
            pSprite->yrepeat = 6;
            pSprite->xvel = 16;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3123__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 8;
            pSprite->xrepeat = 13;
            pSprite->yrepeat = 13;
            pSprite->xvel = 16;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3124__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 8;
            pSprite->xrepeat = 17;
            pSprite->yrepeat = 12;
            pSprite->xvel = 32;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3132__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 8;
            pSprite->xrepeat = 13;
            pSprite->yrepeat = 10;
            pSprite->xvel = 0;
            changespritestat(newSprite, 1);
            break;
        case RRTILE3440__STATICRR:
            pSprite->cstat = 257;
            pSprite->clipdist = 48;
            pSprite->xrepeat = 23;
            pSprite->yrepeat = 23;
            changespritestat(newSprite, 2);
            break;

        case DUKELYINGDEAD__STATIC:
            if (spriteNum >= 0 && sprite[spriteNum].picnum == APLAYER)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                pSprite->shade   = sprite[spriteNum].shade;
                pSprite->pal     = g_player[P_Get(spriteNum)].ps->palookup;
            }
            fallthrough__;
        case DUKECAR__STATIC:
        case HELECOPT__STATIC:
            //                if(sp->picnum == HELECOPT || sp->picnum == DUKECAR) sp->xvel = 1024;
            if (RR && (pSprite->picnum == DUKECAR || pSprite->picnum == HELECOPT)) goto default_case;
            pSprite->cstat = 0;
            pSprite->extra = 1;
            pSprite->xvel  = 292;
            pSprite->zvel  = 360;
            fallthrough__;
        case BLIMP__STATIC:
            if (RR && pSprite->picnum == BLIMP) goto default_case;
            pSprite->cstat   |= 257;
            pSprite->clipdist = 128;
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case RESPAWNMARKERRED__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = RR ? 8 : 24;
            if (spriteNum >= 0)
                pSprite->z = actor[spriteNum].floorz;  // -(1<<4);
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case MIKE__STATIC:
            pSprite->yvel  = pSprite->hitag;
            pSprite->hitag = 0;
            changespritestat(newSprite, STAT_ACTOR);
            break;
        case WEATHERWARN__STATIC:
            if (RR) goto default_case;
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case SPOTLITE__STATIC:
            T1(newSprite) = pSprite->x;
            T2(newSprite) = pSprite->y;
            break;
        case BULLETHOLE__STATIC:
            pSprite->xrepeat = 3;
            pSprite->yrepeat = 3;
            pSprite->cstat   = 16 + (krand2() & 12);

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case MONEY__STATIC:
        case MAIL__STATIC:
        case PAPER__STATIC:
            if (RR && (pSprite->picnum == MAIL || pSprite->picnum == PAPER)) goto default_case;
            pActor->t_data[0] = krand2() & 2047;

            pSprite->cstat   = krand2() & 12;
            pSprite->xrepeat = 8;
            pSprite->yrepeat = 8;
            pSprite->ang     = krand2() & 2047;

            changespritestat(newSprite, STAT_MISC);
            break;

        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            if (RR) goto default_case;
            pSprite->owner = newSprite;
            pSprite->lotag = pSprite->extra = 1;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case SHELL__STATIC: //From the player
        case SHOTGUNSHELL__STATIC:
            if (spriteNum >= 0)
            {
                int shellAng;

                if (sprite[spriteNum].picnum == APLAYER)
                {
                    int const                 playerNum = P_Get(spriteNum);
                    const DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

                    shellAng = fix16_to_int(pPlayer->q16ang) - (krand2() & 63) + 8;  // Fine tune

                    T1(newSprite) = krand2() & 1;

                    pSprite->z = (3 << 8) + pPlayer->pyoff + pPlayer->pos.z - (fix16_to_int((pPlayer->q16horizoff + pPlayer->q16horiz - F16(100))) << 4);

                    if (pSprite->picnum == SHOTGUNSHELL)
                        pSprite->z += (3 << 8);

                    pSprite->zvel = -(krand2() & 255);
                }
                else
                {
                    shellAng          = pSprite->ang;
                    pSprite->z = sprite[spriteNum].z - PHEIGHT + (RR ? (7 << 8) : (3 << 8));
                }

                pSprite->x     = sprite[spriteNum].x + (sintable[(shellAng + 512) & 2047] >> 7);
                pSprite->y     = sprite[spriteNum].y + (sintable[shellAng & 2047] >> 7);
                pSprite->shade = -8;

                if (NAM_WW2GI)
                {
                    pSprite->ang  = shellAng + 512;
                    pSprite->xvel = 30;
                }
                else
                {
                    pSprite->ang  = shellAng - 512;
                    pSprite->xvel = 20;
                }

                if (RR && pSprite->picnum == SHELL)
                    pSprite->xrepeat = pSprite->yrepeat = 2;
                else
                    pSprite->xrepeat = pSprite->yrepeat = 4;

                changespritestat(newSprite, STAT_MISC);
            }
            break;
        case RESPAWN__STATIC:
            pSprite->extra = 66-13;
            fallthrough__;
        case MUSICANDSFX__STATIC:
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->cstat = 32768;
            changespritestat(newSprite, STAT_FX);
            break;
        case SOUNDFX__STATICRR:
            pSprite->cstat |= 32768;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case EXPLOSION2__STATIC:
        case EXPLOSION3__STATICRR:
#ifdef POLYMER
            if (pSprite->yrepeat > 32)
            {
                G_AddGameLight(0, newSprite, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 32768, 255+(95<<8),PR_LIGHT_PRIO_MAX_GAME);
                pActor->lightcount = 2;
            }
            fallthrough__;
#endif
        case EXPLOSION2BOT__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
        case SMALLSMOKE__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case COOLEXPLOSION1__STATIC:
            if (RR && (pSprite->picnum == EXPLOSION2BOT || pSprite->picnum == BURNING2
                || pSprite->picnum == SHRINKEREXPLOSION || pSprite->picnum == COOLEXPLOSION1)) goto default_case;
            if (spriteNum >= 0)
            {
                pSprite->ang = sprite[spriteNum].ang;
                pSprite->shade = -64;
                pSprite->cstat = 128|(krand2()&4);
            }

            if (pSprite->picnum == EXPLOSION2 || (!RR && pSprite->picnum == EXPLOSION2BOT))
            {
                pSprite->xrepeat = pSprite->yrepeat = 48;
                pSprite->shade = -127;
                pSprite->cstat |= 128;
            }
            else if (RR && pSprite->picnum == EXPLOSION3)
            {
                pSprite->xrepeat = pSprite->yrepeat = 128;
                pSprite->shade = -127;
                pSprite->cstat |= 128;
            }
            else if (!RR && pSprite->picnum == SHRINKEREXPLOSION)
                pSprite->xrepeat = pSprite->yrepeat = 32;
            else if (pSprite->picnum == SMALLSMOKE)
            {
                // 64 "money"
                pSprite->xrepeat = pSprite->yrepeat = RR ? 12 : 24;
            }
            else if (pSprite->picnum == BURNING || (!RR && pSprite->picnum == BURNING2))
                pSprite->xrepeat = pSprite->yrepeat = 4;

            pSprite->cstat |= 8192;

            if (spriteNum >= 0)
            {
                int const floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);

                if (pSprite->z > floorZ-ZOFFSET4)
                    pSprite->z = floorZ-ZOFFSET4;
            }

            changespritestat(newSprite, STAT_MISC);

            break;

        case PLAYERONWATER__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                pSprite->zvel = 128;
                if (sector[pSprite->sectnum].lotag != ST_2_UNDERWATER)
                    pSprite->cstat |= 32768;
            }
            changespritestat(newSprite, STAT_DUMMYPLAYER);
            break;

        case APLAYER__STATIC:
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            pSprite->cstat   = 32768;

            changespritestat(newSprite, ((!g_netServer && ud.multimode < 2)
                                         || ((g_gametypeFlags[ud.coop] & GAMETYPE_COOPSPAWN) / GAMETYPE_COOPSPAWN) != pSprite->lotag)
                                        ? STAT_MISC
                                        : STAT_PLAYER);
            break;

        case WATERBUBBLE__STATIC:
            if (spriteNum >= 0)
            {
                if (sprite[spriteNum].picnum == APLAYER)
                    pSprite->z -= (16 << 8);

                pSprite->ang = sprite[spriteNum].ang;
            }

            pSprite->xrepeat = pSprite->yrepeat = RR ? (1+(krand2()&7)) : 4;
            changespritestat(newSprite, STAT_MISC);
            break;

        case CRANE__STATIC:

            pSprite->cstat |= 64|257;

            pSprite->picnum += 2;
            pSprite->z = sector[sectNum].ceilingz+(48<<8);
            T5(newSprite) = tempwallptr;

            g_origins[tempwallptr] = *(vec2_t *) pSprite;
            g_origins[tempwallptr+2].x = pSprite->z;


            if (headspritestat[STAT_DEFAULT] != -1)
            {
                int findSprite = headspritestat[STAT_DEFAULT];

                do
                {
                    if (sprite[findSprite].picnum == CRANEPOLE && pSprite->hitag == (sprite[findSprite].hitag))
                    {
                        g_origins[tempwallptr + 2].y = findSprite;

                        T2(newSprite) = sprite[findSprite].sectnum;

                        sprite[findSprite].xrepeat = 48;
                        sprite[findSprite].yrepeat = 128;

                        g_origins[tempwallptr + 1]     = *(vec2_t *) &sprite[findSprite];
                        *(vec3_t *) &sprite[findSprite] = *(vec3_t *) pSprite;
                        sprite[findSprite].shade       = pSprite->shade;

                        setsprite(findSprite, (vec3_t *) &sprite[findSprite]);
                        break;
                    }
                    findSprite = nextspritestat[findSprite];
                } while (findSprite >= 0);
            }

            tempwallptr += 3;
            pSprite->owner = -1;
            pSprite->extra = 8;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case TRASH__STATIC:
            pSprite->ang = krand2()&2047;
            pSprite->xrepeat = pSprite->yrepeat = 24;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case WATERDRIP__STATIC:
            if (spriteNum >= 0 && (sprite[spriteNum].statnum == STAT_PLAYER || sprite[spriteNum].statnum == STAT_ACTOR))
            {
                if (sprite[spriteNum].pal != 1)
                {
                    pSprite->pal = 2;
                    pSprite->z -= (18<<8);
                }
                else pSprite->z -= (13<<8);

                pSprite->shade = 32;
                pSprite->ang   = getangle(g_player[0].ps->pos.x - pSprite->x, g_player[0].ps->pos.y - pSprite->y);
                pSprite->xvel  = 48 - (krand2() & 31);

                A_SetSprite(newSprite, CLIPMASK0);
            }
            else if (spriteNum == -1)
            {
                pSprite->z += ZOFFSET6;
                T1(newSprite) = pSprite->z;
                if (!RR)
                    T2(newSprite) = krand2()&127;
            }
            fallthrough__;
        case WATERDRIPSPLASH__STATIC:
            if (RR && pSprite->picnum == WATERDRIPSPLASH) goto default_case;
            pSprite->xrepeat = pSprite->yrepeat = 24;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case PLUG__STATIC:
            pSprite->lotag = 9999;
            changespritestat(newSprite, STAT_STANDABLE);
            break;
        case TOUCHPLATE__STATIC:
            T3(newSprite) = sector[sectNum].floorz;

            if (sector[sectNum].lotag != ST_1_ABOVE_WATER && sector[sectNum].lotag != ST_2_UNDERWATER)
                sector[sectNum].floorz = pSprite->z;

            if (pSprite->pal && (g_netServer || ud.multimode > 1))
            {
                pSprite->xrepeat=pSprite->yrepeat=0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            fallthrough__;
        case WATERBUBBLEMAKER__STATIC:
            if (EDUKE32_PREDICT_FALSE(pSprite->hitag && pSprite->picnum == WATERBUBBLEMAKER))
            {
                // JBF 20030913: Pisses off X_Move(), eg. in bobsp2
                Printf(OSD_ERROR "WARNING: WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n",
                           newSprite,TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                pSprite->hitag = 0;
            }
            pSprite->cstat |= 32768;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case BOLT1__STATIC:
        case SIDEBOLT1__STATIC:
            if (RR && pSprite->picnum >= SIDEBOLT1 && pSprite->picnum <= SIDEBOLT1+3) goto default_case;
            T1(newSprite) = pSprite->xrepeat;
            T2(newSprite) = pSprite->yrepeat;
            pSprite->yvel = 0;

            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case MASTERSWITCH__STATIC:
            if (pSprite->picnum == MASTERSWITCH)
                pSprite->cstat |= 32768;
            pSprite->yvel = 0;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case TARGET__STATIC:
        case DUCK__STATIC:
        case LETTER__STATIC:
            if (RR) goto default_case;
            pSprite->extra = 1;
            pSprite->cstat |= 257;
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case OCTABRAINSTAYPUT__STATIC:
        case LIZTROOPSTAYPUT__STATIC:
        case PIGCOPSTAYPUT__STATIC:
        case LIZMANSTAYPUT__STATIC:
        case BOSS1STAYPUT__STATIC:
        case PIGCOPDIVE__STATIC:
        case COMMANDERSTAYPUT__STATIC:
        case BOSS4STAYPUT__STATIC:
            if (RR) goto default_case;
            pActor->actorstayput = pSprite->sectnum;
            fallthrough__;
        case BOSS1__STATIC:
        case BOSS2__STATIC:
        case BOSS3__STATIC:
        case BOSS4__STATIC:
        case ROTATEGUN__STATIC:
        case GREENSLIME__STATIC:
            if (RR) goto default_case;
            if (pSprite->picnum == GREENSLIME)
                pSprite->extra = 1;
            fallthrough__;
        case DRONE__STATIC:
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
        case OCTABRAIN__STATIC:
        case COMMANDER__STATIC:
        case PIGCOP__STATIC:
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
        case ORGANTIC__STATIC:
        case RAT__STATIC:
        case SHARK__STATIC:
            if (RR)
            {
                if (pSprite->picnum == RAT || pSprite->picnum == SHARK || pSprite->picnum == DRONE)
                    goto rr_badguy;
                goto default_case;
            }

            if (pSprite->pal == 0)
            {
                switch (DYNAMICTILEMAP(pSprite->picnum))
                {
                case LIZTROOPONTOILET__STATIC:
                case LIZTROOPSHOOT__STATIC:
                case LIZTROOPJETPACK__STATIC:
                case LIZTROOPDUCKING__STATIC:
                case LIZTROOPRUNNING__STATIC:
                case LIZTROOPSTAYPUT__STATIC:
                case LIZTROOPJUSTSIT__STATIC:
                case LIZTROOP__STATIC: pSprite->pal = 22; break;
                }
            }
            else
            {
                if (!PLUTOPAK)
                    pSprite->extra <<= 1;
            }

            if (pSprite->picnum == BOSS4STAYPUT || pSprite->picnum == BOSS1 || pSprite->picnum == BOSS2 ||
                pSprite->picnum == BOSS1STAYPUT || pSprite->picnum == BOSS3 || pSprite->picnum == BOSS4)
            {
                if (spriteNum >= 0 && sprite[spriteNum].picnum == RESPAWN)
                    pSprite->pal = sprite[spriteNum].pal;

                if (pSprite->pal)
                {
                    pSprite->clipdist = 80;
                    pSprite->xrepeat  = pSprite->yrepeat = 40;
                }
                else
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 80;
                    pSprite->clipdist = 164;
                }
            }
            else
            {
                if (pSprite->picnum != SHARK)
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 40;
                    pSprite->clipdist = 80;
                }
                else
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 60;
                    pSprite->clipdist = 40;
                }
            }

            // If spawned from parent sprite (as opposed to 'from premap'),
            // ignore skill.
            if (spriteNum >= 0)
                pSprite->lotag = 0;

            if ((pSprite->lotag > ud.player_skill) || ud.monsters_off == 1)
            {
                pSprite->xrepeat=pSprite->yrepeat=0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            else
            {
                A_Fall(newSprite);

                if (pSprite->picnum == RAT)
                {
                    pSprite->ang = krand2()&2047;
                    pSprite->xrepeat = pSprite->yrepeat = 48;
                    pSprite->cstat = 0;
                }
                else
                {
                    pSprite->cstat |= 257;

                    if (pSprite->picnum != SHARK)
                        g_player[myconnectindex].ps->max_actors_killed++;
                }

                if (pSprite->picnum == ORGANTIC) pSprite->cstat |= 128;

                if (spriteNum >= 0)
                {
                    pActor->timetosleep = 0;
                    A_PlayAlertSound(newSprite);
                    changespritestat(newSprite, STAT_ACTOR);
                }
                else changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }

            if (pSprite->picnum == ROTATEGUN)
                pSprite->zvel = 0;

            break;
        case BIKERB__STATICRR:
        case BIKERBV2__STATICRR:
        case BIKER__STATICRR:
        case MAKEOUT__STATICRR:
        case CHEERB__STATICRR:
        case CHEER__STATICRR:
        case COOTPLAY__STATICRR:
        case BILLYPLAY__STATICRR:
        case MINIONBOAT__STATICRR:
        case HULKBOAT__STATICRR:
        case CHEERBOAT__STATICRR:
        case RABBIT__STATICRR:
        case ROCK__STATICRR:
        case ROCK2__STATICRR:
        case MAMACLOUD__STATICRR:
        case MAMA__STATICRR:
            if (!RRRA) goto default_case;
            goto rr_badguy;
        case BILLYRAYSTAYPUT__STATICRR:
        case BRAYSNIPER__STATICRR:
        case BUBBASTAND__STATICRR:
        case HULKSTAYPUT__STATICRR:
        case HENSTAYPUT__STATICRR:
        case PIGSTAYPUT__STATICRR:
        case MINIONSTAYPUT__STATICRR:
        case COOTSTAYPUT__STATICRR:
        case SBSWIPE__STATICRR:
        case CHEERSTAYPUT__STATICRR:
        case SBMOVE__STATICRR:
            if ((RRRA && pSprite->picnum == SBMOVE) || (!RRRA && (pSprite->picnum == SBSWIPE || pSprite->picnum == CHEERSTAYPUT))) goto default_case;
            pActor->actorstayput = pSprite->sectnum;
            fallthrough__;
        case BOULDER__STATICRR:
        case BOULDER1__STATICRR:
        //case RAT__STATIC:
        case TORNADO__STATICRR:
        case BILLYCOCK__STATICRR:
        case BILLYRAY__STATICRR:
        case DOGRUN__STATICRR:
        case LTH__STATICRR:
        case HULK__STATICRR:
        case HEN__STATICRR:
        //case DRONE__STATIC:
        case PIG__STATICRR:
        case MINION__STATICRR:
        case UFO1__STATICRR:
        case UFO2__STATICRR:
        case UFO3__STATICRR:
        case UFO4__STATICRR:
        case UFO5__STATICRR:
        case COW__STATICRR:
        case COOT__STATICRR:
        //case SHARK__STATIC:
        case VIXEN__STATICRR:
rr_badguy:
            pSprite->xrepeat = 40;
            pSprite->yrepeat = 40;
            switch (DYNAMICTILEMAP(pSprite->picnum))
            {
            case VIXEN__STATICRR:
                if (pSprite->pal == 34)
                {
                    pSprite->xrepeat = 22;
                    pSprite->yrepeat = 21;
                }
                else
                {
                    pSprite->xrepeat = 22;
                    pSprite->yrepeat = 20;
                }
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case HULKHANG__STATICRR:
            case HULKHANGDEAD__STATICRR:
            case HULKJUMP__STATICRR:
            case HULK__STATICRR:
            case HULKSTAYPUT__STATICRR:
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case COOTPLAY__STATICRR:
                if (!RRRA) break;
                fallthrough__;
            case COOT__STATICRR:
            case COOTSTAYPUT__STATICRR:
                pSprite->xrepeat = 24;
                pSprite->yrepeat = 18;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                pSprite->clipdist <<= 2;
                break;
            case DRONE__STATIC:
                pSprite->xrepeat = 14;
                pSprite->yrepeat = 7;
                pSprite->clipdist = 128;
                break;
            case SBSWIPE__STATICRR:
            case BILLYPLAY__STATICRR:
                if (!RRRA) break;
                fallthrough__;
            case BILLYCOCK__STATICRR:
            case BILLYRAY__STATICRR:
            case BILLYRAYSTAYPUT__STATICRR:
            case BRAYSNIPER__STATICRR:
            case BUBBASTAND__STATICRR:
                pSprite->xrepeat = 25;
                pSprite->yrepeat = 21;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case COW__STATICRR:
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case HEN__STATICRR:
            case HENSTAYPUT__STATICRR:
            case HENSTAND__STATICRR:
                if (pSprite->pal == 35)
                {
                    pSprite->xrepeat = 42;
                    pSprite->yrepeat = 30;
                    pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                }
                else
                {
                    pSprite->xrepeat = 21;
                    pSprite->yrepeat = 15;
                    pSprite->clipdist = 64;
                }
                break;
            case MINION__STATICRR:
            case MINIONSTAYPUT__STATICRR:
                pSprite->xrepeat = 16;
                pSprite->yrepeat = 16;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                if (RRRA && g_ufoSpawnMinion)
                    pSprite->pal = 8;
                break;
            case DOGRUN__STATICRR:
            case PIG__STATICRR:
                pSprite->xrepeat = 16;
                pSprite->yrepeat = 16;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case RABBIT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 18;
                pSprite->yrepeat = 18;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case MAMACLOUD__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 64;
                pSprite->yrepeat = 64;
                pSprite->cstat = 2;
                pSprite->cstat |= 512;
                pSprite->x += (krand2() & 2047) - 1024;
                pSprite->y += (krand2() & 2047) - 1024;
                pSprite->z += (krand2() & 2047) - 1024;
                break;
            case MAMA__STATICRR:
                if (!RRRA) break;
                if (pSprite->pal == 30)
                {
                    pSprite->xrepeat = 26;
                    pSprite->yrepeat = 26;
                    pSprite->clipdist = 75;
                }
                else if (pSprite->pal == 31)
                {
                    pSprite->xrepeat = 36;
                    pSprite->yrepeat = 36;
                    pSprite->clipdist = 100;
                }
                else if (pSprite->pal == 32)
                {
                    pSprite->xrepeat = 50;
                    pSprite->yrepeat = 50;
                    pSprite->clipdist = 100;
                }
                else
                {
                    pSprite->xrepeat = 50;
                    pSprite->yrepeat = 50;
                    pSprite->clipdist = 100;
                }
                break;
            case BIKERB__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 28;
                pSprite->yrepeat = 22;
                pSprite->clipdist = 72;
                break;
            case BIKERBV2__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 28;
                pSprite->yrepeat = 22;
                pSprite->clipdist = 72;
                break;
            case BIKER__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 28;
                pSprite->yrepeat = 22;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case CHEERB__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 28;
                pSprite->yrepeat = 22;
                pSprite->clipdist = 72;
                break;
            case CHEER__STATICRR:
            case CHEERSTAYPUT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 20;
                pSprite->yrepeat = 20;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case MAKEOUT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 26;
                pSprite->yrepeat = 26;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case MINIONBOAT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 16;
                pSprite->yrepeat = 16;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case HULKBOAT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 48;
                pSprite->yrepeat = 48;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case CHEERBOAT__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case TORNADO__STATICRR:
                pSprite->xrepeat = 64;
                pSprite->yrepeat = 128;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                pSprite->clipdist >>= 2;
                pSprite->cstat = 2;
                break;
            case LTH__STATICRR:
                pSprite->xrepeat = 24;
                pSprite->yrepeat = 22;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case ROCK__STATICRR:
            case ROCK2__STATICRR:
                if (!RRRA) break;
                pSprite->xrepeat = 64;
                pSprite->yrepeat = 64;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            case UFO1__STATICRR:
            case UFO2__STATICRR:
            case UFO3__STATICRR:
            case UFO4__STATICRR:
            case UFO5__STATICRR:
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                pSprite->extra = 50;
                break;
            case SBMOVE__STATICRR:
                if (RRRA) break;
                pSprite->xrepeat = 48;
                pSprite->yrepeat = 48;
                pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
                break;
            }

            if (spriteNum >= 0) pSprite->lotag = 0;

            if ((pSprite->lotag > ud.player_skill) || ud.monsters_off == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            else
            {
                A_Fall(newSprite);

                if (pSprite->picnum == RAT)
                {
                    pSprite->ang = krand2() & 2047;
                    pSprite->xrepeat = pSprite->yrepeat = 48;
                    pSprite->cstat = 0;
                }
                else
                {
                    pSprite->cstat |= 257;

                    if (pSprite->picnum != SHARK)
                        if (!RR || A_CheckSpriteFlags(newSprite, SFLAG_KILLCOUNT))
                            g_player[myconnectindex].ps->max_actors_killed++;
                }

                if (spriteNum >= 0)
                {
                    pActor->timetosleep = 0;
                    A_PlayAlertSound(newSprite);
                    changespritestat(newSprite, STAT_ACTOR);
                }
                else changespritestat(newSprite, STAT_ZOMBIEACTOR);

                pSprite->shade = sprite[spriteNum].shade;
            }

            break;
        case LOCATORS__STATIC:
            pSprite->cstat |= 32768;
            changespritestat(newSprite, STAT_LOCATOR);
            break;

        case ACTIVATORLOCKED__STATIC:
        case ACTIVATOR__STATIC:
            if (RR)
            {
                pSprite->cstat |= 32768;
                if (pSprite->picnum == ACTIVATORLOCKED)
                    sector[pSprite->sectnum].lotag ^= 16384;
            }
            else
            {
                pSprite->cstat = 32768;
                if (pSprite->picnum == ACTIVATORLOCKED)
                    sector[pSprite->sectnum].lotag |= 16384;
            }
            changespritestat(newSprite, STAT_ACTIVATOR);
            break;

        case DOORSHOCK__STATIC:
            pSprite->cstat |= 1+256;
            pSprite->shade = -12;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case OOZ__STATIC:
        case OOZ2__STATIC:
        {
            if (RR && pSprite->picnum == OOZ2) goto default_case;
            pSprite->shade = -12;

            if (spriteNum >= 0)
            {
                if (sprite[spriteNum].picnum == NUKEBARREL)
                    pSprite->pal = 8;
                if (!RR)
                    A_AddToDeleteQueue(newSprite);
            }

            changespritestat(newSprite, STAT_ACTOR);

            A_GetZLimits(newSprite);

            int const oozSize = (pActor->floorz-pActor->ceilingz)>>9;

            pSprite->yrepeat = oozSize;
            pSprite->xrepeat = 25 - (oozSize >> 1);
            pSprite->cstat |= (krand2() & 4);

            break;
        }

        case REACTOR2__STATIC:
        case REACTOR__STATIC:
            pSprite->extra = g_impactDamage;
            pSprite->cstat |= 257;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case HEAVYHBOMB__STATIC:
            if (!RR && spriteNum >= 0)
                pSprite->owner = spriteNum;
            else pSprite->owner = newSprite;

            pSprite->xrepeat = pSprite->yrepeat = 9;
            pSprite->yvel = 4;
            pSprite->cstat |= 257;

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case RECON__STATIC:
            if (pSprite->lotag > ud.player_skill)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                goto SPAWN_END;
            }
            if (!RR || A_CheckSpriteFlags(newSprite, SFLAG_KILLCOUNT))
                g_player[myconnectindex].ps->max_actors_killed++;
            pActor->t_data[5] = 0;
            if (ud.monsters_off == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->extra = 130;
            pSprite->cstat |= 257; // Make it hitable

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case ATOMICHEALTH__STATIC:
        case STEROIDS__STATIC:
        case HEATSENSOR__STATIC:
        case SHIELD__STATIC:
        case AIRTANK__STATIC:
        case TRIPBOMBSPRITE__STATIC:
        case JETPACK__STATIC:
        case HOLODUKE__STATIC:

        case FIRSTGUNSPRITE__STATIC:
        case CHAINGUNSPRITE__STATIC:
        case SHOTGUNSPRITE__STATIC:
        case RPGSPRITE__STATIC:
        case SHRINKERSPRITE__STATIC:
        case FREEZESPRITE__STATIC:
        case DEVISTATORSPRITE__STATIC:

        case SHOTGUNAMMO__STATIC:
        case FREEZEAMMO__STATIC:
        case HBOMBAMMO__STATIC:
        case CRYSTALAMMO__STATIC:
        case GROWAMMO__STATIC:
        case BATTERYAMMO__STATIC:
        case DEVISTATORAMMO__STATIC:
        case RPGAMMO__STATIC:
        case BOOTS__STATIC:
        case AMMO__STATIC:
        case AMMOLOTS__STATIC:
        case COLA__STATIC:
        case FIRSTAID__STATIC:
        case SIXPAK__STATIC:
        case RRTILE43__STATICRR:
        case BOWLINGBALLSPRITE__STATICRR:
        case RPG2SPRITE__STATICRR:
        case MOTOAMMO__STATICRR:
        case BOATAMMO__STATICRR:

            if (RR && !RRRA && (pSprite->picnum == RPG2SPRITE || pSprite->picnum == MOTOAMMO || pSprite->picnum == BOATAMMO)) goto default_case;

            if (spriteNum >= 0)
            {
                pSprite->lotag = 0;
                if (RR && pSprite->picnum == BOWLINGBALLSPRITE)
                    pSprite->zvel = 0;
                else
                {
                    pSprite->z -= ZOFFSET5;
                    pSprite->zvel = -1024;
                }
                A_SetSprite(newSprite, CLIPMASK0);
                pSprite->cstat = krand2()&4;
            }
            else
            {
                pSprite->owner = newSprite;
                pSprite->cstat = 0;
            }

            if (((!g_netServer && ud.multimode < 2) && pSprite->pal != 0) || (pSprite->lotag > ud.player_skill))
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal = 0;

            if (pSprite->picnum == ATOMICHEALTH)
                pSprite->cstat |= 128;

            fallthrough__;
        case ACCESSCARD__STATIC:
            if ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_ACCESSCARDSPRITES) && pSprite->picnum == ACCESSCARD)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            else
            {
                if (pSprite->picnum == AMMO)
                    pSprite->xrepeat = pSprite->yrepeat = 16;
                else pSprite->xrepeat = pSprite->yrepeat = 32;
            }

            pSprite->shade = -17;

            if (spriteNum >= 0)
            {
                changespritestat(newSprite, STAT_ACTOR);
            }
            else
            {
                changespritestat(newSprite, STAT_ZOMBIEACTOR);
                A_Fall(newSprite);
            }
            if (RR)
            {
                switch (DYNAMICTILEMAP(pSprite->picnum))
                {
                case FIRSTGUNSPRITE__STATIC:
                    pSprite->xrepeat = 16;
                    pSprite->yrepeat = 16;
                    break;
                case SHOTGUNAMMO__STATIC:
                    pSprite->xrepeat = 18;
                    pSprite->yrepeat = 17;
                    if (RRRA)
                        pSprite->cstat = 256;
                    break;
                case SIXPAK__STATIC:
                    pSprite->xrepeat = 13;
                    pSprite->yrepeat = 9;
                    if (RRRA)
                        pSprite->cstat = 256;
                    break;
                case FIRSTAID__STATIC:
                    pSprite->xrepeat = 8;
                    pSprite->yrepeat = 8;
                    break;
                case COLA__STATIC:
                    pSprite->xrepeat = 5;
                    pSprite->yrepeat = 4;
                    break;
                case AMMO__STATIC:
                    pSprite->xrepeat = 9;
                    pSprite->yrepeat = 9;
                    break;
                case MOTOAMMO__STATICRR:
                    if (!RRRA) break;
                    pSprite->xrepeat = 23;
                    pSprite->yrepeat = 23;
                    break;
                case BOATAMMO__STATICRR:
                    if (!RRRA) break;
                    pSprite->xrepeat = 16;
                    pSprite->yrepeat = 16;
                    break;
                case JETPACK__STATIC:
                    pSprite->xrepeat = 8;
                    pSprite->yrepeat = 6;
                    break;
                case STEROIDS__STATIC:
                    pSprite->xrepeat = 13;
                    pSprite->yrepeat = 9;
                    break;
                case ACCESSCARD__STATIC:
                    pSprite->xrepeat = 11;
                    pSprite->yrepeat = 12;
                    break;
                case HEATSENSOR__STATIC:
                    pSprite->xrepeat = 6;
                    pSprite->yrepeat = 4;
                    break;
                case AIRTANK__STATIC:
                    pSprite->xrepeat = 19;
                    pSprite->yrepeat = 16;
                    break;
                case BATTERYAMMO__STATIC:
                    pSprite->xrepeat = 15;
                    pSprite->yrepeat = 15;
                    break;
                case BOWLINGBALLSPRITE__STATICRR:
                    pSprite->xrepeat = 11;
                    pSprite->yrepeat = 11;
                    break;
                case TRIPBOMBSPRITE__STATIC:
                    pSprite->xrepeat = 11;
                    pSprite->yrepeat = 11;
                    pSprite->yvel = 4;
                    pSprite->xvel = 32;
                    break;
                case RPGSPRITE__STATIC:
                    pSprite->xrepeat = 16;
                    pSprite->yrepeat = 14;
                    break;
                case RPG2SPRITE__STATICRR:
                    if (!RRRA) break;
                    pSprite->xrepeat = 20;
                    pSprite->yrepeat = 20;
                    break;
                case SHRINKERSPRITE__STATIC:
                    pSprite->xrepeat = 22;
                    pSprite->yrepeat = 13;
                    break;
                case DEVISTATORSPRITE__STATIC:
                    pSprite->xrepeat = 18;
                    pSprite->yrepeat = 17;
                    break;
                case RRTILE43__STATICRR:
                    pSprite->xrepeat = 12;
                    pSprite->yrepeat = 7;
                    break;
                case GROWSPRITEICON__STATIC:
                    pSprite->xrepeat = 10;
                    pSprite->yrepeat = 9;
                    break;
                case DEVISTATORAMMO__STATIC:
                    pSprite->xrepeat = 10;
                    pSprite->yrepeat = 9;
                    break;
                case ATOMICHEALTH__STATIC:
                    pSprite->xrepeat = 8;
                    pSprite->yrepeat = 8;
                    break;
                case FREEZESPRITE__STATIC:
                    pSprite->xrepeat = 17;
                    pSprite->yrepeat = 16;
                    break;
                }
                pSprite->shade = sector[pSprite->sectnum].floorshade;
            }
            break;

        case WATERFOUNTAIN__STATIC:
            SLT(newSprite) = 1;
            fallthrough__;
        case TREE1__STATIC:
        case TREE2__STATIC:
        case TIRE__STATIC:
        case CONE__STATIC:
        case BOX__STATIC:
            if (RR && (pSprite->picnum == CONE || pSprite->picnum == BOX)) goto default_case;
            pSprite->cstat = 257; // Make it hitable
            sprite[newSprite].extra = 1;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case FLOORFLAME__STATIC:
            if (RR) goto default_case;
            pSprite->shade = -127;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case BOUNCEMINE__STATIC:
            if (RR) goto default_case;
            pSprite->owner = newSprite;
            pSprite->cstat |= 1+256; //Make it hitable
            pSprite->xrepeat = pSprite->yrepeat = 24;
            pSprite->shade = -127;
            pSprite->extra = g_impactDamage<<2;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;
        case CAMERA1__STATIC:
            pSprite->extra = 1;
            pSprite->cstat &= 32768;

            if (g_damageCameras)
                pSprite->cstat |= 257;

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else
            {
                pSprite->pal = 0;
                changespritestat(newSprite, STAT_ACTOR);
            }
            break;
        case CAMERAPOLE__STATIC:
            pSprite->extra = 1;
            pSprite->cstat &= 32768;

            if (g_damageCameras)
                pSprite->cstat |= 257;
            fallthrough__;
        case GENERICPOLE__STATIC:
            if (RR && pSprite->picnum == GENERICPOLE) goto default_case;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else
                pSprite->pal = 0;
            break;

        case STEAM__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->ang = sprite[spriteNum].ang;
                pSprite->cstat = 16+128+2;
                pSprite->xrepeat=pSprite->yrepeat=1;
                pSprite->xvel = -8;
                A_SetSprite(newSprite, CLIPMASK0);
            }
            fallthrough__;
        case CEILINGSTEAM__STATIC:
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case SECTOREFFECTOR__STATIC:
            pSprite->cstat |= 32768;
            pSprite->xrepeat = pSprite->yrepeat = 0;

            switch (pSprite->lotag)
            {
#ifdef LEGACY_ROR
            case 40:
            case 41:
                if (RR) break;
                pSprite->cstat = 32;
                pSprite->xrepeat = pSprite->yrepeat = 64;
                changespritestat(newSprite, STAT_EFFECTOR);
                for (spriteNum=0; spriteNum < MAXSPRITES; spriteNum++)
                    if (sprite[spriteNum].picnum == SECTOREFFECTOR && (sprite[spriteNum].lotag == 40 || sprite[spriteNum].lotag == 41) &&
                            sprite[spriteNum].hitag == pSprite->hitag && newSprite != spriteNum)
                    {
//                        Printf("found ror match\n");
                        pSprite->yvel = spriteNum;
                        break;
                    }
                goto SPAWN_END;
                break;
            case 46:
                if (RR) break;
                ror_protectedsectors[pSprite->sectnum] = 1;
                /* XXX: fall-through intended? */
                fallthrough__;
#endif
            case SE_49_POINT_LIGHT:
            case SE_50_SPOT_LIGHT:
            {
                int32_t j, nextj;

                for (TRAVERSE_SPRITE_SECT(headspritesect[pSprite->sectnum], j, nextj))
                    if (sprite[j].picnum == ACTIVATOR || sprite[j].picnum == ACTIVATORLOCKED)
                        pActor->flags |= SFLAG_USEACTIVATOR;
            }
            changespritestat(newSprite, pSprite->lotag==46 ? STAT_EFFECTOR : STAT_LIGHT);
            goto SPAWN_END;
            break;
            }

            pSprite->yvel = sector[sectNum].extra;

            switch (pSprite->lotag)
            {
            case SE_28_LIGHTNING:
                if (!RR)
                    T6(newSprite) = 65;// Delay for lightning
                break;
            case SE_7_TELEPORT: // Transporters!!!!
            case SE_23_ONE_WAY_TELEPORT:// XPTR END
                if (pSprite->lotag != SE_23_ONE_WAY_TELEPORT)
                {
                    for (spriteNum=0; spriteNum<MAXSPRITES; spriteNum++)
                        if (sprite[spriteNum].statnum < MAXSTATUS && sprite[spriteNum].picnum == SECTOREFFECTOR &&
                                (sprite[spriteNum].lotag == SE_7_TELEPORT || sprite[spriteNum].lotag == SE_23_ONE_WAY_TELEPORT) && newSprite != spriteNum && sprite[spriteNum].hitag == SHT(newSprite))
                        {
                            OW(newSprite) = spriteNum;
                            break;
                        }
                }
                else OW(newSprite) = newSprite;

                T5(newSprite) = (sector[sectNum].floorz == SZ(newSprite));  // ONFLOORZ
                pSprite->cstat = 0;
                changespritestat(newSprite, STAT_TRANSPORT);
                goto SPAWN_END;
            case SE_1_PIVOT:
                pSprite->owner = -1;
                T1(newSprite) = 1;
                break;
            case SE_18_INCREMENTAL_SECTOR_RISE_FALL:

                if (pSprite->ang == 512)
                {
                    T2(newSprite) = sector[sectNum].ceilingz;
                    if (pSprite->pal)
                        sector[sectNum].ceilingz = pSprite->z;
                }
                else
                {
                    T2(newSprite) = sector[sectNum].floorz;
                    if (pSprite->pal)
                        sector[sectNum].floorz = pSprite->z;
                }

                pSprite->hitag <<= 2;
                break;

            case SE_19_EXPLOSION_LOWERS_CEILING:
                pSprite->owner = -1;
                break;
            case SE_25_PISTON: // Pistons
                if (RR)
                    T5(newSprite) = sector[sectNum].ceilingz;
                else
                {
                    T4(newSprite) = sector[sectNum].ceilingz;
                    T5(newSprite) = 1;
                }
                sector[sectNum].ceilingz = pSprite->z;
                G_SetInterpolation(&sector[sectNum].ceilingz);
                break;
            case SE_35:
                sector[sectNum].ceilingz = pSprite->z;
                break;
            case SE_27_DEMO_CAM:
                if (ud.recstat == 1)
                {
                    pSprite->xrepeat=pSprite->yrepeat=64;
                    pSprite->cstat &= 32768;
                }
                break;
            case 47:
            case 48:
                if (!RRRA) break;
                fallthrough__;
            case SE_12_LIGHT_SWITCH:

                T2(newSprite) = sector[sectNum].floorshade;
                T3(newSprite) = sector[sectNum].ceilingshade;
                break;

            case SE_13_EXPLOSIVE:

                T1(newSprite) = sector[sectNum].ceilingz;
                T2(newSprite) = sector[sectNum].floorz;

                if (klabs(T1(newSprite)-pSprite->z) < klabs(T2(newSprite)-pSprite->z))
                    pSprite->owner = 1;
                else pSprite->owner = 0;

                if (pSprite->ang == 512)
                {
                    if (pSprite->owner)
                        sector[sectNum].ceilingz = pSprite->z;
                    else
                        sector[sectNum].floorz = pSprite->z;
#ifdef YAX_ENABLE
                    {
                        int16_t cf=!pSprite->owner, bn=yax_getbunch(sectNum, cf);
                        int32_t jj, daz=SECTORFLD(sectNum,z, cf);

                        if (bn >= 0)
                        {
                            for (SECTORS_OF_BUNCH(bn, cf, jj))
                            {
                                SECTORFLD(jj,z, cf) = daz;
                                SECTORFLD(jj,stat, cf) &= ~256;
                                SECTORFLD(jj,stat, cf) |= 128 + 512+2048;
                            }
                            for (SECTORS_OF_BUNCH(bn, !cf, jj))
                            {
                                SECTORFLD(jj,z, !cf) = daz;
                                SECTORFLD(jj,stat, !cf) &= ~256;
                                SECTORFLD(jj,stat, !cf) |= 128 + 512+2048;
                            }
                        }
                    }
#endif
                }
                else
                    sector[sectNum].ceilingz = sector[sectNum].floorz = pSprite->z;

                if (sector[sectNum].ceilingstat&1)
                {
                    sector[sectNum].ceilingstat ^= 1;
                    T4(newSprite) = 1;

                    if (!pSprite->owner && pSprite->ang==512)
                    {
                        sector[sectNum].ceilingstat ^= 1;
                        T4(newSprite) = 0;
                    }

                    sector[sectNum].ceilingshade =
                        sector[sectNum].floorshade;

                    if (pSprite->ang==512)
                    {
                        int const startwall = sector[sectNum].wallptr;
                        int const endwall   = startwall + sector[sectNum].wallnum;
                        for (bssize_t j = startwall; j < endwall; j++)
                        {
                            int const nextSect = wall[j].nextsector;

                            if (nextSect >= 0)
                            {
                                if (!(sector[nextSect].ceilingstat & 1))
                                {
                                    sector[sectNum].ceilingpicnum = sector[nextSect].ceilingpicnum;
                                    sector[sectNum].ceilingshade  = sector[nextSect].ceilingshade;
                                    break;  // Leave earily
                                }
                            }
                        }
                    }
                }

                break;

            case SE_17_WARP_ELEVATOR:
            {
                T3(newSprite) = sector[sectNum].floorz;  // Stopping loc

                int nextSectNum = nextsectorneighborz(sectNum, sector[sectNum].floorz, -1, -1);

                if (EDUKE32_PREDICT_TRUE(nextSectNum >= 0))
                    T4(newSprite) = sector[nextSectNum].ceilingz;
                else
                {
                    // use elevator sector's ceiling as heuristic
                    T4(newSprite) = sector[sectNum].ceilingz;

                    Printf(OSD_ERROR "WARNING: SE17 sprite %d using own sector's ceilingz to "
                                         "determine when to warp. Sector %d adjacent to a door?\n",
                               newSprite, sectNum);
                }

                nextSectNum = nextsectorneighborz(sectNum, sector[sectNum].ceilingz, 1, 1);

                if (EDUKE32_PREDICT_TRUE(nextSectNum >= 0))
                    T5(newSprite) = sector[nextSectNum].floorz;
                else
                {
                    // heuristic
                    T5(newSprite) = sector[sectNum].floorz;

                    Printf(OSD_ERROR "WARNING: SE17 sprite %d using own sector %d's floorz.\n",
                               newSprite, sectNum);
                }

                if (numplayers < 2 && !g_netServer)
                {
                    G_SetInterpolation(&sector[sectNum].floorz);
                    G_SetInterpolation(&sector[sectNum].ceilingz);
                }
            }
            break;

            case SE_24_CONVEYOR:
                pSprite->yvel <<= 1;
            case SE_36_PROJ_SHOOTER:
                break;

            case SE_20_STRETCH_BRIDGE:
            {
                int       closestDist = INT32_MAX;
                int       closestWall = 0;
                int const startWall   = sector[sectNum].wallptr;
                int const endWall     = startWall + sector[sectNum].wallnum;

                for (bssize_t findWall=startWall; findWall<endWall; findWall++)
                {
                    int const x = wall[findWall].x;
                    int const y = wall[findWall].y;
                    int const d = FindDistance2D(pSprite->x - x, pSprite->y - y);

                    if (d < closestDist)
                    {
                        closestDist = d;
                        closestWall = findWall;
                    }
                }

                T2(newSprite) = closestWall;

                closestDist = INT32_MAX;

                for (bssize_t findWall=startWall; findWall<endWall; findWall++)
                {
                    int const x = wall[findWall].x;
                    int const y = wall[findWall].y;
                    int const d = FindDistance2D(pSprite->x - x, pSprite->y - y);

                    if (d < closestDist && findWall != T2(newSprite))
                    {
                        closestDist = d;
                        closestWall = findWall;
                    }
                }

                T3(newSprite) = closestWall;
            }

            break;

            case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
            {

                T4(newSprite)=sector[sectNum].floorshade;

                sector[sectNum].floorshade   = pSprite->shade;
                sector[sectNum].ceilingshade = pSprite->shade;

                pSprite->owner = sector[sectNum].ceilingpal << 8;
                pSprite->owner |= sector[sectNum].floorpal;

                //fix all the walls;

                int const startWall = sector[sectNum].wallptr;
                int const endWall = startWall+sector[sectNum].wallnum;

                for (bssize_t w=startWall; w<endWall; ++w)
                {
                    if (!(wall[w].hitag & 1))
                        wall[w].shade = pSprite->shade;

                    if ((wall[w].cstat & 2) && wall[w].nextwall >= 0)
                        wall[wall[w].nextwall].shade = pSprite->shade;
                }
                break;
            }

            case SE_31_FLOOR_RISE_FALL:
            {
                T2(newSprite) = sector[sectNum].floorz;

                if (pSprite->ang != 1536)
                {
                    sector[sectNum].floorz = pSprite->z;
                    Yax_SetBunchZs(sectNum, YAX_FLOOR, pSprite->z);
                }

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].hitag == 0)
                        wall[w].hitag = 9999;

                G_SetInterpolation(&sector[sectNum].floorz);
                Yax_SetBunchInterpolation(sectNum, YAX_FLOOR);
            }
            break;

            case SE_32_CEILING_RISE_FALL:
            {
                T2(newSprite) = sector[sectNum].ceilingz;
                T3(newSprite) = pSprite->hitag;

                if (pSprite->ang != 1536)
                {
                    sector[sectNum].ceilingz = pSprite->z;
                    Yax_SetBunchZs(sectNum, YAX_CEILING, pSprite->z);
                }

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].hitag == 0)
                        wall[w].hitag = 9999;

                G_SetInterpolation(&sector[sectNum].ceilingz);
                Yax_SetBunchInterpolation(sectNum, YAX_CEILING);
            }
            break;

            case SE_4_RANDOM_LIGHTS: //Flashing lights
            {
                T3(newSprite) = sector[sectNum].floorshade;

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                pSprite->owner = sector[sectNum].ceilingpal << 8;
                pSprite->owner |= sector[sectNum].floorpal;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].shade > T4(newSprite))
                        T4(newSprite) = wall[w].shade;
            }
            break;

            case SE_9_DOWN_OPEN_DOOR_LIGHTS:
                if (sector[sectNum].lotag &&
                        labs(sector[sectNum].ceilingz-pSprite->z) > 1024)
                    sector[sectNum].lotag |= 32768u; //If its open
                fallthrough__;
            case SE_8_UP_OPEN_DOOR_LIGHTS:
                //First, get the ceiling-floor shade
                {
                    T1(newSprite) = sector[sectNum].floorshade;
                    T2(newSprite) = sector[sectNum].ceilingshade;

                    int const startWall = sector[sectNum].wallptr;
                    int const endWall   = startWall + sector[sectNum].wallnum;

                    for (bssize_t w = startWall; w < endWall; ++w)
                        if (wall[w].shade > T3(newSprite))
                            T3(newSprite) = wall[w].shade;

                    T4(newSprite) = 1;  // Take Out;
                }
                break;
            case 88:
                //First, get the ceiling-floor shade
                {
                    if (!RRRA) break;
                    T1(newSprite) = sector[sectNum].floorshade;
                    T2(newSprite) = sector[sectNum].ceilingshade;

                    int const startWall = sector[sectNum].wallptr;
                    int const endWall   = startWall + sector[sectNum].wallnum;

                    for (bssize_t w = startWall; w < endWall; ++w)
                        if (wall[w].shade > T3(newSprite))
                            T3(newSprite) = wall[w].shade;

                    T4(newSprite) = 1;  // Take Out;
                }
                break;

            case SE_11_SWINGING_DOOR:  // Pivitor rotater
                T4(newSprite) = (pSprite->ang > 1024) ? 2 : -2;
                fallthrough__;
            case SE_0_ROTATING_SECTOR:
            case SE_2_EARTHQUAKE:      // Earthquakemakers
            case SE_5:                 // Boss Creature
            case SE_6_SUBWAY:          // Subway
            case SE_14_SUBWAY_CAR:     // Caboos
            case SE_15_SLIDING_DOOR:   // Subwaytype sliding door
            case SE_16_REACTOR:        // That rotating blocker reactor thing
            case SE_26:                // ESCELATOR
            case SE_30_TWO_WAY_TRAIN:  // No rotational subways
                if (pSprite->lotag == SE_0_ROTATING_SECTOR)
                {
                    if (sector[sectNum].lotag == ST_30_ROTATE_RISE_BRIDGE)
                    {
                        sprite[newSprite].clipdist = (pSprite->pal) ? 1 : 0;
                        T4(newSprite) = sector[sectNum].floorz;
                        sector[sectNum].hitag = newSprite;
                    }

                    for (spriteNum = MAXSPRITES-1; spriteNum>=0; spriteNum--)
                    {
                        if (sprite[spriteNum].statnum < MAXSTATUS)
                            if (sprite[spriteNum].picnum == SECTOREFFECTOR &&
                                    sprite[spriteNum].lotag == SE_1_PIVOT &&
                                    sprite[spriteNum].hitag == pSprite->hitag)
                            {
                                if (pSprite->ang == 512)
                                {
                                    pSprite->x = sprite[spriteNum].x;
                                    pSprite->y = sprite[spriteNum].y;
                                }
                                break;
                            }
                    }
                    if (EDUKE32_PREDICT_FALSE(spriteNum == -1))
                    {
                        Printf(OSD_ERROR "Found lonely Sector Effector (lotag 0) at (%d,%d)\n",
                            TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                        changespritestat(newSprite, STAT_ACTOR);
                        goto SPAWN_END;
                    }
                    pSprite->owner = spriteNum;
                }

                {
                    int const startWall = sector[sectNum].wallptr;
                    int const endWall = startWall+sector[sectNum].wallnum;

                    T2(newSprite) = tempwallptr;
                    for (bssize_t w = startWall; w < endWall; ++w)
                    {
                        g_origins[tempwallptr].x = wall[w].x - pSprite->x;
                        g_origins[tempwallptr].y = wall[w].y - pSprite->y;

                        tempwallptr++;
                        if (EDUKE32_PREDICT_FALSE(tempwallptr >= MAXANIMPOINTS))
                        {
                            Bsprintf(tempbuf, "Too many moving sectors at (%d,%d).\n",
                                TrackerCast(wall[w].x), TrackerCast(wall[w].y));
                            G_GameExit(tempbuf);
                        }
                    }
                }

                if (pSprite->lotag == SE_5 || pSprite->lotag == SE_30_TWO_WAY_TRAIN ||
                        pSprite->lotag == SE_6_SUBWAY || pSprite->lotag == SE_14_SUBWAY_CAR)
                {
#ifdef YAX_ENABLE
                    int outerWall = -1;
#endif
                    int const startWall = sector[sectNum].wallptr;
                    int const endWall   = startWall + sector[sectNum].wallnum;

                    pSprite->extra = (uint16_t)((uint16_t)sector[sectNum].hitag != UINT16_MAX);

                    // TRAIN_SECTOR_TO_SE_INDEX
                    sector[sectNum].hitag = newSprite;

                    spriteNum = 0;

                    int foundWall = startWall;

                    for (; foundWall<endWall; foundWall++)
                    {
                        if (wall[ foundWall ].nextsector >= 0 &&
                                sector[ wall[ foundWall ].nextsector].hitag == 0 &&
                                ((int16_t)sector[ wall[ foundWall ].nextsector].lotag < 3 || (RRRA && (int16_t)sector[wall[foundWall].nextsector].lotag == 160)))
                        {
#ifdef YAX_ENABLE
                            outerWall = wall[foundWall].nextwall;
#endif
                            foundWall = wall[foundWall].nextsector;
                            spriteNum = 1;
                            break;
                        }
                    }

#ifdef YAX_ENABLE
                    pActor->t_data[9] = -1;

                    if (outerWall >= 0)
                    {
                        int upperSect = yax_vnextsec(outerWall, YAX_CEILING);

                        if (upperSect >= 0)
                        {
                            int foundEffector = headspritesect[upperSect];

                            for (; foundEffector >= 0; foundEffector = nextspritesect[foundEffector])
                                if (sprite[foundEffector].picnum == SECTOREFFECTOR && sprite[foundEffector].lotag == pSprite->lotag)
                                    break;

                            if (foundEffector < 0)
                            {
                                Sect_SetInterpolation(upperSect);
                                pActor->t_data[9] = upperSect;
                            }
                        }
                    }
#endif
                    if (spriteNum == 0)
                    {
                        Bsprintf(tempbuf,"Subway found no zero'd sectors with locators\nat (%d,%d).\n",
                            TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                        G_GameExit(tempbuf);
                    }

                    pSprite->owner = -1;
                    T1(newSprite) = foundWall;

                    if (pSprite->lotag != SE_30_TWO_WAY_TRAIN)
                        T4(newSprite) = pSprite->hitag;
                }
                else if (pSprite->lotag == SE_16_REACTOR)
                    T4(newSprite) = sector[sectNum].ceilingz;
                else if (pSprite->lotag == SE_26)
                {
                    T4(newSprite)  = pSprite->x;
                    T5(newSprite)  = pSprite->y;
                    pSprite->zvel  = (pSprite->shade == sector[sectNum].floorshade) ? -256 : 256;  // UP
                    pSprite->shade = 0;
                }
                else if (pSprite->lotag == SE_2_EARTHQUAKE)
                {
                    T6(newSprite) = sector[pSprite->sectnum].floorheinum;
                    sector[pSprite->sectnum].floorheinum = 0;
                }
            }

            switch (pSprite->lotag)
            {
                case SE_6_SUBWAY:
                case SE_14_SUBWAY_CAR:
                    S_FindMusicSFX(sectNum, &spriteNum);
                    // XXX: uh.. what?
                    if (spriteNum == -1)
                    {
                        if (RR && sector[pSprite->sectnum].floorpal == 7)
                            spriteNum = 456;
                        else
                            spriteNum = SUBWAY;
                    }
                    pActor->lastv.x = spriteNum;
                    fallthrough__;
                case SE_30_TWO_WAY_TRAIN:
                    if (g_netServer || numplayers > 1)
                        break;
                    fallthrough__;
                case SE_0_ROTATING_SECTOR:
                case SE_1_PIVOT:
                case SE_5:
                case SE_11_SWINGING_DOOR:
                case SE_15_SLIDING_DOOR:
                case SE_16_REACTOR:
                case SE_26: Sect_SetInterpolation(sprite[newSprite].sectnum); break;
            }

            if (RRRA && sprite[newSprite].lotag >= 150 && sprite[newSprite].lotag <= 155)
                changespritestat(newSprite, STAT_RAROR);
            else
                changespritestat(newSprite, STAT_EFFECTOR);
            break;

        case SEENINE__STATIC:
        case OOZFILTER__STATIC:
            pSprite->shade = -16;
            if (pSprite->xrepeat <= 8)
            {
                pSprite->cstat   = 32768;
                pSprite->xrepeat = 0;
                pSprite->yrepeat = 0;
            }
            else pSprite->cstat = 1+256;

            pSprite->extra = g_impactDamage << 2;
            pSprite->owner = newSprite;

            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case CRACK1__STATIC:
        case CRACK2__STATIC:
        case CRACK3__STATIC:
        case CRACK4__STATIC:
        case FIREEXT__STATIC:
            if (RR && pSprite->picnum == FIREEXT) goto default_case;
            if (!RR && pSprite->picnum == FIREEXT)
            {
                pSprite->cstat = 257;
                pSprite->extra = g_impactDamage<<2;
            }
            else
            {
                pSprite->cstat |= (pSprite->cstat & 48) ? 1 : 17;
                pSprite->extra = 1;
            }

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal   = 0;
            pSprite->owner = newSprite;
            pSprite->xvel  = 8;

            changespritestat(newSprite, STAT_STANDABLE);
            A_SetSprite(newSprite,CLIPMASK0);
            break;

        case EMPTYBIKE__STATICRR:
            if (!RRRA) goto default_case;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                break;
            }
            pSprite->pal = 0;
            pSprite->xrepeat = 18;
            pSprite->yrepeat = 18;
            pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
            pSprite->owner = 100;
            pSprite->cstat = 257;
            pSprite->lotag = 1;
            changespritestat(newSprite, 1);
            break;
        case EMPTYBOAT__STATICRR:
            if (!RRRA) goto default_case;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                break;
            }
            pSprite->pal = 0;
            pSprite->xrepeat = 32;
            pSprite->yrepeat = 32;
            pSprite->clipdist = mulscale7(pSprite->xrepeat, tilesiz[pSprite->picnum].x);
            pSprite->owner = 20;
            pSprite->cstat = 257;
            pSprite->lotag = 1;
            changespritestat(newSprite, 1);
            break;

        case TOILET__STATIC:
        case STALL__STATIC:
        case RRTILE2121__STATICRR:
        case RRTILE2122__STATICRR:
            pSprite->lotag = 1;
            pSprite->cstat |= 257;
            pSprite->clipdist = 8;
            pSprite->owner = newSprite;
            break;

        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
            if (RR) goto default_case;
            fallthrough__;
        case CANWITHSOMETHING__STATIC:
        case RUBBERCAN__STATIC:
            pSprite->extra = 0;
            fallthrough__;
        case EXPLODINGBARREL__STATIC:
        case HORSEONSIDE__STATIC:
        case FIREBARREL__STATIC:
        case NUKEBARREL__STATIC:
        case FIREVASE__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case WOODENHORSE__STATIC:
            if (spriteNum >= 0)
                pSprite->xrepeat = pSprite->yrepeat = 32;
            pSprite->clipdist = 72;
            A_Fall(newSprite);
            if (spriteNum >= 0)
                pSprite->owner = spriteNum;
            else pSprite->owner = newSprite;
            fallthrough__;
        case EGG__STATIC:
            if (ud.monsters_off == 1 && pSprite->picnum == EGG)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else
            {
                if (pSprite->picnum == EGG)
                    pSprite->clipdist = 24;
                pSprite->cstat = 257|(krand2()&4);
                changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }
            break;

        case TOILETWATER__STATIC:
            pSprite->shade = -16;
            changespritestat(newSprite, STAT_STANDABLE);
            break;
        case RRTILE63__STATICRR:
            pSprite->cstat |= 32768;
            pSprite->xrepeat = 1;
            pSprite->yrepeat = 1;
            pSprite->clipdist = 1;
            changespritestat(newSprite, 100);
            break;
        }

SPAWN_END:
    return newSprite;
}

static int G_MaybeTakeOnFloorPal(tspritetype *pSprite, int sectNum)
{
    int const floorPal = sector[sectNum].floorpal;

    if (floorPal && !g_noFloorPal[floorPal] && !A_CheckSpriteFlags(pSprite->owner, SFLAG_NOPAL))
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

        if (t->picnum < GREENSLIME || t->picnum > GREENSLIME+7)
            switch (DYNAMICTILEMAP(t->picnum))
            {
            case PIG__STATICRR:
            case DOGRUN__STATICRR:
            case VIXEN__STATICRR:
            case CHEER__STATICRR:
                if (DEER)
                {
                    if ((t->cstat & 32768) == 0 && (t->cstat & 257) != 0)
                    {
                        if (klabs(ourx - t->x) + klabs(oury - t->y) < 46080)
                        {
                            if (klabs(oura - (getangle(t->x - ourx, t->y - oury) & 2047)) < 128)
                                sub_5A250(32);
                            else
                                sub_5A250(64);
                        }
                    }
                }
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
                    if (RR && g_shadedSector[s->sectnum] == 1)
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

        if (adult_lockout && G_CheckAdultTile(DYNAMICTILEMAP(pSprite->picnum)))
        {
            t->xrepeat = t->yrepeat = 0;
            continue;
        }

        if (!RR && pSprite->picnum == NATURALLIGHTNING)
        {
            t->shade = -127;
            t->clipdist |= TSPR_FLAGS_NO_SHADOW;
        }

        if (t->statnum == TSPR_TEMP)
            continue;

        Bassert(i >= 0);

        const DukePlayer_t *const ps = (pSprite->statnum != STAT_ACTOR && pSprite->picnum == APLAYER && pSprite->owner >= 0) ? g_player[P_GetP(pSprite)].ps : NULL;
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
        else if ((pSprite->statnum == STAT_DEFAULT && pSprite->picnum != CRANEPOLE) || pSprite->statnum == STAT_PLAYER ||
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
        if ((pSprite->picnum >= SCRAP6) && (pSprite->picnum<=SCRAP6+7))
            switchpic = SCRAP5;
        else if ((pSprite->picnum==MONEY+1) || (pSprite->picnum==MAIL+1) || (pSprite->picnum==PAPER+1))
            switchpic--;

        switch (DYNAMICTILEMAP(switchpic))
        {
        case RESPAWNMARKERRED__STATICRR:
        case RESPAWNMARKERYELLOW__STATICRR:
        case RESPAWNMARKERGREEN__STATICRR:
            if (!RR) goto default_case2;
            t->picnum = 861 + (((int32_t) totalclock >> 4) & 13);
            if (pSprite->picnum == RESPAWNMARKERRED)
                t->pal = 0;
            else if (pSprite->picnum == RESPAWNMARKERYELLOW)
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
            //case MONEY+1__STATIC:
        case MAIL__STATIC:
            //case MAIL+1__STATIC:
        case PAPER__STATIC:
            //case PAPER+1__STATIC:
            if (RR && (switchpic == PUKE || switchpic == MAIL || switchpic == PAPER)) goto default_case2;
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

                if (klabs(G_GetAngleDelta(sqa,sqb)) > 512)
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
                t->picnum = STATIC;
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
                if (RRRA && (sprite[pSprite->owner].picnum == CHEER || sprite[pSprite->owner].picnum == CHEERSTAYPUT))
                {
                    t->picnum = CHEERBLADE + (((int32_t) totalclock >> 4) & 3);
                    t->shade = -127;
                }
                else
                    t->picnum = SHRINKSPARK + (((int32_t) totalclock >> 4) & 7);
            }
            else
                t->picnum = SHRINKSPARK+(((int32_t) totalclock>>4)&3);
            break;
        case CHEERBOMB__STATICRR:
            if (!RRRA) goto default_case2;
            t->picnum = CHEERBOMB+( ((int32_t) totalclock>>4)&3 );
            break;
        case GROWSPARK__STATIC:
            if (RR) goto default_case2;
            t->picnum = GROWSPARK+(((int32_t) totalclock>>4)&3);
            break;
        case SPIT__STATIC:
            if (!RR) goto default_case2;
            t->picnum = SPIT + (((int32_t) totalclock >> 4) & 3);
            if (RRRA)
            {
                if (sprite[pSprite->owner].picnum == MINION && sprite[pSprite->owner].pal == 8)
                    t->picnum = RRTILE3500 + (((int32_t) totalclock >> 4) % 6);
                else if (sprite[pSprite->owner].picnum == MINION && sprite[pSprite->owner].pal == 19)
                {
                    t->picnum = RRTILE5090 + (((int32_t) totalclock >> 4) & 3);
                    t->shade = -127;
                }
                else if (sprite[pSprite->owner].picnum == MAMA)
                {
#ifdef USE_OPENGL
                    if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0 &&
                        !(spriteext[i].flags & SPREXT_NOTMD))
                    {
                        int32_t v = getangle(t->xvel, t->zvel >> 4);

                        spriteext[i].pitch = (v > 1023 ? v - 2048 : v);
                        t->cstat &= ~4;
                        t->picnum = RRTILE7274;
                        break;
                    }
#endif
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));
                    t->picnum = RRTILE7274 + frameOffset;
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
            t->picnum = EMPTYBIKE + frameOffset;
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
            t->picnum = EMPTYBOAT + frameOffset;
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
            t->picnum = RPG+frameOffset;
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
            t->picnum = RPG2+frameOffset;
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

            t->picnum = RECON+frameOffset;

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
                    newTspr->picnum  = (currentWeapon == GROW_WEAPON ? GROWSPRITEICON : WeaponPickupSprites[currentWeapon]);
                    if (RR)
                    {
                        newTspr->picnum = 0;
                        switch(DYNAMICWEAPONMAP(g_player[playerNum].ps->curr_weapon))
                        {
                            case PISTOL_WEAPON__STATIC:      newTspr->picnum = FIRSTGUNSPRITE;       break;
                            case SHOTGUN_WEAPON__STATIC:     newTspr->picnum = SHOTGUNSPRITE;        break;
                            case CHAINGUN_WEAPON__STATIC:    newTspr->picnum = CHAINGUNSPRITE;       break;
                            case RPG_WEAPON__STATIC:         newTspr->picnum = RPGSPRITE;            break;
                            case CHICKEN_WEAPON__STATIC:     newTspr->picnum = RPGSPRITE;            break;
                            case HANDREMOTE_WEAPON__STATIC:
                            case HANDBOMB_WEAPON__STATIC:    newTspr->picnum = HEAVYHBOMB;           break;
                            case TRIPBOMB_WEAPON__STATIC:    newTspr->picnum = TRIPBOMBSPRITE;       break;
                            case BOWLINGBALL_WEAPON__STATIC: newTspr->picnum = BOWLINGBALLSPRITE;    break;
                            case SHRINKER_WEAPON__STATIC:    newTspr->picnum = SHRINKSPARK;          break;
                            case GROW_WEAPON__STATIC:        newTspr->picnum = SHRINKSPARK;          break;
                            case FREEZE_WEAPON__STATIC:      newTspr->picnum = DEVISTATORSPRITE;     break;
                            case DEVISTATOR_WEAPON__STATIC:  newTspr->picnum = FREEZESPRITE;         break;
                        }
                    }
                    newTspr->z       = (pSprite->owner >= 0) ? g_player[playerNum].ps->pos.z - ZOFFSET4 : pSprite->z - (51 << 8);
                    newTspr->xrepeat = (newTspr->picnum == HEAVYHBOMB) ? 10 : 16;
                    if (RRRA && (g_player[playerNum].ps->on_motorcycle || g_player[playerNum].ps->on_boat))
                        newTspr->xrepeat = 0;
                    newTspr->yrepeat = newTspr->xrepeat;

                    spritesortcnt++;
                }

                if (g_player[playerNum].input->extbits & (1 << 7) && !ud.pause_on && spritesortcnt < maxspritesonscreen)
                {
                    tspritetype *const playerTyping = t;

                    playerTyping->statnum = TSPR_TEMP;
                    playerTyping->cstat   = 0;
                    playerTyping->picnum  = RESPAWNMARKERGREEN;
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
                // Display APLAYER sprites with action PSTAND when viewed through
                // a camera.  Not implemented for Lunatic.
                const intptr_t *aplayer_scr = g_tile[APLAYER].execPtr;
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
                if (g_player[playerNum].ps->on_motorcycle && playerNum == screenpeek)
                {
                    t->picnum = RRTILE7219;
                    t->xrepeat = 18;
                    t->yrepeat = 18;
                    scrofs_action = 0;
                    curframe = 0;
                }
                else if (g_player[playerNum].ps->on_motorcycle)
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
                        t->picnum = RRTILE7213;
                    }
                    else
#endif
                        frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - oura);

                    t->picnum = RRTILE7213 + frameOffset;
                }
                else if (g_player[playerNum].ps->on_boat && playerNum == screenpeek)
                {
                    t->picnum = RRTILE7190;
                    t->xrepeat = 32;
                    t->yrepeat = 32;
                    scrofs_action = 0;
                    curframe = 0;
                }
                else if (g_player[playerNum].ps->on_boat)
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
                        t->picnum = RRTILE7213;
                    }
                    else
#endif
                        frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - oura);

                    t->picnum = RRTILE7184 + frameOffset;
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
                if (switchpic == HEADJIB1 || switchpic == LEGJIB1 || switchpic == ARMJIB1
                    || switchpic == LIZMANHEAD1 || switchpic == LIZMANARM1 || switchpic == LIZMANLEG1)
                    goto default_case2;
            }
            if (RRRA && t->pal == 19 && (switchpic == MINJIBA || switchpic == MINJIBB || switchpic == MINJIBC))
                t->shade = -127;
            if (adult_lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            if (t->pal == 6)
                t->shade = -120;
            if (RR && g_shadedSector[pSprite->sectnum] == 1)
                t->shade = 16;
            fallthrough__;
        case SCRAP1__STATIC:
        case SCRAP2__STATIC:
        case SCRAP3__STATIC:
        case SCRAP4__STATIC:
        case SCRAP5__STATIC:
            if ((RR || actor[i].picnum == BLIMP) && t->picnum == SCRAP1 && pSprite->yvel >= 0)
                t->picnum = pSprite->yvel < MAXUSERTILES ? pSprite->yvel : 0;
            else t->picnum += T1(i);
            if (!RR)
                t->shade = -128+6 < t->shade ? t->shade-6 : -128; // effectively max(t->shade-6, -128) while avoiding (signed!) underflow

            G_MaybeTakeOnFloorPal(t, sect);
            break;
        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == FLOORSLIME)
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

            l = apScript[scrofs_action + ACTION_VIEWTYPE];
            uint16_t const action_flags = apScript[scrofs_action + ACTION_FLAGS];

            int const invertp = l < 0;
            l = klabs(l);

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

            t->picnum += frameOffset + apScript[scrofs_action + ACTION_STARTFRAME] + l*curframe;
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
                (A_CheckEnemySprite(pSprite) || A_CheckSpriteFlags(t->owner,SFLAG_NVG) || pSprite->picnum == APLAYER || pSprite->statnum == STAT_DUMMYPLAYER))
        {
            t->pal = 6;
            t->shade = 0;
        }

        if (RR && !RRRA && pSprite->picnum == SBMOVE)
            t->shade = -127;

        // Fake floor shadow, implemented by inserting a new tsprite.
        if (pSprite->statnum == STAT_DUMMYPLAYER || A_CheckEnemySprite(pSprite) || A_CheckSpriteFlags(t->owner,SFLAG_SHADOW) || (pSprite->picnum == APLAYER && pSprite->owner >= 0))
            if ((!RR || (pSprite->cstat&48) == 0) && t->statnum != TSPR_TEMP && pSprite->picnum != EXPLOSION2 && (RR || pSprite->picnum != HANGLIGHT) && pSprite->picnum != DOMELITE && (RR || pSprite->picnum != HOTMEAT)
                && (!RR || pSprite->picnum != TORNADO) && (!RR || pSprite->picnum != EXPLOSION3) && (!RR || RRRA || pSprite->picnum != SBMOVE))
            {
                if (actor[i].dispicnum < 0)
                {
#ifdef DEBUGGINGAIDS
                    // A negative actor[i].dispicnum used to mean 'no floor shadow please', but
                    // that was a bad hack since the value could propagate to sprite[].picnum.
                    Printf(OSD_ERROR "actor[%d].dispicnum = %d\n", i, actor[i].dispicnum);
#endif
                    actor[i].dispicnum=0;
                    continue;
                }

                if (actor[i].flags & SFLAG_NOFLOORSHADOW)
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
                                   pSprite->statnum == STAT_MISC || pSprite->picnum == DRONE || (!RR && pSprite->picnum == COMMANDER))
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
        
        if (DEER)
        {
            if (pSprite->picnum == 806)
                t->picnum = 1023;
        }
        else switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case LASERLINE__STATIC:
            if (RR) break;
            if (sector[t->sectnum].lotag == ST_2_UNDERWATER) t->pal = 8;
            t->z = sprite[pSprite->owner].z-(3<<8);
            if (g_tripbombLaserMode == 2 && g_player[screenpeek].ps->heat_on == 0)
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
            if (RR && !RRRA && (pSprite->picnum == RPG2 || pSprite->picnum == RRTILE1790)) break;
            if (t->picnum == EXPLOSION2)
            {
                g_player[screenpeek].ps->visibility = -127;
                //g_restorePalette = 1;   // JBF 20040101: why?
                if (RR)
                    t->pal = 0;
            }
            else if (RR && t->picnum == FIRELASER)
                t->picnum = FIRELASER+(((int32_t) totalclock>>2)&5);
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
            if (RR && pSprite->picnum == FIRE2) break;
            t->cstat |= 128;
            fallthrough__;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (RR && pSprite->picnum == BURNING2) break;
            if (sprite[pSprite->owner].picnum != TREE1 && sprite[pSprite->owner].picnum != TREE2)
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
            if (t->picnum >= CHEER + 102 && t->picnum <= CHEER + 151)
                t->shade = -127;
            break;
        case MINION__STATICRR:
            if (!RRRA) break;
            if (t->pal == 19)
                t->shade = -127;
            break;
        case BIKER__STATICRR:
            if (!RRRA) break;
            if (t->picnum >= BIKER + 54 && t->picnum <= BIKER + 58)
                t->shade = -127;
            else if (t->picnum >= BIKER + 84 && t->picnum <= BIKER + 88)
                t->shade = -127;
            break;
        case BILLYRAY__STATICRR:
        case BILLYRAYSTAYPUT__STATICRR:
            if (!RRRA) break;
            if (t->picnum >= BILLYRAY + 5 && t->picnum <= BILLYRAY + 9)
                t->shade = -127;
            break;
        case RRTILE2034__STATICRR:
            t->picnum = RRTILE2034 + ((int32_t) totalclock & 1);
            break;
        case RRTILE2944__STATICRR:
            t->shade = -127;
            t->picnum = RRTILE2944 + (((int32_t) totalclock >> 2) & 4);
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
            t->picnum = MUD+T2(i);
            break;
        case WATERSPLASH2__STATIC:
            // WATERSPLASH_T2
            t->picnum = WATERSPLASH2+T2(i);
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
                if (sprite[pSprite->owner].picnum == APLAYER)
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

                    if (RR && sprite[pSprite->owner].picnum == APLAYER)
                        t->picnum = SMALLSMOKE;

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
        if (sector[t->sectnum].floorpicnum == MIRROR)
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

    if (DEER)
    {
        sub_53304();
        return;
    }

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

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE)) || GUICapture)
        return;

    if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_COOPVIEW) || ud.recstat == 2))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = 0;
        g_restorePalette = -1;
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

            if (!RRRA || (!g_player[myconnectindex].ps->on_motorcycle && !g_player[myconnectindex].ps->on_boat))
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
        g_restorePalette = 1;
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



static void G_FreeHashAnim(const char * /*string*/, intptr_t key)
{
    Xfree((void *)key);
}

static void G_Cleanup(void)
{
    int32_t i;

    for (i=(MAXLEVELS*(MAXVOLUMES+1))-1; i>=0; i--) // +1 volume for "intro", "briefing" music
    {
        G_FreeMapState(i);
    }

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Xfree(g_player[i].ps);
        Xfree(g_player[i].input);
    }

    if (label != (char *)&sprite[0]) Xfree(label);
    if (labelcode != (int32_t *)&sector[0]) Xfree(labelcode);
    if (labeltype != (int32_t*)&wall[0]) Xfree(labeltype);
    Xfree(apScript);
    Xfree(bitptr);

//    Xfree(MusicPtr);

    Gv_Clear();

    hash_free(&h_gamevars);
    hash_free(&h_labels);
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
    labeltype = (int32_t *)&wall[0];   // V8: 16384*32/4 = 131072  V7: 8192*32/4 = 65536

    C_Compile(G_ConFile());

    if ((uint32_t)g_labelCnt > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        G_GameExit("Error: too many labels defined!");

    {
        char *newlabel;
        int32_t *newlabelcode;
        int32_t *newlabeltype;

        newlabel     = (char *)Xmalloc(g_labelCnt << 6);
        newlabelcode = (int32_t *)Xmalloc(g_labelCnt * sizeof(int32_t));
        newlabeltype = (int32_t *)Xmalloc(g_labelCnt * sizeof(int32_t));

        Bmemcpy(newlabel, label, g_labelCnt*64);
        Bmemcpy(newlabelcode, labelcode, g_labelCnt*sizeof(int32_t));
        Bmemcpy(newlabeltype, labeltype, g_labelCnt*sizeof(int32_t));

        label = newlabel;
        labelcode = newlabelcode;
        labeltype = newlabeltype;
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

static void G_PostLoadPalette(void)
{
    if (!(duke3d_globalflags & DUKE3D_NO_PALETTE_CHANGES))
    {
        // Make color index 255 of default/water/slime palette black.
        if (basepaltable[BASEPAL] != NULL)
            Bmemset(&basepaltable[BASEPAL][255*3], 0, 3);
        if (basepaltable[WATERPAL] != NULL)
            Bmemset(&basepaltable[WATERPAL][255*3], 0, 3);
        if (basepaltable[SLIMEPAL] != NULL)
            Bmemset(&basepaltable[SLIMEPAL][255*3], 0, 3);
    }

    //if (!(duke3d_globalflags & DUKE3D_NO_HARDCODED_FOGPALS))
    //    paletteSetupDefaultFog();

    if (!(duke3d_globalflags & DUKE3D_NO_PALETTE_CHANGES))
        paletteFixTranslucencyMask();

    palettePostLoadLookups();
}

#define SETFLAG(Tilenum, Flag) g_tile[Tilenum].flags |= Flag

// Has to be after setting the dynamic names (e.g. SHARK).
static void A_InitEnemyFlags(void)
{
    if (DEER)
    {
        int DukeEnemies[] = {
            DOGRUN, PIG, VIXEN, CHEER };

        for (bssize_t i = ARRAY_SIZE(DukeEnemies) - 1; i >= 0; i--)
            SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY|SFLAG_BADGUY_TILE|SFLAG_KILLCOUNT);
    }
    else if (RRRA)
    {
        int DukeEnemies[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT, HEN,
            DRONE, PIG, RECON, MINION, MINIONSTAYPUT, UFO1, COOT, COOTSTAYPUT, SHARK,
            VIXEN, SBSWIPE, BIKERB, BIKERBV2, BIKER, MAKEOUT, CHEERB, CHEER, CHEERSTAYPUT,
            COOTPLAY, BILLYPLAY, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, MAMA };

        int DukeEnemiesTile[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT,
            DRONE, PIG, RECON, MINION, MINIONSTAYPUT, UFO1, COOT, COOTSTAYPUT, SHARK,
            VIXEN, SBSWIPE, BIKERB, BIKERBV2, BIKER, MAKEOUT, CHEERB, CHEER, CHEERSTAYPUT,
            COOTPLAY, BILLYPLAY, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, MAMA };

        int KillCountEnemies[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT,
            DRONE, PIG, RECON, MINION, MINIONSTAYPUT, UFO1, COOT, COOTSTAYPUT, SHARK,
            VIXEN, SBSWIPE, BIKERB, BIKERBV2, BIKER, MAKEOUT, CHEERB, CHEER, CHEERSTAYPUT,
            COOTPLAY, BILLYPLAY, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, MAMA,
            ROCK, ROCK2 };

        int SolidEnemies[] = { HULK, MAMA, BILLYPLAY, COOTPLAY, MAMACLOUD };
        int NoWaterDipEnemies[] = { DRONE };
        int NoCanSeeCheck[] = {
            COOT, COOTSTAYPUT, VIXEN, BIKERB, BIKERBV2, CHEER, CHEERB,
            CHEERSTAYPUT, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, COOTPLAY,
            BILLYPLAY, MAKEOUT, MAMA };

        for (bssize_t i = ARRAY_SIZE(DukeEnemies) - 1; i >= 0; i--)
            SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY);

        for (bssize_t i = ARRAY_SIZE(DukeEnemiesTile) - 1; i >= 0; i--)
            SETFLAG(DukeEnemiesTile[i], SFLAG_BADGUY_TILE);

        for (bssize_t i = ARRAY_SIZE(KillCountEnemies) - 1; i >= 0; i--)
            SETFLAG(KillCountEnemies[i], SFLAG_KILLCOUNT);

        for (bssize_t i = ARRAY_SIZE(SolidEnemies) - 1; i >= 0; i--)
            SETFLAG(SolidEnemies[i], SFLAG_NODAMAGEPUSH);

        for (bssize_t i = ARRAY_SIZE(NoWaterDipEnemies) - 1; i >= 0; i--)
            SETFLAG(NoWaterDipEnemies[i], SFLAG_NOWATERDIP);

        for (bssize_t i = ARRAY_SIZE(NoCanSeeCheck) - 1; i >= 0; i--)
            SETFLAG(NoCanSeeCheck[i], SFLAG_NOCANSEECHECK);
    }
    else if (RR)
    {
        int DukeEnemies[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT, HEN,
            DRONE, PIG, RECON, SBMOVE, MINION, MINIONSTAYPUT, UFO1, UFO2, UFO3, UFO4, UFO5,
            COOT, COOTSTAYPUT, SHARK, VIXEN };

        int DukeEnemiesTile[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT,
            DRONE, PIG, RECON, SBMOVE, MINION, MINIONSTAYPUT, UFO1, UFO2, UFO3, UFO4, UFO5,
            COOT, COOTSTAYPUT, SHARK, VIXEN };

        int KillCountEnemies[] = {
            BOULDER, BOULDER1, EGG, RAT, TORNADO, BILLYCOCK, BILLYRAY, BILLYRAYSTAYPUT,
            BRAYSNIPER, DOGRUN, LTH, HULKJUMP, BUBBASTAND, HULK, HULKSTAYPUT,
            DRONE, PIG, RECON, SBMOVE, MINION, MINIONSTAYPUT, UFO1, UFO2, UFO3, UFO4, UFO5,
            COOT, COOTSTAYPUT, SHARK, VIXEN };

        int SolidEnemies[] = { HULK, SBMOVE };
        int NoWaterDipEnemies[] = { DRONE };
        int NoCanSeeCheck[] = { VIXEN };

        for (bssize_t i = ARRAY_SIZE(DukeEnemies) - 1; i >= 0; i--)
            SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY);

        for (bssize_t i = ARRAY_SIZE(DukeEnemiesTile) - 1; i >= 0; i--)
            SETFLAG(DukeEnemiesTile[i], SFLAG_BADGUY_TILE);

        for (bssize_t i = ARRAY_SIZE(KillCountEnemies) - 1; i >= 0; i--)
            SETFLAG(KillCountEnemies[i], SFLAG_KILLCOUNT);

        for (bssize_t i = ARRAY_SIZE(SolidEnemies) - 1; i >= 0; i--)
            SETFLAG(SolidEnemies[i], SFLAG_NODAMAGEPUSH);

        for (bssize_t i = ARRAY_SIZE(NoWaterDipEnemies) - 1; i >= 0; i--)
            SETFLAG(NoWaterDipEnemies[i], SFLAG_NOWATERDIP);

        for (bssize_t i = ARRAY_SIZE(NoCanSeeCheck) - 1; i >= 0; i--)
            SETFLAG(NoCanSeeCheck[i], SFLAG_NOCANSEECHECK);
    }
    else
    {
        int DukeEnemies[] = {
            SHARK, RECON, DRONE,
            LIZTROOPONTOILET, LIZTROOPJUSTSIT, LIZTROOPSTAYPUT, LIZTROOPSHOOT,
            LIZTROOPJETPACK, LIZTROOPDUCKING, LIZTROOPRUNNING, LIZTROOP,
            OCTABRAIN, COMMANDER, COMMANDERSTAYPUT, PIGCOP, EGG, PIGCOPSTAYPUT, PIGCOPDIVE,
            LIZMAN, LIZMANSPITTING, LIZMANFEEDING, LIZMANJUMP, ORGANTIC,
            BOSS1, BOSS2, BOSS3, BOSS4, RAT, ROTATEGUN };

        int SolidEnemies[] = { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN };
        int NoWaterDipEnemies[] = { OCTABRAIN, COMMANDER, DRONE };
        int GreenSlimeFoodEnemies[] = { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST };

        for (bssize_t i=GREENSLIME; i<=GREENSLIME+7; i++)
            SETFLAG(i, SFLAG_HARDCODED_BADGUY|SFLAG_BADGUY_TILE);

        for (bssize_t i=ARRAY_SIZE(DukeEnemies)-1; i>=0; i--)
            SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY|SFLAG_BADGUY_TILE);

        for (bssize_t i=ARRAY_SIZE(SolidEnemies)-1; i>=0; i--)
            SETFLAG(SolidEnemies[i], SFLAG_NODAMAGEPUSH);

        for (bssize_t i=ARRAY_SIZE(NoWaterDipEnemies)-1; i>=0; i--)
            SETFLAG(NoWaterDipEnemies[i], SFLAG_NOWATERDIP);

        for (bssize_t i=ARRAY_SIZE(GreenSlimeFoodEnemies)-1; i>=0; i--)
            SETFLAG(GreenSlimeFoodEnemies[i], SFLAG_GREENSLIMEFOOD);
    }
}
#undef SETFLAG

// Throw in everything here that needs to be called after a Lua game state
// recreation (or on initial startup in a non-Lunatic build.)
void G_PostCreateGameState(void)
{
    Net_SendClientInfo();
    A_InitEnemyFlags();
}

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
    G_InitMultiPsky(CLOUDYOCEAN, MOONSKY1, BIGORBIT1, LA);
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

    // Make the fullscreen nuke logo background non-fullbright.  Has to be
    // after dynamic tile remapping (from C_Compile) and loading tiles.
    picanm[LOADSCREEN].sf |= PICANM_NOFULLBRIGHT_BIT;

//    Printf("Loading palette/lookups...\n");
    G_LoadLookups();

    if (DEER)
        sub_54DE0();

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
        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        /*int32_t j = g_player[myconnectindex].ps->team;*/

        P_SetupMiscInputSettings();
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);

        g_player[myconnectindex].pteam = playerteam;

        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
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
EDUKE32_STATIC_ASSERT(sizeof(DukePlayer_t)%4 == 0);

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

int GameInterface::app_main()
{
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

    CONFIG_ReadSetup();


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

#ifndef NETCODE_DISABLE
    Net_InitNetwork();
#endif
    numplayers = 1;
    g_mostConcurrentPlayers = ud.multimode;  // Lunatic needs this (player[] bound)

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

    // NOTE: Allocating the DukePlayer_t structs has to be before compiling scripts,
    // because in Lunatic, the {pipe,trip}bomb* members are initialized.
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

    G_PostLoadPalette();

    tileDelete(MIRROR);
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

    V_Init2();
    videoSetPalette(0, g_player[myconnectindex].ps->palette, 0);

    // check if the minifont will support lowercase letters (3136-3161)
    // there is room for them in tiles012.art between "[\]^_." and "{|}~"
    minitext_lowercase = 1;

    for (bssize_t i = MINIFONT + ('a'-'!'); minitext_lowercase && i < MINIFONT + ('z'-'!') + 1; ++i)
        minitext_lowercase &= (int)tileCheck(i);

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

    if (DEER)
        ghtrophy_loadbestscores();

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

        if (M_Active() || GUICapture || ud.pause_on != 0)
        {
            ototalclock = totalclock - TICSPERFRAME;
            buttonMap.ResetButtonStates();
        }
        else
        {
            while (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
            {
                ototalclock += TICSPERFRAME;

                if (RRRA && g_player[myconnectindex].ps->on_motorcycle)
                    P_GetInputMotorcycle(myconnectindex);
                else if (RRRA && g_player[myconnectindex].ps->on_boat)
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

                if (((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU || ud.recstat == 2 || (g_netServer || ud.multimode > 1)) &&
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
        }

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
            if (RRRA && g_player[myconnectindex].ps->on_motorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (RRRA && g_player[myconnectindex].ps->on_boat)
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

            if (DEER)
                sub_5A02C();
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
    if (DEER)
        sub_579A0();
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
                sprite[hitData.sprite].picnum == APLAYER)
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
        A_MoveDummyPlayers();//ST 13
    }

    for (bssize_t TRAVERSE_CONNECT(i))
    {
        if (g_player[i].input->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                actor[g_player[i].ps->i].picnum = APLAYERTOP;
                P_QuickKill(g_player[i].ps);
            }
        }
        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = G_GetTeamPalette(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

        if (!DEER)
        P_HandleSharedKeys(i);

        if (ud.pause_on == 0)
        {
            P_ProcessInput(i);
            if (!DEER)
            P_CheckSectors(i);
        }
    }

    if (ud.pause_on == 0)
        G_MoveWorld();

    Net_CorrectPrediction();

    //if (g_netServer)
    //    Net_SendServerUpdates();

    if ((everyothertime&1) == 0)
    {
        if (DEER)
        {
            ghsound_ambientlooppoll();
        }
        else
        {
        G_AnimateWalls();
        A_MoveCyclers();
        }

        //if (g_netServer && (everyothertime % 10) == 0)
        //{
        //    Net_SendMapUpdate();
        //}
    }

    //if (g_netClient)   //Slave
    //    Net_SendClientUpdate();

    if (RR && ud.recstat == 0 && ud.multimode < 2 && g_torchCnt)
        G_DoTorch();

    return 0;
}

void A_SpawnWallGlass(int spriteNum, int wallNum, int glassCnt)
{
    if (wallNum < 0)
    {
        for (bssize_t j = glassCnt - 1; j >= 0; --j)
        {
            int const a = SA(spriteNum) - 256 + (krand2() & 511) + 1024;
            int32_t const r1 = krand2(), r2 = krand2();
            A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum), GLASSPIECES + (j % 3), -32, 36, 36, a,
                           32 + (r2 & 63), 1024 - (r1 & 1023), spriteNum, 5);
        }
        return;
    }

    vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
    vec2_t v  = { wall[wall[wallNum].point2].x - v1.x, wall[wall[wallNum].point2].y - v1.y };

    v1.x -= ksgn(v.y);
    v1.y += ksgn(v.x);

    v.x = tabledivide32_noinline(v.x, glassCnt+1);
    v.y = tabledivide32_noinline(v.y, glassCnt+1);

    int16_t sect = -1;

    for (bsize_t j = glassCnt; j > 0; --j)
    {
        v1.x += v.x;
        v1.y += v.y;

        updatesector(v1.x,v1.y,&sect);
        if (sect >= 0)
        {
            int z = sector[sect].floorz - (krand2() & (klabs(sector[sect].ceilingz - sector[sect].floorz)));

            if (z < -ZOFFSET5 || z > ZOFFSET5)
                z = SZ(spriteNum) - ZOFFSET5 + (krand2() & ((64 << 8) - 1));

            int32_t const r1 = krand2(), r2 = krand2();
            A_InsertSprite(SECT(spriteNum), v1.x, v1.y, z, GLASSPIECES + (j % 3), -32, 36, 36, SA(spriteNum) - 1024, 32 + (r2 & 63),
                           -(r1 & 1023), spriteNum, 5);
        }
    }
}

void A_SpawnWallPopcorn(int spriteNum, int wallNum, int glassCnt)
{
    if (wallNum < 0)
    {
        for (bssize_t j = glassCnt - 1; j >= 0; --j)
        {
            int const a = SA(spriteNum) - 256 + (krand2() & 511) + 1024;
            int32_t const r1 = krand2(), r2 = krand2();
            A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum), POPCORN, -32, 36, 36, a,
                           32 + (r2 & 63), 1024 - (r1 & 1023), spriteNum, 5);
        }
        return;
    }

    vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
    vec2_t v  = { wall[wall[wallNum].point2].x - v1.x, wall[wall[wallNum].point2].y - v1.y };

    v1.x -= ksgn(v.y);
    v1.y += ksgn(v.x);

    v.x = tabledivide32_noinline(v.x, glassCnt+1);
    v.y = tabledivide32_noinline(v.y, glassCnt+1);

    int16_t sect = -1;

    for (bsize_t j = glassCnt; j > 0; --j)
    {
        v1.x += v.x;
        v1.y += v.y;

        updatesector(v1.x,v1.y,&sect);
        if (sect >= 0)
        {
            int z = sector[sect].floorz - (krand2() & (klabs(sector[sect].ceilingz - sector[sect].floorz)));

            if (z < -ZOFFSET5 || z > ZOFFSET5)
                z = SZ(spriteNum) - ZOFFSET5 + (krand2() & ((64 << 8) - 1));

            int32_t const r1 = krand2(), r2 = krand2();
            A_InsertSprite(SECT(spriteNum), v1.x, v1.y, z, POPCORN, -32, 36, 36, SA(spriteNum) - 1024, 32 + (r2 & 63),
                           -(r1 & 1023), spriteNum, 5);
        }
    }
}

void A_SpawnGlass(int spriteNum, int glassCnt)
{
    for (; glassCnt>0; glassCnt--)
    {
        int const a = krand2()&2047;
        int const z = SZ(spriteNum)-((krand2()&16)<<8);
        int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2();
        int const k
        = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), z, GLASSPIECES + (glassCnt % 3),
                         r3 & 15, 36, 36, a, 32 + (r2 & 63), -512 - (r1 & 2047), spriteNum, 5);
        sprite[k].pal = sprite[spriteNum].pal;
    }
}

void A_SpawnCeilingGlass(int spriteNum, int sectNum, int glassCnt)
{
    int const startWall = sector[sectNum].wallptr;
    int const endWall = startWall+sector[sectNum].wallnum;

    for (bssize_t wallNum = startWall; wallNum < (endWall - 1); wallNum++)
    {
        vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
        vec2_t v  = { tabledivide32_noinline(wall[wallNum + 1].x - v1.x, glassCnt + 1),
                     tabledivide32_noinline(wall[wallNum + 1].y - v1.y, glassCnt + 1) };

        for (bsize_t j = glassCnt; j > 0; j--)
        {
            v1.x += v.x;
            v1.y += v.y;
            int const a = krand2()&2047;
            int const z = sector[sectNum].ceilingz+((krand2()&15)<<8);
            A_InsertSprite(sectNum, v1.x, v1.y, z, GLASSPIECES + (j % 3), -32, 36, 36,
                           a, (krand2() & 31), 0, spriteNum, 5);
        }
    }
}

void A_SpawnRandomGlass(int spriteNum, int wallNum, int glassCnt)
{
    if (wallNum < 0)
    {
        for (bssize_t j = glassCnt - 1; j >= 0; j--)
        {
            int const a = krand2() & 2047;
            int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2();
            int const k
            = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (r3 & (63 << 8)), GLASSPIECES + (j % 3),
                             -32, 36, 36, a, 32 + (r2 & 63), 1024 - (r1 & 2047), spriteNum, 5);
            sprite[k].pal = krand2() & 15;
        }
        return;
    }

    vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
    vec2_t v  = { tabledivide32_noinline(wall[wall[wallNum].point2].x - wall[wallNum].x, glassCnt + 1),
                 tabledivide32_noinline(wall[wall[wallNum].point2].y - wall[wallNum].y, glassCnt + 1) };
    int16_t sectNum = sprite[spriteNum].sectnum;

    for (bsize_t j = glassCnt; j > 0; j--)
    {
        v1.x += v.x;
        v1.y += v.y;

        updatesector(v1.x, v1.y, &sectNum);

        int z = sector[sectNum].floorz - (krand2() & (klabs(sector[sectNum].ceilingz - sector[sectNum].floorz)));

        if (z < -ZOFFSET5 || z > ZOFFSET5)
            z       = SZ(spriteNum) - ZOFFSET5 + (krand2() & ((64 << 8) - 1));

        int32_t const r1 = krand2(), r2 = krand2();
        int const k = A_InsertSprite(SECT(spriteNum), v1.x, v1.y, z, GLASSPIECES + (j % 3), -32, 36, 36, SA(spriteNum) - 1024,
                                     32 + (r2 & 63), -(r1 & 2047), spriteNum, 5);
        sprite[k].pal = krand2() & 7;
    }
}

void GameInterface::FreeGameData()
{
    G_SetFog(0);
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

END_RR_NS
