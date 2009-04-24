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
/*
#include "duke3d.h"
#include "gamedef.h"
#include "osd.h"
*/
// this is all the crap for accessing the game's structs through the CON VM
// I got a 3-4 fps gain by inlining these...

#ifndef _gamevars_c_
static inline void X_AccessUserdef(int32_t iSet, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;

    if (vm.g_p != myconnectindex)
    {
//        if (lVar2 == MAXGAMEVARS)
//            insptr++;
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case USERDEFS_GOD:
        if (iSet)
        {
            ud.god = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.god, vm.g_i, vm.g_p);
        return;

    case USERDEFS_WARP_ON:
        if (iSet)
        {
            ud.warp_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.warp_on, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CASHMAN:
        if (iSet)
        {
            ud.cashman = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.cashman, vm.g_i, vm.g_p);
        return;

    case USERDEFS_EOG:
        if (iSet)
        {
            ud.eog = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.eog, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SHOWALLMAP:
        if (iSet)
        {
            ud.showallmap = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.showallmap, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SHOW_HELP:
        if (iSet)
        {
            ud.show_help = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.show_help, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SCROLLMODE:
        if (iSet)
        {
            ud.scrollmode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.scrollmode, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CLIPPING:
        if (iSet)
        {
            ud.clipping = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.clipping, vm.g_i, vm.g_p);
        return;

        //  case USERDEFS_USER_NAME:
        //      if(iSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.user_name[MAXPLAYERS][32], vm.g_i, vm.g_p);
        //      return;

        //  case USERDEFS_RIDECULE:
        //      if(iSet)
        //      {
        //          ud.ridecule = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.ridecule, vm.g_i, vm.g_p);
        //      return;

        //  case USERDEFS_SAVEGAME:
        //      if(iSet)
        //      {
        //          ud.savegame = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.savegame, vm.g_i, vm.g_p);
        //      return;

        //  case USERDEFS_PWLOCKOUT:
        //      if(iSet)
        //      {
        //          ud.pwlockout = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.pwlockout, vm.g_i, vm.g_p);
        //      return;

        //  case USERDEFS_RTSNAME:
        //      if(iSet)
        //      {
        //          ud.rtsname = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.rtsname, vm.g_i, vm.g_p);
        //      return;

    case USERDEFS_OVERHEAD_ON:
        if (iSet)
        {
            ud.overhead_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.overhead_on, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LAST_OVERHEAD:
        if (iSet)
        {
            ud.last_overhead = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_overhead, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SHOWWEAPONS:
        if (iSet)
        {
            ud.showweapons = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.showweapons, vm.g_i, vm.g_p);
        return;

    case USERDEFS_PAUSE_ON:
        if (iSet)
        {
            ud.pause_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.pause_on, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FROM_BONUS:
        if (iSet)
        {
            ud.from_bonus = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.from_bonus, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CAMERASPRITE:
        if (iSet)
        {
            ud.camerasprite = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.camerasprite, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LAST_CAMSPRITE:
        if (iSet)
        {
            ud.last_camsprite = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_camsprite, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LAST_LEVEL:
        if (iSet)
        {
            ud.last_level = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_level, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SECRETLEVEL:
        if (iSet)
        {
            ud.secretlevel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.secretlevel, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CONST_VISIBILITY:
        if (iSet)
        {
            ud.const_visibility = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.const_visibility, vm.g_i, vm.g_p);
        return;

    case USERDEFS_UW_FRAMERATE:
        if (iSet)
        {
            ud.uw_framerate = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.uw_framerate, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CAMERA_TIME:
        if (iSet)
        {
            ud.camera_time = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.camera_time, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FOLFVEL:
        if (iSet)
        {
            ud.folfvel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folfvel, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FOLAVEL:
        if (iSet)
        {
            ud.folavel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folavel, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FOLX:
        if (iSet)
        {
            ud.folx = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folx, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FOLY:
        if (iSet)
        {
            ud.foly = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.foly, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FOLA:
        if (iSet)
        {
            ud.fola = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.fola, vm.g_i, vm.g_p);
        return;

    case USERDEFS_RECCNT:
        if (iSet)
        {
            ud.reccnt = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.reccnt, vm.g_i, vm.g_p);
        return;

    case USERDEFS_ENTERED_NAME:
        if (iSet)
        {
            ud.entered_name = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.entered_name, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SCREEN_TILTING:
        if (iSet)
        {
            ud.screen_tilting = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.screen_tilting, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SHADOWS:
        if (iSet)
        {
            ud.shadows = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.shadows, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FTA_ON:
        if (iSet)
        {
            ud.fta_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.fta_on, vm.g_i, vm.g_p);
        return;

    case USERDEFS_EXECUTIONS:
        if (iSet)
        {
            ud.executions = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.executions, vm.g_i, vm.g_p);
        return;

    case USERDEFS_AUTO_RUN:
        if (iSet)
        {
            ud.auto_run = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.auto_run, vm.g_i, vm.g_p);
        return;

    case USERDEFS_COORDS:
        if (iSet)
        {
            ud.coords = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.coords, vm.g_i, vm.g_p);
        return;

    case USERDEFS_TICKRATE:
        if (iSet)
        {
            ud.tickrate = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.tickrate, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_COOP:
        if (iSet)
        {
            ud.m_coop = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_coop, vm.g_i, vm.g_p);
        return;

    case USERDEFS_COOP:
        if (iSet)
        {
            ud.coop = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.coop, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SCREEN_SIZE:
        if (iSet)
        {
            if (ud.screen_size != lValue)
            {
                ud.screen_size = lValue;
                G_UpdateScreenArea();
            }
            return;
        }
        Gv_SetVar(lVar2, ud.screen_size, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LOCKOUT:
        if (iSet)
        {
            ud.lockout = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.lockout, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CROSSHAIR:
        if (iSet)
        {
            ud.crosshair = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.crosshair, vm.g_i, vm.g_p);
        return;

        //  case USERDEFS_WCHOICE:
        //      if(iSet)
        //      {
        //          ud.wchoice = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.wchoice, vm.g_i, vm.g_p);
        //      return;

    case USERDEFS_PLAYERAI:
        if (iSet)
        {
            ud.playerai = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.playerai, vm.g_i, vm.g_p);
        return;

    case USERDEFS_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.respawn_monsters = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_monsters, vm.g_i, vm.g_p);
        return;

    case USERDEFS_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.respawn_items = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_items, vm.g_i, vm.g_p);
        return;

    case USERDEFS_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.respawn_inventory = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_inventory, vm.g_i, vm.g_p);
        return;

    case USERDEFS_RECSTAT:
        if (iSet)
        {
            ud.recstat = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.recstat, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MONSTERS_OFF:
        if (iSet)
        {
            ud.monsters_off = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.monsters_off, vm.g_i, vm.g_p);
        return;

    case USERDEFS_BRIGHTNESS:
        if (iSet)
        {
            ud.brightness = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.brightness, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.m_respawn_items = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_items, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.m_respawn_monsters = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_monsters, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.m_respawn_inventory = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_inventory, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_RECSTAT:
        if (iSet)
        {
            ud.m_recstat = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_recstat, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_MONSTERS_OFF:
        if (iSet)
        {
            ud.m_monsters_off = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_monsters_off, vm.g_i, vm.g_p);
        return;

    case USERDEFS_DETAIL:
        if (iSet)
        {
            ud.detail = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.detail, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_FFIRE:
        if (iSet)
        {
            ud.m_ffire = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_ffire, vm.g_i, vm.g_p);
        return;

    case USERDEFS_FFIRE:
        if (iSet)
        {
            ud.ffire = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.ffire, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_PLAYER_SKILL:
        if (iSet)
        {
            ud.m_player_skill = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_player_skill, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_LEVEL_NUMBER:
        if (iSet)
        {
            ud.m_level_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_level_number, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_VOLUME_NUMBER:
        if (iSet)
        {
            ud.m_volume_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_volume_number, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MULTIMODE:
        if (iSet)
        {
            ud.multimode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.multimode, vm.g_i, vm.g_p);
        return;

    case USERDEFS_PLAYER_SKILL:
        if (iSet)
        {
            ud.player_skill = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.player_skill, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LEVEL_NUMBER:
        if (iSet)
        {
            ud.level_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.level_number, vm.g_i, vm.g_p);
        return;

    case USERDEFS_VOLUME_NUMBER:
        if (iSet)
        {
            ud.volume_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.volume_number, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_MARKER:
        if (iSet)
        {
            ud.m_marker = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_marker, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MARKER:
        if (iSet)
        {
            ud.marker = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.marker, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MOUSEFLIP:
        if (iSet)
        {
            ud.mouseflip = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.mouseflip, vm.g_i, vm.g_p);
        return;

    case USERDEFS_STATUSBARSCALE:
        if (iSet)
        {
            ud.statusbarscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.statusbarscale, vm.g_i, vm.g_p);
        return;

    case USERDEFS_DRAWWEAPON:
        if (iSet)
        {
            ud.drawweapon = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.drawweapon, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MOUSEAIMING:
        if (iSet)
        {
            ud.mouseaiming = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.mouseaiming, vm.g_i, vm.g_p);
        return;

    case USERDEFS_WEAPONSWITCH:
        if (iSet)
        {
            ud.weaponswitch = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponswitch, vm.g_i, vm.g_p);
        return;

    case USERDEFS_DEMOCAMS:
        if (iSet)
        {
            ud.democams = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.democams, vm.g_i, vm.g_p);
        return;

    case USERDEFS_COLOR:
        if (iSet)
        {
            ud.color = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.color, vm.g_i, vm.g_p);
        return;

    case USERDEFS_MSGDISPTIME:
        if (iSet)
        {
            ud.msgdisptime = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.msgdisptime, vm.g_i, vm.g_p);
        return;

    case USERDEFS_STATUSBARMODE:
        if (iSet)
        {
            ud.statusbarmode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.statusbarmode, vm.g_i, vm.g_p);
        return;

    case USERDEFS_M_NOEXITS:
        if (iSet)
        {
            ud.m_noexits = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_noexits, vm.g_i, vm.g_p);
        return;

    case USERDEFS_NOEXITS:
        if (iSet)
        {
            ud.noexits = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.noexits, vm.g_i, vm.g_p);
        return;

    case USERDEFS_AUTOVOTE:
        if (iSet)
        {
            ud.autovote = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.autovote, vm.g_i, vm.g_p);
        return;

    case USERDEFS_AUTOMSG:
        if (iSet)
        {
            ud.automsg = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.automsg, vm.g_i, vm.g_p);
        return;

    case USERDEFS_IDPLAYERS:
        if (iSet)
        {
            ud.idplayers = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.idplayers, vm.g_i, vm.g_p);
        return;

    case USERDEFS_TEAM:
        if (iSet)
        {
            ud.team = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.team, vm.g_i, vm.g_p);
        return;

    case USERDEFS_VIEWBOB:
        if (iSet)
        {
            ud.viewbob = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.viewbob, vm.g_i, vm.g_p);
        return;

    case USERDEFS_WEAPONSWAY:
        if (iSet)
        {
            ud.weaponsway = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponsway, vm.g_i, vm.g_p);
        return;

    case USERDEFS_ANGLEINTERPOLATION:
        if (iSet)
        {
            ud.angleinterpolation = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.angleinterpolation, vm.g_i, vm.g_p);
        return;

    case USERDEFS_OBITUARIES:
        if (iSet)
        {
            ud.obituaries = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.obituaries, vm.g_i, vm.g_p);
        return;

    case USERDEFS_LEVELSTATS:
        if (iSet)
        {
            ud.levelstats = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.levelstats, vm.g_i, vm.g_p);
        return;

    case USERDEFS_CROSSHAIRSCALE:
        if (iSet)
        {
            ud.crosshairscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.crosshairscale, vm.g_i, vm.g_p);
        return;

    case USERDEFS_ALTHUD:
        if (iSet)
        {
            ud.althud = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.althud, vm.g_i, vm.g_p);
        return;

    case USERDEFS_DISPLAY_BONUS_SCREEN:
        if (iSet)
        {
            ud.display_bonus_screen = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.display_bonus_screen, vm.g_i, vm.g_p);
        return;

    case USERDEFS_SHOW_LEVEL_TEXT:
        if (iSet)
        {
            ud.show_level_text = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.show_level_text, vm.g_i, vm.g_p);
        return;

    case USERDEFS_WEAPONSCALE:
        if (iSet)
        {
            ud.weaponscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponscale, vm.g_i, vm.g_p);
        return;

    case USERDEFS_TEXTSCALE:
        if (iSet)
        {
            ud.textscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.textscale, vm.g_i, vm.g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessActiveProjectile(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0,proj=vm.g_i;

    if (lVar1 != g_iThisActorID)
        proj=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((proj < 0 || proj >= MAXSPRITES) && g_scriptSanityChecks)
    {
//        OSD_Printf("X_AccessActiveProjectile(): invalid projectile (%d)\n",proj);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target projectile (%d) %d %d from %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ProjectileLabels[lLabelID].name,proj,vm.g_i,vm.g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            ActorExtra[proj].projectile.workslike=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.workslike, vm.g_i, vm.g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            ActorExtra[proj].projectile.spawns=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.spawns, vm.g_i, vm.g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.sxrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.sxrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.syrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.syrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.sound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.sound, vm.g_i, vm.g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.isound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.isound, vm.g_i, vm.g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            ActorExtra[proj].projectile.vel=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.vel, vm.g_i, vm.g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            ActorExtra[proj].projectile.extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.extra, vm.g_i, vm.g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            ActorExtra[proj].projectile.decal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.decal, vm.g_i, vm.g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            ActorExtra[proj].projectile.trail=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.trail, vm.g_i, vm.g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.txrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.txrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.tyrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.tyrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            ActorExtra[proj].projectile.toffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.toffset, vm.g_i, vm.g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            ActorExtra[proj].projectile.tnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.tnum, vm.g_i, vm.g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            ActorExtra[proj].projectile.drop=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.drop, vm.g_i, vm.g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.cstat, vm.g_i, vm.g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            ActorExtra[proj].projectile.clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.clipdist, vm.g_i, vm.g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            ActorExtra[proj].projectile.shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.shade, vm.g_i, vm.g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.xrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.yrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            ActorExtra[proj].projectile.pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.pal, vm.g_i, vm.g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            ActorExtra[proj].projectile.extra_rand=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.extra_rand, vm.g_i, vm.g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            ActorExtra[proj].projectile.hitradius=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.hitradius, vm.g_i, vm.g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            ActorExtra[proj].projectile.velmult=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.velmult, vm.g_i, vm.g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            ActorExtra[proj].projectile.offset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.offset, vm.g_i, vm.g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            ActorExtra[proj].projectile.bounces=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.bounces, vm.g_i, vm.g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.bsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.bsound, vm.g_i, vm.g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            ActorExtra[proj].projectile.range=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.range, vm.g_i, vm.g_p);
        return;

    case PROJ_FLASH_COLOR:
        if (iSet)
        {
            ActorExtra[proj].projectile.flashcolor=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.flashcolor, vm.g_i, vm.g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessPlayer(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2, int32_t lParm2)
{
    int32_t lValue=0;
    int32_t iPlayer=vm.g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && g_scriptSanityChecks)
    {
//        OSD_Printf("X_AccessPlayer(): invalid target player (%d) %d\n",iPlayer,vm.g_i);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target player (%d) from spr %d gv %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",PlayerLabels[lLabelID].name,iPlayer,vm.g_i,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= PlayerLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on player (%d) from spr %d\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        if (iSet)
        {
            g_player[iPlayer].ps->zoom=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->zoom, vm.g_i, vm.g_p);
        return;

    case PLAYER_EXITX:
        if (iSet)
        {
            g_player[iPlayer].ps->exitx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->exitx, vm.g_i, vm.g_p);
        return;

    case PLAYER_EXITY:
        if (iSet)
        {
            g_player[iPlayer].ps->exity=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->exity, vm.g_i, vm.g_p);
        return;

    case PLAYER_LOOGIEX:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiex[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogiex[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_LOOGIEY:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiey[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogiey[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_NUMLOOGS:
        if (iSet)
        {
            g_player[iPlayer].ps->numloogs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->numloogs, vm.g_i, vm.g_p);
        return;

    case PLAYER_LOOGCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->loogcnt=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogcnt, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSX:
        if (iSet)
        {
            g_player[iPlayer].ps->posx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posx, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSY:
        if (iSet)
        {
            g_player[iPlayer].ps->posy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posy, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->posz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posz, vm.g_i, vm.g_p);
        return;

    case PLAYER_HORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->horiz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->horiz, vm.g_i, vm.g_p);
        return;

    case PLAYER_OHORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->ohoriz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ohoriz, vm.g_i, vm.g_p);
        return;

    case PLAYER_OHORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->ohorizoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ohorizoff, vm.g_i, vm.g_p);
        return;

    case PLAYER_INVDISPTIME:
        if (iSet)
        {
            g_player[iPlayer].ps->invdisptime=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->invdisptime, vm.g_i, vm.g_p);
        return;

    case PLAYER_BOBPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobposx, vm.g_i, vm.g_p);
        return;

    case PLAYER_BOBPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobposy, vm.g_i, vm.g_p);
        return;

    case PLAYER_OPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->oposx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposx, vm.g_i, vm.g_p);
        return;

    case PLAYER_OPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->oposy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposy, vm.g_i, vm.g_p);
        return;

    case PLAYER_OPOSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->oposz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposz, vm.g_i, vm.g_p);
        return;

    case PLAYER_PYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->pyoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pyoff, vm.g_i, vm.g_p);
        return;

    case PLAYER_OPYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->opyoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->opyoff, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSXV:
        if (iSet)
        {
            g_player[iPlayer].ps->posxv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posxv, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSYV:
        if (iSet)
        {
            g_player[iPlayer].ps->posyv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posyv, vm.g_i, vm.g_p);
        return;

    case PLAYER_POSZV:
        if (iSet)
        {
            g_player[iPlayer].ps->poszv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->poszv, vm.g_i, vm.g_p);
        return;

    case PLAYER_LAST_PISSED_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->last_pissed_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_pissed_time, vm.g_i, vm.g_p);
        return;

    case PLAYER_TRUEFZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truefz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->truefz, vm.g_i, vm.g_p);
        return;

    case PLAYER_TRUECZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truecz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->truecz, vm.g_i, vm.g_p);
        return;

    case PLAYER_PLAYER_PAR:
        if (iSet)
        {
            g_player[iPlayer].ps->player_par=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->player_par, vm.g_i, vm.g_p);
        return;

    case PLAYER_VISIBILITY:
        if (iSet)
        {
            g_player[iPlayer].ps->visibility=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->visibility, vm.g_i, vm.g_p);
        return;

    case PLAYER_BOBCOUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->bobcounter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobcounter, vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPON_SWAY:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_sway=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_sway, vm.g_i, vm.g_p);
        return;

    case PLAYER_PALS_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->pals_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pals_time, vm.g_i, vm.g_p);
        return;

    case PLAYER_RANDOMFLAMEX:
        if (iSet)
        {
            g_player[iPlayer].ps->randomflamex=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->randomflamex, vm.g_i, vm.g_p);
        return;

    case PLAYER_CRACK_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->crack_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->crack_time, vm.g_i, vm.g_p);
        return;

    case PLAYER_AIM_MODE:
        if (iSet)
        {
            g_player[iPlayer].ps->aim_mode=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->aim_mode, vm.g_i, vm.g_p);
        return;

    case PLAYER_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ang, vm.g_i, vm.g_p);
        return;

    case PLAYER_OANG:
        if (iSet)
        {
            g_player[iPlayer].ps->oang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oang, vm.g_i, vm.g_p);
        return;

    case PLAYER_ANGVEL:
        if (iSet)
        {
            g_player[iPlayer].ps->angvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->angvel, vm.g_i, vm.g_p);
        return;

    case PLAYER_CURSECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->cursectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->cursectnum, vm.g_i, vm.g_p);
        return;

    case PLAYER_LOOK_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->look_ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->look_ang, vm.g_i, vm.g_p);
        return;

    case PLAYER_LAST_EXTRA:
        if (iSet)
        {
            g_player[iPlayer].ps->last_extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_extra, vm.g_i, vm.g_p);
        return;

    case PLAYER_SUBWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->subweapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->subweapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->ammo_amount[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ammo_amount[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_WACKEDBYACTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->wackedbyactor=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->wackedbyactor, vm.g_i, vm.g_p);
        return;

    case PLAYER_FRAG:
        if (iSet)
        {
            g_player[iPlayer].ps->frag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->frag, vm.g_i, vm.g_p);
        return;

    case PLAYER_FRAGGEDSELF:
        if (iSet)
        {
            g_player[iPlayer].ps->fraggedself=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fraggedself, vm.g_i, vm.g_p);
        return;

    case PLAYER_CURR_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->curr_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->curr_weapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_LAST_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_weapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_TIPINCS:
        if (iSet)
        {
            g_player[iPlayer].ps->tipincs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->tipincs, vm.g_i, vm.g_p);
        return;

    case PLAYER_HORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->horizoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->horizoff, vm.g_i, vm.g_p);
        return;

    case PLAYER_WANTWEAPONFIRE:
        if (iSet)
        {
            g_player[iPlayer].ps->wantweaponfire=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->wantweaponfire, vm.g_i, vm.g_p);
        return;

    case PLAYER_HOLODUKE_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holoduke_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_NEWOWNER:
        if (iSet)
        {
            g_player[iPlayer].ps->newowner=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->newowner, vm.g_i, vm.g_p);
        return;

    case PLAYER_HURT_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hurt_delay=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hurt_delay, vm.g_i, vm.g_p);
        return;

    case PLAYER_HBOMB_HOLD_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_hold_delay=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hbomb_hold_delay, vm.g_i, vm.g_p);
        return;

    case PLAYER_JUMPING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_counter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jumping_counter, vm.g_i, vm.g_p);
        return;

    case PLAYER_AIRLEFT:
        if (iSet)
        {
            g_player[iPlayer].ps->airleft=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->airleft, vm.g_i, vm.g_p);
        return;

    case PLAYER_KNEE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knee_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->knee_incs, vm.g_i, vm.g_p);
        return;

    case PLAYER_ACCESS_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->access_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_incs, vm.g_i, vm.g_p);
        return;

    case PLAYER_FTA:
        if (iSet)
        {
            g_player[iPlayer].ps->fta=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fta, vm.g_i, vm.g_p);
        return;

    case PLAYER_FTQ:
        if (iSet)
        {
            g_player[iPlayer].ps->ftq=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ftq, vm.g_i, vm.g_p);
        return;

    case PLAYER_ACCESS_WALLNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_wallnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_wallnum, vm.g_i, vm.g_p);
        return;

    case PLAYER_ACCESS_SPRITENUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_spritenum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_spritenum, vm.g_i, vm.g_p);
        return;

    case PLAYER_KICKBACK_PIC:
        if (iSet)
        {
            g_player[iPlayer].ps->kickback_pic=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->kickback_pic, vm.g_i, vm.g_p);
        return;

    case PLAYER_GOT_ACCESS:
        if (iSet)
        {
            g_player[iPlayer].ps->got_access=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->got_access, vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPON_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_ang, vm.g_i, vm.g_p);
        return;

    case PLAYER_FIRSTAID_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->firstaid_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->firstaid_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_SOMETHINGONPLAYER:
        if (iSet)
        {
            g_player[iPlayer].ps->somethingonplayer=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->somethingonplayer, vm.g_i, vm.g_p);
        return;

    case PLAYER_ON_CRANE:
        if (iSet)
        {
            g_player[iPlayer].ps->on_crane=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_crane, vm.g_i, vm.g_p);
        return;

    case PLAYER_I:
        if (iSet)
        {
            g_player[iPlayer].ps->i=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->i, vm.g_i, vm.g_p);
        return;

    case PLAYER_ONE_PARALLAX_SECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->one_parallax_sectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->one_parallax_sectnum, vm.g_i, vm.g_p);
        return;

    case PLAYER_OVER_SHOULDER_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->over_shoulder_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->over_shoulder_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_RANDOM_CLUB_FRAME:
        if (iSet)
        {
            g_player[iPlayer].ps->random_club_frame=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->random_club_frame, vm.g_i, vm.g_p);
        return;

    case PLAYER_FIST_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->fist_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fist_incs, vm.g_i, vm.g_p);
        return;

    case PLAYER_ONE_EIGHTY_COUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->one_eighty_count=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->one_eighty_count, vm.g_i, vm.g_p);
        return;

    case PLAYER_CHEAT_PHASE:
        if (iSet)
        {
            g_player[iPlayer].ps->cheat_phase=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->cheat_phase, vm.g_i, vm.g_p);
        return;

    case PLAYER_DUMMYPLAYERSPRITE:
        if (iSet)
        {
            g_player[iPlayer].ps->dummyplayersprite=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->dummyplayersprite, vm.g_i, vm.g_p);
        return;

    case PLAYER_EXTRA_EXTRA8:
        if (iSet)
        {
            g_player[iPlayer].ps->extra_extra8=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->extra_extra8, vm.g_i, vm.g_p);
        return;

    case PLAYER_QUICK_KICK:
        if (iSet)
        {
            g_player[iPlayer].ps->quick_kick=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->quick_kick, vm.g_i, vm.g_p);
        return;

    case PLAYER_HEAT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->heat_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->heat_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_ACTORSQU:
        if (iSet)
        {
            g_player[iPlayer].ps->actorsqu=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->actorsqu, vm.g_i, vm.g_p);
        return;

    case PLAYER_TIMEBEFOREEXIT:
        if (iSet)
        {
            g_player[iPlayer].ps->timebeforeexit=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->timebeforeexit, vm.g_i, vm.g_p);
        return;

    case PLAYER_CUSTOMEXITSOUND:
        if (iSet)
        {
            g_player[iPlayer].ps->customexitsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->customexitsound, vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPRECS:
        if (iSet)
        {
            g_player[iPlayer].ps->weaprecs[15]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weaprecs[15], vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPRECCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->weapreccnt=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapreccnt, vm.g_i, vm.g_p);
        return;

    case PLAYER_INTERFACE_TOGGLE_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->interface_toggle_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->interface_toggle_flag, vm.g_i, vm.g_p);
        return;

    case PLAYER_ROTSCRNANG:
        if (iSet)
        {
            g_player[iPlayer].ps->rotscrnang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->rotscrnang, vm.g_i, vm.g_p);
        return;

    case PLAYER_DEAD_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->dead_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->dead_flag, vm.g_i, vm.g_p);
        return;

    case PLAYER_SHOW_EMPTY_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->show_empty_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->show_empty_weapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_SCUBA_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scuba_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_JETPACK_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jetpack_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_STEROIDS_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->steroids_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->steroids_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->shield_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->shield_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_HOLODUKE_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holoduke_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_PYCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->pycount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pycount, vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPON_POS:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_pos=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_pos, vm.g_i, vm.g_p);
        return;

    case PLAYER_FRAG_PS:
        if (iSet)
        {
            g_player[iPlayer].ps->frag_ps=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->frag_ps, vm.g_i, vm.g_p);
        return;

    case PLAYER_TRANSPORTER_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->transporter_hold=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->transporter_hold, vm.g_i, vm.g_p);
        return;

    case PLAYER_LAST_FULL_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_full_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_full_weapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_FOOTPRINTSHADE:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintshade, vm.g_i, vm.g_p);
        return;

    case PLAYER_BOOT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->boot_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->boot_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_SCREAM_VOICE:
        if (iSet)
        {
            g_player[iPlayer].ps->scream_voice=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scream_voice, vm.g_i, vm.g_p);
        return;

    case PLAYER_GM:
        if (iSet)
        {
            g_player[iPlayer].ps->gm=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->gm, vm.g_i, vm.g_p);
        return;

    case PLAYER_ON_WARPING_SECTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->on_warping_sector=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_warping_sector, vm.g_i, vm.g_p);
        return;

    case PLAYER_FOOTPRINTCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintcount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintcount, vm.g_i, vm.g_p);
        return;

    case PLAYER_HBOMB_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hbomb_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_JUMPING_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_toggle=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jumping_toggle, vm.g_i, vm.g_p);
        return;

    case PLAYER_RAPID_FIRE_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->rapid_fire_hold=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->rapid_fire_hold, vm.g_i, vm.g_p);
        return;

    case PLAYER_ON_GROUND:
        if (iSet)
        {
            g_player[iPlayer].ps->on_ground=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_ground, vm.g_i, vm.g_p);
        return;

        /*    case PLAYER_NAME:
                if (iSet)
                {
                    g_player[iPlayer].ps->name[32]=lValue;
                    return;
                }
                Gv_SetVar(lVar2, g_player[iPlayer].ps->name[32], vm.g_i, vm.g_p);
                return;*/

    case PLAYER_INVEN_ICON:
        if (iSet)
        {
            g_player[iPlayer].ps->inven_icon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->inven_icon, vm.g_i, vm.g_p);
        return;

    case PLAYER_BUTTONPALETTE:
        if (iSet)
        {
            g_player[iPlayer].ps->buttonpalette=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->buttonpalette, vm.g_i, vm.g_p);
        return;

    case PLAYER_JETPACK_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jetpack_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_SPRITEBRIDGE:
        if (iSet)
        {
            g_player[iPlayer].ps->spritebridge=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->spritebridge, vm.g_i, vm.g_p);
        return;

    case PLAYER_LASTRANDOMSPOT:
        if (iSet)
        {
            g_player[iPlayer].ps->lastrandomspot=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->lastrandomspot, vm.g_i, vm.g_p);
        return;

    case PLAYER_SCUBA_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scuba_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_FOOTPRINTPAL:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintpal, vm.g_i, vm.g_p);
        return;

    case PLAYER_HEAT_ON:
        if (iSet)
        {
            {
                if (g_player[iPlayer].ps->heat_on != lValue)
                {
                    g_player[iPlayer].ps->heat_on=lValue;
                    P_UpdateScreenPal(g_player[iPlayer].ps);
                }
            }
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->heat_on, vm.g_i, vm.g_p);
        return;

    case PLAYER_HOLSTER_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->holster_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holster_weapon, vm.g_i, vm.g_p);
        return;

    case PLAYER_FALLING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->falling_counter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->falling_counter, vm.g_i, vm.g_p);
        return;

    case PLAYER_GOTWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->gotweapon[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->gotweapon[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_REFRESH_INVENTORY:
        if (iSet)
        {
            g_player[iPlayer].ps->refresh_inventory=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->refresh_inventory, vm.g_i, vm.g_p);
        return;

        //      case PLAYER_PALETTE:
        //          if(iSet)
        //          {
        //              g_player[iPlayer].ps->palette=lValue;
        //          }
        //          return; }
        //          {
        //              Gv_SetVar(lVar2, g_player[iPlayer].ps->palette, vm.g_i, vm.g_p);
        //          }
        //          return;

    case PLAYER_TOGGLE_KEY_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->toggle_key_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->toggle_key_flag, vm.g_i, vm.g_p);
        return;

    case PLAYER_KNUCKLE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knuckle_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->knuckle_incs, vm.g_i, vm.g_p);
        return;

    case PLAYER_WALKING_SND_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->walking_snd_toggle=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->walking_snd_toggle, vm.g_i, vm.g_p);
        return;

    case PLAYER_PALOOKUP:
        if (iSet)
        {
            g_player[iPlayer].ps->palookup=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->palookup, vm.g_i, vm.g_p);
        return;

    case PLAYER_HARD_LANDING:
        if (iSet)
        {
            g_player[iPlayer].ps->hard_landing=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hard_landing, vm.g_i, vm.g_p);
        return;

    case PLAYER_MAX_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->max_secret_rooms=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_secret_rooms, vm.g_i, vm.g_p);
        return;

    case PLAYER_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->secret_rooms=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->secret_rooms, vm.g_i, vm.g_p);
        return;

    case PLAYER_PALS:
        if (iSet)
        {
            g_player[iPlayer].ps->pals[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pals[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_MAX_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->max_actors_killed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_actors_killed, vm.g_i, vm.g_p);
        return;

    case PLAYER_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->actors_killed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->actors_killed, vm.g_i, vm.g_p);
        return;

    case PLAYER_RETURN_TO_CENTER:
        if (iSet)
        {
            g_player[iPlayer].ps->return_to_center=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->return_to_center, vm.g_i, vm.g_p);
        return;

    case PLAYER_RUNSPEED:
        if (iSet)
        {
            g_player[iPlayer].ps->runspeed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->runspeed, vm.g_i, vm.g_p);
        return;

    case PLAYER_SBS:
        if (iSet)
        {
            g_player[iPlayer].ps->sbs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->sbs, vm.g_i, vm.g_p);
        return;

    case PLAYER_RELOADING:
        if (iSet)
        {
            g_player[iPlayer].ps->reloading=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->reloading, vm.g_i, vm.g_p);
        return;

    case PLAYER_AUTO_AIM:
        if (iSet)
        {
            g_player[iPlayer].ps->auto_aim=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->auto_aim, vm.g_i, vm.g_p);
        return;

    case PLAYER_MOVEMENT_LOCK:
        if (iSet)
        {
            g_player[iPlayer].ps->movement_lock=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->movement_lock, vm.g_i, vm.g_p);
        return;

    case PLAYER_SOUND_PITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->sound_pitch=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->sound_pitch, vm.g_i, vm.g_p);
        return;

    case PLAYER_WEAPONSWITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->weaponswitch=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weaponswitch, vm.g_i, vm.g_p);
        return;

    case PLAYER_TEAM:
        if (iSet)
        {
            g_player[iPlayer].ps->team=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->team, vm.g_i, vm.g_p);
        return;

    case PLAYER_MAX_PLAYER_HEALTH:
        if (iSet)
        {
            g_player[iPlayer].ps->max_player_health = lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_player_health, vm.g_i, vm.g_p);
        return;

    case PLAYER_MAX_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_shield_amount = lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_shield_amount, vm.g_i, vm.g_p);
        return;

    case PLAYER_MAX_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_ammo_amount[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_ammo_amount[lParm2], vm.g_i, vm.g_p);
        return;

    case PLAYER_LAST_QUICK_KICK:
        if (iSet)
        {
            g_player[iPlayer].ps->last_quick_kick=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_quick_kick, vm.g_i, vm.g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessPlayerInput(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iPlayer=vm.g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && g_scriptSanityChecks)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "invalid target player (%d) %d\n",g_errorLineNum,keyw[g_tw],iPlayer,vm.g_i);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case INPUT_AVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->avel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->avel, vm.g_i, vm.g_p);
        return;

    case INPUT_HORZ:
        if (iSet)
        {
            g_player[iPlayer].sync->horz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->horz, vm.g_i, vm.g_p);
        return;

    case INPUT_FVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->fvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->fvel, vm.g_i, vm.g_p);
        return;

    case INPUT_SVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->svel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->svel, vm.g_i, vm.g_p);
        return;

    case INPUT_BITS:
        if (iSet)
        {
            g_player[iPlayer].sync->bits=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->bits, vm.g_i, vm.g_p);
        return;

    case INPUT_EXTBITS:
        if (iSet)
        {
            g_player[iPlayer].sync->extbits=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->extbits, vm.g_i, vm.g_p);
        return;
    default:
        return;
    }
}

static inline void X_AccessWall(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iWall = Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iWall<0 || iWall >= numwalls) && g_scriptSanityChecks)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "Invalid wall %d\n",g_errorLineNum,keyw[g_tw],iWall);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case WALL_X:
        if (iSet)
        {
            wall[iWall].x=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].x, vm.g_i, vm.g_p);
        return;

    case WALL_Y:
        if (iSet)
        {
            wall[iWall].y=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].y, vm.g_i, vm.g_p);
        return;

    case WALL_POINT2:
        if (iSet)
        {
            wall[iWall].point2=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].point2, vm.g_i, vm.g_p);
        return;

    case WALL_NEXTWALL:
        if (iSet)
        {
            wall[iWall].nextwall=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].nextwall, vm.g_i, vm.g_p);
        return;

    case WALL_NEXTSECTOR:
        if (iSet)
        {
            wall[iWall].nextsector=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].nextsector, vm.g_i, vm.g_p);
        return;

    case WALL_CSTAT:
        if (iSet)
        {
            wall[iWall].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].cstat, vm.g_i, vm.g_p);
        return;

    case WALL_PICNUM:
        if (iSet)
        {
            wall[iWall].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].picnum, vm.g_i, vm.g_p);
        return;

    case WALL_OVERPICNUM:
        if (iSet)
        {
            wall[iWall].overpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].overpicnum, vm.g_i, vm.g_p);
        return;

    case WALL_SHADE:
        if (iSet)
        {
            wall[iWall].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].shade, vm.g_i, vm.g_p);
        return;

    case WALL_PAL:
        if (iSet)
        {
            wall[iWall].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].pal, vm.g_i, vm.g_p);
        return;

    case WALL_XREPEAT:
        if (iSet)
        {
            wall[iWall].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].xrepeat, vm.g_i, vm.g_p);
        return;

    case WALL_YREPEAT:
        if (iSet)
        {
            wall[iWall].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].yrepeat, vm.g_i, vm.g_p);
        return;

    case WALL_XPANNING:
        if (iSet)
        {
            wall[iWall].xpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].xpanning, vm.g_i, vm.g_p);
        return;

    case WALL_YPANNING:
        if (iSet)
        {
            wall[iWall].ypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].ypanning, vm.g_i, vm.g_p);
        return;

    case WALL_LOTAG:
        if (iSet)
        {
            wall[iWall].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].lotag, vm.g_i, vm.g_p);
        return;

    case WALL_HITAG:
        if (iSet)
        {
            wall[iWall].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].hitag, vm.g_i, vm.g_p);
        return;

    case WALL_EXTRA:
        if (iSet)
        {
            wall[iWall].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].extra, vm.g_i, vm.g_p);
        return;
    default:
        return;
    }
}

static inline void X_AccessSector(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iSector=sprite[vm.g_i].sectnum;

    if (lVar1 != g_iThisActorID)
        iSector=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iSector<0 || iSector >= numsectors) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],iSector);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case SECTOR_WALLPTR:
        if (iSet)
        {
            sector[iSector].wallptr=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].wallptr,vm.g_i,vm.g_p);
        return;

    case SECTOR_WALLNUM:
        if (iSet)
        {
            sector[iSector].wallnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].wallnum,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGZ:
        if (iSet)
        {
            sector[iSector].ceilingz=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingz,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORZ:
        if (iSet)
        {
            sector[iSector].floorz=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorz,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGSTAT:
        if (iSet)
        {
            sector[iSector].ceilingstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingstat,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORSTAT:
        if (iSet)
        {
            sector[iSector].floorstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorstat,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGPICNUM:
        if (iSet)
        {
            sector[iSector].ceilingpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingpicnum,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGSLOPE:
        if (iSet)
        {
            sector[iSector].ceilingheinum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingheinum,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGSHADE:
        if (iSet)
        {
            sector[iSector].ceilingshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingshade,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGPAL:
        if (iSet)
        {
            sector[iSector].ceilingpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingpal,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGXPANNING:
        if (iSet)
        {
            sector[iSector].ceilingxpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingxpanning,vm.g_i,vm.g_p);
        return;

    case SECTOR_CEILINGYPANNING:
        if (iSet)
        {
            sector[iSector].ceilingypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingypanning,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORPICNUM:
        if (iSet)
        {
            sector[iSector].floorpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorpicnum,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORSLOPE:
        if (iSet)
        {
            sector[iSector].floorheinum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorheinum,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORSHADE:
        if (iSet)
        {
            sector[iSector].floorshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorshade,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORPAL:
        if (iSet)
        {
            sector[iSector].floorpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorpal,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORXPANNING:
        if (iSet)
        {
            sector[iSector].floorxpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorxpanning,vm.g_i,vm.g_p);
        return;

    case SECTOR_FLOORYPANNING:
        if (iSet)
        {
            sector[iSector].floorypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorypanning,vm.g_i,vm.g_p);
        return;

    case SECTOR_VISIBILITY:
        if (iSet)
        {
            sector[iSector].visibility=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].visibility,vm.g_i,vm.g_p);
        return;

    case SECTOR_ALIGNTO:
        if (iSet)
        {
            sector[iSector].filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].filler,vm.g_i,vm.g_p);
        return;

    case SECTOR_LOTAG:
        if (iSet)
        {
            sector[iSector].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].lotag,vm.g_i,vm.g_p);
        return;

    case SECTOR_HITAG:
        if (iSet)
        {
            sector[iSector].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].hitag,vm.g_i,vm.g_p);
        return;

    case SECTOR_EXTRA:
        if (iSet)
        {
            sector[iSector].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].extra,vm.g_i,vm.g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessSprite(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2, int32_t lParm2)
{
    int32_t lValue=0;
    int32_t iActor=vm.g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target sprite (%d) from spr %d pic %d gv %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ActorLabels[lLabelID].name,iActor,vm.g_i,vm.g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= ActorLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on sprite (%d) from spr %d\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
        {
            sprite[iActor].x=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].x,vm.g_i,vm.g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            sprite[iActor].y=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].y,vm.g_i,vm.g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            sprite[iActor].z=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].z,vm.g_i,vm.g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            sprite[iActor].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].cstat,vm.g_i,vm.g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            sprite[iActor].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].picnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            sprite[iActor].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].shade,vm.g_i,vm.g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            sprite[iActor].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].pal,vm.g_i,vm.g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            sprite[iActor].clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].clipdist,vm.g_i,vm.g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            sprite[iActor].filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].filler,vm.g_i,vm.g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            sprite[iActor].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xrepeat,vm.g_i,vm.g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            sprite[iActor].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yrepeat,vm.g_i,vm.g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            sprite[iActor].xoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xoffset,vm.g_i,vm.g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            sprite[iActor].yoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yoffset,vm.g_i,vm.g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            changespritesect(iActor,lValue);
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].sectnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            changespritestat(iActor,lValue);
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].statnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            sprite[iActor].ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].ang,vm.g_i,vm.g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            sprite[iActor].owner=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].owner,vm.g_i,vm.g_p);
        return;

    case ACTOR_XVEL:
        if (iSet)
        {
            sprite[iActor].xvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            sprite[iActor].yvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            sprite[iActor].zvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].zvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            sprite[iActor].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].lotag,vm.g_i,vm.g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            sprite[iActor].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].hitag,vm.g_i,vm.g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            sprite[iActor].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].extra,vm.g_i,vm.g_p);
        return;

    case ACTOR_HTCGG:
        if (iSet)
        {
            ActorExtra[iActor].cgg=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].cgg, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTPICNUM :
        if (iSet)
        {
            ActorExtra[iActor].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].picnum, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTANG:
        if (iSet)
        {
            ActorExtra[iActor].ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].ang, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTEXTRA:
        if (iSet)
        {
            ActorExtra[iActor].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].extra, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTOWNER:
        if (iSet)
        {
            ActorExtra[iActor].owner=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].owner, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTMOVFLAG:
        if (iSet)
        {
            ActorExtra[iActor].movflag=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].movflag, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTTEMPANG:
        if (iSet)
        {
            ActorExtra[iActor].tempang=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].tempang, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTACTORSTAYPUT:
        if (iSet)
        {
            ActorExtra[iActor].actorstayput=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].actorstayput, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTDISPICNUM:
        if (iSet)
        {
            ActorExtra[iActor].dispicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].dispicnum, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTTIMETOSLEEP:
        if (iSet)
        {
            ActorExtra[iActor].timetosleep=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].timetosleep, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTFLOORZ:
        if (iSet)
        {
            ActorExtra[iActor].floorz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].floorz, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTCEILINGZ:
        if (iSet)
        {
            ActorExtra[iActor].ceilingz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].ceilingz, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTLASTVX:
        if (iSet)
        {
            ActorExtra[iActor].lastvx=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].lastvx, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTLASTVY:
        if (iSet)
        {
            ActorExtra[iActor].lastvy=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].lastvy, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTBPOSX:
        if (iSet)
        {
            ActorExtra[iActor].bposx=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposx, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTBPOSY:
        if (iSet)
        {
            ActorExtra[iActor].bposy=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposy, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTBPOSZ:
        if (iSet)
        {
            ActorExtra[iActor].bposz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposz, vm.g_i, vm.g_p);
        return;

    case ACTOR_HTG_T:
        if (iSet)
        {
            ActorExtra[iActor].temp_data[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].temp_data[lParm2], vm.g_i, vm.g_p);
        return;

    case ACTOR_ANGOFF:
        if (iSet)
        {
            spriteext[iActor].angoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].angoff, vm.g_i, vm.g_p);
        return;

    case ACTOR_PITCH:
        if (iSet)
        {
            spriteext[iActor].pitch=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].pitch, vm.g_i, vm.g_p);
        return;

    case ACTOR_ROLL:
        if (iSet)
        {
            spriteext[iActor].roll=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].roll, vm.g_i, vm.g_p);
        return;

    case ACTOR_MDXOFF:
        if (iSet)
        {
            spriteext[iActor].xoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].xoff, vm.g_i, vm.g_p);
        return;

    case ACTOR_MDYOFF:
        if (iSet)
        {
            spriteext[iActor].yoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].yoff, vm.g_i, vm.g_p);
        return;

    case ACTOR_MDZOFF:
        if (iSet)
        {
            spriteext[iActor].zoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].zoff, vm.g_i, vm.g_p);
        return;

    case ACTOR_MDFLAGS:
        if (iSet)
        {
            spriteext[iActor].flags=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].flags, vm.g_i, vm.g_p);
        return;

    case ACTOR_XPANNING:
        if (iSet)
        {
            spriteext[iActor].xpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].xpanning,vm.g_i,vm.g_p);
        return;

    case ACTOR_YPANNING:
        if (iSet)
        {
            spriteext[iActor].ypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].ypanning,vm.g_i,vm.g_p);
        return;

    case ACTOR_HTFLAGS:
        if (iSet)
        {
            ActorExtra[iActor].flags=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].flags, vm.g_i, vm.g_p);
        return;

    case ACTOR_ALPHA:
        if (iSet)
        {
            spriteext[iActor].alpha=(float)(lValue/255.0f);
            return;
        }
        Gv_SetVar(lVar2, (uint8_t)(spriteext[iActor].alpha * 255.0f), vm.g_i, vm.g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessTsprite(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iActor=vm.g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVar(lVar1, vm.g_i, vm.g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "invalid target sprite (%d) %d %d\n",g_errorLineNum,keyw[g_tw],iActor,vm.g_i,vm.g_sp->picnum);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    if ((!spriteext[iActor].tspr) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "Internal bug, tsprite is unavailable\n",g_errorLineNum,keyw[g_tw]);
        return;
    }

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
        {
            spriteext[iActor].tspr->x=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->x,vm.g_i,vm.g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            spriteext[iActor].tspr->y=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->y,vm.g_i,vm.g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            spriteext[iActor].tspr->z=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->z,vm.g_i,vm.g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            spriteext[iActor].tspr->cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->cstat,vm.g_i,vm.g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->picnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            spriteext[iActor].tspr->shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->shade,vm.g_i,vm.g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            spriteext[iActor].tspr->pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->pal,vm.g_i,vm.g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            spriteext[iActor].tspr->clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->clipdist,vm.g_i,vm.g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            spriteext[iActor].tspr->filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->filler,vm.g_i,vm.g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xrepeat,vm.g_i,vm.g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yrepeat,vm.g_i,vm.g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->xoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xoffset,vm.g_i,vm.g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->yoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yoffset,vm.g_i,vm.g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->sectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->sectnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->statnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->statnum,vm.g_i,vm.g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            spriteext[iActor].tspr->ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->ang,vm.g_i,vm.g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            spriteext[iActor].tspr->owner=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->owner,vm.g_i,vm.g_p);
        return;

#if 1
    case ACTOR_XVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->xvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->yvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->zvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->zvel,vm.g_i,vm.g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            spriteext[iActor].tspr->lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->lotag,vm.g_i,vm.g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            spriteext[iActor].tspr->hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->hitag,vm.g_i,vm.g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            spriteext[iActor].tspr->extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->extra,vm.g_i,vm.g_p);
        return;
#endif

    default:
        return;
    }
}

static inline void X_AccessProjectile(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;

    if ((lVar1 < 0 || lVar1 >= MAXTILES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "invalid tile (%d)\n",g_errorLineNum,keyw[g_tw],lVar1);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, vm.g_i, vm.g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            ProjectileData[lVar1].workslike=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].workslike, vm.g_i, vm.g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            ProjectileData[lVar1].spawns=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].spawns, vm.g_i, vm.g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].sxrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].sxrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].syrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].syrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            ProjectileData[lVar1].sound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].sound, vm.g_i, vm.g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            ProjectileData[lVar1].isound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].isound, vm.g_i, vm.g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            ProjectileData[lVar1].vel=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].vel, vm.g_i, vm.g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            ProjectileData[lVar1].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].extra, vm.g_i, vm.g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            ProjectileData[lVar1].decal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].decal, vm.g_i, vm.g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            ProjectileData[lVar1].trail=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].trail, vm.g_i, vm.g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].txrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].txrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].tyrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].tyrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].toffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].toffset, vm.g_i, vm.g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            ProjectileData[lVar1].tnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].tnum, vm.g_i, vm.g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            ProjectileData[lVar1].drop=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].drop, vm.g_i, vm.g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            ProjectileData[lVar1].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].cstat, vm.g_i, vm.g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            ProjectileData[lVar1].clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].clipdist, vm.g_i, vm.g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            ProjectileData[lVar1].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].shade, vm.g_i, vm.g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].xrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].yrepeat, vm.g_i, vm.g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            ProjectileData[lVar1].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].pal, vm.g_i, vm.g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            ProjectileData[lVar1].extra_rand=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].extra_rand, vm.g_i, vm.g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            ProjectileData[lVar1].hitradius=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].hitradius, vm.g_i, vm.g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            ProjectileData[lVar1].velmult=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].velmult, vm.g_i, vm.g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].offset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].offset, vm.g_i, vm.g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            ProjectileData[lVar1].bounces=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].bounces, vm.g_i, vm.g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            ProjectileData[lVar1].bsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].bsound, vm.g_i, vm.g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            ProjectileData[lVar1].range=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].range, vm.g_i, vm.g_p);
        return;

    default:
        return;
    }
}
#else
static int32_t X_AccessSpriteX(int32_t iActor, int32_t lLabelID, int32_t lParm2)
{
    if ((ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= ActorLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to get invalid %s position %d on sprite (%d) from spr %d\n",
                   g_errorLineNum,keyw[g_tw],ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
        return -1;
    }

    switch (lLabelID)
    {
    case ACTOR_X: return sprite[iActor].x;
    case ACTOR_Y: return sprite[iActor].y;
    case ACTOR_Z: return sprite[iActor].z;
    case ACTOR_CSTAT: return sprite[iActor].cstat;
    case ACTOR_PICNUM: return sprite[iActor].picnum;
    case ACTOR_SHADE: return sprite[iActor].shade;
    case ACTOR_PAL: return sprite[iActor].pal;
    case ACTOR_CLIPDIST: return sprite[iActor].clipdist;
    case ACTOR_DETAIL: return sprite[iActor].filler;
    case ACTOR_XREPEAT: return sprite[iActor].xrepeat;
    case ACTOR_YREPEAT: return sprite[iActor].yrepeat;
    case ACTOR_XOFFSET: return sprite[iActor].xoffset;
    case ACTOR_YOFFSET: return sprite[iActor].yoffset;
    case ACTOR_SECTNUM: return sprite[iActor].sectnum;
    case ACTOR_STATNUM: return sprite[iActor].statnum;
    case ACTOR_ANG: return sprite[iActor].ang;
    case ACTOR_OWNER: return sprite[iActor].owner;
    case ACTOR_XVEL: return sprite[iActor].xvel;
    case ACTOR_YVEL: return sprite[iActor].yvel;
    case ACTOR_ZVEL: return sprite[iActor].zvel;
    case ACTOR_LOTAG: return sprite[iActor].lotag;
    case ACTOR_HITAG: return sprite[iActor].hitag;
    case ACTOR_EXTRA: return sprite[iActor].extra;
    case ACTOR_HTCGG: return ActorExtra[iActor].cgg;
    case ACTOR_HTPICNUM : return ActorExtra[iActor].picnum;
    case ACTOR_HTANG: return ActorExtra[iActor].ang;
    case ACTOR_HTEXTRA: return ActorExtra[iActor].extra;
    case ACTOR_HTOWNER: return ActorExtra[iActor].owner;
    case ACTOR_HTMOVFLAG: return ActorExtra[iActor].movflag;
    case ACTOR_HTTEMPANG: return ActorExtra[iActor].tempang;
    case ACTOR_HTACTORSTAYPUT: return ActorExtra[iActor].actorstayput;
    case ACTOR_HTDISPICNUM: return ActorExtra[iActor].dispicnum;
    case ACTOR_HTTIMETOSLEEP: return ActorExtra[iActor].timetosleep;
    case ACTOR_HTFLOORZ: return ActorExtra[iActor].floorz;
    case ACTOR_HTCEILINGZ: return ActorExtra[iActor].ceilingz;
    case ACTOR_HTLASTVX: return ActorExtra[iActor].lastvx;
    case ACTOR_HTLASTVY: return ActorExtra[iActor].lastvy;
    case ACTOR_HTBPOSX: return ActorExtra[iActor].bposx;
    case ACTOR_HTBPOSY: return ActorExtra[iActor].bposy;
    case ACTOR_HTBPOSZ: return ActorExtra[iActor].bposz;
    case ACTOR_HTG_T: return ActorExtra[iActor].temp_data[lParm2];
    case ACTOR_ANGOFF: return spriteext[iActor].angoff;
    case ACTOR_PITCH: return spriteext[iActor].pitch;
    case ACTOR_ROLL: return spriteext[iActor].roll;
    case ACTOR_MDXOFF: return spriteext[iActor].xoff;
    case ACTOR_MDYOFF: return spriteext[iActor].yoff;
    case ACTOR_MDZOFF: return spriteext[iActor].zoff;
    case ACTOR_MDFLAGS: return spriteext[iActor].flags;
    case ACTOR_XPANNING: return spriteext[iActor].xpanning;
    case ACTOR_YPANNING: return spriteext[iActor].ypanning;
    case ACTOR_HTFLAGS: return ActorExtra[iActor].flags;
    case ACTOR_ALPHA: return (uint8_t)(spriteext[iActor].alpha*255.0f);
    default: return -1;
    }
}

static int32_t X_AccessSectorX(int32_t iSector, int32_t lLabelID)
{
    switch (lLabelID)
    {
    case SECTOR_WALLPTR: return sector[iSector].wallptr;
    case SECTOR_WALLNUM: return sector[iSector].wallnum;
    case SECTOR_CEILINGZ: return sector[iSector].ceilingz;
    case SECTOR_FLOORZ: return sector[iSector].floorz;
    case SECTOR_CEILINGSTAT: return sector[iSector].ceilingstat;
    case SECTOR_FLOORSTAT: return sector[iSector].floorstat;
    case SECTOR_CEILINGPICNUM: return sector[iSector].ceilingpicnum;
    case SECTOR_CEILINGSLOPE: return sector[iSector].ceilingheinum;
    case SECTOR_CEILINGSHADE: return sector[iSector].ceilingshade;
    case SECTOR_CEILINGPAL: return sector[iSector].ceilingpal;
    case SECTOR_CEILINGXPANNING: return sector[iSector].ceilingxpanning;
    case SECTOR_CEILINGYPANNING: return sector[iSector].ceilingypanning;
    case SECTOR_FLOORPICNUM: return sector[iSector].floorpicnum;
    case SECTOR_FLOORSLOPE: return sector[iSector].floorheinum;
    case SECTOR_FLOORSHADE: return sector[iSector].floorshade;
    case SECTOR_FLOORPAL: return sector[iSector].floorpal;
    case SECTOR_FLOORXPANNING: return sector[iSector].floorxpanning;
    case SECTOR_FLOORYPANNING: return sector[iSector].floorypanning;
    case SECTOR_VISIBILITY: return sector[iSector].visibility;
    case SECTOR_ALIGNTO: return sector[iSector].filler;
    case SECTOR_LOTAG: return sector[iSector].lotag;
    case SECTOR_HITAG: return sector[iSector].hitag;
    case SECTOR_EXTRA: return sector[iSector].extra;
    default: return -1;
    }
}

static int32_t X_AccessPlayerX(int32_t iPlayer, int32_t lLabelID, int32_t lParm2)
{
    if ((PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= PlayerLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on player (%d) from spr %d\n",
                   g_errorLineNum,keyw[g_tw],PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
    }

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        return g_player[iPlayer].ps->zoom;
    case PLAYER_EXITX:
        return g_player[iPlayer].ps->exitx;
    case PLAYER_EXITY:
        return g_player[iPlayer].ps->exity;
    case PLAYER_LOOGIEX:
        return g_player[iPlayer].ps->loogiex[lParm2];
    case PLAYER_LOOGIEY:
        return g_player[iPlayer].ps->loogiey[lParm2];
    case PLAYER_NUMLOOGS:
        return g_player[iPlayer].ps->numloogs;
    case PLAYER_LOOGCNT:
        return g_player[iPlayer].ps->loogcnt;
    case PLAYER_POSX:
        return g_player[iPlayer].ps->posx;
    case PLAYER_POSY:
        return g_player[iPlayer].ps->posy;
    case PLAYER_POSZ:
        return g_player[iPlayer].ps->posz;
    case PLAYER_HORIZ:
        return g_player[iPlayer].ps->horiz;
    case PLAYER_OHORIZ:
        return g_player[iPlayer].ps->ohoriz;
    case PLAYER_OHORIZOFF:
        return g_player[iPlayer].ps->ohorizoff;
    case PLAYER_INVDISPTIME:
        return g_player[iPlayer].ps->invdisptime;
    case PLAYER_BOBPOSX:
        return g_player[iPlayer].ps->bobposx;
    case PLAYER_BOBPOSY:
        return g_player[iPlayer].ps->bobposy;
    case PLAYER_OPOSX:
        return g_player[iPlayer].ps->oposx;
    case PLAYER_OPOSY:
        return g_player[iPlayer].ps->oposy;
    case PLAYER_OPOSZ:
        return g_player[iPlayer].ps->oposz;
    case PLAYER_PYOFF:
        return g_player[iPlayer].ps->pyoff;
    case PLAYER_OPYOFF:
        return g_player[iPlayer].ps->opyoff;
    case PLAYER_POSXV:
        return g_player[iPlayer].ps->posxv;
    case PLAYER_POSYV:
        return g_player[iPlayer].ps->posyv;
    case PLAYER_POSZV:
        return g_player[iPlayer].ps->poszv;
    case PLAYER_LAST_PISSED_TIME:
        return g_player[iPlayer].ps->last_pissed_time;
    case PLAYER_TRUEFZ:
        return g_player[iPlayer].ps->truefz;
    case PLAYER_TRUECZ:
        return g_player[iPlayer].ps->truecz;
    case PLAYER_PLAYER_PAR:
        return g_player[iPlayer].ps->player_par;
    case PLAYER_VISIBILITY:
        return g_player[iPlayer].ps->visibility;
    case PLAYER_BOBCOUNTER:
        return g_player[iPlayer].ps->bobcounter;
    case PLAYER_WEAPON_SWAY:
        return g_player[iPlayer].ps->weapon_sway;
    case PLAYER_PALS_TIME:
        return g_player[iPlayer].ps->pals_time;
    case PLAYER_RANDOMFLAMEX:
        return g_player[iPlayer].ps->randomflamex;
    case PLAYER_CRACK_TIME:
        return g_player[iPlayer].ps->crack_time;
    case PLAYER_AIM_MODE:
        return g_player[iPlayer].ps->aim_mode;
    case PLAYER_ANG:
        return g_player[iPlayer].ps->ang;
    case PLAYER_OANG:
        return g_player[iPlayer].ps->oang;
    case PLAYER_ANGVEL:
        return g_player[iPlayer].ps->angvel;
    case PLAYER_CURSECTNUM:
        return g_player[iPlayer].ps->cursectnum;
    case PLAYER_LOOK_ANG:
        return g_player[iPlayer].ps->look_ang;
    case PLAYER_LAST_EXTRA:
        return g_player[iPlayer].ps->last_extra;
    case PLAYER_SUBWEAPON:
        return g_player[iPlayer].ps->subweapon;
    case PLAYER_AMMO_AMOUNT:
        return g_player[iPlayer].ps->ammo_amount[lParm2];
    case PLAYER_WACKEDBYACTOR:
        return g_player[iPlayer].ps->wackedbyactor;
    case PLAYER_FRAG:
        return g_player[iPlayer].ps->frag;
    case PLAYER_FRAGGEDSELF:
        return g_player[iPlayer].ps->fraggedself;
    case PLAYER_CURR_WEAPON:
        return g_player[iPlayer].ps->curr_weapon;
    case PLAYER_LAST_WEAPON:
        return g_player[iPlayer].ps->last_weapon;
    case PLAYER_TIPINCS:
        return g_player[iPlayer].ps->tipincs;
    case PLAYER_HORIZOFF:
        return g_player[iPlayer].ps->horizoff;
    case PLAYER_WANTWEAPONFIRE:
        return g_player[iPlayer].ps->wantweaponfire;
    case PLAYER_HOLODUKE_AMOUNT:
        return g_player[iPlayer].ps->holoduke_amount;
    case PLAYER_NEWOWNER:
        return g_player[iPlayer].ps->newowner;
    case PLAYER_HURT_DELAY:
        return g_player[iPlayer].ps->hurt_delay;
    case PLAYER_HBOMB_HOLD_DELAY:
        return g_player[iPlayer].ps->hbomb_hold_delay;
    case PLAYER_JUMPING_COUNTER:
        return g_player[iPlayer].ps->jumping_counter;
    case PLAYER_AIRLEFT:
        return g_player[iPlayer].ps->airleft;
    case PLAYER_KNEE_INCS:
        return g_player[iPlayer].ps->knee_incs;
    case PLAYER_ACCESS_INCS:
        return g_player[iPlayer].ps->access_incs;
    case PLAYER_FTA:
        return g_player[iPlayer].ps->fta;
    case PLAYER_FTQ:
        return g_player[iPlayer].ps->ftq;
    case PLAYER_ACCESS_WALLNUM:
        return g_player[iPlayer].ps->access_wallnum;
    case PLAYER_ACCESS_SPRITENUM:
        return g_player[iPlayer].ps->access_spritenum;
    case PLAYER_KICKBACK_PIC:
        return g_player[iPlayer].ps->kickback_pic;
    case PLAYER_GOT_ACCESS:
        return g_player[iPlayer].ps->got_access;
    case PLAYER_WEAPON_ANG:
        return g_player[iPlayer].ps->weapon_ang;
    case PLAYER_FIRSTAID_AMOUNT:
        return g_player[iPlayer].ps->firstaid_amount;
    case PLAYER_SOMETHINGONPLAYER:
        return g_player[iPlayer].ps->somethingonplayer;
    case PLAYER_ON_CRANE:
        return g_player[iPlayer].ps->on_crane;
    case PLAYER_I:
        return g_player[iPlayer].ps->i;
    case PLAYER_ONE_PARALLAX_SECTNUM:
        return g_player[iPlayer].ps->one_parallax_sectnum;
    case PLAYER_OVER_SHOULDER_ON:
        return g_player[iPlayer].ps->over_shoulder_on;
    case PLAYER_RANDOM_CLUB_FRAME:
        return g_player[iPlayer].ps->random_club_frame;
    case PLAYER_FIST_INCS:
        return g_player[iPlayer].ps->fist_incs;
    case PLAYER_ONE_EIGHTY_COUNT:
        return g_player[iPlayer].ps->one_eighty_count;
    case PLAYER_CHEAT_PHASE:
        return g_player[iPlayer].ps->cheat_phase;
    case PLAYER_DUMMYPLAYERSPRITE:
        return g_player[iPlayer].ps->dummyplayersprite;
    case PLAYER_EXTRA_EXTRA8:
        return g_player[iPlayer].ps->extra_extra8;
    case PLAYER_QUICK_KICK:
        return g_player[iPlayer].ps->quick_kick;
    case PLAYER_HEAT_AMOUNT:
        return g_player[iPlayer].ps->heat_amount;
    case PLAYER_ACTORSQU:
        return g_player[iPlayer].ps->actorsqu;
    case PLAYER_TIMEBEFOREEXIT:
        return g_player[iPlayer].ps->timebeforeexit;
    case PLAYER_CUSTOMEXITSOUND:
        return g_player[iPlayer].ps->customexitsound;
    case PLAYER_WEAPRECS:
        return g_player[iPlayer].ps->weaprecs[15];
    case PLAYER_WEAPRECCNT:
        return g_player[iPlayer].ps->weapreccnt;
    case PLAYER_INTERFACE_TOGGLE_FLAG:
        return g_player[iPlayer].ps->interface_toggle_flag;
    case PLAYER_ROTSCRNANG:
        return g_player[iPlayer].ps->rotscrnang;
    case PLAYER_DEAD_FLAG:
        return g_player[iPlayer].ps->dead_flag;
    case PLAYER_SHOW_EMPTY_WEAPON:
        return g_player[iPlayer].ps->show_empty_weapon;
    case PLAYER_SCUBA_AMOUNT:
        return g_player[iPlayer].ps->scuba_amount;
    case PLAYER_JETPACK_AMOUNT:
        return g_player[iPlayer].ps->jetpack_amount;
    case PLAYER_STEROIDS_AMOUNT:
        return g_player[iPlayer].ps->steroids_amount;
    case PLAYER_SHIELD_AMOUNT:
        return g_player[iPlayer].ps->shield_amount;
    case PLAYER_HOLODUKE_ON:
        return g_player[iPlayer].ps->holoduke_on;
    case PLAYER_PYCOUNT:
        return g_player[iPlayer].ps->pycount;
    case PLAYER_WEAPON_POS:
        return g_player[iPlayer].ps->weapon_pos;
    case PLAYER_FRAG_PS:
        return g_player[iPlayer].ps->frag_ps;
    case PLAYER_TRANSPORTER_HOLD:
        return g_player[iPlayer].ps->transporter_hold;
    case PLAYER_LAST_FULL_WEAPON:
        return g_player[iPlayer].ps->last_full_weapon;
    case PLAYER_FOOTPRINTSHADE:
        return g_player[iPlayer].ps->footprintshade;
    case PLAYER_BOOT_AMOUNT:
        return g_player[iPlayer].ps->boot_amount;
    case PLAYER_SCREAM_VOICE:
        return g_player[iPlayer].ps->scream_voice;
    case PLAYER_GM:
        return g_player[iPlayer].ps->gm;
    case PLAYER_ON_WARPING_SECTOR:
        return g_player[iPlayer].ps->on_warping_sector;
    case PLAYER_FOOTPRINTCOUNT:
        return g_player[iPlayer].ps->footprintcount;
    case PLAYER_HBOMB_ON:
        return g_player[iPlayer].ps->hbomb_on;
    case PLAYER_JUMPING_TOGGLE:
        return g_player[iPlayer].ps->jumping_toggle;
    case PLAYER_RAPID_FIRE_HOLD:
        return g_player[iPlayer].ps->rapid_fire_hold;
    case PLAYER_ON_GROUND:
        return g_player[iPlayer].ps->on_ground;
    case PLAYER_INVEN_ICON:
        return g_player[iPlayer].ps->inven_icon;
    case PLAYER_BUTTONPALETTE:
        return g_player[iPlayer].ps->buttonpalette;
    case PLAYER_JETPACK_ON:
        return g_player[iPlayer].ps->jetpack_on;
    case PLAYER_SPRITEBRIDGE:
        return g_player[iPlayer].ps->spritebridge;
    case PLAYER_LASTRANDOMSPOT:
        return g_player[iPlayer].ps->lastrandomspot;
    case PLAYER_SCUBA_ON:
        return g_player[iPlayer].ps->scuba_on;
    case PLAYER_FOOTPRINTPAL:
        return g_player[iPlayer].ps->footprintpal;
    case PLAYER_HEAT_ON:
        return g_player[iPlayer].ps->heat_on;
    case PLAYER_HOLSTER_WEAPON:
        return g_player[iPlayer].ps->holster_weapon;
    case PLAYER_FALLING_COUNTER:
        return g_player[iPlayer].ps->falling_counter;
    case PLAYER_GOTWEAPON:
        return g_player[iPlayer].ps->gotweapon[lParm2];
    case PLAYER_REFRESH_INVENTORY:
        return g_player[iPlayer].ps->refresh_inventory;
    case PLAYER_TOGGLE_KEY_FLAG:
        return g_player[iPlayer].ps->toggle_key_flag;
    case PLAYER_KNUCKLE_INCS:
        return g_player[iPlayer].ps->knuckle_incs;
    case PLAYER_WALKING_SND_TOGGLE:
        return g_player[iPlayer].ps->walking_snd_toggle;
    case PLAYER_PALOOKUP:
        return g_player[iPlayer].ps->palookup;
    case PLAYER_HARD_LANDING:
        return g_player[iPlayer].ps->hard_landing;
    case PLAYER_MAX_SECRET_ROOMS:
        return g_player[iPlayer].ps->max_secret_rooms;
    case PLAYER_SECRET_ROOMS:
        return g_player[iPlayer].ps->secret_rooms;
    case PLAYER_PALS:
        return g_player[iPlayer].ps->pals[lParm2];
    case PLAYER_MAX_ACTORS_KILLED:
        return g_player[iPlayer].ps->max_actors_killed;
    case PLAYER_ACTORS_KILLED:
        return g_player[iPlayer].ps->actors_killed;
    case PLAYER_RETURN_TO_CENTER:
        return g_player[iPlayer].ps->return_to_center;
    case PLAYER_RUNSPEED:
        return g_player[iPlayer].ps->runspeed;
    case PLAYER_SBS:
        return g_player[iPlayer].ps->sbs;
    case PLAYER_RELOADING:
        return g_player[iPlayer].ps->reloading;
    case PLAYER_AUTO_AIM:
        return g_player[iPlayer].ps->auto_aim;
    case PLAYER_MOVEMENT_LOCK:
        return g_player[iPlayer].ps->movement_lock;
    case PLAYER_SOUND_PITCH:
        return g_player[iPlayer].ps->sound_pitch;
    case PLAYER_WEAPONSWITCH:
        return g_player[iPlayer].ps->weaponswitch;
    case PLAYER_TEAM:
        return g_player[iPlayer].ps->team;
    case PLAYER_MAX_PLAYER_HEALTH:
        return g_player[iPlayer].ps->max_player_health;
    case PLAYER_MAX_SHIELD_AMOUNT:
        return g_player[iPlayer].ps->max_shield_amount;
    case PLAYER_MAX_AMMO_AMOUNT:
        return g_player[iPlayer].ps->max_ammo_amount[lParm2];
    case PLAYER_LAST_QUICK_KICK:
        return g_player[iPlayer].ps->last_quick_kick;
    default:
        return -1;
    }
}

static int32_t X_AccessWallX(int32_t iWall, int32_t lLabelID)
{
    switch (lLabelID)
    {
    case WALL_X:
        return wall[iWall].x;
    case WALL_Y:
        return wall[iWall].y;
    case WALL_POINT2:
        return wall[iWall].point2;
    case WALL_NEXTWALL:
        return wall[iWall].nextwall;
    case WALL_NEXTSECTOR:
        return wall[iWall].nextsector;
    case WALL_CSTAT:
        return wall[iWall].cstat;
    case WALL_PICNUM:
        return wall[iWall].picnum;
    case WALL_OVERPICNUM:
        return wall[iWall].overpicnum;
    case WALL_SHADE:
        return wall[iWall].shade;
    case WALL_PAL:
        return wall[iWall].pal;
    case WALL_XREPEAT:
        return wall[iWall].xrepeat;
    case WALL_YREPEAT:
        return wall[iWall].yrepeat;
    case WALL_XPANNING:
        return wall[iWall].xpanning;
    case WALL_YPANNING:
        return wall[iWall].ypanning;
    case WALL_LOTAG:
        return wall[iWall].lotag;
    case WALL_HITAG:
        return wall[iWall].hitag;
    case WALL_EXTRA:
        return wall[iWall].extra;
    default:
        return -1;
    }
}
#endif // _gamevars_c_
