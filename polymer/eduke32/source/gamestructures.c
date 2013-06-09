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
static void __fastcall VM_AccessUserdef(int32_t iSet, int32_t lLabelID, int32_t lVar2)
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
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case USERDEFS_GOD:
        if (iSet)
        {
            ud.god = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.god);
        return;

    case USERDEFS_WARP_ON:
        if (iSet)
        {
            ud.warp_on = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.warp_on);
        return;

    case USERDEFS_CASHMAN:
        if (iSet)
        {
            ud.cashman = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.cashman);
        return;

    case USERDEFS_EOG:
        if (iSet)
        {
            ud.eog = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.eog);
        return;

    case USERDEFS_SHOWALLMAP:
        if (iSet)
        {
            ud.showallmap = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.showallmap);
        return;

    case USERDEFS_SHOW_HELP:
        if (iSet)
        {
            ud.show_help = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.show_help);
        return;

    case USERDEFS_SCROLLMODE:
        if (iSet)
        {
            ud.scrollmode = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.scrollmode);
        return;

    case USERDEFS_CLIPPING:
        if (iSet)
        {
            ud.noclip = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.noclip);
        return;

        //  case USERDEFS_USER_NAME:
        //      if(iSet)
        //      {
        //          ud.user_name[MAXPLAYERS][32] = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.user_name[MAXPLAYERS][32]);
        //      return;

        //  case USERDEFS_RIDECULE:
        //      if(iSet)
        //      {
        //          ud.ridecule = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.ridecule);
        //      return;

        //  case USERDEFS_SAVEGAME:
        //      if(iSet)
        //      {
        //          ud.savegame = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.savegame);
        //      return;

        //  case USERDEFS_PWLOCKOUT:
        //      if(iSet)
        //      {
        //          ud.pwlockout = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.pwlockout);
        //      return;

        //  case USERDEFS_RTSNAME:
        //      if(iSet)
        //      {
        //          ud.rtsname = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.rtsname);
        //      return;

    case USERDEFS_OVERHEAD_ON:
        if (iSet)
        {
            ud.overhead_on = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.overhead_on);
        return;

    case USERDEFS_LAST_OVERHEAD:
        if (iSet)
        {
            ud.last_overhead = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.last_overhead);
        return;

    case USERDEFS_SHOWWEAPONS:
        if (iSet)
        {
            ud.showweapons = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.showweapons);
        return;

    case USERDEFS_PAUSE_ON:
        if (iSet)
        {
            ud.pause_on = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.pause_on);
        return;

    case USERDEFS_FROM_BONUS:
        if (iSet)
        {
            ud.from_bonus = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.from_bonus);
        return;

    case USERDEFS_CAMERASPRITE:
        if (iSet)
        {
            ud.camerasprite = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.camerasprite);
        return;

    case USERDEFS_LAST_CAMSPRITE:
        if (iSet)
        {
            ud.last_camsprite = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.last_camsprite);
        return;

    case USERDEFS_LAST_LEVEL:
        if (iSet)
        {
            ud.last_level = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.last_level);
        return;

    case USERDEFS_SECRETLEVEL:
        if (iSet)
        {
            ud.secretlevel = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.secretlevel);
        return;

    case USERDEFS_CONST_VISIBILITY:
        if (iSet)
        {
            ud.const_visibility = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.const_visibility);
        return;

    case USERDEFS_UW_FRAMERATE:
        if (iSet)
        {
            ud.uw_framerate = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.uw_framerate);
        return;

    case USERDEFS_CAMERA_TIME:
        if (iSet)
        {
            ud.camera_time = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.camera_time);
        return;

    case USERDEFS_FOLFVEL:
        if (iSet)
        {
            ud.folfvel = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.folfvel);
        return;

    case USERDEFS_FOLAVEL:
        if (iSet)
        {
            ud.folavel = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.folavel);
        return;

    case USERDEFS_FOLX:
        if (iSet)
        {
            ud.folx = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.folx);
        return;

    case USERDEFS_FOLY:
        if (iSet)
        {
            ud.foly = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.foly);
        return;

    case USERDEFS_FOLA:
        if (iSet)
        {
            ud.fola = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.fola);
        return;

    case USERDEFS_RECCNT:
        if (iSet)
        {
            ud.reccnt = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.reccnt);
        return;

    case USERDEFS_ENTERED_NAME:
        if (iSet)
        {
            ud.entered_name = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.entered_name);
        return;

    case USERDEFS_SCREEN_TILTING:
        if (iSet)
        {
            ud.screen_tilting = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.screen_tilting);
        return;

    case USERDEFS_SHADOWS:
        if (iSet)
        {
            ud.shadows = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.shadows);
        return;

    case USERDEFS_FTA_ON:
        if (iSet)
        {
            ud.fta_on = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.fta_on);
        return;

    case USERDEFS_EXECUTIONS:
        if (iSet)
        {
            ud.executions = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.executions);
        return;

    case USERDEFS_AUTO_RUN:
        if (iSet)
        {
            ud.auto_run = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.auto_run);
        return;

    case USERDEFS_COORDS:
        if (iSet)
        {
            ud.coords = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.coords);
        return;

    case USERDEFS_TICKRATE:
        if (iSet)
        {
            ud.tickrate = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.tickrate);
        return;

    case USERDEFS_M_COOP:
        if (iSet)
        {
            ud.m_coop = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_coop);
        return;

    case USERDEFS_COOP:
        if (iSet)
        {
            ud.coop = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.coop);
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
        Gv_SetVarX(lVar2, ud.screen_size);
        return;

    case USERDEFS_LOCKOUT:
        if (iSet)
        {
            ud.lockout = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.lockout);
        return;

    case USERDEFS_CROSSHAIR:
        if (iSet)
        {
            ud.crosshair = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.crosshair);
        return;

        //  case USERDEFS_WCHOICE:
        //      if(iSet)
        //      {
        //          ud.wchoice = lValue;
        //          return;
        //      }
        //      Gv_SetVarX(lVar2, ud.wchoice);
        //      return;

    case USERDEFS_PLAYERAI:
        if (iSet)
        {
            ud.playerai = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.playerai);
        return;

    case USERDEFS_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.respawn_monsters = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.respawn_monsters);
        return;

    case USERDEFS_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.respawn_items = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.respawn_items);
        return;

    case USERDEFS_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.respawn_inventory = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.respawn_inventory);
        return;

    case USERDEFS_RECSTAT:
        if (iSet)
        {
            ud.recstat = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.recstat);
        return;

    case USERDEFS_MONSTERS_OFF:
        if (iSet)
        {
            ud.monsters_off = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.monsters_off);
        return;

    case USERDEFS_BRIGHTNESS:
        if (iSet)
        {
            ud.brightness = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.brightness);
        return;

    case USERDEFS_M_RESPAWN_ITEMS:
        if (iSet)
        {
            ud.m_respawn_items = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_respawn_items);
        return;

    case USERDEFS_M_RESPAWN_MONSTERS:
        if (iSet)
        {
            ud.m_respawn_monsters = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_respawn_monsters);
        return;

    case USERDEFS_M_RESPAWN_INVENTORY:
        if (iSet)
        {
            ud.m_respawn_inventory = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_respawn_inventory);
        return;

    case USERDEFS_M_RECSTAT:
        if (iSet)
        {
            ud.m_recstat = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_recstat);
        return;

    case USERDEFS_M_MONSTERS_OFF:
        if (iSet)
        {
            ud.m_monsters_off = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_monsters_off);
        return;

    case USERDEFS_DETAIL:
        if (iSet)
        {
            // REMINDER: must implement "boolean" setters like this in Lunatic, too.
            ud.detail = !!lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.detail);
        return;

    case USERDEFS_M_FFIRE:
        if (iSet)
        {
            ud.m_ffire = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_ffire);
        return;

    case USERDEFS_FFIRE:
        if (iSet)
        {
            ud.ffire = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.ffire);
        return;

    case USERDEFS_M_PLAYER_SKILL:
        if (iSet)
        {
            ud.m_player_skill = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_player_skill);
        return;

    case USERDEFS_M_LEVEL_NUMBER:
        if (iSet)
        {
            ud.m_level_number = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_level_number);
        return;

    case USERDEFS_M_VOLUME_NUMBER:
        if (iSet)
        {
            ud.m_volume_number = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_volume_number);
        return;

    case USERDEFS_MULTIMODE:
        if (iSet)
        {
            ud.multimode = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.multimode);
        return;

    case USERDEFS_PLAYER_SKILL:
        if (iSet)
        {
            ud.player_skill = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.player_skill);
        return;

    case USERDEFS_LEVEL_NUMBER:
        if (iSet)
        {
            ud.level_number = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.level_number);
        return;

    case USERDEFS_VOLUME_NUMBER:
        if (iSet)
        {
            ud.volume_number = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.volume_number);
        return;

    case USERDEFS_M_MARKER:
        if (iSet)
        {
            ud.m_marker = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_marker);
        return;

    case USERDEFS_MARKER:
        if (iSet)
        {
            ud.marker = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.marker);
        return;

    case USERDEFS_MOUSEFLIP:
        if (iSet)
        {
            ud.mouseflip = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.mouseflip);
        return;

    case USERDEFS_STATUSBARSCALE:
        if (iSet)
        {
            ud.statusbarscale = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.statusbarscale);
        return;

    case USERDEFS_DRAWWEAPON:
        if (iSet)
        {
            ud.drawweapon = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.drawweapon);
        return;

    case USERDEFS_MOUSEAIMING:
        if (iSet)
        {
            ud.mouseaiming = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.mouseaiming);
        return;

    case USERDEFS_WEAPONSWITCH:
        if (iSet)
        {
            ud.weaponswitch = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.weaponswitch);
        return;

    case USERDEFS_DEMOCAMS:
        if (iSet)
        {
            ud.democams = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.democams);
        return;

    case USERDEFS_COLOR:
        if (iSet)
        {
            ud.color = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.color);
        return;

    case USERDEFS_MSGDISPTIME:
        if (iSet)
        {
            ud.msgdisptime = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.msgdisptime);
        return;

    case USERDEFS_STATUSBARMODE:
        if (iSet)
        {
            ud.statusbarmode = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.statusbarmode);
        return;

    case USERDEFS_M_NOEXITS:
        if (iSet)
        {
            ud.m_noexits = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.m_noexits);
        return;

    case USERDEFS_NOEXITS:
        if (iSet)
        {
            ud.noexits = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.noexits);
        return;

    case USERDEFS_AUTOVOTE:
        if (iSet)
        {
            ud.autovote = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.autovote);
        return;

    case USERDEFS_AUTOMSG:
        if (iSet)
        {
            ud.automsg = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.automsg);
        return;

    case USERDEFS_IDPLAYERS:
        if (iSet)
        {
            ud.idplayers = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.idplayers);
        return;

    case USERDEFS_TEAM:
        if (iSet)
        {
            ud.team = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.team);
        return;

    case USERDEFS_VIEWBOB:
        if (iSet)
        {
            ud.viewbob = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.viewbob);
        return;

    case USERDEFS_WEAPONSWAY:
        if (iSet)
        {
            ud.weaponsway = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.weaponsway);
        return;

    case USERDEFS_ANGLEINTERPOLATION:
        if (iSet)
        {
            ud.angleinterpolation = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.angleinterpolation);
        return;

    case USERDEFS_OBITUARIES:
        if (iSet)
        {
            ud.obituaries = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.obituaries);
        return;

    case USERDEFS_LEVELSTATS:
        if (iSet)
        {
            ud.levelstats = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.levelstats);
        return;

    case USERDEFS_CROSSHAIRSCALE:
        if (iSet)
        {
            ud.crosshairscale = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.crosshairscale);
        return;

    case USERDEFS_ALTHUD:
        if (iSet)
        {
            ud.althud = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.althud);
        return;

    case USERDEFS_DISPLAY_BONUS_SCREEN:
        if (iSet)
        {
            ud.display_bonus_screen = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.display_bonus_screen);
        return;

    case USERDEFS_SHOW_LEVEL_TEXT:
        if (iSet)
        {
            ud.show_level_text = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.show_level_text);
        return;

    case USERDEFS_WEAPONSCALE:
        if (iSet)
        {
            ud.weaponscale = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.weaponscale);
        return;

    case USERDEFS_TEXTSCALE:
        if (iSet)
        {
            ud.textscale = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.textscale);
        return;

    case USERDEFS_RUNKEY_MODE:
        if (iSet)
        {
            ud.runkey_mode = lValue;
            return;
        }
        Gv_SetVarX(lVar2, ud.runkey_mode);
        return;

    default:
        return;
    }
}

static void __fastcall VM_AccessActiveProjectile(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0,proj=vm.g_i;

    // http://forums.duke4.net/topic/775-eduke32-20-and-polymer/page__view__findpost__p__143260

    if (lVar1 != g_iThisActorID)
        proj=Gv_GetVarX(lVar1);

    if ((unsigned)proj >= MAXSPRITES)
    {
        //        OSD_Printf("VM_AccessActiveProjectile(): invalid projectile (%d)\n",proj);
        CON_ERRPRINTF("tried to %s %s on invalid target projectile (%d) %d %d from %s\n",
                      iSet?"set":"get",ProjectileLabels[lLabelID].name,proj,vm.g_i,TrackerCast(vm.g_sp->picnum),
                      (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
        insptr += (lVar2 == MAXGAMEVARS);
        return;
    }

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            SpriteProjectile[proj].workslike=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].workslike);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            SpriteProjectile[proj].spawns=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].spawns);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].sxrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].sxrepeat);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].syrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].syrepeat);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            SpriteProjectile[proj].sound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].sound);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            SpriteProjectile[proj].isound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].isound);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            SpriteProjectile[proj].vel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].vel);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            SpriteProjectile[proj].extra=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].extra);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            SpriteProjectile[proj].decal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].decal);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            SpriteProjectile[proj].trail=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].trail);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].txrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].txrepeat);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].tyrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].tyrepeat);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            SpriteProjectile[proj].toffset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].toffset);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            SpriteProjectile[proj].tnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].tnum);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            SpriteProjectile[proj].drop=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].drop);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            SpriteProjectile[proj].cstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].cstat);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            SpriteProjectile[proj].clipdist=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].clipdist);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            SpriteProjectile[proj].shade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].shade);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].xrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].xrepeat);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            SpriteProjectile[proj].yrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].yrepeat);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            SpriteProjectile[proj].pal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].pal);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            SpriteProjectile[proj].extra_rand=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].extra_rand);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            SpriteProjectile[proj].hitradius=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].hitradius);
        return;

    case PROJ_MOVECNT:
        if (iSet)
        {
            SpriteProjectile[proj].movecnt=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].movecnt);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            SpriteProjectile[proj].offset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].offset);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            SpriteProjectile[proj].bounces=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].bounces);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            SpriteProjectile[proj].bsound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].bsound);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            SpriteProjectile[proj].range=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].range);
        return;

    case PROJ_FLASH_COLOR:
        if (iSet)
        {
            SpriteProjectile[proj].flashcolor=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].flashcolor);
        return;

    case PROJ_USERDATA:
        if (iSet)
        {
            SpriteProjectile[proj].userdata=lValue;
            return;
        }
        Gv_SetVarX(lVar2, SpriteProjectile[proj].userdata);
        return;

    default:
        return;
    }
}

static void __fastcall VM_GetPlayer(register int32_t lVar1, register int32_t lLabelID, register int32_t lVar2, int32_t lParm2)
{
    register int32_t iPlayer=vm.g_p;
    DukePlayer_t *ps;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVarX(lVar1);

    if ((unsigned)iPlayer >= (unsigned)playerswhenstarted)
        goto badplayer;

    if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && ((unsigned)lParm2 >= (unsigned)PlayerLabels[lLabelID].maxParm2))
        goto badpos;

    ps = g_player[iPlayer].ps;

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        Gv_SetVarX(lVar2, ps->zoom); return;
    case PLAYER_EXITX:
        Gv_SetVarX(lVar2, ps->exitx); return;
    case PLAYER_EXITY:
        Gv_SetVarX(lVar2, ps->exity); return;
    case PLAYER_LOOGIEX:
        Gv_SetVarX(lVar2, ps->loogiex[lParm2]); return;
    case PLAYER_LOOGIEY:
        Gv_SetVarX(lVar2, ps->loogiey[lParm2]); return;
    case PLAYER_NUMLOOGS:
        Gv_SetVarX(lVar2, ps->numloogs); return;
    case PLAYER_LOOGCNT:
        Gv_SetVarX(lVar2, ps->loogcnt); return;
    case PLAYER_POSX:
        Gv_SetVarX(lVar2, ps->pos.x); return;
    case PLAYER_POSY:
        Gv_SetVarX(lVar2, ps->pos.y); return;
    case PLAYER_POSZ:
        Gv_SetVarX(lVar2, ps->pos.z); return;
    case PLAYER_HORIZ:
        Gv_SetVarX(lVar2, ps->horiz); return;
    case PLAYER_OHORIZ:
        Gv_SetVarX(lVar2, ps->ohoriz); return;
    case PLAYER_OHORIZOFF:
        Gv_SetVarX(lVar2, ps->ohorizoff); return;
    case PLAYER_INVDISPTIME:
        Gv_SetVarX(lVar2, ps->invdisptime); return;
    case PLAYER_BOBPOSX:
        Gv_SetVarX(lVar2, ps->bobposx); return;
    case PLAYER_BOBPOSY:
        Gv_SetVarX(lVar2, ps->bobposy); return;
    case PLAYER_OPOSX:
        Gv_SetVarX(lVar2, ps->opos.x); return;
    case PLAYER_OPOSY:
        Gv_SetVarX(lVar2, ps->opos.y); return;
    case PLAYER_OPOSZ:
        Gv_SetVarX(lVar2, ps->opos.z); return;
    case PLAYER_PYOFF:
        Gv_SetVarX(lVar2, ps->pyoff); return;
    case PLAYER_OPYOFF:
        Gv_SetVarX(lVar2, ps->opyoff); return;
    case PLAYER_POSXV:
        Gv_SetVarX(lVar2, ps->vel.x); return;
    case PLAYER_POSYV:
        Gv_SetVarX(lVar2, ps->vel.y); return;
    case PLAYER_POSZV:
        Gv_SetVarX(lVar2, ps->vel.z); return;
    case PLAYER_LAST_PISSED_TIME:
        Gv_SetVarX(lVar2, ps->last_pissed_time); return;
    case PLAYER_TRUEFZ:
        Gv_SetVarX(lVar2, ps->truefz); return;
    case PLAYER_TRUECZ:
        Gv_SetVarX(lVar2, ps->truecz); return;
    case PLAYER_PLAYER_PAR:
        Gv_SetVarX(lVar2, ps->player_par); return;
    case PLAYER_VISIBILITY:
        Gv_SetVarX(lVar2, ps->visibility); return;
    case PLAYER_BOBCOUNTER:
        Gv_SetVarX(lVar2, ps->bobcounter); return;
    case PLAYER_WEAPON_SWAY:
        Gv_SetVarX(lVar2, ps->weapon_sway); return;
    case PLAYER_PALS_TIME:
        Gv_SetVarX(lVar2, ps->pals.f); return;
    case PLAYER_RANDOMFLAMEX:
        Gv_SetVarX(lVar2, ps->randomflamex); return;
    case PLAYER_CRACK_TIME:
        Gv_SetVarX(lVar2, ps->crack_time); return;
    case PLAYER_AIM_MODE:
        Gv_SetVarX(lVar2, ps->aim_mode); return;
    case PLAYER_ANG:
        Gv_SetVarX(lVar2, ps->ang); return;
    case PLAYER_OANG:
        Gv_SetVarX(lVar2, ps->oang); return;
    case PLAYER_ANGVEL:
        Gv_SetVarX(lVar2, ps->angvel); return;
    case PLAYER_CURSECTNUM:
        Gv_SetVarX(lVar2, ps->cursectnum); return;
    case PLAYER_LOOK_ANG:
        Gv_SetVarX(lVar2, ps->look_ang); return;
    case PLAYER_LAST_EXTRA:
        Gv_SetVarX(lVar2, ps->last_extra); return;
    case PLAYER_SUBWEAPON:
        Gv_SetVarX(lVar2, ps->subweapon); return;
    case PLAYER_AMMO_AMOUNT:
        Gv_SetVarX(lVar2, ps->ammo_amount[lParm2]); return;
    case PLAYER_WACKEDBYACTOR:
        Gv_SetVarX(lVar2, ps->wackedbyactor); return;
    case PLAYER_FRAG:
        Gv_SetVarX(lVar2, ps->frag); return;
    case PLAYER_FRAGGEDSELF:
        Gv_SetVarX(lVar2, ps->fraggedself); return;
    case PLAYER_CURR_WEAPON:
        Gv_SetVarX(lVar2, ps->curr_weapon); return;
    case PLAYER_LAST_WEAPON:
        Gv_SetVarX(lVar2, ps->last_weapon); return;
    case PLAYER_TIPINCS:
        Gv_SetVarX(lVar2, ps->tipincs); return;
    case PLAYER_HORIZOFF:
        Gv_SetVarX(lVar2, ps->horizoff); return;
    case PLAYER_WANTWEAPONFIRE:
        Gv_SetVarX(lVar2, ps->wantweaponfire); return;
    case PLAYER_HOLODUKE_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_HOLODUKE]); return;
    case PLAYER_NEWOWNER:
        Gv_SetVarX(lVar2, ps->newowner); return;
    case PLAYER_HURT_DELAY:
        Gv_SetVarX(lVar2, ps->hurt_delay); return;
    case PLAYER_HBOMB_HOLD_DELAY:
        Gv_SetVarX(lVar2, ps->hbomb_hold_delay); return;
    case PLAYER_JUMPING_COUNTER:
        Gv_SetVarX(lVar2, ps->jumping_counter); return;
    case PLAYER_AIRLEFT:
        Gv_SetVarX(lVar2, ps->airleft); return;
    case PLAYER_KNEE_INCS:
        Gv_SetVarX(lVar2, ps->knee_incs); return;
    case PLAYER_ACCESS_INCS:
        Gv_SetVarX(lVar2, ps->access_incs); return;
    case PLAYER_FTA:
        Gv_SetVarX(lVar2, ps->fta); return;
    case PLAYER_FTQ:
        Gv_SetVarX(lVar2, ps->ftq); return;
    case PLAYER_ACCESS_WALLNUM:
        Gv_SetVarX(lVar2, ps->access_wallnum); return;
    case PLAYER_ACCESS_SPRITENUM:
        Gv_SetVarX(lVar2, ps->access_spritenum); return;
    case PLAYER_KICKBACK_PIC:
        Gv_SetVarX(lVar2, ps->kickback_pic); return;
    case PLAYER_GOT_ACCESS:
        Gv_SetVarX(lVar2, ps->got_access); return;
    case PLAYER_WEAPON_ANG:
        Gv_SetVarX(lVar2, ps->weapon_ang); return;
    case PLAYER_FIRSTAID_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_FIRSTAID]); return;
    case PLAYER_SOMETHINGONPLAYER:
        Gv_SetVarX(lVar2, ps->somethingonplayer); return;
    case PLAYER_ON_CRANE:
        Gv_SetVarX(lVar2, ps->on_crane); return;
    case PLAYER_I:
        Gv_SetVarX(lVar2, ps->i); return;
    case PLAYER_ONE_PARALLAX_SECTNUM:
        Gv_SetVarX(lVar2, ps->one_parallax_sectnum); return;
    case PLAYER_OVER_SHOULDER_ON:
        Gv_SetVarX(lVar2, ps->over_shoulder_on); return;
    case PLAYER_RANDOM_CLUB_FRAME:
        Gv_SetVarX(lVar2, ps->random_club_frame); return;
    case PLAYER_FIST_INCS:
        Gv_SetVarX(lVar2, ps->fist_incs); return;
    case PLAYER_ONE_EIGHTY_COUNT:
        Gv_SetVarX(lVar2, ps->one_eighty_count); return;
    case PLAYER_CHEAT_PHASE:
        Gv_SetVarX(lVar2, ps->cheat_phase); return;
    case PLAYER_DUMMYPLAYERSPRITE:
        Gv_SetVarX(lVar2, ps->dummyplayersprite); return;
    case PLAYER_EXTRA_EXTRA8:
        Gv_SetVarX(lVar2, ps->extra_extra8); return;
    case PLAYER_QUICK_KICK:
        Gv_SetVarX(lVar2, ps->quick_kick); return;
    case PLAYER_HEAT_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_HEATS]); return;
    case PLAYER_ACTORSQU:
        Gv_SetVarX(lVar2, ps->actorsqu); return;
    case PLAYER_TIMEBEFOREEXIT:
        Gv_SetVarX(lVar2, ps->timebeforeexit); return;
    case PLAYER_CUSTOMEXITSOUND:
        Gv_SetVarX(lVar2, ps->customexitsound); return;
    case PLAYER_WEAPRECS:
        Gv_SetVarX(lVar2, ps->weaprecs[lParm2]); return;
    case PLAYER_WEAPRECCNT:
        Gv_SetVarX(lVar2, ps->weapreccnt); return;
    case PLAYER_INTERFACE_TOGGLE_FLAG:
        Gv_SetVarX(lVar2, ps->interface_toggle_flag); return;
    case PLAYER_ROTSCRNANG:
        Gv_SetVarX(lVar2, ps->rotscrnang); return;
    case PLAYER_DEAD_FLAG:
        Gv_SetVarX(lVar2, ps->dead_flag); return;
    case PLAYER_SHOW_EMPTY_WEAPON:
        Gv_SetVarX(lVar2, ps->show_empty_weapon); return;
    case PLAYER_SCUBA_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_SCUBA]); return;
    case PLAYER_JETPACK_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_JETPACK]); return;
    case PLAYER_STEROIDS_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_STEROIDS]); return;
    case PLAYER_SHIELD_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_SHIELD]); return;
    case PLAYER_HOLODUKE_ON:
        Gv_SetVarX(lVar2, ps->holoduke_on); return;
    case PLAYER_PYCOUNT:
        Gv_SetVarX(lVar2, ps->pycount); return;
    case PLAYER_WEAPON_POS:
        Gv_SetVarX(lVar2, ps->weapon_pos); return;
    case PLAYER_FRAG_PS:
        Gv_SetVarX(lVar2, ps->frag_ps); return;
    case PLAYER_TRANSPORTER_HOLD:
        Gv_SetVarX(lVar2, ps->transporter_hold); return;
    case PLAYER_LAST_FULL_WEAPON:
        Gv_SetVarX(lVar2, ps->last_full_weapon); return;
    case PLAYER_FOOTPRINTSHADE:
        Gv_SetVarX(lVar2, ps->footprintshade); return;
    case PLAYER_BOOT_AMOUNT:
        Gv_SetVarX(lVar2, ps->inv_amount[GET_BOOTS]); return;
    case PLAYER_SCREAM_VOICE:
        Gv_SetVarX(lVar2, ps->scream_voice); return;
    case PLAYER_GM:
        Gv_SetVarX(lVar2, ps->gm); return;
    case PLAYER_ON_WARPING_SECTOR:
        Gv_SetVarX(lVar2, ps->on_warping_sector); return;
    case PLAYER_FOOTPRINTCOUNT:
        Gv_SetVarX(lVar2, ps->footprintcount); return;
    case PLAYER_HBOMB_ON:
        Gv_SetVarX(lVar2, ps->hbomb_on); return;
    case PLAYER_JUMPING_TOGGLE:
        Gv_SetVarX(lVar2, ps->jumping_toggle); return;
    case PLAYER_RAPID_FIRE_HOLD:
        Gv_SetVarX(lVar2, ps->rapid_fire_hold); return;
    case PLAYER_ON_GROUND:
        Gv_SetVarX(lVar2, ps->on_ground); return;
    case PLAYER_INVEN_ICON:
        Gv_SetVarX(lVar2, ps->inven_icon); return;
    case PLAYER_BUTTONPALETTE:
        Gv_SetVarX(lVar2, ps->buttonpalette); return;
    case PLAYER_JETPACK_ON:
        Gv_SetVarX(lVar2, ps->jetpack_on); return;
    case PLAYER_SPRITEBRIDGE:
        Gv_SetVarX(lVar2, ps->spritebridge); return;
    case PLAYER_LASTRANDOMSPOT:
        Gv_SetVarX(lVar2, ps->lastrandomspot); return;
    case PLAYER_SCUBA_ON:
        Gv_SetVarX(lVar2, ps->scuba_on); return;
    case PLAYER_FOOTPRINTPAL:
        Gv_SetVarX(lVar2, ps->footprintpal); return;
    case PLAYER_HEAT_ON:
        Gv_SetVarX(lVar2, ps->heat_on); return;
    case PLAYER_HOLSTER_WEAPON:
        Gv_SetVarX(lVar2, ps->holster_weapon); return;
    case PLAYER_FALLING_COUNTER:
        Gv_SetVarX(lVar2, ps->falling_counter); return;
    case PLAYER_GOTWEAPON:
        Gv_SetVarX(lVar2, (ps->gotweapon & (1<<lParm2)) != 0); return;
    case PLAYER_REFRESH_INVENTORY:
        Gv_SetVarX(lVar2, ps->refresh_inventory); return;
    case PLAYER_PALETTE:  // no set
        Gv_SetVarX(lVar2, ps->palette); return;
    case PLAYER_TOGGLE_KEY_FLAG:
        Gv_SetVarX(lVar2, ps->toggle_key_flag); return;
    case PLAYER_KNUCKLE_INCS:
        Gv_SetVarX(lVar2, ps->knuckle_incs); return;
    case PLAYER_WALKING_SND_TOGGLE:
        Gv_SetVarX(lVar2, ps->walking_snd_toggle); return;
    case PLAYER_PALOOKUP:
        Gv_SetVarX(lVar2, ps->palookup); return;
    case PLAYER_HARD_LANDING:
        Gv_SetVarX(lVar2, ps->hard_landing); return;
    case PLAYER_MAX_SECRET_ROOMS:
        Gv_SetVarX(lVar2, ps->max_secret_rooms); return;
    case PLAYER_SECRET_ROOMS:
        Gv_SetVarX(lVar2, ps->secret_rooms); return;
    case PLAYER_PALS:
        switch (lParm2)
        {
        case 0:
            Gv_SetVarX(lVar2, ps->pals.r); return;
        case 1:
            Gv_SetVarX(lVar2, ps->pals.g); return;
        case 2:
            Gv_SetVarX(lVar2, ps->pals.b); return;
        }
        return;
    case PLAYER_MAX_ACTORS_KILLED:
        Gv_SetVarX(lVar2, ps->max_actors_killed); return;
    case PLAYER_ACTORS_KILLED:
        Gv_SetVarX(lVar2, ps->actors_killed); return;
    case PLAYER_RETURN_TO_CENTER:
        Gv_SetVarX(lVar2, ps->return_to_center); return;
    case PLAYER_RUNSPEED:
        Gv_SetVarX(lVar2, ps->runspeed); return;
    case PLAYER_SBS:
        Gv_SetVarX(lVar2, ps->sbs); return;
    case PLAYER_RELOADING:
        Gv_SetVarX(lVar2, ps->reloading); return;
    case PLAYER_AUTO_AIM:
        Gv_SetVarX(lVar2, ps->auto_aim); return;
    case PLAYER_MOVEMENT_LOCK:
        Gv_SetVarX(lVar2, ps->movement_lock); return;
    case PLAYER_SOUND_PITCH:
        Gv_SetVarX(lVar2, ps->sound_pitch); return;
    case PLAYER_WEAPONSWITCH:
        Gv_SetVarX(lVar2, ps->weaponswitch); return;
    case PLAYER_TEAM:
        Gv_SetVarX(lVar2, ps->team); return;
    case PLAYER_MAX_PLAYER_HEALTH:
        Gv_SetVarX(lVar2, ps->max_player_health); return;
    case PLAYER_MAX_SHIELD_AMOUNT:
        Gv_SetVarX(lVar2, ps->max_shield_amount); return;
    case PLAYER_MAX_AMMO_AMOUNT:
        Gv_SetVarX(lVar2, ps->max_ammo_amount[lParm2]); return;
    case PLAYER_LAST_QUICK_KICK:
        Gv_SetVarX(lVar2, ps->last_quick_kick); return;
    case PLAYER_AUTOSTEP:
        Gv_SetVarX(lVar2, ps->autostep); return;
    case PLAYER_AUTOSTEP_SBW:
        Gv_SetVarX(lVar2, ps->autostep_sbw);
        return;

    default:
        return;
    }

badplayer:
    //        OSD_Printf("VM_AccessPlayer(): invalid target player (%d) %d\n",iPlayer,vm.g_i);
    CON_ERRPRINTF("tried to get %s on invalid target player (%d) from spr %d gv %s\n",
                  PlayerLabels[lLabelID].name,iPlayer,vm.g_i,
                  (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
    return;

badpos:
    CON_ERRPRINTF("tried to get invalid %s position %d on player (%d) from spr %d\n",
                  PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
    return;
}

static void __fastcall VM_SetPlayer(int32_t lVar1, int32_t lLabelID, int32_t lVar2, int32_t lParm2)
{
    register int32_t iPlayer=vm.g_p;
    DukePlayer_t *ps;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVarX(lVar1);

    ps = g_player[iPlayer].ps;

    if ((unsigned)iPlayer >= (unsigned)playerswhenstarted)
        goto badplayer;

    if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)PlayerLabels[lLabelID].maxParm2)
        goto badpos;

    lVar1=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        ps->zoom=lVar1; return;
    case PLAYER_EXITX:
        ps->exitx=lVar1; return;
    case PLAYER_EXITY:
        ps->exity=lVar1; return;
    case PLAYER_LOOGIEX:
        ps->loogiex[lParm2]=lVar1; return;
    case PLAYER_LOOGIEY:
        ps->loogiey[lParm2]=lVar1; return;
    case PLAYER_NUMLOOGS:
        ps->numloogs=lVar1; return;
    case PLAYER_LOOGCNT:
        ps->loogcnt=lVar1; return;
    case PLAYER_POSX:
        ps->pos.x=lVar1; return;
    case PLAYER_POSY:
        ps->pos.y=lVar1; return;
    case PLAYER_POSZ:
        ps->pos.z=lVar1; return;
    case PLAYER_HORIZ:
        ps->horiz=lVar1; return;
    case PLAYER_OHORIZ:
        ps->ohoriz=lVar1; return;
    case PLAYER_OHORIZOFF:
        ps->ohorizoff=lVar1; return;
    case PLAYER_INVDISPTIME:
        ps->invdisptime=lVar1; return;
    case PLAYER_BOBPOSX:
        ps->bobposx=lVar1; return;
    case PLAYER_BOBPOSY:
        ps->bobposy=lVar1; return;
    case PLAYER_OPOSX:
        ps->opos.x=lVar1; return;
    case PLAYER_OPOSY:
        ps->opos.y=lVar1; return;
    case PLAYER_OPOSZ:
        ps->opos.z=lVar1; return;
    case PLAYER_PYOFF:
        ps->pyoff=lVar1; return;
    case PLAYER_OPYOFF:
        ps->opyoff=lVar1; return;
    case PLAYER_POSXV:
        ps->vel.x=lVar1; return;
    case PLAYER_POSYV:
        ps->vel.y=lVar1; return;
    case PLAYER_POSZV:
        ps->vel.z=lVar1; return;
    case PLAYER_LAST_PISSED_TIME:
        ps->last_pissed_time=lVar1; return;
    case PLAYER_TRUEFZ:
        ps->truefz=lVar1; return;
    case PLAYER_TRUECZ:
        ps->truecz=lVar1; return;
    case PLAYER_PLAYER_PAR:
        ps->player_par=lVar1; return;
    case PLAYER_VISIBILITY:
        ps->visibility=lVar1; return;
    case PLAYER_BOBCOUNTER:
        ps->bobcounter=lVar1; return;
    case PLAYER_WEAPON_SWAY:
        ps->weapon_sway=lVar1; return;
    case PLAYER_PALS_TIME:
        ps->pals.f=lVar1; return;
    case PLAYER_RANDOMFLAMEX:
        ps->randomflamex=lVar1; return;
    case PLAYER_CRACK_TIME:
        ps->crack_time=lVar1; return;
    case PLAYER_AIM_MODE:
        ps->aim_mode=lVar1; return;
    case PLAYER_ANG:
        ps->ang=lVar1; return;
    case PLAYER_OANG:
        ps->oang=lVar1; return;
    case PLAYER_ANGVEL:
        ps->angvel=lVar1; return;
    case PLAYER_CURSECTNUM:
        ps->cursectnum=lVar1; return;
    case PLAYER_LOOK_ANG:
        ps->look_ang=lVar1; return;
    case PLAYER_LAST_EXTRA:
        ps->last_extra=lVar1; return;
    case PLAYER_SUBWEAPON:
        ps->subweapon=lVar1; return;
    case PLAYER_AMMO_AMOUNT:
        ps->ammo_amount[lParm2]=lVar1; return;
    case PLAYER_WACKEDBYACTOR:
        ps->wackedbyactor=lVar1; return;
    case PLAYER_FRAG:
        ps->frag=lVar1; return;
    case PLAYER_FRAGGEDSELF:
        ps->fraggedself=lVar1; return;
    case PLAYER_CURR_WEAPON:
        ps->curr_weapon=lVar1; return;
    case PLAYER_LAST_WEAPON:
        ps->last_weapon=lVar1; return;
    case PLAYER_TIPINCS:
        ps->tipincs=lVar1; return;
    case PLAYER_HORIZOFF:
        ps->horizoff=lVar1; return;
    case PLAYER_WANTWEAPONFIRE:
        ps->wantweaponfire=lVar1; return;
    case PLAYER_HOLODUKE_AMOUNT:
        ps->inv_amount[GET_HOLODUKE]=lVar1; return;
    case PLAYER_NEWOWNER:
        ps->newowner=lVar1; return;
    case PLAYER_HURT_DELAY:
        ps->hurt_delay=lVar1; return;
    case PLAYER_HBOMB_HOLD_DELAY:
        ps->hbomb_hold_delay=lVar1; return;
    case PLAYER_JUMPING_COUNTER:
        ps->jumping_counter=lVar1; return;
    case PLAYER_AIRLEFT:
        ps->airleft=lVar1; return;
    case PLAYER_KNEE_INCS:
        ps->knee_incs=lVar1; return;
    case PLAYER_ACCESS_INCS:
        ps->access_incs=lVar1; return;
    case PLAYER_FTA:
        ps->fta=lVar1; return;
    case PLAYER_FTQ:
        ps->ftq=lVar1; return;
    case PLAYER_ACCESS_WALLNUM:
        ps->access_wallnum=lVar1; return;
    case PLAYER_ACCESS_SPRITENUM:
        ps->access_spritenum=lVar1; return;
    case PLAYER_KICKBACK_PIC:
        ps->kickback_pic=lVar1; return;
    case PLAYER_GOT_ACCESS:
        ps->got_access=lVar1; return;
    case PLAYER_WEAPON_ANG:
        ps->weapon_ang=lVar1; return;
    case PLAYER_FIRSTAID_AMOUNT:
        ps->inv_amount[GET_FIRSTAID]=lVar1; return;
    case PLAYER_SOMETHINGONPLAYER:
        ps->somethingonplayer=lVar1; return;
    case PLAYER_ON_CRANE:
        ps->on_crane=lVar1; return;
    case PLAYER_I:
        ps->i=lVar1; return;
    case PLAYER_ONE_PARALLAX_SECTNUM:
        ps->one_parallax_sectnum=lVar1; return;
    case PLAYER_OVER_SHOULDER_ON:
        ps->over_shoulder_on=lVar1; return;
    case PLAYER_RANDOM_CLUB_FRAME:
        ps->random_club_frame=lVar1; return;
    case PLAYER_FIST_INCS:
        ps->fist_incs=lVar1; return;
    case PLAYER_ONE_EIGHTY_COUNT:
        ps->one_eighty_count=lVar1; return;
    case PLAYER_CHEAT_PHASE:
        ps->cheat_phase=lVar1; return;
    case PLAYER_DUMMYPLAYERSPRITE:
        ps->dummyplayersprite=lVar1; return;
    case PLAYER_EXTRA_EXTRA8:
        ps->extra_extra8=lVar1; return;
    case PLAYER_QUICK_KICK:
        ps->quick_kick=lVar1; return;
    case PLAYER_HEAT_AMOUNT:
        ps->inv_amount[GET_HEATS]=lVar1; return;
    case PLAYER_ACTORSQU:
        ps->actorsqu=lVar1; return;
    case PLAYER_TIMEBEFOREEXIT:
        ps->timebeforeexit=lVar1; return;
    case PLAYER_CUSTOMEXITSOUND:
        ps->customexitsound=lVar1; return;
    case PLAYER_WEAPRECS:
        ps->weaprecs[lParm2]=lVar1; return;
    case PLAYER_WEAPRECCNT:
        ps->weapreccnt=lVar1; return;
    case PLAYER_INTERFACE_TOGGLE_FLAG:
        ps->interface_toggle_flag=lVar1; return;
    case PLAYER_ROTSCRNANG:
        ps->rotscrnang=lVar1; return;
    case PLAYER_DEAD_FLAG:
        ps->dead_flag=lVar1; return;
    case PLAYER_SHOW_EMPTY_WEAPON:
        ps->show_empty_weapon=lVar1; return;
    case PLAYER_SCUBA_AMOUNT:
        ps->inv_amount[GET_SCUBA]=lVar1; return;
    case PLAYER_JETPACK_AMOUNT:
        ps->inv_amount[GET_JETPACK]=lVar1; return;
    case PLAYER_STEROIDS_AMOUNT:
        ps->inv_amount[GET_STEROIDS]=lVar1; return;
    case PLAYER_SHIELD_AMOUNT:
        ps->inv_amount[GET_SHIELD]=lVar1; return;
    case PLAYER_HOLODUKE_ON:
        ps->holoduke_on=lVar1; return;
    case PLAYER_PYCOUNT:
        ps->pycount=lVar1; return;
    case PLAYER_WEAPON_POS:
        ps->weapon_pos=lVar1; return;
    case PLAYER_FRAG_PS:
        ps->frag_ps=lVar1; return;
    case PLAYER_TRANSPORTER_HOLD:
        ps->transporter_hold=lVar1; return;
    case PLAYER_LAST_FULL_WEAPON:
        ps->last_full_weapon=lVar1; return;
    case PLAYER_FOOTPRINTSHADE:
        ps->footprintshade=lVar1; return;
    case PLAYER_BOOT_AMOUNT:
        ps->inv_amount[GET_BOOTS]=lVar1; return;
    case PLAYER_SCREAM_VOICE:
        ps->scream_voice=lVar1; return;
    case PLAYER_GM:
        ps->gm=lVar1; return;
    case PLAYER_ON_WARPING_SECTOR:
        ps->on_warping_sector=lVar1; return;
    case PLAYER_FOOTPRINTCOUNT:
        ps->footprintcount=lVar1; return;
    case PLAYER_HBOMB_ON:
        ps->hbomb_on=lVar1; return;
    case PLAYER_JUMPING_TOGGLE:
        ps->jumping_toggle=lVar1; return;
    case PLAYER_RAPID_FIRE_HOLD:
        ps->rapid_fire_hold=lVar1; return;
    case PLAYER_ON_GROUND:
        ps->on_ground=lVar1; return;
    case PLAYER_INVEN_ICON:
        ps->inven_icon=lVar1; return;
    case PLAYER_BUTTONPALETTE:
        ps->buttonpalette=lVar1; return;
    case PLAYER_JETPACK_ON:
        ps->jetpack_on=lVar1; return;
    case PLAYER_SPRITEBRIDGE:
        ps->spritebridge=lVar1; return;
    case PLAYER_LASTRANDOMSPOT:
        ps->lastrandomspot=lVar1; return;
    case PLAYER_SCUBA_ON:
        ps->scuba_on=lVar1; return;
    case PLAYER_FOOTPRINTPAL:
        ps->footprintpal=lVar1; return;
    case PLAYER_HEAT_ON:
        if (ps->heat_on != lVar1)
        {
            ps->heat_on=lVar1;
            P_UpdateScreenPal(ps);
        } return;
    case PLAYER_HOLSTER_WEAPON:
        ps->holster_weapon=lVar1; return;
    case PLAYER_FALLING_COUNTER:
        ps->falling_counter=lVar1; return;
    case PLAYER_GOTWEAPON:
        if (lVar1)
        {
            ps->gotweapon |= (1<<lParm2);
            return;
        }

        ps->gotweapon &= ~(1<<lParm2);
        return;
    case PLAYER_REFRESH_INVENTORY:
        ps->refresh_inventory=lVar1; return;
    case PLAYER_TOGGLE_KEY_FLAG:
        ps->toggle_key_flag=lVar1; return;
    case PLAYER_KNUCKLE_INCS:
        ps->knuckle_incs=lVar1; return;
    case PLAYER_WALKING_SND_TOGGLE:
        ps->walking_snd_toggle=lVar1; return;
    case PLAYER_PALOOKUP:
        ps->palookup=lVar1; return;
    case PLAYER_HARD_LANDING:
        ps->hard_landing=lVar1; return;
    case PLAYER_MAX_SECRET_ROOMS:
        ps->max_secret_rooms=lVar1; return;
    case PLAYER_SECRET_ROOMS:
        ps->secret_rooms=lVar1; return;
    case PLAYER_PALS:
        switch (lParm2)
        {
        case 0:
            ps->pals.r = lVar1; return;
        case 1:
            ps->pals.g = lVar1; return;
        case 2:
            ps->pals.b = lVar1; return;
        }
        return;
    case PLAYER_MAX_ACTORS_KILLED:
        ps->max_actors_killed=lVar1; return;
    case PLAYER_ACTORS_KILLED:
        ps->actors_killed=lVar1; return;
    case PLAYER_RETURN_TO_CENTER:
        ps->return_to_center=lVar1; return;
    case PLAYER_RUNSPEED:
        ps->runspeed=lVar1; return;
    case PLAYER_SBS:
        ps->sbs=lVar1; return;
    case PLAYER_RELOADING:
        ps->reloading=lVar1; return;
    case PLAYER_AUTO_AIM:
        ps->auto_aim=lVar1; return;
    case PLAYER_MOVEMENT_LOCK:
        ps->movement_lock=lVar1; return;
    case PLAYER_SOUND_PITCH:
        ps->sound_pitch=lVar1; return;
    case PLAYER_WEAPONSWITCH:
        ps->weaponswitch=lVar1; return;
    case PLAYER_TEAM:
        ps->team=lVar1; return;
    case PLAYER_MAX_PLAYER_HEALTH:
        ps->max_player_health = lVar1; return;
    case PLAYER_MAX_SHIELD_AMOUNT:
        ps->max_shield_amount = lVar1; return;
    case PLAYER_MAX_AMMO_AMOUNT:
        ps->max_ammo_amount[lParm2]=lVar1; return;
    case PLAYER_LAST_QUICK_KICK:
        ps->last_quick_kick=lVar1; return;
    case PLAYER_AUTOSTEP:
        ps->autostep=lVar1; return;
    case PLAYER_AUTOSTEP_SBW:
        ps->autostep_sbw=lVar1;
        return;

    default:
        return;
    }

badplayer:
    //        OSD_Printf("VM_AccessPlayer(): invalid target player (%d) %d\n",iPlayer,vm.g_i);
    CON_ERRPRINTF("tried to set %s on invalid target player (%d) from spr %d gv %s\n",
                  PlayerLabels[lLabelID].name,iPlayer,vm.g_i,
                  (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
    insptr += (lVar2 == MAXGAMEVARS);
    return;

badpos:
    CON_ERRPRINTF("tried to set invalid %s position %d on player (%d) from spr %d\n",
                  PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
    insptr += (lVar2 == MAXGAMEVARS);
    return;
}

static void __fastcall VM_AccessPlayerInput(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iPlayer=vm.g_p;

    if (lVar1 != g_iThisActorID)
        iPlayer=Gv_GetVarX(lVar1);

    if ((unsigned)iPlayer >= (unsigned)playerswhenstarted)
        goto badplayer;

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case INPUT_AVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->avel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->avel);
        return;

    case INPUT_HORZ:
        if (iSet)
        {
            g_player[iPlayer].sync->horz=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->horz);
        return;

    case INPUT_FVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->fvel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->fvel);
        return;

    case INPUT_SVEL:
        if (iSet)
        {
            g_player[iPlayer].sync->svel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->svel);
        return;

    case INPUT_BITS:
        if (iSet)
        {
            g_player[iPlayer].sync->bits=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->bits);
        return;

    case INPUT_EXTBITS:
        if (iSet)
        {
            g_player[iPlayer].sync->extbits=lValue;
            return;
        }
        Gv_SetVarX(lVar2, g_player[iPlayer].sync->extbits);
        return;
    default:
        return;
    }

badplayer:
    insptr += (lVar2 == MAXGAMEVARS);
    CON_ERRPRINTF("invalid target player (%d) %d\n", iPlayer,vm.g_i);
    return;
}

static void __fastcall VM_AccessWall(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iWall = Gv_GetVarX(lVar1);

    if ((unsigned)iWall >= (unsigned)numwalls)
        goto badwall;

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case WALL_X:
        if (iSet)
        {
            wall[iWall].x=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].x);
        return;

    case WALL_Y:
        if (iSet)
        {
            wall[iWall].y=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].y);
        return;

    case WALL_POINT2:
        if (iSet)
        {
            wall[iWall].point2=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].point2);
        return;

    case WALL_NEXTWALL:
        if (iSet)
        {
            wall[iWall].nextwall=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].nextwall);
        return;

    case WALL_NEXTSECTOR:
        if (iSet)
        {
            wall[iWall].nextsector=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].nextsector);
        return;

    case WALL_CSTAT:
        if (iSet)
        {
            wall[iWall].cstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].cstat);
        return;

    case WALL_PICNUM:
        if (iSet)
        {
            wall[iWall].picnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].picnum);
        return;

    case WALL_OVERPICNUM:
        if (iSet)
        {
            wall[iWall].overpicnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].overpicnum);
        return;

    case WALL_SHADE:
        if (iSet)
        {
            wall[iWall].shade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].shade);
        return;

    case WALL_PAL:
        if (iSet)
        {
            wall[iWall].pal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].pal);
        return;

    case WALL_XREPEAT:
        if (iSet)
        {
            wall[iWall].xrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].xrepeat);
        return;

    case WALL_YREPEAT:
        if (iSet)
        {
            wall[iWall].yrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].yrepeat);
        return;

    case WALL_XPANNING:
        if (iSet)
        {
            wall[iWall].xpanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].xpanning);
        return;

    case WALL_YPANNING:
        if (iSet)
        {
            wall[iWall].ypanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].ypanning);
        return;

    case WALL_LOTAG:
        if (iSet)
        {
            wall[iWall].lotag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)wall[iWall].lotag);
        return;

    case WALL_HITAG:
        if (iSet)
        {
            wall[iWall].hitag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)wall[iWall].hitag);
        return;

    case WALL_ULOTAG:
        if (iSet)
        {
            wall[iWall].lotag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].lotag);
        return;

    case WALL_UHITAG:
        if (iSet)
        {
            wall[iWall].hitag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].hitag);
        return;

    case WALL_EXTRA:
        if (iSet)
        {
            wall[iWall].extra=lValue;
            return;
        }
        Gv_SetVarX(lVar2, wall[iWall].extra);
        return;
    default:
        return;
    }

badwall:
    insptr += (lVar2 == MAXGAMEVARS);
    CON_ERRPRINTF("Invalid wall %d\n", iWall);
    return;
}

static void __fastcall VM_AccessSector(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iSector=sprite[vm.g_i].sectnum;

    if (lVar1 != g_iThisActorID)
        iSector=Gv_GetVarX(lVar1);

    if ((unsigned)iSector >= (unsigned)numsectors)
        goto badsector;

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case SECTOR_WALLPTR:
        if (iSet)
        {
            sector[iSector].wallptr=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].wallptr);
        return;

    case SECTOR_WALLNUM:
        if (iSet)
        {
            sector[iSector].wallnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].wallnum);
        return;

    case SECTOR_CEILINGZ:
        if (iSet)
        {
            sector[iSector].ceilingz=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingz);
        return;

    case SECTOR_FLOORZ:
        if (iSet)
        {
            sector[iSector].floorz=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorz);
        return;

    case SECTOR_CEILINGSTAT:
        if (iSet)
        {
            sector[iSector].ceilingstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingstat);
        return;

    case SECTOR_FLOORSTAT:
        if (iSet)
        {
            sector[iSector].floorstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorstat);
        return;

    case SECTOR_CEILINGPICNUM:
        if (iSet)
        {
            sector[iSector].ceilingpicnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingpicnum);
        return;

    case SECTOR_CEILINGSLOPE:
        if (iSet)
        {
            sector[iSector].ceilingheinum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingheinum);
        return;

    case SECTOR_CEILINGSHADE:
        if (iSet)
        {
            sector[iSector].ceilingshade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingshade);
        return;

    case SECTOR_CEILINGPAL:
        if (iSet)
        {
            sector[iSector].ceilingpal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingpal);
        return;

    case SECTOR_CEILINGXPANNING:
        if (iSet)
        {
            sector[iSector].ceilingxpanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingxpanning);
        return;

    case SECTOR_CEILINGYPANNING:
        if (iSet)
        {
            sector[iSector].ceilingypanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].ceilingypanning);
        return;

    case SECTOR_FLOORPICNUM:
        if (iSet)
        {
            sector[iSector].floorpicnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorpicnum);
        return;

    case SECTOR_FLOORSLOPE:
        if (iSet)
        {
            sector[iSector].floorheinum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorheinum);
        return;

    case SECTOR_FLOORSHADE:
        if (iSet)
        {
            sector[iSector].floorshade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorshade);
        return;

    case SECTOR_FLOORPAL:
        if (iSet)
        {
            sector[iSector].floorpal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorpal);
        return;

    case SECTOR_FLOORXPANNING:
        if (iSet)
        {
            sector[iSector].floorxpanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorxpanning);
        return;

    case SECTOR_FLOORYPANNING:
        if (iSet)
        {
            sector[iSector].floorypanning=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].floorypanning);
        return;

    case SECTOR_VISIBILITY:
        if (iSet)
        {
            sector[iSector].visibility=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].visibility);
        return;

    case SECTOR_ALIGNTO:
        if (iSet)
        {
            sector[iSector].filler=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].filler);
        return;

    case SECTOR_LOTAG:
        if (iSet)
        {
            sector[iSector].lotag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)sector[iSector].lotag);
        return;

    case SECTOR_HITAG:
        if (iSet)
        {
            sector[iSector].hitag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)sector[iSector].hitag);
        return;

    case SECTOR_ULOTAG:
        if (iSet)
        {
            sector[iSector].lotag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].lotag);
        return;

    case SECTOR_UHITAG:
        if (iSet)
        {
            sector[iSector].hitag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].hitag);
        return;

    case SECTOR_EXTRA:
        if (iSet)
        {
            sector[iSector].extra=lValue;
            return;
        }
        Gv_SetVarX(lVar2, sector[iSector].extra);
        return;

    case SECTOR_CEILINGBUNCH:
    case SECTOR_FLOORBUNCH:
        if (!iSet)
        {
#ifdef YAX_ENABLE
            Gv_SetVarX(lVar2, yax_getbunch(iSector, lLabelID==SECTOR_FLOORBUNCH));
#else
            Gv_SetVarX(lVar2, -1);
#endif
        }
        return;

    default:
        return;
    }

badsector:
    CON_ERRPRINTF("Invalid sector %d\n", iSector);
    insptr += (lVar2 == MAXGAMEVARS);
    return;
}

static void __fastcall VM_SetSprite(int32_t lVar1, int32_t lLabelID, int32_t lVar2, int32_t lParm2)
{
    register int32_t iActor=vm.g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVarX(lVar1);

    if ((unsigned)iActor >= MAXSPRITES)
        goto badactor;

    if (ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[lLabelID].maxParm2)
        goto badpos;

    lVar1=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case ACTOR_X:
        sprite[iActor].x=lVar1;
        return;

    case ACTOR_Y:
        sprite[iActor].y=lVar1;
        return;

    case ACTOR_Z:
        sprite[iActor].z=lVar1;
        return;

    case ACTOR_CSTAT:
        sprite[iActor].cstat=lVar1;
        return;

    case ACTOR_PICNUM:
        sprite[iActor].picnum=lVar1;
        return;

    case ACTOR_SHADE:
        sprite[iActor].shade=lVar1;
        return;

    case ACTOR_PAL:
        sprite[iActor].pal=lVar1;
        return;

    case ACTOR_CLIPDIST:
        sprite[iActor].clipdist=lVar1;
        return;

    case ACTOR_DETAIL:
        sprite[iActor].filler=lVar1;
        return;

    case ACTOR_XREPEAT:
        sprite[iActor].xrepeat=lVar1;
        return;

    case ACTOR_YREPEAT:
        sprite[iActor].yrepeat=lVar1;
        return;

    case ACTOR_XOFFSET:
        sprite[iActor].xoffset=lVar1;
        return;

    case ACTOR_YOFFSET:
        sprite[iActor].yoffset=lVar1;
        return;

    case ACTOR_SECTNUM:
        changespritesect(iActor,lVar1);
        return;

    case ACTOR_STATNUM:
        changespritestat(iActor,lVar1);
        return;

    case ACTOR_ANG:
        sprite[iActor].ang=lVar1;
        return;

    case ACTOR_OWNER:
        sprite[iActor].owner=lVar1;
        return;

    case ACTOR_XVEL:
        sprite[iActor].xvel=lVar1;
        return;

    case ACTOR_YVEL:
        sprite[iActor].yvel=lVar1;
        return;

    case ACTOR_ZVEL:
        sprite[iActor].zvel=lVar1;
        return;

    case ACTOR_LOTAG:
        sprite[iActor].lotag=(int16_t)lVar1;
        return;

    case ACTOR_HITAG:
        sprite[iActor].hitag=(int16_t)lVar1;
        return;

    case ACTOR_ULOTAG:
        sprite[iActor].lotag=lVar1;
        return;

    case ACTOR_UHITAG:
        sprite[iActor].hitag=lVar1;
        return;

    case ACTOR_EXTRA:
        sprite[iActor].extra=lVar1;
        return;

    case ACTOR_HTCGG:
        actor[iActor].cgg=lVar1;
        return;

    case ACTOR_HTPICNUM :
        actor[iActor].picnum=lVar1;
        return;

    case ACTOR_HTANG:
        actor[iActor].ang=lVar1;
        return;

    case ACTOR_HTEXTRA:
        actor[iActor].extra=lVar1;
        return;

    case ACTOR_HTOWNER:
        actor[iActor].owner=lVar1;
        return;

    case ACTOR_HTMOVFLAG:
        actor[iActor].movflag=lVar1;
        return;

    case ACTOR_HTTEMPANG:
        actor[iActor].tempang=lVar1;
        return;

    case ACTOR_HTACTORSTAYPUT:
        actor[iActor].actorstayput=lVar1;
        return;

    case ACTOR_HTDISPICNUM:
        actor[iActor].dispicnum=lVar1;
        return;

    case ACTOR_HTTIMETOSLEEP:
        actor[iActor].timetosleep=lVar1;
        return;

    case ACTOR_HTFLOORZ:
        actor[iActor].floorz=lVar1;
        return;

    case ACTOR_HTCEILINGZ:
        actor[iActor].ceilingz=lVar1;
        return;

    case ACTOR_HTLASTVX:
        actor[iActor].lastvx=lVar1;
        return;

    case ACTOR_HTLASTVY:
        actor[iActor].lastvy=lVar1;
        return;

    case ACTOR_HTBPOSX:
        actor[iActor].bpos.x=lVar1;
        return;

    case ACTOR_HTBPOSY:
        actor[iActor].bpos.y=lVar1;
        return;

    case ACTOR_HTBPOSZ:
        actor[iActor].bpos.z=lVar1;
        return;

    case ACTOR_HTG_T:
        actor[iActor].t_data[lParm2]=lVar1;
        return;

    case ACTOR_ANGOFF:
        spriteext[iActor].angoff=lVar1;
        return;

    case ACTOR_PITCH:
        spriteext[iActor].pitch=lVar1;
        return;

    case ACTOR_ROLL:
        spriteext[iActor].roll=lVar1;
        return;

    case ACTOR_MDXOFF:
        spriteext[iActor].xoff=lVar1;
        return;

    case ACTOR_MDYOFF:
        spriteext[iActor].yoff=lVar1;
        return;

    case ACTOR_MDZOFF:
        spriteext[iActor].zoff=lVar1;
        return;

    case ACTOR_MDFLAGS:
        spriteext[iActor].flags=lVar1;
        return;

    case ACTOR_XPANNING:
        spriteext[iActor].xpanning=lVar1;
        return;

    case ACTOR_YPANNING:
        spriteext[iActor].ypanning=lVar1;
        return;

    case ACTOR_HTFLAGS:
        actor[iActor].flags=lVar1;
        return;

    case ACTOR_ALPHA:
        spriteext[iActor].alpha=(float)(lVar1/255.0f);
        return;

    default:
        return;
    }

badactor:
    CON_ERRPRINTF("tried to set %s on invalid target sprite (%d) from spr %d pic %d gv %s\n",
                  ActorLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum),
                  (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
    insptr += (lVar2 == MAXGAMEVARS);
    return;

badpos:
    CON_ERRPRINTF("tried to set invalid %s position %d on sprite (%d) from spr %d\n",
                  ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
    insptr += (lVar2 == MAXGAMEVARS);
    return;
}


static void __fastcall VM_GetSprite(int32_t lVar1, int32_t lLabelID, int32_t lVar2, int32_t lParm2)
{
    register int32_t iActor=vm.g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVarX(lVar1);

    if ((unsigned)iActor >= MAXSPRITES)
        goto badactor;

    if (ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[lLabelID].maxParm2)
        goto badpos;

    switch (lLabelID)
    {
    case ACTOR_X:
        Gv_SetVarX(lVar2, sprite[iActor].x);
        return;

    case ACTOR_Y:
        Gv_SetVarX(lVar2, sprite[iActor].y);
        return;

    case ACTOR_Z:
        Gv_SetVarX(lVar2, sprite[iActor].z);
        return;

    case ACTOR_CSTAT:
        Gv_SetVarX(lVar2, sprite[iActor].cstat);
        return;

    case ACTOR_PICNUM:
        Gv_SetVarX(lVar2, sprite[iActor].picnum);
        return;

    case ACTOR_SHADE:
        Gv_SetVarX(lVar2, sprite[iActor].shade);
        return;

    case ACTOR_PAL:
        Gv_SetVarX(lVar2, sprite[iActor].pal);
        return;

    case ACTOR_CLIPDIST:
        Gv_SetVarX(lVar2, sprite[iActor].clipdist);
        return;

    case ACTOR_DETAIL:
        Gv_SetVarX(lVar2, sprite[iActor].filler);
        return;

    case ACTOR_XREPEAT:
        Gv_SetVarX(lVar2, sprite[iActor].xrepeat);
        return;

    case ACTOR_YREPEAT:
        Gv_SetVarX(lVar2, sprite[iActor].yrepeat);
        return;

    case ACTOR_XOFFSET:
        Gv_SetVarX(lVar2, sprite[iActor].xoffset);
        return;

    case ACTOR_YOFFSET:
        Gv_SetVarX(lVar2, sprite[iActor].yoffset);
        return;

    case ACTOR_SECTNUM:
        Gv_SetVarX(lVar2, sprite[iActor].sectnum);
        return;

    case ACTOR_STATNUM:
        Gv_SetVarX(lVar2, sprite[iActor].statnum);
        return;

    case ACTOR_ANG:
        Gv_SetVarX(lVar2, sprite[iActor].ang);
        return;

    case ACTOR_OWNER:
        Gv_SetVarX(lVar2, sprite[iActor].owner);
        return;

    case ACTOR_XVEL:
        Gv_SetVarX(lVar2, sprite[iActor].xvel);
        return;

    case ACTOR_YVEL:
        Gv_SetVarX(lVar2, sprite[iActor].yvel);
        return;

    case ACTOR_ZVEL:
        Gv_SetVarX(lVar2, sprite[iActor].zvel);
        return;

    case ACTOR_LOTAG:
        Gv_SetVarX(lVar2, (int16_t)sprite[iActor].lotag);
        return;

    case ACTOR_HITAG:
        Gv_SetVarX(lVar2, (int16_t)sprite[iActor].hitag);
        return;

    case ACTOR_ULOTAG:
        Gv_SetVarX(lVar2, sprite[iActor].lotag);
        return;

    case ACTOR_UHITAG:
        Gv_SetVarX(lVar2, sprite[iActor].hitag);
        return;

    case ACTOR_EXTRA:
        Gv_SetVarX(lVar2, sprite[iActor].extra);
        return;

    case ACTOR_HTCGG:
        Gv_SetVarX(lVar2, actor[iActor].cgg);
        return;

    case ACTOR_HTPICNUM :
        Gv_SetVarX(lVar2, actor[iActor].picnum);
        return;

    case ACTOR_HTANG:
        Gv_SetVarX(lVar2, actor[iActor].ang);
        return;

    case ACTOR_HTEXTRA:
        Gv_SetVarX(lVar2,actor[iActor].extra);
        return;

    case ACTOR_HTOWNER:
        Gv_SetVarX(lVar2,actor[iActor].owner);
        return;

    case ACTOR_HTMOVFLAG:
        Gv_SetVarX(lVar2,actor[iActor].movflag);
        return;

    case ACTOR_HTTEMPANG:
        Gv_SetVarX(lVar2,actor[iActor].tempang);
        return;

    case ACTOR_HTACTORSTAYPUT:
        Gv_SetVarX(lVar2,actor[iActor].actorstayput);
        return;

    case ACTOR_HTDISPICNUM:
        Gv_SetVarX(lVar2,actor[iActor].dispicnum);
        return;

    case ACTOR_HTTIMETOSLEEP:
        Gv_SetVarX(lVar2,actor[iActor].timetosleep);
        return;

    case ACTOR_HTFLOORZ:
        Gv_SetVarX(lVar2,actor[iActor].floorz);
        return;

    case ACTOR_HTCEILINGZ:
        Gv_SetVarX(lVar2,actor[iActor].ceilingz);
        return;

    case ACTOR_HTLASTVX:
        Gv_SetVarX(lVar2,actor[iActor].lastvx);
        return;

    case ACTOR_HTLASTVY:
        Gv_SetVarX(lVar2,actor[iActor].lastvy);
        return;

    case ACTOR_HTBPOSX:
        Gv_SetVarX(lVar2,actor[iActor].bpos.x);
        return;

    case ACTOR_HTBPOSY:
        Gv_SetVarX(lVar2,actor[iActor].bpos.y);
        return;

    case ACTOR_HTBPOSZ:
        Gv_SetVarX(lVar2,actor[iActor].bpos.z);
        return;

    case ACTOR_HTG_T:
        Gv_SetVarX(lVar2, actor[iActor].t_data[lParm2]);
        return;

    case ACTOR_ANGOFF:
        Gv_SetVarX(lVar2,spriteext[iActor].angoff);
        return;

    case ACTOR_PITCH:
        Gv_SetVarX(lVar2,spriteext[iActor].pitch);
        return;

    case ACTOR_ROLL:
        Gv_SetVarX(lVar2,spriteext[iActor].roll);
        return;

    case ACTOR_MDXOFF:
        Gv_SetVarX(lVar2,spriteext[iActor].xoff);
        return;

    case ACTOR_MDYOFF:
        Gv_SetVarX(lVar2,spriteext[iActor].yoff);
        return;

    case ACTOR_MDZOFF:
        Gv_SetVarX(lVar2,spriteext[iActor].zoff);
        return;

    case ACTOR_MDFLAGS:
        Gv_SetVarX(lVar2,spriteext[iActor].flags);
        return;

    case ACTOR_XPANNING:
        Gv_SetVarX(lVar2, spriteext[iActor].xpanning);
        return;

    case ACTOR_YPANNING:
        Gv_SetVarX(lVar2, spriteext[iActor].ypanning);
        return;

    case ACTOR_HTFLAGS:
        Gv_SetVarX(lVar2,actor[iActor].flags);
        return;

    case ACTOR_ALPHA:
        Gv_SetVarX(lVar2, (uint8_t)(spriteext[iActor].alpha * 255.0f));
        return;

    default:
        return;
    }

badactor:
    CON_ERRPRINTF("tried to get %s on invalid target sprite (%d) from spr %d pic %d gv %s\n",
                  ActorLabels[lLabelID].name,iActor,vm.g_i,TrackerCast(vm.g_sp->picnum),
                  (lVar1<MAXGAMEVARS)?aGameVars[lVar1].szLabel:"extended");
    insptr += (lVar2 == MAXGAMEVARS);
    return;

badpos:
    CON_ERRPRINTF("tried to get invalid %s position %d on sprite (%d) from spr %d\n",
                  ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
    insptr += (lVar2 == MAXGAMEVARS);
    return;
}


static void __fastcall VM_AccessTsprite(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;
    int32_t iActor=vm.g_i;

    if (lVar1 != g_iThisActorID)
        iActor=Gv_GetVarX(lVar1);

    if ((unsigned)iActor >= MAXSPRITES)
        goto badsprite;

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    if (!spriteext[iActor].tspr)
        goto badtspr;

    switch (lLabelID)
    {
    case ACTOR_X:
        if (iSet)
        {
            spriteext[iActor].tspr->x=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->x);
        return;

    case ACTOR_Y:
        if (iSet)
        {
            spriteext[iActor].tspr->y=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->y);
        return;

    case ACTOR_Z:
        if (iSet)
        {
            spriteext[iActor].tspr->z=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->z);
        return;

    case ACTOR_CSTAT:
        if (iSet)
        {
            spriteext[iActor].tspr->cstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->cstat);
        return;

    case ACTOR_PICNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->picnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->picnum);
        return;

    case ACTOR_SHADE:
        if (iSet)
        {
            spriteext[iActor].tspr->shade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->shade);
        return;

    case ACTOR_PAL:
        if (iSet)
        {
            spriteext[iActor].tspr->pal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->pal);
        return;

    case ACTOR_CLIPDIST:
        if (iSet)
        {
            spriteext[iActor].tspr->clipdist=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->clipdist);
        return;

    case ACTOR_DETAIL:
        if (iSet)
        {
            spriteext[iActor].tspr->filler=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->filler);
        return;

    case ACTOR_XREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->xrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->xrepeat);
        return;

    case ACTOR_YREPEAT:
        if (iSet)
        {
            spriteext[iActor].tspr->yrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->yrepeat);
        return;

    case ACTOR_XOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->xoffset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->xoffset);
        return;

    case ACTOR_YOFFSET:
        if (iSet)
        {
            spriteext[iActor].tspr->yoffset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->yoffset);
        return;

    case ACTOR_SECTNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->sectnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->sectnum);
        return;

    case ACTOR_STATNUM:
        if (iSet)
        {
            spriteext[iActor].tspr->statnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->statnum);
        return;

    case ACTOR_ANG:
        if (iSet)
        {
            spriteext[iActor].tspr->ang=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->ang);
        return;

    case ACTOR_OWNER:
        if (iSet)
        {
            spriteext[iActor].tspr->owner=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->owner);
        return;

#if 1
    case ACTOR_XVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->xvel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->xvel);
        return;

    case ACTOR_YVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->yvel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->yvel);
        return;

    case ACTOR_ZVEL:
        if (iSet)
        {
            spriteext[iActor].tspr->zvel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->zvel);
        return;

    case ACTOR_LOTAG:
        if (iSet)
        {
            spriteext[iActor].tspr->lotag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)spriteext[iActor].tspr->lotag);
        return;

    case ACTOR_HITAG:
        if (iSet)
        {
            spriteext[iActor].tspr->hitag=(int16_t)lValue;
            return;
        }
        Gv_SetVarX(lVar2, (int16_t)spriteext[iActor].tspr->hitag);
        return;

    case ACTOR_ULOTAG:
        if (iSet)
        {
            spriteext[iActor].tspr->lotag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->lotag);
        return;

    case ACTOR_UHITAG:
        if (iSet)
        {
            spriteext[iActor].tspr->hitag=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->hitag);
        return;

    case ACTOR_EXTRA:
        if (iSet)
        {
            spriteext[iActor].tspr->extra=lValue;
            return;
        }
        Gv_SetVarX(lVar2, spriteext[iActor].tspr->extra);
        return;
#endif

    default:
        return;
    }

badsprite:
    CON_ERRPRINTF("invalid target sprite (%d) %d %d\n", iActor, vm.g_i, TrackerCast(vm.g_sp->picnum));
    insptr += (lVar2 == MAXGAMEVARS);
    return;

badtspr:
    CON_ERRPRINTF("Internal bug, tsprite is unavailable\n");
    return;
}

static void __fastcall VM_AccessProjectile(int32_t iSet, int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    int32_t lValue=0;

    if ((unsigned)lVar1 >= MAXTILES)
        goto badtile;

    if (iSet)
        lValue=Gv_GetVarX(lVar2);

    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        if (iSet)
        {
            ProjectileData[lVar1].workslike=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].workslike);
        return;

    case PROJ_SPAWNS:
        if (iSet)
        {
            ProjectileData[lVar1].spawns=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].spawns);
        return;

    case PROJ_SXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].sxrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].sxrepeat);
        return;

    case PROJ_SYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].syrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].syrepeat);
        return;

    case PROJ_SOUND:
        if (iSet)
        {
            ProjectileData[lVar1].sound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].sound);
        return;

    case PROJ_ISOUND:
        if (iSet)
        {
            ProjectileData[lVar1].isound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].isound);
        return;

    case PROJ_VEL:
        if (iSet)
        {
            ProjectileData[lVar1].vel=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].vel);
        return;

    case PROJ_EXTRA:
        if (iSet)
        {
            ProjectileData[lVar1].extra=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].extra);
        return;

    case PROJ_DECAL:
        if (iSet)
        {
            ProjectileData[lVar1].decal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].decal);
        return;

    case PROJ_TRAIL:
        if (iSet)
        {
            ProjectileData[lVar1].trail=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].trail);
        return;

    case PROJ_TXREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].txrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].txrepeat);
        return;

    case PROJ_TYREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].tyrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].tyrepeat);
        return;

    case PROJ_TOFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].toffset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].toffset);
        return;

    case PROJ_TNUM:
        if (iSet)
        {
            ProjectileData[lVar1].tnum=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].tnum);
        return;

    case PROJ_DROP:
        if (iSet)
        {
            ProjectileData[lVar1].drop=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].drop);
        return;

    case PROJ_CSTAT:
        if (iSet)
        {
            ProjectileData[lVar1].cstat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].cstat);
        return;

    case PROJ_CLIPDIST:
        if (iSet)
        {
            ProjectileData[lVar1].clipdist=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].clipdist);
        return;

    case PROJ_SHADE:
        if (iSet)
        {
            ProjectileData[lVar1].shade=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].shade);
        return;

    case PROJ_XREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].xrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].xrepeat);
        return;

    case PROJ_YREPEAT:
        if (iSet)
        {
            ProjectileData[lVar1].yrepeat=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].yrepeat);
        return;

    case PROJ_PAL:
        if (iSet)
        {
            ProjectileData[lVar1].pal=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].pal);
        return;

    case PROJ_EXTRA_RAND:
        if (iSet)
        {
            ProjectileData[lVar1].extra_rand=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].extra_rand);
        return;

    case PROJ_HITRADIUS:
        if (iSet)
        {
            ProjectileData[lVar1].hitradius=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].hitradius);
        return;

    case PROJ_MOVECNT:
        if (iSet)
        {
            ProjectileData[lVar1].movecnt=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].movecnt);
        return;

    case PROJ_OFFSET:
        if (iSet)
        {
            ProjectileData[lVar1].offset=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].offset);
        return;

    case PROJ_BOUNCES:
        if (iSet)
        {
            ProjectileData[lVar1].bounces=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].bounces);
        return;

    case PROJ_BSOUND:
        if (iSet)
        {
            ProjectileData[lVar1].bsound=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].bsound);
        return;

    case PROJ_RANGE:
        if (iSet)
        {
            ProjectileData[lVar1].range=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].range);
        return;

    case PROJ_USERDATA:
        if (iSet)
        {
            ProjectileData[lVar1].userdata=lValue;
            return;
        }
        Gv_SetVarX(lVar2, ProjectileData[lVar1].userdata);
        return;

    default:
        return;
    }

badtile:
    CON_ERRPRINTF("invalid tile (%d)\n", lVar1);
    insptr += (lVar2 == MAXGAMEVARS);
    return;
}
#else
static int32_t __fastcall VM_AccessSpriteX(int32_t iActor, int32_t lLabelID, int32_t lParm2)
{
    if (ActorLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[lLabelID].maxParm2)
        goto badpos;

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
    case ACTOR_LOTAG: return (int16_t)sprite[iActor].lotag;
    case ACTOR_HITAG: return (int16_t)sprite[iActor].hitag;
    case ACTOR_ULOTAG: return sprite[iActor].lotag;
    case ACTOR_UHITAG: return sprite[iActor].hitag;
    case ACTOR_EXTRA: return sprite[iActor].extra;
    case ACTOR_HTCGG: return actor[iActor].cgg;
    case ACTOR_HTPICNUM : return actor[iActor].picnum;
    case ACTOR_HTANG: return actor[iActor].ang;
    case ACTOR_HTEXTRA: return actor[iActor].extra;
    case ACTOR_HTOWNER: return actor[iActor].owner;
    case ACTOR_HTMOVFLAG: return actor[iActor].movflag;
    case ACTOR_HTTEMPANG: return actor[iActor].tempang;
    case ACTOR_HTACTORSTAYPUT: return actor[iActor].actorstayput;
    case ACTOR_HTDISPICNUM: return actor[iActor].dispicnum;
    case ACTOR_HTTIMETOSLEEP: return actor[iActor].timetosleep;
    case ACTOR_HTFLOORZ: return actor[iActor].floorz;
    case ACTOR_HTCEILINGZ: return actor[iActor].ceilingz;
    case ACTOR_HTLASTVX: return actor[iActor].lastvx;
    case ACTOR_HTLASTVY: return actor[iActor].lastvy;
    case ACTOR_HTBPOSX: return actor[iActor].bpos.x;
    case ACTOR_HTBPOSY: return actor[iActor].bpos.y;
    case ACTOR_HTBPOSZ: return actor[iActor].bpos.z;
    case ACTOR_HTG_T: return actor[iActor].t_data[lParm2];
    case ACTOR_ANGOFF: return spriteext[iActor].angoff;
    case ACTOR_PITCH: return spriteext[iActor].pitch;
    case ACTOR_ROLL: return spriteext[iActor].roll;
    case ACTOR_MDXOFF: return spriteext[iActor].xoff;
    case ACTOR_MDYOFF: return spriteext[iActor].yoff;
    case ACTOR_MDZOFF: return spriteext[iActor].zoff;
    case ACTOR_MDFLAGS: return spriteext[iActor].flags;
    case ACTOR_XPANNING: return spriteext[iActor].xpanning;
    case ACTOR_YPANNING: return spriteext[iActor].ypanning;
    case ACTOR_HTFLAGS: return actor[iActor].flags;
    case ACTOR_ALPHA: return (uint8_t)(spriteext[iActor].alpha*255.0f);
    default: return -1;
    }

badpos:
    CON_ERRPRINTF("tried to get invalid %s position %d on sprite (%d) from spr %d\n",
                  ActorLabels[lLabelID].name,lParm2,iActor,vm.g_i);
    return -1;
}

static int32_t __fastcall VM_AccessSectorX(int32_t iSector, int32_t lLabelID)
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
    case SECTOR_LOTAG: return (int16_t)sector[iSector].lotag;
    case SECTOR_HITAG: return (int16_t)sector[iSector].hitag;
    case SECTOR_ULOTAG: return sector[iSector].lotag;
    case SECTOR_UHITAG: return sector[iSector].hitag;
    case SECTOR_EXTRA: return sector[iSector].extra;
    case SECTOR_CEILINGBUNCH:
    case SECTOR_FLOORBUNCH:
#ifdef YAX_ENABLE
        return yax_getbunch(iSector, lLabelID==SECTOR_FLOORBUNCH);
#else
        return -1;
#endif
    default: return -1;
    }
}

static int32_t __fastcall VM_AccessPlayerX(int32_t iPlayer, int32_t lLabelID, int32_t lParm2)
{
    DukePlayer_t *const ps = g_player[iPlayer].ps;

    if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)PlayerLabels[lLabelID].maxParm2)
        goto badpos;

    switch (lLabelID)
    {
    case PLAYER_ZOOM:
        return ps->zoom;
    case PLAYER_EXITX:
        return ps->exitx;
    case PLAYER_EXITY:
        return ps->exity;
    case PLAYER_LOOGIEX:
        return ps->loogiex[lParm2];
    case PLAYER_LOOGIEY:
        return ps->loogiey[lParm2];
    case PLAYER_NUMLOOGS:
        return ps->numloogs;
    case PLAYER_LOOGCNT:
        return ps->loogcnt;
    case PLAYER_POSX:
        return ps->pos.x;
    case PLAYER_POSY:
        return ps->pos.y;
    case PLAYER_POSZ:
        return ps->pos.z;
    case PLAYER_HORIZ:
        return ps->horiz;
    case PLAYER_OHORIZ:
        return ps->ohoriz;
    case PLAYER_OHORIZOFF:
        return ps->ohorizoff;
    case PLAYER_INVDISPTIME:
        return ps->invdisptime;
    case PLAYER_BOBPOSX:
        return ps->bobposx;
    case PLAYER_BOBPOSY:
        return ps->bobposy;
    case PLAYER_OPOSX:
        return ps->opos.x;
    case PLAYER_OPOSY:
        return ps->opos.y;
    case PLAYER_OPOSZ:
        return ps->opos.z;
    case PLAYER_PYOFF:
        return ps->pyoff;
    case PLAYER_OPYOFF:
        return ps->opyoff;
    case PLAYER_POSXV:
        return ps->vel.x;
    case PLAYER_POSYV:
        return ps->vel.y;
    case PLAYER_POSZV:
        return ps->vel.z;
    case PLAYER_LAST_PISSED_TIME:
        return ps->last_pissed_time;
    case PLAYER_TRUEFZ:
        return ps->truefz;
    case PLAYER_TRUECZ:
        return ps->truecz;
    case PLAYER_PLAYER_PAR:
        return ps->player_par;
    case PLAYER_VISIBILITY:
        return ps->visibility;
    case PLAYER_BOBCOUNTER:
        return ps->bobcounter;
    case PLAYER_WEAPON_SWAY:
        return ps->weapon_sway;
    case PLAYER_PALS_TIME:
        return ps->pals.f;
    case PLAYER_RANDOMFLAMEX:
        return ps->randomflamex;
    case PLAYER_CRACK_TIME:
        return ps->crack_time;
    case PLAYER_AIM_MODE:
        return ps->aim_mode;
    case PLAYER_ANG:
        return ps->ang;
    case PLAYER_OANG:
        return ps->oang;
    case PLAYER_ANGVEL:
        return ps->angvel;
    case PLAYER_CURSECTNUM:
        return ps->cursectnum;
    case PLAYER_LOOK_ANG:
        return ps->look_ang;
    case PLAYER_LAST_EXTRA:
        return ps->last_extra;
    case PLAYER_SUBWEAPON:
        return ps->subweapon;
    case PLAYER_AMMO_AMOUNT:
        return ps->ammo_amount[lParm2];
    case PLAYER_WACKEDBYACTOR:
        return ps->wackedbyactor;
    case PLAYER_FRAG:
        return ps->frag;
    case PLAYER_FRAGGEDSELF:
        return ps->fraggedself;
    case PLAYER_CURR_WEAPON:
        return ps->curr_weapon;
    case PLAYER_LAST_WEAPON:
        return ps->last_weapon;
    case PLAYER_TIPINCS:
        return ps->tipincs;
    case PLAYER_HORIZOFF:
        return ps->horizoff;
    case PLAYER_WANTWEAPONFIRE:
        return ps->wantweaponfire;
    case PLAYER_HOLODUKE_AMOUNT:
        return ps->inv_amount[GET_HOLODUKE];
    case PLAYER_NEWOWNER:
        return ps->newowner;
    case PLAYER_HURT_DELAY:
        return ps->hurt_delay;
    case PLAYER_HBOMB_HOLD_DELAY:
        return ps->hbomb_hold_delay;
    case PLAYER_JUMPING_COUNTER:
        return ps->jumping_counter;
    case PLAYER_AIRLEFT:
        return ps->airleft;
    case PLAYER_KNEE_INCS:
        return ps->knee_incs;
    case PLAYER_ACCESS_INCS:
        return ps->access_incs;
    case PLAYER_FTA:
        return ps->fta;
    case PLAYER_FTQ:
        return ps->ftq;
    case PLAYER_ACCESS_WALLNUM:
        return ps->access_wallnum;
    case PLAYER_ACCESS_SPRITENUM:
        return ps->access_spritenum;
    case PLAYER_KICKBACK_PIC:
        return ps->kickback_pic;
    case PLAYER_GOT_ACCESS:
        return ps->got_access;
    case PLAYER_WEAPON_ANG:
        return ps->weapon_ang;
    case PLAYER_FIRSTAID_AMOUNT:
        return ps->inv_amount[GET_FIRSTAID];
    case PLAYER_SOMETHINGONPLAYER:
        return ps->somethingonplayer;
    case PLAYER_ON_CRANE:
        return ps->on_crane;
    case PLAYER_I:
        return ps->i;
    case PLAYER_ONE_PARALLAX_SECTNUM:
        return ps->one_parallax_sectnum;
    case PLAYER_OVER_SHOULDER_ON:
        return ps->over_shoulder_on;
    case PLAYER_RANDOM_CLUB_FRAME:
        return ps->random_club_frame;
    case PLAYER_FIST_INCS:
        return ps->fist_incs;
    case PLAYER_ONE_EIGHTY_COUNT:
        return ps->one_eighty_count;
    case PLAYER_CHEAT_PHASE:
        return ps->cheat_phase;
    case PLAYER_DUMMYPLAYERSPRITE:
        return ps->dummyplayersprite;
    case PLAYER_EXTRA_EXTRA8:
        return ps->extra_extra8;
    case PLAYER_QUICK_KICK:
        return ps->quick_kick;
    case PLAYER_HEAT_AMOUNT:
        return ps->inv_amount[GET_HEATS];
    case PLAYER_ACTORSQU:
        return ps->actorsqu;
    case PLAYER_TIMEBEFOREEXIT:
        return ps->timebeforeexit;
    case PLAYER_CUSTOMEXITSOUND:
        return ps->customexitsound;
    case PLAYER_WEAPRECS:
        return ps->weaprecs[lParm2];
    case PLAYER_WEAPRECCNT:
        return ps->weapreccnt;
    case PLAYER_INTERFACE_TOGGLE_FLAG:
        return ps->interface_toggle_flag;
    case PLAYER_ROTSCRNANG:
        return ps->rotscrnang;
    case PLAYER_DEAD_FLAG:
        return ps->dead_flag;
    case PLAYER_SHOW_EMPTY_WEAPON:
        return ps->show_empty_weapon;
    case PLAYER_SCUBA_AMOUNT:
        return ps->inv_amount[GET_SCUBA];
    case PLAYER_JETPACK_AMOUNT:
        return ps->inv_amount[GET_JETPACK];
    case PLAYER_STEROIDS_AMOUNT:
        return ps->inv_amount[GET_STEROIDS];
    case PLAYER_SHIELD_AMOUNT:
        return ps->inv_amount[GET_SHIELD];
    case PLAYER_HOLODUKE_ON:
        return ps->holoduke_on;
    case PLAYER_PYCOUNT:
        return ps->pycount;
    case PLAYER_WEAPON_POS:
        return ps->weapon_pos;
    case PLAYER_FRAG_PS:
        return ps->frag_ps;
    case PLAYER_TRANSPORTER_HOLD:
        return ps->transporter_hold;
    case PLAYER_LAST_FULL_WEAPON:
        return ps->last_full_weapon;
    case PLAYER_FOOTPRINTSHADE:
        return ps->footprintshade;
    case PLAYER_BOOT_AMOUNT:
        return ps->inv_amount[GET_BOOTS];
    case PLAYER_SCREAM_VOICE:
        return ps->scream_voice;
    case PLAYER_GM:
        return ps->gm;
    case PLAYER_ON_WARPING_SECTOR:
        return ps->on_warping_sector;
    case PLAYER_FOOTPRINTCOUNT:
        return ps->footprintcount;
    case PLAYER_HBOMB_ON:
        return ps->hbomb_on;
    case PLAYER_JUMPING_TOGGLE:
        return ps->jumping_toggle;
    case PLAYER_RAPID_FIRE_HOLD:
        return ps->rapid_fire_hold;
    case PLAYER_ON_GROUND:
        return ps->on_ground;
    case PLAYER_INVEN_ICON:
        return ps->inven_icon;
    case PLAYER_BUTTONPALETTE:
        return ps->buttonpalette;
    case PLAYER_JETPACK_ON:
        return ps->jetpack_on;
    case PLAYER_SPRITEBRIDGE:
        return ps->spritebridge;
    case PLAYER_LASTRANDOMSPOT:
        return ps->lastrandomspot;
    case PLAYER_SCUBA_ON:
        return ps->scuba_on;
    case PLAYER_FOOTPRINTPAL:
        return ps->footprintpal;
    case PLAYER_HEAT_ON:
        return ps->heat_on;
    case PLAYER_HOLSTER_WEAPON:
        return ps->holster_weapon;
    case PLAYER_FALLING_COUNTER:
        return ps->falling_counter;
    case PLAYER_GOTWEAPON:
        return (ps->gotweapon & (1<<lParm2)) != 0;
    case PLAYER_REFRESH_INVENTORY:
        return ps->refresh_inventory;
    case PLAYER_TOGGLE_KEY_FLAG:
        return ps->toggle_key_flag;
    case PLAYER_KNUCKLE_INCS:
        return ps->knuckle_incs;
    case PLAYER_WALKING_SND_TOGGLE:
        return ps->walking_snd_toggle;
    case PLAYER_PALOOKUP:
        return ps->palookup;
    case PLAYER_HARD_LANDING:
        return ps->hard_landing;
    case PLAYER_MAX_SECRET_ROOMS:
        return ps->max_secret_rooms;
    case PLAYER_SECRET_ROOMS:
        return ps->secret_rooms;
    case PLAYER_PALS:
        switch (lParm2)
        {
        case 0:
            return ps->pals.r;
        case 1:
            return ps->pals.g;
        case 2:
            return ps->pals.b;
        }
        return -1;
    case PLAYER_MAX_ACTORS_KILLED:
        return ps->max_actors_killed;
    case PLAYER_ACTORS_KILLED:
        return ps->actors_killed;
    case PLAYER_RETURN_TO_CENTER:
        return ps->return_to_center;
    case PLAYER_RUNSPEED:
        return ps->runspeed;
    case PLAYER_SBS:
        return ps->sbs;
    case PLAYER_RELOADING:
        return ps->reloading;
    case PLAYER_AUTO_AIM:
        return ps->auto_aim;
    case PLAYER_MOVEMENT_LOCK:
        return ps->movement_lock;
    case PLAYER_SOUND_PITCH:
        return ps->sound_pitch;
    case PLAYER_WEAPONSWITCH:
        return ps->weaponswitch;
    case PLAYER_TEAM:
        return ps->team;
    case PLAYER_MAX_PLAYER_HEALTH:
        return ps->max_player_health;
    case PLAYER_MAX_SHIELD_AMOUNT:
        return ps->max_shield_amount;
    case PLAYER_MAX_AMMO_AMOUNT:
        return ps->max_ammo_amount[lParm2];
    case PLAYER_LAST_QUICK_KICK:
        return ps->last_quick_kick;
    case PLAYER_AUTOSTEP:
        return ps->autostep;
    case PLAYER_AUTOSTEP_SBW:
        return ps->autostep_sbw;
    default:
        return -1;
    }

badpos:
    CON_ERRPRINTF("tried to get invalid %s position %d on player (%d) from spr %d\n",
                  PlayerLabels[lLabelID].name,lParm2,iPlayer,vm.g_i);
    return -1;
}

static int32_t __fastcall VM_AccessWallX(int32_t iWall, int32_t lLabelID)
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
        return (int16_t)wall[iWall].lotag;
    case WALL_HITAG:
        return (int16_t)wall[iWall].hitag;
    case WALL_ULOTAG:
        return wall[iWall].lotag;
    case WALL_UHITAG:
        return wall[iWall].hitag;
    case WALL_EXTRA:
        return wall[iWall].extra;
    default:
        return -1;
    }
}
#endif // _gamevars_c_
