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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

/*
#include "duke3d.h"
#include "gamedef.h"
#include "osd.h"
*/
// this is all the crap for accessing the game's structs through the CON VM
// I got a 3-4 fps gain by inlining these...

#ifndef gamevars_c_
int32_t __fastcall VM_GetUserdef(register int32_t lLabelID);
void __fastcall VM_SetUserdef(register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetActiveProjectile(register int32_t const iActor, register int32_t lLabelID);
void __fastcall VM_SetActiveProjectile(register int32_t const iActor, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetPlayer(register int32_t const iPlayer, register int32_t lLabelID, register int32_t const lParm2);
void __fastcall VM_SetPlayer(register int32_t const iPlayer, register int32_t const lLabelID, register int32_t const lParm2, register int32_t const iSet);
int32_t __fastcall VM_GetPlayerInput(register int32_t const iPlayer, register int32_t lLabelID);
void __fastcall VM_SetPlayerInput(register int32_t const iPlayer, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetWall(register int32_t const iWall, register int32_t lLabelID);
void __fastcall VM_SetWall(register int32_t const iWall, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetSector(register int32_t const iSector, register int32_t lLabelID);
void __fastcall VM_SetSector(register int32_t const iSector, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetSprite(register int32_t const iActor, register int32_t lLabelID, register int32_t const lParm2);
void __fastcall VM_SetSprite(register int32_t const iActor, register int32_t const lLabelID, register int32_t const lParm2, register int32_t const iSet);
int32_t __fastcall VM_GetTsprite(register int32_t const iActor, register int32_t lLabelID);
void __fastcall VM_SetTsprite(register int32_t const iActor, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetProjectile(register int32_t const iTile, register int32_t lLabelID);
void __fastcall VM_SetProjectile(register int32_t const iTile, register int32_t const lLabelID, register int32_t const iSet);
int32_t __fastcall VM_GetTileData(register int32_t const iTile, register int32_t lLabelID);
int32_t __fastcall VM_GetPalData(register int32_t const iPal, register int32_t lLabelID);
#else
int32_t __fastcall VM_GetUserdef(register int32_t lLabelID)
{
    switch (lLabelID)
    {
        case USERDEFS_GOD: lLabelID = ud.god; break;
        case USERDEFS_WARP_ON: lLabelID = ud.warp_on; break;
        case USERDEFS_CASHMAN: lLabelID = ud.cashman; break;
        case USERDEFS_EOG: lLabelID = ud.eog; break;
        case USERDEFS_SHOWALLMAP: lLabelID = ud.showallmap; break;
        case USERDEFS_SHOW_HELP: lLabelID = ud.show_help; break;
        case USERDEFS_SCROLLMODE: lLabelID = ud.scrollmode; break;
        case USERDEFS_CLIPPING: lLabelID = ud.noclip; break;
        //  case USERDEFS_USER_NAME: lLabelID= ud.user_name[MAXPLAYERS][32]; break;
        //  case USERDEFS_RIDECULE: lLabelID= ud.ridecule; break;
        //  case USERDEFS_SAVEGAME: lLabelID= ud.savegame; break;
        //  case USERDEFS_PWLOCKOUT: lLabelID= ud.pwlockout; break;
        //  case USERDEFS_RTSNAME: lLabelID= ud.rtsname; break;
        case USERDEFS_OVERHEAD_ON: lLabelID = ud.overhead_on; break;
        case USERDEFS_LAST_OVERHEAD: lLabelID = ud.last_overhead; break;
        case USERDEFS_SHOWWEAPONS: lLabelID = ud.showweapons; break;
        case USERDEFS_PAUSE_ON: lLabelID = ud.pause_on; break;
        case USERDEFS_FROM_BONUS: lLabelID = ud.from_bonus; break;
        case USERDEFS_CAMERASPRITE: lLabelID = ud.camerasprite; break;
        case USERDEFS_LAST_CAMSPRITE: lLabelID = ud.last_camsprite; break;
        case USERDEFS_LAST_LEVEL: lLabelID = ud.last_level; break;
        case USERDEFS_SECRETLEVEL: lLabelID = ud.secretlevel; break;
        case USERDEFS_CONST_VISIBILITY: lLabelID = ud.const_visibility; break;
        case USERDEFS_UW_FRAMERATE: lLabelID = ud.uw_framerate; break;
        case USERDEFS_CAMERA_TIME: lLabelID = ud.camera_time; break;
        case USERDEFS_FOLFVEL: lLabelID = ud.folfvel; break;
        case USERDEFS_FOLAVEL: lLabelID = ud.folavel; break;
        case USERDEFS_FOLX: lLabelID = ud.folx; break;
        case USERDEFS_FOLY: lLabelID = ud.foly; break;
        case USERDEFS_FOLA: lLabelID = ud.fola; break;
        case USERDEFS_RECCNT: lLabelID = ud.reccnt; break;
        case USERDEFS_ENTERED_NAME: lLabelID = ud.entered_name; break;
        case USERDEFS_SCREEN_TILTING: lLabelID = ud.screen_tilting; break;
        case USERDEFS_SHADOWS: lLabelID = ud.shadows; break;
        case USERDEFS_FTA_ON: lLabelID = ud.fta_on; break;
        case USERDEFS_EXECUTIONS: lLabelID = ud.executions; break;
        case USERDEFS_AUTO_RUN: lLabelID = ud.auto_run; break;
        case USERDEFS_COORDS: lLabelID = ud.coords; break;
        case USERDEFS_TICKRATE: lLabelID = ud.tickrate; break;
        case USERDEFS_M_COOP: lLabelID = ud.m_coop; break;
        case USERDEFS_COOP: lLabelID = ud.coop; break;
        case USERDEFS_SCREEN_SIZE: lLabelID = ud.screen_size; break;
        case USERDEFS_LOCKOUT: lLabelID = ud.lockout; break;
        case USERDEFS_CROSSHAIR: lLabelID = ud.crosshair; break;
        case USERDEFS_PLAYERAI: lLabelID = ud.playerai; break;
        case USERDEFS_RESPAWN_MONSTERS: lLabelID = ud.respawn_monsters; break;
        case USERDEFS_RESPAWN_ITEMS: lLabelID = ud.respawn_items; break;
        case USERDEFS_RESPAWN_INVENTORY: lLabelID = ud.respawn_inventory; break;
        case USERDEFS_RECSTAT: lLabelID = ud.recstat; break;
        case USERDEFS_MONSTERS_OFF: lLabelID = ud.monsters_off; break;
        case USERDEFS_BRIGHTNESS: lLabelID = ud.brightness; break;
        case USERDEFS_M_RESPAWN_ITEMS: lLabelID = ud.m_respawn_items; break;
        case USERDEFS_M_RESPAWN_MONSTERS: lLabelID = ud.m_respawn_monsters; break;
        case USERDEFS_M_RESPAWN_INVENTORY: lLabelID = ud.m_respawn_inventory; break;
        case USERDEFS_M_RECSTAT: lLabelID = ud.m_recstat; break;
        case USERDEFS_M_MONSTERS_OFF: lLabelID = ud.m_monsters_off; break;
        case USERDEFS_DETAIL: lLabelID = ud.detail; break;
        case USERDEFS_M_FFIRE: lLabelID = ud.m_ffire; break;
        case USERDEFS_FFIRE: lLabelID = ud.ffire; break;
        case USERDEFS_M_PLAYER_SKILL: lLabelID = ud.m_player_skill; break;
        case USERDEFS_M_LEVEL_NUMBER: lLabelID = ud.m_level_number; break;
        case USERDEFS_M_VOLUME_NUMBER: lLabelID = ud.m_volume_number; break;
        case USERDEFS_MULTIMODE: lLabelID = ud.multimode; break;
        case USERDEFS_PLAYER_SKILL: lLabelID = ud.player_skill; break;
        case USERDEFS_LEVEL_NUMBER: lLabelID = ud.level_number; break;
        case USERDEFS_VOLUME_NUMBER: lLabelID = ud.volume_number; break;
        case USERDEFS_M_MARKER: lLabelID = ud.m_marker; break;
        case USERDEFS_MARKER: lLabelID = ud.marker; break;
        case USERDEFS_MOUSEFLIP: lLabelID = ud.mouseflip; break;
        case USERDEFS_STATUSBARSCALE: lLabelID = ud.statusbarscale; break;
        case USERDEFS_DRAWWEAPON: lLabelID = ud.drawweapon; break;
        case USERDEFS_MOUSEAIMING: lLabelID = ud.mouseaiming; break;
        case USERDEFS_WEAPONSWITCH: lLabelID = ud.weaponswitch; break;
        case USERDEFS_DEMOCAMS: lLabelID = ud.democams; break;
        case USERDEFS_COLOR: lLabelID = ud.color; break;
        case USERDEFS_MSGDISPTIME: lLabelID = ud.msgdisptime; break;
        case USERDEFS_STATUSBARMODE: lLabelID = ud.statusbarmode; break;
        case USERDEFS_M_NOEXITS: lLabelID = ud.m_noexits; break;
        case USERDEFS_NOEXITS: lLabelID = ud.noexits; break;
        case USERDEFS_AUTOVOTE: lLabelID = ud.autovote; break;
        case USERDEFS_AUTOMSG: lLabelID = ud.automsg; break;
        case USERDEFS_IDPLAYERS: lLabelID = ud.idplayers; break;
        case USERDEFS_TEAM: lLabelID = ud.team; break;
        case USERDEFS_VIEWBOB: lLabelID = ud.viewbob; break;
        case USERDEFS_WEAPONSWAY: lLabelID = ud.weaponsway; break;
        case USERDEFS_ANGLEINTERPOLATION: lLabelID = ud.angleinterpolation; break;
        case USERDEFS_OBITUARIES: lLabelID = ud.obituaries; break;
        case USERDEFS_LEVELSTATS: lLabelID = ud.levelstats; break;
        case USERDEFS_CROSSHAIRSCALE: lLabelID = ud.crosshairscale; break;
        case USERDEFS_ALTHUD: lLabelID = ud.althud; break;
        case USERDEFS_DISPLAY_BONUS_SCREEN: lLabelID = ud.display_bonus_screen; break;
        case USERDEFS_SHOW_LEVEL_TEXT: lLabelID = ud.show_level_text; break;
        case USERDEFS_WEAPONSCALE: lLabelID = ud.weaponscale; break;
        case USERDEFS_TEXTSCALE: lLabelID = ud.textscale; break;
        case USERDEFS_RUNKEY_MODE: lLabelID = ud.runkey_mode; break;
        case USERDEFS_M_ORIGIN_X: lLabelID = ud.m_origin.x; break;
        case USERDEFS_M_ORIGIN_Y: lLabelID = ud.m_origin.y; break;
        case USERDEFS_PLAYERBEST: lLabelID = ud.playerbest; break;
        case USERDEFS_MUSICTOGGLE: lLabelID = ud.config.MusicToggle; break;
        case USERDEFS_USEVOXELS: lLabelID = usevoxels; break;
        case USERDEFS_USEHIGHTILE:
#ifdef USE_OPENGL
            lLabelID = usehightile;
            break;
#endif
        case USERDEFS_USEMODELS:
#ifdef USE_OPENGL
            lLabelID = usemodels;
#else
            lLabelID = 0;
#endif
            break;
        case USERDEFS_GAMETYPEFLAGS: lLabelID = GametypeFlags[ud.coop]; break;
        case USERDEFS_M_GAMETYPEFLAGS: lLabelID = GametypeFlags[ud.m_coop]; break;
        case USERDEFS_GLOBALFLAGS: lLabelID = globalflags; break;
        case USERDEFS_GLOBALGAMEFLAGS: lLabelID = duke3d_globalflags; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetUserdef(register int32_t const lLabelID, register int32_t const iSet)
{
    switch (lLabelID)
    {
        case USERDEFS_GOD: ud.god = iSet; break;
        case USERDEFS_WARP_ON: ud.warp_on = iSet; break;
        case USERDEFS_CASHMAN: ud.cashman = iSet; break;
        case USERDEFS_EOG: ud.eog = iSet; break;
        case USERDEFS_SHOWALLMAP: ud.showallmap = iSet; break;
        case USERDEFS_SHOW_HELP: ud.show_help = iSet; break;
        case USERDEFS_SCROLLMODE: ud.scrollmode = iSet; break;
        case USERDEFS_CLIPPING: ud.noclip = iSet; break;
        //  case USERDEFS_USER_NAME: ud.user_name[MAXPLAYERS][32] = lValue; break;
        //  case USERDEFS_RIDECULE: ud.ridecule = lValue; break;
        //  case USERDEFS_SAVEGAME: ud.savegame = lValue; break;
        //  case USERDEFS_PWLOCKOUT: ud.pwlockout = lValue; break;
        //  case USERDEFS_RTSNAME: ud.rtsname = lValue; break;
        case USERDEFS_OVERHEAD_ON: ud.overhead_on = iSet; break;
        case USERDEFS_LAST_OVERHEAD: ud.last_overhead = iSet; break;
        case USERDEFS_SHOWWEAPONS: ud.showweapons = iSet; break;
        case USERDEFS_PAUSE_ON: ud.pause_on = iSet; break;
        case USERDEFS_FROM_BONUS: ud.from_bonus = iSet; break;
        case USERDEFS_CAMERASPRITE: ud.camerasprite = iSet; break;
        case USERDEFS_LAST_CAMSPRITE: ud.last_camsprite = iSet; break;
        case USERDEFS_LAST_LEVEL: ud.last_level = iSet; break;
        case USERDEFS_SECRETLEVEL: ud.secretlevel = iSet; break;
        case USERDEFS_CONST_VISIBILITY: ud.const_visibility = iSet; break;
        case USERDEFS_UW_FRAMERATE: ud.uw_framerate = iSet; break;
        case USERDEFS_CAMERA_TIME: ud.camera_time = iSet; break;
        case USERDEFS_FOLFVEL: ud.folfvel = iSet; break;
        case USERDEFS_FOLAVEL: ud.folavel = iSet; break;
        case USERDEFS_FOLX: ud.folx = iSet; break;
        case USERDEFS_FOLY: ud.foly = iSet; break;
        case USERDEFS_FOLA: ud.fola = iSet; break;
        case USERDEFS_RECCNT: ud.reccnt = iSet; break;
        case USERDEFS_ENTERED_NAME: ud.entered_name = iSet; break;
        case USERDEFS_SCREEN_TILTING: ud.screen_tilting = iSet; break;
        case USERDEFS_SHADOWS: ud.shadows = iSet; break;
        case USERDEFS_FTA_ON: ud.fta_on = iSet; break;
        case USERDEFS_EXECUTIONS: ud.executions = iSet; break;
        case USERDEFS_AUTO_RUN: ud.auto_run = iSet; break;
        case USERDEFS_COORDS: ud.coords = iSet; break;
        case USERDEFS_TICKRATE: ud.tickrate = iSet; break;
        case USERDEFS_M_COOP: ud.m_coop = iSet; break;
        case USERDEFS_COOP: ud.coop = iSet; break;
        case USERDEFS_SCREEN_SIZE:
            if (ud.screen_size != iSet)
            {
                ud.screen_size = iSet;
                G_UpdateScreenArea();
            }
            break;
        case USERDEFS_LOCKOUT: ud.lockout = iSet; break;
        case USERDEFS_CROSSHAIR: ud.crosshair = iSet; break;
        case USERDEFS_PLAYERAI: ud.playerai = iSet; break;
        case USERDEFS_RESPAWN_MONSTERS: ud.respawn_monsters = iSet; break;
        case USERDEFS_RESPAWN_ITEMS: ud.respawn_items = iSet; break;
        case USERDEFS_RESPAWN_INVENTORY: ud.respawn_inventory = iSet; break;
        case USERDEFS_RECSTAT: ud.recstat = iSet; break;
        case USERDEFS_MONSTERS_OFF: ud.monsters_off = iSet; break;
        case USERDEFS_BRIGHTNESS: ud.brightness = iSet; break;
        case USERDEFS_M_RESPAWN_ITEMS: ud.m_respawn_items = iSet; break;
        case USERDEFS_M_RESPAWN_MONSTERS: ud.m_respawn_monsters = iSet; break;
        case USERDEFS_M_RESPAWN_INVENTORY: ud.m_respawn_inventory = iSet; break;
        case USERDEFS_M_RECSTAT: ud.m_recstat = iSet; break;
        case USERDEFS_M_MONSTERS_OFF: ud.m_monsters_off = iSet; break;
        // REMINDER: must implement "boolean" setters like this in Lunatic, too.
        case USERDEFS_DETAIL: ud.detail = !!iSet; break;
        case USERDEFS_M_FFIRE: ud.m_ffire = iSet; break;
        case USERDEFS_FFIRE: ud.ffire = iSet; break;
        case USERDEFS_M_PLAYER_SKILL: ud.m_player_skill = iSet; break;
        case USERDEFS_M_LEVEL_NUMBER: ud.m_level_number = iSet; break;
        case USERDEFS_M_VOLUME_NUMBER: ud.m_volume_number = iSet; break;
        case USERDEFS_MULTIMODE: ud.multimode = iSet; break;
        case USERDEFS_PLAYER_SKILL: ud.player_skill = iSet; break;
        case USERDEFS_LEVEL_NUMBER: ud.level_number = iSet; break;
        case USERDEFS_VOLUME_NUMBER: ud.volume_number = iSet; break;
        case USERDEFS_M_MARKER: ud.m_marker = iSet; break;
        case USERDEFS_MARKER: ud.marker = iSet; break;
        case USERDEFS_MOUSEFLIP: ud.mouseflip = iSet; break;
        case USERDEFS_STATUSBARSCALE: ud.statusbarscale = iSet; break;
        case USERDEFS_DRAWWEAPON: ud.drawweapon = iSet; break;
        case USERDEFS_MOUSEAIMING: ud.mouseaiming = iSet; break;
        case USERDEFS_WEAPONSWITCH: ud.weaponswitch = iSet; break;
        case USERDEFS_DEMOCAMS: ud.democams = iSet; break;
        case USERDEFS_COLOR: ud.color = iSet; break;
        case USERDEFS_MSGDISPTIME: ud.msgdisptime = iSet; break;
        case USERDEFS_STATUSBARMODE: ud.statusbarmode = iSet; break;
        case USERDEFS_M_NOEXITS: ud.m_noexits = iSet; break;
        case USERDEFS_NOEXITS: ud.noexits = iSet; break;
        case USERDEFS_AUTOVOTE: ud.autovote = iSet; break;
        case USERDEFS_AUTOMSG: ud.automsg = iSet; break;
        case USERDEFS_IDPLAYERS: ud.idplayers = iSet; break;
        case USERDEFS_TEAM: ud.team = iSet; break;
        case USERDEFS_VIEWBOB: ud.viewbob = iSet; break;
        case USERDEFS_WEAPONSWAY: ud.weaponsway = iSet; break;
        case USERDEFS_ANGLEINTERPOLATION: ud.angleinterpolation = iSet; break;
        case USERDEFS_OBITUARIES: ud.obituaries = iSet; break;
        case USERDEFS_LEVELSTATS: ud.levelstats = iSet; break;
        case USERDEFS_CROSSHAIRSCALE: ud.crosshairscale = iSet; break;
        case USERDEFS_ALTHUD: ud.althud = iSet; break;
        case USERDEFS_DISPLAY_BONUS_SCREEN: ud.display_bonus_screen = iSet; break;
        case USERDEFS_SHOW_LEVEL_TEXT: ud.show_level_text = iSet; break;
        case USERDEFS_WEAPONSCALE: ud.weaponscale = iSet; break;
        case USERDEFS_TEXTSCALE: ud.textscale = iSet; break;
        case USERDEFS_RUNKEY_MODE: ud.runkey_mode = iSet; break;
        case USERDEFS_M_ORIGIN_X: ud.m_origin.x = iSet; break;
        case USERDEFS_M_ORIGIN_Y: ud.m_origin.y = iSet; break;
        case USERDEFS_GLOBALFLAGS: globalflags = iSet; break;
        case USERDEFS_GLOBALGAMEFLAGS: duke3d_globalflags = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetActiveProjectile(register int32_t const iActor, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("tried to get %s on invalid target projectile (%d) %d %d\n",
                     ProjectileLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum));
        return -1;
    }

    switch (lLabelID)
    {
        case PROJ_WORKSLIKE: lLabelID = SpriteProjectile[iActor].workslike; break;
        case PROJ_SPAWNS: lLabelID = SpriteProjectile[iActor].spawns; break;
        case PROJ_SXREPEAT: lLabelID = SpriteProjectile[iActor].sxrepeat; break;
        case PROJ_SYREPEAT: lLabelID = SpriteProjectile[iActor].syrepeat; break;
        case PROJ_SOUND: lLabelID = SpriteProjectile[iActor].sound; break;
        case PROJ_ISOUND: lLabelID = SpriteProjectile[iActor].isound; break;
        case PROJ_VEL: lLabelID = SpriteProjectile[iActor].vel; break;
        case PROJ_EXTRA: lLabelID = SpriteProjectile[iActor].extra; break;
        case PROJ_DECAL: lLabelID = SpriteProjectile[iActor].decal; break;
        case PROJ_TRAIL: lLabelID = SpriteProjectile[iActor].trail; break;
        case PROJ_TXREPEAT: lLabelID = SpriteProjectile[iActor].txrepeat; break;
        case PROJ_TYREPEAT: lLabelID = SpriteProjectile[iActor].tyrepeat; break;
        case PROJ_TOFFSET: lLabelID = SpriteProjectile[iActor].toffset; break;
        case PROJ_TNUM: lLabelID = SpriteProjectile[iActor].tnum; break;
        case PROJ_DROP: lLabelID = SpriteProjectile[iActor].drop; break;
        case PROJ_CSTAT: lLabelID = SpriteProjectile[iActor].cstat; break;
        case PROJ_CLIPDIST: lLabelID = SpriteProjectile[iActor].clipdist; break;
        case PROJ_SHADE: lLabelID = SpriteProjectile[iActor].shade; break;
        case PROJ_XREPEAT: lLabelID = SpriteProjectile[iActor].xrepeat; break;
        case PROJ_YREPEAT: lLabelID = SpriteProjectile[iActor].yrepeat; break;
        case PROJ_PAL: lLabelID = SpriteProjectile[iActor].pal; break;
        case PROJ_EXTRA_RAND: lLabelID = SpriteProjectile[iActor].extra_rand; break;
        case PROJ_HITRADIUS: lLabelID = SpriteProjectile[iActor].hitradius; break;
        case PROJ_MOVECNT: lLabelID = SpriteProjectile[iActor].movecnt; break;
        case PROJ_OFFSET: lLabelID = SpriteProjectile[iActor].offset; break;
        case PROJ_BOUNCES: lLabelID = SpriteProjectile[iActor].bounces; break;
        case PROJ_BSOUND: lLabelID = SpriteProjectile[iActor].bsound; break;
        case PROJ_RANGE: lLabelID = SpriteProjectile[iActor].range; break;
        case PROJ_FLASH_COLOR: lLabelID = SpriteProjectile[iActor].flashcolor; break;
        case PROJ_USERDATA: lLabelID = SpriteProjectile[iActor].userdata; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetActiveProjectile(register int32_t const iActor, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("tried to set %s on invalid target projectile (%d) %d %d\n",
                     ProjectileLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum));
        return;
    }

    switch (lLabelID)
    {
        case PROJ_WORKSLIKE: SpriteProjectile[iActor].workslike = iSet; break;
        case PROJ_SPAWNS: SpriteProjectile[iActor].spawns = iSet; break;
        case PROJ_SXREPEAT: SpriteProjectile[iActor].sxrepeat = iSet; break;
        case PROJ_SYREPEAT: SpriteProjectile[iActor].syrepeat = iSet; break;
        case PROJ_SOUND: SpriteProjectile[iActor].sound = iSet; break;
        case PROJ_ISOUND: SpriteProjectile[iActor].isound = iSet; break;
        case PROJ_VEL: SpriteProjectile[iActor].vel = iSet; break;
        case PROJ_EXTRA: SpriteProjectile[iActor].extra = iSet; break;
        case PROJ_DECAL: SpriteProjectile[iActor].decal = iSet; break;
        case PROJ_TRAIL: SpriteProjectile[iActor].trail = iSet; break;
        case PROJ_TXREPEAT: SpriteProjectile[iActor].txrepeat = iSet; break;
        case PROJ_TYREPEAT: SpriteProjectile[iActor].tyrepeat = iSet; break;
        case PROJ_TOFFSET: SpriteProjectile[iActor].toffset = iSet; break;
        case PROJ_TNUM: SpriteProjectile[iActor].tnum = iSet; break;
        case PROJ_DROP: SpriteProjectile[iActor].drop = iSet; break;
        case PROJ_CSTAT: SpriteProjectile[iActor].cstat = iSet; break;
        case PROJ_CLIPDIST: SpriteProjectile[iActor].clipdist = iSet; break;
        case PROJ_SHADE: SpriteProjectile[iActor].shade = iSet; break;
        case PROJ_XREPEAT: SpriteProjectile[iActor].xrepeat = iSet; break;
        case PROJ_YREPEAT: SpriteProjectile[iActor].yrepeat = iSet; break;
        case PROJ_PAL: SpriteProjectile[iActor].pal = iSet; break;
        case PROJ_EXTRA_RAND: SpriteProjectile[iActor].extra_rand = iSet; break;
        case PROJ_HITRADIUS: SpriteProjectile[iActor].hitradius = iSet; break;
        case PROJ_MOVECNT: SpriteProjectile[iActor].movecnt = iSet; break;
        case PROJ_OFFSET: SpriteProjectile[iActor].offset = iSet; break;
        case PROJ_BOUNCES: SpriteProjectile[iActor].bounces = iSet; break;
        case PROJ_BSOUND: SpriteProjectile[iActor].bsound = iSet; break;
        case PROJ_RANGE: SpriteProjectile[iActor].range = iSet; break;
        case PROJ_FLASH_COLOR: SpriteProjectile[iActor].flashcolor = iSet; break;
        case PROJ_USERDATA: SpriteProjectile[iActor].userdata = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetPlayer(register int32_t const iPlayer, register int32_t lLabelID, register int32_t const lParm2)
{
    DukePlayer_t *const ps = g_player[iPlayer].ps;

    if (EDUKE32_PREDICT_FALSE((unsigned)iPlayer >= (unsigned)playerswhenstarted))
    {
        CON_ERRPRINTF("tried to get %s on invalid target player (%d) from spr %d\n",
                      PlayerLabels[lLabelID].name,iPlayer,vm.g_i);
        return -1;
    }

    if (EDUKE32_PREDICT_FALSE(PlayerLabels[lLabelID].flags & LABEL_HASPARM2 &&
        (unsigned)lParm2 >= (unsigned)PlayerLabels[lLabelID].maxParm2))
    {
        CON_ERRPRINTF("tried to get invalid %s position %d on player (%d) from spr %d\n",
                      PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
        return -1;
    }

    switch (lLabelID)
    {
        case PLAYER_ZOOM: lLabelID = ps->zoom; break;
        case PLAYER_EXITX: lLabelID = ps->exitx; break;
        case PLAYER_EXITY: lLabelID = ps->exity; break;
        case PLAYER_LOOGIEX: lLabelID = ps->loogiex[lParm2]; break;
        case PLAYER_LOOGIEY: lLabelID = ps->loogiey[lParm2]; break;
        case PLAYER_NUMLOOGS: lLabelID = ps->numloogs; break;
        case PLAYER_LOOGCNT: lLabelID = ps->loogcnt; break;
        case PLAYER_POSX: lLabelID = ps->pos.x; break;
        case PLAYER_POSY: lLabelID = ps->pos.y; break;
        case PLAYER_POSZ: lLabelID = ps->pos.z; break;
        case PLAYER_HORIZ: lLabelID = ps->horiz; break;
        case PLAYER_OHORIZ: lLabelID = ps->ohoriz; break;
        case PLAYER_OHORIZOFF: lLabelID = ps->ohorizoff; break;
        case PLAYER_INVDISPTIME: lLabelID = ps->invdisptime; break;
        case PLAYER_BOBPOSX: lLabelID = ps->bobpos.x; break;
        case PLAYER_BOBPOSY: lLabelID = ps->bobpos.y; break;
        case PLAYER_OPOSX: lLabelID = ps->opos.x; break;
        case PLAYER_OPOSY: lLabelID = ps->opos.y; break;
        case PLAYER_OPOSZ: lLabelID = ps->opos.z; break;
        case PLAYER_PYOFF: lLabelID = ps->pyoff; break;
        case PLAYER_OPYOFF: lLabelID = ps->opyoff; break;
        case PLAYER_POSXV: lLabelID = ps->vel.x; break;
        case PLAYER_POSYV: lLabelID = ps->vel.y; break;
        case PLAYER_POSZV: lLabelID = ps->vel.z; break;
        case PLAYER_LAST_PISSED_TIME: lLabelID = ps->last_pissed_time; break;
        case PLAYER_TRUEFZ: lLabelID = ps->truefz; break;
        case PLAYER_TRUECZ: lLabelID = ps->truecz; break;
        case PLAYER_PLAYER_PAR: lLabelID = ps->player_par; break;
        case PLAYER_VISIBILITY: lLabelID = ps->visibility; break;
        case PLAYER_BOBCOUNTER: lLabelID = ps->bobcounter; break;
        case PLAYER_WEAPON_SWAY: lLabelID = ps->weapon_sway; break;
        case PLAYER_PALS_TIME: lLabelID = ps->pals.f; break;
        case PLAYER_RANDOMFLAMEX: lLabelID = ps->randomflamex; break;
        case PLAYER_CRACK_TIME: lLabelID = ps->crack_time; break;
        case PLAYER_AIM_MODE: lLabelID = ps->aim_mode; break;
        case PLAYER_ANG: lLabelID = ps->ang; break;
        case PLAYER_OANG: lLabelID = ps->oang; break;
        case PLAYER_ANGVEL: lLabelID = ps->angvel; break;
        case PLAYER_CURSECTNUM: lLabelID = ps->cursectnum; break;
        case PLAYER_LOOK_ANG: lLabelID = ps->look_ang; break;
        case PLAYER_LAST_EXTRA: lLabelID = ps->last_extra; break;
        case PLAYER_SUBWEAPON: lLabelID = ps->subweapon; break;
        case PLAYER_AMMO_AMOUNT: lLabelID = ps->ammo_amount[lParm2]; break;
        case PLAYER_WACKEDBYACTOR: lLabelID = ps->wackedbyactor; break;
        case PLAYER_FRAG: lLabelID = ps->frag; break;
        case PLAYER_FRAGGEDSELF: lLabelID = ps->fraggedself; break;
        case PLAYER_CURR_WEAPON: lLabelID = ps->curr_weapon; break;
        case PLAYER_LAST_WEAPON: lLabelID = ps->last_weapon; break;
        case PLAYER_TIPINCS: lLabelID = ps->tipincs; break;
        case PLAYER_HORIZOFF: lLabelID = ps->horizoff; break;
        case PLAYER_WANTWEAPONFIRE: lLabelID = ps->wantweaponfire; break;
        case PLAYER_HOLODUKE_AMOUNT: lLabelID = ps->inv_amount[GET_HOLODUKE]; break;
        case PLAYER_NEWOWNER: lLabelID = ps->newowner; break;
        case PLAYER_HURT_DELAY: lLabelID = ps->hurt_delay; break;
        case PLAYER_HBOMB_HOLD_DELAY: lLabelID = ps->hbomb_hold_delay; break;
        case PLAYER_JUMPING_COUNTER: lLabelID = ps->jumping_counter; break;
        case PLAYER_AIRLEFT: lLabelID = ps->airleft; break;
        case PLAYER_KNEE_INCS: lLabelID = ps->knee_incs; break;
        case PLAYER_ACCESS_INCS: lLabelID = ps->access_incs; break;
        case PLAYER_FTA: lLabelID = ps->fta; break;
        case PLAYER_FTQ: lLabelID = ps->ftq; break;
        case PLAYER_ACCESS_WALLNUM: lLabelID = ps->access_wallnum; break;
        case PLAYER_ACCESS_SPRITENUM: lLabelID = ps->access_spritenum; break;
        case PLAYER_KICKBACK_PIC: lLabelID = ps->kickback_pic; break;
        case PLAYER_GOT_ACCESS: lLabelID = ps->got_access; break;
        case PLAYER_WEAPON_ANG: lLabelID = ps->weapon_ang; break;
        case PLAYER_FIRSTAID_AMOUNT: lLabelID = ps->inv_amount[GET_FIRSTAID]; break;
        case PLAYER_SOMETHINGONPLAYER: lLabelID = ps->somethingonplayer; break;
        case PLAYER_ON_CRANE: lLabelID = ps->on_crane; break;
        case PLAYER_I: lLabelID = ps->i; break;
        case PLAYER_ONE_PARALLAX_SECTNUM: lLabelID = ps->one_parallax_sectnum; break;
        case PLAYER_OVER_SHOULDER_ON: lLabelID = ps->over_shoulder_on; break;
        case PLAYER_RANDOM_CLUB_FRAME: lLabelID = ps->random_club_frame; break;
        case PLAYER_FIST_INCS: lLabelID = ps->fist_incs; break;
        case PLAYER_ONE_EIGHTY_COUNT: lLabelID = ps->one_eighty_count; break;
        case PLAYER_CHEAT_PHASE: lLabelID = ps->cheat_phase; break;
        case PLAYER_DUMMYPLAYERSPRITE: lLabelID = ps->dummyplayersprite; break;
        case PLAYER_EXTRA_EXTRA8: lLabelID = ps->extra_extra8; break;
        case PLAYER_QUICK_KICK: lLabelID = ps->quick_kick; break;
        case PLAYER_HEAT_AMOUNT: lLabelID = ps->inv_amount[GET_HEATS]; break;
        case PLAYER_ACTORSQU: lLabelID = ps->actorsqu; break;
        case PLAYER_TIMEBEFOREEXIT: lLabelID = ps->timebeforeexit; break;
        case PLAYER_CUSTOMEXITSOUND: lLabelID = ps->customexitsound; break;
        case PLAYER_WEAPRECS: lLabelID = ps->weaprecs[lParm2]; break;
        case PLAYER_WEAPRECCNT: lLabelID = ps->weapreccnt; break;
        case PLAYER_INTERFACE_TOGGLE_FLAG: lLabelID = ps->interface_toggle_flag; break;
        case PLAYER_ROTSCRNANG: lLabelID = ps->rotscrnang; break;
        case PLAYER_DEAD_FLAG: lLabelID = ps->dead_flag; break;
        case PLAYER_SHOW_EMPTY_WEAPON: lLabelID = ps->show_empty_weapon; break;
        case PLAYER_SCUBA_AMOUNT: lLabelID = ps->inv_amount[GET_SCUBA]; break;
        case PLAYER_JETPACK_AMOUNT: lLabelID = ps->inv_amount[GET_JETPACK]; break;
        case PLAYER_STEROIDS_AMOUNT: lLabelID = ps->inv_amount[GET_STEROIDS]; break;
        case PLAYER_SHIELD_AMOUNT: lLabelID = ps->inv_amount[GET_SHIELD]; break;
        case PLAYER_HOLODUKE_ON: lLabelID = ps->holoduke_on; break;
        case PLAYER_PYCOUNT: lLabelID = ps->pycount; break;
        case PLAYER_WEAPON_POS: lLabelID = ps->weapon_pos; break;
        case PLAYER_FRAG_PS: lLabelID = ps->frag_ps; break;
        case PLAYER_TRANSPORTER_HOLD: lLabelID = ps->transporter_hold; break;
        case PLAYER_LAST_FULL_WEAPON: lLabelID = ps->last_full_weapon; break;
        case PLAYER_FOOTPRINTSHADE: lLabelID = ps->footprintshade; break;
        case PLAYER_BOOT_AMOUNT: lLabelID = ps->inv_amount[GET_BOOTS]; break;
        case PLAYER_SCREAM_VOICE: lLabelID = ps->scream_voice; break;
        case PLAYER_GM: lLabelID = ps->gm; break;
        case PLAYER_ON_WARPING_SECTOR: lLabelID = ps->on_warping_sector; break;
        case PLAYER_FOOTPRINTCOUNT: lLabelID = ps->footprintcount; break;
        case PLAYER_HBOMB_ON: lLabelID = ps->hbomb_on; break;
        case PLAYER_JUMPING_TOGGLE: lLabelID = ps->jumping_toggle; break;
        case PLAYER_RAPID_FIRE_HOLD: lLabelID = ps->rapid_fire_hold; break;
        case PLAYER_ON_GROUND: lLabelID = ps->on_ground; break;
        case PLAYER_INVEN_ICON: lLabelID = ps->inven_icon; break;
        case PLAYER_BUTTONPALETTE: lLabelID = ps->buttonpalette; break;
        case PLAYER_JETPACK_ON: lLabelID = ps->jetpack_on; break;
        case PLAYER_SPRITEBRIDGE: lLabelID = ps->spritebridge; break;
        case PLAYER_LASTRANDOMSPOT: lLabelID = ps->lastrandomspot; break;
        case PLAYER_SCUBA_ON: lLabelID = ps->scuba_on; break;
        case PLAYER_FOOTPRINTPAL: lLabelID = ps->footprintpal; break;
        case PLAYER_HEAT_ON: lLabelID = ps->heat_on; break;
        case PLAYER_HOLSTER_WEAPON: lLabelID = ps->holster_weapon; break;
        case PLAYER_FALLING_COUNTER: lLabelID = ps->falling_counter; break;
        case PLAYER_GOTWEAPON: lLabelID = (ps->gotweapon & (1 << lParm2)) != 0; break;
        case PLAYER_REFRESH_INVENTORY: lLabelID = ps->refresh_inventory; break;
        case PLAYER_PALETTE: lLabelID = ps->palette; break;
        case PLAYER_TOGGLE_KEY_FLAG: lLabelID = ps->toggle_key_flag; break;
        case PLAYER_KNUCKLE_INCS: lLabelID = ps->knuckle_incs; break;
        case PLAYER_WALKING_SND_TOGGLE: lLabelID = ps->walking_snd_toggle; break;
        case PLAYER_PALOOKUP: lLabelID = ps->palookup; break;
        case PLAYER_HARD_LANDING: lLabelID = ps->hard_landing; break;
        case PLAYER_MAX_SECRET_ROOMS: lLabelID = ps->max_secret_rooms; break;
        case PLAYER_SECRET_ROOMS: lLabelID = ps->secret_rooms; break;
        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: lLabelID = ps->pals.r; break;
                case 1: lLabelID = ps->pals.g; break;
                case 2: lLabelID = ps->pals.b; break;
                default: lLabelID = -1; break;
            }
            break;
        case PLAYER_MAX_ACTORS_KILLED: lLabelID = ps->max_actors_killed; break;
        case PLAYER_ACTORS_KILLED: lLabelID = ps->actors_killed; break;
        case PLAYER_RETURN_TO_CENTER: lLabelID = ps->return_to_center; break;
        case PLAYER_RUNSPEED: lLabelID = ps->runspeed; break;
        case PLAYER_SBS: lLabelID = ps->sbs; break;
        case PLAYER_RELOADING: lLabelID = ps->reloading; break;
        case PLAYER_AUTO_AIM: lLabelID = ps->auto_aim; break;
        case PLAYER_MOVEMENT_LOCK: lLabelID = ps->movement_lock; break;
        case PLAYER_SOUND_PITCH: lLabelID = ps->sound_pitch; break;
        case PLAYER_WEAPONSWITCH: lLabelID = ps->weaponswitch; break;
        case PLAYER_TEAM: lLabelID = ps->team; break;
        case PLAYER_MAX_PLAYER_HEALTH: lLabelID = ps->max_player_health; break;
        case PLAYER_MAX_SHIELD_AMOUNT: lLabelID = ps->max_shield_amount; break;
        case PLAYER_MAX_AMMO_AMOUNT: lLabelID = ps->max_ammo_amount[lParm2]; break;
        case PLAYER_LAST_QUICK_KICK: lLabelID = ps->last_quick_kick; break;
        case PLAYER_AUTOSTEP: lLabelID = ps->autostep; break;
        case PLAYER_AUTOSTEP_SBW: lLabelID = ps->autostep_sbw; break;
        case PLAYER_HUDPAL: lLabelID = P_GetHudPal(ps); break;
        case PLAYER_INDEX: lLabelID = iPlayer; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetPlayer(register int32_t const iPlayer, register int32_t const lLabelID, register int32_t const lParm2, register int32_t const iSet)
{
    DukePlayer_t * const ps = g_player[iPlayer].ps;

    if (EDUKE32_PREDICT_FALSE((unsigned)iPlayer >= (unsigned)playerswhenstarted))
    {
        CON_ERRPRINTF("tried to set %s on invalid target player (%d) from spr %d\n",
                      PlayerLabels[lLabelID].name,iPlayer,vm.g_i);
        return;
    }

    if (EDUKE32_PREDICT_FALSE(PlayerLabels[lLabelID].flags & LABEL_HASPARM2 &&
                              (unsigned)lParm2 >= (unsigned)PlayerLabels[lLabelID].maxParm2))
    {
        CON_ERRPRINTF("tried to set invalid %s position %d on player (%d) from spr %d\n",
                      PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
        return;
    }

    switch (lLabelID)
    {
        case PLAYER_ZOOM: ps->zoom = iSet; break;
        case PLAYER_EXITX: ps->exitx = iSet; break;
        case PLAYER_EXITY: ps->exity = iSet; break;
        case PLAYER_LOOGIEX: ps->loogiex[lParm2] = iSet; break;
        case PLAYER_LOOGIEY: ps->loogiey[lParm2] = iSet; break;
        case PLAYER_NUMLOOGS: ps->numloogs = iSet; break;
        case PLAYER_LOOGCNT: ps->loogcnt = iSet; break;
        case PLAYER_POSX: ps->pos.x = iSet; break;
        case PLAYER_POSY: ps->pos.y = iSet; break;
        case PLAYER_POSZ: ps->pos.z = iSet; break;
        case PLAYER_HORIZ: ps->horiz = iSet; break;
        case PLAYER_OHORIZ: ps->ohoriz = iSet; break;
        case PLAYER_OHORIZOFF: ps->ohorizoff = iSet; break;
        case PLAYER_INVDISPTIME: ps->invdisptime = iSet; break;
        case PLAYER_BOBPOSX: ps->bobpos.x = iSet; break;
        case PLAYER_BOBPOSY: ps->bobpos.y = iSet; break;
        case PLAYER_OPOSX: ps->opos.x = iSet; break;
        case PLAYER_OPOSY: ps->opos.y = iSet; break;
        case PLAYER_OPOSZ: ps->opos.z = iSet; break;
        case PLAYER_PYOFF: ps->pyoff = iSet; break;
        case PLAYER_OPYOFF: ps->opyoff = iSet; break;
        case PLAYER_POSXV: ps->vel.x = iSet; break;
        case PLAYER_POSYV: ps->vel.y = iSet; break;
        case PLAYER_POSZV: ps->vel.z = iSet; break;
        case PLAYER_LAST_PISSED_TIME: ps->last_pissed_time = iSet; break;
        case PLAYER_TRUEFZ: ps->truefz = iSet; break;
        case PLAYER_TRUECZ: ps->truecz = iSet; break;
        case PLAYER_PLAYER_PAR: ps->player_par = iSet; break;
        case PLAYER_VISIBILITY: ps->visibility = iSet; break;
        case PLAYER_BOBCOUNTER: ps->bobcounter = iSet; break;
        case PLAYER_WEAPON_SWAY: ps->weapon_sway = iSet; break;
        case PLAYER_PALS_TIME: ps->pals.f = iSet; break;
        case PLAYER_RANDOMFLAMEX: ps->randomflamex = iSet; break;
        case PLAYER_CRACK_TIME: ps->crack_time = iSet; break;
        case PLAYER_AIM_MODE: ps->aim_mode = iSet; break;
        case PLAYER_ANG: ps->ang = iSet; break;
        case PLAYER_OANG: ps->oang = iSet; break;
        case PLAYER_ANGVEL: ps->angvel = iSet; break;
        case PLAYER_CURSECTNUM: ps->cursectnum = iSet; break;
        case PLAYER_LOOK_ANG: ps->look_ang = iSet; break;
        case PLAYER_LAST_EXTRA: ps->last_extra = iSet; break;
        case PLAYER_SUBWEAPON: ps->subweapon = iSet; break;
        case PLAYER_AMMO_AMOUNT: ps->ammo_amount[lParm2] = iSet; break;
        case PLAYER_WACKEDBYACTOR: ps->wackedbyactor = iSet; break;
        case PLAYER_FRAG: ps->frag = iSet; break;
        case PLAYER_FRAGGEDSELF: ps->fraggedself = iSet; break;
        case PLAYER_CURR_WEAPON: ps->curr_weapon = iSet; break;
        case PLAYER_LAST_WEAPON: ps->last_weapon = iSet; break;
        case PLAYER_TIPINCS: ps->tipincs = iSet; break;
        case PLAYER_HORIZOFF: ps->horizoff = iSet; break;
        case PLAYER_WANTWEAPONFIRE: ps->wantweaponfire = iSet; break;
        case PLAYER_HOLODUKE_AMOUNT: ps->inv_amount[GET_HOLODUKE] = iSet; break;
        case PLAYER_NEWOWNER: ps->newowner = iSet; break;
        case PLAYER_HURT_DELAY: ps->hurt_delay = iSet; break;
        case PLAYER_HBOMB_HOLD_DELAY: ps->hbomb_hold_delay = iSet; break;
        case PLAYER_JUMPING_COUNTER: ps->jumping_counter = iSet; break;
        case PLAYER_AIRLEFT: ps->airleft = iSet; break;
        case PLAYER_KNEE_INCS: ps->knee_incs = iSet; break;
        case PLAYER_ACCESS_INCS: ps->access_incs = iSet; break;
        case PLAYER_FTA: ps->fta = iSet; break;
        case PLAYER_FTQ: ps->ftq = iSet; break;
        case PLAYER_ACCESS_WALLNUM: ps->access_wallnum = iSet; break;
        case PLAYER_ACCESS_SPRITENUM: ps->access_spritenum = iSet; break;
        case PLAYER_KICKBACK_PIC: ps->kickback_pic = iSet; break;
        case PLAYER_GOT_ACCESS: ps->got_access = iSet; break;
        case PLAYER_WEAPON_ANG: ps->weapon_ang = iSet; break;
        case PLAYER_FIRSTAID_AMOUNT: ps->inv_amount[GET_FIRSTAID] = iSet; break;
        case PLAYER_SOMETHINGONPLAYER: ps->somethingonplayer = iSet; break;
        case PLAYER_ON_CRANE: ps->on_crane = iSet; break;
        case PLAYER_I: ps->i = iSet; break;
        case PLAYER_ONE_PARALLAX_SECTNUM: ps->one_parallax_sectnum = iSet; break;
        case PLAYER_OVER_SHOULDER_ON: ps->over_shoulder_on = iSet; break;
        case PLAYER_RANDOM_CLUB_FRAME: ps->random_club_frame = iSet; break;
        case PLAYER_FIST_INCS: ps->fist_incs = iSet; break;
        case PLAYER_ONE_EIGHTY_COUNT: ps->one_eighty_count = iSet; break;
        case PLAYER_CHEAT_PHASE: ps->cheat_phase = iSet; break;
        case PLAYER_DUMMYPLAYERSPRITE: ps->dummyplayersprite = iSet; break;
        case PLAYER_EXTRA_EXTRA8: ps->extra_extra8 = iSet; break;
        case PLAYER_QUICK_KICK: ps->quick_kick = iSet; break;
        case PLAYER_HEAT_AMOUNT: ps->inv_amount[GET_HEATS] = iSet; break;
        case PLAYER_ACTORSQU: ps->actorsqu = iSet; break;
        case PLAYER_TIMEBEFOREEXIT: ps->timebeforeexit = iSet; break;
        case PLAYER_CUSTOMEXITSOUND: ps->customexitsound = iSet; break;
        case PLAYER_WEAPRECS: ps->weaprecs[lParm2] = iSet; break;
        case PLAYER_WEAPRECCNT: ps->weapreccnt = iSet; break;
        case PLAYER_INTERFACE_TOGGLE_FLAG: ps->interface_toggle_flag = iSet; break;
        case PLAYER_ROTSCRNANG: ps->rotscrnang = iSet; break;
        case PLAYER_DEAD_FLAG: ps->dead_flag = iSet; break;
        case PLAYER_SHOW_EMPTY_WEAPON: ps->show_empty_weapon = iSet; break;
        case PLAYER_SCUBA_AMOUNT: ps->inv_amount[GET_SCUBA] = iSet; break;
        case PLAYER_JETPACK_AMOUNT: ps->inv_amount[GET_JETPACK] = iSet; break;
        case PLAYER_STEROIDS_AMOUNT: ps->inv_amount[GET_STEROIDS] = iSet; break;
        case PLAYER_SHIELD_AMOUNT: ps->inv_amount[GET_SHIELD] = iSet; break;
        case PLAYER_HOLODUKE_ON: ps->holoduke_on = iSet; break;
        case PLAYER_PYCOUNT: ps->pycount = iSet; break;
        case PLAYER_WEAPON_POS: ps->weapon_pos = iSet; break;
        case PLAYER_FRAG_PS: ps->frag_ps = iSet; break;
        case PLAYER_TRANSPORTER_HOLD: ps->transporter_hold = iSet; break;
        case PLAYER_LAST_FULL_WEAPON: ps->last_full_weapon = iSet; break;
        case PLAYER_FOOTPRINTSHADE: ps->footprintshade = iSet; break;
        case PLAYER_BOOT_AMOUNT: ps->inv_amount[GET_BOOTS] = iSet; break;
        case PLAYER_SCREAM_VOICE: ps->scream_voice = iSet; break;
        case PLAYER_GM:
            if (!(ps->gm & MODE_MENU) && (iSet & MODE_MENU))
                M_OpenMenu(iPlayer);
            else if ((ps->gm & MODE_MENU) && !(iSet & MODE_MENU))
                M_CloseMenu(iPlayer);
            ps->gm = iSet;
            break;
        case PLAYER_ON_WARPING_SECTOR: ps->on_warping_sector = iSet; break;
        case PLAYER_FOOTPRINTCOUNT: ps->footprintcount = iSet; break;
        case PLAYER_HBOMB_ON: ps->hbomb_on = iSet; break;
        case PLAYER_JUMPING_TOGGLE: ps->jumping_toggle = iSet; break;
        case PLAYER_RAPID_FIRE_HOLD: ps->rapid_fire_hold = iSet; break;
        case PLAYER_ON_GROUND: ps->on_ground = iSet; break;
        case PLAYER_INVEN_ICON: ps->inven_icon = iSet; break;
        case PLAYER_BUTTONPALETTE: ps->buttonpalette = iSet; break;
        case PLAYER_JETPACK_ON: ps->jetpack_on = iSet; break;
        case PLAYER_SPRITEBRIDGE: ps->spritebridge = iSet; break;
        case PLAYER_LASTRANDOMSPOT: ps->lastrandomspot = iSet; break;
        case PLAYER_SCUBA_ON: ps->scuba_on = iSet; break;
        case PLAYER_FOOTPRINTPAL: ps->footprintpal = iSet; break;
        case PLAYER_HEAT_ON:
            if (ps->heat_on != iSet)
            {
                ps->heat_on = iSet;
                P_UpdateScreenPal(ps);
            }
            break;
        case PLAYER_HOLSTER_WEAPON: ps->holster_weapon = iSet; break;
        case PLAYER_FALLING_COUNTER: ps->falling_counter = iSet; break;
        case PLAYER_GOTWEAPON:
            if (iSet) ps->gotweapon |= (1 << lParm2);
            else ps->gotweapon &= ~(1 << lParm2);
            break;
        case PLAYER_REFRESH_INVENTORY: ps->refresh_inventory = iSet; break;
        case PLAYER_PALETTE: P_SetGamePalette(ps, iSet, 2+16); break;
        case PLAYER_TOGGLE_KEY_FLAG: ps->toggle_key_flag = iSet; break;
        case PLAYER_KNUCKLE_INCS: ps->knuckle_incs = iSet; break;
        case PLAYER_WALKING_SND_TOGGLE: ps->walking_snd_toggle = iSet; break;
        case PLAYER_PALOOKUP: ps->palookup = iSet; break;
        case PLAYER_HARD_LANDING: ps->hard_landing = iSet; break;
        case PLAYER_MAX_SECRET_ROOMS: ps->max_secret_rooms = iSet; break;
        case PLAYER_SECRET_ROOMS: ps->secret_rooms = iSet; break;
        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: ps->pals.r = iSet; break;
                case 1: ps->pals.g = iSet; break;
                case 2: ps->pals.b = iSet; break;
            }
            break;
        case PLAYER_MAX_ACTORS_KILLED: ps->max_actors_killed = iSet; break;
        case PLAYER_ACTORS_KILLED: ps->actors_killed = iSet; break;
        case PLAYER_RETURN_TO_CENTER: ps->return_to_center = iSet; break;
        case PLAYER_RUNSPEED: ps->runspeed = iSet; break;
        case PLAYER_SBS: ps->sbs = iSet; break;
        case PLAYER_RELOADING: ps->reloading = iSet; break;
        case PLAYER_AUTO_AIM: ps->auto_aim = iSet; break;
        case PLAYER_MOVEMENT_LOCK: ps->movement_lock = iSet; break;
        case PLAYER_SOUND_PITCH: ps->sound_pitch = iSet; break;
        case PLAYER_WEAPONSWITCH: ps->weaponswitch = iSet; break;
        case PLAYER_TEAM: ps->team = iSet; break;
        case PLAYER_MAX_PLAYER_HEALTH: ps->max_player_health = iSet; break;
        case PLAYER_MAX_SHIELD_AMOUNT: ps->max_shield_amount = iSet; break;
        case PLAYER_MAX_AMMO_AMOUNT: ps->max_ammo_amount[lParm2] = iSet; break;
        case PLAYER_LAST_QUICK_KICK: ps->last_quick_kick = iSet; break;
        case PLAYER_AUTOSTEP: ps->autostep = iSet; break;
        case PLAYER_AUTOSTEP_SBW: ps->autostep_sbw = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetPlayerInput(register int32_t const iPlayer, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iPlayer >= (unsigned)playerswhenstarted))
    {
        CON_ERRPRINTF("VM_GetPlayerInput: invalid target player (%d) %d\n", iPlayer,vm.g_i);
        return -1;
    }

    switch (lLabelID)
    {
        case INPUT_AVEL: lLabelID = g_player[iPlayer].sync->avel; break;
        case INPUT_HORZ: lLabelID = g_player[iPlayer].sync->horz; break;
        case INPUT_FVEL: lLabelID = g_player[iPlayer].sync->fvel; break;
        case INPUT_SVEL: lLabelID = g_player[iPlayer].sync->svel; break;
        case INPUT_BITS: lLabelID = g_player[iPlayer].sync->bits; break;
        case INPUT_EXTBITS: lLabelID = g_player[iPlayer].sync->extbits; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetPlayerInput(register int32_t const iPlayer, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iPlayer >= (unsigned)playerswhenstarted))
    {
        CON_ERRPRINTF("VM_SetPlayerInput: invalid target player (%d) %d\n", iPlayer,vm.g_i);
        return;
    }

    switch (lLabelID)
    {
        case INPUT_AVEL: g_player[iPlayer].sync->avel = iSet; break;
        case INPUT_HORZ: g_player[iPlayer].sync->horz = iSet; break;
        case INPUT_FVEL: g_player[iPlayer].sync->fvel = iSet; break;
        case INPUT_SVEL: g_player[iPlayer].sync->svel = iSet; break;
        case INPUT_BITS: g_player[iPlayer].sync->bits = iSet; break;
        case INPUT_EXTBITS: g_player[iPlayer].sync->extbits = iSet; break;
        default: break;
    }
}



int32_t __fastcall VM_GetWall(register int32_t const iWall, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iWall >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("VM_GetWall: Invalid wall %d\n", iWall);
        return -1;
    }

    switch (lLabelID)
    {
        case WALL_X: lLabelID = wall[iWall].x; break;
        case WALL_Y: lLabelID = wall[iWall].y; break;
        case WALL_POINT2: lLabelID = wall[iWall].point2; break;
        case WALL_NEXTWALL: lLabelID = wall[iWall].nextwall; break;
        case WALL_NEXTSECTOR: lLabelID = wall[iWall].nextsector; break;
        case WALL_CSTAT: lLabelID = wall[iWall].cstat; break;
        case WALL_PICNUM: lLabelID = wall[iWall].picnum; break;
        case WALL_OVERPICNUM: lLabelID = wall[iWall].overpicnum; break;
        case WALL_SHADE: lLabelID = wall[iWall].shade; break;
        case WALL_PAL: lLabelID = wall[iWall].pal; break;
        case WALL_XREPEAT: lLabelID = wall[iWall].xrepeat; break;
        case WALL_YREPEAT: lLabelID = wall[iWall].yrepeat; break;
        case WALL_XPANNING: lLabelID = wall[iWall].xpanning; break;
        case WALL_YPANNING: lLabelID = wall[iWall].ypanning; break;
        case WALL_LOTAG: lLabelID = (int16_t)wall[iWall].lotag; break;
        case WALL_HITAG: lLabelID = (int16_t)wall[iWall].hitag; break;
        case WALL_ULOTAG: lLabelID = wall[iWall].lotag; break;
        case WALL_UHITAG: lLabelID = wall[iWall].hitag; break;
        case WALL_EXTRA: lLabelID = wall[iWall].extra; break;
        default: lLabelID = -1;
    }

    return lLabelID;
}

void __fastcall VM_SetWall(register int32_t const iWall, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iWall >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("VM_SetWall: Invalid wall %d\n", iWall);
        return;
    }

    switch (lLabelID)
    {
        case WALL_X: wall[iWall].x = iSet; break;
        case WALL_Y: wall[iWall].y = iSet; break;
        case WALL_POINT2: wall[iWall].point2 = iSet; break;
        case WALL_NEXTWALL: wall[iWall].nextwall = iSet; break;
        case WALL_NEXTSECTOR: wall[iWall].nextsector = iSet; break;
        case WALL_CSTAT: wall[iWall].cstat = iSet; break;
        case WALL_PICNUM: wall[iWall].picnum = iSet; break;
        case WALL_OVERPICNUM: wall[iWall].overpicnum = iSet; break;
        case WALL_SHADE: wall[iWall].shade = iSet; break;
        case WALL_PAL: wall[iWall].pal = iSet; break;
        case WALL_XREPEAT: wall[iWall].xrepeat = iSet; break;
        case WALL_YREPEAT: wall[iWall].yrepeat = iSet; break;
        case WALL_XPANNING: wall[iWall].xpanning = iSet; break;
        case WALL_YPANNING: wall[iWall].ypanning = iSet; break;
        case WALL_LOTAG: wall[iWall].lotag = (int16_t)iSet; break;
        case WALL_HITAG: wall[iWall].hitag = (int16_t)iSet; break;
        case WALL_ULOTAG: wall[iWall].lotag = iSet; break;
        case WALL_UHITAG: wall[iWall].hitag = iSet; break;
        case WALL_EXTRA: wall[iWall].extra = iSet; break;
    }

    return;
}

int32_t __fastcall VM_GetSector(register int32_t const iSector, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iSector >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("VM_GetSector: Invalid sector %d\n", iSector);
        return -1;
    }

    switch (lLabelID)
    {
        case SECTOR_WALLPTR: lLabelID = sector[iSector].wallptr; break;
        case SECTOR_WALLNUM: lLabelID = sector[iSector].wallnum; break;
        case SECTOR_CEILINGZ: lLabelID = sector[iSector].ceilingz; break;
        case SECTOR_FLOORZ: lLabelID = sector[iSector].floorz; break;
        case SECTOR_CEILINGSTAT: lLabelID = sector[iSector].ceilingstat; break;
        case SECTOR_FLOORSTAT: lLabelID = sector[iSector].floorstat; break;
        case SECTOR_CEILINGPICNUM: lLabelID = sector[iSector].ceilingpicnum; break;
        case SECTOR_CEILINGSLOPE: lLabelID = sector[iSector].ceilingheinum; break;
        case SECTOR_CEILINGSHADE: lLabelID = sector[iSector].ceilingshade; break;
        case SECTOR_CEILINGPAL: lLabelID = sector[iSector].ceilingpal; break;
        case SECTOR_CEILINGXPANNING: lLabelID = sector[iSector].ceilingxpanning; break;
        case SECTOR_CEILINGYPANNING: lLabelID = sector[iSector].ceilingypanning; break;
        case SECTOR_FLOORPICNUM: lLabelID = sector[iSector].floorpicnum; break;
        case SECTOR_FLOORSLOPE: lLabelID = sector[iSector].floorheinum; break;
        case SECTOR_FLOORSHADE: lLabelID = sector[iSector].floorshade; break;
        case SECTOR_FLOORPAL: lLabelID = sector[iSector].floorpal; break;
        case SECTOR_FLOORXPANNING: lLabelID = sector[iSector].floorxpanning; break;
        case SECTOR_FLOORYPANNING: lLabelID = sector[iSector].floorypanning; break;
        case SECTOR_VISIBILITY: lLabelID = sector[iSector].visibility; break;
        case SECTOR_FOGPAL: lLabelID = sector[iSector].fogpal; break;
        case SECTOR_LOTAG: lLabelID = (int16_t)sector[iSector].lotag; break;
        case SECTOR_HITAG: lLabelID = (int16_t)sector[iSector].hitag; break;
        case SECTOR_ULOTAG: lLabelID = sector[iSector].lotag; break;
        case SECTOR_UHITAG: lLabelID = sector[iSector].hitag; break;
        case SECTOR_EXTRA: lLabelID = sector[iSector].extra; break;
        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
#ifdef YAX_ENABLE
            lLabelID = yax_getbunch(iSector, lLabelID == SECTOR_FLOORBUNCH);
#else
            lLabelID = -1;
#endif
            break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetSector(register int32_t const iSector, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iSector >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("VM_SetSector: Invalid sector %d\n", iSector);
        return;
    }

    switch (lLabelID)
    {
        case SECTOR_WALLPTR: sector[iSector].wallptr = iSet; break;
        case SECTOR_WALLNUM: sector[iSector].wallnum = iSet; break;
        case SECTOR_CEILINGZ: sector[iSector].ceilingz = iSet; break;
        case SECTOR_FLOORZ: sector[iSector].floorz = iSet; break;
        case SECTOR_CEILINGSTAT: sector[iSector].ceilingstat = iSet; break;
        case SECTOR_FLOORSTAT: sector[iSector].floorstat = iSet; break;
        case SECTOR_CEILINGPICNUM: sector[iSector].ceilingpicnum = iSet; break;
        case SECTOR_CEILINGSLOPE: sector[iSector].ceilingheinum = iSet; break;
        case SECTOR_CEILINGSHADE: sector[iSector].ceilingshade = iSet; break;
        case SECTOR_CEILINGPAL: sector[iSector].ceilingpal = iSet; break;
        case SECTOR_CEILINGXPANNING: sector[iSector].ceilingxpanning = iSet; break;
        case SECTOR_CEILINGYPANNING: sector[iSector].ceilingypanning = iSet; break;
        case SECTOR_FLOORPICNUM: sector[iSector].floorpicnum = iSet; break;
        case SECTOR_FLOORSLOPE: sector[iSector].floorheinum = iSet; break;
        case SECTOR_FLOORSHADE: sector[iSector].floorshade = iSet; break;
        case SECTOR_FLOORPAL: sector[iSector].floorpal = iSet; break;
        case SECTOR_FLOORXPANNING: sector[iSector].floorxpanning = iSet; break;
        case SECTOR_FLOORYPANNING: sector[iSector].floorypanning = iSet; break;
        case SECTOR_VISIBILITY: sector[iSector].visibility = iSet; break;
        case SECTOR_FOGPAL: sector[iSector].fogpal = iSet; break;
        case SECTOR_LOTAG: sector[iSector].lotag = (int16_t)iSet; break;
        case SECTOR_HITAG: sector[iSector].hitag = (int16_t)iSet; break;
        case SECTOR_ULOTAG: sector[iSector].lotag = iSet; break;
        case SECTOR_UHITAG: sector[iSector].hitag = iSet; break;
        case SECTOR_EXTRA: sector[iSector].extra = iSet; break;
        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
        default: break;
    }
}

void __fastcall VM_SetSprite(register int32_t const iActor, register int32_t const lLabelID, register int32_t const lParm2, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("tried to set %s on invalid target sprite (%d) from spr %d pic %d\n",
                      ActorLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum));
        return;
    }

    if (EDUKE32_PREDICT_FALSE(ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[lLabelID].maxParm2))
    {
        CON_ERRPRINTF("tried to set invalid %s position %d on sprite (%d) from spr %d\n",
                      ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
        return;
    }

    switch (lLabelID)
    {
        case ACTOR_X: sprite[iActor].x = iSet; break;
        case ACTOR_Y: sprite[iActor].y = iSet; break;
        case ACTOR_Z: sprite[iActor].z = iSet; break;
        case ACTOR_CSTAT: sprite[iActor].cstat = iSet; break;
        case ACTOR_PICNUM: sprite[iActor].picnum = iSet; break;
        case ACTOR_SHADE: sprite[iActor].shade = iSet; break;
        case ACTOR_PAL: sprite[iActor].pal = iSet; break;
        case ACTOR_CLIPDIST: sprite[iActor].clipdist = iSet; break;
        case ACTOR_DETAIL: sprite[iActor].blend = iSet; break;
        case ACTOR_XREPEAT: sprite[iActor].xrepeat = iSet; break;
        case ACTOR_YREPEAT: sprite[iActor].yrepeat = iSet; break;
        case ACTOR_XOFFSET: sprite[iActor].xoffset = iSet; break;
        case ACTOR_YOFFSET: sprite[iActor].yoffset = iSet; break;
        case ACTOR_SECTNUM: changespritesect(iActor, iSet); break;
        case ACTOR_STATNUM: changespritestat(iActor, iSet); break;
        case ACTOR_ANG: sprite[iActor].ang = iSet; break;
        case ACTOR_OWNER: sprite[iActor].owner = iSet; break;
        case ACTOR_XVEL: sprite[iActor].xvel = iSet; break;
        case ACTOR_YVEL: sprite[iActor].yvel = iSet; break;
        case ACTOR_ZVEL: sprite[iActor].zvel = iSet; break;
        case ACTOR_LOTAG: sprite[iActor].lotag = (int16_t)iSet; break;
        case ACTOR_HITAG: sprite[iActor].hitag = (int16_t)iSet; break;
        case ACTOR_ULOTAG: sprite[iActor].lotag = iSet; break;
        case ACTOR_UHITAG: sprite[iActor].hitag = iSet; break;
        case ACTOR_EXTRA: sprite[iActor].extra = iSet; break;
        case ACTOR_HTCGG: actor[iActor].cgg = iSet; break;
        case ACTOR_HTPICNUM: actor[iActor].picnum = iSet; break;
        case ACTOR_HTANG: actor[iActor].ang = iSet; break;
        case ACTOR_HTEXTRA: actor[iActor].extra = iSet; break;
        case ACTOR_HTOWNER: actor[iActor].owner = iSet; break;
        case ACTOR_HTMOVFLAG: actor[iActor].movflag = iSet; break;
        case ACTOR_HTTEMPANG: actor[iActor].tempang = iSet; break;
        case ACTOR_HTACTORSTAYPUT: actor[iActor].actorstayput = iSet; break;
        case ACTOR_HTDISPICNUM: actor[iActor].dispicnum = iSet; break;
        case ACTOR_HTTIMETOSLEEP: actor[iActor].timetosleep = iSet; break;
        case ACTOR_HTFLOORZ: actor[iActor].floorz = iSet; break;
        case ACTOR_HTCEILINGZ: actor[iActor].ceilingz = iSet; break;
        case ACTOR_HTLASTVX: actor[iActor].lastvx = iSet; break;
        case ACTOR_HTLASTVY: actor[iActor].lastvy = iSet; break;
        case ACTOR_HTBPOSX: actor[iActor].bpos.x = iSet; break;
        case ACTOR_HTBPOSY: actor[iActor].bpos.y = iSet; break;
        case ACTOR_HTBPOSZ: actor[iActor].bpos.z = iSet; break;
        case ACTOR_HTG_T: actor[iActor].t_data[lParm2] = iSet; break;
        case ACTOR_ANGOFF: spriteext[iActor].angoff = iSet; break;
        case ACTOR_PITCH: spriteext[iActor].pitch = iSet; break;
        case ACTOR_ROLL: spriteext[iActor].roll = iSet; break;
        case ACTOR_MDXOFF: spriteext[iActor].offset.x = iSet; break;
        case ACTOR_MDYOFF: spriteext[iActor].offset.y = iSet; break;
        case ACTOR_MDZOFF: spriteext[iActor].offset.z = iSet; break;
        case ACTOR_MDFLAGS: spriteext[iActor].flags = iSet; break;
        case ACTOR_XPANNING: spriteext[iActor].xpanning = iSet; break;
        case ACTOR_YPANNING: spriteext[iActor].ypanning = iSet; break;
        case ACTOR_HTFLAGS: actor[iActor].flags = iSet; break;
        case ACTOR_ALPHA: spriteext[iActor].alpha = (float)iSet * (1.f / 255.0f); break;
        default: break;
    }
}


int32_t __fastcall VM_GetSprite(register int32_t const iActor, register int32_t lLabelID, register int32_t const lParm2)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("tried to get %s on invalid target sprite (%d) from spr %d pic %d\n",
                      ActorLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum));
        return -1;
    }

    if (EDUKE32_PREDICT_FALSE(ActorLabels[lLabelID].flags & LABEL_HASPARM2 &&
        (unsigned)lParm2 >= (unsigned)ActorLabels[lLabelID].maxParm2))
    {
        CON_ERRPRINTF("tried to get invalid %s position %d on sprite (%d) from spr %d\n",
                      ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
        return -1;
    }

    switch (lLabelID)
    {
        case ACTOR_X: lLabelID = sprite[iActor].x; break;
        case ACTOR_Y: lLabelID = sprite[iActor].y; break;
        case ACTOR_Z: lLabelID = sprite[iActor].z; break;
        case ACTOR_CSTAT: lLabelID = sprite[iActor].cstat; break;
        case ACTOR_PICNUM: lLabelID = sprite[iActor].picnum; break;
        case ACTOR_SHADE: lLabelID = sprite[iActor].shade; break;
        case ACTOR_PAL: lLabelID = sprite[iActor].pal; break;
        case ACTOR_CLIPDIST: lLabelID = sprite[iActor].clipdist; break;
        case ACTOR_DETAIL: lLabelID = sprite[iActor].blend; break;
        case ACTOR_XREPEAT: lLabelID = sprite[iActor].xrepeat; break;
        case ACTOR_YREPEAT: lLabelID = sprite[iActor].yrepeat; break;
        case ACTOR_XOFFSET: lLabelID = sprite[iActor].xoffset; break;
        case ACTOR_YOFFSET: lLabelID = sprite[iActor].yoffset; break;
        case ACTOR_SECTNUM: lLabelID = sprite[iActor].sectnum; break;
        case ACTOR_STATNUM: lLabelID = sprite[iActor].statnum; break;
        case ACTOR_ANG: lLabelID = sprite[iActor].ang; break;
        case ACTOR_OWNER: lLabelID = sprite[iActor].owner; break;
        case ACTOR_XVEL: lLabelID = sprite[iActor].xvel; break;
        case ACTOR_YVEL: lLabelID = sprite[iActor].yvel; break;
        case ACTOR_ZVEL: lLabelID = sprite[iActor].zvel; break;
        case ACTOR_LOTAG: lLabelID = (int16_t)sprite[iActor].lotag; break;
        case ACTOR_HITAG: lLabelID = (int16_t)sprite[iActor].hitag; break;
        case ACTOR_ULOTAG: lLabelID = sprite[iActor].lotag; break;
        case ACTOR_UHITAG: lLabelID = sprite[iActor].hitag; break;
        case ACTOR_EXTRA: lLabelID = sprite[iActor].extra; break;
        case ACTOR_HTCGG: lLabelID = actor[iActor].cgg; break;
        case ACTOR_HTPICNUM: lLabelID = actor[iActor].picnum; break;
        case ACTOR_HTANG: lLabelID = actor[iActor].ang; break;
        case ACTOR_HTEXTRA: lLabelID = actor[iActor].extra; break;
        case ACTOR_HTOWNER: lLabelID = actor[iActor].owner; break;
        case ACTOR_HTMOVFLAG: lLabelID = actor[iActor].movflag; break;
        case ACTOR_HTTEMPANG: lLabelID = actor[iActor].tempang; break;
        case ACTOR_HTACTORSTAYPUT: lLabelID = actor[iActor].actorstayput; break;
        case ACTOR_HTDISPICNUM: lLabelID = actor[iActor].dispicnum; break;
        case ACTOR_HTTIMETOSLEEP: lLabelID = actor[iActor].timetosleep; break;
        case ACTOR_HTFLOORZ: lLabelID = actor[iActor].floorz; break;
        case ACTOR_HTCEILINGZ: lLabelID = actor[iActor].ceilingz; break;
        case ACTOR_HTLASTVX: lLabelID = actor[iActor].lastvx; break;
        case ACTOR_HTLASTVY: lLabelID = actor[iActor].lastvy; break;
        case ACTOR_HTBPOSX: lLabelID = actor[iActor].bpos.x; break;
        case ACTOR_HTBPOSY: lLabelID = actor[iActor].bpos.y; break;
        case ACTOR_HTBPOSZ: lLabelID = actor[iActor].bpos.z; break;
        case ACTOR_HTG_T: lLabelID = actor[iActor].t_data[lParm2]; break;
        case ACTOR_ANGOFF: lLabelID = spriteext[iActor].angoff; break;
        case ACTOR_PITCH: lLabelID = spriteext[iActor].pitch; break;
        case ACTOR_ROLL: lLabelID = spriteext[iActor].roll; break;
        case ACTOR_MDXOFF: lLabelID = spriteext[iActor].offset.x; break;
        case ACTOR_MDYOFF: lLabelID = spriteext[iActor].offset.y; break;
        case ACTOR_MDZOFF: lLabelID = spriteext[iActor].offset.z; break;
        case ACTOR_MDFLAGS: lLabelID = spriteext[iActor].flags; break;
        case ACTOR_XPANNING: lLabelID = spriteext[iActor].xpanning; break;
        case ACTOR_YPANNING: lLabelID = spriteext[iActor].ypanning; break;
        case ACTOR_HTFLAGS: lLabelID = actor[iActor].flags; break;
        case ACTOR_ALPHA: lLabelID = (uint8_t)(spriteext[iActor].alpha * 255.0f); break;
        case ACTOR_ISVALID: lLabelID = (sprite[iActor].statnum != MAXSTATUS); break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

int32_t __fastcall VM_GetTsprite(register int32_t const iActor, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("VM_GetTsprite: invalid target sprite (%d) %d %d\n", iActor, vm.g_i, TrackerCast(vm.g_sp->picnum));
        return -1;
    }

    tspritetype * const tspr = spriteext[iActor].tspr;

    if (EDUKE32_PREDICT_FALSE(!tspr))
    {
        CON_ERRPRINTF("VM_GetTsprite: Internal bug, tsprite is unavailable\n");
        return -1;
    }

    switch (lLabelID)
    {
        case ACTOR_X: lLabelID = tspr->x; break;
        case ACTOR_Y: lLabelID = tspr->y; break;
        case ACTOR_Z: lLabelID = tspr->z; break;
        case ACTOR_CSTAT: lLabelID = tspr->cstat; break;
        case ACTOR_PICNUM: lLabelID = tspr->picnum; break;
        case ACTOR_SHADE: lLabelID = tspr->shade; break;
        case ACTOR_PAL: lLabelID = tspr->pal; break;
        case ACTOR_CLIPDIST: lLabelID = tspr->clipdist; break;
        case ACTOR_DETAIL: lLabelID = tspr->blend; break;
        case ACTOR_XREPEAT: lLabelID = tspr->xrepeat; break;
        case ACTOR_YREPEAT: lLabelID = tspr->yrepeat; break;
        case ACTOR_XOFFSET: lLabelID = tspr->xoffset; break;
        case ACTOR_YOFFSET: lLabelID = tspr->yoffset; break;
        case ACTOR_SECTNUM: lLabelID = tspr->sectnum; break;
        case ACTOR_STATNUM: lLabelID = tspr->statnum; break;
        case ACTOR_ANG: lLabelID = tspr->ang; break;
        case ACTOR_OWNER: lLabelID = tspr->owner; break;
        case ACTOR_XVEL: lLabelID = tspr->xvel; break;
        case ACTOR_YVEL: lLabelID = tspr->yvel; break;
        case ACTOR_ZVEL: lLabelID = tspr->zvel; break;
        case ACTOR_LOTAG: lLabelID = (int16_t) tspr->lotag; break;
        case ACTOR_HITAG: lLabelID = (int16_t) tspr->hitag; break;
        case ACTOR_ULOTAG: lLabelID = tspr->lotag; break;
        case ACTOR_UHITAG: lLabelID = tspr->hitag; break;
        case ACTOR_EXTRA: lLabelID = tspr->extra; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetTsprite(register int32_t const iActor, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iActor >= MAXSPRITES))
    {
        CON_ERRPRINTF("VM_SetTsprite: invalid target sprite (%d) %d %d\n", iActor, vm.g_i, TrackerCast(vm.g_sp->picnum));
        return;
    }

    tspritetype * const tspr = spriteext[iActor].tspr;

    if (EDUKE32_PREDICT_FALSE(!tspr))
    {
        CON_ERRPRINTF("VM_SetTsprite: Internal bug, tsprite is unavailable\n");
        return;
    }

    switch (lLabelID)
    {
        case ACTOR_X: tspr->x = iSet; break;
        case ACTOR_Y: tspr->y = iSet; break;
        case ACTOR_Z: tspr->z = iSet; break;
        case ACTOR_CSTAT: tspr->cstat = iSet; break;
        case ACTOR_PICNUM: tspr->picnum = iSet; break;
        case ACTOR_SHADE: tspr->shade = iSet; break;
        case ACTOR_PAL: tspr->pal = iSet; break;
        case ACTOR_CLIPDIST: tspr->clipdist = iSet; break;
        case ACTOR_DETAIL: tspr->blend = iSet; break;
        case ACTOR_XREPEAT: tspr->xrepeat = iSet; break;
        case ACTOR_YREPEAT: tspr->yrepeat = iSet; break;
        case ACTOR_XOFFSET: tspr->xoffset = iSet; break;
        case ACTOR_YOFFSET: tspr->yoffset = iSet; break;
        case ACTOR_SECTNUM: tspr->sectnum = iSet; break;
        case ACTOR_STATNUM: tspr->statnum = iSet; break;
        case ACTOR_ANG: tspr->ang = iSet; break;
        case ACTOR_OWNER: tspr->owner = iSet; break;
        case ACTOR_XVEL: tspr->xvel = iSet; break;
        case ACTOR_YVEL: tspr->yvel = iSet; break;
        case ACTOR_ZVEL: tspr->zvel = iSet; break;
        case ACTOR_LOTAG: tspr->lotag = (int16_t) iSet; break;
        case ACTOR_HITAG: tspr->hitag = (int16_t) iSet; break;
        case ACTOR_ULOTAG: tspr->lotag = iSet; break;
        case ACTOR_UHITAG: tspr->hitag = iSet; break;
        case ACTOR_EXTRA: tspr->extra = iSet; break;
    }
}

int32_t __fastcall VM_GetProjectile(register int32_t const iTile, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iTile >= MAXTILES))
    {
        CON_ERRPRINTF("VM_GetProjectile: invalid projectile (%d)\n", iTile);
        return -1;
    }

    projectile_t * const proj = g_tile[iTile].proj;

    if (EDUKE32_PREDICT_FALSE(!proj))
    {
        CON_ERRPRINTF("VM_GetProjectile: no projectile defined for tile %d\n", iTile);
        return -1;
    }

    switch (lLabelID)
    {
        case PROJ_WORKSLIKE: lLabelID = proj->workslike; break;
        case PROJ_SPAWNS: lLabelID = proj->spawns; break;
        case PROJ_SXREPEAT: lLabelID = proj->sxrepeat; break;
        case PROJ_SYREPEAT: lLabelID = proj->syrepeat; break;
        case PROJ_SOUND: lLabelID = proj->sound; break;
        case PROJ_ISOUND: lLabelID = proj->isound; break;
        case PROJ_VEL: lLabelID = proj->vel; break;
        case PROJ_EXTRA: lLabelID = proj->extra; break;
        case PROJ_DECAL: lLabelID = proj->decal; break;
        case PROJ_TRAIL: lLabelID = proj->trail; break;
        case PROJ_TXREPEAT: lLabelID = proj->txrepeat; break;
        case PROJ_TYREPEAT: lLabelID = proj->tyrepeat; break;
        case PROJ_TOFFSET: lLabelID = proj->toffset; break;
        case PROJ_TNUM: lLabelID = proj->tnum; break;
        case PROJ_DROP: lLabelID = proj->drop; break;
        case PROJ_CSTAT: lLabelID = proj->cstat; break;
        case PROJ_CLIPDIST: lLabelID = proj->clipdist; break;
        case PROJ_SHADE: lLabelID = proj->shade; break;
        case PROJ_XREPEAT: lLabelID = proj->xrepeat; break;
        case PROJ_YREPEAT: lLabelID = proj->yrepeat; break;
        case PROJ_PAL: lLabelID = proj->pal; break;
        case PROJ_EXTRA_RAND: lLabelID = proj->extra_rand; break;
        case PROJ_HITRADIUS: lLabelID = proj->hitradius; break;
        case PROJ_MOVECNT: lLabelID = proj->movecnt; break;
        case PROJ_OFFSET: lLabelID = proj->offset; break;
        case PROJ_BOUNCES: lLabelID = proj->bounces; break;
        case PROJ_BSOUND: lLabelID = proj->bsound; break;
        case PROJ_RANGE: lLabelID = proj->range; break;
        case PROJ_FLASH_COLOR: lLabelID = proj->flashcolor; break;
        case PROJ_USERDATA: lLabelID = proj->userdata; break;
        default: lLabelID = -1; break;
    }

    return lLabelID;
}

void __fastcall VM_SetProjectile(register int32_t const iTile, register int32_t const lLabelID, register int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iTile >= MAXTILES))
    {
        CON_ERRPRINTF("VM_SetProjectile: invalid projectile (%d)\n", iTile);
        return;
    }

    projectile_t * const proj = g_tile[iTile].proj;

    if (EDUKE32_PREDICT_FALSE(!proj))
    {
        CON_ERRPRINTF("VM_SetProjectile: no projectile defined for tile %d\n", iTile);
        return;
    }

    switch (lLabelID)
    {
        case PROJ_WORKSLIKE: proj->workslike = iSet; break;
        case PROJ_SPAWNS: proj->spawns = iSet; break;
        case PROJ_SXREPEAT: proj->sxrepeat = iSet; break;
        case PROJ_SYREPEAT: proj->syrepeat = iSet; break;
        case PROJ_SOUND: proj->sound = iSet; break;
        case PROJ_ISOUND: proj->isound = iSet; break;
        case PROJ_VEL: proj->vel = iSet; break;
        case PROJ_EXTRA: proj->extra = iSet; break;
        case PROJ_DECAL: proj->decal = iSet; break;
        case PROJ_TRAIL: proj->trail = iSet; break;
        case PROJ_TXREPEAT: proj->txrepeat = iSet; break;
        case PROJ_TYREPEAT: proj->tyrepeat = iSet; break;
        case PROJ_TOFFSET: proj->toffset = iSet; break;
        case PROJ_TNUM: proj->tnum = iSet; break;
        case PROJ_DROP: proj->drop = iSet; break;
        case PROJ_CSTAT: proj->cstat = iSet; break;
        case PROJ_CLIPDIST: proj->clipdist = iSet; break;
        case PROJ_SHADE: proj->shade = iSet; break;
        case PROJ_XREPEAT: proj->xrepeat = iSet; break;
        case PROJ_YREPEAT: proj->yrepeat = iSet; break;
        case PROJ_PAL: proj->pal = iSet; break;
        case PROJ_EXTRA_RAND: proj->extra_rand = iSet; break;
        case PROJ_HITRADIUS: proj->hitradius = iSet; break;
        case PROJ_MOVECNT: proj->movecnt = iSet; break;
        case PROJ_OFFSET: proj->offset = iSet; break;
        case PROJ_BOUNCES: proj->bounces = iSet; break;
        case PROJ_BSOUND: proj->bsound = iSet; break;
        case PROJ_RANGE: proj->range = iSet; break;
        case PROJ_FLASH_COLOR: proj->flashcolor = iSet; break;
        case PROJ_USERDATA: proj->userdata = iSet; break;
    }
}

int32_t __fastcall VM_GetTileData(register int32_t const iTile, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iTile >= MAXTILES))
    {
        CON_ERRPRINTF("VM_GetTileData: invalid tile (%d)\n", iTile);
        return -1;
    }

    switch (lLabelID)
    {
        case TILEDATA_XSIZE: lLabelID = tilesiz[iTile].x; break;
        case TILEDATA_YSIZE: lLabelID = tilesiz[iTile].y; break;

        case TILEDATA_ANIMFRAMES: lLabelID = picanm[iTile].num; break;
        case TILEDATA_XOFFSET: lLabelID = picanm[iTile].xofs; break;
        case TILEDATA_YOFFSET: lLabelID = picanm[iTile].yofs; break;
        case TILEDATA_ANIMSPEED: lLabelID = picanm[iTile].sf & PICANM_ANIMSPEED_MASK; break;
        case TILEDATA_ANIMTYPE: lLabelID = (picanm[iTile].sf & PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT; break;

        case TILEDATA_GAMEFLAGS: lLabelID = g_tile[iTile].flags; break;

        default: lLabelID = -1; break;
    }

    return lLabelID;
}

int32_t __fastcall VM_GetPalData(register int32_t const iPal, register int32_t lLabelID)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)iPal >= MAXPALOOKUPS))
    {
        CON_ERRPRINTF("VM_GetPalData: invalid pal (%d)\n", iPal);
        return -1;
    }

    switch (lLabelID)
    {
        case PALDATA_NOFLOORPAL: lLabelID = g_noFloorPal[iPal]; break;

        default: lLabelID = -1; break;
    }

    return lLabelID;
}

#endif
