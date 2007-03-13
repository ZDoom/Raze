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

#include <time.h>

#include "duke3d.h"
#include "gamedef.h"
#include "scriplib.h"

#include "osd.h"

int g_i,g_p;
static long g_x,*g_t;
static spritetype *g_sp;
static int killit_flag;

extern int32 scripthandle;

static void DoUserDef(int iSet, int lLabelID, int lVar2)
{
    long lValue;

    if (g_p != myconnectindex) return;

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case USERDEFS_GOD:
        if (iSet)
            ud.god = lValue;
        else
            SetGameVarID(lVar2, ud.god, g_i, g_p);
        break;

    case USERDEFS_WARP_ON:
        if (iSet)
            ud.warp_on = lValue;
        else
            SetGameVarID(lVar2, ud.warp_on, g_i, g_p);
        break;

    case USERDEFS_CASHMAN:
        if (iSet)
            ud.cashman = lValue;
        else
            SetGameVarID(lVar2, ud.cashman, g_i, g_p);
        break;

    case USERDEFS_EOG:
        if (iSet)
            ud.eog = lValue;
        else
            SetGameVarID(lVar2, ud.eog, g_i, g_p);
        break;

    case USERDEFS_SHOWALLMAP:
        if (iSet)
            ud.showallmap = lValue;
        else
            SetGameVarID(lVar2, ud.showallmap, g_i, g_p);
        break;

    case USERDEFS_SHOW_HELP:
        if (iSet)
            ud.show_help = lValue;
        else
            SetGameVarID(lVar2, ud.show_help, g_i, g_p);
        break;

    case USERDEFS_SCROLLMODE:
        if (iSet)
            ud.scrollmode = lValue;
        else
            SetGameVarID(lVar2, ud.scrollmode, g_i, g_p);
        break;

    case USERDEFS_CLIPPING:
        if (iSet)
            ud.clipping = lValue;
        else
            SetGameVarID(lVar2, ud.clipping, g_i, g_p);
        break;

        //  case USERDEFS_USER_NAME:
        //      if(iSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.user_name[MAXPLAYERS][32], g_i, g_p);
        //      }
        //      break;

        //  case USERDEFS_RIDECULE:
        //      if(iSet)
        //      {
        //          ud.ridecule = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.ridecule, g_i, g_p);
        //      }
        //      break;

        //  case USERDEFS_SAVEGAME:
        //      if(iSet)
        //      {
        //          ud.savegame = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.savegame, g_i, g_p);
        //      }
        //      break;

        //  case USERDEFS_PWLOCKOUT:
        //      if(iSet)
        //      {
        //          ud.pwlockout = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.pwlockout, g_i, g_p);
        //      }
        //      break;

        //  case USERDEFS_RTSNAME:
        //      if(iSet)
        //      {
        //          ud.rtsname = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.rtsname, g_i, g_p);
        //      }
        //      break;

    case USERDEFS_OVERHEAD_ON:
        if (iSet)
            ud.overhead_on = lValue;
        else
            SetGameVarID(lVar2, ud.overhead_on, g_i, g_p);
        break;

    case USERDEFS_LAST_OVERHEAD:
        if (iSet)
            ud.last_overhead = lValue;
        else
            SetGameVarID(lVar2, ud.last_overhead, g_i, g_p);
        break;

    case USERDEFS_SHOWWEAPONS:
        if (iSet)
            ud.showweapons = lValue;
        else
            SetGameVarID(lVar2, ud.showweapons, g_i, g_p);
        break;

    case USERDEFS_PAUSE_ON:
        if (iSet)
            ud.pause_on = lValue;
        else
            SetGameVarID(lVar2, ud.pause_on, g_i, g_p);
        break;

    case USERDEFS_FROM_BONUS:
        if (iSet)
            ud.from_bonus = lValue;
        else
            SetGameVarID(lVar2, ud.from_bonus, g_i, g_p);
        break;

    case USERDEFS_CAMERASPRITE:
        if (iSet)
            ud.camerasprite = lValue;
        else
            SetGameVarID(lVar2, ud.camerasprite, g_i, g_p);
        break;

    case USERDEFS_LAST_CAMSPRITE:
        if (iSet)
            ud.last_camsprite = lValue;
        else
            SetGameVarID(lVar2, ud.last_camsprite, g_i, g_p);
        break;

    case USERDEFS_LAST_LEVEL:
        if (iSet)
            ud.last_level = lValue;
        else
            SetGameVarID(lVar2, ud.last_level, g_i, g_p);
        break;

    case USERDEFS_SECRETLEVEL:
        if (iSet)
            ud.secretlevel = lValue;
        else
            SetGameVarID(lVar2, ud.secretlevel, g_i, g_p);
        break;

    case USERDEFS_CONST_VISIBILITY:
        if (iSet)
            ud.const_visibility = lValue;
        else
            SetGameVarID(lVar2, ud.const_visibility, g_i, g_p);
        break;

    case USERDEFS_UW_FRAMERATE:
        if (iSet)
            ud.uw_framerate = lValue;
        else
            SetGameVarID(lVar2, ud.uw_framerate, g_i, g_p);
        break;

    case USERDEFS_CAMERA_TIME:
        if (iSet)
            ud.camera_time = lValue;
        else
            SetGameVarID(lVar2, ud.camera_time, g_i, g_p);
        break;

    case USERDEFS_FOLFVEL:
        if (iSet)
            ud.folfvel = lValue;
        else
            SetGameVarID(lVar2, ud.folfvel, g_i, g_p);
        break;

    case USERDEFS_FOLAVEL:
        if (iSet)
            ud.folavel = lValue;
        else
            SetGameVarID(lVar2, ud.folavel, g_i, g_p);
        break;

    case USERDEFS_FOLX:
        if (iSet)
            ud.folx = lValue;
        else
            SetGameVarID(lVar2, ud.folx, g_i, g_p);
        break;

    case USERDEFS_FOLY:
        if (iSet)
            ud.foly = lValue;
        else
            SetGameVarID(lVar2, ud.foly, g_i, g_p);
        break;

    case USERDEFS_FOLA:
        if (iSet)
            ud.fola = lValue;
        else
            SetGameVarID(lVar2, ud.fola, g_i, g_p);
        break;

    case USERDEFS_RECCNT:
        if (iSet)
            ud.reccnt = lValue;
        else
            SetGameVarID(lVar2, ud.reccnt, g_i, g_p);
        break;

    case USERDEFS_ENTERED_NAME:
        if (iSet)
            ud.entered_name = lValue;
        else
            SetGameVarID(lVar2, ud.entered_name, g_i, g_p);
        break;

    case USERDEFS_SCREEN_TILTING:
        if (iSet)
            ud.screen_tilting = lValue;
        else
            SetGameVarID(lVar2, ud.screen_tilting, g_i, g_p);
        break;

    case USERDEFS_SHADOWS:
        if (iSet)
            ud.shadows = lValue;
        else
            SetGameVarID(lVar2, ud.shadows, g_i, g_p);
        break;

    case USERDEFS_FTA_ON:
        if (iSet)
            ud.fta_on = lValue;
        else
            SetGameVarID(lVar2, ud.fta_on, g_i, g_p);
        break;

    case USERDEFS_EXECUTIONS:
        if (iSet)
            ud.executions = lValue;
        else
            SetGameVarID(lVar2, ud.executions, g_i, g_p);
        break;

    case USERDEFS_AUTO_RUN:
        if (iSet)
            ud.auto_run = lValue;
        else
            SetGameVarID(lVar2, ud.auto_run, g_i, g_p);
        break;

    case USERDEFS_COORDS:
        if (iSet)
            ud.coords = lValue;
        else
            SetGameVarID(lVar2, ud.coords, g_i, g_p);
        break;

    case USERDEFS_TICKRATE:
        if (iSet)
            ud.tickrate = lValue;
        else
            SetGameVarID(lVar2, ud.tickrate, g_i, g_p);
        break;

    case USERDEFS_M_COOP:
        if (iSet)
            ud.m_coop = lValue;
        else
            SetGameVarID(lVar2, ud.m_coop, g_i, g_p);
        break;

    case USERDEFS_COOP:
        if (iSet)
            ud.coop = lValue;
        else
            SetGameVarID(lVar2, ud.coop, g_i, g_p);
        break;

    case USERDEFS_SCREEN_SIZE:
        if (iSet)
        {
            if (ud.screen_size != lValue)
            {
                ud.screen_size = lValue;
                vscrn();
            }
        }
        else
            SetGameVarID(lVar2, ud.screen_size, g_i, g_p);
        break;

    case USERDEFS_LOCKOUT:
        if (iSet)
            ud.lockout = lValue;
        else
            SetGameVarID(lVar2, ud.lockout, g_i, g_p);
        break;

    case USERDEFS_CROSSHAIR:
        if (iSet)
            ud.crosshair = lValue;
        else
            SetGameVarID(lVar2, ud.crosshair, g_i, g_p);
        break;

        //  case USERDEFS_WCHOICE:
        //      if(iSet)
        //      {
        //          ud.wchoice = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID(lVar2, ud.wchoice, g_i, g_p);
        //      }
        //      break;

    case USERDEFS_PLAYERAI:
        if (iSet)
            ud.playerai = lValue;
        else
            SetGameVarID(lVar2, ud.playerai, g_i, g_p);
        break;

    case USERDEFS_RESPAWN_MONSTERS:
        if (iSet)
            ud.respawn_monsters = lValue;
        else
            SetGameVarID(lVar2, ud.respawn_monsters, g_i, g_p);
        break;

    case USERDEFS_RESPAWN_ITEMS:
        if (iSet)
            ud.respawn_items = lValue;
        else
            SetGameVarID(lVar2, ud.respawn_items, g_i, g_p);
        break;

    case USERDEFS_RESPAWN_INVENTORY:
        if (iSet)
            ud.respawn_inventory = lValue;
        else
            SetGameVarID(lVar2, ud.respawn_inventory, g_i, g_p);
        break;

    case USERDEFS_RECSTAT:
        if (iSet)
            ud.recstat = lValue;
        else
            SetGameVarID(lVar2, ud.recstat, g_i, g_p);
        break;

    case USERDEFS_MONSTERS_OFF:
        if (iSet)
            ud.monsters_off = lValue;
        else
            SetGameVarID(lVar2, ud.monsters_off, g_i, g_p);
        break;

    case USERDEFS_BRIGHTNESS:
        if (iSet)
            ud.brightness = lValue;
        else
            SetGameVarID(lVar2, ud.brightness, g_i, g_p);
        break;

    case USERDEFS_M_RESPAWN_ITEMS:
        if (iSet)
            ud.m_respawn_items = lValue;
        else
            SetGameVarID(lVar2, ud.m_respawn_items, g_i, g_p);
        break;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if (iSet)
            ud.m_respawn_monsters = lValue;
        else
            SetGameVarID(lVar2, ud.m_respawn_monsters, g_i, g_p);
        break;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if (iSet)
            ud.m_respawn_inventory = lValue;
        else
            SetGameVarID(lVar2, ud.m_respawn_inventory, g_i, g_p);
        break;

    case USERDEFS_M_RECSTAT:
        if (iSet)
            ud.m_recstat = lValue;
        else
            SetGameVarID(lVar2, ud.m_recstat, g_i, g_p);
        break;

    case USERDEFS_M_MONSTERS_OFF:
        if (iSet)
            ud.m_monsters_off = lValue;
        else
            SetGameVarID(lVar2, ud.m_monsters_off, g_i, g_p);
        break;

    case USERDEFS_DETAIL:
        if (iSet)
            ud.detail = lValue;
        else
            SetGameVarID(lVar2, ud.detail, g_i, g_p);
        break;

    case USERDEFS_M_FFIRE:
        if (iSet)
            ud.m_ffire = lValue;
        else
            SetGameVarID(lVar2, ud.m_ffire, g_i, g_p);
        break;

    case USERDEFS_FFIRE:
        if (iSet)
            ud.ffire = lValue;
        else
            SetGameVarID(lVar2, ud.ffire, g_i, g_p);
        break;

    case USERDEFS_M_PLAYER_SKILL:
        if (iSet)
            ud.m_player_skill = lValue;
        else
            SetGameVarID(lVar2, ud.m_player_skill, g_i, g_p);
        break;

    case USERDEFS_M_LEVEL_NUMBER:
        if (iSet)
            ud.m_level_number = lValue;
        else
            SetGameVarID(lVar2, ud.m_level_number, g_i, g_p);
        break;

    case USERDEFS_M_VOLUME_NUMBER:
        if (iSet)
            ud.m_volume_number = lValue;
        else
            SetGameVarID(lVar2, ud.m_volume_number, g_i, g_p);
        break;

    case USERDEFS_MULTIMODE:
        if (iSet)
            ud.multimode = lValue;
        else
            SetGameVarID(lVar2, ud.multimode, g_i, g_p);
        break;

    case USERDEFS_PLAYER_SKILL:
        if (iSet)
            ud.player_skill = lValue;
        else
            SetGameVarID(lVar2, ud.player_skill, g_i, g_p);
        break;

    case USERDEFS_LEVEL_NUMBER:
        if (iSet)
            ud.level_number = lValue;
        else
            SetGameVarID(lVar2, ud.level_number, g_i, g_p);
        break;

    case USERDEFS_VOLUME_NUMBER:
        if (iSet)
            ud.volume_number = lValue;
        else
            SetGameVarID(lVar2, ud.volume_number, g_i, g_p);
        break;

    case USERDEFS_M_MARKER:
        if (iSet)
            ud.m_marker = lValue;
        else
            SetGameVarID(lVar2, ud.m_marker, g_i, g_p);
        break;

    case USERDEFS_MARKER:
        if (iSet)
            ud.marker = lValue;
        else
            SetGameVarID(lVar2, ud.marker, g_i, g_p);
        break;

    case USERDEFS_MOUSEFLIP:
        if (iSet)
            ud.mouseflip = lValue;
        else
            SetGameVarID(lVar2, ud.mouseflip, g_i, g_p);
        break;

    case USERDEFS_STATUSBARSCALE:
        if (iSet)
            ud.statusbarscale = lValue;
        else
            SetGameVarID(lVar2, ud.statusbarscale, g_i, g_p);
        break;

    case USERDEFS_DRAWWEAPON:
        if (iSet)
            ud.drawweapon = lValue;
        else
            SetGameVarID(lVar2, ud.drawweapon, g_i, g_p);
        break;

    case USERDEFS_MOUSEAIMING:
        if (iSet)
            ud.mouseaiming = lValue;
        else
            SetGameVarID(lVar2, ud.mouseaiming, g_i, g_p);
        break;

    case USERDEFS_WEAPONSWITCH:
        if (iSet)
            ud.weaponswitch = lValue;
        else
            SetGameVarID(lVar2, ud.weaponswitch, g_i, g_p);
        break;

    case USERDEFS_DEMOCAMS:
        if (iSet)
            ud.democams = lValue;
        else
            SetGameVarID(lVar2, ud.democams, g_i, g_p);
        break;

    case USERDEFS_COLOR:
        if (iSet)
            ud.color = lValue;
        else
            SetGameVarID(lVar2, ud.color, g_i, g_p);
        break;

    case USERDEFS_MSGDISPTIME:
        if (iSet)
            ud.msgdisptime = lValue;
        else
            SetGameVarID(lVar2, ud.msgdisptime, g_i, g_p);
        break;

    case USERDEFS_STATUSBARMODE:
        if (iSet)
            ud.statusbarmode = lValue;
        else
            SetGameVarID(lVar2, ud.statusbarmode, g_i, g_p);
        break;

    case USERDEFS_M_NOEXITS:
        if (iSet)
            ud.m_noexits = lValue;
        else
            SetGameVarID(lVar2, ud.m_noexits, g_i, g_p);
        break;

    case USERDEFS_NOEXITS:
        if (iSet)
            ud.noexits = lValue;
        else
            SetGameVarID(lVar2, ud.noexits, g_i, g_p);
        break;

    case USERDEFS_AUTOVOTE:
        if (iSet)
            ud.autovote = lValue;
        else
            SetGameVarID(lVar2, ud.autovote, g_i, g_p);
        break;

    case USERDEFS_AUTOMSG:
        if (iSet)
            ud.automsg = lValue;
        else
            SetGameVarID(lVar2, ud.automsg, g_i, g_p);
        break;

    case USERDEFS_IDPLAYERS:
        if (iSet)
            ud.idplayers = lValue;
        else
            SetGameVarID(lVar2, ud.idplayers, g_i, g_p);
        break;

    case USERDEFS_TEAM:
        if (iSet)
            ud.team = lValue;
        else
            SetGameVarID(lVar2, ud.team, g_i, g_p);
        break;

    case USERDEFS_VIEWBOB:
        if (iSet)
            ud.viewbob = lValue;
        else
            SetGameVarID(lVar2, ud.viewbob, g_i, g_p);
        break;

    case USERDEFS_WEAPONSWAY:
        if (iSet)
            ud.weaponsway = lValue;
        else
            SetGameVarID(lVar2, ud.weaponsway, g_i, g_p);
        break;

    default:
        break;
    }
    return;
}

static void DoThisProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    long lValue,proj=g_i;

    if (lVar1 != g_iThisActorID)
        proj=GetGameVarID(lVar1, g_i, g_p);

    if (proj < 0 || proj >= MAXSPRITES)
    {
        OSD_Printf("DoThisProjectile(): invalid projectile (%d)\n",proj);
        return;
    }

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
            thisprojectile[proj].workslike=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].workslike, g_i, g_p);
        break;

    case PROJ_SPAWNS:
        if (iSet)
            thisprojectile[proj].spawns=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].spawns, g_i, g_p);
        break;

    case PROJ_SXREPEAT:
        if (iSet)
            thisprojectile[proj].sxrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].sxrepeat, g_i, g_p);
        break;

    case PROJ_SYREPEAT:
        if (iSet)
            thisprojectile[proj].syrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].syrepeat, g_i, g_p);
        break;

    case PROJ_SOUND:
        if (iSet)
            thisprojectile[proj].sound=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].sound, g_i, g_p);
        break;

    case PROJ_ISOUND:
        if (iSet)
            thisprojectile[proj].isound=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].isound, g_i, g_p);
        break;

    case PROJ_VEL:
        if (iSet)
            thisprojectile[proj].vel=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].vel, g_i, g_p);
        break;

    case PROJ_EXTRA:
        if (iSet)
            thisprojectile[proj].extra=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].extra, g_i, g_p);
        break;

    case PROJ_DECAL:
        if (iSet)
            thisprojectile[proj].decal=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].decal, g_i, g_p);
        break;

    case PROJ_TRAIL:
        if (iSet)
            thisprojectile[proj].trail=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].trail, g_i, g_p);
        break;

    case PROJ_TXREPEAT:
        if (iSet)
            thisprojectile[proj].txrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].txrepeat, g_i, g_p);
        break;

    case PROJ_TYREPEAT:
        if (iSet)
            thisprojectile[proj].tyrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].tyrepeat, g_i, g_p);
        break;

    case PROJ_TOFFSET:
        if (iSet)
            thisprojectile[proj].toffset=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].toffset, g_i, g_p);
        break;

    case PROJ_TNUM:
        if (iSet)
            thisprojectile[proj].tnum=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].tnum, g_i, g_p);
        break;

    case PROJ_DROP:
        if (iSet)
            thisprojectile[proj].drop=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].drop, g_i, g_p);
        break;

    case PROJ_CSTAT:
        if (iSet)
            thisprojectile[proj].cstat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].cstat, g_i, g_p);
        break;

    case PROJ_CLIPDIST:
        if (iSet)
            thisprojectile[proj].clipdist=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].clipdist, g_i, g_p);
        break;

    case PROJ_SHADE:
        if (iSet)
            thisprojectile[proj].shade=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].shade, g_i, g_p);
        break;

    case PROJ_XREPEAT:
        if (iSet)
            thisprojectile[proj].xrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].xrepeat, g_i, g_p);
        break;

    case PROJ_YREPEAT:
        if (iSet)
            thisprojectile[proj].yrepeat=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].yrepeat, g_i, g_p);
        break;

    case PROJ_PAL:
        if (iSet)
            thisprojectile[proj].pal=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].pal, g_i, g_p);
        break;

    case PROJ_EXTRA_RAND:
        if (iSet)
            thisprojectile[proj].extra_rand=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].extra_rand, g_i, g_p);
        break;

    case PROJ_HITRADIUS:
        if (iSet)
            thisprojectile[proj].hitradius=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].hitradius, g_i, g_p);
        break;

    case PROJ_VEL_MULT:
        if (iSet)
            thisprojectile[proj].velmult=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].velmult, g_i, g_p);
        break;

    case PROJ_OFFSET:
        if (iSet)
            thisprojectile[proj].offset=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].offset, g_i, g_p);
        break;

    case PROJ_BOUNCES:
        if (iSet)
            thisprojectile[proj].bounces=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].bounces, g_i, g_p);
        break;

    case PROJ_BSOUND:
        if (iSet)
            thisprojectile[proj].bsound=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].bsound, g_i, g_p);
        break;

    case PROJ_RANGE:
        if (iSet)
            thisprojectile[proj].range=lValue;
        else
            SetGameVarID(lVar2, thisprojectile[proj].range, g_i, g_p);
        break;

    default:
        break;
    }
    return;
}

static void DoPlayer(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    long lValue;
    long lTemp;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=GetGameVarID(lVar1, g_i, g_p);

    if (iPlayer<0 || iPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        if (iSet)
            ps[iPlayer].zoom=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].zoom, g_i, g_p);
        break;

    case PLAYER_EXITX:
        if (iSet)
            ps[iPlayer].exitx=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].exitx, g_i, g_p);
        break;

    case PLAYER_EXITY:
        if (iSet)
            ps[iPlayer].exity=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].exity, g_i, g_p);
        break;

    case PLAYER_LOOGIEX:
        lTemp=lParm2;
        if (iSet)
            ps[iPlayer].loogiex[lTemp]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].loogiex[lTemp], g_i, g_p);
        break;

    case PLAYER_LOOGIEY:
        lTemp=lParm2;
        if (iSet)
            ps[iPlayer].loogiey[lTemp]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].loogiey[lTemp], g_i, g_p);
        break;

    case PLAYER_NUMLOOGS:
        if (iSet)
            ps[iPlayer].numloogs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].numloogs, g_i, g_p);
        break;

    case PLAYER_LOOGCNT:
        if (iSet)
            ps[iPlayer].loogcnt=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].loogcnt, g_i, g_p);
        break;

    case PLAYER_POSX:
        if (iSet)
            ps[iPlayer].posx=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].posx, g_i, g_p);
        break;

    case PLAYER_POSY:
        if (iSet)
            ps[iPlayer].posy=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].posy, g_i, g_p);
        break;

    case PLAYER_POSZ:
        if (iSet)
            ps[iPlayer].posz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].posz, g_i, g_p);
        break;

    case PLAYER_HORIZ:
        if (iSet)
            ps[iPlayer].horiz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].horiz, g_i, g_p);
        break;

    case PLAYER_OHORIZ:
        if (iSet)
            ps[iPlayer].ohoriz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].ohoriz, g_i, g_p);
        break;

    case PLAYER_OHORIZOFF:
        if (iSet)
            ps[iPlayer].ohorizoff=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].ohorizoff, g_i, g_p);
        break;

    case PLAYER_INVDISPTIME:
        if (iSet)
            ps[iPlayer].invdisptime=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].invdisptime, g_i, g_p);
        break;

    case PLAYER_BOBPOSX:
        if (iSet)
            ps[iPlayer].bobposx=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].bobposx, g_i, g_p);
        break;

    case PLAYER_BOBPOSY:
        if (iSet)
            ps[iPlayer].bobposy=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].bobposy, g_i, g_p);
        break;

    case PLAYER_OPOSX:
        if (iSet)
            ps[iPlayer].oposx=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].oposx, g_i, g_p);
        break;

    case PLAYER_OPOSY:
        if (iSet)
            ps[iPlayer].oposy=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].oposy, g_i, g_p);
        break;

    case PLAYER_OPOSZ:
        if (iSet)
            ps[iPlayer].oposz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].oposz, g_i, g_p);
        break;

    case PLAYER_PYOFF:
        if (iSet)
            ps[iPlayer].pyoff=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].pyoff, g_i, g_p);
        break;

    case PLAYER_OPYOFF:
        if (iSet)
            ps[iPlayer].opyoff=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].opyoff, g_i, g_p);
        break;

    case PLAYER_POSXV:
        if (iSet)
            ps[iPlayer].posxv=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].posxv, g_i, g_p);
        break;

    case PLAYER_POSYV:
        if (iSet)
            ps[iPlayer].posyv=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].posyv, g_i, g_p);
        break;

    case PLAYER_POSZV:
        if (iSet)
            ps[iPlayer].poszv=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].poszv, g_i, g_p);
        break;

    case PLAYER_LAST_PISSED_TIME:
        if (iSet)
            ps[iPlayer].last_pissed_time=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].last_pissed_time, g_i, g_p);
        break;

    case PLAYER_TRUEFZ:
        if (iSet)
            ps[iPlayer].truefz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].truefz, g_i, g_p);
        break;

    case PLAYER_TRUECZ:
        if (iSet)
            ps[iPlayer].truecz=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].truecz, g_i, g_p);
        break;

    case PLAYER_PLAYER_PAR:
        if (iSet)
            ps[iPlayer].player_par=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].player_par, g_i, g_p);
        break;

    case PLAYER_VISIBILITY:
        if (iSet)
            ps[iPlayer].visibility=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].visibility, g_i, g_p);
        break;

    case PLAYER_BOBCOUNTER:
        if (iSet)
            ps[iPlayer].bobcounter=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].bobcounter, g_i, g_p);
        break;

    case PLAYER_WEAPON_SWAY:
        if (iSet)
            ps[iPlayer].weapon_sway=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weapon_sway, g_i, g_p);
        break;

    case PLAYER_PALS_TIME:
        if (iSet)
            ps[iPlayer].pals_time=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].pals_time, g_i, g_p);
        break;

    case PLAYER_RANDOMFLAMEX:
        if (iSet)
            ps[iPlayer].randomflamex=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].randomflamex, g_i, g_p);
        break;

    case PLAYER_CRACK_TIME:
        if (iSet)
            ps[iPlayer].crack_time=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].crack_time, g_i, g_p);
        break;

    case PLAYER_AIM_MODE:
        if (iSet)
            ps[iPlayer].aim_mode=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].aim_mode, g_i, g_p);
        break;

    case PLAYER_ANG:
        if (iSet)
            ps[iPlayer].ang=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].ang, g_i, g_p);
        break;

    case PLAYER_OANG:
        if (iSet)
            ps[iPlayer].oang=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].oang, g_i, g_p);
        break;

    case PLAYER_ANGVEL:
        if (iSet)
            ps[iPlayer].angvel=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].angvel, g_i, g_p);
        break;

    case PLAYER_CURSECTNUM:
        if (iSet)
            ps[iPlayer].cursectnum=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].cursectnum, g_i, g_p);
        break;

    case PLAYER_LOOK_ANG:
        if (iSet)
            ps[iPlayer].look_ang=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].look_ang, g_i, g_p);
        break;

    case PLAYER_LAST_EXTRA:
        if (iSet)
            ps[iPlayer].last_extra=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].last_extra, g_i, g_p);
        break;

    case PLAYER_SUBWEAPON:
        if (iSet)
            ps[iPlayer].subweapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].subweapon, g_i, g_p);
        break;

    case PLAYER_AMMO_AMOUNT:
        lTemp=lParm2;
        if (iSet)
            ps[iPlayer].ammo_amount[lTemp]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].ammo_amount[lTemp], g_i, g_p);
        break;

    case PLAYER_WACKEDBYACTOR:
        if (iSet)
            ps[iPlayer].wackedbyactor=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].wackedbyactor, g_i, g_p);
        break;

    case PLAYER_FRAG:
        if (iSet)
            ps[iPlayer].frag=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].frag, g_i, g_p);
        break;

    case PLAYER_FRAGGEDSELF:
        if (iSet)
            ps[iPlayer].fraggedself=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].fraggedself, g_i, g_p);
        break;

    case PLAYER_CURR_WEAPON:
        if (iSet)
            ps[iPlayer].curr_weapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].curr_weapon, g_i, g_p);
        break;

    case PLAYER_LAST_WEAPON:
        if (iSet)
            ps[iPlayer].last_weapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].last_weapon, g_i, g_p);
        break;

    case PLAYER_TIPINCS:
        if (iSet)
            ps[iPlayer].tipincs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].tipincs, g_i, g_p);
        break;

    case PLAYER_HORIZOFF:
        if (iSet)
            ps[iPlayer].horizoff=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].horizoff, g_i, g_p);
        break;

    case PLAYER_WANTWEAPONFIRE:
        if (iSet)
            ps[iPlayer].wantweaponfire=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].wantweaponfire, g_i, g_p);
        break;

    case PLAYER_HOLODUKE_AMOUNT:
        if (iSet)
            ps[iPlayer].holoduke_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].holoduke_amount, g_i, g_p);
        break;

    case PLAYER_NEWOWNER:
        if (iSet)
            ps[iPlayer].newowner=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].newowner, g_i, g_p);
        break;

    case PLAYER_HURT_DELAY:
        if (iSet)
            ps[iPlayer].hurt_delay=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].hurt_delay, g_i, g_p);
        break;

    case PLAYER_HBOMB_HOLD_DELAY:
        if (iSet)
            ps[iPlayer].hbomb_hold_delay=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].hbomb_hold_delay, g_i, g_p);
        break;

    case PLAYER_JUMPING_COUNTER:
        if (iSet)
            ps[iPlayer].jumping_counter=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].jumping_counter, g_i, g_p);
        break;

    case PLAYER_AIRLEFT:
        if (iSet)
            ps[iPlayer].airleft=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].airleft, g_i, g_p);
        break;

    case PLAYER_KNEE_INCS:
        if (iSet)
            ps[iPlayer].knee_incs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].knee_incs, g_i, g_p);
        break;

    case PLAYER_ACCESS_INCS:
        if (iSet)
            ps[iPlayer].access_incs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].access_incs, g_i, g_p);
        break;

    case PLAYER_FTA:
        if (iSet)
            ps[iPlayer].fta=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].fta, g_i, g_p);
        break;

    case PLAYER_FTQ:
        if (iSet)
            ps[iPlayer].ftq=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].ftq, g_i, g_p);
        break;

    case PLAYER_ACCESS_WALLNUM:
        if (iSet)
            ps[iPlayer].access_wallnum=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].access_wallnum, g_i, g_p);
        break;

    case PLAYER_ACCESS_SPRITENUM:
        if (iSet)
            ps[iPlayer].access_spritenum=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].access_spritenum, g_i, g_p);
        break;

    case PLAYER_KICKBACK_PIC:
        if (iSet)
            ps[iPlayer].kickback_pic=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].kickback_pic, g_i, g_p);
        break;

    case PLAYER_GOT_ACCESS:
        if (iSet)
            ps[iPlayer].got_access=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].got_access, g_i, g_p);
        break;

    case PLAYER_WEAPON_ANG:
        if (iSet)
            ps[iPlayer].weapon_ang=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weapon_ang, g_i, g_p);
        break;

    case PLAYER_FIRSTAID_AMOUNT:
        if (iSet)
            ps[iPlayer].firstaid_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].firstaid_amount, g_i, g_p);
        break;

    case PLAYER_SOMETHINGONPLAYER:
        if (iSet)
            ps[iPlayer].somethingonplayer=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].somethingonplayer, g_i, g_p);
        break;

    case PLAYER_ON_CRANE:
        if (iSet)
            ps[iPlayer].on_crane=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].on_crane, g_i, g_p);
        break;

    case PLAYER_I:
        if (iSet)
            ps[iPlayer].i=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].i, g_i, g_p);
        break;

    case PLAYER_ONE_PARALLAX_SECTNUM:
        if (iSet)
            ps[iPlayer].one_parallax_sectnum=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].one_parallax_sectnum, g_i, g_p);
        break;

    case PLAYER_OVER_SHOULDER_ON:
        if (iSet)
            ps[iPlayer].over_shoulder_on=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].over_shoulder_on, g_i, g_p);
        break;

    case PLAYER_RANDOM_CLUB_FRAME:
        if (iSet)
            ps[iPlayer].random_club_frame=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].random_club_frame, g_i, g_p);
        break;

    case PLAYER_FIST_INCS:
        if (iSet)
            ps[iPlayer].fist_incs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].fist_incs, g_i, g_p);
        break;

    case PLAYER_ONE_EIGHTY_COUNT:
        if (iSet)
            ps[iPlayer].one_eighty_count=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].one_eighty_count, g_i, g_p);
        break;

    case PLAYER_CHEAT_PHASE:
        if (iSet)
            ps[iPlayer].cheat_phase=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].cheat_phase, g_i, g_p);
        break;

    case PLAYER_DUMMYPLAYERSPRITE:
        if (iSet)
            ps[iPlayer].dummyplayersprite=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].dummyplayersprite, g_i, g_p);
        break;

    case PLAYER_EXTRA_EXTRA8:
        if (iSet)
            ps[iPlayer].extra_extra8=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].extra_extra8, g_i, g_p);
        break;

    case PLAYER_QUICK_KICK:
        if (iSet)
            ps[iPlayer].quick_kick=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].quick_kick, g_i, g_p);
        break;

    case PLAYER_HEAT_AMOUNT:
        if (iSet)
            ps[iPlayer].heat_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].heat_amount, g_i, g_p);
        break;

    case PLAYER_ACTORSQU:
        if (iSet)
            ps[iPlayer].actorsqu=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].actorsqu, g_i, g_p);
        break;

    case PLAYER_TIMEBEFOREEXIT:
        if (iSet)
            ps[iPlayer].timebeforeexit=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].timebeforeexit, g_i, g_p);
        break;

    case PLAYER_CUSTOMEXITSOUND:
        if (iSet)
            ps[iPlayer].customexitsound=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].customexitsound, g_i, g_p);
        break;

    case PLAYER_WEAPRECS:
        if (iSet)
            ps[iPlayer].weaprecs[16]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weaprecs[16], g_i, g_p);
        break;

    case PLAYER_WEAPRECCNT:
        if (iSet)
            ps[iPlayer].weapreccnt=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weapreccnt, g_i, g_p);
        break;

    case PLAYER_INTERFACE_TOGGLE_FLAG:
        if (iSet)
            ps[iPlayer].interface_toggle_flag=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].interface_toggle_flag, g_i, g_p);
        break;

    case PLAYER_ROTSCRNANG:
        if (iSet)
            ps[iPlayer].rotscrnang=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].rotscrnang, g_i, g_p);
        break;

    case PLAYER_DEAD_FLAG:
        if (iSet)
            ps[iPlayer].dead_flag=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].dead_flag, g_i, g_p);
        break;

    case PLAYER_SHOW_EMPTY_WEAPON:
        if (iSet)
            ps[iPlayer].show_empty_weapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].show_empty_weapon, g_i, g_p);
        break;

    case PLAYER_SCUBA_AMOUNT:
        if (iSet)
            ps[iPlayer].scuba_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].scuba_amount, g_i, g_p);
        break;

    case PLAYER_JETPACK_AMOUNT:
        if (iSet)
            ps[iPlayer].jetpack_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].jetpack_amount, g_i, g_p);
        break;

    case PLAYER_STEROIDS_AMOUNT:
        if (iSet)
            ps[iPlayer].steroids_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].steroids_amount, g_i, g_p);
        break;

    case PLAYER_SHIELD_AMOUNT:
        if (iSet)
            ps[iPlayer].shield_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].shield_amount, g_i, g_p);
        break;

    case PLAYER_HOLODUKE_ON:
        if (iSet)
            ps[iPlayer].holoduke_on=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].holoduke_on, g_i, g_p);
        break;

    case PLAYER_PYCOUNT:
        if (iSet)
            ps[iPlayer].pycount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].pycount, g_i, g_p);
        break;

    case PLAYER_WEAPON_POS:
        if (iSet)
            ps[iPlayer].weapon_pos=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weapon_pos, g_i, g_p);
        break;

    case PLAYER_FRAG_PS:
        if (iSet)
            ps[iPlayer].frag_ps=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].frag_ps, g_i, g_p);
        break;

    case PLAYER_TRANSPORTER_HOLD:
        if (iSet)
            ps[iPlayer].transporter_hold=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].transporter_hold, g_i, g_p);
        break;

    case PLAYER_LAST_FULL_WEAPON:
        if (iSet)
            ps[iPlayer].last_full_weapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].last_full_weapon, g_i, g_p);
        break;

    case PLAYER_FOOTPRINTSHADE:
        if (iSet)
            ps[iPlayer].footprintshade=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].footprintshade, g_i, g_p);
        break;

    case PLAYER_BOOT_AMOUNT:
        if (iSet)
            ps[iPlayer].boot_amount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].boot_amount, g_i, g_p);
        break;

    case PLAYER_SCREAM_VOICE:
        if (iSet)
            ps[iPlayer].scream_voice=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].scream_voice, g_i, g_p);
        break;

    case PLAYER_GM:
        if (iSet)
            ps[iPlayer].gm=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].gm, g_i, g_p);
        break;

    case PLAYER_ON_WARPING_SECTOR:
        if (iSet)
            ps[iPlayer].on_warping_sector=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].on_warping_sector, g_i, g_p);
        break;

    case PLAYER_FOOTPRINTCOUNT:
        if (iSet)
            ps[iPlayer].footprintcount=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].footprintcount, g_i, g_p);
        break;

    case PLAYER_HBOMB_ON:
        if (iSet)
            ps[iPlayer].hbomb_on=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].hbomb_on, g_i, g_p);
        break;

    case PLAYER_JUMPING_TOGGLE:
        if (iSet)
            ps[iPlayer].jumping_toggle=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].jumping_toggle, g_i, g_p);
        break;

    case PLAYER_RAPID_FIRE_HOLD:
        if (iSet)
            ps[iPlayer].rapid_fire_hold=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].rapid_fire_hold, g_i, g_p);
        break;

    case PLAYER_ON_GROUND:
        if (iSet)
            ps[iPlayer].on_ground=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].on_ground, g_i, g_p);
        break;

    case PLAYER_NAME:
        if (iSet)
            ps[iPlayer].name[32]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].name[32], g_i, g_p);
        break;

    case PLAYER_INVEN_ICON:
        if (iSet)
            ps[iPlayer].inven_icon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].inven_icon, g_i, g_p);
        break;

    case PLAYER_BUTTONPALETTE:
        if (iSet)
            ps[iPlayer].buttonpalette=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].buttonpalette, g_i, g_p);
        break;

    case PLAYER_JETPACK_ON:
        if (iSet)
            ps[iPlayer].jetpack_on=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].jetpack_on, g_i, g_p);
        break;

    case PLAYER_SPRITEBRIDGE:
        if (iSet)
            ps[iPlayer].spritebridge=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].spritebridge, g_i, g_p);
        break;

    case PLAYER_LASTRANDOMSPOT:
        if (iSet)
            ps[iPlayer].lastrandomspot=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].lastrandomspot, g_i, g_p);
        break;

    case PLAYER_SCUBA_ON:
        if (iSet)
            ps[iPlayer].scuba_on=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].scuba_on, g_i, g_p);
        break;

    case PLAYER_FOOTPRINTPAL:
        if (iSet)
            ps[iPlayer].footprintpal=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].footprintpal, g_i, g_p);
        break;

    case PLAYER_HEAT_ON:
        if (iSet)
        {
            if (ps[iPlayer].heat_on != lValue)
            {
                ps[iPlayer].heat_on=lValue;
                setpal(&ps[iPlayer]);
            }
        }
        else
            SetGameVarID(lVar2, ps[iPlayer].heat_on, g_i, g_p);
        break;

    case PLAYER_HOLSTER_WEAPON:
        if (iSet)
            ps[iPlayer].holster_weapon=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].holster_weapon, g_i, g_p);
        break;

    case PLAYER_FALLING_COUNTER:
        if (iSet)
            ps[iPlayer].falling_counter=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].falling_counter, g_i, g_p);
        break;

    case PLAYER_GOTWEAPON:
        lTemp=lParm2;
        if (iSet)
            ps[iPlayer].gotweapon[lTemp]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].gotweapon[lTemp], g_i, g_p);
        break;

    case PLAYER_REFRESH_INVENTORY:
        if (iSet)
            ps[iPlayer].refresh_inventory=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].refresh_inventory, g_i, g_p);
        break;

        //      case PLAYER_PALETTE:
        //          if(iSet)
        //          {
        //              ps[iPlayer].palette=lValue;
        //          }
        //          else
        //          {
        //              SetGameVarID(lVar2, ps[iPlayer].palette, g_i, g_p);
        //          }
        //          break;

    case PLAYER_TOGGLE_KEY_FLAG:
        if (iSet)
            ps[iPlayer].toggle_key_flag=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].toggle_key_flag, g_i, g_p);
        break;

    case PLAYER_KNUCKLE_INCS:
        if (iSet)
            ps[iPlayer].knuckle_incs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].knuckle_incs, g_i, g_p);
        break;

    case PLAYER_WALKING_SND_TOGGLE:
        if (iSet)
            ps[iPlayer].walking_snd_toggle=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].walking_snd_toggle, g_i, g_p);
        break;

    case PLAYER_PALOOKUP:
        if (iSet)
            ps[iPlayer].palookup=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].palookup, g_i, g_p);
        break;

    case PLAYER_HARD_LANDING:
        if (iSet)
            ps[iPlayer].hard_landing=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].hard_landing, g_i, g_p);
        break;

    case PLAYER_MAX_SECRET_ROOMS:
        if (iSet)
            ps[iPlayer].max_secret_rooms=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].max_secret_rooms, g_i, g_p);
        break;

    case PLAYER_SECRET_ROOMS:
        if (iSet)
            ps[iPlayer].secret_rooms=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].secret_rooms, g_i, g_p);
        break;

    case PLAYER_PALS:
        lTemp=lParm2;
        if (iSet)
            ps[iPlayer].pals[lTemp]=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].pals[lTemp], g_i, g_p);
        break;

    case PLAYER_MAX_ACTORS_KILLED:
        if (iSet)
            ps[iPlayer].max_actors_killed=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].max_actors_killed, g_i, g_p);
        break;

    case PLAYER_ACTORS_KILLED:
        if (iSet)
            ps[iPlayer].actors_killed=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].actors_killed, g_i, g_p);
        break;

    case PLAYER_RETURN_TO_CENTER:
        if (iSet)
            ps[iPlayer].return_to_center=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].return_to_center, g_i, g_p);
        break;

    case PLAYER_RUNSPEED:
        if (iSet)
            ps[iPlayer].runspeed=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].runspeed, g_i, g_p);
        break;

    case PLAYER_SBS:
        if (iSet)
            ps[iPlayer].sbs=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].sbs, g_i, g_p);
        break;

    case PLAYER_RELOADING:
        if (iSet)
            ps[iPlayer].reloading=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].reloading, g_i, g_p);
        break;

    case PLAYER_AUTO_AIM:
        if (iSet)
            ps[iPlayer].auto_aim=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].auto_aim, g_i, g_p);
        break;

    case PLAYER_MOVEMENT_LOCK:
        if (iSet)
            ps[iPlayer].movement_lock=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].movement_lock, g_i, g_p);
        break;

    case PLAYER_SOUND_PITCH:
        if (iSet)
            ps[iPlayer].sound_pitch=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].sound_pitch, g_i, g_p);
        break;

    case PLAYER_WEAPONSWITCH:
        if (iSet)
            ps[iPlayer].weaponswitch=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].weaponswitch, g_i, g_p);
        break;

    case PLAYER_TEAM:
        if (iSet)
            ps[iPlayer].team=lValue;
        else
            SetGameVarID(lVar2, ps[iPlayer].team, g_i, g_p);
        break;

    default:
        break;
    }
    return;
}

static void DoInput(int iSet, int lVar1, int lLabelID, int lVar2)
{
    long lValue;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=GetGameVarID(lVar1, g_i, g_p);

    if (iPlayer<0 || iPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case INPUT_AVEL:
        if (iSet)
            sync[iPlayer].avel=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].avel, g_i, g_p);
        break;

    case INPUT_HORZ:
        if (iSet)
            sync[iPlayer].horz=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].horz, g_i, g_p);
        break;

    case INPUT_FVEL:
        if (iSet)
            sync[iPlayer].fvel=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].fvel, g_i, g_p);
        break;

    case INPUT_SVEL:
        if (iSet)
            sync[iPlayer].svel=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].svel, g_i, g_p);
        break;

    case INPUT_BITS:
        if (iSet)
            sync[iPlayer].bits=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].bits, g_i, g_p);
        break;

    case INPUT_EXTBITS:
        if (iSet)
            sync[iPlayer].extbits=lValue;
        else
            SetGameVarID(lVar2, sync[iPlayer].extbits, g_i, g_p);
        break;
    default:
        break;
    }
    return;
}

static void DoWall(int iSet, int lVar1, int lLabelID, int lVar2)
{
    long lValue;
    int iWall = GetGameVarID(lVar1, g_i, g_p);

    if (iWall<0 || iWall >= MAXWALLS)
        return;

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case WALL_X:
        if (iSet)
            wall[iWall].x=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].x, g_i, g_p);
        break;

    case WALL_Y:
        if (iSet)
            wall[iWall].y=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].y, g_i, g_p);
        break;

    case WALL_POINT2:
        if (iSet)
            wall[iWall].point2=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].point2, g_i, g_p);
        break;

    case WALL_NEXTWALL:
        if (iSet)
            wall[iWall].nextwall=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].nextwall, g_i, g_p);
        break;

    case WALL_NEXTSECTOR:
        if (iSet)
            wall[iWall].nextsector=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].nextsector, g_i, g_p);
        break;

    case WALL_CSTAT:
        if (iSet)
            wall[iWall].cstat=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].cstat, g_i, g_p);
        break;

    case WALL_PICNUM:
        if (iSet)
            wall[iWall].picnum=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].picnum, g_i, g_p);
        break;

    case WALL_OVERPICNUM:
        if (iSet)
            wall[iWall].overpicnum=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].overpicnum, g_i, g_p);
        break;

    case WALL_SHADE:
        if (iSet)
            wall[iWall].shade=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].shade, g_i, g_p);
        break;

    case WALL_PAL:
        if (iSet)
            wall[iWall].pal=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].pal, g_i, g_p);
        break;

    case WALL_XREPEAT:
        if (iSet)
            wall[iWall].xrepeat=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].xrepeat, g_i, g_p);
        break;

    case WALL_YREPEAT:
        if (iSet)
            wall[iWall].yrepeat=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].yrepeat, g_i, g_p);
        break;

    case WALL_XPANNING:
        if (iSet)
            wall[iWall].xpanning=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].xpanning, g_i, g_p);
        break;

    case WALL_YPANNING:
        if (iSet)
            wall[iWall].ypanning=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].ypanning, g_i, g_p);
        break;

    case WALL_LOTAG:
        if (iSet)
            wall[iWall].lotag=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].lotag, g_i, g_p);
        break;

    case WALL_HITAG:
        if (iSet)
            wall[iWall].hitag=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].hitag, g_i, g_p);
        break;

    case WALL_EXTRA:
        if (iSet)
            wall[iWall].extra=lValue;
        else
            SetGameVarID(lVar2, wall[iWall].extra, g_i, g_p);
        break;
    default:
        break;
    }
    return;
}

static void DoSector(int iSet, int lVar1, int lLabelID, int lVar2)
{
    long lValue;
    int iSector=sprite[g_i].sectnum;

    if (lVar1 != g_iThisActorID)
        iSector=GetGameVarID(lVar1, g_i, g_p);

    if (iSector<0 || iSector >= MAXSECTORS)
        return;

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case SECTOR_WALLPTR:
        if (iSet)
            sector[iSector].wallptr=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].wallptr,g_i,g_p);
        break;

    case SECTOR_WALLNUM:
        if (iSet)
            sector[iSector].wallnum=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].wallnum,g_i,g_p);
        break;

    case SECTOR_CEILINGZ:
        if (iSet)
            sector[iSector].ceilingz=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingz,g_i,g_p);
        break;

    case SECTOR_FLOORZ:
        if (iSet)
            sector[iSector].floorz=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorz,g_i,g_p);
        break;

    case SECTOR_CEILINGSTAT:
        if (iSet)
            sector[iSector].ceilingstat=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingstat,g_i,g_p);
        break;

    case SECTOR_FLOORSTAT:
        if (iSet)
            sector[iSector].floorstat=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorstat,g_i,g_p);
        break;

    case SECTOR_CEILINGPICNUM:
        if (iSet)
            sector[iSector].ceilingpicnum=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingpicnum,g_i,g_p);
        break;

    case SECTOR_CEILINGSLOPE:
        if (iSet)
            sector[iSector].ceilingheinum=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingheinum,g_i,g_p);
        break;

    case SECTOR_CEILINGSHADE:
        if (iSet)
            sector[iSector].ceilingshade=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingshade,g_i,g_p);
        break;

    case SECTOR_CEILINGPAL:
        if (iSet)
            sector[iSector].ceilingpal=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingpal,g_i,g_p);
        break;

    case SECTOR_CEILINGXPANNING:
        if (iSet)
            sector[iSector].ceilingxpanning=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].ceilingypanning,g_i,g_p);
        break;

    case SECTOR_FLOORPICNUM:
        if (iSet)
            sector[iSector].floorpicnum=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorpicnum,g_i,g_p);
        break;

    case SECTOR_FLOORSLOPE:
        if (iSet)
            sector[iSector].floorheinum=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorheinum,g_i,g_p);
        break;

    case SECTOR_FLOORSHADE:
        if (iSet)
            sector[iSector].floorshade=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorshade,g_i,g_p);
        break;

    case SECTOR_FLOORPAL:
        if (iSet)
            sector[iSector].floorpal=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorpal,g_i,g_p);
        break;

    case SECTOR_FLOORXPANNING:
        if (iSet)
            sector[iSector].floorxpanning=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorxpanning,g_i,g_p);
        break;

    case SECTOR_FLOORYPANNING:
        if (iSet)
            sector[iSector].floorypanning=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].floorypanning,g_i,g_p);
        break;

    case SECTOR_VISIBILITY:
        if (iSet)
            sector[iSector].visibility=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].visibility,g_i,g_p);
        break;

    case SECTOR_ALIGNTO:
        if (iSet)
            sector[iSector].filler=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].filler,g_i,g_p);
        break;

    case SECTOR_LOTAG:
        if (iSet)
            sector[iSector].lotag=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].lotag,g_i,g_p);
        break;

    case SECTOR_HITAG:
        if (iSet)
            sector[iSector].hitag=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].hitag,g_i,g_p);
        break;

    case SECTOR_EXTRA:
        if (iSet)
            sector[iSector].extra=lValue;
        else
            SetGameVarID(lVar2, sector[iSector].extra,g_i,g_p);
        break;

    default:
        break;
    }
    return;
}

static void DoActor(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    long lValue;
    long lTemp;
    int iActor=g_i;

    if (lVar1 != g_iThisActorID)
        iActor=GetGameVarID(lVar1, g_i, g_p);

    if (iActor < 0 || iActor >= MAXSPRITES)
    {
        OSD_Printf("DoActor(): invalid target sprite (%d) %d %d\n",iActor,g_i,g_sp->picnum);
        return;
    }

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
            sprite[iActor].x=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].x,g_i,g_p);
        break;

    case ACTOR_Y:
        if (iSet)
            sprite[iActor].y=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].y,g_i,g_p);
        break;

    case ACTOR_Z:
        if (iSet)
            sprite[iActor].z=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].z,g_i,g_p);
        break;

    case ACTOR_CSTAT:
        if (iSet)
            sprite[iActor].cstat=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].cstat,g_i,g_p);
        break;

    case ACTOR_PICNUM:
        if (iSet)
            sprite[iActor].picnum=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].picnum,g_i,g_p);
        break;

    case ACTOR_SHADE:
        if (iSet)
            sprite[iActor].shade=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].shade,g_i,g_p);
        break;

    case ACTOR_PAL:
        if (iSet)
            sprite[iActor].pal=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].pal,g_i,g_p);
        break;

    case ACTOR_CLIPDIST:
        if (iSet)
            sprite[iActor].clipdist=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].clipdist,g_i,g_p);
        break;

    case ACTOR_DETAIL:
        if (iSet)
            sprite[iActor].filler=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].filler,g_i,g_p);
        break;

    case ACTOR_XREPEAT:
        if (iSet)
            sprite[iActor].xrepeat=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].xrepeat,g_i,g_p);
        break;

    case ACTOR_YREPEAT:
        if (iSet)
            sprite[iActor].yrepeat=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].yrepeat,g_i,g_p);
        break;

    case ACTOR_XOFFSET:
        if (iSet)
            sprite[iActor].xoffset=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].xoffset,g_i,g_p);
        break;

    case ACTOR_YOFFSET:
        if (iSet)
            sprite[iActor].yoffset=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].yoffset,g_i,g_p);
        break;

    case ACTOR_SECTNUM:
        if (iSet)
            changespritesect(iActor,lValue);
        else
            SetGameVarID(lVar2, sprite[iActor].sectnum,g_i,g_p);
        break;

    case ACTOR_STATNUM:
        if (iSet)
            changespritestat(iActor,lValue);
        else
            SetGameVarID(lVar2, sprite[iActor].statnum,g_i,g_p);
        break;

    case ACTOR_ANG:
        if (iSet)
            sprite[iActor].ang=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].ang,g_i,g_p);
        break;

    case ACTOR_OWNER:
        if (iSet)
            sprite[iActor].owner=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].owner,g_i,g_p);
        break;

    case ACTOR_XVEL:
        if (iSet)
            sprite[iActor].xvel=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].xvel,g_i,g_p);
        break;

    case ACTOR_YVEL:
        if (iSet)
            sprite[iActor].yvel=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].yvel,g_i,g_p);
        break;

    case ACTOR_ZVEL:
        if (iSet)
            sprite[iActor].zvel=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].zvel,g_i,g_p);
        break;

    case ACTOR_LOTAG:
        if (iSet)
            sprite[iActor].lotag=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].lotag,g_i,g_p);
        break;

    case ACTOR_HITAG:
        if (iSet)
            sprite[iActor].hitag=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].hitag,g_i,g_p);
        break;

    case ACTOR_EXTRA:
        if (iSet)
            sprite[iActor].extra=lValue;
        else
            SetGameVarID(lVar2, sprite[iActor].extra,g_i,g_p);
        break;

    case ACTOR_HTCGG:
        if (iSet)
            hittype[iActor].cgg=lValue;
        else
            SetGameVarID(lVar2, hittype[iActor].cgg, g_i, g_p);
        break;

    case ACTOR_HTPICNUM :
        if (iSet)
            hittype[iActor].picnum=lValue;
        else
            SetGameVarID(lVar2, hittype[iActor].picnum, g_i, g_p);
        break;

    case ACTOR_HTANG:
        if (iSet)
            hittype[iActor].ang=lValue;
        else
            SetGameVarID(lVar2, hittype[iActor].ang, g_i, g_p);
        break;

    case ACTOR_HTEXTRA:
        if (iSet)
            hittype[iActor].extra=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].extra, g_i, g_p);
        break;

    case ACTOR_HTOWNER:
        if (iSet)
            hittype[iActor].owner=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].owner, g_i, g_p);
        break;

    case ACTOR_HTMOVFLAG:
        if (iSet)
            hittype[iActor].movflag=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].movflag, g_i, g_p);
        break;

    case ACTOR_HTTEMPANG:
        if (iSet)
            hittype[iActor].tempang=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].tempang, g_i, g_p);
        break;

    case ACTOR_HTACTORSTAYPUT:
        if (iSet)
            hittype[iActor].actorstayput=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].actorstayput, g_i, g_p);
        break;

    case ACTOR_HTDISPICNUM:
        if (iSet)
            hittype[iActor].dispicnum=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].dispicnum, g_i, g_p);
        break;

    case ACTOR_HTTIMETOSLEEP:
        if (iSet)
            hittype[iActor].timetosleep=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].timetosleep, g_i, g_p);
        break;

    case ACTOR_HTFLOORZ:
        if (iSet)
            hittype[iActor].floorz=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].floorz, g_i, g_p);
        break;

    case ACTOR_HTCEILINGZ:
        if (iSet)
            hittype[iActor].ceilingz=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].ceilingz, g_i, g_p);
        break;

    case ACTOR_HTLASTVX:
        if (iSet)
            hittype[iActor].lastvx=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].lastvx, g_i, g_p);
        break;

    case ACTOR_HTLASTVY:
        if (iSet)
            hittype[iActor].lastvy=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].lastvy, g_i, g_p);
        break;

    case ACTOR_HTBPOSX:
        if (iSet)
            hittype[iActor].bposx=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].bposx, g_i, g_p);
        break;

    case ACTOR_HTBPOSY:
        if (iSet)
            hittype[iActor].bposy=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].bposy, g_i, g_p);
        break;

    case ACTOR_HTBPOSZ:
        if (iSet)
            hittype[iActor].bposz=lValue;
        else
            SetGameVarID(lVar2,hittype[iActor].bposz, g_i, g_p);
        break;

    case ACTOR_HTG_T:
        lTemp=lParm2;
        if (iSet)
            hittype[iActor].temp_data[lTemp]=lValue;
        else
            SetGameVarID(lVar2, hittype[iActor].temp_data[lTemp], g_i, g_p);
        break;

    case ACTOR_ANGOFF:
        if (iSet)
            spriteext[iActor].angoff=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].angoff, g_i, g_p);
        break;

    case ACTOR_PITCH:
        if (iSet)
            spriteext[iActor].pitch=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].pitch, g_i, g_p);
        break;

    case ACTOR_ROLL:
        if (iSet)
            spriteext[iActor].roll=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].roll, g_i, g_p);
        break;

    case ACTOR_MDXOFF:
        if (iSet)
            spriteext[iActor].xoff=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].xoff, g_i, g_p);
        break;

    case ACTOR_MDYOFF:
        if (iSet)
            spriteext[iActor].yoff=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].yoff, g_i, g_p);
        break;

    case ACTOR_MDZOFF:
        if (iSet)
            spriteext[iActor].zoff=lValue;
        else
            SetGameVarID(lVar2,spriteext[iActor].zoff, g_i, g_p);
        break;

    default:
        break;
    }
    return;
}

static void DoProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    long lValue;

    if (lVar1 < 0 || lVar1 >= MAXTILES)
    {
        OSD_Printf("DoProjectile(): invalid tile (%d)\n",lVar1);
        return;
    }

    lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
            projectile[lVar1].workslike=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].workslike, g_i, g_p);
        break;

    case PROJ_SPAWNS:
        if (iSet)
            projectile[lVar1].spawns=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].spawns, g_i, g_p);
        break;

    case PROJ_SXREPEAT:
        if (iSet)
            projectile[lVar1].sxrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].sxrepeat, g_i, g_p);
        break;

    case PROJ_SYREPEAT:
        if (iSet)
            projectile[lVar1].syrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].syrepeat, g_i, g_p);
        break;

    case PROJ_SOUND:
        if (iSet)
            projectile[lVar1].sound=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].sound, g_i, g_p);
        break;

    case PROJ_ISOUND:
        if (iSet)
            projectile[lVar1].isound=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].isound, g_i, g_p);
        break;

    case PROJ_VEL:
        if (iSet)
            projectile[lVar1].vel=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].vel, g_i, g_p);
        break;

    case PROJ_EXTRA:
        if (iSet)
            projectile[lVar1].extra=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].extra, g_i, g_p);
        break;

    case PROJ_DECAL:
        if (iSet)
            projectile[lVar1].decal=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].decal, g_i, g_p);
        break;

    case PROJ_TRAIL:
        if (iSet)
            projectile[lVar1].trail=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].trail, g_i, g_p);
        break;

    case PROJ_TXREPEAT:
        if (iSet)
            projectile[lVar1].txrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].txrepeat, g_i, g_p);
        break;

    case PROJ_TYREPEAT:
        if (iSet)
            projectile[lVar1].tyrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].tyrepeat, g_i, g_p);
        break;

    case PROJ_TOFFSET:
        if (iSet)
            projectile[lVar1].toffset=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].toffset, g_i, g_p);
        break;

    case PROJ_TNUM:
        if (iSet)
            projectile[lVar1].tnum=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].tnum, g_i, g_p);
        break;

    case PROJ_DROP:
        if (iSet)
            projectile[lVar1].drop=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].drop, g_i, g_p);
        break;

    case PROJ_CSTAT:
        if (iSet)
            projectile[lVar1].cstat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].cstat, g_i, g_p);
        break;

    case PROJ_CLIPDIST:
        if (iSet)
            projectile[lVar1].clipdist=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].clipdist, g_i, g_p);
        break;

    case PROJ_SHADE:
        if (iSet)
            projectile[lVar1].shade=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].shade, g_i, g_p);
        break;

    case PROJ_XREPEAT:
        if (iSet)
            projectile[lVar1].xrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].xrepeat, g_i, g_p);
        break;

    case PROJ_YREPEAT:
        if (iSet)
            projectile[lVar1].yrepeat=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].yrepeat, g_i, g_p);
        break;

    case PROJ_PAL:
        if (iSet)
            projectile[lVar1].pal=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].pal, g_i, g_p);
        break;

    case PROJ_EXTRA_RAND:
        if (iSet)
            projectile[lVar1].extra_rand=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].extra_rand, g_i, g_p);
        break;

    case PROJ_HITRADIUS:
        if (iSet)
            projectile[lVar1].hitradius=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].hitradius, g_i, g_p);
        break;

    case PROJ_VEL_MULT:
        if (iSet)
            projectile[lVar1].velmult=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].velmult, g_i, g_p);
        break;

    case PROJ_OFFSET:
        if (iSet)
            projectile[lVar1].offset=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].offset, g_i, g_p);
        break;

    case PROJ_BOUNCES:
        if (iSet)
            projectile[lVar1].bounces=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].bounces, g_i, g_p);
        break;

    case PROJ_BSOUND:
        if (iSet)
            projectile[lVar1].bsound=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].bsound, g_i, g_p);
        break;

    case PROJ_RANGE:
        if (iSet)
            projectile[lVar1].range=lValue;
        else
            SetGameVarID(lVar2, projectile[lVar1].range, g_i, g_p);
        break;

    default:
        break;
    }
    return;
}

static int parse(void);

void OnEvent(int iEventID, int iActor, int iPlayer, long lDist)
{
    if (iEventID >= MAXGAMEEVENTS)
    {
        OSD_Printf("OnEvent(): invalid event ID");
        return;
    }

    if (apScriptGameEvent[iEventID] == 0)
    {
        //Bsprintf(g_szBuf,"No event found for %d",iEventID);
        //AddLog(g_szBuf);
        return;
    }

    {
        int og_i=g_i, og_p=g_p, okillit_flag=killit_flag;
        long og_x=g_x, *og_t=g_t, *oinsptr=insptr;
        spritetype *og_sp=g_sp;

        g_i = iActor;    // current sprite ID
        g_p = iPlayer;    // current player ID
        g_x = lDist;    // ?
        g_sp = &sprite[g_i];
        g_t = &hittype[g_i].temp_data[0];

        insptr = (apScriptGameEvent[iEventID]);
        //Bsprintf(g_szBuf,"Executing event for %d at %lX",iEventID, insptr);
        //AddLog(g_szBuf);

        killit_flag = 0;

        while (1) if (parse()) break;

        if (killit_flag == 1)
        {
            // if player was set to squish, first stop that...
            if (g_p >= 0)
            {
                if (ps[g_p].actorsqu == g_i)
                    ps[g_p].actorsqu = -1;
            }
            deletesprite(g_i);
        }

        // restore old values...
        g_i=og_i;
        g_p=og_p;
        g_x=og_x;
        g_sp=og_sp;
        g_t=og_t;
        killit_flag=okillit_flag;
        insptr=oinsptr;

        //AddLog("End of Execution");
    }
}

static int ifsquished(int i, int p)
{
    sectortype *sc = &sector[SECT];
    int squishme;

    if (PN == APLAYER && ud.clipping)
        return 0;

    if (sc->lotag != 23)
    {
        squishme = (sc->floorz - sc->ceilingz < (12<<8)); // && (sc->lotag&32768) == 0;

        if (sprite[i].pal == 1)
            squishme = (sc->floorz - sc->ceilingz < (32<<8) && (sc->lotag&32768) == 0);
    }
    else squishme = 0;

    if (squishme)
    {
        FTA(10,&ps[p]);

        if (badguy(&sprite[i])) sprite[i].xvel = 0;

        if (sprite[i].pal == 1)
        {
            hittype[i].picnum = SHOTSPARK1;
            hittype[i].extra = 1;
            return 0;
        }

        return 1;
    }
    return 0;
}

static void forceplayerangle(struct player_struct *p)
{
    int n = 128-(TRAND&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = n>>1;
    p->rotscrnang = n>>1;
}

static char dodge(spritetype *s)
{
    long bx,by,bxvect,byvect,d,i;
    long mx = s->x, my = s->y;
    long mxvect = sintable[(s->ang+512)&2047];
    long myvect = sintable[s->ang&2047];

    for (i=headspritestat[4];i>=0;i=nextspritestat[i]) //weapons list
    {
        if (OW == i || SECT != s->sectnum)
            continue;

        bx = SX-mx;
        by = SY-my;
        bxvect = sintable[(SA+512)&2047];
        byvect = sintable[SA&2047];

        if (mxvect*bx + myvect*by >= 0)
            if (bxvect*bx + byvect*by < 0)
            {
                d = bxvect*by - byvect*bx;
                if (klabs(d) < 65536*64)
                {
                    s->ang -= 512+(TRAND&1024);
                    return 1;
                }
            }
    }
    return 0;
}

int furthestangle(int iActor,int angs)
{
    short hitsect,hitwall,hitspr,furthest_angle=0;
    long hx, hy, hz, d;
    spritetype *s = &sprite[iActor];
    long greatestd = -(1<<30);
    int angincs = 2048/angs,j;

    if (s->picnum != APLAYER)
        if ((g_t[0]&63) > 2) return(s->ang + 1024);

    for (j=s->ang;j<(2048+s->ang);j+=angincs)
    {
        hitscan(s->x, s->y, s->z-(8<<8), s->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047],0,
                &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-s->x) + klabs(hy-s->y);

        if (d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return (furthest_angle&2047);
}

int furthestcanseepoint(int iActor,spritetype *ts,long *dax,long *day)
{
    short hitsect,hitwall,hitspr, angincs;
    long hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
    int j;
    spritetype *s = &sprite[iActor];

    if ((g_t[0]&63)) return -1;

    if (ud.multimode < 2 && ud.player_skill < 3)
        angincs = 2048/2;
    else angincs = 2048/(1+(TRAND&1));

    for (j=ts->ang;j<(2048+ts->ang);j+=(angincs-(TRAND&511)))
    {
        hitscan(ts->x, ts->y, ts->z-(16<<8), ts->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047],16384-(TRAND&32767),
                &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-ts->x)+klabs(hy-ts->y);
        da = klabs(hx-s->x)+klabs(hy-s->y);

        if (d < da)
            if (cansee(hx,hy,hz,hitsect,s->x,s->y,s->z-(16<<8),s->sectnum))
            {
                *dax = hx;
                *day = hy;
                return hitsect;
            }
    }
    return -1;
}

void getglobalz(int iActor)
{
    spritetype *s = &sprite[iActor];

    if (s->statnum == 10 || s->statnum == 6 || s->statnum == 2 || s->statnum == 1 || s->statnum == 4)
    {
        long hz,lz,zr = 127L;

        if (s->statnum == 4)
            zr = 4L;

        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[iActor].ceilingz,&hz,&hittype[iActor].floorz,&lz,zr,CLIPMASK0);

        if ((lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0)
        {
            lz &= (MAXSPRITES-1);
            if (badguy(&sprite[lz]) && sprite[lz].pal != 1)
            {
                if (s->statnum != 4)
                {
                    hittype[iActor].dispicnum = -4; // No shadows on actors
                    s->xvel = -256;
                    ssp(iActor,CLIPMASK0);
                }
            }
            else if (sprite[lz].picnum == APLAYER && badguy(s))
            {
                hittype[iActor].dispicnum = -4; // No shadows on actors
                s->xvel = -256;
                ssp(iActor,CLIPMASK0);
            }
            else if (s->statnum == 4 && sprite[lz].picnum == APLAYER)
                if (s->owner == lz)
                {
                    hittype[iActor].ceilingz = sector[s->sectnum].ceilingz;
                    hittype[iActor].floorz   = sector[s->sectnum].floorz;
                }
        }
    }
    else
    {
        hittype[iActor].ceilingz = sector[s->sectnum].ceilingz;
        hittype[iActor].floorz   = sector[s->sectnum].floorz;
    }
}

void makeitfall(int iActor)
{
    spritetype *s = &sprite[iActor];
    long hz,lz,c = gc;

    if (floorspace(s->sectnum))
        c = 0;
    else
    {
        if (ceilingspace(s->sectnum) || sector[s->sectnum].lotag == 2)
            c = gc/6;
    }

    if ((s->statnum == 1 || s->statnum == 10 || s->statnum == 2 || s->statnum == 6))
        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[iActor].ceilingz,&hz,&hittype[iActor].floorz,&lz,127L,CLIPMASK0);
    else
    {
        hittype[iActor].ceilingz = sector[s->sectnum].ceilingz;
        hittype[iActor].floorz   = sector[s->sectnum].floorz;
    }

    if (s->z < hittype[iActor].floorz-(FOURSLEIGHT))
    {
        if (sector[s->sectnum].lotag == 2 && s->zvel > 3122)
            s->zvel = 3144;
        if (s->zvel < 6144)
            s->zvel += c;
        else s->zvel = 6144;
        s->z += s->zvel;
    }
    if (s->z >= hittype[iActor].floorz-(FOURSLEIGHT))
    {
        s->z = hittype[iActor].floorz - FOURSLEIGHT;
        s->zvel = 0;
    }
}

int getincangle(int a,int na)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a-na) < 1024)
    {
//        OSD_Printf("getincangle() returning %d\n",na-a);
        return (na-a);
    }

    if (na > 1024) na -= 2048;
    if (a > 1024) a -= 2048;

    na -= 2048;
    a -= 2048;
//    OSD_Printf("getincangle() returning %d\n",na-a);
    return (na-a);
}

static void alterang(int a)
{
    int aang = g_sp->ang, angdif, goalang;
    long ticselapsed = (g_t[0])&31, *moveptr = (long *)g_t[1];
    int j = ps[g_p].holoduke_on;

    g_sp->xvel += (*moveptr-g_sp->xvel)/5;
    if (g_sp->zvel < 648) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)/5;

    if (a&seekplayer)
    {
        // NOTE: looks like 'owner' is set to target sprite ID...

        if (j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,g_sp->x,g_sp->y,g_sp->z,g_sp->sectnum))
            g_sp->owner = j;
        else g_sp->owner = ps[g_p].i;

        if (sprite[g_sp->owner].picnum == APLAYER)
            goalang = getangle(hittype[g_i].lastvx-g_sp->x,hittype[g_i].lastvy-g_sp->y);
        else
            goalang = getangle(sprite[g_sp->owner].x-g_sp->x,sprite[g_sp->owner].y-g_sp->y);

        if (g_sp->xvel && g_sp->picnum != DRONE)
        {
            angdif = getincangle(aang,goalang);

            if (ticselapsed < 2)
            {
                if (klabs(angdif) < 256)
                {
                    j = 128-(TRAND&256);
                    g_sp->ang += j;
                    if (hits(g_i) < 844)
                        g_sp->ang -= j;
                }
            }
            else if (ticselapsed > 18 && ticselapsed < 26) // choose
            {
                if (klabs(angdif>>2) < 128) g_sp->ang = goalang;
                else g_sp->ang += angdif>>2;
            }
        }
        else g_sp->ang = goalang;
    }

    if (ticselapsed < 1)
    {
        j = 2;
        if (a&furthestdir)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang;
            g_sp->owner = ps[g_p].i;
        }

        if (a&fleeenemy)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang; // += angdif; //  = getincangle(aang,goalang)>>1;
        }
    }
}

static void move(void)
{
    long l, *moveptr;
    int a = g_sp->hitag, goalang, angdif;
    long daxvel;

    if (a == -1) a = 0;

    g_t[0]++;

    if (a&face_player)
    {
        if (ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        g_sp->ang += angdif;
    }

    if (a&spin)
        g_sp->ang += sintable[((g_t[0]<<3)&2047)]>>6;

    if (a&face_player_slow)
    {
        if (ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>4;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        g_sp->ang += angdif;
    }

    if ((a&jumptoplayer) == jumptoplayer)
    {
        if (g_t[0] < 16)
            g_sp->zvel -= (sintable[(512+(g_t[0]<<4))&2047]>>5);
    }

    if (a&face_player_smart)
    {
        long newx = ps[g_p].posx+(ps[g_p].posxv/768);
        long newy = ps[g_p].posy+(ps[g_p].posyv/768);

        goalang = getangle(newx-g_sp->x,newy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        g_sp->ang += angdif;
    }

    if (g_t[1] == 0 || a == 0)
    {
        if ((badguy(g_sp) && g_sp->extra <= 0) || (hittype[g_i].bposx != g_sp->x) || (hittype[g_i].bposy != g_sp->y))
        {
            hittype[g_i].bposx = g_sp->x;
            hittype[g_i].bposy = g_sp->y;
            setsprite(g_i,g_sp->x,g_sp->y,g_sp->z);
        }
        return;
    }

    moveptr = (long *)g_t[1];

    if (a&geth) g_sp->xvel += (*moveptr-g_sp->xvel)>>1;
    if (a&getv) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)>>1;

    if (a&dodgebullet)
        dodge(g_sp);

    if (g_sp->picnum != APLAYER)
        alterang(a);

    if (g_sp->xvel > -6 && g_sp->xvel < 6) g_sp->xvel = 0;

    a = badguy(g_sp);

    if (g_sp->xvel || g_sp->zvel)
    {
        if (a && g_sp->picnum != ROTATEGUN)
        {
            if ((g_sp->picnum == DRONE || g_sp->picnum == COMMANDER) && g_sp->extra > 0)
            {
                if (g_sp->picnum == COMMANDER)
                {
                    hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if (g_sp->z > (l-(8<<8)))
                    {
                        if (g_sp->z > (l-(8<<8))) g_sp->z = l-(8<<8);
                        g_sp->zvel = 0;
                    }

                    hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if ((g_sp->z-l) < (80<<8))
                    {
                        g_sp->z = l+(80<<8);
                        g_sp->zvel = 0;
                    }
                }
                else
                {
                    if (g_sp->zvel > 0)
                    {
                        hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if (g_sp->z > (l-(30<<8)))
                            g_sp->z = l-(30<<8);
                    }
                    else
                    {
                        hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if ((g_sp->z-l) < (50<<8))
                        {
                            g_sp->z = l+(50<<8);
                            g_sp->zvel = 0;
                        }
                    }
                }
            }
            else if (g_sp->picnum != ORGANTIC)
            {
                if (g_sp->zvel > 0 && hittype[g_i].floorz < g_sp->z)
                    g_sp->z = hittype[g_i].floorz;
                if (g_sp->zvel < 0)
                {
                    l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if ((g_sp->z-l) < (66<<8))
                    {
                        g_sp->z = l+(66<<8);
                        g_sp->zvel >>= 1;
                    }
                }
            }
        }
        else if (g_sp->picnum == APLAYER)
            if ((g_sp->z-hittype[g_i].ceilingz) < (32<<8))
                g_sp->z = hittype[g_i].ceilingz+(32<<8);

        daxvel = g_sp->xvel;
        angdif = g_sp->ang;

        if (a && g_sp->picnum != ROTATEGUN)
        {
            if (g_x < 960 && g_sp->xrepeat > 16)
            {

                daxvel = -(1024-g_x);
                angdif = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);

                if (g_x < 512)
                {
                    ps[g_p].posxv = 0;
                    ps[g_p].posyv = 0;
                }
                else
                {
                    ps[g_p].posxv = mulscale(ps[g_p].posxv,ps[g_p].runspeed-0x2000,16);
                    ps[g_p].posyv = mulscale(ps[g_p].posyv,ps[g_p].runspeed-0x2000,16);
                }
            }
            else if (g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != COMMANDER)
            {
                if (hittype[g_i].bposz != g_sp->z || (ud.multimode < 2 && ud.player_skill < 2))
                {
                    if ((g_t[0]&1) || ps[g_p].actorsqu == g_i) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if ((g_t[0]&3) || ps[g_p].actorsqu == g_i) return;
                    else daxvel <<= 2;
                }
            }
        }

        hittype[g_i].movflag = movesprite(g_i,
                                          (daxvel*(sintable[(angdif+512)&2047]))>>14,
                                          (daxvel*(sintable[angdif&2047]))>>14,g_sp->zvel,CLIPMASK0);
    }

    if (a)
    {
        if (sector[g_sp->sectnum].ceilingstat&1)
            g_sp->shade += (sector[g_sp->sectnum].ceilingshade-g_sp->shade)>>1;
        else g_sp->shade += (sector[g_sp->sectnum].floorshade-g_sp->shade)>>1;

        if (sector[g_sp->sectnum].floorpicnum == MIRROR)
            deletesprite(g_i);
    }
}

static void parseifelse(long condition)
{
    if (condition)
    {
        // skip 'else' pointer.. and...
        insptr+=2;
        parse();
    }
    else
    {
        insptr = (long *) *(insptr+1);
        if (*insptr == 10)
        {
            // else...
            // skip 'else' and...
            insptr+=2;
            parse();
        }
    }
}

// long *it = 0x00589a04;

static int parse(void)
{
    long j, l, s, tw;

    if (killit_flag) return 1;

    //    if(*it == 1668249134L) gameexit("\nERR");
    //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
    //      AddLog(g_szBuf);

    tw = *insptr;

    switch (tw)
    {
    case CON_REDEFINEQUOTE:
        insptr++;
        {
            int q = *insptr++, i = *insptr++;
            if (fta_quotes[q] == NULL || redefined_quotes[i] == NULL)
            {
                OSD_Printf("%s %d null quote %d %d\n",__FILE__,__LINE__,q,i);
                break;
            }
            Bstrcpy(fta_quotes[q],redefined_quotes[i]);
            break;
        }

    case CON_GETTHISPROJECTILE:
    case CON_SETTHISPROJECTILE:
        insptr++;
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

            DoThisProjectile(tw==CON_SETTHISPROJECTILE,lVar1,lLabelID,lVar2);
            break;
        }

    case CON_IFRND:
        insptr++;
        parseifelse(rnd(*insptr));
        break;

    case CON_IFCANSHOOTTARGET:
        if (g_x > 1024)
        {
            short temphit, sclip = 768, angdif = 16;

            if (badguy(g_sp) && g_sp->xrepeat > 56)
            {
                sclip = 3084;
                angdif = 48;
            }

            j = hitasprite(g_i,&temphit);
            if (j == (1<<30))
            {
                parseifelse(1);
                break;
            }
            if (j > sclip)
            {
                if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                    j = 0;
                else
                {
                    g_sp->ang += angdif;
                    j = hitasprite(g_i,&temphit);
                    g_sp->ang -= angdif;
                    if (j > sclip)
                    {
                        if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                            j = 0;
                        else
                        {
                            g_sp->ang -= angdif;
                            j = hitasprite(g_i,&temphit);
                            g_sp->ang += angdif;
                            if (j > 768)
                            {
                                if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                                    j = 0;
                                else j = 1;
                            }
                            else j = 0;
                        }
                    }
                    else j = 0;
                }
            }
            else j =  0;
        }
        else j = 1;

        parseifelse(j);
        break;

    case CON_IFCANSEETARGET:
        j = cansee(g_sp->x,g_sp->y,g_sp->z-((TRAND&41)<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz/*-((TRAND&41)<<8)*/,sprite[ps[g_p].i].sectnum);
        parseifelse(j);
        if (j) hittype[g_i].timetosleep = SLEEPTIME;
        break;

    case CON_IFACTORNOTSTAYPUT:
        parseifelse(hittype[g_i].actorstayput == -1);
        break;

    case CON_IFCANSEE:
    {
        spritetype *s;

        // select sprite for monster to target
        // if holoduke is on, let them target holoduke first.
        //
        if (ps[g_p].holoduke_on >= 0)
        {
            s = &sprite[ps[g_p].holoduke_on];
            j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((32<<8)-1)),g_sp->sectnum,
                       s->x,s->y,s->z,s->sectnum);

            if (j == 0)
            {
                // they can't see player's holoduke
                // check for player...
                s = &sprite[ps[g_p].i];
            }
        }
        else s = &sprite[ps[g_p].i];    // holoduke not on. look for player

        // can they see player, (or player's holoduke)
        j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((47<<8))),g_sp->sectnum,
                   s->x,s->y,s->z-(24<<8),s->sectnum);

        if (j == 0)
        {
            // they can't see it.

            // Huh?.  This does nothing....
            // (the result is always j==0....)
            if ((klabs(hittype[g_i].lastvx-g_sp->x)+klabs(hittype[g_i].lastvy-g_sp->y)) <
                    (klabs(hittype[g_i].lastvx-s->x)+klabs(hittype[g_i].lastvy-s->y)))
                j = 0;

            // um yeah, this if() will always fire....
            if (j == 0)
            {
                // search around for target player

                // also modifies 'target' x&y if found..

                j = furthestcanseepoint(g_i,s,&hittype[g_i].lastvx,&hittype[g_i].lastvy);

                if (j == -1) j = 0;
                else j = 1;
            }
        }
        else
        {
            // else, they did see it.
            // save where we were looking...
            hittype[g_i].lastvx = s->x;
            hittype[g_i].lastvy = s->y;
        }

        if (j == 1 && (g_sp->statnum == 1 || g_sp->statnum == 6))
            hittype[g_i].timetosleep = SLEEPTIME;

        parseifelse(j == 1);
        break;
    }

    case CON_IFHITWEAPON:
        parseifelse(ifhitbyweapon(g_i) >= 0);
        break;

    case CON_IFSQUISHED:
        parseifelse(ifsquished(g_i, g_p) == 1);
        break;

    case CON_IFDEAD:
        j = g_sp->extra;
        if (g_sp->picnum == APLAYER)
            j--;
        parseifelse(j < 0);
        break;

    case CON_AI:
        insptr++;
        g_t[5] = *insptr++; // Ai
        g_t[4] = *(long *)(g_t[5]);       // Action
        g_t[1] = *(long *)(g_t[5]+4);       // move
        g_sp->hitag = *(long *)(g_t[5]+8);    // move flags
        g_t[0] = g_t[2] = g_t[3] = 0; // count, actioncount... g_t[3] = ???
        if (g_sp->hitag&random_angle)
            g_sp->ang = TRAND&2047;
        break;

    case CON_ACTION:
        insptr++;
        g_t[2] = 0;
        g_t[3] = 0;
        g_t[4] = *insptr++;
        break;

    case CON_IFPDISTL:
        insptr++;
        parseifelse(g_x < *insptr);
        if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
            hittype[g_i].timetosleep = SLEEPTIME;
        break;

    case CON_IFPDISTG:
        insptr++;
        parseifelse(g_x > *insptr);
        if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
            hittype[g_i].timetosleep = SLEEPTIME;
        break;

    case CON_ELSE:
        insptr = (long *) *(insptr+1);
        break;

    case CON_ADDSTRENGTH:
        insptr++;
        g_sp->extra += *insptr++;
        break;

    case CON_STRENGTH:
        insptr++;
        g_sp->extra = *insptr++;
        break;

    case CON_IFGOTWEAPONCE:
        insptr++;

        if ((gametype_flags[ud.coop]&GAMETYPE_FLAG_WEAPSTAY) && ud.multimode > 1)
        {
            if (*insptr == 0)
            {
                for (j=0;j < ps[g_p].weapreccnt;j++)
                    if (ps[g_p].weaprecs[j] == g_sp->picnum)
                        break;

                parseifelse(j < ps[g_p].weapreccnt && g_sp->owner == g_i);
            }
            else if (ps[g_p].weapreccnt < 16)
            {
                ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
                parseifelse(g_sp->owner == g_i);
            }
            else parseifelse(0);
        }
        else parseifelse(0);
        break;

    case CON_GETLASTPAL:
        insptr++;
        if (g_sp->picnum == APLAYER)
            g_sp->pal = ps[g_sp->yvel].palookup;
        else g_sp->pal = hittype[g_i].tempang;
        hittype[g_i].tempang = 0;
        break;

    case CON_TOSSWEAPON:
        insptr++;
        checkweapons(&ps[g_sp->yvel]);
        break;

    case CON_NULLOP:
        insptr++;
        break;

    case CON_MIKESND:
        insptr++;
        if (!isspritemakingsound(g_i,g_sp->yvel))
            spritesound(g_sp->yvel,g_i);
        break;

    case CON_PKICK:
        insptr++;

        if (ud.multimode > 1 && g_sp->picnum == APLAYER)
        {
            if (ps[otherp].quick_kick == 0)
                ps[otherp].quick_kick = 14;
        }
        else if (g_sp->picnum != APLAYER && ps[g_p].quick_kick == 0)
            ps[g_p].quick_kick = 14;
        break;

    case CON_SIZETO:
        insptr++;

        j = (*insptr++-g_sp->xrepeat)<<1;
        g_sp->xrepeat += ksgn(j);

        if ((g_sp->picnum == APLAYER && g_sp->yrepeat < 36) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat*(tilesizy[g_sp->picnum]+8))<<2) < (hittype[g_i].floorz - hittype[g_i].ceilingz))
        {
            j = ((*insptr)-g_sp->yrepeat)<<1;
            if (klabs(j)) g_sp->yrepeat += ksgn(j);
        }
        insptr++;

        break;

    case CON_SIZEAT:
        insptr++;
        g_sp->xrepeat = (char) *insptr++;
        g_sp->yrepeat = (char) *insptr++;
        break;

    case CON_SHOOT:
        insptr++;
        shoot(g_i,(short)*insptr++);
        break;

    case CON_SOUNDONCE:
        insptr++;
        if (!isspritemakingsound(g_i,*insptr))
            spritesound((short) *insptr,g_i);
        insptr++;
        break;

    case CON_IFSOUND:
        insptr++;
        parseifelse(isspritemakingsound(g_i,*insptr));
        //    parseifelse(SoundOwner[*insptr][0].i == g_i);
        break;

    case CON_STOPSOUND:
        insptr++;
        if (isspritemakingsound(g_i,*insptr))
            stopspritesound((short)*insptr,g_i);
        insptr++;
        break;

    case CON_GLOBALSOUND:
        insptr++;
        if (g_p == screenpeek || (gametype_flags[ud.coop]&GAMETYPE_FLAG_COOPSOUND))
            spritesound((short) *insptr,ps[screenpeek].i);
        insptr++;
        break;

    case CON_SOUND:
        insptr++;
        spritesound((short) *insptr++,g_i);
        break;

    case CON_TIP:
        insptr++;
        ps[g_p].tipincs = 26;
        break;

    case CON_FALL:
        insptr++;
        g_sp->xoffset = 0;
        g_sp->yoffset = 0;
        //            if(!gotz)
        {
            j = gc;

            if (ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
                j = gc/6;
            else if (floorspace(g_sp->sectnum))
                j = 0;

            if (hittype[g_i].cgg <= 0 || (sector[g_sp->sectnum].floorstat&2))
            {
                getglobalz(g_i);
                hittype[g_i].cgg = 6;
            }
            else hittype[g_i].cgg --;

            if (g_sp->z < (hittype[g_i].floorz-FOURSLEIGHT))
            {
                g_sp->zvel += j;
                g_sp->z+=g_sp->zvel;

                if (g_sp->zvel > 6144) g_sp->zvel = 6144;
            }
            else
            {
                g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

                if (badguy(g_sp) || (g_sp->picnum == APLAYER && g_sp->owner >= 0))
                {
                    if (g_sp->zvel > 3084 && g_sp->extra <= 1)
                    {
                        if (g_sp->pal != 1 && g_sp->picnum != DRONE)
                        {
                            if (!(g_sp->picnum == APLAYER && g_sp->extra > 0))
                            {
                                guts(g_i,JIBS6,15,g_p);
                                spritesound(SQUISHED,g_i);
                                spawn(g_i,BLOODPOOL);
                            }
                        }

                        hittype[g_i].picnum = SHOTSPARK1;
                        hittype[g_i].extra = 1;
                        g_sp->zvel = 0;
                    }
                    else if (g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != 1)
                    {
                        j = g_sp->sectnum;
                        pushmove(&g_sp->x,&g_sp->y,&g_sp->z,(short*)&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        if (j != g_sp->sectnum && j >= 0 && j < MAXSECTORS)
                            changespritesect(g_i,j);

                        spritesound(THUD,g_i);
                    }
                }
                if (sector[g_sp->sectnum].lotag == 1)
                    switch (dynamictostatic[g_sp->picnum])
                    {
                    case OCTABRAIN__STATIC:
                    case COMMANDER__STATIC:
                    case DRONE__STATIC:
                        break;
                    default:
                        g_sp->z += (24<<8);
                        break;
                    }
                else g_sp->zvel = 0;
            }

            if (g_sp->z > (hittype[g_i].floorz-FOURSLEIGHT))
                g_sp->z = (hittype[g_i].floorz - FOURSLEIGHT);
        }

        break;

    case CON_ENDA:
    case CON_BREAK:
    case CON_ENDS:
        return 1;
    case CON_RIGHTBRACE:
        insptr++;
        return 1;
    case CON_ADDAMMO:
        insptr++;
        if (ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr])
        {
            killit_flag = 2;
            break;
        }
        addammo(*insptr, &ps[g_p], *(insptr+1));
        if (ps[g_p].curr_weapon == KNEE_WEAPON)
            if (ps[g_p].gotweapon[*insptr])
            {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
                else addweapon(&ps[g_p], *insptr);
            }
        insptr += 2;
        break;

    case CON_MONEY:
        insptr++;
        lotsofmoney(g_i,*insptr++);
        break;

    case CON_MAIL:
        insptr++;
        lotsofmail(g_i,*insptr++);
        break;

    case CON_SLEEPTIME:
        insptr++;
        hittype[g_i].timetosleep = (short)*insptr++;
        break;

    case CON_PAPER:
        insptr++;
        lotsofpaper(g_i,*insptr++);
        break;

    case CON_ADDKILLS:
        insptr++;
        ps[g_p].actors_killed += *insptr++;
        hittype[g_i].actorstayput = -1;
        break;

    case CON_LOTSOFGLASS:
        insptr++;
        spriteglass(g_i,*insptr++);
        break;

    case CON_KILLIT:
        insptr++;
        killit_flag = 1;
        break;

    case CON_ADDWEAPON:
        insptr++;
        if (ps[g_p].gotweapon[*insptr] == 0)
        {
            if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
            else addweapon(&ps[g_p], *insptr);
        }
        else if (ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr])
        {
            killit_flag = 2;
            break;
        }
        addammo(*insptr, &ps[g_p], *(insptr+1));
        if (ps[g_p].curr_weapon == KNEE_WEAPON)
            if (ps[g_p].gotweapon[*insptr])
            {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
                else addweapon(&ps[g_p], *insptr);
            }
        insptr+=2;
        break;

    case CON_DEBUG:
        insptr++;
        initprintf("%ld\n",*insptr++);
        break;

    case CON_ENDOFGAME:
        insptr++;
        ps[g_p].timebeforeexit = *insptr++;
        ps[g_p].customexitsound = -1;
        ud.eog = 1;
        break;

    case CON_ADDPHEALTH:
        insptr++;

        if (ps[g_p].newowner >= 0)
        {
            ps[g_p].newowner = -1;
            ps[g_p].posx = ps[g_p].oposx;
            ps[g_p].posy = ps[g_p].oposy;
            ps[g_p].posz = ps[g_p].oposz;
            ps[g_p].ang = ps[g_p].oang;
            updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
            setpal(&ps[g_p]);

            j = headspritestat[1];
            while (j >= 0)
            {
                if (sprite[j].picnum==CAMERA1)
                    sprite[j].yvel = 0;
                j = nextspritestat[j];
            }
        }

        j = sprite[ps[g_p].i].extra;

        if (g_sp->picnum != ATOMICHEALTH)
        {
            if (j > max_player_health && *insptr > 0)
            {
                insptr++;
                break;
            }
            else
            {
                if (j > 0)
                    j += *insptr;
                if (j > max_player_health && *insptr > 0)
                    j = max_player_health;
            }
        }
        else
        {
            if (j > 0)
                j += *insptr;
            if (j > (max_player_health<<1))
                j = (max_player_health<<1);
        }

        if (j < 0) j = 0;

        if (ud.god == 0)
        {
            if (*insptr > 0)
            {
                if ((j - *insptr) < (max_player_health>>2) &&
                        j >= (max_player_health>>2))
                    spritesound(DUKE_GOTHEALTHATLOW,ps[g_p].i);

                ps[g_p].last_extra = j;
            }

            sprite[ps[g_p].i].extra = j;
        }

        insptr++;
        break;

    case CON_STATE:
    {
        long *tempscrptr=insptr+2;

        insptr = (long *) *(insptr+1);
        while (1) if (parse()) break;
        insptr = tempscrptr;
    }
    break;

    case CON_LEFTBRACE:
        insptr++;
        while (1) if (parse()) break;
        break;

    case CON_MOVE:
        insptr++;
        g_t[0]=0;
        g_t[1] = *insptr++;
        g_sp->hitag = *insptr++;
        if (g_sp->hitag&random_angle)
            g_sp->ang = TRAND&2047;
        break;

    case CON_ADDWEAPONVAR:
        insptr++;
        if (ps[g_p].gotweapon[GetGameVarID(*(insptr),g_i,g_p)] == 0)
        {
            if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
            else addweapon(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
        }
        else if (ps[g_p].ammo_amount[GetGameVarID(*(insptr),g_i,g_p)] >= max_ammo_amount[GetGameVarID(*(insptr),g_i,g_p)])
        {
            killit_flag = 2;
            break;
        }
        addammo(GetGameVarID(*(insptr),g_i,g_p), &ps[g_p], GetGameVarID(*(insptr+1),g_i,g_p));
        if (ps[g_p].curr_weapon == KNEE_WEAPON)
            if (ps[g_p].gotweapon[GetGameVarID(*(insptr),g_i,g_p)])
            {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
                else addweapon(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
            }
        insptr+=2;
        break;

    case CON_ACTIVATEBYSECTOR:
    case CON_OPERATESECTORS:
    case CON_OPERATEACTIVATORS:
    case CON_SETASPECT:
    case CON_SSP:
        insptr++;
        {
            int var1 = GetGameVarID(*insptr++,g_i,g_p), var2;
            if (tw == CON_OPERATEACTIVATORS && *insptr == g_iThisActorID)
            {
                var2 = g_p;
                insptr++;
            }
            else var2 = GetGameVarID(*insptr++,g_i,g_p);

            switch (tw)
            {
            case CON_ACTIVATEBYSECTOR:
                activatebysector(var1, var2);
                break;
            case CON_OPERATESECTORS:
                operatesectors(var1, var2);
                break;
            case CON_OPERATEACTIVATORS:
                operateactivators(var1, var2);
                break;
            case CON_SETASPECT:
                setaspect(var1, var2);
                break;
            case CON_SSP:
                ssp(var1, var2);
                break;
            }
            break;
        }

    case CON_CANSEESPR:
        insptr++;
        {
            int lVar1 = GetGameVarID(*insptr++,g_i,g_p), lVar2 = GetGameVarID(*insptr++,g_i,g_p);

            SetGameVarID(*insptr++, cansee(sprite[lVar1].x,sprite[lVar1].y,sprite[lVar1].z,sprite[lVar1].sectnum,
                                           sprite[lVar2].x,sprite[lVar2].y,sprite[lVar2].z,sprite[lVar2].sectnum), g_i, g_p);
            break;
        }

    case CON_OPERATERESPAWNS:
    case CON_OPERATEMASTERSWITCHES:
    case CON_CHECKACTIVATORMOTION:
        insptr++;
        {
            int var1 = GetGameVarID(*insptr++,g_i,g_p);

            switch (tw)
            {
            case CON_OPERATERESPAWNS:
                operaterespawns(var1);
                break;
            case CON_OPERATEMASTERSWITCHES:
                operatemasterswitches(var1);
                break;
            case CON_CHECKACTIVATORMOTION:
                SetGameVarID(g_iReturnVarID, check_activator_motion(var1), g_i, g_p);
                break;
            }
            break;
        }

    case CON_INSERTSPRITEQ:
        insptr++;
        insertspriteq(g_i);
        break;

    case CON_QSTRLEN:
        insptr++;
        {
            int i=*insptr++;
            j=GetGameVarID(*insptr++, g_i, g_p);
            if (fta_quotes[j] == NULL)
            {
                SetGameVarID(i,-1,g_i,g_p);
                break;
            }
            SetGameVarID(i,Bstrlen(fta_quotes[j]),g_i,g_p);
            break;
        }

    case CON_GETPNAME:
    case CON_QSTRCAT:
    case CON_QSTRCPY:
    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
        insptr++;
        {
            int i = GetGameVarID(*insptr++, g_i, g_p), j;
            if (tw == CON_GETPNAME && *insptr == g_iThisActorID)
            {
                j = g_p;
                insptr++;
            }
            else j = GetGameVarID(*insptr++, g_i, g_p);

            switch (tw)
            {
            case CON_GETPNAME:
                if (fta_quotes[i] != NULL)
                {
                    if (ud.user_name[j][0])
                        Bstrcpy(fta_quotes[i],ud.user_name[j]);
                    else Bsprintf(fta_quotes[i],"%d",j);
                }
                else OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,i);
                break;
            case CON_QSTRCAT:
                if (fta_quotes[i] != NULL && fta_quotes[j] != NULL)
                    Bstrncat(fta_quotes[i],fta_quotes[j],(MAXQUOTELEN-1)-Bstrlen(fta_quotes[i]));
                else OSD_Printf("%s %d null quote %d %d\n",__FILE__,__LINE__,i,j);
                break;
            case CON_QSTRCPY:
                if (fta_quotes[i] != NULL && fta_quotes[j] != NULL)
                    Bstrcpy(fta_quotes[i],fta_quotes[j]);
                else OSD_Printf("%s %d null quote %d %d\n",__FILE__,__LINE__,i,j);
                break;
            case CON_CHANGESPRITESTAT:
                changespritestat(i,j);
                break;
            case CON_CHANGESPRITESECT:
                changespritesect(i,j);
                break;
            }
            break;
        }

    case CON_STARTLEVEL:
        insptr++; // skip command
        {
            // from 'level' cheat in game.c (about line 6250)
            int volnume=GetGameVarID(*insptr++,g_i,g_p), levnume=GetGameVarID(*insptr++,g_i,g_p);

            if (volnume > MAXVOLUMES-1 || volnume < 0)
            {
                OSD_Printf("parse():CON_STARTLEVEL: invalid volume (%d)\n",volnume);
                break;
            }

            if (levnume > MAXLEVELS-1 || levnume < 0)
            {
                OSD_Printf("parse():CON_STARTLEVEL: invalid level (%d)\n",levnume);
                break;
            }

            ud.m_volume_number = ud.volume_number = volnume;
            ud.m_level_number = ud.level_number = levnume;
            if (numplayers > 1 && myconnectindex == connecthead)
                mpchangemap(volnume,levnume);
            else
            {
                ps[myconnectindex].gm |= MODE_EOL;
                display_bonus_screen = 0;
            } // MODE_RESTART;

            break;
        }

    case CON_MYOSX:
    case CON_MYOSPALX:
    case CON_MYOS:
    case CON_MYOSPAL:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), tilenum=GetGameVarID(*insptr++,g_i,g_p);
            int shade=GetGameVarID(*insptr++,g_i,g_p), orientation=GetGameVarID(*insptr++,g_i,g_p);

            switch (tw)
            {
            case CON_MYOS:
                myos(x,y,tilenum,shade,orientation);
                break;
            case CON_MYOSPAL:
            {
                int pal=GetGameVarID(*insptr++,g_i,g_p);
                myospal(x,y,tilenum,shade,orientation,pal);
                break;
            }
            case CON_MYOSX:
                myosx(x,y,tilenum,shade,orientation);
                break;
            case CON_MYOSPALX:
            {
                int pal=GetGameVarID(*insptr++,g_i,g_p);
                myospalx(x,y,tilenum,shade,orientation,pal);
                break;
            }
            }
            break;
        }

    case CON_SWITCH:
        insptr++; // p-code
        {
            // command format:
            // variable ID to check
            // script offset to 'end'
            // count of case statements
            // script offset to default case (null if none)
            // For each case: value, ptr to code
            //AddLog("Processing Switch...");
            long lVarID=*insptr++, lValue=GetGameVarID(lVarID, g_i, g_p), lEnd=*insptr++, lCases=*insptr++;
            long *lpDefault=insptr++, *lpCases=insptr, bMatched=0, *lTempInsPtr, lCheckCase;
            insptr+=lCases*2;
            lTempInsPtr=insptr;
            //Bsprintf(g_szBuf,"lEnd= %ld *lpDefault=%ld",lEnd,*lpDefault);
            //AddLog(g_szBuf);

            //Bsprintf(g_szBuf,"Checking %ld cases for %ld",lCases, lValue);
            //AddLog(g_szBuf);
            for (lCheckCase=0; lCheckCase<lCases && !bMatched; lCheckCase++)
            {
                //Bsprintf(g_szBuf,"Checking #%ld Value= %ld",lCheckCase, lpCases[lCheckCase*2]);
                //AddLog(g_szBuf);

                if (lpCases[lCheckCase*2] == lValue)
                {
                    //AddLog("Found Case Match");
                    //Bsprintf(g_szBuf,"insptr=%ld. lCheckCase=%ld, offset=%ld, &script[0]=%ld",
                    //            (long)insptr,(long)lCheckCase,lpCases[lCheckCase*2+1],(long)&script[0]);
                    //AddLog(g_szBuf);
                    // fake a 2-d Array
                    insptr=(long*)(lpCases[lCheckCase*2+1] + &script[0]);
                    //Bsprintf(g_szBuf,"insptr=%ld. ",     (long)insptr);
                    //AddLog(g_szBuf);
                    while (1)
                    {
                        if (parse())
                            break;
                    }
                    //AddLog("Done Executing Case");
                    bMatched=1;
                }
            }
            if (!bMatched)
            {
                if (*lpDefault)
                {
                    //AddLog("No Matching Case: Using Default");
                    insptr=(long*)(*lpDefault + &script[0]);
                    while (1) if (parse()) break;
                }
                else
                {
                    //AddLog("No Matching Case: No Default to use");
                }
            }
            insptr=(long*)(lEnd + (long)&script[0]);
            //Bsprintf(g_szBuf,"insptr=%ld. ",     (long)insptr);
            //AddLog(g_szBuf);
            //AddLog("Done Processing Switch");

            break;
        }

    case CON_ENDSWITCH:
    case CON_ENDEVENT:
        insptr++;
        return 1;
        break;

    case CON_DISPLAYRAND:
        insptr++;
        SetGameVarID(*insptr++, rand(), g_i, g_p);
        break;

    case CON_DRAGPOINT:
        insptr++;
        {
            int wallnum = GetGameVarID(*insptr++, g_i, g_p), newx = GetGameVarID(*insptr++, g_i, g_p), newy = GetGameVarID(*insptr++, g_i, g_p);

            dragpoint(wallnum,newx,newy);
            break;
        }

    case CON_DIST:
    case CON_LDIST:
        insptr++;
        {
            int distvar = *insptr++, xvar = *insptr++, yvar = *insptr++, distx=0;

            switch (tw)
            {
            case CON_DIST:
                distx = dist(&sprite[GetGameVarID(xvar, g_i, g_p)],&sprite[GetGameVarID(yvar, g_i, g_p)]);
                break;
            case CON_LDIST:
                distx = ldist(&sprite[GetGameVarID(xvar, g_i, g_p)],&sprite[GetGameVarID(yvar, g_i, g_p)]);
                break;
            }

            SetGameVarID(distvar, distx, g_i, g_p);
            break;
        }

    case CON_GETINCANGLE:
    case CON_GETANGLE:
        insptr++;
        {
            int angvar = *insptr++, xvar = *insptr++, yvar = *insptr++;

            if (tw==CON_GETANGLE)
            {
                SetGameVarID(angvar, getangle(GetGameVarID(xvar, g_i, g_p),GetGameVarID(yvar, g_i, g_p)), g_i, g_p);
                break;
            }
            SetGameVarID(angvar, getincangle(GetGameVarID(xvar, g_i, g_p),GetGameVarID(yvar, g_i, g_p)), g_i, g_p);
            break;
        }

    case CON_MULSCALE:
        insptr++;
        {
            int var1 = *insptr++, var2 = GetGameVarID(*insptr++, g_i, g_p);
            int var3 = GetGameVarID(*insptr++, g_i, g_p), var4 = GetGameVarID(*insptr++, g_i, g_p);

            SetGameVarID(var1, mulscale(var2, var3, var4), g_i, g_p);
            break;
        }

    case CON_INITTIMER:
        insptr++;
        j = GetGameVarID(*insptr++, g_i, g_p);
        if (timer != j)
        {
            uninittimer();
            inittimer(j);
            timer = j;
        }
        break;

    case CON_TIME:
        insptr += 2;
        break;

    case CON_ESPAWNVAR:
    case CON_EQSPAWNVAR:
    case CON_QSPAWNVAR:
        insptr++;
        {
            int lIn=GetGameVarID(*insptr++, g_i, g_p);
            j = -1;
            if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                j = spawn(g_i, lIn);
            switch (tw)
            {
            case CON_EQSPAWNVAR:
                if (j != -1)
                    insertspriteq(j);
            case CON_ESPAWNVAR:
                SetGameVarID(g_iReturnVarID, j, g_i, g_p);
                break;
            case CON_QSPAWNVAR:
                if (j != -1)
                    insertspriteq(j);
                break;
            }
            break;
        }

    case CON_ESPAWN:
    case CON_EQSPAWN:
    case CON_QSPAWN:
        insptr++;

        j=-1;

        if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
            j = spawn(g_i,*insptr++);
        else insptr++;

        switch (tw)
        {
        case CON_EQSPAWN:
            if (j != -1)
                insertspriteq(j);
        case CON_ESPAWN:
            SetGameVarID(g_iReturnVarID, j, g_i, g_p);
            break;
        case CON_QSPAWN:
            if (j != -1)
                insertspriteq(j);
            break;
        }
        break;

    case CON_ESHOOT:
    case CON_EZSHOOT:
    case CON_ZSHOOT:
        insptr++;

        if (tw == CON_ZSHOOT || tw == CON_EZSHOOT)
        {
            hittype[g_i].temp_data[9] = GetGameVarID(*insptr++, g_i, g_p);
            if (hittype[g_i].temp_data[9] == 0)
                hittype[g_i].temp_data[9] = 1;
        }
        if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
        {
            if (tw == CON_EZSHOOT || tw == CON_ESHOOT)
                SetGameVarID(g_iReturnVarID, shoot(g_i,*insptr++), g_i, g_p);
            else shoot(g_i,*insptr++);
        }
        else insptr++;

        hittype[g_i].temp_data[9]=0;
        break;

    case CON_SHOOTVAR:
    case CON_ESHOOTVAR:
    case CON_EZSHOOTVAR:
    case CON_ZSHOOTVAR:
    {
        long lReturn=-1;

        insptr++;

        if (tw == CON_ZSHOOTVAR || tw == CON_EZSHOOTVAR)
        {
            hittype[g_i].temp_data[9] = GetGameVarID(*insptr++, g_i, g_p);
            if (hittype[g_i].temp_data[9] == 0)
                hittype[g_i].temp_data[9] = 1;
        }
        j=GetGameVarID(*insptr++, g_i, g_p);
        if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
        {
            lReturn = shoot(g_i, j);
            if (tw == CON_ESHOOTVAR || tw == CON_EZSHOOTVAR)
                SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
        }
        hittype[g_i].temp_data[9]=0;
        break;
    }

    case CON_SOUNDVAR:
    case CON_STOPSOUNDVAR:
    case CON_SOUNDONCEVAR:
    case CON_GLOBALSOUNDVAR:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);

        switch (tw)
        {
        case CON_SOUNDONCEVAR:
            if (!isspritemakingsound(g_i,j))
                spritesound((short)j,g_i);
            break;
        case CON_GLOBALSOUNDVAR:
            spritesound((short)j,ps[screenpeek].i);
            break;
        case CON_STOPSOUNDVAR:
            if (isspritemakingsound(g_i,j))
                stopspritesound((short)j,g_i);
            break;
        case CON_SOUNDVAR:
            spritesound((short)j,g_i);
            break;
        }
        break;

    case CON_GUNIQHUDID:
        insptr++;
        {
            j=GetGameVarID(*insptr++, g_i, g_p);
            if (j < MAXUNIQHUDID-1)
                guniqhudid = j;
            break;
        }

    case CON_SAVEGAMEVAR:
    case CON_READGAMEVAR:
    {
        int32 i=0;
        insptr++;
        if (scripthandle < 0) break;
        switch (tw)
        {
        case CON_SAVEGAMEVAR:
            i=GetGameVarID(*insptr, g_i, g_p);
            SCRIPT_PutNumber(scripthandle, "Gamevars",aGameVars[*insptr++].szLabel,i,false,false);
            break;
        case CON_READGAMEVAR:
            SCRIPT_GetNumber(scripthandle, "Gamevars",aGameVars[*insptr].szLabel,&i);
            SetGameVarID(*insptr++, i, g_i, g_p);
            break;
        }
        break;
    }

    case CON_SHOWVIEW:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p);
            int y=GetGameVarID(*insptr++,g_i,g_p);
            int z=GetGameVarID(*insptr++,g_i,g_p);
            int a=GetGameVarID(*insptr++,g_i,g_p);
            int horiz=GetGameVarID(*insptr++,g_i,g_p);
            int sect=GetGameVarID(*insptr++,g_i,g_p);
            long x1=scale(GetGameVarID(*insptr++,g_i,g_p),xdim,320);
            long y1=scale(GetGameVarID(*insptr++,g_i,g_p),ydim,200);
            long x2=scale(GetGameVarID(*insptr++,g_i,g_p),xdim,320);
            long y2=scale(GetGameVarID(*insptr++,g_i,g_p),ydim,200);
            long smoothratio = 65536;

            if (x1 > x2) swaplong(&x1,&x2);
            if (y1 > y2) swaplong(&y1,&y2);

            if (x1 < 0 || y1 < 0 || x2 > xdim-1 || y2 > ydim-1 || x2-x1 < 2 || y2-y1 < 2)
                break;

#if defined(USE_OPENGL) && defined(POLYMOST)
            j = glprojectionhacks;
            glprojectionhacks = 0;
#endif
            setview(x1,y1,x2,y2);

#if 0
            if (!ud.pause_on && ((ud.show_help == 0 && ud.multimode < 2 && !(ps[myconnectindex].gm&MODE_MENU)) || ud.multimode > 1 || ud.recstat == 2))
                smoothratio = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
#endif
            dointerpolations(smoothratio);

#define SE40

#ifdef SE40
            se40code(x,y,z,a,horiz,smoothratio);
#endif

            if ((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
            {
                long j, i = 0, k, dst = 0x7fffffff;

                for (k=0;k<mirrorcnt;k++)
                {
                    j = klabs(wall[mirrorwall[k]].x-x);
                    j += klabs(wall[mirrorwall[k]].y-y);
                    if (j < dst) dst = j, i = k;
                }

                if (wall[mirrorwall[i]].overpicnum == MIRROR)
                {
                    long tposx,tposy;
                    short tang;

                    preparemirror(x,y,z,a,horiz,mirrorwall[i],mirrorsector[i],&tposx,&tposy,&tang);

                    j = visibility;
                    visibility = (j>>1) + (j>>2);

                    drawrooms(tposx,tposy,z,tang,horiz,mirrorsector[i]+MAXSECTORS);

                    display_mirror = 1;
                    animatesprites(tposx,tposy,tang,smoothratio);
                    display_mirror = 0;

                    drawmasks();
                    completemirror();   //Reverse screen x-wise in this function
                    visibility = j;
                }
                gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
            }

            drawrooms(x,y,z,a,horiz,sect);
            display_mirror = 2;
            animatesprites(x,y,a,smoothratio);
            display_mirror = 0;
            drawmasks();
            restoreinterpolations();
            vscrn();
#if defined(USE_OPENGL) && defined(POLYMOST)
            glprojectionhacks = j;
#endif
            break;
        }

    case CON_ROTATESPRITE:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p),   y=GetGameVarID(*insptr++,g_i,g_p),           z=GetGameVarID(*insptr++,g_i,g_p);
            int a=GetGameVarID(*insptr++,g_i,g_p),   tilenum=GetGameVarID(*insptr++,g_i,g_p),     shade=GetGameVarID(*insptr++,g_i,g_p);
            int pal=GetGameVarID(*insptr++,g_i,g_p), orientation=GetGameVarID(*insptr++,g_i,g_p);
            long x1=GetGameVarID(*insptr++,g_i,g_p),  y1=GetGameVarID(*insptr++,g_i,g_p);
            long x2=GetGameVarID(*insptr++,g_i,g_p),  y2=GetGameVarID(*insptr++,g_i,g_p);

            rotatesprite(x<<16,y<<16,z,a,tilenum,shade,pal,2|orientation,x1,y1,x2,y2);
            break;
        }

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_DIGITALNUMBER:
        insptr++;
        {
            int tilenum = (tw == CON_GAMETEXT || tw == CON_DIGITALNUMBER)?GetGameVarID(*insptr++,g_i,g_p):0;
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), q=GetGameVarID(*insptr++,g_i,g_p);
            int shade=GetGameVarID(*insptr++,g_i,g_p), pal=GetGameVarID(*insptr++,g_i,g_p);

            if (tw == CON_GAMETEXT || tw == CON_DIGITALNUMBER)
            {
                int orientation=GetGameVarID(*insptr++,g_i,g_p);
                int x1=GetGameVarID(*insptr++,g_i,g_p), y1=GetGameVarID(*insptr++,g_i,g_p);
                int x2=GetGameVarID(*insptr++,g_i,g_p), y2=GetGameVarID(*insptr++,g_i,g_p);

                if (tw == CON_GAMETEXT)
                {
                    if (fta_quotes[q] == NULL)
                    {
                        OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,q);
                        break;
                    }
                    gametext_(0,tilenum,x>>1,y,fta_quotes[q],shade,pal,orientation,x1,y1,x2,y2);
                    break;
                }
                txdigitalnumber(tilenum,x,y,q,shade,pal,orientation,x1,y1,x2,y2);
                break;
            }

            if (fta_quotes[q] == NULL)
            {
                OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,q);
                break;
            }
            minitextshade(x,y,fta_quotes[q],shade,pal,26);
            break;
        }

    case CON_ANGOFF:
        insptr++;
        spriteext[g_i].angoff=*insptr++;
        break;

    case CON_GETZRANGE:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), z=GetGameVarID(*insptr++,g_i,g_p);
            int sectnum=GetGameVarID(*insptr++,g_i,g_p);
            int ceilzvar=*insptr++, ceilhitvar=*insptr++, florzvar=*insptr++, florhitvar=*insptr++;
            int walldist=GetGameVarID(*insptr++,g_i,g_p), clipmask=GetGameVarID(*insptr++,g_i,g_p);
            long ceilz, ceilhit, florz, florhit;

            getzrange(x, y, z, sectnum, &ceilz, &ceilhit, &florz, &florhit, walldist, clipmask);
            SetGameVarID(ceilzvar, ceilz, g_i, g_p);
            SetGameVarID(ceilhitvar, ceilhit, g_i, g_p);
            SetGameVarID(florzvar, florz, g_i, g_p);
            SetGameVarID(florhitvar, florhit, g_i, g_p);
            break;
        }

    case CON_HITSCAN:
        insptr++;
        {
            int xs=GetGameVarID(*insptr++,g_i,g_p), ys=GetGameVarID(*insptr++,g_i,g_p), zs=GetGameVarID(*insptr++,g_i,g_p);
            int sectnum=GetGameVarID(*insptr++,g_i,g_p);
            int vx=GetGameVarID(*insptr++,g_i,g_p), vy=GetGameVarID(*insptr++,g_i,g_p), vz=GetGameVarID(*insptr++,g_i,g_p);
            int hitsectvar=*insptr++, hitwallvar=*insptr++, hitspritevar=*insptr++;
            int hitxvar=*insptr++, hityvar=*insptr++, hitzvar=*insptr++, cliptype=GetGameVarID(*insptr++,g_i,g_p);
            short hitsect, hitwall, hitsprite;
            long hitx, hity, hitz;

            hitscan(xs, ys, zs, sectnum, vx, vy, vz, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, cliptype);
            SetGameVarID(hitsectvar, hitsect, g_i, g_p);
            SetGameVarID(hitwallvar, hitwall, g_i, g_p);
            SetGameVarID(hitspritevar, hitsprite, g_i, g_p);
            SetGameVarID(hitxvar, hitx, g_i, g_p);
            SetGameVarID(hityvar, hity, g_i, g_p);
            SetGameVarID(hitzvar, hitz, g_i, g_p);
            break;
        }

    case CON_CANSEE:
        insptr++;
        {
            int x1=GetGameVarID(*insptr++,g_i,g_p), y1=GetGameVarID(*insptr++,g_i,g_p), z1=GetGameVarID(*insptr++,g_i,g_p);
            int sect1=GetGameVarID(*insptr++,g_i,g_p);
            int x2=GetGameVarID(*insptr++,g_i,g_p), y2=GetGameVarID(*insptr++,g_i,g_p), z2=GetGameVarID(*insptr++,g_i,g_p);
            int sect2=GetGameVarID(*insptr++,g_i,g_p), rvar=*insptr++;

            SetGameVarID(rvar, cansee(x1,y1,z1,sect1,x2,y2,z2,sect2), g_i, g_p);
            break;
        }

    case CON_ROTATEPOINT:
        insptr++;
        {
            int xpivot=GetGameVarID(*insptr++,g_i,g_p), ypivot=GetGameVarID(*insptr++,g_i,g_p);
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), daang=GetGameVarID(*insptr++,g_i,g_p);
            int x2var=*insptr++, y2var=*insptr++;
            long x2, y2;

            rotatepoint(xpivot,ypivot,x,y,daang,&x2,&y2);
            SetGameVarID(x2var, x2, g_i, g_p);
            SetGameVarID(y2var, y2, g_i, g_p);
            break;
        }

    case CON_NEARTAG:
        insptr++;
        {
            //             neartag(long x, long y, long z, short sectnum, short ang,  //Starting position & angle
            //                     short *neartagsector,   //Returns near sector if sector[].tag != 0
            //                     short *neartagwall,     //Returns near wall if wall[].tag != 0
            //                     short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
            //                     long *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
            //                     long neartagrange,      //Choose maximum distance to scan (scale: 1024=largest grid size)
            //                     char tagsearch)         //1-lotag only, 2-hitag only, 3-lotag&hitag

            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), z=GetGameVarID(*insptr++,g_i,g_p);
            int sectnum=GetGameVarID(*insptr++,g_i,g_p), ang=GetGameVarID(*insptr++,g_i,g_p);
            int neartagsectorvar=*insptr++, neartagwallvar=*insptr++, neartagspritevar=*insptr++, neartaghitdistvar=*insptr++;
            int neartagrange=GetGameVarID(*insptr++,g_i,g_p), tagsearch=GetGameVarID(*insptr++,g_i,g_p);

            neartag(x, y, z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, neartagrange, tagsearch);

            SetGameVarID(neartagsectorvar, neartagsector, g_i, g_p);
            SetGameVarID(neartagwallvar, neartagwall, g_i, g_p);
            SetGameVarID(neartagspritevar, neartagsprite, g_i, g_p);
            SetGameVarID(neartaghitdistvar, neartaghitdist, g_i, g_p);
            break;
        }

    case CON_MOVESPRITE:
    case CON_SETSPRITE:
        insptr++;
        {
            int spritenum = GetGameVarID(*insptr++,g_i,g_p);
            int x = GetGameVarID(*insptr++,g_i,g_p), y = GetGameVarID(*insptr++,g_i,g_p), z = GetGameVarID(*insptr++,g_i,g_p);

            if (tw == CON_SETSPRITE)
            {
                setsprite(spritenum, x, y, z);
                break;
            }

            {
                int cliptype = GetGameVarID(*insptr++,g_i,g_p);

                SetGameVarID(*insptr++, movesprite(spritenum, x, y, z, cliptype), g_i, g_p);
                break;
            }
        }

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        insptr++;
        {
            int sectnum = GetGameVarID(*insptr++,g_i,g_p), x = GetGameVarID(*insptr++,g_i,g_p), y = GetGameVarID(*insptr++,g_i,g_p);

            if (tw == CON_GETFLORZOFSLOPE)
            {
                SetGameVarID(*insptr++, getflorzofslope(sectnum,x,y), g_i, g_p);
                break;
            }
            SetGameVarID(*insptr++, getceilzofslope(sectnum,x,y), g_i, g_p);
            break;
        }

    case CON_UPDATESECTOR:
    case CON_UPDATESECTORZ:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p);
            int z=(tw==CON_UPDATESECTORZ)?GetGameVarID(*insptr++,g_i,g_p):0;
            int var=*insptr++;
            short w=sprite[g_i].sectnum;

            if (tw==CON_UPDATESECTOR) updatesector(x,y,&w);
            else if (tw==CON_UPDATESECTORZ) updatesectorz(x,y,z,&w);

            SetGameVarID(var, w, g_i, g_p);
            break;
        }

    case CON_SPAWN:
        insptr++;
        if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
            spawn(g_i,*insptr);
        insptr++;
        break;

    case CON_IFWASWEAPON:
        insptr++;
        parseifelse(hittype[g_i].picnum == *insptr);
        break;

    case CON_IFAI:
        insptr++;
        parseifelse(g_t[5] == *insptr);
        break;

    case CON_IFACTION:
        insptr++;
        parseifelse(g_t[4] == *insptr);
        break;

    case CON_IFACTIONCOUNT:
        insptr++;
        parseifelse(g_t[2] >= *insptr);
        break;

    case CON_RESETACTIONCOUNT:
        insptr++;
        g_t[2] = 0;
        break;

    case CON_DEBRIS:
        insptr++;
        {
            int dnum = *insptr++;

            if (g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                for (j=(*insptr)-1;j>=0;j--)
                {
                    if (g_sp->picnum == BLIMP && dnum == SCRAP1)
                        s = 0;
                    else s = (TRAND%3);

                    l = EGS(g_sp->sectnum,
                            g_sp->x+(TRAND&255)-128,g_sp->y+(TRAND&255)-128,g_sp->z-(8<<8)-(TRAND&8191),
                            dnum+s,g_sp->shade,32+(TRAND&15),32+(TRAND&15),
                            TRAND&2047,(TRAND&127)+32,
                            -(TRAND&2047),g_i,5);
                    if (g_sp->picnum == BLIMP && dnum == SCRAP1)
                        sprite[l].yvel = weaponsandammosprites[j%14];
                    else sprite[l].yvel = -1;
                    sprite[l].pal = g_sp->pal;
                }
            insptr++;
        }
        break;

    case CON_COUNT:
        insptr++;
        g_t[0] = (short) *insptr++;
        break;

    case CON_CSTATOR:
        insptr++;
        g_sp->cstat |= (short) *insptr++;
        break;

    case CON_CLIPDIST:
        insptr++;
        g_sp->clipdist = (short) *insptr++;
        break;

    case CON_CSTAT:
        insptr++;
        g_sp->cstat = (short) *insptr++;
        break;

    case CON_SAVE:
        insptr++;
        {
            time_t curtime;

            lastsavedpos = *insptr++;

            if ((movesperpacket == 4 && connecthead != myconnectindex) || lastsavedpos > 9)
                break;

            curtime = time(NULL);
            Bstrcpy(tempbuf,asctime(localtime(&curtime)));
            clearbuf(ud.savegame[lastsavedpos],sizeof(ud.savegame[lastsavedpos]),0);
            Bsprintf(ud.savegame[lastsavedpos],"Auto");
            for (j=0;j<13;j++)
                Bmemcpy(&ud.savegame[lastsavedpos][j+4],&tempbuf[j+3],sizeof(tempbuf[j+3]));
            ud.savegame[lastsavedpos][j+4] = '\0';
            OSD_Printf("Saving to slot %d\n",lastsavedpos);

            KB_FlushKeyboardQueue();

            screencapt = 1;
            displayrooms(myconnectindex,65536);
            screencapt = 0;
            if (ud.multimode > 1)
                saveplayer(-1-(lastsavedpos));
            else saveplayer(lastsavedpos);

            break;
        }

    case CON_QUAKE:
        insptr++;
        earthquaketime = (char)GetGameVarID(*insptr++,g_i,g_p);
        spritesound(EARTHQUAKE,ps[screenpeek].i);
        break;

    case CON_IFMOVE:
        insptr++;
        parseifelse(g_t[1] == *insptr);
        break;

    case CON_RESETPLAYER:
    {
        insptr++;

        //AddLog("resetplayer");
        if (ud.multimode < 2)
        {
            if (lastsavedpos >= 0 && ud.recstat != 2)
            {
                ps[g_p].gm = MODE_MENU;
                KB_ClearKeyDown(sc_Space);
                cmenu(15000);
            }
            else ps[g_p].gm = MODE_RESTART;
            killit_flag = 2;
        }
        else
        {
            pickrandomspot(g_p);
            g_sp->x = hittype[g_i].bposx = ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx;
            g_sp->y = hittype[g_i].bposy = ps[g_p].bobposy = ps[g_p].oposy =ps[g_p].posy;
            g_sp->z = hittype[g_i].bposy = ps[g_p].oposz =ps[g_p].posz;
            updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
            setsprite(ps[g_p].i,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+PHEIGHT);
            g_sp->cstat = 257;

            g_sp->shade = -12;
            g_sp->clipdist = 64;
            g_sp->xrepeat = 42;
            g_sp->yrepeat = 36;
            g_sp->owner = g_i;
            g_sp->xoffset = 0;
            g_sp->pal = ps[g_p].palookup;

            ps[g_p].last_extra = g_sp->extra = max_player_health;
            ps[g_p].wantweaponfire = -1;
            ps[g_p].horiz = 100;
            ps[g_p].on_crane = -1;
            ps[g_p].frag_ps = g_p;
            ps[g_p].horizoff = 0;
            ps[g_p].opyoff = 0;
            ps[g_p].wackedbyactor = -1;
            ps[g_p].shield_amount = max_armour_amount;
            ps[g_p].dead_flag = 0;
            ps[g_p].pals_time = 0;
            ps[g_p].footprintcount = 0;
            ps[g_p].weapreccnt = 0;
            ps[g_p].fta = 0;
            ps[g_p].ftq = 0;
            ps[g_p].posxv = ps[g_p].posyv = 0;
            ps[g_p].rotscrnang = 0;
            ps[g_p].runspeed = dukefriction;
            ps[g_p].falling_counter = 0;

            hittype[g_i].extra = -1;
            hittype[g_i].owner = g_i;

            hittype[g_i].cgg = 0;
            hittype[g_i].movflag = 0;
            hittype[g_i].tempang = 0;
            hittype[g_i].actorstayput = -1;
            hittype[g_i].dispicnum = 0;
            hittype[g_i].owner = ps[g_p].i;

            resetinventory(g_p);
            resetweapons(g_p);

            ps[g_p].reloading = 0;

            ps[g_p].movement_lock = 0;

            OnEvent(EVENT_RESETPLAYER, ps[g_p].i, g_p, -1);
            cameradist = 0;
            cameraclock = totalclock;
        }
        setpal(&ps[g_p]);
        //AddLog("EOF: resetplayer");
    }
    break;

    case CON_IFONWATER:
        parseifelse(klabs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == 1);
        break;

    case CON_IFINWATER:
        parseifelse(sector[g_sp->sectnum].lotag == 2);
        break;

    case CON_IFCOUNT:
        insptr++;
        parseifelse(g_t[0] >= *insptr);
        break;

    case CON_IFACTOR:
        insptr++;
        parseifelse(g_sp->picnum == *insptr);
        break;

    case CON_RESETCOUNT:
        insptr++;
        g_t[0] = 0;
        break;

    case CON_ADDINVENTORY:
        insptr+=2;
        switch (*(insptr-1))
        {
        case GET_STEROIDS:
            ps[g_p].steroids_amount = *insptr;
            ps[g_p].inven_icon = 2;
            break;

        case GET_SHIELD:
            ps[g_p].shield_amount +=          *insptr;// 100;
            if (ps[g_p].shield_amount > max_player_health)
                ps[g_p].shield_amount = max_player_health;
            break;

        case GET_SCUBA:
            ps[g_p].scuba_amount =             *insptr;// 1600;
            ps[g_p].inven_icon = 6;
            break;

        case GET_HOLODUKE:
            ps[g_p].holoduke_amount =          *insptr;// 1600;
            ps[g_p].inven_icon = 3;
            break;

        case GET_JETPACK:
            ps[g_p].jetpack_amount =           *insptr;// 1600;
            ps[g_p].inven_icon = 4;
            break;

        case GET_ACCESS:
            switch (g_sp->pal)
            {
            case  0:
                ps[g_p].got_access |= 1;
                break;
            case 21:
                ps[g_p].got_access |= 2;
                break;
            case 23:
                ps[g_p].got_access |= 4;
                break;
            }
            break;

        case GET_HEATS:
            ps[g_p].heat_amount = *insptr;
            ps[g_p].inven_icon = 5;
            break;

        case GET_FIRSTAID:
            ps[g_p].inven_icon = 1;
            ps[g_p].firstaid_amount = *insptr;
            break;

        case GET_BOOTS:
            ps[g_p].inven_icon = 7;
            ps[g_p].boot_amount = *insptr;
            break;
        }
        insptr++;
        break;

    case CON_HITRADIUS:
        hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
        insptr+=6;
        break;

    case CON_IFP:
    {
        insptr++;

        l = *insptr;
        j = 0;

        s = sprite[ps[g_p].i].xvel;

        if ((l&8) && ps[g_p].on_ground && (sync[g_p].bits&2))
            j = 1;
        else if ((l&16) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&
                 ps[g_p].poszv > 2048)
            j = 1;
        else if ((l&32) && ps[g_p].jumping_counter > 348)
            j = 1;
        else if ((l&1) && s >= 0 && s < 8)
            j = 1;
        else if ((l&2) && s >= 8 && !(sync[g_p].bits&(1<<5)))
            j = 1;
        else if ((l&4) && s >= 8 && sync[g_p].bits&(1<<5))
            j = 1;
        else if ((l&64) && ps[g_p].posz < (g_sp->z-(48<<8)))
            j = 1;
        else if ((l&128) && s <= -8 && !(sync[g_p].bits&(1<<5)))
            j = 1;
        else if ((l&256) && s <= -8 && (sync[g_p].bits&(1<<5)))
            j = 1;
        else if ((l&512) && (ps[g_p].quick_kick > 0 || (ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0)))
            j = 1;
        else if ((l&1024) && sprite[ps[g_p].i].xrepeat < 32)
            j = 1;
        else if ((l&2048) && ps[g_p].jetpack_on)
            j = 1;
        else if ((l&4096) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400)
            j = 1;
        else if ((l&8192) && ps[g_p].on_ground)
            j = 1;
        else if ((l&16384) && sprite[ps[g_p].i].xrepeat > 32 && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0)
            j = 1;
        else if ((l&32768) && sprite[ps[g_p].i].extra <= 0)
            j = 1;
        else if ((l&65536L))
        {
            if (g_sp->picnum == APLAYER && ud.multimode > 1)
                j = getincangle(ps[otherp].ang,getangle(ps[g_p].posx-ps[otherp].posx,ps[g_p].posy-ps[otherp].posy));
            else
                j = getincangle(ps[g_p].ang,getangle(g_sp->x-ps[g_p].posx,g_sp->y-ps[g_p].posy));

            if (j > -128 && j < 128)
                j = 1;
            else
                j = 0;
        }
        parseifelse((long) j);
    }
    break;

    case CON_IFSTRENGTH:
        insptr++;
        parseifelse(g_sp->extra <= *insptr);
        break;

    case CON_GUTS:
        insptr += 2;
        guts(g_i,*(insptr-1),*insptr,g_p);
        insptr++;
        break;

    case CON_IFSPAWNEDBY:
        insptr++;
        parseifelse(hittype[g_i].picnum == *insptr);
        break;

    case CON_WACKPLAYER:
        insptr++;
        forceplayerangle(&ps[g_p]);
        return 0;

    case CON_FLASH:
        insptr++;
        sprite[g_i].shade = -127;
        ps[g_p].visibility = -127;
        lastvisinc = totalclock+32;
        return 0;

    case CON_STOPALLSOUNDS:
        insptr++;
        if (screenpeek == g_p)
            FX_StopAllSounds();
        return 0;

    case CON_IFGAPZL:
        insptr++;
        parseifelse(((hittype[g_i].floorz - hittype[g_i].ceilingz) >> 8) < *insptr);
        break;

    case CON_IFHITSPACE:
        parseifelse(sync[g_p].bits&(1<<29));
        break;

    case CON_IFOUTSIDE:
        parseifelse(sector[g_sp->sectnum].ceilingstat&1);
        break;

    case CON_IFMULTIPLAYER:
        parseifelse(ud.multimode > 1);
        break;

    case CON_OPERATE:
        insptr++;
        if (sector[g_sp->sectnum].lotag == 0)
        {
            neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
            if (neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag))
                if ((sector[neartagsector].lotag&0xff) == 23 || sector[neartagsector].floorz == sector[neartagsector].ceilingz)
                    if ((sector[neartagsector].lotag&16384) == 0)
                        if ((sector[neartagsector].lotag&32768) == 0)
                        {
                            j = headspritesect[neartagsector];
                            while (j >= 0)
                            {
                                if (sprite[j].picnum == ACTIVATOR)
                                    break;
                                j = nextspritesect[j];
                            }
                            if (j == -1)
                                operatesectors(neartagsector,g_i);
                        }
        }
        break;

    case CON_IFINSPACE:
        parseifelse(ceilingspace(g_sp->sectnum));
        break;

    case CON_SPRITEPAL:
        insptr++;
        if (g_sp->picnum != APLAYER)
            hittype[g_i].tempang = g_sp->pal;
        g_sp->pal = *insptr++;
        break;

    case CON_CACTOR:
        insptr++;
        g_sp->picnum = *insptr++;
        break;

    case CON_IFBULLETNEAR:
        parseifelse(dodge(g_sp) == 1);
        break;

    case CON_IFRESPAWN:
        if (badguy(g_sp))
            parseifelse(ud.respawn_monsters);
        else if (inventory(g_sp))
            parseifelse(ud.respawn_inventory);
        else
            parseifelse(ud.respawn_items);
        break;

    case CON_IFFLOORDISTL:
        insptr++;
        parseifelse((hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
        break;

    case CON_IFCEILINGDISTL:
        insptr++;
        parseifelse((g_sp->z - hittype[g_i].ceilingz) <= ((*insptr)<<8));
        break;

    case CON_PALFROM:
        insptr++;
        ps[g_p].pals_time = *insptr++;
        for (j=0;j<3;j++)
            ps[g_p].pals[j] = *insptr++;
        break;

    case CON_QSPRINTF:
        insptr++;
        {
            int dq = *insptr++, sq = *insptr++;
            if (fta_quotes[sq] != NULL && fta_quotes[dq] != NULL)
            {
                int var1 = GetGameVarID(*insptr++, g_i, g_p), var2 = GetGameVarID(*insptr++, g_i, g_p);
                int var3 = GetGameVarID(*insptr++, g_i, g_p), var4 = GetGameVarID(*insptr++, g_i, g_p);
                Bstrcpy(tempbuf,fta_quotes[sq]);
                Bsprintf(fta_quotes[dq],tempbuf,var1,var2,var3,var4);
                break;
            }
            if (fta_quotes[sq] == NULL) OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,sq);
            if (fta_quotes[dq] == NULL) OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,dq);
            insptr += 5;
            break;
        }

    case CON_ADDLOG:
    {
        insptr++;
        OSD_Printf("CONLOG: L=%ld\n",*insptr++);
        break;
    }

    case CON_ADDLOGVAR:
        insptr++;
        {
            int m=1;
            char szBuf[256];
            long l=*insptr++, lVarID = *insptr;

            if ((lVarID >= iGameVarCount) || lVarID < 0)
            {
                if (*insptr==MAXGAMEVARS) // addlogvar for a constant?  Har.
                    insptr++;
                else if (*insptr&(MAXGAMEVARS<<1))
                {
                    m = -1;
                    lVarID ^= (MAXGAMEVARS<<1);
                }
                else
                {
                    // invalid varID
                    insptr++;
                    OSD_Printf("CONLOGVAR: L=%ld INVALID VARIABLE\n",l);
                    break;  // out of switch
                }
            }
            Bsprintf(szBuf,"CONLOGVAR: L=%ld %s ",l, aGameVars[lVarID].szLabel);
            strcpy(g_szBuf,szBuf);

            if (aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_READONLY)
            {
                Bsprintf(szBuf," (read-only)");
                strcat(g_szBuf,szBuf);
            }
            if (aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            {
                Bsprintf(szBuf," (Per Player. Player=%d)",g_p);
            }
            else if (aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_PERACTOR)
            {
                Bsprintf(szBuf," (Per Actor. Actor=%d)",g_i);
            }
            else
            {
                Bsprintf(szBuf," (Global)");
            }
            Bstrcat(g_szBuf,szBuf);
            Bsprintf(szBuf," =%ld\n", GetGameVarID(lVarID, g_i, g_p)*m);
            Bstrcat(g_szBuf,szBuf);
            OSD_Printf(g_szBuf);
            insptr++;
            break;
        }

    case CON_SETSECTOR:
    case CON_GETSECTOR:
        insptr++;
        {
            // syntax [gs]etsector[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

            DoSector(tw==CON_SETSECTOR, lVar1, lLabelID, lVar2);
            break;
        }

    case CON_SQRT:
        insptr++;
        {
            // syntax sqrt <invar> <outvar>
            int lInVarID=*insptr++, lOutVarID=*insptr++;

            SetGameVarID(lOutVarID, ksqrt(GetGameVarID(lInVarID, g_i, g_p)), g_i, g_p);
            break;
        }

    case CON_FINDNEARACTOR:
    case CON_FINDNEARSPRITE:
    case CON_FINDNEARACTOR3D:
    case CON_FINDNEARSPRITE3D:
        insptr++;
        {
            // syntax findnearactorvar <type> <maxdist> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdist> <varid>
            int lType=*insptr++, lMaxDist=*insptr++, lVarID=*insptr++;
            int lFound=-1, lTemp, j, k;

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[(tw==CON_FINDNEARACTOR||tw==CON_FINDNEARACTOR3D)?1:k];    // all sprites
                while (j>=0)
                {
                    if (sprite[j].picnum == lType && j != g_i)
                    {
                        if (tw==CON_FINDNEARACTOR3D || tw==CON_FINDNEARSPRITE3D)
                            lTemp=dist(&sprite[g_i], &sprite[j]);
                        else lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if (lTemp < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                    }
                    j = nextspritestat[j];
                }
                if ((tw==CON_FINDNEARACTOR||tw==CON_FINDNEARACTOR3D) || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_FINDNEARACTORVAR:
    case CON_FINDNEARSPRITEVAR:
    case CON_FINDNEARACTOR3DVAR:
    case CON_FINDNEARSPRITE3DVAR:
        insptr++;
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            int lType=*insptr++, lMaxDist=GetGameVarID(*insptr++, g_i, g_p), lVarID=*insptr++;
            int lFound=-1, lTemp, j, k;

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[(tw==CON_FINDNEARACTORVAR||tw==CON_FINDNEARACTOR3DVAR)?1:k];    // all sprites
                while (j>=0)
                {
                    if (sprite[j].picnum == lType && j != g_i)
                    {
                        if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                            lTemp=dist(&sprite[g_i], &sprite[j]);
                        else lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if (lTemp < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                    }
                    j = nextspritestat[j];
                }
                if ((tw==CON_FINDNEARACTORVAR||tw==CON_FINDNEARACTOR3DVAR) || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_FINDNEARACTORZVAR:
    case CON_FINDNEARSPRITEZVAR:
        insptr++;
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            int lType=*insptr++, lMaxDist=GetGameVarID(*insptr++, g_i, g_p), lMaxZDist=GetGameVarID(*insptr++, g_i, g_p);
            int lVarID=*insptr++, lFound=-1, lTemp, lTemp2, j, k;

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTORZVAR?1:k];    // all sprites
                while (j>=0)
                {
                    if (sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if (lTemp < lMaxDist)
                        {
                            lTemp2=klabs(sprite[g_i].z-sprite[j].z);
                            if (lTemp2 < lMaxZDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                        }
                    }
                    j = nextspritestat[j];
                }
                if (tw==CON_FINDNEARACTORZVAR || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);

            break;
        }

    case CON_FINDNEARACTORZ:
    case CON_FINDNEARSPRITEZ:
        insptr++;
        {
            // syntax findnearactorvar <type> <maxdist> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdist> <varid>
            int lType=*insptr++, lMaxDist=*insptr++, lMaxZDist=*insptr++, lVarID=*insptr++;
            int lTemp, lTemp2, lFound=-1, j, k;

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTORZ?1:k];    // all sprites
                while (j>=0)
                {
                    if (sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if (lTemp < lMaxDist)
                        {
                            lTemp2=klabs(sprite[g_i].z-sprite[j].z);
                            if (lTemp2 < lMaxZDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                        }
                    }
                    j = nextspritestat[j];
                }
                if (tw==CON_FINDNEARACTORZ || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_FINDPLAYER:
    case CON_FINDOTHERPLAYER:
        insptr++;
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            long var1 = *insptr++, d;

            if (tw == CON_FINDPLAYER) j=findplayer(&sprite[g_i],&d);
            else if (tw == CON_FINDOTHERPLAYER) j=findotherplayer(g_i,&d);

            SetGameVarID(g_iReturnVarID, j, g_i, g_p);
            SetGameVarID(var1, d, g_i, g_p);

            break;
        }

    case CON_SETPLAYER:
    case CON_GETPLAYER:
        insptr++;
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=*insptr++, lLabelID=*insptr++, lParm2, lVar2;
            // HACK: need to have access to labels structure at run-time...

            switch (lLabelID)
            {
            case PLAYER_AMMO_AMOUNT:
            case PLAYER_GOTWEAPON:
            case PLAYER_PALS:
            case PLAYER_LOOGIEX:
            case PLAYER_LOOGIEY:
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
                break;
            default:
                lParm2=0;
                break;
            }
            lVar2=*insptr++;

            DoPlayer(tw==CON_SETPLAYER, lVar1, lLabelID, lVar2, lParm2);
            break;
        }

    case CON_SETINPUT:
    case CON_GETINPUT:
        insptr++;
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

            DoInput(tw==CON_SETINPUT, lVar1, lLabelID, lVar2);
            break;
        }

    case CON_GETUSERDEF:
    case CON_SETUSERDEF:
        insptr++;
        {
            // syntax [gs]etuserdef.xxx <VAR>
            //  <xxxid> <varid>
            int lLabelID=*insptr++, lVar2=*insptr++;

            DoUserDef(tw==CON_SETUSERDEF, lLabelID, lVar2);
            break;
        }

    case CON_GETPROJECTILE:
    case CON_SETPROJECTILE:
        insptr++;
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=GetGameVarID(*insptr++, g_i, g_p), lLabelID=*insptr++, lVar2=*insptr++;

            DoProjectile(tw==CON_SETPROJECTILE,lVar1,lLabelID,lVar2);
            break;
        }

    case CON_SETWALL:
    case CON_GETWALL:
        insptr++;
        {
            // syntax [gs]etwall[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            int lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

            DoWall(tw==CON_SETWALL, lVar1, lLabelID, lVar2);
            break;
        }

    case CON_SETACTORVAR:
    case CON_GETACTORVAR:
        insptr++;
        {
            // syntax [gs]etactorvar[<var>].<varx> <VAR>
            // gets the value of the per-actor variable varx into VAR
            // <var> <varx> <VAR>
            int lSprite=GetGameVarID(*insptr++, g_i, g_p), lVar1=*insptr++, lVar2=*insptr++;

            if (tw == CON_SETACTORVAR)
            {
                SetGameVarID(lVar1, GetGameVarID(lVar2, g_i, g_p), lSprite, g_p);
                break;
            }
            SetGameVarID(lVar2, GetGameVarID(lVar1, lSprite, g_p), g_i, g_p);
            break;
        }

    case CON_SETPLAYERVAR:
    case CON_GETPLAYERVAR:
        insptr++;
        {
            int iPlayer;

            if (*insptr != g_iThisActorID)
                iPlayer=GetGameVarID(*insptr, g_i, g_p);
            else iPlayer = g_p;

            insptr++;
            {
                int lVar1=*insptr++, lVar2=*insptr++;

                if (tw == CON_SETPLAYERVAR)
                {
                    SetGameVarID(lVar1, GetGameVarID(lVar2, g_i, g_p), g_i, iPlayer);
                    break;
                }
                SetGameVarID(lVar2, GetGameVarID(lVar1, g_i, iPlayer), g_i, g_p);
                break;
            }
        }

    case CON_SETACTOR:
    case CON_GETACTOR:
        insptr++;
        {
            // syntax [gs]etactor[<var>].x <VAR>
            // <varid> <xxxid> <varid>

            int lVar1=*insptr++, lLabelID=*insptr++, lParm2, lVar2;

            switch (lLabelID)
            {
            case ACTOR_HTG_T:
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
                break;
            default:
                lParm2=0;
                break;
            }
            lVar2=*insptr++;

            DoActor(tw==CON_SETACTOR, lVar1, lLabelID, lVar2, lParm2);
            break;
        }

    case CON_GETANGLETOTARGET:
        insptr++;
        // hittype[g_i].lastvx and lastvy are last known location of target.
        SetGameVarID(*insptr++, getangle(hittype[g_i].lastvx-g_sp->x,hittype[g_i].lastvy-g_sp->y), g_i, g_p);
        break;

    case CON_ANGOFFVAR:
        insptr++;
        spriteext[g_i].angoff=GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_LOCKPLAYER:
        insptr++;
        ps[g_p].transporter_hold=GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_CHECKAVAILWEAPON:
    case CON_CHECKAVAILINVEN:
        insptr++;
        j = g_p;

        if (*insptr != g_iThisActorID)
            j=GetGameVarID(*insptr, g_i, g_p);

        insptr++;

        if (j < MAXPLAYERS)
        {
            if (tw == CON_CHECKAVAILWEAPON)
                checkavailweapon(&ps[j]);
            else checkavailinven(&ps[j]);
        }
        break;

    case CON_GETPLAYERANGLE:
        insptr++;
        SetGameVarID(*insptr++, ps[g_p].ang, g_i, g_p);
        break;

    case CON_SETPLAYERANGLE:
        insptr++;
        ps[g_p].ang=GetGameVarID(*insptr++, g_i, g_p);
        ps[g_p].ang &= 2047;
        break;

    case CON_GETACTORANGLE:
        insptr++;
        SetGameVarID(*insptr++, g_sp->ang, g_i, g_p);
        break;

    case CON_SETACTORANGLE:
        insptr++;
        g_sp->ang=GetGameVarID(*insptr++, g_i, g_p);
        g_sp->ang &= 2047;
        break;

    case CON_SETVAR:
        insptr++;
        SetGameVarID(*insptr, *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_RANDVAR:
        insptr++;
        SetGameVarID(*insptr, mulscale(krand(), *(insptr+1)+1, 16), g_i, g_p);
        insptr += 2;
        break;

    case CON_DISPLAYRANDVAR:
        insptr++;
        SetGameVarID(*insptr, mulscale(rand(), *(insptr+1)+1, 15), g_i, g_p);
        insptr += 2;
        break;

    case CON_MULVAR:
        insptr++;
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) * *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_DIVVAR:
        insptr++;
        if (*(insptr+1) == 0)
            gameexit("CON_DIVVAR: Divide by zero.");
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) / *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_MODVAR:
        insptr++;
        if (*(insptr+1) == 0)
            gameexit("CON_MODVAR: Mod by zero.");
        SetGameVarID(*insptr,GetGameVarID(*insptr, g_i, g_p)%*(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_ANDVAR:
        insptr++;
        SetGameVarID(*insptr,GetGameVarID(*insptr, g_i, g_p) & *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_ORVAR:
        insptr++;
        SetGameVarID(*insptr,GetGameVarID(*insptr, g_i, g_p) | *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_XORVAR:
        insptr++;
        SetGameVarID(*insptr,GetGameVarID(*insptr, g_i, g_p) ^ *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_SETVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(*insptr++, g_i, g_p), g_i, g_p);
        break;

    case CON_RANDVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j,mulscale(krand(), GetGameVarID(*insptr++, g_i, g_p)+1, 16), g_i, g_p);
        break;

    case CON_DISPLAYRANDVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j,mulscale(rand(), GetGameVarID(*insptr++, g_i, g_p)+1, 15), g_i, g_p);
        break;

    case CON_GMAXAMMO:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);
        SetGameVarID(*insptr++, max_ammo_amount[j], g_i, g_p);
        break;

    case CON_SMAXAMMO:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);
        max_ammo_amount[j]=GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_MULVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p)*GetGameVarID(*insptr++, g_i, g_p), g_i, g_p);
        break;

    case CON_DIVVARVAR:
        insptr++;
        j=*insptr++;
        {
            int l2=GetGameVarID(*insptr++, g_i, g_p);

            if (l2==0)
                gameexit("CON_DIVVARVAR: Divide by zero.");

            SetGameVarID(j, GetGameVarID(j, g_i, g_p)/l2 , g_i, g_p);
            break;
        }

    case CON_MODVARVAR:
        insptr++;
        j=*insptr++;
        {
            int l2=GetGameVarID(*insptr++, g_i, g_p);

            if (l2==0)
                gameexit("CON_MODVARVAR: Mod by zero.");

            SetGameVarID(j, GetGameVarID(j, g_i, g_p) % l2, g_i, g_p);
            break;
        }

    case CON_ANDVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p) & GetGameVarID(*insptr++, g_i, g_p), g_i, g_p);
        break;

    case CON_XORVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p) ^ GetGameVarID(*insptr++, g_i, g_p) , g_i, g_p);
        break;

    case CON_ORVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p) | GetGameVarID(*insptr++, g_i, g_p) , g_i, g_p);
        break;

    case CON_SUBVAR:
        insptr++;
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) - *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_SUBVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p) - GetGameVarID(*insptr++, g_i, g_p), g_i, g_p);
        break;

    case CON_ADDVAR:
        insptr++;
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) + *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_SHIFTVARL:
        insptr++;
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) << *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_SHIFTVARR:
        insptr++;
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) >> *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_SIN:
        insptr++;
        SetGameVarID(*insptr, sintable[GetGameVarID(*(insptr+1), g_i, g_p)&2047], g_i, g_p);
        insptr += 2;
        break;

    case CON_COS:
        insptr++;
        SetGameVarID(*insptr, sintable[(GetGameVarID(*(insptr+1), g_i, g_p)+512)&2047], g_i, g_p);
        insptr += 2;
        break;

    case CON_ADDVARVAR:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, GetGameVarID(j, g_i, g_p) + GetGameVarID(*insptr++, g_i, g_p), g_i, g_p);
        break;

    case CON_SPGETLOTAG:
        insptr++;
        SetGameVarID(g_iLoTagID, g_sp->lotag, g_i, g_p);
        break;

    case CON_SPGETHITAG:
        insptr++;
        SetGameVarID(g_iHiTagID, g_sp->hitag, g_i, g_p);
        break;

    case CON_SECTGETLOTAG:
        insptr++;
        SetGameVarID(g_iLoTagID, sector[g_sp->sectnum].lotag, g_i, g_p);
        break;

    case CON_SECTGETHITAG:
        insptr++;
        SetGameVarID(g_iHiTagID, sector[g_sp->sectnum].hitag, g_i, g_p);
        break;

    case CON_GETTEXTUREFLOOR:
        insptr++;
        SetGameVarID(g_iTextureID, sector[g_sp->sectnum].floorpicnum, g_i, g_p);
        break;

    case CON_STARTTRACK:
        insptr++;
        music_select=*insptr++;
        if (music_fn[ud.volume_number][(unsigned char)music_select] != NULL)
            playmusic(&music_fn[ud.volume_number][(unsigned char)music_select][0]);
        break;

    case CON_GETTEXTURECEILING:
        insptr++;
        SetGameVarID(g_iTextureID, sector[g_sp->sectnum].ceilingpicnum, g_i, g_p);
        break;

    case CON_IFVARVARAND:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) & GetGameVarID(*(insptr), g_i, g_p));
        break;

    case CON_IFVARVARN:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) != GetGameVarID(*(insptr), g_i, g_p));
        break;

    case CON_IFVARVARE:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) == GetGameVarID(*(insptr), g_i, g_p));
        break;

    case CON_IFVARVARG:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) > GetGameVarID(*(insptr), g_i, g_p));
        break;

    case CON_IFVARVARL:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) < GetGameVarID(*(insptr), g_i, g_p));
        break;

    case CON_IFVARE:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) == *insptr);
        break;

    case CON_IFVARN:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) != *insptr);
        break;

    case CON_WHILEVARN:
    {
        int i;
        long *savedinsptr=insptr;
        j=1;
        while (j)
        {
            insptr=savedinsptr;
            insptr++;
            i=*insptr++;
            j=0;

            if (GetGameVarID(i, g_i, g_p) != *insptr)
                j=1;
            parseifelse(j);
        }
        break;
    }

    case CON_WHILEVARVARN:
    {
        int i,k;
        long *savedinsptr=insptr;
        j=1;
        while (j)
        {
            insptr=savedinsptr;
            insptr++;
            i=*insptr++;
            k=*(insptr);
            j=0;

            if (GetGameVarID(i, g_i, g_p) != GetGameVarID(k, g_i, g_p))
                j=1;
            parseifelse(j);
        }
        break;
    }

    case CON_IFVARAND:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) & *insptr);
        break;

    case CON_IFVARG:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) > *insptr);
        break;

    case CON_IFVARL:
        insptr++;
        j=*insptr++;
        parseifelse(GetGameVarID(j, g_i, g_p) < *insptr);
        break;

    case CON_IFPHEALTHL:
        insptr++;
        parseifelse(sprite[ps[g_p].i].extra < *insptr);
        break;

    case CON_IFPINVENTORY:
    {
        insptr++;
        j = 0;
        switch (*insptr++)
        {
        case GET_STEROIDS:
            if (ps[g_p].steroids_amount != *insptr)
                j = 1;
            break;
        case GET_SHIELD:
            if (ps[g_p].shield_amount != max_player_health)
                j = 1;
            break;
        case GET_SCUBA:
            if (ps[g_p].scuba_amount != *insptr) j = 1;
            break;
        case GET_HOLODUKE:
            if (ps[g_p].holoduke_amount != *insptr) j = 1;
            break;
        case GET_JETPACK:
            if (ps[g_p].jetpack_amount != *insptr) j = 1;
            break;
        case GET_ACCESS:
            switch (g_sp->pal)
            {
            case  0:
                if (ps[g_p].got_access&1) j = 1;
                break;
            case 21:
                if (ps[g_p].got_access&2) j = 1;
                break;
            case 23:
                if (ps[g_p].got_access&4) j = 1;
                break;
            }
            break;
        case GET_HEATS:
            if (ps[g_p].heat_amount != *insptr) j = 1;
            break;
        case GET_FIRSTAID:
            if (ps[g_p].firstaid_amount != *insptr) j = 1;
            break;
        case GET_BOOTS:
            if (ps[g_p].boot_amount != *insptr) j = 1;
            break;
        }

        parseifelse(j);
        break;
    }

    case CON_PSTOMP:
        insptr++;
        if (ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= 40)
            if (cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum))
            {
                for (j=0;j<MAXPLAYERS;j++)
                {
                    if (ps[j].actorsqu == g_i)
                        break;
                }
                if (j == MAXPLAYERS)
                {
                    ps[g_p].knee_incs = 1;
                    if (ps[g_p].weapon_pos == 0)
                        ps[g_p].weapon_pos = -1;
                    ps[g_p].actorsqu = g_i;
                }
            }
        break;

    case CON_IFAWAYFROMWALL:
    {
        short s1=g_sp->sectnum;

        j = 0;

        updatesector(g_sp->x+108,g_sp->y+108,&s1);
        if (s1 == g_sp->sectnum)
        {
            updatesector(g_sp->x-108,g_sp->y-108,&s1);
            if (s1 == g_sp->sectnum)
            {
                updatesector(g_sp->x+108,g_sp->y-108,&s1);
                if (s1 == g_sp->sectnum)
                {
                    updatesector(g_sp->x-108,g_sp->y+108,&s1);
                    if (s1 == g_sp->sectnum)
                        j = 1;
                }
            }
        }
        parseifelse(j);
    }
    break;

    case CON_QUOTE:
        insptr++;

        if (fta_quotes[*insptr] == NULL)
        {
            OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,*insptr);
            insptr++;
            break;
        }
        FTA(*insptr++,&ps[g_p]);
        break;

    case CON_USERQUOTE:
        insptr++;
        if (fta_quotes[*insptr] == NULL)
        {
            OSD_Printf("%s %d null quote %d\n",__FILE__,__LINE__,*insptr);
            insptr++;
            break;
        }
        adduserquote(fta_quotes[*insptr++]);
        break;

    case CON_IFINOUTERSPACE:
        parseifelse(floorspace(g_sp->sectnum));
        break;

    case CON_IFNOTMOVING:
        parseifelse((hittype[g_i].movflag&49152) > 16384);
        break;

    case CON_RESPAWNHITAG:
        insptr++;
        switch (dynamictostatic[g_sp->picnum])
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
        case FEM10__STATIC:
        case PODFEM1__STATIC:
        case NAKED1__STATIC:
        case STATUE__STATIC:
            if (g_sp->yvel) operaterespawns(g_sp->yvel);
            break;
        default:
            if (g_sp->hitag >= 0) operaterespawns(g_sp->hitag);
            break;
        }
        break;

    case CON_IFSPRITEPAL:
        insptr++;
        parseifelse(g_sp->pal == *insptr);
        break;

    case CON_IFANGDIFFL:
        insptr++;
        j = klabs(getincangle(ps[g_p].ang,g_sp->ang));
        parseifelse(j <= *insptr);
        break;

    case CON_IFNOSOUNDS:
        for (j=1;j<NUM_SOUNDS;j++)
            if (SoundOwner[j][0].i == g_i)
                break;

        parseifelse(j == NUM_SOUNDS);
        break;

    case CON_SPRITEFLAGS:
        insptr++;
        actorspriteflags[g_i] = GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_GETCURRADDRESS:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, (long)insptr, g_i, g_p);
        break;

    case CON_JUMP:
        insptr++;
        j = GetGameVarID(*insptr++, g_i, g_p);
        insptr = (long *)j;
        break;

    default:
        OSD_Printf("fatal error: default processing: prev inst: %ld, curr inst: %ld, next inst: %ld\ncurrent actor: %d (%d)\n",*(insptr-1),*insptr,*(insptr+1),g_i,g_sp->picnum);
        gameexit("An error has occurred in the EDuke32 CON executor.\n\nPlease mail all of your CON files along with the file eduke32.log\nto terminx@gmail.com.\n\nThank you.");
        break;
    }
    return 0;
}

void LoadActor(long iActor)
{
    g_i = iActor;    // Sprite ID
    g_p = -1; // iPlayer;    // Player ID
    g_x = -1; // lDist;    // ??
    g_sp = &sprite[g_i];    // Pointer to sprite structure
    g_t = &hittype[g_i].temp_data[0];   // Sprite's 'extra' data

    if (actorLoadEventScrptr[g_sp->picnum] == 0) return;

    insptr = actorLoadEventScrptr[g_sp->picnum];

    killit_flag = 0;

    if (g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        //      if(badguy(g_sp))
        //          ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }

    while (1) if (parse()) break;

    if (killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if (g_p >= 0)
        {
            if (ps[g_p].actorsqu == g_i)
                ps[g_p].actorsqu = -1;
        }
        deletesprite(g_i);
    }
}

void execute(int iActor,int iPlayer,long lDist)
{
    g_i = iActor;    // Sprite ID
    g_p = iPlayer;   // Player ID
    g_x = lDist;     // ??
    g_sp = &sprite[g_i];    // Pointer to sprite structure
    g_t = &hittype[g_i].temp_data[0];   // Sprite's 'extra' data

    if (actorscrptr[g_sp->picnum] == 0) return;

    insptr = 4 + (actorscrptr[g_sp->picnum]);

    killit_flag = 0;

    if (g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        if (badguy(g_sp))
            ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }

    if (g_t[4])
    {
        g_sp->lotag += TICSPERFRAME;

        if (g_sp->lotag > *(long *)(g_t[4]+16))
        {
            g_t[2]++;
            g_sp->lotag = 0;
            g_t[3] +=  *(long *)(g_t[4]+12);
        }

        if (klabs(g_t[3]) >= klabs(*(long *)(g_t[4]+4) * *(long *)(g_t[4]+12)))
            g_t[3] = 0;
    }

    while (1) if (parse()) break;

    if (killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if (ps[g_p].actorsqu == g_i)
            ps[g_p].actorsqu = -1;
        deletesprite(g_i);
    }
    else
    {
        move();

#if 0 
        sprpos[g_i].x = g_sp->x;
        sprpos[g_i].y = g_sp->y;
        sprpos[g_i].z = g_sp->z;
        sprpos[g_i].ang = g_sp->ang;
#endif

        if (g_sp->statnum == 1)
        {
            if (badguy(g_sp))
            {
                if (g_sp->xrepeat > 60) return;
                if (ud.respawn_monsters == 1 && g_sp->extra <= 0) return;
            }
            else if (ud.respawn_items == 1 && (g_sp->cstat&32768)) return;

            if (hittype[g_i].timetosleep > 1)
                hittype[g_i].timetosleep--;
            else if (hittype[g_i].timetosleep == 1)
                changespritestat(g_i,2);
        }

        else if (g_sp->statnum == 6)
            switch (dynamictostatic[g_sp->picnum])
            {
            case RUBBERCAN__STATIC:
            case EXPLODINGBARREL__STATIC:
            case WOODENHORSE__STATIC:
            case HORSEONSIDE__STATIC:
            case CANWITHSOMETHING__STATIC:
            case FIREBARREL__STATIC:
            case NUKEBARREL__STATIC:
            case NUKEBARRELDENTED__STATIC:
            case NUKEBARRELLEAKED__STATIC:
            case TRIPBOMB__STATIC:
            case EGG__STATIC:
                if (hittype[g_i].timetosleep > 1)
                    hittype[g_i].timetosleep--;
                else if (hittype[g_i].timetosleep == 1)
                    changespritestat(g_i,2);
                break;
            }
    }
}
