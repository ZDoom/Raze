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
int32_t __fastcall VM_GetUserdef(int32_t labelNum);
void __fastcall VM_SetUserdef(int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetActiveProjectile(int32_t const spriteNum, int32_t labelNum);
void __fastcall VM_SetActiveProjectile(int32_t const spriteNum, int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetPlayer(int32_t const playerNum, int32_t labelNum, int32_t const lParm2);
void __fastcall VM_SetPlayer(int32_t const playerNum, int32_t const labelNum, int32_t const lParm2, int32_t const iSet);
int32_t __fastcall VM_GetPlayerInput(int32_t const playerNum, int32_t labelNum);
void __fastcall VM_SetPlayerInput(int32_t const playerNum, int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetWall(int32_t const wallNum, int32_t labelNum);
void __fastcall VM_SetWall(int32_t const wallNum, int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetSector(int32_t const sectNum, int32_t labelNum);
void __fastcall VM_SetSector(int32_t const sectNum, int32_t const labelNum, int32_t iSet);
int32_t __fastcall VM_GetSprite(int32_t const spriteNum, int32_t labelNum, int32_t const lParm2);
void __fastcall VM_SetSprite(int32_t const spriteNum, int32_t const labelNum, int32_t const lParm2, int32_t const iSet);
int32_t __fastcall VM_GetTsprite(int32_t const spriteNum, int32_t labelNum);
void __fastcall VM_SetTsprite(int32_t const spriteNum, int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetProjectile(int32_t const tileNum, int32_t labelNum);
void __fastcall VM_SetProjectile(int32_t const tileNum, int32_t const labelNum, int32_t const iSet);
int32_t __fastcall VM_GetTileData(int32_t const tileNum, int32_t labelNum);
int32_t __fastcall VM_GetPalData(int32_t const palNum, int32_t labelNum);
#else
int32_t __fastcall VM_GetUserdef(int32_t labelNum)
{
    switch (labelNum)
    {
        case USERDEFS_GOD: labelNum = ud.god; break;
        case USERDEFS_WARP_ON: labelNum = ud.warp_on; break;
        case USERDEFS_CASHMAN: labelNum = ud.cashman; break;
        case USERDEFS_EOG: labelNum = ud.eog; break;
        case USERDEFS_SHOWALLMAP: labelNum = ud.showallmap; break;
        case USERDEFS_SHOW_HELP: labelNum = ud.show_help; break;
        case USERDEFS_SCROLLMODE: labelNum = ud.scrollmode; break;
        case USERDEFS_CLIPPING: labelNum = ud.noclip; break;
        //  case USERDEFS_USER_NAME: labelNum= ud.user_name[MAXPLAYERS][32]; break;
        //  case USERDEFS_RIDECULE: labelNum= ud.ridecule; break;
        //  case USERDEFS_PWLOCKOUT: labelNum= ud.pwlockout; break;
        //  case USERDEFS_RTSNAME: labelNum= ud.rtsname; break;
        case USERDEFS_OVERHEAD_ON: labelNum = ud.overhead_on; break;
        case USERDEFS_LAST_OVERHEAD: labelNum = ud.last_overhead; break;
        case USERDEFS_SHOWWEAPONS: labelNum = ud.showweapons; break;
        case USERDEFS_PAUSE_ON: labelNum = ud.pause_on; break;
        case USERDEFS_FROM_BONUS: labelNum = ud.from_bonus; break;
        case USERDEFS_CAMERASPRITE: labelNum = ud.camerasprite; break;
        case USERDEFS_LAST_CAMSPRITE: labelNum = ud.last_camsprite; break;
        case USERDEFS_LAST_LEVEL: labelNum = ud.last_level; break;
        case USERDEFS_SECRETLEVEL: labelNum = ud.secretlevel; break;
        case USERDEFS_CONST_VISIBILITY: labelNum = ud.const_visibility; break;
        case USERDEFS_UW_FRAMERATE: labelNum = ud.uw_framerate; break;
        case USERDEFS_CAMERA_TIME: labelNum = ud.camera_time; break;
        case USERDEFS_FOLFVEL: labelNum = ud.folfvel; break;
        case USERDEFS_FOLAVEL: labelNum = ud.folavel; break;
        case USERDEFS_FOLX: labelNum = ud.folx; break;
        case USERDEFS_FOLY: labelNum = ud.foly; break;
        case USERDEFS_FOLA: labelNum = ud.fola; break;
        case USERDEFS_RECCNT: labelNum = ud.reccnt; break;
        case USERDEFS_ENTERED_NAME: labelNum = ud.entered_name; break;
        case USERDEFS_SCREEN_TILTING: labelNum = ud.screen_tilting; break;
        case USERDEFS_SHADOWS: labelNum = ud.shadows; break;
        case USERDEFS_FTA_ON: labelNum = ud.fta_on; break;
        case USERDEFS_EXECUTIONS: labelNum = ud.executions; break;
        case USERDEFS_AUTO_RUN: labelNum = ud.auto_run; break;
        case USERDEFS_COORDS: labelNum = ud.coords; break;
        case USERDEFS_TICKRATE: labelNum = ud.showfps; break;
        case USERDEFS_M_COOP: labelNum = ud.m_coop; break;
        case USERDEFS_COOP: labelNum = ud.coop; break;
        case USERDEFS_SCREEN_SIZE: labelNum = ud.screen_size; break;
        case USERDEFS_LOCKOUT: labelNum = ud.lockout; break;
        case USERDEFS_CROSSHAIR: labelNum = ud.crosshair; break;
        case USERDEFS_PLAYERAI: labelNum = ud.playerai; break;
        case USERDEFS_RESPAWN_MONSTERS: labelNum = ud.respawn_monsters; break;
        case USERDEFS_RESPAWN_ITEMS: labelNum = ud.respawn_items; break;
        case USERDEFS_RESPAWN_INVENTORY: labelNum = ud.respawn_inventory; break;
        case USERDEFS_RECSTAT: labelNum = ud.recstat; break;
        case USERDEFS_MONSTERS_OFF: labelNum = ud.monsters_off; break;
        case USERDEFS_BRIGHTNESS: labelNum = ud.brightness; break;
        case USERDEFS_M_RESPAWN_ITEMS: labelNum = ud.m_respawn_items; break;
        case USERDEFS_M_RESPAWN_MONSTERS: labelNum = ud.m_respawn_monsters; break;
        case USERDEFS_M_RESPAWN_INVENTORY: labelNum = ud.m_respawn_inventory; break;
        case USERDEFS_M_RECSTAT: labelNum = ud.m_recstat; break;
        case USERDEFS_M_MONSTERS_OFF: labelNum = ud.m_monsters_off; break;
        case USERDEFS_DETAIL: labelNum = ud.detail; break;
        case USERDEFS_M_FFIRE: labelNum = ud.m_ffire; break;
        case USERDEFS_FFIRE: labelNum = ud.ffire; break;
        case USERDEFS_M_PLAYER_SKILL: labelNum = ud.m_player_skill; break;
        case USERDEFS_M_LEVEL_NUMBER: labelNum = ud.m_level_number; break;
        case USERDEFS_M_VOLUME_NUMBER: labelNum = ud.m_volume_number; break;
        case USERDEFS_MULTIMODE: labelNum = ud.multimode; break;
        case USERDEFS_PLAYER_SKILL: labelNum = ud.player_skill; break;
        case USERDEFS_LEVEL_NUMBER: labelNum = ud.level_number; break;
        case USERDEFS_VOLUME_NUMBER: labelNum = ud.volume_number; break;
        case USERDEFS_M_MARKER: labelNum = ud.m_marker; break;
        case USERDEFS_MARKER: labelNum = ud.marker; break;
        case USERDEFS_MOUSEFLIP: labelNum = ud.mouseflip; break;
        case USERDEFS_STATUSBARSCALE: labelNum = ud.statusbarscale; break;
        case USERDEFS_DRAWWEAPON: labelNum = ud.drawweapon; break;
        case USERDEFS_MOUSEAIMING: labelNum = ud.mouseaiming; break;
        case USERDEFS_WEAPONSWITCH: labelNum = ud.weaponswitch; break;
        case USERDEFS_DEMOCAMS: labelNum = ud.democams; break;
        case USERDEFS_COLOR: labelNum = ud.color; break;
        case USERDEFS_MSGDISPTIME: labelNum = ud.msgdisptime; break;
        case USERDEFS_STATUSBARMODE: labelNum = ud.statusbarmode; break;
        case USERDEFS_M_NOEXITS: labelNum = ud.m_noexits; break;
        case USERDEFS_NOEXITS: labelNum = ud.noexits; break;
        case USERDEFS_AUTOVOTE: labelNum = ud.autovote; break;
        case USERDEFS_AUTOMSG: labelNum = ud.automsg; break;
        case USERDEFS_IDPLAYERS: labelNum = ud.idplayers; break;
        case USERDEFS_TEAM: labelNum = ud.team; break;
        case USERDEFS_VIEWBOB: labelNum = ud.viewbob; break;
        case USERDEFS_WEAPONSWAY: labelNum = ud.weaponsway; break;
        case USERDEFS_ANGLEINTERPOLATION: labelNum = ud.angleinterpolation; break;
        case USERDEFS_OBITUARIES: labelNum = ud.obituaries; break;
        case USERDEFS_LEVELSTATS: labelNum = ud.levelstats; break;
        case USERDEFS_CROSSHAIRSCALE: labelNum = ud.crosshairscale; break;
        case USERDEFS_ALTHUD: labelNum = ud.althud; break;
        case USERDEFS_DISPLAY_BONUS_SCREEN: labelNum = ud.display_bonus_screen; break;
        case USERDEFS_SHOW_LEVEL_TEXT: labelNum = ud.show_level_text; break;
        case USERDEFS_WEAPONSCALE: labelNum = ud.weaponscale; break;
        case USERDEFS_TEXTSCALE: labelNum = ud.textscale; break;
        case USERDEFS_RUNKEY_MODE: labelNum = ud.runkey_mode; break;
        case USERDEFS_M_ORIGIN_X: labelNum = ud.m_origin.x; break;
        case USERDEFS_M_ORIGIN_Y: labelNum = ud.m_origin.y; break;
        case USERDEFS_PLAYERBEST: labelNum = ud.playerbest; break;
        case USERDEFS_MUSICTOGGLE: labelNum = ud.config.MusicToggle; break;
        case USERDEFS_USEVOXELS: labelNum = usevoxels; break;
        case USERDEFS_USEHIGHTILE:
#ifdef USE_OPENGL
            labelNum = usehightile;
            break;
#endif
        case USERDEFS_USEMODELS:
#ifdef USE_OPENGL
            labelNum = usemodels;
#else
            labelNum = 0;
#endif
            break;
        case USERDEFS_GAMETYPEFLAGS: labelNum = g_gametypeFlags[ud.coop]; break;
        case USERDEFS_M_GAMETYPEFLAGS: labelNum = g_gametypeFlags[ud.m_coop]; break;
        case USERDEFS_GLOBALFLAGS: labelNum = globalflags; break;
        case USERDEFS_GLOBALGAMEFLAGS: labelNum = duke3d_globalflags; break;
        case USERDEFS_VM_PLAYER: labelNum = vm.playerNum; break;
        case USERDEFS_VM_SPRITE: labelNum = vm.spriteNum; break;
        case USERDEFS_VM_DISTANCE: labelNum = vm.playerDist; break;
        case USERDEFS_SOUNDTOGGLE: labelNum = ud.config.SoundToggle; break;
        case USERDEFS_GAMETEXT_TRACKING: labelNum = MF_BluefontGame.between.x; break;
        case USERDEFS_MGAMETEXT_TRACKING: labelNum = MF_Bluefont.between.x; break;
        case USERDEFS_MENUTEXT_TRACKING: labelNum = MF_Redfont.between.x; break;
        case USERDEFS_MAXSPRITESONSCREEN: labelNum = maxspritesonscreen; break;
        case USERDEFS_SCREENAREA_X1: labelNum = ud.screenarea_x1; break;
        case USERDEFS_SCREENAREA_Y1: labelNum = ud.screenarea_y1; break;
        case USERDEFS_SCREENAREA_X2: labelNum = ud.screenarea_x2; break;
        case USERDEFS_SCREENAREA_Y2: labelNum = ud.screenarea_y2; break;
        case USERDEFS_SCREENFADE: labelNum = ud.screenfade; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetUserdef(int32_t const labelNum, int32_t const iSet)
{
    switch (labelNum)
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
        case USERDEFS_TICKRATE: ud.showfps = iSet; break;
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
        case USERDEFS_VM_PLAYER: vm.playerNum = iSet; vm.pPlayer = g_player[iSet].ps; break;
        case USERDEFS_VM_SPRITE: vm.spriteNum = iSet; vm.pSprite = &sprite[iSet]; vm.pActor = &actor[iSet];  vm.pData = &actor[iSet].t_data[0]; break;
        case USERDEFS_VM_DISTANCE: vm.playerDist = iSet; break;
        case USERDEFS_GAMETEXT_TRACKING: MF_BluefontGame.between.x = iSet; break;
        case USERDEFS_MGAMETEXT_TRACKING: MF_BluefontRed.between.x = MF_Bluefont.between.x = iSet; break;
        case USERDEFS_MENUTEXT_TRACKING: MF_Redfont.between.x = iSet; break;
        case USERDEFS_MAXSPRITESONSCREEN: maxspritesonscreen = clamp(iSet, MAXSPRITESONSCREEN>>2, MAXSPRITESONSCREEN); break;
        case USERDEFS_SCREENAREA_X1: ud.screenarea_x1 = iSet; break;
        case USERDEFS_SCREENAREA_Y1: ud.screenarea_y1 = iSet; break;
        case USERDEFS_SCREENAREA_X2: ud.screenarea_x2 = iSet; break;
        case USERDEFS_SCREENAREA_Y2: ud.screenarea_y2 = iSet; break;
        case USERDEFS_SCREENFADE: ud.screenfade = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetActiveProjectile(int32_t const spriteNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return -1;
    }

    projectile_t * const p = &SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE: labelNum = p->workslike; break;
        case PROJ_SPAWNS: labelNum = p->spawns; break;
        case PROJ_SXREPEAT: labelNum = p->sxrepeat; break;
        case PROJ_SYREPEAT: labelNum = p->syrepeat; break;
        case PROJ_SOUND: labelNum = p->sound; break;
        case PROJ_ISOUND: labelNum = p->isound; break;
        case PROJ_VEL: labelNum = p->vel; break;
        case PROJ_EXTRA: labelNum = p->extra; break;
        case PROJ_DECAL: labelNum = p->decal; break;
        case PROJ_TRAIL: labelNum = p->trail; break;
        case PROJ_TXREPEAT: labelNum = p->txrepeat; break;
        case PROJ_TYREPEAT: labelNum = p->tyrepeat; break;
        case PROJ_TOFFSET: labelNum = p->toffset; break;
        case PROJ_TNUM: labelNum = p->tnum; break;
        case PROJ_DROP: labelNum = p->drop; break;
        case PROJ_CSTAT: labelNum = p->cstat; break;
        case PROJ_CLIPDIST: labelNum = p->clipdist; break;
        case PROJ_SHADE: labelNum = p->shade; break;
        case PROJ_XREPEAT: labelNum = p->xrepeat; break;
        case PROJ_YREPEAT: labelNum = p->yrepeat; break;
        case PROJ_PAL: labelNum = p->pal; break;
        case PROJ_EXTRA_RAND: labelNum = p->extra_rand; break;
        case PROJ_HITRADIUS: labelNum = p->hitradius; break;
        case PROJ_MOVECNT: labelNum = p->movecnt; break;
        case PROJ_OFFSET: labelNum = p->offset; break;
        case PROJ_BOUNCES: labelNum = p->bounces; break;
        case PROJ_BSOUND: labelNum = p->bsound; break;
        case PROJ_RANGE: labelNum = p->range; break;
        case PROJ_FLASH_COLOR: labelNum = p->flashcolor; break;
        case PROJ_USERDATA: labelNum = p->userdata; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetActiveProjectile(int32_t const spriteNum, int32_t const labelNum, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return;
    }

    projectile_t * const p = &SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE: p->workslike = iSet; break;
        case PROJ_SPAWNS: p->spawns = iSet; break;
        case PROJ_SXREPEAT: p->sxrepeat = iSet; break;
        case PROJ_SYREPEAT: p->syrepeat = iSet; break;
        case PROJ_SOUND: p->sound = iSet; break;
        case PROJ_ISOUND: p->isound = iSet; break;
        case PROJ_VEL: p->vel = iSet; break;
        case PROJ_EXTRA: p->extra = iSet; break;
        case PROJ_DECAL: p->decal = iSet; break;
        case PROJ_TRAIL: p->trail = iSet; break;
        case PROJ_TXREPEAT: p->txrepeat = iSet; break;
        case PROJ_TYREPEAT: p->tyrepeat = iSet; break;
        case PROJ_TOFFSET: p->toffset = iSet; break;
        case PROJ_TNUM: p->tnum = iSet; break;
        case PROJ_DROP: p->drop = iSet; break;
        case PROJ_CSTAT: p->cstat = iSet; break;
        case PROJ_CLIPDIST: p->clipdist = iSet; break;
        case PROJ_SHADE: p->shade = iSet; break;
        case PROJ_XREPEAT: p->xrepeat = iSet; break;
        case PROJ_YREPEAT: p->yrepeat = iSet; break;
        case PROJ_PAL: p->pal = iSet; break;
        case PROJ_EXTRA_RAND: p->extra_rand = iSet; break;
        case PROJ_HITRADIUS: p->hitradius = iSet; break;
        case PROJ_MOVECNT: p->movecnt = iSet; break;
        case PROJ_OFFSET: p->offset = iSet; break;
        case PROJ_BOUNCES: p->bounces = iSet; break;
        case PROJ_BSOUND: p->bsound = iSet; break;
        case PROJ_RANGE: p->range = iSet; break;
        case PROJ_FLASH_COLOR: p->flashcolor = iSet; break;
        case PROJ_USERDATA: p->userdata = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetPlayer(int32_t const playerNum, int32_t labelNum, int32_t const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned) playerNum >= (unsigned) g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return -1;
    }

    DukePlayer_t *const ps = g_player[playerNum].ps;

    switch (labelNum)
    {
        case PLAYER_ZOOM: labelNum = ps->zoom; break;
        case PLAYER_EXITX: labelNum = ps->exitx; break;
        case PLAYER_EXITY: labelNum = ps->exity; break;
        case PLAYER_LOOGIEX: labelNum = ps->loogiex[lParm2]; break;
        case PLAYER_LOOGIEY: labelNum = ps->loogiey[lParm2]; break;
        case PLAYER_NUMLOOGS: labelNum = ps->numloogs; break;
        case PLAYER_LOOGCNT: labelNum = ps->loogcnt; break;
        case PLAYER_POSX: labelNum = ps->pos.x; break;
        case PLAYER_POSY: labelNum = ps->pos.y; break;
        case PLAYER_POSZ: labelNum = ps->pos.z; break;
        case PLAYER_HORIZ: labelNum = ps->horiz; break;
        case PLAYER_OHORIZ: labelNum = ps->ohoriz; break;
        case PLAYER_OHORIZOFF: labelNum = ps->ohorizoff; break;
        case PLAYER_INVDISPTIME: labelNum = ps->invdisptime; break;
        case PLAYER_BOBPOSX: labelNum = ps->bobpos.x; break;
        case PLAYER_BOBPOSY: labelNum = ps->bobpos.y; break;
        case PLAYER_OPOSX: labelNum = ps->opos.x; break;
        case PLAYER_OPOSY: labelNum = ps->opos.y; break;
        case PLAYER_OPOSZ: labelNum = ps->opos.z; break;
        case PLAYER_PYOFF: labelNum = ps->pyoff; break;
        case PLAYER_OPYOFF: labelNum = ps->opyoff; break;
        case PLAYER_POSXV: labelNum = ps->vel.x; break;
        case PLAYER_POSYV: labelNum = ps->vel.y; break;
        case PLAYER_POSZV: labelNum = ps->vel.z; break;
        case PLAYER_LAST_PISSED_TIME: labelNum = ps->last_pissed_time; break;
        case PLAYER_TRUEFZ: labelNum = ps->truefz; break;
        case PLAYER_TRUECZ: labelNum = ps->truecz; break;
        case PLAYER_PLAYER_PAR: labelNum = ps->player_par; break;
        case PLAYER_VISIBILITY: labelNum = ps->visibility; break;
        case PLAYER_BOBCOUNTER: labelNum = ps->bobcounter; break;
        case PLAYER_WEAPON_SWAY: labelNum = ps->weapon_sway; break;
        case PLAYER_PALS_TIME: labelNum = ps->pals.f; break;
        case PLAYER_RANDOMFLAMEX: labelNum = ps->randomflamex; break;
        case PLAYER_CRACK_TIME: labelNum = ps->crack_time; break;
        case PLAYER_AIM_MODE: labelNum = ps->aim_mode; break;
        case PLAYER_ANG: labelNum = ps->ang; break;
        case PLAYER_OANG: labelNum = ps->oang; break;
        case PLAYER_ANGVEL: labelNum = ps->angvel; break;
        case PLAYER_CURSECTNUM: labelNum = ps->cursectnum; break;
        case PLAYER_LOOK_ANG: labelNum = ps->look_ang; break;
        case PLAYER_LAST_EXTRA: labelNum = ps->last_extra; break;
        case PLAYER_SUBWEAPON: labelNum = ps->subweapon; break;
        case PLAYER_AMMO_AMOUNT: labelNum = ps->ammo_amount[lParm2]; break;
        case PLAYER_WACKEDBYACTOR: labelNum = ps->wackedbyactor; break;
        case PLAYER_FRAG: labelNum = ps->frag; break;
        case PLAYER_FRAGGEDSELF: labelNum = ps->fraggedself; break;
        case PLAYER_CURR_WEAPON: labelNum = ps->curr_weapon; break;
        case PLAYER_LAST_WEAPON: labelNum = ps->last_weapon; break;
        case PLAYER_TIPINCS: labelNum = ps->tipincs; break;
        case PLAYER_HORIZOFF: labelNum = ps->horizoff; break;
        case PLAYER_WANTWEAPONFIRE: labelNum = ps->wantweaponfire; break;
        case PLAYER_HOLODUKE_AMOUNT: labelNum = ps->inv_amount[GET_HOLODUKE]; break;
        case PLAYER_NEWOWNER: labelNum = ps->newowner; break;
        case PLAYER_HURT_DELAY: labelNum = ps->hurt_delay; break;
        case PLAYER_HBOMB_HOLD_DELAY: labelNum = ps->hbomb_hold_delay; break;
        case PLAYER_JUMPING_COUNTER: labelNum = ps->jumping_counter; break;
        case PLAYER_AIRLEFT: labelNum = ps->airleft; break;
        case PLAYER_KNEE_INCS: labelNum = ps->knee_incs; break;
        case PLAYER_ACCESS_INCS: labelNum = ps->access_incs; break;
        case PLAYER_FTA: labelNum = ps->fta; break;
        case PLAYER_FTQ: labelNum = ps->ftq; break;
        case PLAYER_ACCESS_WALLNUM: labelNum = ps->access_wallnum; break;
        case PLAYER_ACCESS_SPRITENUM: labelNum = ps->access_spritenum; break;
        case PLAYER_KICKBACK_PIC: labelNum = ps->kickback_pic; break;
        case PLAYER_GOT_ACCESS: labelNum = ps->got_access; break;
        case PLAYER_WEAPON_ANG: labelNum = ps->weapon_ang; break;
        case PLAYER_FIRSTAID_AMOUNT: labelNum = ps->inv_amount[GET_FIRSTAID]; break;
        case PLAYER_SOMETHINGONPLAYER: labelNum = ps->somethingonplayer; break;
        case PLAYER_ON_CRANE: labelNum = ps->on_crane; break;
        case PLAYER_I: labelNum = ps->i; break;
        case PLAYER_ONE_PARALLAX_SECTNUM: labelNum = ps->one_parallax_sectnum; break;
        case PLAYER_OVER_SHOULDER_ON: labelNum = ps->over_shoulder_on; break;
        case PLAYER_RANDOM_CLUB_FRAME: labelNum = ps->random_club_frame; break;
        case PLAYER_FIST_INCS: labelNum = ps->fist_incs; break;
        case PLAYER_ONE_EIGHTY_COUNT: labelNum = ps->one_eighty_count; break;
        case PLAYER_CHEAT_PHASE: labelNum = ps->cheat_phase; break;
        case PLAYER_DUMMYPLAYERSPRITE: labelNum = ps->dummyplayersprite; break;
        case PLAYER_EXTRA_EXTRA8: labelNum = ps->extra_extra8; break;
        case PLAYER_QUICK_KICK: labelNum = ps->quick_kick; break;
        case PLAYER_HEAT_AMOUNT: labelNum = ps->inv_amount[GET_HEATS]; break;
        case PLAYER_ACTORSQU: labelNum = ps->actorsqu; break;
        case PLAYER_TIMEBEFOREEXIT: labelNum = ps->timebeforeexit; break;
        case PLAYER_CUSTOMEXITSOUND: labelNum = ps->customexitsound; break;
        case PLAYER_WEAPRECS: labelNum = ps->weaprecs[lParm2]; break;
        case PLAYER_WEAPRECCNT: labelNum = ps->weapreccnt; break;
        case PLAYER_INTERFACE_TOGGLE_FLAG: labelNum = ps->interface_toggle_flag; break;
        case PLAYER_ROTSCRNANG: labelNum = ps->rotscrnang; break;
        case PLAYER_DEAD_FLAG: labelNum = ps->dead_flag; break;
        case PLAYER_SHOW_EMPTY_WEAPON: labelNum = ps->show_empty_weapon; break;
        case PLAYER_SCUBA_AMOUNT: labelNum = ps->inv_amount[GET_SCUBA]; break;
        case PLAYER_JETPACK_AMOUNT: labelNum = ps->inv_amount[GET_JETPACK]; break;
        case PLAYER_STEROIDS_AMOUNT: labelNum = ps->inv_amount[GET_STEROIDS]; break;
        case PLAYER_SHIELD_AMOUNT: labelNum = ps->inv_amount[GET_SHIELD]; break;
        case PLAYER_HOLODUKE_ON: labelNum = ps->holoduke_on; break;
        case PLAYER_PYCOUNT: labelNum = ps->pycount; break;
        case PLAYER_WEAPON_POS: labelNum = ps->weapon_pos; break;
        case PLAYER_FRAG_PS: labelNum = ps->frag_ps; break;
        case PLAYER_TRANSPORTER_HOLD: labelNum = ps->transporter_hold; break;
        case PLAYER_CLIPDIST: labelNum = ps->clipdist; break;
        case PLAYER_LAST_FULL_WEAPON: labelNum = ps->last_full_weapon; break;
        case PLAYER_FOOTPRINTSHADE: labelNum = ps->footprintshade; break;
        case PLAYER_BOOT_AMOUNT: labelNum = ps->inv_amount[GET_BOOTS]; break;
        case PLAYER_SCREAM_VOICE: labelNum = ps->scream_voice; break;
        case PLAYER_GM: labelNum = ps->gm; break;
        case PLAYER_ON_WARPING_SECTOR: labelNum = ps->on_warping_sector; break;
        case PLAYER_FOOTPRINTCOUNT: labelNum = ps->footprintcount; break;
        case PLAYER_HBOMB_ON: labelNum = ps->hbomb_on; break;
        case PLAYER_JUMPING_TOGGLE: labelNum = ps->jumping_toggle; break;
        case PLAYER_RAPID_FIRE_HOLD: labelNum = ps->rapid_fire_hold; break;
        case PLAYER_ON_GROUND: labelNum = ps->on_ground; break;
        case PLAYER_INVEN_ICON: labelNum = ps->inven_icon; break;
        case PLAYER_BUTTONPALETTE: labelNum = ps->buttonpalette; break;
        case PLAYER_JETPACK_ON: labelNum = ps->jetpack_on; break;
        case PLAYER_SPRITEBRIDGE: labelNum = ps->spritebridge; break;
        case PLAYER_LASTRANDOMSPOT: labelNum = ps->lastrandomspot; break;
        case PLAYER_SCUBA_ON: labelNum = ps->scuba_on; break;
        case PLAYER_FOOTPRINTPAL: labelNum = ps->footprintpal; break;
        case PLAYER_HEAT_ON: labelNum = ps->heat_on; break;
        case PLAYER_HOLSTER_WEAPON: labelNum = ps->holster_weapon; break;
        case PLAYER_FALLING_COUNTER: labelNum = ps->falling_counter; break;
        case PLAYER_GOTWEAPON: labelNum = (ps->gotweapon & (1 << lParm2)) != 0; break;
        case PLAYER_REFRESH_INVENTORY: labelNum = ps->refresh_inventory; break;
        case PLAYER_PALETTE: labelNum = ps->palette; break;
        case PLAYER_TOGGLE_KEY_FLAG: labelNum = ps->toggle_key_flag; break;
        case PLAYER_KNUCKLE_INCS: labelNum = ps->knuckle_incs; break;
        case PLAYER_WALKING_SND_TOGGLE: labelNum = ps->walking_snd_toggle; break;
        case PLAYER_PALOOKUP: labelNum = ps->palookup; break;
        case PLAYER_HARD_LANDING: labelNum = ps->hard_landing; break;
        case PLAYER_MAX_SECRET_ROOMS: labelNum = ps->max_secret_rooms; break;
        case PLAYER_SECRET_ROOMS: labelNum = ps->secret_rooms; break;
        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: labelNum = ps->pals.r; break;
                case 1: labelNum = ps->pals.g; break;
                case 2: labelNum = ps->pals.b; break;
            }
            break;
        case PLAYER_MAX_ACTORS_KILLED: labelNum = ps->max_actors_killed; break;
        case PLAYER_ACTORS_KILLED: labelNum = ps->actors_killed; break;
        case PLAYER_RETURN_TO_CENTER: labelNum = ps->return_to_center; break;
        case PLAYER_RUNSPEED: labelNum = ps->runspeed; break;
        case PLAYER_SBS: labelNum = ps->sbs; break;
        case PLAYER_RELOADING: labelNum = ps->reloading; break;
        case PLAYER_AUTO_AIM: labelNum = ps->auto_aim; break;
        case PLAYER_MOVEMENT_LOCK: labelNum = ps->movement_lock; break;
        case PLAYER_SOUND_PITCH: labelNum = ps->sound_pitch; break;
        case PLAYER_WEAPONSWITCH: labelNum = ps->weaponswitch; break;
        case PLAYER_TEAM: labelNum = ps->team; break;
        case PLAYER_MAX_PLAYER_HEALTH: labelNum = ps->max_player_health; break;
        case PLAYER_MAX_SHIELD_AMOUNT: labelNum = ps->max_shield_amount; break;
        case PLAYER_MAX_AMMO_AMOUNT: labelNum = ps->max_ammo_amount[lParm2]; break;
        case PLAYER_LAST_QUICK_KICK: labelNum = ps->last_quick_kick; break;
        case PLAYER_AUTOSTEP: labelNum = ps->autostep; break;
        case PLAYER_AUTOSTEP_SBW: labelNum = ps->autostep_sbw; break;
        case PLAYER_HUDPAL: labelNum = P_GetHudPal(ps); break;
        case PLAYER_INDEX: labelNum = playerNum; break;
        case PLAYER_CONNECTED: labelNum = g_player[playerNum].playerquitflag; break;
        case PLAYER_FRAGS:
            if (playerNum == lParm2)
                labelNum = ps->fraggedself;
            else
                labelNum = g_player[playerNum].frags[lParm2];
        break;
        case PLAYER_DEATHS: labelNum = g_player[playerNum].frags[playerNum]; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetPlayer(int32_t const playerNum, int32_t const labelNum, int32_t const lParm2, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return;
    }

    DukePlayer_t * const ps = g_player[playerNum].ps;

    switch (labelNum)
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
        case PLAYER_CLIPDIST: ps->clipdist = iSet; break;
        case PLAYER_LAST_FULL_WEAPON: ps->last_full_weapon = iSet; break;
        case PLAYER_FOOTPRINTSHADE: ps->footprintshade = iSet; break;
        case PLAYER_BOOT_AMOUNT: ps->inv_amount[GET_BOOTS] = iSet; break;
        case PLAYER_SCREAM_VOICE: ps->scream_voice = iSet; break;
        case PLAYER_GM:
            if (!(ps->gm & MODE_MENU) && (iSet & MODE_MENU))
                Menu_Open(playerNum);
            else if ((ps->gm & MODE_MENU) && !(iSet & MODE_MENU))
                Menu_Close(playerNum);
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
        case PLAYER_FRAGS:
            if (playerNum == lParm2)
                ps->fraggedself = iSet;
            else
                g_player[playerNum].frags[lParm2] = iSet;
            break;
        case PLAYER_DEATHS: g_player[playerNum].frags[playerNum] = iSet; break;
        default: break;
    }
}

int32_t __fastcall VM_GetPlayerInput(int32_t const playerNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return -1;
    }

    input_t * const i = g_player[playerNum].inputBits;

    switch (labelNum)
    {
        case INPUT_AVEL: labelNum = i->avel; break;
        case INPUT_HORZ: labelNum = i->horz; break;
        case INPUT_FVEL: labelNum = i->fvel; break;
        case INPUT_SVEL: labelNum = i->svel; break;
        case INPUT_BITS: labelNum = i->bits; break;
        case INPUT_EXTBITS: labelNum = i->extbits; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetPlayerInput(int32_t const playerNum, int32_t const labelNum, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return;
    }

    input_t * const i = g_player[playerNum].inputBits;

    switch (labelNum)
    {
        case INPUT_AVEL: i->avel = iSet; break;
        case INPUT_HORZ: i->horz = iSet; break;
        case INPUT_FVEL: i->fvel = iSet; break;
        case INPUT_SVEL: i->svel = iSet; break;
        case INPUT_BITS: i->bits = iSet; break;
        case INPUT_EXTBITS: i->extbits = iSet; break;
        default: break;
    }
}



int32_t __fastcall VM_GetWall(int32_t const wallNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return -1;
    }

    uwalltype const * const pWall = (uwalltype *)&wall[wallNum];

    switch (labelNum)
    {
        case WALL_X: labelNum = pWall->x; break;
        case WALL_Y: labelNum = pWall->y; break;
        case WALL_POINT2: labelNum = pWall->point2; break;
        case WALL_NEXTWALL: labelNum = pWall->nextwall; break;
        case WALL_NEXTSECTOR: labelNum = pWall->nextsector; break;
        case WALL_CSTAT: labelNum = pWall->cstat; break;
        case WALL_PICNUM: labelNum = pWall->picnum; break;
        case WALL_OVERPICNUM: labelNum = pWall->overpicnum; break;
        case WALL_SHADE: labelNum = pWall->shade; break;
        case WALL_PAL: labelNum = pWall->pal; break;
        case WALL_XREPEAT: labelNum = pWall->xrepeat; break;
        case WALL_YREPEAT: labelNum = pWall->yrepeat; break;
        case WALL_XPANNING: labelNum = pWall->xpanning; break;
        case WALL_YPANNING: labelNum = pWall->ypanning; break;
        case WALL_LOTAG: labelNum = (int16_t)pWall->lotag; break;
        case WALL_HITAG: labelNum = (int16_t)pWall->hitag; break;
        case WALL_ULOTAG: labelNum = pWall->lotag; break;
        case WALL_UHITAG: labelNum = pWall->hitag; break;
        case WALL_EXTRA: labelNum = pWall->extra; break;
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            labelNum = pWall->blend;
#else
            labelNum = wallext[wallNum].blend;
#endif
            break;
        default: labelNum = -1;
    }

    return labelNum;
}

void __fastcall VM_SetWall(int32_t const wallNum, int32_t const labelNum, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return;
    }

    walltype * const pWall = &wall[wallNum];

    switch (labelNum)
    {
        case WALL_X: pWall->x = iSet; break;
        case WALL_Y: pWall->y = iSet; break;
        case WALL_POINT2: pWall->point2 = iSet; break;
        case WALL_NEXTWALL: pWall->nextwall = iSet; break;
        case WALL_NEXTSECTOR: pWall->nextsector = iSet; break;
        case WALL_CSTAT: pWall->cstat = iSet; break;
        case WALL_PICNUM: pWall->picnum = iSet; break;
        case WALL_OVERPICNUM: pWall->overpicnum = iSet; break;
        case WALL_SHADE: pWall->shade = iSet; break;
        case WALL_PAL: pWall->pal = iSet; break;
        case WALL_XREPEAT: pWall->xrepeat = iSet; break;
        case WALL_YREPEAT: pWall->yrepeat = iSet; break;
        case WALL_XPANNING: pWall->xpanning = iSet; break;
        case WALL_YPANNING: pWall->ypanning = iSet; break;
        case WALL_LOTAG: pWall->lotag = (int16_t)iSet; break;
        case WALL_HITAG: pWall->hitag = (int16_t)iSet; break;
        case WALL_ULOTAG: pWall->lotag = iSet; break;
        case WALL_UHITAG: pWall->hitag = iSet; break;
        case WALL_EXTRA: pWall->extra = iSet; break;
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            pWall->blend = iSet;
#else
            wallext[wallNum].blend = iSet;
#endif
            break;
    }

    return;
}

int32_t __fastcall VM_GetSector(int32_t const sectNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return -1;
    }

    usectortype const * const pSector = (usectortype *)&sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_WALLPTR: labelNum = pSector->wallptr; break;
        case SECTOR_WALLNUM: labelNum = pSector->wallnum; break;

        case SECTOR_CEILINGZ: labelNum = pSector->ceilingz; break;
        case SECTOR_CEILINGZVEL: labelNum = (GetAnimationGoal(&pSector->ceilingz) == -1) ? 0 : pSector->extra; break;
        case SECTOR_CEILINGZGOAL: labelNum = GetAnimationGoal(&pSector->ceilingz); break;

        case SECTOR_FLOORZ: labelNum = pSector->floorz; break;
        case SECTOR_FLOORZVEL: labelNum = (GetAnimationGoal(&pSector->floorz) == -1) ? 0 : pSector->extra; break;
        case SECTOR_FLOORZGOAL: labelNum = GetAnimationGoal(&pSector->floorz); break;

        case SECTOR_CEILINGSTAT: labelNum = pSector->ceilingstat; break;
        case SECTOR_FLOORSTAT: labelNum = pSector->floorstat; break;
        case SECTOR_CEILINGPICNUM: labelNum = pSector->ceilingpicnum; break;
        case SECTOR_CEILINGSLOPE: labelNum = pSector->ceilingheinum; break;
        case SECTOR_CEILINGSHADE: labelNum = pSector->ceilingshade; break;
        case SECTOR_CEILINGPAL: labelNum = pSector->ceilingpal; break;
        case SECTOR_CEILINGXPANNING: labelNum = pSector->ceilingxpanning; break;
        case SECTOR_CEILINGYPANNING: labelNum = pSector->ceilingypanning; break;
        case SECTOR_FLOORPICNUM: labelNum = pSector->floorpicnum; break;
        case SECTOR_FLOORSLOPE: labelNum = pSector->floorheinum; break;
        case SECTOR_FLOORSHADE: labelNum = pSector->floorshade; break;
        case SECTOR_FLOORPAL: labelNum = pSector->floorpal; break;
        case SECTOR_FLOORXPANNING: labelNum = pSector->floorxpanning; break;
        case SECTOR_FLOORYPANNING: labelNum = pSector->floorypanning; break;
        case SECTOR_VISIBILITY: labelNum = pSector->visibility; break;
        case SECTOR_FOGPAL: labelNum = pSector->fogpal; break;
        case SECTOR_LOTAG: labelNum = (int16_t)pSector->lotag; break;
        case SECTOR_HITAG: labelNum = (int16_t)pSector->hitag; break;
        case SECTOR_ULOTAG: labelNum = pSector->lotag; break;
        case SECTOR_UHITAG: labelNum = pSector->hitag; break;
        case SECTOR_EXTRA: labelNum = pSector->extra; break;
        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
#ifdef YAX_ENABLE
            labelNum = yax_getbunch(sectNum, labelNum == SECTOR_FLOORBUNCH);
#else
            labelNum = -1;
#endif
            break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetSector(int32_t const sectNum, int32_t const labelNum, int32_t iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return;
    }

    sectortype * const pSector = &sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_WALLPTR: pSector->wallptr = iSet; break;
        case SECTOR_WALLNUM: pSector->wallnum = iSet; break;

        case SECTOR_CEILINGZ: pSector->ceilingz = iSet; break;
        case SECTOR_CEILINGZVEL: pSector->extra = iSet;
            if ((iSet = GetAnimationGoal(&pSector->ceilingz)) != -1)
        case SECTOR_CEILINGZGOAL:
            SetAnimation(sectNum, &pSector->ceilingz, iSet, pSector->extra);
            break;

        case SECTOR_FLOORZ: pSector->floorz = iSet; break;
        case SECTOR_FLOORZVEL: pSector->extra = iSet;
            if ((iSet = GetAnimationGoal(&pSector->floorz)) != -1)
        case SECTOR_FLOORZGOAL:
            SetAnimation(sectNum, &pSector->floorz, iSet, pSector->extra);
            break;

        case SECTOR_CEILINGSTAT: pSector->ceilingstat = iSet; break;
        case SECTOR_FLOORSTAT: pSector->floorstat = iSet; break;
        case SECTOR_CEILINGPICNUM: pSector->ceilingpicnum = iSet; break;
        case SECTOR_CEILINGSLOPE: pSector->ceilingheinum = iSet; break;
        case SECTOR_CEILINGSHADE: pSector->ceilingshade = iSet; break;
        case SECTOR_CEILINGPAL: pSector->ceilingpal = iSet; break;
        case SECTOR_CEILINGXPANNING: pSector->ceilingxpanning = iSet; break;
        case SECTOR_CEILINGYPANNING: pSector->ceilingypanning = iSet; break;
        case SECTOR_FLOORPICNUM: pSector->floorpicnum = iSet; break;
        case SECTOR_FLOORSLOPE: pSector->floorheinum = iSet; break;
        case SECTOR_FLOORSHADE: pSector->floorshade = iSet; break;
        case SECTOR_FLOORPAL: pSector->floorpal = iSet; break;
        case SECTOR_FLOORXPANNING: pSector->floorxpanning = iSet; break;
        case SECTOR_FLOORYPANNING: pSector->floorypanning = iSet; break;
        case SECTOR_VISIBILITY: pSector->visibility = iSet; break;
        case SECTOR_FOGPAL: pSector->fogpal = iSet; break;
        case SECTOR_LOTAG: pSector->lotag = (int16_t) iSet; break;
        case SECTOR_HITAG: pSector->hitag = (int16_t) iSet; break;
        case SECTOR_ULOTAG: pSector->lotag = iSet; break;
        case SECTOR_UHITAG: pSector->hitag = iSet; break;
        case SECTOR_EXTRA: pSector->extra = iSet; break;
        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
        default: break;
    }
}

void __fastcall VM_SetSprite(int32_t const spriteNum, int32_t const labelNum, int32_t const lParm2, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned)spriteNum >= MAXSPRITES) ||
        (ActorLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)ActorLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for sprite %d\n", ActorLabels[labelNum].name, lParm2, spriteNum);
        return;
    }

    spritetype * const pSprite = &sprite[spriteNum];

    switch (labelNum)
    {
        case ACTOR_X: pSprite->x = iSet; break;
        case ACTOR_Y: pSprite->y = iSet; break;
        case ACTOR_Z: pSprite->z = iSet; break;
        case ACTOR_CSTAT: pSprite->cstat = iSet; break;
        case ACTOR_PICNUM: pSprite->picnum = iSet; break;
        case ACTOR_SHADE: pSprite->shade = iSet; break;
        case ACTOR_PAL: pSprite->pal = iSet; break;
        case ACTOR_CLIPDIST: pSprite->clipdist = iSet; break;
        case ACTOR_DETAIL: pSprite->blend = iSet; break;
        case ACTOR_XREPEAT: pSprite->xrepeat = iSet; break;
        case ACTOR_YREPEAT: pSprite->yrepeat = iSet; break;
        case ACTOR_XOFFSET: pSprite->xoffset = iSet; break;
        case ACTOR_YOFFSET: pSprite->yoffset = iSet; break;
        case ACTOR_SECTNUM: changespritesect(spriteNum, iSet); break;
        case ACTOR_STATNUM: changespritestat(spriteNum, iSet); break;
        case ACTOR_ANG: pSprite->ang = iSet; break;
        case ACTOR_OWNER: pSprite->owner = iSet; break;
        case ACTOR_XVEL: pSprite->xvel = iSet; break;
        case ACTOR_YVEL: pSprite->yvel = iSet; break;
        case ACTOR_ZVEL: pSprite->zvel = iSet; break;
        case ACTOR_LOTAG: pSprite->lotag = (int16_t)iSet; break;
        case ACTOR_HITAG: pSprite->hitag = (int16_t)iSet; break;
        case ACTOR_ULOTAG: pSprite->lotag = iSet; break;
        case ACTOR_UHITAG: pSprite->hitag = iSet; break;
        case ACTOR_EXTRA: pSprite->extra = iSet; break;
        case ACTOR_HTCGG: actor[spriteNum].cgg = iSet; break;
        case ACTOR_HTPICNUM: actor[spriteNum].picnum = iSet; break;
        case ACTOR_HTANG: actor[spriteNum].ang = iSet; break;
        case ACTOR_HTEXTRA: actor[spriteNum].extra = iSet; break;
        case ACTOR_HTOWNER: actor[spriteNum].owner = iSet; break;
        case ACTOR_HTMOVFLAG: actor[spriteNum].movflag = iSet; break;
        case ACTOR_HTTEMPANG: actor[spriteNum].tempang = iSet; break;
        case ACTOR_HTACTORSTAYPUT: actor[spriteNum].actorstayput = iSet; break;
        case ACTOR_HTDISPICNUM: actor[spriteNum].dispicnum = iSet; break;
        case ACTOR_HTTIMETOSLEEP: actor[spriteNum].timetosleep = iSet; break;
        case ACTOR_HTFLOORZ: actor[spriteNum].floorz = iSet; break;
        case ACTOR_HTCEILINGZ: actor[spriteNum].ceilingz = iSet; break;
        case ACTOR_HTLASTVX: actor[spriteNum].lastv.x = iSet; break;
        case ACTOR_HTLASTVY: actor[spriteNum].lastv.y = iSet; break;
        case ACTOR_HTBPOSX: actor[spriteNum].bpos.x = iSet; break;
        case ACTOR_HTBPOSY: actor[spriteNum].bpos.y = iSet; break;
        case ACTOR_HTBPOSZ: actor[spriteNum].bpos.z = iSet; break;
        case ACTOR_HTG_T: actor[spriteNum].t_data[lParm2] = iSet; break;
        case ACTOR_ANGOFF: spriteext[spriteNum].angoff = iSet; break;
        case ACTOR_PITCH: spriteext[spriteNum].pitch = iSet; break;
        case ACTOR_ROLL: spriteext[spriteNum].roll = iSet; break;
        case ACTOR_MDXOFF: spriteext[spriteNum].offset.x = iSet; break;
        case ACTOR_MDYOFF: spriteext[spriteNum].offset.y = iSet; break;
        case ACTOR_MDZOFF: spriteext[spriteNum].offset.z = iSet; break;
        case ACTOR_MDFLAGS: spriteext[spriteNum].flags = iSet; break;
        case ACTOR_XPANNING: spriteext[spriteNum].xpanning = iSet; break;
        case ACTOR_YPANNING: spriteext[spriteNum].ypanning = iSet; break;
        case ACTOR_HTFLAGS: actor[spriteNum].flags = iSet; break;
        case ACTOR_ALPHA: spriteext[spriteNum].alpha = (float)iSet * (1.f / 255.0f); break;
        default: break;
    }
}


int32_t __fastcall VM_GetSprite(int32_t const spriteNum, int32_t labelNum, int32_t const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned) spriteNum >= MAXSPRITES) ||
        (ActorLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) ActorLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for sprite %d\n", ActorLabels[labelNum].name, lParm2, spriteNum);
        return -1;
    }

    uspritetype const * const pSprite = (uspritetype *)&sprite[spriteNum];

    switch (labelNum)
    {
        case ACTOR_X: labelNum = pSprite->x; break;
        case ACTOR_Y: labelNum = pSprite->y; break;
        case ACTOR_Z: labelNum = pSprite->z; break;
        case ACTOR_CSTAT: labelNum = pSprite->cstat; break;
        case ACTOR_PICNUM: labelNum = pSprite->picnum; break;
        case ACTOR_SHADE: labelNum = pSprite->shade; break;
        case ACTOR_PAL: labelNum = pSprite->pal; break;
        case ACTOR_CLIPDIST: labelNum = pSprite->clipdist; break;
        case ACTOR_DETAIL: labelNum = pSprite->blend; break;
        case ACTOR_XREPEAT: labelNum = pSprite->xrepeat; break;
        case ACTOR_YREPEAT: labelNum = pSprite->yrepeat; break;
        case ACTOR_XOFFSET: labelNum = pSprite->xoffset; break;
        case ACTOR_YOFFSET: labelNum = pSprite->yoffset; break;
        case ACTOR_SECTNUM: labelNum = pSprite->sectnum; break;
        case ACTOR_STATNUM: labelNum = pSprite->statnum; break;
        case ACTOR_ANG: labelNum = pSprite->ang; break;
        case ACTOR_OWNER: labelNum = pSprite->owner; break;
        case ACTOR_XVEL: labelNum = pSprite->xvel; break;
        case ACTOR_YVEL: labelNum = pSprite->yvel; break;
        case ACTOR_ZVEL: labelNum = pSprite->zvel; break;
        case ACTOR_LOTAG: labelNum = (int16_t)pSprite->lotag; break;
        case ACTOR_HITAG: labelNum = (int16_t)pSprite->hitag; break;
        case ACTOR_ULOTAG: labelNum = pSprite->lotag; break;
        case ACTOR_UHITAG: labelNum = pSprite->hitag; break;
        case ACTOR_EXTRA: labelNum = pSprite->extra; break;
        case ACTOR_HTCGG: labelNum = actor[spriteNum].cgg; break;
        case ACTOR_HTPICNUM: labelNum = actor[spriteNum].picnum; break;
        case ACTOR_HTANG: labelNum = actor[spriteNum].ang; break;
        case ACTOR_HTEXTRA: labelNum = actor[spriteNum].extra; break;
        case ACTOR_HTOWNER: labelNum = actor[spriteNum].owner; break;
        case ACTOR_HTMOVFLAG: labelNum = actor[spriteNum].movflag; break;
        case ACTOR_HTTEMPANG: labelNum = actor[spriteNum].tempang; break;
        case ACTOR_HTACTORSTAYPUT: labelNum = actor[spriteNum].actorstayput; break;
        case ACTOR_HTDISPICNUM: labelNum = actor[spriteNum].dispicnum; break;
        case ACTOR_HTTIMETOSLEEP: labelNum = actor[spriteNum].timetosleep; break;
        case ACTOR_HTFLOORZ: labelNum = actor[spriteNum].floorz; break;
        case ACTOR_HTCEILINGZ: labelNum = actor[spriteNum].ceilingz; break;
        case ACTOR_HTLASTVX: labelNum = actor[spriteNum].lastv.x; break;
        case ACTOR_HTLASTVY: labelNum = actor[spriteNum].lastv.y; break;
        case ACTOR_HTBPOSX: labelNum = actor[spriteNum].bpos.x; break;
        case ACTOR_HTBPOSY: labelNum = actor[spriteNum].bpos.y; break;
        case ACTOR_HTBPOSZ: labelNum = actor[spriteNum].bpos.z; break;
        case ACTOR_HTG_T: labelNum = actor[spriteNum].t_data[lParm2]; break;
        case ACTOR_ANGOFF: labelNum = spriteext[spriteNum].angoff; break;
        case ACTOR_PITCH: labelNum = spriteext[spriteNum].pitch; break;
        case ACTOR_ROLL: labelNum = spriteext[spriteNum].roll; break;
        case ACTOR_MDXOFF: labelNum = spriteext[spriteNum].offset.x; break;
        case ACTOR_MDYOFF: labelNum = spriteext[spriteNum].offset.y; break;
        case ACTOR_MDZOFF: labelNum = spriteext[spriteNum].offset.z; break;
        case ACTOR_MDFLAGS: labelNum = spriteext[spriteNum].flags; break;
        case ACTOR_XPANNING: labelNum = spriteext[spriteNum].xpanning; break;
        case ACTOR_YPANNING: labelNum = spriteext[spriteNum].ypanning; break;
        case ACTOR_HTFLAGS: labelNum = actor[spriteNum].flags; break;
        case ACTOR_ALPHA: labelNum = (uint8_t)(spriteext[spriteNum].alpha * 255.0f); break;
        case ACTOR_ISVALID: labelNum = (sprite[spriteNum].statnum != MAXSTATUS); break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

int32_t __fastcall VM_GetTsprite(int32_t const spriteNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
error:
        CON_ERRPRINTF("invalid sprite %d or no tsprite\n", spriteNum);
        return -1;
    }

    uspritetype const * const tspr = spriteext[spriteNum].tspr;

    if (EDUKE32_PREDICT_FALSE(!tspr))
        goto error;

    switch (labelNum)
    {
        case ACTOR_X: labelNum = tspr->x; break;
        case ACTOR_Y: labelNum = tspr->y; break;
        case ACTOR_Z: labelNum = tspr->z; break;
        case ACTOR_CSTAT: labelNum = tspr->cstat; break;
        case ACTOR_PICNUM: labelNum = tspr->picnum; break;
        case ACTOR_SHADE: labelNum = tspr->shade; break;
        case ACTOR_PAL: labelNum = tspr->pal; break;
        case ACTOR_CLIPDIST: labelNum = tspr->clipdist; break;
        case ACTOR_DETAIL: labelNum = tspr->blend; break;
        case ACTOR_XREPEAT: labelNum = tspr->xrepeat; break;
        case ACTOR_YREPEAT: labelNum = tspr->yrepeat; break;
        case ACTOR_XOFFSET: labelNum = tspr->xoffset; break;
        case ACTOR_YOFFSET: labelNum = tspr->yoffset; break;
        case ACTOR_SECTNUM: labelNum = tspr->sectnum; break;
        case ACTOR_STATNUM: labelNum = tspr->statnum; break;
        case ACTOR_ANG: labelNum = tspr->ang; break;
        case ACTOR_OWNER: labelNum = tspr->owner; break;
        case ACTOR_XVEL: labelNum = tspr->xvel; break;
        case ACTOR_YVEL: labelNum = tspr->yvel; break;
        case ACTOR_ZVEL: labelNum = tspr->zvel; break;
        case ACTOR_LOTAG: labelNum = (int16_t) tspr->lotag; break;
        case ACTOR_HITAG: labelNum = (int16_t) tspr->hitag; break;
        case ACTOR_ULOTAG: labelNum = tspr->lotag; break;
        case ACTOR_UHITAG: labelNum = tspr->hitag; break;
        case ACTOR_EXTRA: labelNum = tspr->extra; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetTsprite(int32_t const spriteNum, int32_t const labelNum, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum >= MAXSPRITES))
    {
error:
        CON_ERRPRINTF("invalid sprite %d or no tsprite\n", spriteNum);
        return;
    }

    uspritetype * const tspr = spriteext[spriteNum].tspr;

    if (EDUKE32_PREDICT_FALSE(!tspr))
        goto error;

    switch (labelNum)
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

int32_t __fastcall VM_GetProjectile(int32_t const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
error:
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return -1;
    }

    projectile_t const * const proj = g_tile[tileNum].proj;

    if (EDUKE32_PREDICT_FALSE(!proj))
        goto error;

    switch (labelNum)
    {
        case PROJ_WORKSLIKE: labelNum = proj->workslike; break;
        case PROJ_SPAWNS: labelNum = proj->spawns; break;
        case PROJ_SXREPEAT: labelNum = proj->sxrepeat; break;
        case PROJ_SYREPEAT: labelNum = proj->syrepeat; break;
        case PROJ_SOUND: labelNum = proj->sound; break;
        case PROJ_ISOUND: labelNum = proj->isound; break;
        case PROJ_VEL: labelNum = proj->vel; break;
        case PROJ_EXTRA: labelNum = proj->extra; break;
        case PROJ_DECAL: labelNum = proj->decal; break;
        case PROJ_TRAIL: labelNum = proj->trail; break;
        case PROJ_TXREPEAT: labelNum = proj->txrepeat; break;
        case PROJ_TYREPEAT: labelNum = proj->tyrepeat; break;
        case PROJ_TOFFSET: labelNum = proj->toffset; break;
        case PROJ_TNUM: labelNum = proj->tnum; break;
        case PROJ_DROP: labelNum = proj->drop; break;
        case PROJ_CSTAT: labelNum = proj->cstat; break;
        case PROJ_CLIPDIST: labelNum = proj->clipdist; break;
        case PROJ_SHADE: labelNum = proj->shade; break;
        case PROJ_XREPEAT: labelNum = proj->xrepeat; break;
        case PROJ_YREPEAT: labelNum = proj->yrepeat; break;
        case PROJ_PAL: labelNum = proj->pal; break;
        case PROJ_EXTRA_RAND: labelNum = proj->extra_rand; break;
        case PROJ_HITRADIUS: labelNum = proj->hitradius; break;
        case PROJ_MOVECNT: labelNum = proj->movecnt; break;
        case PROJ_OFFSET: labelNum = proj->offset; break;
        case PROJ_BOUNCES: labelNum = proj->bounces; break;
        case PROJ_BSOUND: labelNum = proj->bsound; break;
        case PROJ_RANGE: labelNum = proj->range; break;
        case PROJ_FLASH_COLOR: labelNum = proj->flashcolor; break;
        case PROJ_USERDATA: labelNum = proj->userdata; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

void __fastcall VM_SetProjectile(int32_t const tileNum, int32_t const labelNum, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) tileNum >= MAXTILES))
    {
error:
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return;
    }

    projectile_t * const proj = g_tile[tileNum].proj;

    if (EDUKE32_PREDICT_FALSE(!proj))
        goto error;

    switch (labelNum)
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

int32_t __fastcall VM_GetTileData(int32_t const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
        CON_ERRPRINTF("invalid tile %d\n", tileNum);
        return -1;
    }

    switch (labelNum)
    {
        case TILEDATA_XSIZE: labelNum = tilesiz[tileNum].x; break;
        case TILEDATA_YSIZE: labelNum = tilesiz[tileNum].y; break;

        case TILEDATA_ANIMFRAMES: labelNum = picanm[tileNum].num; break;
        case TILEDATA_XOFFSET: labelNum = picanm[tileNum].xofs; break;
        case TILEDATA_YOFFSET: labelNum = picanm[tileNum].yofs; break;
        case TILEDATA_ANIMSPEED: labelNum = picanm[tileNum].sf & PICANM_ANIMSPEED_MASK; break;
        case TILEDATA_ANIMTYPE: labelNum = (picanm[tileNum].sf & PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT; break;

        case TILEDATA_GAMEFLAGS: labelNum = g_tile[tileNum].flags; break;

        default: labelNum = -1; break;
    }

    return labelNum;
}

int32_t __fastcall VM_GetPalData(int32_t const palNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)palNum >= MAXPALOOKUPS))
    {
        CON_ERRPRINTF("invalid palette %d\n", palNum);
        return -1;
    }

    switch (labelNum)
    {
        case PALDATA_NOFLOORPAL: labelNum = g_noFloorPal[palNum]; break;
        default: labelNum = -1; break;
    }

    return labelNum;
}

#endif
