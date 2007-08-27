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

#define VERSION " 1.4.0 rc1"

#define HEAD  "EDuke32"VERSION" (shareware mode)"
#define HEAD2 "EDuke32"VERSION

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

// JBF
#include "compat.h"
#include "a.h"
#include "build.h"
#if defined(POLYMOST) && defined(USE_OPENGL)
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
#define BYTEVERSION_JF  177 // increase by 3, because atomic GRP adds 1, and Shareware adds 2

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
#include "develop.h"
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

#define NUM_SOUNDS 1500

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

// #define P(X) printf("%ld\n",X);

#define WAIT(X) ototalclock=totalclock+(X);while(totalclock<ototalclock)getpackets()

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

#define MAXSCRIPTSIZE 98304

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

enum actormotion {
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

enum useractortypes {
    notenemy,
    enemy,
    enemystayput
};

// Player Actions.

enum playeractions {
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

enum inventory {
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

enum weapons {
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
#define KILLIT(KX) {deletesprite(KX);goto BOLT;}


#define IFMOVING if(ssp(i,CLIPMASK0))
#define IFHIT j=ifhitbyweapon(i);if(j >= 0)
#define IFHITSECT j=ifhitsectors(s->sectnum);if(j >= 0)

#define AFLAMABLE(X) (X==BOX||X==TREE1||X==TREE2||X==TIRE||X==CONE)


#define IFSKILL1 if(player_skill<1)
#define IFSKILL2 if(player_skill<2)
#define IFSKILL3 if(player_skill<3)
#define IFSKILL4 if(player_skill<4)

#define rnd(X) ((TRAND>>8)>=(255-(X)))

typedef struct {
    short i;
    int voice;
} SOUNDOWNER;

#define __USRHOOKS_H

enum USRHOOKS_Errors {
    USRHOOKS_Warning = -2,
    USRHOOKS_Error   = -1,
    USRHOOKS_Ok      = 0
};

typedef struct {
    signed char avel, horz;
    short fvel, svel;
    unsigned long bits, extbits;
} input;

#define sync dsync  // JBF 20040604: sync is a function on some platforms
extern input recsync[RECSYNCBUFSIZ];

extern long movefifosendplc;

typedef struct {
    char *ptr;
    volatile char lock;
int  length, num;
} SAMPLE;

typedef struct {
short wallnum;
long tag;
} animwalltype;

extern animwalltype animwall[MAXANIMWALLS];

extern short numanimwalls;
extern int probey;

extern char typebuflen,typebuf[141];
extern char MusicPtr[72000*2];
extern long msx[2048],msy[2048];
extern short cyclers[MAXCYCLERS][6],numcyclers;
extern char myname[32];

struct savehead {
    char name[19];
    int32 numplr,volnum,levnum,plrskl;
    char boardfn[BMAX_PATH];
};

typedef struct {
	//
	// Sound variables
	//
	int32 FXDevice;
	int32 MusicDevice;
	int32 FXVolume;
	int32 MusicVolume;
	int32 SoundToggle;
	int32 MusicToggle;
	int32 VoiceToggle;
	int32 AmbienceToggle;

	int32 NumVoices;
	int32 NumChannels;
	int32 NumBits;
	int32 MixRate;
	
	int32 ReverseStereo;

	int32 UseJoystick;
	int32 UseMouse;
	int32 RunMode;
	int32 AutoAim;
	int32 ShowOpponentWeapons;
	int32 MouseFilter,MouseBias;
	int32 SmoothInput;

	// JBF 20031211: Store the input settings because
	// (currently) jmact can't regurgitate them
	byte KeyboardKeys[NUMGAMEFUNCTIONS][2];
	int32 MouseFunctions[MAXMOUSEBUTTONS][2];
	int32 MouseDigitalFunctions[MAXMOUSEAXES][2];
	int32 MouseAnalogueAxes[MAXMOUSEAXES];
	int32 MouseAnalogueScale[MAXMOUSEAXES];
	int32 JoystickFunctions[MAXJOYBUTTONS][2];
	int32 JoystickDigitalFunctions[MAXJOYAXES][2];
	int32 JoystickAnalogueAxes[MAXJOYAXES];
	int32 JoystickAnalogueScale[MAXJOYAXES];
	int32 JoystickAnalogueDead[MAXJOYAXES];
	int32 JoystickAnalogueSaturate[MAXJOYAXES];

	//
	// Screen variables
	//

	int32 ScreenMode;

	int32 ScreenWidth;
	int32 ScreenHeight;
	int32 ScreenBPP;

	int32 ForceSetup;

	int32 scripthandle;
	int32 setupread;

	int32 CheckForUpdates;
	int32 LastUpdateCheck;
	int32 useprecache;
} config_t;

typedef struct {
    char god,warp_on,cashman,eog,showallmap;
    char show_help,scrollmode,clipping;
    char ridecule[10][40];
    char savegame[10][22];
    char pwlockout[128],rtsname[128];
    char overhead_on,last_overhead,showweapons;

    short pause_on,from_bonus;
    short camerasprite,last_camsprite;
    short last_level,secretlevel;

    short cameraang, camerasect, camerahoriz;
    long camerax,cameray,cameraz;

    long const_visibility,uw_framerate;
    long camera_time,folfvel,folavel,folx,foly,fola;
    long reccnt;

    int32 runkey_mode,statusbarscale,mouseaiming,weaponswitch,drawweapon;   // JBF 20031125
    int32 democams,color,msgdisptime,statusbarmode;
    int32 m_noexits,noexits,autovote,automsg,idplayers;
	int32 team, viewbob, weaponsway;

    int32 entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32 coords,tickrate,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int32 playerai,angleinterpolation,deathmsgs;

    int32 respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32 m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int32 m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32 player_skill,level_number,volume_number,m_marker,marker,mouseflip;
	config_t config;
} user_defs;

typedef struct {
    long ox,oy,oz;
    short oa,os;
} player_orig;


extern char numplayersprites;

extern long fricxv,fricyv;

typedef struct {
    long zoom,exitx,exity,loogiex[64],loogiey[64],numloogs,loogcnt;
    long posx, posy, posz, horiz, ohoriz, ohorizoff, invdisptime;
    long bobposx,bobposy,oposx,oposy,oposz,pyoff,opyoff;
    long posxv,posyv,poszv,last_pissed_time,truefz,truecz;
    long player_par,visibility;
    long bobcounter,weapon_sway;
    long pals_time,randomflamex,crack_time;

    char aim_mode,auto_aim,weaponswitch;

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
    unsigned long interface_toggle_flag;

    short orotscrnang,rotscrnang,dead_flag,show_empty_weapon;   // JBF 20031220: added orotscrnang
    short scuba_amount,jetpack_amount,steroids_amount,shield_amount;
    short holoduke_on,pycount,weapon_pos,frag_ps;
    short transporter_hold,last_full_weapon,footprintshade,boot_amount;

    int scream_voice;

    char gm,on_warping_sector,footprintcount;
    char hbomb_on,jumping_toggle,rapid_fire_hold,on_ground;
    char name[32],inven_icon,buttonpalette;

    char jetpack_on,spritebridge,lastrandomspot;
    char scuba_on,footprintpal,heat_on;

    char  holster_weapon,falling_counter;
    char  gotweapon[MAX_WEAPONS],refresh_inventory,*palette;

    char toggle_key_flag,knuckle_incs; // ,select_dir;
    char walking_snd_toggle, palookup, hard_landing;
    char /*fire_flag,*/pals[3];
    char return_to_center, reloading;

    long max_secret_rooms,secret_rooms,max_actors_killed,actors_killed;
    long runspeed, movement_lock, team;
    long max_player_health, max_shield_amount, max_ammo_amount[MAX_WEAPONS];
    short sbs, sound_pitch;
} player_struct;

extern char tempbuf[2048], packbuf[576];

extern long gc;

extern long impact_damage,respawnactortime,respawnitemtime;
extern long start_armour_amount;

#define MOVFIFOSIZ 256

extern short spriteq[1024],spriteqloc,spriteqamount;
extern user_defs ud;
extern short int moustat;
extern short int global_random;
extern long scaredfallz;
extern char buf[1024]; //My own generic input buffer

#define MAXQUOTELEN 128

extern char *fta_quotes[MAXQUOTES],*redefined_quotes[MAXQUOTES];
extern char ready2send;

//extern fx_device device;
extern SAMPLE Sound[ NUM_SOUNDS ];
extern SOUNDOWNER SoundOwner[NUM_SOUNDS][4];

extern char sounds[NUM_SOUNDS][BMAX_PATH];

// JBF 20040531: adding 16 extra to the script so we have some leeway
// to (hopefully) safely abort when hitting the limit
extern long script[MAXSCRIPTSIZE+16],*scriptptr,*insptr,*labelcode,labelcnt,defaultlabelcnt,*labeltype;
extern char *label;
extern long *actorscrptr[MAXTILES],*parsing_actor;
extern long *actorLoadEventScrptr[MAXTILES];
extern char actortype[MAXTILES];
extern char *music_pointer;

extern char music_select;
extern char env_music_fn[MAXVOLUMES+1][BMAX_PATH];
extern short camsprite;

// extern char gotz;
typedef struct {
    char cgg;
    short picnum,ang,extra,owner,movflag;
    short tempang,actorstayput,dispicnum;
    short timetosleep;
    long floorz,ceilingz,lastvx,lastvy,bposx,bposy,bposz;
    long temp_data[10];
} weaponhit;

extern weaponhit hittype[MAXSPRITES];

typedef struct {
    long x;
    long y;
    long z;
    short ang, oldang, angdir, angdif;
} spriteinterpolate;

extern spriteinterpolate sprpos[MAXSPRITES];

extern input loc;
extern input recsync[RECSYNCBUFSIZ];
extern long avgfvel, avgsvel, avgavel, avghorz, avgbits, avgextbits;

extern long numplayers, myconnectindex;
extern long connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)
extern short screenpeek;

extern int current_menu;
extern long tempwallptr,animatecnt;
extern long lockclock,frameplace;
extern char display_mirror,loadfromgrouponly,rtsplaying;

extern long groupfile;
extern long ototalclock;

extern long *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
extern long animatevel[MAXANIMATES];
// extern long oanimateval[MAXANIMATES];
extern short neartagsector, neartagwall, neartagsprite;
extern long neartaghitdist;
extern short animatesect[MAXANIMATES];
extern long movefifoplc, vel,svel,angvel,horiz;

extern short mirrorwall[64], mirrorsector[64], mirrorcnt;

#define NUMKEYS 19

#include "funct.h"

extern int screencapt;
extern short soundps[NUM_SOUNDS],soundpe[NUM_SOUNDS],soundvo[NUM_SOUNDS];
extern char soundpr[NUM_SOUNDS],soundm[NUM_SOUNDS];
extern long soundsiz[NUM_SOUNDS];
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
	GAMETYPE_FLAG_TDM					 = 65536,
	GAMETYPE_FLAG_TDMSPAWN				 = 131072
};

#define GTFLAGS(x) (gametype_flags[ud.coop] & x)

extern char num_volumes;

extern int lastsavedpos;
extern int restorepalette;
extern int packetrate;

extern long cachecount;
extern char boardfilename[BMAX_PATH],waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768],animpal[768];
extern char cachedebug,earthquaketime;
extern int networkmode;
extern char lumplockbyte[11];

//DUKE3D.H - replace the end "my's" with this
extern long myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
extern short myhoriz, omyhoriz, myhorizoff, omyhorizoff, globalskillsound;
extern short myang, omyang, mycursectnum, myjumpingcounter;
extern char myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern long fakemovefifoplc;
extern long myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern long myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];

extern short weaponsandammosprites[15];

//DUKE3D.H:
typedef struct {
    short frag[MAXPLAYERS], got_access, last_extra, shield_amount, curr_weapon;
    short ammo_amount[MAX_WEAPONS], holoduke_on;
    char gotweapon[MAX_WEAPONS], inven_icon, jetpack_on, heat_on;
    short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
    short heat_amount, scuba_amount, boot_amount;
    short last_weapon, weapon_pos, kickback_pic;
} STATUSBARTYPE;

extern STATUSBARTYPE sbar;
extern long cameradist, cameraclock, dukefriction,show_shareware;
extern int networkmode, movesperpacket;
extern int gamequit;

extern char pus,pub;
extern int camerashitable,freezerhurtowner,lasermode;
extern char syncstat;
extern signed char multiwho, multipos, multiwhat, multiflag;
extern long syncvaltail, syncvaltottail;
extern long numfreezebounces,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
extern int everyothertime;
extern long mymaxlag, otherminlag, bufferjitter;

extern long numinterpolations, startofdynamicinterpolations;
extern long oldipos[MAXINTERPOLATIONS];
extern long bakipos[MAXINTERPOLATIONS];
extern long *curipos[MAXINTERPOLATIONS];

extern short numclouds,clouds[128],cloudx[128],cloudy[128];
extern long cloudtotalclock,totalmemory;

extern long stereomode, stereowidth, stereopixelwidth;

extern long myaimmode, myaimstat, omyaimstat;

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
    EVENT_DISPLAYROOMS
};

// store global game definitions

enum gamevarflags {
    MAXGAMEVARS             = 2048,
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
    GAMEVAR_FLAG_PLONG      = 8192,  // plValue is a pointer to a long
    GAMEVAR_FLAG_SYNCCHECK  = 16384, // check event sync when translating
    GAMEVAR_FLAG_PSHORT     = 32768, // plValue is a pointer to a short
    GAMEVAR_FLAG_PCHAR      = 65536  // plValue is a pointer to a char
};

typedef struct {
    long lValue;
    char *szLabel;
    unsigned long dwFlags;

    long *plValues;     // array of values when 'per-player', or 'per-actor'
} MATTGAMEVAR;

extern MATTGAMEVAR aGameVars[MAXGAMEVARS];
extern MATTGAMEVAR aDefaultGameVars[MAXGAMEVARS];
extern int iGameVarCount;

extern int spriteflags[MAXTILES], actorspriteflags[MAXSPRITES];

enum spriteflags {
    SPRITE_FLAG_SHADOW       = 1,
    SPRITE_FLAG_NVG          = 2,
    SPRITE_FLAG_NOSHADE      = 4,
    SPRITE_FLAG_PROJECTILE   = 8,
    SPRITE_FLAG_DECAL        = 16,
    SPRITE_FLAG_BADGUY       = 32
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

extern long *aplWeaponClip[MAX_WEAPONS];            // number of items in clip
extern long *aplWeaponReload[MAX_WEAPONS];          // delay to reload (include fire)
extern long *aplWeaponFireDelay[MAX_WEAPONS];       // delay to fire
extern long *aplWeaponHoldDelay[MAX_WEAPONS];       // delay after release fire button to fire (0 for none)
extern long *aplWeaponTotalTime[MAX_WEAPONS];       // The total time the weapon is cycling before next fire.
extern long *aplWeaponFlags[MAX_WEAPONS];           // Flags for weapon
extern long *aplWeaponShoots[MAX_WEAPONS];          // what the weapon shoots
extern long *aplWeaponSpawnTime[MAX_WEAPONS];       // the frame at which to spawn an item
extern long *aplWeaponSpawn[MAX_WEAPONS];           // the item to spawn
extern long *aplWeaponShotsPerBurst[MAX_WEAPONS];   // number of shots per 'burst' (one ammo per 'burst'
extern long *aplWeaponWorksLike[MAX_WEAPONS];       // What original the weapon works like
extern long *aplWeaponInitialSound[MAX_WEAPONS];    // Sound made when initialy firing. zero for no sound
extern long *aplWeaponFireSound[MAX_WEAPONS];       // Sound made when firing (each time for automatic)
extern long *aplWeaponSound2Time[MAX_WEAPONS];      // Alternate sound time
extern long *aplWeaponSound2Sound[MAX_WEAPONS];     // Alternate sound sound ID
extern long *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
extern long *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted

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
    PROJECTILE_FLAG_ACCURATE_AUTOAIM    = 131072
};

typedef struct {
    int workslike, extra, cstat, extra_rand, hitradius, range;
    short spawns, sound, isound, vel, decal, trail, tnum, drop, clipdist, offset, bounces, bsound, toffset;
    signed char sxrepeat, syrepeat, txrepeat, tyrepeat, shade, xrepeat, yrepeat, pal, velmult;
} proj_struct;

extern proj_struct projectile[MAXTILES], thisprojectile[MAXSPRITES], defaultprojectile[MAXTILES];

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
	LOGO_FLAG_SHAREWARESCREENS	= 256,
	LOGO_FLAG_TENSCREEN			= 512
};

extern int g_NumPalettes;
extern int g_ScriptDebug;

#define MAXCHEATLEN 20
#define NUMCHEATCODES (signed int)(sizeof(cheatquotes)/sizeof(cheatquotes[MAXCHEATLEN]))

extern char cheatkey[2];
extern char cheatquotes[][MAXCHEATLEN];

extern short weapon_sprites[MAX_WEAPONS];

extern int redefined_quote_count;

extern char setupfilename[BMAX_PATH];

typedef struct {
	char *name, *filename, *musicfn;
	long partime, designertime;
} map_t;

extern map_t map[MAXVOLUMES*MAXLEVELS];

typedef struct {
	player_struct *ps;
	input *sync;

	long movefifoend;
	long syncvalhead;
	long myminlag;

	int	frags[MAXPLAYERS];
	int32 pcolor;
	int32 pteam;
	int32 wchoice[MAX_WEAPONS];

	char syncval[MOVEFIFOSIZ];
	char user_name[32];
	char playerreadyflag;
	char playerquitflag;

	char vote;
	char gotvote;
} playerdata_t;

extern input inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern player_orig g_PlayerSpawnPoints[MAXPLAYERS];
extern playerdata_t g_player[MAXPLAYERS];
#include "funct.h"

#ifdef __cplusplus
}
#endif
