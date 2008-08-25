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

#include "osdcmds.h"
#include "osd.h"

void restoremapstate(mapstate_t *save);
void savemapstate(mapstate_t *save);
#define CON_ERROR OSD_ERROR "Line %d, %s: "

int g_i,g_p;
static int g_x;
static intptr_t *g_t;
static spritetype *g_sp;
static int killit_flag;
int line_num;
int g_tw;
extern const char *keyw[];
extern int checkCON;

static int parse(void);

static void DoUserDef(int iSet, int lLabelID, int lVar2)
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
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case USERDEFS_GOD:
        if (iSet)
        {
            ud.god = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.god, g_i, g_p);
        return;

    case USERDEFS_WARP_ON:
        if (iSet)
        {
            ud.warp_on = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.warp_on, g_i, g_p);
        return;

    case USERDEFS_CASHMAN:
        if (iSet)
        {
            ud.cashman = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.cashman, g_i, g_p);
        return;

    case USERDEFS_EOG:
        if (iSet)
        {
            ud.eog = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.eog, g_i, g_p);
        return;

    case USERDEFS_SHOWALLMAP:
        if (iSet)
        {
            ud.showallmap = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.showallmap, g_i, g_p);
        return;

    case USERDEFS_SHOW_HELP:
        if (iSet)
        {
            ud.show_help = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.show_help, g_i, g_p);
        return;

    case USERDEFS_SCROLLMODE:
        if (iSet)
        {
            ud.scrollmode = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.scrollmode, g_i, g_p);
        return;

    case USERDEFS_CLIPPING:
        if (iSet)
        {
            ud.clipping = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.clipping, g_i, g_p);
        return;

        //  case USERDEFS_USER_NAME:
        //      if(iSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.user_name[MAXPLAYERS][32], g_i, g_p);
        //      return;

        //  case USERDEFS_RIDECULE:
        //      if(iSet)
        //      {
        //          ud.ridecule = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.ridecule, g_i, g_p);
        //      return;

        //  case USERDEFS_SAVEGAME:
        //      if(iSet)
        //      {
        //          ud.savegame = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.savegame, g_i, g_p);
        //      return;

        //  case USERDEFS_PWLOCKOUT:
        //      if(iSet)
        //      {
        //          ud.pwlockout = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.pwlockout, g_i, g_p);
        //      return;

        //  case USERDEFS_RTSNAME:
        //      if(iSet)
        //      {
        //          ud.rtsname = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.rtsname, g_i, g_p);
        //      return;

    case USERDEFS_OVERHEAD_ON:
        if (iSet)
        {
            ud.overhead_on = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.overhead_on, g_i, g_p);
        return;

    case USERDEFS_LAST_OVERHEAD:
        if (iSet)
        {
            ud.last_overhead = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.last_overhead, g_i, g_p);
        return;

    case USERDEFS_SHOWWEAPONS:
        if (iSet)
        {
            ud.showweapons = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.showweapons, g_i, g_p);
        return;

    case USERDEFS_PAUSE_ON:
        if (iSet)
        {
            ud.pause_on = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.pause_on, g_i, g_p);
        return;

    case USERDEFS_FROM_BONUS:
        if (iSet)
        {
            ud.from_bonus = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.from_bonus, g_i, g_p);
        return;

    case USERDEFS_CAMERASPRITE:
        if (iSet)
        {
            ud.camerasprite = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.camerasprite, g_i, g_p);
        return;

    case USERDEFS_LAST_CAMSPRITE:
        if (iSet)
        {
            ud.last_camsprite = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.last_camsprite, g_i, g_p);
        return;

    case USERDEFS_LAST_LEVEL:
        if (iSet)
        {
            ud.last_level = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.last_level, g_i, g_p);
        return;

    case USERDEFS_SECRETLEVEL:
        if (iSet)
        {
            ud.secretlevel = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.secretlevel, g_i, g_p);
        return;

    case USERDEFS_CONST_VISIBILITY:
        if (iSet)
        {
            ud.const_visibility = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.const_visibility, g_i, g_p);
        return;

    case USERDEFS_UW_FRAMERATE:
        if (iSet)
        {
            ud.uw_framerate = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.uw_framerate, g_i, g_p);
        return;

    case USERDEFS_CAMERA_TIME:
        if (iSet)
        {
            ud.camera_time = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.camera_time, g_i, g_p);
        return;

    case USERDEFS_FOLFVEL:
        if (iSet)
        {
            ud.folfvel = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.folfvel, g_i, g_p);
        return;

    case USERDEFS_FOLAVEL:
        if (iSet)
        {
            ud.folavel = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.folavel, g_i, g_p);
        return;

    case USERDEFS_FOLX:
        if (iSet)
        {
            ud.folx = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.folx, g_i, g_p);
        return;

    case USERDEFS_FOLY:
        if (iSet)
        {
            ud.foly = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.foly, g_i, g_p);
        return;

    case USERDEFS_FOLA:
        if (iSet)
        {
            ud.fola = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.fola, g_i, g_p);
        return;

    case USERDEFS_RECCNT:
        if (iSet)
        {
            ud.reccnt = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.reccnt, g_i, g_p);
        return;

    case USERDEFS_ENTERED_NAME:
        if (iSet)
        {
            ud.entered_name = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.entered_name, g_i, g_p);
        return;

    case USERDEFS_SCREEN_TILTING:
        if (iSet)
        {
            ud.screen_tilting = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.screen_tilting, g_i, g_p);
        return;

    case USERDEFS_SHADOWS:
        if (iSet)
        {
            ud.shadows = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.shadows, g_i, g_p);
        return;

    case USERDEFS_FTA_ON:
        if (iSet)
        {
            ud.fta_on = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.fta_on, g_i, g_p);
        return;

    case USERDEFS_EXECUTIONS:
        if (iSet)
        {
            ud.executions = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.executions, g_i, g_p);
        return;

    case USERDEFS_AUTO_RUN:
        if (iSet)
        {
            ud.auto_run = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.auto_run, g_i, g_p);
        return;

    case USERDEFS_COORDS:
        if (iSet)
        {
            ud.coords = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.coords, g_i, g_p);
        return;

    case USERDEFS_TICKRATE:
        if (iSet)
        {
            ud.tickrate = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.tickrate, g_i, g_p);
        return;

    case USERDEFS_M_COOP:
        if (iSet)
        {
            ud.m_coop = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_coop, g_i, g_p);
        return;

    case USERDEFS_COOP:
        if (iSet)
        {
            ud.coop = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.coop, g_i, g_p);
        return;

    case USERDEFS_SCREEN_SIZE:
        if (iSet)
        {
            if (ud.screen_size != lValue)
            {
                ud.screen_size = lValue;
                vscrn();
            }
            return;
        }
        SetGameVarID(lVar2, ud.screen_size, g_i, g_p);
        return;

    case USERDEFS_LOCKOUT:
        if (iSet)
        {
            ud.lockout = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.lockout, g_i, g_p);
        return;

    case USERDEFS_CROSSHAIR:
        if (iSet)
        {
            ud.crosshair = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.crosshair, g_i, g_p);
        return;

        //  case USERDEFS_WCHOICE:
        //      if(iSet)
        //      {
        //          ud.wchoice = lValue;
        //          return;
        //      }
        //      SetGameVarID(lVar2, ud.wchoice, g_i, g_p);
        //      return;

    case USERDEFS_PLAYERAI:
        if (iSet)
        {
            ud.playerai = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.playerai, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.respawn_monsters = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.respawn_monsters, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.respawn_items = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.respawn_items, g_i, g_p);
        return;

    case USERDEFS_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.respawn_inventory = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.respawn_inventory, g_i, g_p);
        return;

    case USERDEFS_RECSTAT:
        if (iSet)
        {
            ud.recstat = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.recstat, g_i, g_p);
        return;

    case USERDEFS_MONSTERS_OFF:
        if (iSet)
        {
            ud.monsters_off = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.monsters_off, g_i, g_p);
        return;

    case USERDEFS_BRIGHTNESS:
        if (iSet)
        {
            ud.brightness = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.brightness, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.m_respawn_items = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_respawn_items, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.m_respawn_monsters = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_respawn_monsters, g_i, g_p);
        return;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.m_respawn_inventory = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_respawn_inventory, g_i, g_p);
        return;

    case USERDEFS_M_RECSTAT:
        if (iSet)
        {
            ud.m_recstat = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_recstat, g_i, g_p);
        return;

    case USERDEFS_M_MONSTERS_OFF:
        if (iSet)
        {
            ud.m_monsters_off = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_monsters_off, g_i, g_p);
        return;

    case USERDEFS_DETAIL:
        if (iSet)
        {
            ud.detail = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.detail, g_i, g_p);
        return;

    case USERDEFS_M_FFIRE:
        if (iSet)
        {
            ud.m_ffire = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_ffire, g_i, g_p);
        return;

    case USERDEFS_FFIRE:
        if (iSet)
        {
            ud.ffire = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.ffire, g_i, g_p);
        return;

    case USERDEFS_M_PLAYER_SKILL:
        if (iSet)
        {
            ud.m_player_skill = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_player_skill, g_i, g_p);
        return;

    case USERDEFS_M_LEVEL_NUMBER:
        if (iSet)
        {
            ud.m_level_number = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_level_number, g_i, g_p);
        return;

    case USERDEFS_M_VOLUME_NUMBER:
        if (iSet)
        {
            ud.m_volume_number = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_volume_number, g_i, g_p);
        return;

    case USERDEFS_MULTIMODE:
        if (iSet)
        {
            ud.multimode = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.multimode, g_i, g_p);
        return;

    case USERDEFS_PLAYER_SKILL:
        if (iSet)
        {
            ud.player_skill = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.player_skill, g_i, g_p);
        return;

    case USERDEFS_LEVEL_NUMBER:
        if (iSet)
        {
            ud.level_number = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.level_number, g_i, g_p);
        return;

    case USERDEFS_VOLUME_NUMBER:
        if (iSet)
        {
            ud.volume_number = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.volume_number, g_i, g_p);
        return;

    case USERDEFS_M_MARKER:
        if (iSet)
        {
            ud.m_marker = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_marker, g_i, g_p);
        return;

    case USERDEFS_MARKER:
        if (iSet)
        {
            ud.marker = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.marker, g_i, g_p);
        return;

    case USERDEFS_MOUSEFLIP:
        if (iSet)
        {
            ud.mouseflip = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.mouseflip, g_i, g_p);
        return;

    case USERDEFS_STATUSBARSCALE:
        if (iSet)
        {
            ud.statusbarscale = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.statusbarscale, g_i, g_p);
        return;

    case USERDEFS_DRAWWEAPON:
        if (iSet)
        {
            ud.drawweapon = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.drawweapon, g_i, g_p);
        return;

    case USERDEFS_MOUSEAIMING:
        if (iSet)
        {
            ud.mouseaiming = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.mouseaiming, g_i, g_p);
        return;

    case USERDEFS_WEAPONSWITCH:
        if (iSet)
        {
            ud.weaponswitch = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.weaponswitch, g_i, g_p);
        return;

    case USERDEFS_DEMOCAMS:
        if (iSet)
        {
            ud.democams = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.democams, g_i, g_p);
        return;

    case USERDEFS_COLOR:
        if (iSet)
        {
            ud.color = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.color, g_i, g_p);
        return;

    case USERDEFS_MSGDISPTIME:
        if (iSet)
        {
            ud.msgdisptime = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.msgdisptime, g_i, g_p);
        return;

    case USERDEFS_STATUSBARMODE:
        if (iSet)
        {
            ud.statusbarmode = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.statusbarmode, g_i, g_p);
        return;

    case USERDEFS_M_NOEXITS:
        if (iSet)
        {
            ud.m_noexits = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.m_noexits, g_i, g_p);
        return;

    case USERDEFS_NOEXITS:
        if (iSet)
        {
            ud.noexits = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.noexits, g_i, g_p);
        return;

    case USERDEFS_AUTOVOTE:
        if (iSet)
        {
            ud.autovote = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.autovote, g_i, g_p);
        return;

    case USERDEFS_AUTOMSG:
        if (iSet)
        {
            ud.automsg = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.automsg, g_i, g_p);
        return;

    case USERDEFS_IDPLAYERS:
        if (iSet)
        {
            ud.idplayers = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.idplayers, g_i, g_p);
        return;

    case USERDEFS_TEAM:
        if (iSet)
        {
            ud.team = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.team, g_i, g_p);
        return;

    case USERDEFS_VIEWBOB:
        if (iSet)
        {
            ud.viewbob = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.viewbob, g_i, g_p);
        return;

    case USERDEFS_WEAPONSWAY:
        if (iSet)
        {
            ud.weaponsway = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.weaponsway, g_i, g_p);
        return;

    case USERDEFS_ANGLEINTERPOLATION:
        if (iSet)
        {
            ud.angleinterpolation = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.angleinterpolation, g_i, g_p);
        return;

    case USERDEFS_OBITUARIES:
        if (iSet)
        {
            ud.obituaries = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.obituaries, g_i, g_p);
        return;

    case USERDEFS_LEVELSTATS:
        if (iSet)
        {
            ud.levelstats = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.levelstats, g_i, g_p);
        return;

    case USERDEFS_CROSSHAIRSCALE:
        if (iSet)
        {
            ud.crosshairscale = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.crosshairscale, g_i, g_p);
        return;

    case USERDEFS_ALTHUD:
        if (iSet)
        {
            ud.althud = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.althud, g_i, g_p);
        return;

    case USERDEFS_DISPLAY_BONUS_SCREEN:
        if (iSet)
        {
            ud.display_bonus_screen = lValue;
            return;
        }
        SetGameVarID(lVar2, ud.display_bonus_screen, g_i, g_p);
        return;

    default:
        return;
    }
}

static void DoThisProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0,proj=g_i;

    if (lVar1 != g_iThisActorID)
        proj=GetGameVarID(lVar1, g_i, g_p);

    if ((proj < 0 || proj >= MAXSPRITES) && checkCON)
    {
//        OSD_Printf("DoThisProjectile(): invalid projectile (%d)\n",proj);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target projectile (%d) %d %d from %s\n",line_num,keyw[g_tw],
                   iSet?"set":"get",projectilelabels[lLabelID].name,proj,g_i,g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            hittype[proj].projectile.workslike=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.workslike, g_i, g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            hittype[proj].projectile.spawns=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.spawns, g_i, g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.sxrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.sxrepeat, g_i, g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.syrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.syrepeat, g_i, g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            hittype[proj].projectile.sound=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.sound, g_i, g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            hittype[proj].projectile.isound=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.isound, g_i, g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            hittype[proj].projectile.vel=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.vel, g_i, g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            hittype[proj].projectile.extra=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.extra, g_i, g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            hittype[proj].projectile.decal=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.decal, g_i, g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            hittype[proj].projectile.trail=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.trail, g_i, g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.txrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.txrepeat, g_i, g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.tyrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.tyrepeat, g_i, g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            hittype[proj].projectile.toffset=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.toffset, g_i, g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            hittype[proj].projectile.tnum=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.tnum, g_i, g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            hittype[proj].projectile.drop=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.drop, g_i, g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            hittype[proj].projectile.cstat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.cstat, g_i, g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            hittype[proj].projectile.clipdist=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.clipdist, g_i, g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            hittype[proj].projectile.shade=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.shade, g_i, g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.xrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.xrepeat, g_i, g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            hittype[proj].projectile.yrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.yrepeat, g_i, g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            hittype[proj].projectile.pal=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.pal, g_i, g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            hittype[proj].projectile.extra_rand=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.extra_rand, g_i, g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            hittype[proj].projectile.hitradius=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.hitradius, g_i, g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            hittype[proj].projectile.velmult=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.velmult, g_i, g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            hittype[proj].projectile.offset=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.offset, g_i, g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            hittype[proj].projectile.bounces=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.bounces, g_i, g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            hittype[proj].projectile.bsound=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.bsound, g_i, g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            hittype[proj].projectile.range=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[proj].projectile.range, g_i, g_p);
        return;

    default:
        return;
    }
}

static void DoPlayer(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    int lValue=0;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=GetGameVarID(lVar1, g_i, g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && checkCON)
    {
//        OSD_Printf("DoPlayer(): invalid target player (%d) %d\n",iPlayer,g_i);
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target player (%d) from spr %d gv %s\n",line_num,keyw[g_tw],
                   iSet?"set":"get",playerlabels[lLabelID].name,iPlayer,g_i,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((playerlabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= playerlabels[lLabelID].maxParm2)) && checkCON)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on player (%d) from spr %d\n",line_num,keyw[g_tw],
                   iSet?"set":"get",playerlabels[lLabelID].name,lParm2,iPlayer,g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        if (iSet)
        {
            g_player[iPlayer].ps->zoom=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->zoom, g_i, g_p);
        return;

    case PLAYER_EXITX:
        if (iSet)
        {
            g_player[iPlayer].ps->exitx=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->exitx, g_i, g_p);
        return;

    case PLAYER_EXITY:
        if (iSet)
        {
            g_player[iPlayer].ps->exity=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->exity, g_i, g_p);
        return;

    case PLAYER_LOOGIEX:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiex[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->loogiex[lParm2], g_i, g_p);
        return;

    case PLAYER_LOOGIEY:
        if (iSet)
        {
            g_player[iPlayer].ps->loogiey[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->loogiey[lParm2], g_i, g_p);
        return;

    case PLAYER_NUMLOOGS:
        if (iSet)
        {
            g_player[iPlayer].ps->numloogs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->numloogs, g_i, g_p);
        return;

    case PLAYER_LOOGCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->loogcnt=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->loogcnt, g_i, g_p);
        return;

    case PLAYER_POSX:
        if (iSet)
        {
            g_player[iPlayer].ps->posx=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->posx, g_i, g_p);
        return;

    case PLAYER_POSY:
        if (iSet)
        {
            g_player[iPlayer].ps->posy=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->posy, g_i, g_p);
        return;

    case PLAYER_POSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->posz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->posz, g_i, g_p);
        return;

    case PLAYER_HORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->horiz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->horiz, g_i, g_p);
        return;

    case PLAYER_OHORIZ:
        if (iSet)
        {
            g_player[iPlayer].ps->ohoriz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->ohoriz, g_i, g_p);
        return;

    case PLAYER_OHORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->ohorizoff=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->ohorizoff, g_i, g_p);
        return;

    case PLAYER_INVDISPTIME:
        if (iSet)
        {
            g_player[iPlayer].ps->invdisptime=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->invdisptime, g_i, g_p);
        return;

    case PLAYER_BOBPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposx=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->bobposx, g_i, g_p);
        return;

    case PLAYER_BOBPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->bobposy=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->bobposy, g_i, g_p);
        return;

    case PLAYER_OPOSX:
        if (iSet)
        {
            g_player[iPlayer].ps->oposx=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->oposx, g_i, g_p);
        return;

    case PLAYER_OPOSY:
        if (iSet)
        {
            g_player[iPlayer].ps->oposy=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->oposy, g_i, g_p);
        return;

    case PLAYER_OPOSZ:
        if (iSet)
        {
            g_player[iPlayer].ps->oposz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->oposz, g_i, g_p);
        return;

    case PLAYER_PYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->pyoff=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->pyoff, g_i, g_p);
        return;

    case PLAYER_OPYOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->opyoff=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->opyoff, g_i, g_p);
        return;

    case PLAYER_POSXV:
        if (iSet)
        {
            g_player[iPlayer].ps->posxv=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->posxv, g_i, g_p);
        return;

    case PLAYER_POSYV:
        if (iSet)
        {
            g_player[iPlayer].ps->posyv=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->posyv, g_i, g_p);
        return;

    case PLAYER_POSZV:
        if (iSet)
        {
            g_player[iPlayer].ps->poszv=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->poszv, g_i, g_p);
        return;

    case PLAYER_LAST_PISSED_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->last_pissed_time=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->last_pissed_time, g_i, g_p);
        return;

    case PLAYER_TRUEFZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truefz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->truefz, g_i, g_p);
        return;

    case PLAYER_TRUECZ:
        if (iSet)
        {
            g_player[iPlayer].ps->truecz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->truecz, g_i, g_p);
        return;

    case PLAYER_PLAYER_PAR:
        if (iSet)
        {
            g_player[iPlayer].ps->player_par=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->player_par, g_i, g_p);
        return;

    case PLAYER_VISIBILITY:
        if (iSet)
        {
            g_player[iPlayer].ps->visibility=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->visibility, g_i, g_p);
        return;

    case PLAYER_BOBCOUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->bobcounter=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->bobcounter, g_i, g_p);
        return;

    case PLAYER_WEAPON_SWAY:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_sway=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weapon_sway, g_i, g_p);
        return;

    case PLAYER_PALS_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->pals_time=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->pals_time, g_i, g_p);
        return;

    case PLAYER_RANDOMFLAMEX:
        if (iSet)
        {
            g_player[iPlayer].ps->randomflamex=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->randomflamex, g_i, g_p);
        return;

    case PLAYER_CRACK_TIME:
        if (iSet)
        {
            g_player[iPlayer].ps->crack_time=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->crack_time, g_i, g_p);
        return;

    case PLAYER_AIM_MODE:
        if (iSet)
        {
            g_player[iPlayer].ps->aim_mode=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->aim_mode, g_i, g_p);
        return;

    case PLAYER_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->ang=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->ang, g_i, g_p);
        return;

    case PLAYER_OANG:
        if (iSet)
        {
            g_player[iPlayer].ps->oang=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->oang, g_i, g_p);
        return;

    case PLAYER_ANGVEL:
        if (iSet)
        {
            g_player[iPlayer].ps->angvel=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->angvel, g_i, g_p);
        return;

    case PLAYER_CURSECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->cursectnum=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->cursectnum, g_i, g_p);
        return;

    case PLAYER_LOOK_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->look_ang=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->look_ang, g_i, g_p);
        return;

    case PLAYER_LAST_EXTRA:
        if (iSet)
        {
            g_player[iPlayer].ps->last_extra=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->last_extra, g_i, g_p);
        return;

    case PLAYER_SUBWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->subweapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->subweapon, g_i, g_p);
        return;

    case PLAYER_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->ammo_amount[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->ammo_amount[lParm2], g_i, g_p);
        return;

    case PLAYER_WACKEDBYACTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->wackedbyactor=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->wackedbyactor, g_i, g_p);
        return;

    case PLAYER_FRAG:
        if (iSet)
        {
            g_player[iPlayer].ps->frag=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->frag, g_i, g_p);
        return;

    case PLAYER_FRAGGEDSELF:
        if (iSet)
        {
            g_player[iPlayer].ps->fraggedself=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->fraggedself, g_i, g_p);
        return;

    case PLAYER_CURR_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->curr_weapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->curr_weapon, g_i, g_p);
        return;

    case PLAYER_LAST_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_weapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->last_weapon, g_i, g_p);
        return;

    case PLAYER_TIPINCS:
        if (iSet)
        {
            g_player[iPlayer].ps->tipincs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->tipincs, g_i, g_p);
        return;

    case PLAYER_HORIZOFF:
        if (iSet)
        {
            g_player[iPlayer].ps->horizoff=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->horizoff, g_i, g_p);
        return;

    case PLAYER_WANTWEAPONFIRE:
        if (iSet)
        {
            g_player[iPlayer].ps->wantweaponfire=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->wantweaponfire, g_i, g_p);
        return;

    case PLAYER_HOLODUKE_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->holoduke_amount, g_i, g_p);
        return;

    case PLAYER_NEWOWNER:
        if (iSet)
        {
            g_player[iPlayer].ps->newowner=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->newowner, g_i, g_p);
        return;

    case PLAYER_HURT_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hurt_delay=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->hurt_delay, g_i, g_p);
        return;

    case PLAYER_HBOMB_HOLD_DELAY:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_hold_delay=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->hbomb_hold_delay, g_i, g_p);
        return;

    case PLAYER_JUMPING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_counter=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->jumping_counter, g_i, g_p);
        return;

    case PLAYER_AIRLEFT:
        if (iSet)
        {
            g_player[iPlayer].ps->airleft=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->airleft, g_i, g_p);
        return;

    case PLAYER_KNEE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knee_incs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->knee_incs, g_i, g_p);
        return;

    case PLAYER_ACCESS_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->access_incs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->access_incs, g_i, g_p);
        return;

    case PLAYER_FTA:
        if (iSet)
        {
            g_player[iPlayer].ps->fta=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->fta, g_i, g_p);
        return;

    case PLAYER_FTQ:
        if (iSet)
        {
            g_player[iPlayer].ps->ftq=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->ftq, g_i, g_p);
        return;

    case PLAYER_ACCESS_WALLNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_wallnum=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->access_wallnum, g_i, g_p);
        return;

    case PLAYER_ACCESS_SPRITENUM:
        if (iSet)
        {
            g_player[iPlayer].ps->access_spritenum=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->access_spritenum, g_i, g_p);
        return;

    case PLAYER_KICKBACK_PIC:
        if (iSet)
        {
            g_player[iPlayer].ps->kickback_pic=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->kickback_pic, g_i, g_p);
        return;

    case PLAYER_GOT_ACCESS:
        if (iSet)
        {
            g_player[iPlayer].ps->got_access=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->got_access, g_i, g_p);
        return;

    case PLAYER_WEAPON_ANG:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_ang=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weapon_ang, g_i, g_p);
        return;

    case PLAYER_FIRSTAID_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->firstaid_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->firstaid_amount, g_i, g_p);
        return;

    case PLAYER_SOMETHINGONPLAYER:
        if (iSet)
        {
            g_player[iPlayer].ps->somethingonplayer=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->somethingonplayer, g_i, g_p);
        return;

    case PLAYER_ON_CRANE:
        if (iSet)
        {
            g_player[iPlayer].ps->on_crane=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->on_crane, g_i, g_p);
        return;

    case PLAYER_I:
        if (iSet)
        {
            g_player[iPlayer].ps->i=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->i, g_i, g_p);
        return;

    case PLAYER_ONE_PARALLAX_SECTNUM:
        if (iSet)
        {
            g_player[iPlayer].ps->one_parallax_sectnum=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->one_parallax_sectnum, g_i, g_p);
        return;

    case PLAYER_OVER_SHOULDER_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->over_shoulder_on=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->over_shoulder_on, g_i, g_p);
        return;

    case PLAYER_RANDOM_CLUB_FRAME:
        if (iSet)
        {
            g_player[iPlayer].ps->random_club_frame=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->random_club_frame, g_i, g_p);
        return;

    case PLAYER_FIST_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->fist_incs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->fist_incs, g_i, g_p);
        return;

    case PLAYER_ONE_EIGHTY_COUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->one_eighty_count=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->one_eighty_count, g_i, g_p);
        return;

    case PLAYER_CHEAT_PHASE:
        if (iSet)
        {
            g_player[iPlayer].ps->cheat_phase=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->cheat_phase, g_i, g_p);
        return;

    case PLAYER_DUMMYPLAYERSPRITE:
        if (iSet)
        {
            g_player[iPlayer].ps->dummyplayersprite=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->dummyplayersprite, g_i, g_p);
        return;

    case PLAYER_EXTRA_EXTRA8:
        if (iSet)
        {
            g_player[iPlayer].ps->extra_extra8=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->extra_extra8, g_i, g_p);
        return;

    case PLAYER_QUICK_KICK:
        if (iSet)
        {
            g_player[iPlayer].ps->quick_kick=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->quick_kick, g_i, g_p);
        return;

    case PLAYER_HEAT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->heat_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->heat_amount, g_i, g_p);
        return;

    case PLAYER_ACTORSQU:
        if (iSet)
        {
            g_player[iPlayer].ps->actorsqu=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->actorsqu, g_i, g_p);
        return;

    case PLAYER_TIMEBEFOREEXIT:
        if (iSet)
        {
            g_player[iPlayer].ps->timebeforeexit=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->timebeforeexit, g_i, g_p);
        return;

    case PLAYER_CUSTOMEXITSOUND:
        if (iSet)
        {
            g_player[iPlayer].ps->customexitsound=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->customexitsound, g_i, g_p);
        return;

    case PLAYER_WEAPRECS:
        if (iSet)
        {
            g_player[iPlayer].ps->weaprecs[15]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weaprecs[15], g_i, g_p);
        return;

    case PLAYER_WEAPRECCNT:
        if (iSet)
        {
            g_player[iPlayer].ps->weapreccnt=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weapreccnt, g_i, g_p);
        return;

    case PLAYER_INTERFACE_TOGGLE_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->interface_toggle_flag=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->interface_toggle_flag, g_i, g_p);
        return;

    case PLAYER_ROTSCRNANG:
        if (iSet)
        {
            g_player[iPlayer].ps->rotscrnang=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->rotscrnang, g_i, g_p);
        return;

    case PLAYER_DEAD_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->dead_flag=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->dead_flag, g_i, g_p);
        return;

    case PLAYER_SHOW_EMPTY_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->show_empty_weapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->show_empty_weapon, g_i, g_p);
        return;

    case PLAYER_SCUBA_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->scuba_amount, g_i, g_p);
        return;

    case PLAYER_JETPACK_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->jetpack_amount, g_i, g_p);
        return;

    case PLAYER_STEROIDS_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->steroids_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->steroids_amount, g_i, g_p);
        return;

    case PLAYER_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->shield_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->shield_amount, g_i, g_p);
        return;

    case PLAYER_HOLODUKE_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->holoduke_on=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->holoduke_on, g_i, g_p);
        return;

    case PLAYER_PYCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->pycount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->pycount, g_i, g_p);
        return;

    case PLAYER_WEAPON_POS:
        if (iSet)
        {
            g_player[iPlayer].ps->weapon_pos=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weapon_pos, g_i, g_p);
        return;

    case PLAYER_FRAG_PS:
        if (iSet)
        {
            g_player[iPlayer].ps->frag_ps=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->frag_ps, g_i, g_p);
        return;

    case PLAYER_TRANSPORTER_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->transporter_hold=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->transporter_hold, g_i, g_p);
        return;

    case PLAYER_LAST_FULL_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->last_full_weapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->last_full_weapon, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTSHADE:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintshade=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->footprintshade, g_i, g_p);
        return;

    case PLAYER_BOOT_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->boot_amount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->boot_amount, g_i, g_p);
        return;

    case PLAYER_SCREAM_VOICE:
        if (iSet)
        {
            g_player[iPlayer].ps->scream_voice=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->scream_voice, g_i, g_p);
        return;

    case PLAYER_GM:
        if (iSet)
        {
            g_player[iPlayer].ps->gm=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->gm, g_i, g_p);
        return;

    case PLAYER_ON_WARPING_SECTOR:
        if (iSet)
        {
            g_player[iPlayer].ps->on_warping_sector=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->on_warping_sector, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTCOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintcount=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->footprintcount, g_i, g_p);
        return;

    case PLAYER_HBOMB_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->hbomb_on=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->hbomb_on, g_i, g_p);
        return;

    case PLAYER_JUMPING_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->jumping_toggle=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->jumping_toggle, g_i, g_p);
        return;

    case PLAYER_RAPID_FIRE_HOLD:
        if (iSet)
        {
            g_player[iPlayer].ps->rapid_fire_hold=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->rapid_fire_hold, g_i, g_p);
        return;

    case PLAYER_ON_GROUND:
        if (iSet)
        {
            g_player[iPlayer].ps->on_ground=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->on_ground, g_i, g_p);
        return;

        /*    case PLAYER_NAME:
                if (iSet)
                {
                    g_player[iPlayer].ps->name[32]=lValue;
                    return;
                }
                SetGameVarID(lVar2, g_player[iPlayer].ps->name[32], g_i, g_p);
                return;*/

    case PLAYER_INVEN_ICON:
        if (iSet)
        {
            g_player[iPlayer].ps->inven_icon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->inven_icon, g_i, g_p);
        return;

    case PLAYER_BUTTONPALETTE:
        if (iSet)
        {
            g_player[iPlayer].ps->buttonpalette=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->buttonpalette, g_i, g_p);
        return;

    case PLAYER_JETPACK_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->jetpack_on=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->jetpack_on, g_i, g_p);
        return;

    case PLAYER_SPRITEBRIDGE:
        if (iSet)
        {
            g_player[iPlayer].ps->spritebridge=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->spritebridge, g_i, g_p);
        return;

    case PLAYER_LASTRANDOMSPOT:
        if (iSet)
        {
            g_player[iPlayer].ps->lastrandomspot=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->lastrandomspot, g_i, g_p);
        return;

    case PLAYER_SCUBA_ON:
        if (iSet)
        {
            g_player[iPlayer].ps->scuba_on=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->scuba_on, g_i, g_p);
        return;

    case PLAYER_FOOTPRINTPAL:
        if (iSet)
        {
            g_player[iPlayer].ps->footprintpal=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->footprintpal, g_i, g_p);
        return;

    case PLAYER_HEAT_ON:
        if (iSet)
        {
            {
                if (g_player[iPlayer].ps->heat_on != lValue)
                {
                    g_player[iPlayer].ps->heat_on=lValue;
                    setpal(g_player[iPlayer].ps);
                }
            }
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->heat_on, g_i, g_p);
        return;

    case PLAYER_HOLSTER_WEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->holster_weapon=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->holster_weapon, g_i, g_p);
        return;

    case PLAYER_FALLING_COUNTER:
        if (iSet)
        {
            g_player[iPlayer].ps->falling_counter=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->falling_counter, g_i, g_p);
        return;

    case PLAYER_GOTWEAPON:
        if (iSet)
        {
            g_player[iPlayer].ps->gotweapon[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->gotweapon[lParm2], g_i, g_p);
        return;

    case PLAYER_REFRESH_INVENTORY:
        if (iSet)
        {
            g_player[iPlayer].ps->refresh_inventory=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->refresh_inventory, g_i, g_p);
        return;

        //      case PLAYER_PALETTE:
        //          if(iSet)
        //          {
        //              g_player[iPlayer].ps->palette=lValue;
        //          }
        //          return; }
        //          {
        //              SetGameVarID(lVar2, g_player[iPlayer].ps->palette, g_i, g_p);
        //          }
        //          return;

    case PLAYER_TOGGLE_KEY_FLAG:
        if (iSet)
        {
            g_player[iPlayer].ps->toggle_key_flag=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->toggle_key_flag, g_i, g_p);
        return;

    case PLAYER_KNUCKLE_INCS:
        if (iSet)
        {
            g_player[iPlayer].ps->knuckle_incs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->knuckle_incs, g_i, g_p);
        return;

    case PLAYER_WALKING_SND_TOGGLE:
        if (iSet)
        {
            g_player[iPlayer].ps->walking_snd_toggle=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->walking_snd_toggle, g_i, g_p);
        return;

    case PLAYER_PALOOKUP:
        if (iSet)
        {
            g_player[iPlayer].ps->palookup=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->palookup, g_i, g_p);
        return;

    case PLAYER_HARD_LANDING:
        if (iSet)
        {
            g_player[iPlayer].ps->hard_landing=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->hard_landing, g_i, g_p);
        return;

    case PLAYER_MAX_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->max_secret_rooms=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->max_secret_rooms, g_i, g_p);
        return;

    case PLAYER_SECRET_ROOMS:
        if (iSet)
        {
            g_player[iPlayer].ps->secret_rooms=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->secret_rooms, g_i, g_p);
        return;

    case PLAYER_PALS:
        if (iSet)
        {
            g_player[iPlayer].ps->pals[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->pals[lParm2], g_i, g_p);
        return;

    case PLAYER_MAX_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->max_actors_killed=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->max_actors_killed, g_i, g_p);
        return;

    case PLAYER_ACTORS_KILLED:
        if (iSet)
        {
            g_player[iPlayer].ps->actors_killed=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->actors_killed, g_i, g_p);
        return;

    case PLAYER_RETURN_TO_CENTER:
        if (iSet)
        {
            g_player[iPlayer].ps->return_to_center=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->return_to_center, g_i, g_p);
        return;

    case PLAYER_RUNSPEED:
        if (iSet)
        {
            g_player[iPlayer].ps->runspeed=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->runspeed, g_i, g_p);
        return;

    case PLAYER_SBS:
        if (iSet)
        {
            g_player[iPlayer].ps->sbs=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->sbs, g_i, g_p);
        return;

    case PLAYER_RELOADING:
        if (iSet)
        {
            g_player[iPlayer].ps->reloading=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->reloading, g_i, g_p);
        return;

    case PLAYER_AUTO_AIM:
        if (iSet)
        {
            g_player[iPlayer].ps->auto_aim=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->auto_aim, g_i, g_p);
        return;

    case PLAYER_MOVEMENT_LOCK:
        if (iSet)
        {
            g_player[iPlayer].ps->movement_lock=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->movement_lock, g_i, g_p);
        return;

    case PLAYER_SOUND_PITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->sound_pitch=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->sound_pitch, g_i, g_p);
        return;

    case PLAYER_WEAPONSWITCH:
        if (iSet)
        {
            g_player[iPlayer].ps->weaponswitch=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->weaponswitch, g_i, g_p);
        return;

    case PLAYER_TEAM:
        if (iSet)
        {
            g_player[iPlayer].ps->team=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->team, g_i, g_p);
        return;

    case PLAYER_MAX_PLAYER_HEALTH:
        if (iSet)
        {
            g_player[iPlayer].ps->max_player_health = lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->max_player_health, g_i, g_p);
        return;

    case PLAYER_MAX_SHIELD_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_shield_amount = lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->max_shield_amount, g_i, g_p);
        return;

    case PLAYER_MAX_AMMO_AMOUNT:
        if (iSet)
        {
            g_player[iPlayer].ps->max_ammo_amount[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].ps->max_ammo_amount[lParm2], g_i, g_p);
        return;

    default:
        return;
    }
}

static void DoInput(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iPlayer=g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=GetGameVarID(lVar1, g_i, g_p);

    if ((iPlayer<0 || iPlayer >= ud.multimode) && checkCON)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "invalid target player (%d) %d\n",line_num,keyw[g_tw],iPlayer,g_i);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case INPUT_AVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->avel=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->avel, g_i, g_p);
        return;

    case INPUT_HORZ:
        if (iSet)
        {
            g_player[iPlayer].sync->horz=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->horz, g_i, g_p);
        return;

    case INPUT_FVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->fvel=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->fvel, g_i, g_p);
        return;

    case INPUT_SVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->svel=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->svel, g_i, g_p);
        return;

    case INPUT_BITS:
        if (iSet)
        {
            g_player[iPlayer].sync->bits=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->bits, g_i, g_p);
        return;

    case INPUT_EXTBITS:
        if (iSet)
        {
            g_player[iPlayer].sync->extbits=lValue;
            return;
        }
        SetGameVarID(lVar2, g_player[iPlayer].sync->extbits, g_i, g_p);
        return;
    default:
        return;
    }
}

static void DoWall(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iWall = GetGameVarID(lVar1, g_i, g_p);

    if ((iWall<0 || iWall >= numwalls) && checkCON)
    {
        insptr += (lVar2 == MAXGAMEVARS);
        OSD_Printf(CON_ERROR "Invalid wall %d\n",line_num,keyw[g_tw],iWall);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case WALL_X:
        if (iSet)
        {
            wall[iWall].x=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].x, g_i, g_p);
        return;

    case WALL_Y:
        if (iSet)
        {
            wall[iWall].y=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].y, g_i, g_p);
        return;

    case WALL_POINT2:
        if (iSet)
        {
            wall[iWall].point2=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].point2, g_i, g_p);
        return;

    case WALL_NEXTWALL:
        if (iSet)
        {
            wall[iWall].nextwall=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].nextwall, g_i, g_p);
        return;

    case WALL_NEXTSECTOR:
        if (iSet)
        {
            wall[iWall].nextsector=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].nextsector, g_i, g_p);
        return;

    case WALL_CSTAT:
        if (iSet)
        {
            wall[iWall].cstat=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].cstat, g_i, g_p);
        return;

    case WALL_PICNUM:
        if (iSet)
        {
            wall[iWall].picnum=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].picnum, g_i, g_p);
        return;

    case WALL_OVERPICNUM:
        if (iSet)
        {
            wall[iWall].overpicnum=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].overpicnum, g_i, g_p);
        return;

    case WALL_SHADE:
        if (iSet)
        {
            wall[iWall].shade=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].shade, g_i, g_p);
        return;

    case WALL_PAL:
        if (iSet)
        {
            wall[iWall].pal=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].pal, g_i, g_p);
        return;

    case WALL_XREPEAT:
        if (iSet)
        {
            wall[iWall].xrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].xrepeat, g_i, g_p);
        return;

    case WALL_YREPEAT:
        if (iSet)
        {
            wall[iWall].yrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].yrepeat, g_i, g_p);
        return;

    case WALL_XPANNING:
        if (iSet)
        {
            wall[iWall].xpanning=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].xpanning, g_i, g_p);
        return;

    case WALL_YPANNING:
        if (iSet)
        {
            wall[iWall].ypanning=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].ypanning, g_i, g_p);
        return;

    case WALL_LOTAG:
        if (iSet)
        {
            wall[iWall].lotag=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].lotag, g_i, g_p);
        return;

    case WALL_HITAG:
        if (iSet)
        {
            wall[iWall].hitag=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].hitag, g_i, g_p);
        return;

    case WALL_EXTRA:
        if (iSet)
        {
            wall[iWall].extra=lValue;
            return;
        }
        SetGameVarID(lVar2, wall[iWall].extra, g_i, g_p);
        return;
    default:
        return;
    }
}

static void DoSector(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iSector=sprite[g_i].sectnum;

    if (lVar1 != g_iThisActorID)
        iSector=GetGameVarID(lVar1, g_i, g_p);

    if ((iSector<0 || iSector >= numsectors) && checkCON)
    {
        OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],iSector);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case SECTOR_WALLPTR:
        if (iSet)
        {
            sector[iSector].wallptr=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].wallptr,g_i,g_p);
        return;

    case SECTOR_WALLNUM:
        if (iSet)
        {
            sector[iSector].wallnum=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].wallnum,g_i,g_p);
        return;

    case SECTOR_CEILINGZ:
        if (iSet)
        {
            sector[iSector].ceilingz=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingz,g_i,g_p);
        return;

    case SECTOR_FLOORZ:
        if (iSet)
        {
            sector[iSector].floorz=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorz,g_i,g_p);
        return;

    case SECTOR_CEILINGSTAT:
        if (iSet)
        {
            sector[iSector].ceilingstat=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingstat,g_i,g_p);
        return;

    case SECTOR_FLOORSTAT:
        if (iSet)
        {
            sector[iSector].floorstat=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorstat,g_i,g_p);
        return;

    case SECTOR_CEILINGPICNUM:
        if (iSet)
        {
            sector[iSector].ceilingpicnum=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingpicnum,g_i,g_p);
        return;

    case SECTOR_CEILINGSLOPE:
        if (iSet)
        {
            sector[iSector].ceilingheinum=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingheinum,g_i,g_p);
        return;

    case SECTOR_CEILINGSHADE:
        if (iSet)
        {
            sector[iSector].ceilingshade=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingshade,g_i,g_p);
        return;

    case SECTOR_CEILINGPAL:
        if (iSet)
        {
            sector[iSector].ceilingpal=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingpal,g_i,g_p);
        return;

    case SECTOR_CEILINGXPANNING:
        if (iSet)
        {
            sector[iSector].ceilingxpanning=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingxpanning,g_i,g_p);
        return;

    case SECTOR_CEILINGYPANNING:
        if (iSet)
        {
            sector[iSector].ceilingypanning=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].ceilingypanning,g_i,g_p);
        return;

    case SECTOR_FLOORPICNUM:
        if (iSet)
        {
            sector[iSector].floorpicnum=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorpicnum,g_i,g_p);
        return;

    case SECTOR_FLOORSLOPE:
        if (iSet)
        {
            sector[iSector].floorheinum=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorheinum,g_i,g_p);
        return;

    case SECTOR_FLOORSHADE:
        if (iSet)
        {
            sector[iSector].floorshade=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorshade,g_i,g_p);
        return;

    case SECTOR_FLOORPAL:
        if (iSet)
        {
            sector[iSector].floorpal=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorpal,g_i,g_p);
        return;

    case SECTOR_FLOORXPANNING:
        if (iSet)
        {
            sector[iSector].floorxpanning=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorxpanning,g_i,g_p);
        return;

    case SECTOR_FLOORYPANNING:
        if (iSet)
        {
            sector[iSector].floorypanning=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].floorypanning,g_i,g_p);
        return;

    case SECTOR_VISIBILITY:
        if (iSet)
        {
            sector[iSector].visibility=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].visibility,g_i,g_p);
        return;

    case SECTOR_ALIGNTO:
        if (iSet)
        {
            sector[iSector].filler=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].filler,g_i,g_p);
        return;

    case SECTOR_LOTAG:
        if (iSet)
        {
            sector[iSector].lotag=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].lotag,g_i,g_p);
        return;

    case SECTOR_HITAG:
        if (iSet)
        {
            sector[iSector].hitag=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].hitag,g_i,g_p);
        return;

    case SECTOR_EXTRA:
        if (iSet)
        {
            sector[iSector].extra=lValue;
            return;
        }
        SetGameVarID(lVar2, sector[iSector].extra,g_i,g_p);
        return;

    default:
        return;
    }
}

static void DoActor(int iSet, int lVar1, int lLabelID, int lVar2, int lParm2)
{
    int lValue=0;
    int iActor=g_i;

    if (lVar1 != g_iThisActorID)
        iActor=GetGameVarID(lVar1, g_i, g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && checkCON)
    {
        OSD_Printf(CON_ERROR "tried to %s %s on invalid target sprite (%d) from spr %d pic %d gv %s\n",line_num,keyw[g_tw],
                   iSet?"set":"get",actorlabels[lLabelID].name,iActor,g_i,g_sp->picnum,
                   (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if ((actorlabels[lLabelID].flags & LABEL_HASPARM2 && (lParm2 < 0 || lParm2 >= actorlabels[lLabelID].maxParm2)) && checkCON)
    {
        OSD_Printf(CON_ERROR "tried to %s invalid %s position %d on sprite (%d) from spr %d\n",line_num,keyw[g_tw],
                   iSet?"set":"get",actorlabels[lLabelID].name,lParm2,iActor,g_i);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
        {
            sprite[iActor].x=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].x,g_i,g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            sprite[iActor].y=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].y,g_i,g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            sprite[iActor].z=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].z,g_i,g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            sprite[iActor].cstat=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].cstat,g_i,g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            sprite[iActor].picnum=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].picnum,g_i,g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            sprite[iActor].shade=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].shade,g_i,g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            sprite[iActor].pal=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].pal,g_i,g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            sprite[iActor].clipdist=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].clipdist,g_i,g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            sprite[iActor].filler=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].filler,g_i,g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            sprite[iActor].xrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].xrepeat,g_i,g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            sprite[iActor].yrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].yrepeat,g_i,g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            sprite[iActor].xoffset=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].xoffset,g_i,g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            sprite[iActor].yoffset=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].yoffset,g_i,g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            changespritesect(iActor,lValue);
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].sectnum,g_i,g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            changespritestat(iActor,lValue);
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].statnum,g_i,g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            sprite[iActor].ang=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].ang,g_i,g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            sprite[iActor].owner=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].owner,g_i,g_p);
        return;

    case ACTOR_XVEL:
        if (iSet)
        {
            sprite[iActor].xvel=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].xvel,g_i,g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            sprite[iActor].yvel=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].yvel,g_i,g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            sprite[iActor].zvel=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].zvel,g_i,g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            sprite[iActor].lotag=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].lotag,g_i,g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            sprite[iActor].hitag=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].hitag,g_i,g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            sprite[iActor].extra=lValue;
            return;
        }
        SetGameVarID(lVar2, sprite[iActor].extra,g_i,g_p);
        return;

    case ACTOR_HTCGG:
        if (iSet)
        {
            hittype[iActor].cgg=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[iActor].cgg, g_i, g_p);
        return;

    case ACTOR_HTPICNUM :
        if (iSet)
        {
            hittype[iActor].picnum=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[iActor].picnum, g_i, g_p);
        return;

    case ACTOR_HTANG:
        if (iSet)
        {
            hittype[iActor].ang=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[iActor].ang, g_i, g_p);
        return;

    case ACTOR_HTEXTRA:
        if (iSet)
        {
            hittype[iActor].extra=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].extra, g_i, g_p);
        return;

    case ACTOR_HTOWNER:
        if (iSet)
        {
            hittype[iActor].owner=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].owner, g_i, g_p);
        return;

    case ACTOR_HTMOVFLAG:
        if (iSet)
        {
            hittype[iActor].movflag=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].movflag, g_i, g_p);
        return;

    case ACTOR_HTTEMPANG:
        if (iSet)
        {
            hittype[iActor].tempang=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].tempang, g_i, g_p);
        return;

    case ACTOR_HTACTORSTAYPUT:
        if (iSet)
        {
            hittype[iActor].actorstayput=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].actorstayput, g_i, g_p);
        return;

    case ACTOR_HTDISPICNUM:
        if (iSet)
        {
            hittype[iActor].dispicnum=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].dispicnum, g_i, g_p);
        return;

    case ACTOR_HTTIMETOSLEEP:
        if (iSet)
        {
            hittype[iActor].timetosleep=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].timetosleep, g_i, g_p);
        return;

    case ACTOR_HTFLOORZ:
        if (iSet)
        {
            hittype[iActor].floorz=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].floorz, g_i, g_p);
        return;

    case ACTOR_HTCEILINGZ:
        if (iSet)
        {
            hittype[iActor].ceilingz=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].ceilingz, g_i, g_p);
        return;

    case ACTOR_HTLASTVX:
        if (iSet)
        {
            hittype[iActor].lastvx=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].lastvx, g_i, g_p);
        return;

    case ACTOR_HTLASTVY:
        if (iSet)
        {
            hittype[iActor].lastvy=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].lastvy, g_i, g_p);
        return;

    case ACTOR_HTBPOSX:
        if (iSet)
        {
            hittype[iActor].bposx=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].bposx, g_i, g_p);
        return;

    case ACTOR_HTBPOSY:
        if (iSet)
        {
            hittype[iActor].bposy=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].bposy, g_i, g_p);
        return;

    case ACTOR_HTBPOSZ:
        if (iSet)
        {
            hittype[iActor].bposz=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].bposz, g_i, g_p);
        return;

    case ACTOR_HTG_T:
        if (iSet)
        {
            hittype[iActor].temp_data[lParm2]=lValue;
            return;
        }
        SetGameVarID(lVar2, hittype[iActor].temp_data[lParm2], g_i, g_p);
        return;

    case ACTOR_ANGOFF:
        if (iSet)
        {
            spriteext[iActor].angoff=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].angoff, g_i, g_p);
        return;

    case ACTOR_PITCH:
        if (iSet)
        {
            spriteext[iActor].pitch=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].pitch, g_i, g_p);
        return;

    case ACTOR_ROLL:
        if (iSet)
        {
            spriteext[iActor].roll=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].roll, g_i, g_p);
        return;

    case ACTOR_MDXOFF:
        if (iSet)
        {
            spriteext[iActor].xoff=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].xoff, g_i, g_p);
        return;

    case ACTOR_MDYOFF:
        if (iSet)
        {
            spriteext[iActor].yoff=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].yoff, g_i, g_p);
        return;

    case ACTOR_MDZOFF:
        if (iSet)
        {
            spriteext[iActor].zoff=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].zoff, g_i, g_p);
        return;

    case ACTOR_MDFLAGS:
        if (iSet)
        {
            spriteext[iActor].flags=lValue;
            return;
        }
        SetGameVarID(lVar2,spriteext[iActor].flags, g_i, g_p);
        return;

    case ACTOR_XPANNING:
        if (iSet)
        {
            spriteext[iActor].xpanning=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].xpanning,g_i,g_p);
        return;

    case ACTOR_YPANNING:
        if (iSet)
        {
            spriteext[iActor].ypanning=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].ypanning,g_i,g_p);
        return;

    case ACTOR_HTFLAGS:
        if (iSet)
        {
            hittype[iActor].flags=lValue;
            return;
        }
        SetGameVarID(lVar2,hittype[iActor].flags, g_i, g_p);
        return;

    default:
        return;
    }
}

static void DoTsprite(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;
    int iActor=g_i;

    if (lVar1 != g_iThisActorID)
        iActor=GetGameVarID(lVar1, g_i, g_p);

    if ((iActor < 0 || iActor >= MAXSPRITES) && checkCON)
    {
        OSD_Printf(CON_ERROR "invalid target sprite (%d) %d %d\n",line_num,keyw[g_tw],iActor,g_i,g_sp->picnum);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    if ((!spriteext[iActor].tspr) && checkCON)
    {
        OSD_Printf(CON_ERROR "Internal bug, tsprite is unavailable\n",line_num,keyw[g_tw]);
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
        SetGameVarID(lVar2, spriteext[iActor].tspr->x,g_i,g_p);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            spriteext[iActor].tspr->y=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->y,g_i,g_p);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            spriteext[iActor].tspr->z=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->z,g_i,g_p);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            spriteext[iActor].tspr->cstat=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->cstat,g_i,g_p);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->picnum=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->picnum,g_i,g_p);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            spriteext[iActor].tspr->shade=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->shade,g_i,g_p);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            spriteext[iActor].tspr->pal=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->pal,g_i,g_p);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            spriteext[iActor].tspr->clipdist=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->clipdist,g_i,g_p);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            spriteext[iActor].tspr->filler=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->filler,g_i,g_p);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->xrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->xrepeat,g_i,g_p);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->yrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->yrepeat,g_i,g_p);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->xoffset=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->xoffset,g_i,g_p);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->yoffset=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->yoffset,g_i,g_p);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->sectnum=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->sectnum,g_i,g_p);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->statnum=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->statnum,g_i,g_p);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            spriteext[iActor].tspr->ang=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->ang,g_i,g_p);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            spriteext[iActor].tspr->owner=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->owner,g_i,g_p);
        return;

#if 1
    case ACTOR_XVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->xvel=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->xvel,g_i,g_p);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->yvel=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->yvel,g_i,g_p);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->zvel=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->zvel,g_i,g_p);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            spriteext[iActor].tspr->lotag=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->lotag,g_i,g_p);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            spriteext[iActor].tspr->hitag=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->hitag,g_i,g_p);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            spriteext[iActor].tspr->extra=lValue;
            return;
        }
        SetGameVarID(lVar2, spriteext[iActor].tspr->extra,g_i,g_p);
        return;
#endif

    default:
        return;
    }
}

static void DoProjectile(int iSet, int lVar1, int lLabelID, int lVar2)
{
    int lValue=0;

    if ((lVar1 < 0 || lVar1 >= MAXTILES) && checkCON)
    {
        OSD_Printf(CON_ERROR "invalid tile (%d)\n",line_num,keyw[g_tw],lVar1);
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=GetGameVarID(lVar2, g_i, g_p);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            projectile[lVar1].workslike=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].workslike, g_i, g_p);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            projectile[lVar1].spawns=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].spawns, g_i, g_p);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            projectile[lVar1].sxrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].sxrepeat, g_i, g_p);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            projectile[lVar1].syrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].syrepeat, g_i, g_p);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            projectile[lVar1].sound=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].sound, g_i, g_p);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            projectile[lVar1].isound=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].isound, g_i, g_p);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            projectile[lVar1].vel=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].vel, g_i, g_p);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            projectile[lVar1].extra=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].extra, g_i, g_p);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            projectile[lVar1].decal=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].decal, g_i, g_p);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            projectile[lVar1].trail=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].trail, g_i, g_p);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            projectile[lVar1].txrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].txrepeat, g_i, g_p);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            projectile[lVar1].tyrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].tyrepeat, g_i, g_p);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            projectile[lVar1].toffset=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].toffset, g_i, g_p);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            projectile[lVar1].tnum=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].tnum, g_i, g_p);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            projectile[lVar1].drop=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].drop, g_i, g_p);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            projectile[lVar1].cstat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].cstat, g_i, g_p);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            projectile[lVar1].clipdist=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].clipdist, g_i, g_p);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            projectile[lVar1].shade=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].shade, g_i, g_p);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            projectile[lVar1].xrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].xrepeat, g_i, g_p);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            projectile[lVar1].yrepeat=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].yrepeat, g_i, g_p);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            projectile[lVar1].pal=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].pal, g_i, g_p);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            projectile[lVar1].extra_rand=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].extra_rand, g_i, g_p);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            projectile[lVar1].hitradius=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].hitradius, g_i, g_p);
        return;

    case PROJ_VEL_MULT:
        if (iSet)
        {
            projectile[lVar1].velmult=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].velmult, g_i, g_p);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            projectile[lVar1].offset=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].offset, g_i, g_p);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            projectile[lVar1].bounces=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].bounces, g_i, g_p);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            projectile[lVar1].bsound=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].bsound, g_i, g_p);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            projectile[lVar1].range=lValue;
            return;
        }
        SetGameVarID(lVar2, projectile[lVar1].range, g_i, g_p);
        return;

    default:
        return;
    }
}

void OnEvent(int iEventID, int iActor, int iPlayer, int lDist)
{
    if ((iEventID<0 || iEventID >= MAXGAMEEVENTS) && checkCON)
    {
        OSD_Printf(CON_ERROR "invalid event ID",line_num,keyw[g_tw]);
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
        int og_x=g_x;// *og_t=g_t;
        intptr_t *oinsptr=insptr, *og_t=g_t;
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
                if (g_player[g_p].ps->actorsqu == g_i)
                    g_player[g_p].ps->actorsqu = -1;
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
        FTA(10,g_player[p].ps);

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

static void forceplayerangle(player_struct *p)
{
    int n = 128-(TRAND&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = n>>1;
    p->rotscrnang = n>>1;
}

static char dodge(spritetype *s)
{
    int bx,by,bxvect,byvect,d,i;
    int mx = s->x, my = s->y;
    int mxvect = sintable[(s->ang+512)&2047];
    int myvect = sintable[s->ang&2047];

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
    int hx, hy, hz, d;
    spritetype *s = &sprite[iActor];
    int greatestd = -(1<<30);
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

int furthestcanseepoint(int iActor,spritetype *ts,int *dax,int *day)
{
    short hitsect,hitwall,hitspr, angincs;
    int hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
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

        if (d < da && hitsect > -1)
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
        int hz,lz,zr = 127L;

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
    int hz,lz,c = gc;

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
    int ticselapsed = (g_t[0])&31;
    intptr_t *moveptr = (intptr_t *)g_t[1];
    int j = g_player[g_p].ps->holoduke_on;

    g_sp->xvel += (*moveptr-g_sp->xvel)/5;
    if (g_sp->zvel < 648) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)/5;

    if (a&seekplayer)
    {
        // NOTE: looks like 'owner' is set to target sprite ID...

        if (j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,g_sp->x,g_sp->y,g_sp->z,g_sp->sectnum))
            g_sp->owner = j;
        else g_sp->owner = g_player[g_p].ps->i;

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
            g_sp->owner = g_player[g_p].ps->i;
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
    int l;
    intptr_t *moveptr;
    int a = g_sp->hitag, goalang, angdif;
    int daxvel;

    if (a == -1) a = 0;

    g_t[0]++;

    if (a&face_player)
    {
        if (g_player[g_p].ps->newowner >= 0)
            goalang = getangle(g_player[g_p].ps->oposx-g_sp->x,g_player[g_p].ps->oposy-g_sp->y);
        else goalang = getangle(g_player[g_p].ps->posx-g_sp->x,g_player[g_p].ps->posy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
            angdif *= 2;
        g_sp->ang += angdif;
    }

    if (a&spin)
        g_sp->ang += sintable[((g_t[0]<<3)&2047)]>>6;

    if (a&face_player_slow)
    {
        if (g_player[g_p].ps->newowner >= 0)
            goalang = getangle(g_player[g_p].ps->oposx-g_sp->x,g_player[g_p].ps->oposy-g_sp->y);
        else goalang = getangle(g_player[g_p].ps->posx-g_sp->x,g_player[g_p].ps->posy-g_sp->y);
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
        int newx = g_player[g_p].ps->posx+(g_player[g_p].ps->posxv/768);
        int newy = g_player[g_p].ps->posy+(g_player[g_p].ps->posyv/768);

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

    moveptr = (intptr_t *)g_t[1];

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
                angdif = getangle(g_player[g_p].ps->posx-g_sp->x,g_player[g_p].ps->posy-g_sp->y);

                if (g_x < 512)
                {
                    g_player[g_p].ps->posxv = 0;
                    g_player[g_p].ps->posyv = 0;
                }
                else
                {
                    g_player[g_p].ps->posxv = mulscale(g_player[g_p].ps->posxv,g_player[g_p].ps->runspeed-0x2000,16);
                    g_player[g_p].ps->posyv = mulscale(g_player[g_p].ps->posyv,g_player[g_p].ps->runspeed-0x2000,16);
                }
            }
            else if (g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != COMMANDER)
            {
                if (hittype[g_i].bposz != g_sp->z || (ud.multimode < 2 && ud.player_skill < 2))
                {
                    if ((g_t[0]&1) || g_player[g_p].ps->actorsqu == g_i) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if ((g_t[0]&3) || g_player[g_p].ps->actorsqu == g_i) return;
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

static inline void parseifelse(int condition)
{
    if (condition)
    {
        // skip 'else' pointer.. and...
        insptr+=2;
        parse();
        return;
    }
    insptr = (intptr_t *) *(insptr+1);
    if (((*insptr)&0xFFF) == CON_ELSE)
    {
        // else...
        // skip 'else' and...
        insptr+=2;
        parse();
    }
}

// int *it = 0x00589a04;

static int parse(void)
{
    int j, l, s, tw = *insptr;

    if (killit_flag) return 1;

    //    if(*it == 1668249134L) gameexit("\nERR");
    //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
    //      AddLog(g_szBuf);

    line_num = tw>>12;
    g_tw = tw &= 0xFFF;

    switch (tw)
    {
    case CON_REDEFINEQUOTE:
        insptr++;
        {
            int q = *insptr++, i = *insptr++;
            if ((fta_quotes[q] == NULL || redefined_quotes[i] == NULL) && checkCON)
            {
                OSD_Printf(CON_ERROR "%s %d null quote\n",line_num,keyw[g_tw],q,i);
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
        j = cansee(g_sp->x,g_sp->y,g_sp->z-((TRAND&41)<<8),g_sp->sectnum,g_player[g_p].ps->posx,g_player[g_p].ps->posy,g_player[g_p].ps->posz/*-((TRAND&41)<<8)*/,sprite[g_player[g_p].ps->i].sectnum);
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
        if (g_player[g_p].ps->holoduke_on >= 0)
        {
            s = &sprite[g_player[g_p].ps->holoduke_on];
            j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((32<<8)-1)),g_sp->sectnum,
                       s->x,s->y,s->z,s->sectnum);

            if (j == 0)
            {
                // they can't see player's holoduke
                // check for player...
                s = &sprite[g_player[g_p].ps->i];
            }
        }
        else s = &sprite[g_player[g_p].ps->i];    // holoduke not on. look for player

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
        //Following changed to use pointersizes
        g_t[5] = *insptr++; // Ai
        g_t[4] = *(intptr_t *)(g_t[5]);       // Action
        g_t[1] = *(((intptr_t *)g_t[5])+1);       // move
        g_sp->hitag = *(((intptr_t *)g_t[5])+2);    // move flags
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
        insptr = (intptr_t *) *(insptr+1);
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
                for (j=0;j < g_player[g_p].ps->weapreccnt;j++)
                    if (g_player[g_p].ps->weaprecs[j] == g_sp->picnum)
                        break;

                parseifelse(j < g_player[g_p].ps->weapreccnt && g_sp->owner == g_i);
            }
            else if (g_player[g_p].ps->weapreccnt < 16)
            {
                g_player[g_p].ps->weaprecs[g_player[g_p].ps->weapreccnt++] = g_sp->picnum;
                parseifelse(g_sp->owner == g_i);
            }
            else parseifelse(0);
        }
        else parseifelse(0);
        break;

    case CON_GETLASTPAL:
        insptr++;
        if (g_sp->picnum == APLAYER)
            g_sp->pal = g_player[g_sp->yvel].ps->palookup;
        else g_sp->pal = hittype[g_i].tempang;
        hittype[g_i].tempang = 0;
        break;

    case CON_TOSSWEAPON:
        insptr++;
        checkweapons(g_player[g_sp->yvel].ps);
        break;

    case CON_NULLOP:
        insptr++;
        break;

    case CON_MIKESND:
        insptr++;
        if ((g_sp->yvel<0 || g_sp->yvel>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],g_sp->yvel);
            insptr++;
            break;
        }
        if (!isspritemakingsound(g_i,g_sp->yvel))
            spritesound(g_sp->yvel,g_i);
        break;

    case CON_PKICK:
        insptr++;

        if (ud.multimode > 1 && g_sp->picnum == APLAYER)
        {
            if (g_player[otherp].ps->quick_kick == 0)
                g_player[otherp].ps->quick_kick = 14;
        }
        else if (g_sp->picnum != APLAYER && g_player[g_p].ps->quick_kick == 0)
            g_player[g_p].ps->quick_kick = 14;
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
        if ((*insptr<0 || *insptr>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;break;
        }
        if (!isspritemakingsound(g_i,*insptr))
            spritesound((short) *insptr,g_i);
        insptr++;
        break;

    case CON_IFSOUND:
        insptr++;
        if ((*insptr<0 || *insptr>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;break;
        }
        parseifelse(isspritemakingsound(g_i,*insptr));
        //    parseifelse(SoundOwner[*insptr][0].i == g_i);
        break;

    case CON_STOPSOUND:
        insptr++;
        if ((*insptr<0 || *insptr>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;break;
        }
        if (isspritemakingsound(g_i,*insptr))
            stopspritesound((short)*insptr,g_i);
        insptr++;
        break;

    case CON_GLOBALSOUND:
        insptr++;
        if ((*insptr<0 || *insptr>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;break;
        }
        if (g_p == screenpeek || (gametype_flags[ud.coop]&GAMETYPE_FLAG_COOPSOUND))
            spritesound((short) *insptr,g_player[screenpeek].ps->i);
        insptr++;
        break;

    case CON_SOUND:
        insptr++;
        if ((*insptr<0 || *insptr>=MAXSOUNDS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;break;
        }
        spritesound((short) *insptr++,g_i);
        break;

    case CON_TIP:
        insptr++;
        g_player[g_p].ps->tipincs = 26;
        break;

    case CON_FALL:
        insptr++;
        g_sp->xoffset = g_sp->yoffset = 0;

        j = gc;

        if (ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
            j = gc/6;
        else if (floorspace(g_sp->sectnum))
            j = 0;

        if (!hittype[g_i].cgg-- || (sector[g_sp->sectnum].floorstat&2))
        {
            getglobalz(g_i);
            hittype[g_i].cgg = 3;
        }

        if (g_sp->z < (hittype[g_i].floorz-FOURSLEIGHT))
        {
            g_sp->z += g_sp->zvel += j;

            if (g_sp->zvel > 6144) g_sp->zvel = 6144;

            if (g_sp->z > (hittype[g_i].floorz - FOURSLEIGHT))
                g_sp->z = (hittype[g_i].floorz - FOURSLEIGHT);
            break;
        }
        g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

        if (badguy(g_sp) || (g_sp->picnum == APLAYER && g_sp->owner >= 0))
        {
            if (g_sp->zvel > 3084 && g_sp->extra <= 1)
            {
                if (!(g_sp->picnum == APLAYER && g_sp->extra > 0) && g_sp->pal != 1 && g_sp->picnum != DRONE)
                {
                    guts(g_i,JIBS6,15);
                    spritesound(SQUISHED,g_i);
                    spawn(g_i,BLOODPOOL);
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

        if (g_sp->z > (hittype[g_i].floorz - FOURSLEIGHT))
        {
            getglobalz(g_i);
            if (hittype[g_i].floorz != sector[g_sp->sectnum].floorz)
                g_sp->z = (hittype[g_i].floorz - FOURSLEIGHT);
            break;
        }
        else if (sector[g_sp->sectnum].lotag == 1)
        {
            switch (dynamictostatic[g_sp->picnum])
            {
            default:
                g_sp->z += (24<<8);
            case OCTABRAIN__STATIC:
            case COMMANDER__STATIC:
            case DRONE__STATIC:
                break;
            }
            break;
        }
        g_sp->zvel = 0;
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
        if ((*insptr<0 || *insptr>=MAX_WEAPONS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",line_num,keyw[g_tw],*insptr);
            insptr+=2;break;
        }
        if (g_player[g_p].ps->ammo_amount[*insptr] >= g_player[g_p].ps->max_ammo_amount[*insptr])
        {
            killit_flag = 2;
            break;
        }
        addammo(*insptr, g_player[g_p].ps, *(insptr+1));
        if (g_player[g_p].ps->curr_weapon == KNEE_WEAPON && g_player[g_p].ps->gotweapon[*insptr])
        {
            if (!(g_player[g_p].ps->weaponswitch & 1)) addweaponnoswitch(g_player[g_p].ps, *insptr);
            else addweapon(g_player[g_p].ps, *insptr);
        }
        insptr += 2;
        break;

    case CON_MONEY:
        insptr++;
        lotsofmoneymailpaper(g_i,*insptr++,MONEY);
        break;

    case CON_MAIL:
        insptr++;
        lotsofmoneymailpaper(g_i,*insptr++,MAIL);
        break;

    case CON_SLEEPTIME:
        insptr++;
        hittype[g_i].timetosleep = (short)*insptr++;
        break;

    case CON_PAPER:
        insptr++;
        lotsofmoneymailpaper(g_i,*insptr++,PAPER);
        break;

    case CON_ADDKILLS:
        insptr++;
        g_player[g_p].ps->actors_killed += *insptr++;
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
        if ((*insptr<0 ||*insptr>=MAX_WEAPONS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",line_num,keyw[g_tw],*insptr);
            insptr+=2;break;
        }
        if (g_player[g_p].ps->gotweapon[*insptr] == 0)
        {
            if (!(g_player[g_p].ps->weaponswitch & 1)) addweaponnoswitch(g_player[g_p].ps, *insptr);
            else addweapon(g_player[g_p].ps, *insptr);
        }
        else if (g_player[g_p].ps->ammo_amount[*insptr] >= g_player[g_p].ps->max_ammo_amount[*insptr])
        {
            killit_flag = 2;
            break;
        }
        addammo(*insptr, g_player[g_p].ps, *(insptr+1));
        if (g_player[g_p].ps->curr_weapon == KNEE_WEAPON && g_player[g_p].ps->gotweapon[*insptr])
        {
            if (!(g_player[g_p].ps->weaponswitch & 1)) addweaponnoswitch(g_player[g_p].ps, *insptr);
            else addweapon(g_player[g_p].ps, *insptr);
        }
        insptr+=2;
        break;

    case CON_DEBUG:
        insptr++;
        initprintf("%d\n",*insptr++);
        break;

    case CON_ENDOFGAME:
        insptr++;
        g_player[g_p].ps->timebeforeexit = *insptr++;
        g_player[g_p].ps->customexitsound = -1;
        ud.eog = 1;
        break;

    case CON_ADDPHEALTH:
        insptr++;

        if (g_player[g_p].ps->newowner >= 0)
        {
            g_player[g_p].ps->newowner = -1;
            g_player[g_p].ps->posx = g_player[g_p].ps->oposx;
            g_player[g_p].ps->posy = g_player[g_p].ps->oposy;
            g_player[g_p].ps->posz = g_player[g_p].ps->oposz;
            g_player[g_p].ps->ang = g_player[g_p].ps->oang;
            updatesector(g_player[g_p].ps->posx,g_player[g_p].ps->posy,&g_player[g_p].ps->cursectnum);
            setpal(g_player[g_p].ps);

            j = headspritestat[1];
            while (j >= 0)
            {
                if (sprite[j].picnum==CAMERA1)
                    sprite[j].yvel = 0;
                j = nextspritestat[j];
            }
        }

        j = sprite[g_player[g_p].ps->i].extra;

        if (g_sp->picnum != ATOMICHEALTH)
        {
            if (j > g_player[g_p].ps->max_player_health && *insptr > 0)
            {
                insptr++;
                break;
            }
            else
            {
                if (j > 0)
                    j += *insptr;
                if (j > g_player[g_p].ps->max_player_health && *insptr > 0)
                    j = g_player[g_p].ps->max_player_health;
            }
        }
        else
        {
            if (j > 0)
                j += *insptr;
            if (j > (g_player[g_p].ps->max_player_health<<1))
                j = (g_player[g_p].ps->max_player_health<<1);
        }

        if (j < 0) j = 0;

        if (ud.god == 0)
        {
            if (*insptr > 0)
            {
                if ((j - *insptr) < (g_player[g_p].ps->max_player_health>>2) &&
                        j >= (g_player[g_p].ps->max_player_health>>2))
                    spritesound(DUKE_GOTHEALTHATLOW,g_player[g_p].ps->i);

                g_player[g_p].ps->last_extra = j;
            }

            sprite[g_player[g_p].ps->i].extra = j;
        }

        insptr++;
        break;

    case CON_STATE:
    {
        intptr_t *tempscrptr=insptr+2;

        insptr = (intptr_t *) *(insptr+1);
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
        if (g_player[g_p].ps->gotweapon[GetGameVarID(*(insptr),g_i,g_p)] == 0)
        {
            if (!(g_player[g_p].ps->weaponswitch & 1)) addweaponnoswitch(g_player[g_p].ps, GetGameVarID(*(insptr),g_i,g_p));
            else addweapon(g_player[g_p].ps, GetGameVarID(*(insptr),g_i,g_p));
        }
        else if (g_player[g_p].ps->ammo_amount[GetGameVarID(*(insptr),g_i,g_p)] >= g_player[g_p].ps->max_ammo_amount[GetGameVarID(*(insptr),g_i,g_p)])
        {
            killit_flag = 2;
            break;
        }
        addammo(GetGameVarID(*(insptr),g_i,g_p), g_player[g_p].ps, GetGameVarID(*(insptr+1),g_i,g_p));
        if (g_player[g_p].ps->curr_weapon == KNEE_WEAPON && g_player[g_p].ps->gotweapon[GetGameVarID(*(insptr),g_i,g_p)])
        {
            if (!(g_player[g_p].ps->weaponswitch & 1)) addweaponnoswitch(g_player[g_p].ps, GetGameVarID(*(insptr),g_i,g_p));
            else addweapon(g_player[g_p].ps, GetGameVarID(*(insptr),g_i,g_p));
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
                if ((var1<0 || var1>=numsectors) && checkCON) {OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],var1);break;}
                activatebysector(var1, var2);
                break;
            case CON_OPERATESECTORS:
                if ((var1<0 || var1>=numsectors) && checkCON) {OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],var1);break;}
                operatesectors(var1, var2);
                break;
            case CON_OPERATEACTIVATORS:
                if ((var2<0 || var2>=ud.multimode) && checkCON) {OSD_Printf(CON_ERROR "Invalid player %d\n",line_num,keyw[g_tw],var2);break;}
                operateactivators(var1, var2);
                break;
            case CON_SETASPECT:
                setaspect(var1, var2);
                break;
            case CON_SSP:
                if ((var1<0 || var1>=MAXSPRITES) && checkCON) { OSD_Printf(CON_ERROR "Invalid sprite %d\n",line_num,keyw[g_tw],var1);break;}
                ssp(var1, var2);
                break;
            }
            break;
        }

    case CON_CANSEESPR:
        insptr++;
        {
            int lVar1 = GetGameVarID(*insptr++,g_i,g_p), lVar2 = GetGameVarID(*insptr++,g_i,g_p), res;

            if ((lVar1<0 || lVar1>=MAXSPRITES || lVar2<0 || lVar2>=MAXSPRITES) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sprite %d\n",line_num,keyw[g_tw],lVar1<0||lVar1>=MAXSPRITES?lVar1:lVar2);
                res=0;
            }
            else res=cansee(sprite[lVar1].x,sprite[lVar1].y,sprite[lVar1].z,sprite[lVar1].sectnum,
                                sprite[lVar2].x,sprite[lVar2].y,sprite[lVar2].z,sprite[lVar2].sectnum);

            SetGameVarID(*insptr++, res, g_i, g_p);
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
            if ((fta_quotes[j] == NULL) && checkCON)
            {
                OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],j);
                SetGameVarID(i,-1,g_i,g_p);
                break;
            }
            SetGameVarID(i,Bstrlen(fta_quotes[j]),g_i,g_p);
            break;
        }

    case CON_HEADSPRITESTAT:
    case CON_PREVSPRITESTAT:
    case CON_NEXTSPRITESTAT:
    case CON_HEADSPRITESECT:
    case CON_PREVSPRITESECT:
    case CON_NEXTSPRITESECT:
        insptr++;
        {
            int i=*insptr++;
            j=GetGameVarID(*insptr++, g_i, g_p);
            switch (tw)
            {
            case CON_HEADSPRITESTAT:
                if ((j < 0 || j > MAXSTATUS) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid status list %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,headspritestat[j],g_i,g_p);
                break;
            case CON_PREVSPRITESTAT:
                if ((j < 0 || j >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,prevspritestat[j],g_i,g_p);
                break;
            case CON_NEXTSPRITESTAT:
                if ((j < 0 || j >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,nextspritestat[j],g_i,g_p);
                break;
            case CON_HEADSPRITESECT:
                if ((j < 0 || j > numsectors) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sector %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,headspritesect[j],g_i,g_p);
                break;
            case CON_PREVSPRITESECT:
                if ((j < 0 || j >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,prevspritesect[j],g_i,g_p);
                break;
            case CON_NEXTSPRITESECT:
                if ((j < 0 || j >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],j);
                    break;
                }
                SetGameVarID(i,nextspritesect[j],g_i,g_p);
                break;
            }
            break;
        }

    case CON_GETKEYNAME:
        insptr++;
        {
            int i = GetGameVarID(*insptr++, g_i, g_p),
                    f=GetGameVarID(*insptr++, g_i, g_p);
            j=GetGameVarID(*insptr++, g_i, g_p);
            if ((i<0 || i>=MAXQUOTES) && checkCON)
                OSD_Printf(CON_ERROR "invalid quote ID %d\n",line_num,keyw[g_tw],i);
            else if ((fta_quotes[i] == NULL) && checkCON)
                OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],i);
            else if ((f<0 || f>=NUMGAMEFUNCTIONS) && checkCON)
                OSD_Printf(CON_ERROR "invalid function %d\n",line_num,keyw[g_tw],f);
            else
            {
                if (j<2)
                    Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][j]));
                else
                {
                    Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][0]));
                    if (!*tempbuf)
                        Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][1]));
                }
            }

            if (*tempbuf)
                Bstrcpy(fta_quotes[i],tempbuf);
            break;
        }
    case CON_QSUBSTR:
        insptr++;
        {
            char *s1,*s2;
            int q1,q2,st,ln;

            q1 = GetGameVarID(*insptr++, g_i, g_p),
                 q2 = GetGameVarID(*insptr++, g_i, g_p);
            st = GetGameVarID(*insptr++, g_i, g_p);
            ln = GetGameVarID(*insptr++, g_i, g_p);

            if ((q1<0 || q1>=MAXQUOTES) && checkCON)       OSD_Printf(CON_ERROR "invalid quote ID %d\n",line_num,keyw[g_tw],q1);
            else if ((fta_quotes[q1] == NULL) && checkCON) OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],q1);
            else if ((q2<0 || q2>=MAXQUOTES) && checkCON)  OSD_Printf(CON_ERROR "invalid quote ID %d\n",line_num,keyw[g_tw],q2);
            else if ((fta_quotes[q2] == NULL) && checkCON) OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],q2);
            else
            {
                s1=fta_quotes[q1];
                s2=fta_quotes[q2];
                while (*s2&&st--)s2++;
                while ((*s1=*s2)&&ln--) {s1++;s2++;}
                *s1=0;
            }
            break;
        }

    case CON_GETPNAME:
    case CON_QSTRCAT:
    case CON_QSTRCPY:
    case CON_QGETSYSSTR:
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
                if ((fta_quotes[i] == NULL) && checkCON)
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],i);
                    break;
                }
                if (g_player[j].user_name[0])
                    Bstrcpy(fta_quotes[i],g_player[j].user_name);
                else Bsprintf(fta_quotes[i],"%d",j);
                break;
            case CON_QGETSYSSTR:
                if ((fta_quotes[i] == NULL) && checkCON)
                {
                    OSD_Printf(CON_ERROR "null quote %d %d\n",line_num,keyw[g_tw],i,j);
                    break;
                }
                switch (j)
                {
                case STR_MAPNAME:
                    Bstrcpy(fta_quotes[i],map[ud.volume_number*MAXLEVELS + ud.level_number].name);
                    break;
                case STR_MAPFILENAME:
                    Bstrcpy(fta_quotes[i],map[ud.volume_number*MAXLEVELS + ud.level_number].filename);
                    break;
                case STR_PLAYERNAME:
                    Bstrcpy(fta_quotes[i],g_player[g_p].user_name);
                    break;
                case STR_VERSION:
                    Bstrcpy(fta_quotes[i],HEAD2);
                    break;
                case STR_GAMETYPE:
                    Bstrcpy(fta_quotes[i],gametype_names[ud.coop]);
                    break;
                default:
                    OSD_Printf(CON_ERROR "unknown str ID %d %d\n",line_num,keyw[g_tw],i,j);
                }
                break;
            case CON_QSTRCAT:
                if ((fta_quotes[i] == NULL || fta_quotes[j] == NULL) && checkCON)
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],fta_quotes[i] ? j : i);
                    break;
                }
                Bstrncat(fta_quotes[i],fta_quotes[j],(MAXQUOTELEN-1)-Bstrlen(fta_quotes[i]));
                break;
            case CON_QSTRCPY:
                if ((fta_quotes[i] == NULL || fta_quotes[j] == NULL) && checkCON)
                {
                    OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],fta_quotes[i] ? j : i);
                    break;
                }
                Bstrcpy(fta_quotes[i],fta_quotes[j]);
                break;
            case CON_CHANGESPRITESTAT:
                if ((i<0 || i>=MAXSPRITES) && checkCON) {OSD_Printf(CON_ERROR "Invalid sprite %d\n",line_num,keyw[g_tw],i);break;}
                if ((j<0 || j>=MAXSTATUS) && checkCON) {OSD_Printf(CON_ERROR "Invalid status %d\n",line_num,keyw[g_tw],j);break;}
                changespritestat(i,j);
                break;
            case CON_CHANGESPRITESECT:
                if ((i<0 || i>=MAXSPRITES) && checkCON) {OSD_Printf(CON_ERROR "Invalid sprite %d\n",line_num,keyw[g_tw],i);break;}
                if ((j<0 || j>=numsectors) && checkCON) {OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],j);break;}
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

            if ((volnume > MAXVOLUMES-1 || volnume < 0) && checkCON)
            {
                OSD_Printf(CON_ERROR "invalid volume (%d)\n",line_num,keyw[g_tw],volnume);
                break;
            }

            if ((levnume > MAXLEVELS-1 || levnume < 0) && checkCON)
            {
                OSD_Printf(CON_ERROR "invalid level (%d)\n",line_num,keyw[g_tw],levnume);
                break;
            }

            ud.m_volume_number = ud.volume_number = volnume;
            ud.m_level_number = ud.level_number = levnume;
            if (numplayers > 1 && myconnectindex == connecthead)
                mpchangemap(volnume,levnume);
            else
            {
                g_player[myconnectindex].ps->gm |= MODE_EOL;
                ud.display_bonus_screen = 0;
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
            int lValue=GetGameVarID(*insptr++, g_i, g_p), lEnd=*insptr++, lCases=*insptr++;
            intptr_t *lpDefault=insptr++, *lpCases=insptr, *lTempInsPtr;
            int bMatched=0, lCheckCase;
            int left,right;
            insptr+=lCases*2;
            lTempInsPtr=insptr;
            //Bsprintf(g_szBuf,"lEnd= %d *lpDefault=%d",lEnd,*lpDefault);
            //AddLog(g_szBuf);

            //Bsprintf(g_szBuf,"Checking %d cases for %d",lCases, lValue);
            //AddLog(g_szBuf);
            left=0;right=lCases-1;
            while (!bMatched)
            {
                //Bsprintf(g_szBuf,"Checking #%d Value= %d",lCheckCase, lpCases[lCheckCase*2]);
                //AddLog(g_szBuf);
                lCheckCase=(left+right)/2;
//                initprintf("(%2d..%2d..%2d) [%2d..%2d..%2d]==%2d\n",left,lCheckCase,right,lpCases[left*2],lpCases[lCheckCase*2],lpCases[right*2],lValue);
                if (lpCases[lCheckCase*2] > lValue)
                    right=lCheckCase-1;
                else if (lpCases[lCheckCase*2] < lValue)
                    left =lCheckCase+1;
                else if (lpCases[lCheckCase*2] == lValue)
                {
                    //AddLog("Found Case Match");
                    //Bsprintf(g_szBuf,"insptr=%d. lCheckCase=%d, offset=%d, &script[0]=%d",
                    //            (int)insptr,(int)lCheckCase,lpCases[lCheckCase*2+1],(int)&script[0]);
                    //AddLog(g_szBuf);
                    // fake a 2-d Array
                    insptr=(intptr_t*)(lpCases[lCheckCase*2+1] + &script[0]);
                    //Bsprintf(g_szBuf,"insptr=%d. ",     (int)insptr);
                    //AddLog(g_szBuf);
                    while (1)
                    {
                        if (parse())
                            break;
                    }
                    //AddLog("Done Executing Case");
                    bMatched=1;
                }
                if (right-left < 0)
                    break;
            }
            if (!bMatched)
            {
                if (*lpDefault)
                {
                    //AddLog("No Matching Case: Using Default");
                    insptr=(intptr_t*)(*lpDefault + &script[0]);
                    while (1) if (parse()) break;
                }
                else
                {
                    //AddLog("No Matching Case: No Default to use");
                }
            }
            insptr=(intptr_t *)(lEnd + (intptr_t)&script[0]);
            //Bsprintf(g_szBuf,"insptr=%d. ",     (int)insptr);
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

            if ((wallnum<0 || wallnum>=numwalls) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid wall %d\n",line_num,keyw[g_tw],wallnum);
                break;
            }
            dragpoint(wallnum,newx,newy);
            break;
        }

    case CON_DIST:
    case CON_LDIST:
        insptr++;
        {
            int distvar = *insptr++, xvar = GetGameVarID(*insptr++, g_i, g_p), yvar = GetGameVarID(*insptr++, g_i, g_p), distx=0;

            if ((xvar < 0 || yvar < 0 || xvar >= MAXSPRITES || yvar >= MAXSPRITES) && checkCON)
            {
                OSD_Printf(CON_ERROR "invalid sprite\n",line_num,keyw[g_tw]);
                break;
            }
            if (tw == CON_DIST) distx = dist(&sprite[xvar],&sprite[yvar]);
            else distx = ldist(&sprite[xvar],&sprite[yvar]);

            SetGameVarID(distvar, distx, g_i, g_p);
            break;
        }

    case CON_GETINCANGLE:
    case CON_GETANGLE:
        insptr++;
        {
            int angvar = *insptr++;
            int xvar = GetGameVarID(*insptr++, g_i, g_p);
            int yvar = GetGameVarID(*insptr++, g_i, g_p);

            if (tw==CON_GETANGLE)
            {
                SetGameVarID(angvar, getangle(xvar,yvar), g_i, g_p);
                break;
            }
            SetGameVarID(angvar, getincangle(xvar,yvar), g_i, g_p);
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
        if (timer == j)
            break;
        uninittimer();
        inittimer(j);
        timer = j;
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
            if ((g_sp->sectnum < 0 || g_sp->sectnum >= numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],g_sp->sectnum);
                break;
            }
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

        if ((g_sp->sectnum < 0 || g_sp->sectnum >= numsectors) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],g_sp->sectnum);
            insptr++;
            break;
        }

        j = spawn(g_i,*insptr++);

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

        if ((g_sp->sectnum < 0 || g_sp->sectnum >= numsectors) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],g_sp->sectnum);
            insptr++;
            hittype[g_i].temp_data[9]=0;
            break;
        }

        j = shoot(g_i,*insptr++);

        if (tw == CON_EZSHOOT || tw == CON_ESHOOT)
            SetGameVarID(g_iReturnVarID, j, g_i, g_p);

        hittype[g_i].temp_data[9]=0;
        break;

    case CON_SHOOTVAR:
    case CON_ESHOOTVAR:
    case CON_EZSHOOTVAR:
    case CON_ZSHOOTVAR:
    {
        int lReturn=-1;

        insptr++;

        if (tw == CON_ZSHOOTVAR || tw == CON_EZSHOOTVAR)
        {
            hittype[g_i].temp_data[9] = GetGameVarID(*insptr++, g_i, g_p);
            if (hittype[g_i].temp_data[9] == 0)
                hittype[g_i].temp_data[9] = 1;
        }
        j=GetGameVarID(*insptr++, g_i, g_p);

        if ((g_sp->sectnum < 0 || g_sp->sectnum >= numsectors) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],g_sp->sectnum);
            hittype[g_i].temp_data[9]=0;
            break;
        }

        lReturn = shoot(g_i, j);
        if (tw == CON_ESHOOTVAR || tw == CON_EZSHOOTVAR)
            SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
        hittype[g_i].temp_data[9]=0;
        break;
    }

    case CON_CMENU:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);
        cmenu(j);
        break;

    case CON_SOUNDVAR:
    case CON_STOPSOUNDVAR:
    case CON_SOUNDONCEVAR:
    case CON_GLOBALSOUNDVAR:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);

        switch (tw)
        {
        case CON_SOUNDONCEVAR:
            if ((j<0 || j>=MAXSOUNDS) && checkCON) {OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],j);break;}
            if (!isspritemakingsound(g_i,j))
                spritesound((short)j,g_i);
            break;
        case CON_GLOBALSOUNDVAR:
            if ((j<0 || j>=MAXSOUNDS) && checkCON) {OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],j);break;}
            spritesound((short)j,g_player[screenpeek].ps->i);
            break;
        case CON_STOPSOUNDVAR:
            if ((j<0 || j>=MAXSOUNDS) && checkCON) {OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],j);break;}
            if (isspritemakingsound(g_i,j))
                stopspritesound((short)j,g_i);
            break;
        case CON_SOUNDVAR:
            if ((j<0 || j>=MAXSOUNDS) && checkCON) {OSD_Printf(CON_ERROR "Invalid sound %d\n",line_num,keyw[g_tw],j);break;}
            spritesound((short)j,g_i);
            break;
        }
        break;

    case CON_GUNIQHUDID:
        insptr++;
        {
            j=GetGameVarID(*insptr++, g_i, g_p);
            if (j >= 0 && j < MAXUNIQHUDID-1)
                guniqhudid = j;
            else
                OSD_Printf(CON_ERROR "Invalid ID %d\n",line_num,keyw[g_tw],j);
            break;
        }

    case CON_SAVEGAMEVAR:
    case CON_READGAMEVAR:
    {
        int32 i=0;
        insptr++;
        if (ud.config.scripthandle < 0)
        {
            insptr++;
            break;
        }
        switch (tw)
        {
        case CON_SAVEGAMEVAR:
            i=GetGameVarID(*insptr, g_i, g_p);
            SCRIPT_PutNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr++].szLabel,i,false,false);
            break;
        case CON_READGAMEVAR:
            SCRIPT_GetNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr].szLabel,&i);
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
            int x1=scale(GetGameVarID(*insptr++,g_i,g_p),xdim,320);
            int y1=scale(GetGameVarID(*insptr++,g_i,g_p),ydim,200);
            int x2=scale(GetGameVarID(*insptr++,g_i,g_p),xdim,320);
            int y2=scale(GetGameVarID(*insptr++,g_i,g_p),ydim,200);
            int smoothratio = 65536;

            if (x1 > x2) swaplong(&x1,&x2);
            if (y1 > y2) swaplong(&y1,&y2);

            if ((x1 < 0 || y1 < 0 || x2 > xdim-1 || y2 > ydim-1 || x2-x1 < 2 || y2-y1 < 2) && checkCON)
            {
                OSD_Printf(CON_ERROR "incorrect coordinates\n",line_num,keyw[g_tw]);
                break;
            }
            if ((sect<0 || sect>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],sect);
                break;
            }

#if defined(USE_OPENGL) && defined(POLYMOST)
            j = glprojectionhacks;
            glprojectionhacks = 0;
#endif
            setview(x1,y1,x2,y2);

#if 0
            if (!ud.pause_on && ((ud.show_help == 0 && ud.multimode < 2 && !(g_player[myconnectindex].ps->gm&MODE_MENU)) || ud.multimode > 1 || ud.recstat == 2))
                smoothratio = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
#endif
            dointerpolations(smoothratio);

#define SE40

#ifdef SE40
            se40code(x,y,z,a,horiz,smoothratio);
#endif
            if (((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
#if defined(POLYMER) && defined(USE_OPENGL)
                    && (getrendermode() != 4)
#endif
               )
            {
                int j, i = 0, k, dst = 0x7fffffff;

                for (k=mirrorcnt-1;k>=0;k--)
                {
                    j = klabs(wall[mirrorwall[k]].x-x);
                    j += klabs(wall[mirrorwall[k]].y-y);
                    if (j < dst) dst = j, i = k;
                }

                if (wall[mirrorwall[i]].overpicnum == MIRROR)
                {
                    int tposx,tposy;
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

#ifdef POLYMER
            if (getrendermode() == 4)
                polymer_setanimatesprites(animatesprites, x,y,a,smoothratio);
#endif
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

    case CON_ROTATESPRITE16:
    case CON_ROTATESPRITE:
        insptr++;
        {
            int x=GetGameVarID(*insptr++,g_i,g_p),   y=GetGameVarID(*insptr++,g_i,g_p),           z=GetGameVarID(*insptr++,g_i,g_p);
            int a=GetGameVarID(*insptr++,g_i,g_p),   tilenum=GetGameVarID(*insptr++,g_i,g_p),     shade=GetGameVarID(*insptr++,g_i,g_p);
            int pal=GetGameVarID(*insptr++,g_i,g_p), orientation=GetGameVarID(*insptr++,g_i,g_p);
            int x1=GetGameVarID(*insptr++,g_i,g_p),  y1=GetGameVarID(*insptr++,g_i,g_p);
            int x2=GetGameVarID(*insptr++,g_i,g_p),  y2=GetGameVarID(*insptr++,g_i,g_p);

            if (tw == CON_ROTATESPRITE && !(orientation & 256)) {x<<=16;y<<=16;}
            rotatesprite(x,y,z,a,tilenum,shade,pal,2|orientation,x1,y1,x2,y2);
            break;
        }

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_GAMETEXTZ:
    case CON_DIGITALNUMBER:
    case CON_DIGITALNUMBERZ:
        insptr++;
        {
            int tilenum = (tw == CON_GAMETEXT || tw == CON_GAMETEXTZ || tw == CON_DIGITALNUMBER || tw == CON_DIGITALNUMBERZ)?GetGameVarID(*insptr++,g_i,g_p):0;
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), q=GetGameVarID(*insptr++,g_i,g_p);
            int shade=GetGameVarID(*insptr++,g_i,g_p), pal=GetGameVarID(*insptr++,g_i,g_p);

            if (tw == CON_GAMETEXT || tw == CON_GAMETEXTZ || tw == CON_DIGITALNUMBER || tw == CON_DIGITALNUMBERZ)
            {
                int orientation=GetGameVarID(*insptr++,g_i,g_p);
                int x1=GetGameVarID(*insptr++,g_i,g_p), y1=GetGameVarID(*insptr++,g_i,g_p);
                int x2=GetGameVarID(*insptr++,g_i,g_p), y2=GetGameVarID(*insptr++,g_i,g_p);
                int z=65536;

                if (tw == CON_GAMETEXT || tw == CON_GAMETEXTZ)
                {
                    int z=65536;
                    if ((fta_quotes[q] == NULL) && checkCON)
                    {
                        OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],q);
                        break;
                    }
                    if (tw == CON_GAMETEXTZ)z=GetGameVarID(*insptr++,g_i,g_p);
                    gametext_z(0,tilenum,x>>1,y,fta_quotes[q],shade,pal,orientation,x1,y1,x2,y2,z);
                    break;
                }
                if (tw == CON_DIGITALNUMBERZ)z=GetGameVarID(*insptr++,g_i,g_p);
                txdigitalnumberz(tilenum,x,y,q,shade,pal,orientation,x1,y1,x2,y2,z);
                break;
            }

            if ((fta_quotes[q] == NULL) && checkCON)
            {
                OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],q);
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
            int ceilz, ceilhit, florz, florhit;

            if ((sectnum<0 || sectnum>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],sectnum);
                break;
            }
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
            int hitx, hity, hitz;

            if ((sectnum<0 || sectnum>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],sectnum);
                break;
            }
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

            if ((sect1<0 || sect1>=numsectors || sect2<0 || sect2>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector\n",line_num,keyw[g_tw]);
                SetGameVarID(rvar, 0, g_i, g_p);
            }

            SetGameVarID(rvar, cansee(x1,y1,z1,sect1,x2,y2,z2,sect2), g_i, g_p);
            break;
        }

    case CON_ROTATEPOINT:
        insptr++;
        {
            int xpivot=GetGameVarID(*insptr++,g_i,g_p), ypivot=GetGameVarID(*insptr++,g_i,g_p);
            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), daang=GetGameVarID(*insptr++,g_i,g_p);
            int x2var=*insptr++, y2var=*insptr++;
            int x2, y2;

            rotatepoint(xpivot,ypivot,x,y,daang,&x2,&y2);
            SetGameVarID(x2var, x2, g_i, g_p);
            SetGameVarID(y2var, y2, g_i, g_p);
            break;
        }

    case CON_NEARTAG:
        insptr++;
        {
            //             neartag(int x, int y, int z, short sectnum, short ang,  //Starting position & angle
            //                     short *neartagsector,   //Returns near sector if sector[].tag != 0
            //                     short *neartagwall,     //Returns near wall if wall[].tag != 0
            //                     short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
            //                     int *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
            //                     int neartagrange,      //Choose maximum distance to scan (scale: 1024=largest grid size)
            //                     char tagsearch)         //1-lotag only, 2-hitag only, 3-lotag&hitag

            int x=GetGameVarID(*insptr++,g_i,g_p), y=GetGameVarID(*insptr++,g_i,g_p), z=GetGameVarID(*insptr++,g_i,g_p);
            int sectnum=GetGameVarID(*insptr++,g_i,g_p), ang=GetGameVarID(*insptr++,g_i,g_p);
            int neartagsectorvar=*insptr++, neartagwallvar=*insptr++, neartagspritevar=*insptr++, neartaghitdistvar=*insptr++;
            int neartagrange=GetGameVarID(*insptr++,g_i,g_p), tagsearch=GetGameVarID(*insptr++,g_i,g_p);

            if ((sectnum<0 || sectnum>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],sectnum);
                break;
            }
            neartag(x, y, z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, neartagrange, tagsearch);

            SetGameVarID(neartagsectorvar, neartagsector, g_i, g_p);
            SetGameVarID(neartagwallvar, neartagwall, g_i, g_p);
            SetGameVarID(neartagspritevar, neartagsprite, g_i, g_p);
            SetGameVarID(neartaghitdistvar, neartaghitdist, g_i, g_p);
            break;
        }

    case CON_GETTIMEDATE:
        insptr++;
        {
            int v1=*insptr++,v2=*insptr++,v3=*insptr++,v4=*insptr++,v5=*insptr++,v6=*insptr++,v7=*insptr++,v8=*insptr++;
            time_t rawtime;
            struct tm * ti;

            time(&rawtime);
            ti=localtime(&rawtime);
            // initprintf("Time&date: %s\n",asctime (ti));

            SetGameVarID(v1, ti->tm_sec,  g_i, g_p);
            SetGameVarID(v2, ti->tm_min,  g_i, g_p);
            SetGameVarID(v3, ti->tm_hour, g_i, g_p);
            SetGameVarID(v4, ti->tm_mday, g_i, g_p);
            SetGameVarID(v5, ti->tm_mon,  g_i, g_p);
            SetGameVarID(v6, ti->tm_year+1900, g_i, g_p);
            SetGameVarID(v7, ti->tm_wday, g_i, g_p);
            SetGameVarID(v8, ti->tm_yday, g_i, g_p);
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
                if ((spritenum < 0 || spritenum >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],spritenum);
                    break;
                }
                setsprite(spritenum, x, y, z);
                break;
            }

            {
                int cliptype = GetGameVarID(*insptr++,g_i,g_p);

                if ((spritenum < 0 && spritenum >= MAXSPRITES) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],spritenum);
                    insptr++;
                    break;
                }
                SetGameVarID(*insptr++, movesprite(spritenum, x, y, z, cliptype), g_i, g_p);
                break;
            }
        }

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        insptr++;
        {
            int sectnum = GetGameVarID(*insptr++,g_i,g_p), x = GetGameVarID(*insptr++,g_i,g_p), y = GetGameVarID(*insptr++,g_i,g_p);
            if ((sectnum<0 || sectnum>=numsectors) && checkCON)
            {
                OSD_Printf(CON_ERROR "Invalid sector %d\n",line_num,keyw[g_tw],sectnum);
                insptr++;
                break;
            }

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
            else updatesectorz(x,y,z,&w);

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
//            for (j=0;j<13;j++)
//                Bmemcpy(&ud.savegame[lastsavedpos][j+4],&tempbuf[j+3],sizeof(tempbuf[j+3]));
//            ud.savegame[lastsavedpos][j+4] = '\0';
            Bmemcpy(&ud.savegame[lastsavedpos][4],&tempbuf[3],sizeof(tempbuf[0])*13);
            ud.savegame[lastsavedpos][17] = '\0';
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
        spritesound(EARTHQUAKE,g_player[screenpeek].ps->i);
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
                g_player[g_p].ps->gm = MODE_MENU;
                KB_ClearKeyDown(sc_Space);
                cmenu(15000);
            }
            else g_player[g_p].ps->gm = MODE_RESTART;
            killit_flag = 2;
        }
        else
        {
            pickrandomspot(g_p);
            g_sp->x = hittype[g_i].bposx = g_player[g_p].ps->bobposx = g_player[g_p].ps->oposx = g_player[g_p].ps->posx;
            g_sp->y = hittype[g_i].bposy = g_player[g_p].ps->bobposy = g_player[g_p].ps->oposy =g_player[g_p].ps->posy;
            g_sp->z = hittype[g_i].bposy = g_player[g_p].ps->oposz =g_player[g_p].ps->posz;
            updatesector(g_player[g_p].ps->posx,g_player[g_p].ps->posy,&g_player[g_p].ps->cursectnum);
            setsprite(g_player[g_p].ps->i,g_player[g_p].ps->posx,g_player[g_p].ps->posy,g_player[g_p].ps->posz+PHEIGHT);
            g_sp->cstat = 257;

            g_sp->shade = -12;
            g_sp->clipdist = 64;
            g_sp->xrepeat = 42;
            g_sp->yrepeat = 36;
            g_sp->owner = g_i;
            g_sp->xoffset = 0;
            g_sp->pal = g_player[g_p].ps->palookup;

            g_player[g_p].ps->last_extra = g_sp->extra = g_player[g_p].ps->max_player_health;
            g_player[g_p].ps->wantweaponfire = -1;
            g_player[g_p].ps->horiz = 100;
            g_player[g_p].ps->on_crane = -1;
            g_player[g_p].ps->frag_ps = g_p;
            g_player[g_p].ps->horizoff = 0;
            g_player[g_p].ps->opyoff = 0;
            g_player[g_p].ps->wackedbyactor = -1;
            g_player[g_p].ps->shield_amount = start_armour_amount;
            g_player[g_p].ps->dead_flag = 0;
            g_player[g_p].ps->pals_time = 0;
            g_player[g_p].ps->footprintcount = 0;
            g_player[g_p].ps->weapreccnt = 0;
            g_player[g_p].ps->fta = 0;
            g_player[g_p].ps->ftq = 0;
            g_player[g_p].ps->posxv = g_player[g_p].ps->posyv = 0;
            g_player[g_p].ps->rotscrnang = 0;
            g_player[g_p].ps->runspeed = dukefriction;
            g_player[g_p].ps->falling_counter = 0;

            hittype[g_i].extra = -1;
            hittype[g_i].owner = g_i;

            hittype[g_i].cgg = 0;
            hittype[g_i].movflag = 0;
            hittype[g_i].tempang = 0;
            hittype[g_i].actorstayput = -1;
            hittype[g_i].dispicnum = 0;
            hittype[g_i].owner = g_player[g_p].ps->i;

            resetinventory(g_p);
            resetweapons(g_p);

            g_player[g_p].ps->reloading = 0;

            g_player[g_p].ps->movement_lock = 0;

            OnEvent(EVENT_RESETPLAYER, g_player[g_p].ps->i, g_p, -1);
            cameradist = 0;
            cameraclock = totalclock;
        }
        setpal(g_player[g_p].ps);
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
            g_player[g_p].ps->steroids_amount = *insptr;
            g_player[g_p].ps->inven_icon = 2;
            break;

        case GET_SHIELD:
            g_player[g_p].ps->shield_amount +=          *insptr;// 100;
            if (g_player[g_p].ps->shield_amount > g_player[g_p].ps->max_shield_amount)
                g_player[g_p].ps->shield_amount = g_player[g_p].ps->max_shield_amount;
            break;

        case GET_SCUBA:
            g_player[g_p].ps->scuba_amount =             *insptr;// 1600;
            g_player[g_p].ps->inven_icon = 6;
            break;

        case GET_HOLODUKE:
            g_player[g_p].ps->holoduke_amount =          *insptr;// 1600;
            g_player[g_p].ps->inven_icon = 3;
            break;

        case GET_JETPACK:
            g_player[g_p].ps->jetpack_amount =           *insptr;// 1600;
            g_player[g_p].ps->inven_icon = 4;
            break;

        case GET_ACCESS:
            switch (g_sp->pal)
            {
            case  0:
                g_player[g_p].ps->got_access |= 1;
                break;
            case 21:
                g_player[g_p].ps->got_access |= 2;
                break;
            case 23:
                g_player[g_p].ps->got_access |= 4;
                break;
            }
            break;

        case GET_HEATS:
            g_player[g_p].ps->heat_amount = *insptr;
            g_player[g_p].ps->inven_icon = 5;
            break;

        case GET_FIRSTAID:
            g_player[g_p].ps->inven_icon = 1;
            g_player[g_p].ps->firstaid_amount = *insptr;
            break;

        case GET_BOOTS:
            g_player[g_p].ps->inven_icon = 7;
            g_player[g_p].ps->boot_amount = *insptr;
            break;
        default:
            OSD_Printf(CON_ERROR "Invalid inventory ID %d\n",line_num,keyw[g_tw],*(insptr-1));
            break;
        }
        insptr++;
        break;

    case CON_HITRADIUSVAR:
        insptr++;
        {
            int v1=GetGameVarID(*insptr++,g_i,g_p),v2=GetGameVarID(*insptr++,g_i,g_p),v3=GetGameVarID(*insptr++,g_i,g_p);
            int v4=GetGameVarID(*insptr++,g_i,g_p),v5=GetGameVarID(*insptr++,g_i,g_p);
            hitradius(g_i,v1,v2,v3,v4,v5);
        }
        break;

    case CON_HITRADIUS:
        hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
        insptr+=6;
        break;

    case CON_IFP:
    {
//        insptr++;

        l = *(++insptr);
        j = 0;

        s = sprite[g_player[g_p].ps->i].xvel;

        if ((l&8) && g_player[g_p].ps->on_ground && (g_player[g_p].sync->bits&2))
            j = 1;
        else if ((l&16) && g_player[g_p].ps->jumping_counter == 0 && !g_player[g_p].ps->on_ground &&
                 g_player[g_p].ps->poszv > 2048)
            j = 1;
        else if ((l&32) && g_player[g_p].ps->jumping_counter > 348)
            j = 1;
        else if ((l&1) && s >= 0 && s < 8)
            j = 1;
        else if ((l&2) && s >= 8 && !(g_player[g_p].sync->bits&(1<<5)))
            j = 1;
        else if ((l&4) && s >= 8 && g_player[g_p].sync->bits&(1<<5))
            j = 1;
        else if ((l&64) && g_player[g_p].ps->posz < (g_sp->z-(48<<8)))
            j = 1;
        else if ((l&128) && s <= -8 && !(g_player[g_p].sync->bits&(1<<5)))
            j = 1;
        else if ((l&256) && s <= -8 && (g_player[g_p].sync->bits&(1<<5)))
            j = 1;
        else if ((l&512) && (g_player[g_p].ps->quick_kick > 0 || (g_player[g_p].ps->curr_weapon == KNEE_WEAPON && g_player[g_p].ps->kickback_pic > 0)))
            j = 1;
        else if ((l&1024) && sprite[g_player[g_p].ps->i].xrepeat < 32)
            j = 1;
        else if ((l&2048) && g_player[g_p].ps->jetpack_on)
            j = 1;
        else if ((l&4096) && g_player[g_p].ps->steroids_amount > 0 && g_player[g_p].ps->steroids_amount < 400)
            j = 1;
        else if ((l&8192) && g_player[g_p].ps->on_ground)
            j = 1;
        else if ((l&16384) && sprite[g_player[g_p].ps->i].xrepeat > 32 && sprite[g_player[g_p].ps->i].extra > 0 && g_player[g_p].ps->timebeforeexit == 0)
            j = 1;
        else if ((l&32768) && sprite[g_player[g_p].ps->i].extra <= 0)
            j = 1;
        else if ((l&65536L))
        {
            if (g_sp->picnum == APLAYER && ud.multimode > 1)
                j = getincangle(g_player[otherp].ps->ang,getangle(g_player[g_p].ps->posx-g_player[otherp].ps->posx,g_player[g_p].ps->posy-g_player[otherp].ps->posy));
            else
                j = getincangle(g_player[g_p].ps->ang,getangle(g_sp->x-g_player[g_p].ps->posx,g_sp->y-g_player[g_p].ps->posy));

            if (j > -128 && j < 128)
                j = 1;
            else
                j = 0;
        }
        parseifelse((intptr_t) j);
    }
    break;

    case CON_IFSTRENGTH:
        insptr++;
        parseifelse(g_sp->extra <= *insptr);
        break;

    case CON_GUTS:
        insptr += 2;
        guts(g_i,*(insptr-1),*insptr);
        insptr++;
        break;

    case CON_IFSPAWNEDBY:
        insptr++;
        parseifelse(hittype[g_i].picnum == *insptr);
        break;

    case CON_WACKPLAYER:
        insptr++;
        forceplayerangle(g_player[g_p].ps);
        return 0;

    case CON_FLASH:
        insptr++;
        sprite[g_i].shade = -127;
        g_player[g_p].ps->visibility = -127;
        lastvisinc = totalclock+32;
        return 0;

    case CON_SAVEMAPSTATE:
        if (map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate == NULL)
            map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate = Bcalloc(1,sizeof(mapstate_t));
        savemapstate(map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
        insptr++;
        return 0;

    case CON_LOADMAPSTATE:
        if (map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate)
            restoremapstate(map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
        insptr++;
        return 0;

    case CON_CLEARMAPSTATE:
        insptr++;
        j = GetGameVarID(*insptr++,g_i,g_p);
        if ((j < 0 || j >= MAXVOLUMES*MAXLEVELS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid map number: %d\n",line_num,keyw[g_tw],j);
            return 0;
        }
        if (map[j].savedstate)
            FreeMapState(j);
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
        parseifelse(g_player[g_p].sync->bits&(1<<29));
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
        g_player[g_p].ps->pals_time = *insptr++;
        for (j=2;j>=0;j--)
            g_player[g_p].ps->pals[2-j] = *insptr++;
        break;

    case CON_QSPRINTF:
        insptr++;
        {
            int dq = *insptr++, sq = *insptr++;
            if ((fta_quotes[sq] == NULL || fta_quotes[dq] == NULL) && checkCON)
            {
                OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],fta_quotes[sq] ? dq : sq);
                insptr += 4;
                break;
            }

            {
                int var1 = GetGameVarID(*insptr++, g_i, g_p), var2 = GetGameVarID(*insptr++, g_i, g_p);
                int var3 = GetGameVarID(*insptr++, g_i, g_p), var4 = GetGameVarID(*insptr++, g_i, g_p);
                Bstrcpy(tempbuf,fta_quotes[sq]);
                Bsprintf(fta_quotes[dq],tempbuf,var1,var2,var3,var4);
                break;
            }
        }

    case CON_ADDLOG:
    {
        insptr++;
        OSD_Printf(OSDTEXT_GREEN "CONLOG: L=%d\n",line_num);
        break;
    }

    case CON_ADDLOGVAR:
        insptr++;
        {
            int m=1;
            char szBuf[256];
            int lVarID = *insptr;

            if ((lVarID >= iGameVarCount) || lVarID < 0)
            {
                if (*insptr==MAXGAMEVARS) // addlogvar for a constant?  Har.
                    insptr++;
//                else if (*insptr > iGameVarCount && (*insptr < (MAXGAMEVARS<<1)+MAXGAMEVARS+1+MAXGAMEARRAYS))
                else if (*insptr&(MAXGAMEVARS<<2))
                {
                    int index;

                    lVarID ^= (MAXGAMEVARS<<2);

                    if (lVarID&(MAXGAMEVARS<<1))
                    {
                        m = -m;
                        lVarID ^= (MAXGAMEVARS<<1);
                    }

                    insptr++;

                    index=GetGameVarID(*insptr++,g_i,g_p);
                    if ((index < aGameArrays[lVarID].size)&&(index>=0))
                    {
                        OSD_Printf(OSDTEXT_GREEN "%s: L=%d %s[%d] =%d\n",line_num,keyw[g_tw],
                                   aGameArrays[lVarID].szLabel,index,m*aGameArrays[lVarID].plValues[index]);
                        break;
                    }
                    else
                    {
                        OSD_Printf(CON_ERROR "invalid array index\n",line_num,keyw[g_tw]);
                        break;
                    }
                }
                else if (*insptr&(MAXGAMEVARS<<1))
                {
                    m = -m;
                    lVarID ^= (MAXGAMEVARS<<1);
                }
                else
                {
                    // invalid varID
                    insptr++;
                    OSD_Printf(CON_ERROR "invalid variable\n",line_num,keyw[g_tw]);
                    break;  // out of switch
                }
            }
            Bsprintf(szBuf,"CONLOGVAR: L=%d %s ",line_num, aGameVars[lVarID].szLabel);
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
            Bsprintf(szBuf," =%d\n", GetGameVarID(lVarID, g_i, g_p)*m);
            Bstrcat(g_szBuf,szBuf);
            OSD_Printf(OSDTEXT_GREEN "%s",g_szBuf);
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
            int lFound=-1, j, k = MAXSTATUS-1;

            if (tw == CON_FINDNEARACTOR || tw == CON_FINDNEARACTOR3D)
                k = 1;
            do
            {
                j=headspritestat[k];    // all sprites
                if (tw==CON_FINDNEARSPRITE3D || tw==CON_FINDNEARACTOR3D)
                {
                    while (j>=0)
                    {
                        if (sprite[j].picnum == lType && j != g_i && dist(&sprite[g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }
                }
                else
                {
                    while (j>=0)
                    {
                        if (sprite[j].picnum == lType && j != g_i && ldist(&sprite[g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }
                }
                if (j == MAXSPRITES || tw == CON_FINDNEARACTOR || tw == CON_FINDNEARACTOR3D)
                    break;
            }
            while (k--);
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
            int lFound=-1, j, k = MAXSTATUS-1;

            if (tw == CON_FINDNEARACTORVAR || tw == CON_FINDNEARACTOR3DVAR)
                k = 1;

            do
            {
                j=headspritestat[k];    // all sprites
                if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                {
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == lType && j != g_i && dist(&sprite[g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }
                }
                else
                {
                    while (j >= 0)
                    {
                        if (sprite[j].picnum == lType && j != g_i && ldist(&sprite[g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }
                }
                if (j == MAXSPRITES || tw==CON_FINDNEARACTORVAR || tw==CON_FINDNEARACTOR3DVAR)
                    break;
            }
            while (k--);
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

            k=MAXSTATUS-1;
            do
            {
                j=headspritestat[tw==CON_FINDNEARACTORZVAR?1:k];    // all sprites
                if (j == -1) continue;
                do
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
                while (j>=0);
                if (tw==CON_FINDNEARACTORZVAR || j == MAXSPRITES)
                    break;
            }
            while (k--);
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

            k=MAXSTATUS-1;
            do
            {
                j=headspritestat[tw==CON_FINDNEARACTORZ?1:k];    // all sprites
                if (j == -1) continue;
                do
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
                while (j>=0);
                if (tw==CON_FINDNEARACTORZ || j == MAXSPRITES)
                    break;
            }
            while (k--);
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
            int var1 = *insptr++, d;

            if (tw == CON_FINDPLAYER) j=findplayer(&sprite[g_i],&d);
            else j=findotherplayer(g_i,&d);

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
            int lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0, lVar2;
            // HACK: need to have access to labels structure at run-time...

            if (playerlabels[lLabelID].flags & LABEL_HASPARM2)
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
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
            int lSprite=GetGameVarID(*insptr++, g_i, g_p), lVar1=*insptr++;
            j=*insptr++;

            if ((lSprite < 0 || lSprite >= MAXSPRITES) && checkCON)
            {
                OSD_Printf(CON_ERROR "invalid sprite ID %d\n",line_num,keyw[g_tw],lSprite);
                if (lVar1 == MAXGAMEVARS) insptr++;
                if (j == MAXGAMEVARS) insptr++;
                break;
            }

            if (tw == CON_SETACTORVAR)
            {
                SetGameVarID(lVar1, GetGameVarID(j, g_i, g_p), lSprite, g_p);
                break;
            }
            SetGameVarID(j, GetGameVarID(lVar1, lSprite, g_p), g_i, g_p);
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

                if ((iPlayer < 0 || iPlayer >= ud.multimode) && checkCON)
                {
                    OSD_Printf(CON_ERROR "invalid player ID %d\n",line_num,keyw[g_tw],iPlayer);
                    break;
                }

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

            int lVar1=*insptr++, lLabelID=*insptr++, lParm2 = 0, lVar2;

            if (actorlabels[lLabelID].flags & LABEL_HASPARM2)
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
            lVar2=*insptr++;

            DoActor(tw==CON_SETACTOR, lVar1, lLabelID, lVar2, lParm2);
            break;
        }

    case CON_SETTSPR:
    case CON_GETTSPR:
        insptr++;
        {
            // syntax [gs]etactor[<var>].x <VAR>
            // <varid> <xxxid> <varid>

            int lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

            DoTsprite(tw==CON_SETTSPR, lVar1, lLabelID, lVar2);
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
        g_player[g_p].ps->transporter_hold=GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_CHECKAVAILWEAPON:
    case CON_CHECKAVAILINVEN:
        insptr++;
        j = g_p;

        if (*insptr != g_iThisActorID)
            j=GetGameVarID(*insptr, g_i, g_p);

        insptr++;

        if ((j < 0 || j >= ud.multimode) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid player ID %d\n",line_num,keyw[g_tw],j);
            break;
        }

        if (tw == CON_CHECKAVAILWEAPON)
            checkavailweapon(g_player[j].ps);
        else checkavailinven(g_player[j].ps);

        break;

    case CON_GETPLAYERANGLE:
        insptr++;
        SetGameVarID(*insptr++, g_player[g_p].ps->ang, g_i, g_p);
        break;

    case CON_SETPLAYERANGLE:
        insptr++;
        g_player[g_p].ps->ang=GetGameVarID(*insptr++, g_i, g_p);
        g_player[g_p].ps->ang &= 2047;
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

    case CON_SETARRAY:
        insptr++;
        j=*insptr++;
        {
            int index = GetGameVarID(*insptr++, g_i, g_p);
            int value = GetGameVarID(*insptr++, g_i, g_p);

//            SetGameArrayID(j,index,value);
            if (j<0 || j >= iGameArrayCount || index >= aGameArrays[j].size || index < 0)
            {
                OSD_Printf(OSD_ERROR "SetGameVarID(): tried to set invalid array ID (%d) or index out of bounds from sprite %d (%d), player %d\n",j,g_i,sprite[g_i].picnum,g_p);
                return 0;
            }
            aGameArrays[j].plValues[index]=value;
            break;
        }

    case CON_RESIZEARRAY:
        insptr++;
        j=*insptr++;
        {
            int asize = GetGameVarID(*insptr++, g_i, g_p);
            if (asize > 0)
            {
                OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n", aGameArrays[j].szLabel, aGameArrays[j].size, asize);
                aGameArrays[j].plValues=Brealloc(aGameArrays[j].plValues, sizeof(int) * asize);
                aGameArrays[j].size = asize;
            }
            break;
        }

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
        {
            OSD_Printf(CON_ERROR "Divide by zero.\n",line_num,keyw[g_tw]);
            insptr += 2;
            break;
        }
        SetGameVarID(*insptr, GetGameVarID(*insptr, g_i, g_p) / *(insptr+1), g_i, g_p);
        insptr += 2;
        break;

    case CON_MODVAR:
        insptr++;
        if (*(insptr+1) == 0)
        {
            OSD_Printf(CON_ERROR "Mod by zero.\n",line_num,keyw[g_tw]);
            insptr += 2;
            break;
        }
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
        if ((j<0 || j>=MAX_WEAPONS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",line_num,keyw[g_tw],j);
            insptr++;
            break;
        }
        SetGameVarID(*insptr++, g_player[g_p].ps->max_ammo_amount[j], g_i, g_p);
        break;

    case CON_SMAXAMMO:
        insptr++;
        j=GetGameVarID(*insptr++, g_i, g_p);
        if ((j<0 || j>=MAX_WEAPONS) && checkCON)
        {
            OSD_Printf(CON_ERROR "Invalid weapon ID %d\n",line_num,keyw[g_tw],j);
            insptr++;
            break;
        }
        g_player[g_p].ps->max_ammo_amount[j]=GetGameVarID(*insptr++, g_i, g_p);
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
            {
                OSD_Printf(CON_ERROR "Divide by zero.\n",line_num,keyw[g_tw]);
                break;
            }
            SetGameVarID(j, GetGameVarID(j, g_i, g_p)/l2 , g_i, g_p);
            break;
        }

    case CON_MODVARVAR:
        insptr++;
        j=*insptr++;
        {
            int l2=GetGameVarID(*insptr++, g_i, g_p);

            if (l2==0)
            {
                OSD_Printf(CON_ERROR "Mod by zero.\n",line_num,keyw[g_tw]);
                break;
            }

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
    case CON_STARTTRACKVAR:
        insptr++;
        if (tw == CON_STARTTRACK) music_select=(ud.volume_number*MAXLEVELS)+(*(insptr++));
        else music_select=(ud.volume_number*MAXLEVELS)+(GetGameVarID(*(insptr++), g_i, g_p));
        if (map[(unsigned char)music_select].musicfn == NULL)
        {
            OSD_Printf(CON_ERROR "null music for map %d\n",line_num,keyw[g_tw],music_select);
            insptr++;
            break;
        }
        playmusic(&map[(unsigned char)music_select].musicfn[0],music_select);
        break;

    case CON_ACTIVATECHEAT:
        insptr++;
        j=GetGameVarID(*(insptr++), g_i, g_p);
        if (numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            OSD_Printf(CON_ERROR "not in a single-player game.\n",line_num,keyw[g_tw]);
            break;
        }
        osdcmd_cheatsinfo_stat.cheatnum = j;
        break;

    case CON_SETGAMEPALETTE:
        insptr++;
        j=GetGameVarID(*(insptr++), g_i, g_p);
        switch (j)
        {
        default:
        case 0:setgamepalette(g_player[g_p].ps,palette  ,0);break;
        case 1:setgamepalette(g_player[g_p].ps,waterpal ,0);break;
        case 2:setgamepalette(g_player[g_p].ps,slimepal ,0);break;
        case 3:setgamepalette(g_player[g_p].ps,drealms  ,0);break;
        case 4:setgamepalette(g_player[g_p].ps,titlepal ,0);break;
        case 5:setgamepalette(g_player[g_p].ps,endingpal,0);break;
        case 6:setgamepalette(g_player[g_p].ps,animpal  ,0);break;
        }
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
        intptr_t *savedinsptr=insptr+2;
        j=1;
        do
        {
            insptr=savedinsptr;
            if (GetGameVarID(*(insptr-1), g_i, g_p) == *insptr)
                j=0;
            parseifelse(j);
        }
        while (j);
        break;
    }

    case CON_WHILEVARVARN:
    {
        int i,k;
        intptr_t *savedinsptr=insptr+2;
        j=1;
        do
        {
            insptr=savedinsptr;
            i = GetGameVarID(*(insptr-1), g_i, g_p);
            k=*(insptr);
            if (i == GetGameVarID(k, g_i, g_p))
                j=0;
            parseifelse(j);
        }
        while (j);
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
        parseifelse(sprite[g_player[g_p].ps->i].extra < *insptr);
        break;

    case CON_IFPINVENTORY:
    {
        insptr++;
        j = 0;
        switch (*insptr++)
        {
        case GET_STEROIDS:
            if (g_player[g_p].ps->steroids_amount != *insptr)
                j = 1;
            break;
        case GET_SHIELD:
            if (g_player[g_p].ps->shield_amount != g_player[g_p].ps->max_shield_amount)
                j = 1;
            break;
        case GET_SCUBA:
            if (g_player[g_p].ps->scuba_amount != *insptr) j = 1;
            break;
        case GET_HOLODUKE:
            if (g_player[g_p].ps->holoduke_amount != *insptr) j = 1;
            break;
        case GET_JETPACK:
            if (g_player[g_p].ps->jetpack_amount != *insptr) j = 1;
            break;
        case GET_ACCESS:
            switch (g_sp->pal)
            {
            case  0:
                if (g_player[g_p].ps->got_access&1) j = 1;
                break;
            case 21:
                if (g_player[g_p].ps->got_access&2) j = 1;
                break;
            case 23:
                if (g_player[g_p].ps->got_access&4) j = 1;
                break;
            }
            break;
        case GET_HEATS:
            if (g_player[g_p].ps->heat_amount != *insptr) j = 1;
            break;
        case GET_FIRSTAID:
            if (g_player[g_p].ps->firstaid_amount != *insptr) j = 1;
            break;
        case GET_BOOTS:
            if (g_player[g_p].ps->boot_amount != *insptr) j = 1;
            break;
        default:
            OSD_Printf(CON_ERROR "invalid inventory ID: %d\n",line_num,keyw[g_tw],*(insptr-1));
        }

        parseifelse(j);
        break;
    }

    case CON_PSTOMP:
        insptr++;
        if (g_player[g_p].ps->knee_incs == 0 && sprite[g_player[g_p].ps->i].xrepeat >= 40)
            if (cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,g_player[g_p].ps->posx,g_player[g_p].ps->posy,g_player[g_p].ps->posz+(16<<8),sprite[g_player[g_p].ps->i].sectnum))
            {
                for (j=ud.multimode-1;j>=0;j--)
                {
                    if (g_player[j].ps->actorsqu == g_i)
                        break;
                }
                if (j == -1)
                {
                    g_player[g_p].ps->knee_incs = 1;
                    if (g_player[g_p].ps->weapon_pos == 0)
                        g_player[g_p].ps->weapon_pos = -1;
                    g_player[g_p].ps->actorsqu = g_i;
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

        if ((fta_quotes[*insptr] == NULL) && checkCON)
        {
            OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],*insptr);
            insptr++;
            break;
        }

        if ((g_p < 0 || g_p >= MAXPLAYERS) && checkCON)
        {
            OSD_Printf(CON_ERROR "bad player for quote %d: (%d)\n",line_num,keyw[g_tw],*insptr,g_p);
            insptr++;
            break;
        }

        FTA(*(insptr++)|MAXQUOTES,g_player[g_p].ps);
        break;

    case CON_USERQUOTE:
        insptr++;
        {
            int i=GetGameVarID(*insptr++, g_i, g_p);

            if ((fta_quotes[i] == NULL) && checkCON)
            {
                OSD_Printf(CON_ERROR "null quote %d\n",line_num,keyw[g_tw],i);
                break;
            }
            adduserquote(fta_quotes[i]);
        }
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
        j = klabs(getincangle(g_player[g_p].ps->ang,g_sp->ang));
        parseifelse(j <= *insptr);
        break;

    case CON_IFNOSOUNDS:
        for (j=MAXSOUNDS-1;j>=0;j--)
            if (g_sounds[j].SoundOwner[0].i == g_i)
                break;

        parseifelse(j < 0);
        break;

    case CON_SPRITEFLAGS:
        insptr++;
        hittype[g_i].flags = GetGameVarID(*insptr++, g_i, g_p);
        break;

    case CON_GETTICKS:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, getticks(), g_i, g_p);
        break;

    case CON_GETCURRADDRESS:
        insptr++;
        j=*insptr++;
        SetGameVarID(j, (intptr_t)(insptr-script), g_i, g_p);
        break;

    case CON_JUMP:
        insptr++;
        j = GetGameVarID(*insptr++, g_i, g_p);
        insptr = (intptr_t *)(j+script);
        break;

    default:
        OSD_Printf("fatal error: default processing: previous five values: %d, %d, %d, %d, %d, "
                   "current opcode: %d, next five values: %d, %d, %d, %d, %d\ncurrent actor: %d (%d)\n",
                   *(insptr-5),*(insptr-4),*(insptr-3),*(insptr-2),*(insptr-1),*insptr,*(insptr+1),
                   *(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5),g_i,g_sp->picnum);
        OSD_Printf("line_num: %d, g_tw: %d\n",line_num,g_tw);

        gameexit("An error has occurred in the EDuke32 CON executor.\n\n"
                 "If you are an end user, please e-mail the file eduke32.log\n"
                 "along with links to any mods you're using to terminx@gmail.com.\n\n"
                 "If you are a mod developer, please attach all of your CON files\n"
                 "along with instructions on how to reproduce this error.\n\n"
                 "Thank you!");
        break;
    }
    return 0;
}

void LoadActor(int iActor)
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
        //          g_player[g_p].ps->actors_killed++;
        deletesprite(g_i);
        return;
    }

    while (1) if (parse()) break;

    if (killit_flag == 1)
        deletesprite(g_i);
}

void execute(int iActor,int iPlayer,int lDist)
{
    int temp, temp2;

//    if (actorscrptr[sprite[iActor].picnum] == 0) return;

    g_i = iActor;    // Sprite ID
    g_p = iPlayer;   // Player ID
    g_x = lDist;     // ??
    g_sp = &sprite[g_i];    // Pointer to sprite structure
    g_t = &hittype[g_i].temp_data[0];   // Sprite's 'extra' data

    insptr = 4 + (actorscrptr[g_sp->picnum]);

    killit_flag = 0;

    if (g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        if (badguy(g_sp))
            g_player[g_p].ps->actors_killed++;
        deletesprite(g_i);
        return;
    }

    /* Qbix: Changed variables to be aware of the sizeof *insptr
     * (wether it is int vs intptr_t), Although it is specificly cast to intptr_t*
     * which might be corrected if the code is converted to use offsets */
    if (g_t[4])
    {
        g_sp->lotag += TICSPERFRAME;

        if (g_sp->lotag > *(intptr_t *)(g_t[4]+4*sizeof(*insptr)))
        {
            g_t[2]++;
            g_sp->lotag = 0;
            g_t[3] +=  *(intptr_t *)(g_t[4]+3*sizeof(*insptr));
        }

        if (klabs(g_t[3]) >= klabs(*(intptr_t *)(g_t[4]+sizeof(*insptr)) * *(intptr_t *)(g_t[4]+3*sizeof(*insptr))))
            g_t[3] = 0;
    }

    while (1) if (parse()) break;

    if (killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if (g_player[g_p].ps->actorsqu == g_i)
            g_player[g_p].ps->actorsqu = -1;
        deletesprite(g_i);
        return;
    }

    move();

    if (ud.angleinterpolation)
    {
        temp = (g_sp->ang & 2047) - sprpos[g_i].ang;
        sprpos[g_i].oldang = sprpos[g_i].ang;
        if (temp)
        {
            temp2 = temp/klabs(temp);
            if (klabs(temp) > 1024) temp2 = -(temp2);
            sprpos[g_i].angdir = temp2;
            sprpos[g_i].angdif = min(ud.angleinterpolation,klabs(temp));
            sprpos[g_i].ang += sprpos[g_i].angdif * sprpos[g_i].angdir;
            sprpos[g_i].ang &= 2047;
        }
    }

    if (g_sp->statnum == 6)
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
        default:
            return;
        }

    if (g_sp->statnum != 1)
        return;

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

void savemapstate(mapstate_t *save)
{
    if (save != NULL)
    {
        int i;
        intptr_t j;

        Bmemcpy(&save->numwalls,&numwalls,sizeof(numwalls));
        Bmemcpy(&save->wall[0],&wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&save->numsectors,&numsectors,sizeof(numsectors));
        Bmemcpy(&save->sector[0],&sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&save->sprite[0],&sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&save->spriteext[0],&spriteext[0],sizeof(spriteexttype)*MAXSPRITES);
        Bmemcpy(&save->headspritesect[0],&headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&save->prevspritesect[0],&prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&save->nextspritesect[0],&nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&save->headspritestat[0],&headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&save->prevspritestat[0],&prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&save->nextspritestat[0],&nextspritestat[0],sizeof(nextspritestat));

        for (i=MAXSPRITES-1;i>=0;i--)
        {
            save->scriptptrs[i] = 0;

            if (actorscrptr[PN] == 0) continue;

            j = (intptr_t)&script[0];

            if (T2 >= j && T2 < (intptr_t)(&script[g_ScriptSize]))
            {
                save->scriptptrs[i] |= 1;
                T2 -= j;
            }
            if (T5 >= j && T5 < (intptr_t)(&script[g_ScriptSize]))
            {
                save->scriptptrs[i] |= 2;
                T5 -= j;
            }
            if (T6 >= j && T6 < (intptr_t)(&script[g_ScriptSize]))
            {
                save->scriptptrs[i] |= 4;
                T6 -= j;
            }
        }

        Bmemcpy(&save->hittype[0],&hittype[0],sizeof(actordata_t)*MAXSPRITES);

        for (i=MAXSPRITES-1;i>=0;i--)
        {
            if (actorscrptr[PN] == 0) continue;
            j = (intptr_t)&script[0];

            if (save->scriptptrs[i]&1)
                T2 += j;
            if (save->scriptptrs[i]&2)
                T5 += j;
            if (save->scriptptrs[i]&4)
                T6 += j;
        }

        Bmemcpy(&save->numcyclers,&numcyclers,sizeof(numcyclers));
        Bmemcpy(&save->cyclers[0][0],&cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&save->g_PlayerSpawnPoints[0],&g_PlayerSpawnPoints[0],sizeof(g_PlayerSpawnPoints));
        Bmemcpy(&save->numanimwalls,&numanimwalls,sizeof(numanimwalls));
        Bmemcpy(&save->spriteq[0],&spriteq[0],sizeof(spriteq));
        Bmemcpy(&save->spriteqloc,&spriteqloc,sizeof(spriteqloc));
        Bmemcpy(&save->animwall[0],&animwall[0],sizeof(animwall));
        Bmemcpy(&save->msx[0],&msx[0],sizeof(msx));
        Bmemcpy(&save->msy[0],&msy[0],sizeof(msy));
        Bmemcpy(&save->mirrorwall[0],&mirrorwall[0],sizeof(mirrorwall));
        Bmemcpy(&save->mirrorsector[0],&mirrorsector[0],sizeof(mirrorsector));
        Bmemcpy(&save->mirrorcnt,&mirrorcnt,sizeof(mirrorcnt));
        Bmemcpy(&save->show2dsector[0],&show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&save->numclouds,&numclouds,sizeof(numclouds));
        Bmemcpy(&save->clouds[0],&clouds[0],sizeof(clouds));
        Bmemcpy(&save->cloudx[0],&cloudx[0],sizeof(cloudx));
        Bmemcpy(&save->cloudy[0],&cloudy[0],sizeof(cloudy));
        Bmemcpy(&save->pskyoff[0],&pskyoff[0],sizeof(pskyoff));
        Bmemcpy(&save->pskybits,&pskybits,sizeof(pskybits));
        Bmemcpy(&save->animategoal[0],&animategoal[0],sizeof(animategoal));
        Bmemcpy(&save->animatevel[0],&animatevel[0],sizeof(animatevel));
        Bmemcpy(&save->animatecnt,&animatecnt,sizeof(animatecnt));
        Bmemcpy(&save->animatesect[0],&animatesect[0],sizeof(animatesect));
        for (i = animatecnt-1;i>=0;i--) animateptr[i] = (int *)((intptr_t)animateptr[i]-(intptr_t)(&sector[0]));
        Bmemcpy(&save->animateptr[0],&animateptr[0],sizeof(animateptr));
        for (i = animatecnt-1;i>=0;i--) animateptr[i] = (int *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
        Bmemcpy(&save->numplayersprites,&numplayersprites,sizeof(numplayersprites));
        Bmemcpy(&save->earthquaketime,&earthquaketime,sizeof(earthquaketime));
        Bmemcpy(&save->lockclock,&lockclock,sizeof(lockclock));
        Bmemcpy(&save->randomseed,&randomseed,sizeof(randomseed));
        Bmemcpy(&save->global_random,&global_random,sizeof(global_random));

        for (i=iGameVarCount-1; i>=0;i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_FLAG_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXPLAYERS,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].plValues[0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
            {
                if (!save->vars[i])
                    save->vars[i] = Bcalloc(MAXSPRITES,sizeof(intptr_t));
                Bmemcpy(&save->vars[i][0],&aGameVars[i].plValues[0],sizeof(intptr_t) * MAXSPRITES);
            }
            else save->vars[i] = (intptr_t *)aGameVars[i].lValue;
        }

        ototalclock = totalclock;
    }
}

extern void ResetPointerVars(void);

void restoremapstate(mapstate_t *save)
{
    if (save != NULL)
    {
        int i, k, x;
        intptr_t j;
        char phealth[MAXPLAYERS];

        for (i=0;i<ud.multimode;i++)
            phealth[i] = sprite[g_player[i].ps->i].extra;

        pub = NUMPAGES;
        pus = NUMPAGES;
        vscrn();

        Bmemcpy(&numwalls,&save->numwalls,sizeof(numwalls));
        Bmemcpy(&wall[0],&save->wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&numsectors,&save->numsectors,sizeof(numsectors));
        Bmemcpy(&sector[0],&save->sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&sprite[0],&save->sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&spriteext[0],&save->spriteext[0],sizeof(spriteexttype)*MAXSPRITES);
        Bmemcpy(&headspritesect[0],&save->headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&prevspritesect[0],&save->prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&nextspritesect[0],&save->nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&headspritestat[0],&save->headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&prevspritestat[0],&save->prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&nextspritestat[0],&save->nextspritestat[0],sizeof(nextspritestat));
        Bmemcpy(&hittype[0],&save->hittype[0],sizeof(actordata_t)*MAXSPRITES);

        for (i=MAXSPRITES-1;i>=0;i--)
        {
            j = (intptr_t)(&script[0]);
            if (save->scriptptrs[i]&1) T2 += j;
            if (save->scriptptrs[i]&2) T5 += j;
            if (save->scriptptrs[i]&4) T6 += j;
        }

        Bmemcpy(&numcyclers,&save->numcyclers,sizeof(numcyclers));
        Bmemcpy(&cyclers[0][0],&save->cyclers[0][0],sizeof(cyclers));
        Bmemcpy(&g_PlayerSpawnPoints[0],&save->g_PlayerSpawnPoints[0],sizeof(g_PlayerSpawnPoints));
        Bmemcpy(&numanimwalls,&save->numanimwalls,sizeof(numanimwalls));
        Bmemcpy(&spriteq[0],&save->spriteq[0],sizeof(spriteq));
        Bmemcpy(&spriteqloc,&save->spriteqloc,sizeof(spriteqloc));
        Bmemcpy(&animwall[0],&save->animwall[0],sizeof(animwall));
        Bmemcpy(&msx[0],&save->msx[0],sizeof(msx));
        Bmemcpy(&msy[0],&save->msy[0],sizeof(msy));
        Bmemcpy(&mirrorwall[0],&save->mirrorwall[0],sizeof(mirrorwall));
        Bmemcpy(&mirrorsector[0],&save->mirrorsector[0],sizeof(mirrorsector));
        Bmemcpy(&mirrorcnt,&save->mirrorcnt,sizeof(mirrorcnt));
        Bmemcpy(&show2dsector[0],&save->show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&numclouds,&save->numclouds,sizeof(numclouds));
        Bmemcpy(&clouds[0],&save->clouds[0],sizeof(clouds));
        Bmemcpy(&cloudx[0],&save->cloudx[0],sizeof(cloudx));
        Bmemcpy(&cloudy[0],&save->cloudy[0],sizeof(cloudy));
        Bmemcpy(&pskyoff[0],&save->pskyoff[0],sizeof(pskyoff));
        Bmemcpy(&pskybits,&save->pskybits,sizeof(pskybits));
        Bmemcpy(&animategoal[0],&save->animategoal[0],sizeof(animategoal));
        Bmemcpy(&animatevel[0],&save->animatevel[0],sizeof(animatevel));
        Bmemcpy(&animatecnt,&save->animatecnt,sizeof(animatecnt));
        Bmemcpy(&animatesect[0],&save->animatesect[0],sizeof(animatesect));
        Bmemcpy(&animateptr[0],&save->animateptr[0],sizeof(animateptr));
        for (i = animatecnt-1;i>=0;i--) animateptr[i] = (int *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
        Bmemcpy(&numplayersprites,&save->numplayersprites,sizeof(numplayersprites));
        Bmemcpy(&earthquaketime,&save->earthquaketime,sizeof(earthquaketime));
        Bmemcpy(&lockclock,&save->lockclock,sizeof(lockclock));
        Bmemcpy(&randomseed,&save->randomseed,sizeof(randomseed));
        Bmemcpy(&global_random,&save->global_random,sizeof(global_random));

        for (i=iGameVarCount-1;i>=0;i--)
        {
            if (aGameVars[i].dwFlags & GAMEVAR_FLAG_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
                Bmemcpy(&aGameVars[i].plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXPLAYERS);
            else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
                Bmemcpy(&aGameVars[i].plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXSPRITES);
            else aGameVars[i].lValue = (intptr_t)save->vars[i];
        }

        ResetPointerVars();

        for (i=0;i<ud.multimode;i++)
            sprite[g_player[i].ps->i].extra = phealth[i];

        if (g_player[myconnectindex].ps->over_shoulder_on != 0)
        {
            cameradist = 0;
            cameraclock = 0;
            g_player[myconnectindex].ps->over_shoulder_on = 1;
        }

        screenpeek = myconnectindex;

        if (ud.lockout == 0)
        {
            for (x=numanimwalls-1;x>=0;x--)
                if (wall[animwall[x].wallnum].extra >= 0)
                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
        }
        else
        {
            for (x=numanimwalls-1;x>=0;x--)
                switch (dynamictostatic[wall[animwall[x].wallnum].picnum])
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }

        numinterpolations = 0;
        startofdynamicinterpolations = 0;

        k = headspritestat[3];
        while (k >= 0)
        {
            switch (sprite[k].lotag)
            {
            case 31:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 0:
            case 5:
            case 6:
            case 11:
            case 14:
            case 15:
            case 16:
            case 26:
            case 30:
                setsectinterpolate(k);
                break;
            }

            k = nextspritestat[k];
        }

        for (i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];
        for (i = animatecnt-1;i>=0;i--)
            setinterpolation(animateptr[i]);

        resetmys();

        flushpackets();
        clearfifo();
        waitforeverybody();
        resettimevars();
    }
}
