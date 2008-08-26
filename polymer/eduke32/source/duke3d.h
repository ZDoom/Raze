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

// this is checked against http://eduke32.com/VERSION
#define BUILDDATE " 20080826"
#define APPNAME "EDuke32"
#define VERSION " 1.5.0devel"
#define HEAD2 APPNAME VERSION BUILDDATE

#ifdef __cplusplus
extern "C" {
#endif

// JBF
#include "compat.h"
#include "a.h"
#include "build.h"
#if defined(POLYMOST) && defined(USE_OPENGL)
# include "polymost.h"
#endif
#ifdef POLYMER
# include "polymer.h"
#endif
#include "cache1d.h"
#include "pragmas.h"
#include "mmulti.h"

#include "baselayer.h"

#include "function.h"

extern int g_ScriptVersion, g_Shareware, g_GameType;

#define GAMEDUKE 0
#define GAMENAM 1
#define GAMEWW2 3

#define VOLUMEALL (g_Shareware==0)
#define PLUTOPAK (g_ScriptVersion==14)
#define VOLUMEONE (g_Shareware==1)
#define NAM (g_GameType&1)
#define WW2GI (g_GameType&2)

#define MAXSLEEPDIST  16384
#define SLEEPTIME 24*64

#define BYTEVERSION_13  27
#define BYTEVERSION_14  116
#define BYTEVERSION_15  117
#define BYTEVERSION_JF  183 // increase by 3, because atomic GRP adds 1, and Shareware adds 2

#define BYTEVERSION (BYTEVERSION_JF+(PLUTOPAK?1:(VOLUMEONE<<1)))    // JBF 20040116: different data files give different versions

#define NUMPAGES 1

#define AUTO_AIM_ANGLE          48
#define RECSYNCBUFSIZ 2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ 256

#define FOURSLEIGHT (1<<8)

#define MAXVOLUMES 7
#define MAXLEVELS 32

#include "types.h"
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
#define TICSPERFRAME (TICRATE/26)

// #define GC (TICSPERFRAME*44)

#define MAXSOUNDS 2560

/*
#pragma aux sgn =\
        "add ebx, ebx",\
        "sbb eax, eax",\
        "cmp eax, ebx",\
        "adc eax, 0",\
        parm [ebx]\
*/

#define    ALT_IS_PRESSED ( KB_KeyPressed( sc_RightAlt ) || KB_KeyPressed( sc_LeftAlt ) )
#define    SHIFTS_IS_PRESSED ( KB_KeyPressed( sc_RightShift ) || KB_KeyPressed( sc_LeftShift ) )
#define    RANDOMSCRAP EGS(s->sectnum,s->x+(TRAND&255)-128,s->y+(TRAND&255)-128,s->z-(8<<8)-(TRAND&8191),SCRAP6+(TRAND&15),-8,48,48,TRAND&2047,(TRAND&63)+64,-512-(TRAND&2047),i,5)

#define    PHEIGHT (38<<8)

enum gamemodes {
    MODE_MENU       = 1,
    MODE_DEMO       = 2,
    MODE_GAME       = 4,
    MODE_EOL        = 8,
    MODE_TYPE       = 16,
    MODE_RESTART    = 32,
    MODE_SENDTOWHOM = 64,
    MODE_END        = 128
};

#define MAXANIMWALLS 512
#define MAXINTERPOLATIONS MAXSPRITES

#define MAXQUOTES 16384

#define PPDEATHSTRINGS MAXQUOTES-128
#define PSDEATHSTRINGS MAXQUOTES-32

#define MAXCYCLERS 1024

#define MAXANIMATES 256

#define SP  sprite[i].yvel
#define SX  sprite[i].x
#define SY  sprite[i].y
#define SZ  sprite[i].z
#define SS  sprite[i].shade
#define PN  sprite[i].picnum
#define SA  sprite[i].ang
#define SV  sprite[i].xvel
#define ZV  sprite[i].zvel
#define RX  sprite[i].xrepeat
#define RY  sprite[i].yrepeat
#define OW  sprite[i].owner
#define CS  sprite[i].cstat
#define SH  sprite[i].extra
#define CX  sprite[i].xoffset
#define CY  sprite[i].yoffset
#define CD  sprite[i].clipdist
#define PL  sprite[i].pal
#define SLT  sprite[i].lotag
#define SHT  sprite[i].hitag
#define SECT sprite[i].sectnum

// Defines the motion characteristics of an actor

enum actormotion_flags {
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

enum useractor_types {
    notenemy,
    enemy,
    enemystayput
};

// Player Actions.

enum player_action_flags {
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

enum inventory_indexes {
    GET_STEROIDS,
    GET_SHIELD,
    GET_SCUBA,
    GET_HOLODUKE,
    GET_JETPACK,
    GET_ACCESS   = 6,
    GET_HEATS,
    GET_FIRSTAID = 9,
    GET_BOOTS
};

#define TRAND krand()

enum weapon_indexes {
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

#define T1  hittype[i].temp_data[0]
#define T2  hittype[i].temp_data[1]
#define T3  hittype[i].temp_data[2]
#define T4  hittype[i].temp_data[3]
#define T5  hittype[i].temp_data[4]
#define T6  hittype[i].temp_data[5]
#define T7  hittype[i].temp_data[6]
#define T8  hittype[i].temp_data[7]
#define T9  hittype[i].temp_data[8]

#define ESCESCAPE if(KB_KeyPressed( sc_Escape ) ) gameexit(" ");

#define IFWITHIN(B,E) if((PN)>=(B) && (PN)<=(E))

#define deletesprite deletespriteEVENT
void deletespriteEVENT(int s);
#define KILLIT(KX) {deletesprite(KX);goto BOLT;}


#define IFMOVING if(ssp(i,CLIPMASK0))
#define IFHIT j=ifhitbyweapon(i);if(j >= 0)
#define IFHITSECT j=ifhitsectors(s->sectnum);if(j >= 0)

#define AFLAMABLE(X) (X==BOX||X==TREE1||X==TREE2||X==TIRE||X==CONE)

#define rnd(X) ((TRAND>>8)>=(255-(X)))

#define __USRHOOKS_H

enum USRHOOKS_Errors {
    USRHOOKS_Warning = -2,
    USRHOOKS_Error   = -1,
    USRHOOKS_Ok      = 0
};

typedef struct {
    unsigned int bits, extbits;
    short fvel, svel;
    signed char avel, horz;
} input_t;

#define sync dsync  // JBF 20040604: sync is a function on some platforms
extern input_t recsync[RECSYNCBUFSIZ];

extern int movefifosendplc;

typedef struct {
    int voice;
    int i;
} SOUNDOWNER;

typedef struct {
    int  length, num, soundsiz;
    char *filename, *ptr, *filename1;
    SOUNDOWNER SoundOwner[4];
    short ps,pe,vo;
    char pr,m;
    volatile char lock;
} sound_t;

extern sound_t g_sounds[MAXSOUNDS];

typedef struct {
    int wallnum, tag;
} animwalltype;

extern animwalltype animwall[MAXANIMWALLS];

extern short numanimwalls;
extern int probey;

extern char typebuflen,typebuf[141];
extern char *MusicPtr;
extern int Musicsize;
extern int msx[2048],msy[2048];
extern short cyclers[MAXCYCLERS][6],numcyclers;
extern char myname[32];

struct savehead {
    char name[19];
    int numplr,volnum,levnum,plrskl;
    char boardfn[BMAX_PATH];
};

typedef struct {
    int UseJoystick;
    int UseMouse;
    int RunMode;
    int AutoAim;
    int ShowOpponentWeapons;
    int MouseFilter,MouseBias;
    int SmoothInput;

    // JBF 20031211: Store the input settings because
    // (currently) jmact can't regurgitate them
    int MouseFunctions[MAXMOUSEBUTTONS][2];
    int MouseDigitalFunctions[MAXMOUSEAXES][2];
    int MouseAnalogueAxes[MAXMOUSEAXES];
    int MouseAnalogueScale[MAXMOUSEAXES];
    int JoystickFunctions[MAXJOYBUTTONS][2];
    int JoystickDigitalFunctions[MAXJOYAXES][2];
    int JoystickAnalogueAxes[MAXJOYAXES];
    int JoystickAnalogueScale[MAXJOYAXES];
    int JoystickAnalogueDead[MAXJOYAXES];
    int JoystickAnalogueSaturate[MAXJOYAXES];
    byte KeyboardKeys[NUMGAMEFUNCTIONS][2];

    //
    // Sound variables
    //
    int FXDevice;
    int MusicDevice;
    int FXVolume;
    int MusicVolume;
    int SoundToggle;
    int MusicToggle;
    int VoiceToggle;
    int AmbienceToggle;

    int NumVoices;
    int NumChannels;
    int NumBits;
    int MixRate;
    
    int ReverseStereo;

    //
    // Screen variables
    //

    int ScreenMode;

    int ScreenWidth;
    int ScreenHeight;
    int ScreenBPP;

    int ForceSetup;

    int scripthandle;
    int setupread;

    int CheckForUpdates;
    int LastUpdateCheck;
    int useprecache;
} config_t;

typedef struct {
    int const_visibility,uw_framerate;
    int camera_time,folfvel,folavel,folx,foly,fola;
    int reccnt,crosshairscale;

    int runkey_mode,statusbarscale,mouseaiming,weaponswitch,drawweapon;   // JBF 20031125
    int democams,color,msgdisptime,statusbarmode;
    int m_noexits,noexits,autovote,automsg,idplayers;
    int team, viewbob, weaponsway, althud;

    int entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int coords,tickrate,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int playerai,angleinterpolation,obituaries;

    int respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int player_skill,level_number,volume_number,m_marker,marker,mouseflip;

    int camerax,cameray,cameraz;
    int configversion;

    short cameraang, camerasect, camerahoriz;
    short pause_on,from_bonus;
    short camerasprite,last_camsprite;
    short last_level,secretlevel;

    char overhead_on,last_overhead,showweapons;
    char god,warp_on,cashman,eog,showallmap;
    char show_help,scrollmode,clipping;
    char ridecule[10][40];
    char savegame[10][22];
    char pwlockout[128],rtsname[128];
    char display_bonus_screen;

    config_t config;
} user_defs;

typedef struct {
    int ox,oy,oz;
    short oa,os;
} playerspawn_t;

extern char numplayersprites;

extern int fricxv,fricyv;

typedef struct {
    int zoom,exitx,exity;
    int posx, posy, posz, horiz, ohoriz, ohorizoff, invdisptime;
    int bobposx,bobposy,oposx,oposy,oposz,pyoff,opyoff;
    int posxv,posyv,poszv,last_pissed_time,truefz,truecz;
    int player_par,visibility;
    int bobcounter,weapon_sway;
    int pals_time,randomflamex,crack_time;

    unsigned int interface_toggle_flag;

    int max_secret_rooms,secret_rooms,max_actors_killed,actors_killed;
    int runspeed, movement_lock, team;
    int max_player_health, max_shield_amount, max_ammo_amount[MAX_WEAPONS];

    int scream_voice;

    int loogiex[64],loogiey[64],numloogs,loogcnt;

    char *palette;

    short sbs, sound_pitch;

    short ang,oang,angvel,cursectnum,look_ang,last_extra,subweapon;
    short ammo_amount[MAX_WEAPONS],wackedbyactor,frag,fraggedself;

    short curr_weapon, last_weapon, tipincs, horizoff, wantweaponfire;
    short holoduke_amount,newowner,hurt_delay,hbomb_hold_delay;
    short jumping_counter,airleft,knee_incs,access_incs;
    short fta,ftq,access_wallnum,access_spritenum;
    short kickback_pic,got_access,weapon_ang,firstaid_amount;
    short somethingonplayer,on_crane,i,one_parallax_sectnum;
    short over_shoulder_on,random_club_frame,fist_incs;
    short one_eighty_count,cheat_phase;
    short dummyplayersprite,extra_extra8,quick_kick;
    short heat_amount,actorsqu,timebeforeexit,customexitsound;

    short weaprecs[16],weapreccnt;


    short orotscrnang,rotscrnang,dead_flag,show_empty_weapon;   // JBF 20031220: added orotscrnang
    short scuba_amount,jetpack_amount,steroids_amount,shield_amount;
    short holoduke_on,pycount,weapon_pos,frag_ps;
    short transporter_hold,last_full_weapon,footprintshade,boot_amount;

    char aim_mode,auto_aim,weaponswitch;

    char gm,on_warping_sector,footprintcount;
    char hbomb_on,jumping_toggle,rapid_fire_hold,on_ground;
    char inven_icon,buttonpalette;

    char jetpack_on,spritebridge,lastrandomspot;
    char scuba_on,footprintpal,heat_on;

    char  holster_weapon,falling_counter;
    char  gotweapon[MAX_WEAPONS],refresh_inventory;

    char toggle_key_flag,knuckle_incs; // ,select_dir;
    char walking_snd_toggle, palookup, hard_landing;
    char /*fire_flag,*/pals[3];
    char return_to_center, reloading, name[32];
} player_struct;

extern char tempbuf[2048], packbuf[576], menutextbuf[128];

extern int gc;

extern int impact_damage,respawnactortime,respawnitemtime;
extern int start_armour_amount;

#define MOVFIFOSIZ 256

extern short spriteq[1024],spriteqloc,spriteqamount;
extern user_defs ud;
extern short int moustat;
extern short int global_random;
extern char buf[1024]; //My own generic input buffer

#define MAXQUOTELEN 128

extern char *fta_quotes[MAXQUOTES],*redefined_quotes[MAXQUOTES];
extern char ready2send;

// JBF 20040531: adding 16 extra to the script so we have some leeway
// to (hopefully) safely abort when hitting the limit
void scriptinfo();
extern intptr_t *script,*scriptptr,*insptr,*labelcode,*labeltype;
extern int labelcnt,defaultlabelcnt;
extern int g_ScriptSize;
extern char *label;
extern intptr_t *actorscrptr[MAXTILES],*parsing_actor;
extern intptr_t *actorLoadEventScrptr[MAXTILES];
extern char actortype[MAXTILES];
extern char *music_pointer;

extern char music_select;
extern char env_music_fn[MAXVOLUMES+1][BMAX_PATH];
extern short camsprite;

typedef struct {
    int workslike, extra, cstat, extra_rand, hitradius, range;
    short spawns, sound, isound, vel, decal, trail, tnum, drop, clipdist, offset, bounces, bsound, toffset;
    signed char sxrepeat, syrepeat, txrepeat, tyrepeat, shade, xrepeat, yrepeat, pal, velmult;
} projectile_t;

// extern char gotz;

typedef struct {
/*    int x;
    int y;
    int z; */
    short ang, oldang, angdir, angdif;
} spriteinterpolate;

spriteinterpolate sprpos[MAXSPRITES];

typedef struct {
    int floorz,ceilingz,lastvx,lastvy,bposx,bposy,bposz;
    int flags;
    intptr_t temp_data[10]; // sometimes used to hold pointers to con code
    short picnum,ang,extra,owner,movflag;
    short tempang,actorstayput,dispicnum;
    short timetosleep;
    char cgg;
    projectile_t projectile;
} actordata_t;

extern actordata_t hittype[MAXSPRITES];

extern input_t loc;
extern input_t recsync[RECSYNCBUFSIZ];
extern int avgfvel, avgsvel, avgavel, avghorz, avgbits, avgextbits;

extern int numplayers, myconnectindex;
extern int connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)
extern int screenpeek;

extern int current_menu;
extern int tempwallptr,animatecnt;
extern int lockclock;
extern intptr_t frameplace;
extern char display_mirror,loadfromgrouponly,rtsplaying;

extern int groupfile;
extern int ototalclock;

extern int *animateptr[MAXANIMATES];
extern int animategoal[MAXANIMATES];
extern int animatevel[MAXANIMATES];
// extern int oanimateval[MAXANIMATES];
extern short neartagsector, neartagwall, neartagsprite;
extern int neartaghitdist;
extern short animatesect[MAXANIMATES];
extern int movefifoplc, vel,svel,angvel,horiz;

extern short mirrorwall[64], mirrorsector[64], mirrorcnt;

#include "funct.h"

extern int screencapt;
extern char volume_names[MAXVOLUMES][33];
extern char skill_names[5][33];

extern int framerate;

#define MAXGAMETYPES 16
extern char gametype_names[MAXGAMETYPES][33];
extern int gametype_flags[MAXGAMETYPES];
extern char num_gametypes;

enum gametypeflags {
    GAMETYPE_FLAG_COOP                   = 1,
    GAMETYPE_FLAG_WEAPSTAY               = 2,
    GAMETYPE_FLAG_FRAGBAR                = 4,
    GAMETYPE_FLAG_SCORESHEET             = 8,
    GAMETYPE_FLAG_DMSWITCHES             = 16,
    GAMETYPE_FLAG_COOPSPAWN              = 32,
    GAMETYPE_FLAG_ACCESSCARDSPRITES      = 64,
    GAMETYPE_FLAG_COOPVIEW               = 128,
    GAMETYPE_FLAG_COOPSOUND              = 256,
    GAMETYPE_FLAG_OTHERPLAYERSINMAP      = 512,
    GAMETYPE_FLAG_ITEMRESPAWN            = 1024,
    GAMETYPE_FLAG_MARKEROPTION           = 2048,
    GAMETYPE_FLAG_PLAYERSFRIENDLY        = 4096,
    GAMETYPE_FLAG_FIXEDRESPAWN           = 8192,
    GAMETYPE_FLAG_ACCESSATSTART          = 16384,
    GAMETYPE_FLAG_PRESERVEINVENTORYDEATH = 32768,
    GAMETYPE_FLAG_TDM                    = 65536,
    GAMETYPE_FLAG_TDMSPAWN               = 131072
};

#define GTFLAGS(x) (gametype_flags[ud.coop] & x)

extern char num_volumes;

extern int lastsavedpos;
extern int restorepalette;
extern int packetrate;

extern int cachecount;
extern char boardfilename[BMAX_PATH],waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768],animpal[768];
extern char cachedebug,earthquaketime;
extern int networkmode;
extern char lumplockbyte[11];

//DUKE3D.H - replace the end "my's" with this
extern int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
extern short myhoriz, omyhoriz, myhorizoff, omyhorizoff, globalskillsound;
extern short myang, omyang, mycursectnum, myjumpingcounter;
extern char myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern int fakemovefifoplc;
extern int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern int myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];

extern short weaponsandammosprites[15];

//DUKE3D.H:
typedef struct {
    short got_access, last_extra, shield_amount, curr_weapon, holoduke_on;
    short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
    short heat_amount, scuba_amount, boot_amount;
    short last_weapon, weapon_pos, kickback_pic;
    short ammo_amount[MAX_WEAPONS], frag[MAXPLAYERS];
    char inven_icon, jetpack_on, heat_on, gotweapon[MAX_WEAPONS];
} STATUSBARTYPE;

extern STATUSBARTYPE sbar;
extern int cameradist, cameraclock, dukefriction,show_shareware;
extern int networkmode, movesperpacket;
extern int gamequit;

extern char pus,pub;
extern int camerashitable,freezerhurtowner,lasermode;
extern char syncstat;
extern signed char multiwho, multipos, multiwhat, multiflag;
extern int syncvaltail, syncvaltottail;
extern int numfreezebounces,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
extern int everyothertime;
extern int mymaxlag, otherminlag, bufferjitter;

extern int numinterpolations, startofdynamicinterpolations;
extern int oldipos[MAXINTERPOLATIONS];
extern int bakipos[MAXINTERPOLATIONS];
extern int *curipos[MAXINTERPOLATIONS];

extern short numclouds,clouds[128],cloudx[128],cloudy[128];
extern int cloudtotalclock,totalmemory;

extern int stereomode, stereowidth, stereopixelwidth;

extern int myaimmode, myaimstat, omyaimstat;

#define IFISGLMODE if (getrendermode() >= 3)
#define IFISSOFTMODE if (getrendermode() < 3)

#define TILE_SAVESHOT (MAXTILES-1)
#define TILE_LOADSHOT (MAXTILES-3)
#define TILE_TILT     (MAXTILES-2)
#define TILE_ANIM     (MAXTILES-4)
#define TILE_VIEWSCR  (MAXTILES-5)

enum events {
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
    MAXEVENTS
};

enum sysstrs {
    STR_MAPNAME,
    STR_MAPFILENAME,
    STR_PLAYERNAME,
    STR_VERSION,
    STR_GAMETYPE
};

// store global game definitions



enum gamevarflags {
    MAXGAMEVARS             = 2048,  // must be a power of two
    MAXVARLABEL             = 26,
    GAMEVAR_FLAG_NORMAL     = 0,     // normal
    GAMEVAR_FLAG_PERPLAYER  = 1,     // per-player variable
    GAMEVAR_FLAG_PERACTOR   = 2,     // per-actor variable
    GAMEVAR_FLAG_USER_MASK  = 3,
    GAMEVAR_FLAG_DEFAULT    = 256,   // allow override
    GAMEVAR_FLAG_SECRET     = 512,   // don't dump...
    GAMEVAR_FLAG_NODEFAULT  = 1024,  // don't reset on actor spawn
    GAMEVAR_FLAG_SYSTEM     = 2048,  // cannot change mode flags...(only default value)
    GAMEVAR_FLAG_READONLY   = 4096,  // values are read-only (no setvar allowed)
    GAMEVAR_FLAG_INTPTR     = 8192,  // plValue is a pointer to an int
    GAMEVAR_FLAG_SYNCCHECK  = 16384, // check event sync when translating
    GAMEVAR_FLAG_SHORTPTR   = 32768, // plValue is a pointer to a short
    GAMEVAR_FLAG_CHARPTR    = 65536, // plValue is a pointer to a char
    GAMEVAR_FLAG_NORESET    = 131072, // var values are not reset when restoring map state
};

enum gamearrayflags {
    MAXGAMEARRAYS           = (MAXGAMEVARS>>2), // must be lower than MAXGAMEVARS
    MAXARRAYLABEL           = MAXVARLABEL,
    GAMEARRAY_FLAG_NORMAL   = 0,
    GAMEARRAY_FLAG_NORESET  = 1,
};

typedef struct {
    intptr_t lValue;
    intptr_t lDefault;
    intptr_t *plValues;     // array of values when 'per-player', or 'per-actor'
    unsigned int dwFlags;
    char *szLabel;
    char bReset;
} gamevar_t;

typedef struct {
    char *szLabel;
    int *plValues;     // array of values
    int size;
    char bReset;
} gamearray_t;

extern gamevar_t aGameVars[MAXGAMEVARS];
extern gamearray_t aGameArrays[MAXGAMEARRAYS];
extern int iGameVarCount;
extern int iGameArrayCount;

extern int spriteflags[MAXTILES];

enum spriteflags {
    SPRITE_FLAG_SHADOW       = 1,
    SPRITE_FLAG_NVG          = 2,
    SPRITE_FLAG_NOSHADE      = 4,
    SPRITE_FLAG_PROJECTILE   = 8,
    SPRITE_FLAG_DECAL        = 16,
    SPRITE_FLAG_BADGUY       = 32,
    SPRITE_FLAG_NOPAL        = 64
};

extern short spritecache[MAXTILES][3];

extern int g_iReturnVarID;
extern int g_iWeaponVarID;
extern int g_iWorksLikeVarID;
extern int g_iZRangeVarID;
extern int g_iAngRangeVarID;
extern int g_iAimAngleVarID;
extern int g_iLoTagID;          // var ID of "LOTAG"
extern int g_iHiTagID;          // ver ID of "HITAG"
extern int g_iTextureID;        // ver ID of "TEXTURE"

extern char g_bEnhanced;    // are we 'enhanced' (more minerals, etc)

extern char g_szBuf[1024];

#define NAM_GRENADE_LIFETIME    120
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

extern short timer;

enum weaponflags {
    WEAPON_FLAG_HOLSTER_CLEARS_CLIP = 1,     // 'holstering' clears the current clip
    WEAPON_FLAG_GLOWS               = 2,     // weapon 'glows' (shrinker and grower)
    WEAPON_FLAG_AUTOMATIC           = 4,     // automatic fire (continues while 'fire' is held down
    WEAPON_FLAG_FIREEVERYOTHER      = 8,     // during 'hold time' fire every frame
    WEAPON_FLAG_FIREEVERYTHIRD      = 16,    // during 'hold time' fire every third frame
    WEAPON_FLAG_RANDOMRESTART       = 32,    // restart for automatic is 'randomized' by RND 3
    WEAPON_FLAG_AMMOPERSHOT         = 64,    // uses ammo for each shot (for automatic)
    WEAPON_FLAG_BOMB_TRIGGER        = 128,   // weapon is the 'bomb' trigger
    WEAPON_FLAG_NOVISIBLE           = 256,   // weapon use does not cause user to become 'visible'
    WEAPON_FLAG_THROWIT             = 512,   // weapon 'throws' the 'shoots' item...
    WEAPON_FLAG_CHECKATRELOAD       = 1024,  // check weapon availability at 'reload' time
    WEAPON_FLAG_STANDSTILL          = 2048,  // player stops jumping before actual fire (like tripbomb in duke)
    WEAPON_FLAG_SPAWNTYPE1          = 0,     // just spawn
    WEAPON_FLAG_SPAWNTYPE2          = 4096,  // spawn like shotgun shells
    WEAPON_FLAG_SPAWNTYPE3          = 8192,  // spawn like chaingun shells
    WEAPON_FLAG_SEMIAUTO            = 16384, // cancel button press after each shot
    WEAPON_FLAG_RELOAD_TIMING       = 32768, // special casing for pistol reload sounds
    WEAPON_FLAG_RESET               = 65536  // cycle weapon back to frame 1 if fire is held, 0 if not
};

#define TRIPBOMB_TRIPWIRE           1
#define TRIPBOMB_TIMER              2

#define PIPEBOMB_REMOTE             1
#define PIPEBOMB_TIMER              2

// custom projectiles

enum projectileflags {
    PROJECTILE_FLAG_HITSCAN             = 1,
    PROJECTILE_FLAG_RPG                 = 2,
    PROJECTILE_FLAG_BOUNCESOFFWALLS     = 4,
    PROJECTILE_FLAG_BOUNCESOFFMIRRORS   = 8,
    PROJECTILE_FLAG_KNEE                = 16,
    PROJECTILE_FLAG_WATERBUBBLES        = 32,
    PROJECTILE_FLAG_TIMED               = 64,
    PROJECTILE_FLAG_BOUNCESOFFSPRITES   = 128,
    PROJECTILE_FLAG_SPIT                = 256,
    PROJECTILE_FLAG_COOLEXPLOSION1      = 512,
    PROJECTILE_FLAG_BLOOD               = 1024,
    PROJECTILE_FLAG_LOSESVELOCITY       = 2048,
    PROJECTILE_FLAG_NOAIM               = 4096,
    PROJECTILE_FLAG_RANDDECALSIZE       = 8192,
    PROJECTILE_FLAG_EXPLODEONTIMER      = 16384,
    PROJECTILE_FLAG_RPG_IMPACT          = 32768,
    PROJECTILE_FLAG_RADIUS_PICNUM       = 65536,
    PROJECTILE_FLAG_ACCURATE_AUTOAIM    = 131072,
    PROJECTILE_FLAG_FORCEIMPACT         = 262144
};

extern projectile_t projectile[MAXTILES], defaultprojectile[MAXTILES];

// logo control

enum logoflags {
    LOGO_FLAG_ENABLED           = 1,
    LOGO_FLAG_PLAYANIM          = 2,
    LOGO_FLAG_PLAYMUSIC         = 4,
    LOGO_FLAG_3DRSCREEN         = 8,
    LOGO_FLAG_TITLESCREEN       = 16,
    LOGO_FLAG_DUKENUKEM         = 32,
    LOGO_FLAG_THREEDEE          = 64,
    LOGO_FLAG_PLUTOPAKSPRITE    = 128,
    LOGO_FLAG_SHAREWARESCREENS  = 256,
    LOGO_FLAG_TENSCREEN         = 512
};

extern int g_NumPalettes;
extern int g_ScriptDebug;

#define MAXCHEATLEN 20
#define NUMCHEATCODES (signed int)(sizeof(cheatstrings)/sizeof(cheatstrings[0]))

extern char cheatkey[2];
extern char cheatstrings[][MAXCHEATLEN];

extern short weapon_sprites[MAX_WEAPONS];

extern int redefined_quote_count;

extern char setupfilename[BMAX_PATH];

typedef struct {
// this needs to have a copy of everything related to the map/actor state
// see savegame.c
    short numwalls;
    walltype wall[MAXWALLS];
    short numsectors;
    sectortype sector[MAXSECTORS];
    spritetype sprite[MAXSPRITES];
    spriteexttype spriteext[MAXSPRITES];
    short headspritesect[MAXSECTORS+1];
    short prevspritesect[MAXSPRITES];
    short nextspritesect[MAXSPRITES];
    short headspritestat[MAXSTATUS+1];
    short prevspritestat[MAXSPRITES];
    short nextspritestat[MAXSPRITES];
    short numcyclers;
    short cyclers[MAXCYCLERS][6];
    playerspawn_t g_PlayerSpawnPoints[MAXPLAYERS];
    short numanimwalls;
    short spriteq[1024],spriteqloc;
    animwalltype animwall[MAXANIMWALLS];
    int msx[2048], msy[2048];
    short mirrorwall[64], mirrorsector[64], mirrorcnt;
    char show2dsector[(MAXSECTORS+7)>>3];
    short numclouds,clouds[128],cloudx[128],cloudy[128];
    actordata_t hittype[MAXSPRITES];
    short pskyoff[MAXPSKYTILES], pskybits;

    int animategoal[MAXANIMATES], animatevel[MAXANIMATES], animatecnt;
    short animatesect[MAXANIMATES];
    int animateptr[MAXANIMATES];
    char numplayersprites;
    char earthquaketime;
    int lockclock;
    int randomseed, global_random;
    char scriptptrs[MAXSPRITES];
    intptr_t *vars[MAXGAMEVARS];
} mapstate_t;

typedef struct {
    int partime, designertime;
    char *name, *filename, *musicfn, *musicfn1;
    mapstate_t *savedstate;
} map_t;

extern map_t map[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

typedef struct {
    player_struct *ps;
    input_t *sync;

    int movefifoend, syncvalhead, myminlag;
    int pcolor, pteam, frags[MAXPLAYERS], wchoice[MAX_WEAPONS];

    char vote, gotvote, playerreadyflag, playerquitflag;
    char user_name[32], syncval[MOVEFIFOSIZ];
} playerdata_t;

extern input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern playerspawn_t g_PlayerSpawnPoints[MAXPLAYERS];
extern playerdata_t g_player[MAXPLAYERS];
#include "funct.h"

// key bindings stuff
typedef struct {
    char *name;
    int id;
} keydef;

extern keydef keynames[];
extern char *mousenames[];

extern char *duke3dgrp, *duke3dgrpstring;
extern char mod_dir[BMAX_PATH];

extern struct HASH_table gamevarH;
extern struct HASH_table arrayH;
extern struct HASH_table keywH;
extern struct HASH_table gamefuncH;

#ifdef __cplusplus
}
#endif
