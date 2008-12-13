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

static inline void X_AccessUserdef(int iSet, int lLabelID, int lVar2)
{
    int lValue=0;

    if (g_p != myconnectindex)
    {
//        if (lVar2 == MAXGAMEVARS)
//            insptr++;
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case USERDEFS_GOD:
        if (iSet)
        {
            ud.god = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.god, g_i, g_p);
        return;

    case USERDEFS_WARP_ON:
        if (iSet)
        {
            ud.warp_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.warp_on, g_i, g_p);
        return;

    case USERDEFS_CASHMAN:
        if (iSet)
        {
            ud.cashman = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.cashman, g_i, g_p);
        return;

    case USERDEFS_EOG:
        if (iSet)
        {
            ud.eog = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.eog, g_i, g_p);
        return;

    case USERDEFS_SHOWALLMAP:
        if (iSet)
        {
            ud.showallmap = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.showallmap, g_i, g_p);
        return;

    case USERDEFS_SHOW_HELP:
        if (iSet)
        {
            ud.show_help = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.show_help, g_i, g_p);
        return;

    case USERDEFS_SCROLLMODE:
        if (iSet)
        {
            ud.scrollmode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.scrollmode, g_i, g_p);
        return;

    case USERDEFS_CLIPPING:
        if (iSet)
        {
            ud.clipping = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.clipping, g_i, g_p);
        return;

        //  case USERDEFS_USER_NAME:
        //      if(iSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.user_name[MAXPLAYERS][32], g_i, g_p);
        //      return;

        //  case USERDEFS_RIDECULE:
        //      if(iSet)
        //      {
        //          ud.ridecule = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.ridecule, g_i, g_p);
        //      return;

        //  case USERDEFS_SAVEGAME:
        //      if(iSet)
        //      {
        //          ud.savegame = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.savegame, g_i, g_p);
        //      return;

        //  case USERDEFS_PWLOCKOUT:
        //      if(iSet)
        //      {
        //          ud.pwlockout = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.pwlockout, g_i, g_p);
        //      return;

        //  case USERDEFS_RTSNAME:
        //      if(iSet)
        //      {
        //          ud.rtsname = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.rtsname, g_i, g_p);
        //      return;

    case USERDEFS_OVERHEAD_ON:
        if (iSet)
        {
            ud.overhead_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.overhead_on, g_i, g_p);
        return;

    case USERDEFS_LAST_OVERHEAD:
        if (iSet)
        {
            ud.last_overhead = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_overhead, g_i, g_p);
        return;

    case USERDEFS_SHOWWEAPONS:
        if (iSet)
        {
            ud.showweapons = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.showweapons, g_i, g_p);
        return;

    case USERDEFS_PAUSE_ON:
        if (iSet)
        {
            ud.pause_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.pause_on, g_i, g_p);
        return;

    case USERDEFS_FROM_BONUS:
        if (iSet)
        {
            ud.from_bonus = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.from_bonus, g_i, g_p);
        return;

    case USERDEFS_CAMERASPRITE:
        if (iSet)
        {
            ud.camerasprite = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.camerasprite, g_i, g_p);
        return;

    case USERDEFS_LAST_CAMSPRITE:
        if (iSet)
        {
            ud.last_camsprite = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_camsprite, g_i, g_p);
        return;

    case USERDEFS_LAST_LEVEL:
        if (iSet)
        {
            ud.last_level = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.last_level, g_i, g_p);
        return;

    case USERDEFS_SECRETLEVEL:
        if (iSet)
        {
            ud.secretlevel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.secretlevel, g_i, g_p);
        return;

    case USERDEFS_CONST_VISIBILITY:
        if (iSet)
        {
            ud.const_visibility = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.const_visibility, g_i, g_p);
        return;

    case USERDEFS_UW_FRAMERATE:
        if (iSet)
        {
            ud.uw_framerate = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.uw_framerate, g_i, g_p);
        return;

    case USERDEFS_CAMERA_TIME:
        if (iSet)
        {
            ud.camera_time = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.camera_time, g_i, g_p);
        return;

    case USERDEFS_FOLFVEL:
        if (iSet)
        {
            ud.folfvel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folfvel, g_i, g_p);
        return;

    case USERDEFS_FOLAVEL:
        if (iSet)
        {
            ud.folavel = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folavel, g_i, g_p);
        return;

    case USERDEFS_FOLX:
        if (iSet)
        {
            ud.folx = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.folx, g_i, g_p);
        return;

    case USERDEFS_FOLY:
        if (iSet)
        {
            ud.foly = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.foly, g_i, g_p);
        return;

    case USERDEFS_FOLA:
        if (iSet)
        {
            ud.fola = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.fola, g_i, g_p);
        return;

    case USERDEFS_RECCNT:
        if (iSet)
        {
            ud.reccnt = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.reccnt, g_i, g_p);
        return;

    case USERDEFS_ENTERED_NAME:
        if (iSet)
        {
            ud.entered_name = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.entered_name, g_i, g_p);
        return;

    case USERDEFS_SCREEN_TILTING:
        if (iSet)
        {
            ud.screen_tilting = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.screen_tilting, g_i, g_p);
        return;

    case USERDEFS_SHADOWS:
        if (iSet)
        {
            ud.shadows = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.shadows, g_i, g_p);
        return;

    case USERDEFS_FTA_ON:
        if (iSet)
        {
            ud.fta_on = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.fta_on, g_i, g_p);
        return;

    case USERDEFS_EXECUTIONS:
        if (iSet)
        {
            ud.executions = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.executions, g_i, g_p);
        return;

    case USERDEFS_AUTO_RUN:
        if (iSet)
        {
            ud.auto_run = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.auto_run, g_i, g_p);
        return;

    case USERDEFS_COORDS:
        if (iSet)
        {
            ud.coords = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.coords, g_i, g_p);
        return;

    case USERDEFS_TICKRATE:
        if (iSet)
        {
            ud.tickrate = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.tickrate, g_i, g_p);
        return;

    case USERDEFS_M_COOP:
        if (iSet)
        {
            ud.m_coop = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_coop, g_i, g_p);
        return;

    case USERDEFS_COOP:
        if (iSet)
        {
            ud.coop = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.coop, g_i, g_p);
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
        Gv_SetVar(lVar2, ud.screen_size, g_i, g_p);
        return;

    case USERDEFS_LOCKOUT:
        if (iSet)
        {
            ud.lockout = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.lockout, g_i, g_p);
        return;

    case USERDEFS_CROSSHAIR:
        if (iSet)
        {
            ud.crosshair = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.crosshair, g_i, g_p);
        return;

        //  case USERDEFS_WCHOICE:
        //      if(iSet)
        //      {
        //          ud.wchoice = lValue;
        //          return;
        //      }
        //      Gv_SetVar(lVar2, ud.wchoice, g_i, g_p);
        //      return;

    case USERDEFS_PLAYERAI:
        if (iSet)
        {
            ud.playerai = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.playerai, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.respawn_monsters = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_monsters, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.respawn_items = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_items, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.respawn_inventory = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.respawn_inventory, g_i, g_p);
        return;

    case USERDEFS_RECSTAT:
        if (iSet)
        {
            ud.recstat = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.recstat, g_i, g_p);
        return;

    case USERDEFS_MONSTERS_OFF:
        if (iSet)
        {
            ud.monsters_off = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.monsters_off, g_i, g_p);
        return;

    case USERDEFS_BRIGHTNESS:
        if (iSet)
        {
            ud.brightness = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.brightness, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.m_respawn_items = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_items, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.m_respawn_monsters = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_monsters, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.m_respawn_inventory = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_respawn_inventory, g_i, g_p);
        return;

    case USERDEFS_M_RECSTAT:
        if (iSet)
        {
            ud.m_recstat = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_recstat, g_i, g_p);
        return;

    case USERDEFS_M_MONSTERS_OFF:
        if (iSet)
        {
            ud.m_monsters_off = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_monsters_off, g_i, g_p);
        return;

    case USERDEFS_DETAIL:
        if (iSet)
        {
            ud.detail = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.detail, g_i, g_p);
        return;

    case USERDEFS_M_FFIRE:
        if (iSet)
        {
            ud.m_ffire = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_ffire, g_i, g_p);
        return;

    case USERDEFS_FFIRE:
        if (iSet)
        {
            ud.ffire = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.ffire, g_i, g_p);
        return;

    case USERDEFS_M_PLAYER_SKILL:
        if (iSet)
        {
            ud.m_player_skill = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_player_skill, g_i, g_p);
        return;

    case USERDEFS_M_LEVEL_NUMBER:
        if (iSet)
        {
            ud.m_level_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_level_number, g_i, g_p);
        return;

    case USERDEFS_M_VOLUME_NUMBER:
        if (iSet)
        {
            ud.m_volume_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_volume_number, g_i, g_p);
        return;

    case USERDEFS_MULTIMODE:
        if (iSet)
        {
            ud.multimode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.multimode, g_i, g_p);
        return;

    case USERDEFS_PLAYER_SKILL:
        if (iSet)
        {
            ud.player_skill = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.player_skill, g_i, g_p);
        return;

    case USERDEFS_LEVEL_NUMBER:
        if (iSet)
        {
            ud.level_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.level_number, g_i, g_p);
        return;

    case USERDEFS_VOLUME_NUMBER:
        if (iSet)
        {
            ud.volume_number = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.volume_number, g_i, g_p);
        return;

    case USERDEFS_M_MARKER:
        if (iSet)
        {
            ud.m_marker = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_marker, g_i, g_p);
        return;

    case USERDEFS_MARKER:
        if (iSet)
        {
            ud.marker = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.marker, g_i, g_p);
        return;

    case USERDEFS_MOUSEFLIP:
        if (iSet)
        {
            ud.mouseflip = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.mouseflip, g_i, g_p);
        return;

    case USERDEFS_STATUSBARSCALE:
        if (iSet)
        {
            ud.statusbarscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.statusbarscale, g_i, g_p);
        return;

    case USERDEFS_DRAWWEAPON:
        if (iSet)
        {
            ud.drawweapon = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.drawweapon, g_i, g_p);
        return;

    case USERDEFS_MOUSEAIMING:
        if (iSet)
        {
            ud.mouseaiming = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.mouseaiming, g_i, g_p);
        return;

    case USERDEFS_WEAPONSWITCH:
        if (iSet)
        {
            ud.weaponswitch = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponswitch, g_i, g_p);
        return;

    case USERDEFS_DEMOCAMS:
        if (iSet)
        {
            ud.democams = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.democams, g_i, g_p);
        return;

    case USERDEFS_COLOR:
        if (iSet)
        {
            ud.color = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.color, g_i, g_p);
        return;

    case USERDEFS_MSGDISPTIME:
        if (iSet)
        {
            ud.msgdisptime = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.msgdisptime, g_i, g_p);
        return;

    case USERDEFS_STATUSBARMODE:
        if (iSet)
        {
            ud.statusbarmode = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.statusbarmode, g_i, g_p);
        return;

    case USERDEFS_M_NOEXITS:
        if (iSet)
        {
            ud.m_noexits = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.m_noexits, g_i, g_p);
        return;

    case USERDEFS_NOEXITS:
        if (iSet)
        {
            ud.noexits = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.noexits, g_i, g_p);
        return;

    case USERDEFS_AUTOVOTE:
        if (iSet)
        {
            ud.autovote = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.autovote, g_i, g_p);
        return;

    case USERDEFS_AUTOMSG:
        if (iSet)
        {
            ud.automsg = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.automsg, g_i, g_p);
        return;

    case USERDEFS_IDPLAYERS:
        if (iSet)
        {
            ud.idplayers = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.idplayers, g_i, g_p);
        return;

    case USERDEFS_TEAM:
        if (iSet)
        {
            ud.team = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.team, g_i, g_p);
        return;

    case USERDEFS_VIEWBOB:
        if (iSet)
        {
            ud.viewbob = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.viewbob, g_i, g_p);
        return;

    case USERDEFS_WEAPONSWAY:
        if (iSet)
        {
            ud.weaponsway = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponsway, g_i, g_p);
        return;

    case USERDEFS_ANGLEINTERPOLATION:
        if (iSet)
        {
            ud.angleinterpolation = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.angleinterpolation, g_i, g_p);
        return;

    case USERDEFS_OBITUARIES:
        if (iSet)
        {
            ud.obituaries = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.obituaries, g_i, g_p);
        return;

    case USERDEFS_LEVELSTATS:
        if (iSet)
        {
            ud.levelstats = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.levelstats, g_i, g_p);
        return;

    case USERDEFS_CROSSHAIRSCALE:
        if (iSet)
        {
            ud.crosshairscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.crosshairscale, g_i, g_p);
        return;

    case USERDEFS_ALTHUD:
        if (iSet)
        {
            ud.althud = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.althud, g_i, g_p);
        return;

    case USERDEFS_DISPLAY_BONUS_SCREEN:
        if (iSet)
        {
            ud.display_bonus_screen = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.display_bonus_screen, g_i, g_p);
        return;

    case USERDEFS_SHOW_LEVEL_TEXT:
        if (iSet)
        {
            ud.show_level_text = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.show_level_text, g_i, g_p);
        return;

    case USERDEFS_WEAPONSCALE:
        if (iSet)
        {
            ud.weaponscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.weaponscale, g_i, g_p);
        return;

    case USERDEFS_TEXTSCALE:
        if (iSet)
        {
            ud.textscale = lValue;
            return;
        }
        Gv_SetVar(lVar2, ud.textscale, g_i, g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessActiveProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0,proj=g_i;

    if (lVar1 != g_iThisActorID)
        proj=Gv_GetVar(lVar1, g_i, g_p);

    if ((proj < 0 || proj >= MAXSPRITES) && g_scriptSanityChecks)
    {
//        OSD_Printf("X_AccessActiveProjectile(): invalid projectile (%d)\n",proj);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target projectile (%d) %d %d from %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ProjectileLabels[lLabelID].name,proj,g_i,g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            ActorExtra[proj].projectile.workslike=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.workslike, g_i, g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            ActorExtra[proj].projectile.spawns=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.spawns, g_i, g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.sxrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.sxrepeat, g_i, g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.syrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.syrepeat, g_i, g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.sound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.sound, g_i, g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.isound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.isound, g_i, g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            ActorExtra[proj].projectile.vel=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.vel, g_i, g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            ActorExtra[proj].projectile.extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.extra, g_i, g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            ActorExtra[proj].projectile.decal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.decal, g_i, g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            ActorExtra[proj].projectile.trail=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.trail, g_i, g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.txrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.txrepeat, g_i, g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.tyrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.tyrepeat, g_i, g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            ActorExtra[proj].projectile.toffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.toffset, g_i, g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            ActorExtra[proj].projectile.tnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.tnum, g_i, g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            ActorExtra[proj].projectile.drop=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.drop, g_i, g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.cstat, g_i, g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            ActorExtra[proj].projectile.clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.clipdist, g_i, g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            ActorExtra[proj].projectile.shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.shade, g_i, g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.xrepeat, g_i, g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            ActorExtra[proj].projectile.yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.yrepeat, g_i, g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            ActorExtra[proj].projectile.pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.pal, g_i, g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            ActorExtra[proj].projectile.extra_rand=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.extra_rand, g_i, g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            ActorExtra[proj].projectile.hitradius=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.hitradius, g_i, g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            ActorExtra[proj].projectile.velmult=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.velmult, g_i, g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            ActorExtra[proj].projectile.offset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.offset, g_i, g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            ActorExtra[proj].projectile.bounces=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.bounces, g_i, g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            ActorExtra[proj].projectile.bsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.bsound, g_i, g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            ActorExtra[proj].projectile.range=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[proj].projectile.range, g_i, g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessPlayer(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    int lValue=0;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVar(lVar1, g_i, g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && g_scriptSanityChecks)
    {
//        OSD_Printf("X_AccessPlayer(): invalid target player (%d) %d\n",iPlayer,g_i);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target player (%d) from spr %d gv %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",PlayerLabels[lLabelID].name,iPlayer,g_i,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= PlayerLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on player (%d) from spr %d\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",PlayerLabels[lLabelID].name,lParm2,iPlayer,g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        if (iSet)
        {
            g_player[iPlayer].ps->zoom=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->zoom, g_i, g_p);
        return;

    case PLAYER_EXITX:
        if (iSet)
        {
            g_player[iPlayer].ps->exitx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->exitx, g_i, g_p);
        return;

    case PLAYER_EXITY:
        if (iSet)
        {
            g_player[iPlayer].ps->exity=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->exity, g_i, g_p);
        return;

    case PLAYER_LOOGIEX:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiex[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogiex[lParm2], g_i, g_p);
        return;

    case PLAYER_LOOGIEY:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiey[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogiey[lParm2], g_i, g_p);
        return;

    case PLAYER_NUMLOOGS:
        if (iSet)
        {
            g_player[iPlayer].ps->numloogs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->numloogs, g_i, g_p);
        return;

    case PLAYER_LOOGCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->loogcnt=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->loogcnt, g_i, g_p);
        return;

    case PLAYER_POSX:
        if (iSet)
        {
            g_player[iPlayer].ps->posx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posx, g_i, g_p);
        return;

    case PLAYER_POSY:
        if (iSet)
        {
            g_player[iPlayer].ps->posy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posy, g_i, g_p);
        return;

    case PLAYER_POSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->posz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posz, g_i, g_p);
        return;

    case PLAYER_HORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->horiz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->horiz, g_i, g_p);
        return;

    case PLAYER_OHORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->ohoriz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ohoriz, g_i, g_p);
        return;

    case PLAYER_OHORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->ohorizoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ohorizoff, g_i, g_p);
        return;

    case PLAYER_INVDISPTIME:
        if (iSet)
        {
            g_player[iPlayer].ps->invdisptime=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->invdisptime, g_i, g_p);
        return;

    case PLAYER_BOBPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobposx, g_i, g_p);
        return;

    case PLAYER_BOBPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobposy, g_i, g_p);
        return;

    case PLAYER_OPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->oposx=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposx, g_i, g_p);
        return;

    case PLAYER_OPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->oposy=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposy, g_i, g_p);
        return;

    case PLAYER_OPOSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->oposz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oposz, g_i, g_p);
        return;

    case PLAYER_PYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->pyoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pyoff, g_i, g_p);
        return;

    case PLAYER_OPYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->opyoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->opyoff, g_i, g_p);
        return;

    case PLAYER_POSXV:
        if (iSet)
        {
            g_player[iPlayer].ps->posxv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posxv, g_i, g_p);
        return;

    case PLAYER_POSYV:
        if (iSet)
        {
            g_player[iPlayer].ps->posyv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->posyv, g_i, g_p);
        return;

    case PLAYER_POSZV:
        if (iSet)
        {
            g_player[iPlayer].ps->poszv=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->poszv, g_i, g_p);
        return;

    case PLAYER_LAST_PISSED_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->last_pissed_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_pissed_time, g_i, g_p);
        return;

    case PLAYER_TRUEFZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truefz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->truefz, g_i, g_p);
        return;

    case PLAYER_TRUECZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truecz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->truecz, g_i, g_p);
        return;

    case PLAYER_PLAYER_PAR:
        if (iSet)
        {
            g_player[iPlayer].ps->player_par=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->player_par, g_i, g_p);
        return;

    case PLAYER_VISIBILITY:
        if (iSet)
        {
            g_player[iPlayer].ps->visibility=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->visibility, g_i, g_p);
        return;

    case PLAYER_BOBCOUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->bobcounter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->bobcounter, g_i, g_p);
        return;

    case PLAYER_WEAPON_SWAY:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_sway=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_sway, g_i, g_p);
        return;

    case PLAYER_PALS_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->pals_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pals_time, g_i, g_p);
        return;

    case PLAYER_RANDOMFLAMEX:
        if (iSet)
        {
            g_player[iPlayer].ps->randomflamex=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->randomflamex, g_i, g_p);
        return;

    case PLAYER_CRACK_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->crack_time=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->crack_time, g_i, g_p);
        return;

    case PLAYER_AIM_MODE:
        if (iSet)
        {
            g_player[iPlayer].ps->aim_mode=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->aim_mode, g_i, g_p);
        return;

    case PLAYER_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ang, g_i, g_p);
        return;

    case PLAYER_OANG:
        if (iSet)
        {
            g_player[iPlayer].ps->oang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->oang, g_i, g_p);
        return;

    case PLAYER_ANGVEL:
        if (iSet)
        {
            g_player[iPlayer].ps->angvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->angvel, g_i, g_p);
        return;

    case PLAYER_CURSECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->cursectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->cursectnum, g_i, g_p);
        return;

    case PLAYER_LOOK_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->look_ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->look_ang, g_i, g_p);
        return;

    case PLAYER_LAST_EXTRA:
        if (iSet)
        {
            g_player[iPlayer].ps->last_extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_extra, g_i, g_p);
        return;

    case PLAYER_SUBWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->subweapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->subweapon, g_i, g_p);
        return;

    case PLAYER_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->ammo_amount[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ammo_amount[lParm2], g_i, g_p);
        return;

    case PLAYER_WACKEDBYACTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->wackedbyactor=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->wackedbyactor, g_i, g_p);
        return;

    case PLAYER_FRAG:
        if (iSet)
        {
            g_player[iPlayer].ps->frag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->frag, g_i, g_p);
        return;

    case PLAYER_FRAGGEDSELF:
        if (iSet)
        {
            g_player[iPlayer].ps->fraggedself=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fraggedself, g_i, g_p);
        return;

    case PLAYER_CURR_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->curr_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->curr_weapon, g_i, g_p);
        return;

    case PLAYER_LAST_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_weapon, g_i, g_p);
        return;

    case PLAYER_TIPINCS:
        if (iSet)
        {
            g_player[iPlayer].ps->tipincs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->tipincs, g_i, g_p);
        return;

    case PLAYER_HORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->horizoff=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->horizoff, g_i, g_p);
        return;

    case PLAYER_WANTWEAPONFIRE:
        if (iSet)
        {
            g_player[iPlayer].ps->wantweaponfire=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->wantweaponfire, g_i, g_p);
        return;

    case PLAYER_HOLODUKE_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holoduke_amount, g_i, g_p);
        return;

    case PLAYER_NEWOWNER:
        if (iSet)
        {
            g_player[iPlayer].ps->newowner=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->newowner, g_i, g_p);
        return;

    case PLAYER_HURT_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hurt_delay=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hurt_delay, g_i, g_p);
        return;

    case PLAYER_HBOMB_HOLD_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_hold_delay=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hbomb_hold_delay, g_i, g_p);
        return;

    case PLAYER_JUMPING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_counter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jumping_counter, g_i, g_p);
        return;

    case PLAYER_AIRLEFT:
        if (iSet)
        {
            g_player[iPlayer].ps->airleft=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->airleft, g_i, g_p);
        return;

    case PLAYER_KNEE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knee_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->knee_incs, g_i, g_p);
        return;

    case PLAYER_ACCESS_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->access_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_incs, g_i, g_p);
        return;

    case PLAYER_FTA:
        if (iSet)
        {
            g_player[iPlayer].ps->fta=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fta, g_i, g_p);
        return;

    case PLAYER_FTQ:
        if (iSet)
        {
            g_player[iPlayer].ps->ftq=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->ftq, g_i, g_p);
        return;

    case PLAYER_ACCESS_WALLNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_wallnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_wallnum, g_i, g_p);
        return;

    case PLAYER_ACCESS_SPRITENUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_spritenum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->access_spritenum, g_i, g_p);
        return;

    case PLAYER_KICKBACK_PIC:
        if (iSet)
        {
            g_player[iPlayer].ps->kickback_pic=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->kickback_pic, g_i, g_p);
        return;

    case PLAYER_GOT_ACCESS:
        if (iSet)
        {
            g_player[iPlayer].ps->got_access=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->got_access, g_i, g_p);
        return;

    case PLAYER_WEAPON_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_ang, g_i, g_p);
        return;

    case PLAYER_FIRSTAID_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->firstaid_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->firstaid_amount, g_i, g_p);
        return;

    case PLAYER_SOMETHINGONPLAYER:
        if (iSet)
        {
            g_player[iPlayer].ps->somethingonplayer=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->somethingonplayer, g_i, g_p);
        return;

    case PLAYER_ON_CRANE:
        if (iSet)
        {
            g_player[iPlayer].ps->on_crane=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_crane, g_i, g_p);
        return;

    case PLAYER_I:
        if (iSet)
        {
            g_player[iPlayer].ps->i=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->i, g_i, g_p);
        return;

    case PLAYER_ONE_PARALLAX_SECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->one_parallax_sectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->one_parallax_sectnum, g_i, g_p);
        return;

    case PLAYER_OVER_SHOULDER_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->over_shoulder_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->over_shoulder_on, g_i, g_p);
        return;

    case PLAYER_RANDOM_CLUB_FRAME:
        if (iSet)
        {
            g_player[iPlayer].ps->random_club_frame=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->random_club_frame, g_i, g_p);
        return;

    case PLAYER_FIST_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->fist_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->fist_incs, g_i, g_p);
        return;

    case PLAYER_ONE_EIGHTY_COUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->one_eighty_count=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->one_eighty_count, g_i, g_p);
        return;

    case PLAYER_CHEAT_PHASE:
        if (iSet)
        {
            g_player[iPlayer].ps->cheat_phase=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->cheat_phase, g_i, g_p);
        return;

    case PLAYER_DUMMYPLAYERSPRITE:
        if (iSet)
        {
            g_player[iPlayer].ps->dummyplayersprite=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->dummyplayersprite, g_i, g_p);
        return;

    case PLAYER_EXTRA_EXTRA8:
        if (iSet)
        {
            g_player[iPlayer].ps->extra_extra8=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->extra_extra8, g_i, g_p);
        return;

    case PLAYER_QUICK_KICK:
        if (iSet)
        {
            g_player[iPlayer].ps->quick_kick=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->quick_kick, g_i, g_p);
        return;

    case PLAYER_HEAT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->heat_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->heat_amount, g_i, g_p);
        return;

    case PLAYER_ACTORSQU:
        if (iSet)
        {
            g_player[iPlayer].ps->actorsqu=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->actorsqu, g_i, g_p);
        return;

    case PLAYER_TIMEBEFOREEXIT:
        if (iSet)
        {
            g_player[iPlayer].ps->timebeforeexit=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->timebeforeexit, g_i, g_p);
        return;

    case PLAYER_CUSTOMEXITSOUND:
        if (iSet)
        {
            g_player[iPlayer].ps->customexitsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->customexitsound, g_i, g_p);
        return;

    case PLAYER_WEAPRECS:
        if (iSet)
        {
            g_player[iPlayer].ps->weaprecs[15]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weaprecs[15], g_i, g_p);
        return;

    case PLAYER_WEAPRECCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->weapreccnt=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapreccnt, g_i, g_p);
        return;

    case PLAYER_INTERFACE_TOGGLE_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->interface_toggle_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->interface_toggle_flag, g_i, g_p);
        return;

    case PLAYER_ROTSCRNANG:
        if (iSet)
        {
            g_player[iPlayer].ps->rotscrnang=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->rotscrnang, g_i, g_p);
        return;

    case PLAYER_DEAD_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->dead_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->dead_flag, g_i, g_p);
        return;

    case PLAYER_SHOW_EMPTY_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->show_empty_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->show_empty_weapon, g_i, g_p);
        return;

    case PLAYER_SCUBA_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scuba_amount, g_i, g_p);
        return;

    case PLAYER_JETPACK_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jetpack_amount, g_i, g_p);
        return;

    case PLAYER_STEROIDS_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->steroids_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->steroids_amount, g_i, g_p);
        return;

    case PLAYER_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->shield_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->shield_amount, g_i, g_p);
        return;

    case PLAYER_HOLODUKE_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holoduke_on, g_i, g_p);
        return;

    case PLAYER_PYCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->pycount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pycount, g_i, g_p);
        return;

    case PLAYER_WEAPON_POS:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_pos=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weapon_pos, g_i, g_p);
        return;

    case PLAYER_FRAG_PS:
        if (iSet)
        {
            g_player[iPlayer].ps->frag_ps=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->frag_ps, g_i, g_p);
        return;

    case PLAYER_TRANSPORTER_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->transporter_hold=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->transporter_hold, g_i, g_p);
        return;

    case PLAYER_LAST_FULL_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_full_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_full_weapon, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTSHADE:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintshade, g_i, g_p);
        return;

    case PLAYER_BOOT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->boot_amount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->boot_amount, g_i, g_p);
        return;

    case PLAYER_SCREAM_VOICE:
        if (iSet)
        {
            g_player[iPlayer].ps->scream_voice=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scream_voice, g_i, g_p);
        return;

    case PLAYER_GM:
        if (iSet)
        {
            g_player[iPlayer].ps->gm=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->gm, g_i, g_p);
        return;

    case PLAYER_ON_WARPING_SECTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->on_warping_sector=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_warping_sector, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintcount=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintcount, g_i, g_p);
        return;

    case PLAYER_HBOMB_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hbomb_on, g_i, g_p);
        return;

    case PLAYER_JUMPING_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_toggle=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jumping_toggle, g_i, g_p);
        return;

    case PLAYER_RAPID_FIRE_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->rapid_fire_hold=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->rapid_fire_hold, g_i, g_p);
        return;

    case PLAYER_ON_GROUND:
        if (iSet)
        {
            g_player[iPlayer].ps->on_ground=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->on_ground, g_i, g_p);
        return;

        /*    case PLAYER_NAME:
                if (iSet)
                {
                    g_player[iPlayer].ps->name[32]=lValue;
                    return;
                }
                Gv_SetVar(lVar2, g_player[iPlayer].ps->name[32], g_i, g_p);
                return;*/

    case PLAYER_INVEN_ICON:
        if (iSet)
        {
            g_player[iPlayer].ps->inven_icon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->inven_icon, g_i, g_p);
        return;

    case PLAYER_BUTTONPALETTE:
        if (iSet)
        {
            g_player[iPlayer].ps->buttonpalette=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->buttonpalette, g_i, g_p);
        return;

    case PLAYER_JETPACK_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->jetpack_on, g_i, g_p);
        return;

    case PLAYER_SPRITEBRIDGE:
        if (iSet)
        {
            g_player[iPlayer].ps->spritebridge=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->spritebridge, g_i, g_p);
        return;

    case PLAYER_LASTRANDOMSPOT:
        if (iSet)
        {
            g_player[iPlayer].ps->lastrandomspot=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->lastrandomspot, g_i, g_p);
        return;

    case PLAYER_SCUBA_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_on=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->scuba_on, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTPAL:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->footprintpal, g_i, g_p);
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
        Gv_SetVar(lVar2, g_player[iPlayer].ps->heat_on, g_i, g_p);
        return;

    case PLAYER_HOLSTER_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->holster_weapon=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->holster_weapon, g_i, g_p);
        return;

    case PLAYER_FALLING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->falling_counter=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->falling_counter, g_i, g_p);
        return;

    case PLAYER_GOTWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->gotweapon[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->gotweapon[lParm2], g_i, g_p);
        return;

    case PLAYER_REFRESH_INVENTORY:
        if (iSet)
        {
            g_player[iPlayer].ps->refresh_inventory=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->refresh_inventory, g_i, g_p);
        return;

        //      case PLAYER_PALETTE:
        //          if(iSet)
        //          {
        //              g_player[iPlayer].ps->palette=lValue;
        //          }
        //          return; }
        //          {
        //              Gv_SetVar(lVar2, g_player[iPlayer].ps->palette, g_i, g_p);
        //          }
        //          return;

    case PLAYER_TOGGLE_KEY_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->toggle_key_flag=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->toggle_key_flag, g_i, g_p);
        return;

    case PLAYER_KNUCKLE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knuckle_incs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->knuckle_incs, g_i, g_p);
        return;

    case PLAYER_WALKING_SND_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->walking_snd_toggle=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->walking_snd_toggle, g_i, g_p);
        return;

    case PLAYER_PALOOKUP:
        if (iSet)
        {
            g_player[iPlayer].ps->palookup=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->palookup, g_i, g_p);
        return;

    case PLAYER_HARD_LANDING:
        if (iSet)
        {
            g_player[iPlayer].ps->hard_landing=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->hard_landing, g_i, g_p);
        return;

    case PLAYER_MAX_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->max_secret_rooms=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_secret_rooms, g_i, g_p);
        return;

    case PLAYER_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->secret_rooms=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->secret_rooms, g_i, g_p);
        return;

    case PLAYER_PALS:
        if (iSet)
        {
            g_player[iPlayer].ps->pals[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->pals[lParm2], g_i, g_p);
        return;

    case PLAYER_MAX_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->max_actors_killed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_actors_killed, g_i, g_p);
        return;

    case PLAYER_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->actors_killed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->actors_killed, g_i, g_p);
        return;

    case PLAYER_RETURN_TO_CENTER:
        if (iSet)
        {
            g_player[iPlayer].ps->return_to_center=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->return_to_center, g_i, g_p);
        return;

    case PLAYER_RUNSPEED:
        if (iSet)
        {
            g_player[iPlayer].ps->runspeed=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->runspeed, g_i, g_p);
        return;

    case PLAYER_SBS:
        if (iSet)
        {
            g_player[iPlayer].ps->sbs=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->sbs, g_i, g_p);
        return;

    case PLAYER_RELOADING:
        if (iSet)
        {
            g_player[iPlayer].ps->reloading=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->reloading, g_i, g_p);
        return;

    case PLAYER_AUTO_AIM:
        if (iSet)
        {
            g_player[iPlayer].ps->auto_aim=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->auto_aim, g_i, g_p);
        return;

    case PLAYER_MOVEMENT_LOCK:
        if (iSet)
        {
            g_player[iPlayer].ps->movement_lock=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->movement_lock, g_i, g_p);
        return;

    case PLAYER_SOUND_PITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->sound_pitch=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->sound_pitch, g_i, g_p);
        return;

    case PLAYER_WEAPONSWITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->weaponswitch=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->weaponswitch, g_i, g_p);
        return;

    case PLAYER_TEAM:
        if (iSet)
        {
            g_player[iPlayer].ps->team=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->team, g_i, g_p);
        return;

    case PLAYER_MAX_PLAYER_HEALTH:
        if (iSet)
        {
            g_player[iPlayer].ps->max_player_health = lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_player_health, g_i, g_p);
        return;

    case PLAYER_MAX_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_shield_amount = lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_shield_amount, g_i, g_p);
        return;

    case PLAYER_MAX_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_ammo_amount[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->max_ammo_amount[lParm2], g_i, g_p);
        return;

    case PLAYER_LAST_QUICK_KICK:
        if (iSet)
        {
            g_player[iPlayer].ps->last_quick_kick=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].ps->last_quick_kick, g_i, g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessPlayerInput(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVar(lVar1, g_i, g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && g_scriptSanityChecks)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "invalid target player (%d) %d\n",g_errorLineNum,keyw[g_tw],iPlayer,g_i);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case INPUT_AVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->avel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->avel, g_i, g_p);
        return;

    case INPUT_HORZ:
        if (iSet)
        {
            g_player[iPlayer].sync->horz=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->horz, g_i, g_p);
        return;

    case INPUT_FVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->fvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->fvel, g_i, g_p);
        return;

    case INPUT_SVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->svel=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->svel, g_i, g_p);
        return;

    case INPUT_BITS:
        if (iSet)
        {
            g_player[iPlayer].sync->bits=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->bits, g_i, g_p);
        return;

    case INPUT_EXTBITS:
        if (iSet)
        {
            g_player[iPlayer].sync->extbits=lValue;
            return;
        }
        Gv_SetVar(lVar2, g_player[iPlayer].sync->extbits, g_i, g_p);
        return;
    default:
        return;
    }
}

static inline void X_AccessWall(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iWall = Gv_GetVar(lVar1, g_i, g_p);

    if ((iWall<0 || iWall >= numwalls) && g_scriptSanityChecks)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "Invalid wall %d\n",g_errorLineNum,keyw[g_tw],iWall);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case WALL_X:
        if (iSet)
        {
            wall[iWall].x=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].x, g_i, g_p);
        return;

    case WALL_Y:
        if (iSet)
        {
            wall[iWall].y=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].y, g_i, g_p);
        return;

    case WALL_POINT2:
        if (iSet)
        {
            wall[iWall].point2=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].point2, g_i, g_p);
        return;

    case WALL_NEXTWALL:
        if (iSet)
        {
            wall[iWall].nextwall=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].nextwall, g_i, g_p);
        return;

    case WALL_NEXTSECTOR:
        if (iSet)
        {
            wall[iWall].nextsector=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].nextsector, g_i, g_p);
        return;

    case WALL_CSTAT:
        if (iSet)
        {
            wall[iWall].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].cstat, g_i, g_p);
        return;

    case WALL_PICNUM:
        if (iSet)
        {
            wall[iWall].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].picnum, g_i, g_p);
        return;

    case WALL_OVERPICNUM:
        if (iSet)
        {
            wall[iWall].overpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].overpicnum, g_i, g_p);
        return;

    case WALL_SHADE:
        if (iSet)
        {
            wall[iWall].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].shade, g_i, g_p);
        return;

    case WALL_PAL:
        if (iSet)
        {
            wall[iWall].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].pal, g_i, g_p);
        return;

    case WALL_XREPEAT:
        if (iSet)
        {
            wall[iWall].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].xrepeat, g_i, g_p);
        return;

    case WALL_YREPEAT:
        if (iSet)
        {
            wall[iWall].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].yrepeat, g_i, g_p);
        return;

    case WALL_XPANNING:
        if (iSet)
        {
            wall[iWall].xpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].xpanning, g_i, g_p);
        return;

    case WALL_YPANNING:
        if (iSet)
        {
            wall[iWall].ypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].ypanning, g_i, g_p);
        return;

    case WALL_LOTAG:
        if (iSet)
        {
            wall[iWall].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].lotag, g_i, g_p);
        return;

    case WALL_HITAG:
        if (iSet)
        {
            wall[iWall].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].hitag, g_i, g_p);
        return;

    case WALL_EXTRA:
        if (iSet)
        {
            wall[iWall].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, wall[iWall].extra, g_i, g_p);
        return;
    default:
        return;
    }
}

static inline void X_AccessSector(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iSector=sprite[g_i].sectnum;

    if (lVar1 != g_iThisActorID)
        iSector=Gv_GetVar(lVar1, g_i, g_p);

    if ((iSector<0 || iSector >= numsectors) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "Invalid sector %d\n",g_errorLineNum,keyw[g_tw],iSector);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case SECTOR_WALLPTR:
        if (iSet)
        {
            sector[iSector].wallptr=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].wallptr,g_i,g_p);
        return;

    case SECTOR_WALLNUM:
        if (iSet)
        {
            sector[iSector].wallnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].wallnum,g_i,g_p);
        return;

    case SECTOR_CEILINGZ:
        if (iSet)
        {
            sector[iSector].ceilingz=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingz,g_i,g_p);
        return;

    case SECTOR_FLOORZ:
        if (iSet)
        {
            sector[iSector].floorz=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorz,g_i,g_p);
        return;

    case SECTOR_CEILINGSTAT:
        if (iSet)
        {
            sector[iSector].ceilingstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingstat,g_i,g_p);
        return;

    case SECTOR_FLOORSTAT:
        if (iSet)
        {
            sector[iSector].floorstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorstat,g_i,g_p);
        return;

    case SECTOR_CEILINGPICNUM:
        if (iSet)
        {
            sector[iSector].ceilingpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingpicnum,g_i,g_p);
        return;

    case SECTOR_CEILINGSLOPE:
        if (iSet)
        {
            sector[iSector].ceilingheinum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingheinum,g_i,g_p);
        return;

    case SECTOR_CEILINGSHADE:
        if (iSet)
        {
            sector[iSector].ceilingshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingshade,g_i,g_p);
        return;

    case SECTOR_CEILINGPAL:
        if (iSet)
        {
            sector[iSector].ceilingpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingpal,g_i,g_p);
        return;

    case SECTOR_CEILINGXPANNING:
        if (iSet)
        {
            sector[iSector].ceilingxpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingxpanning,g_i,g_p);
        return;

    case SECTOR_CEILINGYPANNING:
        if (iSet)
        {
            sector[iSector].ceilingypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].ceilingypanning,g_i,g_p);
        return;

    case SECTOR_FLOORPICNUM:
        if (iSet)
        {
            sector[iSector].floorpicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorpicnum,g_i,g_p);
        return;

    case SECTOR_FLOORSLOPE:
        if (iSet)
        {
            sector[iSector].floorheinum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorheinum,g_i,g_p);
        return;

    case SECTOR_FLOORSHADE:
        if (iSet)
        {
            sector[iSector].floorshade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorshade,g_i,g_p);
        return;

    case SECTOR_FLOORPAL:
        if (iSet)
        {
            sector[iSector].floorpal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorpal,g_i,g_p);
        return;

    case SECTOR_FLOORXPANNING:
        if (iSet)
        {
            sector[iSector].floorxpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorxpanning,g_i,g_p);
        return;

    case SECTOR_FLOORYPANNING:
        if (iSet)
        {
            sector[iSector].floorypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].floorypanning,g_i,g_p);
        return;

    case SECTOR_VISIBILITY:
        if (iSet)
        {
            sector[iSector].visibility=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].visibility,g_i,g_p);
        return;

    case SECTOR_ALIGNTO:
        if (iSet)
        {
            sector[iSector].filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].filler,g_i,g_p);
        return;

    case SECTOR_LOTAG:
        if (iSet)
        {
            sector[iSector].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].lotag,g_i,g_p);
        return;

    case SECTOR_HITAG:
        if (iSet)
        {
            sector[iSector].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].hitag,g_i,g_p);
        return;

    case SECTOR_EXTRA:
        if (iSet)
        {
            sector[iSector].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, sector[iSector].extra,g_i,g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessSprite(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    int lValue=0;
    int iActor=g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVar(lVar1, g_i, g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target sprite (%d) from spr %d pic %d gv %s\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ActorLabels[lLabelID].name,iActor,g_i,g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= ActorLabels[lLabelID].maxParm2)) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on sprite (%d) from spr %d\n",g_errorLineNum,keyw[g_tw],
                   iSet?"set":"get",ActorLabels[lLabelID].name,lParm2,iActor,g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
        {
            sprite[iActor].x=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].x,g_i,g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            sprite[iActor].y=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].y,g_i,g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            sprite[iActor].z=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].z,g_i,g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            sprite[iActor].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].cstat,g_i,g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            sprite[iActor].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].picnum,g_i,g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            sprite[iActor].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].shade,g_i,g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            sprite[iActor].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].pal,g_i,g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            sprite[iActor].clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].clipdist,g_i,g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            sprite[iActor].filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].filler,g_i,g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            sprite[iActor].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xrepeat,g_i,g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            sprite[iActor].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yrepeat,g_i,g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            sprite[iActor].xoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xoffset,g_i,g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            sprite[iActor].yoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yoffset,g_i,g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            changespritesect(iActor,lValue);
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].sectnum,g_i,g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            changespritestat(iActor,lValue);
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].statnum,g_i,g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            sprite[iActor].ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].ang,g_i,g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            sprite[iActor].owner=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].owner,g_i,g_p);
        return;

    case ACTOR_XVEL:
        if (iSet)
        {
            sprite[iActor].xvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].xvel,g_i,g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            sprite[iActor].yvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].yvel,g_i,g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            sprite[iActor].zvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].zvel,g_i,g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            sprite[iActor].lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].lotag,g_i,g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            sprite[iActor].hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].hitag,g_i,g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            sprite[iActor].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, sprite[iActor].extra,g_i,g_p);
        return;

    case ACTOR_HTCGG:
        if (iSet)
        {
            ActorExtra[iActor].cgg=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].cgg, g_i, g_p);
        return;

    case ACTOR_HTPICNUM :
        if (iSet)
        {
            ActorExtra[iActor].picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].picnum, g_i, g_p);
        return;

    case ACTOR_HTANG:
        if (iSet)
        {
            ActorExtra[iActor].ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].ang, g_i, g_p);
        return;

    case ACTOR_HTEXTRA:
        if (iSet)
        {
            ActorExtra[iActor].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].extra, g_i, g_p);
        return;

    case ACTOR_HTOWNER:
        if (iSet)
        {
            ActorExtra[iActor].owner=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].owner, g_i, g_p);
        return;

    case ACTOR_HTMOVFLAG:
        if (iSet)
        {
            ActorExtra[iActor].movflag=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].movflag, g_i, g_p);
        return;

    case ACTOR_HTTEMPANG:
        if (iSet)
        {
            ActorExtra[iActor].tempang=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].tempang, g_i, g_p);
        return;

    case ACTOR_HTACTORSTAYPUT:
        if (iSet)
        {
            ActorExtra[iActor].actorstayput=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].actorstayput, g_i, g_p);
        return;

    case ACTOR_HTDISPICNUM:
        if (iSet)
        {
            ActorExtra[iActor].dispicnum=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].dispicnum, g_i, g_p);
        return;

    case ACTOR_HTTIMETOSLEEP:
        if (iSet)
        {
            ActorExtra[iActor].timetosleep=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].timetosleep, g_i, g_p);
        return;

    case ACTOR_HTFLOORZ:
        if (iSet)
        {
            ActorExtra[iActor].floorz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].floorz, g_i, g_p);
        return;

    case ACTOR_HTCEILINGZ:
        if (iSet)
        {
            ActorExtra[iActor].ceilingz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].ceilingz, g_i, g_p);
        return;

    case ACTOR_HTLASTVX:
        if (iSet)
        {
            ActorExtra[iActor].lastvx=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].lastvx, g_i, g_p);
        return;

    case ACTOR_HTLASTVY:
        if (iSet)
        {
            ActorExtra[iActor].lastvy=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].lastvy, g_i, g_p);
        return;

    case ACTOR_HTBPOSX:
        if (iSet)
        {
            ActorExtra[iActor].bposx=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposx, g_i, g_p);
        return;

    case ACTOR_HTBPOSY:
        if (iSet)
        {
            ActorExtra[iActor].bposy=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposy, g_i, g_p);
        return;

    case ACTOR_HTBPOSZ:
        if (iSet)
        {
            ActorExtra[iActor].bposz=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].bposz, g_i, g_p);
        return;

    case ACTOR_HTG_T:
        if (iSet)
        {
            ActorExtra[iActor].temp_data[lParm2]=lValue;
            return;
        }
        Gv_SetVar(lVar2, ActorExtra[iActor].temp_data[lParm2], g_i, g_p);
        return;

    case ACTOR_ANGOFF:
        if (iSet)
        {
            spriteext[iActor].angoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].angoff, g_i, g_p);
        return;

    case ACTOR_PITCH:
        if (iSet)
        {
            spriteext[iActor].pitch=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].pitch, g_i, g_p);
        return;

    case ACTOR_ROLL:
        if (iSet)
        {
            spriteext[iActor].roll=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].roll, g_i, g_p);
        return;

    case ACTOR_MDXOFF:
        if (iSet)
        {
            spriteext[iActor].xoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].xoff, g_i, g_p);
        return;

    case ACTOR_MDYOFF:
        if (iSet)
        {
            spriteext[iActor].yoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].yoff, g_i, g_p);
        return;

    case ACTOR_MDZOFF:
        if (iSet)
        {
            spriteext[iActor].zoff=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].zoff, g_i, g_p);
        return;

    case ACTOR_MDFLAGS:
        if (iSet)
        {
            spriteext[iActor].flags=lValue;
            return;
        }
        Gv_SetVar(lVar2,spriteext[iActor].flags, g_i, g_p);
        return;

    case ACTOR_XPANNING:
        if (iSet)
        {
            spriteext[iActor].xpanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].xpanning,g_i,g_p);
        return;

    case ACTOR_YPANNING:
        if (iSet)
        {
            spriteext[iActor].ypanning=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].ypanning,g_i,g_p);
        return;

    case ACTOR_HTFLAGS:
        if (iSet)
        {
            ActorExtra[iActor].flags=lValue;
            return;
        }
        Gv_SetVar(lVar2,ActorExtra[iActor].flags, g_i, g_p);
        return;

    default:
        return;
    }
}

static inline void X_AccessTsprite(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iActor=g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVar(lVar1, g_i, g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "invalid target sprite (%d) %d %d\n",g_errorLineNum,keyw[g_tw],iActor,g_i,g_sp->picnum);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

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
        Gv_SetVar(lVar2, spriteext[iActor].tspr->x,g_i,g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            spriteext[iActor].tspr->y=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->y,g_i,g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            spriteext[iActor].tspr->z=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->z,g_i,g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            spriteext[iActor].tspr->cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->cstat,g_i,g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->picnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->picnum,g_i,g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            spriteext[iActor].tspr->shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->shade,g_i,g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            spriteext[iActor].tspr->pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->pal,g_i,g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            spriteext[iActor].tspr->clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->clipdist,g_i,g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            spriteext[iActor].tspr->filler=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->filler,g_i,g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xrepeat,g_i,g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yrepeat,g_i,g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->xoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xoffset,g_i,g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->yoffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yoffset,g_i,g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->sectnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->sectnum,g_i,g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->statnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->statnum,g_i,g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            spriteext[iActor].tspr->ang=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->ang,g_i,g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            spriteext[iActor].tspr->owner=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->owner,g_i,g_p);
        return;

#if 1
    case ACTOR_XVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->xvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->xvel,g_i,g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->yvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->yvel,g_i,g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->zvel=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->zvel,g_i,g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            spriteext[iActor].tspr->lotag=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->lotag,g_i,g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            spriteext[iActor].tspr->hitag=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->hitag,g_i,g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            spriteext[iActor].tspr->extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, spriteext[iActor].tspr->extra,g_i,g_p);
        return;
#endif

    default:
        return;
    }
}

static inline void X_AccessProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;

    if ((lVar1 < 0 || lVar1 >= MAXTILES) && g_scriptSanityChecks)
    {
        OSD_Printf(CON_ERROR "invalid tile (%d)\n",g_errorLineNum,keyw[g_tw],lVar1);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVar(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            ProjectileData[lVar1].workslike=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].workslike, g_i, g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            ProjectileData[lVar1].spawns=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].spawns, g_i, g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].sxrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].sxrepeat, g_i, g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].syrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].syrepeat, g_i, g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            ProjectileData[lVar1].sound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].sound, g_i, g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            ProjectileData[lVar1].isound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].isound, g_i, g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            ProjectileData[lVar1].vel=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].vel, g_i, g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            ProjectileData[lVar1].extra=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].extra, g_i, g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            ProjectileData[lVar1].decal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].decal, g_i, g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            ProjectileData[lVar1].trail=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].trail, g_i, g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].txrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].txrepeat, g_i, g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].tyrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].tyrepeat, g_i, g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].toffset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].toffset, g_i, g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            ProjectileData[lVar1].tnum=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].tnum, g_i, g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            ProjectileData[lVar1].drop=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].drop, g_i, g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            ProjectileData[lVar1].cstat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].cstat, g_i, g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            ProjectileData[lVar1].clipdist=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].clipdist, g_i, g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            ProjectileData[lVar1].shade=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].shade, g_i, g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].xrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].xrepeat, g_i, g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].yrepeat=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].yrepeat, g_i, g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            ProjectileData[lVar1].pal=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].pal, g_i, g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            ProjectileData[lVar1].extra_rand=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].extra_rand, g_i, g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            ProjectileData[lVar1].hitradius=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].hitradius, g_i, g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            ProjectileData[lVar1].velmult=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].velmult, g_i, g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].offset=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].offset, g_i, g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            ProjectileData[lVar1].bounces=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].bounces, g_i, g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            ProjectileData[lVar1].bsound=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].bsound, g_i, g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            ProjectileData[lVar1].range=lValue;
            return;
        }
        Gv_SetVar(lVar2, ProjectileData[lVar1].range, g_i, g_p);
        return;

    default:
        return;
    }
}
