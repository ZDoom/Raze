//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

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
#include "gamedef.h"

#include "osd.h"

int conversion = 13;

static char compilefile[BMAX_PATH] = "(none)";  // file we're currently compiling
static char parsing_item_name[MAXVARLABEL] = "(none)", previous_item_name[MAXVARLABEL] = "NULL";

static short total_lines,line_number;
static char checking_ifelse,parsing_state;
char g_szBuf[1024];

long *casescriptptr=NULL;      // the pointer to the start of the case table in a switch statement
// first entry is 'default' code.
int casecount = 0;
signed char checking_switch = 0, current_event = -1;
char labelsonly = 0, nokeywordcheck = 0, dynamicremap = 0;
static short num_braces = 0;    // init to some sensible defaults

int redefined_quote_count = 0;

long *aplWeaponClip[MAX_WEAPONS];       // number of items in magazine
long *aplWeaponReload[MAX_WEAPONS];     // delay to reload (include fire)
long *aplWeaponFireDelay[MAX_WEAPONS];      // delay to fire
long *aplWeaponHoldDelay[MAX_WEAPONS];      // delay after release fire button to fire (0 for none)
long *aplWeaponTotalTime[MAX_WEAPONS];      // The total time the weapon is cycling before next fire.
long *aplWeaponFlags[MAX_WEAPONS];      // Flags for weapon
long *aplWeaponShoots[MAX_WEAPONS];     // what the weapon shoots
long *aplWeaponSpawnTime[MAX_WEAPONS];      // the frame at which to spawn an item
long *aplWeaponSpawn[MAX_WEAPONS];      // the item to spawn
long *aplWeaponShotsPerBurst[MAX_WEAPONS];  // number of shots per 'burst' (one ammo per 'burst'
long *aplWeaponWorksLike[MAX_WEAPONS];      // What original the weapon works like
long *aplWeaponInitialSound[MAX_WEAPONS];   // Sound made when initialy firing. zero for no sound
long *aplWeaponFireSound[MAX_WEAPONS];      // Sound made when firing (each time for automatic)
long *aplWeaponSound2Time[MAX_WEAPONS];     // Alternate sound time
long *aplWeaponSound2Sound[MAX_WEAPONS];    // Alternate sound sound ID
long *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
long *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted

int g_iReturnVarID=-1;      // var ID of "RETURN"
int g_iWeaponVarID=-1;      // var ID of "WEAPON"
int g_iWorksLikeVarID=-1;   // var ID of "WORKSLIKE"
int g_iZRangeVarID=-1;      // var ID of "ZRANGE"
int g_iAngRangeVarID=-1;    // var ID of "ANGRANGE"
int g_iAimAngleVarID=-1;    // var ID of "AUTOAIMANGLE"
int g_iLoTagID=-1;          // var ID of "LOTAG"
int g_iHiTagID=-1;          // var ID of "HITAG"
int g_iTextureID=-1;        // var ID of "TEXTURE"
int g_iThisActorID=-1;      // var ID of "THISACTOR"

long *actorLoadEventScrptr[MAXTILES];

long *apScriptGameEvent[MAXGAMEEVENTS];
long *parsing_event=NULL;

MATTGAMEVAR aGameVars[MAXGAMEVARS];
int iGameVarCount=0;

MATTGAMEVAR aDefaultGameVars[MAXGAMEVARS];  // the 'original' values
int iDefaultGameVarCount=0;

void ReportError(int iError);
void FreeGameVars(void);

extern long qsetmode;

enum errors {
    ERROR_CLOSEBRACKET,
    ERROR_EVENTONLY,
    ERROR_EXCEEDSMAXTILES,
    ERROR_EXPECTEDKEYWORD,
    ERROR_FOUNDWITHIN,
    ERROR_ISAKEYWORD,
    ERROR_NOENDSWITCH,
    ERROR_NOTAGAMEDEF,
    ERROR_NOTAGAMEVAR,
    ERROR_OPENBRACKET,
    ERROR_PARAMUNDEFINED,
    ERROR_SYMBOLNOTRECOGNIZED,
    ERROR_SYNTAXERROR,
    ERROR_VARREADONLY,
    ERROR_VARTYPEMISMATCH,
    WARNING_DUPLICATEDEFINITION,
    WARNING_EVENTSYNC,
    WARNING_LABELSONLY,
};

enum labeltypes {
    LABEL_ANY       = -1,
    LABEL_ACTION    = 1,
    LABEL_AI        = 2,
    LABEL_DEFINE    = 4,
    LABEL_MOVE      = 8,
    LABEL_STATE     = 16,
};

static char *labeltypenames[] = {
                                    "define",
                                    "state",
                                    "actor",
                                    "action",
                                    "ai",
                                    "move"
                                };

static char *translatelabeltype(long type)
{
    int i;
    char x[64];

    x[0] = 0;
    for (i=0;i<6;i++) {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, labeltypenames[i]);
    }
    return strdup(x);
}

#define NUMKEYWORDS (signed int)(sizeof(keyw)/sizeof(keyw[0]))

char *keyw[] = {
                   "definelevelname",          // 0  defines level name
                   "actor",                    // 1  defines an actor
                   "addammo",                  // 2  adds ammo to a weapon
                   "ifrnd",                    // 3  checks against a randomizer
                   "enda",                     // 4  ends an actor definition
                   "ifcansee",                 // 5  checks if the player can see an object
                   "ifhitweapon",              // 6  checks if an object was hit by a weapon
                   "action",                   // 7  defines an action if used outside a state or actor, otherwise triggers actor to perform action
                   "ifpdistl",                 // 8  checks if player distance is less than value
                   "ifpdistg",                 // 9  checks if player distance is more than value
                   "else",                     // 10 used with if checks
                   "strength",                 // 11 sets health
                   "break",                    // 12 stops processing
                   "shoot",                    // 13 shoots a projectile
                   "palfrom",                  // 14 used for player screen shading effect, sets p->pals_time and p->pals[0-2]
                   "sound",                    // 15 plays a sound that was defined with definesound
                   "fall",                     // 16 causes actor to fall to sector floor height
                   "state",                    // 17 begins defining a state if used outside a state or actor, otherwise calls a state
                   "ends",                     // 18 ends defining a state
                   "define",                   // 19 defines a value
                   "<null>",                   // 20 was previously used to define a comment
                   "ifai",                     // 21 checks if actor is currently performing a specific ai function
                   "killit",                   // 22 kills an actor
                   "addweapon",                // 23 adds a weapon to the closest player
                   "ai",                       // 24 sets an ai function to be used by an actor
                   "addphealth",               // 25 adds health to the player
                   "ifdead",                   // 26 checks if actor is dead
                   "ifsquished",               // 27 checks if actor has been squished
                   "sizeto",                   // 28 gradually increases actor size until it matches parameters given
                   "{",                        // 29 used to indicate segments of code
                   "}",                        // 30 used to indicate segments of code
                   "spawn",                    // 31 spawns an actor
                   "move",                     // 32
                   "ifwasweapon",              // 33
                   "ifaction",                 // 34
                   "ifactioncount",            // 35
                   "resetactioncount",         // 36
                   "debris",                   // 37
                   "pstomp",                   // 38
                   "<null>",                   // 39 was previously used to define the start of a comment block
                   "cstat",                    // 40
                   "ifmove",                   // 41
                   "resetplayer",              // 42
                   "ifonwater",                // 43
                   "ifinwater",                // 44
                   "ifcanshoottarget",         // 45
                   "ifcount",                  // 46
                   "resetcount",               // 47
                   "addinventory",             // 48
                   "ifactornotstayput",        // 49
                   "hitradius",                // 50
                   "ifp",                      // 51
                   "count",                    // 52
                   "ifactor",                  // 53
                   "music",                    // 54
                   "include",                  // 55
                   "ifstrength",               // 56
                   "definesound",              // 57
                   "guts",                     // 58
                   "ifspawnedby",              // 59
                   "gamestartup",              // 60
                   "wackplayer",               // 61
                   "ifgapzl",                  // 62
                   "ifhitspace",               // 63
                   "ifoutside",                // 64
                   "ifmultiplayer",            // 65
                   "operate",                  // 66
                   "ifinspace",                // 67
                   "debug",                    // 68
                   "endofgame",                // 69
                   "ifbulletnear",             // 70
                   "ifrespawn",                // 71
                   "iffloordistl",             // 72
                   "ifceilingdistl",           // 73
                   "spritepal",                // 74
                   "ifpinventory",             // 75
                   "betaname",                 // 76
                   "cactor",                   // 77
                   "ifphealthl",               // 78
                   "definequote",              // 79
                   "quote",                    // 80
                   "ifinouterspace",           // 81
                   "ifnotmoving",              // 82
                   "respawnhitag",             // 83
                   "tip",                      // 84
                   "ifspritepal",              // 85
                   "money",                    // 86
                   "soundonce",                // 87
                   "addkills",                 // 88
                   "stopsound",                // 89
                   "ifawayfromwall",           // 90
                   "ifcanseetarget",           // 91
                   "globalsound",              // 92
                   "lotsofglass",              // 93
                   "ifgotweaponce",            // 94
                   "getlastpal",               // 95
                   "pkick",                    // 96
                   "mikesnd",                  // 97
                   "useractor",                // 98
                   "sizeat",                   // 99
                   "addstrength",              // 100  [#]
                   "cstator",                  // 101
                   "mail",                     // 102
                   "paper",                    // 103
                   "tossweapon",               // 104
                   "sleeptime",                // 105
                   "nullop",                   // 106
                   "definevolumename",         // 107
                   "defineskillname",          // 108
                   "ifnosounds",               // 109
                   "clipdist",                 // 110
                   "ifangdiffl",               // 111
                   "gamevar",                  // 112
                   "ifvarl",                   // 113
                   "ifvarg",                   // 114
                   "setvarvar",                // 115
                   "setvar",                   // 116
                   "addvarvar",                // 117
                   "addvar",                   // 118
                   "ifvarvarl",                // 119
                   "ifvarvarg",                // 120
                   "addlogvar",                // 121
                   "addlog",                   // 122
                   "onevent",                  // 123
                   "endevent",                 // 124
                   "ifvare",                   // 125
                   "ifvarvare",                // 126
                   "spgetlotag",               // 127
                   "spgethitag",               // 128
                   "sectgetlotag",             // 129
                   "sectgethitag",             // 130
                   "ifsound",                  // 131
                   "gettexturefloor",          // 132
                   "gettextureceiling",        // 133
                   "inittimer",                // 134
                   "starttrack",               // 135
                   "randvar",                  // 136
                   "enhanced",                 // 137
                   "getangletotarget",         // 138
                   "getactorangle",            // 139
                   "setactorangle",            // 140
                   "mulvar",                   // 141
                   "mulvarvar",                // 142
                   "divvar",                   // 143
                   "divvarvar",                // 144
                   "modvar",                   // 145
                   "modvarvar",                // 146
                   "andvar",                   // 147
                   "andvarvar",                // 148
                   "orvar",                    // 149
                   "orvarvar",                 // 150
                   "getplayerangle",           // 151
                   "setplayerangle",           // 152
                   "lockplayer",               // 153
                   "setsector",                // 154
                   "getsector",                // 155
                   "setactor",                 // 156
                   "getactor",                 // 157
                   "setwall",                  // 158
                   "getwall",                  // 159
                   "findnearactor",            // 160
                   "findnearactorvar",         // 161
                   "setactorvar",              // 162
                   "getactorvar",              // 163
                   "espawn",                   // 164
                   "getplayer",                // 165
                   "setplayer",                // 166
                   "sqrt",                     // 167
                   "eventloadactor",           // 168
                   "espawnvar",                // 169
                   "getuserdef",               // 170
                   "setuserdef",               // 171
                   "subvarvar",                // 172
                   "subvar",                   // 173
                   "ifvarn",                   // 174
                   "ifvarvarn",                // 175
                   "ifvarand",                 // 176
                   "ifvarvarand",              // 177
                   "myos",                     // 178
                   "myospal",                  // 179
                   "displayrand",              // 180
                   "sin",                      // 181
                   "xorvarvar",                // 182
                   "xorvar",                   // 183
                   "randvarvar",               // 184
                   "myosx",                    // 185
                   "myospalx",                 // 186
                   "gmaxammo",                 // 187
                   "smaxammo",                 // 188
                   "startlevel",               // 189
                   "eshoot",                   // 190
                   "qspawn",                   // 191
                   "rotatesprite",             // 192
                   "defineprojectile",         // 193
                   "spriteshadow",             // 194
                   "cos",                      // 195
                   "eshootvar",                // 196
                   "findnearactor3d",          // 197
                   "findnearactor3dvar",       // 198
                   "flash",                    // 199
                   "qspawnvar",                // 200
                   "eqspawn",                  // 201
                   "eqspawnvar",               // 202
                   "minitext",                 // 203
                   "gametext",                 // 204
                   "digitalnumber",            // 205
                   "addweaponvar",             // 206
                   "setprojectile",            // 207
                   "angoff",                   // 208
                   "updatesector",             // 209
                   "insertspriteq",            // 210
                   "angoffvar",                // 211
                   "whilevarn",                // 212
                   "switch",                   // 213
                   "case",                     // 214
                   "default",                  // 215
                   "endswitch",                // 216
                   "shootvar",                 // 217
                   "soundvar",                 // 218
                   "findplayer",               // 219
                   "findotherplayer",          // 220
                   "activatebysector",         // 221 sectnum, spriteid
                   "operatesectors",           // 222 sectnum, spriteid
                   "operaterespawns",          // 223 lotag
                   "operateactivators",        // 224 lotag, player index
                   "operatemasterswitches",    // 225 lotag
                   "checkactivatormotion",     // 226 lotag
                   "zshoot",                   // 227 zvar projnum
                   "dist",                     // 228 sprite1 sprite2
                   "ldist",                    // 229 sprite1 sprite2
                   "shiftvarl",                // 230
                   "shiftvarr",                // 231
                   "spritenvg",                // 232
                   "getangle",                 // 233
                   "whilevarvarn",             // 234
                   "hitscan",                  // 235
                   "time",                     // 236
                   "getplayervar",             // 237
                   "setplayervar",             // 238
                   "mulscale",                 // 239
                   "setaspect",                // 240
                   "txdist",                   // 241
                   "spritenoshade",            // 242
                   "movesprite",               // 243
                   "checkavailweapon",         // 244
                   "soundoncevar",             // 245
                   "updatesectorz",            // 246
                   "stopallsounds",            // 247
                   "ssp",                      // 248
                   "stopsoundvar",             // 249
                   "displayrandvar",           // 250
                   "displayrandvarvar",        // 251
                   "checkavailinven",          // 252
                   "globalsoundvar",           // 253
                   "guniqhudid",               // 254
                   "getprojectile",            // 255
                   "getthisprojectile",        // 256
                   "setthisprojectile",        // 257
                   "definecheat",              // 258
                   "cheatkeys",                // 259
                   "userquote",                // 260
                   "precache",                 // 261
                   "definegamefuncname",       // 262
                   "redefinequote",            // 263
                   "dynquote",                 // 264
                   "getpname",                 // 265
                   "qstrcat",                  // 266
                   "qstrcpy",                  // 267
                   "setsprite",                // 268
                   "rotatepoint",              // 269
                   "dragpoint",                // 270
                   "getzrange",                // 271
                   "changespritestat",         // 272
                   "getceilzofslope",          // 273
                   "getflorzofslope",          // 274
                   "neartag",                  // 275
                   "definegametype",           // 276
                   "changespritesect",         // 277
                   "spriteflags",              // 278
                   "savegamevar",              // 279
                   "readgamevar",              // 280
                   "findnearsprite",           // 281
                   "findnearspritevar",        // 282
                   "findnearsprite3d",         // 283
                   "findnearsprite3dvar",      // 284
                   "dynamicremap",             // 285
                   "setinput",                 // 286
                   "getinput",                 // 287
                   "save",					   // 288
                   "cansee",                   // 289
                   "canseespr",                // 290
                   "<null>"
               };

LABELS sectorlabels[]= {
                           { "wallptr", SECTOR_WALLPTR, 0, 0 },
                           { "wallnum", SECTOR_WALLNUM, 0, 0 },
                           { "ceilingz", SECTOR_CEILINGZ, 0, 0 },
                           { "floorz", SECTOR_FLOORZ, 0, 0 },
                           { "ceilingstat", SECTOR_CEILINGSTAT, 0, 0 },
                           { "floorstat", SECTOR_FLOORSTAT, 0, 0 },
                           { "ceilingpicnum", SECTOR_CEILINGPICNUM, 0, 0 },
                           { "ceilingslope", SECTOR_CEILINGSLOPE, 0, 0 },
                           { "ceilingshade", SECTOR_CEILINGSHADE, 0, 0 },
                           { "ceilingpal", SECTOR_CEILINGPAL, 0, 0 },
                           { "ceilingxpanning", SECTOR_CEILINGXPANNING, 0, 0 },
                           { "ceilingypanning", SECTOR_CEILINGYPANNING, 0, 0 },
                           { "floorpicnum", SECTOR_FLOORPICNUM, 0, 0 },
                           { "floorslope", SECTOR_FLOORSLOPE, 0, 0 },
                           { "floorshade", SECTOR_FLOORSHADE, 0, 0 },
                           { "floorpal", SECTOR_FLOORPAL, 0, 0 },
                           { "floorxpanning", SECTOR_FLOORXPANNING, 0, 0 },
                           { "floorypanning", SECTOR_FLOORYPANNING, 0, 0 },
                           { "visibility", SECTOR_VISIBILITY, 0, 0 },
                           { "alignto", SECTOR_ALIGNTO, 0, 0 },
                           { "lotag", SECTOR_LOTAG, 0, 0 },
                           { "hitag", SECTOR_HITAG, 0, 0 },
                           { "extra", SECTOR_EXTRA, 0, 0 },
                           { "", -1, 0, 0  }     // END OF LIST
                       };

LABELS walllabels[]= {
                         { "x", WALL_X, 0, 0 },
                         { "y", WALL_Y, 0, 0 },
                         { "point2", WALL_POINT2, 0, 0 },
                         { "nextwall", WALL_NEXTWALL, 0, 0 },
                         { "nextsector", WALL_NEXTSECTOR, 0, 0 },
                         { "cstat", WALL_CSTAT, 0, 0 },
                         { "picnum", WALL_PICNUM, 0, 0 },
                         { "overpicnum", WALL_OVERPICNUM, 0, 0 },
                         { "shade", WALL_SHADE, 0, 0 },
                         { "pal", WALL_PAL, 0, 0 },
                         { "xrepeat", WALL_XREPEAT, 0, 0 },
                         { "yrepeat", WALL_YREPEAT, 0, 0 },
                         { "xpanning", WALL_XPANNING, 0, 0 },
                         { "ypanning", WALL_YPANNING, 0, 0 },
                         { "lotag", WALL_LOTAG, 0, 0 },
                         { "hitag", WALL_HITAG, 0, 0 },
                         { "extra", WALL_EXTRA, 0, 0 },
                         { "", -1, 0, 0  }     // END OF LIST
                     };

LABELS actorlabels[]= {
                          { "x", ACTOR_X, 0, 0 },
                          { "y", ACTOR_Y, 0, 0 },
                          { "z", ACTOR_Z, 0, 0 },
                          { "cstat", ACTOR_CSTAT, 0, 0 },
                          { "picnum", ACTOR_PICNUM, 0, 0 },
                          { "shade", ACTOR_SHADE, 0, 0 },
                          { "pal", ACTOR_PAL, 0, 0 },
                          { "clipdist", ACTOR_CLIPDIST, 0, 0 },
                          { "detail", ACTOR_DETAIL, 0, 0 },
                          { "xrepeat", ACTOR_XREPEAT, 0, 0 },
                          { "yrepeat", ACTOR_YREPEAT, 0, 0 },
                          { "xoffset", ACTOR_XOFFSET, 0, 0 },
                          { "yoffset", ACTOR_YOFFSET, 0, 0 },
                          { "sectnum", ACTOR_SECTNUM, 0, 0 },
                          { "statnum", ACTOR_STATNUM, 0, 0 },
                          { "ang", ACTOR_ANG, 0, 0 },
                          { "owner", ACTOR_OWNER, 0, 0 },
                          { "xvel", ACTOR_XVEL, 0, 0 },
                          { "yvel", ACTOR_YVEL, 0, 0 },
                          { "zvel", ACTOR_ZVEL, 0, 0 },
                          { "lotag", ACTOR_LOTAG, 0, 0 },
                          { "hitag", ACTOR_HITAG, 0, 0 },
                          { "extra", ACTOR_EXTRA, 0, 0 },

                          // hittype labels...
                          { "htcgg", ACTOR_HTCGG, 0, 0 },
                          { "htpicnum", ACTOR_HTPICNUM, 0, 0 },
                          { "htang", ACTOR_HTANG, 0, 0 },
                          { "htextra", ACTOR_HTEXTRA, 0, 0 },
                          { "htowner", ACTOR_HTOWNER, 0, 0 },
                          { "htmovflag", ACTOR_HTMOVFLAG, 0, 0 },
                          { "httempang", ACTOR_HTTEMPANG, 0, 0 },
                          { "htactorstayput", ACTOR_HTACTORSTAYPUT, 0, 0 },
                          { "htdispicnum", ACTOR_HTDISPICNUM, 0, 0 },
                          { "httimetosleep", ACTOR_HTTIMETOSLEEP, 0, 0 },
                          { "htfloorz", ACTOR_HTFLOORZ, 0, 0 },
                          { "htceilingz", ACTOR_HTCEILINGZ, 0, 0 },
                          { "htlastvx", ACTOR_HTLASTVX, 0, 0 },
                          { "htlastvy", ACTOR_HTLASTVY, 0, 0 },
                          { "htbposx", ACTOR_HTBPOSX, 0, 0 },
                          { "htbposy", ACTOR_HTBPOSY, 0, 0 },
                          { "htbposz", ACTOR_HTBPOSZ, 0, 0 },
                          { "htg_t", ACTOR_HTG_T, LABEL_HASPARM2, 8 },

                          // model flags

                          { "angoff", ACTOR_ANGOFF, 0, 0 },
                          { "pitch", ACTOR_PITCH, 0, 0 },
                          { "roll", ACTOR_ROLL, 0, 0 },
                          { "mdxoff", ACTOR_MDXOFF, 0, 0 },
                          { "mdyoff", ACTOR_MDYOFF, 0, 0 },
                          { "mdzoff", ACTOR_MDZOFF, 0, 0 },
                          { "", -1, 0, 0  }     // END OF LIST
                      };

LABELS playerlabels[]= {
                           { "zoom", PLAYER_ZOOM, 0, 0 },
                           { "exitx", PLAYER_EXITX, 0, 0 },
                           { "exity", PLAYER_EXITY, 0, 0 },
                           { "loogiex", PLAYER_LOOGIEX, LABEL_HASPARM2, 64 },
                           { "loogiey", PLAYER_LOOGIEY, LABEL_HASPARM2, 64 },
                           { "numloogs", PLAYER_NUMLOOGS, 0, 0 },
                           { "loogcnt", PLAYER_LOOGCNT, 0, 0 },
                           { "posx", PLAYER_POSX, 0, 0 },
                           { "posy", PLAYER_POSY, 0, 0 },
                           { "posz", PLAYER_POSZ, 0, 0 },
                           { "horiz", PLAYER_HORIZ, 0, 0 },
                           { "ohoriz", PLAYER_OHORIZ, 0, 0 },
                           { "ohorizoff", PLAYER_OHORIZOFF, 0, 0 },
                           { "invdisptime", PLAYER_INVDISPTIME, 0, 0 },
                           { "bobposx", PLAYER_BOBPOSX, 0, 0 },
                           { "bobposy", PLAYER_BOBPOSY, 0, 0 },
                           { "oposx", PLAYER_OPOSX, 0, 0 },
                           { "oposy", PLAYER_OPOSY, 0, 0 },
                           { "oposz", PLAYER_OPOSZ, 0, 0 },
                           { "pyoff", PLAYER_PYOFF, 0, 0 },
                           { "opyoff", PLAYER_OPYOFF, 0, 0 },
                           { "posxv", PLAYER_POSXV, 0, 0 },
                           { "posyv", PLAYER_POSYV, 0, 0 },
                           { "poszv", PLAYER_POSZV, 0, 0 },
                           { "last_pissed_time", PLAYER_LAST_PISSED_TIME, 0, 0 },
                           { "truefz", PLAYER_TRUEFZ, 0, 0 },
                           { "truecz", PLAYER_TRUECZ, 0, 0 },
                           { "player_par", PLAYER_PLAYER_PAR, 0, 0 },
                           { "visibility", PLAYER_VISIBILITY, 0, 0 },
                           { "bobcounter", PLAYER_BOBCOUNTER, 0, 0 },
                           { "weapon_sway", PLAYER_WEAPON_SWAY, 0, 0 },
                           { "pals_time", PLAYER_PALS_TIME, 0, 0 },
                           { "randomflamex", PLAYER_RANDOMFLAMEX, 0, 0 },
                           { "crack_time", PLAYER_CRACK_TIME, 0, 0 },
                           { "aim_mode", PLAYER_AIM_MODE, 0, 0 },
                           { "ang", PLAYER_ANG, 0, 0 },
                           { "oang", PLAYER_OANG, 0, 0 },
                           { "angvel", PLAYER_ANGVEL, 0, 0 },
                           { "cursectnum", PLAYER_CURSECTNUM, 0, 0 },
                           { "look_ang", PLAYER_LOOK_ANG, 0, 0 },
                           { "last_extra", PLAYER_LAST_EXTRA, 0, 0 },
                           { "subweapon", PLAYER_SUBWEAPON, 0, 0 },
                           { "ammo_amount", PLAYER_AMMO_AMOUNT, LABEL_HASPARM2, MAX_WEAPONS },
                           { "wackedbyactor", PLAYER_WACKEDBYACTOR, 0, 0 },
                           { "frag", PLAYER_FRAG, 0, 0 },
                           { "fraggedself", PLAYER_FRAGGEDSELF, 0, 0 },
                           { "curr_weapon", PLAYER_CURR_WEAPON, 0, 0 },
                           { "last_weapon", PLAYER_LAST_WEAPON, 0, 0 },
                           { "tipincs", PLAYER_TIPINCS, 0, 0 },
                           { "horizoff", PLAYER_HORIZOFF, 0, 0 },
                           { "wantweaponfire", PLAYER_WANTWEAPONFIRE, 0, 0 },
                           { "holoduke_amount", PLAYER_HOLODUKE_AMOUNT, 0, 0 },
                           { "newowner", PLAYER_NEWOWNER, 0, 0 },
                           { "hurt_delay", PLAYER_HURT_DELAY, 0, 0 },
                           { "hbomb_hold_delay", PLAYER_HBOMB_HOLD_DELAY, 0, 0 },
                           { "jumping_counter", PLAYER_JUMPING_COUNTER, 0, 0 },
                           { "airleft", PLAYER_AIRLEFT, 0, 0 },
                           { "knee_incs", PLAYER_KNEE_INCS, 0, 0 },
                           { "access_incs", PLAYER_ACCESS_INCS, 0, 0 },
                           { "fta", PLAYER_FTA, 0, 0 },
                           { "ftq", PLAYER_FTQ, 0, 0 },
                           { "access_wallnum", PLAYER_ACCESS_WALLNUM, 0, 0 },
                           { "access_spritenum", PLAYER_ACCESS_SPRITENUM, 0, 0 },
                           { "kickback_pic", PLAYER_KICKBACK_PIC, 0, 0 },
                           { "got_access", PLAYER_GOT_ACCESS, 0, 0 },
                           { "weapon_ang", PLAYER_WEAPON_ANG, 0, 0 },
                           { "firstaid_amount", PLAYER_FIRSTAID_AMOUNT, 0, 0 },
                           { "somethingonplayer", PLAYER_SOMETHINGONPLAYER, 0, 0 },
                           { "on_crane", PLAYER_ON_CRANE, 0, 0 },
                           { "i", PLAYER_I, 0, 0 },
                           { "one_parallax_sectnum", PLAYER_ONE_PARALLAX_SECTNUM, 0, 0 },
                           { "over_shoulder_on", PLAYER_OVER_SHOULDER_ON, 0, 0 },
                           { "random_club_frame", PLAYER_RANDOM_CLUB_FRAME, 0, 0 },
                           { "fist_incs", PLAYER_FIST_INCS, 0, 0 },
                           { "one_eighty_count", PLAYER_ONE_EIGHTY_COUNT, 0, 0 },
                           { "cheat_phase", PLAYER_CHEAT_PHASE, 0, 0 },
                           { "dummyplayersprite", PLAYER_DUMMYPLAYERSPRITE, 0, 0 },
                           { "extra_extra8", PLAYER_EXTRA_EXTRA8, 0, 0 },
                           { "quick_kick", PLAYER_QUICK_KICK, 0, 0 },
                           { "heat_amount", PLAYER_HEAT_AMOUNT, 0, 0 },
                           { "actorsqu", PLAYER_ACTORSQU, 0, 0 },
                           { "timebeforeexit", PLAYER_TIMEBEFOREEXIT, 0, 0 },
                           { "customexitsound", PLAYER_CUSTOMEXITSOUND, 0, 0 },
                           { "weaprecs[16]", PLAYER_WEAPRECS, 0, 0 },
                           { "weapreccnt", PLAYER_WEAPRECCNT, 0, 0 },
                           { "interface_toggle_flag", PLAYER_INTERFACE_TOGGLE_FLAG, 0, 0 },
                           { "rotscrnang", PLAYER_ROTSCRNANG, 0, 0 },
                           { "dead_flag", PLAYER_DEAD_FLAG, 0, 0 },
                           { "show_empty_weapon", PLAYER_SHOW_EMPTY_WEAPON, 0, 0 },
                           { "scuba_amount", PLAYER_SCUBA_AMOUNT, 0, 0 },
                           { "jetpack_amount", PLAYER_JETPACK_AMOUNT, 0, 0 },
                           { "steroids_amount", PLAYER_STEROIDS_AMOUNT, 0, 0 },
                           { "shield_amount", PLAYER_SHIELD_AMOUNT, 0, 0 },
                           { "holoduke_on", PLAYER_HOLODUKE_ON, 0, 0 },
                           { "pycount", PLAYER_PYCOUNT, 0, 0 },
                           { "weapon_pos", PLAYER_WEAPON_POS, 0, 0 },
                           { "frag_ps", PLAYER_FRAG_PS, 0, 0 },
                           { "transporter_hold", PLAYER_TRANSPORTER_HOLD, 0, 0 },
                           { "last_full_weapon", PLAYER_LAST_FULL_WEAPON, 0, 0 },
                           { "footprintshade", PLAYER_FOOTPRINTSHADE, 0, 0 },
                           { "boot_amount", PLAYER_BOOT_AMOUNT, 0, 0 },
                           { "scream_voice", PLAYER_SCREAM_VOICE, 0, 0 },
                           { "gm", PLAYER_GM, 0, 0 },
                           { "on_warping_sector", PLAYER_ON_WARPING_SECTOR, 0, 0 },
                           { "footprintcount", PLAYER_FOOTPRINTCOUNT, 0, 0 },
                           { "hbomb_on", PLAYER_HBOMB_ON, 0, 0 },
                           { "jumping_toggle", PLAYER_JUMPING_TOGGLE, 0, 0 },
                           { "rapid_fire_hold", PLAYER_RAPID_FIRE_HOLD, 0, 0 },
                           { "on_ground", PLAYER_ON_GROUND, 0, 0 },
                           { "name", PLAYER_NAME,  LABEL_ISSTRING, 32 },
                           { "inven_icon", PLAYER_INVEN_ICON, 0, 0 },
                           { "buttonpalette", PLAYER_BUTTONPALETTE, 0, 0 },
                           { "jetpack_on", PLAYER_JETPACK_ON, 0, 0 },
                           { "spritebridge", PLAYER_SPRITEBRIDGE, 0, 0 },
                           { "lastrandomspot", PLAYER_LASTRANDOMSPOT, 0, 0 },
                           { "scuba_on", PLAYER_SCUBA_ON, 0, 0 },
                           { "footprintpal", PLAYER_FOOTPRINTPAL, 0, 0 },
                           { "heat_on", PLAYER_HEAT_ON, 0, 0 },
                           { "holster_weapon", PLAYER_HOLSTER_WEAPON, 0, 0 },
                           { "falling_counter", PLAYER_FALLING_COUNTER, 0, 0 },
                           { "gotweapon", PLAYER_GOTWEAPON, LABEL_HASPARM2, MAX_WEAPONS },
                           { "refresh_inventory", PLAYER_REFRESH_INVENTORY, 0, 0 },
                           { "palette", PLAYER_PALETTE, 0, 0 },
                           { "toggle_key_flag", PLAYER_TOGGLE_KEY_FLAG, 0, 0 },
                           { "knuckle_incs", PLAYER_KNUCKLE_INCS, 0, 0 },
                           { "walking_snd_toggle", PLAYER_WALKING_SND_TOGGLE, 0, 0 },
                           { "palookup", PLAYER_PALOOKUP, 0, 0 },
                           { "hard_landing", PLAYER_HARD_LANDING, 0, 0 },
                           { "max_+_rooms", PLAYER_MAX_SECRET_ROOMS, 0, 0 },
                           { "secret_rooms", PLAYER_SECRET_ROOMS, 0, 0 },
                           { "pals", PLAYER_PALS, LABEL_HASPARM2, 2 },
                           { "max_actors_killed", PLAYER_MAX_ACTORS_KILLED, 0, 0 },
                           { "actors_killed", PLAYER_ACTORS_KILLED, 0, 0 },
                           { "return_to_center", PLAYER_RETURN_TO_CENTER, 0, 0 },
                           { "runspeed", PLAYER_RUNSPEED, 0, 0 },
                           { "sbs", PLAYER_SBS, 0, 0 },
                           { "reloading", PLAYER_RELOADING, 0, 0 },
                           { "auto_aim", PLAYER_AUTO_AIM, 0, 0 },
                           { "movement_lock", PLAYER_MOVEMENT_LOCK, LABEL_HASPARM2, 4 },
                           { "sound_pitch", PLAYER_SOUND_PITCH, 0, 0 },
                           { "weaponswitch", PLAYER_WEAPONSWITCH, 0, 0 },
                           { "", -1, 0, 0  }     // END OF LIST
                       };

LABELS projectilelabels[]= {
                               { "workslike", PROJ_WORKSLIKE, 0, 0 },
                               { "spawns", PROJ_SPAWNS, 0, 0 },
                               { "sxrepeat", PROJ_SXREPEAT, 0, 0 },
                               { "syrepeat", PROJ_SYREPEAT, 0, 0 },
                               { "sound", PROJ_SOUND, 0, 0 },
                               { "isound", PROJ_ISOUND, 0, 0 },
                               { "vel", PROJ_VEL, 0, 0 },
                               { "extra", PROJ_EXTRA, 0, 0 },
                               { "decal", PROJ_DECAL, 0, 0 },
                               { "trail", PROJ_TRAIL, 0, 0 },
                               { "txrepeat", PROJ_TXREPEAT, 0, 0 },
                               { "tyrepeat", PROJ_TYREPEAT, 0, 0 },
                               { "toffset", PROJ_TOFFSET, 0, 0 },
                               { "tnum", PROJ_TNUM, 0, 0 },
                               { "drop", PROJ_DROP, 0, 0 },
                               { "cstat", PROJ_CSTAT, 0, 0 },
                               { "clipdist", PROJ_CLIPDIST, 0, 0 },
                               { "shade", PROJ_SHADE, 0, 0 },
                               { "xrepeat", PROJ_XREPEAT, 0, 0 },
                               { "yrepeat", PROJ_YREPEAT, 0, 0 },
                               { "pal", PROJ_PAL, 0, 0 },
                               { "extra_rand", PROJ_EXTRA_RAND, 0, 0 },
                               { "hitradius", PROJ_HITRADIUS, 0, 0 },
                               { "velmult", PROJ_VEL_MULT, 0, 0 },
                               { "offset", PROJ_OFFSET, 0, 0 },
                               { "bounces", PROJ_BOUNCES, 0, 0 },
                               { "bsound", PROJ_BSOUND, 0, 0 },
                               { "range", PROJ_RANGE, 0, 0 },
                               { "", -1, 0, 0  }     // END OF LIST
                           };

LABELS userdefslabels[]= {
                             //        { "<null>", 1, 0, 0 },
                             { "god", USERDEFS_GOD, 0, 0 },
                             { "warp_on", USERDEFS_WARP_ON, 0, 0 },
                             { "cashman", USERDEFS_CASHMAN, 0, 0 },
                             { "eog", USERDEFS_EOG, 0, 0 },
                             { "showallmap", USERDEFS_SHOWALLMAP, 0, 0 },
                             { "show_help", USERDEFS_SHOW_HELP, 0, 0 },
                             { "scrollmode", USERDEFS_SCROLLMODE, 0, 0 },
                             { "clipping", USERDEFS_CLIPPING, 0, 0 },
                             { "user_name", USERDEFS_USER_NAME, LABEL_HASPARM2, MAXPLAYERS },
                             { "ridecule", USERDEFS_RIDECULE, LABEL_HASPARM2 | LABEL_ISSTRING, 10 },
                             { "savegame", USERDEFS_SAVEGAME, LABEL_HASPARM2 | LABEL_ISSTRING, 10 },
                             { "pwlockout", USERDEFS_PWLOCKOUT, LABEL_ISSTRING, 128 },
                             { "rtsname;", USERDEFS_RTSNAME,  LABEL_ISSTRING, 128 },
                             { "overhead_on", USERDEFS_OVERHEAD_ON, 0, 0 },
                             { "last_overhead", USERDEFS_LAST_OVERHEAD, 0, 0 },
                             { "showweapons", USERDEFS_SHOWWEAPONS, 0, 0 },

                             { "pause_on", USERDEFS_PAUSE_ON, 0, 0 },
                             { "from_bonus", USERDEFS_FROM_BONUS, 0, 0 },
                             { "camerasprite", USERDEFS_CAMERASPRITE, 0, 0 },
                             { "last_camsprite", USERDEFS_LAST_CAMSPRITE, 0, 0 },
                             { "last_level", USERDEFS_LAST_LEVEL, 0, 0 },
                             { "secretlevel", USERDEFS_SECRETLEVEL, 0, 0 },

                             { "const_visibility", USERDEFS_CONST_VISIBILITY, 0, 0 },
                             { "uw_framerate", USERDEFS_UW_FRAMERATE, 0, 0 },
                             { "camera_time", USERDEFS_CAMERA_TIME, 0, 0 },
                             { "folfvel", USERDEFS_FOLFVEL, 0, 0 },
                             { "folavel", USERDEFS_FOLAVEL, 0, 0 },
                             { "folx", USERDEFS_FOLX, 0, 0 },
                             { "foly", USERDEFS_FOLY, 0, 0 },
                             { "fola", USERDEFS_FOLA, 0, 0 },
                             { "reccnt", USERDEFS_RECCNT, 0, 0 },

                             { "entered_name", USERDEFS_ENTERED_NAME, 0, 0 },
                             { "screen_tilting", USERDEFS_SCREEN_TILTING, 0, 0 },
                             { "shadows", USERDEFS_SHADOWS, 0, 0 },
                             { "fta_on", USERDEFS_FTA_ON, 0, 0 },
                             { "executions", USERDEFS_EXECUTIONS, 0, 0 },
                             { "auto_run", USERDEFS_AUTO_RUN, 0, 0 },
                             { "coords", USERDEFS_COORDS, 0, 0 },
                             { "tickrate", USERDEFS_TICKRATE, 0, 0 },
                             { "m_coop", USERDEFS_M_COOP, 0, 0 },
                             { "coop", USERDEFS_COOP, 0, 0 },
                             { "screen_size", USERDEFS_SCREEN_SIZE, 0, 0 },
                             { "lockout", USERDEFS_LOCKOUT, 0, 0 },
                             { "crosshair", USERDEFS_CROSSHAIR, 0, 0 },
                             { "wchoice[MAXPLAYERS][MAX_WEAPONS]", USERDEFS_WCHOICE, 0, 0 },
                             { "playerai", USERDEFS_PLAYERAI, 0, 0 },
                             { "respawn_monsters", USERDEFS_RESPAWN_MONSTERS, 0, 0 },
                             { "respawn_items", USERDEFS_RESPAWN_ITEMS, 0, 0 },
                             { "respawn_inventory", USERDEFS_RESPAWN_INVENTORY, 0, 0 },
                             { "recstat", USERDEFS_RECSTAT, 0, 0 },
                             { "monsters_off", USERDEFS_MONSTERS_OFF, 0, 0 },
                             { "brightness", USERDEFS_BRIGHTNESS, 0, 0 },
                             { "m_respawn_items", USERDEFS_M_RESPAWN_ITEMS, 0, 0 },
                             { "m_respawn_monsters", USERDEFS_M_RESPAWN_MONSTERS, 0, 0 },
                             { "m_respawn_inventory", USERDEFS_M_RESPAWN_INVENTORY, 0, 0 },
                             { "m_recstat", USERDEFS_M_RECSTAT, 0, 0 },
                             { "m_monsters_off", USERDEFS_M_MONSTERS_OFF, 0, 0 },
                             { "detail", USERDEFS_DETAIL, 0, 0 },
                             { "m_ffire", USERDEFS_M_FFIRE, 0, 0 },
                             { "ffire", USERDEFS_FFIRE, 0, 0 },
                             { "m_player_skill", USERDEFS_M_PLAYER_SKILL, 0, 0 },
                             { "m_level_number", USERDEFS_M_LEVEL_NUMBER, 0, 0 },
                             { "m_volume_number", USERDEFS_M_VOLUME_NUMBER, 0, 0 },
                             { "multimode", USERDEFS_MULTIMODE, 0, 0 },
                             { "player_skill", USERDEFS_PLAYER_SKILL, 0, 0 },
                             { "level_number", USERDEFS_LEVEL_NUMBER, 0, 0 },
                             { "volume_number", USERDEFS_VOLUME_NUMBER, 0, 0 },
                             { "m_marker", USERDEFS_M_MARKER, 0, 0 },
                             { "marker", USERDEFS_MARKER, 0, 0 },
                             { "mouseflip", USERDEFS_MOUSEFLIP, 0, 0 },
                             { "statusbarscale", USERDEFS_STATUSBARSCALE, 0, 0 },
                             { "drawweapon", USERDEFS_DRAWWEAPON, 0, 0 },
                             { "mouseaiming", USERDEFS_MOUSEAIMING, 0, 0 },
                             { "weaponswitch", USERDEFS_WEAPONSWITCH, 0, 0 },
                             { "brightskins", USERDEFS_BRIGHTSKINS, 0, 0 },
                             { "democams", USERDEFS_DEMOCAMS, 0, 0 },
                             { "color", USERDEFS_COLOR, 0, 0 },
                             { "msgdisptime", USERDEFS_MSGDISPTIME, 0, 0 },
                             { "", -1, 0, 0  }     // END OF LIST
                         };

LABELS inputlabels[]= {
                          { "avel", INPUT_AVEL, 0, 0 },
                          { "horz", INPUT_HORZ, 0, 0 },
                          { "fvel", INPUT_FVEL, 0, 0 },
                          { "svel", INPUT_SVEL, 0, 0 },
                          { "bits", INPUT_BITS, 0, 0 },
                          { "bits2", INPUT_BITS2, 0, 0 },
                          { "", -1, 0, 0  }     // END OF LIST
                      };

void skipcomments(void)
{
    char c;
    while ((c = *textptr))
    {
        if (c == ' ' || c == '\t' || c == '\r')
            textptr++;
        else if (c == '\n') {
            line_number++;
            textptr++;
        }
        else if (c == '/' && textptr[1] == '/')
        {
            if (!(error || warning) && condebug > 1)
                initprintf("%s:%ld: debug: got comment.\n",compilefile,line_number);
            while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
                textptr++;
        }
        else if (c == '/' && textptr[1] == '*')
        {
            if (!(error || warning) && condebug > 1)
                initprintf("%s:%ld: debug: got start of comment block.\n",compilefile,line_number);
            while (*textptr && !(textptr[0] == '*' && textptr[1] == '/'))
            {
                if (*textptr == '\n')
                    line_number++;
                textptr++;
            }
            if ((!(error || warning) && condebug > 1) && (textptr[0] == '*' && textptr[1] == '/'))
                initprintf("%s:%ld: debug: got end of comment block.\n",compilefile,line_number);
            if (!*textptr)
            {
                if(!(error || warning) && condebug)
                    initprintf("%s:%ld: debug: EOF in comment!\n",compilefile,line_number);
                ReportError(-1);
                initprintf("%s:%ld: error: found `/*' with no `*/'.\n",compilefile,line_number);
                parsing_state = num_braces = 0;
                parsing_actor = 0;
                error++;
                break;
            }
            else textptr+=2;
        }
        else break;
    }
}

void DefineProjectile(long lVar1, long lLabelID, long lVar2)
{
    switch(lLabelID)
    {
    case PROJ_WORKSLIKE:
        projectile[lVar1].workslike=lVar2;
        break;

    case PROJ_SPAWNS:
        projectile[lVar1].spawns=lVar2;
        break;

    case PROJ_SXREPEAT:
        projectile[lVar1].sxrepeat=lVar2;
        break;

    case PROJ_SYREPEAT:
        projectile[lVar1].syrepeat=lVar2;
        break;

    case PROJ_SOUND:
        projectile[lVar1].sound=lVar2;
        break;

    case PROJ_ISOUND:
        projectile[lVar1].isound=lVar2;
        break;

    case PROJ_VEL:
        projectile[lVar1].vel=lVar2;
        break;

    case PROJ_EXTRA:
        projectile[lVar1].extra=lVar2;
        break;

    case PROJ_DECAL:
        projectile[lVar1].decal=lVar2;
        break;

    case PROJ_TRAIL:
        projectile[lVar1].trail=lVar2;
        break;

    case PROJ_TXREPEAT:
        projectile[lVar1].txrepeat=lVar2;
        break;

    case PROJ_TYREPEAT:
        projectile[lVar1].tyrepeat=lVar2;
        break;

    case PROJ_TOFFSET:
        projectile[lVar1].toffset=lVar2;
        break;

    case PROJ_TNUM:
        projectile[lVar1].tnum=lVar2;
        break;

    case PROJ_DROP:
        projectile[lVar1].drop=lVar2;
        break;

    case PROJ_CSTAT:
        projectile[lVar1].cstat=lVar2;
        break;

    case PROJ_CLIPDIST:
        projectile[lVar1].clipdist=lVar2;
        break;

    case PROJ_SHADE:
        projectile[lVar1].shade=lVar2;
        break;

    case PROJ_XREPEAT:
        projectile[lVar1].xrepeat=lVar2;
        break;

    case PROJ_YREPEAT:
        projectile[lVar1].yrepeat=lVar2;
        break;

    case PROJ_PAL:
        projectile[lVar1].pal=lVar2;
        break;

    case PROJ_EXTRA_RAND:
        projectile[lVar1].extra_rand=lVar2;
        break;

    case PROJ_HITRADIUS:
        projectile[lVar1].hitradius=lVar2;
        break;

    case PROJ_VEL_MULT:
        projectile[lVar1].velmult=lVar2;
        break;

    case PROJ_OFFSET:
        projectile[lVar1].offset=lVar2;
        break;

    case PROJ_BOUNCES:
        projectile[lVar1].bounces=lVar2;
        break;

    case PROJ_BSOUND:
        projectile[lVar1].bsound=lVar2;
        break;

    case PROJ_RANGE:
        projectile[lVar1].range=lVar2;
        break;

    default:
        break;
    }

    //  defaultprojectile[lVar1] = projectile[lVar1];
    Bmemcpy(&defaultprojectile[lVar1], &projectile[lVar1], sizeof(projectile[lVar1]));

    return;
}

char CheckEventSync(int iEventID)
{
    if(parsing_event || parsing_actor)
    {
        switch(iEventID)
        {
        case EVENT_CHEATGETSTEROIDS:
        case EVENT_CHEATGETHEAT:
        case EVENT_CHEATGETBOOT:
        case EVENT_CHEATGETSHIELD:
        case EVENT_CHEATGETSCUBA:
        case EVENT_CHEATGETHOLODUKE:
        case EVENT_CHEATGETJETPACK:
        case EVENT_CHEATGETFIRSTAID:
        case EVENT_DISPLAYWEAPON:
        case EVENT_DRAWWEAPON:
        case EVENT_DISPLAYCROSSHAIR:
        case EVENT_DISPLAYREST:
        case EVENT_ENTERLEVEL:
        case EVENT_GETLOADTILE:
        case EVENT_GETMENUTILE:
        case EVENT_INIT:
        case EVENT_LOGO:
            return 0;
        default:
            return 1;
        }
    }
    return 1;
}

void AddLog(char *psz)
{
    Bstrcpy(tempbuf,psz);
    if(tempbuf[Bstrlen(psz)] != '\n')
        Bstrcat(tempbuf,"\n");
    if (qsetmode == 200) OSD_Printf(tempbuf);
    else initprintf(tempbuf);
}

char AddGameVar(char *pszLabel, long lValue, unsigned long dwFlags);

int ReadGameVars(long fil)
{
    int i;
    long l;

    //     AddLog("Reading gamevars from savegame");

    FreeGameVars(); // nuke 'em from orbit, it's the only way to be sure...
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if(kdfread(&iGameVarCount,sizeof(iGameVarCount),1,fil) != 1) goto corrupt;

    for(i=0;i<iGameVarCount;i++)
    {
        if(kdfread(&(aGameVars[i]),sizeof(MATTGAMEVAR),1,fil) != 1) goto corrupt;
        aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
        if(kdfread(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil) != 1) goto corrupt;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(long));
        else if( aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
            aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(long));
        else
            // else nothing 'extra...'
            aGameVars[i].plValues=NULL;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    InitGameVarPointers();

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXPLAYERS);
            //AddLog(g_szBuf);
            if(kdfread(aGameVars[i].plValues,sizeof(long) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
        }
        else if( aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXSPRITES);
            //AddLog(g_szBuf);
            if(kdfread(aGameVars[i].plValues,sizeof(long) * MAXSPRITES, 1, fil) != 1) goto corrupt;
        }
        // else nothing 'extra...'
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if(kdfread(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil) != 1) goto corrupt;
    for(i=0;i<MAXGAMEEVENTS;i++)
        if(apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]+(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
        }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if(kdfread(&l,sizeof(l),1,fil) != 1) goto corrupt;
    if(kdfread(g_szBuf,l,1,fil) != 1) goto corrupt;
    g_szBuf[l]=0;
    AddLog(g_szBuf);

#if 0
    {
        FILE *fp;
        AddLog("Dumping Vars...");
        fp=fopen("xxx.txt","w");
        if(fp)
        {
            DumpGameVars(fp);
            fclose(fp);
        }
        AddLog("Done Dumping...");
    }
#endif
    return(0);
corrupt:
    return(1);
}

void SaveGameVars(FILE *fil)
{
    int i;
    long l;

    //   AddLog("Saving Game Vars to File");
    dfwrite(&iGameVarCount,sizeof(iGameVarCount),1,fil);

    for(i=0;i<iGameVarCount;i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(MATTGAMEVAR),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil);
    }

    //     dfwrite(&aGameVars,sizeof(aGameVars),1,fil);

    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].plValues,sizeof(long) * MAXPLAYERS, 1, fil);
        }
        else if( aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].plValues,sizeof(long) * MAXSPRITES, 1, fil);
        }
        // else nothing 'extra...'
    }

    for(i=0;i<MAXGAMEEVENTS;i++)
        if(apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]-(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
        }
    dfwrite(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil);
    for(i=0;i<MAXGAMEEVENTS;i++)
        if(apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]+(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
        }

    Bsprintf(g_szBuf,"EOF: EDuke32");
    l=strlen(g_szBuf);
    dfwrite(&l,sizeof(l),1,fil);
    dfwrite(g_szBuf,l,1,fil);
}

void DumpGameVars(FILE *fp)
{
    int i;
    if(!fp)
    {
        return;
    }
    fprintf(fp,"// Current Game Definitions\n\n");
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_SECRET) )
        {
            continue; // do nothing...
        }
        else
        {
            fprintf(fp,"gamevar %s ",aGameVars[i].szLabel);

            if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PLONG) )
                fprintf(fp,"%ld",*((long*)aGameVars[i].lValue));
            else
                fprintf(fp,"%ld",aGameVars[i].lValue);
            if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERPLAYER) )
                fprintf(fp," GAMEVAR_FLAG_PERPLAYER");
            else if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR) )
                fprintf(fp," GAMEVAR_FLAG_PERACTOR");
            else
                fprintf(fp," %ld",aGameVars[i].dwFlags & (GAMEVAR_FLAG_USER_MASK));
            fprintf(fp," // ");
            if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_SYSTEM))
                fprintf(fp," (system)");
            if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PLONG))
                fprintf(fp," (pointer)");
            if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_READONLY) )
                fprintf(fp," (read only)");
            fprintf(fp,"\n");
        }
    }
    fprintf(fp,"\n// end of game definitions\n");
}

void ResetGameVars(void)
{
    int i;

    //AddLog("Reset Game Vars");
    FreeGameVars();

    for(i=0;i<iDefaultGameVarCount;i++)
    {
        //Bsprintf(g_szBuf,"Resetting %d: '%s' to %ld",i,aDefaultGameVars[i].szLabel,
        //      aDefaultGameVars[i].lValue
        //      );
        //AddLog(g_szBuf);
        AddGameVar(aDefaultGameVars[i].szLabel,aDefaultGameVars[i].lValue,aDefaultGameVars[i].dwFlags|GAMEVAR_FLAG_NODEFAULT);
    }
}

char AddGameVar(char *pszLabel, long lValue, unsigned long dwFlags)
{
    int i;
    int j;

    //Bsprintf(g_szBuf,"AddGameVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if(Bstrlen(pszLabel) > (MAXVARLABEL-1) )
    {
        error++;
        initprintf("%s:%ld: error: variable name `%s' exceeds limit of %d characters.\n",compilefile,line_number,pszLabel, MAXVARLABEL);
        return 0;
    }
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].szLabel != NULL)
        {
            if( Bstrcmp(pszLabel,aGameVars[i].szLabel) == 0 )
            {
                // found it...
                if(aGameVars[i].dwFlags & GAMEVAR_FLAG_PLONG)
                {
                    //                 warning++;
                    //                 initprintf("%s:%ld: warning: Internal gamevar '%s' cannot be redefined.\n",compilefile,line_number,label+(labelcnt<<6));
                    ReportError(-1);
                    initprintf("%s:%ld: warning: cannot redefine internal gamevar `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
                    return 0;
                }
                else if((aGameVars[i].dwFlags & GAMEVAR_FLAG_DEFAULT) || (aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM))
                {
                    //Bsprintf(g_szBuf,"Replacing %s at %d",pszLabel,i);
                    //AddLog(g_szBuf);
                    //b=1;
                    // it's OK to replace
                    break;
                }
                else
                {
                    // it's a duplicate in error
                    warning++;
                    ReportError(WARNING_DUPLICATEDEFINITION);
                    return 0;
                }
            }
        }
    }
    if( i < MAXGAMEVARS)
    {
        // Set values
        if(aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM)
        {
            //if(b)
            //{
            //Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
            //}
            // if existing is system, they only get to change default value....
            aGameVars[i].lValue=lValue;
            if(!(dwFlags & GAMEVAR_FLAG_NODEFAULT))
            {
                aDefaultGameVars[i].lValue=lValue;
            }
        }
        else
        {
            if(aGameVars[i].szLabel == NULL)
                aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
            Bstrcpy(aGameVars[i].szLabel,pszLabel);
            aGameVars[i].dwFlags=dwFlags;
            aGameVars[i].lValue=lValue;
            if(!(dwFlags & GAMEVAR_FLAG_NODEFAULT))
            {
                if(aDefaultGameVars[i].szLabel == NULL)
                    aDefaultGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
                Bstrcpy(aDefaultGameVars[i].szLabel,pszLabel);
                aDefaultGameVars[i].dwFlags=dwFlags;
                aDefaultGameVars[i].lValue=lValue;
            }
        }

        if(i==iGameVarCount)
        {
            // we're adding a new one.
            iGameVarCount++;
            if(!(dwFlags & GAMEVAR_FLAG_NODEFAULT))
            {
                iDefaultGameVarCount++;
            }
        }
        if(aGameVars[i].plValues && !(aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM))
        {
            // only free if not system
            Bfree(aGameVars[i].plValues);
            aGameVars[i].plValues=NULL;
        }
        if(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            if(!aGameVars[i].plValues)
                aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(long));
            for(j=0;j<MAXPLAYERS;j++)
                aGameVars[i].plValues[j]=lValue;
        }
        else if( aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            if(!aGameVars[i].plValues)
                aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(long));
            for(j=0;j<MAXSPRITES;j++)
                aGameVars[i].plValues[j]=lValue;
        }
        return 1;
    }
    else
    {
        // no room to add...
        return 0;
    }
}

void ResetActorGameVars(short sActor)
{
    int i;

    for(i=0;i<iDefaultGameVarCount;i++)
        if( i < MAXGAMEVARS)
            if( aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
                aGameVars[i].plValues[sActor]=aDefaultGameVars[i].lValue;
}

int GetGameID(char *szGameLabel)
{
    int i;

    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].szLabel != NULL)
        {
            if( Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
            {
                return i;
            }
        }
    }
    return -1;
}

long GetGameVarID(int id, short sActor, short sPlayer)
{
    int m=0;
    if(id<0 || id >= iGameVarCount)
    {
        if(id==MAXGAMEVARS)
            return(*insptr++);
        else if(id&(MAXGAMEVARS<<1))
        {
            m=1;
            id ^= (MAXGAMEVARS<<1);
        }
        else
        {
            AddLog("GetGameVarID: Invalid Game ID");
            return -1;
        }
    }
    if( id == g_iThisActorID )
    {
        return sActor;
    }
    if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER )
    {
        // for the current player
        if(sPlayer >=0 && sPlayer < MAXPLAYERS)
        {
            //Bsprintf(g_szBuf,"GetGameVarID( %d, %d, %d) returns %ld\n",id,sActor,sPlayer, aGameVars[id].plValues[sPlayer]);
            //AddLog(g_szBuf);
            if(m) return -aGameVars[id].plValues[sPlayer];
            else return aGameVars[id].plValues[sPlayer];
        }
        else
        {
            if(m) return -aGameVars[id].lValue;
            else return aGameVars[id].lValue;
        }
    }
    else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR )
    {
        // for the current actor
        if(sActor >= 0 && sActor <=MAXSPRITES)
        {
            if(m) return -aGameVars[id].plValues[sActor];
            else return aGameVars[id].plValues[sActor];
        }
        else
        {
            if(m) return -aGameVars[id].lValue;
            else return aGameVars[id].lValue;
        }
    }
    else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG )
    {
        if(m) return -(*((long*)aGameVars[id].lValue));
        else return (*((long*)aGameVars[id].lValue));
    }
    else
    {
        if(m) return -aGameVars[id].lValue;
        else return aGameVars[id].lValue;
    }
}

void SetGameVarID(int id, long lValue, short sActor, short sPlayer)
{
    if(id<0 || id >= iGameVarCount)
    {
        AddLog("Invalid Game ID");
        return;
    }
    //Bsprintf(g_szBuf,"SGVI: %d ('%s') to %ld for %d %d",id,aGameVars[id].szLabel,lValue,sActor,sPlayer);
    //AddLog(g_szBuf);
    if((aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER) && (sPlayer != -1))
    {
        // for the current player
        aGameVars[id].plValues[sPlayer]=lValue;
    }
    else if((aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR) && (sActor != -1))
    {
        // for the current actor
        aGameVars[id].plValues[sActor]=lValue;
    }
    else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG )
    {
        // set the value at pointer
        *((long*)aGameVars[id].lValue)=lValue;
    }
    else
    {
        aGameVars[id].lValue=lValue;
    }
}

long GetGameVar(char *szGameLabel, long lDefault, short sActor, short sPlayer)
{
    int i;
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].szLabel != NULL)
        {
            if( Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
            {
                return GetGameVarID(i, sActor, sPlayer);
            }
        }
    }
    return lDefault;
}

long *GetGameValuePtr(char *szGameLabel)
{
    int i;
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].szLabel != NULL)
        {
            if( Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
            {
                if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR | GAMEVAR_FLAG_PERPLAYER))
                {
                    if(!aGameVars[i].plValues)
                    {
                        AddLog("INTERNAL ERROR: NULL array !!!");
                    }
                    return aGameVars[i].plValues;
                }
                return &(aGameVars[i].lValue);
            }
        }
    }
    //Bsprintf(g_szBuf,"Could not find value '%s'\n",szGameLabel);
    //AddLog(g_szBuf);
    return NULL;
}

long GetDefID(char *szGameLabel)
{
    int i;
    for(i=0;i<iGameVarCount;i++)
    {
        if(aGameVars[i].szLabel != NULL)
        {
            if( Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
            {
                return i;
            }
        }
    }
    return -1;
}

char ispecial(char c)
{
    if(c == 0x0a)
    {
        line_number++;
        return 1;
    }

    if(c == ' ' || c == 0x0d)
        return 1;

    return 0;
}

char isaltok(char c)
{
    return ( isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

long getlabelid(LABELS *pLabel, char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    long l=-1;
    int i;

    for(i=0;pLabel[i].lId >=0 ; i++)
    {
        if(!Bstrcasecmp(pLabel[i].name,psz))
        {
            l= pLabel[i].lId;
            break;  // stop for loop
        }
    }
    return l;
}

long getlabeloffset(LABELS *pLabel, char *psz)
{
    // find the label psz in the table pLabel.
    // returns the offset in the array for the label, or -1
    int i;

    for(i=0;pLabel[i].lId >=0 ; i++)
    {
        if(!Bstrcasecmp(pLabel[i].name,psz))
        {
            //    printf("Label has flags of %02X\n",pLabel[i].flags);
            return i;
        }
    }
    return -1;
}

void getlabel(void)
{
    long i;

    skipcomments();

    while( isalnum(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0)
            return;
    }

    i = 0;
    while( ispecial(*textptr) == 0 && *textptr!=']' && *textptr!='\t' && *textptr!='\n' && *textptr!='\r')
        label[(labelcnt<<6)+i++] = *(textptr++);

    label[(labelcnt<<6)+i] = 0;
    if (!(error || warning) && condebug > 1)
        initprintf("%s:%ld: debug: got label `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
}

long keyword(void)
{
    long i;
    char *temptextptr;

    skipcomments();

    temptextptr = textptr;

    while( isaltok(*temptextptr) == 0 )
    {
        temptextptr++;
        if( *temptextptr == 0 )
            return 0;
    }

    i = 0;
    while( isaltok(*temptextptr) )
    {
        tempbuf[i] = *(temptextptr++);
        i++;
    }
    tempbuf[i] = 0;
    for(i=0;i<NUMKEYWORDS;i++)
        if( Bstrcmp( tempbuf,keyw[i]) == 0 )
            return i;

    return -1;
}

long transword(void) //Returns its code #
{
    long i, l;

    skipcomments();

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        if( *textptr == 0 )
            return -1;
        textptr++;
    }

    l = 0;
    while( isaltok(*(textptr+l)) && !(*(textptr + l) == '.') )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
    {
        if( Bstrcmp( tempbuf,keyw[i]) == 0 )
        {
            *scriptptr = i;
            textptr += l;
            scriptptr++;
            if (!(error || warning) && condebug)
                initprintf("%s:%ld: debug: translating keyword `%s'.\n",compilefile,line_number,keyw[i]);
            return i;
        }
    }

    textptr += l;

    if( tempbuf[0] == '{' && tempbuf[1] != 0)
    {
        ReportError(-1);
        initprintf("%s:%ld: error: expected a SPACE or CR between `{' and `%s'.\n",compilefile,line_number,tempbuf+1);
    }
    else if( tempbuf[0] == '}' && tempbuf[1] != 0)
    {
        ReportError(-1);
        initprintf("%s:%ld: error: expected a SPACE or CR between `}' and `%s'.\n",compilefile,line_number,tempbuf+1);
    }
    else ReportError(ERROR_EXPECTEDKEYWORD);
    error++;
    return -1;
}

void transvartype(int type)
{
    int i=0,f=0;

    skipcomments();
    if(!type && !labelsonly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        if (!(error || warning) && condebug)
            initprintf("%s:%ld: debug: accepted constant %ld in place of gamevar.\n",compilefile,line_number,atol(textptr));
        *scriptptr++=MAXGAMEVARS;
        *scriptptr++=atol(textptr);
        getlabel();
        return;
    }
    else if((*textptr == '-') && !isdigit(*(textptr+1)))
    {
        if(!type)
        {
            if (!(error || warning) && condebug)
                initprintf("%s:%ld: debug: flagging gamevar as negative.\n",compilefile,line_number,atol(textptr));
            f = (MAXGAMEVARS<<1);
        }
        else
        {
            error++;
            ReportError(ERROR_SYNTAXERROR);
            getlabel();
            return;
        }
    }
    getlabel();
    if(!nokeywordcheck)
        for(i=0;i<NUMKEYWORDS;i++)
            if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return;
            }
    i=GetDefID(label+(labelcnt<<6));
    if(i<0)
    {
        if(!type && !labelsonly)
        {
            Bstrcpy(tempbuf,label+(labelcnt<<6));
            for(i=0;i<labelcnt;i++)
            {
                if( Bstrcmp(tempbuf,label+(i<<6)) == 0 && (labeltype[i] & LABEL_DEFINE))
                {
                    if (!(error || warning) && condebug)
                        initprintf("%s:%ld: debug: accepted defined label `%s' instead of gamevar.\n",compilefile,line_number,label+(i<<6));
                    *scriptptr++=MAXGAMEVARS;
                    *scriptptr++=labelcode[i];
                    textptr++;
                    return;
                }
            }
            error++;
            ReportError(ERROR_NOTAGAMEVAR);
            textptr++;
            return;
        }
        else
        {
            error++;
            ReportError(ERROR_NOTAGAMEVAR);
            textptr++;
            return;
        }
    }
    if(type == GAMEVAR_FLAG_READONLY && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
    {
        error++;
        ReportError(ERROR_VARREADONLY);
        return;
    }
    else if(aGameVars[i].dwFlags & type)
    {
        error++;
        ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }
    if((aGameVars[i].dwFlags & GAMEVAR_FLAG_SYNCCHECK) && parsing_actor && CheckEventSync(current_event))
    {
        ReportError(-1);
        initprintf("%s:%ld: warning: found local gamevar `%s' used within %s.\n",compilefile,line_number,label+(labelcnt<<6),parsing_event?"a synced event":"an actor");
    }
    if (!(error || warning) && condebug > 1)
        initprintf("%s:%ld: debug: accepted gamevar `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
    i |= f;
    *scriptptr++=i;
}

inline void transvar(void)
{
    transvartype(0);
}

inline void transmultvarstype(int type, char num)
{
    char i;
    for(i=0;i<num;i++)
        transvartype(type);
}

inline void transmultvars(char num)
{
    transmultvarstype(0,num);
}

long transnum(long type)
{
    long i, l;

    skipcomments();

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0 )
            return -1; // eof
    }

    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if(!nokeywordcheck)
        for(i=0;i<NUMKEYWORDS;i++)
            if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                textptr+=l;
            }

    for(i=0;i<labelcnt;i++)
    {
        if( !Bstrcmp(tempbuf,label+(i<<6)) )
        {
            char *el,*gl;

            if (labeltype[i] & type)
            {
                if (!(error || warning) && condebug > 1)
                {
                    gl = translatelabeltype(labeltype[i]);
                    initprintf("%s:%ld: debug: accepted %s label `%s'.\n",compilefile,line_number,gl,label+(i<<6));
                    Bfree(gl);
                }
                *(scriptptr++) = labelcode[i];
                textptr += l;
                return labeltype[i];
            }
            *(scriptptr++) = 0;
            textptr += l;
            el = translatelabeltype(type);
            gl = translatelabeltype(labeltype[i]);
            ReportError(-1);
            initprintf("%s:%ld: warning: expected a %s, found a %s.\n",compilefile,line_number,el,gl);
            Bfree(el);
            Bfree(gl);
            return -1;  // valid label name, but wrong type
        }
    }

    if( isdigit(*textptr) == 0 && *textptr != '-')
    {
        ReportError(ERROR_PARAMUNDEFINED);
        error++;
        textptr+=l;
        return -1; // error!
    }

    if( isdigit(*textptr) && labelsonly )
    {
        ReportError(WARNING_LABELSONLY);
        //         warning++;
    }
    if (!(error || warning) && condebug > 1)
        initprintf("%s:%ld: debug: accepted constant %ld.\n",compilefile,line_number,atol(textptr));
    *scriptptr = atol(textptr);
    scriptptr++;

    textptr += l;

    return 0;   // literal value
}

long CountCaseStatements()
{
    long lCount;
    char *temptextptr;
    long *savescript;
    long *savecase;
    short temp_line_number;

    temp_line_number=line_number;

    casecount=0;
    temptextptr=textptr;
    savescript=scriptptr;
    savecase=casescriptptr;
    casescriptptr=NULL;
    //Bsprintf(g_szBuf,"CSS: %.12s",textptr);
    //AddLog(g_szBuf);
    while(parsecommand() == 0)
    {
        //Bsprintf(g_szBuf,"CSSL: %.20s",textptr);
        //AddLog(g_szBuf);
        ;
    }
    // since we processed the endswitch, we need to re-increment checking_switch
    checking_switch++;

    textptr=temptextptr;
    scriptptr=savescript;

    line_number = temp_line_number;

    lCount=casecount;
    casecount=0;
    casescriptptr=savecase;
    return lCount;
}

char parsecommand(void)
{
    long i, j=0, k=0, *tempscrptr;
    char done, *temptextptr;

    long tw;

    if (((unsigned)(scriptptr-script) > MAXSCRIPTSIZE) && error == 0) {
        /* Bsprintf(tempbuf,"fatal error: Size of compiled CON code exceeds maximum size! (%ud, %d)\n",(unsigned)(scriptptr-script),MAXSCRIPTSIZE); */
        ReportError(-1);
        initprintf("%s:%ld: internal compiler error: Aborted (%ud)\n",compilefile,line_number,(unsigned)(scriptptr-script));
        initprintf(tempbuf);
        error++;
    }

    if( (error+warning) > 63 || ( *textptr == '\0' ) || ( *(textptr+1) == '\0' ) ) return 1;

    if (checking_switch > 0 )
    {
        //Bsprintf(g_szBuf,"PC(): '%.25s'",textptr);
        //AddLog(g_szBuf);
    }
    tw = transword();
    //    Bsprintf(tempbuf,"%s",keyw[tw]);
    //    AddLog(tempbuf);

    skipcomments(); // yes?  no?

    switch(tw)
    {
    default:
    case -1:
        return 0; //End
    case CON_STATE:
        if( parsing_actor == 0 && parsing_state == 0 )
        {
            getlabel();
            scriptptr--;
            labelcode[labelcnt] = (long) scriptptr;
            labeltype[labelcnt] = LABEL_STATE;

            parsing_state = 1;
            Bsprintf(parsing_item_name,"%s",label+(labelcnt<<6));
            labelcnt++;
            return 0;
        }

        getlabel();

        for(i=0;i<NUMKEYWORDS;i++)
            if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }
        for(j=0;j<labelcnt;j++)
        {
            if( Bstrcmp(label+(j<<6),label+(labelcnt<<6)) == 0)
            {
                if (labeltype[j] & LABEL_STATE)
                {
                    if (!(error || warning) && condebug > 1)
                        initprintf("%s:%ld: debug: accepted state label `%s'.\n",compilefile,line_number,label+(j<<6));
                    *scriptptr = labelcode[j];
                    break;
                }
                else
                {
                    char *gl;
                    gl = translatelabeltype(labeltype[j]);
                    ReportError(-1);
                    initprintf("%s:%ld: warning: expected a state, found a %s.\n",compilefile,line_number,gl);
                    Bfree(gl);
                    *(scriptptr-1) = CON_NULLOP; // get rid of the state, leaving a nullop to satisfy if conditions
                    return 0;  // valid label name, but wrong type
                }
            }
        }
        if(j==labelcnt)
        {
            ReportError(-1);
            initprintf("%s:%ld: error: state `%s' not found.\n",compilefile,line_number,label+(labelcnt<<6));
            error++;
        }
        scriptptr++;
        return 0;

    case CON_ENDS:
        if( parsing_state == 0 )
        {
            ReportError(-1);
            initprintf("%s:%ld: error: found `ends' without open `state'.\n",compilefile,line_number);
            error++;
        }
        //            else
        {
            if( num_braces > 0 )
            {
                ReportError(ERROR_OPENBRACKET);
                error++;
            }
            if( num_braces < 0 )
            {
                ReportError(ERROR_CLOSEBRACKET);
                error++;
            }
            if( checking_switch > 0  )
            {
                ReportError(ERROR_NOENDSWITCH);
                error++;

                checking_switch = 0; // can't be checking anymore...
            }

            parsing_state = 0;
            Bsprintf(parsing_item_name,"(none)");
        }
        return 0;

    case CON_SETTHISPROJECTILE:
    case CON_SETPROJECTILE:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETTHISPROJECTILE:
    case CON_GETPROJECTILE:
        {
            long lLabelID;

            // syntax getwall[<var>].x <VAR>
            // gets the value of wall[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            if(tw == CON_SETTHISPROJECTILE)
                labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabeloffset(projectilelabels,label+(labelcnt<<6));
            //printf("LabelID is %ld\n",lLabelID);
            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }

            *scriptptr++=projectilelabels[lLabelID].lId;

            //printf("member's flags are: %02Xh\n",playerlabels[lLabelID].flags);

            // now at target VAR...

            // get the ID of the DEF
            switch(tw)
            {
            case CON_SETPROJECTILE:
            case CON_SETTHISPROJECTILE:
                transvar();
                break;
            default:
                transvartype(GAMEVAR_FLAG_READONLY);
                break;
            }
            break;
        }

    case CON_GAMEVAR:
        // syntax: gamevar <var1> <initial value> <flags>
        // defines var1 and sets initial value.
        // flags are used to define usage
        // (see top of this files for flags)
        //printf("Got gamedef. Getting Label. '%.20s'\n",textptr);

        if(isdigit(*textptr) || (*textptr == '-'))
        {
            getlabel();
            error++;
            ReportError(ERROR_SYNTAXERROR);
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            scriptptr -= 3; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }

        getlabel();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined

        for(i=0;i<NUMKEYWORDS;i++)
            if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }
#if 0
        for(i=0;i<iGameVarCount;i++)
        {
            if(aGameVars[i].szLabel != NULL)
            {
                if( Bstrcmp(label+(labelcnt<<6),aGameVars[i].szLabel) == 0 )
                {
                    warning++;
                    initprintf("  * WARNING.(L%ld) duplicate Game definition `%s' ignored.\n",line_number,label+(labelcnt<<6));
                    break;
                }
            }
        }
#endif

        //printf("Translating number  '%.20s'\n",textptr);
        transnum(LABEL_DEFINE); // get initial value
        //printf("Done Translating number.  '%.20s'\n",textptr);

        transnum(LABEL_DEFINE); // get flags
        //Bsprintf(g_szBuf,"Adding GameVar='%s', val=%l, flags=%lX",label+(labelcnt<<6),
        //      *(scriptptr-2), *(scriptptr-1));
        //AddLog(g_szBuf);

        AddGameVar(label+(labelcnt<<6),*(scriptptr-2),
                   (*(scriptptr-1))
                   // can't define default or secret
                   & (~( GAMEVAR_FLAG_DEFAULT | GAMEVAR_FLAG_SECRET))
                  );
        //AddLog("Added gamevar");
        scriptptr -= 3; // no need to save in script...
        return 0;

    case CON_DEFINE:
        {
            //printf("Got definition. Getting Label. '%.20s'\n",textptr);
            getlabel();
            //printf("Got label. '%.20s'\n",textptr);
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
            {
                if(Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 && (labeltype[i] & LABEL_DEFINE))
                {
                    if (i >= defaultlabelcnt)
                    {
                        warning++;
                        ReportError(WARNING_DUPLICATEDEFINITION);
                    }
                    break;
                }
            }
            //printf("Translating. '%.20s'\n",textptr);
            transnum(LABEL_DEFINE);
            //printf("Translated. '%.20s'\n",textptr);
            if(i == labelcnt)
            {
                //              printf("Defining Definition '%s' to be '%d'\n",label+(labelcnt<<6),*(scriptptr-1));
                labeltype[labelcnt] = LABEL_DEFINE;
                labelcode[labelcnt++] = *(scriptptr-1);
                if (*(scriptptr-1) >= 0 && *(scriptptr-1) < MAXTILES && dynamicremap)
                    processnames(label+(i<<6),*(scriptptr-1));
            }
            scriptptr -= 2;
            return 0;
        }

    case CON_PALFROM:
        for(j=0;j<4;j++)
        {
            if( keyword() == -1 )
                transnum(LABEL_DEFINE);
            else break;
        }

        while(j<4)
        {
            *scriptptr = 0;
            scriptptr++;
            j++;
        }
        return 0;

    case CON_MOVE:
        if( parsing_actor || parsing_state )
        {
            if(!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transnum(LABEL_MOVE);

            j = 0;
            while(keyword() == -1)
            {
                transnum(LABEL_DEFINE);
                scriptptr--;
                j |= *scriptptr;
            }
            *scriptptr = j;

            scriptptr++;
        }
        else
        {
            scriptptr--;
            getlabel();
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
                if( Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 && (labeltype[i] & LABEL_MOVE))
                {
                    warning++;
                    initprintf("%s:%ld: warning: duplicate move `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }
            if(i == labelcnt)
            {
                labeltype[labelcnt] = LABEL_MOVE;
                labelcode[labelcnt++] = (long) scriptptr;
            }
            for(j=0;j<2;j++)
            {
                if(keyword() >= 0) break;
                transnum(LABEL_DEFINE);
            }
            for(k=j;k<2;k++)
            {
                *scriptptr = 0;
                scriptptr++;
            }
        }
        return 0;

    case CON_MUSIC:
        {
            // NOTE: this doesn't get stored in the PCode...

            // music 1 stalker.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
            //    thecall.mid ahgeez.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
            scriptptr--;
            transnum(LABEL_DEFINE); // Volume Number (0/4)
            scriptptr--;

            k = *scriptptr-1;

            if(k >= 0) // if it's background music
            {
                i = 0;
                // get the file name...
                while(keyword() == -1)
                {
                    while( isaltok(*textptr) == 0 )
                    {
                        if(*textptr == 0x0a) line_number++;
                        textptr++;
                        if( *textptr == 0 ) break;
                    }
                    j = 0;
                    while( isaltok(*(textptr+j)) )
                    {
                        music_fn[k][i][j] = textptr[j];
                        j++;
                    }
                    music_fn[k][i][j] = '\0';
                    textptr += j;
                    if(i > 9) break;
                    i++;
                }
            }
            else
            {
                i = 0;
                while(keyword() == -1)
                {
                    while( isaltok(*textptr) == 0 )
                    {
                        if(*textptr == 0x0a) line_number++;
                        textptr++;
                        if( *textptr == 0 ) break;
                    }
                    j = 0;
                    while( isaltok(*(textptr+j)) )
                    {
                        env_music_fn[i][j] = textptr[j];
                        j++;
                    }
                    env_music_fn[i][j] = '\0';

                    textptr += j;
                    if(i > 9) break;
                    i++;
                }
            }
        }
        return 0;

    case CON_INCLUDE:
        scriptptr--;
        while( isaltok(*textptr) == 0 )
        {
            if(*textptr == 0x0a) line_number++;
            textptr++;
            if( *textptr == 0 ) break;
        }
        j = 0;
        while( isaltok(*textptr) )
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';

        {
            short temp_line_number;
            char  temp_ifelse_check;
            char *origtptr, *mptr;
            char parentcompilefile[255];
            int fp;

            fp = kopen4load(tempbuf,loadfromgrouponly);
            if(fp < 0)
            {
                error++;
                initprintf("%s:%ld: error: could not find file `%s'.\n",compilefile,line_number,tempbuf);
                return 0;
            }

            j = kfilelength(fp);

            mptr = (char *)Bmalloc(j+1);
            if (!mptr)
            {
                kclose(fp);
                error++;
                initprintf("%s:%ld: error: could not allocate %ld bytes to include `%s'.\n",
                           line_number,compilefile,j,tempbuf);
                return 0;
            }

            initprintf("Including: %s (%ld bytes)\n",tempbuf, j);
            kread(fp, mptr, j);
            kclose(fp);
            mptr[j] = 0;

            if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
                textptr++;
            origtptr = textptr;

            Bstrcpy(parentcompilefile, compilefile);
            Bstrcpy(compilefile, tempbuf);
            temp_line_number = line_number;
            line_number = 1;
            temp_ifelse_check = checking_ifelse;
            checking_ifelse = 0;

            textptr = mptr;
            do done = parsecommand(); while (!done);

            Bstrcpy(compilefile, parentcompilefile);
            total_lines += line_number;
            line_number = temp_line_number;
            checking_ifelse = temp_ifelse_check;

            textptr = origtptr;

            Bfree(mptr);
        }
        return 0;

    case CON_AI:
        if( parsing_actor || parsing_state )
        {
            if(!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transnum(LABEL_AI);
        }
        else
        {
            scriptptr--;
            getlabel();

            for(i=0;i<NUMKEYWORDS;i++)
                if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
                if( Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 && (labeltype[i] & LABEL_AI))
                {
                    warning++;
                    initprintf("%s:%ld: warning: duplicate ai `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }

            if(i == labelcnt)
            {
                labeltype[labelcnt] = LABEL_AI;
                labelcode[labelcnt++] = (long) scriptptr;
            }

            for(j=0;j<3;j++)
            {
                if(keyword() >= 0) break;
                if(j == 1)
                    transnum(LABEL_ACTION);
                else if(j == 2)
                {
                    transnum(LABEL_MOVE);
                    k = 0;
                    while(keyword() == -1)
                    {
                        transnum(LABEL_DEFINE);
                        scriptptr--;
                        k |= *scriptptr;
                    }
                    *scriptptr = k;
                    scriptptr++;
                    return 0;
                }
            }
            for(k=j;k<3;k++)
            {
                *scriptptr = 0;
                scriptptr++;
            }
        }
        return 0;

    case CON_ACTION:
        if( parsing_actor || parsing_state )
        {
            if(!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transnum(LABEL_ACTION);
        }
        else
        {
            scriptptr--;
            getlabel();
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }
            for(i=0;i<labelcnt;i++)
                if( Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 && (labeltype[i] & LABEL_ACTION))
                {
                    warning++;
                    initprintf("%s:%ld: warning: duplicate action `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }

            if(i == labelcnt)
            {
                labeltype[labelcnt] = LABEL_ACTION;
                labelcode[labelcnt] = (long) scriptptr;
                labelcnt++;
            }

            for(j=0;j<5;j++)
            {
                if(keyword() >= 0) break;
                transnum(LABEL_DEFINE);
            }
            for(k=j;k<5;k++)
            {
                *scriptptr = 0;
                scriptptr++;
            }
        }
        return 0;

    case CON_ACTOR:
        if( parsing_state || parsing_actor )
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_actor = scriptptr;

        skipcomments();
        j = 0;
        while( isaltok(*(textptr+j)) )
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;
        transnum(LABEL_DEFINE);
        //         Bsprintf(parsing_item_name,"%s",label+(labelcnt<<6));
        scriptptr--;
        actorscrptr[*scriptptr] = parsing_actor;

        for(j=0;j<4;j++)
        {
            *(parsing_actor+j) = 0;
            if(j == 3)
            {
                j = 0;
                while(keyword() == -1)
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                *scriptptr = j;
                scriptptr++;
                break;
            }
            else
            {
                if(keyword() >= 0)
                {
                    scriptptr += (4-j);
                    break;
                }
                switch(j)
                {
                case 0: transnum(LABEL_DEFINE); break;
                case 1: transnum(LABEL_ACTION); break;
                case 2: transnum(LABEL_MOVE|LABEL_DEFINE); break;
                }
                *(parsing_actor+j) = *(scriptptr-1);
            }
        }
        checking_ifelse = 0;
        return 0;

    case CON_ONEVENT:
        if( parsing_state || parsing_actor )
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_event = scriptptr;
        parsing_actor = scriptptr;

        skipcomments();
        j = 0;
        while( isaltok(*(textptr+j)) )
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;
        labelsonly = 1;
        transnum(LABEL_DEFINE);
        labelsonly = 0;
        scriptptr--;
        j= *scriptptr;  // type of event
        current_event = j;
        //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, parsing_event);
        //AddLog(g_szBuf);
        if(j > MAXGAMEEVENTS-1 || j < 0)
        {
            initprintf("%s:%ld: error: invalid event ID.\n",compilefile,line_number);
            error++;
            return 0;
        }

        if(apScriptGameEvent[j])
        {
            tempscrptr = parsing_event;
            parsing_event = parsing_actor = 0;
            ReportError(-1);
            parsing_event = parsing_actor = tempscrptr;
            initprintf("%s:%ld: warning: duplicate event `%s'.\n",compilefile,line_number,parsing_item_name);
        }
        else apScriptGameEvent[j]=parsing_event;

        checking_ifelse = 0;

        return 0;

    case CON_EVENTLOADACTOR:
        if( parsing_state || parsing_actor )
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_actor = scriptptr;

        skipcomments();
        j = 0;
        while( isaltok(*(textptr+j)) )
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;
        transnum(LABEL_DEFINE);
        scriptptr--;
        actorLoadEventScrptr[*scriptptr] = parsing_actor;

        checking_ifelse = 0;
        return 0;

    case CON_USERACTOR:
        if( parsing_state || parsing_actor )
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_actor = scriptptr;

        transnum(LABEL_DEFINE);
        scriptptr--;

        skipcomments();
        j = 0;
        while( isaltok(*(textptr+j)) )
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;

        j = *scriptptr;
        transnum(LABEL_DEFINE);
        scriptptr--;
        actorscrptr[*scriptptr] = parsing_actor;
        actortype[*scriptptr] = j;

        for(j=0;j<4;j++)
        {
            *(parsing_actor+j) = 0;
            if(j == 3)
            {
                j = 0;
                while(keyword() == -1)
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                *scriptptr = j;
                scriptptr++;
                break;
            }
            else
            {
                if(keyword() >= 0)
                {
                    for (i=4-j; i; i--) *(scriptptr++) = 0;
                    break;
                }
                switch(j)
                {
                case 0: transnum(LABEL_DEFINE); break;
                case 1: transnum(LABEL_ACTION); break;
                case 2: transnum(LABEL_MOVE|LABEL_DEFINE); break;
                }
                *(parsing_actor+j) = *(scriptptr-1);
            }
        }
        checking_ifelse = 0;
        return 0;

    case CON_INSERTSPRITEQ:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        return 0;

    case CON_DYNQUOTE:
        transnum(LABEL_DEFINE);
        for(j = 0;j < 4;j++)
        {
            if( keyword() == -1 )
                transvar();
            else break;
        }

        while(j < 4)
        {
            *scriptptr = 0;
            scriptptr++;
            j++;
        }
        return 0;

    case CON_ESPAWN:
    case CON_ESHOOT:
    case CON_QSPAWN:
    case CON_EQSPAWN:
    case CON_STRENGTH:
    case CON_SHOOT:
    case CON_ADDPHEALTH:
    case CON_SPAWN:
    case CON_CSTAT:
    case CON_COUNT:
    case CON_ENDOFGAME:
    case CON_SPRITEPAL:
    case CON_CACTOR:
    case CON_MONEY:
    case CON_ADDKILLS:
    case CON_DEBUG:
    case CON_ADDSTRENGTH:
    case CON_CSTATOR:
    case CON_MAIL:
    case CON_PAPER:
    case CON_SLEEPTIME:
    case CON_CLIPDIST:
    case CON_LOTSOFGLASS:
    case CON_SAVE:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_ANGOFF:
    case CON_USERQUOTE:
    case CON_QUOTE:
    case CON_SOUND:
    case CON_GLOBALSOUND:
    case CON_SOUNDONCE:
    case CON_STOPSOUND:
        transnum(LABEL_DEFINE);
        if (tw == CON_CSTAT)
        {
            if(*(scriptptr-1) == 32767)
            {
                ReportError(-1);
                initprintf("%s:%ld: warning: tried to set cstat 32767, using 32768 instead.\n",compilefile,line_number);
                *(scriptptr-1) = 32768;
            }
            else if((*(scriptptr-1) & 32) && (*(scriptptr-1) & 16))
            {
                i = *(scriptptr-1);
                *(scriptptr-1) ^= 48;
                ReportError(-1);
                initprintf("%s:%ld: warning: tried to set cstat %ld, using %ld instead.\n",compilefile,line_number,i,*(scriptptr-1));
            }
        }
        return 0;

    case CON_HITRADIUS:
        transnum(LABEL_DEFINE);
        transnum(LABEL_DEFINE);
        transnum(LABEL_DEFINE);
    case CON_ADDAMMO:
    case CON_ADDWEAPON:
    case CON_SIZETO:
    case CON_SIZEAT:
    case CON_DEBRIS:
    case CON_ADDINVENTORY:
    case CON_GUTS:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transnum(LABEL_DEFINE);
        transnum(LABEL_DEFINE);
        break;

    case CON_ELSE:
        if( checking_ifelse )
        {
            checking_ifelse--;
            tempscrptr = scriptptr;
            scriptptr++; //Leave a spot for the fail location
            parsecommand();
            *tempscrptr = (long) scriptptr;
        }
        else
        {
            scriptptr--;
            error++;
            ReportError(-1);
            initprintf("%s:%ld: error: found `else' with no `if'.\n",compilefile,line_number);
        }
        return 0;

    case CON_SETSECTOR:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETSECTOR:
        {
            long lLabelID;

            // syntax getsector[<var>].x <VAR>
            // gets the value of sector[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabelid(sectorlabels,label+(labelcnt<<6));

            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }
            *scriptptr++=lLabelID;

            // now at target VAR...

            // get the ID of the DEF
            if (tw==CON_GETSECTOR)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_FINDNEARACTOR:
    case CON_FINDNEARACTOR3D:
    case CON_FINDNEARSPRITE:
    case CON_FINDNEARSPRITE3D:
        {
            // syntax findnearactor <type> <maxdist> <getvar>
            // gets the sprite ID of the nearest actor within max dist
            // that is of <type> into <getvar>
            // -1 for none found

            transnum(LABEL_DEFINE); // get <type>
            transnum(LABEL_DEFINE); // get maxdist

            switch(tw)
            {
            case CON_FINDNEARACTOR3D:
            case CON_FINDNEARSPRITE3D:
                transnum(LABEL_DEFINE);
            default:
                break;
            }

            // target var
            // get the ID of the DEF
            transvartype(GAMEVAR_FLAG_READONLY);
            break;
        }

    case CON_FINDNEARACTORVAR:
    case CON_FINDNEARACTOR3DVAR:
    case CON_FINDNEARSPRITEVAR:
    case CON_FINDNEARSPRITE3DVAR:
        {
            transnum(LABEL_DEFINE); // get <type>

            // get the ID of the DEF
            transvar();
            switch(tw)
            {
            case CON_FINDNEARACTOR3DVAR:
            case CON_FINDNEARSPRITE3DVAR:
                transvar();
            default:
                break;
            }
            // target var
            // get the ID of the DEF
            transvartype(GAMEVAR_FLAG_READONLY);
            break;
        }

    case CON_SQRT:
        {
            // syntax sqrt <invar> <outvar>
            // gets the sqrt of invar into outvar

            // get the ID of the DEF
            transvar();
            // target var
            // get the ID of the DEF
            transvartype(GAMEVAR_FLAG_READONLY);
            break;
        }

    case CON_SETWALL:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETWALL:
        {
            long lLabelID;

            // syntax getwall[<var>].x <VAR>
            // gets the value of wall[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabelid(walllabels,label+(labelcnt<<6));

            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }
            *scriptptr++=lLabelID;

            // now at target VAR...

            // get the ID of the DEF
            if (tw == CON_GETWALL)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_SETPLAYER:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETPLAYER:
        {
            long lLabelID;

            // syntax getwall[<var>].x <VAR>
            // gets the value of wall[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabeloffset(playerlabels,label+(labelcnt<<6));
            //printf("LabelID is %ld\n",lLabelID);
            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }

            *scriptptr++=playerlabels[lLabelID].lId;

            //printf("member's flags are: %02Xh\n",playerlabels[lLabelID].flags);
            if (playerlabels[lLabelID].flags & LABEL_HASPARM2)
            {
                //printf("Member has PARM2\n");
                // get parm2
                // get the ID of the DEF
                transvar();
            }
            else
            {
                //printf("Member does not have Parm2\n");
            }

            // now at target VAR...

            // get the ID of the DEF
            if (tw==CON_GETPLAYER)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_SETINPUT:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETINPUT:
        {
            long lLabelID;

            // syntax getwall[<var>].x <VAR>
            // gets the value of wall[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabeloffset(inputlabels,label+(labelcnt<<6));
            //printf("LabelID is %ld\n",lLabelID);
            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }

            *scriptptr++=inputlabels[lLabelID].lId;

            // now at target VAR...

            // get the ID of the DEF
            if (tw==CON_GETINPUT)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_SETUSERDEF:
    case CON_GETUSERDEF:
        {
            long lLabelID;

            // syntax [gs]etuserdef.x <VAR>
            // gets the value of ud.xxx into <VAR>

            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabelid(userdefslabels,label+(labelcnt<<6));

            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }
            *scriptptr++=lLabelID;

            // now at target VAR...

            // get the ID of the DEF
            if (tw==CON_GETUSERDEF)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_SETACTORVAR:
    case CON_SETPLAYERVAR:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETACTORVAR:
    case CON_GETPLAYERVAR:
        {
            // syntax [gs]etactorvar[<var>].<varx> <VAR>
            // gets the value of the per-actor variable varx into VAR

            // now get name of <var>
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .<varx>
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'

            // get the ID of the DEF
            getlabel();
            //printf("found label of '%s'\n",   label+(labelcnt<<6));

            // Check to see if it's a keyword
            for(i=0;i<NUMKEYWORDS;i++)
                if( Bstrcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            i=GetDefID(label+(labelcnt<<6));
            //printf("Label '%s' ID is %d\n",label+(labelcnt<<6), i);
            if(i<0)
            {   // not a defined DEF
                error++;
                ReportError(ERROR_NOTAGAMEVAR);
                return 0;
            }
            if(aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
            {
                error++;
                ReportError(ERROR_VARREADONLY);
                return 0;

            }

            switch(tw)
            {
            case CON_SETACTORVAR:
                {
                    if(!(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR))
                    {
                        error++;
                        ReportError(-1);
                        initprintf("%s:%ld: error: variable `%s' is not per-actor.\n",compilefile,line_number,label+(labelcnt<<6));
                        return 0;

                    }
                    break;
                }
            case CON_SETPLAYERVAR:
                {
                    if(!(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER))
                    {
                        error++;
                        ReportError(-1);
                        initprintf("%s:%ld: error: variable `%s' is not per-player.\n",compilefile,line_number,label+(labelcnt<<6));
                        return 0;

                    }
                    break;
                }
            }

            *scriptptr++=i; // the ID of the DEF (offset into array...)

            switch(tw)
            {
            case CON_GETACTORVAR:
            case CON_GETPLAYERVAR:
                transvartype(GAMEVAR_FLAG_READONLY);
                break;
            default:
                transvar();
                break;
            }
            break;
        }

    case CON_SETACTOR:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETACTOR:
        {
            long lLabelID;

            // syntax getwall[<var>].x <VAR>
            // gets the value of wall[<var>].xxx into <VAR>

            // now get name of .xxx
            while((*textptr != '['))
            {
                textptr++;
            }
            if(*textptr == '[')
                textptr++;

            // get the ID of the DEF
            labelsonly = 1;
            transvar();
            labelsonly = 0;
            // now get name of .xxx
            while(*textptr != '.')
            {
                if(*textptr == 0xa)
                    break;
                if(!*textptr)
                    break;

                textptr++;
            }
            if(*textptr!='.')
            {
                error++;
                ReportError(ERROR_SYNTAXERROR);
                return 0;
            }
            textptr++;
            /// now pointing at 'xxx'
            getlabel();
            //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

            lLabelID=getlabeloffset(actorlabels,label+(labelcnt<<6));
            //printf("LabelID is %ld\n",lLabelID);
            if(lLabelID == -1 )
            {
                error++;
                ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return 0;
            }

            *scriptptr++=actorlabels[lLabelID].lId;

            //printf("member's flags are: %02Xh\n",actorlabels[lLabelID].flags);
            if (actorlabels[lLabelID].flags & LABEL_HASPARM2)
            {
                //printf("Member has PARM2\n");
                // get parm2
                // get the ID of the DEF
                transvar();
            }
            else
            {
                //printf("Member does not have Parm2\n");
            }

            // now at target VAR...

            // get the ID of the DEF
            if(tw == CON_GETACTOR)
                transvartype(GAMEVAR_FLAG_READONLY);
            else
                transvar();
            break;
        }

    case CON_ESHOOTVAR:
    case CON_ESPAWNVAR:
    case CON_QSPAWNVAR:
    case CON_EQSPAWNVAR:
    case CON_OPERATERESPAWNS:
    case CON_OPERATEMASTERSWITCHES:
    case CON_CHECKACTIVATORMOTION:
    case CON_TIME:
    case CON_INITTIMER:
    case CON_LOCKPLAYER:
    case CON_SHOOTVAR:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_SOUNDVAR:
    case CON_GLOBALSOUNDVAR:
    case CON_STOPSOUNDVAR:
    case CON_SOUNDONCEVAR:
    case CON_ANGOFFVAR:
    case CON_CHECKAVAILWEAPON:
    case CON_CHECKAVAILINVEN:
    case CON_GUNIQHUDID:
    case CON_SAVEGAMEVAR:
    case CON_READGAMEVAR:
        transvar();
        return 0;

    case CON_ENHANCED:
        {
            // don't store in pCode...
            scriptptr--;
            //printf("We are enhanced, baby...\n");
            transnum(LABEL_DEFINE);
            scriptptr--;
            if(*scriptptr > BYTEVERSION_JF)
            {
                warning++;
                initprintf("%s:%ld: warning: need build %ld, found build %ld\n",compilefile,line_number,k,BYTEVERSION_JF);
            }
            break;
        }

    case CON_DYNAMICREMAP:
        {
            scriptptr--;
            initprintf("Dynamic tile remapping enabled.\n");
            dynamicremap = 1;
            break;
        }

    case CON_RANDVAR:
    case CON_ZSHOOT:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_SETVAR:
    case CON_ADDVAR:
    case CON_SUBVAR:
    case CON_DISPLAYRANDVAR:
    case CON_MULVAR:
    case CON_DIVVAR:
    case CON_MODVAR:
    case CON_ANDVAR:
    case CON_ORVAR:
    case CON_XORVAR:
    case CON_SHIFTVARL:
    case CON_SHIFTVARR:

        // syntax: [rand|add|set]var    <var1> <const1>
        // sets var1 to const1
        // adds const1 to var1 (const1 can be negative...)
        //printf("Found [add|set]var at line= %ld\n",line_number);

        // get the ID of the DEF
        if(tw != CON_ZSHOOT)
            transvartype(GAMEVAR_FLAG_READONLY);
        else transvar();

        transnum(LABEL_DEFINE); // the number to check against...
        return 0;
    case CON_RANDVARVAR:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_SETVARVAR:
    case CON_ADDVARVAR:
    case CON_SUBVARVAR:
    case CON_MULVARVAR:
    case CON_DIVVARVAR:
    case CON_MODVARVAR:
    case CON_ANDVARVAR:
    case CON_ORVARVAR:
    case CON_XORVARVAR:
    case CON_DISPLAYRANDVARVAR:
    case CON_SIN:
    case CON_COS:
        transvartype(GAMEVAR_FLAG_READONLY);
        transvar();
        return 0;

    case CON_SMAXAMMO:
    case CON_ADDWEAPONVAR:
    case CON_ACTIVATEBYSECTOR:
    case CON_OPERATESECTORS:
    case CON_OPERATEACTIVATORS:
    case CON_SSP:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GMAXAMMO:
    case CON_DIST:
    case CON_LDIST:
    case CON_TXDIST:
    case CON_GETANGLE:
    case CON_MULSCALE:
    case CON_SETASPECT:
        // get the ID of the DEF
        switch(tw)
        {
        case CON_DIST:
        case CON_LDIST:
        case CON_TXDIST:
        case CON_GETANGLE:
            transvartype(GAMEVAR_FLAG_READONLY);
            break;
        default:
            transvar();
            break;
        }

        // get the ID of the DEF
        if (tw == CON_GMAXAMMO)
            transvartype(GAMEVAR_FLAG_READONLY);
        else transvar();

        switch(tw)
        {
        case CON_DIST:
        case CON_LDIST:
        case CON_TXDIST:
        case CON_GETANGLE:
            transvartype(GAMEVAR_FLAG_READONLY);
            break;
        case CON_MULSCALE:
            transmultvars(2);
            break;
        }
        return 0;

    case CON_FLASH:
        return 0;

    case CON_DRAGPOINT:
        transmultvars(3);
        return 0;

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        transmultvars(3);
        transvartype(GAMEVAR_FLAG_READONLY);
        return 0;

    case CON_DEFINEPROJECTILE:
        {
            short y;
            signed long z;

            if( parsing_state || parsing_actor )
            {
                ReportError(ERROR_FOUNDWITHIN);
                error++;
            }

            scriptptr--;

            transnum(LABEL_DEFINE);
            j = *(scriptptr-1);

            if(j > MAXTILES-1)
            {
                ReportError(ERROR_EXCEEDSMAXTILES);
                error++;
            }

            transnum(LABEL_DEFINE);
            y = *(scriptptr-1);
            transnum(LABEL_DEFINE);
            z = *(scriptptr-1);

            DefineProjectile(j,y,z);
            spriteflags[j] |= SPRITE_FLAG_PROJECTILE;
            return 0;
        }

    case CON_SPRITEFLAGS:
        {
            if( parsing_actor == 0 && parsing_state == 0 )
            {
                scriptptr--;

                transnum(LABEL_DEFINE);
                scriptptr--;
                j = *scriptptr;

                if(j > MAXTILES-1)
                {
                    ReportError(ERROR_EXCEEDSMAXTILES);
                    error++;
                }

                transnum(LABEL_DEFINE);
                scriptptr--;
                spriteflags[j] = *scriptptr;

                return 0;
            }
            if(!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transvar();
            return 0;
        }

    case CON_SPRITESHADOW:
    case CON_SPRITENVG:
    case CON_SPRITENOSHADE:
    case CON_PRECACHE:
        {
            if( parsing_state || parsing_actor )
            {
                ReportError(ERROR_FOUNDWITHIN);
                error++;
            }

            scriptptr--;

            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;

            if(j > MAXTILES-1)
            {
                ReportError(ERROR_EXCEEDSMAXTILES);
                error++;
            }

            switch (tw)
            {
            case CON_SPRITESHADOW:
                spriteflags[*scriptptr] |= SPRITE_FLAG_SHADOW;
                break;
            case CON_SPRITENVG:
                spriteflags[*scriptptr] |= SPRITE_FLAG_NVG;
                break;
            case CON_SPRITENOSHADE:
                spriteflags[*scriptptr] |= SPRITE_FLAG_NOSHADE;
                break;
            case CON_PRECACHE:
                spritecache[*scriptptr][0] = j;
                transnum(LABEL_DEFINE);
                scriptptr--;
                i = *scriptptr;
                if(i > MAXTILES-1)
                {
                    ReportError(ERROR_EXCEEDSMAXTILES);
                    error++;
                }
                spritecache[j][1] = i;
                transnum(LABEL_DEFINE);
                scriptptr--;
                i = *scriptptr;
                spritecache[j][2] = i;
                break;
            }
            return 0;
        }

    case CON_IFVARVARG:
    case CON_IFVARVARL:
    case CON_IFVARVARE:
    case CON_IFVARVARN:
    case CON_IFVARVARAND:
    case CON_WHILEVARVARN:
        transmultvars(2);
        tempscrptr = scriptptr;
        scriptptr++; // Leave a spot for the fail location

        j = keyword();
        parsecommand();

        *tempscrptr = (long) scriptptr;

        if(tw != CON_WHILEVARVARN) checking_ifelse++;
        return 0;

    case CON_SPGETLOTAG:
    case CON_SPGETHITAG:
    case CON_SECTGETLOTAG:
    case CON_SECTGETHITAG:
    case CON_GETTEXTUREFLOOR:
    case CON_GETTEXTURECEILING:
        // no paramaters...
        return 0;

    case CON_STARTTRACK:
        // one parameter (track#)
        transnum(LABEL_DEFINE);
        return 0;

    case CON_IFVARL:
    case CON_IFVARG:
    case CON_IFVARE:
    case CON_IFVARN:
    case CON_IFVARAND:
    case CON_WHILEVARN:

        // get the ID of the DEF
        transvar();
        transnum(LABEL_DEFINE); // the number to check against...

        tempscrptr = scriptptr;
        scriptptr++; //Leave a spot for the fail location

        j = keyword();
        parsecommand();

        *tempscrptr = (long) scriptptr;

        if(tw != CON_WHILEVARN) checking_ifelse++;
        return 0;

    case CON_ADDLOGVAR:

        // syntax: addlogvar <var>

        // prints the line number in the log file.
        *scriptptr=line_number;
        scriptptr++;

        // get the ID of the DEF
        transvar();
        return 0;

    case CON_ROTATESPRITE:
        if( parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        // syntax:
        // long x, long y, long z, short a, short tilenum, signed char shade, char orientation, x1, y1, x2, y2
        // myospal adds char pal

        // get the ID of the DEFs

        transmultvars(12);
        break;

    case CON_GETZRANGE:
        transmultvars(4);
        transmultvarstype(GAMEVAR_FLAG_READONLY,4);
        transmultvars(2);
        break;

    case CON_HITSCAN:
    case CON_CANSEE:
        // get the ID of the DEF
        transmultvars(tw==CON_CANSEE?8:7);
        transmultvarstype(GAMEVAR_FLAG_READONLY,tw==CON_CANSEE?1:6);
        transvar();
        break;

    case CON_CANSEESPR:
        transmultvars(2);
        transvartype(GAMEVAR_FLAG_READONLY);
        break;

    case CON_ROTATEPOINT:
    case CON_NEARTAG:
        transmultvars(5);
        transmultvarstype(GAMEVAR_FLAG_READONLY,2);
        if (tw == CON_NEARTAG)
        {
            transmultvarstype(GAMEVAR_FLAG_READONLY,2);
            transmultvars(2);
        }
        break;

    case CON_MOVESPRITE:
    case CON_SETSPRITE:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transmultvars(4);
        if (tw == CON_MOVESPRITE) {
            transvar();
            transvartype(GAMEVAR_FLAG_READONLY);
        }
        break;

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_DIGITALNUMBER:
        if( parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        switch(tw)
        {
        case CON_GAMETEXT:
        case CON_DIGITALNUMBER:
            transmultvars(6);
        default:
            transmultvars(5);
            break;
        }
        break;

    case CON_UPDATESECTOR:
    case CON_UPDATESECTORZ:
        transmultvars(2);
        if (tw==CON_UPDATESECTORZ)
            transvar();
        transvartype(GAMEVAR_FLAG_READONLY);
        break;

    case CON_MYOS:
    case CON_MYOSPAL:
    case CON_MYOSX:
    case CON_MYOSPALX:
        if( parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        // syntax:
        // long x, long y, short tilenum, signed char shade, char orientation
        // myospal adds char pal

        transmultvars(5);
        if(tw==CON_MYOSPAL || tw==CON_MYOSPALX)
        {
            // Parse: pal

            // get the ID of the DEF
            transvar();
        }
        break;

    case CON_FINDPLAYER:
    case CON_FINDOTHERPLAYER:
    case CON_DISPLAYRAND:

        // syntax: displayrand <var>
        // gets rand (not game rand) into <var>

        // Get The ID of the DEF
        transvartype(GAMEVAR_FLAG_READONLY);
        break;

    case CON_SWITCH:
        //AddLog("Got Switch statement");
        if(checking_switch)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        checking_switch++; // allow nesting (if other things work)
        // Get The ID of the DEF
        transvar();

        tempscrptr= scriptptr;
        *scriptptr++=0; // leave spot for end location (for after processing)
        *scriptptr++=0; // count of case statements
        casescriptptr=scriptptr;        // the first case's pointer.

        *scriptptr++=0; // leave spot for 'default' location (null if none)

        j = keyword();
        temptextptr=textptr;
        // probably does not allow nesting...

        //AddLog("Counting Case Statements...");

        j=CountCaseStatements();
        //Bsprintf(g_szBuf,"Done Counting Case Statements: found %d.", j);
        //AddLog(g_szBuf);
        if(checking_switch>1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        if( j<0 )
        {
            return 1;
        }
        if (tempscrptr)
        {
            tempscrptr[1]=(long)j;  // save count of cases
        }
        else
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
        }

        while(j--)
        {
            // leave room for statements
            *scriptptr++=0; // value check
            *scriptptr++=0; // code offset
        }

        //Bsprintf(g_szBuf,"SWITCH1: '%.22s'",textptr);
        //AddLog(g_szBuf);

        casecount=0;

        while ( parsecommand() == 0 )
        {
            //Bsprintf(g_szBuf,"SWITCH2: '%.22s'",textptr);
            //AddLog(g_szBuf);
        }
        //Bsprintf(g_szBuf,"SWITCHXX: '%.22s'",textptr);
        //AddLog(g_szBuf);
        // done processing switch.  clean up.
        if(checking_switch!=1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=%d",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        casecount=0;
        if (tempscrptr)
        {
            tempscrptr[0]= (long)scriptptr - (long)&script[0];    // save 'end' location
        }
        else
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
        }
        casescriptptr=NULL;
        // decremented in endswitch.  Don't decrement here...
        //                    checking_switch--; // allow nesting (maybe if other things work)
        tempscrptr=NULL;
        if(checking_switch)
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=%d",__FILE__,__LINE__, checking_switch);
            //AddLog(g_szBuf);
        }
        //AddLog("End of Switch statement");
        break;

    case CON_CASE:
        //AddLog("Found Case");
repeatcase:
        scriptptr--; // don't save in code
        if(checking_switch<1)
        {
            error++;
            ReportError(-1);
            initprintf("%s:%ld: error: found `case' statement when not in switch\n",compilefile,line_number);
            return 1;
        }
        casecount++;
        //Bsprintf(g_szBuf,"case1: %.12s",textptr);
        //AddLog(g_szBuf);
        transnum(LABEL_DEFINE);
        if(*textptr == ':')
            textptr++;
        //Bsprintf(g_szBuf,"case2: %.12s",textptr);
        //AddLog(g_szBuf);

        j=*(--scriptptr);      // get value
        //Bsprintf(g_szBuf,"case: Value of case %ld is %ld",(long)casecount,(long)j);
        //AddLog(g_szBuf);
        if( casescriptptr)
        {
            //AddLog("Adding value to script");
            casescriptptr[casecount++]=j;   // save value
            casescriptptr[casecount]=(long)((long*)scriptptr-&script[0]);   // save offset
        }
        //      j = keyword();
        //Bsprintf(g_szBuf,"case3: %.12s",textptr);
        //AddLog(g_szBuf);

        j = keyword();
        if (j == CON_CASE)
        {
            //AddLog("Found Repeat Case");
            transword();    // eat 'case'
            goto repeatcase;
        }
        //Bsprintf(g_szBuf,"case4: '%.12s'",textptr);
        //AddLog(g_szBuf);
        while(parsecommand() == 0)
        {
            //Bsprintf(g_szBuf,"case5 '%.25s'",textptr);
            //AddLog(g_szBuf);
            j = keyword();
            if (j == CON_CASE)
            {
                //AddLog("Found Repeat Case");
                transword();    // eat 'case'
                goto repeatcase;
            }
        }
        //AddLog("End Case");
        return 0;
        //      break;
    case CON_DEFAULT:
        scriptptr--;    // don't save
        if(checking_switch<1)
        {
            error++;
            ReportError(-1);
            initprintf("%s:%ld: error: found `default' statement when not in switch\n",compilefile,line_number);
            return 1;
        }
        if(casescriptptr && casescriptptr[0]!=0)
        {
            // duplicate default statement
            error++;
            ReportError(-1);
            initprintf("%s:%ld: error: multiple `default' statements found in switch\n",compilefile,line_number);
        }
        if (casescriptptr)
        {
            casescriptptr[0]=(long)(scriptptr-&script[0]);   // save offset
        }
        //Bsprintf(g_szBuf,"default: '%.22s'",textptr);
        //AddLog(g_szBuf);
        while( parsecommand() == 0)
        {
            //Bsprintf(g_szBuf,"defaultParse: '%.22s'",textptr);
            //AddLog(g_szBuf);
            ;
        }
        break;

    case CON_ENDSWITCH:
        //AddLog("End Switch");
        checking_switch--;
        if(checking_switch < 0 )
        {
            error++;
            ReportError(-1);
            initprintf("%s:%ld: error: found `endswitch' without matching `switch'\n",compilefile,line_number);
        }
        if (casescriptptr)
        {
            int i;

            //Bsprintf(g_szBuf,"Default Offset is %ld\n Total of %ld cases",casescriptptr[0],(long)casecount/2);
            //AddLog(g_szBuf);
            for(i=1;i<=casecount;i++)
            {
                if (i & 1)
                {
                    //Bsprintf(g_szBuf,"Case Value %d is %ld",i/2+1,casescriptptr[i]);
                    //AddLog(g_szBuf);
                }
                else
                {
                    //Bsprintf(g_szBuf,"Offset %d is %ld",i/2+1,casescriptptr[i]);
                    //AddLog(g_szBuf);
                }
            }
        }
        else
        {
            //AddLog("Not saving case value: just counting");
        }
        return 1;      // end of block
        break;

    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETPNAME:
    case CON_STARTLEVEL:
    case CON_QSTRCAT:
    case CON_QSTRCPY:
        transmultvars(2);
        return 0;
    case CON_SETACTORANGLE:
    case CON_SETPLAYERANGLE:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETANGLETOTARGET:
    case CON_GETACTORANGLE:
    case CON_GETPLAYERANGLE:
        // Syntax:   <command> <var>

        // get the ID of the DEF
        transvar();
        return 0;

    case CON_ADDLOG:
        // syntax: addlog

        // prints the line number in the log file.
        *scriptptr=line_number;
        scriptptr++;
        return 0;

    case CON_IFPINVENTORY:
    case CON_IFRND:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_IFPDISTL:
    case CON_IFPDISTG:
    case CON_IFWASWEAPON:
    case CON_IFACTIONCOUNT:
    case CON_IFCOUNT:
    case CON_IFACTOR:
    case CON_IFSTRENGTH:
    case CON_IFSPAWNEDBY:
    case CON_IFGAPZL:
    case CON_IFFLOORDISTL:
    case CON_IFCEILINGDISTL:
        //        case 74:
    case CON_IFPHEALTHL:
    case CON_IFSPRITEPAL:
    case CON_IFGOTWEAPONCE:
    case CON_IFANGDIFFL:
    case CON_IFSOUND:
    case CON_IFAI:
    case CON_IFACTION:
    case CON_IFMOVE:
        switch(tw)
        {
        case CON_IFAI:
            transnum(LABEL_AI);
            break;
        case CON_IFACTION:
            transnum(LABEL_ACTION);
            break;
        case CON_IFMOVE:
            transnum(LABEL_MOVE);
            break;
        case CON_IFPINVENTORY:
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            break;
        default:
            transnum(LABEL_DEFINE);
            break;
        }

    case CON_IFONWATER:
    case CON_IFINWATER:
    case CON_IFACTORNOTSTAYPUT:
    case CON_IFCANSEE:
    case CON_IFHITWEAPON:
    case CON_IFSQUISHED:
    case CON_IFDEAD:
    case CON_IFCANSHOOTTARGET:
    case CON_IFP:
    case CON_IFHITSPACE:
    case CON_IFOUTSIDE:
    case CON_IFMULTIPLAYER:
    case CON_IFINSPACE:
    case CON_IFBULLETNEAR:
    case CON_IFRESPAWN:
    case CON_IFINOUTERSPACE:
    case CON_IFNOTMOVING:
    case CON_IFAWAYFROMWALL:
    case CON_IFCANSEETARGET:
    case CON_IFNOSOUNDS:
        if(tw == CON_IFP)
        {
            j = 0;
            do
            {
                transnum(LABEL_DEFINE);
                scriptptr--;
                j |= *scriptptr;
            }
            while(keyword() == -1);
            *scriptptr = j;
            scriptptr++;
        }

        tempscrptr = scriptptr;
        scriptptr++; //Leave a spot for the fail location

        j = keyword();
        parsecommand();

        *tempscrptr = (long) scriptptr;

        checking_ifelse++;
        return 0;

    case CON_LEFTBRACE:
        if(!(parsing_state || parsing_actor || parsing_event))
        {
            error++;
            ReportError(ERROR_SYNTAXERROR);
        }
        num_braces++;
        do
            done = parsecommand();
        while( done == 0 );
        return 0;

    case CON_RIGHTBRACE:
        num_braces--;
        if( num_braces < 0 )
        {
            if(checking_switch)
            {
                ReportError(ERROR_NOENDSWITCH);
            }

            ReportError(-1);
            initprintf("%s:%ld: error: found more `}' than `{'.\n",compilefile,line_number);
            error++;
        }
        return 1;

    case CON_BETANAME:
        scriptptr--;
        j = 0;
        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
            textptr++;
        return 0;

    case CON_DEFINEVOLUMENAME:
        scriptptr--;
        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;
        while( *textptr == ' ' ) textptr++;

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%ld: error: volume number exceeds maximum volume count.\n",compilefile,line_number);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }

        i = 0;

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            volume_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if(i >= (signed)sizeof(volume_names[j])-1)
            {
                initprintf("%s:%ld: error: volume name exceeds limit of %ld characters.\n",compilefile,line_number,sizeof(volume_names[j])-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        num_volumes = j+1;
        volume_names[j][i] = '\0';
        return 0;

    case CON_DEFINEGAMEFUNCNAME:
        scriptptr--;
        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;
        while( *textptr == ' ' ) textptr++;

        if (j < 0 || j > NUMGAMEFUNCTIONS-1)
        {
            initprintf("%s:%ld: error: function number exceeds number of game functions.\n",compilefile,line_number);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }

        i = 0;

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            gamefunctions[j][i] = *textptr;
            keydefaults[j*3][i] = *textptr;
            textptr++,i++;
            if(i >= MAXGAMEFUNCLEN-1)
            {
                initprintf("%s:%ld: error: function name exceeds limit of %ld characters.\n",compilefile,line_number,MAXGAMEFUNCLEN);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        gamefunctions[j][i] = '\0';
        keydefaults[j*3][i] = '\0';
        return 0;

    case CON_DEFINESKILLNAME:
        scriptptr--;
        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;
        while( *textptr == ' ' ) textptr++;

        if (j < 0 || j > 4)
        {
            initprintf("%s:%ld: error: skill number exceeds maximum skill count.\n",compilefile,line_number);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }

        i = 0;

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            skill_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if(i >= (signed)sizeof(skill_names[j])-1)
            {
                initprintf("%s:%ld: error: skill name exceeds limit of %ld characters.\n",compilefile,line_number,sizeof(skill_names[j])-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        skill_names[j][i] = '\0';
        return 0;

    case CON_DEFINEGAMETYPE:
        scriptptr--; //remove opcode from compiled code
        transnum(LABEL_DEFINE); //translate number
        scriptptr--; //remove it from compiled code
        j = *scriptptr; //put it into j

        transnum(LABEL_DEFINE); //translate number
        scriptptr--; //remove it from compiled code
        gametype_flags[j] = *scriptptr; //put it into the flags

        while( *textptr == ' ' ) textptr++;

        if (j < 0 || j > MAXGAMETYPES-1)
        {
            initprintf("%s:%ld: error: gametype number exceeds maximum gametype count.\n",compilefile,line_number);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }
        num_gametypes = j+1;

        i = 0;

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            gametype_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if(i >= (signed)sizeof(gametype_names[j])-1)
            {
                initprintf("%s:%ld: error: gametype name exceeds limit of %ld characters.\n",compilefile,line_number,sizeof(gametype_names[j])-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        gametype_names[j][i] = '\0';
        return 0;

    case CON_DEFINELEVELNAME:
        scriptptr--;
        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;
        transnum(LABEL_DEFINE);
        scriptptr--;
        k = *scriptptr;
        while( *textptr == ' ' ) textptr++;

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%ld: error: volume number exceeds maximum volume count.\n",compilefile,line_number);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }
        if (k < 0 || k > 10)
        {
            initprintf("%s:%ld: error: level number exceeds maximum number of levels per episode.\n",
                       line_number,compilefile);
            error++;
            while( *textptr != 0x0a && *textptr != 0 ) textptr++;
            break;
        }

        i = 0;
        while( *textptr != ' ' && *textptr != 0x0a )
        {
            level_file_names[j*11+k][i] = *textptr;
            textptr++,i++;
            if(i >= BMAX_PATH)
            {
                initprintf("%s:%ld: error: level file name exceeds limit of %d characters.\n",compilefile,line_number,BMAX_PATH);
                error++;
                while( *textptr != ' ') textptr++;
                break;
            }
        }
        level_names[j*11+k][i] = '\0';

        while( *textptr == ' ' ) textptr++;

        partime[j*11+k] =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

        textptr += 5;
        while( *textptr == ' ' ) textptr++;

        designertime[j*11+k] =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

        textptr += 5;
        while( *textptr == ' ' ) textptr++;

        i = 0;

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            level_names[j*11+k][i] = toupper(*textptr);
            textptr++,i++;
            if(i >= (signed)sizeof(level_names[j*11+k])-1)
            {
                initprintf("%s:%ld: error: level name exceeds limit of %ld characters.\n",compilefile,line_number,sizeof(level_names[j*11+k])-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        level_names[j*11+k][i] = '\0';
        return 0;

    case CON_DEFINEQUOTE:
    case CON_REDEFINEQUOTE:
        if (tw == CON_DEFINEQUOTE)
            scriptptr--;

        transnum(LABEL_DEFINE);

        k = *(scriptptr-1);

        if(k >= MAXQUOTES)
        {
            initprintf("%s:%ld: error: quote number exceeds limit of %ld.\n",compilefile,line_number,MAXQUOTES);
            error++;
        }

        if(fta_quotes[k] == NULL)
            fta_quotes[k] = Bcalloc(MAXQUOTELEN,sizeof(char));
        if (!fta_quotes[k])
        {
            fta_quotes[k] = NULL;
            Bsprintf(tempbuf,"Failed allocating %d byte quote text buffer.",sizeof(char) * MAXQUOTELEN);
            gameexit(tempbuf);
        }

        if (tw == CON_DEFINEQUOTE)
            scriptptr--;

        i = 0;

        while( *textptr == ' ' )
            textptr++;

        if (tw == CON_REDEFINEQUOTE)
        {
            if(redefined_quotes[redefined_quote_count] == NULL)
                redefined_quotes[redefined_quote_count] = Bcalloc(MAXQUOTELEN,sizeof(char));
            if (!redefined_quotes[redefined_quote_count])
            {
                redefined_quotes[redefined_quote_count] = NULL;
                Bsprintf(tempbuf,"Failed allocating %d byte quote text buffer.",sizeof(char) * MAXQUOTELEN);
                gameexit(tempbuf);
            }
        }

        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 )
        {
            if(*textptr == '%' && *(textptr+1) == 's')
            {
                initprintf("%s:%ld: error: quote text contains string identifier.\n",compilefile,line_number);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
            if (tw == CON_DEFINEQUOTE)
                *(fta_quotes[k]+i) = *textptr;
            else
                *(redefined_quotes[redefined_quote_count]+i) = *textptr;
            textptr++,i++;
            if(i >= MAXQUOTELEN-1)
            {
                initprintf("%s:%ld: error: quote text exceeds limit of %ld characters.\n",compilefile,line_number,MAXQUOTELEN-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 ) textptr++;
                break;
            }
        }
        if (tw == CON_DEFINEQUOTE)
            *(fta_quotes[k]+i) = '\0';
        else
        {
            *(fta_quotes[redefined_quote_count]+i) = '\0';
            *scriptptr++=redefined_quote_count;
            redefined_quote_count++;
        }
        return 0;

    case CON_CHEATKEYS:
        scriptptr--;
        transnum(LABEL_DEFINE);
        cheatkey[0] = *(scriptptr-1);
        transnum(LABEL_DEFINE);
        cheatkey[1] = *(scriptptr-1);
        return 0;

    case CON_DEFINECHEAT:
        scriptptr--;
        transnum(LABEL_DEFINE);
        k = *(scriptptr-1);

        if(k > 25)
        {
            initprintf("%s:%ld: error: cheat redefinition attempts to redefine nonexistant cheat.\n",compilefile,line_number);
            error++;
        }
        scriptptr--;
        i = 0;
        while( *textptr == ' ' )
            textptr++;
        while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ')
        {
            cheatquotes[k][i] = *textptr;
            textptr++,i++;
            if(i >= (signed)sizeof(cheatquotes[k])-1)
            {
                initprintf("%s:%ld: error: cheat exceeds limit of %ld characters.\n",compilefile,line_number,MAXCHEATLEN,sizeof(cheatquotes[k])-1);
                error++;
                while( *textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ') textptr++;
                break;
            }
        }
        cheatquotes[k][i] = '\0';
        return 0;

    case CON_DEFINESOUND:
        scriptptr--;
        transnum(LABEL_DEFINE);
        k = *(scriptptr-1);
        if(k >= NUM_SOUNDS)
        {
            initprintf("%s:%ld: error: exceeded sound limit of %ld.\n",compilefile,line_number,NUM_SOUNDS);
            error++;
        }
        scriptptr--;
        i = 0;
        skipcomments();

        while( *textptr != ' ' )
        {
            sounds[k][i] = *textptr;
            textptr++,i++;
            if(i >= BMAX_PATH)
            {
                initprintf("%s:%ld: error: sound filename exceeds limit of %d characters.\n",compilefile,line_number,BMAX_PATH);
                error++;
                skipcomments();
                break;
            }
        }
        sounds[k][i] = '\0';

        transnum(LABEL_DEFINE);
        soundps[k] = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        soundpe[k] = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        soundpr[k] = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        soundm[k] = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        soundvo[k] = *(scriptptr-1);
        scriptptr--;
        return 0;

    case CON_ENDEVENT:

        if( parsing_event == 0)
        {
            ReportError(-1);
            initprintf("%s:%ld: error: found `endevent' without open `onevent'.\n",compilefile,line_number);
            error++;
        }
        if( num_braces > 0 )
        {
            ReportError(ERROR_OPENBRACKET);
            error++;
        }
        if( num_braces < 0 )
        {
            ReportError(ERROR_CLOSEBRACKET);
            error++;
        }
        parsing_event = 0;
        parsing_actor = 0;
        current_event = -1;
        Bsprintf(parsing_item_name,"(none)");
        return 0;

    case CON_ENDA:
        if( parsing_actor == 0 )
        {
            ReportError(-1);
            initprintf("%s:%ld: error: found `enda' without open `actor'.\n",compilefile,line_number);
            error++;
        }
        if( num_braces > 0 )
        {
            ReportError(ERROR_OPENBRACKET);
            error++;
        }
        if( num_braces < 0 )
        {
            ReportError(ERROR_CLOSEBRACKET);
            error++;
        }
        parsing_actor = 0;
        Bsprintf(parsing_item_name,"(none)");
        return 0;

    case CON_BREAK:
        if(checking_switch)
        {
            //Bsprintf(g_szBuf,"  * (L%ld) case Break statement.\n",line_number);
            //AddLog(g_szBuf);
            return 1;
        }
        return 0;

    case CON_FALL:
    case CON_TIP:
        //        case 21:
    case CON_KILLIT:
    case CON_RESETACTIONCOUNT:
    case CON_PSTOMP:
    case CON_RESETPLAYER:
    case CON_RESETCOUNT:
    case CON_WACKPLAYER:
    case CON_OPERATE:
    case CON_RESPAWNHITAG:
    case CON_GETLASTPAL:
    case CON_PKICK:
    case CON_MIKESND:
    case CON_TOSSWEAPON:
        if(!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_NULLOP:
    case CON_STOPALLSOUNDS:
        return 0;
    case CON_GAMESTARTUP:
        {
            long params[30];

            scriptptr--;
            for(j = 0; j < 30; j++)
            {
                transnum(LABEL_DEFINE);
                scriptptr--;
                params[j] = *scriptptr;

                if (j != 25) continue;

                if (keyword() != -1) {
                    initprintf("Version 1.3D CON files detected.\n");
                    break;
                } else {
                    conversion = 14;
                    initprintf("Version 1.4+ CON files detected.\n");
                }

            }

            /*
            v1.3d                   v1.5
            DEFAULTVISIBILITY       DEFAULTVISIBILITY
            GENERICIMPACTDAMAGE     GENERICIMPACTDAMAGE
            MAXPLAYERHEALTH         MAXPLAYERHEALTH
            STARTARMORHEALTH        STARTARMORHEALTH
            RESPAWNACTORTIME        RESPAWNACTORTIME
            RESPAWNITEMTIME         RESPAWNITEMTIME
            RUNNINGSPEED            RUNNINGSPEED
            RPGBLASTRADIUS          GRAVITATIONALCONSTANT
            PIPEBOMBRADIUS          RPGBLASTRADIUS
            SHRINKERBLASTRADIUS     PIPEBOMBRADIUS
            TRIPBOMBBLASTRADIUS     SHRINKERBLASTRADIUS
            MORTERBLASTRADIUS       TRIPBOMBBLASTRADIUS
            BOUNCEMINEBLASTRADIUS   MORTERBLASTRADIUS
            SEENINEBLASTRADIUS      BOUNCEMINEBLASTRADIUS
            MAXPISTOLAMMO           SEENINEBLASTRADIUS
            MAXSHOTGUNAMMO          MAXPISTOLAMMO
            MAXCHAINGUNAMMO         MAXSHOTGUNAMMO
            MAXRPGAMMO              MAXCHAINGUNAMMO
            MAXHANDBOMBAMMO         MAXRPGAMMO
            MAXSHRINKERAMMO         MAXHANDBOMBAMMO
            MAXDEVISTATORAMMO       MAXSHRINKERAMMO
            MAXTRIPBOMBAMMO         MAXDEVISTATORAMMO
            MAXFREEZEAMMO           MAXTRIPBOMBAMMO
            CAMERASDESTRUCTABLE     MAXFREEZEAMMO
            NUMFREEZEBOUNCES        MAXGROWAMMO
            FREEZERHURTOWNER        CAMERASDESTRUCTABLE
                                    NUMFREEZEBOUNCES
                                    FREEZERHURTOWNER
                                    QSIZE
                                    TRIPBOMBLASERMODE
            */

            j = 0;
            ud.const_visibility = params[j++];
            impact_damage = params[j++];
            max_player_health = params[j++];
            max_armour_amount = params[j++];
            respawnactortime = params[j++];
            respawnitemtime = params[j++];
            dukefriction = params[j++];
            if (conversion == 14) gc = params[j++];
            rpgblastradius = params[j++];
            pipebombblastradius = params[j++];
            shrinkerblastradius = params[j++];
            tripbombblastradius = params[j++];
            morterblastradius = params[j++];
            bouncemineblastradius = params[j++];
            seenineblastradius = params[j++];
            max_ammo_amount[PISTOL_WEAPON] = params[j++];
            max_ammo_amount[SHOTGUN_WEAPON] = params[j++];
            max_ammo_amount[CHAINGUN_WEAPON] = params[j++];
            max_ammo_amount[RPG_WEAPON] = params[j++];
            max_ammo_amount[HANDBOMB_WEAPON] = params[j++];
            max_ammo_amount[SHRINKER_WEAPON] = params[j++];
            max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
            max_ammo_amount[TRIPBOMB_WEAPON] = params[j++];
            max_ammo_amount[FREEZE_WEAPON] = params[j++];
            if (conversion == 14) max_ammo_amount[GROW_WEAPON] = params[j++];
            camerashitable = params[j++];
            numfreezebounces = params[j++];
            freezerhurtowner = params[j++];
            if (conversion == 14) {
                spriteqamount = params[j++];
                if(spriteqamount > 1024) spriteqamount = 1024;
                else if(spriteqamount < 0) spriteqamount = 0;

                lasermode = params[j++];
            }
        }
        return 0;
    }
    return 0;
}

void passone(void)
{
#ifdef DEBUG
    int i;
#endif

    while( parsecommand() == 0 );

    if( (error+warning) > 63)
        initprintf(  "fatal error: too many warnings or errors: Aborted\n");

#ifdef DEBUG
    initprintf("Game Definitions\n");
    for(i=0;i<iGameVarCount;i++)
    {
        initprintf("%20s\t%d\n",apszGameVarLabel[i],lGameVarValue[i]);
    }
#endif
}

#define NUM_DEFAULT_CONS    4
char *defaultcons[NUM_DEFAULT_CONS] =
    {
        "EDUKE.CON",
        "GAME.CON",
        "USER.CON",
        "DEFS.CON"
    };

void copydefaultcons(void)
{
    long i, fs, fpi;
    FILE *fpo;

    for(i=0;i<NUM_DEFAULT_CONS;i++)
    {
        fpi = kopen4load( defaultcons[i] , 1 );
        if (fpi < 0) continue;

        fpo = fopenfrompath(defaultcons[i],"wb");

        if (fpo == NULL) {
            kclose(fpi);
            continue;
        }

        fs = kfilelength(fpi);

        kread(fpi,&hittype[0],fs);
        fwrite(&hittype[0],fs,1,fpo);

        kclose(fpi);
        fclose(fpo);
    }
}

void FreeGameVars(void)
{
    // call this function as many times as needed.
    int i;
    //  AddLog("FreeGameVars");
    for(i=0;i<MAXGAMEVARS;i++)
    {
        aGameVars[i].lValue=0;
        if(aGameVars[i].szLabel)
            Bfree(aGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;
        aGameVars[i].dwFlags=0;

        if(aGameVars[i].plValues)
            Bfree(aGameVars[i].plValues);
        aGameVars[i].plValues=NULL;
    }
    iGameVarCount=0;
    return;
}

void ClearGameVars(void)
{
    // only call this function ONCE...
    int i;

    //AddLog("ClearGameVars");

    for(i=0;i<MAXGAMEVARS;i++)
    {
        aGameVars[i].lValue=0;
        if(aGameVars[i].szLabel)
            Bfree(aGameVars[i].szLabel);
        if(aDefaultGameVars[i].szLabel)
            Bfree(aDefaultGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;
        aDefaultGameVars[i].szLabel=NULL;
        aGameVars[i].dwFlags=0;

        if(aGameVars[i].plValues)
            Bfree(aGameVars[i].plValues);
        aGameVars[i].plValues=NULL;
    }
    iGameVarCount=0;
    iDefaultGameVarCount=0;
    return;
}

void InitGameVarPointers(void)
{
    int i;
    char aszBuf[64];
    // called from game Init AND when level is loaded...

    //AddLog("InitGameVarPointers");

    for(i=0;i<MAX_WEAPONS;i++)
    {
        Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
        aplWeaponClip[i]=GetGameValuePtr(aszBuf);
        if(!aplWeaponClip[i])
        {
            initprintf("ERROR: NULL Weapon\n");
            exit(0);
        }
        Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
        aplWeaponReload[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
        aplWeaponFireDelay[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
        aplWeaponTotalTime[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
        aplWeaponHoldDelay[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
        aplWeaponFlags[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
        aplWeaponShoots[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
        aplWeaponSpawnTime[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
        aplWeaponSpawn[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
        aplWeaponShotsPerBurst[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
        aplWeaponWorksLike[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
        aplWeaponInitialSound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
        aplWeaponFireSound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
        aplWeaponSound2Time[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
        aplWeaponSound2Sound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
        aplWeaponReloadSound1[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
        aplWeaponReloadSound2[i]=GetGameValuePtr(aszBuf);
    }
}

void AddSystemVars()
{
    // only call ONCE
    char aszBuf[64];

    //AddLog("AddSystemVars");

    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",KNEE_WEAPON);
    AddGameVar(aszBuf, KNEE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",KNEE_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 14, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",KNEE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_NOVISIBLE | WEAPON_FLAG_RANDOMRESTART | WEAPON_FLAG_AUTOMATIC, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",KNEE_WEAPON);
    AddGameVar(aszBuf, KNEE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",KNEE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",KNEE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",PISTOL_WEAPON);
    AddGameVar(aszBuf, PISTOL_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",PISTOL_WEAPON);
    AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",PISTOL_WEAPON);
    AddGameVar(aszBuf, 27, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",PISTOL_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 6, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",PISTOL_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_RELOAD_TIMING, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",PISTOL_WEAPON);
    AddGameVar(aszBuf, SHOTSPARK1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",PISTOL_WEAPON);
    AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, PISTOL_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",PISTOL_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",PISTOL_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 13, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 31, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_CHECKATRELOAD, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 24, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUNSHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 15, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_COCK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_FIREEVERYTHIRD | WEAPON_FLAG_AMMOPERSHOT \
               | WEAPON_FLAG_SPAWNTYPE3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",RPG_WEAPON);
    AddGameVar(aszBuf, RPG_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",RPG_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",RPG_WEAPON);
    AddGameVar(aszBuf, 20, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",RPG_WEAPON);
    AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",RPG_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",RPG_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, HANDBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 6, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 19, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_THROWIT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, HEAVYHBOMB, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHRINKER_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHRINKER_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHRINKER_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, DEVISTATOR_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_FIREEVERYOTHER | WEAPON_FLAG_AMMOPERSHOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, TRIPBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 16, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 16, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_STANDSTILL | WEAPON_FLAG_CHECKATRELOAD, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, HANDHOLDINGLASER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",FREEZE_WEAPON);
    AddGameVar(aszBuf, FREEZE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",FREEZE_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 8, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",FREEZE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_FIREEVERYTHIRD | WEAPON_FLAG_AUTOMATIC, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",FREEZE_WEAPON);
    AddGameVar(aszBuf, FREEZEBLAST, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",FREEZE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",FREEZE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, HANDREMOTE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_BOMB_TRIGGER | WEAPON_FLAG_NOVISIBLE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    ///////////////////////////////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",GROW_WEAPON);
    AddGameVar(aszBuf, GROW_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",GROW_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",GROW_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",GROW_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",GROW_WEAPON);
    AddGameVar(aszBuf, GROWSPARK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",GROW_WEAPON);
    AddGameVar(aszBuf, EXPANDERSHOOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",GROW_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",GROW_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("RESPAWN_MONSTERS", (long)&ud.respawn_monsters,GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("RESPAWN_ITEMS",(long)&ud.respawn_items, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("RESPAWN_INVENTORY",(long)&ud.respawn_inventory, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MONSTERS_OFF",(long)&ud.monsters_off, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MARKER",(long)&ud.marker, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("FFIRE",(long)&ud.ffire, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("LEVEL",(long)&ud.level_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("VOLUME",(long)&ud.volume_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);

    AddGameVar("COOP",(long)&ud.coop, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MULTIMODE",(long)&ud.multimode, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);

    AddGameVar("WEAPON", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("WORKSLIKE", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("RETURN", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("ZRANGE", 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("ANGRANGE", 18, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("AUTOAIMANGLE", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("LOTAG", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("HITAG", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("TEXTURE", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("THISACTOR", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("myconnectindex", (long)&myconnectindex, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("screenpeek", (long)&screenpeek, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("currentweapon",(long)&g_currentweapon, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gs",(long)&g_gs, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_arc",(long)&g_looking_arc, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gun_pos",(long)&g_gun_pos, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weapon_xoffset",(long)&g_weapon_xoffset, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weaponcount",(long)&g_kb, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_angSR1",(long)&g_looking_angSR1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("xdim",(long)&xdim, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("ydim",(long)&ydim, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx1",(long)&windowx1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx2",(long)&windowx2, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy1",(long)&windowy1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy2",(long)&windowy2, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("totalclock",(long)&totalclock, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("lastvisinc",(long)&lastvisinc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("numsectors",(long)&numsectors, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("numplayers",(long)&numplayers, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("viewingrange",(long)&viewingrange, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("yxaspect",(long)&yxaspect, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gravitationalconstant",(long)&gc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("gametype_flags",(long)&gametype_flags[ud.coop], GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("CLIPMASK0", CLIPMASK0, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);
    AddGameVar("CLIPMASK1", CLIPMASK1, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);
}

/* Anything added with AddDefinition cannot be overwritten in the CONs */

void AddDefinition(char *lLabel,long lValue,long lType)
{
    Bstrcpy(label+(labelcnt<<6),lLabel);
    labeltype[labelcnt] = lType;
    labelcode[labelcnt++] = lValue;
    defaultlabelcnt++;
}

void AddDefaultDefinitions(void)
{
    AddDefinition("EVENT_AIMDOWN",EVENT_AIMDOWN,LABEL_DEFINE);
    AddDefinition("EVENT_AIMUP",EVENT_AIMUP,LABEL_DEFINE);
    AddDefinition("EVENT_CHANGEWEAPON",EVENT_CHANGEWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETBOOT",EVENT_CHEATGETBOOT,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETFIRSTAID",EVENT_CHEATGETFIRSTAID,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETHEAT",EVENT_CHEATGETHEAT,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETHOLODUKE",EVENT_CHEATGETHOLODUKE,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETJETPACK",EVENT_CHEATGETJETPACK,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETSCUBA",EVENT_CHEATGETSCUBA,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETSHIELD",EVENT_CHEATGETSHIELD,LABEL_DEFINE);
    AddDefinition("EVENT_CHEATGETSTEROIDS",EVENT_CHEATGETSTEROIDS,LABEL_DEFINE);
    AddDefinition("EVENT_CROUCH",EVENT_CROUCH,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYCROSSHAIR",EVENT_DISPLAYCROSSHAIR,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYREST",EVENT_DISPLAYREST,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYWEAPON",EVENT_DISPLAYWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_DOFIRE",EVENT_DOFIRE,LABEL_DEFINE);
    AddDefinition("EVENT_DRAWWEAPON",EVENT_DRAWWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_EGS",EVENT_EGS,LABEL_DEFINE);
    AddDefinition("EVENT_ENTERLEVEL",EVENT_ENTERLEVEL,LABEL_DEFINE);
    AddDefinition("EVENT_FIRE",EVENT_FIRE,LABEL_DEFINE);
    AddDefinition("EVENT_FIREWEAPON",EVENT_FIREWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_GAME",EVENT_GAME,LABEL_DEFINE);
    AddDefinition("EVENT_GETAUTOAIMANGLE",EVENT_GETAUTOAIMANGLE,LABEL_DEFINE);
    AddDefinition("EVENT_GETLOADTILE",EVENT_GETLOADTILE,LABEL_DEFINE);
    AddDefinition("EVENT_GETMENUTILE",EVENT_GETMENUTILE,LABEL_DEFINE);
    AddDefinition("EVENT_GETSHOTRANGE",EVENT_GETSHOTRANGE,LABEL_DEFINE);
    AddDefinition("EVENT_HOLODUKEOFF",EVENT_HOLODUKEOFF,LABEL_DEFINE);
    AddDefinition("EVENT_HOLODUKEON",EVENT_HOLODUKEON,LABEL_DEFINE);
    AddDefinition("EVENT_HOLSTER",EVENT_HOLSTER,LABEL_DEFINE);
    AddDefinition("EVENT_INCURDAMAGE",EVENT_INCURDAMAGE,LABEL_DEFINE);
    AddDefinition("EVENT_INIT",EVENT_INIT,LABEL_DEFINE);
    AddDefinition("EVENT_INVENTORY",EVENT_INVENTORY,LABEL_DEFINE);
    AddDefinition("EVENT_INVENTORYLEFT",EVENT_INVENTORYLEFT,LABEL_DEFINE);
    AddDefinition("EVENT_INVENTORYRIGHT",EVENT_INVENTORYRIGHT,LABEL_DEFINE);
    AddDefinition("EVENT_JUMP",EVENT_JUMP,LABEL_DEFINE);
    AddDefinition("EVENT_LOGO",EVENT_LOGO,LABEL_DEFINE);
    AddDefinition("EVENT_LOOKDOWN",EVENT_LOOKDOWN,LABEL_DEFINE);
    AddDefinition("EVENT_LOOKLEFT",EVENT_LOOKLEFT,LABEL_DEFINE);
    AddDefinition("EVENT_LOOKRIGHT",EVENT_LOOKRIGHT,LABEL_DEFINE);
    AddDefinition("EVENT_LOOKUP",EVENT_LOOKUP,LABEL_DEFINE);
    AddDefinition("EVENT_MOVEBACKWARD",EVENT_MOVEBACKWARD,LABEL_DEFINE);
    AddDefinition("EVENT_MOVEFORWARD",EVENT_MOVEFORWARD,LABEL_DEFINE);
    AddDefinition("EVENT_NEXTWEAPON",EVENT_NEXTWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_PREVIOUSWEAPON",EVENT_PREVIOUSWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_PRESSEDFIRE",EVENT_PRESSEDFIRE,LABEL_DEFINE);
    AddDefinition("EVENT_PROCESSINPUT",EVENT_PROCESSINPUT,LABEL_DEFINE);
    AddDefinition("EVENT_QUICKKICK",EVENT_QUICKKICK,LABEL_DEFINE);
    AddDefinition("EVENT_RESETINVENTORY",EVENT_RESETINVENTORY,LABEL_DEFINE);
    AddDefinition("EVENT_RESETPLAYER",EVENT_RESETPLAYER,LABEL_DEFINE);
    AddDefinition("EVENT_RESETWEAPONS",EVENT_RESETWEAPONS,LABEL_DEFINE);
    AddDefinition("EVENT_RETURNTOCENTER",EVENT_RETURNTOCENTER,LABEL_DEFINE);
    AddDefinition("EVENT_SELECTWEAPON",EVENT_SELECTWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_SOARDOWN",EVENT_SOARDOWN,LABEL_DEFINE);
    AddDefinition("EVENT_SOARUP",EVENT_SOARUP,LABEL_DEFINE);
    AddDefinition("EVENT_SPAWN",EVENT_SPAWN,LABEL_DEFINE);
    AddDefinition("EVENT_STRAFELEFT",EVENT_STRAFELEFT,LABEL_DEFINE);
    AddDefinition("EVENT_STRAFERIGHT",EVENT_STRAFERIGHT,LABEL_DEFINE);
    AddDefinition("EVENT_SWIMDOWN",EVENT_SWIMDOWN,LABEL_DEFINE);
    AddDefinition("EVENT_SWIMUP",EVENT_SWIMUP,LABEL_DEFINE);
    AddDefinition("EVENT_TURNAROUND",EVENT_TURNAROUND,LABEL_DEFINE);
    AddDefinition("EVENT_TURNLEFT",EVENT_TURNLEFT,LABEL_DEFINE);
    AddDefinition("EVENT_TURNRIGHT",EVENT_TURNRIGHT,LABEL_DEFINE);
    AddDefinition("EVENT_USE",EVENT_USE,LABEL_DEFINE);
    AddDefinition("EVENT_USEJETPACK",EVENT_USEJETPACK,LABEL_DEFINE);
    AddDefinition("EVENT_USEMEDKIT",EVENT_USEMEDKIT,LABEL_DEFINE);
    AddDefinition("EVENT_USENIGHTVISION",EVENT_USENIGHTVISION,LABEL_DEFINE);
    AddDefinition("EVENT_USESTEROIDS",EVENT_USESTEROIDS,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY10",EVENT_WEAPKEY10,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY1",EVENT_WEAPKEY1,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY2",EVENT_WEAPKEY2,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY3",EVENT_WEAPKEY3,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY4",EVENT_WEAPKEY4,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY5",EVENT_WEAPKEY5,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY6",EVENT_WEAPKEY6,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY7",EVENT_WEAPKEY7,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY8",EVENT_WEAPKEY8,LABEL_DEFINE);
    AddDefinition("EVENT_WEAPKEY9",EVENT_WEAPKEY9,LABEL_DEFINE);

    AddDefinition("NO",0,LABEL_DEFINE|LABEL_ACTION|LABEL_AI|LABEL_MOVE);

    AddDefinition("PROJ_BOUNCES",PROJ_BOUNCES,LABEL_DEFINE);
    AddDefinition("PROJ_BSOUND",PROJ_BSOUND,LABEL_DEFINE);
    AddDefinition("PROJ_CLIPDIST",PROJ_CLIPDIST,LABEL_DEFINE);
    AddDefinition("PROJ_CSTAT",PROJ_CSTAT,LABEL_DEFINE);
    AddDefinition("PROJ_DECAL",PROJ_DECAL,LABEL_DEFINE);
    AddDefinition("PROJ_DROP",PROJ_DROP,LABEL_DEFINE);
    AddDefinition("PROJ_EXTRA",PROJ_EXTRA,LABEL_DEFINE);
    AddDefinition("PROJ_EXTRA_RAND",PROJ_EXTRA_RAND,LABEL_DEFINE);
    AddDefinition("PROJ_HITRADIUS",PROJ_HITRADIUS,LABEL_DEFINE);
    AddDefinition("PROJ_ISOUND",PROJ_ISOUND,LABEL_DEFINE);
    AddDefinition("PROJ_OFFSET",PROJ_OFFSET,LABEL_DEFINE);
    AddDefinition("PROJ_PAL",PROJ_PAL,LABEL_DEFINE);
    AddDefinition("PROJ_RANGE",PROJ_RANGE,LABEL_DEFINE);
    AddDefinition("PROJ_SHADE",PROJ_SHADE,LABEL_DEFINE);
    AddDefinition("PROJ_SOUND",PROJ_SOUND,LABEL_DEFINE);
    AddDefinition("PROJ_SPAWNS",PROJ_SPAWNS,LABEL_DEFINE);
    AddDefinition("PROJ_SXREPEAT",PROJ_SXREPEAT,LABEL_DEFINE);
    AddDefinition("PROJ_SYREPEAT",PROJ_SYREPEAT,LABEL_DEFINE);
    AddDefinition("PROJ_TNUM",PROJ_TNUM,LABEL_DEFINE);
    AddDefinition("PROJ_TOFFSET",PROJ_TOFFSET,LABEL_DEFINE);
    AddDefinition("PROJ_TRAIL",PROJ_TRAIL,LABEL_DEFINE);
    AddDefinition("PROJ_TXREPEAT",PROJ_TXREPEAT,LABEL_DEFINE);
    AddDefinition("PROJ_TYREPEAT",PROJ_TYREPEAT,LABEL_DEFINE);
    AddDefinition("PROJ_VEL_MULT",PROJ_VEL_MULT,LABEL_DEFINE);
    AddDefinition("PROJ_VEL",PROJ_VEL,LABEL_DEFINE);
    AddDefinition("PROJ_WORKSLIKE",PROJ_WORKSLIKE,LABEL_DEFINE);
    AddDefinition("PROJ_XREPEAT",PROJ_XREPEAT,LABEL_DEFINE);
    AddDefinition("PROJ_YREPEAT",PROJ_YREPEAT,LABEL_DEFINE);
}

void InitProjectiles(void)
{
    int i;
    for(i=0;i<MAXTILES;i++) {
        projectile[i].workslike = 1;
        projectile[i].spawns = SMALLSMOKE;
        projectile[i].sxrepeat = projectile[i].syrepeat = -1;
        projectile[i].sound = projectile[i].isound = -1;
        projectile[i].vel = 600;
        projectile[i].extra = 100;
        projectile[i].decal = BULLETHOLE;
        projectile[i].trail = -1;
        projectile[i].tnum = projectile[i].toffset = 0;
        projectile[i].txrepeat = projectile[i].tyrepeat = -1;
        projectile[i].drop = projectile[i].range = 0;
        projectile[i].cstat = -1;
        projectile[i].shade = -96;
        projectile[i].xrepeat = projectile[i].yrepeat = 18;
        projectile[i].clipdist = 32;
        projectile[i].pal = 0;
        projectile[i].extra_rand = -1;
        projectile[i].hitradius = 2048;
        projectile[i].velmult = 1;
        projectile[i].offset = 448;
        projectile[i].bounces = numfreezebounces;
        projectile[i].bsound = PIPEBOMB_BOUNCE;
        //      defaultprojectile[i] = projectile[i];
    }
    Bmemcpy(&defaultprojectile, &projectile, sizeof(projectile));
}

void ResetSystemDefaults(void)
{
    // call many times...

    int i,j;
    char aszBuf[64];

    //AddLog("ResetWeaponDefaults");

    for(j=0;j<MAXPLAYERS;j++)
    {
        for(i=0;i<MAX_WEAPONS;i++)
        {
            Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
            aplWeaponClip[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
            aplWeaponReload[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
            aplWeaponFireDelay[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
            aplWeaponTotalTime[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
            aplWeaponHoldDelay[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
            aplWeaponFlags[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
            aplWeaponShoots[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
            aplWeaponSpawnTime[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
            aplWeaponSpawn[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
            aplWeaponShotsPerBurst[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
            aplWeaponWorksLike[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
            aplWeaponInitialSound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
            aplWeaponFireSound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
            aplWeaponSound2Time[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
            aplWeaponSound2Sound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
            aplWeaponReloadSound1[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
            aplWeaponReloadSound2[i][j]=GetGameVar(aszBuf,0, -1, j);
        }
    }

    g_iReturnVarID=GetGameID("RETURN");
    g_iWeaponVarID=GetGameID("WEAPON");
    g_iWorksLikeVarID=GetGameID("WORKSLIKE");
    g_iZRangeVarID=GetGameID("ZRANGE");
    g_iAngRangeVarID=GetGameID("ANGRANGE");
    g_iAimAngleVarID=GetGameID("AUTOAIMANGLE");
    g_iLoTagID=GetGameID("LOTAG");
    g_iHiTagID=GetGameID("HITAG");
    g_iTextureID=GetGameID("TEXTURE");
    g_iThisActorID=GetGameID("THISACTOR");

    //  for(i=0;i<MAXTILES;i++)
    //      projectile[i] = defaultprojectile[i];

    Bmemcpy(&projectile,&defaultprojectile,sizeof(defaultprojectile));

    //AddLog("EOF:ResetWeaponDefaults");
}

void InitGameVars(void)
{
    // only call ONCE

    //  initprintf("Initializing game variables\n");
    //AddLog("InitGameVars");

    ClearGameVars();
    AddSystemVars();
    InitGameVarPointers();
    ResetSystemDefaults();
    InitProjectiles();
}

void loadefs(char *filenam)
{
    char *mptr;
    int i;
    long fs,fp;

    clearbuf(apScriptGameEvent,MAXGAMEEVENTS,0L);

    InitGameVars();

    /* JBF 20040109: Don't prompt to extract CONs from GRP if they're missing.
     * If someone really wants them they can Kextract them.
    if(!SafeFileExists(filenam) && loadfromgrouponly == 0)
    {
        initprintf("Missing external CON file(s).\n");
        initprintf("COPY INTERNAL DEFAULTS TO DIRECTORY(Y/n)?\n");

    i=wm_ynbox("Missing CON file(s)", "Missing external CON file(s). "
    "Copy internal defaults to directory?");
    if (i) i = 'y';
        if(i == 'y' || i == 'Y' )
        {
            initprintf(" Yes\n");
            copydefaultcons();
        }
    }
    */
    fp = kopen4load(filenam,loadfromgrouponly);
    if( fp == -1 )  // JBF: was 0
    {
        if( loadfromgrouponly == 1 )
            gameexit("Missing CON file(s); replace duke3d.grp with a known good copy.");
        else {
            Bsprintf(tempbuf,"CON file `%s' missing.", filenam);
            gameexit(tempbuf);
            return;
        }

        //loadfromgrouponly = 1;
        return; //Not there
    }

    fs = kfilelength(fp);

    initprintf("Compiling: %s (%ld bytes)\n",filenam,fs);

    mptr = (char *)Bmalloc(fs+1);
    if (!mptr) {
        Bsprintf(tempbuf,"Failed allocating %ld byte CON text buffer.", fs+1);
        gameexit(tempbuf);
    }

    mptr[fs] = 0;

    textptr = (char *) mptr;
    kread(fp,(char *)textptr,fs);
    kclose(fp);

    clearbuf(actorscrptr,MAXTILES,0L);  // JBF 20040531: MAXSPRITES? I think Todd meant MAXTILES...
    clearbuf(actorLoadEventScrptr,MAXTILES,0L); // I think this should be here...
    clearbufbyte(actortype,MAXTILES,0L);
    clearbufbyte(script,sizeof(script),0l); // JBF 20040531: yes? no?

    labelcnt = defaultlabelcnt = 0;
    scriptptr = script+1;
    warning = 0;
    error = 0;
    line_number = 1;
    total_lines = 0;

    AddDefaultDefinitions();

    Bstrcpy(compilefile, filenam);   // JBF 20031130: Store currently compiling file name
    passone(); //Tokenize
    *script = (long) scriptptr;

    Bfree(mptr);
    mptr = NULL;

    if(warning|error)
        initprintf("Found %ld warning(s), %ld error(s).\n",warning,error);

    if( error == 0 && warning != 0)
    {
        if( groupfile != -1 && loadfromgrouponly == 0 )
        {
            initprintf("Warning(s) found in file `%s'.  Do you want to use the INTERNAL DEFAULTS (y/N)?",filenam);

            i=wm_ynbox("CON File Compilation Warning", "Warning(s) found in file `%s'. Do you want to use the "
                       "INTERNAL DEFAULTS?",filenam);
            if (i) i = 'y';
            if(i == 'y' || i == 'Y' )
            {
                loadfromgrouponly = 1;
                initprintf(" Yes\n");
                return;
            }
        }
    }

    if(error)
    {
        if( loadfromgrouponly )
        {
            sprintf(buf,"Error compiling CON files.");
            gameexit(buf);
        }
        else
        {
            if( groupfile != -1 && loadfromgrouponly == 0 )
            {
                initprintf("Error(s) found in file `%s'.  Do you want to use the INTERNAL DEFAULTS (y/N)?\n",filenam);

                i=wm_ynbox("CON File Compilation Error", "Error(s) found in file `%s'. Do you want to use the "
                           "INTERNAL DEFAULTS?",filenam);
                if (i) i = 'y';
                if( i == 'y' || i == 'Y' )
                {
                    initprintf(" Yes\n");
                    loadfromgrouponly = 1;
                    return;
                }
                else gameexit("");
            }
        }
    }
    else
    {
        int j, k;

        total_lines += line_number;
        for(i=j=0;i<MAXGAMEEVENTS;i++)
        {
            if(apScriptGameEvent[i])
                j++;
        }
        for(i=k=0;i<MAXTILES;i++)
        {
            if(actorscrptr[i])
                k++;
        }

        initprintf("\nCompiled code size: %ld/%ld bytes\n",(unsigned)(scriptptr-script),MAXSCRIPTSIZE);
        initprintf("%ld/%ld labels, %d/%d variables\n",labelcnt,min((sizeof(sector)/sizeof(long)),(sizeof(sprite)/(1<<6))),iGameVarCount,MAXGAMEVARS);
        initprintf("%ld event definitions, %ld defined actors\n\n",j,k);

        for(i=0;i<128;i++)
            if(fta_quotes[i] == NULL)
                fta_quotes[i] = Bcalloc(MAXQUOTELEN,sizeof(char));
    }
}

void ReportError(int iError)
{
    if(Bstrcmp(parsing_item_name,previous_item_name))
    {
        if(parsing_event || parsing_state || parsing_actor)
            initprintf("%s: In %s `%s':\n",compilefile,parsing_event?"event":parsing_actor?"actor":"state",parsing_item_name);
        else initprintf("%s: At top level:\n",compilefile);
        Bstrcpy(previous_item_name,parsing_item_name);
    }
    switch(iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%ld: error: found more `}' than `{' before `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_EVENTONLY:
        initprintf("%s:%ld: error: command `%s' only valid during events.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        initprintf("%s:%ld: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",compilefile,line_number,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%ld: error: expected a keyword but found `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%ld: error: found `%s' within %s.\n",compilefile,line_number,tempbuf,parsing_event?"an event":parsing_actor?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%ld: error: symbol `%s' is a keyword.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%ld: error: did not find `endswitch' before `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%ld: error: symbol `%s' is not a game definition.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%ld: error: symbol `%s' is not a game variable.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_OPENBRACKET:
        initprintf("%s:%ld: error: found more `{' than `}' before `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%ld: error: parameter `%s' is undefined.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_SYMBOLNOTRECOGNIZED:
        initprintf("%s:%ld: error: symbol `%s' is not recognized.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%ld: error: syntax error.\n",compilefile,line_number);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%ld: error: variable `%s' is read-only.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%ld: error: variable `%s' is of the wrong type.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%ld: warning: duplicate game definition `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case WARNING_EVENTSYNC:
        initprintf("%s:%ld: warning: found `%s' within a local event.\n",compilefile,line_number,tempbuf);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%ld: warning: expected a label, found a constant.\n",compilefile,line_number);
        break;
    }
}
