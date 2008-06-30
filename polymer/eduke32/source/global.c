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
#include "duke3d.h"

char *MusicPtr = NULL;
int Musicsize;

short global_random;
short neartagsector, neartagwall, neartagsprite;

int neartaghitdist,lockclock,start_armour_amount;
// JBF: gc modified to default to Atomic ed. default when using 1.3d CONs
int gc=176;

// int temp_data[MAXSPRITES][6];
actordata_t hittype[MAXSPRITES];

short spriteq[1024],spriteqloc,spriteqamount=64;
animwalltype animwall[MAXANIMWALLS];
short numanimwalls;
int *animateptr[MAXANIMATES];
int animategoal[MAXANIMATES], animatevel[MAXANIMATES], animatecnt;
// int oanimateval[MAXANIMATES];
short animatesect[MAXANIMATES];
int msx[2048],msy[2048];
short cyclers[MAXCYCLERS][6],numcyclers;

char *fta_quotes[MAXQUOTES],*redefined_quotes[MAXQUOTES];

char tempbuf[2048], packbuf[576];

char buf[1024];

short camsprite;
short mirrorwall[64], mirrorsector[64], mirrorcnt;

int current_menu;

map_t map[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

char volume_names[MAXVOLUMES][33] = { "L.A. MELTDOWN", "LUNAR APOCALYPSE", "SHRAPNEL CITY" };
char skill_names[5][33] = { "PIECE OF CAKE", "LET'S ROCK", "COME GET SOME", "DAMN I'M GOOD" };

char gametype_names[MAXGAMETYPES][33] = { "DUKEMATCH (SPAWN)","COOPERATIVE PLAY","DUKEMATCH (NO SPAWN)","TEAM DM (SPAWN)","TEAM DM (NO SPAWN)"};
int gametype_flags[MAXGAMETYPES] = {4+8+16+1024+2048+16384,1+2+32+64+128+256+512+4096+8192+32768,2+4+8+16+16384,4+8+16+1024+2048+16384+65536+131072,2+4+8+16+16384+65536+131072};
char num_gametypes = 5;

short title_zoom;

int framerate;

char num_volumes = 3;

short timer=120;
//fx_device device;

sound_t g_sounds[ MAXSOUNDS ];

char numplayersprites,loadfromgrouponly=0,earthquaketime;

int fricxv,fricyv;
playerdata_t g_player[MAXPLAYERS];
input inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
playerspawn_t g_PlayerSpawnPoints[MAXPLAYERS];
user_defs ud;

char pus, pub;
char syncstat;
int syncvaltail, syncvaltottail;

input loc;
input recsync[RECSYNCBUFSIZ];
int avgfvel, avgsvel, avgavel, avghorz, avgbits, avgextbits;

int movefifosendplc;

//Multiplayer syncing variables
int screenpeek;

//Game recording variables

char ready2send;
int vel, svel, angvel, horiz, ototalclock, respawnactortime=768, respawnitemtime=768, groupfile;

intptr_t *scriptptr,*insptr,*labelcode,*labeltype;
int labelcnt,defaultlabelcnt;
intptr_t *actorscrptr[MAXTILES],*parsing_actor;
char *label;
char *music_pointer;
char actortype[MAXTILES];
intptr_t *script = NULL;

int g_ScriptSize = 16384;

char typebuflen,typebuf[141];

char *music_fn[MAXVOLUMES+1][MAXLEVELS],music_select;
char env_music_fn[MAXVOLUMES+1][BMAX_PATH];
char rtsplaying;


short weaponsandammosprites[15] =
{
    RPGSPRITE__STATIC,
    CHAINGUNSPRITE__STATIC,
    DEVISTATORAMMO__STATIC,
    RPGAMMO__STATIC,
    RPGAMMO__STATIC,
    JETPACK__STATIC,
    SHIELD__STATIC,
    FIRSTAID__STATIC,
    STEROIDS__STATIC,
    RPGAMMO__STATIC,
    RPGAMMO__STATIC,
    RPGSPRITE__STATIC,
    RPGAMMO__STATIC,
    FREEZESPRITE__STATIC,
    FREEZEAMMO__STATIC
};

int impact_damage;
int g_ScriptDebug;

//GLOBAL.C - replace the end "my's" with this
int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
short myhoriz, omyhoriz, myhorizoff, omyhorizoff;
short myang, omyang, mycursectnum, myjumpingcounter;

char myjumpingtoggle, myonground, myhardlanding, myreturntocenter;
signed char multiwho, multipos, multiwhat, multiflag;

int fakemovefifoplc,movefifoplc;
int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
int myhorizbak[MOVEFIFOSIZ],dukefriction = 0xcc00, show_shareware;

short myangbak[MOVEFIFOSIZ];
char myname[32];
int camerashitable,freezerhurtowner=0,lasermode=0;
int networkmode = 255, movesperpacket = 1,gamequit = 0,everyothertime;
int numfreezebounces=3,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
STATUSBARTYPE sbar;

int mymaxlag, otherminlag, bufferjitter = 1;
short numclouds,clouds[128],cloudx[128],cloudy[128];
int cloudtotalclock = 0,totalmemory = 0;
int numinterpolations = 0, startofdynamicinterpolations = 0;
int oldipos[MAXINTERPOLATIONS];
int bakipos[MAXINTERPOLATIONS];
int *curipos[MAXINTERPOLATIONS];

int nextvoxid = 0;

int spriteflags[MAXTILES];

proj_struct projectile[MAXTILES], thisprojectile[MAXSPRITES], defaultprojectile[MAXTILES];

char cheatkey[2] = { sc_D, sc_N };
char setupfilename[BMAX_PATH]= "duke3d.cfg";
char datetimestring[] = ""__DATE__" "__TIME__"";

int doquicksave = 0;

