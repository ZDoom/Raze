{
engine_main_arrays_are_static;
engine_v8;

saveboard_maptext;
loadboard_maptext;

sector;
wall;
sprite;
tsprite;
spriteext;

Numsprites;
numsectors;
numwalls;
numyaxbunches;
spritesortcnt;
guniqhudid;

qsetmode;
rendmode;
totalclock;
randomseed;
xdim;
ydim;
windowxy1;
windowxy2;
yxaspect;
viewingrange;

sectorofwall_noquick;
getceilzofslopeptr;
getflorzofslopeptr;
getzsofslopeptr;
spriteheightofsptr;

changespritesect;
changespritestat;

headspritesect;
headspritestat;
prevspritesect;
prevspritestat;
nextspritesect;
nextspritestat;
tilesiz;
picanm;

show2dsector;

headsectbunch;
nextsectbunch;

krand;
ksqrt;
inside;
getangle;
Mulscale;
cansee;
hitscan;
dragpoint;
neartag;
getzrange;
clipmovex;
rotatesprite_;
setaspect;

getclosestcol_lim;
palookup;
palette;
getblendtab;
setblendtab;
setpalookup;

kopen4load;
kfilelength;
kclose;
kread;
klseek;

pathsearchmode;
klistfree;
klistpath;

dist;
ldist;

updatesector;
updatesectorz;
updatesectorbreadth;

getticks;
gethiticks;
OSD_Printf;

Bcrc32;

luaJIT_BC_defs_common;
luaJIT_BC_engine_maptext;
luaJIT_BC_engine;

g_argv;


g_elCONSize;
g_elCON;
El_SetCON;
El_OnError;

basepaltable;

g_elSavecode;
El_FreeSaveCode;
El_SerializeGamevars;
El_RestoreGamevars;
El_GetLabelValue;

s_buildRev;
g_sizes_of_what;
g_sizes_of;
g_elFirstTime;
g_elCallDepth;
block_deletesprite;
myconnectindex;
g_RETURN;
g_elModules;
g_modDir;
g_elSessionVar;

g_mapInfo;
g_volumeNames;

kopen4loadfrommod;

g_scriptModules;
g_scriptModulesNum;

G_ConFile;
G_DoGameStartup;
C_DefineSound;
C_DefineMusic;
C_DefineQuote;
C_DefineVolumeName;
C_DefineVolumeFlags;
C_UndefineVolume;
C_DefineSkillName;
C_UndefineSkill;
C_DefineLevelName;
C_UndefineLevel;
C_DefineProjectile;
C_DefineGameFuncName;
C_DefineGameType;
C_SetDefName;
C_SetCfgName;

SCRIPT_GetNumber;
SCRIPT_PutNumber;

actor;
g_camera;
ud;
g_player;
g_player_ps;
g_playerWeapon;
g_weaponOverridden;
WeaponPickupSprites;
g_tile;
SpriteProjectile;

g_noResetVars;
A_ResetVars;

g_dynTileList;
g_dynSoundList;

apStrings;

screenpeek;
hudweap;
g_mostConcurrentPlayers;
g_deleteQueueSize;
g_blimpSpawnItems;
g_scriptVersion;
g_frameRate;
g_currentMenu;
g_earthquakeTime;
g_moveThingsCount;
CheatKeys;
g_logoFlags;

luaJIT_BC_lunacon;
luaJIT_BC_con_lang;
luaJIT_BC_randgen;
luaJIT_BC_stat;
luaJIT_BC_bitar;
luaJIT_BC_control;
luaJIT_BC_bcarray;
luaJIT_BC_bcheck;
luaJIT_BC_xmath;
luaJIT_BC__defs_game;
luaJIT_BC_v;
luaJIT_BC_dump;
luaJIT_BC_dis_x86;
luaJIT_BC_dis_x64;
luaJIT_BC_lunasave;
luaJIT_BC_fs;

rand_jkiss_u32;
rand_jkiss_dbl;

A_IncurDamage;
G_CheckActivatorMotion;
A_Dodge;
A_MoveSpriteClipdist;
P_DoQuote;
P_SetGamePalette;
G_AddUserQuote;
G_ClearCameraView;
VM_DrawTileGeneric;
G_InitTimer;
G_GetTimeDate;
G_ToggleWallInterpolation;
G_StartTrack;
VM_CheckSquished2;
Menu_Change;

KB_ScanCodeToString;

A_CheckAnySoundPlaying;
S_CheckSoundPlaying;
S_StopEnvSound;
S_StopAllSounds;
S_ChangeSoundPitch;
S_GetMusicPosition;
S_SetMusicPosition;

minitext_;
G_DrawTXDigiNumZ;
G_PrintGameText;
G_ScreenText;
G_ScreenTextSize;
G_PrintYourTime;
G_PrintParTime;
G_PrintDesignerTime;
G_PrintBestTime;

G_UpdateScreenArea;
G_SaveMapState;
G_RestoreMapState;
G_FreeMapState;
};
