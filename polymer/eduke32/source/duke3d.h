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

#ifndef _duke3d_h_
# define _duke3d_h_

#ifdef __cplusplus
extern "C" {
#endif

// JBF
#include "compat.h"
#include "a.h"
#include "build.h"
#ifdef POLYMER
# include "polymer.h"
#else
#ifdef POLYMOST
# include "polymost.h"
#endif
#endif
#include "cache1d.h"
#include "pragmas.h"

#include "baselayer.h"

#include "function.h"

#include "macros.h"

#include "enet/enet.h"

extern ENetHost * g_netServer;
extern ENetHost * g_netClient;
extern ENetPeer * g_netClientPeer;

enum netchan_t
{
    CHAN_MOVE,      // unreliable movement packets
    CHAN_GAMESTATE, // gamestate changes... frags, respawns, player names, etc
    CHAN_SYNC,      // client join sync packets
    CHAN_CHAT,      // chat and RTS
    CHAN_MISC,      // whatever else
    CHAN_MAX
};

#define APPNAME "EDuke32"
#define VERSION " 2.0.0devel"
// this is checked against http://eduke32.com/VERSION
extern const char *s_buildDate;
#define HEAD2 APPNAME VERSION

#define HORIZ_MIN -99
#define HORIZ_MAX 299

extern int32_t g_scriptVersion, g_Shareware, g_gameType;

#define GAMEDUKE 0
#define GAMENAM 1
#define GAMEWW2 3

#define VOLUMEALL (g_Shareware==0)
#define PLUTOPAK (g_scriptVersion==14)
#define VOLUMEONE (g_Shareware==1)
#define NAM (g_gameType&1)
#define WW2GI (g_gameType&2)

#define MAXSLEEPDIST  16384
#define SLEEPTIME 24*64

#define BYTEVERSION_13  27
#define BYTEVERSION_14  116
#define BYTEVERSION_15  117

#define BYTEVERSION_JF  195 // increase by 3, because atomic GRP adds 1, and Shareware adds 2

#define BYTEVERSION (BYTEVERSION_JF+(PLUTOPAK?1:(VOLUMEONE<<1)))    // JBF 20040116: different data files give different versions

#define NUMPAGES 1

#define AUTO_AIM_ANGLE          48
#define RECSYNCBUFSIZ 2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ 2

#define FOURSLEIGHT (1<<8)

#define MAXVOLUMES 7
#define MAXLEVELS 32


#include "file_lib.h"
#include "gamedefs.h"
#include "keyboard.h"
#include "util_lib.h"
#include "mathutil.h"
#include "function.h"
#include "fx_man.h"
#include "config.h"
#include "sounds.h"
#include "control.h"
#include "_rts.h"
#include "rts.h"
#include "soundefs.h"

#include "music.h"

#include "namesdyn.h"

#define TICRATE (120)
#define GAMETICSPERSEC 26 // used as a constant to satisfy all of the calculations written with ticrate = 26 in mind
#define TICSPERFRAME 4 // this used to be TICRATE/GAMETICSPERSEC which was 4.615~ truncated to 4 by integer division
#define REALGAMETICSPERSEC 30

#define MAXSOUNDS 2560

#define STAT_DEFAULT        0
#define STAT_ACTOR          1
#define STAT_ZOMBIEACTOR    2
#define STAT_EFFECTOR       3
#define STAT_PROJECTILE     4
#define STAT_MISC           5
#define STAT_STANDABLE      6
#define STAT_LOCATOR        7
#define STAT_ACTIVATOR      8
#define STAT_TRANSPORT      9
#define STAT_PLAYER         10
#define STAT_FX             11
#define STAT_FALLER         12
#define STAT_DUMMYPLAYER    13

#define    ALT_IS_PRESSED ( KB_KeyPressed( sc_RightAlt ) || KB_KeyPressed( sc_LeftAlt ) )
#define    SHIFTS_IS_PRESSED ( KB_KeyPressed( sc_RightShift ) || KB_KeyPressed( sc_LeftShift ) )
#define    RANDOMSCRAP A_InsertSprite(s->sectnum,s->x+(krand()&255)-128,s->y+(krand()&255)-128,s->z-(8<<8)-(krand()&8191),SCRAP6+(krand()&15),-8,48,48,krand()&2047,(krand()&63)+64,-512-(krand()&2047),i,5)

#define    PHEIGHT (38<<8)

enum GameMode_t {
    MODE_MENU       = 0x00000001,
    MODE_DEMO       = 0x00000002,
    MODE_GAME       = 0x00000004,
    MODE_EOL        = 0x00000008,
    MODE_TYPE       = 0x00000010,
    MODE_RESTART    = 0x00000020,
    MODE_SENDTOWHOM = 0x00000040,
};

#define MAXANIMWALLS 512
#define MAXINTERPOLATIONS MAXSPRITES

#define MAXQUOTES 16384

#define FIRST_OBITUARY_QUOTE MAXQUOTES-128
#define FIRST_SUICIDE_QUOTE MAXQUOTES-32

#define MAXCYCLERS 1024

#define MAXANIMATES 256

// Defines the motion characteristics of an actor

enum ActorMoveFlags_t {
    face_player         = 1,
    geth                = 2,
    getv                = 4,
    random_angle        = 8,
    face_player_slow    = 16,
    spin                = 32,
    face_player_smart   = 64,
    fleeenemy           = 128,
    jumptoplayer        = 257,
    seekplayer          = 512,
    furthestdir         = 1024,
    dodgebullet         = 4096
};

// Some misc #defines
#define NO       0
#define YES      1

// Defines for 'useractor' keyword

enum UserActorTypes_t {
    notenemy,
    enemy,
    enemystayput
};

// Player Actions.

enum PlayerActionFlags_t {
    pstanding       = 1,
    pwalking        = 2,
    prunning        = 4,
    pducking        = 8,
    pfalling        = 16,
    pjumping        = 32,
    phigher         = 64,
    pwalkingback    = 128,
    prunningback    = 256,
    pkicking        = 512,
    pshrunk         = 1024,
    pjetpack        = 2048,
    ponsteroids     = 4096,
    ponground       = 8192,
    palive          = 16384,
    pdead           = 32768,
    pfacing         = 65536
};

enum DukeInventory_t {
    GET_STEROIDS,
    GET_SHIELD,
    GET_SCUBA,
    GET_HOLODUKE,
    GET_JETPACK,
    GET_DUMMY1,
    GET_ACCESS,
    GET_HEATS,
    GET_DUMMY2,
    GET_FIRSTAID,
    GET_BOOTS,
    GET_MAX
};

enum DukeWeapon_t {
    KNEE_WEAPON,
    PISTOL_WEAPON,
    SHOTGUN_WEAPON,
    CHAINGUN_WEAPON,
    RPG_WEAPON,
    HANDBOMB_WEAPON,
    SHRINKER_WEAPON,
    DEVISTATOR_WEAPON,
    TRIPBOMB_WEAPON,
    FREEZE_WEAPON,
    HANDREMOTE_WEAPON,
    GROW_WEAPON,
    MAX_WEAPONS
};

#define deletesprite A_DeleteSprite
void A_DeleteSprite(int32_t s);

#pragma pack(push,1)

typedef struct {
    uint32_t bits; // 4b
    int16_t fvel, svel; // 4b
    int8_t avel, horz; // 2b
    int8_t extbits, filler; // 2b
} input_t;

#define sync dsync  // JBF 20040604: sync is a function on some platforms
extern input_t recsync[RECSYNCBUFSIZ];

extern int32_t movefifosendplc;

typedef struct {
    int16_t voice;
    int16_t i;
} SOUNDOWNER;

#define MAXSOUNDINSTANCES 8

typedef struct {
    int32_t  length, num, soundsiz; // 12b
    char *filename, *ptr, *filename1; // 12b/24b
    SOUNDOWNER SoundOwner[MAXSOUNDINSTANCES]; // 32b
    int16_t ps,pe,vo; // 6b
    char pr,m; // 2b
} sound_t;

extern volatile char g_soundlocks[MAXSOUNDS];
extern sound_t g_sounds[MAXSOUNDS];

typedef struct {
    int16_t wallnum, tag;
} animwalltype;

extern animwalltype animwall[MAXANIMWALLS];

extern int16_t g_numAnimWalls;
extern int32_t probey;

extern char typebuflen;
extern char typebuf[141];
extern int32_t g_musicSize;
extern int32_t msx[2048],msy[2048];
extern int16_t cyclers[MAXCYCLERS][6],g_numCyclers;
extern char szPlayerName[32];

struct savehead {
    char name[19];
    int32_t numplr,volnum,levnum,plrskl;
    char boardfn[BMAX_PATH];
};

typedef struct {
    vec3_t camera;
    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt,crosshairscale;

    int32_t runkey_mode,statusbarscale,mouseaiming,weaponswitch,drawweapon;   // JBF 20031125
    int32_t democams,color,msgdisptime,statusbarmode;
    int32_t m_noexits,noexits,autovote,automsg,idplayers;
    int32_t team, viewbob, weaponsway, althud, weaponscale, textscale;

    int32_t entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32_t coords,tickrate,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int32_t playerai,angleinterpolation,obituaries;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int32_t m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32_t player_skill,level_number,volume_number,m_marker,marker,mouseflip;

    int32_t configversion;

    int16_t cameraang, camerasect, camerahoriz;
    int16_t pause_on,from_bonus;
    int16_t camerasprite,last_camsprite;
    int16_t last_level,secretlevel, bgstretch;

    struct {
        int32_t UseJoystick;
        int32_t UseMouse;
        int32_t AutoAim;
        int32_t ShowOpponentWeapons;
        int32_t MouseDeadZone,MouseBias;
        int32_t SmoothInput;

        // JBF 20031211: Store the input settings because
        // (currently) jmact can't regurgitate them
        int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
        int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
        int32_t MouseAnalogueAxes[MAXMOUSEAXES];
        int32_t MouseAnalogueScale[MAXMOUSEAXES];
        int32_t JoystickFunctions[MAXJOYBUTTONS][2];
        int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
        int32_t JoystickAnalogueAxes[MAXJOYAXES];
        int32_t JoystickAnalogueScale[MAXJOYAXES];
        int32_t JoystickAnalogueDead[MAXJOYAXES];
        int32_t JoystickAnalogueSaturate[MAXJOYAXES];
        uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];

        //
        // Sound variables
        //
        int32_t FXDevice;
        int32_t MusicDevice;
        int32_t FXVolume;
        int32_t MusicVolume;
        int32_t SoundToggle;
        int32_t MusicToggle;
        int32_t VoiceToggle;
        int32_t AmbienceToggle;

        int32_t NumVoices;
        int32_t NumChannels;
        int32_t NumBits;
        int32_t MixRate;

        int32_t ReverseStereo;

        //
        // Screen variables
        //

        int32_t ScreenMode;

        int32_t ScreenWidth;
        int32_t ScreenHeight;
        int32_t ScreenBPP;

        int32_t ForceSetup;
        int32_t NoAutoLoad;

        int32_t scripthandle;
        int32_t setupread;

        int32_t CheckForUpdates;
        int32_t LastUpdateCheck;
        int32_t useprecache;
    } config;

    char overhead_on,last_overhead,showweapons;
    char god,warp_on,cashman,eog,showallmap;
    char show_help,scrollmode,clipping;
    char ridecule[10][40];
    char savegame[10][22];
    char pwlockout[128],rtsname[128];
    char display_bonus_screen;
    char show_level_text;
} user_defs;

typedef struct {
    int32_t ox,oy,oz;
    int16_t oa,os;
} PlayerSpawn_t;

extern char g_numPlayerSprites;

extern int32_t fricxv,fricyv;

// TODO: make a dummy player struct for interpolation and sort the first members
// of this struct into the same order so we can just memcpy it and get rid of the
// mywhatever type globals

typedef struct {
    vec3_t pos, opos, posvel;
    int32_t bobposx, bobposy;
    int32_t truefz, truecz, player_par;
    int32_t randomflamex, exitx, exity;
    int32_t runspeed, max_player_health, max_shield_amount;

    uint32_t interface_toggle_flag;

    uint8_t *palette;

    uint16_t max_actors_killed, actors_killed;
    uint16_t gotweapon, zoom;

    int16_t loogiex[64], loogiey[64], sbs, sound_pitch;

    int16_t ang, oang, angvel, cursectnum, look_ang, last_extra, subweapon;
    int16_t max_ammo_amount[MAX_WEAPONS], ammo_amount[MAX_WEAPONS], inv_amount[GET_MAX];
    int16_t wackedbyactor, pyoff, opyoff;

    int16_t horiz, horizoff, ohoriz, ohorizoff;
    int16_t newowner, jumping_counter, airleft;
    int16_t fta, ftq, access_wallnum, access_spritenum;
    int16_t got_access, weapon_ang, visibility;
    int16_t somethingonplayer, on_crane, i, one_parallax_sectnum;
    int16_t random_club_frame, one_eighty_count;
    int16_t dummyplayersprite, extra_extra8;
    int16_t actorsqu, timebeforeexit, customexitsound, last_pissed_time;

    int16_t weaprecs[MAX_WEAPONS], weapon_sway, crack_time, bobcounter;

    int16_t orotscrnang, rotscrnang, dead_flag;   // JBF 20031220: added orotscrnang
    int16_t holoduke_on, pycount;

    uint8_t max_secret_rooms, secret_rooms;
    uint8_t frag, fraggedself, quick_kick, last_quick_kick;
    uint8_t return_to_center, reloading, weapreccnt;
    uint8_t aim_mode, auto_aim, weaponswitch, movement_lock, team;
    uint8_t tipincs, hbomb_hold_delay, frag_ps, kickback_pic;

    uint8_t gm, on_warping_sector, footprintcount, hurt_delay;
    uint8_t hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
    uint8_t inven_icon, buttonpalette, over_shoulder_on, show_empty_weapon;

    uint8_t jetpack_on, spritebridge, lastrandomspot;
    uint8_t scuba_on, footprintpal, heat_on, invdisptime;

    uint8_t holster_weapon, falling_counter, footprintshade;
    uint8_t refresh_inventory, last_full_weapon;

    uint8_t toggle_key_flag, knuckle_incs, knee_incs, access_incs;
    uint8_t walking_snd_toggle, palookup, hard_landing, fist_incs;

    int8_t numloogs, loogcnt, scream_voice, transporter_hold;
    int8_t last_weapon, cheat_phase, weapon_pos, wantweaponfire, curr_weapon;

    palette_t pals;

    char name[32];
} DukePlayer_t;

extern char tempbuf[2048], packbuf[4096], menutextbuf[128];

extern int32_t g_spriteGravity;

extern int32_t g_impactDamage,g_actorRespawnTime,g_itemRespawnTime;
extern int32_t g_startArmorAmount;

#define MOVFIFOSIZ 256

extern int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos,g_spriteDeleteQueueSize;
extern user_defs ud;
extern int16_t g_globalRandom;
extern char buf[1024]; //My own generic input buffer

#define MAXQUOTELEN 128

extern char *ScriptQuotes[MAXQUOTES],*ScriptQuoteRedefinitions[MAXQUOTES];
extern char ready2send;

void VM_ScriptInfo(void);
extern intptr_t *script,*insptr,*labelcode,*labeltype;
extern int32_t g_numLabels,g_numDefaultLabels;
extern int32_t g_scriptSize;
extern char *label;
extern intptr_t *actorscrptr[MAXTILES],*g_parsingActorPtr;
extern intptr_t *actorLoadEventScrptr[MAXTILES];
extern char ActorType[MAXTILES];

extern char g_musicIndex;
extern char EnvMusicFilename[MAXVOLUMES+1][BMAX_PATH];
extern int16_t camsprite;

typedef struct {
    int32_t workslike, cstat; // 8b
    int32_t hitradius, range, flashcolor; // 12b
    int16_t spawns, sound, isound, vel; // 8b
    int16_t decal, trail, tnum, drop; // 8b
    int16_t offset, bounces, bsound; // 6b
    int16_t toffset; // 2b
    int16_t extra, extra_rand; // 4b
    int8_t sxrepeat, syrepeat, txrepeat, tyrepeat; // 4b
    int8_t shade, xrepeat, yrepeat, pal; // 4b
    int8_t velmult; // 1b
    uint8_t clipdist; // 1b
    int8_t filler[6]; // 6b
} projectile_t;

typedef struct {
    intptr_t t_data[10]; // 40b/80b sometimes used to hold pointers to con code

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; //6b

    int32_t flags, bposx,bposy,bposz; //16b
    int32_t floorz,ceilingz,lastvx,lastvy; //16b
    int32_t lasttransport; //4b

    int16_t lightId, lightcount, lightmaxrange, cgg; //8b
    int16_t actorstayput, dispicnum, shootzvel; // 6b

#ifdef POLYMER
    _prlight *lightptr; //4b/8b
#else
    void *lightptr;
#endif

    projectile_t *projectile; //4b/8b

    int8_t filler[16]; // pad struct to 128 bytes
} ActorData_t;

// this struct needs to match the beginning of ActorData_t above
typedef struct {
    intptr_t t_data[10]; // 40b/80b sometimes used to hold pointers to con code

    int16_t picnum,ang,extra,owner; //8b
    int16_t movflag,tempang,timetosleep; // 6b

    int32_t flags; // 4b
} NetActorData_t;

extern ActorData_t actor[MAXSPRITES];

extern input_t loc;
extern input_t recsync[RECSYNCBUFSIZ];
extern input_t avg;

extern int32_t numplayers, myconnectindex;
extern int32_t connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)
extern int32_t screenpeek;

extern int32_t g_currentMenu;
extern int32_t tempwallptr,g_animateCount;
extern int32_t lockclock;
extern intptr_t frameplace;
extern char display_mirror,g_loadFromGroupOnly,g_RTSPlaying;

extern int32_t g_groupFileHandle;
extern int32_t ototalclock;

extern int32_t *animateptr[MAXANIMATES];
extern int32_t animategoal[MAXANIMATES];
extern int32_t animatevel[MAXANIMATES];
extern int16_t neartagsector, neartagwall, neartagsprite;
extern int32_t neartaghitdist;
extern int16_t animatesect[MAXANIMATES];
extern int32_t movefifoplc, vel,svel,angvel,horiz;

extern int16_t g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;

#include "funct.h"

extern int32_t g_screenCapture;
extern char EpisodeNames[MAXVOLUMES][33];
extern char SkillNames[5][33];

extern int32_t g_currentFrameRate;

#define MAXGAMETYPES 16
extern char GametypeNames[MAXGAMETYPES][33];
extern int32_t GametypeFlags[MAXGAMETYPES];
extern char g_numGametypes;

enum GametypeFlags_t {
    GAMETYPE_COOP                   = 0x00000001,
    GAMETYPE_WEAPSTAY               = 0x00000002,
    GAMETYPE_FRAGBAR                = 0x00000004,
    GAMETYPE_SCORESHEET             = 0x00000008,
    GAMETYPE_DMSWITCHES             = 0x00000010,
    GAMETYPE_COOPSPAWN              = 0x00000020,
    GAMETYPE_ACCESSCARDSPRITES      = 0x00000040,
    GAMETYPE_COOPVIEW               = 0x00000080,
    GAMETYPE_COOPSOUND              = 0x00000100,
    GAMETYPE_OTHERPLAYERSINMAP      = 0x00000200,
    GAMETYPE_ITEMRESPAWN            = 0x00000400,
    GAMETYPE_MARKEROPTION           = 0x00000800,
    GAMETYPE_PLAYERSFRIENDLY        = 0x00001000,
    GAMETYPE_FIXEDRESPAWN           = 0x00002000,
    GAMETYPE_ACCESSATSTART          = 0x00004000,
    GAMETYPE_PRESERVEINVENTORYDEATH = 0x00008000,
    GAMETYPE_TDM                    = 0x00010000,
    GAMETYPE_TDMSPAWN               = 0x00020000
};

extern char g_numVolumes;

extern int32_t g_lastSaveSlot;
extern int32_t g_restorePalette;

extern int32_t cachecount;
extern char boardfilename[BMAX_PATH];
extern uint8_t waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768],*animpal;
extern char currentboardfilename[BMAX_PATH];
extern char cachedebug,g_earthquakeTime;
extern char lumplockbyte[11];

//DUKE3D.H - replace the end "my's" with this
extern vec3_t my;
extern vec3_t my, omy, myvel;
extern int16_t myhoriz, omyhoriz, myhorizoff, omyhorizoff, g_skillSoundID;
extern int16_t myang, omyang, mycursectnum, myjumpingcounter;
extern char myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern int32_t predictfifoplc;
extern int32_t myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern int32_t myhorizbak[MOVEFIFOSIZ];
extern int16_t myangbak[MOVEFIFOSIZ];

extern int16_t BlimpSpawnSprites[15];

//DUKE3D.H:

#pragma pack(pop)
typedef struct {
    int16_t got_access, last_extra, inv_amount[GET_MAX], curr_weapon, holoduke_on;
    int16_t last_weapon, weapon_pos, kickback_pic;
    int16_t ammo_amount[MAX_WEAPONS], frag[MAXPLAYERS];
    uint16_t gotweapon;
    char inven_icon, jetpack_on, heat_on;
} DukeStatus_t;
#pragma pack(push,1)

extern DukeStatus_t sbar;
extern int32_t g_cameraDistance, g_cameraClock, g_playerFriction,g_showShareware;
extern int32_t g_gameQuit;

extern int32_t playerswhenstarted;

extern char pus,pub;
extern int32_t g_damageCameras,g_freezerSelfDamage,g_tripbombLaserMode;

// TENSW: on really bad network connections, the sync FIFO queue can overflow if it is the
// same size as the move fifo.
#define MAXSYNCBYTES 16
#define SYNCFIFOSIZ 1024

extern char syncstat[MAXSYNCBYTES];
extern int8_t multiwho, multipos, multiwhat, multiflag;
extern int32_t syncvaltail, syncvaltottail;
extern int32_t g_numFreezeBounces,g_rpgBlastRadius,g_pipebombBlastRadius,g_tripbombBlastRadius;
extern int32_t g_shrinkerBlastRadius,g_morterBlastRadius,g_bouncemineBlastRadius,g_seenineBlastRadius;
extern int32_t everyothertime;
extern int32_t mymaxlag, otherminlag, bufferjitter;

extern int32_t g_numInterpolations, startofdynamicinterpolations;
extern int32_t g_interpolationLock;
extern int32_t oldipos[MAXINTERPOLATIONS];
extern int32_t bakipos[MAXINTERPOLATIONS];
extern int32_t *curipos[MAXINTERPOLATIONS];

extern int16_t g_numClouds,clouds[128],cloudx[128],cloudy[128];
extern int32_t cloudtotalclock,totalmemory;

extern int32_t stereomode, stereowidth, stereopixelwidth;

extern int32_t g_myAimMode, g_myAimStat, g_oldAimStat;

extern uint32_t g_moveThingsCount;

#define IFISGLMODE if (getrendermode() >= 3)
#define IFISSOFTMODE if (getrendermode() < 3)

#define TILE_SAVESHOT (MAXTILES-1)
#define TILE_LOADSHOT (MAXTILES-3)
#define TILE_TILT     (MAXTILES-2)
#define TILE_ANIM     (MAXTILES-4)
#define TILE_VIEWSCR  (MAXTILES-5)

// the order of these can't be changed or else compatibility with EDuke 2.0 mods will break
enum GameEvent_t {
    EVENT_INIT,
    EVENT_ENTERLEVEL,
    EVENT_RESETWEAPONS,
    EVENT_RESETINVENTORY,
    EVENT_HOLSTER,
    EVENT_LOOKLEFT,
    EVENT_LOOKRIGHT,
    EVENT_SOARUP,
    EVENT_SOARDOWN,
    EVENT_CROUCH,
    EVENT_JUMP,
    EVENT_RETURNTOCENTER,
    EVENT_LOOKUP,
    EVENT_LOOKDOWN,
    EVENT_AIMUP,
    EVENT_FIRE,
    EVENT_CHANGEWEAPON,
    EVENT_GETSHOTRANGE,
    EVENT_GETAUTOAIMANGLE,
    EVENT_GETLOADTILE,
    EVENT_CHEATGETSTEROIDS,
    EVENT_CHEATGETHEAT,
    EVENT_CHEATGETBOOT,
    EVENT_CHEATGETSHIELD,
    EVENT_CHEATGETSCUBA,
    EVENT_CHEATGETHOLODUKE,
    EVENT_CHEATGETJETPACK,
    EVENT_CHEATGETFIRSTAID,
    EVENT_QUICKKICK,
    EVENT_INVENTORY,
    EVENT_USENIGHTVISION,
    EVENT_USESTEROIDS,
    EVENT_INVENTORYLEFT,
    EVENT_INVENTORYRIGHT,
    EVENT_HOLODUKEON,
    EVENT_HOLODUKEOFF,
    EVENT_USEMEDKIT,
    EVENT_USEJETPACK,
    EVENT_TURNAROUND,
    EVENT_DISPLAYWEAPON,
    EVENT_FIREWEAPON,
    EVENT_SELECTWEAPON,
    EVENT_MOVEFORWARD,
    EVENT_MOVEBACKWARD,
    EVENT_TURNLEFT,
    EVENT_TURNRIGHT,
    EVENT_STRAFELEFT,
    EVENT_STRAFERIGHT,
    EVENT_WEAPKEY1,
    EVENT_WEAPKEY2,
    EVENT_WEAPKEY3,
    EVENT_WEAPKEY4,
    EVENT_WEAPKEY5,
    EVENT_WEAPKEY6,
    EVENT_WEAPKEY7,
    EVENT_WEAPKEY8,
    EVENT_WEAPKEY9,
    EVENT_WEAPKEY10,
    EVENT_DRAWWEAPON,
    EVENT_DISPLAYCROSSHAIR,
    EVENT_DISPLAYREST,
    EVENT_DISPLAYSBAR,
    EVENT_RESETPLAYER,
    EVENT_INCURDAMAGE,
    EVENT_AIMDOWN,
    EVENT_GAME,
    EVENT_PREVIOUSWEAPON,
    EVENT_NEXTWEAPON,
    EVENT_SWIMUP,
    EVENT_SWIMDOWN,
    EVENT_GETMENUTILE,
    EVENT_SPAWN,
    EVENT_LOGO,
    EVENT_EGS,
    EVENT_DOFIRE,
    EVENT_PRESSEDFIRE,
    EVENT_USE,
    EVENT_PROCESSINPUT,
    EVENT_FAKEDOMOVETHINGS,
    EVENT_DISPLAYROOMS,
    EVENT_KILLIT,
    EVENT_LOADACTOR,
    EVENT_DISPLAYBONUSSCREEN,
    EVENT_DISPLAYMENU,
    EVENT_DISPLAYMENUREST,
    EVENT_DISPLAYLOADINGSCREEN,
    EVENT_ANIMATESPRITES,
    EVENT_NEWGAME,
    MAXEVENTS
};

// store global game definitions



enum GamevarFlags_t {
    MAXGAMEVARS        = 2048,       // must be a power of two
    MAXVARLABEL        = 26,
    GAMEVAR_PERPLAYER  = 0x00000001, // per-player variable
    GAMEVAR_PERACTOR   = 0x00000002, // per-actor variable
    GAMEVAR_USER_MASK  = (0x00000001|0x00000002),
    GAMEVAR_RESET      = 0x00000008, // marks var for to default
    GAMEVAR_DEFAULT    = 0x00000100, // allow override
    GAMEVAR_SECRET     = 0x00000200, // don't dump...
    GAMEVAR_NODEFAULT  = 0x00000400, // don't reset on actor spawn
    GAMEVAR_SYSTEM     = 0x00000800, // cannot change mode flags...(only default value)
    GAMEVAR_READONLY   = 0x00001000, // values are read-only (no setvar allowed)
    GAMEVAR_INTPTR     = 0x00002000, // plValues is a pointer to an int32_t
    GAMEVAR_SYNCCHECK  = 0x00004000, // throw warnings during compile if used in local event
    GAMEVAR_SHORTPTR   = 0x00008000, // plValues is a pointer to a short
    GAMEVAR_CHARPTR    = 0x00010000, // plValues is a pointer to a char
    GAMEVAR_NORESET    = 0x00020000, // var values are not reset when restoring map state
    GAMEVAR_SPECIAL    = 0x00040000, // flag for structure member shortcut vars
    GAMEVAR_NOMULTI    = 0x00080000, // don't attach to multiplayer packets
};

enum GamearrayFlags_t {
    MAXGAMEARRAYS      = (MAXGAMEVARS>>2), // must be lower than MAXGAMEVARS
    MAXARRAYLABEL      = MAXVARLABEL,
    GAMEARRAY_NORMAL   = 0,
    GAMEARRAY_NORESET  = 0x00000001,
};

typedef struct {
    union {
        intptr_t lValue;
        intptr_t *plValues;     // array of values when 'per-player', or 'per-actor'
    } val;
    intptr_t lDefault;
    uintptr_t dwFlags;
    char *szLabel;
} gamevar_t;

typedef struct {
    char *szLabel;
    int32_t *plValues;     // array of values
    intptr_t size;
    intptr_t bReset;
} gamearray_t;

extern gamevar_t aGameVars[MAXGAMEVARS];
extern gamearray_t aGameArrays[MAXGAMEARRAYS];
extern int32_t g_gameVarCount;
extern int32_t g_gameArrayCount;

extern int32_t SpriteFlags[MAXTILES];

enum SpriteFlags_t {
    SPRITE_SHADOW       = 0x00000001,
    SPRITE_NVG          = 0x00000002,
    SPRITE_NOSHADE      = 0x00000004,
    SPRITE_PROJECTILE   = 0x00000008,
    SPRITE_DECAL        = 0x00000010,
    SPRITE_BADGUY       = 0x00000020,
    SPRITE_NOPAL        = 0x00000040,
    SPRITE_NOEVENTCODE  = 0x00000080,
    SPRITE_NOLIGHT      = 0x00000100,
    SPRITE_USEACTIVATOR = 0x00000200,
    SPRITE_NULL         = 0x00000400, // null sprite in multiplayer
};

extern int16_t SpriteCacheList[MAXTILES][3];

extern int32_t g_iReturnVarID;
extern int32_t g_iWeaponVarID;
extern int32_t g_iWorksLikeVarID;
extern int32_t g_iZRangeVarID;
extern int32_t g_iAngRangeVarID;
extern int32_t g_iAimAngleVarID;
extern int32_t g_iLoTagID;          // var ID of "LOTAG"
extern int32_t g_iHiTagID;          // var ID of "HITAG"
extern int32_t g_iTextureID;        // var ID of "TEXTURE"

extern char g_bEnhanced;    // are we 'enhanced' (more minerals, etc)

extern char g_szBuf[1024];

#define NAM_GRENADE_LIFETIME        120
#define NAM_GRENADE_LIFETIME_VAR    30

extern intptr_t *aplWeaponClip[MAX_WEAPONS];            // number of items in clip
extern intptr_t *aplWeaponReload[MAX_WEAPONS];          // delay to reload (include fire)
extern intptr_t *aplWeaponFireDelay[MAX_WEAPONS];       // delay to fire
extern intptr_t *aplWeaponHoldDelay[MAX_WEAPONS];       // delay after release fire button to fire (0 for none)
extern intptr_t *aplWeaponTotalTime[MAX_WEAPONS];       // The total time the weapon is cycling before next fire.
extern intptr_t *aplWeaponFlags[MAX_WEAPONS];           // Flags for weapon
extern intptr_t *aplWeaponShoots[MAX_WEAPONS];          // what the weapon shoots
extern intptr_t *aplWeaponSpawnTime[MAX_WEAPONS];       // the frame at which to spawn an item
extern intptr_t *aplWeaponSpawn[MAX_WEAPONS];           // the item to spawn
extern intptr_t *aplWeaponShotsPerBurst[MAX_WEAPONS];   // number of shots per 'burst' (one ammo per 'burst'
extern intptr_t *aplWeaponWorksLike[MAX_WEAPONS];       // What original the weapon works like
extern intptr_t *aplWeaponInitialSound[MAX_WEAPONS];    // Sound made when initialy firing. zero for no sound
extern intptr_t *aplWeaponFireSound[MAX_WEAPONS];       // Sound made when firing (each time for automatic)
extern intptr_t *aplWeaponSound2Time[MAX_WEAPONS];      // Alternate sound time
extern intptr_t *aplWeaponSound2Sound[MAX_WEAPONS];     // Alternate sound sound ID
extern intptr_t *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
extern intptr_t *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted
extern intptr_t *aplWeaponSelectSound[MAX_WEAPONS];     // Sound for weapon selection
extern intptr_t *aplWeaponFlashColor[MAX_WEAPONS];      // Color for polymer muzzle flash

extern int32_t g_timerTicsPerSecond;

enum WeaponFlags_t {
    WEAPON_SPAWNTYPE1          = 0x00000000, // just spawn
    WEAPON_HOLSTER_CLEARS_CLIP = 0x00000001, // 'holstering' clears the current clip
    WEAPON_GLOWS               = 0x00000002, // weapon 'glows' (shrinker and grower)
    WEAPON_AUTOMATIC           = 0x00000004, // automatic fire (continues while 'fire' is held down
    WEAPON_FIREEVERYOTHER      = 0x00000008, // during 'hold time' fire every frame
    WEAPON_FIREEVERYTHIRD      = 0x00000010, // during 'hold time' fire every third frame
    WEAPON_RANDOMRESTART       = 0x00000020, // restart for automatic is 'randomized' by RND 3
    WEAPON_AMMOPERSHOT         = 0x00000040, // uses ammo for each shot (for automatic)
    WEAPON_BOMB_TRIGGER        = 0x00000080, // weapon is the 'bomb' trigger
    WEAPON_NOVISIBLE           = 0x00000100, // weapon use does not cause user to become 'visible'
    WEAPON_THROWIT             = 0x00000200, // weapon 'throws' the 'shoots' item...
    WEAPON_CHECKATRELOAD       = 0x00000400, // check weapon availability at 'reload' time
    WEAPON_STANDSTILL          = 0x00000800, // player stops jumping before actual fire (like tripbomb in duke)
    WEAPON_SPAWNTYPE2          = 0x00001000, // spawn like shotgun shells
    WEAPON_SPAWNTYPE3          = 0x00002000, // spawn like chaingun shells
    WEAPON_SEMIAUTO            = 0x00004000, // cancel button press after each shot
    WEAPON_RELOAD_TIMING       = 0x00008000, // special casing for pistol reload sounds
    WEAPON_RESET               = 0x00010000  // cycle weapon back to frame 1 if fire is held, 0 if not
};

#define TRIPBOMB_TRIPWIRE           0x00000001
#define TRIPBOMB_TIMER              0x00000002

#define PIPEBOMB_REMOTE             0x00000001
#define PIPEBOMB_TIMER              0x00000002

// custom projectiles

enum ProjectileFlags_t {
    PROJECTILE_HITSCAN             = 0x00000001,
    PROJECTILE_RPG                 = 0x00000002,
    PROJECTILE_BOUNCESOFFWALLS     = 0x00000004,
    PROJECTILE_BOUNCESOFFMIRRORS   = 0x00000008,
    PROJECTILE_KNEE                = 0x00000010,
    PROJECTILE_WATERBUBBLES        = 0x00000020,
    PROJECTILE_TIMED               = 0x00000040,
    PROJECTILE_BOUNCESOFFSPRITES   = 0x00000080,
    PROJECTILE_SPIT                = 0x00000100,
    PROJECTILE_COOLEXPLOSION1      = 0x00000200,
    PROJECTILE_BLOOD               = 0x00000400,
    PROJECTILE_LOSESVELOCITY       = 0x00000800,
    PROJECTILE_NOAIM               = 0x00001000,
    PROJECTILE_RANDDECALSIZE       = 0x00002000,
    PROJECTILE_EXPLODEONTIMER      = 0x00004000,
    PROJECTILE_RPG_IMPACT          = 0x00008000,
    PROJECTILE_RADIUS_PICNUM       = 0x00010000,
    PROJECTILE_ACCURATE_AUTOAIM    = 0x00020000,
    PROJECTILE_FORCEIMPACT         = 0x00040000,
    PROJECTILE_REALCLIPDIST        = 0x00080000,
    PROJECTILE_ACCURATE            = 0x00100000,
};

extern projectile_t ProjectileData[MAXTILES], DefaultProjectileData[MAXTILES], SpriteProjectile[MAXSPRITES];

// logo control

enum LogoFlags_t {
    LOGO_ENABLED           = 0x00000001,
    LOGO_PLAYANIM          = 0x00000002,
    LOGO_PLAYMUSIC         = 0x00000004,
    LOGO_3DRSCREEN         = 0x00000008,
    LOGO_TITLESCREEN       = 0x00000010,
    LOGO_DUKENUKEM         = 0x00000020,
    LOGO_THREEDEE          = 0x00000040,
    LOGO_PLUTOPAKSPRITE    = 0x00000080,
    LOGO_SHAREWARESCREENS  = 0x00000100,
    LOGO_TENSCREEN         = 0x00000200
};

extern int32_t g_numRealPalettes;
extern int32_t g_scriptDebug;

#define MAXCHEATLEN 20
#define NUMCHEATCODES (int32_t)(sizeof(CheatStrings)/sizeof(CheatStrings[0]))

extern char CheatKeys[2];
extern char CheatStrings[][MAXCHEATLEN];

extern int16_t WeaponPickupSprites[MAX_WEAPONS];

extern int32_t g_numQuoteRedefinitions;

extern char setupfilename[BMAX_PATH];

typedef struct {
// this needs to have a copy of everything related to the map/actor state
// see savegame.c
    int32_t animategoal[MAXANIMATES], animatevel[MAXANIMATES], g_animateCount;
    int32_t animateptr[MAXANIMATES];
    int32_t lockclock;
    int32_t msx[2048], msy[2048];
    int32_t randomseed, g_globalRandom;
    intptr_t *vars[MAXGAMEVARS];

    int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos;
    int16_t animatesect[MAXANIMATES];
    int16_t cyclers[MAXCYCLERS][6];
    int16_t g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;
    int16_t g_numAnimWalls;
    int16_t g_numClouds,clouds[128],cloudx[128],cloudy[128];
    int16_t g_numCyclers;
    int16_t headspritesect[MAXSECTORS+1];
    int16_t headspritestat[MAXSTATUS+1];
    int16_t nextspritesect[MAXSPRITES];
    int16_t nextspritestat[MAXSPRITES];
    int16_t numsectors;
    int16_t numwalls;
    int16_t prevspritesect[MAXSPRITES];
    int16_t prevspritestat[MAXSPRITES];
    int16_t pskyoff[MAXPSKYTILES], pskybits;

    uint8_t g_earthquakeTime;
    uint8_t g_numPlayerSprites;
    uint8_t scriptptrs[MAXSPRITES];
    uint8_t show2dsector[(MAXSECTORS+7)>>3];

    ActorData_t actor[MAXSPRITES];
    PlayerSpawn_t g_playerSpawnPoints[MAXPLAYERS];
    animwalltype animwall[MAXANIMWALLS];
    sectortype sector[MAXSECTORS];
    spriteext_t spriteext[MAXSPRITES];
    spritetype sprite[MAXSPRITES];
    walltype wall[MAXWALLS];
} mapstate_t;

extern void G_SaveMapState(mapstate_t *save);
extern void G_RestoreMapState(mapstate_t *save);

typedef struct {
    int32_t partime, designertime;
    char *name, *filename, *musicfn, *alt_musicfn;
    mapstate_t *savedstate;
} map_t;

extern map_t MapInfo[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

typedef struct {
    DukePlayer_t *ps;
    input_t *sync;

    int32_t movefifoend, syncvalhead;
    int16_t ping, filler;
    int32_t pcolor, pteam;
    uint8_t frags[MAXPLAYERS], wchoice[MAX_WEAPONS];

    char vote, gotvote, playerreadyflag, playerquitflag;
    char user_name[32];
    char syncval[SYNCFIFOSIZ][MAXSYNCBYTES];
} playerdata_t;

extern input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern PlayerSpawn_t g_playerSpawnPoints[MAXPLAYERS];
extern playerdata_t g_player[MAXPLAYERS];
#include "funct.h"

// key bindings stuff
typedef struct {
    char *name;
    int32_t id;
} keydef_t;

extern keydef_t ConsoleKeys[];
extern char *ConsoleButtons[];

extern char *g_grpNamePtr, *g_gameNamePtr;
extern char g_modDir[BMAX_PATH];

extern hashtable_t h_gamevars;
extern hashtable_t h_arrays;
extern hashtable_t h_keywords;
extern hashtable_t h_gamefuncs;
extern hashtable_t h_labels;

enum DukePacket_t
{
    PACKET_MASTER_TO_SLAVE,
    PACKET_SLAVE_TO_MASTER,

    PACKET_NUM_PLAYERS,
    PACKET_PLAYER_INDEX,
    PACKET_PLAYER_DISCONNECTED,
    PACKET_PLAYER_SPAWN,
    PACKET_FRAG,
    PACKET_REQUEST_GAMESTATE,
    PACKET_LOAD_GAME,
    PACKET_VERSION,
    PACKET_AUTH,
    PACKET_PLAYER_READY,

    // any packet with an ID higher than PACKET_BROADCAST is rebroadcast by server
    // so hacked clients can't create fake server packets and get the server to
    // send them to everyone
    // newer versions of the netcode also make this determination based on which
    // channel the packet was broadcast on

    PACKET_BROADCAST,
    PACKET_NEW_GAME,
    PACKET_RTS,
    PACKET_CLIENT_INFO,
    PACKET_MESSAGE,
    PACKET_USER_MAP,

    PACKET_MAP_VOTE,
    PACKET_MAP_VOTE_INITIATE,
    PACKET_MAP_VOTE_CANCEL,
};

enum NetDisconnect_t
{
    DISC_BAD_PASSWORD = 1,
    DISC_KICKED,
    DISC_BANNED
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif
