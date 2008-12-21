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
  #ifdef __GNUC__
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
extern int S_PlayMusic(const char *fn, const int sel);
extern int S_LoadSound(unsigned num);
extern int S_PlaySoundXYZ(int num,int i,int x,int y,int z);
extern void S_PlaySound(int num);
extern int A_PlaySound(unsigned int num,int i);
extern void S_StopSound(int num);
extern void S_StopEnvSound(int num,int i);
extern void pan3dsound(void);
extern void S_TestSoundCallback(unsigned int num);
extern void S_ClearSoundLocks(void);
extern int A_CallSound(int sn,int whatsprite);
extern int check_activator_motion(int lotag);
extern int CheckDoorTile(int dapic);
extern int isanunderoperator(int lotag);
extern int isanearoperator(int lotag);
extern inline int CheckPlayerInSector(int sect);
extern int ldist(spritetype *s1,spritetype *s2);
extern int dist(spritetype *s1,spritetype *s2);
extern int A_FindPlayer(spritetype *s,int *d);
extern int P_FindOtherPlayer(int p,int *d);
extern void G_DoSectorAnimations(void);
extern int GetAnimationGoal(int *animptr);
extern int SetAnimation(int animsect,int *animptr,int thegoal,int thevel);
extern void G_AnimateCamSprite(void);
extern void G_AnimateWalls(void);
extern int G_ActivateWarpElevators(int s,int d);
extern void G_OperateSectors(int sn,int ii);
extern void G_OperateRespawns(int low);
extern void G_OperateActivators(int low,int snum);
extern void G_OperateMasterSwitches(int low);
extern void G_OperateForceFields(int s,int low);
extern int P_ActivateSwitch(int snum,int w,int switchtype);
extern void activatebysector(int sect,int j);
extern void A_DamageWall(int spr,int dawallnum,int x,int y,int z,int atwith);
extern void P_CheckTouchDamage(DukePlayer_t *p,int j);
extern int Sect_DamageCeiling(int sn);
extern void A_DamageObject(int i,int sn);
extern void allignwarpelevators(void);
extern void G_HandleSharedKeys(int snum);
extern void checksectors(int snum);
extern int32 RTS_AddFile(const char *filename);
extern void RTS_Init(const char *filename);
extern int32 RTS_NumSounds(void);
extern int32 RTS_SoundLength(int32 lump);
extern const char *RTS_GetSoundName(int32 i);
extern void RTS_ReadLump(int32 lump,void *dest);
extern void *RTS_GetSound(int32 lump);
extern void G_CacheMapData(void);
extern void xyzmirror(int i,int wn);
extern void G_UpdateScreenArea(void);
extern void P_RandomSpawnPoint(int snum);
extern void P_ResetWeapons(int snum);
extern void P_ResetInventory(int snum);
extern void G_NewGame(int vn,int ln,int sk);
extern void G_ResetTimers(void);
extern void waitforeverybody(void);
extern void clearfifo(void);
extern void Net_ResetPrediction(void);
extern int  G_EnterLevel(int g);
extern void G_BackToMenu(void);
extern void P_UpdateScreenPal(DukePlayer_t *p);
extern void P_QuickKill(DukePlayer_t *p);
extern int A_GetHitscanRange(int i);
extern int A_CheckHitSprite(int i,short *hitsp);
extern int A_Shoot(int i,int atwith);
extern void P_DisplayScubaMask(int snum);
extern void P_DisplayWeapon(int snum);
extern void getinput(int snum);
extern void P_DropWeapon(DukePlayer_t *p);
extern void P_ProcessInput(int snum);
extern void ChangeToMenu(int cm);
// extern void savetemp(const char *fn,int daptr,int dasiz);
// extern int G_LoadSaveHeader(char spot,int32 *vn,int32 *ln,int32 *psk,int32 *numplr);
extern int G_LoadPlayer(int spot);
extern int G_SavePlayer(int spot);
extern int menutext_(int x,int y,int s,int p,char *t,int bits);
#define menutext(x,y,s,p,t) menutext_(x,y,s,p,(char *)stripcolorcodes(menutextbuf,t),10+16)
extern void M_DisplayMenus(void);
extern void G_FadePalette(int r,int g,int b,int e);
extern void G_PlayAnim(const char *fn,char);
extern int G_GetAngleDelta(int a,int na);
extern void A_GetZLimits(int iActor);
extern void A_Fall(int iActor);
extern void C_Compile(const char *fn);
extern int A_GetFurthestAngle(int iActor,int angs);
extern void A_Execute(int iActor,int iPlayer,int lDist);
extern void overwritesprite(int thex,int they,int tilenum,int shade,int stat,int dapalnum);
extern void gamenumber(int x,int y,int n,int s);
extern void G_Shutdown(void);
extern void getpackets(void);
extern void A_PlayAlertSound(int i);
extern inline int A_CheckInventorySprite(spritetype *s);
extern inline int A_CheckEnemySprite(spritetype *s);
extern int A_CheckEnemyTile(int pn);
extern void G_DrawTile(int x,int y,int tilenum,int shade,int orientation);
extern void G_DrawTilePal(int x,int y,int tilenum,int shade,int orientation,int p);
extern void G_DrawFrags(void);
extern void P_DoQuote(int q,DukePlayer_t *p);
extern void G_GameExit(const char *t);
extern void G_DisplayRest(int smoothratio);
extern void updatesectorz(int x,int y,int z,short *sectnum);
extern void G_DrawBackground(void);
extern void G_DrawRooms(int snum,int smoothratio);
extern int A_InsertSprite(int whatsect,int s_x,int s_y,int s_z,int s_pn,int s_s,int s_xr,int s_yr,int s_a,int s_ve,int s_zv,int s_ow,int s_ss);
extern int A_CheckSwitchTile(int i);
extern int A_Spawn(int j,int pn);
extern void G_DoSpriteAnimations(int x,int y,int a,int smoothratio);
extern int main(int argc,char **argv);
extern void G_OpenDemoWrite(void);
extern void G_CloseDemoWrite(void);
extern void G_BonusScreen(int bonusonly);
extern void A_SpawnWallGlass(int i,int wallnum,int n);
extern void A_SpawnGlass(int i,int n);
extern void A_SpawnCeilingGlass(int i,int sectnum,int n);
extern void A_SpawnRandomGlass(int i,int wallnum,int n);
extern inline int GetTime(void);
extern void CONFIG_GetSetupFilename(void);
extern int32 CONFIG_FunctionNameToNum(char *func);
extern char *CONFIG_FunctionNumToName(int32 func);
extern int32 CONFIG_AnalogNameToNum(char *func);
extern char *CONFIG_AnalogNumToName(int32 func);
extern void CONFIG_SetDefaults(void);
extern void CONFIG_ReadKeys(void);
extern void ReadSaveGameHeaders(void);
extern int32 CONFIG_ReadSetup(void);
extern void CONFIG_WriteSetup(void);
extern inline void G_UpdateInterpolations(void);
extern void G_SetInterpolation(int *posptr);
extern void G_StopInterpolation(int *posptr);
extern void G_DoInterpolations(int smoothratio);
extern inline void G_RestoreInterpolations(void);
extern inline int G_CheckForSpaceCeiling(int sectnum);
extern inline int G_CheckForSpaceFloor(int sectnum);
extern void P_AddAmmo(int weapon,DukePlayer_t *p,int amount);
extern void P_AddWeaponNoSwitch(DukePlayer_t *p,int weapon);
extern void P_AddWeapon(DukePlayer_t *p,int weapon);
extern void P_SelectNextInvItem(DukePlayer_t *p);
extern void P_CheckWeapon(DukePlayer_t *p);
extern void A_RadiusDamage(int i,int r,int hp1,int hp2,int hp3,int hp4);
extern int A_MoveSprite(int spritenum,int xchange,int ychange,int zchange,unsigned int cliptype);
extern inline int A_SetSprite(int i,unsigned int cliptype);
extern void A_AddToDeleteQueue(int i);
extern void A_SpawnMultiple(int sp,int pic,int n);
extern void A_DoGuts(int sp,int gtype,int n);
extern void Sect_SetInterpolation(int i);
extern void Sect_ClearInterpolation(int i);
extern int A_IncurDamage(int sn);
extern void G_MoveWorld(void);
extern void A_MoveCyclers(void);
extern void A_MoveDummyPlayers(void);

// game.c
extern inline void G_SetStatusBarScale(int sc);

extern void P_SetGamePalette(DukePlayer_t *player, char *pal, int set);
extern void fadepal(int r, int g, int b, int start, int end, int step);

extern int gametext_z(int small, int starttile, int x,int y,const char *t,int s,int p,int orientation,int x1, int y1, int x2, int y2,int z);
extern void G_DrawTXDigiNumZ(int starttile, int x,int y,int n,int s,int pal,int cs,int x1, int y1, int x2, int y2, int z);
extern void G_DrawTileSmall(int x,int y,int tilenum,int shade,int orientation);
extern void G_DrawTilePalSmall(int x,int y,int tilenum,int shade,int orientation,int p);
extern void Gv_ResetVars(void);
extern void A_ResetVars(int iActor);

extern int minitext_(int x,int y,const char *t,int s,int p,int sb);

#define minitextshade(x, y, t, s, p, sb) minitext_(x,y,t,s,p,sb)
#define minitext(x, y, t, p, sb) minitext_(x,y,t,0,p,sb)

#define gametext(x,y,t,s,dabits) gametext_z(0,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define gametextscaled(x,y,t,s,dabits) gametext_z(1,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define gametextpal(x,y,t,s,p) gametext_z(0,STARTALPHANUM, x,y,t,s,p,26,0, 0, xdim-1, ydim-1, 65536)
#define gametextpalbits(x,y,t,s,p,dabits) gametext_z(0,STARTALPHANUM, x,y,t,s,p,dabits,0, 0, xdim-1, ydim-1, 65536)

extern void G_InitDynamicTiles();
extern void G_ProcessDynamicTileMapping(const char *szLabel, int lValue);

extern void A_LoadActor(int sActor);

extern int Gv_GetVarByLabel(const char *szGameLabel, int lDefault, int iActor, int iPlayer);
extern void Gv_DumpValues(FILE *fp);
// extern void AddLog(const char *psz, ...);

extern void Gv_ResetSystemDefaults(void);
extern void Gv_InitWeaponPointers(void);
extern void Gv_Init(void);
extern void Gv_WriteSave(FILE *fil);
extern int Gv_ReadSave(int fil);

extern int __fastcall Gv_GetVar(int id, int iActor, int iPlayer);
extern void __fastcall Gv_SetVar(int id, int lValue, int iActor, int iPlayer);
extern int __fastcall Gv_GetVarX(int id);
extern void __fastcall Gv_SetVarX(int id, int lValue);

// extern void SetGameArrayID(int id,int index, int lValue);

extern int Gv_NewVar(const char *pszLabel, int lValue, unsigned int dwFlags);
extern int Gv_NewArray(const char *pszLabel, int asize);
extern void C_ReportError(int iError);

extern void onvideomodechange(int newmode);

extern void X_OnEvent(int iEventID, int sActor, int sPlayer, int lDist);

extern int A_CheckSoundPlaying(int i, int num);
extern int S_CheckSoundPlaying(int i, int num);
extern void A_StopSound(int num, int i);
extern void G_UpdatePlayerFromMenu(void);
extern void Net_SendPlayerName(void);
extern void Net_SendUserMapName(void);
extern void Net_SendQuit(void);

extern void G_AddUserQuote(const char *daquote);
extern void Net_NewGame(int volume, int level);

extern inline int A_CheckSpriteFlags(int iActor, int iType);
extern inline int A_CheckSpriteTileFlags(int iPicnum, int iType);

extern int G_GetTeamPalette(int team);

extern void se40code(int x,int y,int z,int a,int h, int smoothratio);

extern void G_FreeMapState(int mapnum);
extern void G_FindLevelForFilename(const char *fn, char *volume, char *level);

extern void G_GetCrosshairColor(void);
extern void G_SetCrosshairColor(int r, int g, int b);
extern int kopen4loadfrommod(char *filename, char searchfirst);

extern int _EnterText(int small,int x,int y,char *t,int dalen,int c);

#define G_EnterText(x, y, t, dalen, c) _EnterText(0,x,y,t,dalen,c)
#define Net_EnterText(x, y, t, dalen, c) _EnterText(1,x,y,t,dalen,c)

#endif // __funct_h__
