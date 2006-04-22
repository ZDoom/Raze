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

#include <time.h>

#include "duke3d.h"
#include "gamedef.h"
#include "scriplib.h"

#include "osd.h"

static short g_i,g_p;
static long g_x,*g_t;
static spritetype *g_sp;

extern int32 scripthandle;

void DoUserDef(char bSet, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    long lValue;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    if(sPlayer != myconnectindex) return;

    switch(lLabelID)
    {
    case USERDEFS_GOD:
        if(bSet)
            ud.god = lValue;
        else
            SetGameVarID((int)lVar2, ud.god, sActor, sPlayer);
        break;

    case USERDEFS_WARP_ON:
        if(bSet)
            ud.warp_on = lValue;
        else
            SetGameVarID((int)lVar2, ud.warp_on, sActor, sPlayer);
        break;

    case USERDEFS_CASHMAN:
        if(bSet)
            ud.cashman = lValue;
        else
            SetGameVarID((int)lVar2, ud.cashman, sActor, sPlayer);
        break;

    case USERDEFS_EOG:
        if(bSet)
            ud.eog = lValue;
        else
            SetGameVarID((int)lVar2, ud.eog, sActor, sPlayer);
        break;

    case USERDEFS_SHOWALLMAP:
        if(bSet)
            ud.showallmap = lValue;
        else
            SetGameVarID((int)lVar2, ud.showallmap, sActor, sPlayer);
        break;

    case USERDEFS_SHOW_HELP:
        if(bSet)
            ud.show_help = lValue;
        else
            SetGameVarID((int)lVar2, ud.show_help, sActor, sPlayer);
        break;

    case USERDEFS_SCROLLMODE:
        if(bSet)
            ud.scrollmode = lValue;
        else
            SetGameVarID((int)lVar2, ud.scrollmode, sActor, sPlayer);
        break;

    case USERDEFS_CLIPPING:
        if(bSet)
            ud.clipping = lValue;
        else
            SetGameVarID((int)lVar2, ud.clipping, sActor, sPlayer);
        break;

        //  case USERDEFS_USER_NAME:
        //      if(bSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.user_name[MAXPLAYERS][32], sActor, sPlayer);
        //      }
        //      break;

        //  case USERDEFS_RIDECULE:
        //      if(bSet)
        //      {
        //          ud.ridecule = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.ridecule, sActor, sPlayer);
        //      }
        //      break;

        //  case USERDEFS_SAVEGAME:
        //      if(bSet)
        //      {
        //          ud.savegame = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.savegame, sActor, sPlayer);
        //      }
        //      break;

        //  case USERDEFS_PWLOCKOUT:
        //      if(bSet)
        //      {
        //          ud.pwlockout = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.pwlockout, sActor, sPlayer);
        //      }
        //      break;

        //  case USERDEFS_RTSNAME:
        //      if(bSet)
        //      {
        //          ud.rtsname = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.rtsname, sActor, sPlayer);
        //      }
        //      break;

    case USERDEFS_OVERHEAD_ON:
        if(bSet)
            ud.overhead_on = lValue;
        else
            SetGameVarID((int)lVar2, ud.overhead_on, sActor, sPlayer);
        break;

    case USERDEFS_LAST_OVERHEAD:
        if(bSet)
            ud.last_overhead = lValue;
        else
            SetGameVarID((int)lVar2, ud.last_overhead, sActor, sPlayer);
        break;

    case USERDEFS_SHOWWEAPONS:
        if(bSet)
            ud.showweapons = lValue;
        else
            SetGameVarID((int)lVar2, ud.showweapons, sActor, sPlayer);
        break;

    case USERDEFS_PAUSE_ON:
        if(bSet)
            ud.pause_on = lValue;
        else
            SetGameVarID((int)lVar2, ud.pause_on, sActor, sPlayer);
        break;

    case USERDEFS_FROM_BONUS:
        if(bSet)
            ud.from_bonus = lValue;
        else
            SetGameVarID((int)lVar2, ud.from_bonus, sActor, sPlayer);
        break;

    case USERDEFS_CAMERASPRITE:
        if(bSet)
            ud.camerasprite = lValue;
        else
            SetGameVarID((int)lVar2, ud.camerasprite, sActor, sPlayer);
        break;

    case USERDEFS_LAST_CAMSPRITE:
        if(bSet)
            ud.last_camsprite = lValue;
        else
            SetGameVarID((int)lVar2, ud.last_camsprite, sActor, sPlayer);
        break;

    case USERDEFS_LAST_LEVEL:
        if(bSet)
            ud.last_level = lValue;
        else
            SetGameVarID((int)lVar2, ud.last_level, sActor, sPlayer);
        break;

    case USERDEFS_SECRETLEVEL:
        if(bSet)
            ud.secretlevel = lValue;
        else
            SetGameVarID((int)lVar2, ud.secretlevel, sActor, sPlayer);
        break;

    case USERDEFS_CONST_VISIBILITY:
        if(bSet)
            ud.const_visibility = lValue;
        else
            SetGameVarID((int)lVar2, ud.const_visibility, sActor, sPlayer);
        break;

    case USERDEFS_UW_FRAMERATE:
        if(bSet)
            ud.uw_framerate = lValue;
        else
            SetGameVarID((int)lVar2, ud.uw_framerate, sActor, sPlayer);
        break;

    case USERDEFS_CAMERA_TIME:
        if(bSet)
            ud.camera_time = lValue;
        else
            SetGameVarID((int)lVar2, ud.camera_time, sActor, sPlayer);
        break;

    case USERDEFS_FOLFVEL:
        if(bSet)
            ud.folfvel = lValue;
        else
            SetGameVarID((int)lVar2, ud.folfvel, sActor, sPlayer);
        break;

    case USERDEFS_FOLAVEL:
        if(bSet)
            ud.folavel = lValue;
        else
            SetGameVarID((int)lVar2, ud.folavel, sActor, sPlayer);
        break;

    case USERDEFS_FOLX:
        if(bSet)
            ud.folx = lValue;
        else
            SetGameVarID((int)lVar2, ud.folx, sActor, sPlayer);
        break;

    case USERDEFS_FOLY:
        if(bSet)
            ud.foly = lValue;
        else
            SetGameVarID((int)lVar2, ud.foly, sActor, sPlayer);
        break;

    case USERDEFS_FOLA:
        if(bSet)
            ud.fola = lValue;
        else
            SetGameVarID((int)lVar2, ud.fola, sActor, sPlayer);
        break;

    case USERDEFS_RECCNT:
        if(bSet)
            ud.reccnt = lValue;
        else
            SetGameVarID((int)lVar2, ud.reccnt, sActor, sPlayer);
        break;

    case USERDEFS_ENTERED_NAME:
        if(bSet)
            ud.entered_name = lValue;
        else
            SetGameVarID((int)lVar2, ud.entered_name, sActor, sPlayer);
        break;

    case USERDEFS_SCREEN_TILTING:
        if(bSet)
            ud.screen_tilting = lValue;
        else
            SetGameVarID((int)lVar2, ud.screen_tilting, sActor, sPlayer);
        break;

    case USERDEFS_SHADOWS:
        if(bSet)
            ud.shadows = lValue;
        else
            SetGameVarID((int)lVar2, ud.shadows, sActor, sPlayer);
        break;

    case USERDEFS_FTA_ON:
        if(bSet)
            ud.fta_on = lValue;
        else
            SetGameVarID((int)lVar2, ud.fta_on, sActor, sPlayer);
        break;

    case USERDEFS_EXECUTIONS:
        if(bSet)
            ud.executions = lValue;
        else
            SetGameVarID((int)lVar2, ud.executions, sActor, sPlayer);
        break;

    case USERDEFS_AUTO_RUN:
        if(bSet)
            ud.auto_run = lValue;
        else
            SetGameVarID((int)lVar2, ud.auto_run, sActor, sPlayer);
        break;

    case USERDEFS_COORDS:
        if(bSet)
            ud.coords = lValue;
        else
            SetGameVarID((int)lVar2, ud.coords, sActor, sPlayer);
        break;

    case USERDEFS_TICKRATE:
        if(bSet)
            ud.tickrate = lValue;
        else
            SetGameVarID((int)lVar2, ud.tickrate, sActor, sPlayer);
        break;

    case USERDEFS_M_COOP:
        if(bSet)
            ud.m_coop = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_coop, sActor, sPlayer);
        break;

    case USERDEFS_COOP:
        if(bSet)
            ud.coop = lValue;
        else
            SetGameVarID((int)lVar2, ud.coop, sActor, sPlayer);
        break;

    case USERDEFS_SCREEN_SIZE:
        if(bSet)
            ud.screen_size = lValue;
        else
            SetGameVarID((int)lVar2, ud.screen_size, sActor, sPlayer);
        break;

    case USERDEFS_LOCKOUT:
        if(bSet)
            ud.lockout = lValue;
        else
            SetGameVarID((int)lVar2, ud.lockout, sActor, sPlayer);
        break;

    case USERDEFS_CROSSHAIR:
        if(bSet)
            ud.crosshair = lValue;
        else
            SetGameVarID((int)lVar2, ud.crosshair, sActor, sPlayer);
        break;

        //  case USERDEFS_WCHOICE:
        //      if(bSet)
        //      {
        //          ud.wchoice = lValue;
        //      }
        //      else
        //      {
        //          SetGameVarID((int)lVar2, ud.wchoice, sActor, sPlayer);
        //      }
        //      break;

    case USERDEFS_PLAYERAI:
        if(bSet)
            ud.playerai = lValue;
        else
            SetGameVarID((int)lVar2, ud.playerai, sActor, sPlayer);
        break;

    case USERDEFS_RESPAWN_MONSTERS:
        if(bSet)
            ud.respawn_monsters = lValue;
        else
            SetGameVarID((int)lVar2, ud.respawn_monsters, sActor, sPlayer);
        break;

    case USERDEFS_RESPAWN_ITEMS:
        if(bSet)
            ud.respawn_items = lValue;
        else
            SetGameVarID((int)lVar2, ud.respawn_items, sActor, sPlayer);
        break;

    case USERDEFS_RESPAWN_INVENTORY:
        if(bSet)
            ud.respawn_inventory = lValue;
        else
            SetGameVarID((int)lVar2, ud.respawn_inventory, sActor, sPlayer);
        break;

    case USERDEFS_RECSTAT:
        if(bSet)
            ud.recstat = lValue;
        else
            SetGameVarID((int)lVar2, ud.recstat, sActor, sPlayer);
        break;

    case USERDEFS_MONSTERS_OFF:
        if(bSet)
            ud.monsters_off = lValue;
        else
            SetGameVarID((int)lVar2, ud.monsters_off, sActor, sPlayer);
        break;

    case USERDEFS_BRIGHTNESS:
        if(bSet)
            ud.brightness = lValue;
        else
            SetGameVarID((int)lVar2, ud.brightness, sActor, sPlayer);
        break;

    case USERDEFS_M_RESPAWN_ITEMS:
        if(bSet)
            ud.m_respawn_items = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_respawn_items, sActor, sPlayer);
        break;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if(bSet)
            ud.m_respawn_monsters = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_respawn_monsters, sActor, sPlayer);
        break;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if(bSet)
            ud.m_respawn_inventory = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_respawn_inventory, sActor, sPlayer);
        break;

    case USERDEFS_M_RECSTAT:
        if(bSet)
            ud.m_recstat = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_recstat, sActor, sPlayer);
        break;

    case USERDEFS_M_MONSTERS_OFF:
        if(bSet)
            ud.m_monsters_off = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_monsters_off, sActor, sPlayer);
        break;

    case USERDEFS_DETAIL:
        if(bSet)
            ud.detail = lValue;
        else
            SetGameVarID((int)lVar2, ud.detail, sActor, sPlayer);
        break;

    case USERDEFS_M_FFIRE:
        if(bSet)
            ud.m_ffire = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_ffire, sActor, sPlayer);
        break;

    case USERDEFS_FFIRE:
        if(bSet)
            ud.ffire = lValue;
        else
            SetGameVarID((int)lVar2, ud.ffire, sActor, sPlayer);
        break;

    case USERDEFS_M_PLAYER_SKILL:
        if(bSet)
            ud.m_player_skill = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_player_skill, sActor, sPlayer);
        break;

    case USERDEFS_M_LEVEL_NUMBER:
        if(bSet)
            ud.m_level_number = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_level_number, sActor, sPlayer);
        break;

    case USERDEFS_M_VOLUME_NUMBER:
        if(bSet)
            ud.m_volume_number = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_volume_number, sActor, sPlayer);
        break;

    case USERDEFS_MULTIMODE:
        if(bSet)
            ud.multimode = lValue;
        else
            SetGameVarID((int)lVar2, ud.multimode, sActor, sPlayer);
        break;

    case USERDEFS_PLAYER_SKILL:
        if(bSet)
            ud.player_skill = lValue;
        else
            SetGameVarID((int)lVar2, ud.player_skill, sActor, sPlayer);
        break;

    case USERDEFS_LEVEL_NUMBER:
        if(bSet)
            ud.level_number = lValue;
        else
            SetGameVarID((int)lVar2, ud.level_number, sActor, sPlayer);
        break;

    case USERDEFS_VOLUME_NUMBER:
        if(bSet)
            ud.volume_number = lValue;
        else
            SetGameVarID((int)lVar2, ud.volume_number, sActor, sPlayer);
        break;

    case USERDEFS_M_MARKER:
        if(bSet)
            ud.m_marker = lValue;
        else
            SetGameVarID((int)lVar2, ud.m_marker, sActor, sPlayer);
        break;

    case USERDEFS_MARKER:
        if(bSet)
            ud.marker = lValue;
        else
            SetGameVarID((int)lVar2, ud.marker, sActor, sPlayer);
        break;

    case USERDEFS_MOUSEFLIP:
        if(bSet)
            ud.mouseflip = lValue;
        else
            SetGameVarID((int)lVar2, ud.mouseflip, sActor, sPlayer);
        break;

    case USERDEFS_STATUSBARSCALE:
        if(bSet)
            ud.statusbarscale = lValue;
        else
            SetGameVarID((int)lVar2, ud.statusbarscale, sActor, sPlayer);
        break;

    case USERDEFS_DRAWWEAPON:
        if(bSet)
            ud.drawweapon = lValue;
        else
            SetGameVarID((int)lVar2, ud.drawweapon, sActor, sPlayer);
        break;

    case USERDEFS_MOUSEAIMING:
        if(bSet)
            ud.mouseaiming = lValue;
        else
            SetGameVarID((int)lVar2, ud.mouseaiming, sActor, sPlayer);
        break;

    case USERDEFS_WEAPONSWITCH:
        if(bSet)
            ud.weaponswitch = lValue;
        else
            SetGameVarID((int)lVar2, ud.weaponswitch, sActor, sPlayer);
        break;

    case USERDEFS_BRIGHTSKINS:
        if(bSet)
            ud.brightskins = lValue;
        else
            SetGameVarID((int)lVar2, ud.brightskins, sActor, sPlayer);
        break;

    case USERDEFS_DEMOCAMS:
        if(bSet)
            ud.democams = lValue;
        else
            SetGameVarID((int)lVar2, ud.democams, sActor, sPlayer);
        break;

    case USERDEFS_COLOR:
        if(bSet)
            ud.color = lValue;
        else
            SetGameVarID((int)lVar2, ud.color, sActor, sPlayer);
        break;

    case USERDEFS_MSGDISPTIME:
        if(bSet)
            ud.msgdisptime = lValue;
        else
            SetGameVarID((int)lVar2, ud.msgdisptime, sActor, sPlayer);
        break;

    default:
        break;
    }
    return;
}

void DoThisProjectile(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    long lValue,proj;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    if(lVar1 == g_iThisActorID )
        // if they've asked for 'this', then use 'this'...
        proj=sActor;
    else
        proj=GetGameVarID((int)lVar1, sActor, sPlayer);

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    switch(lLabelID)
    {
    case PROJ_WORKSLIKE:
        if(bSet)
            thisprojectile[proj].workslike=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].workslike, sActor, sPlayer);
        break;

    case PROJ_SPAWNS:
        if(bSet)
            thisprojectile[proj].spawns=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].spawns, sActor, sPlayer);
        break;

    case PROJ_SXREPEAT:
        if(bSet)
            thisprojectile[proj].sxrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].sxrepeat, sActor, sPlayer);
        break;

    case PROJ_SYREPEAT:
        if(bSet)
            thisprojectile[proj].syrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].syrepeat, sActor, sPlayer);
        break;

    case PROJ_SOUND:
        if(bSet)
            thisprojectile[proj].sound=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].sound, sActor, sPlayer);
        break;

    case PROJ_ISOUND:
        if(bSet)
            thisprojectile[proj].isound=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].isound, sActor, sPlayer);
        break;

    case PROJ_VEL:
        if(bSet)
            thisprojectile[proj].vel=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].vel, sActor, sPlayer);
        break;

    case PROJ_EXTRA:
        if(bSet)
            thisprojectile[proj].extra=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].extra, sActor, sPlayer);
        break;

    case PROJ_DECAL:
        if(bSet)
            thisprojectile[proj].decal=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].decal, sActor, sPlayer);
        break;

    case PROJ_TRAIL:
        if(bSet)
            thisprojectile[proj].trail=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].trail, sActor, sPlayer);
        break;

    case PROJ_TXREPEAT:
        if(bSet)
            thisprojectile[proj].txrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].txrepeat, sActor, sPlayer);
        break;

    case PROJ_TYREPEAT:
        if(bSet)
            thisprojectile[proj].tyrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].tyrepeat, sActor, sPlayer);
        break;

    case PROJ_TOFFSET:
        if(bSet)
            thisprojectile[proj].toffset=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].toffset, sActor, sPlayer);
        break;

    case PROJ_TNUM:
        if(bSet)
            thisprojectile[proj].tnum=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].tnum, sActor, sPlayer);
        break;

    case PROJ_DROP:
        if(bSet)
            thisprojectile[proj].drop=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].drop, sActor, sPlayer);
        break;

    case PROJ_CSTAT:
        if(bSet)
            thisprojectile[proj].cstat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].cstat, sActor, sPlayer);
        break;

    case PROJ_CLIPDIST:
        if(bSet)
            thisprojectile[proj].clipdist=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].clipdist, sActor, sPlayer);
        break;

    case PROJ_SHADE:
        if(bSet)
            thisprojectile[proj].shade=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].shade, sActor, sPlayer);
        break;

    case PROJ_XREPEAT:
        if(bSet)
            thisprojectile[proj].xrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].xrepeat, sActor, sPlayer);
        break;

    case PROJ_YREPEAT:
        if(bSet)
            thisprojectile[proj].yrepeat=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].yrepeat, sActor, sPlayer);
        break;

    case PROJ_PAL:
        if(bSet)
            thisprojectile[proj].pal=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].pal, sActor, sPlayer);
        break;

    case PROJ_EXTRA_RAND:
        if(bSet)
            thisprojectile[proj].extra_rand=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].extra_rand, sActor, sPlayer);
        break;

    case PROJ_HITRADIUS:
        if(bSet)
            thisprojectile[proj].hitradius=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].hitradius, sActor, sPlayer);
        break;

    case PROJ_VEL_MULT:
        if(bSet)
            thisprojectile[proj].velmult=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].velmult, sActor, sPlayer);
        break;

    case PROJ_OFFSET:
        if(bSet)
            thisprojectile[proj].offset=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].offset, sActor, sPlayer);
        break;

    case PROJ_BOUNCES:
        if(bSet)
            thisprojectile[proj].bounces=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].bounces, sActor, sPlayer);
        break;

    case PROJ_BSOUND:
        if(bSet)
            thisprojectile[proj].bsound=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].bsound, sActor, sPlayer);
        break;

    case PROJ_RANGE:
        if(bSet)
            thisprojectile[proj].range=lValue;
        else
            SetGameVarID((int)lVar2, thisprojectile[proj].range, sActor, sPlayer);
        break;

    default:
        break;
    }
    return;
}

void DoPlayer(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer, long lParm2)
{
    int iPlayer;
    long lValue;
    long lTemp;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    if(lVar1 == g_iThisActorID )
        // if they've asked for 'this', then use 'this player'...
        iPlayer=g_p;
    else
        iPlayer=GetGameVarID((int)lVar1, sActor, sPlayer);

    if(iPlayer<0 || iPlayer >= MAXPLAYERS)
        return;

    switch(lLabelID)
    {
    case PLAYER_ZOOM:
        if(bSet)
            ps[iPlayer].zoom=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].zoom, sActor, sPlayer);
        break;

    case PLAYER_EXITX:
        if(bSet)
            ps[iPlayer].exitx=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].exitx, sActor, sPlayer);
        break;

    case PLAYER_EXITY:
        if(bSet)
            ps[iPlayer].exity=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].exity, sActor, sPlayer);
        break;

    case PLAYER_LOOGIEX:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].loogiex[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].loogiex[lTemp], sActor, sPlayer);
        break;

    case PLAYER_LOOGIEY:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].loogiey[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].loogiey[lTemp], sActor, sPlayer);
        break;

    case PLAYER_NUMLOOGS:
        if(bSet)
            ps[iPlayer].numloogs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].numloogs, sActor, sPlayer);
        break;

    case PLAYER_LOOGCNT:
        if(bSet)
            ps[iPlayer].loogcnt=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].loogcnt, sActor, sPlayer);
        break;

    case PLAYER_POSX:
        if(bSet)
            ps[iPlayer].posx=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].posx, sActor, sPlayer);
        break;

    case PLAYER_POSY:
        if(bSet)
            ps[iPlayer].posy=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].posy, sActor, sPlayer);
        break;

    case PLAYER_POSZ:
        if(bSet)
            ps[iPlayer].posz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].posz, sActor, sPlayer);
        break;

    case PLAYER_HORIZ:
        if(bSet)
            ps[iPlayer].horiz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].horiz, sActor, sPlayer);
        break;

    case PLAYER_OHORIZ:
        if(bSet)
            ps[iPlayer].ohoriz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].ohoriz, sActor, sPlayer);
        break;

    case PLAYER_OHORIZOFF:
        if(bSet)
            ps[iPlayer].ohorizoff=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].ohorizoff, sActor, sPlayer);
        break;

    case PLAYER_INVDISPTIME:
        if(bSet)
            ps[iPlayer].invdisptime=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].invdisptime, sActor, sPlayer);
        break;

    case PLAYER_BOBPOSX:
        if(bSet)
            ps[iPlayer].bobposx=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].bobposx, sActor, sPlayer);
        break;

    case PLAYER_BOBPOSY:
        if(bSet)
            ps[iPlayer].bobposy=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].bobposy, sActor, sPlayer);
        break;

    case PLAYER_OPOSX:
        if(bSet)
            ps[iPlayer].oposx=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].oposx, sActor, sPlayer);
        break;

    case PLAYER_OPOSY:
        if(bSet)
            ps[iPlayer].oposy=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].oposy, sActor, sPlayer);
        break;

    case PLAYER_OPOSZ:
        if(bSet)
            ps[iPlayer].oposz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].oposz, sActor, sPlayer);
        break;

    case PLAYER_PYOFF:
        if(bSet)
            ps[iPlayer].pyoff=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].pyoff, sActor, sPlayer);
        break;

    case PLAYER_OPYOFF:
        if(bSet)
            ps[iPlayer].opyoff=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].opyoff, sActor, sPlayer);
        break;

    case PLAYER_POSXV:
        if(bSet)
            ps[iPlayer].posxv=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].posxv, sActor, sPlayer);
        break;

    case PLAYER_POSYV:
        if(bSet)
            ps[iPlayer].posyv=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].posyv, sActor, sPlayer);
        break;

    case PLAYER_POSZV:
        if(bSet)
            ps[iPlayer].poszv=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].poszv, sActor, sPlayer);
        break;

    case PLAYER_LAST_PISSED_TIME:
        if(bSet)
            ps[iPlayer].last_pissed_time=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].last_pissed_time, sActor, sPlayer);
        break;

    case PLAYER_TRUEFZ:
        if(bSet)
            ps[iPlayer].truefz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].truefz, sActor, sPlayer);
        break;

    case PLAYER_TRUECZ:
        if(bSet)
            ps[iPlayer].truecz=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].truecz, sActor, sPlayer);
        break;

    case PLAYER_PLAYER_PAR:
        if(bSet)
            ps[iPlayer].player_par=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].player_par, sActor, sPlayer);
        break;

    case PLAYER_VISIBILITY:
        if(bSet)
            ps[iPlayer].visibility=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].visibility, sActor, sPlayer);
        break;

    case PLAYER_BOBCOUNTER:
        if(bSet)
            ps[iPlayer].bobcounter=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].bobcounter, sActor, sPlayer);
        break;

    case PLAYER_WEAPON_SWAY:
        if(bSet)
            ps[iPlayer].weapon_sway=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weapon_sway, sActor, sPlayer);
        break;

    case PLAYER_PALS_TIME:
        if(bSet)
            ps[iPlayer].pals_time=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].pals_time, sActor, sPlayer);
        break;

    case PLAYER_RANDOMFLAMEX:
        if(bSet)
            ps[iPlayer].randomflamex=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].randomflamex, sActor, sPlayer);
        break;

    case PLAYER_CRACK_TIME:
        if(bSet)
            ps[iPlayer].crack_time=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].crack_time, sActor, sPlayer);
        break;

    case PLAYER_AIM_MODE:
        if(bSet)
            ps[iPlayer].aim_mode=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].aim_mode, sActor, sPlayer);
        break;

    case PLAYER_ANG:
        if(bSet)
            ps[iPlayer].ang=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].ang, sActor, sPlayer);
        break;

    case PLAYER_OANG:
        if(bSet)
            ps[iPlayer].oang=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].oang, sActor, sPlayer);
        break;

    case PLAYER_ANGVEL:
        if(bSet)
            ps[iPlayer].angvel=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].angvel, sActor, sPlayer);
        break;

    case PLAYER_CURSECTNUM:
        if(bSet)
            ps[iPlayer].cursectnum=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].cursectnum, sActor, sPlayer);
        break;

    case PLAYER_LOOK_ANG:
        if(bSet)
            ps[iPlayer].look_ang=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].look_ang, sActor, sPlayer);
        break;

    case PLAYER_LAST_EXTRA:
        if(bSet)
            ps[iPlayer].last_extra=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].last_extra, sActor, sPlayer);
        break;

    case PLAYER_SUBWEAPON:
        if(bSet)
            ps[iPlayer].subweapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].subweapon, sActor, sPlayer);
        break;

    case PLAYER_AMMO_AMOUNT:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].ammo_amount[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].ammo_amount[lTemp], sActor, sPlayer);
        break;

    case PLAYER_WACKEDBYACTOR:
        if(bSet)
            ps[iPlayer].wackedbyactor=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].wackedbyactor, sActor, sPlayer);
        break;

    case PLAYER_FRAG:
        if(bSet)
            ps[iPlayer].frag=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].frag, sActor, sPlayer);
        break;

    case PLAYER_FRAGGEDSELF:
        if(bSet)
            ps[iPlayer].fraggedself=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].fraggedself, sActor, sPlayer);
        break;

    case PLAYER_CURR_WEAPON:
        if(bSet)
            ps[iPlayer].curr_weapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].curr_weapon, sActor, sPlayer);
        break;

    case PLAYER_LAST_WEAPON:
        if(bSet)
            ps[iPlayer].last_weapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].last_weapon, sActor, sPlayer);
        break;

    case PLAYER_TIPINCS:
        if(bSet)
            ps[iPlayer].tipincs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].tipincs, sActor, sPlayer);
        break;

    case PLAYER_HORIZOFF:
        if(bSet)
            ps[iPlayer].horizoff=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].horizoff, sActor, sPlayer);
        break;

    case PLAYER_WANTWEAPONFIRE:
        if(bSet)
            ps[iPlayer].wantweaponfire=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].wantweaponfire, sActor, sPlayer);
        break;

    case PLAYER_HOLODUKE_AMOUNT:
        if(bSet)
            ps[iPlayer].holoduke_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].holoduke_amount, sActor, sPlayer);
        break;

    case PLAYER_NEWOWNER:
        if(bSet)
            ps[iPlayer].newowner=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].newowner, sActor, sPlayer);
        break;

    case PLAYER_HURT_DELAY:
        if(bSet)
            ps[iPlayer].hurt_delay=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].hurt_delay, sActor, sPlayer);
        break;

    case PLAYER_HBOMB_HOLD_DELAY:
        if(bSet)
            ps[iPlayer].hbomb_hold_delay=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].hbomb_hold_delay, sActor, sPlayer);
        break;

    case PLAYER_JUMPING_COUNTER:
        if(bSet)
            ps[iPlayer].jumping_counter=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].jumping_counter, sActor, sPlayer);
        break;

    case PLAYER_AIRLEFT:
        if(bSet)
            ps[iPlayer].airleft=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].airleft, sActor, sPlayer);
        break;

    case PLAYER_KNEE_INCS:
        if(bSet)
            ps[iPlayer].knee_incs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].knee_incs, sActor, sPlayer);
        break;

    case PLAYER_ACCESS_INCS:
        if(bSet)
            ps[iPlayer].access_incs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].access_incs, sActor, sPlayer);
        break;

    case PLAYER_FTA:
        if(bSet)
            ps[iPlayer].fta=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].fta, sActor, sPlayer);
        break;

    case PLAYER_FTQ:
        if(bSet)
            ps[iPlayer].ftq=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].ftq, sActor, sPlayer);
        break;

    case PLAYER_ACCESS_WALLNUM:
        if(bSet)
            ps[iPlayer].access_wallnum=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].access_wallnum, sActor, sPlayer);
        break;

    case PLAYER_ACCESS_SPRITENUM:
        if(bSet)
            ps[iPlayer].access_spritenum=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].access_spritenum, sActor, sPlayer);
        break;

    case PLAYER_KICKBACK_PIC:
        if(bSet)
            ps[iPlayer].kickback_pic=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].kickback_pic, sActor, sPlayer);
        break;

    case PLAYER_GOT_ACCESS:
        if(bSet)
            ps[iPlayer].got_access=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].got_access, sActor, sPlayer);
        break;

    case PLAYER_WEAPON_ANG:
        if(bSet)
            ps[iPlayer].weapon_ang=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weapon_ang, sActor, sPlayer);
        break;

    case PLAYER_FIRSTAID_AMOUNT:
        if(bSet)
            ps[iPlayer].firstaid_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].firstaid_amount, sActor, sPlayer);
        break;

    case PLAYER_SOMETHINGONPLAYER:
        if(bSet)
            ps[iPlayer].somethingonplayer=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].somethingonplayer, sActor, sPlayer);
        break;

    case PLAYER_ON_CRANE:
        if(bSet)
            ps[iPlayer].on_crane=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].on_crane, sActor, sPlayer);
        break;

    case PLAYER_I:
        if(bSet)
            ps[iPlayer].i=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].i, sActor, sPlayer);
        break;

    case PLAYER_ONE_PARALLAX_SECTNUM:
        if(bSet)
            ps[iPlayer].one_parallax_sectnum=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].one_parallax_sectnum, sActor, sPlayer);
        break;

    case PLAYER_OVER_SHOULDER_ON:
        if(bSet)
            ps[iPlayer].over_shoulder_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].over_shoulder_on, sActor, sPlayer);
        break;

    case PLAYER_RANDOM_CLUB_FRAME:
        if(bSet)
            ps[iPlayer].random_club_frame=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].random_club_frame, sActor, sPlayer);
        break;

    case PLAYER_FIST_INCS:
        if(bSet)
            ps[iPlayer].fist_incs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].fist_incs, sActor, sPlayer);
        break;

    case PLAYER_ONE_EIGHTY_COUNT:
        if(bSet)
            ps[iPlayer].one_eighty_count=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].one_eighty_count, sActor, sPlayer);
        break;

    case PLAYER_CHEAT_PHASE:
        if(bSet)
            ps[iPlayer].cheat_phase=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].cheat_phase, sActor, sPlayer);
        break;

    case PLAYER_DUMMYPLAYERSPRITE:
        if(bSet)
            ps[iPlayer].dummyplayersprite=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].dummyplayersprite, sActor, sPlayer);
        break;

    case PLAYER_EXTRA_EXTRA8:
        if(bSet)
            ps[iPlayer].extra_extra8=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].extra_extra8, sActor, sPlayer);
        break;

    case PLAYER_QUICK_KICK:
        if(bSet)
            ps[iPlayer].quick_kick=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].quick_kick, sActor, sPlayer);
        break;

    case PLAYER_HEAT_AMOUNT:
        if(bSet)
            ps[iPlayer].heat_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].heat_amount, sActor, sPlayer);
        break;

    case PLAYER_ACTORSQU:
        if(bSet)
            ps[iPlayer].actorsqu=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].actorsqu, sActor, sPlayer);
        break;

    case PLAYER_TIMEBEFOREEXIT:
        if(bSet)
            ps[iPlayer].timebeforeexit=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].timebeforeexit, sActor, sPlayer);
        break;

    case PLAYER_CUSTOMEXITSOUND:
        if(bSet)
            ps[iPlayer].customexitsound=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].customexitsound, sActor, sPlayer);
        break;

    case PLAYER_WEAPRECS:
        if(bSet)
            ps[iPlayer].weaprecs[16]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weaprecs[16], sActor, sPlayer);
        break;

    case PLAYER_WEAPRECCNT:
        if(bSet)
            ps[iPlayer].weapreccnt=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weapreccnt, sActor, sPlayer);
        break;

    case PLAYER_INTERFACE_TOGGLE_FLAG:
        if(bSet)
            ps[iPlayer].interface_toggle_flag=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].interface_toggle_flag, sActor, sPlayer);
        break;

    case PLAYER_ROTSCRNANG:
        if(bSet)
            ps[iPlayer].rotscrnang=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].rotscrnang, sActor, sPlayer);
        break;

    case PLAYER_DEAD_FLAG:
        if(bSet)
            ps[iPlayer].dead_flag=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].dead_flag, sActor, sPlayer);
        break;

    case PLAYER_SHOW_EMPTY_WEAPON:
        if(bSet)
            ps[iPlayer].show_empty_weapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].show_empty_weapon, sActor, sPlayer);
        break;

    case PLAYER_SCUBA_AMOUNT:
        if(bSet)
            ps[iPlayer].scuba_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].scuba_amount, sActor, sPlayer);
        break;

    case PLAYER_JETPACK_AMOUNT:
        if(bSet)
            ps[iPlayer].jetpack_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].jetpack_amount, sActor, sPlayer);
        break;

    case PLAYER_STEROIDS_AMOUNT:
        if(bSet)
            ps[iPlayer].steroids_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].steroids_amount, sActor, sPlayer);
        break;

    case PLAYER_SHIELD_AMOUNT:
        if(bSet)
            ps[iPlayer].shield_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].shield_amount, sActor, sPlayer);
        break;

    case PLAYER_HOLODUKE_ON:
        if(bSet)
            ps[iPlayer].holoduke_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].holoduke_on, sActor, sPlayer);
        break;

    case PLAYER_PYCOUNT:
        if(bSet)
            ps[iPlayer].pycount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].pycount, sActor, sPlayer);
        break;

    case PLAYER_WEAPON_POS:
        if(bSet)
            ps[iPlayer].weapon_pos=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weapon_pos, sActor, sPlayer);
        break;

    case PLAYER_FRAG_PS:
        if(bSet)
            ps[iPlayer].frag_ps=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].frag_ps, sActor, sPlayer);
        break;

    case PLAYER_TRANSPORTER_HOLD:
        if(bSet)
            ps[iPlayer].transporter_hold=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].transporter_hold, sActor, sPlayer);
        break;

    case PLAYER_LAST_FULL_WEAPON:
        if(bSet)
            ps[iPlayer].last_full_weapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].last_full_weapon, sActor, sPlayer);
        break;

    case PLAYER_FOOTPRINTSHADE:
        if(bSet)
            ps[iPlayer].footprintshade=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].footprintshade, sActor, sPlayer);
        break;

    case PLAYER_BOOT_AMOUNT:
        if(bSet)
            ps[iPlayer].boot_amount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].boot_amount, sActor, sPlayer);
        break;

    case PLAYER_SCREAM_VOICE:
        if(bSet)
            ps[iPlayer].scream_voice=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].scream_voice, sActor, sPlayer);
        break;

    case PLAYER_GM:
        if(bSet)
            ps[iPlayer].gm=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].gm, sActor, sPlayer);
        break;

    case PLAYER_ON_WARPING_SECTOR:
        if(bSet)
            ps[iPlayer].on_warping_sector=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].on_warping_sector, sActor, sPlayer);
        break;

    case PLAYER_FOOTPRINTCOUNT:
        if(bSet)
            ps[iPlayer].footprintcount=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].footprintcount, sActor, sPlayer);
        break;

    case PLAYER_HBOMB_ON:
        if(bSet)
            ps[iPlayer].hbomb_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].hbomb_on, sActor, sPlayer);
        break;

    case PLAYER_JUMPING_TOGGLE:
        if(bSet)
            ps[iPlayer].jumping_toggle=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].jumping_toggle, sActor, sPlayer);
        break;

    case PLAYER_RAPID_FIRE_HOLD:
        if(bSet)
            ps[iPlayer].rapid_fire_hold=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].rapid_fire_hold, sActor, sPlayer);
        break;

    case PLAYER_ON_GROUND:
        if(bSet)
            ps[iPlayer].on_ground=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].on_ground, sActor, sPlayer);
        break;

    case PLAYER_NAME:
        if(bSet)
            ps[iPlayer].name[32]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].name[32], sActor, sPlayer);
        break;

    case PLAYER_INVEN_ICON:
        if(bSet)
            ps[iPlayer].inven_icon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].inven_icon, sActor, sPlayer);
        break;

    case PLAYER_BUTTONPALETTE:
        if(bSet)
            ps[iPlayer].buttonpalette=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].buttonpalette, sActor, sPlayer);
        break;

    case PLAYER_JETPACK_ON:
        if(bSet)
            ps[iPlayer].jetpack_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].jetpack_on, sActor, sPlayer);
        break;

    case PLAYER_SPRITEBRIDGE:
        if(bSet)
            ps[iPlayer].spritebridge=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].spritebridge, sActor, sPlayer);
        break;

    case PLAYER_LASTRANDOMSPOT:
        if(bSet)
            ps[iPlayer].lastrandomspot=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].lastrandomspot, sActor, sPlayer);
        break;

    case PLAYER_SCUBA_ON:
        if(bSet)
            ps[iPlayer].scuba_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].scuba_on, sActor, sPlayer);
        break;

    case PLAYER_FOOTPRINTPAL:
        if(bSet)
            ps[iPlayer].footprintpal=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].footprintpal, sActor, sPlayer);
        break;

    case PLAYER_HEAT_ON:
        if(bSet)
            ps[iPlayer].heat_on=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].heat_on, sActor, sPlayer);
        break;

    case PLAYER_HOLSTER_WEAPON:
        if(bSet)
            ps[iPlayer].holster_weapon=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].holster_weapon, sActor, sPlayer);
        break;

    case PLAYER_FALLING_COUNTER:
        if(bSet)
            ps[iPlayer].falling_counter=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].falling_counter, sActor, sPlayer);
        break;

    case PLAYER_GOTWEAPON:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].gotweapon[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].gotweapon[lTemp], sActor, sPlayer);
        break;

    case PLAYER_REFRESH_INVENTORY:
        if(bSet)
            ps[iPlayer].refresh_inventory=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].refresh_inventory, sActor, sPlayer);
        break;

        //      case PLAYER_PALETTE:
        //          if(bSet)
        //          {
        //              ps[iPlayer].palette=lValue;
        //          }
        //          else
        //          {
        //              SetGameVarID((int)lVar2, ps[iPlayer].palette, sActor, sPlayer);
        //          }
        //          break;

    case PLAYER_TOGGLE_KEY_FLAG:
        if(bSet)
            ps[iPlayer].toggle_key_flag=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].toggle_key_flag, sActor, sPlayer);
        break;

    case PLAYER_KNUCKLE_INCS:
        if(bSet)
            ps[iPlayer].knuckle_incs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].knuckle_incs, sActor, sPlayer);
        break;

    case PLAYER_WALKING_SND_TOGGLE:
        if(bSet)
            ps[iPlayer].walking_snd_toggle=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].walking_snd_toggle, sActor, sPlayer);
        break;

    case PLAYER_PALOOKUP:
        if(bSet)
            ps[iPlayer].palookup=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].palookup, sActor, sPlayer);
        break;

    case PLAYER_HARD_LANDING:
        if(bSet)
            ps[iPlayer].hard_landing=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].hard_landing, sActor, sPlayer);
        break;

    case PLAYER_MAX_SECRET_ROOMS:
        if(bSet)
            ps[iPlayer].max_secret_rooms=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].max_secret_rooms, sActor, sPlayer);
        break;

    case PLAYER_SECRET_ROOMS:
        if(bSet)
            ps[iPlayer].secret_rooms=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].secret_rooms, sActor, sPlayer);
        break;

    case PLAYER_PALS:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].pals[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].pals[lTemp], sActor, sPlayer);
        break;

    case PLAYER_MAX_ACTORS_KILLED:
        if(bSet)
            ps[iPlayer].max_actors_killed=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].max_actors_killed, sActor, sPlayer);
        break;

    case PLAYER_ACTORS_KILLED:
        if(bSet)
            ps[iPlayer].actors_killed=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].actors_killed, sActor, sPlayer);
        break;

    case PLAYER_RETURN_TO_CENTER:
        if(bSet)
            ps[iPlayer].return_to_center=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].return_to_center, sActor, sPlayer);
        break;

    case PLAYER_RUNSPEED:
        if(bSet)
            ps[iPlayer].runspeed=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].runspeed, sActor, sPlayer);
        break;

    case PLAYER_SBS:
        if(bSet)
            ps[iPlayer].sbs=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].sbs, sActor, sPlayer);
        break;

    case PLAYER_RELOADING:
        if(bSet)
            ps[iPlayer].reloading=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].reloading, sActor, sPlayer);
        break;

    case PLAYER_AUTO_AIM:
        if(bSet)
            ps[iPlayer].auto_aim=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].auto_aim, sActor, sPlayer);
        break;

    case PLAYER_MOVEMENT_LOCK:
        lTemp=lParm2;
        if(bSet)
            ps[iPlayer].movement_lock[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].movement_lock[lTemp], sActor, sPlayer);
        break;

    case PLAYER_SOUND_PITCH:
        if(bSet)
            ps[iPlayer].sound_pitch=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].sound_pitch, sActor, sPlayer);
        break;

    case PLAYER_WEAPONSWITCH:
        if(bSet)
            ps[iPlayer].weaponswitch=lValue;
        else
            SetGameVarID((int)lVar2, ps[iPlayer].weaponswitch, sActor, sPlayer);
        break;

    default:
        break;
    }
    return;
}

void DoInput(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    int iPlayer;
    long lValue;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    if(lVar1 == g_iThisActorID )
    {
        // if they've asked for 'this', then use 'this player'...
        iPlayer=g_p;
    }
    else
        iPlayer=GetGameVarID((int)lVar1, sActor, sPlayer);

    if(iPlayer<0 || iPlayer >= MAXPLAYERS)
        return;

    switch(lLabelID)
    {
    case INPUT_AVEL:
        if(bSet)
            sync[iPlayer].avel=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].avel, sActor, sPlayer);
        break;

    case INPUT_HORZ:
        if(bSet)
            sync[iPlayer].horz=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].horz, sActor, sPlayer);
        break;

    case INPUT_FVEL:
        if(bSet)
            sync[iPlayer].fvel=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].fvel, sActor, sPlayer);
        break;

    case INPUT_SVEL:
        if(bSet)
            sync[iPlayer].svel=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].svel, sActor, sPlayer);
        break;

    case INPUT_BITS:
        if(bSet)
            sync[iPlayer].bits=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].bits, sActor, sPlayer);
        break;

    case INPUT_BITS2:
        if(bSet)
            sync[iPlayer].bits2=lValue;
        else
            SetGameVarID((int)lVar2, sync[iPlayer].bits2, sActor, sPlayer);
        break;
    default:
        break;
    }
    return;
}

void DoWall(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    int iWall;
    long lValue;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    iWall=GetGameVarID((int)lVar1, sActor, sPlayer);

    if(iWall<0 || iWall >= MAXWALLS)
        return;

    switch(lLabelID)
    {
    case WALL_X:
        if(bSet)
            wall[iWall].x=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].x, sActor, sPlayer);
        break;

    case WALL_Y:
        if(bSet)
            wall[iWall].y=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].y, sActor, sPlayer);
        break;

    case WALL_POINT2:
        if(bSet)
            wall[iWall].point2=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].point2, sActor, sPlayer);
        break;

    case WALL_NEXTWALL:
        if(bSet)
            wall[iWall].nextwall=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].nextwall, sActor, sPlayer);
        break;

    case WALL_NEXTSECTOR:
        if(bSet)
            wall[iWall].nextsector=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].nextsector, sActor, sPlayer);
        break;

    case WALL_CSTAT:
        if(bSet)
            wall[iWall].cstat=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].cstat, sActor, sPlayer);
        break;

    case WALL_PICNUM:
        if(bSet)
            wall[iWall].picnum=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].picnum, sActor, sPlayer);
        break;

    case WALL_OVERPICNUM:
        if(bSet)
            wall[iWall].overpicnum=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].overpicnum, sActor, sPlayer);
        break;

    case WALL_SHADE:
        if(bSet)
            wall[iWall].shade=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].shade, sActor, sPlayer);
        break;

    case WALL_PAL:
        if(bSet)
            wall[iWall].pal=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].pal, sActor, sPlayer);
        break;

    case WALL_XREPEAT:
        if(bSet)
            wall[iWall].xrepeat=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].xrepeat, sActor, sPlayer);
        break;

    case WALL_YREPEAT:
        if(bSet)
            wall[iWall].yrepeat=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].yrepeat, sActor, sPlayer);
        break;

    case WALL_XPANNING:
        if(bSet)
            wall[iWall].xpanning=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].xpanning, sActor, sPlayer);
        break;

    case WALL_YPANNING:
        if(bSet)
            wall[iWall].ypanning=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].ypanning, sActor, sPlayer);
        break;

    case WALL_LOTAG:
        if(bSet)
            wall[iWall].lotag=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].lotag, sActor, sPlayer);
        break;

    case WALL_HITAG:
        if(bSet)
            wall[iWall].hitag=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].hitag, sActor, sPlayer);
        break;

    case WALL_EXTRA:
        if(bSet)
            wall[iWall].extra=lValue;
        else
            SetGameVarID((int)lVar2, wall[iWall].extra, sActor, sPlayer);
        break;
    default:
        break;
    }
    return;
}

void DoSector(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    int iSector;
    long lValue;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    if(lVar1 == g_iThisActorID )
        // if they've asked for 'this', then use 'this'...
        iSector=sprite[g_i].sectnum;
    else
        iSector=GetGameVarID((int)lVar1, sActor, sPlayer);

    if(iSector<0 || iSector >= MAXSECTORS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    switch(lLabelID)
    {
    case SECTOR_WALLPTR:
        if(bSet)
            sector[iSector].wallptr=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].wallptr,sActor,sPlayer);
        break;

    case SECTOR_WALLNUM:
        if(bSet)
            sector[iSector].wallnum=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].wallnum,sActor,sPlayer);
        break;

    case SECTOR_CEILINGZ:
        if(bSet)
            sector[iSector].ceilingz=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingz,sActor,sPlayer);
        break;

    case SECTOR_FLOORZ:
        if(bSet)
            sector[iSector].floorz=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorz,sActor,sPlayer);
        break;

    case SECTOR_CEILINGSTAT:
        if(bSet)
            sector[iSector].ceilingstat=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingstat,sActor,sPlayer);
        break;

    case SECTOR_FLOORSTAT:
        if(bSet)
            sector[iSector].floorstat=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorstat,sActor,sPlayer);
        break;

    case SECTOR_CEILINGPICNUM:
        if(bSet)
            sector[iSector].ceilingpicnum=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingpicnum,sActor,sPlayer);
        break;

    case SECTOR_CEILINGSLOPE:
        if(bSet)
            sector[iSector].ceilingheinum=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingheinum,sActor,sPlayer);
        break;

    case SECTOR_CEILINGSHADE:
        if(bSet)
            sector[iSector].ceilingshade=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingshade,sActor,sPlayer);
        break;

    case SECTOR_CEILINGPAL:
        if(bSet)
            sector[iSector].ceilingpal=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingpal,sActor,sPlayer);
        break;

    case SECTOR_CEILINGXPANNING:
        if(bSet)
            sector[iSector].ceilingxpanning=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].ceilingypanning,sActor,sPlayer);
        break;

    case SECTOR_FLOORPICNUM:
        if(bSet)
            sector[iSector].floorpicnum=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorpicnum,sActor,sPlayer);
        break;

    case SECTOR_FLOORSLOPE:
        if(bSet)
            sector[iSector].floorheinum=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorheinum,sActor,sPlayer);
        break;

    case SECTOR_FLOORSHADE:
        if(bSet)
            sector[iSector].floorshade=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorshade,sActor,sPlayer);
        break;

    case SECTOR_FLOORPAL:
        if(bSet)
            sector[iSector].floorpal=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorpal,sActor,sPlayer);
        break;

    case SECTOR_FLOORXPANNING:
        if(bSet)
            sector[iSector].floorxpanning=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorxpanning,sActor,sPlayer);
        break;

    case SECTOR_FLOORYPANNING:
        if(bSet)
            sector[iSector].floorypanning=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].floorypanning,sActor,sPlayer);
        break;

    case SECTOR_VISIBILITY:
        if(bSet)
            sector[iSector].visibility=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].visibility,sActor,sPlayer);
        break;

    case SECTOR_ALIGNTO:
        if(bSet)
            sector[iSector].filler=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].filler,sActor,sPlayer);
        break;

    case SECTOR_LOTAG:
        if(bSet)
            sector[iSector].lotag=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].lotag,sActor,sPlayer);
        break;

    case SECTOR_HITAG:
        if(bSet)
            sector[iSector].hitag=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].hitag,sActor,sPlayer);
        break;

    case SECTOR_EXTRA:
        if(bSet)
            sector[iSector].extra=lValue;
        else
            SetGameVarID((int)lVar2, sector[iSector].extra,sActor,sPlayer);
        break;

    default:
        break;
    }
    return;
}

void DoActor(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer, long lParm2)
{
    int iActor;
    long lValue;
    long lTemp;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    if(lVar1 == g_iThisActorID )
        // if they've asked for 'this', then use 'this'...
        iActor=g_i;
    else
        iActor=GetGameVarID((int)lVar1, sActor, sPlayer);

    if(iActor < 0 || iActor >= MAXSPRITES)
        return;

    switch(lLabelID)
    {
    case ACTOR_X:
        if(bSet)
            sprite[iActor].x=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].x,sActor,sPlayer);
        break;

    case ACTOR_Y:
        if(bSet)
            sprite[iActor].y=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].y,sActor,sPlayer);
        break;

    case ACTOR_Z:
        if(bSet)
            sprite[iActor].z=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].z,sActor,sPlayer);
        break;

    case ACTOR_CSTAT:
        if(bSet)
            sprite[iActor].cstat=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].cstat,sActor,sPlayer);
        break;

    case ACTOR_PICNUM:
        if(bSet)
            sprite[iActor].picnum=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].picnum,sActor,sPlayer);
        break;

    case ACTOR_SHADE:
        if(bSet)
            sprite[iActor].shade=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].shade,sActor,sPlayer);
        break;

    case ACTOR_PAL:
        if(bSet)
            sprite[iActor].pal=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].pal,sActor,sPlayer);
        break;

    case ACTOR_CLIPDIST:
        if(bSet)
            sprite[iActor].clipdist=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].clipdist,sActor,sPlayer);
        break;

    case ACTOR_DETAIL:
        if(bSet)
            sprite[iActor].filler=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].filler,sActor,sPlayer);
        break;

    case ACTOR_XREPEAT:
        if(bSet)
            sprite[iActor].xrepeat=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].xrepeat,sActor,sPlayer);
        break;

    case ACTOR_YREPEAT:
        if(bSet)
            sprite[iActor].yrepeat=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].yrepeat,sActor,sPlayer);
        break;

    case ACTOR_XOFFSET:
        if(bSet)
            sprite[iActor].xoffset=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].xoffset,sActor,sPlayer);
        break;

    case ACTOR_YOFFSET:
        if(bSet)
            sprite[iActor].yoffset=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].yoffset,sActor,sPlayer);
        break;

    case ACTOR_SECTNUM:
        if(bSet)
            changespritesect(iActor,lValue);
        else
            SetGameVarID((int)lVar2, sprite[iActor].sectnum,sActor,sPlayer);
        break;

    case ACTOR_STATNUM:
        if(bSet)
            changespritestat(iActor,lValue);
        else
            SetGameVarID((int)lVar2, sprite[iActor].statnum,sActor,sPlayer);
        break;

    case ACTOR_ANG:
        if(bSet)
            sprite[iActor].ang=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].ang,sActor,sPlayer);
        break;

    case ACTOR_OWNER:
        if(bSet)
            sprite[iActor].owner=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].owner,sActor,sPlayer);
        break;

    case ACTOR_XVEL:
        if(bSet)
            sprite[iActor].xvel=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].xvel,sActor,sPlayer);
        break;

    case ACTOR_YVEL:
        if(bSet)
            sprite[iActor].yvel=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].yvel,sActor,sPlayer);
        break;

    case ACTOR_ZVEL:
        if(bSet)
            sprite[iActor].zvel=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].zvel,sActor,sPlayer);
        break;

    case ACTOR_LOTAG:
        if(bSet)
            sprite[iActor].lotag=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].lotag,sActor,sPlayer);
        break;

    case ACTOR_HITAG:
        if(bSet)
            sprite[iActor].hitag=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].hitag,sActor,sPlayer);
        break;

    case ACTOR_EXTRA:
        if(bSet)
            sprite[iActor].extra=lValue;
        else
            SetGameVarID((int)lVar2, sprite[iActor].extra,sActor,sPlayer);
        break;

    case ACTOR_HTCGG:
        if(bSet)
            hittype[iActor].cgg=lValue;
        else
            SetGameVarID((int)lVar2, hittype[iActor].cgg, sActor, sPlayer);
        break;

    case ACTOR_HTPICNUM :
        if(bSet)
            hittype[iActor].picnum=lValue;
        else
            SetGameVarID((int)lVar2, hittype[iActor].picnum, sActor, sPlayer);
        break;

    case ACTOR_HTANG:
        if(bSet)
            hittype[iActor].ang=lValue;
        else
            SetGameVarID((int)lVar2, hittype[iActor].ang, sActor, sPlayer);
        break;

    case ACTOR_HTEXTRA:
        if(bSet)
            hittype[iActor].extra=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].extra, sActor, sPlayer);
        break;

    case ACTOR_HTOWNER:
        if(bSet)
            hittype[iActor].owner=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].owner, sActor, sPlayer);
        break;

    case ACTOR_HTMOVFLAG:
        if(bSet)
            hittype[iActor].movflag=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].movflag, sActor, sPlayer);
        break;

    case ACTOR_HTTEMPANG:
        if(bSet)
            hittype[iActor].tempang=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].tempang, sActor, sPlayer);
        break;

    case ACTOR_HTACTORSTAYPUT:
        if(bSet)
            hittype[iActor].actorstayput=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].actorstayput, sActor, sPlayer);
        break;

    case ACTOR_HTDISPICNUM:
        if(bSet)
            hittype[iActor].dispicnum=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].dispicnum, sActor, sPlayer);
        break;

    case ACTOR_HTTIMETOSLEEP:
        if(bSet)
            hittype[iActor].timetosleep=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].timetosleep, sActor, sPlayer);
        break;

    case ACTOR_HTFLOORZ:
        if(bSet)
            hittype[iActor].floorz=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].floorz, sActor, sPlayer);
        break;

    case ACTOR_HTCEILINGZ:
        if(bSet)
            hittype[iActor].ceilingz=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].ceilingz, sActor, sPlayer);
        break;

    case ACTOR_HTLASTVX:
        if(bSet)
            hittype[iActor].lastvx=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].lastvx, sActor, sPlayer);
        break;

    case ACTOR_HTLASTVY:
        if(bSet)
            hittype[iActor].lastvy=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].lastvy, sActor, sPlayer);
        break;

    case ACTOR_HTBPOSX:
        if(bSet)
            hittype[iActor].bposx=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].bposx, sActor, sPlayer);
        break;

    case ACTOR_HTBPOSY:
        if(bSet)
            hittype[iActor].bposy=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].bposy, sActor, sPlayer);
        break;

    case ACTOR_HTBPOSZ:
        if(bSet)
            hittype[iActor].bposz=lValue;
        else
            SetGameVarID((int)lVar2,hittype[iActor].bposz, sActor, sPlayer);
        break;

    case ACTOR_HTG_T:
        lTemp=lParm2;
        if(bSet)
            hittype[iActor].temp_data[lTemp]=lValue;
        else
            SetGameVarID((int)lVar2, hittype[iActor].temp_data[lTemp], sActor, sPlayer);
        break;

    case ACTOR_ANGOFF:
        if(bSet)
            spriteext[iActor].angoff=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].angoff, sActor, sPlayer);
        break;

    case ACTOR_PITCH:
        if(bSet)
            spriteext[iActor].pitch=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].pitch, sActor, sPlayer);
        break;

    case ACTOR_ROLL:
        if(bSet)
            spriteext[iActor].roll=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].roll, sActor, sPlayer);
        break;

    case ACTOR_MDXOFF:
        if(bSet)
            spriteext[iActor].xoff=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].xoff, sActor, sPlayer);
        break;

    case ACTOR_MDYOFF:
        if(bSet)
            spriteext[iActor].yoff=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].yoff, sActor, sPlayer);
        break;

    case ACTOR_MDZOFF:
        if(bSet)
            spriteext[iActor].zoff=lValue;
        else
            SetGameVarID((int)lVar2,spriteext[iActor].zoff, sActor, sPlayer);
        break;

    default:
        break;
    }
    return;
}

void DoProjectile(char bSet, long lVar1, long lLabelID, long lVar2, short sActor, short sPlayer)
{
    long lValue,proj;

    //     proj=GetGameVarID((int)lVar1, sActor, sPlayer);
    proj=lVar1;

    if(sActor < 0 || sActor >= MAXSPRITES || sPlayer < 0 || sPlayer >= MAXPLAYERS)
        return;

    lValue=GetGameVarID((int)lVar2, sActor, sPlayer);

    switch(lLabelID)
    {
    case PROJ_WORKSLIKE:
        if(bSet)
            projectile[proj].workslike=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].workslike, sActor, sPlayer);
        break;

    case PROJ_SPAWNS:
        if(bSet)
            projectile[proj].spawns=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].spawns, sActor, sPlayer);
        break;

    case PROJ_SXREPEAT:
        if(bSet)
            projectile[proj].sxrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].sxrepeat, sActor, sPlayer);
        break;

    case PROJ_SYREPEAT:
        if(bSet)
            projectile[proj].syrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].syrepeat, sActor, sPlayer);
        break;

    case PROJ_SOUND:
        if(bSet)
            projectile[proj].sound=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].sound, sActor, sPlayer);
        break;

    case PROJ_ISOUND:
        if(bSet)
            projectile[proj].isound=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].isound, sActor, sPlayer);
        break;

    case PROJ_VEL:
        if(bSet)
            projectile[proj].vel=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].vel, sActor, sPlayer);
        break;

    case PROJ_EXTRA:
        if(bSet)
            projectile[proj].extra=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].extra, sActor, sPlayer);
        break;

    case PROJ_DECAL:
        if(bSet)
            projectile[proj].decal=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].decal, sActor, sPlayer);
        break;

    case PROJ_TRAIL:
        if(bSet)
            projectile[proj].trail=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].trail, sActor, sPlayer);
        break;

    case PROJ_TXREPEAT:
        if(bSet)
            projectile[proj].txrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].txrepeat, sActor, sPlayer);
        break;

    case PROJ_TYREPEAT:
        if(bSet)
            projectile[proj].tyrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].tyrepeat, sActor, sPlayer);
        break;

    case PROJ_TOFFSET:
        if(bSet)
            projectile[proj].toffset=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].toffset, sActor, sPlayer);
        break;

    case PROJ_TNUM:
        if(bSet)
            projectile[proj].tnum=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].tnum, sActor, sPlayer);
        break;

    case PROJ_DROP:
        if(bSet)
            projectile[proj].drop=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].drop, sActor, sPlayer);
        break;

    case PROJ_CSTAT:
        if(bSet)
            projectile[proj].cstat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].cstat, sActor, sPlayer);
        break;

    case PROJ_CLIPDIST:
        if(bSet)
            projectile[proj].clipdist=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].clipdist, sActor, sPlayer);
        break;

    case PROJ_SHADE:
        if(bSet)
            projectile[proj].shade=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].shade, sActor, sPlayer);
        break;

    case PROJ_XREPEAT:
        if(bSet)
            projectile[proj].xrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].xrepeat, sActor, sPlayer);
        break;

    case PROJ_YREPEAT:
        if(bSet)
            projectile[proj].yrepeat=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].yrepeat, sActor, sPlayer);
        break;

    case PROJ_PAL:
        if(bSet)
            projectile[proj].pal=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].pal, sActor, sPlayer);
        break;

    case PROJ_EXTRA_RAND:
        if(bSet)
            projectile[proj].extra_rand=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].extra_rand, sActor, sPlayer);
        break;

    case PROJ_HITRADIUS:
        if(bSet)
            projectile[proj].hitradius=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].hitradius, sActor, sPlayer);
        break;

    case PROJ_VEL_MULT:
        if(bSet)
            projectile[proj].velmult=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].velmult, sActor, sPlayer);
        break;

    case PROJ_OFFSET:
        if(bSet)
            projectile[proj].offset=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].offset, sActor, sPlayer);
        break;

    case PROJ_BOUNCES:
        if(bSet)
            projectile[proj].bounces=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].bounces, sActor, sPlayer);
        break;

    case PROJ_BSOUND:
        if(bSet)
            projectile[proj].bsound=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].bsound, sActor, sPlayer);
        break;

    case PROJ_RANGE:
        if(bSet)
            projectile[proj].range=lValue;
        else
            SetGameVarID((int)lVar2, projectile[proj].range, sActor, sPlayer);
        break;

    default:
        break;
    }
    return;
}

void OnEvent(int iEventID, short sActor,short sPlayer,long lDist)
{
    short og_i,og_p;
    long og_x;
    long *og_t;
    spritetype *og_sp;
    char okillit_flag;
    long *oinsptr;

    char done;

    if( iEventID >= MAXGAMEEVENTS)
    {
        AddLog("Invalid Event ID");
        return;
    }

    if( apScriptGameEvent[iEventID] == 0 )
    {
        //Bsprintf(g_szBuf,"No event found for %d",iEventID);
        //AddLog(g_szBuf);
        return;
    }

    // save current values...
    og_i=g_i;
    og_p=g_p;
    og_x=g_x;
    og_sp=g_sp;
    og_t=g_t;
    okillit_flag=killit_flag;
    oinsptr=insptr;

    g_i = sActor;    // current sprite ID
    g_p = sPlayer;    // current player ID
    g_x = lDist;    // ?
    g_sp = &sprite[g_i];
    g_t = &hittype[g_i].temp_data[0];

    insptr = (apScriptGameEvent[iEventID]);
    //Bsprintf(g_szBuf,"Executing event for %d at %lX",iEventID, insptr);
    //AddLog(g_szBuf);

    killit_flag = 0;
    do
        done = parse();
    while( done == 0 );

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

char dodge(spritetype *s)
{
    short i;
    long bx,by,mx,my,bxvect,byvect,mxvect,myvect,d;

    mx = s->x;
    my = s->y;
    mxvect = sintable[(s->ang+512)&2047]; myvect = sintable[s->ang&2047];

    for(i=headspritestat[4];i>=0;i=nextspritestat[i]) //weapons list
    {
        if( OW == i || SECT != s->sectnum)
            continue;

        bx = SX-mx;
        by = SY-my;
        bxvect = sintable[(SA+512)&2047]; byvect = sintable[SA&2047];

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

short furthestangle(short sActor,short angs)
{
    short j, hitsect,hitwall,hitspr,furthest_angle=0, angincs;
    long hx, hy, hz, d, greatestd;
    spritetype *s = &sprite[sActor];

    greatestd = -(1<<30);
    angincs = 2048/angs;

    if(s->picnum != APLAYER)
        if( (g_t[0]&63) > 2 ) return( s->ang + 1024 );

    for(j=s->ang;j<(2048+s->ang);j+=angincs)
    {
        hitscan(s->x, s->y, s->z-(8<<8), s->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047],0,
                &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-s->x) + klabs(hy-s->y);

        if(d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return (furthest_angle&2047);
}

short furthestcanseepoint(short sActor,spritetype *ts,long *dax,long *day)
{
    short j, hitsect,hitwall,hitspr, angincs;
    long hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
    spritetype *s = &sprite[sActor];

    if( (g_t[0]&63) ) return -1;

    if(ud.multimode < 2 && ud.player_skill < 3)
        angincs = 2048/2;
    else angincs = 2048/(1+(TRAND&1));

    for(j=ts->ang;j<(2048+ts->ang);j+=(angincs-(TRAND&511)))
    {
        hitscan(ts->x, ts->y, ts->z-(16<<8), ts->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047],16384-(TRAND&32767),
                &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-ts->x)+klabs(hy-ts->y);
        da = klabs(hx-s->x)+klabs(hy-s->y);

        if( d < da )
            if(cansee(hx,hy,hz,hitsect,s->x,s->y,s->z-(16<<8),s->sectnum))
            {
                *dax = hx;
                *day = hy;
                return hitsect;
            }
    }
    return -1;
}

void getglobalz(short sActor)
{
    long hz,lz,zr;

    spritetype *s = &sprite[sActor];

    if( s->statnum == 10 || s->statnum == 6 || s->statnum == 2 || s->statnum == 1 || s->statnum == 4)
    {
        if(s->statnum == 4)
            zr = 4L;
        else zr = 127L;

        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[sActor].ceilingz,&hz,&hittype[sActor].floorz,&lz,zr,CLIPMASK0);

        if( (lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0 )
        {
            lz &= (MAXSPRITES-1);
            if( badguy(&sprite[lz]) && sprite[lz].pal != 1)
            {
                if( s->statnum != 4 )
                {
                    hittype[sActor].dispicnum = -4; // No shadows on actors
                    s->xvel = -256;
                    ssp(sActor,CLIPMASK0);
                }
            }
            else if(sprite[lz].picnum == APLAYER && badguy(s) )
            {
                hittype[sActor].dispicnum = -4; // No shadows on actors
                s->xvel = -256;
                ssp(sActor,CLIPMASK0);
            }
            else if(s->statnum == 4 && sprite[lz].picnum == APLAYER)
                if(s->owner == lz)
                {
                    hittype[sActor].ceilingz = sector[s->sectnum].ceilingz;
                    hittype[sActor].floorz   = sector[s->sectnum].floorz;
                }
        }
    }
    else
    {
        hittype[sActor].ceilingz = sector[s->sectnum].ceilingz;
        hittype[sActor].floorz   = sector[s->sectnum].floorz;
    }
}

void makeitfall(short sActor)
{
    spritetype *s = &sprite[sActor];
    long hz,lz,c;

    if( floorspace(s->sectnum) )
        c = 0;
    else
    {
        if( ceilingspace(s->sectnum) || sector[s->sectnum].lotag == 2)
            c = gc/6;
        else c = gc;
    }

    if( ( s->statnum == 1 || s->statnum == 10 || s->statnum == 2 || s->statnum == 6 ) )
        getzrange(s->x,s->y,s->z-(FOURSLEIGHT),s->sectnum,&hittype[sActor].ceilingz,&hz,&hittype[sActor].floorz,&lz,127L,CLIPMASK0);
    else
    {
        hittype[sActor].ceilingz = sector[s->sectnum].ceilingz;
        hittype[sActor].floorz   = sector[s->sectnum].floorz;
    }

    if( s->z < hittype[sActor].floorz-(FOURSLEIGHT) )
    {
        if( sector[s->sectnum].lotag == 2 && s->zvel > 3122 )
            s->zvel = 3144;
        if(s->zvel < 6144)
            s->zvel += c;
        else s->zvel = 6144;
        s->z += s->zvel;
    }
    if( s->z >= hittype[sActor].floorz-(FOURSLEIGHT) )
    {
        s->z = hittype[sActor].floorz - FOURSLEIGHT;
        s->zvel = 0;
    }
}

short getincangle(short a,short na)
{
    a &= 2047;
    na &= 2047;

    if(klabs(a-na) < 1024)
        return (na-a);
    else
    {
        if(na > 1024) na -= 2048;
        if(a > 1024) a -= 2048;

        na -= 2048;
        a -= 2048;
        return (na-a);
    }
}

void alterang(short a)
{
    short aang, angdif, goalang,j;
    long ticselapsed, *moveptr;

    moveptr = (long *)g_t[1];

    ticselapsed = (g_t[0])&31;

    aang = g_sp->ang;

    g_sp->xvel += (*moveptr-g_sp->xvel)/5;
    if(g_sp->zvel < 648) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)/5;

    if(a&seekplayer)
    {
        j = ps[g_p].holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        if(j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,g_sp->x,g_sp->y,g_sp->z,g_sp->sectnum) )
            g_sp->owner = j;
        else g_sp->owner = ps[g_p].i;

        if(sprite[g_sp->owner].picnum == APLAYER)
            goalang = getangle(hittype[g_i].lastvx-g_sp->x,hittype[g_i].lastvy-g_sp->y);
        else
            goalang = getangle(sprite[g_sp->owner].x-g_sp->x,sprite[g_sp->owner].y-g_sp->y);

        if(g_sp->xvel && g_sp->picnum != DRONE)
        {
            angdif = getincangle(aang,goalang);

            if(ticselapsed < 2)
            {
                if( klabs(angdif) < 256)
                {
                    j = 128-(TRAND&256);
                    g_sp->ang += j;
                    if( hits(g_i) < 844 )
                        g_sp->ang -= j;
                }
            }
            else if(ticselapsed > 18 && ticselapsed < 26) // choose
            {
                if(klabs(angdif>>2) < 128) g_sp->ang = goalang;
                else g_sp->ang += angdif>>2;
            }
        }
        else g_sp->ang = goalang;
    }

    if(ticselapsed < 1)
    {
        j = 2;
        if(a&furthestdir)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang;
            g_sp->owner = ps[g_p].i;
        }

        if(a&fleeenemy)
        {
            goalang = furthestangle(g_i,j);
            g_sp->ang = goalang; // += angdif; //  = getincangle(aang,goalang)>>1;
        }
    }
}

void move()
{
    long l, *moveptr;
    short a, goalang, angdif;
    long daxvel;

    a = g_sp->hitag;

    if(a == -1) a = 0;

    g_t[0]++;

    if(a&face_player)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        g_sp->ang += angdif;
    }

    if(a&spin)
        g_sp->ang += sintable[ ((g_t[0]<<3)&2047) ]>>6;

    if(a&face_player_slow)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-g_sp->x,ps[g_p].oposy-g_sp->y);
        else goalang = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);
        angdif = ksgn(getincangle(g_sp->ang,goalang))<<5;
        if(angdif > -32 && angdif < 0)
        {
            angdif = 0;
            g_sp->ang = goalang;
        }
        g_sp->ang += angdif;
    }

    if((a&jumptoplayer) == jumptoplayer)
    {
        if(g_t[0] < 16)
            g_sp->zvel -= (sintable[(512+(g_t[0]<<4))&2047]>>5);
    }

    if(a&face_player_smart)
    {
        long newx,newy;

        newx = ps[g_p].posx+(ps[g_p].posxv/768);
        newy = ps[g_p].posy+(ps[g_p].posyv/768);
        goalang = getangle(newx-g_sp->x,newy-g_sp->y);
        angdif = getincangle(g_sp->ang,goalang)>>2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        g_sp->ang += angdif;
    }

    if( g_t[1] == 0 || a == 0 )
    {
        if( ( badguy(g_sp) && g_sp->extra <= 0 ) || (hittype[g_i].bposx != g_sp->x) || (hittype[g_i].bposy != g_sp->y) )
        {
            hittype[g_i].bposx = g_sp->x;
            hittype[g_i].bposy = g_sp->y;
            setsprite(g_i,g_sp->x,g_sp->y,g_sp->z);
        }
        return;
    }

    moveptr = (long *)g_t[1];

    if(a&geth) g_sp->xvel += (*moveptr-g_sp->xvel)>>1;
    if(a&getv) g_sp->zvel += ((*(moveptr+1)<<4)-g_sp->zvel)>>1;

    if(a&dodgebullet)
        dodge(g_sp);

    if(g_sp->picnum != APLAYER)
        alterang(a);

    if(g_sp->xvel > -6 && g_sp->xvel < 6 ) g_sp->xvel = 0;

    a = badguy(g_sp);

    if(g_sp->xvel || g_sp->zvel)
    {
        if(a && g_sp->picnum != ROTATEGUN)
        {
            if( (g_sp->picnum == DRONE || g_sp->picnum == COMMANDER) && g_sp->extra > 0)
            {
                if(g_sp->picnum == COMMANDER)
                {
                    hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( g_sp->z > (l-(8<<8)) )
                    {
                        if( g_sp->z > (l-(8<<8)) ) g_sp->z = l-(8<<8);
                        g_sp->zvel = 0;
                    }

                    hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( (g_sp->z-l) < (80<<8) )
                    {
                        g_sp->z = l+(80<<8);
                        g_sp->zvel = 0;
                    }
                }
                else
                {
                    if( g_sp->zvel > 0 )
                    {
                        hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if( g_sp->z > (l-(30<<8)) )
                            g_sp->z = l-(30<<8);
                    }
                    else
                    {
                        hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                        if( (g_sp->z-l) < (50<<8) )
                        {
                            g_sp->z = l+(50<<8);
                            g_sp->zvel = 0;
                        }
                    }
                }
            }
            else if(g_sp->picnum != ORGANTIC)
            {
                if(g_sp->zvel > 0 && hittype[g_i].floorz < g_sp->z)
                    g_sp->z = hittype[g_i].floorz;
                if( g_sp->zvel < 0)
                {
                    l = getceilzofslope(g_sp->sectnum,g_sp->x,g_sp->y);
                    if( (g_sp->z-l) < (66<<8) )
                    {
                        g_sp->z = l+(66<<8);
                        g_sp->zvel >>= 1;
                    }
                }
            }
        }
        else if(g_sp->picnum == APLAYER)
            if( (g_sp->z-hittype[g_i].ceilingz) < (32<<8) )
                g_sp->z = hittype[g_i].ceilingz+(32<<8);

        daxvel = g_sp->xvel;
        angdif = g_sp->ang;

        if( a && g_sp->picnum != ROTATEGUN )
        {
            if( g_x < 960 && g_sp->xrepeat > 16 )
            {

                daxvel = -(1024-g_x);
                angdif = getangle(ps[g_p].posx-g_sp->x,ps[g_p].posy-g_sp->y);

                if(g_x < 512)
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
            else if(g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != COMMANDER)
            {
                if( hittype[g_i].bposz != g_sp->z || ( ud.multimode < 2 && ud.player_skill < 2 ) )
                {
                    if( (g_t[0]&1) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if( (g_t[0]&3) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 2;
                }
            }
        }

        hittype[g_i].movflag = movesprite(g_i,
                                          (daxvel*(sintable[(angdif+512)&2047]))>>14,
                                          (daxvel*(sintable[angdif&2047]))>>14,g_sp->zvel,CLIPMASK0);
    }

    if( a )
    {
        if (sector[g_sp->sectnum].ceilingstat&1)
            g_sp->shade += (sector[g_sp->sectnum].ceilingshade-g_sp->shade)>>1;
        else g_sp->shade += (sector[g_sp->sectnum].floorshade-g_sp->shade)>>1;

        if( sector[g_sp->sectnum].floorpicnum == MIRROR )
            deletesprite(g_i);
    }
}

char parse(void);

void parseifelse(long condition)
{
    if( condition )
    {
        // skip 'else' pointer.. and...
        insptr+=2;
        parse();
    }
    else
    {
        insptr = (long *) *(insptr+1);
        if(*insptr == 10)
        {
            // else...
            // skip 'else' and...
            insptr+=2;
            parse();
        }
    }
}

// long *it = 0x00589a04;

char parse(void)
{
    long j, l, s, tw;

    if (!(error || warning) && condebug)
    {
        //        Bsprintf(g_szBuf,"  * DEBUG! Executing: %s",keyw[*insptr]);
        //        AddLog(g_szBuf);
    }

    if(killit_flag) return 1;

    //    if(*it == 1668249134L) gameexit("\nERR");
    //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
    //      AddLog(g_szBuf);

    tw = *insptr;

    switch(tw)
    {
    case CON_REDEFINEQUOTE:
        {
            int q, i;
            insptr++;
            q = *insptr++;
            i = *insptr++;
            Bstrcpy(fta_quotes[q],redefined_fta_quotes[i]);
            break;
        }

    case CON_GETTHISPROJECTILE:
    case CON_SETTHISPROJECTILE:
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;
            lVar2=*insptr++;
            DoThisProjectile(tw==CON_SETTHISPROJECTILE,lVar1,lLabelID,lVar2,g_i,g_p);
            break;
        }

    case CON_IFRND:
        insptr++;
        parseifelse( rnd(*insptr));
        break;

    case CON_IFCANSHOOTTARGET:

        if(g_x > 1024)
        {
            short temphit, sclip, angdif;

            if( badguy(g_sp) && g_sp->xrepeat > 56 )
            {
                sclip = 3084;
                angdif = 48;
            }
            else
            {
                sclip = 768;
                angdif = 16;
            }

            j = hitasprite(g_i,&temphit);
            if(j == (1<<30))
            {
                parseifelse(1);
                break;
            }
            if(j > sclip)
            {
                if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                    j = 0;
                else
                {
                    g_sp->ang += angdif;j = hitasprite(g_i,&temphit);g_sp->ang -= angdif;
                    if(j > sclip)
                    {
                        if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
                            j = 0;
                        else
                        {
                            g_sp->ang -= angdif;j = hitasprite(g_i,&temphit);g_sp->ang += angdif;
                            if( j > 768 )
                            {
                                if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
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
        if( j ) hittype[g_i].timetosleep = SLEEPTIME;
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
            if(ps[g_p].holoduke_on >= 0)
            {
                s = &sprite[ps[g_p].holoduke_on];
                j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((32<<8)-1)),g_sp->sectnum,
                           s->x,s->y,s->z,s->sectnum);

                if(j == 0)
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

            if(j == 0)
            {
                // they can't see it.

                // Huh?.  This does nothing....
                // (the result is always j==0....)
                if( ( klabs(hittype[g_i].lastvx-g_sp->x)+klabs(hittype[g_i].lastvy-g_sp->y) ) <
                        ( klabs(hittype[g_i].lastvx-s->x)+klabs(hittype[g_i].lastvy-s->y) ) )
                    j = 0;

                // um yeah, this if() will always fire....
                if( j == 0 )
                {
                    // search around for target player

                    // also modifies 'target' x&y if found..

                    j = furthestcanseepoint(g_i,s,&hittype[g_i].lastvx,&hittype[g_i].lastvy);

                    if(j == -1) j = 0;
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

            if( j == 1 && ( g_sp->statnum == 1 || g_sp->statnum == 6 ) )
                hittype[g_i].timetosleep = SLEEPTIME;

            parseifelse(j == 1);
            break;
        }

    case CON_IFHITWEAPON:
        parseifelse(ifhitbyweapon(g_i) >= 0);
        break;

    case CON_IFSQUISHED:
        parseifelse( ifsquished(g_i, g_p) == 1);
        break;

    case CON_IFDEAD:
        {
            j = g_sp->extra;
            if(g_sp->picnum == APLAYER)
                j--;
            parseifelse(j < 0);
        }
        break;

    case CON_AI:
        insptr++;
        g_t[5] = *insptr++;
        g_t[4] = *(long *)(g_t[5]);       // Action
        g_t[1] = *(long *)(g_t[5]+4);       // move
        g_sp->hitag = *(long *)(g_t[5]+8);    // Ai
        g_t[0] = g_t[2] = g_t[3] = 0;
        if(g_sp->hitag&random_angle)
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
        if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
            hittype[g_i].timetosleep = SLEEPTIME;
        break;

    case CON_IFPDISTG:
        insptr++;
        parseifelse(g_x > *insptr);
        if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
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

        if((gametype_flags[ud.coop]&GAMETYPE_FLAG_WEAPSTAY) && ud.multimode > 1)
        {
            if(*insptr == 0)
            {
                for(j=0;j < ps[g_p].weapreccnt;j++)
                    if( ps[g_p].weaprecs[j] == g_sp->picnum )
                        break;

                parseifelse(j < ps[g_p].weapreccnt && g_sp->owner == g_i);
            }
            else if(ps[g_p].weapreccnt < 16)
            {
                ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
                parseifelse(g_sp->owner == g_i);
            }
        }
        else parseifelse(0);
        break;

    case CON_GETLASTPAL:
        insptr++;
        if(g_sp->picnum == APLAYER)
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
        if(!isspritemakingsound(g_i,g_sp->yvel))
            spritesound(g_sp->yvel,g_i);
        break;

    case CON_PKICK:
        insptr++;

        if( ud.multimode > 1 && g_sp->picnum == APLAYER )
        {
            if(ps[otherp].quick_kick == 0)
                ps[otherp].quick_kick = 14;
        }
        else if(g_sp->picnum != APLAYER && ps[g_p].quick_kick == 0)
            ps[g_p].quick_kick = 14;
        break;

    case CON_SIZETO:
        insptr++;

        j = (*insptr++-g_sp->xrepeat)<<1;
        g_sp->xrepeat += ksgn(j);

        if( ( g_sp->picnum == APLAYER && g_sp->yrepeat < 36 ) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat*(tilesizy[g_sp->picnum]+8))<<2) < (hittype[g_i].floorz - hittype[g_i].ceilingz) )
        {
            j = ((*insptr)-g_sp->yrepeat)<<1;
            if( klabs(j) ) g_sp->yrepeat += ksgn(j);
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
        if(!isspritemakingsound(g_i,*insptr))
            spritesound((short) *insptr,g_i);
        insptr++;
        break;

    case CON_IFSOUND:
        insptr++;
        parseifelse( Sound[*insptr].num > 0 );
        //    parseifelse(SoundOwner[*insptr][0].i == g_i);
        break;

    case CON_STOPSOUND:
        insptr++;
        if(isspritemakingsound(g_i,*insptr))
            stopspritesound((short)*insptr,g_i);
        insptr++;
        break;

    case CON_GLOBALSOUND:
        insptr++;
        if(g_p == screenpeek || (gametype_flags[ud.coop]&GAMETYPE_FLAG_COOPSOUND))
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
            long c;

            if( floorspace(g_sp->sectnum) )
                c = 0;
            else
            {
                if( ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
                    c = gc/6;
                else c = gc;
            }

            if( hittype[g_i].cgg <= 0 || (sector[g_sp->sectnum].floorstat&2) )
            {
                getglobalz(g_i);
                hittype[g_i].cgg = 6;
            }
            else hittype[g_i].cgg --;

            if( g_sp->z < (hittype[g_i].floorz-FOURSLEIGHT) )
            {
                g_sp->zvel += c;
                g_sp->z+=g_sp->zvel;

                if(g_sp->zvel > 6144) g_sp->zvel = 6144;
            }
            else
            {
                g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

                if( badguy(g_sp) || ( g_sp->picnum == APLAYER && g_sp->owner >= 0) )
                {

                    if( g_sp->zvel > 3084 && g_sp->extra <= 1)
                    {
                        if(g_sp->pal != 1 && g_sp->picnum != DRONE)
                        {
                            if(g_sp->picnum == APLAYER && g_sp->extra > 0)
                                goto SKIPJIBS;
                            guts(g_sp,JIBS6,15,g_p);
                            spritesound(SQUISHED,g_i);
                            spawn(g_i,BLOODPOOL);
                        }

SKIPJIBS:

                        hittype[g_i].picnum = SHOTSPARK1;
                        hittype[g_i].extra = 1;
                        g_sp->zvel = 0;
                    }
                    else if(g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != 1)
                    {

                        j = g_sp->sectnum;
                        pushmove(&g_sp->x,&g_sp->y,&g_sp->z,(short*)&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        if(j != g_sp->sectnum && j >= 0 && j < MAXSECTORS)
                            changespritesect(g_i,j);

                        spritesound(THUD,g_i);
                    }
                }
                if(sector[g_sp->sectnum].lotag == 1)
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
        if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
        {
            killit_flag = 2;
            break;
        }
        addammo( *insptr, &ps[g_p], *(insptr+1) );
        if(ps[g_p].curr_weapon == KNEE_WEAPON)
            if( ps[g_p].gotweapon[*insptr]) {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
                else addweapon( &ps[g_p], *insptr );
            }
        insptr += 2;
        break;

    case CON_MONEY:
        insptr++;
        lotsofmoney(g_sp,*insptr++);
        break;

    case CON_MAIL:
        insptr++;
        lotsofmail(g_sp,*insptr++);
        break;

    case CON_SLEEPTIME:
        insptr++;
        hittype[g_i].timetosleep = (short)*insptr++;
        break;

    case CON_PAPER:
        insptr++;
        lotsofpaper(g_sp,*insptr++);
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
        if( ps[g_p].gotweapon[*insptr] == 0 ) {
            if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
            else addweapon( &ps[g_p], *insptr );
        }
        else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
        {
            killit_flag = 2;
            break;
        }
        addammo( *insptr, &ps[g_p], *(insptr+1) );
        if(ps[g_p].curr_weapon == KNEE_WEAPON)
            if( ps[g_p].gotweapon[*insptr]) {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], *insptr);
                else addweapon( &ps[g_p], *insptr );
            }
        insptr+=2;
        break;

    case CON_DEBUG:
        insptr++;
        printf("%ld\n",*insptr++);
        break;

    case CON_ENDOFGAME:
        insptr++;
        ps[g_p].timebeforeexit = *insptr++;
        ps[g_p].customexitsound = -1;
        ud.eog = 1;
        break;

    case CON_ADDPHEALTH:
        insptr++;

        if(ps[g_p].newowner >= 0)
        {
            ps[g_p].newowner = -1;
            ps[g_p].posx = ps[g_p].oposx;
            ps[g_p].posy = ps[g_p].oposy;
            ps[g_p].posz = ps[g_p].oposz;
            ps[g_p].ang = ps[g_p].oang;
            updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
            setpal(&ps[g_p]);

            j = headspritestat[1];
            while(j >= 0)
            {
                if(sprite[j].picnum==CAMERA1)
                    sprite[j].yvel = 0;
                j = nextspritestat[j];
            }
        }

        j = sprite[ps[g_p].i].extra;

        if(g_sp->picnum != ATOMICHEALTH)
        {
            if( j > max_player_health && *insptr > 0 )
            {
                insptr++;
                break;
            }
            else
            {
                if(j > 0)
                    j += *insptr;
                if ( j > max_player_health && *insptr > 0 )
                    j = max_player_health;
            }
        }
        else
        {
            if( j > 0 )
                j += *insptr;
            if ( j > (max_player_health<<1) )
                j = (max_player_health<<1);
        }

        if(j < 0) j = 0;

        if(ud.god == 0)
        {
            if(*insptr > 0)
            {
                if( ( j - *insptr ) < (max_player_health>>2) &&
                        j >= (max_player_health>>2) )
                    spritesound(DUKE_GOTHEALTHATLOW,ps[g_p].i);

                ps[g_p].last_extra = j;
            }

            sprite[ps[g_p].i].extra = j;
        }

        insptr++;
        break;

    case CON_STATE:
        {
            long *tempscrptr;

            tempscrptr = insptr+2;

            insptr = (long *) *(insptr+1);
            while(1) if(parse()) break;
            insptr = tempscrptr;
        }
        break;

    case CON_LEFTBRACE:
        insptr++;
        while(1) if(parse()) break;
        break;

    case CON_MOVE:
        insptr++;
        g_t[0]=0;
        g_t[1] = *insptr++;
        g_sp->hitag = *insptr++;
        if(g_sp->hitag&random_angle)
            g_sp->ang = TRAND&2047;
        break;

    case CON_ADDWEAPONVAR:
        insptr++;
        if( ps[g_p].gotweapon[GetGameVarID(*(insptr),g_i,g_p)] == 0 ) {
            if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
            else addweapon( &ps[g_p], GetGameVarID(*(insptr),g_i,g_p) );
        }
        else if( ps[g_p].ammo_amount[GetGameVarID(*(insptr),g_i,g_p)] >= max_ammo_amount[GetGameVarID(*(insptr),g_i,g_p)] )
        {
            killit_flag = 2;
            break;
        }
        addammo( GetGameVarID(*(insptr),g_i,g_p), &ps[g_p], GetGameVarID(*(insptr+1),g_i,g_p) );
        if(ps[g_p].curr_weapon == KNEE_WEAPON)
            if( ps[g_p].gotweapon[GetGameVarID(*(insptr),g_i,g_p)] ) {
                if (!(ps[g_p].weaponswitch & 1)) addweaponnoswitch(&ps[g_p], GetGameVarID(*(insptr),g_i,g_p));
                else addweapon( &ps[g_p], GetGameVarID(*(insptr),g_i,g_p) );
            }
        insptr+=2;
        break;

    case CON_ACTIVATEBYSECTOR:
    case CON_OPERATESECTORS:
    case CON_OPERATEACTIVATORS:
    case CON_SETASPECT:
    case CON_SSP:
        {
            long var1, var2;

            insptr++;
            var1 = GetGameVarID(*insptr++,g_i,g_p);
            var2 = GetGameVarID(*insptr++,g_i,g_p);

            switch(tw)
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

    case CON_OPERATERESPAWNS:
    case CON_OPERATEMASTERSWITCHES:
    case CON_CHECKACTIVATORMOTION:
        {
            long var1;

            insptr++;
            var1 = GetGameVarID(*insptr++,g_i,g_p);

            switch(tw)
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

    case CON_GETPNAME:
    case CON_QSTRCAT:
    case CON_QSTRCPY:
    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
        {
            int i,j;
            insptr++;
            i = GetGameVarID(*insptr++, g_i, g_p);
            j = GetGameVarID(*insptr++, g_i, g_p);
            switch(tw)
            {
            case CON_GETPNAME:
                if (ud.user_name[j][0] != 0)
                    Bsprintf(fta_quotes[i],ud.user_name[j]);
                else Bsprintf(fta_quotes[i],"%d",j);
                break;
            case CON_QSTRCAT:
                Bstrncat(fta_quotes[i],fta_quotes[j],63-Bstrlen(fta_quotes[i]));
                break;
            case CON_QSTRCPY:
                Bstrcpy(fta_quotes[i],fta_quotes[j]);
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
        {
            // from 'level' cheat in game.c (about line 6250)
            long volnume;
            long levnume;
            int i;

            insptr++; // skip command
            volnume=GetGameVarID(*insptr++,g_i,g_p);
            levnume=GetGameVarID(*insptr++,g_i,g_p);

            if(volnume > num_volumes || volnume < 0)
            {
                /*
                if( g_cmddebug&CMDDEBUG_COMPILE)
                {
                Bsprintf(g_szBuf,"startlevel: Invalid Volume number: %ld.  Command ignored.",volnume);
                AddLog(g_szBuf);
                }
                */
                break;
            }

            if(levnume >= 11 || levnume <0)
            {
                /*
                if( g_cmddebug&CMDDEBUG_COMPILE)
                {
                Bsprintf(g_szBuf,"startlevel: Invalid Level number: %ld.  Command ignored.",levnume);
                AddLog(g_szBuf);
                }
                */
                break;
            }

            ud.m_volume_number = ud.volume_number = volnume;
            ud.m_level_number = ud.level_number = levnume;
            if(numplayers > 1 && myconnectindex == connecthead)
            {
                tempbuf[0] = 5;
                tempbuf[1] = ud.m_level_number;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_player_skill;
                tempbuf[4] = ud.m_monsters_off;
                tempbuf[5] = ud.m_respawn_monsters;
                tempbuf[6] = ud.m_respawn_items;
                tempbuf[7] = ud.m_respawn_inventory;
                tempbuf[8] = ud.m_coop;
                tempbuf[9] = ud.m_marker;
                tempbuf[10] = ud.m_ffire;

                for(i=connecthead;i>=0;i=connectpoint2[i])
                    sendpacket(i,tempbuf,11);
            }
        else { ps[myconnectindex].gm |= MODE_EOL; display_bonus_screen = 0; } // MODE_RESTART;

            break;
        }

    case CON_MYOSX:
    case CON_MYOSPALX:
    case CON_MYOS:
    case CON_MYOSPAL:
        {
            long x,y;
            short tilenum;
            signed char shade;
            char orientation;
            char pal;

            insptr++;
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            tilenum=GetGameVarID(*insptr++,g_i,g_p);
            shade=GetGameVarID(*insptr++,g_i,g_p);
            orientation=GetGameVarID(*insptr++,g_i,g_p);

            switch(tw)
            {
            case CON_MYOS:
                myos(x,y,tilenum,shade,orientation);
                break;
            case CON_MYOSPAL:
                pal=GetGameVarID(*insptr++,g_i,g_p);
                myospal(x,y,tilenum,shade,orientation,pal);
                break;
            case CON_MYOSX:
                myosx(x,y,tilenum,shade,orientation);
                break;
            case CON_MYOSPALX:
                pal=GetGameVarID(*insptr++,g_i,g_p);
                myospalx(x,y,tilenum,shade,orientation,pal);
                break;
            }
            break;
        }

    case CON_SWITCH:
        {
            long lVarID;
            long lValue;
            long *lpDefault;
            long *lpCases;
            long lCases;
            long lEnd;
            long lCheckCase;
            char bMatched;
            long *lTempInsPtr;

            // command format:
            // variable ID to check
            // script offset to 'end'
            // count of case statements
            // script offset to default case (null if none)
            // For each case: value, ptr to code
            //AddLog("Processing Switch...");
            insptr++; // p-code
            lVarID=*insptr++;
            lValue=GetGameVarID(lVarID, g_i, g_p);
            lEnd=*insptr++;
            lCases=*insptr++;
            lpDefault=insptr++;
            lpCases=insptr;
            insptr+=lCases*2;
            bMatched=0;
            lTempInsPtr=insptr;
            //Bsprintf(g_szBuf,"lEnd= %ld *lpDefault=%ld",lEnd,*lpDefault);
            //AddLog(g_szBuf);

            //Bsprintf(g_szBuf,"Checking %ld cases for %ld",lCases, lValue);
            //AddLog(g_szBuf);
            for(lCheckCase=0; lCheckCase<lCases && !bMatched; lCheckCase++)
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
                    while(1)
                    {
                        if(parse())
                            break;
                    }
                    //AddLog("Done Executing Case");
                    bMatched=1;
                }
            }
            if(!bMatched)
            {
                if(*lpDefault)
                {
                    //AddLog("No Matching Case: Using Default");
                    insptr=(long*)(*lpDefault + &script[0]);
                    while(1) if(parse()) break;
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
        {
            int i;
            insptr++;

            i=*insptr++;  // ID of def
            SetGameVarID(i, rand(), g_i, g_p );
            break;
        }

    case CON_DRAGPOINT:
        {
            short wallnum;
            long newx, newy;

            insptr++;
            wallnum = GetGameVarID(*insptr++, g_i, g_p);
            newx = GetGameVarID(*insptr++, g_i, g_p);
            newy = GetGameVarID(*insptr++, g_i, g_p);

            dragpoint(wallnum,newx,newy);
            break;
        }

    case CON_DIST:
    case CON_LDIST:
    case CON_TXDIST:
        {
            long distvar, xvar, yvar, distx=0;

            insptr++;
            distvar = *insptr++;
            xvar = *insptr++;
            yvar = *insptr++;

            switch(tw)
            {
            case CON_DIST:
                distx = dist(&sprite[GetGameVarID(xvar, g_i, g_p)],&sprite[GetGameVarID(yvar, g_i, g_p)]);
                break;
            case CON_LDIST:
                distx = ldist(&sprite[GetGameVarID(xvar, g_i, g_p)],&sprite[GetGameVarID(yvar, g_i, g_p)]);
                break;
            case CON_TXDIST:
                distx = txdist(&sprite[GetGameVarID(xvar, g_i, g_p)],&sprite[GetGameVarID(yvar, g_i, g_p)]);
                break;
            }

            SetGameVarID(distvar, distx, g_i, g_p);
            break;
        }

    case CON_GETANGLE:
        {
            long angvar, xvar, yvar;
            short ang;

            insptr++;
            angvar = *insptr++;
            xvar = *insptr++;
            yvar = *insptr++;
            ang = getangle(GetGameVarID(xvar, g_i, g_p),GetGameVarID(yvar, g_i, g_p));

            SetGameVarID(angvar, ang, g_i, g_p);
            break;
        }

    case CON_MULSCALE:
        {
            long var1, var2, var3, var4;
            long what;

            insptr++;
            var1 = *insptr++;
            var2 = GetGameVarID(*insptr++, g_i, g_p);
            var3 = GetGameVarID(*insptr++, g_i, g_p);
            var4 = GetGameVarID(*insptr++, g_i, g_p);

            what = mulscale(var2, var3, var4);

            SetGameVarID(var1, what, g_i, g_p );
            break;
        }

    case CON_INITTIMER:
        {
            short i;
            insptr++;
            i = GetGameVarID(*insptr++, g_i, g_p);
            if (timer != i)
            {
                uninittimer();
                inittimer(i);
                timer = i;
            }
            break;
        }

    case CON_TIME:
        {
            insptr += 2;
            break;
        }

    case CON_ESPAWNVAR:
    case CON_EQSPAWNVAR:
    case CON_QSPAWNVAR:
        {
            long lIn, lReturn=-1;

            insptr++;

            lIn=*insptr++;
            lIn=GetGameVarID(lIn, g_i, g_p);
            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                lReturn = spawn(g_i, lIn);
            switch(tw) {
            case CON_EQSPAWNVAR:
                insertspriteq(lReturn);
            case CON_ESPAWNVAR:
                SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
                break;
            case CON_QSPAWNVAR:
                insertspriteq(lReturn);
                break;
            }
            break;
        }

    case CON_ESPAWN:
    case CON_EQSPAWN:
    case CON_QSPAWN:
        {
            long lReturn=-1;

            insptr++;
            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                lReturn = spawn(g_i,*insptr++);
            switch(tw) {
            case CON_EQSPAWN:
                insertspriteq(lReturn);
            case CON_ESPAWN:
                SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
                break;
            case CON_QSPAWN:
                insertspriteq(lReturn);
                break;
            }
            break;
        }

    case CON_ESHOOT:
        {
            long lReturn=-1;

            insptr++;
            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                lReturn = shoot(g_i,*insptr);
            insptr++;
            SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
            break;
        }

    case CON_ZSHOOT:
        {
            insptr++;
            hittype[g_i].temp_data[9] = GetGameVarID(*insptr++, g_i, g_p);
            if (hittype[g_i].temp_data[9] == 0)
                hittype[g_i].temp_data[9] = 1;
            shoot(g_i,*insptr++);
            hittype[g_i].temp_data[9]=0;
            break;
        }

    case CON_SHOOTVAR:
    case CON_ESHOOTVAR:
        {
            long lIn, lReturn=-1;

            insptr++;

            lIn=GetGameVarID(*insptr++, g_i, g_p);
            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                lReturn = shoot(g_i, lIn);
            if (tw == CON_ESHOOTVAR)
                SetGameVarID(g_iReturnVarID, lReturn, g_i, g_p);
            break;
        }

    case CON_SOUNDVAR:
    case CON_STOPSOUNDVAR:
    case CON_SOUNDONCEVAR:
    case CON_GLOBALSOUNDVAR:
        {
            int sound;

            insptr++;
            sound=GetGameVarID(*insptr++, g_i, g_p);

            switch(tw)
            {
            case CON_SOUNDONCEVAR:
                if(!isspritemakingsound(g_i,sound))
                    spritesound((short)sound,g_i);
                break;
            case CON_GLOBALSOUNDVAR:
                spritesound((short)sound,ps[screenpeek].i);
                break;
            case CON_STOPSOUNDVAR:
                if(isspritemakingsound(g_i,sound))
                    stopspritesound((short)sound,g_i);
                break;
            case CON_SOUNDVAR:
                spritesound((short)sound,g_i);
                break;
            }
            break;
        }

    case CON_GUNIQHUDID:
        {
            int i;
            insptr++;
            i=GetGameVarID(*insptr++, g_i, g_p);
            if (i < MAXUNIQHUDID-1)
                guniqhudid = i;
            break;
        }

    case CON_SAVEGAMEVAR:
    case CON_READGAMEVAR:
        {
            int32 i=0;
            insptr++;
            if (scripthandle < 0) break;
            switch(tw)
            {
            case CON_SAVEGAMEVAR:
                i=GetGameVarID(*insptr, g_i, g_p);
                SCRIPT_PutNumber( scripthandle, "Gamevars",aGameVars[*insptr++].szLabel,i,false,false);
                break;
            case CON_READGAMEVAR:
                SCRIPT_GetNumber( scripthandle, "Gamevars",aGameVars[*insptr].szLabel,&i);
                SetGameVarID(*insptr++, i, g_i, g_p);
                break;
            }
            break;
        }

    case CON_ROTATESPRITE:
        {
            long x,y,z;
            short tilenum;
            short a;
            signed char shade;
            char orientation;
            char pal;
            long x1, y1, x2, y2;
            insptr++;
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            z=GetGameVarID(*insptr++,g_i,g_p);
            a=GetGameVarID(*insptr++,g_i,g_p);
            tilenum=GetGameVarID(*insptr++,g_i,g_p);
            shade=GetGameVarID(*insptr++,g_i,g_p);
            pal=GetGameVarID(*insptr++,g_i,g_p);
            orientation=GetGameVarID(*insptr++,g_i,g_p);
            x1=GetGameVarID(*insptr++,g_i,g_p);
            y1=GetGameVarID(*insptr++,g_i,g_p);
            x2=GetGameVarID(*insptr++,g_i,g_p);
            y2=GetGameVarID(*insptr++,g_i,g_p);

            rotatesprite(x<<16,y<<16,z,a,tilenum,shade,pal,2|orientation,x1,y1,x2,y2);
            break;
        }

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_DIGITALNUMBER:
        {
            long x,y;
            short tilenum=0;
            signed char shade;
            char orientation=0;
            char pal;
            long x1=0, y1=0, x2=0, y2=0;
            long q;
            insptr++;

            if (tw == CON_GAMETEXT || tw == CON_DIGITALNUMBER)
                tilenum=GetGameVarID(*insptr++,g_i,g_p);
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            q=GetGameVarID(*insptr++,g_i,g_p);
            shade=GetGameVarID(*insptr++,g_i,g_p);
            pal=GetGameVarID(*insptr++,g_i,g_p);
            if (tw == CON_GAMETEXT || tw == CON_DIGITALNUMBER)
            {
                orientation=GetGameVarID(*insptr++,g_i,g_p);
                x1=GetGameVarID(*insptr++,g_i,g_p);
                y1=GetGameVarID(*insptr++,g_i,g_p);
                x2=GetGameVarID(*insptr++,g_i,g_p);
                y2=GetGameVarID(*insptr++,g_i,g_p);
            }

            if (tw == CON_MINITEXT) minitextshade(x,y,fta_quotes[q],shade,pal,26);
            else if (tw == CON_GAMETEXT) txgametext(tilenum,x>>1,y,fta_quotes[q],shade,pal,orientation,x1,y1,x2,y2);
            else if (tw == CON_DIGITALNUMBER) txdigitalnumber(tilenum,x,y,q,shade,pal,orientation,x1,y1,x2,y2);
            break;
        }

    case CON_ANGOFF:
        insptr++;
        spriteext[g_i].angoff=*insptr++;
        break;

    case CON_GETZRANGE:
        {
            long x, y, z, ceilz, ceilhit, florz, florhit, walldist, clipmask;
            long ceilzvar, ceilhitvar, florzvar, florhitvar;
            short sectnum;

            insptr++;
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            z=GetGameVarID(*insptr++,g_i,g_p);
            sectnum=GetGameVarID(*insptr++,g_i,g_p);
            ceilzvar=*insptr++;
            ceilhitvar=*insptr++;
            florzvar=*insptr++;
            florhitvar=*insptr++;
            walldist=GetGameVarID(*insptr++,g_i,g_p);
            clipmask=GetGameVarID(*insptr++,g_i,g_p);

            getzrange(x, y, z, sectnum, &ceilz, &ceilhit, &florz, &florhit, walldist, clipmask);
            SetGameVarID(ceilzvar, ceilz, g_i, g_p);
            SetGameVarID(ceilhitvar, ceilhit, g_i, g_p);
            SetGameVarID(florzvar, florz, g_i, g_p);
            SetGameVarID(florhitvar, florhit, g_i, g_p);
            break;
        }

    case CON_HITSCAN:
        {
            long xs, ys, zs, vx, vy, vz, hitx, hity, hitz;
            short sectnum, hitsect, hitwall, hitsprite;
            unsigned long cliptype;

            long hitxvar, hityvar, hitzvar;
            short hitsectvar, hitwallvar, hitspritevar;

            insptr++;
            xs=GetGameVarID(*insptr++,g_i,g_p);
            ys=GetGameVarID(*insptr++,g_i,g_p);
            zs=GetGameVarID(*insptr++,g_i,g_p);
            sectnum=GetGameVarID(*insptr++,g_i,g_p);
            vx=GetGameVarID(*insptr++,g_i,g_p);
            vy=GetGameVarID(*insptr++,g_i,g_p);
            vz=GetGameVarID(*insptr++,g_i,g_p);
            hitsectvar=*insptr++;
            hitwallvar=*insptr++;
            hitspritevar=*insptr++;
            hitxvar=*insptr++;
            hityvar=*insptr++;
            hitzvar=*insptr++;
            cliptype=GetGameVarID(*insptr++,g_i,g_p);
            hitscan(xs, ys, zs, sectnum, vx, vy, vz, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, cliptype);
            SetGameVarID(hitsectvar, hitsect, g_i, g_p);
            SetGameVarID(hitwallvar, hitwall, g_i, g_p);
            SetGameVarID(hitspritevar, hitsprite, g_i, g_p);
            SetGameVarID(hitxvar, hitx, g_i, g_p);
            SetGameVarID(hityvar, hity, g_i, g_p);
            SetGameVarID(hitzvar, hitz, g_i, g_p);
            break;
        }

    case CON_ROTATEPOINT:
        {
            long xpivot, ypivot, x, y, x2, y2, x2var, y2var;
            short daang;

            insptr++;
            xpivot=GetGameVarID(*insptr++,g_i,g_p);
            ypivot=GetGameVarID(*insptr++,g_i,g_p);
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            daang=GetGameVarID(*insptr++,g_i,g_p);
            x2var=*insptr++;
            y2var=*insptr++;
            rotatepoint(xpivot,ypivot,x,y,daang,&x2,&y2);
            SetGameVarID(x2var, x2, g_i, g_p);
            SetGameVarID(y2var, y2, g_i, g_p);
            break;
        }

    case CON_NEARTAG:
        {
            //             neartag(long x, long y, long z, short sectnum, short ang,  //Starting position & angle
            //                     short *neartagsector,   //Returns near sector if sector[].tag != 0
            //                     short *neartagwall,     //Returns near wall if wall[].tag != 0
            //                     short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
            //                     long *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
            //                     long neartagrange,      //Choose maximum distance to scan (scale: 1024=largest grid size)
            //                     char tagsearch)         //1-lotag only, 2-hitag only, 3-lotag&hitag

            long x, y, z, neartaghitdist, neartagrange;
            short sectnum, ang, neartagsector, neartagwall, neartagsprite;
            long neartagsectorvar, neartagwallvar, neartagspritevar, neartaghitdistvar;
            char tagsearch;

            insptr++;
            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            z=GetGameVarID(*insptr++,g_i,g_p);
            sectnum=GetGameVarID(*insptr++,g_i,g_p);
            ang=GetGameVarID(*insptr++,g_i,g_p);
            neartagsectorvar=*insptr++;
            neartagwallvar=*insptr++;
            neartagspritevar=*insptr++;
            neartaghitdistvar=*insptr++;
            neartagrange=GetGameVarID(*insptr++,g_i,g_p);
            tagsearch=GetGameVarID(*insptr++,g_i,g_p);
            neartag(x, y, z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, neartagrange, tagsearch);
            SetGameVarID(neartagsectorvar, neartagsector, g_i, g_p);
            SetGameVarID(neartagwallvar, neartagwall, g_i, g_p);
            SetGameVarID(neartagspritevar, neartagsprite, g_i, g_p);
            SetGameVarID(neartaghitdistvar, neartaghitdist, g_i, g_p);
            break;
        }

    case CON_MOVESPRITE:
    case CON_SETSPRITE:
        {
            short spritenum;
            long x, y, z;
            unsigned long cliptype, returnvar;
            insptr++;
            spritenum = GetGameVarID(*insptr++,g_i,g_p);
            x = GetGameVarID(*insptr++,g_i,g_p);
            y = GetGameVarID(*insptr++,g_i,g_p);
            z = GetGameVarID(*insptr++,g_i,g_p);
            switch(tw)
            {
            case CON_MOVESPRITE:
                cliptype = GetGameVarID(*insptr++,g_i,g_p);
                returnvar = *insptr++;
                SetGameVarID(returnvar, movesprite(spritenum, x, y, z, cliptype), g_i, g_p);
                break;
            case CON_SETSPRITE:
                setsprite(spritenum, x, y, z);
                break;
            }
            break;
        }

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        {
            short sectnum;
            long x, y;
            unsigned long returnvar;
            insptr++;
            sectnum = GetGameVarID(*insptr++,g_i,g_p);
            x = GetGameVarID(*insptr++,g_i,g_p);
            y = GetGameVarID(*insptr++,g_i,g_p);
            returnvar = *insptr++;
            switch(tw)
            {
            case CON_GETFLORZOFSLOPE:
                SetGameVarID(returnvar, getflorzofslope(sectnum,x,y), g_i, g_p);
                break;
            case CON_GETCEILZOFSLOPE:
                SetGameVarID(returnvar, getceilzofslope(sectnum,x,y), g_i, g_p);
                break;
            }
            break;
        }

    case CON_UPDATESECTOR:
    case CON_UPDATESECTORZ:
        {
            long x,y,z=0;
            int var;
            short w;

            w=sprite[g_i].sectnum;

            insptr++;

            x=GetGameVarID(*insptr++,g_i,g_p);
            y=GetGameVarID(*insptr++,g_i,g_p);
            if (tw==CON_UPDATESECTORZ) z=GetGameVarID(*insptr++,g_i,g_p);
            var=*insptr++;

            if (tw==CON_UPDATESECTOR) updatesector(x,y,&w);
            else if (tw==CON_UPDATESECTORZ) updatesectorz(x,y,z,&w);

            SetGameVarID(var, w, g_i, g_p);
            break;
        }

    case CON_SPAWN:
        insptr++;
        if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
            spawn(g_i,*insptr);
        insptr++;
        break;

    case CON_IFWASWEAPON:
        insptr++;
        parseifelse( hittype[g_i].picnum == *insptr);
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
        {
            short dnum;

            insptr++;
            dnum = *insptr++;

            if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
                for(j=(*insptr)-1;j>=0;j--)
                {
                    if(g_sp->picnum == BLIMP && dnum == SCRAP1)
                        s = 0;
                    else s = (TRAND%3);

                    l = EGS(g_sp->sectnum,
                            g_sp->x+(TRAND&255)-128,g_sp->y+(TRAND&255)-128,g_sp->z-(8<<8)-(TRAND&8191),
                            dnum+s,g_sp->shade,32+(TRAND&15),32+(TRAND&15),
                            TRAND&2047,(TRAND&127)+32,
                            -(TRAND&2047),g_i,5);
                    if(g_sp->picnum == BLIMP && dnum == SCRAP1)
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
        {
            int i;
            time_t curtime;

            insptr++;
            i = *insptr++;
            if(movesperpacket == 4 && connecthead != myconnectindex)
                break;

            lastsavedpos = i;
            curtime = time(NULL);
            Bstrcpy(tempbuf,asctime(localtime(&curtime)));
            clearbuf(ud.savegame[lastsavedpos],sizeof(ud.savegame[lastsavedpos]),0);
            Bsprintf(ud.savegame[lastsavedpos],"Auto");
            for(i=0;i<13;i++)
                Bmemcpy(&ud.savegame[lastsavedpos][i+4],&tempbuf[i+3],sizeof(tempbuf[i+3]));
            ud.savegame[lastsavedpos][i+4] = '\0';
            OSD_Printf("Saving to slot %d\n",lastsavedpos);

            KB_FlushKeyboardQueue();

            screencapt = 1;
            displayrooms(myconnectindex,65536);
            //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
            screencapt = 0;
            if(ud.multimode > 1)
                saveplayer(-1-(lastsavedpos));
            else saveplayer(lastsavedpos);

            break;
        }

    case CON_IFMOVE:
        insptr++;
        parseifelse(g_t[1] == *insptr);
        break;

    case CON_RESETPLAYER:
        {
            insptr++;

            //AddLog("resetplayer");
            if(ud.multimode < 2)
            {
                if( lastsavedpos >= 0 && ud.recstat != 2 )
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

                ps[g_p].movement_lock[1] = 0;
                ps[g_p].movement_lock[2] = 0;
                ps[g_p].movement_lock[3] = 0;
                ps[g_p].movement_lock[4] = 0;

                OnEvent(EVENT_RESETPLAYER, ps[g_p].i, g_p, -1);
                cameradist = 0;
                cameraclock = totalclock;
            }
            setpal(&ps[g_p]);
            //AddLog("EOF: resetplayer");
        }
        break;

    case CON_IFONWATER:
        parseifelse( klabs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == 1);
        break;

    case CON_IFINWATER:
        parseifelse( sector[g_sp->sectnum].lotag == 2);
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
        switch(*(insptr-1))
        {
        case GET_STEROIDS:
            ps[g_p].steroids_amount = *insptr;
            ps[g_p].inven_icon = 2;
            break;

        case GET_SHIELD:
            ps[g_p].shield_amount +=          *insptr;// 100;
            if(ps[g_p].shield_amount > max_player_health)
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
            switch(g_sp->pal)
            {
            case  0: ps[g_p].got_access |= 1;break;
            case 21: ps[g_p].got_access |= 2;break;
            case 23: ps[g_p].got_access |= 4;break;
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

            s = g_sp->xvel;

            if( (l&8) && ps[g_p].on_ground && (sync[g_p].bits&2) )
                j = 1;
            else if( (l&16) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&
                     ps[g_p].poszv > 2048 )
                j = 1;
            else if( (l&32) && ps[g_p].jumping_counter > 348 )
                j = 1;
            else if( (l&1) && s >= 0 && s < 8)
                j = 1;
            else if( (l&2) && s >= 8 && !(sync[g_p].bits&(1<<5)) )
                j = 1;
            else if( (l&4) && s >= 8 && sync[g_p].bits&(1<<5) )
                j = 1;
            else if( (l&64) && ps[g_p].posz < (g_sp->z-(48<<8)) )
                j = 1;
            else if( (l&128) && s <= -8 && !(sync[g_p].bits&(1<<5)) )
                j = 1;
            else if( (l&256) && s <= -8 && (sync[g_p].bits&(1<<5)) )
                j = 1;
            else if( (l&512) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
                j = 1;
            else if( (l&1024) && sprite[ps[g_p].i].xrepeat < 32 )
                j = 1;
            else if( (l&2048) && ps[g_p].jetpack_on )
                j = 1;
            else if( (l&4096) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
                j = 1;
            else if( (l&8192) && ps[g_p].on_ground)
                j = 1;
            else if( (l&16384) && sprite[ps[g_p].i].xrepeat > 32 && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0 )
                j = 1;
            else if( (l&32768) && sprite[ps[g_p].i].extra <= 0)
                j = 1;
            else if( (l&65536L) )
            {
                if(g_sp->picnum == APLAYER && ud.multimode > 1)
                    j = getincangle(ps[otherp].ang,getangle(ps[g_p].posx-ps[otherp].posx,ps[g_p].posy-ps[otherp].posy));
                else
                    j = getincangle(ps[g_p].ang,getangle(g_sp->x-ps[g_p].posx,g_sp->y-ps[g_p].posy));

                if( j > -128 && j < 128 )
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
        guts(g_sp,*(insptr-1),*insptr,g_p);
        insptr++;
        break;

    case CON_IFSPAWNEDBY:
        insptr++;
        //            if(g_sp->owner >= 0 && sprite[g_sp->owner].picnum == *insptr)
        //            parseifelse(1);
        //            else
        parseifelse( hittype[g_i].picnum == *insptr);
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
        parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) < *insptr);
        break;

    case CON_IFHITSPACE:
        parseifelse( sync[g_p].bits&(1<<29));
        break;

    case CON_IFOUTSIDE:
        parseifelse(sector[g_sp->sectnum].ceilingstat&1);
        break;

    case CON_IFMULTIPLAYER:
        parseifelse(ud.multimode > 1);
        break;

    case CON_OPERATE:
        insptr++;
        if( sector[g_sp->sectnum].lotag == 0 )
        {
            neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
            if( neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag) )
                if( (sector[neartagsector].lotag&0xff) == 23 || sector[neartagsector].floorz == sector[neartagsector].ceilingz )
                    if( (sector[neartagsector].lotag&16384) == 0 )
                        if( (sector[neartagsector].lotag&32768) == 0 )
                        {
                            j = headspritesect[neartagsector];
                            while(j >= 0)
                            {
                                if(sprite[j].picnum == ACTIVATOR)
                                    break;
                                j = nextspritesect[j];
                            }
                            if(j == -1)
                                operatesectors(neartagsector,g_i);
                        }
        }
        break;

    case CON_IFINSPACE:
        parseifelse(ceilingspace(g_sp->sectnum));
        break;

    case CON_SPRITEPAL:
        insptr++;
        if(g_sp->picnum != APLAYER)
            hittype[g_i].tempang = g_sp->pal;
        g_sp->pal = *insptr++;
        break;

    case CON_CACTOR:
        insptr++;
        g_sp->picnum = *insptr++;
        break;

    case CON_IFBULLETNEAR:
        parseifelse( dodge(g_sp) == 1);
        break;

    case CON_IFRESPAWN:
        if( badguy(g_sp) )
            parseifelse( ud.respawn_monsters );
        else if( inventory(g_sp) )
            parseifelse( ud.respawn_inventory );
        else
            parseifelse( ud.respawn_items );
        break;

    case CON_IFFLOORDISTL:
        insptr++;
        //            getglobalz(g_i);
        parseifelse( (hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
        break;

    case CON_IFCEILINGDISTL:
        insptr++;
        //            getglobalz(g_i);
        parseifelse( ( g_sp->z - hittype[g_i].ceilingz ) <= ((*insptr)<<8));
        break;

    case CON_PALFROM:
        insptr++;
        ps[g_p].pals_time = *insptr++;
        for(j=0;j<3;j++)
            ps[g_p].pals[j] = *insptr++;
        break;

    case CON_DYNQUOTE:
        {
            long var1, var2, var3, var4;
            insptr++;
            Bstrcpy(tempbuf,fta_quotes[*insptr++]);
            var1 = GetGameVarID(*insptr++, g_i, g_p);
            var2 = GetGameVarID(*insptr++, g_i, g_p);
            var3 = GetGameVarID(*insptr++, g_i, g_p);
            var4 = GetGameVarID(*insptr++, g_i, g_p);
            Bsprintf(fta_quotes[122],tempbuf,var1,var2,var3,var4);
            FTA(122,&ps[g_p]);
            break;
        }

        /*        case 74:
        insptr++;
        getglobalz(g_i);
        parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) >= *insptr);
        break;
        */

    case CON_ADDLOG:
        {
            long l;
            insptr++;
            l=*insptr++;  // var
            Bsprintf(g_szBuf,"CONLOG: L=%ld",l);
            AddLog(g_szBuf);
            break;
        }

    case CON_ADDLOGVAR:
        {
            long l,m=1,lVarID;
            char szBuf[256];
            insptr++;
            l=*insptr++;  // l=Line number, *instpr=varID

            lVarID = *insptr;
            if( (lVarID >= iGameVarCount) || lVarID < 0)
            {
                if(*insptr==MAXGAMEVARS) // addlogvar for a constant?  Har.
                    insptr++;
                else if(*insptr&(MAXGAMEVARS<<1))
                {
                    m = -1;
                    lVarID ^= (MAXGAMEVARS<<1);
                    goto good;
                }
                // invalid varID
                insptr++;
                Bsprintf(g_szBuf,"CONLOGVAR: L=%ld INVALID VARIABLE",l);
                AddLog(g_szBuf);
                break;  // out of switch
            }
good:
            Bsprintf(szBuf,"CONLOGVAR: L=%ld %s ",l, aGameVars[lVarID].szLabel);
            strcpy(g_szBuf,szBuf);

            if( aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_READONLY)
            {
                Bsprintf(szBuf," (read-only)");
                strcat(g_szBuf,szBuf);
            }
            if( aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            {
                Bsprintf(szBuf," (Per Player. Player=%d)",g_p);
            }
            else if( aGameVars[lVarID].dwFlags & GAMEVAR_FLAG_PERACTOR)
            {
                Bsprintf(szBuf," (Per Actor. Actor=%d)",g_i);
            }
            else
            {
                Bsprintf(szBuf," (Global)");
            }
            strcat(g_szBuf,szBuf);
            Bsprintf(szBuf," =%ld", GetGameVarID(lVarID, g_i, g_p)*m);
            strcat(g_szBuf,szBuf);
            AddLog(g_szBuf);
            insptr++;
            break;
        }

    case CON_SETSECTOR:
    case CON_GETSECTOR:
        {
            // syntax [gs]etsector[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;
            lVar2=*insptr++;
            DoSector(tw==CON_SETSECTOR, lVar1, lLabelID, lVar2, g_i, g_p);
            break;
        }

    case CON_SQRT:
        {
            // syntax sqrt <invar> <outvar>

            long lInVarID;
            long lOutVarID;
            long lIn;

            insptr++;
            lInVarID=*insptr++;
            lOutVarID=*insptr++;
            lIn=GetGameVarID(lInVarID, g_i, g_p);
            SetGameVarID(lOutVarID, ksqrt(lIn), g_i, g_p);
            break;
        }

    case CON_FINDNEARACTOR:
    case CON_FINDNEARSPRITE:
        {
            // syntax findnearactorvar <type> <maxdist> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdist> <varid>
            long lType;
            long lMaxDist;
            long lVarID;
            long lTemp;
            long lFound;
            long lDist;
            short j, k;

            insptr++;

            lType=*insptr++;
            lMaxDist=*insptr++;
            lVarID=*insptr++;

            lFound=-1;
            lDist=32767;    // big number

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTOR?1:k];    // all sprites
                while(j>=0)
                {
                    if(sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if( lTemp < lMaxDist )
                        {
                            if (lTemp < lDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                        }
                    }
                    j = nextspritestat[j];
                }
                if(tw==CON_FINDNEARACTOR || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_FINDNEARACTORVAR:
    case CON_FINDNEARSPRITEVAR:
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            long lType, lMaxDistVar, lMaxDist;
            long lVarID, lTemp, lFound, lDist;
            short j, k;

            insptr++;

            lType=*insptr++;
            lMaxDistVar=*insptr++;
            lVarID=*insptr++;
            lMaxDist=GetGameVarID(lMaxDistVar, g_i, g_p);
            lFound=-1;
            lDist=32767;    // big number

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTORVAR?1:k];    // all sprites
                while(j>=0)
                {
                    if(sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if( lTemp < lMaxDist )
                        {
                            if (lTemp < lDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                        }
                    }
                    j = nextspritestat[j];
                }
                if(tw==CON_FINDNEARACTORVAR || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_FINDPLAYER:
    case CON_FINDOTHERPLAYER:
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            short j=0;
            long var1, d;

            insptr++;
            var1 = *insptr++;

            if (tw == CON_FINDPLAYER) j=findplayer(&sprite[g_i],&d);
            else if (tw == CON_FINDOTHERPLAYER) j=findotherplayer(g_i,&d);

            SetGameVarID(g_iReturnVarID, j, g_i, g_p);
            SetGameVarID(var1, d, g_i, g_p);

            break;
        }

    case CON_FINDNEARACTOR3DVAR:
    case CON_FINDNEARSPRITE3DVAR:
        {
            // syntax findnearactorvar <type> <maxdistvar> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdistvarid> <varid>
            long lType, lMaxDistVar, lMaxZDistVar, lMaxDist, lMaxZDist;
            long lVarID, lTemp, lTemp2, lFound, lDist;
            short j, k;

            insptr++;

            lType=*insptr++;
            lMaxDistVar=*insptr++;
            lMaxZDistVar=*insptr++;
            lVarID=*insptr++;
            lMaxDist=GetGameVarID(lMaxDistVar, g_i, g_p);
            lMaxZDist=GetGameVarID(lMaxZDistVar, g_i, g_p);
            lFound=-1;
            lDist=32767;    // big number

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTOR3DVAR?1:k];    // all sprites
                while(j>=0)
                {
                    if(sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if(lTemp < lMaxDist && lTemp < lDist)
                        {
                            lTemp2=txdist(&sprite[g_i], &sprite[j]);
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
                if(tw==CON_FINDNEARACTOR3DVAR || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);

            break;
        }

    case CON_FINDNEARACTOR3D:
    case CON_FINDNEARSPRITE3D:
        {
            // syntax findnearactorvar <type> <maxdist> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found
            // <type> <maxdist> <varid>
            long lType, lMaxDist, lMaxZDist, lVarID;
            long lTemp, lTemp2, lFound, lDist;
            short j, k;

            insptr++;

            lType=*insptr++;
            lMaxDist=*insptr++;
            lMaxZDist=*insptr++;
            lVarID=*insptr++;

            lFound=-1;
            lDist=32767;    // big number

            for (k=0;k<MAXSTATUS;k++)
            {
                j=headspritestat[tw==CON_FINDNEARACTOR3D?1:k];    // all sprites
                while(j>=0)
                {
                    if(sprite[j].picnum == lType && j != g_i)
                    {
                        lTemp=ldist(&sprite[g_i], &sprite[j]);
                        if(lTemp < lMaxDist && lTemp < lDist)
                        {
                            lTemp2=txdist(&sprite[g_i], &sprite[j]);
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
                if(tw==CON_FINDNEARACTOR3D || j == MAXSPRITES)
                    break;
            }
            SetGameVarID(lVarID, lFound, g_i, g_p);
            break;
        }

    case CON_SETPLAYER:
    case CON_GETPLAYER:
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;
            long lParm2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;
            // HACK: need to have access to labels structure at run-time...

            switch(lLabelID)
            {
            case PLAYER_AMMO_AMOUNT:
            case PLAYER_GOTWEAPON:
            case PLAYER_PALS:
            case PLAYER_MOVEMENT_LOCK:
            case PLAYER_LOOGIEX:
            case PLAYER_LOOGIEY:
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
                break;
            default:
                lParm2=0;
                break;
            }
            lVar2=*insptr++;

            DoPlayer(tw==CON_SETPLAYER, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
            break;
        }

    case CON_SETINPUT:
    case CON_GETINPUT:
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;

            lVar2=*insptr++;

            DoInput(tw==CON_SETINPUT, lVar1, lLabelID, lVar2, g_i, g_p);
            break;
        }

    case CON_GETUSERDEF:
    case CON_SETUSERDEF:
        {
            // syntax [gs]etuserdef.xxx <VAR>
            //  <xxxid> <varid>
            long lLabelID;
            long lVar2;

            insptr++;
            lLabelID=*insptr++;
            lVar2=*insptr++;

            DoUserDef(tw==CON_SETUSERDEF, lLabelID, lVar2, g_i, g_p);
            break;
        }

    case CON_GETPROJECTILE:
    case CON_SETPROJECTILE:
        {
            // syntax [gs]etplayer[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;

            insptr++;
            lVar1=GetGameVarID(*insptr++, g_i, g_p);
            lLabelID=*insptr++;
            lVar2=*insptr++;
            DoProjectile(tw==CON_SETPROJECTILE,lVar1,lLabelID,lVar2,g_i,g_p);

            break;
        }

    case CON_SETWALL:
    case CON_GETWALL:
        {
            // syntax [gs]etwall[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;
            lVar2=*insptr++;

            DoWall(tw==CON_SETWALL, lVar1, lLabelID, lVar2, g_i, g_p);
            break;
        }

    case CON_SETACTORVAR:
    case CON_GETACTORVAR:
        {
            // syntax [gs]etactorvar[<var>].<varx> <VAR>
            // gets the value of the per-actor variable varx into VAR
            // <var> <varx> <VAR>
            long lVar1, lVar2, lVar3;
            long lTemp,lSprite;

            insptr++;

            lVar1=*insptr++;
            lVar2=*insptr++;
            lVar3=*insptr++;

            lSprite=GetGameVarID(lVar1, g_i, g_p);
            if(lSprite >= 0)
            {
                switch(tw)
                {
                case CON_SETACTORVAR:
                    lTemp=GetGameVarID(lVar3, g_i, g_p);
                    SetGameVarID(lVar2, lTemp, lSprite, g_p);
                    break;
                case CON_GETACTORVAR:
                    lTemp=GetGameVarID(lVar2, lSprite, g_p);
                    SetGameVarID(lVar3, lTemp, g_i, g_p);
                    break;
                }
            }
            break;
        }

    case CON_SETPLAYERVAR:
    case CON_GETPLAYERVAR:
        {
            // syntax [gs]etactorvar[<var>].<varx> <VAR>
            // gets the value of the per-actor variable varx into VAR
            // <var> <varx> <VAR>
            long lVar1, lVar2, lVar3;
            long lTemp,lSprite;

            insptr++;

            lVar1=*insptr++;
            lVar2=*insptr++;
            lVar3=*insptr++;

            lSprite=GetGameVarID(lVar1, g_i, g_p);
            if(lSprite >= 0)
            {
                switch(tw)
                {
                case CON_SETPLAYERVAR:
                    lTemp=GetGameVarID(lVar3, g_i, g_p);
                    SetGameVarID(lVar2, lTemp, g_i, lSprite);
                    break;
                case CON_GETPLAYERVAR:
                    lTemp=GetGameVarID(lVar2, g_i, lSprite);
                    SetGameVarID(lVar3, lTemp, g_i, g_p);
                    break;
                }
            }
            break;
        }

    case CON_SETACTOR:
    case CON_GETACTOR:
        {
            // syntax [gs]etactor[<var>].x <VAR>
            // <varid> <xxxid> <varid>
            long lLabelID;
            long lVar1, lVar2;
            long lParm2;

            insptr++;
            lVar1=*insptr++;
            lLabelID=*insptr++;

            switch(lLabelID)
            {
            case ACTOR_HTG_T:
                lParm2=GetGameVarID(*insptr++, g_i, g_p);
                break;
            default:
                lParm2=0;
                break;
            }
            lVar2=*insptr++;

            DoActor(tw==CON_SETACTOR, lVar1, lLabelID, lVar2, g_i, g_p, lParm2);
            break;
        }

    case CON_GETANGLETOTARGET:
        {
            int i;
            short ang;

            insptr++;
            i=*insptr++;  // ID of def

            // hittype[g_i].lastvx and lastvy are last known location of target.
            ang=getangle(hittype[g_i].lastvx-g_sp->x,hittype[g_i].lastvy-g_sp->y);
            SetGameVarID(i, ang, g_i, g_p );
            break;
        }

    case CON_ANGOFFVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            spriteext[g_i].angoff=GetGameVarID(i, g_i, g_p);
            break;
        }

    case CON_LOCKPLAYER:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            ps[g_p].transporter_hold=GetGameVarID(i, g_i, g_p);
            break;
        }

    case CON_CHECKAVAILWEAPON:
    case CON_CHECKAVAILINVEN:
        {
            int i;

            insptr++;

            if (*insptr == g_iThisActorID)
            {
                i = g_p;
                insptr++;
            }
            else
                i=GetGameVarID(*insptr++, g_i, g_p);
            if (i < MAXPLAYERS)
            {
                if (tw == CON_CHECKAVAILWEAPON)
                    checkavailweapon(&ps[i]);
                else checkavailinven(&ps[i]);
            }
            break;
        }

    case CON_GETPLAYERANGLE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, ps[g_p].ang, g_i, g_p );
            break;
        }

    case CON_SETPLAYERANGLE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            ps[g_p].ang=GetGameVarID(i, g_i, g_p);
            ps[g_p].ang &= 2047;
            break;
        }

    case CON_GETACTORANGLE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, g_sp->ang, g_i, g_p );
            break;
        }

    case CON_SETACTORANGLE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            g_sp->ang=GetGameVarID(i, g_i, g_p);
            g_sp->ang &= 2047;
            break;
        }

    case CON_SETVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, *insptr++, g_i, g_p );
            break;
        }

    case CON_SETVARVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(*insptr++, g_i, g_p), g_i, g_p );
            break;
        }

    case CON_RANDVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, mulscale(krand(), *insptr++, 16), g_i, g_p );
            break;
        }

    case CON_DISPLAYRANDVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, mulscale(rand(), *insptr++, 15), g_i, g_p );
            break;
        }

    case CON_MULVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) * *insptr++, g_i, g_p );
            break;
        }

    case CON_DIVVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            if( (*insptr) == 0 )
                gameexit("CON_DIVVAR: Divide by zero.");
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) / *insptr++, g_i, g_p );
            break;
        }

    case CON_MODVAR:
        {
            int i;
            long l;
            insptr++;
            i=*insptr++;  // ID of def
            l=*insptr++;
            if( l == 0 )
                gameexit("CON_MODVAR: Mod by zero.");
            SetGameVarID(i,GetGameVarID(i, g_i, g_p)%l, g_i, g_p );
            break;
        }

    case CON_ANDVAR:
        {
            int i;
            long l;
            insptr++;
            i=*insptr++;  // ID of def
            l=*insptr++;
            SetGameVarID(i,GetGameVarID(i, g_i, g_p) & l, g_i, g_p );
            break;
        }

    case CON_ORVAR:
        {
            int i;
            long l;
            insptr++;
            i=*insptr++;  // ID of def
            l=*insptr++;
            SetGameVarID(i,GetGameVarID(i, g_i, g_p) | l, g_i, g_p );
            break;
        }

    case CON_XORVAR:
        {
            int i;
            long l;
            insptr++;
            i=*insptr++;  // ID of def
            l=*insptr++;
            SetGameVarID(i,GetGameVarID(i, g_i, g_p) ^ l, g_i, g_p );
            break;
        }

    case CON_RANDVARVAR:
        {
            int i;
            long l1;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(*insptr++, g_i, g_p);
            SetGameVarID(i,mulscale(krand(), l1+1, 16), g_i, g_p );
            break;
        }

    case CON_DISPLAYRANDVARVAR:
        {
            int i;
            long l1;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(*insptr++, g_i, g_p);

            SetGameVarID(i,mulscale(rand(), l1+1, 15), g_i, g_p );
            break;
        }

    case CON_GMAXAMMO:
        {
            int i;
            long l1; // l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            SetGameVarID(*insptr++, max_ammo_amount[l1], g_i, g_p );
            break;
        }

    case CON_SMAXAMMO:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            max_ammo_amount[l1]=l2;
            break;
        }

    case CON_MULVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            SetGameVarID(i, l1*l2, g_i, g_p );
            break;
        }

    case CON_DIVVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            if(l2==0)
            {
                gameexit("CON_DIVVARVAR: Divide by zero.");
            }
            SetGameVarID(i, l1/l2 , g_i, g_p );
            break;
        }

    case CON_MODVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            if(l2==0)
                gameexit("CON_MODVARVAR: Mod by zero.");
            SetGameVarID(i, l1 % l2, g_i, g_p );
            break;
        }

    case CON_ANDVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            SetGameVarID(i, l1 & l2 , g_i, g_p );
            break;
        }

    case CON_XORVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            SetGameVarID(i, l1 ^ l2 , g_i, g_p );
            break;
        }

    case CON_ORVARVAR:
        {
            int i;
            long l1,l2;
            insptr++;
            i=*insptr++;  // ID of def
            l1=GetGameVarID(i, g_i, g_p);
            l2=GetGameVarID(*insptr++, g_i, g_p);
            SetGameVarID(i, l1 | l2 , g_i, g_p );
            break;
        }

    case CON_SUBVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) - *insptr++, g_i, g_p );
            break;
        }

    case CON_SUBVARVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) - GetGameVarID(*insptr++, g_i, g_p), g_i, g_p );
            break;
        }

    case CON_ADDVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) + *insptr++, g_i, g_p );
            break;
        }

    case CON_SHIFTVARL:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) << *insptr++, g_i, g_p );
            break;
        }

    case CON_SHIFTVARR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) >> *insptr++, g_i, g_p );
            break;
        }

    case CON_SIN:
        {
            int i;
            long lValue;
            insptr++;
            i=*insptr++;  // ID of def
            lValue=GetGameVarID(*insptr++, g_i, g_p);
            lValue=sintable[lValue&2047];
            SetGameVarID(i, lValue , g_i, g_p );
            break;
        }

    case CON_COS:
        {
            int i;
            long lValue;
            insptr++;
            i=*insptr++;  // ID of def
            lValue=GetGameVarID(*insptr++, g_i, g_p);
            lValue=sintable[(lValue+512)&2047];
            SetGameVarID(i, lValue , g_i, g_p );
            break;
        }

    case CON_ADDVARVAR:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            SetGameVarID(i, GetGameVarID(i, g_i, g_p) + GetGameVarID(*insptr++, g_i, g_p), g_i, g_p );
            break;
        }

    case CON_SPGETLOTAG:
        {
            insptr++;
            SetGameVarID(g_iLoTagID, g_sp->lotag, g_i, g_p);
            break;
        }

    case CON_SPGETHITAG:
        {
            insptr++;
            SetGameVarID(g_iHiTagID, g_sp->hitag, g_i, g_p);
            break;
        }

    case CON_SECTGETLOTAG:
        {
            insptr++;
            SetGameVarID(g_iLoTagID, sector[g_sp->sectnum].lotag, g_i, g_p);
            break;
        }

    case CON_SECTGETHITAG:
        {
            insptr++;
            SetGameVarID(g_iHiTagID, sector[g_sp->sectnum].hitag, g_i, g_p);
            break;
        }

    case CON_GETTEXTUREFLOOR:
        {
            insptr++;
            SetGameVarID(g_iTextureID, sector[g_sp->sectnum].floorpicnum, g_i, g_p);
            break;
        }

    case CON_STARTTRACK:
        {
            insptr++;
            music_select=*insptr++;
            playmusic(&music_fn[ud.volume_number][music_select][0]);
            break;
        }

    case CON_GETTEXTURECEILING:
        {
            insptr++;
            SetGameVarID(g_iTextureID, sector[g_sp->sectnum].ceilingpicnum, g_i, g_p);
            break;
        }

    case CON_IFVARVARAND:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) & GetGameVarID(*(insptr), g_i, g_p) )
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARVARN:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) != GetGameVarID(*(insptr), g_i, g_p) )
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARVARE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) == GetGameVarID(*(insptr), g_i, g_p) )
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARVARG:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) > GetGameVarID(*(insptr), g_i, g_p) )
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARVARL:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) < GetGameVarID(*(insptr), g_i, g_p) )
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARE:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) == *insptr)
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARN:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) != *insptr)
                j=1;
            parseifelse( j );
            break;
        }

    case CON_WHILEVARN:
        {
            int i;
            long *savedinsptr;
            savedinsptr=insptr;
            j=1;
            while (j)
            {
                insptr=savedinsptr;
                insptr++;
                i=*insptr++;      // ID of def
                j=0;

                if(GetGameVarID(i, g_i, g_p) != *insptr)
                    j=1;
                parseifelse( j );
            }
            break;
        }

    case CON_WHILEVARVARN:
        {
            int i,k;
            long *savedinsptr;
            savedinsptr=insptr;
            j=1;
            while (j)
            {
                insptr=savedinsptr;
                insptr++;
                i=*insptr++;      // ID of def
                k=*(insptr);        // ID of def
                j=0;

                if(GetGameVarID(i, g_i, g_p) != GetGameVarID(k, g_i, g_p))
                    j=1;
                parseifelse( j );
            }
            break;
        }

    case CON_IFVARAND:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) & *insptr)
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARG:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) > *insptr)
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFVARL:
        {
            int i;
            insptr++;
            i=*insptr++;  // ID of def
            j=0;
            if(GetGameVarID(i, g_i, g_p) < *insptr)
                j=1;
            parseifelse( j );
            break;
        }

    case CON_IFPHEALTHL:
        insptr++;
        parseifelse( sprite[ps[g_p].i].extra < *insptr);
        break;

    case CON_IFPINVENTORY:
        {
            insptr++;
            j = 0;
            switch(*insptr++)
            {
            case GET_STEROIDS:if( ps[g_p].steroids_amount != *insptr)
                j = 1;
                break;
            case GET_SHIELD:if(ps[g_p].shield_amount != max_player_health )
                j = 1;
                break;
            case GET_SCUBA:if(ps[g_p].scuba_amount != *insptr) j = 1;break;
            case GET_HOLODUKE:if(ps[g_p].holoduke_amount != *insptr) j = 1;break;
            case GET_JETPACK:if(ps[g_p].jetpack_amount != *insptr) j = 1;break;
            case GET_ACCESS:
                switch(g_sp->pal)
                {
                case  0: if(ps[g_p].got_access&1) j = 1;break;
                case 21: if(ps[g_p].got_access&2) j = 1;break;
                case 23: if(ps[g_p].got_access&4) j = 1;break;
                }
                break;
            case GET_HEATS:if(ps[g_p].heat_amount != *insptr) j = 1;break;
            case GET_FIRSTAID:
                if(ps[g_p].firstaid_amount != *insptr) j = 1;break;
            case GET_BOOTS:
                if(ps[g_p].boot_amount != *insptr) j = 1;break;
            }

            parseifelse(j);
            break;
        }

    case CON_PSTOMP:
        insptr++;
        if( ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= 40 )
            if( cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum) )
            {
                ps[g_p].knee_incs = 1;
                if(ps[g_p].weapon_pos == 0)
                    ps[g_p].weapon_pos = -1;
                ps[g_p].actorsqu = g_i;
            }
        break;

    case CON_IFAWAYFROMWALL:
        {
            short s1;

            s1 = g_sp->sectnum;

            j = 0;

            updatesector(g_sp->x+108,g_sp->y+108,&s1);
            if( s1 == g_sp->sectnum )
            {
                updatesector(g_sp->x-108,g_sp->y-108,&s1);
                if( s1 == g_sp->sectnum )
                {
                    updatesector(g_sp->x+108,g_sp->y-108,&s1);
                    if( s1 == g_sp->sectnum )
                    {
                        updatesector(g_sp->x-108,g_sp->y+108,&s1);
                        if( s1 == g_sp->sectnum )
                            j = 1;
                    }
                }
            }
            parseifelse( j );
        }
        break;

    case CON_QUOTE:
        insptr++;
        FTA(*insptr++,&ps[g_p]);
        break;

    case CON_USERQUOTE:
        insptr++;
        adduserquote(fta_quotes[*insptr]);
        insptr++;
        break;

    case CON_IFINOUTERSPACE:
        parseifelse( floorspace(g_sp->sectnum));
        break;

    case CON_IFNOTMOVING:
        parseifelse( (hittype[g_i].movflag&49152) > 16384 );
        break;

    case CON_RESPAWNHITAG:
        insptr++;
        switch(dynamictostatic[g_sp->picnum])
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
            if(g_sp->yvel) operaterespawns(g_sp->yvel);
            break;
        default:
            if(g_sp->hitag >= 0) operaterespawns(g_sp->hitag);
            break;
        }
        break;

    case CON_IFSPRITEPAL:
        insptr++;
        parseifelse( g_sp->pal == *insptr);
        break;

    case CON_IFANGDIFFL:
        insptr++;
        j = klabs(getincangle(ps[g_p].ang,g_sp->ang));
        parseifelse( j <= *insptr);
        break;

    case CON_IFNOSOUNDS:
        for(j=1;j<NUM_SOUNDS;j++)
            if( SoundOwner[j][0].i == g_i )
                break;

        parseifelse( j == NUM_SOUNDS );
        break;

    case CON_SPRITEFLAGS:
        {
            insptr++;
            actorspriteflags[g_i] = GetGameVarID(*insptr++, g_i, g_p);
            break;
        }

    default:
        Bsprintf(tempbuf,"fatal error: Default processing in parse(): %ld, %ld",*insptr,*(insptr-1));
        AddLog(tempbuf);
        gameexit("An error has occurred in the EDuke32 CON parser.\n\nPlease mail all of your CON files along with the file eduke32.log\nto terminx@gmail.com.\n\nThank you.");
        break;
    }
    return 0;
}

void LoadActor(short sActor,short sPlayer,long lDist)
{
    char done;

    g_i = sActor;    // Sprite ID
    g_p = sPlayer;    // Player ID
    g_x = lDist;    // ??
    g_sp = &sprite[g_i];    // Pointer to sprite structure
    g_t = &hittype[g_i].temp_data[0];   // Sprite's 'extra' data

    if( actorLoadEventScrptr[g_sp->picnum] == 0 ) return;

    insptr = actorLoadEventScrptr[g_sp->picnum];

    killit_flag = 0;

    if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        //      if(badguy(g_sp))
        //          ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }
    do
        done = parse();
    while( done == 0 );

    if(killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if (g_p >= 0 )
        {
            if(ps[g_p].actorsqu == g_i)
                ps[g_p].actorsqu = -1;
        }
        deletesprite(g_i);
    }
}

void execute(short sActor,short sPlayer,long lDist)
{
    char done;

    g_i = sActor;    // Sprite ID
    g_p = sPlayer;   // Player ID
    g_x = lDist;     // ??
    g_sp = &sprite[g_i];    // Pointer to sprite structure
    g_t = &hittype[g_i].temp_data[0];   // Sprite's 'extra' data

    if( actorscrptr[g_sp->picnum] == 0 ) return;

    insptr = 4 + (actorscrptr[g_sp->picnum]);

    killit_flag = 0;

    if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
    {
        if(badguy(g_sp))
            ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }

    if(g_t[4])
    {
        g_sp->lotag += TICSPERFRAME;

        if(g_sp->lotag > *(long *)(g_t[4]+16) )
        {
            g_t[2]++;
            g_sp->lotag = 0;
            g_t[3] +=  *(long *)( g_t[4]+12 );
        }

        if( klabs(g_t[3]) >= klabs( *(long *)(g_t[4]+4) * *(long *)(g_t[4]+12) ) )
            g_t[3] = 0;
    }

    do
        done = parse();
    while( done == 0 );

    if(killit_flag == 1)
    {
        // if player was set to squish, first stop that...
        if(ps[g_p].actorsqu == g_i)
            ps[g_p].actorsqu = -1;
        deletesprite(g_i);
    }
    else
    {
        move();

        if( g_sp->statnum == 1)
        {
            if( badguy(g_sp) )
            {
                if( g_sp->xrepeat > 60 ) return;
                if( ud.respawn_monsters == 1 && g_sp->extra <= 0 ) return;
            }
            else if( ud.respawn_items == 1 && (g_sp->cstat&32768) ) return;

            if(hittype[g_i].timetosleep > 1)
                hittype[g_i].timetosleep--;
            else if(hittype[g_i].timetosleep == 1)
                changespritestat(g_i,2);
        }

        else if(g_sp->statnum == 6)
            switch(dynamictostatic[g_sp->picnum])
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
                if(hittype[g_i].timetosleep > 1)
                    hittype[g_i].timetosleep--;
                else if(hittype[g_i].timetosleep == 1)
                    changespritestat(g_i,2);
                break;
            }
    }
}
