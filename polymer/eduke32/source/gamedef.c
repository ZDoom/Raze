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
#include "gamedef.h"

#include "osd.h"

int g_ScriptVersion = 13; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files

char compilefile[BMAX_PATH] = "(none)";  // file we're currently compiling
static char parsing_item_name[MAXVARLABEL] = "(none)", previous_item_name[MAXVARLABEL] = "NULL";

int total_lines,line_number;
static int checking_ifelse,parsing_state;
char g_szBuf[1024];

intptr_t *casescriptptr=NULL;      // the pointer to the start of the case table in a switch statement
// first entry is 'default' code.
static int casecount = 0;
static int checking_switch = 0, current_event = -1;
static int labelsonly = 0, nokeywordcheck = 0, dynamicremap = 0;
static int num_braces = 0;

static int increasescriptsize(int size);

int redefined_quote_count = 0;

intptr_t *aplWeaponClip[MAX_WEAPONS];       // number of items in magazine
intptr_t *aplWeaponReload[MAX_WEAPONS];     // delay to reload (include fire)
intptr_t *aplWeaponFireDelay[MAX_WEAPONS];      // delay to fire
intptr_t *aplWeaponHoldDelay[MAX_WEAPONS];      // delay after release fire button to fire (0 for none)
intptr_t *aplWeaponTotalTime[MAX_WEAPONS];      // The total time the weapon is cycling before next fire.
intptr_t *aplWeaponFlags[MAX_WEAPONS];      // Flags for weapon
intptr_t *aplWeaponShoots[MAX_WEAPONS];     // what the weapon shoots
intptr_t *aplWeaponSpawnTime[MAX_WEAPONS];      // the frame at which to spawn an item
intptr_t *aplWeaponSpawn[MAX_WEAPONS];      // the item to spawn
intptr_t *aplWeaponShotsPerBurst[MAX_WEAPONS];  // number of shots per 'burst' (one ammo per 'burst'
intptr_t *aplWeaponWorksLike[MAX_WEAPONS];      // What original the weapon works like
intptr_t *aplWeaponInitialSound[MAX_WEAPONS];   // Sound made when initialy firing. zero for no sound
intptr_t *aplWeaponFireSound[MAX_WEAPONS];      // Sound made when firing (each time for automatic)
intptr_t *aplWeaponSound2Time[MAX_WEAPONS];     // Alternate sound time
intptr_t *aplWeaponSound2Sound[MAX_WEAPONS];    // Alternate sound sound ID
intptr_t *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
intptr_t *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted

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

intptr_t *actorLoadEventScrptr[MAXTILES];

intptr_t *apScriptGameEvent[MAXGAMEEVENTS];
intptr_t *parsing_event=NULL;

gamevar_t aGameVars[MAXGAMEVARS];
gamearray_t aGameArrays[MAXGAMEARRAYS];
int iGameVarCount=0;
int iGameArrayCount=0;

extern int qsetmode;

char *textptr;
int error,warning;

extern char *duke3dgrpstring;

enum labeltypes
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
};

static const char *labeltypenames[] =
{
    "define",
    "state",
    "actor",
    "action",
    "ai",
    "move"
};

static const char *translatelabeltype(int type)
{
    int i;
    char x[64];

    x[0] = 0;
    for (i=0;i<6;i++)
    {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, labeltypenames[i]);
    }
    return strdup(x);
}

#define NUMKEYWORDS (signed int)(sizeof(keyw)/sizeof(keyw[0]))

const char *keyw[] =
{
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
    "ezshoot",                  // 241
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
    "qsprintf",                 // 264
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
    "save",                     // 288
    "cansee",                   // 289
    "canseespr",                // 290
    "findnearactorz",           // 291
    "findnearactorzvar",        // 292
    "findnearspritez",          // 293
    "findnearspritezvar",       // 294
    "zshootvar",                // 295
    "ezshootvar",               // 296
    "getcurraddress",           // 297
    "jump",                     // 298
    "qstrlen",                  // 299
    "getincangle",              // 300
    "quake",                    // 301
    "showview",                 // 302
    "headspritestat",           // 303
    "prevspritestat",           // 304
    "nextspritestat",           // 305
    "headspritesect",           // 306
    "prevspritesect",           // 307
    "nextspritesect",           // 308
    "getkeyname",               // 309
    "qsubstr",                  // 310
    "gametextz",                // 311
    "digitalnumberz",           // 312
    "spritenopal",              // 313
    "hitradiusvar",             // 314
    "rotatesprite16",           // 315
    "gamearray",                // 316
    "setarray",                 // 317
    "resizearray",              // 318
    "writearraytofile",         // 319
    "readarrayfromfile",        // 320
    "starttrackvar",            // 321
    "qgetsysstr",               // 322
    "getticks",                 // 323
    "gettspr",                  // 324
    "settspr",                  // 325
    "savemapstate",             // 326
    "loadmapstate",             // 327
    "clearmapstate",            // 328
    "scriptsize",               // 329
    "definegamename",           // 330
    "cmenu",                    // 331
    "gettimedate",              // 332
    "activatecheat",            // 333
    "setgamepalette",           // 334
    "<null>"
};

const memberlabel_t sectorlabels[]=
{
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

const memberlabel_t walllabels[]=
{
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

const memberlabel_t actorlabels[]=
{
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
    { "htg_t", ACTOR_HTG_T, LABEL_HASPARM2, 10 },

    // model flags

    { "angoff", ACTOR_ANGOFF, 0, 0 },
    { "pitch", ACTOR_PITCH, 0, 0 },
    { "roll", ACTOR_ROLL, 0, 0 },
    { "mdxoff", ACTOR_MDXOFF, 0, 0 },
    { "mdyoff", ACTOR_MDYOFF, 0, 0 },
    { "mdzoff", ACTOR_MDZOFF, 0, 0 },
    { "mdflags", ACTOR_MDFLAGS, 0, 0 },
    { "xpanning", ACTOR_XPANNING, 0, 0 },
    { "ypanning", ACTOR_YPANNING, 0, 0 },

    { "htflags", ACTOR_HTFLAGS, 0, 0 },

    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t tsprlabels[]=
{
    // tsprite access

    { "tsprx", ACTOR_TSPRX, 0, 0 },
    { "tspry", ACTOR_TSPRY, 0, 0 },
    { "tsprz", ACTOR_TSPRZ, 0, 0 },
    { "tsprcstat", ACTOR_TSPRCSTAT, 0, 0 },
    { "tsprpicnum", ACTOR_TSPRPICNUM, 0, 0 },
    { "tsprshade", ACTOR_TSPRSHADE, 0, 0 },
    { "tsprpal", ACTOR_TSPRPAL, 0, 0 },
    { "tsprxrepeat", ACTOR_TSPRXREPEAT, 0, 0 },
    { "tspryrepeat", ACTOR_TSPRYREPEAT, 0, 0 },
    { "tsprxoffset", ACTOR_TSPRXOFFSET, 0, 0 },
    { "tspryoffset", ACTOR_TSPRYOFFSET, 0, 0 },
    { "tsprsectnum", ACTOR_TSPRSECTNUM, 0, 0 },
    { "tsprang", ACTOR_TSPRANG, 0, 0 },

    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t playerlabels[]=
{
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
    { "max_secret_rooms", PLAYER_MAX_SECRET_ROOMS, 0, 0 },
    { "secret_rooms", PLAYER_SECRET_ROOMS, 0, 0 },
    { "pals", PLAYER_PALS, LABEL_HASPARM2, 3 },
    { "max_actors_killed", PLAYER_MAX_ACTORS_KILLED, 0, 0 },
    { "actors_killed", PLAYER_ACTORS_KILLED, 0, 0 },
    { "return_to_center", PLAYER_RETURN_TO_CENTER, 0, 0 },
    { "runspeed", PLAYER_RUNSPEED, 0, 0 },
    { "sbs", PLAYER_SBS, 0, 0 },
    { "reloading", PLAYER_RELOADING, 0, 0 },
    { "auto_aim", PLAYER_AUTO_AIM, 0, 0 },
    { "movement_lock", PLAYER_MOVEMENT_LOCK, 0, 0 },
    { "sound_pitch", PLAYER_SOUND_PITCH, 0, 0 },
    { "weaponswitch", PLAYER_WEAPONSWITCH, 0, 0 },
    { "team", PLAYER_TEAM, 0, 0 },
    { "max_player_health", PLAYER_MAX_PLAYER_HEALTH, 0, 0 },
    { "max_shield_amount", PLAYER_MAX_SHIELD_AMOUNT, 0, 0 },
    { "max_ammo_amount", PLAYER_MAX_AMMO_AMOUNT, LABEL_HASPARM2, MAX_WEAPONS },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t projectilelabels[]=
{
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

const memberlabel_t userdefslabels[]=
{
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
    { "democams", USERDEFS_DEMOCAMS, 0, 0 },
    { "color", USERDEFS_COLOR, 0, 0 },
    { "msgdisptime", USERDEFS_MSGDISPTIME, 0, 0 },
    { "statusbarmode", USERDEFS_STATUSBARMODE, 0, 0 },
    { "m_noexits", USERDEFS_M_NOEXITS, 0, 0 },
    { "noexits", USERDEFS_NOEXITS, 0, 0 },
    { "autovote", USERDEFS_AUTOVOTE, 0, 0 },
    { "automsg", USERDEFS_AUTOMSG, 0, 0 },
    { "idplayers", USERDEFS_IDPLAYERS, 0, 0 },
    { "team", USERDEFS_TEAM, 0, 0 },
    { "viewbob", USERDEFS_VIEWBOB, 0, 0 },
    { "weaponsway", USERDEFS_WEAPONSWAY, 0, 0 },
    { "angleinterpolation", USERDEFS_ANGLEINTERPOLATION, 0, 0 },
    { "obituaries", USERDEFS_OBITUARIES, 0, 0 },
    { "levelstats", USERDEFS_LEVELSTATS, 0, 0 },
    { "crosshairscale", USERDEFS_CROSSHAIRSCALE, 0, 0 },
    { "althud", USERDEFS_ALTHUD, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t inputlabels[]=
{
    { "avel", INPUT_AVEL, 0, 0 },
    { "horz", INPUT_HORZ, 0, 0 },
    { "fvel", INPUT_FVEL, 0, 0 },
    { "svel", INPUT_SVEL, 0, 0 },
    { "bits", INPUT_BITS, 0, 0 },
    { "extbits", INPUT_EXTBITS, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

char *bitptr;


#define BITPTR_DONTFUCKWITHIT 0
#define BITPTR_POINTER 1

static int increasescriptsize(int size)
{
    intptr_t oscriptptr = (unsigned)(scriptptr-script);
    intptr_t ocasescriptptr = (unsigned)(casescriptptr-script);
    intptr_t oparsing_event = (unsigned)(parsing_event-script);
    intptr_t oparsing_actor = (unsigned)(parsing_actor-script);
    char *scriptptrs;
    intptr_t *newscript;
    intptr_t i, j;
    int osize = g_ScriptSize;

    for (i=MAXSECTORS-1;i>=0;i--)
    {
        if (labelcode[i] && labeltype[i] != LABEL_DEFINE)
        {
            labelcode[i] -= (intptr_t)&script[0];
        }
    }

    scriptptrs = Bcalloc(1,g_ScriptSize * sizeof(char));
    for (i=g_ScriptSize-1;i>=0;i--)
    {
//            initprintf("%d\n",i);
        if (bitptr[i] == BITPTR_POINTER && !((intptr_t)script[i] >= (intptr_t)(&script[0]) && (intptr_t)script[i] < (intptr_t)(&script[g_ScriptSize])))
            initprintf("Internal compiler error at %d (0x%x)\n",i,i);
//        if (bitptr[i] == 0 && ((intptr_t)script[i] >= (intptr_t)(&script[0]) && (intptr_t)script[i] < (intptr_t)(&script[g_ScriptSize])))
//            initprintf("oh no!\n");
        if (bitptr[i] == BITPTR_POINTER /*&& ((intptr_t)script[i] >= (intptr_t)(&script[0]) && (intptr_t)script[i] < (intptr_t)(&script[g_ScriptSize]))*/)
        {
            scriptptrs[i] = 1;
            script[i] -= (intptr_t)&script[0];
        }
        else scriptptrs[i] = 0;
    }

    for (i=MAXTILES-1;i>=0;i--)
    {
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]-(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]-(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }
    }

    for (i=MAXGAMEEVENTS-1;i>=0;i--)
        if (apScriptGameEvent[i])
        {
            j = (intptr_t)apScriptGameEvent[i]-(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)j;
        }

    //initprintf("offset: %d\n",(unsigned)(scriptptr-script));
    g_ScriptSize = size;
    initprintf("Increasing script buffer size to %d bytes...\n",g_ScriptSize * sizeof(intptr_t));
    newscript = (intptr_t *)Brealloc(script, g_ScriptSize * sizeof(intptr_t));

    if (newscript == NULL)
    {
        ReportError(-1);
        initprintf("%s:%d: out of memory: Aborted (%ud)\n",compilefile,line_number,(unsigned)(scriptptr-script));
        initprintf(tempbuf);
        error++;
        return 1;
    }
    Bmemset(&newscript[osize],0,(size-osize) * sizeof(intptr_t));
    script = newscript;
    scriptptr = (intptr_t *)(script+oscriptptr);
    bitptr = (char *)Brealloc(bitptr, g_ScriptSize * sizeof(char));
    Bmemset(&bitptr[osize],0,size-osize);
//    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    //initprintf("offset: %d\n",(unsigned)(scriptptr-script));
    if (casescriptptr != NULL)
        casescriptptr = (intptr_t *)(script+ocasescriptptr);
    if (parsing_event != NULL)
        parsing_event = (intptr_t *)(script+oparsing_event);
    if (parsing_actor != NULL)
        parsing_actor = (intptr_t *)(script+oparsing_actor);

    for (i=MAXSECTORS-1;i>=0;i--)
    {
        if (labelcode[i] && labeltype[i] != LABEL_DEFINE)
        {
            labelcode[i] += (intptr_t)&script[0];
        }
    }

    for (i=g_ScriptSize-(size-osize)-1;i>=0;i--)
        if (scriptptrs[i])
        {
            j = (intptr_t)script[i]+(intptr_t)&script[0];
            script[i] = j;
        }

    for (i=MAXTILES-1;i>=0;i--)
    {
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]+(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]+(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }
    }

    for (i=MAXGAMEEVENTS-1;i>=0;i--)
        if (apScriptGameEvent[i])
        {
            j = (intptr_t)apScriptGameEvent[i]+(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)j;
        }
    Bfree(scriptptrs);
    return 0;
}

static int skipcomments(void)
{
    char c;

    while ((c = *textptr))
    {
        if (c == ' ' || c == '\t' || c == '\r')
            textptr++;
        else if (c == '\n')
        {
            line_number++;
            textptr++;
        }
        else if (c == '/' && textptr[1] == '/')
        {
            if (!(error || warning) && g_ScriptDebug > 1)
                initprintf("%s:%d: debug: got comment.\n",compilefile,line_number);
            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
                textptr++;
        }
        else if (c == '/' && textptr[1] == '*')
        {
            if (!(error || warning) && g_ScriptDebug > 1)
                initprintf("%s:%d: debug: got start of comment block.\n",compilefile,line_number);
            while (*textptr && !(textptr[0] == '*' && textptr[1] == '/'))
            {
                if (*textptr == '\n')
                    line_number++;
                textptr++;
            }
            if ((!(error || warning) && g_ScriptDebug > 1) && (textptr[0] == '*' && textptr[1] == '/'))
                initprintf("%s:%d: debug: got end of comment block.\n",compilefile,line_number);
            if (!*textptr)
            {
                if (!(error || warning) && g_ScriptDebug)
                    initprintf("%s:%d: debug: EOF in comment!\n",compilefile,line_number);
                ReportError(-1);
                initprintf("%s:%d: error: found `/*' with no `*/'.\n",compilefile,line_number);
                parsing_state = num_braces = 0;
                parsing_actor = 0;
                error++;
                break;
            }
            else textptr+=2;
        }
        else break;
    }

    if ((unsigned)(scriptptr-script) > (unsigned)(g_ScriptSize-32))
        return increasescriptsize(g_ScriptSize+16384);

    return 0;
}

static void DefineProjectile(int lVar1, int lLabelID, int lVar2)
{
    switch (lLabelID)
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

static int CheckEventSync(int iEventID)
{
    if (parsing_event || parsing_actor)
    {
        switch (iEventID)
        {
        case EVENT_ANIMATESPRITES:
        case EVENT_CHEATGETSTEROIDS:
        case EVENT_CHEATGETHEAT:
        case EVENT_CHEATGETBOOT:
        case EVENT_CHEATGETSHIELD:
        case EVENT_CHEATGETSCUBA:
        case EVENT_CHEATGETHOLODUKE:
        case EVENT_CHEATGETJETPACK:
        case EVENT_CHEATGETFIRSTAID:
        case EVENT_DISPLAYCROSSHAIR:
        case EVENT_DISPLAYREST:
        case EVENT_DISPLAYBONUSSCREEN:
        case EVENT_DISPLAYMENU:
        case EVENT_DISPLAYMENUREST:
        case EVENT_DISPLAYLOADINGSCREEN:
        case EVENT_DISPLAYROOMS:
        case EVENT_DISPLAYSBAR:
        case EVENT_DISPLAYWEAPON:
        case EVENT_DRAWWEAPON:
        case EVENT_ENTERLEVEL:
        case EVENT_FAKEDOMOVETHINGS:
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

#if 0
void AddLog(const char *psz, ...)
{
    va_list va;

    va_start(va, psz);
    Bvsnprintf(tempbuf, sizeof(tempbuf), psz, va);
    va_end(va);

    if (tempbuf[Bstrlen(tempbuf)] != '\n')
        Bstrcat(tempbuf,"\n");
    if (qsetmode == 200) OSD_Printf(tempbuf);
    else initprintf(tempbuf);
}
#endif

static int GetDefID(const char *szGameLabel)
{
    int i;
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL)
        {
            if (Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}
static int GetADefID(const char *szGameLabel)
{
    int i;
//    initprintf("iGameArrayCount is %i\n",iGameArrayCount);
    for (i=0;i<iGameArrayCount;i++)
    {
        if (aGameArrays[i].szLabel != NULL)
        {
            if (Bstrcmp(szGameLabel, aGameArrays[i].szLabel) == 0)
            {
                return i;
            }
        }
    }
//    initprintf("game array %s not found\n",szGameLabel);
    return -1;
}
static int ispecial(char c)
{
    if (c == 0x0a)
    {
        line_number++;
        return 1;
    }

    if (c == ' ' || c == 0x0d)
        return 1;

    return 0;
}

static inline int isaltok(char c)
{
    return (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

static int getlabelid(const memberlabel_t *pLabel, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int l=-1;
    int i;

    for (i=0;pLabel[i].lId >=0 ; i++)
    {
        if (!Bstrcasecmp(pLabel[i].name,psz))
        {
            l= pLabel[i].lId;
            break;  // stop for loop
        }
    }
    return l;
}

static int getlabeloffset(const memberlabel_t *pLabel, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the offset in the array for the label, or -1
    int i;

    for (i=0;pLabel[i].lId >=0 ; i++)
    {
        if (!Bstrcasecmp(pLabel[i].name,psz))
        {
            //    printf("Label has flags of %02X\n",pLabel[i].flags);
            return i;
        }
    }
    return -1;
}

static void getlabel(void)
{
    int i;

    skipcomments();

    while (isalnum(*textptr) == 0)
    {
        if (*textptr == 0x0a) line_number++;
        textptr++;
        if (*textptr == 0)
            return;
    }

    i = 0;
    while (ispecial(*textptr) == 0 && *textptr!='['&& *textptr!=']' && *textptr!='\t' && *textptr!='\n' && *textptr!='\r')
        label[(labelcnt<<6)+i++] = *(textptr++);

    label[(labelcnt<<6)+i] = 0;
    if (!(error || warning) && g_ScriptDebug > 1)
        initprintf("%s:%d: debug: got label `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
}

static int keyword(void)
{
    int i;
    char *temptextptr;

    skipcomments();

    temptextptr = textptr;

    while (isaltok(*temptextptr) == 0)
    {
        temptextptr++;
        if (*temptextptr == 0)
            return 0;
    }

    i = 0;
    while (isaltok(*temptextptr))
    {
        tempbuf[i] = *(temptextptr++);
        i++;
    }
    tempbuf[i] = 0;
    for (i=0;i<NUMKEYWORDS;i++)
        if (Bstrcmp(tempbuf,keyw[i]) == 0)
            return i;

    return -1;
}

static int transword(void) //Returns its code #
{
    int i, l;

    skipcomments();

    while (isaltok(*textptr) == 0)
    {
        if (*textptr == 0x0a) line_number++;
        if (*textptr == 0)
            return -1;
        textptr++;
    }

    l = 0;
    while (isaltok(*(textptr+l)) && !(*(textptr + l) == '.'))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for (i=0;i<NUMKEYWORDS;i++)
    {
        if (Bstrcmp(tempbuf,keyw[i]) == 0)
        {
            *scriptptr = i + (line_number<<12);
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            textptr += l;
            scriptptr++;
            if (!(error || warning) && g_ScriptDebug)
                initprintf("%s:%d: debug: translating keyword `%s'.\n",compilefile,line_number,keyw[i]);
            return i;
        }
    }

    textptr += l;

    if (tempbuf[0] == '{' && tempbuf[1] != 0)
    {
        ReportError(-1);
        initprintf("%s:%d: error: expected a SPACE or CR between `{' and `%s'.\n",compilefile,line_number,tempbuf+1);
    }
    else if (tempbuf[0] == '}' && tempbuf[1] != 0)
    {
        ReportError(-1);
        initprintf("%s:%d: error: expected a SPACE or CR between `}' and `%s'.\n",compilefile,line_number,tempbuf+1);
    }
    else ReportError(ERROR_EXPECTEDKEYWORD);
    error++;
    return -1;
}

static void transvartype(int type)
{
    int i=0,f=0;

    skipcomments();
    if (!type && !labelsonly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        if (!(error || warning) && g_ScriptDebug)
            initprintf("%s:%d: debug: accepted constant %d in place of gamevar.\n",compilefile,line_number,atol(textptr));
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=MAXGAMEVARS;
        if (tolower(textptr[1])=='x')
            sscanf(textptr+2,"%" PRIxPTR "",scriptptr);
        else
            *scriptptr=atol(textptr);
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        scriptptr++;
        getlabel();
        return;
    }
    else if ((*textptr == '-') && !isdigit(*(textptr+1)))
    {
        if (!type)
        {
            if (!(error || warning) && g_ScriptDebug)
                initprintf("%s:%d: debug: flagging gamevar as negative.\n",compilefile,line_number,atol(textptr));
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

    if (!nokeywordcheck)
        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return;
            }

    skipcomments(); //skip comments and whitespace
    if ((*textptr == '['))     //read of array as a gamevar
    {
        f |= (MAXGAMEVARS<<2);
//        initprintf("got an array");
        textptr++;
        i=GetADefID(label+(labelcnt<<6));
        if (i < 0)
        {
            error++;
            ReportError(ERROR_NOTAGAMEARRAY);
            return;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=(i|f);
        transvartype(0);
        skipcomments(); //skip comments and whitespace

        if (*textptr != ']')
        {
            error++;
            ReportError(ERROR_GAMEARRAYBNC);
            return;
        }
        textptr++;
        if (type)   //writing arrays in this way is not supported because it would require too many changes to other code
        {
            error++;
            ReportError(ERROR_INVALIDARRAYWRITE);
            return;
        }
        return;
    }
//    initprintf("not an array");
    i=GetDefID(label+(labelcnt<<6));
    if (i<0)   //gamevar not found
    {
        if (!type && !labelsonly)
        {
            //try looking for a define instead
            Bstrcpy(tempbuf,label+(labelcnt<<6));
            for (i=0;i<labelcnt;i++)
            {
                if (Bstrcmp(tempbuf,label+(i<<6)) == 0 && (labeltype[i] & LABEL_DEFINE))
                {
                    if (!(error || warning) && g_ScriptDebug)
                        initprintf("%s:%d: debug: accepted defined label `%s' instead of gamevar.\n",compilefile,line_number,label+(i<<6));
                    bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                    *scriptptr++=MAXGAMEVARS;
                    bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                    *scriptptr++=labelcode[i];
                    return;
                }
            }
            error++;
            ReportError(ERROR_NOTAGAMEVAR);
            return;
        }
        error++;
        ReportError(ERROR_NOTAGAMEVAR);
        textptr++;
        return;

    }
    if (type == GAMEVAR_FLAG_READONLY && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
    {
        error++;
        ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (aGameVars[i].dwFlags & type)
    {
        error++;
        ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }
    if ((aGameVars[i].dwFlags & GAMEVAR_FLAG_SYNCCHECK) && parsing_actor && CheckEventSync(current_event))
    {
        ReportError(-1);
        initprintf("%s:%d: warning: found local gamevar `%s' used within %s; expect multiplayer synchronization issues.\n",compilefile,line_number,label+(labelcnt<<6),parsing_event?"a synced event":"an actor");
    }
    if (!(error || warning) && g_ScriptDebug > 1)
        initprintf("%s:%d: debug: accepted gamevar `%s'.\n",compilefile,line_number,label+(labelcnt<<6));

    bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
    *scriptptr++=(i|f);
}

static inline void transvar(void)
{
    transvartype(0);
}

static inline void transmultvarstype(int type, int num)
{
    int i;
    for (i=0;i<num;i++)
        transvartype(type);
}

static inline void transmultvars(int num)
{
    transmultvarstype(0,num);
}

static int transnum(int type)
{
    int i, l;

    skipcomments();

    while (isaltok(*textptr) == 0)
    {
        if (*textptr == 0x0a) line_number++;
        textptr++;
        if (*textptr == 0)
            return -1; // eof
    }

    l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (!nokeywordcheck)
        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                textptr+=l;
            }

    for (i=0;i<labelcnt;i++)
    {
        if (!Bstrcmp(tempbuf,label+(i<<6)))
        {
            char *el,*gl;

            if (labeltype[i] & type)
            {
                if (!(error || warning) && g_ScriptDebug > 1)
                {
                    gl = (char *)translatelabeltype(labeltype[i]);
                    initprintf("%s:%d: debug: accepted %s label `%s'.\n",compilefile,line_number,gl,label+(i<<6));
                    Bfree(gl);
                }
                if (labeltype[i] != LABEL_DEFINE && labelcode[i] >= (intptr_t)&script[0] && labelcode[i] < (intptr_t)&script[g_ScriptSize])
                    bitptr[(scriptptr-script)] = 1;
                else bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                *(scriptptr++) = labelcode[i];
                textptr += l;
                return labeltype[i];
            }
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *(scriptptr++) = 0;
            textptr += l;
            el = (char *)translatelabeltype(type);
            gl = (char *)translatelabeltype(labeltype[i]);
            ReportError(-1);
            initprintf("%s:%d: warning: expected a %s, found a %s.\n",compilefile,line_number,el,gl);
            Bfree(el);
            Bfree(gl);
            return -1;  // valid label name, but wrong type
        }
    }

    if (isdigit(*textptr) == 0 && *textptr != '-')
    {
        ReportError(ERROR_PARAMUNDEFINED);
        error++;
        textptr+=l;
        return -1; // error!
    }

    if (isdigit(*textptr) && labelsonly)
    {
        ReportError(WARNING_LABELSONLY);
        //         warning++;
    }
    if (!(error || warning) && g_ScriptDebug > 1)
        initprintf("%s:%d: debug: accepted constant %d.\n",compilefile,line_number,atol(textptr));
    bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
    if (tolower(textptr[1])=='x')sscanf(textptr+2,"%" PRIxPTR "",scriptptr);
    else
        *scriptptr = atol(textptr);
    scriptptr++;

    textptr += l;

    return 0;   // literal value
}

static int parsecommand(void);

static int CountCaseStatements()
{
    int lCount;
    char *temptextptr = textptr;
    int temp_line_number = line_number;
    intptr_t scriptoffset = (unsigned)(scriptptr-script);
    intptr_t caseoffset = (unsigned)(casescriptptr-script);
//    int i;

    casecount=0;
    casescriptptr=NULL;
    //Bsprintf(g_szBuf,"CSS: %.12s",textptr);
    //AddLog(g_szBuf);
    while (parsecommand() == 0)
    {
        //Bsprintf(g_szBuf,"CSSL: %.20s",textptr);
        //AddLog(g_szBuf);
        ;
    }
    // since we processed the endswitch, we need to re-increment checking_switch
    checking_switch++;

    textptr=temptextptr;
    scriptptr = (intptr_t *)(script+scriptoffset);

    line_number = temp_line_number;

    lCount=casecount;
    casecount=0;
    casescriptptr = (intptr_t *)(script+caseoffset);
    casecount = 0;
    return lCount;
}

static int parsecommand(void)
{
    int i, j=0, k=0, done, tw;
    char *temptextptr;
    intptr_t *tempscrptr;

    if (quitevent)
    {
        initprintf("Aborted.\n");
        Shutdown();
        exit(0);
    }

    if ((error+warning) > 63 || (*textptr == '\0') || (*(textptr+1) == '\0')) return 1;

    if (g_ScriptDebug)
        ReportError(-1);

    if (checking_switch > 0)
    {
        //Bsprintf(g_szBuf,"PC(): '%.25s'",textptr);
        //AddLog(g_szBuf);
    }
    tw = transword();
    //    Bsprintf(tempbuf,"%s",keyw[tw]);
    //    AddLog(tempbuf);

    if (skipcomments())
        return 1;

    switch (tw)
    {
    default:
    case -1:
        return 0; //End
    case CON_STATE:
        if (parsing_actor == 0 && parsing_state == 0)
        {
            getlabel();
            scriptptr--;
            labelcode[labelcnt] = (intptr_t) scriptptr;
            labeltype[labelcnt] = LABEL_STATE;

            parsing_state = 1;
            Bsprintf(parsing_item_name,"%s",label+(labelcnt<<6));
            labelcnt++;
            return 0;
        }

        getlabel();

        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }
        for (j=0;j<labelcnt;j++)
        {
            if (Bstrcmp(label+(j<<6),label+(labelcnt<<6)) == 0)
            {
                if (labeltype[j] & LABEL_STATE)
                {
                    if (!(error || warning) && g_ScriptDebug > 1)
                        initprintf("%s:%d: debug: accepted state label `%s'.\n",compilefile,line_number,label+(j<<6));
                    *scriptptr = labelcode[j];
                    if (labelcode[j] >= (intptr_t)&script[0] && labelcode[j] < (intptr_t)&script[g_ScriptSize])
                        bitptr[(scriptptr-script)] = BITPTR_POINTER;
                    else bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                    break;
                }
                else
                {
                    char *gl = (char *)translatelabeltype(labeltype[j]);
                    ReportError(-1);
                    initprintf("%s:%d: warning: expected a state, found a %s.\n",compilefile,line_number,gl);
                    Bfree(gl);
                    *(scriptptr-1) = CON_NULLOP; // get rid of the state, leaving a nullop to satisfy if conditions
                    bitptr[(scriptptr-script-1)] = BITPTR_DONTFUCKWITHIT;
                    return 0;  // valid label name, but wrong type
                }
            }
        }
        if (j==labelcnt)
        {
            ReportError(-1);
            initprintf("%s:%d: error: state `%s' not found.\n",compilefile,line_number,label+(labelcnt<<6));
            error++;
        }
        scriptptr++;
        return 0;

    case CON_ENDS:
        if (parsing_state == 0)
        {
            ReportError(-1);
            initprintf("%s:%d: error: found `ends' without open `state'.\n",compilefile,line_number);
            error++;
        }
        //            else
        {
            if (num_braces > 0)
            {
                ReportError(ERROR_OPENBRACKET);
                error++;
            }
            if (num_braces < 0)
            {
                ReportError(ERROR_CLOSEBRACKET);
                error++;
            }
            if (checking_switch > 0)
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETTHISPROJECTILE:
    case CON_GETPROJECTILE:
    {
        int lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        if (tw == CON_SETTHISPROJECTILE)
            labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=projectilelabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",playerlabels[lLabelID].flags);

        // now at target VAR...

        // get the ID of the DEF
        switch (tw)
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

        if (isdigit(*textptr) || (*textptr == '-'))
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

        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }
#if 0
        for (i=0;i<iGameVarCount;i++)
        {
            if (aGameVars[i].szLabel != NULL)
            {
                if (Bstrcmp(label+(labelcnt<<6),aGameVars[i].szLabel) == 0)
                {
                    warning++;
                    initprintf("  * WARNING.(L%d) duplicate Game definition `%s' ignored.\n",line_number,label+(labelcnt<<6));
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
        if ((*(scriptptr-1)&GAMEVAR_FLAG_USER_MASK)==3)
        {
            warning++;
            *(scriptptr-1)^=GAMEVAR_FLAG_PERPLAYER;
            ReportError(WARNING_BADGAMEVAR);
        }
        AddGameVar(label+(labelcnt<<6),*(scriptptr-2),
                   (*(scriptptr-1))
                   // can't define default or secret
                   & (~(GAMEVAR_FLAG_DEFAULT | GAMEVAR_FLAG_SECRET))
                  );
        //AddLog("Added gamevar");
        scriptptr -= 3; // no need to save in script...
        return 0;

    case CON_GAMEARRAY:
        if (isdigit(*textptr) || (*textptr == '-'))
        {
            getlabel();
            error++;
            ReportError(ERROR_SYNTAXERROR);
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            scriptptr -= 2; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }
        getlabel();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined

        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }
        transnum(LABEL_DEFINE);
        AddGameArray(label+(labelcnt<<6),*(scriptptr-1));

        scriptptr -= 2; // no need to save in script...
        return 0;


    case CON_DEFINE:
    {
        //printf("Got definition. Getting Label. '%.20s'\n",textptr);
        getlabel();
        //printf("Got label. '%.20s'\n",textptr);
        // Check to see it's already defined

        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }

        for (i=0;i<labelcnt;i++)
        {
            if (Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 /* && (labeltype[i] & LABEL_DEFINE) */)
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
        if (i == labelcnt)
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
        for (j=0;j<4;j++)
        {
            if (keyword() == -1)
                transnum(LABEL_DEFINE);
            else break;
        }

        while (j<4)
        {
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr = 0;
            scriptptr++;
            j++;
        }
        return 0;

    case CON_MOVE:
        if (parsing_actor || parsing_state)
        {
            if (!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);

            if ((transnum(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(scriptptr-1) != 0) && (*(scriptptr-1) != 1))
            {
                ReportError(-1);
                bitptr[(scriptptr-script-1)] = BITPTR_DONTFUCKWITHIT;
                *(scriptptr-1) = 0;
                initprintf("%s:%d: warning: expected a move, found a constant.\n",compilefile,line_number);
            }

            j = 0;
            while (keyword() == -1)
            {
                transnum(LABEL_DEFINE);
                scriptptr--;
                j |= *scriptptr;
            }
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr = j;

            scriptptr++;
        }
        else
        {
            scriptptr--;
            getlabel();
            // Check to see it's already defined

            for (i=0;i<NUMKEYWORDS;i++)
                if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            for (i=0;i<labelcnt;i++)
                if (Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 /* && (labeltype[i] & LABEL_MOVE) */)
                {
                    warning++;
                    initprintf("%s:%d: warning: duplicate move `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }
            if (i == labelcnt)
            {
                labeltype[labelcnt] = LABEL_MOVE;
                labelcode[labelcnt++] = (intptr_t) scriptptr;
            }
            for (j=0;j<2;j++)
            {
                if (keyword() >= 0) break;
                transnum(LABEL_DEFINE);
            }
            for (k=j;k<2;k++)
            {
                bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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

        if (k >= 0) // if it's background music
        {
            i = 0;
            // get the file name...
            while (keyword() == -1)
            {
                while (isaltok(*textptr) == 0)
                {
                    if (*textptr == 0x0a) line_number++;
                    textptr++;
                    if (*textptr == 0) break;
                }
                j = 0;
                tempbuf[j] = '/';
                while (isaltok(*(textptr+j)))
                {
                    tempbuf[j+1] = textptr[j];
                    j++;
                }
                tempbuf[j+1] = '\0';

                if (map[(k*MAXLEVELS)+i].musicfn == NULL)
                    map[(k*MAXLEVELS)+i].musicfn = Bcalloc(Bstrlen(tempbuf)+1,sizeof(char));
                else if ((Bstrlen(tempbuf)+1) > sizeof(map[(k*MAXLEVELS)+i].musicfn))
                    map[(k*MAXLEVELS)+i].musicfn = Brealloc(map[(k*MAXLEVELS)+i].musicfn,(Bstrlen(tempbuf)+1));

                Bstrcpy(map[(k*MAXLEVELS)+i].musicfn,tempbuf);

                textptr += j;
                if (i > MAXLEVELS-1) break;
                i++;
            }
        }
        else
        {
            i = 0;
            while (keyword() == -1)
            {
                while (isaltok(*textptr) == 0)
                {
                    if (*textptr == 0x0a) line_number++;
                    textptr++;
                    if (*textptr == 0) break;
                }
                j = 0;

                while (isaltok(*(textptr+j)))
                {
                    env_music_fn[i][j] = textptr[j];
                    j++;
                }
                env_music_fn[i][j] = '\0';

                textptr += j;
                if (i > MAXVOLUMES-1) break;
                i++;
            }
        }
    }
    return 0;

    case CON_INCLUDE:
        scriptptr--;
        while (isaltok(*textptr) == 0)
        {
            if (*textptr == 0x0a) line_number++;
            textptr++;
            if (*textptr == 0) break;
        }
        j = 0;
        while (isaltok(*textptr))
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';

        {
            int temp_line_number;
            int  temp_ifelse_check;
            char *origtptr, *mptr;
            char parentcompilefile[255];
            int fp;

            fp = kopen4load(tempbuf,loadfromgrouponly);
            if (fp < 0)
            {
                error++;
                initprintf("%s:%d: error: could not find file `%s'.\n",compilefile,line_number,tempbuf);
                return 0;
            }

            j = kfilelength(fp);

            mptr = (char *)Bmalloc(j+1);
            if (!mptr)
            {
                kclose(fp);
                error++;
                initprintf("%s:%d: error: could not allocate %d bytes to include `%s'.\n",
                           line_number,compilefile,j,tempbuf);
                return 0;
            }

            initprintf("Including: %s (%d bytes)\n",tempbuf, j);
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
            do done = parsecommand();
            while (!done);

            Bstrcpy(compilefile, parentcompilefile);
            total_lines += line_number;
            line_number = temp_line_number;
            checking_ifelse = temp_ifelse_check;

            textptr = origtptr;

            Bfree(mptr);
        }
        return 0;

    case CON_AI:
        if (parsing_actor || parsing_state)
        {
            if (!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transnum(LABEL_AI);
        }
        else
        {
            scriptptr--;
            getlabel();

            for (i=0;i<NUMKEYWORDS;i++)
                if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }

            for (i=0;i<labelcnt;i++)
                if (Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 /* && (labeltype[i] & LABEL_AI) */)
                {
                    warning++;
                    initprintf("%s:%d: warning: duplicate ai `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }

            if (i == labelcnt)
            {
                labeltype[labelcnt] = LABEL_AI;
                labelcode[labelcnt++] = (intptr_t) scriptptr;
            }

            for (j=0;j<3;j++)
            {
                if (keyword() >= 0) break;
                if (j == 1)
                    transnum(LABEL_ACTION);
                else if (j == 2)
                {
                    if ((transnum(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(scriptptr-1) != 0) && (*(scriptptr-1) != 1))
                    {
                        ReportError(-1);
                        bitptr[(scriptptr-script-1)] = BITPTR_DONTFUCKWITHIT;
                        *(scriptptr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",compilefile,line_number);
                    }
                    k = 0;
                    while (keyword() == -1)
                    {
                        transnum(LABEL_DEFINE);
                        scriptptr--;
                        k |= *scriptptr;
                    }
                    bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                    *scriptptr = k;
                    scriptptr++;
                    return 0;
                }
            }
            for (k=j;k<3;k++)
            {
                bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                *scriptptr = 0;
                scriptptr++;
            }
        }
        return 0;

    case CON_ACTION:
        if (parsing_actor || parsing_state)
        {
            if (!CheckEventSync(current_event))
                ReportError(WARNING_EVENTSYNC);
            transnum(LABEL_ACTION);
        }
        else
        {
            scriptptr--;
            getlabel();
            // Check to see it's already defined

            for (i=NUMKEYWORDS-1;i>=0;i--)
                if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
                {
                    error++;
                    ReportError(ERROR_ISAKEYWORD);
                    return 0;
                }
            for (i=labelcnt-1;i>=0;i--)
                if (Bstrcmp(label+(labelcnt<<6),label+(i<<6)) == 0 /* && (labeltype[i] & LABEL_ACTION) */)
                {
                    warning++;
                    initprintf("%s:%d: warning: duplicate action `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
                    break;
                }

            if (i == -1)
            {
                labeltype[labelcnt] = LABEL_ACTION;
                labelcode[labelcnt] = (intptr_t) scriptptr;
                labelcnt++;
            }

            for (j=0;j<5;j++)
            {
                if (keyword() >= 0) break;
                transnum(LABEL_DEFINE);
            }
            for (k=j;k<5;k++)
            {
                bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                *scriptptr = 0;
                scriptptr++;
            }
        }
        return 0;

    case CON_ACTOR:
        if (parsing_state || parsing_actor)
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_actor = scriptptr;

        skipcomments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;
        transnum(LABEL_DEFINE);
        //         Bsprintf(parsing_item_name,"%s",label+(labelcnt<<6));
        scriptptr--;
        actorscrptr[*scriptptr] = parsing_actor;

        for (j=0;j<4;j++)
        {
            *(parsing_actor+j) = 0;
            if (j == 3)
            {
                j = 0;
                while (keyword() == -1)
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                *scriptptr = j;
                scriptptr++;
                break;
            }
            else
            {
                if (keyword() >= 0)
                {
                    for (i=4-j; i; i--)
                    {
                        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                        *(scriptptr++) = 0;
                    }
                    break;
                }
                switch (j)
                {
                case 0:
                    transnum(LABEL_DEFINE);
                    break;
                case 1:
                    transnum(LABEL_ACTION);
                    break;
                case 2:
                    if ((transnum(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(scriptptr-1) != 0) && (*(scriptptr-1) != 1))
                    {
                        ReportError(-1);
                        bitptr[(scriptptr-script-1)] = BITPTR_DONTFUCKWITHIT;
                        *(scriptptr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",compilefile,line_number);
                    }
                    break;
                }
                *(parsing_actor+j) = *(scriptptr-1);
            }
        }
        checking_ifelse = 0;
        return 0;

    case CON_ONEVENT:
        if (parsing_state || parsing_actor)
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
        while (isaltok(*(textptr+j)))
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;
//        labelsonly = 1;
        transnum(LABEL_DEFINE);
        labelsonly = 0;
        scriptptr--;
        j= *scriptptr;  // type of event
        current_event = j;
        //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, parsing_event);
        //AddLog(g_szBuf);
        if (j > MAXGAMEEVENTS-1 || j < 0)
        {
            initprintf("%s:%d: error: invalid event ID.\n",compilefile,line_number);
            error++;
            return 0;
        }

        if (apScriptGameEvent[j])
        {
            tempscrptr = parsing_event;
            parsing_event = parsing_actor = 0;
            ReportError(-1);
            parsing_event = parsing_actor = tempscrptr;
            initprintf("%s:%d: warning: duplicate event `%s'.\n",compilefile,line_number,parsing_item_name);
        }
        else apScriptGameEvent[j]=parsing_event;

        checking_ifelse = 0;

        return 0;

    case CON_EVENTLOADACTOR:
        if (parsing_state || parsing_actor)
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        num_braces = 0;
        scriptptr--;
        parsing_actor = scriptptr;

        skipcomments();
        j = 0;
        while (isaltok(*(textptr+j)))
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
        if (parsing_state || parsing_actor)
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
        while (isaltok(*(textptr+j)))
        {
            parsing_item_name[j] = textptr[j];
            j++;
        }
        parsing_item_name[j] = 0;

        j = *scriptptr;

        if (j > 2)
        {
            ReportError(-1);
            initprintf("%s:%d: warning: invalid useractor type.\n",compilefile,line_number);
            j = 0;
        }

        transnum(LABEL_DEFINE);
        scriptptr--;
        actorscrptr[*scriptptr] = parsing_actor;
        actortype[*scriptptr] = j;

        for (j=0;j<4;j++)
        {
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *(parsing_actor+j) = 0;
            if (j == 3)
            {
                j = 0;
                while (keyword() == -1)
                {
                    transnum(LABEL_DEFINE);
                    scriptptr--;
                    j |= *scriptptr;
                }
                bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                *scriptptr = j;
                scriptptr++;
                break;
            }
            else
            {
                if (keyword() >= 0)
                {
                    for (i=4-j; i; i--)
                    {
                        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
                        *(scriptptr++) = 0;
                    }
                    break;
                }
                switch (j)
                {
                case 0:
                    transnum(LABEL_DEFINE);
                    break;
                case 1:
                    transnum(LABEL_ACTION);
                    break;
                case 2:
                    if ((transnum(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(scriptptr-1) != 0) && (*(scriptptr-1) != 1))
                    {
                        ReportError(-1);
                        bitptr[(scriptptr-script-1)] = BITPTR_DONTFUCKWITHIT;
                        *(scriptptr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",compilefile,line_number);
                    }
                    break;
                }
                bitptr[(parsing_actor+j-script)] = BITPTR_DONTFUCKWITHIT;
                *(parsing_actor+j) = *(scriptptr-1);
            }
        }
        checking_ifelse = 0;
        return 0;

    case CON_INSERTSPRITEQ:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        return 0;

    case CON_QSPRINTF:
        transnum(LABEL_DEFINE);
        transnum(LABEL_DEFINE);
        for (j = 0;j < 4;j++)
        {
            if (keyword() == -1)
                transvar();
            else break;
        }

        while (j < 4)
        {
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_ANGOFF:
    case CON_QUOTE:
    case CON_SOUND:
    case CON_GLOBALSOUND:
    case CON_SOUNDONCE:
    case CON_STOPSOUND:
        transnum(LABEL_DEFINE);
        if (tw == CON_CSTAT)
        {
            if (*(scriptptr-1) == 32767)
            {
                *(scriptptr-1) = 32768;
                ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat 32767, using 32768 instead.\n",compilefile,line_number);
            }
            else if ((*(scriptptr-1) & 32) && (*(scriptptr-1) & 16))
            {
                i = *(scriptptr-1);
                *(scriptptr-1) ^= 48;
                ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat %d, using %d instead.\n",compilefile,line_number,i,*(scriptptr-1));
            }
        }
        return 0;

    case CON_HITRADIUSVAR:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transmultvars(5);
        break;
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transnum(LABEL_DEFINE);
        transnum(LABEL_DEFINE);
        break;

    case CON_ELSE:
        if (checking_ifelse)
        {
            intptr_t offset;
            checking_ifelse--;
            tempscrptr = scriptptr;
            offset = (unsigned)(tempscrptr-script);
            scriptptr++; //Leave a spot for the fail location
            parsecommand();
            tempscrptr = (intptr_t *)script+offset;
            *tempscrptr = (intptr_t) scriptptr;
            bitptr[(tempscrptr-script)] = 1;
        }
        else
        {
            scriptptr--;
            error++;
            ReportError(-1);
            initprintf("%s:%d: error: found `else' with no `if'.\n",compilefile,line_number);
        }
        return 0;

    case CON_SETSECTOR:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETSECTOR:
    {
        int lLabelID;

        // syntax getsector[<var>].x <VAR>
        // gets the value of sector[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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

        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
    case CON_FINDNEARACTORZ:
    case CON_FINDNEARSPRITEZ:
    {
        // syntax findnearactor <type> <maxdist> <getvar>
        // gets the sprite ID of the nearest actor within max dist
        // that is of <type> into <getvar>
        // -1 for none found

        transnum(LABEL_DEFINE); // get <type>
        transnum(LABEL_DEFINE); // get maxdist

        switch (tw)
        {
        case CON_FINDNEARACTORZ:
        case CON_FINDNEARSPRITEZ:
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
    case CON_FINDNEARACTORZVAR:
    case CON_FINDNEARSPRITEZVAR:
    {
        transnum(LABEL_DEFINE); // get <type>

        // get the ID of the DEF
        transvar();
        switch (tw)
        {
        case CON_FINDNEARACTORZVAR:
        case CON_FINDNEARSPRITEZVAR:
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETWALL:
    {
        int lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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

        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETPLAYER:
    {
        int lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETINPUT:
    {
        int lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        int lLabelID;

        // syntax [gs]etuserdef.x <VAR>
        // gets the value of ud.xxx into <VAR>

        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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

        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETACTORVAR:
    case CON_GETPLAYERVAR:
    {
        // syntax [gs]etactorvar[<var>].<varx> <VAR>
        // gets the value of the per-actor variable varx into VAR

        // now get name of <var>
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .<varx>
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
        {
            error++;
            ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;

        if (scriptptr[-1] == g_iThisActorID) // convert to "setvarvar"
        {
            scriptptr--;
            scriptptr[-1]=CON_SETVARVAR;
            if (tw == CON_SETACTORVAR || tw == CON_SETPLAYERVAR)
            {
                transvartype(GAMEVAR_FLAG_READONLY);
                transvar();
            }
            else
            {
                scriptptr++;
                transvar();
                scriptptr-=2;
                transvartype(GAMEVAR_FLAG_READONLY);
                scriptptr++;
            }
            break;
        }

        /// now pointing at 'xxx'

        // get the ID of the DEF
        getlabel();
        //printf("found label of '%s'\n",   label+(labelcnt<<6));

        // Check to see if it's a keyword
        for (i=0;i<NUMKEYWORDS;i++)
            if (Bstrcmp(label+(labelcnt<<6),keyw[i]) == 0)
            {
                error++;
                ReportError(ERROR_ISAKEYWORD);
                return 0;
            }

        i=GetDefID(label+(labelcnt<<6));
        //printf("Label '%s' ID is %d\n",label+(labelcnt<<6), i);
        if (i<0)
        {
            // not a defined DEF
            error++;
            ReportError(ERROR_NOTAGAMEVAR);
            return 0;
        }
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
        {
            error++;
            ReportError(ERROR_VARREADONLY);
            return 0;

        }

        switch (tw)
        {
        case CON_SETACTORVAR:
        {
            if (!(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR))
            {
                error++;
                ReportError(-1);
                initprintf("%s:%d: error: variable `%s' is not per-actor.\n",compilefile,line_number,label+(labelcnt<<6));
                return 0;

            }
            break;
        }
        case CON_SETPLAYERVAR:
        {
            if (!(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER))
            {
                error++;
                ReportError(-1);
                initprintf("%s:%d: error: variable `%s' is not per-player.\n",compilefile,line_number,label+(labelcnt<<6));
                return 0;

            }
            break;
        }
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=i; // the ID of the DEF (offset into array...)

        switch (tw)
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETACTOR:
    {
        int lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
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
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        if (tw == CON_GETACTOR)
            transvartype(GAMEVAR_FLAG_READONLY);
        else
            transvar();
        break;
    }

    case CON_GETTSPR:
    case CON_SETTSPR:
    {
        int lLabelID;

        if (current_event != EVENT_ANIMATESPRITES)
        {
            ReportError(-1);
            initprintf("%s:%d: warning: found `%s' outside of EVENT_ANIMATESPRITES\n",compilefile,line_number,tempbuf);
        }

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
        {
            textptr++;
        }
        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        labelsonly = 1;
        transvar();
        labelsonly = 0;
        // now get name of .xxx
        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }
        if (*textptr!='.')
        {
            error++;
            ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        getlabel();
        //printf("found xxx label of '%s'\n",   label+(labelcnt<<6));

        lLabelID=getlabeloffset(tsprlabels,label+(labelcnt<<6));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            error++;
            ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=tsprlabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",actorlabels[lLabelID].flags);

        // now at target VAR...

        // get the ID of the DEF
        if (tw == CON_GETTSPR)
            transvartype(GAMEVAR_FLAG_READONLY);
        else
            transvar();
        break;
    }

    case CON_GETTICKS:
        if (CheckEventSync(current_event))
            ReportError(WARNING_REVEVENTSYNC);
    case CON_GETCURRADDRESS:
        transvartype(GAMEVAR_FLAG_READONLY);
        return 0;

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
    case CON_QUAKE:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_JUMP:
    case CON_CMENU:
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
    case CON_USERQUOTE:
    case CON_STARTTRACKVAR:
    case CON_CLEARMAPSTATE:
    case CON_ACTIVATECHEAT:
    case CON_SETGAMEPALETTE:
        transvar();
        return 0;

    case CON_ENHANCED:
    {
        // don't store in pCode...
        scriptptr--;
        //printf("We are enhanced, baby...\n");
        transnum(LABEL_DEFINE);
        scriptptr--;
        if (*scriptptr > BYTEVERSION_JF)
        {
            warning++;
            initprintf("%s:%d: warning: need build %d, found build %d\n",compilefile,line_number,k,BYTEVERSION_JF);
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
    case CON_EZSHOOT:
        if (!CheckEventSync(current_event))
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
        //printf("Found [add|set]var at line= %d\n",line_number);

        // get the ID of the DEF
        if (tw != CON_ZSHOOT && tw != CON_EZSHOOT)
            transvartype(GAMEVAR_FLAG_READONLY);
        else transvar();

        transnum(LABEL_DEFINE); // the number to check against...
        return 0;
    case CON_SETARRAY:
        getlabel();
        i=GetADefID(label+(labelcnt<<6));
        if (i > (-1))
        {
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr++=i;
        }
        else
            ReportError(ERROR_NOTAGAMEARRAY);
        skipcomments();// skip comments and whitespace
        if (*textptr != '[')
        {
            error++;
            ReportError(ERROR_GAMEARRAYBNO);
            return 1;
        }
        textptr++;
        transvar();
        skipcomments();// skip comments and whitespace
        if (*textptr != ']')
        {
            error++;
            ReportError(ERROR_GAMEARRAYBNC);
            return 1;
        }
        textptr++;
        transvar();
        return 0;
    case CON_RESIZEARRAY:
        getlabel();
        i=GetADefID(label+(labelcnt<<6));
        if (i > (-1))
        {
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr++=i;
        }
        else
            ReportError(ERROR_NOTAGAMEARRAY);
        skipcomments();
        transvar();
        return 0;
    case CON_RANDVARVAR:
        if (!CheckEventSync(current_event))
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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GMAXAMMO:
    case CON_DIST:
    case CON_LDIST:
    case CON_GETINCANGLE:
    case CON_GETANGLE:
    case CON_MULSCALE:
    case CON_SETASPECT:
        // get the ID of the DEF
        switch (tw)
        {
        case CON_DIST:
        case CON_LDIST:
        case CON_GETANGLE:
        case CON_GETINCANGLE:
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

        switch (tw)
        {
        case CON_DIST:
        case CON_LDIST:
        case CON_GETANGLE:
        case CON_GETINCANGLE:
            transvar();
            break;
        case CON_MULSCALE:
            transmultvars(2);
            break;
        }
        return 0;

    case CON_FLASH:
    case CON_SAVEMAPSTATE:
    case CON_LOADMAPSTATE:
        return 0;

    case CON_DRAGPOINT:
    case CON_GETKEYNAME:
        transmultvars(3);
        return 0;

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        transmultvars(3);
        transvartype(GAMEVAR_FLAG_READONLY);
        return 0;

    case CON_DEFINEPROJECTILE:
    {
        int y;
        signed int z;

        if (parsing_state || parsing_actor)
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        scriptptr--;

        transnum(LABEL_DEFINE);
        j = *(scriptptr-1);

        if (j > MAXTILES-1)
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
        if (parsing_actor == 0 && parsing_state == 0)
        {
            scriptptr--;

            transnum(LABEL_DEFINE);
            scriptptr--;
            j = *scriptptr;

            if (j > MAXTILES-1)
            {
                ReportError(ERROR_EXCEEDSMAXTILES);
                error++;
            }

            transnum(LABEL_DEFINE);
            scriptptr--;
            spriteflags[j] = *scriptptr;

            return 0;
        }
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transvar();
        return 0;
    }

    case CON_SPRITESHADOW:
    case CON_SPRITENVG:
    case CON_SPRITENOSHADE:
    case CON_SPRITENOPAL:
    case CON_PRECACHE:
    {
        if (parsing_state || parsing_actor)
        {
            ReportError(ERROR_FOUNDWITHIN);
            error++;
        }

        scriptptr--;

        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;

        if (j > MAXTILES-1)
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
        case CON_SPRITENOPAL:
            spriteflags[*scriptptr] |= SPRITE_FLAG_NOPAL;
            break;
        case CON_PRECACHE:
            spritecache[*scriptptr][0] = j;
            transnum(LABEL_DEFINE);
            scriptptr--;
            i = *scriptptr;
            if (i > MAXTILES-1)
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
    {
        intptr_t offset;

        transmultvars(2);
        tempscrptr = scriptptr;
        offset = (unsigned)(scriptptr-script);
        scriptptr++; // Leave a spot for the fail location

        j = keyword();
        parsecommand();

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) scriptptr;
        bitptr[(tempscrptr-script)] = 1;

        if (tw != CON_WHILEVARVARN) checking_ifelse++;
        return 0;
    }

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
    {
        intptr_t offset;

        // get the ID of the DEF
        transvar();
        transnum(LABEL_DEFINE); // the number to check against...

        tempscrptr = scriptptr;
        offset = (unsigned)(tempscrptr-script);
        scriptptr++; //Leave a spot for the fail location

        j = keyword();
        parsecommand();

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) scriptptr;
        bitptr[(tempscrptr-script)] = 1;

        if (tw != CON_WHILEVARN) checking_ifelse++;
        return 0;
    }
    case CON_ADDLOGVAR:

        // syntax: addlogvar <var>

        // prints the line number in the log file.
        /*        *scriptptr=line_number;
                scriptptr++; */

        // get the ID of the DEF
        transvar();
        return 0;

    case CON_ROTATESPRITE16:
    case CON_ROTATESPRITE:
        if (parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        // syntax:
        // int x, int y, int z, short a, short tilenum, signed char shade, char orientation, x1, y1, x2, y2
        // myospal adds char pal

        // get the ID of the DEFs

        transmultvars(12);
        break;

    case CON_SHOWVIEW:
        if (parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        transmultvars(10);
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
        if (tw==CON_HITSCAN) transvar();
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

    case CON_GETTIMEDATE:
        transmultvarstype(GAMEVAR_FLAG_READONLY,8);
        break;

    case CON_MOVESPRITE:
    case CON_SETSPRITE:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
        transmultvars(4);
        if (tw == CON_MOVESPRITE)
        {
            transvar();
            transvartype(GAMEVAR_FLAG_READONLY);
        }
        break;

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_GAMETEXTZ:
    case CON_DIGITALNUMBER:
    case CON_DIGITALNUMBERZ:
        if (parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        switch (tw)
        {
        case CON_GAMETEXTZ:
        case CON_DIGITALNUMBERZ:
            transmultvars(1);
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
        if (parsing_event == 0 && parsing_state == 0)
        {
            ReportError(ERROR_EVENTONLY);
            error++;
        }

        // syntax:
        // int x, int y, short tilenum, signed char shade, char orientation
        // myospal adds char pal

        transmultvars(5);
        if (tw==CON_MYOSPAL || tw==CON_MYOSPALX)
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
    {
        intptr_t tempoffset;

        //AddLog("Got Switch statement");
        if (checking_switch)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        checking_switch++; // allow nesting (if other things work)
        // Get The ID of the DEF
        transvar();

        tempscrptr= scriptptr;
        tempoffset = (unsigned)(tempscrptr-script);
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=0; // leave spot for end location (for after processing)
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=0; // count of case statements
        casescriptptr=scriptptr;        // the first case's pointer.
        bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
        *scriptptr++=0; // leave spot for 'default' location (null if none)

        j = keyword();
        temptextptr=textptr;
        // probably does not allow nesting...

        //AddLog("Counting Case Statements...");

        j=CountCaseStatements();
//        initprintf("Done Counting Case Statements for switch %d: found %d.\n", checking_switch,j);
        scriptptr+=j*2;
        skipcomments();
        scriptptr-=j*2; // allocate buffer for the table
        tempscrptr = (intptr_t *)(script+tempoffset);

        //AddLog(g_szBuf);
        if (checking_switch>1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        if (j<0)
        {
            return 1;
        }
        if (tempscrptr)
        {
            tempscrptr[1]=(intptr_t)j;  // save count of cases
        }
        else
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
        }

        while (j--)
        {
            // leave room for statements
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr++=0; // value check
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr++=0; // code offset
            skipcomments();
        }

        //Bsprintf(g_szBuf,"SWITCH1: '%.22s'",textptr);
        //AddLog(g_szBuf);

        casecount=0;
        while (parsecommand() == 0)
        {
            //Bsprintf(g_szBuf,"SWITCH2: '%.22s'",textptr);
            //AddLog(g_szBuf);
        }
        tempscrptr = (intptr_t *)(script+tempoffset);

        //Bsprintf(g_szBuf,"SWITCHXX: '%.22s'",textptr);
        //AddLog(g_szBuf);
        // done processing switch.  clean up.
        if (checking_switch<1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=%d",__FILE__,__LINE__, checking_switch);
            //    AddLog(g_szBuf);
        }
        casecount=0;
        if (tempscrptr)
        {
            intptr_t t,n;
            for (i=3;i<3+tempscrptr[1]*2-2;i+=2) // sort them
            {
                t=tempscrptr[i];n=i;
                for (j=i+2;j<3+tempscrptr[1]*2;j+=2)
                    if (tempscrptr[j]<t) {t=tempscrptr[j];n=j;}
                if (n!=i)
                {
                    t=tempscrptr[i  ];tempscrptr[i  ]=tempscrptr[n  ];tempscrptr[n  ]=t;
                    t=tempscrptr[i+1];tempscrptr[i+1]=tempscrptr[n+1];tempscrptr[n+1]=t;
                }
            }
//            for (j=3;j<3+tempscrptr[1]*2;j+=2)initprintf("%5d %8x\n",tempscrptr[j],tempscrptr[j+1]);
            tempscrptr[0]= (intptr_t)scriptptr - (intptr_t)&script[0];    // save 'end' location
//            bitptr[(tempscrptr-script)] = 1;
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
        if (checking_switch)
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d: Checking_switch=%d",__FILE__,__LINE__, checking_switch);
            //AddLog(g_szBuf);
        }
        //AddLog("End of Switch statement");
    }
    break;

    case CON_CASE:
    {
        intptr_t tempoffset;
        //AddLog("Found Case");
repeatcase:
        scriptptr--; // don't save in code
        if (checking_switch<1)
        {
            error++;
            ReportError(-1);
            initprintf("%s:%d: error: found `case' statement when not in switch\n",compilefile,line_number);
            return 1;
        }
        casecount++;
        //Bsprintf(g_szBuf,"case1: %.12s",textptr);
        //AddLog(g_szBuf);
        transnum(LABEL_DEFINE);
        if (*textptr == ':')
            textptr++;
        //Bsprintf(g_szBuf,"case2: %.12s",textptr);
        //AddLog(g_szBuf);

        j=*(--scriptptr);      // get value
        //Bsprintf(g_szBuf,"case: Value of case %d is %d",(int)casecount,(int)j);
        //AddLog(g_szBuf);
        if (casescriptptr)
        {
            for (i=0;i<casecount/2;i++)
                if (casescriptptr[i*2+1]==j)
                {
                    //warning++;
                    ReportError(WARNING_DUPLICATECASE);
                    break;
                }
            //AddLog("Adding value to script");
            casescriptptr[casecount++]=j;   // save value
            casescriptptr[casecount]=(intptr_t)((intptr_t*)scriptptr-&script[0]);   // save offset
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
        tempoffset = (unsigned)(tempscrptr-script);
        while (parsecommand() == 0)
        {
            //Bsprintf(g_szBuf,"case5 '%.25s'",textptr);
            //AddLog(g_szBuf);
            j = keyword();
            if (j == CON_CASE)
            {
                //AddLog("Found Repeat Case");
                transword();    // eat 'case'
                tempscrptr = (intptr_t *)(script+tempoffset);
                goto repeatcase;
            }
        }
        tempscrptr = (intptr_t *)(script+tempoffset);
        //AddLog("End Case");
        return 0;
        //      break;
    }
    case CON_DEFAULT:
        scriptptr--;    // don't save
        if (checking_switch<1)
        {
            error++;
            ReportError(-1);
            initprintf("%s:%d: error: found `default' statement when not in switch\n",compilefile,line_number);
            return 1;
        }
        if (casescriptptr && casescriptptr[0]!=0)
        {
            // duplicate default statement
            error++;
            ReportError(-1);
            initprintf("%s:%d: error: multiple `default' statements found in switch\n",compilefile,line_number);
        }
        if (casescriptptr)
        {
            casescriptptr[0]=(intptr_t)(scriptptr-&script[0]);   // save offset
//            bitptr[(casescriptptr-script)] = 1;
        }
        //Bsprintf(g_szBuf,"default: '%.22s'",textptr);
        //AddLog(g_szBuf);
        while (parsecommand() == 0)
        {
            //Bsprintf(g_szBuf,"defaultParse: '%.22s'",textptr);
            //AddLog(g_szBuf);
            ;
        }
        break;

    case CON_ENDSWITCH:
        //AddLog("End Switch");
        checking_switch--;
        if (checking_switch < 0)
        {
            error++;
            ReportError(-1);
            initprintf("%s:%d: error: found `endswitch' without matching `switch'\n",compilefile,line_number);
        }
        return 1;      // end of block
        break;

    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
    case CON_ZSHOOTVAR:
    case CON_EZSHOOTVAR:
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_GETPNAME:
    case CON_STARTLEVEL:
    case CON_QSTRCAT:
    case CON_QSTRCPY:
    case CON_QSTRLEN:
    case CON_QGETSYSSTR:
    case CON_HEADSPRITESTAT:
    case CON_PREVSPRITESTAT:
    case CON_NEXTSPRITESTAT:
    case CON_HEADSPRITESECT:
    case CON_PREVSPRITESECT:
    case CON_NEXTSPRITESECT:
        transmultvars(2);
        return 0;
    case CON_QSUBSTR:
        transmultvars(4);
        return 0;
    case CON_SETACTORANGLE:
    case CON_SETPLAYERANGLE:
        if (!CheckEventSync(current_event))
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
        /*        *scriptptr=line_number;
                scriptptr++; */
        return 0;

    case CON_IFPINVENTORY:
    case CON_IFRND:
        if (!CheckEventSync(current_event))
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
        switch (tw)
        {
        case CON_IFAI:
            transnum(LABEL_AI);
            break;
        case CON_IFACTION:
            transnum(LABEL_ACTION);
            break;
        case CON_IFMOVE:
            if ((transnum(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(scriptptr-1) != 0) && (*(scriptptr-1) != 1))
            {
                ReportError(-1);
                *(scriptptr-1) = 0;
                initprintf("%s:%d: warning: expected a move, found a constant.\n",compilefile,line_number);
            }
            break;
        case CON_IFPINVENTORY:
            transnum(LABEL_DEFINE);
            transnum(LABEL_DEFINE);
            break;
        case CON_IFSOUND:
            if (CheckEventSync(current_event))
                ReportError(WARNING_REVEVENTSYNC);
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
    {
        intptr_t offset;
        if (tw == CON_IFP)
        {
            j = 0;
            do
            {
                transnum(LABEL_DEFINE);
                scriptptr--;
                j |= *scriptptr;
            }
            while (keyword() == -1);
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
            *scriptptr = j;
            scriptptr++;
        }

        tempscrptr = scriptptr;
        offset = (unsigned)(tempscrptr-script);

        scriptptr++; //Leave a spot for the fail location

        j = keyword();
        parsecommand();
        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) scriptptr;
        bitptr[(tempscrptr-script)] = 1;

        checking_ifelse++;
        return 0;
    }
    case CON_LEFTBRACE:
        if (!(parsing_state || parsing_actor || parsing_event))
        {
            error++;
            ReportError(ERROR_SYNTAXERROR);
        }
        num_braces++;
        do
            done = parsecommand();
        while (done == 0);
        return 0;

    case CON_RIGHTBRACE:
        num_braces--;
        if (num_braces < 0)
        {
            if (checking_switch)
            {
                ReportError(ERROR_NOENDSWITCH);
            }

            ReportError(-1);
            initprintf("%s:%d: error: found more `}' than `{'.\n",compilefile,line_number);
            error++;
        }
        return 1;

    case CON_BETANAME:
        scriptptr--;
        j = 0;
        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            textptr++;
        return 0;

    case CON_DEFINEVOLUMENAME:
        scriptptr--;

        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;
        while (*textptr == ' ' || *textptr == '\t') textptr++;

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            volume_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(volume_names[j])-1)
            {
                initprintf("%s:%d: error: volume name exceeds limit of %d characters.\n",compilefile,line_number,sizeof(volume_names[j])-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
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
        while (*textptr == ' ' || *textptr == '\t') textptr++;

        if (j < 0 || j > NUMGAMEFUNCTIONS-1)
        {
            initprintf("%s:%d: error: function number exceeds number of game functions.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            gamefunctions[j][i] = *textptr;
            keydefaults[j*3][i] = *textptr;
            textptr++,i++;
            if (*textptr == '/' || *textptr == ' ')
            {
                initprintf("%s:%d: warning: invalid character in function name.\n",compilefile,line_number);
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
            if (i >= MAXGAMEFUNCLEN-1)
            {
                initprintf("%s:%d: warning: function name exceeds limit of %d characters.\n",compilefile,line_number,MAXGAMEFUNCLEN);
//                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
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
        while (*textptr == ' ' || *textptr == '\t') textptr++;

        if (j < 0 || j > 4)
        {
            initprintf("%s:%d: error: skill number exceeds maximum skill count.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            skill_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(skill_names[j])-1)
            {
                initprintf("%s:%d: error: skill name exceeds limit of %d characters.\n",compilefile,line_number,sizeof(skill_names[j])-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
        }
        skill_names[j][i] = '\0';
        return 0;

    case CON_DEFINEGAMENAME:
    {
        char gamename[32];
        scriptptr--;

        while (*textptr == ' ' || *textptr == '\t') textptr++;

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            gamename[i] = *textptr;
            textptr++,i++;
            if (i >= (signed)sizeof(gamename)-1)
            {
                initprintf("%s:%d: error: game name exceeds limit of %d characters.\n",compilefile,line_number,sizeof(gamename)-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
        }
        gamename[i] = '\0';
        duke3dgrpstring = Bstrdup(gamename);
        Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
        wm_setapptitle(tempbuf);
    }
    return 0;

    case CON_DEFINEGAMETYPE:
        scriptptr--;
        transnum(LABEL_DEFINE);
        scriptptr--;
        j = *scriptptr;

        transnum(LABEL_DEFINE);
        scriptptr--;
        gametype_flags[j] = *scriptptr;

        while (*textptr == ' ' || *textptr == '\t') textptr++;

        if (j < 0 || j > MAXGAMETYPES-1)
        {
            initprintf("%s:%d: error: gametype number exceeds maximum gametype count.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }
        num_gametypes = j+1;

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            gametype_names[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(gametype_names[j])-1)
            {
                initprintf("%s:%d: error: gametype name exceeds limit of %d characters.\n",compilefile,line_number,sizeof(gametype_names[j])-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
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
        while (*textptr == ' ' || *textptr == '\t') textptr++;

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }
        if (k < 0 || k > MAXLEVELS-1)
        {
            initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0) textptr++;
            break;
        }

        i = 0;

        tempbuf[i] = '/';

        while (*textptr != ' ' && *textptr != '\t' && *textptr != 0x0a)
        {
            tempbuf[i+1] = *textptr;
            textptr++,i++;
            if (i >= BMAX_PATH)
            {
                initprintf("%s:%d: error: level file name exceeds limit of %d characters.\n",compilefile,line_number,BMAX_PATH);
                error++;
                while (*textptr != ' ' && *textptr != '\t') textptr++;
                break;
            }
        }
        tempbuf[i+1] = '\0';

        Bcorrectfilename(tempbuf,0);

        if (map[j*MAXLEVELS+k].filename == NULL)
            map[j*MAXLEVELS+k].filename = Bcalloc(Bstrlen(tempbuf)+1,sizeof(char));
        else if ((Bstrlen(tempbuf)+1) > sizeof(map[j*MAXLEVELS+k].filename))
            map[j*MAXLEVELS+k].filename = Brealloc(map[j*MAXLEVELS+k].filename,(Bstrlen(tempbuf)+1));

        /*         initprintf("level file name string len: %d\n",Bstrlen(tempbuf)); */

        Bstrcpy(map[j*MAXLEVELS+k].filename,tempbuf);

        while (*textptr == ' ' || *textptr == '\t') textptr++;

        map[j*MAXLEVELS+k].partime =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

        textptr += 5;
        while (*textptr == ' '  || *textptr == '\t') textptr++;

        map[j*MAXLEVELS+k].designertime =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

        textptr += 5;
        while (*textptr == ' '  || *textptr == '\t') textptr++;

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            tempbuf[i] = Btoupper(*textptr);
            textptr++,i++;
            if (i >= 32)
            {
                initprintf("%s:%d: error: level name exceeds limit of %d characters.\n",compilefile,line_number,32);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
        }

        tempbuf[i] = '\0';

        if (map[j*MAXLEVELS+k].name == NULL)
            map[j*MAXLEVELS+k].name = Bcalloc(Bstrlen(tempbuf)+1,sizeof(char));
        else if ((Bstrlen(tempbuf)+1) > sizeof(map[j*MAXLEVELS+k].name))
            map[j*MAXLEVELS+k].name = Brealloc(map[j*MAXLEVELS+k].name,(Bstrlen(tempbuf)+1));

        /*         initprintf("level name string len: %d\n",Bstrlen(tempbuf)); */

        Bstrcpy(map[j*MAXLEVELS+k].name,tempbuf);

        return 0;

    case CON_DEFINEQUOTE:
    case CON_REDEFINEQUOTE:
        if (tw == CON_DEFINEQUOTE)
        {
            scriptptr--;
        }

        transnum(LABEL_DEFINE);

        k = *(scriptptr-1);

        if (k >= MAXQUOTES)
        {
            initprintf("%s:%d: error: quote number exceeds limit of %d.\n",compilefile,line_number,MAXQUOTES);
            error++;
        }

        if (fta_quotes[k] == NULL)
            fta_quotes[k] = Bcalloc(MAXQUOTELEN,sizeof(char));
        if (!fta_quotes[k])
        {
            fta_quotes[k] = NULL;
            Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(char) * MAXQUOTELEN);
            gameexit(tempbuf);
        }

        if (tw == CON_DEFINEQUOTE)
            scriptptr--;

        i = 0;

        while (*textptr == ' ' || *textptr == '\t')
            textptr++;

        if (tw == CON_REDEFINEQUOTE)
        {
            if (redefined_quotes[redefined_quote_count] == NULL)
                redefined_quotes[redefined_quote_count] = Bcalloc(MAXQUOTELEN,sizeof(char));
            if (!redefined_quotes[redefined_quote_count])
            {
                redefined_quotes[redefined_quote_count] = NULL;
                Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(char) * MAXQUOTELEN);
                gameexit(tempbuf);
            }
        }

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            if (*textptr == '%' && *(textptr+1) == 's')
            {
                initprintf("%s:%d: error: quote text contains string identifier.\n",compilefile,line_number);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
            if (tw == CON_DEFINEQUOTE)
                *(fta_quotes[k]+i) = *textptr;
            else
                *(redefined_quotes[redefined_quote_count]+i) = *textptr;
            textptr++,i++;
            if (i >= MAXQUOTELEN-1)
            {
                initprintf("%s:%d: error: quote text exceeds limit of %d characters.\n",compilefile,line_number,MAXQUOTELEN-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                break;
            }
        }
        if (tw == CON_DEFINEQUOTE)
            *(fta_quotes[k]+i) = '\0';
        else
        {
            *(redefined_quotes[redefined_quote_count]+i) = '\0';
            bitptr[(scriptptr-script)] = BITPTR_DONTFUCKWITHIT;
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
        scriptptr -= 2;
        return 0;

    case CON_DEFINECHEAT:
        scriptptr--;
        transnum(LABEL_DEFINE);
        k = *(scriptptr-1);

        if (k > 25)
        {
            initprintf("%s:%d: error: cheat redefinition attempts to redefine nonexistant cheat.\n",compilefile,line_number);
            error++;
            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
            break;
        }
        scriptptr--;
        i = 0;
        while (*textptr == ' ' || *textptr == '\t')
            textptr++;
        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ')
        {
            cheatstrings[k][i] = *textptr;
            textptr++,i++;
            if (i >= (signed)sizeof(cheatstrings[k])-1)
            {
                initprintf("%s:%d: error: cheat exceeds limit of %d characters.\n",compilefile,line_number,MAXCHEATLEN,sizeof(cheatstrings[k])-1);
                error++;
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ') textptr++;
                break;
            }
        }
        cheatstrings[k][i] = '\0';
        return 0;

    case CON_DEFINESOUND:
        scriptptr--;
        transnum(LABEL_DEFINE);
        k = *(scriptptr-1);
        if (k >= MAXSOUNDS)
        {
            initprintf("%s:%ld: error: exceeded sound limit of %ld.\n",compilefile,line_number,MAXSOUNDS);
            error++;
        }
        scriptptr--;
        i = 0;
        skipcomments();

        if (g_sounds[k].filename == NULL)
            g_sounds[k].filename = Bcalloc(BMAX_PATH,sizeof(char));
        if (!g_sounds[k].filename)
        {
            Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte buffer.",sizeof(char) * BMAX_PATH);
            gameexit(tempbuf);
        }

        while (*textptr != ' ' || *textptr == '\t')
        {
            g_sounds[k].filename[i] = *textptr;
            textptr++,i++;
            if (i >= BMAX_PATH)
            {
                initprintf("%s:%d: error: sound filename exceeds limit of %d characters.\n",compilefile,line_number,BMAX_PATH);
                error++;
                skipcomments();
                break;
            }
        }
        g_sounds[k].filename[i] = '\0';

        transnum(LABEL_DEFINE);
        g_sounds[k].ps = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        g_sounds[k].pe = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        g_sounds[k].pr = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        g_sounds[k].m = *(scriptptr-1);
        scriptptr--;
        transnum(LABEL_DEFINE);
        g_sounds[k].vo = *(scriptptr-1);
        scriptptr--;
        return 0;

    case CON_ENDEVENT:

        if (parsing_event == 0)
        {
            ReportError(-1);
            initprintf("%s:%d: error: found `endevent' without open `onevent'.\n",compilefile,line_number);
            error++;
        }
        if (num_braces > 0)
        {
            ReportError(ERROR_OPENBRACKET);
            error++;
        }
        if (num_braces < 0)
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
        if (parsing_actor == 0)
        {
            ReportError(-1);
            initprintf("%s:%d: error: found `enda' without open `actor'.\n",compilefile,line_number);
            error++;
        }
        if (num_braces > 0)
        {
            ReportError(ERROR_OPENBRACKET);
            error++;
        }
        if (num_braces < 0)
        {
            ReportError(ERROR_CLOSEBRACKET);
            error++;
        }
        parsing_actor = 0;
        Bsprintf(parsing_item_name,"(none)");
        return 0;

    case CON_BREAK:
        if (checking_switch)
        {
            //Bsprintf(g_szBuf,"  * (L%d) case Break statement.\n",line_number);
            //AddLog(g_szBuf);
            return 1;
        }
        return 0;

    case CON_SCRIPTSIZE:
        scriptptr--;
        transnum(LABEL_DEFINE);
        j = *(scriptptr-1);
        scriptptr--;
        skipcomments();
        return increasescriptsize(j);

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
        if (!CheckEventSync(current_event))
            ReportError(WARNING_EVENTSYNC);
    case CON_NULLOP:
    case CON_STOPALLSOUNDS:
        return 0;
    case CON_GAMESTARTUP:
    {
        int params[30];

        scriptptr--;
        for (j = 0; j < 30; j++)
        {
            transnum(LABEL_DEFINE);
            scriptptr--;
            params[j] = *scriptptr;

            if (j != 25) continue;

            if (keyword() != -1)
            {
//                initprintf("Duke Nukem 3D v1.3D style CON files detected.\n");
                break;
            }
            else
            {
                g_ScriptVersion = 14;
//                initprintf("Duke Nukem 3D v1.4+ style CON files detected.\n");
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
        g_player[0].ps->max_player_health = g_player[0].ps->max_shield_amount = params[j++];
        start_armour_amount = params[j++];
        respawnactortime = params[j++];
        respawnitemtime = params[j++];
        dukefriction = params[j++];
        if (g_ScriptVersion == 14) gc = params[j++];
        rpgblastradius = params[j++];
        pipebombblastradius = params[j++];
        shrinkerblastradius = params[j++];
        tripbombblastradius = params[j++];
        morterblastradius = params[j++];
        bouncemineblastradius = params[j++];
        seenineblastradius = params[j++];
        g_player[0].ps->max_ammo_amount[PISTOL_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[SHOTGUN_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[CHAINGUN_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[RPG_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[HANDBOMB_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[SHRINKER_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[TRIPBOMB_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[FREEZE_WEAPON] = params[j++];
        if (g_ScriptVersion == 14) g_player[0].ps->max_ammo_amount[GROW_WEAPON] = params[j++];
        camerashitable = params[j++];
        numfreezebounces = params[j++];
        freezerhurtowner = params[j++];
        if (g_ScriptVersion == 14)
        {
            spriteqamount = params[j++];
            if (spriteqamount > 1024) spriteqamount = 1024;
            else if (spriteqamount < 0) spriteqamount = 0;

            lasermode = params[j++];
        }
    }
    return 0;
    }
    return 0;
}

static void passone(void)
{
    while (parsecommand() == 0);

    if ((error+warning) > 63)
        initprintf("fatal error: too many warnings or errors: Aborted\n");

#ifdef DEBUG
    {
        int i=0;
        initprintf("Game Definitions\n");
        for (;i<iGameVarCount;i++)
        {
            initprintf("%20s\t%d\n",apszGameVarLabel[i],lGameVarValue[i]);
        }
    }
#endif
}

#define NUM_DEFAULT_CONS    4
static const char *defaultcons[NUM_DEFAULT_CONS] =
{
    "EDUKE.CON",
    "GAME.CON",
    "USER.CON",
    "DEFS.CON"
};

void copydefaultcons(void)
{
    int i, fs, fpi;
    FILE *fpo;

    for (i=0;i<NUM_DEFAULT_CONS;i++)
    {
        fpi = kopen4load((char *)defaultcons[i] , 1);
        if (fpi < 0) continue;

        fpo = fopenfrompath((char *)defaultcons[i],"wb");

        if (fpo == NULL)
        {
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

/* Anything added with AddDefinition cannot be overwritten in the CONs */

static void AddDefinition(const char *lLabel,int lValue,int lType)
{
    Bstrcpy(label+(labelcnt<<6),lLabel);
    labeltype[labelcnt] = lType;
    labelcode[labelcnt++] = lValue;
    defaultlabelcnt++;
}

static void AddDefaultDefinitions(void)
{
    AddDefinition("EVENT_AIMDOWN",EVENT_AIMDOWN,LABEL_DEFINE);
    AddDefinition("EVENT_AIMUP",EVENT_AIMUP,LABEL_DEFINE);
    AddDefinition("EVENT_ANIMATESPRITES",EVENT_ANIMATESPRITES,LABEL_DEFINE);
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
    AddDefinition("EVENT_DISPLAYBONUSSCREEN",EVENT_DISPLAYBONUSSCREEN,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYMENU",EVENT_DISPLAYMENU,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYMENUREST",EVENT_DISPLAYMENUREST,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYLOADINGSCREEN",EVENT_DISPLAYLOADINGSCREEN,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYROOMS",EVENT_DISPLAYROOMS,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYSBAR",EVENT_DISPLAYSBAR,LABEL_DEFINE);
    AddDefinition("EVENT_DISPLAYWEAPON",EVENT_DISPLAYWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_DOFIRE",EVENT_DOFIRE,LABEL_DEFINE);
    AddDefinition("EVENT_DRAWWEAPON",EVENT_DRAWWEAPON,LABEL_DEFINE);
    AddDefinition("EVENT_EGS",EVENT_EGS,LABEL_DEFINE);
    AddDefinition("EVENT_ENTERLEVEL",EVENT_ENTERLEVEL,LABEL_DEFINE);
    AddDefinition("EVENT_FAKEDOMOVETHINGS",EVENT_FAKEDOMOVETHINGS,LABEL_DEFINE);
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
    AddDefinition("EVENT_KILLIT",EVENT_KILLIT,LABEL_DEFINE);
    AddDefinition("EVENT_LOADACTOR",EVENT_LOADACTOR,LABEL_DEFINE);

    AddDefinition("STR_MAPNAME",STR_MAPNAME,LABEL_DEFINE);
    AddDefinition("STR_MAPFILENAME",STR_MAPFILENAME,LABEL_DEFINE);
    AddDefinition("STR_PLAYERNAME",STR_PLAYERNAME,LABEL_DEFINE);
    AddDefinition("STR_VERSION",STR_VERSION,LABEL_DEFINE);
    AddDefinition("STR_GAMETYPE",STR_GAMETYPE,LABEL_DEFINE);

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


static void InitProjectiles(void)
{
    int i;
    for (i=0;i<MAXTILES;i++)
    {
        projectile[i].workslike = 1;
        projectile[i].spawns = SMALLSMOKE;
        projectile[i].sxrepeat = projectile[i].syrepeat = -1;
        projectile[i].sound = projectile[i].isound = -1;
        projectile[i].vel = 600;
        projectile[i].extra = 100;
        projectile[i].decal = BULLETHOLE;
        projectile[i].trail = -1;
        projectile[i].tnum = 0;
        projectile[i].toffset = 1;
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
    }
    Bmemcpy(&defaultprojectile, &projectile, sizeof(projectile));
}

void loadefs(const char *filenam)
{
    char *mptr;
    int i;
    int fs,fp;

    clearbuf(apScriptGameEvent,MAXGAMEEVENTS,0L);

    InitGameVars();
    InitProjectiles();

    /* JBF 20040109: Don't prompt to extract CONs from GRP if they're missing.
     * If someone really wants them they can Kextract them.
    if(!SafeFileExists(filenam) && loadfromgrouponly == 0)
    {
        initprintf("Missing external CON file(s).\n");
        initprintf("COPY INTERNAL DEFAULTS TO DIRECTORY(Y/n)?\n");

    i=wm_ynbox("Missing CON file(s)", "Missing external CON file(s). "
    "Copy internal defaults to directory?");
    if (i) i = 'y';
        if(i == 'y' || i == 'Y')
        {
            initprintf(" Yes\n");
            copydefaultcons();
        }
    }
    */
    fp = kopen4load((char *)filenam,loadfromgrouponly);
    if (fp == -1) // JBF: was 0
    {
        if (loadfromgrouponly == 1)
            gameexit("Missing CON file(s); replace duke3d.grp with a known good copy.");
        else
        {
            Bsprintf(tempbuf,"CON file `%s' missing.", filenam);
            gameexit(tempbuf);
            return;
        }

        //loadfromgrouponly = 1;
        return; //Not there
    }

    fs = kfilelength(fp);

    initprintf("Compiling: %s (%d bytes)\n",filenam,fs);

    mptr = (char *)Bmalloc(fs+1);
    if (!mptr)
    {
        Bsprintf(tempbuf,"Failed allocating %d byte CON text buffer.", fs+1);
        gameexit(tempbuf);
    }

    mptr[fs] = 0;

    textptr = (char *) mptr;
    kread(fp,(char *)textptr,fs);
    kclose(fp);

    clearbuf(actorscrptr,MAXTILES,0L);  // JBF 20040531: MAXSPRITES? I think Todd meant MAXTILES...
    clearbuf(actorLoadEventScrptr,MAXTILES,0L); // I think this should be here...
    clearbufbyte(actortype,MAXTILES,0L);
//    clearbufbyte(script,sizeof(script),0l); // JBF 20040531: yes? no?
    if (script != NULL)
        Bfree(script);

    script = Bcalloc(1,g_ScriptSize * sizeof(intptr_t));
    bitptr = Bcalloc(1,g_ScriptSize * sizeof(char));
//    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    labelcnt = defaultlabelcnt = 0;
    scriptptr = script+1;
    warning = 0;
    error = 0;
    line_number = 1;
    total_lines = 0;

    AddDefaultDefinitions();

    Bstrcpy(compilefile, filenam);   // JBF 20031130: Store currently compiling file name
    passone(); //Tokenize
    //*script = (intptr_t) scriptptr;

    Bfree(mptr);
    mptr = NULL;

    if (warning|error)
        initprintf("Found %d warning(s), %d error(s).\n",warning,error);

    if (error == 0 && warning != 0)
    {
        if (groupfile != -1 && loadfromgrouponly == 0)
        {
            initprintf("Warning(s) found in file `%s'.  Do you want to use the INTERNAL DEFAULTS (y/N)?",filenam);

            i=wm_ynbox("CON File Compilation Warning", "Warning(s) found in file `%s'. Do you want to use the "
                       "INTERNAL DEFAULTS?",filenam);
            if (i) i = 'y';
            if (i == 'y' || i == 'Y')
            {
                loadfromgrouponly = 1;
                initprintf(" Yes\n");
                return;
            }
        }
    }

    if (error)
    {
        if (loadfromgrouponly)
        {
            sprintf(buf,"Error compiling CON files.");
            gameexit(buf);
        }
        else
        {
            if (groupfile != -1 && loadfromgrouponly == 0)
            {
//                initprintf("Error(s) found in file `%s'.  Do you want to use the INTERNAL DEFAULTS (y/N)?\n",filenam);

                i=wm_ynbox("CON File Compilation Error", "Error(s) found in file `%s'. Do you want to use the "
                           "INTERNAL DEFAULTS?",filenam);
                if (i) i = 'y';
                if (i == 'y' || i == 'Y')
                {
                    initprintf(" Yes\n");
                    loadfromgrouponly = 1;
                    return;
                }
                else
                {
#if (defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2))
                    while (!quitevent) // keep the window open so people can copy CON errors out of it
                        handleevents();
#endif
                    gameexit("");
                }
            }
        }
    }
    else
    {
        int j=0, k=0;

        total_lines += line_number;
        for (i=0;i<MAXGAMEEVENTS;i++)
        {
            if (apScriptGameEvent[i])
                j++;
        }
        for (i=0;i<MAXTILES;i++)
        {
            if (actorscrptr[i])
                k++;
        }

        initprintf("Compiled code size: %ld bytes, version %s\n",(unsigned)(scriptptr-script) * sizeof(intptr_t),(g_ScriptVersion == 14?"1.4+":"1.3D"));
        initprintf("%ld/%ld labels, %d/%d variables\n",labelcnt,min((MAXSECTORS * sizeof(sectortype)/sizeof(int)),(MAXSPRITES * sizeof(spritetype)/(1<<6))),iGameVarCount,MAXGAMEVARS);
        initprintf("%ld event definitions, %ld defined actors\n",j,k);

        for (i=0;i<128;i++)
            if (fta_quotes[i] == NULL)
                fta_quotes[i] = Bcalloc(MAXQUOTELEN,sizeof(char));

//        if (!Bstrcmp(fta_quotes[13],"PRESS SPACE TO RESTART LEVEL"))
//            Bstrcpy(fta_quotes[13],"PRESS USE TO RESTART LEVEL");

        for (i=0;i<MAXQUOTELEN-6;i++)
            if (Bstrncmp(&fta_quotes[13][i],"SPACE",5) == 0)
            {
                Bmemset(tempbuf,0,sizeof(tempbuf));
                Bstrncpy(tempbuf,fta_quotes[13],i);
                Bstrcat(tempbuf,"OPEN");
                Bstrcat(tempbuf,&fta_quotes[13][i+5]);
                Bstrncpy(fta_quotes[13],tempbuf,MAXQUOTELEN-1);
                i = 0;
            }

        {
            const char *ppdeathstrings[] =
            {
                "^2%s ^2WAS KICKED TO THE CURB BY %s",
                "^2%s ^2WAS PICKED OFF BY %s",
                "^2%s ^2TOOK %s^2'S SHOT TO THE FACE",
                "^2%s ^2DANCED THE CHAINGUN CHA-CHA WITH %s",
                "^2%s ^2TRIED TO MAKE A BONG OUT OF %s^2'S ROCKET",
                "^2%s ^2EXPLODED.  BLAME %s^2!",
                "^2%s ^2BECAME ONE WITH THE GUM ON %s^2'S SHOE",
                "^2%s ^2WAS TOO COOL FOR %s",
                "^2%s ^2EXPANDED HIS HORIZONS WITH HELP FROM %s",
                "^2%s ^2THINKS %s ^2SHOULD CHECK HIS GLASSES",

                "^2%s ^2TOOK %s^2'S BOOT TO THE HEAD",
                "^2%s ^2FELL VICTIM TO %s^2's MAGIC AUTOAIMING PISTOL",
                "^2%s ^2WAS CHASED OFF OF %s^2'S PORCH",
                "^2%s ^2COULDN'T DANCE FAST ENOUGH FOR %s",
                "^2%s ^2TRIED TO OUTRUN %s^2'S ROCKET",
                "^2%s ^2FOUND %s^2'S HIDDEN WEAPONS OF MASS DESTRUCTION",
                "^2%s ^2WAS JUST TRYING TO HELP %s ^2TIE HIS SHOELACES",
                "^2%s^2's IGLOO WAS WRECKED BY %s",
                "^2%s ^2BECAME A STICKY FILM ON %s^2'S BOOTS",
                "^2%s ^2WISHES %s ^2HAD PRACTICED BEFORE PLAYING",

                "^2%s ^2WAS WALKED ALL OVER BY %s",
                "^2%s ^2WAS PICKED OFF BY %s",
                "^2%s ^2SUCKED %s^2'S SHOTGUN",
                "^2%s ^2ENDED UP WITH A FEW NEW HOLES FROM %s^2'S CHAINGUN",
                "^2%s ^2WAS TURNED INTO %s^2 BRAND CHUNKY SALSA",
                "^2%s ^2FOUND A PRESENT FROM %s",
                "^2%s ^2WAS SCATHED BY %s^2'S SHRINK RAY",
                "^2%s ^2WENT TO PIECES.  %s^2, HOW COULD YOU?",
                "^2%s ^2EXPANDED HIS HORIZONS WITH HELP FROM %s",
                "^2%s ^2WANTS TO KNOW WHY %s ^2IS EVEN PLAYING COOP",
            };

            const char *podeathstrings[] =
            {
                "^2%s ^2KILLED HIMSELF.  WHAT A TOOL!",
                "^2%s ^2TRIED TO LEAVE",
                "^2%s ^2GOT FRAGGED BY A MONSTER.  IT WAS PROBABLY A LIZTROOP.",
                "^2%s ^2SWITCHED TO TEAM %d"
            };

            for (i=0;i<(signed int)(sizeof(ppdeathstrings)/sizeof(ppdeathstrings[0]));i++)
            {
                if (fta_quotes[i+PPDEATHSTRINGS] == NULL)
                {
                    fta_quotes[i+PPDEATHSTRINGS] = Bcalloc(MAXQUOTELEN,sizeof(char));
                    Bstrcpy(fta_quotes[i+PPDEATHSTRINGS],ppdeathstrings[i]);
                }
            }

            for (i=0;i<(signed int)(sizeof(podeathstrings)/sizeof(podeathstrings[0]));i++)
            {
                if (fta_quotes[i+PSDEATHSTRINGS] == NULL)
                {
                    fta_quotes[i+PSDEATHSTRINGS] = Bcalloc(MAXQUOTELEN,sizeof(char));
                    Bstrcpy(fta_quotes[i+PSDEATHSTRINGS],podeathstrings[i]);
                }
            }
        }
    }
}

void ReportError(int iError)
{
    if (Bstrcmp(parsing_item_name,previous_item_name))
    {
        if (parsing_event || parsing_state || parsing_actor)
            initprintf("%s: In %s `%s':\n",compilefile,parsing_event?"event":parsing_actor?"actor":"state",parsing_item_name);
        else initprintf("%s: At top level:\n",compilefile);
        Bstrcpy(previous_item_name,parsing_item_name);
    }
    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%d: error: found more `}' than `{' before `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_EVENTONLY:
        initprintf("%s:%d: error: command `%s' only valid during events.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        initprintf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",compilefile,line_number,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%d: error: expected a keyword but found `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%d: error: found `%s' within %s.\n",compilefile,line_number,tempbuf,parsing_event?"an event":parsing_actor?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%d: error: did not find `endswitch' before `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%d: error: symbol `%s' is not a game definition.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a game variable.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_NOTAGAMEARRAY:
        initprintf("%s:%d: error: symbol `%s' is not a game array.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_GAMEARRAYBNC:
        initprintf("%s:%d: error: square brackets for index of game array not closed, expected ] found %c\n",compilefile,line_number,*textptr);
        break;
    case ERROR_GAMEARRAYBNO:
        initprintf("%s:%d: error: square brackets for index of game array not opened, expected [ found %c\n",compilefile,line_number,*textptr);
        break;
    case ERROR_INVALIDARRAYWRITE:
        initprintf("%s:%d: error: arrays can only be written using setarray %c\n",compilefile,line_number,*textptr);
        break;
    case ERROR_OPENBRACKET:
        initprintf("%s:%d: error: found more `{' than `}' before `%s'.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",compilefile,line_number,tempbuf);
        break;
    case ERROR_SYMBOLNOTRECOGNIZED:
        initprintf("%s:%d: error: symbol `%s' is not recognized.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",compilefile,line_number);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate game definition `%s' ignored.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case WARNING_EVENTSYNC:
        initprintf("%s:%d: warning: found `%s' within a local event.\n",compilefile,line_number,tempbuf);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",compilefile,line_number);
        break;
    case WARNING_BADGAMEVAR:
        initprintf("%s:%ld: warning: variable `%s' should be either per-player OR per-actor, not both.\n",compilefile,line_number,label+(labelcnt<<6));
        break;
    case WARNING_DUPLICATECASE:
        initprintf("%s:%ld: warning: duplicate case ignored.\n",compilefile,line_number);
        break;
    case WARNING_REVEVENTSYNC:
        initprintf("%s:%d: warning: found `%s' outside of a local event.\n",compilefile,line_number,tempbuf);
        break;
    }
}
