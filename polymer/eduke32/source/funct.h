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

#ifndef __funct_h__
#define __funct_h__

#ifndef _MSC_VER
  #if defined(__GNUC__) && defined(__i386__) 
    #ifndef __fastcall
      #define __fastcall __attribute__((fastcall))
    #endif
  #else
    #define __fastcall
  #endif
#endif

extern void sendscore(const char *s);
extern void S_SoundStartup(void);
extern void S_SoundShutdown(void);
extern void S_MusicStartup(void);
extern void S_MusicShutdown(void);
extern void S_MenuSound(void);
extern int32_t S_PlayMusic(const char *fn, const int32_t sel);
extern int32_t S_LoadSound(unsigned num);
extern int32_t S_PlaySoundXYZ(int32_t num,int32_t i,const vec3_t *pos);
extern void S_PlaySound(int32_t num);
extern int32_t A_PlaySound(uint32_t num,int32_t i);
extern void S_StopSound(int32_t num);
extern void S_StopEnvSound(int32_t num,int32_t i);
extern void S_Pan3D(void);
extern void S_TestSoundCallback(uint32_t num);
extern void S_ClearSoundLocks(void);
extern int32_t A_CallSound(int32_t sn,int32_t whatsprite);
extern int32_t G_CheckActivatorMotion(int32_t lotag);
extern int32_t CheckDoorTile(int32_t dapic);
extern int32_t isanunderoperator(int32_t lotag);
extern int32_t isanearoperator(int32_t lotag);
extern inline int32_t G_CheckPlayerInSector(int32_t sect);
extern int32_t ldist(spritetype *s1,spritetype *s2);
extern int32_t dist(spritetype *s1,spritetype *s2);
extern int32_t __fastcall A_FindPlayer(spritetype *s,int32_t *d);
extern int32_t P_FindOtherPlayer(int32_t p,int32_t *d);
extern void G_DoSectorAnimations(void);
extern int32_t GetAnimationGoal(int32_t *animptr);
extern int32_t SetAnimation(int32_t animsect,int32_t *animptr,int32_t thegoal,int32_t thevel);
extern void G_AnimateCamSprite(void);
extern void G_AnimateWalls(void);
extern int32_t G_ActivateWarpElevators(int32_t s,int32_t d);
extern void G_OperateSectors(int32_t sn,int32_t ii);
extern void G_OperateRespawns(int32_t low);
extern void G_OperateActivators(int32_t low,int32_t snum);
extern void G_OperateMasterSwitches(int32_t low);
extern void G_OperateForceFields(int32_t s,int32_t low);
extern int32_t P_ActivateSwitch(int32_t snum,int32_t w,int32_t switchtype);
extern void activatebysector(int32_t sect,int32_t j);
extern void A_DamageWall(int32_t spr,int32_t dawallnum,const vec3_t *pos,int32_t atwith);
extern void P_CheckTouchDamage(DukePlayer_t *p,int32_t j);
extern int32_t Sect_DamageCeiling(int32_t sn);
extern void A_DamageObject(int32_t i,int32_t sn);
extern void allignwarpelevators(void);
extern void G_HandleSharedKeys(int32_t snum);
extern void P_CheckSectors(int32_t snum);
extern int32_t RTS_AddFile(const char *filename);
extern void RTS_Init(const char *filename);
extern int32_t RTS_NumSounds(void);
extern int32_t RTS_SoundLength(int32_t lump);
extern const char *RTS_GetSoundName(int32_t i);
extern void RTS_ReadLump(int32_t lump,void *dest);
extern void *RTS_GetSound(int32_t lump);
extern void G_CacheMapData(void);
extern void xyzmirror(int32_t i,int32_t wn);
extern void G_UpdateScreenArea(void);
extern void P_RandomSpawnPoint(int32_t snum);
extern void P_ResetWeapons(int32_t snum);
extern void P_ResetInventory(int32_t snum);
extern void G_NewGame(int32_t vn,int32_t ln,int32_t sk);
extern void G_ResetTimers(void);
extern void Net_WaitForEverybody(void);
extern void clearfifo(void);
extern void Net_ResetPrediction(void);
extern int32_t  G_EnterLevel(int32_t g);
extern void G_BackToMenu(void);
extern void P_UpdateScreenPal(DukePlayer_t *p);
extern void P_QuickKill(DukePlayer_t *p);
extern int32_t A_GetHitscanRange(int32_t i);
extern int32_t A_CheckHitSprite(int32_t i,short *hitsp);
extern int32_t A_Shoot(int32_t i,int32_t atwith);
extern void P_DisplayScubaMask(int32_t snum);
extern void P_DisplayWeapon(int32_t snum);
extern void getinput(int32_t snum);
extern void P_DropWeapon(DukePlayer_t *p);
extern void P_ProcessInput(int32_t snum);
extern void ChangeToMenu(int32_t cm);
// extern void savetemp(const char *fn,int32_t daptr,int32_t dasiz);
// extern int32_t G_LoadSaveHeader(char spot,int32_t *vn,int32_t *ln,int32_t *psk,int32_t *numplr);
extern int32_t G_LoadPlayer(int32_t spot);
extern int32_t G_SavePlayer(int32_t spot);
extern int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits);
#define menutext(x,y,s,p,t) menutext_(x,y,s,p,(char *)stripcolorcodes(menutextbuf,t),10+16)
extern void M_DisplayMenus(void);
extern void G_FadePalette(int32_t r,int32_t g,int32_t b,int32_t e);
extern void G_PlayAnim(const char *fn,char);
extern int32_t G_GetAngleDelta(int32_t a,int32_t na);
extern void A_GetZLimits(int32_t iActor);
extern void A_Fall(int32_t iActor);
extern void C_Compile(const char *fn);
extern int32_t A_GetFurthestAngle(int32_t iActor,int32_t angs);
extern void A_Execute(int32_t iActor,int32_t iPlayer,int32_t lDist);
extern void gamenumber(int32_t x,int32_t y,int32_t n,int32_t s);
extern void G_Shutdown(void);
extern void Net_GetPackets(void);
extern void A_PlayAlertSound(int32_t i);
extern inline int32_t A_CheckInventorySprite(spritetype *s);
extern inline int32_t A_CheckEnemySprite(spritetype *s);
extern int32_t A_CheckEnemyTile(int32_t pn);
extern void G_DrawTile(int32_t x,int32_t y,int32_t tilenum,int32_t shade,int32_t orientation);
extern void G_DrawTilePal(int32_t x,int32_t y,int32_t tilenum,int32_t shade,int32_t orientation,int32_t p);
extern void G_DrawFrags(void);
extern void P_DoQuote(int32_t q,DukePlayer_t *p);
extern void G_GameExit(const char *t);
extern void G_DisplayRest(int32_t smoothratio);
extern void updatesectorz(int32_t x,int32_t y,int32_t z,short *sectnum);
extern void G_DrawBackground(void);
extern void G_DrawRooms(int32_t snum,int32_t smoothratio);
extern int32_t A_InsertSprite(int32_t whatsect,int32_t s_x,int32_t s_y,int32_t s_z,int32_t s_pn,int32_t s_s,int32_t s_xr,int32_t s_yr,int32_t s_a,int32_t s_ve,int32_t s_zv,int32_t s_ow,int32_t s_ss);
extern int32_t A_CheckSwitchTile(int32_t i);
extern int32_t A_Spawn(int32_t j,int32_t pn);
extern void G_DoSpriteAnimations(int32_t x,int32_t y,int32_t a,int32_t smoothratio);
extern int32_t main(int32_t argc,char **argv);
extern void G_OpenDemoWrite(void);
extern void G_CloseDemoWrite(void);
extern void G_BonusScreen(int32_t bonusonly);
extern void A_SpawnWallGlass(int32_t i,int32_t wallnum,int32_t n);
extern void A_SpawnGlass(int32_t i,int32_t n);
extern void A_SpawnCeilingGlass(int32_t i,int32_t sectnum,int32_t n);
extern void A_SpawnRandomGlass(int32_t i,int32_t wallnum,int32_t n);
extern int32_t GetTime(void);
extern void CONFIG_GetSetupFilename(void);
extern int32_t CONFIG_FunctionNameToNum(char *func);
extern char *CONFIG_FunctionNumToName(int32_t func);
extern int32_t CONFIG_AnalogNameToNum(char *func);
extern char *CONFIG_AnalogNumToName(int32_t func);
extern void CONFIG_SetDefaults(void);
extern void CONFIG_ReadKeys(void);
extern void ReadSaveGameHeaders(void);
extern int32_t CONFIG_ReadSetup(void);
extern void CONFIG_WriteSetup(void);
extern inline void G_UpdateInterpolations(void);
extern void G_SetInterpolation(int32_t *posptr);
extern void G_StopInterpolation(int32_t *posptr);
extern void G_DoInterpolations(int32_t smoothratio);
extern inline void G_RestoreInterpolations(void);
extern inline int32_t G_CheckForSpaceCeiling(int32_t sectnum);
extern inline int32_t G_CheckForSpaceFloor(int32_t sectnum);
extern void P_AddAmmo(int32_t weapon,DukePlayer_t *p,int32_t amount);
extern void P_AddWeaponNoSwitch(DukePlayer_t *p,int32_t weapon);
extern void P_AddWeapon(DukePlayer_t *p,int32_t weapon);
extern void P_SelectNextInvItem(DukePlayer_t *p);
extern void P_CheckWeapon(DukePlayer_t *p);
extern void A_RadiusDamage(int32_t i,int32_t r,int32_t hp1,int32_t hp2,int32_t hp3,int32_t hp4);
extern int32_t A_MoveSprite(int32_t spritenum,const vec3_t *change,uint32_t cliptype);
extern inline int32_t A_SetSprite(int32_t i,uint32_t cliptype);
extern void A_AddToDeleteQueue(int32_t i);
extern void A_SpawnMultiple(int32_t sp,int32_t pic,int32_t n);
extern void A_DoGuts(int32_t sp,int32_t gtype,int32_t n);
extern void Sect_SetInterpolation(int32_t i);
extern void Sect_ClearInterpolation(int32_t i);
extern int32_t A_IncurDamage(int32_t sn);
extern void G_MoveWorld(void);
extern void A_MoveCyclers(void);
extern void A_MoveDummyPlayers(void);

// game.c
extern inline void G_SetStatusBarScale(int32_t sc);

extern void P_SetGamePalette(DukePlayer_t *player, uint8_t *pal, int32_t set);
extern void fadepal(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step);

extern int32_t gametext_z(int32_t small, int32_t starttile, int32_t x,int32_t y,const char *t,int32_t s,int32_t p,int32_t orientation,int32_t x1, int32_t y1, int32_t x2, int32_t y2,int32_t z);
extern void G_DrawTXDigiNumZ(int32_t starttile, int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,int32_t cs,int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z);
extern void G_DrawTileSmall(int32_t x,int32_t y,int32_t tilenum,int32_t shade,int32_t orientation);
extern void G_DrawTilePalSmall(int32_t x,int32_t y,int32_t tilenum,int32_t shade,int32_t orientation,int32_t p);
extern void Gv_ResetVars(void);
extern void A_ResetVars(int32_t iActor);

extern int32_t minitext_(int32_t x,int32_t y,const char *t,int32_t s,int32_t p,int32_t sb);

#define minitextshade(x, y, t, s, p, sb) minitext_(x,y,t,s,p,sb)
#define minitext(x, y, t, p, sb) minitext_(x,y,t,0,p,sb)

#define gametext(x,y,t,s,dabits) gametext_z(0,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define gametextscaled(x,y,t,s,dabits) gametext_z(1,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define gametextpal(x,y,t,s,p) gametext_z(0,STARTALPHANUM, x,y,t,s,p,26,0, 0, xdim-1, ydim-1, 65536)
#define gametextpalbits(x,y,t,s,p,dabits) gametext_z(0,STARTALPHANUM, x,y,t,s,p,dabits,0, 0, xdim-1, ydim-1, 65536)

extern void G_InitDynamicTiles();
extern void G_ProcessDynamicTileMapping(const char *szLabel, int32_t lValue);

extern void A_LoadActor(int32_t sActor);

extern int32_t Gv_GetVarByLabel(const char *szGameLabel, int32_t lDefault, int32_t iActor, int32_t iPlayer);
extern void Gv_DumpValues(void);
// extern void AddLog(const char *psz, ...);

extern void Gv_ResetSystemDefaults(void);
extern void Gv_InitWeaponPointers(void);
extern void Gv_Init(void);
extern void Gv_WriteSave(FILE *fil);
extern int32_t Gv_ReadSave(int32_t fil);

extern int32_t __fastcall Gv_GetVar(int32_t id, int32_t iActor, int32_t iPlayer);
extern void __fastcall Gv_SetVar(int32_t id, int32_t lValue, int32_t iActor, int32_t iPlayer);
extern int32_t __fastcall Gv_GetVarX(int32_t id);
extern void __fastcall Gv_SetVarX(int32_t id, int32_t lValue);

// extern void SetGameArrayID(int32_t id,int32_t index, int32_t lValue);

extern int32_t Gv_NewVar(const char *pszLabel, int32_t lValue, uint32_t dwFlags);
extern int32_t Gv_NewArray(const char *pszLabel, int32_t asize);
extern void C_ReportError(int32_t iError);

extern void onvideomodechange(int32_t newmode);

extern void X_OnEvent(int32_t iEventID, int32_t sActor, int32_t sPlayer, int32_t lDist);

extern int32_t A_CheckSoundPlaying(int32_t i, int32_t num);
extern int32_t S_CheckSoundPlaying(int32_t i, int32_t num);
extern void A_StopSound(int32_t num, int32_t i);
extern void G_UpdatePlayerFromMenu(void);
extern void Net_SendPlayerName(void);
extern void Net_SendUserMapName(void);
extern void Net_SendQuit(void);

extern void G_AddUserQuote(const char *daquote);
extern void Net_NewGame(int32_t volume, int32_t level);

extern int32_t SpriteFlags[MAXTILES];

#define A_CheckSpriteFlags(iActor, iType) (((SpriteFlags[sprite[iActor].picnum]^ActorExtra[iActor].flags) & iType) != 0)
#define A_CheckSpriteTileFlags(iPicnum, iType) ((SpriteFlags[iPicnum] & iType) != 0)

static inline int32_t G_GetTeamPalette(int32_t team)
{
    switch (team)
    {
    case 0:
        return 3;
    case 1:
        return 10;
    case 2:
        return 11;
    case 3:
        return 12;
    }
    return 0;
}

extern inline void G_AddGameLight(int32_t radius, int32_t srcsprite, int32_t zoffset, int32_t range, int32_t color, int32_t priority);

extern void se40code(int32_t x,int32_t y,int32_t z,int32_t a,int32_t h, int32_t smoothratio);

extern void G_FreeMapState(int32_t mapnum);
extern void G_FindLevelForFilename(const char *fn, char *volume, char *level);

extern void G_GetCrosshairColor(void);
extern void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b);
extern int32_t kopen4loadfrommod(char *filename, char searchfirst);

extern int32_t _EnterText(int32_t small,int32_t x,int32_t y,char *t,int32_t dalen,int32_t c);

#define G_EnterText(x, y, t, dalen, c) _EnterText(0,x,y,t,dalen,c)
#define Net_EnterText(x, y, t, dalen, c) _EnterText(1,x,y,t,dalen,c)

#endif // __funct_h__
