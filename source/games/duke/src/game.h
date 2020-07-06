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

#pragma once

#ifndef ONLY_USERDEFS
#include "premap.h" // XXX
#endif

#include "fix16.h"
#include "gamedef.h"
#include "net.h"
#include "mmulti.h"
#include "palette.h"
#include "cmdlib.h"
#include "screenjob.h"
#include "constants.h"

BEGIN_DUKE_NS

struct weaponhit
{
	uint8_t cgg;
	short picnum, ang, extra, owner, movflag;
	short tempang, actorstayput, dispicnum;
	short timetosleep;
	int floorz, ceilingz, lastvx, lastvy, bposx, bposy, bposz, aflags;
	int temp_data[6];
};


// Todo - put more state in here
struct ActorInfo
{
	uint32_t scriptaddress;
	uint32_t flags;
	int aimoffset;
};




#define MAXSAVEGAMENAMESTRUCT 32
#define MAXSAVEGAMENAME (MAXSAVEGAMENAMESTRUCT-1)
#define MAXPWLOCKOUT 128
#define MAXRTSNAME 128

#define MAX_RETURN_VALUES 6

// KEEPINSYNC lunatic/_defs_game.lua

typedef struct {
    vec3_t camerapos;
    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt;

    int32_t statusbarscale,weaponswitch;   // JBF 20031125
    int32_t statusbarmode;
	int32_t noexits,automsg;
    int32_t althud;
    int32_t statusbarrange;

    int32_t entered_name,screen_tilting;
    int32_t coop,screen_size,lockout,crosshair;
    int32_t angleinterpolation;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off;
    int32_t ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32_t player_skill,level_number,volume_number,marker;

    //int32_t returnvar[MAX_RETURN_VALUES-1];

    uint32_t userbytever;

    fix16_t cameraq16ang, cameraq16horiz;
    int16_t camerasect;
    int16_t pause_on,from_bonus;
    int16_t camerasprite,last_camsprite;
    int16_t last_level,secretlevel;

    int8_t menutitle_pal, slidebar_palselected, slidebar_paldisabled;

    struct {
        int32_t AutoAim;
        int32_t ShowOpponentWeapons;
    } config;

    char overhead_on,last_overhead,showweapons;
    char god,warp_on,cashman,eog;
    char scrollmode,clipping;
    char display_bonus_screen;
    char show_level_text;

    uint8_t user_map;
    uint8_t screenfade, menubackground;
    uint8_t shadow_pal;
    uint8_t wchoice[MAXPLAYERS][MAX_WEAPONS];
} user_defs;

extern user_defs ud;
extern int rtsplaying;

#ifndef ONLY_USERDEFS

// this is checked against http://eduke32.com/VERSION
extern const char *s_buildDate;

extern char boardfilename[BMAX_PATH];
#define USERMAPMUSICFAKEVOLUME MAXVOLUMES
#define USERMAPMUSICFAKELEVEL (MAXLEVELS-1)
#define USERMAPMUSICFAKESLOT ((USERMAPMUSICFAKEVOLUME * MAXLEVELS) + USERMAPMUSICFAKELEVEL)

static inline int G_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0);
}

static inline int Menu_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && m_level_number == 7 && ud.m_volume_number == 0);
}

extern int32_t g_Shareware;
extern int32_t cameraclock;
extern int32_t cameradist;
extern int32_t g_crosshairSum;
extern int32_t g_doQuickSave;
extern int32_t restorepalette;
extern int32_t tempwallptr;

//extern int8_t cheatbuf[MAXCHEATLEN],cheatbuflen;

#define A_InsertSprite EGS
int G_DoMoveThings(void);
//int32_t G_EndOfLevel(void);

#ifdef YAX_ENABLE
void Yax_SetBunchZs(int32_t sectnum, int32_t cf, int32_t daz);
#else
#define Yax_SetBunchZs(sectnum, cf, daz)
#endif


void G_BackToMenu(void);

const char* G_PrintYourTime(void);
const char* G_PrintParTime(void);
const char* G_PrintDesignerTime(void);
const char* G_PrintBestTime(void);
void G_BonusScreen(int32_t bonusonly);
void G_BonusScreenRRRA(int32_t bonusonly);
//void G_CheatGetInv(void);
void G_DisplayRest(int32_t smoothratio);
void drawbackground(void);
void G_DrawFrags(void);
void G_HandleMirror(int32_t x, int32_t y, int32_t z, fix16_t a, fix16_t horiz, int32_t smoothratio);
void displayrooms(int32_t playerNum,int32_t smoothratio);
void G_DrawTXDigiNumZ(int32_t starttile,int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,int32_t cs,int32_t x1,int32_t y1,int32_t x2,int32_t y2,int32_t z);
void G_HandleLocalKeys(void);
void G_UpdatePlayerFromMenu(void);
void FTA(int q, struct player_struct* p);

void P_SetGamePalette(DukePlayer_t* player, uint32_t palid, ESetPalFlags flags);
void OnMotorcycle(DukePlayer_t *pPlayer, int spriteNum);
void OffMotorcycle(DukePlayer_t *pPlayer);
void OnBoat(DukePlayer_t *pPlayer, int spriteNum);
void OffBoat(DukePlayer_t *pPlayer);

// Cstat protection mask for (currently) spawned MASKWALL* sprites.
// TODO: look at more cases of cstat=(cstat&PROTECTED)|ADDED in fi.spawn()?
// 2048+(32+16)+8+4
#define SPAWN_PROTECT_CSTAT_MASK (CSTAT_SPRITE_NOSHADE|CSTAT_SPRITE_ALIGNMENT_SLAB|CSTAT_SPRITE_XFLIP|CSTAT_SPRITE_YFLIP);

void G_InitTimer(int32_t ticspersec);

inline int32_t G_GetTeamPalette(int32_t team)
{
    int8_t pal[] = { 3, 10, 11, 12 };

    if ((unsigned)team >= ARRAY_SIZE(pal))
        return 0;

    return pal[team];
}

enum
{
    TFLAG_WALLSWITCH = 1
};
// for now just flags not related to actors, may get more info later.
struct TileInfo
{
    int flags;
};
extern TileInfo tileinfo[MAXTILES];


// (unsigned)iPicnum check: AMC TC Rusty Nails, bayonet MG alt. fire, iPicnum == -1 (via aplWeaponShoots)
#define A_CheckSpriteTileFlags(iPicnum, iType) (((unsigned)iPicnum < MAXTILES) && (actorinfo[iPicnum].flags & iType) != 0)
#define S_StopSound(num) S_StopEnvSound(num, -1)

extern int startrts(int lumpNum, int localPlayer);

extern void G_MaybeAllocPlayer(int32_t pnum);

static inline void G_HandleAsync(void)
{
    handleevents();
}

static inline int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk)
{
    if (!((ud.multimode < 2 && ((g_player[myconnectindex].ps->gm & MODE_MENU) == 0)) ||
          ud.multimode > 1 || ud.recstat == 2) || ud.pause_on)
    {
        return 65536;
    }

    return CalcSmoothRatio(totalclk, ototalclk, REALGAMETICSPERSEC);
}


static inline void G_NewGame_EnterLevel(void)
{
    G_NewGame(ud.m_volume_number, m_level_number, ud.m_player_skill);

    if (G_EnterLevel(MODE_GAME))
        G_BackToMenu();
}

extern void G_PrintCurrentMusic(void);


extern void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN);
extern void G_SetupGlobalPsky(void);

//////////

extern void genspriteremaps(void);

extern int32_t      actor_tog;
extern int32_t      otherp;


extern ActorInfo   actorinfo[MAXTILES];
extern weaponhit      hittype[MAXSPRITES];

#endif

END_DUKE_NS
