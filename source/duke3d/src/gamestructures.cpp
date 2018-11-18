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

// this is all the crap for accessing the game's structs through the CON VM
// I got a 3-4 fps gain by inlining these...

#ifndef gamevars_c_
int32_t __fastcall VM_GetUserdef(int32_t labelNum, int const lParm2);
void __fastcall VM_SetUserdef(int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetActiveProjectile(int const spriteNum, int32_t labelNum);
void __fastcall VM_SetActiveProjectile(int const spriteNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPlayer(int const playerNum, int32_t labelNum, int const lParm2);
void __fastcall VM_SetPlayer(int const playerNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetPlayerInput(int const playerNum, int32_t labelNum);
void __fastcall VM_SetPlayerInput(int const playerNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetWall(int const wallNum, int32_t labelNum);
void __fastcall VM_SetWall(int const wallNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetSector(int const sectNum, int labelNum);
void __fastcall VM_SetSector(int const sectNum, int const labelNum, int32_t newValue);
int32_t __fastcall VM_GetSprite(int const spriteNum, int32_t labelNum, int const lParm2);
void __fastcall VM_SetSprite(int const spriteNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetTsprite(int const spriteNum, int32_t labelNum);
void __fastcall VM_SetTsprite(int const spriteNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetProjectile(int const tileNum, int32_t labelNum);
void __fastcall VM_SetProjectile(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetTileData(int const tileNum, int32_t labelNum);
void __fastcall VM_SetTileData(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPalData(int const palNum, int32_t labelNum);
#else
int32_t __fastcall VM_GetUserdef(int32_t labelNum, int const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(UserdefsLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) UserdefsLabels[labelNum].maxParm2))
    {
        CON_ERRPRINTF("%s[%d] invalid for userdef", UserdefsLabels[labelNum].name, lParm2);
        return -1;
    }

    switch (labelNum)
    {
        case USERDEFS_GOD:                    labelNum = ud.god;                          break;
        case USERDEFS_WARP_ON:                labelNum = ud.warp_on;                      break;
        case USERDEFS_CASHMAN:                labelNum = ud.cashman;                      break;
        case USERDEFS_EOG:                    labelNum = ud.eog;                          break;
        case USERDEFS_SHOWALLMAP:             labelNum = ud.showallmap;                   break;
        case USERDEFS_SHOW_HELP:              labelNum = ud.show_help;                    break;
        case USERDEFS_SCROLLMODE:             labelNum = ud.scrollmode;                   break;
        case USERDEFS_CLIPPING:               labelNum = ud.noclip;                       break;
        //  case USERDEFS_USER_NAME:          labelNum = ud.user_name[MAXPLAYERS][32];    break;
        //  case USERDEFS_RIDECULE:           labelNum = ud.ridecule;                     break;
        //  case USERDEFS_PWLOCKOUT:          labelNum = ud.pwlockout;                    break;
        //  case USERDEFS_RTSNAME:            labelNum = ud.rtsname;                      break;
        case USERDEFS_OVERHEAD_ON:            labelNum = ud.overhead_on;                  break;
        case USERDEFS_LAST_OVERHEAD:          labelNum = ud.last_overhead;                break;
        case USERDEFS_SHOWWEAPONS:            labelNum = ud.showweapons;                  break;
        case USERDEFS_PAUSE_ON:               labelNum = ud.pause_on;                     break;
        case USERDEFS_FROM_BONUS:             labelNum = ud.from_bonus;                   break;
        case USERDEFS_CAMERASPRITE:           labelNum = ud.camerasprite;                 break;
        case USERDEFS_LAST_CAMSPRITE:         labelNum = ud.last_camsprite;               break;
        case USERDEFS_LAST_LEVEL:             labelNum = ud.last_level;                   break;
        case USERDEFS_SECRETLEVEL:            labelNum = ud.secretlevel;                  break;
        case USERDEFS_CONST_VISIBILITY:       labelNum = ud.const_visibility;             break;
        case USERDEFS_UW_FRAMERATE:           labelNum = ud.uw_framerate;                 break;
        case USERDEFS_CAMERA_TIME:            labelNum = ud.camera_time;                  break;
        case USERDEFS_FOLFVEL:                labelNum = ud.folfvel;                      break;
        case USERDEFS_FOLAVEL:                labelNum = ud.folavel;                      break;
        case USERDEFS_FOLX:                   labelNum = ud.folx;                         break;
        case USERDEFS_FOLY:                   labelNum = ud.foly;                         break;
        case USERDEFS_FOLA:                   labelNum = ud.fola;                         break;
        case USERDEFS_RECCNT:                 labelNum = ud.reccnt;                       break;
        case USERDEFS_ENTERED_NAME:           labelNum = ud.entered_name;                 break;
        case USERDEFS_SCREEN_TILTING:         labelNum = ud.screen_tilting;               break;
        case USERDEFS_SHADOWS:                labelNum = ud.shadows;                      break;
        case USERDEFS_FTA_ON:                 labelNum = ud.fta_on;                       break;
        case USERDEFS_EXECUTIONS:             labelNum = ud.executions;                   break;
        case USERDEFS_AUTO_RUN:               labelNum = ud.auto_run;                     break;
        case USERDEFS_COORDS:                 labelNum = ud.coords;                       break;
        case USERDEFS_TICKRATE:               labelNum = ud.showfps;                      break;
        case USERDEFS_M_COOP:                 labelNum = ud.m_coop;                       break;
        case USERDEFS_COOP:                   labelNum = ud.coop;                         break;
        case USERDEFS_SCREEN_SIZE:            labelNum = ud.screen_size;                  break;
        case USERDEFS_LOCKOUT:                labelNum = ud.lockout;                      break;
        case USERDEFS_CROSSHAIR:              labelNum = ud.crosshair;                    break;
        case USERDEFS_PLAYERAI:               labelNum = ud.playerai;                     break;
        case USERDEFS_RESPAWN_MONSTERS:       labelNum = ud.respawn_monsters;             break;
        case USERDEFS_RESPAWN_ITEMS:          labelNum = ud.respawn_items;                break;
        case USERDEFS_RESPAWN_INVENTORY:      labelNum = ud.respawn_inventory;            break;
        case USERDEFS_RECSTAT:                labelNum = ud.recstat;                      break;
        case USERDEFS_MONSTERS_OFF:           labelNum = ud.monsters_off;                 break;
        case USERDEFS_BRIGHTNESS:             labelNum = ud.brightness;                   break;
        case USERDEFS_M_RESPAWN_ITEMS:        labelNum = ud.m_respawn_items;              break;
        case USERDEFS_M_RESPAWN_MONSTERS:     labelNum = ud.m_respawn_monsters;           break;
        case USERDEFS_M_RESPAWN_INVENTORY:    labelNum = ud.m_respawn_inventory;          break;
        case USERDEFS_M_RECSTAT:              labelNum = ud.m_recstat;                    break;
        case USERDEFS_M_MONSTERS_OFF:         labelNum = ud.m_monsters_off;               break;
        case USERDEFS_DETAIL:                 labelNum = ud.detail;                       break;
        case USERDEFS_M_FFIRE:                labelNum = ud.m_ffire;                      break;
        case USERDEFS_FFIRE:                  labelNum = ud.ffire;                        break;
        case USERDEFS_M_PLAYER_SKILL:         labelNum = ud.m_player_skill;               break;
        case USERDEFS_M_LEVEL_NUMBER:         labelNum = ud.m_level_number;               break;
        case USERDEFS_M_VOLUME_NUMBER:        labelNum = ud.m_volume_number;              break;
        case USERDEFS_M_USER_MAP:             labelNum = Menu_HaveUserMap();              break;
        case USERDEFS_MULTIMODE:              labelNum = ud.multimode;                    break;
        case USERDEFS_PLAYER_SKILL:           labelNum = ud.player_skill;                 break;
        case USERDEFS_LEVEL_NUMBER:           labelNum = ud.level_number;                 break;
        case USERDEFS_VOLUME_NUMBER:          labelNum = ud.volume_number;                break;
        case USERDEFS_USER_MAP:               labelNum = G_HaveUserMap();                 break;
        case USERDEFS_M_MARKER:               labelNum = ud.m_marker;                     break;
        case USERDEFS_MARKER:                 labelNum = ud.marker;                       break;
        case USERDEFS_MOUSEFLIP:              labelNum = ud.mouseflip;                    break;
        case USERDEFS_STATUSBARSCALE:         labelNum = ud.statusbarscale;               break;
        case USERDEFS_DRAWWEAPON:             labelNum = ud.drawweapon;                   break;
        case USERDEFS_MOUSEAIMING:            labelNum = ud.mouseaiming;                  break;
        case USERDEFS_WEAPONSWITCH:           labelNum = ud.weaponswitch;                 break;
        case USERDEFS_DEMOCAMS:               labelNum = ud.democams;                     break;
        case USERDEFS_COLOR:                  labelNum = ud.color;                        break;
        case USERDEFS_MSGDISPTIME:            labelNum = ud.msgdisptime;                  break;
        case USERDEFS_STATUSBARMODE:          labelNum = ud.statusbarmode;                break;
        case USERDEFS_M_NOEXITS:              labelNum = ud.m_noexits;                    break;
        case USERDEFS_NOEXITS:                labelNum = ud.noexits;                      break;
        case USERDEFS_AUTOVOTE:               labelNum = ud.autovote;                     break;
        case USERDEFS_AUTOMSG:                labelNum = ud.automsg;                      break;
        case USERDEFS_IDPLAYERS:              labelNum = ud.idplayers;                    break;
        case USERDEFS_TEAM:                   labelNum = ud.team;                         break;
        case USERDEFS_VIEWBOB:                labelNum = ud.viewbob;                      break;
        case USERDEFS_WEAPONSWAY:             labelNum = ud.weaponsway;                   break;
        case USERDEFS_ANGLEINTERPOLATION:     labelNum = ud.angleinterpolation;           break;
        case USERDEFS_OBITUARIES:             labelNum = ud.obituaries;                   break;
        case USERDEFS_LEVELSTATS:             labelNum = ud.levelstats;                   break;
        case USERDEFS_CROSSHAIRSCALE:         labelNum = ud.crosshairscale;               break;
        case USERDEFS_ALTHUD:                 labelNum = ud.althud;                       break;
        case USERDEFS_DISPLAY_BONUS_SCREEN:   labelNum = ud.display_bonus_screen;         break;
        case USERDEFS_SHOW_LEVEL_TEXT:        labelNum = ud.show_level_text;              break;
        case USERDEFS_WEAPONSCALE:            labelNum = ud.weaponscale;                  break;
        case USERDEFS_TEXTSCALE:              labelNum = ud.textscale;                    break;
        case USERDEFS_RUNKEY_MODE:            labelNum = ud.runkey_mode;                  break;
        case USERDEFS_M_ORIGIN_X:             labelNum = ud.returnvar[0];                 break;
        case USERDEFS_M_ORIGIN_Y:             labelNum = ud.returnvar[1];                 break;
        case USERDEFS_PLAYERBEST:             labelNum = ud.playerbest;                   break;
        case USERDEFS_MUSICTOGGLE:            labelNum = ud.config.MusicToggle;           break;
        case USERDEFS_USEVOXELS:              labelNum = usevoxels;                       break;
        case USERDEFS_USEHIGHTILE:
#ifdef USE_OPENGL
                                              labelNum = usehightile;                     break;
#endif
        case USERDEFS_USEMODELS:
#ifdef USE_OPENGL
                                              labelNum = usemodels;                       break;
#else
                                              labelNum = 0;                               break;
#endif
        case USERDEFS_GAMETYPEFLAGS:          labelNum = g_gametypeFlags[ud.coop];        break;
        case USERDEFS_M_GAMETYPEFLAGS:        labelNum = g_gametypeFlags[ud.m_coop];      break;
        case USERDEFS_GLOBALFLAGS:            labelNum = globalflags;                     break;
        case USERDEFS_GLOBALGAMEFLAGS:        labelNum = duke3d_globalflags;              break;
        case USERDEFS_VM_PLAYER:              labelNum = vm.playerNum;                    break;
        case USERDEFS_VM_SPRITE:              labelNum = vm.spriteNum;                    break;
        case USERDEFS_VM_DISTANCE:            labelNum = vm.playerDist;                   break;
        case USERDEFS_SOUNDTOGGLE:            labelNum = ud.config.SoundToggle;           break;
        case USERDEFS_GAMETEXT_TRACKING:      labelNum = MF_Bluefont.between.x;           break;
        case USERDEFS_MENUTEXT_TRACKING:      labelNum = MF_Redfont.between.x;            break;
        case USERDEFS_MAXSPRITESONSCREEN:     labelNum = maxspritesonscreen;              break;
        case USERDEFS_SCREENAREA_X1:          labelNum = aGameVars[g_returnVarID].global; break;
        case USERDEFS_SCREENAREA_Y1:          labelNum = ud.returnvar[0];                 break;
        case USERDEFS_SCREENAREA_X2:          labelNum = ud.returnvar[1];                 break;
        case USERDEFS_SCREENAREA_Y2:          labelNum = ud.returnvar[2];                 break;
        case USERDEFS_SCREENFADE:             labelNum = ud.screenfade;                   break;
        case USERDEFS_MENUBACKGROUND:         labelNum = ud.menubackground;               break;
        case USERDEFS_STATUSBARFLAGS:         labelNum = ud.statusbarflags;               break;
        case USERDEFS_STATUSBARRANGE:         labelNum = ud.statusbarrange;               break;
        case USERDEFS_STATUSBARCUSTOM:        labelNum = ud.statusbarcustom;              break;
        case USERDEFS_HUDONTOP:               labelNum = ud.hudontop;                     break;
        case USERDEFS_MENU_SLIDEBARZ:         labelNum = ud.menu_slidebarz;               break;
        case USERDEFS_MENU_SLIDEBARMARGIN:    labelNum = ud.menu_slidebarmargin;          break;
        case USERDEFS_MENU_SLIDECURSORZ:      labelNum = ud.menu_slidecursorz;            break;
        case USERDEFS_GLOBAL_R:               labelNum = globalr;                         break;
        case USERDEFS_GLOBAL_G:               labelNum = globalg;                         break;
        case USERDEFS_GLOBAL_B:               labelNum = globalb;                         break;
        case USERDEFS_DEFAULT_VOLUME:         labelNum = ud.default_volume;               break;
        case USERDEFS_DEFAULT_SKILL:          labelNum = ud.default_skill;                break;
        case USERDEFS_MENU_SHADEDESELECTED:   labelNum = MF_Redfont.shade_deselected;     break;
        case USERDEFS_MENU_SHADEDISABLED:     labelNum = MF_Redfont.shade_disabled;       break;
        case USERDEFS_MENUTEXT_ZOOM:          labelNum = MF_Redfont.zoom;                 break;
        case USERDEFS_MENUTEXT_XSPACE:        labelNum = MF_Redfont.emptychar.x;          break;
        case USERDEFS_MENUTEXT_PALSELECTED:   labelNum = MF_Redfont.pal_selected;         break;
        case USERDEFS_MENUTEXT_PALDESELECTED: labelNum = MF_Redfont.pal_deselected;       break;
        case USERDEFS_MENUTEXT_PALDISABLED:   labelNum = MF_Redfont.pal_disabled;         break;
        case USERDEFS_GAMETEXT_ZOOM:          labelNum = MF_Bluefont.zoom;                break;
        case USERDEFS_GAMETEXT_XSPACE:        labelNum = MF_Bluefont.emptychar.x;         break;
        case USERDEFS_GAMETEXT_PALSELECTED:   labelNum = MF_Bluefont.pal_selected;        break;
        case USERDEFS_GAMETEXT_PALDESELECTED: labelNum = MF_Bluefont.pal_deselected;      break;
        case USERDEFS_GAMETEXT_PALDISABLED:   labelNum = MF_Bluefont.pal_disabled;        break;
        case USERDEFS_MINITEXT_ZOOM:          labelNum = MF_Minifont.zoom;                break;
        case USERDEFS_MINITEXT_XSPACE:        labelNum = MF_Minifont.emptychar.x;         break;
        case USERDEFS_MINITEXT_TRACKING:      labelNum = MF_Minifont.between.x;           break;
        case USERDEFS_MINITEXT_PALSELECTED:   labelNum = MF_Minifont.pal_selected;        break;
        case USERDEFS_MINITEXT_PALDESELECTED: labelNum = MF_Minifont.pal_deselected;      break;
        case USERDEFS_MINITEXT_PALDISABLED:   labelNum = MF_Minifont.pal_disabled;        break;
        case USERDEFS_MENUTITLE_PAL:          labelNum = ud.menutitle_pal;                break;
        case USERDEFS_SLIDEBAR_PALSELECTED:   labelNum = ud.slidebar_palselected;         break;
        case USERDEFS_SLIDEBAR_PALDISABLED:   labelNum = ud.slidebar_paldisabled;         break;
        case USERDEFS_MUSIC_EPISODE:          labelNum = ud.music_episode;                break;
        case USERDEFS_MUSIC_LEVEL:            labelNum = ud.music_level;                  break;
        case USERDEFS_SHADOW_PAL:             labelNum = ud.shadow_pal;                   break;
        case USERDEFS_MENU_SCROLLBARTILENUM:  labelNum = ud.menu_scrollbartilenum;        break;
        case USERDEFS_MENU_SCROLLBARZ:        labelNum = ud.menu_scrollbarz;              break;
        case USERDEFS_MENU_SCROLLCURSORZ:     labelNum = ud.menu_scrollcursorz;           break;
        case USERDEFS_RETURN:
            if (lParm2 == 0)
                labelNum = aGameVars[g_returnVarID].global;
            else
                labelNum = ud.returnvar[lParm2 - 1];
            break;
        case USERDEFS_USERBYTEVERSION:        labelNum = ud.userbytever;                  break;
        case USERDEFS_AUTOSAVE:               labelNum = ud.autosave;                     break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetUserdef(int const labelNum, int const lParm2, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE(UserdefsLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)UserdefsLabels[labelNum].maxParm2))
    {
        CON_ERRPRINTF("%s[%d] invalid for userdef", UserdefsLabels[labelNum].name, lParm2);
        return;
    }

    switch (labelNum)
    {
        case USERDEFS_GOD:                          ud.god                           = iSet; break;
        case USERDEFS_WARP_ON:                      ud.warp_on                       = iSet; break;
        case USERDEFS_CASHMAN:                      ud.cashman                       = iSet; break;
        case USERDEFS_EOG:                          ud.eog                           = iSet; break;
        case USERDEFS_SHOWALLMAP:                   ud.showallmap                    = iSet; break;
        case USERDEFS_SHOW_HELP:                    ud.show_help                     = iSet; break;
        case USERDEFS_SCROLLMODE:                   ud.scrollmode                    = iSet; break;
        case USERDEFS_CLIPPING:                     ud.noclip                        = iSet; break;
        //  case USERDEFS_USER_NAME:                ud.user_name[MAXPLAYERS][32]     = lValue; break;
        //  case USERDEFS_RIDECULE:                 ud.ridecule                      = lValue; break;
        //  case USERDEFS_PWLOCKOUT:                ud.pwlockout                     = lValue; break;
        //  case USERDEFS_RTSNAME:                  ud.rtsname                       = lValue; break;
        case USERDEFS_OVERHEAD_ON:                  ud.overhead_on                   = iSet; break;
        case USERDEFS_LAST_OVERHEAD:                ud.last_overhead                 = iSet; break;
        case USERDEFS_SHOWWEAPONS:                  ud.showweapons                   = iSet; break;
        case USERDEFS_PAUSE_ON:                     ud.pause_on                      = iSet; break;
        case USERDEFS_FROM_BONUS:                   ud.from_bonus                    = iSet; break;
        case USERDEFS_CAMERASPRITE:                 ud.camerasprite                  = iSet; break;
        case USERDEFS_LAST_CAMSPRITE:               ud.last_camsprite                = iSet; break;
        case USERDEFS_LAST_LEVEL:                   ud.last_level                    = iSet; break;
        case USERDEFS_SECRETLEVEL:                  ud.secretlevel                   = iSet; break;
        case USERDEFS_CONST_VISIBILITY:             ud.const_visibility              = iSet; break;
        case USERDEFS_UW_FRAMERATE:                 ud.uw_framerate                  = iSet; break;
        case USERDEFS_CAMERA_TIME:                  ud.camera_time                   = iSet; break;
        case USERDEFS_FOLFVEL:                      ud.folfvel                       = iSet; break;
        case USERDEFS_FOLAVEL:                      ud.folavel                       = iSet; break;
        case USERDEFS_FOLX:                         ud.folx                          = iSet; break;
        case USERDEFS_FOLY:                         ud.foly                          = iSet; break;
        case USERDEFS_FOLA:                         ud.fola                          = iSet; break;
        case USERDEFS_RECCNT:                       ud.reccnt                        = iSet; break;
        case USERDEFS_ENTERED_NAME:                 ud.entered_name                  = iSet; break;
        case USERDEFS_SCREEN_TILTING:               ud.screen_tilting                = iSet; break;
        case USERDEFS_SHADOWS:                      ud.shadows                       = iSet; break;
        case USERDEFS_FTA_ON:                       ud.fta_on                        = iSet; break;
        case USERDEFS_EXECUTIONS:                   ud.executions                    = iSet; break;
        case USERDEFS_AUTO_RUN:                     ud.auto_run                      = iSet; break;
        case USERDEFS_COORDS:                       ud.coords                        = iSet; break;
        case USERDEFS_TICKRATE:                     ud.showfps                       = iSet; break;
        case USERDEFS_M_COOP:                       ud.m_coop                        = iSet; break;
        case USERDEFS_COOP:                         ud.coop                          = iSet; break;
        case USERDEFS_SCREEN_SIZE:
            if (ud.screen_size != iSet)
            {
                ud.screen_size = iSet;
                G_UpdateScreenArea();
            }
            break;
        case USERDEFS_LOCKOUT:                      ud.lockout                       = iSet; break;
        case USERDEFS_CROSSHAIR:                    ud.crosshair                     = iSet; break;
        case USERDEFS_PLAYERAI:                     ud.playerai                      = iSet; break;
        case USERDEFS_RESPAWN_MONSTERS:             ud.respawn_monsters              = iSet; break;
        case USERDEFS_RESPAWN_ITEMS:                ud.respawn_items                 = iSet; break;
        case USERDEFS_RESPAWN_INVENTORY:            ud.respawn_inventory             = iSet; break;
        case USERDEFS_RECSTAT:                      ud.recstat                       = iSet; break;
        case USERDEFS_MONSTERS_OFF:                 ud.monsters_off                  = iSet; break;
        case USERDEFS_BRIGHTNESS:                   ud.brightness                    = iSet; break;
        case USERDEFS_M_RESPAWN_ITEMS:              ud.m_respawn_items               = iSet; break;
        case USERDEFS_M_RESPAWN_MONSTERS:           ud.m_respawn_monsters            = iSet; break;
        case USERDEFS_M_RESPAWN_INVENTORY:          ud.m_respawn_inventory           = iSet; break;
        case USERDEFS_M_RECSTAT:                    ud.m_recstat                     = iSet; break;
        case USERDEFS_M_MONSTERS_OFF:               ud.m_monsters_off                = iSet; break;
        // REMINDER: must implement "boolean" setters like "!!iSet" in Lunatic, too.
        case USERDEFS_DETAIL:                       ud.detail                        = clamp(iSet, 1, 16); break;
        case USERDEFS_M_FFIRE:                      ud.m_ffire                       = iSet; break;
        case USERDEFS_FFIRE:                        ud.ffire                         = iSet; break;
        case USERDEFS_M_PLAYER_SKILL:               ud.m_player_skill                = iSet; break;
        case USERDEFS_M_LEVEL_NUMBER:               ud.m_level_number                = iSet; break;
        case USERDEFS_M_VOLUME_NUMBER:              ud.m_volume_number               = iSet; break;
        case USERDEFS_MULTIMODE:                    ud.multimode                     = iSet; break;
        case USERDEFS_PLAYER_SKILL:                 ud.player_skill                  = iSet; break;
        case USERDEFS_LEVEL_NUMBER:                 ud.level_number                  = iSet; break;
        case USERDEFS_VOLUME_NUMBER:                ud.volume_number                 = iSet; break;
        case USERDEFS_M_MARKER:                     ud.m_marker                      = iSet; break;
        case USERDEFS_MARKER:                       ud.marker                        = iSet; break;
        case USERDEFS_MOUSEFLIP:                    ud.mouseflip                     = iSet; break;
        case USERDEFS_STATUSBARSCALE:               ud.statusbarscale                = iSet; break;
        case USERDEFS_DRAWWEAPON:                   ud.drawweapon                    = iSet; break;
        case USERDEFS_MOUSEAIMING:                  ud.mouseaiming                   = iSet; break;
        case USERDEFS_WEAPONSWITCH:                 ud.weaponswitch                  = iSet; break;
        case USERDEFS_DEMOCAMS:                     ud.democams                      = iSet; break;
        case USERDEFS_COLOR:                        ud.color                         = iSet; break;
        case USERDEFS_MSGDISPTIME:                  ud.msgdisptime                   = iSet; break;
        case USERDEFS_STATUSBARMODE:                ud.statusbarmode                 = iSet; break;
        case USERDEFS_M_NOEXITS:                    ud.m_noexits                     = iSet; break;
        case USERDEFS_NOEXITS:                      ud.noexits                       = iSet; break;
        case USERDEFS_AUTOVOTE:                     ud.autovote                      = iSet; break;
        case USERDEFS_AUTOMSG:                      ud.automsg                       = iSet; break;
        case USERDEFS_IDPLAYERS:                    ud.idplayers                     = iSet; break;
        case USERDEFS_TEAM:                         ud.team                          = iSet; break;
        case USERDEFS_VIEWBOB:                      ud.viewbob                       = iSet; break;
        case USERDEFS_WEAPONSWAY:                   ud.weaponsway                    = iSet; break;
        case USERDEFS_ANGLEINTERPOLATION:           ud.angleinterpolation            = iSet; break;
        case USERDEFS_OBITUARIES:                   ud.obituaries                    = iSet; break;
        case USERDEFS_LEVELSTATS:                   ud.levelstats                    = iSet; break;
        case USERDEFS_CROSSHAIRSCALE:               ud.crosshairscale                = iSet; break;
        case USERDEFS_ALTHUD:                       ud.althud                        = iSet; break;
        case USERDEFS_DISPLAY_BONUS_SCREEN:         ud.display_bonus_screen          = iSet; break;
        case USERDEFS_SHOW_LEVEL_TEXT:              ud.show_level_text               = iSet; break;
        case USERDEFS_WEAPONSCALE:                  ud.weaponscale                   = iSet; break;
        case USERDEFS_TEXTSCALE:                    ud.textscale                     = iSet; break;
        case USERDEFS_RUNKEY_MODE:                  ud.runkey_mode                   = iSet; break;
        case USERDEFS_M_ORIGIN_X:                   ud.returnvar[0]                  = iSet; break;
        case USERDEFS_M_ORIGIN_Y:                   ud.returnvar[1]                  = iSet; break;
        case USERDEFS_GLOBALFLAGS:                  globalflags                      = iSet; break;
        case USERDEFS_GLOBALGAMEFLAGS:              duke3d_globalflags               = iSet; break;
        case USERDEFS_VM_PLAYER:
            vm.playerNum = iSet;
            vm.pPlayer   = g_player[iSet].ps;
            break;
        case USERDEFS_VM_SPRITE:
            vm.spriteNum = iSet;
            vm.pSprite   = &sprite[iSet];
            vm.pActor    = &actor[iSet];
            vm.pData     = &actor[iSet].t_data[0];
            break;
        case USERDEFS_VM_DISTANCE: vm.playerDist                                     = iSet; break;
        case USERDEFS_GAMETEXT_TRACKING:            MF_Bluefont.between.x            = iSet; break;
        case USERDEFS_MENUTEXT_TRACKING:            MF_Redfont.between.x             = iSet; break;
        case USERDEFS_MAXSPRITESONSCREEN:           maxspritesonscreen               = clamp(iSet, MAXSPRITESONSCREEN>>2, MAXSPRITESONSCREEN); break;
        case USERDEFS_SCREENAREA_X1:                aGameVars[g_returnVarID].global  = iSet; break;
        case USERDEFS_SCREENAREA_Y1:                ud.returnvar[0]                  = iSet; break;
        case USERDEFS_SCREENAREA_X2:                ud.returnvar[1]                  = iSet; break;
        case USERDEFS_SCREENAREA_Y2:                ud.returnvar[2]                  = iSet; break;
        case USERDEFS_SCREENFADE:                   ud.screenfade                    = iSet; break;
        case USERDEFS_MENUBACKGROUND:               ud.menubackground                = iSet; break;
        case USERDEFS_STATUSBARFLAGS:               ud.statusbarflags                = iSet; break;
        case USERDEFS_STATUSBARRANGE:               ud.statusbarrange                = iSet; break;
        case USERDEFS_STATUSBARCUSTOM:              ud.statusbarcustom               = iSet; break;
        case USERDEFS_HUDONTOP:                     ud.hudontop                      = iSet; break;
        case USERDEFS_MENU_SLIDEBARZ:               ud.menu_slidebarz                = iSet; break;
        case USERDEFS_MENU_SLIDEBARMARGIN:          ud.menu_slidebarmargin           = iSet; break;
        case USERDEFS_MENU_SLIDECURSORZ:            ud.menu_slidecursorz             = iSet; break;
        case USERDEFS_GLOBAL_R:                     globalr                          = iSet; break;
        case USERDEFS_GLOBAL_G:                     globalg                          = iSet; break;
        case USERDEFS_GLOBAL_B:                     globalb                          = iSet; break;
        case USERDEFS_DEFAULT_VOLUME:               ud.default_volume                = iSet; break;
        case USERDEFS_DEFAULT_SKILL:                ud.default_skill                 = iSet; break;
        case USERDEFS_MENU_SHADEDESELECTED:         MF_Redfont.shade_deselected      = MF_Bluefont.shade_deselected = MF_Minifont.shade_deselected = iSet; break;
        case USERDEFS_MENU_SHADEDISABLED:           MF_Redfont.shade_disabled        = MF_Bluefont.shade_disabled   = MF_Minifont.shade_disabled   = iSet; break;
        case USERDEFS_MENUTEXT_ZOOM:                MF_Redfont.zoom                  = iSet; break;
        case USERDEFS_MENUTEXT_XSPACE:              MF_Redfont.emptychar.x           = iSet; break;
        case USERDEFS_MENUTEXT_PAL:                 MF_Redfont.pal                   = iSet; break;
        case USERDEFS_MENUTEXT_PALSELECTED:         MF_Redfont.pal_selected          = iSet; break;
        case USERDEFS_MENUTEXT_PALDESELECTED:       MF_Redfont.pal_deselected        = iSet; break;
        case USERDEFS_MENUTEXT_PALDISABLED:         MF_Redfont.pal_disabled          = iSet; break;
        case USERDEFS_MENUTEXT_PALSELECTED_RIGHT:   MF_Redfont.pal_selected_right    = iSet; break;
        case USERDEFS_MENUTEXT_PALDESELECTED_RIGHT: MF_Redfont.pal_deselected_right  = iSet; break;
        case USERDEFS_MENUTEXT_PALDISABLED_RIGHT:   MF_Redfont.pal_disabled_right    = iSet; break;
        case USERDEFS_GAMETEXT_ZOOM:                MF_Bluefont.zoom                 = iSet; break;
        case USERDEFS_GAMETEXT_XSPACE:              MF_Bluefont.emptychar.x          = iSet; break;
        case USERDEFS_GAMETEXT_PAL:                 MF_Bluefont.pal                  = iSet; break;
        case USERDEFS_GAMETEXT_PALSELECTED:         MF_Bluefont.pal_selected         = iSet; break;
        case USERDEFS_GAMETEXT_PALDESELECTED:       MF_Bluefont.pal_deselected       = iSet; break;
        case USERDEFS_GAMETEXT_PALDISABLED:         MF_Bluefont.pal_disabled         = iSet; break;
        case USERDEFS_GAMETEXT_PALSELECTED_RIGHT:   MF_Bluefont.pal_selected_right   = iSet; break;
        case USERDEFS_GAMETEXT_PALDESELECTED_RIGHT: MF_Bluefont.pal_deselected_right = iSet; break;
        case USERDEFS_GAMETEXT_PALDISABLED_RIGHT:   MF_Bluefont.pal_disabled_right   = iSet; break;
        case USERDEFS_MINITEXT_ZOOM:                MF_Minifont.zoom                 = iSet; break;
        case USERDEFS_MINITEXT_XSPACE:              MF_Minifont.emptychar.x          = iSet; break;
        case USERDEFS_MINITEXT_TRACKING:            MF_Minifont.between.x            = iSet; break;
        case USERDEFS_MINITEXT_PAL:                 MF_Minifont.pal                  = iSet; break;
        case USERDEFS_MINITEXT_PALSELECTED:         MF_Minifont.pal_selected         = iSet; break;
        case USERDEFS_MINITEXT_PALDESELECTED:       MF_Minifont.pal_deselected       = iSet; break;
        case USERDEFS_MINITEXT_PALDISABLED:         MF_Minifont.pal_disabled         = iSet; break;
        case USERDEFS_MINITEXT_PALSELECTED_RIGHT:   MF_Minifont.pal_selected_right   = iSet; break;
        case USERDEFS_MINITEXT_PALDESELECTED_RIGHT: MF_Minifont.pal_deselected_right = iSet; break;
        case USERDEFS_MINITEXT_PALDISABLED_RIGHT:   MF_Minifont.pal_disabled_right   = iSet; break;
        case USERDEFS_MENUTITLE_PAL:                ud.menutitle_pal                 = iSet; break;
        case USERDEFS_SLIDEBAR_PALSELECTED:         ud.slidebar_palselected          = iSet; break;
        case USERDEFS_SLIDEBAR_PALDISABLED:         ud.slidebar_paldisabled          = iSet; break;
        case USERDEFS_SHADOW_PAL:                   ud.shadow_pal                    = iSet; break;
        case USERDEFS_MENU_SCROLLBARTILENUM:        ud.menu_scrollbartilenum         = iSet; break;
        case USERDEFS_MENU_SCROLLBARZ:              ud.menu_scrollbarz               = iSet; break;
        case USERDEFS_MENU_SCROLLCURSORZ:           ud.menu_scrollcursorz            = iSet; break;
        case USERDEFS_RETURN:
            if (lParm2 == 0)
                aGameVars[g_returnVarID].global = iSet;
            else
                ud.returnvar[lParm2 - 1] = iSet;
            break;
        case USERDEFS_USERBYTEVERSION:              ud.userbytever                   = iSet; break;
        case USERDEFS_AUTOSAVE:                     ud.autosave                      = iSet; break;
    }
}

int32_t __fastcall VM_GetActiveProjectile(int const spriteNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return -1;
    }

    auto const &p = SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE:   labelNum = p.workslike;  break;
        case PROJ_SPAWNS:      labelNum = p.spawns;     break;
        case PROJ_SXREPEAT:    labelNum = p.sxrepeat;   break;
        case PROJ_SYREPEAT:    labelNum = p.syrepeat;   break;
        case PROJ_SOUND:       labelNum = p.sound;      break;
        case PROJ_ISOUND:      labelNum = p.isound;     break;
        case PROJ_VEL:         labelNum = p.vel;        break;
        case PROJ_EXTRA:       labelNum = p.extra;      break;
        case PROJ_DECAL:       labelNum = p.decal;      break;
        case PROJ_TRAIL:       labelNum = p.trail;      break;
        case PROJ_TXREPEAT:    labelNum = p.txrepeat;   break;
        case PROJ_TYREPEAT:    labelNum = p.tyrepeat;   break;
        case PROJ_TOFFSET:     labelNum = p.toffset;    break;
        case PROJ_TNUM:        labelNum = p.tnum;       break;
        case PROJ_DROP:        labelNum = p.drop;       break;
        case PROJ_CSTAT:       labelNum = p.cstat;      break;
        case PROJ_CLIPDIST:    labelNum = p.clipdist;   break;
        case PROJ_SHADE:       labelNum = p.shade;      break;
        case PROJ_XREPEAT:     labelNum = p.xrepeat;    break;
        case PROJ_YREPEAT:     labelNum = p.yrepeat;    break;
        case PROJ_PAL:         labelNum = p.pal;        break;
        case PROJ_EXTRA_RAND:  labelNum = p.extra_rand; break;
        case PROJ_HITRADIUS:   labelNum = p.hitradius;  break;
        case PROJ_MOVECNT:     labelNum = p.movecnt;    break;
        case PROJ_OFFSET:      labelNum = p.offset;     break;
        case PROJ_BOUNCES:     labelNum = p.bounces;    break;
        case PROJ_BSOUND:      labelNum = p.bsound;     break;
        case PROJ_RANGE:       labelNum = p.range;      break;
        case PROJ_FLASH_COLOR: labelNum = p.flashcolor; break;
        case PROJ_USERDATA:    labelNum = p.userdata;   break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetActiveProjectile(int const spriteNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return;
    }

    auto &p = SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE:   p.workslike  = newValue; break;
        case PROJ_SPAWNS:      p.spawns     = newValue; break;
        case PROJ_SXREPEAT:    p.sxrepeat   = newValue; break;
        case PROJ_SYREPEAT:    p.syrepeat   = newValue; break;
        case PROJ_SOUND:       p.sound      = newValue; break;
        case PROJ_ISOUND:      p.isound     = newValue; break;
        case PROJ_VEL:         p.vel        = newValue; break;
        case PROJ_EXTRA:       p.extra      = newValue; break;
        case PROJ_DECAL:       p.decal      = newValue; break;
        case PROJ_TRAIL:       p.trail      = newValue; break;
        case PROJ_TXREPEAT:    p.txrepeat   = newValue; break;
        case PROJ_TYREPEAT:    p.tyrepeat   = newValue; break;
        case PROJ_TOFFSET:     p.toffset    = newValue; break;
        case PROJ_TNUM:        p.tnum       = newValue; break;
        case PROJ_DROP:        p.drop       = newValue; break;
        case PROJ_CSTAT:       p.cstat      = newValue; break;
        case PROJ_CLIPDIST:    p.clipdist   = newValue; break;
        case PROJ_SHADE:       p.shade      = newValue; break;
        case PROJ_XREPEAT:     p.xrepeat    = newValue; break;
        case PROJ_YREPEAT:     p.yrepeat    = newValue; break;
        case PROJ_PAL:         p.pal        = newValue; break;
        case PROJ_EXTRA_RAND:  p.extra_rand = newValue; break;
        case PROJ_HITRADIUS:   p.hitradius  = newValue; break;
        case PROJ_MOVECNT:     p.movecnt    = newValue; break;
        case PROJ_OFFSET:      p.offset     = newValue; break;
        case PROJ_BOUNCES:     p.bounces    = newValue; break;
        case PROJ_BSOUND:      p.bsound     = newValue; break;
        case PROJ_RANGE:       p.range      = newValue; break;
        case PROJ_FLASH_COLOR: p.flashcolor = newValue; break;
        case PROJ_USERDATA:    p.userdata   = newValue; break;
    }
}

int32_t __fastcall VM_GetPlayer(int const playerNum, int32_t labelNum, int const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned) playerNum >= (unsigned) g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return -1;
    }

    auto const &ps = *g_player[playerNum].ps;

    switch (labelNum)
    {
        case PLAYER_ANG:       labelNum = fix16_to_int(ps.q16ang);       break;
        case PLAYER_OANG:      labelNum = fix16_to_int(ps.oq16ang);      break;
        case PLAYER_ANGVEL:    labelNum = fix16_to_int(ps.q16angvel);    break;
        case PLAYER_HORIZ:     labelNum = fix16_to_int(ps.q16horiz);     break;
        case PLAYER_OHORIZ:    labelNum = fix16_to_int(ps.oq16horiz);    break;
        case PLAYER_HORIZOFF:  labelNum = fix16_to_int(ps.q16horizoff);  break;
        case PLAYER_OHORIZOFF: labelNum = fix16_to_int(ps.oq16horizoff); break;

        case PLAYER_ACCESS_INCS:        labelNum = ps.access_incs;        break;
        case PLAYER_ACCESS_SPRITENUM:   labelNum = ps.access_spritenum;   break;
        case PLAYER_ACCESS_WALLNUM:     labelNum = ps.access_wallnum;     break;
        case PLAYER_ACTORS_KILLED:      labelNum = ps.actors_killed;      break;
        case PLAYER_ACTORSQU:           labelNum = ps.actorsqu;           break;
        case PLAYER_AIM_MODE:           labelNum = ps.aim_mode;           break;
        case PLAYER_AIRLEFT:            labelNum = ps.airleft;            break;
        case PLAYER_AUTO_AIM:           labelNum = ps.auto_aim;           break;
        case PLAYER_AUTOSTEP:           labelNum = ps.autostep;           break;
        case PLAYER_AUTOSTEP_SBW:       labelNum = ps.autostep_sbw;       break;
        case PLAYER_BOBCOUNTER:         labelNum = ps.bobcounter;         break;
        case PLAYER_BOBPOSX:            labelNum = ps.bobpos.x;           break;
        case PLAYER_BOBPOSY:            labelNum = ps.bobpos.y;           break;
        case PLAYER_BUTTONPALETTE:      labelNum = ps.buttonpalette;      break;
        case PLAYER_CHEAT_PHASE:        labelNum = ps.cheat_phase;        break;
        case PLAYER_CLIPDIST:           labelNum = ps.clipdist;           break;
        case PLAYER_CRACK_TIME:         labelNum = ps.crack_time;         break;
        case PLAYER_CURR_WEAPON:        labelNum = ps.curr_weapon;        break;
        case PLAYER_CURSECTNUM:         labelNum = ps.cursectnum;         break;
        case PLAYER_CUSTOMEXITSOUND:    labelNum = ps.customexitsound;    break;
        case PLAYER_DEAD_FLAG:          labelNum = ps.dead_flag;          break;
        case PLAYER_DUMMYPLAYERSPRITE:  labelNum = ps.dummyplayersprite;  break;
        case PLAYER_EXITX:              labelNum = ps.exitx;              break;
        case PLAYER_EXITY:              labelNum = ps.exity;              break;
        case PLAYER_EXTRA_EXTRA8:       labelNum = ps.extra_extra8;       break;
        case PLAYER_FALLING_COUNTER:    labelNum = ps.falling_counter;    break;
        case PLAYER_FIST_INCS:          labelNum = ps.fist_incs;          break;
        case PLAYER_FOOTPRINTCOUNT:     labelNum = ps.footprintcount;     break;
        case PLAYER_FOOTPRINTPAL:       labelNum = ps.footprintpal;       break;
        case PLAYER_FOOTPRINTSHADE:     labelNum = ps.footprintshade;     break;
        case PLAYER_FRAG:               labelNum = ps.frag;               break;
        case PLAYER_FRAG_PS:            labelNum = ps.frag_ps;            break;
        case PLAYER_FRAGGEDSELF:        labelNum = ps.fraggedself;        break;
        case PLAYER_FTA:                labelNum = ps.fta;                break;
        case PLAYER_FTQ:                labelNum = ps.ftq;                break;
        case PLAYER_GM:                 labelNum = ps.gm;                 break;
        case PLAYER_GOT_ACCESS:         labelNum = ps.got_access;         break;
        case PLAYER_HARD_LANDING:       labelNum = ps.hard_landing;       break;
        case PLAYER_HBOMB_HOLD_DELAY:   labelNum = ps.hbomb_hold_delay;   break;
        case PLAYER_HBOMB_ON:           labelNum = ps.hbomb_on;           break;
        case PLAYER_HEAT_ON:            labelNum = ps.heat_on;            break;
        case PLAYER_HOLODUKE_ON:        labelNum = ps.holoduke_on;        break;
        case PLAYER_HOLSTER_WEAPON:     labelNum = ps.holster_weapon;     break;
        case PLAYER_HUDPAL:             labelNum = P_GetHudPal(&ps);      break;
        case PLAYER_HURT_DELAY:         labelNum = ps.hurt_delay;         break;
        case PLAYER_I:                  labelNum = ps.i;                  break;
        case PLAYER_INDEX:              labelNum = playerNum;             break;
        case PLAYER_INTERFACE_TOGGLE:   labelNum = ps.interface_toggle;   break;
        case PLAYER_INVDISPTIME:        labelNum = ps.invdisptime;        break;
        case PLAYER_INVEN_ICON:         labelNum = ps.inven_icon;         break;
        case PLAYER_JETPACK_ON:         labelNum = ps.jetpack_on;         break;
        case PLAYER_JUMPING_COUNTER:    labelNum = ps.jumping_counter;    break;
        case PLAYER_JUMPING_TOGGLE:     labelNum = ps.jumping_toggle;     break;
        case PLAYER_KICKBACK_PIC:       labelNum = ps.kickback_pic;       break;
        case PLAYER_KNEE_INCS:          labelNum = ps.knee_incs;          break;
        case PLAYER_KNUCKLE_INCS:       labelNum = ps.knuckle_incs;       break;
        case PLAYER_LAST_EXTRA:         labelNum = ps.last_extra;         break;
        case PLAYER_LAST_FULL_WEAPON:   labelNum = ps.last_full_weapon;   break;
        case PLAYER_LAST_PISSED_TIME:   labelNum = ps.last_pissed_time;   break;
        case PLAYER_LAST_QUICK_KICK:    labelNum = ps.last_quick_kick;    break;
        case PLAYER_LAST_USED_WEAPON:   labelNum = ps.last_used_weapon;   break;
        case PLAYER_LAST_WEAPON:        labelNum = ps.last_weapon;        break;
        case PLAYER_LASTRANDOMSPOT:     labelNum = ps.lastrandomspot;     break;
        case PLAYER_LOOGCNT:            labelNum = ps.loogcnt;            break;
        case PLAYER_LOOGIEX:            labelNum = ps.loogiex[lParm2];    break;
        case PLAYER_LOOGIEY:            labelNum = ps.loogiey[lParm2];    break;
        case PLAYER_LOOK_ANG:           labelNum = ps.look_ang;           break;
        case PLAYER_MAX_ACTORS_KILLED:  labelNum = ps.max_actors_killed;  break;
        case PLAYER_MAX_PLAYER_HEALTH:  labelNum = ps.max_player_health;  break;
        case PLAYER_MAX_SECRET_ROOMS:   labelNum = ps.max_secret_rooms;   break;
        case PLAYER_MAX_SHIELD_AMOUNT:  labelNum = ps.max_shield_amount;  break;
        case PLAYER_MOVEMENT_LOCK:      labelNum = ps.movement_lock;      break;
        case PLAYER_NEWOWNER:           labelNum = ps.newowner;           break;
        case PLAYER_NUMLOOGS:           labelNum = ps.numloogs;           break;
        case PLAYER_ON_CRANE:           labelNum = ps.on_crane;           break;
        case PLAYER_ON_GROUND:          labelNum = ps.on_ground;          break;
        case PLAYER_ON_WARPING_SECTOR:  labelNum = ps.on_warping_sector;  break;
        case PLAYER_ONE_EIGHTY_COUNT:   labelNum = ps.one_eighty_count;   break;
        case PLAYER_PARALLAX_SECTNUM:   labelNum = ps.parallax_sectnum;   break;
        case PLAYER_OPOSX:              labelNum = ps.opos.x;             break;
        case PLAYER_OPOSY:              labelNum = ps.opos.y;             break;
        case PLAYER_OPOSZ:              labelNum = ps.opos.z;             break;
        case PLAYER_OPYOFF:             labelNum = ps.opyoff;             break;
        case PLAYER_OVER_SHOULDER_ON:   labelNum = ps.over_shoulder_on;   break;
        case PLAYER_PALETTE:            labelNum = ps.palette;            break;
        case PLAYER_PALOOKUP:           labelNum = ps.palookup;           break;
        case PLAYER_PALS_TIME:          labelNum = ps.pals.f;             break;
        case PLAYER_PLAYER_PAR:         labelNum = ps.player_par;         break;
        case PLAYER_POSX:               labelNum = ps.pos.x;              break;
        case PLAYER_POSXV:              labelNum = ps.vel.x;              break;
        case PLAYER_POSY:               labelNum = ps.pos.y;              break;
        case PLAYER_POSYV:              labelNum = ps.vel.y;              break;
        case PLAYER_POSZ:               labelNum = ps.pos.z;              break;
        case PLAYER_POSZV:              labelNum = ps.vel.z;              break;
        case PLAYER_PYCOUNT:            labelNum = ps.pycount;            break;
        case PLAYER_PYOFF:              labelNum = ps.pyoff;              break;
        case PLAYER_QUICK_KICK:         labelNum = ps.quick_kick;         break;
        case PLAYER_RANDOM_CLUB_FRAME:  labelNum = ps.random_club_frame;  break;
        case PLAYER_RANDOMFLAMEX:       labelNum = ps.randomflamex;       break;
        case PLAYER_RAPID_FIRE_HOLD:    labelNum = ps.rapid_fire_hold;    break;
        case PLAYER_REFRESH_INVENTORY:  labelNum = ps.refresh_inventory;  break;
        case PLAYER_RELOADING:          labelNum = ps.reloading;          break;
        case PLAYER_RETURN_TO_CENTER:   labelNum = ps.return_to_center;   break;
        case PLAYER_ROTSCRNANG:         labelNum = ps.rotscrnang;         break;
        case PLAYER_RUNSPEED:           labelNum = ps.runspeed;           break;
        case PLAYER_SBS:                labelNum = ps.sbs;                break;
        case PLAYER_SCREAM_VOICE:       labelNum = ps.scream_voice;       break;
        case PLAYER_SCUBA_ON:           labelNum = ps.scuba_on;           break;
        case PLAYER_SECRET_ROOMS:       labelNum = ps.secret_rooms;       break;
        case PLAYER_SHOW_EMPTY_WEAPON:  labelNum = ps.show_empty_weapon;  break;
        case PLAYER_SOMETHINGONPLAYER:  labelNum = ps.somethingonplayer;  break;
        case PLAYER_SOUND_PITCH:        labelNum = ps.sound_pitch;        break;
        case PLAYER_SPRITEBRIDGE:       labelNum = ps.spritebridge;       break;
        case PLAYER_SUBWEAPON:          labelNum = ps.subweapon;          break;
        case PLAYER_TEAM:               labelNum = ps.team;               break;
        case PLAYER_TIMEBEFOREEXIT:     labelNum = ps.timebeforeexit;     break;
        case PLAYER_TIPINCS:            labelNum = ps.tipincs;            break;
        case PLAYER_TOGGLE_KEY_FLAG:    labelNum = ps.toggle_key_flag;    break;
        case PLAYER_TRANSPORTER_HOLD:   labelNum = ps.transporter_hold;   break;
        case PLAYER_TRUECZ:             labelNum = ps.truecz;             break;
        case PLAYER_TRUEFZ:             labelNum = ps.truefz;             break;
        case PLAYER_VISIBILITY:         labelNum = ps.visibility;         break;
        case PLAYER_WACKEDBYACTOR:      labelNum = ps.wackedbyactor;      break;
        case PLAYER_WALKING_SND_TOGGLE: labelNum = ps.walking_snd_toggle; break;
        case PLAYER_WANTWEAPONFIRE:     labelNum = ps.wantweaponfire;     break;
        case PLAYER_WEAPON_ANG:         labelNum = ps.weapon_ang;         break;
        case PLAYER_WEAPON_POS:         labelNum = ps.weapon_pos;         break;
        case PLAYER_WEAPON_SWAY:        labelNum = ps.weapon_sway;        break;
        case PLAYER_WEAPONSWITCH:       labelNum = ps.weaponswitch;       break;
        case PLAYER_WEAPRECCNT:         labelNum = ps.weapreccnt;         break;
        case PLAYER_WEAPRECS:           labelNum = ps.weaprecs[lParm2];   break;
        case PLAYER_ZOOM:               labelNum = ps.zoom;               break;

        case PLAYER_BOOT_AMOUNT:     labelNum = ps.inv_amount[GET_BOOTS];    break;
        case PLAYER_FIRSTAID_AMOUNT: labelNum = ps.inv_amount[GET_FIRSTAID]; break;
        case PLAYER_HEAT_AMOUNT:     labelNum = ps.inv_amount[GET_HEATS];    break;
        case PLAYER_HOLODUKE_AMOUNT: labelNum = ps.inv_amount[GET_HOLODUKE]; break;
        case PLAYER_JETPACK_AMOUNT:  labelNum = ps.inv_amount[GET_JETPACK];  break;
        case PLAYER_SCUBA_AMOUNT:    labelNum = ps.inv_amount[GET_SCUBA];    break;
        case PLAYER_SHIELD_AMOUNT:   labelNum = ps.inv_amount[GET_SHIELD];   break;
        case PLAYER_STEROIDS_AMOUNT: labelNum = ps.inv_amount[GET_STEROIDS]; break;

        case PLAYER_AMMO_AMOUNT:      labelNum = ps.ammo_amount[lParm2];     break;
        case PLAYER_MAX_AMMO_AMOUNT:  labelNum = ps.max_ammo_amount[lParm2]; break;

        case PLAYER_GOTWEAPON: labelNum = (ps.gotweapon & (1<<lParm2)) != 0; break;

        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: labelNum = ps.pals.r; break;
                case 1: labelNum = ps.pals.g; break;
                case 2: labelNum = ps.pals.b; break;
            }
            break;

        case PLAYER_CONNECTED: labelNum = g_player[playerNum].playerquitflag; break;
        case PLAYER_FRAGS:
            labelNum = (playerNum == lParm2) ? ps.fraggedself : g_player[playerNum].frags[lParm2]; break;
        case PLAYER_DEATHS: labelNum = g_player[playerNum].frags[playerNum]; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetPlayer(int const playerNum, int const labelNum, int const lParm2, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return;
    }

    auto &ps = *g_player[playerNum].ps;

    switch (labelNum)
    {
        case PLAYER_HORIZ:     ps.q16horiz     = fix16_from_int(newValue); break;
        case PLAYER_OHORIZ:    ps.oq16horiz    = fix16_from_int(newValue); break;
        case PLAYER_OHORIZOFF: ps.oq16horizoff = fix16_from_int(newValue); break;
        case PLAYER_ANG:       ps.q16ang       = fix16_from_int(newValue); break;
        case PLAYER_OANG:      ps.oq16ang      = fix16_from_int(newValue); break;
        case PLAYER_ANGVEL:    ps.q16angvel    = fix16_from_int(newValue); break;
        case PLAYER_HORIZOFF:  ps.q16horizoff  = fix16_from_int(newValue); break;

        case PLAYER_ACCESS_INCS:        ps.access_incs        = newValue; break;
        case PLAYER_ACCESS_SPRITENUM:   ps.access_spritenum   = newValue; break;
        case PLAYER_ACCESS_WALLNUM:     ps.access_wallnum     = newValue; break;
        case PLAYER_ACTORS_KILLED:      ps.actors_killed      = newValue; break;
        case PLAYER_ACTORSQU:           ps.actorsqu           = newValue; break;
        case PLAYER_AIM_MODE:           ps.aim_mode           = newValue; break;
        case PLAYER_AIRLEFT:            ps.airleft            = newValue; break;
        case PLAYER_AUTO_AIM:           ps.auto_aim           = newValue; break;
        case PLAYER_AUTOSTEP:           ps.autostep           = newValue; break;
        case PLAYER_AUTOSTEP_SBW:       ps.autostep_sbw       = newValue; break;
        case PLAYER_BOBCOUNTER:         ps.bobcounter         = newValue; break;
        case PLAYER_BOBPOSX:            ps.bobpos.x           = newValue; break;
        case PLAYER_BOBPOSY:            ps.bobpos.y           = newValue; break;
        case PLAYER_BUTTONPALETTE:      ps.buttonpalette      = newValue; break;
        case PLAYER_CHEAT_PHASE:        ps.cheat_phase        = newValue; break;
        case PLAYER_CLIPDIST:           ps.clipdist           = newValue; break;
        case PLAYER_CRACK_TIME:         ps.crack_time         = newValue; break;
        case PLAYER_CURR_WEAPON:        ps.curr_weapon        = newValue; break;
        case PLAYER_CURSECTNUM:         ps.cursectnum         = newValue; break;
        case PLAYER_CUSTOMEXITSOUND:    ps.customexitsound    = newValue; break;
        case PLAYER_DEAD_FLAG:          ps.dead_flag          = newValue; break;
        case PLAYER_DUMMYPLAYERSPRITE:  ps.dummyplayersprite  = newValue; break;
        case PLAYER_EXITX:              ps.exitx              = newValue; break;
        case PLAYER_EXITY:              ps.exity              = newValue; break;
        case PLAYER_EXTRA_EXTRA8:       ps.extra_extra8       = newValue; break;
        case PLAYER_FALLING_COUNTER:    ps.falling_counter    = newValue; break;
        case PLAYER_FIST_INCS:          ps.fist_incs          = newValue; break;
        case PLAYER_FOOTPRINTCOUNT:     ps.footprintcount     = newValue; break;
        case PLAYER_FOOTPRINTPAL:       ps.footprintpal       = newValue; break;
        case PLAYER_FOOTPRINTSHADE:     ps.footprintshade     = newValue; break;
        case PLAYER_FRAG:               ps.frag               = newValue; break;
        case PLAYER_FRAG_PS:            ps.frag_ps            = newValue; break;
        case PLAYER_FRAGGEDSELF:        ps.fraggedself        = newValue; break;
        case PLAYER_FTA:                ps.fta                = newValue; break;
        case PLAYER_FTQ:                ps.ftq                = newValue; break;
        case PLAYER_GOT_ACCESS:         ps.got_access         = newValue; break;
        case PLAYER_HARD_LANDING:       ps.hard_landing       = newValue; break;
        case PLAYER_HBOMB_HOLD_DELAY:   ps.hbomb_hold_delay   = newValue; break;
        case PLAYER_HBOMB_ON:           ps.hbomb_on           = newValue; break;
        case PLAYER_HOLODUKE_ON:        ps.holoduke_on        = newValue; break;
        case PLAYER_HOLSTER_WEAPON:     ps.holster_weapon     = newValue; break;
        case PLAYER_HURT_DELAY:         ps.hurt_delay         = newValue; break;
        case PLAYER_I:                  ps.i                  = newValue; break;
        case PLAYER_INTERFACE_TOGGLE:   ps.interface_toggle   = newValue; break;
        case PLAYER_INVDISPTIME:        ps.invdisptime        = newValue; break;
        case PLAYER_INVEN_ICON:         ps.inven_icon         = newValue; break;
        case PLAYER_JETPACK_ON:         ps.jetpack_on         = newValue; break;
        case PLAYER_JUMPING_COUNTER:    ps.jumping_counter    = newValue; break;
        case PLAYER_JUMPING_TOGGLE:     ps.jumping_toggle     = newValue; break;
        case PLAYER_KICKBACK_PIC:       ps.kickback_pic       = newValue; break;
        case PLAYER_KNEE_INCS:          ps.knee_incs          = newValue; break;
        case PLAYER_KNUCKLE_INCS:       ps.knuckle_incs       = newValue; break;
        case PLAYER_LAST_EXTRA:         ps.last_extra         = newValue; break;
        case PLAYER_LAST_FULL_WEAPON:   ps.last_full_weapon   = newValue; break;
        case PLAYER_LAST_PISSED_TIME:   ps.last_pissed_time   = newValue; break;
        case PLAYER_LAST_QUICK_KICK:    ps.last_quick_kick    = newValue; break;
        case PLAYER_LAST_USED_WEAPON:   ps.last_used_weapon   = newValue; break;
        case PLAYER_LAST_WEAPON:        ps.last_weapon        = newValue; break;
        case PLAYER_LASTRANDOMSPOT:     ps.lastrandomspot     = newValue; break;
        case PLAYER_LOOGCNT:            ps.loogcnt            = newValue; break;
        case PLAYER_LOOGIEX:            ps.loogiex[lParm2]    = newValue; break;
        case PLAYER_LOOGIEY:            ps.loogiey[lParm2]    = newValue; break;
        case PLAYER_LOOK_ANG:           ps.look_ang           = newValue; break;
        case PLAYER_MAX_ACTORS_KILLED:  ps.max_actors_killed  = newValue; break;
        case PLAYER_MAX_PLAYER_HEALTH:  ps.max_player_health  = newValue; break;
        case PLAYER_MAX_SECRET_ROOMS:   ps.max_secret_rooms   = newValue; break;
        case PLAYER_MAX_SHIELD_AMOUNT:  ps.max_shield_amount  = newValue; break;
        case PLAYER_MOVEMENT_LOCK:      ps.movement_lock      = newValue; break;
        case PLAYER_NEWOWNER:           ps.newowner           = newValue; break;
        case PLAYER_NUMLOOGS:           ps.numloogs           = newValue; break;
        case PLAYER_ON_CRANE:           ps.on_crane           = newValue; break;
        case PLAYER_ON_GROUND:          ps.on_ground          = newValue; break;
        case PLAYER_ON_WARPING_SECTOR:  ps.on_warping_sector  = newValue; break;
        case PLAYER_ONE_EIGHTY_COUNT:   ps.one_eighty_count   = newValue; break;
        case PLAYER_PARALLAX_SECTNUM:   ps.parallax_sectnum   = newValue; break;
        case PLAYER_OPOSX:              ps.opos.x             = newValue; break;
        case PLAYER_OPOSY:              ps.opos.y             = newValue; break;
        case PLAYER_OPOSZ:              ps.opos.z             = newValue; break;
        case PLAYER_OPYOFF:             ps.opyoff             = newValue; break;
        case PLAYER_OVER_SHOULDER_ON:   ps.over_shoulder_on   = newValue; break;
        case PLAYER_PALOOKUP:           ps.palookup           = newValue; break;
        case PLAYER_PALS_TIME:          ps.pals.f             = newValue; break;
        case PLAYER_PLAYER_PAR:         ps.player_par         = newValue; break;
        case PLAYER_POSX:               ps.pos.x              = newValue; break;
        case PLAYER_POSXV:              ps.vel.x              = newValue; break;
        case PLAYER_POSY:               ps.pos.y              = newValue; break;
        case PLAYER_POSYV:              ps.vel.y              = newValue; break;
        case PLAYER_POSZ:               ps.pos.z              = newValue; break;
        case PLAYER_POSZV:              ps.vel.z              = newValue; break;
        case PLAYER_PYCOUNT:            ps.pycount            = newValue; break;
        case PLAYER_PYOFF:              ps.pyoff              = newValue; break;
        case PLAYER_QUICK_KICK:         ps.quick_kick         = newValue; break;
        case PLAYER_RANDOM_CLUB_FRAME:  ps.random_club_frame  = newValue; break;
        case PLAYER_RANDOMFLAMEX:       ps.randomflamex       = newValue; break;
        case PLAYER_RAPID_FIRE_HOLD:    ps.rapid_fire_hold    = newValue; break;
        case PLAYER_REFRESH_INVENTORY:  ps.refresh_inventory  = newValue; break;
        case PLAYER_RELOADING:          ps.reloading          = newValue; break;
        case PLAYER_RETURN_TO_CENTER:   ps.return_to_center   = newValue; break;
        case PLAYER_ROTSCRNANG:         ps.rotscrnang         = newValue; break;
        case PLAYER_RUNSPEED:           ps.runspeed           = newValue; break;
        case PLAYER_SBS:                ps.sbs                = newValue; break;
        case PLAYER_SCREAM_VOICE:       ps.scream_voice       = newValue; break;
        case PLAYER_SCUBA_ON:           ps.scuba_on           = newValue; break;
        case PLAYER_SECRET_ROOMS:       ps.secret_rooms       = newValue; break;
        case PLAYER_SHOW_EMPTY_WEAPON:  ps.show_empty_weapon  = newValue; break;
        case PLAYER_SOMETHINGONPLAYER:  ps.somethingonplayer  = newValue; break;
        case PLAYER_SOUND_PITCH:        ps.sound_pitch        = newValue; break;
        case PLAYER_SPRITEBRIDGE:       ps.spritebridge       = newValue; break;
        case PLAYER_SUBWEAPON:          ps.subweapon          = newValue; break;
        case PLAYER_TEAM:               ps.team               = newValue; break;
        case PLAYER_TIMEBEFOREEXIT:     ps.timebeforeexit     = newValue; break;
        case PLAYER_TIPINCS:            ps.tipincs            = newValue; break;
        case PLAYER_TOGGLE_KEY_FLAG:    ps.toggle_key_flag    = newValue; break;
        case PLAYER_TRANSPORTER_HOLD:   ps.transporter_hold   = newValue; break;
        case PLAYER_TRUECZ:             ps.truecz             = newValue; break;
        case PLAYER_TRUEFZ:             ps.truefz             = newValue; break;
        case PLAYER_VISIBILITY:         ps.visibility         = newValue; break;
        case PLAYER_WACKEDBYACTOR:      ps.wackedbyactor      = newValue; break;
        case PLAYER_WALKING_SND_TOGGLE: ps.walking_snd_toggle = newValue; break;
        case PLAYER_WANTWEAPONFIRE:     ps.wantweaponfire     = newValue; break;
        case PLAYER_WEAPON_ANG:         ps.weapon_ang         = newValue; break;
        case PLAYER_WEAPON_POS:         ps.weapon_pos         = newValue; break;
        case PLAYER_WEAPON_SWAY:        ps.weapon_sway        = newValue; break;
        case PLAYER_WEAPONSWITCH:       ps.weaponswitch       = newValue; break;
        case PLAYER_WEAPRECCNT:         ps.weapreccnt         = newValue; break;
        case PLAYER_WEAPRECS:           ps.weaprecs[lParm2]   = newValue; break;
        case PLAYER_ZOOM:               ps.zoom               = newValue; break;

        case PLAYER_BOOT_AMOUNT:     ps.inv_amount[GET_BOOTS]    = newValue; break;
        case PLAYER_FIRSTAID_AMOUNT: ps.inv_amount[GET_FIRSTAID] = newValue; break;
        case PLAYER_HEAT_AMOUNT:     ps.inv_amount[GET_HEATS]    = newValue; break;
        case PLAYER_HOLODUKE_AMOUNT: ps.inv_amount[GET_HOLODUKE] = newValue; break;
        case PLAYER_JETPACK_AMOUNT:  ps.inv_amount[GET_JETPACK]  = newValue; break;
        case PLAYER_SCUBA_AMOUNT:    ps.inv_amount[GET_SCUBA]    = newValue; break;
        case PLAYER_SHIELD_AMOUNT:   ps.inv_amount[GET_SHIELD]   = newValue; break;
        case PLAYER_STEROIDS_AMOUNT: ps.inv_amount[GET_STEROIDS] = newValue; break;

        case PLAYER_AMMO_AMOUNT:     ps.ammo_amount[lParm2]     = newValue; break;
        case PLAYER_MAX_AMMO_AMOUNT: ps.max_ammo_amount[lParm2] = newValue; break;

        case PLAYER_HEAT_ON:
            if (ps.heat_on != newValue)
            {
                ps.heat_on = newValue;
                P_UpdateScreenPal(&ps);
            }
            break;

        case PLAYER_GM:
            if (!(ps.gm & MODE_MENU) && (newValue & MODE_MENU))
                Menu_Open(playerNum);
            else if ((ps.gm & MODE_MENU) && !(newValue & MODE_MENU))
                Menu_Close(playerNum);
            ps.gm = newValue;
            break;

        case PLAYER_GOTWEAPON:
            if (newValue)
                ps.gotweapon |= (1 << lParm2);
            else
                ps.gotweapon &= ~(1 << lParm2);
            break;

        case PLAYER_PALETTE: P_SetGamePalette(&ps, newValue, 2 + 16); break;

        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: ps.pals.r = newValue; break;
                case 1: ps.pals.g = newValue; break;
                case 2: ps.pals.b = newValue; break;
            }
            break;

        case PLAYER_FRAGS:
            if (playerNum == lParm2)
                ps.fraggedself = newValue;
            else
                g_player[playerNum].frags[lParm2] = newValue;
            break;

        case PLAYER_DEATHS: g_player[playerNum].frags[playerNum] = newValue; break;
    }
}

int32_t __fastcall VM_GetPlayerInput(int const playerNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return -1;
    }

    auto const &i = g_player[playerNum].inputBits;

    switch (labelNum)
    {
        case INPUT_AVEL:
            labelNum = (i->q16avel >> 16); break;

        case INPUT_HORZ:
            labelNum = (i->q16horz >> 16); break;

        case INPUT_Q16AVEL: labelNum = i->q16avel; break;
        case INPUT_Q16HORZ: labelNum = i->q16horz; break;
        case INPUT_FVEL:    labelNum = i->fvel;    break;
        case INPUT_SVEL:    labelNum = i->svel;    break;
        case INPUT_BITS:    labelNum = i->bits;    break;
        case INPUT_EXTBITS: labelNum = i->extbits; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetPlayerInput(int const playerNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return;
    }

    auto &i = g_player[playerNum].inputBits;

    switch (labelNum)
    {
        case INPUT_AVEL:
            i->q16avel = fix16_from_int(newValue); break;

        case INPUT_HORZ:
            i->q16horz = fix16_from_int(newValue); break;

        case INPUT_Q16AVEL: i->q16avel = newValue; break;
        case INPUT_Q16HORZ: i->q16horz = newValue; break;
        case INPUT_FVEL:    i->fvel    = newValue; break;
        case INPUT_SVEL:    i->svel    = newValue; break;
        case INPUT_BITS:    i->bits    = newValue; break;
        case INPUT_EXTBITS: i->extbits = newValue; break;
    }
}



int32_t __fastcall VM_GetWall(int const wallNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return -1;
    }

    auto const &w = *(uwalltype *)&wall[wallNum];

    switch (labelNum)
    {
        case WALL_X:          labelNum = w.x;          break;
        case WALL_Y:          labelNum = w.y;          break;
        case WALL_POINT2:     labelNum = w.point2;     break;
        case WALL_NEXTWALL:   labelNum = w.nextwall;   break;
        case WALL_NEXTSECTOR: labelNum = w.nextsector; break;
        case WALL_CSTAT:      labelNum = w.cstat;      break;
        case WALL_PICNUM:     labelNum = w.picnum;     break;
        case WALL_OVERPICNUM: labelNum = w.overpicnum; break;
        case WALL_SHADE:      labelNum = w.shade;      break;
        case WALL_PAL:        labelNum = w.pal;        break;
        case WALL_XREPEAT:    labelNum = w.xrepeat;    break;
        case WALL_YREPEAT:    labelNum = w.yrepeat;    break;
        case WALL_XPANNING:   labelNum = w.xpanning;   break;
        case WALL_YPANNING:   labelNum = w.ypanning;   break;

        case WALL_LOTAG:  labelNum =  (int16_t)w.lotag; break;
        case WALL_HITAG:  labelNum =  (int16_t)w.hitag; break;
        case WALL_ULOTAG: labelNum = (uint16_t)w.lotag; break;
        case WALL_UHITAG: labelNum = (uint16_t)w.hitag; break;

        case WALL_EXTRA: labelNum = w.extra; break;
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            labelNum = w.blend;
#else
            labelNum = wallext[wallNum].blend;
#endif
            break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetWall(int const wallNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return;
    }

    auto &w = wall[wallNum];

    switch (labelNum)
    {
        case WALL_X:          w.x          = newValue; break;
        case WALL_Y:          w.y          = newValue; break;
        case WALL_POINT2:     w.point2     = newValue; break;
        case WALL_NEXTWALL:   w.nextwall   = newValue; break;
        case WALL_NEXTSECTOR: w.nextsector = newValue; break;
        case WALL_CSTAT:      w.cstat      = newValue; break;
        case WALL_PICNUM:     w.picnum     = newValue; break;
        case WALL_OVERPICNUM: w.overpicnum = newValue; break;
        case WALL_SHADE:      w.shade      = newValue; break;
        case WALL_PAL:        w.pal        = newValue; break;
        case WALL_XREPEAT:    w.xrepeat    = newValue; break;
        case WALL_YREPEAT:    w.yrepeat    = newValue; break;
        case WALL_XPANNING:   w.xpanning   = newValue; break;
        case WALL_YPANNING:   w.ypanning   = newValue; break;

        case WALL_LOTAG:  w.lotag =  (int16_t)newValue; break;
        case WALL_HITAG:  w.hitag =  (int16_t)newValue; break;
        case WALL_ULOTAG: w.lotag = (uint16_t)newValue; break;
        case WALL_UHITAG: w.hitag = (uint16_t)newValue; break;

        case WALL_EXTRA: w.extra = newValue; break;
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            w.blend = newValue;
#else
            wallext[wallNum].blend = newValue;
#endif
            break;
    }

}

int32_t __fastcall VM_GetSector(int const sectNum, int labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return -1;
    }

    auto const &s = *(usectortype *)&sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_WALLPTR:  labelNum = s.wallptr;  break;
        case SECTOR_WALLNUM:  labelNum = s.wallnum;  break;

        case SECTOR_CEILINGZ: labelNum = s.ceilingz; break;
        case SECTOR_FLOORZ:   labelNum = s.floorz;   break;

        case SECTOR_CEILINGZVEL:
            labelNum = (GetAnimationGoal(&s.ceilingz) == -1) ? 0 : s.extra; break;
        case SECTOR_CEILINGZGOAL:
            labelNum = GetAnimationGoal(&s.ceilingz); break;

        case SECTOR_FLOORZVEL:
            labelNum = (GetAnimationGoal(&s.floorz) == -1) ? 0 : s.extra; break;

        case SECTOR_FLOORZGOAL:
            labelNum = GetAnimationGoal(&s.floorz); break;

        case SECTOR_CEILINGSTAT:     labelNum = s.ceilingstat;     break;
        case SECTOR_FLOORSTAT:       labelNum = s.floorstat;       break;
        case SECTOR_CEILINGPICNUM:   labelNum = s.ceilingpicnum;   break;
        case SECTOR_CEILINGSLOPE:    labelNum = s.ceilingheinum;   break;
        case SECTOR_CEILINGSHADE:    labelNum = s.ceilingshade;    break;
        case SECTOR_CEILINGPAL:      labelNum = s.ceilingpal;      break;
        case SECTOR_CEILINGXPANNING: labelNum = s.ceilingxpanning; break;
        case SECTOR_CEILINGYPANNING: labelNum = s.ceilingypanning; break;
        case SECTOR_FLOORPICNUM:     labelNum = s.floorpicnum;     break;
        case SECTOR_FLOORSLOPE:      labelNum = s.floorheinum;     break;
        case SECTOR_FLOORSHADE:      labelNum = s.floorshade;      break;
        case SECTOR_FLOORPAL:        labelNum = s.floorpal;        break;
        case SECTOR_FLOORXPANNING:   labelNum = s.floorxpanning;   break;
        case SECTOR_FLOORYPANNING:   labelNum = s.floorypanning;   break;
        case SECTOR_VISIBILITY:      labelNum = s.visibility;      break;
        case SECTOR_FOGPAL:          labelNum = s.fogpal;          break;
        case SECTOR_LOTAG:           labelNum = (int16_t)s.lotag;  break;
        case SECTOR_HITAG:           labelNum = (int16_t)s.hitag;  break;
        case SECTOR_ULOTAG:          labelNum = (uint16_t)s.lotag; break;
        case SECTOR_UHITAG:          labelNum = (uint16_t)s.hitag; break;
        case SECTOR_EXTRA:           labelNum = s.extra;           break;

        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
#ifdef YAX_ENABLE
            labelNum = yax_getbunch(sectNum, labelNum == SECTOR_FLOORBUNCH);
#else
            labelNum = -1;
#endif
            break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetSector(int const sectNum, int const labelNum, int32_t newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return;
    }

    auto &s = sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_WALLPTR:
            setfirstwall(sectNum, newValue); break;

        case SECTOR_WALLNUM:  s.wallnum  = newValue; break;
        case SECTOR_CEILINGZ: s.ceilingz = newValue; break;
        case SECTOR_FLOORZ:   s.floorz   = newValue; break;

        case SECTOR_CEILINGZVEL:
            s.extra = newValue;
            if ((newValue = GetAnimationGoal(&s.ceilingz)) != -1)
            {
        case SECTOR_CEILINGZGOAL:
                SetAnimation(sectNum, &s.ceilingz, newValue, s.extra);
            }
            break;

        case SECTOR_FLOORZVEL:
            s.extra = newValue;
            if ((newValue = GetAnimationGoal(&s.floorz)) != -1)
            {
        case SECTOR_FLOORZGOAL:
                SetAnimation(sectNum, &s.floorz, newValue, s.extra);
            }
            break;

        case SECTOR_CEILINGSTAT:     s.ceilingstat     = newValue; break;
        case SECTOR_FLOORSTAT:       s.floorstat       = newValue; break;
        case SECTOR_CEILINGPICNUM:   s.ceilingpicnum   = newValue; break;
        case SECTOR_CEILINGSLOPE:    s.ceilingheinum   = newValue; break;
        case SECTOR_CEILINGSHADE:    s.ceilingshade    = newValue; break;
        case SECTOR_CEILINGPAL:      s.ceilingpal      = newValue; break;
        case SECTOR_CEILINGXPANNING: s.ceilingxpanning = newValue; break;
        case SECTOR_CEILINGYPANNING: s.ceilingypanning = newValue; break;
        case SECTOR_FLOORPICNUM:     s.floorpicnum     = newValue; break;
        case SECTOR_FLOORSLOPE:      s.floorheinum     = newValue; break;
        case SECTOR_FLOORSHADE:      s.floorshade      = newValue; break;
        case SECTOR_FLOORPAL:        s.floorpal        = newValue; break;
        case SECTOR_FLOORXPANNING:   s.floorxpanning   = newValue; break;
        case SECTOR_FLOORYPANNING:   s.floorypanning   = newValue; break;
        case SECTOR_VISIBILITY:      s.visibility      = newValue; break;
        case SECTOR_FOGPAL:          s.fogpal          = newValue; break;
        case SECTOR_LOTAG:           s.lotag =  (int16_t)newValue; break;
        case SECTOR_HITAG:           s.hitag =  (int16_t)newValue; break;
        case SECTOR_ULOTAG:          s.lotag = (uint16_t)newValue; break;
        case SECTOR_UHITAG:          s.hitag = (uint16_t)newValue; break;
        case SECTOR_EXTRA:           s.extra           = newValue; break;

        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
            break;
    }
}

void __fastcall VM_SetSprite(int const spriteNum, int const labelNum, int const lParm2, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned)spriteNum >= MAXSPRITES) ||
        (ActorLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for sprite %d\n", ActorLabels[labelNum].name, lParm2, spriteNum);
        return;
    }

    auto &s   = sprite[spriteNum];
    auto &a   = actor[spriteNum];
    auto &ext = spriteext[spriteNum];

    switch (labelNum)
    {
        case ACTOR_X:        s.x        = newValue; break;
        case ACTOR_Y:        s.y        = newValue; break;
        case ACTOR_Z:        s.z        = newValue; break;
        case ACTOR_CSTAT:    s.cstat    = newValue; break;
        case ACTOR_PICNUM:   s.picnum   = newValue; break;
        case ACTOR_SHADE:    s.shade    = newValue; break;
        case ACTOR_PAL:      s.pal      = newValue; break;
        case ACTOR_CLIPDIST: s.clipdist = newValue; break;
        case ACTOR_DETAIL:   s.blend    = newValue; break;
        case ACTOR_XREPEAT:  s.xrepeat  = newValue; break;
        case ACTOR_YREPEAT:  s.yrepeat  = newValue; break;
        case ACTOR_XOFFSET:  s.xoffset  = newValue; break;
        case ACTOR_YOFFSET:  s.yoffset  = newValue; break;
        case ACTOR_ANG:      s.ang      = newValue; break;
        case ACTOR_OWNER:    s.owner    = newValue; break;
        case ACTOR_XVEL:     s.xvel     = newValue; break;
        case ACTOR_YVEL:     s.yvel     = newValue; break;
        case ACTOR_ZVEL:     s.zvel     = newValue; break;
        case ACTOR_EXTRA:    s.extra    = newValue; break;

        case ACTOR_LOTAG:  s.lotag =  (int16_t)newValue; break;
        case ACTOR_HITAG:  s.hitag =  (int16_t)newValue; break;
        case ACTOR_ULOTAG: s.lotag = (uint16_t)newValue; break;
        case ACTOR_UHITAG: s.hitag = (uint16_t)newValue; break;

        case ACTOR_SECTNUM: changespritesect(spriteNum, newValue); break;
        case ACTOR_STATNUM: changespritestat(spriteNum, newValue); break;

        case ACTOR_HTCGG:         a.cgg         = newValue; break;
        case ACTOR_HTPICNUM:      a.picnum      = newValue; break;
        case ACTOR_HTANG:         a.ang         = newValue; break;
        case ACTOR_HTEXTRA:       a.extra       = newValue; break;
        case ACTOR_HTOWNER:       a.owner       = newValue; break;
        case ACTOR_HTMOVFLAG:     a.movflag     = newValue; break;
        case ACTOR_HTTEMPANG:     a.tempang     = newValue; break;
        case ACTOR_HTSTAYPUT:     a.stayput     = newValue; break;
        case ACTOR_HTDISPICNUM:   a.dispicnum   = newValue; break;
        case ACTOR_HTTIMETOSLEEP: a.timetosleep = newValue; break;
        case ACTOR_HTFLOORZ:      a.floorz      = newValue; break;
        case ACTOR_HTCEILINGZ:    a.ceilingz    = newValue; break;
        case ACTOR_HTLASTVX:      a.lastv.x     = newValue; break;
        case ACTOR_HTLASTVY:      a.lastv.y     = newValue; break;
        case ACTOR_HTBPOSX:       a.bpos.x      = newValue; break;
        case ACTOR_HTBPOSY:       a.bpos.y      = newValue; break;
        case ACTOR_HTBPOSZ:       a.bpos.z      = newValue; break;
        case ACTOR_HTFLAGS:       a.flags       = newValue; break;

        case ACTOR_HTG_T: a.t_data[lParm2] = newValue; break;

        case ACTOR_ANGOFF:   ext.angoff   = newValue; break;
        case ACTOR_PITCH:    ext.pitch    = newValue; break;
        case ACTOR_ROLL:     ext.roll     = newValue; break;
        case ACTOR_MDXOFF:   ext.offset.x = newValue; break;
        case ACTOR_MDYOFF:   ext.offset.y = newValue; break;
        case ACTOR_MDZOFF:   ext.offset.z = newValue; break;
        case ACTOR_MDFLAGS:  ext.flags    = newValue; break;
        case ACTOR_XPANNING: ext.xpanning = newValue; break;
        case ACTOR_YPANNING: ext.ypanning = newValue; break;

        case ACTOR_ALPHA: ext.alpha = (float)newValue * (1.f / 255.0f); break;

        default: EDUKE32_UNREACHABLE_SECTION(break);
    }
}


int32_t __fastcall VM_GetSprite(int const spriteNum, int32_t labelNum, int const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned) spriteNum >= MAXSPRITES) ||
        (ActorLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) ActorLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for sprite %d\n", ActorLabels[labelNum].name, lParm2, spriteNum);
        return -1;
    }

    auto const &s   = sprite[spriteNum];
    auto const &a   = actor[spriteNum];
    auto const &ext = spriteext[spriteNum];

    switch (labelNum)
    {
        case ACTOR_X:        labelNum = s.x;        break;
        case ACTOR_Y:        labelNum = s.y;        break;
        case ACTOR_Z:        labelNum = s.z;        break;
        case ACTOR_CSTAT:    labelNum = s.cstat;    break;
        case ACTOR_PICNUM:   labelNum = s.picnum;   break;
        case ACTOR_SHADE:    labelNum = s.shade;    break;
        case ACTOR_PAL:      labelNum = s.pal;      break;
        case ACTOR_CLIPDIST: labelNum = s.clipdist; break;
        case ACTOR_DETAIL:   labelNum = s.blend;    break;
        case ACTOR_XREPEAT:  labelNum = s.xrepeat;  break;
        case ACTOR_YREPEAT:  labelNum = s.yrepeat;  break;
        case ACTOR_XOFFSET:  labelNum = s.xoffset;  break;
        case ACTOR_YOFFSET:  labelNum = s.yoffset;  break;
        case ACTOR_SECTNUM:  labelNum = s.sectnum;  break;
        case ACTOR_STATNUM:  labelNum = s.statnum;  break;
        case ACTOR_ANG:      labelNum = s.ang;      break;
        case ACTOR_OWNER:    labelNum = s.owner;    break;
        case ACTOR_XVEL:     labelNum = s.xvel;     break;
        case ACTOR_YVEL:     labelNum = s.yvel;     break;
        case ACTOR_ZVEL:     labelNum = s.zvel;     break;
        case ACTOR_EXTRA:    labelNum = s.extra;    break;

        case ACTOR_LOTAG:  labelNum =  (int16_t)s.lotag; break;
        case ACTOR_HITAG:  labelNum =  (int16_t)s.hitag; break;
        case ACTOR_ULOTAG: labelNum = (uint16_t)s.lotag; break;
        case ACTOR_UHITAG: labelNum = (uint16_t)s.hitag; break;

        case ACTOR_HTCGG:         labelNum = a.cgg;         break;
        case ACTOR_HTPICNUM:      labelNum = a.picnum;      break;
        case ACTOR_HTANG:         labelNum = a.ang;         break;
        case ACTOR_HTEXTRA:       labelNum = a.extra;       break;
        case ACTOR_HTOWNER:       labelNum = a.owner;       break;
        case ACTOR_HTMOVFLAG:     labelNum = a.movflag;     break;
        case ACTOR_HTTEMPANG:     labelNum = a.tempang;     break;
        case ACTOR_HTSTAYPUT:     labelNum = a.stayput;     break;
        case ACTOR_HTDISPICNUM:   labelNum = a.dispicnum;   break;
        case ACTOR_HTTIMETOSLEEP: labelNum = a.timetosleep; break;
        case ACTOR_HTFLOORZ:      labelNum = a.floorz;      break;
        case ACTOR_HTCEILINGZ:    labelNum = a.ceilingz;    break;
        case ACTOR_HTLASTVX:      labelNum = a.lastv.x;     break;
        case ACTOR_HTLASTVY:      labelNum = a.lastv.y;     break;
        case ACTOR_HTBPOSX:       labelNum = a.bpos.x;      break;
        case ACTOR_HTBPOSY:       labelNum = a.bpos.y;      break;
        case ACTOR_HTBPOSZ:       labelNum = a.bpos.z;      break;
        case ACTOR_HTFLAGS:       labelNum = a.flags;       break;

        case ACTOR_HTG_T: labelNum = a.t_data[lParm2]; break;

        case ACTOR_ANGOFF:   labelNum = ext.angoff;   break;
        case ACTOR_PITCH:    labelNum = ext.pitch;    break;
        case ACTOR_ROLL:     labelNum = ext.roll;     break;
        case ACTOR_MDXOFF:   labelNum = ext.offset.x; break;
        case ACTOR_MDYOFF:   labelNum = ext.offset.y; break;
        case ACTOR_MDZOFF:   labelNum = ext.offset.z; break;
        case ACTOR_MDFLAGS:  labelNum = ext.flags;    break;
        case ACTOR_XPANNING: labelNum = ext.xpanning; break;
        case ACTOR_YPANNING: labelNum = ext.ypanning; break;

        case ACTOR_ALPHA: labelNum = (uint8_t)(ext.alpha * 255.0f); break;
        case ACTOR_ISVALID: labelNum = (sprite[spriteNum].statnum != MAXSTATUS); break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

int32_t __fastcall VM_GetTsprite(int const spriteNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES || spriteext[spriteNum].tspr == NULL))
    {
        CON_ERRPRINTF("invalid sprite %d or no tsprite\n", spriteNum);
        return -1;
    }

    auto const &t = *spriteext[spriteNum].tspr;

    switch (labelNum)
    {
        case ACTOR_X:        labelNum = t.x;        break;
        case ACTOR_Y:        labelNum = t.y;        break;
        case ACTOR_Z:        labelNum = t.z;        break;
        case ACTOR_CSTAT:    labelNum = t.cstat;    break;
        case ACTOR_PICNUM:   labelNum = t.picnum;   break;
        case ACTOR_SHADE:    labelNum = t.shade;    break;
        case ACTOR_PAL:      labelNum = t.pal;      break;
        case ACTOR_CLIPDIST: labelNum = t.clipdist; break;
        case ACTOR_DETAIL:   labelNum = t.blend;    break;
        case ACTOR_XREPEAT:  labelNum = t.xrepeat;  break;
        case ACTOR_YREPEAT:  labelNum = t.yrepeat;  break;
        case ACTOR_XOFFSET:  labelNum = t.xoffset;  break;
        case ACTOR_YOFFSET:  labelNum = t.yoffset;  break;
        case ACTOR_SECTNUM:  labelNum = t.sectnum;  break;
        case ACTOR_STATNUM:  labelNum = t.statnum;  break;
        case ACTOR_ANG:      labelNum = t.ang;      break;
        case ACTOR_OWNER:    labelNum = t.owner;    break;
        case ACTOR_XVEL:     labelNum = t.xvel;     break;
        case ACTOR_YVEL:     labelNum = t.yvel;     break;
        case ACTOR_ZVEL:     labelNum = t.zvel;     break;
        case ACTOR_EXTRA:    labelNum = t.extra;    break;

        case ACTOR_LOTAG:  labelNum =  (int16_t)t.lotag; break;
        case ACTOR_HITAG:  labelNum =  (int16_t)t.hitag; break;
        case ACTOR_ULOTAG: labelNum = (uint16_t)t.lotag; break;
        case ACTOR_UHITAG: labelNum = (uint16_t)t.hitag; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetTsprite(int const spriteNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum >= MAXSPRITES || spriteext[spriteNum].tspr == NULL))
    {
        CON_ERRPRINTF("invalid sprite %d or no tsprite\n", spriteNum);
        return;
    }

    auto &t = *spriteext[spriteNum].tspr;

    switch (labelNum)
    {
        case ACTOR_X:        t.x        = newValue; break;
        case ACTOR_Y:        t.y        = newValue; break;
        case ACTOR_Z:        t.z        = newValue; break;
        case ACTOR_CSTAT:    t.cstat    = newValue; break;
        case ACTOR_PICNUM:   t.picnum   = newValue; break;
        case ACTOR_SHADE:    t.shade    = newValue; break;
        case ACTOR_PAL:      t.pal      = newValue; break;
        case ACTOR_CLIPDIST: t.clipdist = newValue; break;
        case ACTOR_DETAIL:   t.blend    = newValue; break;
        case ACTOR_XREPEAT:  t.xrepeat  = newValue; break;
        case ACTOR_YREPEAT:  t.yrepeat  = newValue; break;
        case ACTOR_XOFFSET:  t.xoffset  = newValue; break;
        case ACTOR_YOFFSET:  t.yoffset  = newValue; break;
        case ACTOR_SECTNUM:  t.sectnum  = newValue; break;
        case ACTOR_STATNUM:  t.statnum  = newValue; break;
        case ACTOR_ANG:      t.ang      = newValue; break;
        case ACTOR_OWNER:    t.owner    = newValue; break;
        case ACTOR_XVEL:     t.xvel     = newValue; break;
        case ACTOR_YVEL:     t.yvel     = newValue; break;
        case ACTOR_ZVEL:     t.zvel     = newValue; break;
        case ACTOR_EXTRA:    t.extra    = newValue; break;

        case ACTOR_LOTAG:    t.lotag =  (int16_t)newValue; break;
        case ACTOR_HITAG:    t.hitag =  (int16_t)newValue; break;
        case ACTOR_ULOTAG:   t.lotag = (uint16_t)newValue; break;
        case ACTOR_UHITAG:   t.hitag = (uint16_t)newValue; break;
    }
}

int32_t __fastcall VM_GetProjectile(int const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES || g_tile[tileNum].proj == NULL))
    {
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return -1;
    }

    auto const &p = *g_tile[tileNum].proj;

    switch (labelNum)
    {
        case PROJ_BOUNCES:     labelNum = p.bounces;    break;
        case PROJ_BSOUND:      labelNum = p.bsound;     break;
        case PROJ_CLIPDIST:    labelNum = p.clipdist;   break;
        case PROJ_CSTAT:       labelNum = p.cstat;      break;
        case PROJ_DECAL:       labelNum = p.decal;      break;
        case PROJ_DROP:        labelNum = p.drop;       break;
        case PROJ_EXTRA:       labelNum = p.extra;      break;
        case PROJ_EXTRA_RAND:  labelNum = p.extra_rand; break;
        case PROJ_FLASH_COLOR: labelNum = p.flashcolor; break;
        case PROJ_HITRADIUS:   labelNum = p.hitradius;  break;
        case PROJ_ISOUND:      labelNum = p.isound;     break;
        case PROJ_MOVECNT:     labelNum = p.movecnt;    break;
        case PROJ_OFFSET:      labelNum = p.offset;     break;
        case PROJ_PAL:         labelNum = p.pal;        break;
        case PROJ_RANGE:       labelNum = p.range;      break;
        case PROJ_SHADE:       labelNum = p.shade;      break;
        case PROJ_SOUND:       labelNum = p.sound;      break;
        case PROJ_SPAWNS:      labelNum = p.spawns;     break;
        case PROJ_SXREPEAT:    labelNum = p.sxrepeat;   break;
        case PROJ_SYREPEAT:    labelNum = p.syrepeat;   break;
        case PROJ_TNUM:        labelNum = p.tnum;       break;
        case PROJ_TOFFSET:     labelNum = p.toffset;    break;
        case PROJ_TRAIL:       labelNum = p.trail;      break;
        case PROJ_TXREPEAT:    labelNum = p.txrepeat;   break;
        case PROJ_TYREPEAT:    labelNum = p.tyrepeat;   break;
        case PROJ_USERDATA:    labelNum = p.userdata;   break;
        case PROJ_VEL:         labelNum = p.vel;        break;
        case PROJ_WORKSLIKE:   labelNum = p.workslike;  break;
        case PROJ_XREPEAT:     labelNum = p.xrepeat;    break;
        case PROJ_YREPEAT:     labelNum = p.yrepeat;    break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetProjectile(int const tileNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) tileNum >= MAXTILES || g_tile[tileNum].proj == NULL))
    {
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return;
    }

    auto &p = *g_tile[tileNum].proj;

    switch (labelNum)
    {
        case PROJ_BOUNCES:     p.bounces    = newValue; break;
        case PROJ_BSOUND:      p.bsound     = newValue; break;
        case PROJ_CLIPDIST:    p.clipdist   = newValue; break;
        case PROJ_CSTAT:       p.cstat      = newValue; break;
        case PROJ_DECAL:       p.decal      = newValue; break;
        case PROJ_DROP:        p.drop       = newValue; break;
        case PROJ_EXTRA:       p.extra      = newValue; break;
        case PROJ_EXTRA_RAND:  p.extra_rand = newValue; break;
        case PROJ_FLASH_COLOR: p.flashcolor = newValue; break;
        case PROJ_HITRADIUS:   p.hitradius  = newValue; break;
        case PROJ_ISOUND:      p.isound     = newValue; break;
        case PROJ_MOVECNT:     p.movecnt    = newValue; break;
        case PROJ_OFFSET:      p.offset     = newValue; break;
        case PROJ_PAL:         p.pal        = newValue; break;
        case PROJ_RANGE:       p.range      = newValue; break;
        case PROJ_SHADE:       p.shade      = newValue; break;
        case PROJ_SOUND:       p.sound      = newValue; break;
        case PROJ_SPAWNS:      p.spawns     = newValue; break;
        case PROJ_SXREPEAT:    p.sxrepeat   = newValue; break;
        case PROJ_SYREPEAT:    p.syrepeat   = newValue; break;
        case PROJ_TNUM:        p.tnum       = newValue; break;
        case PROJ_TOFFSET:     p.toffset    = newValue; break;
        case PROJ_TRAIL:       p.trail      = newValue; break;
        case PROJ_TXREPEAT:    p.txrepeat   = newValue; break;
        case PROJ_TYREPEAT:    p.tyrepeat   = newValue; break;
        case PROJ_USERDATA:    p.userdata   = newValue; break;
        case PROJ_VEL:         p.vel        = newValue; break;
        case PROJ_WORKSLIKE:   p.workslike  = newValue; break;
        case PROJ_XREPEAT:     p.xrepeat    = newValue; break;
        case PROJ_YREPEAT:     p.yrepeat    = newValue; break;
    }
}

int32_t __fastcall VM_GetTileData(int const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
        CON_ERRPRINTF("invalid tile %d\n", tileNum);
        return -1;
    }

    auto const &p = picanm[tileNum];

    switch (labelNum)
    {
        case TILEDATA_XSIZE: labelNum = tilesiz[tileNum].x; break;
        case TILEDATA_YSIZE: labelNum = tilesiz[tileNum].y; break;

        case TILEDATA_GAMEFLAGS: labelNum = g_tile[tileNum].flags; break;

        case TILEDATA_ANIMFRAMES: labelNum = p.num;  break;
        case TILEDATA_XOFFSET:    labelNum = p.xofs; break;
        case TILEDATA_YOFFSET:    labelNum = p.yofs; break;

        case TILEDATA_ANIMSPEED: labelNum = (p.sf & PICANM_ANIMSPEED_MASK); break;
        case TILEDATA_ANIMTYPE:  labelNum = (p.sf & PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetTileData(int const tileNum, int const labelNum, int32_t newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
        CON_ERRPRINTF("invalid tile %d\n", tileNum);
        return;
    }

    auto &p = picanm[tileNum];

    switch (labelNum)
    {
        //case TILEDATA_XSIZE: tilesiz[tileNum].x = newValue; break;
        //case TILEDATA_YSIZE: tilesiz[tileNum].y = newValue; break;

        case TILEDATA_GAMEFLAGS: g_tile[tileNum].flags = newValue; break;

        case TILEDATA_ANIMFRAMES: p.num  = newValue; break;
        case TILEDATA_XOFFSET:    p.xofs = newValue; break;
        case TILEDATA_YOFFSET:    p.yofs = newValue; break;

        case TILEDATA_ANIMSPEED: p.sf = (p.sf & ~PICANM_ANIMSPEED_MASK) | (newValue & PICANM_ANIMSPEED_MASK); break;
        case TILEDATA_ANIMTYPE:  p.sf = (p.sf & ~PICANM_ANIMTYPE_MASK) | ((newValue << PICANM_ANIMTYPE_SHIFT) & PICANM_ANIMTYPE_MASK); break;
    }
}

int32_t __fastcall VM_GetPalData(int const palNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)palNum >= MAXPALOOKUPS))
    {
        CON_ERRPRINTF("invalid palette %d\n", palNum);
        return -1;
    }

    switch (labelNum)
    {
        case PALDATA_NOFLOORPAL: labelNum = g_noFloorPal[palNum]; break;
        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

#endif
