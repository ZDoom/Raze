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

#include "duke3d.h"
#include "gamedef.h"
#include "gameexec.h"

#include "osd.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#define NUMKEYWORDS (int32_t)(sizeof(keyw)/sizeof(keyw[0]))

int32_t g_scriptVersion = 13; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files
uint32_t g_scriptDateVersion = 99999999;  // YYYYMMDD
static uint32_t g_scriptLastKeyword; // = NUMKEYWORDS-1;

#define NUMKEYWDATES (int32_t)(sizeof(g_keywdate)/sizeof(g_keywdate[0]))
// { keyw, date } means that at the date, all keywords up to keyw inclusive are available
static struct { uint32_t keyw; uint32_t date; } g_keywdate[] =
{
    // beginning of eduke32 svn
    { CON_CANSEE, 20060423 },    
    { CON_CANSEESPR, 20060424 },
    // some stuff here not representable this way
    { CON_FINDNEARSPRITEZVAR, 20060516 },
    { CON_EZSHOOT, 20060701 },
    { CON_EZSHOOTVAR, 20060822 },
    { CON_JUMP, 20060828 },
    { CON_QSTRLEN, 20060930 },
    { CON_QUAKE, 20070105 },
    { CON_SHOWVIEW, 20070208 },
    { CON_NEXTSPRITESECT, 20070819 },
    { CON_GETKEYNAME, 20071024 },  // keyw numbers have been
    { CON_SPRITENOPAL, 20071220 }, // shuffled around here
    { CON_HITRADIUSVAR, 20080216 },
    { CON_ROTATESPRITE16, 20080314 },
    { CON_SETARRAY, 20080401 },
    { CON_READARRAYFROMFILE, 20080405 },
    { CON_STARTTRACKVAR, 20080510 },
    { CON_QGETSYSSTR, 20080709 },
    { CON_GETTICKS, 20080711 },
    { CON_SETTSPR, 20080713 },
    { CON_CLEARMAPSTATE, 20080716 },
    { CON_SCRIPTSIZE, 20080720 },
    { CON_SETGAMENAME, 20080722 },
    { CON_CMENU, 20080725 },
    { CON_GETTIMEDATE, 20080809 },
    { CON_ACTIVATECHEAT, 20080810 },
    { CON_SETGAMEPALETTE, 20080816 },
    { CON_SETCFGNAME, 20080817 },
    { CON_IFVARVAREITHER, 20080907 },
    { CON_SAVENN, 20080915 },
    { CON_COPY, 20090219 },
//    { CON_INV, 20090619 },
    { CON_QSTRNCAT, 20090712 },
    { CON_STOPACTORSOUND, 20090715 },
    { CON_IFSERVER, 20100722 },
    { CON_CALCHYPOTENUSE, 20100927 },
    { CON_CLIPMOVENOSLIDE, 20101009 },
};

char g_szScriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling
static char g_szCurrentBlockName[256] = "(none)", g_szLastBlockName[256] = "NULL";

int32_t g_totalLines,g_lineNumber;
static int32_t g_checkingIfElse, g_processingState, g_lastKeyword = -1;
char g_szBuf[1024];

intptr_t *g_caseScriptPtr=NULL;      // the pointer to the start of the case table in a switch statement
// first entry is 'default' code.
static intptr_t *previous_event=NULL;
static int32_t g_numCases = 0;
static int32_t g_checkingSwitch = 0, g_currentEvent = -1;
static int32_t g_labelsOnly = 0, g_skipKeywordCheck = 0, g_dynamicTileMapping = 0;
static int32_t g_numBraces = 0;

static int32_t C_SetScriptSize(int32_t size);

int32_t g_numQuoteRedefinitions = 0;

// pointers to weapon gamevar data
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
intptr_t *aplWeaponInitialSound[MAX_WEAPONS];   // Sound made when weapon starts firing. zero for no sound
intptr_t *aplWeaponFireSound[MAX_WEAPONS];      // Sound made when firing (each time for automatic)
intptr_t *aplWeaponSound2Time[MAX_WEAPONS];     // Alternate sound time
intptr_t *aplWeaponSound2Sound[MAX_WEAPONS];    // Alternate sound sound ID
intptr_t *aplWeaponReloadSound1[MAX_WEAPONS];    // Sound of magazine being removed
intptr_t *aplWeaponReloadSound2[MAX_WEAPONS];    // Sound of magazine being inserted
intptr_t *aplWeaponSelectSound[MAX_WEAPONS];     // Sound of weapon being selected
intptr_t *aplWeaponFlashColor[MAX_WEAPONS];     // Muzzle flash color

int32_t g_iReturnVarID=-1;      // var ID of "RETURN"
int32_t g_iWeaponVarID=-1;      // var ID of "WEAPON"
int32_t g_iWorksLikeVarID=-1;   // var ID of "WORKSLIKE"
int32_t g_iZRangeVarID=-1;      // var ID of "ZRANGE"
int32_t g_iAngRangeVarID=-1;    // var ID of "ANGRANGE"
int32_t g_iAimAngleVarID=-1;    // var ID of "AUTOAIMANGLE"
int32_t g_iLoTagID=-1;          // var ID of "LOTAG"
int32_t g_iHiTagID=-1;          // var ID of "HITAG"
int32_t g_iTextureID=-1;        // var ID of "TEXTURE"
int32_t g_iThisActorID=-1;      // var ID of "THISACTOR"
int32_t g_iSpriteVarID=-1;
int32_t g_iSectorVarID=-1;
int32_t g_iWallVarID=-1;
int32_t g_iPlayerVarID=-1;
int32_t g_iActorVarID=-1;

intptr_t *actorLoadEventScrptr[MAXTILES];

intptr_t *apScriptGameEvent[MAXGAMEEVENTS];
static intptr_t *g_parsingEventPtr=NULL;

gamevar_t aGameVars[MAXGAMEVARS];
gamearray_t aGameArrays[MAXGAMEARRAYS];
int32_t g_gameVarCount=0;
int32_t g_gameArrayCount=0;

extern int32_t qsetmode;

char *textptr;
int32_t g_numCompilerErrors,g_numCompilerWarnings;

extern char *g_gameNamePtr;
extern char *g_defNamePtr;

extern int32_t g_maxSoundPos;

enum ScriptLabel_t
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
};

static const char *LabelTypeText[] =
{
    "define",
    "state",
    "actor",
    "action",
    "ai",
    "move"
};

static const char *C_GetLabelType(int32_t type)
{
    int32_t i;
    char x[64];

    x[0] = 0;
    for (i=0; i<6; i++)
    {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, LabelTypeText[i]);
    }
    return Bstrdup(x);
}


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
    "palfrom",                  // 14 used for player screen shading effect, sets p->pals
    "sound",                    // 15 plays a sound that was defined with definesound
    "fall",                     // 16 causes actor to fall to sector floor height
    "state",                    // 17 begins defining a state if used outside a state or actor, otherwise calls a state
    "ends",                     // 18 ends defining a state
    "define",                   // 19 defines a value
    "return",                   // 20
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
    "setgamename",              // 330
    "cmenu",                    // 331
    "gettimedate",              // 332
    "activatecheat",            // 333
    "setgamepalette",           // 334
    "setdefname",               // 335
    "setcfgname",               // 336
    "ifvaror",                  // 337
    "ifvarvaror",               // 338
    "ifvarxor",                 // 339
    "ifvarvarxor",              // 340
    "ifvareither",              // 341
    "ifvarvareither",           // 342
    "getarraysize",             // 343
    "savenn",                   // 344
    "copy",                     // 345
    "<null>",                   // 346 internal inversion function
    "sectorofwall",             // 347
    "qstrncat",                 // 348
    "ifactorsound",             // 349
    "stopactorsound",           // 350
    "ifclient",                 // 351
    "ifserver",                 // 352
    "sectsetinterpolation",     // 353
    "sectclearinterpolation",   // 354
    "clipmove",   // 355
    "lineintersect",   // 356
    "rayintersect",   // 357
    "calchypotenuse",   // 358
    "clipmovenoslide",   // 359
    "<null>"
};

const memberlabel_t SectorLabels[]=
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
//    { "filler", SECTOR_ALIGNTO, 0, 0 },
    { "alignto", SECTOR_ALIGNTO, 0, 0 }, // aka filler, not used
    { "lotag", SECTOR_LOTAG, 0, 0 },
    { "hitag", SECTOR_HITAG, 0, 0 },
    { "extra", SECTOR_EXTRA, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t WallLabels[]=
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

const memberlabel_t ActorLabels[]=
{
    { "x", ACTOR_X, 0, 0 },
    { "y", ACTOR_Y, 0, 0 },
    { "z", ACTOR_Z, 0, 0 },
    { "cstat", ACTOR_CSTAT, 0, 0 },
    { "picnum", ACTOR_PICNUM, 0, 0 },
    { "shade", ACTOR_SHADE, 0, 0 },
    { "pal", ACTOR_PAL, 0, 0 },
    { "clipdist", ACTOR_CLIPDIST, 0, 0 },
//    { "filler", ACTOR_DETAIL, 0, 0 },
    { "detail", ACTOR_DETAIL, 0, 0 }, // aka filler, not used
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

    // ActorExtra labels...
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

    { "alpha", ACTOR_ALPHA, 0, 0 },

    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t TsprLabels[]=
{
    // tsprite access

    { "tsprx", ACTOR_X, 0, 0 },
    { "tspry", ACTOR_Y, 0, 0 },
    { "tsprz", ACTOR_Z, 0, 0 },
    { "tsprcstat", ACTOR_CSTAT, 0, 0 },
    { "tsprpicnum", ACTOR_PICNUM, 0, 0 },
    { "tsprshade", ACTOR_SHADE, 0, 0 },
    { "tsprpal", ACTOR_PAL, 0, 0 },
    { "tsprclipdist", ACTOR_CLIPDIST, 0, 0 },
//    { "tsprfiller", ACTOR_DETAIL, 0, 0 },
    { "tsprdetail", ACTOR_DETAIL, 0, 0 }, // aka filler, not used
    { "tsprxrepeat", ACTOR_XREPEAT, 0, 0 },
    { "tspryrepeat", ACTOR_YREPEAT, 0, 0 },
    { "tsprxoffset", ACTOR_XOFFSET, 0, 0 },
    { "tspryoffset", ACTOR_YOFFSET, 0, 0 },
    { "tsprsectnum", ACTOR_SECTNUM, 0, 0 },
    { "tsprstatnum", ACTOR_STATNUM, 0, 0 },
    { "tsprang", ACTOR_ANG, 0, 0 },
    { "tsprowner", ACTOR_OWNER, 0, 0 },
#if 1
    { "tsprxvel", ACTOR_XVEL, 0, 0 },
    { "tspryvel", ACTOR_YVEL, 0, 0 },
    { "tsprzvel", ACTOR_ZVEL, 0, 0 },
    { "tsprlotag", ACTOR_LOTAG, 0, 0 },
    { "tsprhitag", ACTOR_HITAG, 0, 0 },
    { "tsprextra", ACTOR_EXTRA, 0, 0 },
#endif

    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t PlayerLabels[]=
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
    { "weaprecs", PLAYER_WEAPRECS, LABEL_HASPARM2, MAX_WEAPONS },
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
    { "last_quick_kick", PLAYER_LAST_QUICK_KICK, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t ProjectileLabels[]=
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
    { "flashcolor", PROJ_FLASH_COLOR, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t UserdefsLabels[]=
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
    { "display_bonus_screen", USERDEFS_DISPLAY_BONUS_SCREEN, 0, 0 },
    { "show_level_text", USERDEFS_SHOW_LEVEL_TEXT, 0, 0 },
    { "weaponscale", USERDEFS_WEAPONSCALE, 0, 0 },
    { "textscale", USERDEFS_TEXTSCALE, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

const memberlabel_t InputLabels[]=
{
    { "avel", INPUT_AVEL, 0, 0 },
    { "horz", INPUT_HORZ, 0, 0 },
    { "fvel", INPUT_FVEL, 0, 0 },
    { "svel", INPUT_SVEL, 0, 0 },
    { "bits", INPUT_BITS, 0, 0 },
    { "extbits", INPUT_EXTBITS, 0, 0 },
    { "", -1, 0, 0  }     // END OF LIST
};

char *bitptr; // pointer to bitmap of which bytecode positions contain pointers
#define BITPTR_POINTER 1

hashtable_t h_gamevars    = { MAXGAMEVARS>>1, NULL };
hashtable_t h_arrays      = { MAXGAMEARRAYS>>1, NULL };
hashtable_t h_labels      = { 11264>>1, NULL };
hashtable_t h_keywords       = { CON_END>>1, NULL };

hashtable_t sectorH     = { SECTOR_END>>1, NULL };
hashtable_t wallH       = { WALL_END>>1, NULL };
hashtable_t userdefH    = { USERDEFS_END>>1, NULL };

hashtable_t projectileH = { PROJ_END>>1, NULL };
hashtable_t playerH     = { PLAYER_END>>1, NULL };
hashtable_t inputH      = { INPUT_END>>1, NULL };
hashtable_t actorH      = { ACTOR_END>>1, NULL };
hashtable_t tspriteH    = { ACTOR_END>>1, NULL };

void inithashnames();
void freehashnames();

void C_InitHashes()
{
    int32_t i;

    hash_init(&h_gamevars);
    hash_init(&h_arrays);
    hash_init(&h_labels);
    inithashnames();

    hash_init(&h_keywords);
    hash_init(&sectorH);
    hash_init(&wallH);
    hash_init(&userdefH);
    hash_init(&projectileH);
    hash_init(&playerH);
    hash_init(&inputH);
    hash_init(&actorH);
    hash_init(&tspriteH);

    g_scriptLastKeyword = NUMKEYWORDS-1;
    // determine last CON keyword for backward compatibility with older mods
    if (g_scriptDateVersion < g_keywdate[NUMKEYWDATES-1].date)
    {
        for (i=NUMKEYWDATES-1; i>=0; i--)
        {
            if (g_scriptDateVersion >= g_keywdate[i].date)
            {
                g_scriptLastKeyword = g_keywdate[i].keyw;
                break;
            }
        }

        if (i<0)
            g_scriptLastKeyword = g_keywdate[0].keyw-1;  // may be slightly imprecise
    }

    for (i=g_scriptLastKeyword; i>=0; i--) hash_add(&h_keywords,keyw[i],i,0);
    for (i=0; SectorLabels[i].lId >= 0; i++) hash_add(&sectorH,SectorLabels[i].name,i,0);
    for (i=0; WallLabels[i].lId >= 0; i++) hash_add(&wallH,WallLabels[i].name,i,0);
    for (i=0; UserdefsLabels[i].lId >= 0; i++) hash_add(&userdefH,UserdefsLabels[i].name,i,0);
    for (i=0; ProjectileLabels[i].lId >= 0; i++) hash_add(&projectileH,ProjectileLabels[i].name,i,0);
    for (i=0; PlayerLabels[i].lId >= 0; i++) hash_add(&playerH,PlayerLabels[i].name,i,0);
    for (i=0; InputLabels[i].lId >= 0; i++) hash_add(&inputH,InputLabels[i].name,i,0);
    for (i=0; ActorLabels[i].lId >= 0; i++) hash_add(&actorH,ActorLabels[i].name,i,0);
    for (i=0; TsprLabels[i].lId >= 0; i++) hash_add(&tspriteH,TsprLabels[i].name,i,0);
}

// "magic" number for { and }, overrides line number in compiled code for later detection
#define IFELSE_MAGIC 31337
static int32_t g_ifElseAborted;

static int32_t C_SetScriptSize(int32_t size)
{
    intptr_t oscriptPtr = (unsigned)(g_scriptPtr-script);
    intptr_t ocaseScriptPtr = (unsigned)(g_caseScriptPtr-script);
    intptr_t oparsingEventPtr = (unsigned)(g_parsingEventPtr-script);
    intptr_t oparsingActorPtr = (unsigned)(g_parsingActorPtr-script);
    char *scriptptrs;
    intptr_t *newscript;
    intptr_t i, j;
    int32_t osize = g_scriptSize;
    char *newbitptr;

    for (i=MAXSECTORS-1; i>=0; i--)
    {
        if (labelcode[i] && labeltype[i] != LABEL_DEFINE)
        {
            labelcode[i] -= (intptr_t)&script[0];
        }
    }

    scriptptrs = Bcalloc(1,g_scriptSize * sizeof(uint8_t));
    for (i=g_scriptSize-1; i>=0; i--)
    {
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)) && !((intptr_t)script[i] >= (intptr_t)(&script[0]) && (intptr_t)script[i] < (intptr_t)(&script[g_scriptSize])))
        {
            g_numCompilerErrors++;
            initprintf("Internal compiler error at %d (0x%x)\n",i,i);
        }
//        if (bitptr[i] == 0 && ((intptr_t)script[i] >= (intptr_t)(&script[0]) && (intptr_t)script[i] < (intptr_t)(&script[g_scriptSize])))
//            initprintf("oh no!\n");
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
        {
            scriptptrs[i] = 1;
            script[i] -= (intptr_t)&script[0];
        }
        else scriptptrs[i] = 0;
    }

    for (i=MAXTILES-1; i>=0; i--)
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

    for (i=MAXGAMEEVENTS-1; i>=0; i--)
        if (apScriptGameEvent[i])
        {
            j = (intptr_t)apScriptGameEvent[i]-(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)j;
        }

    //initprintf("offset: %d\n",(unsigned)(g_scriptPtr-script));
    g_scriptSize = size;
    initprintf("Resizing code buffer to %d*%d bytes\n",g_scriptSize, sizeof(intptr_t));

    newscript = (intptr_t *)Brealloc(script, g_scriptSize * sizeof(intptr_t));

//    bitptr = (char *)Brealloc(bitptr, g_scriptSize * sizeof(uint8_t));
    newbitptr = Bcalloc(1,(((size+7)>>3)+1) * sizeof(uint8_t));

    if (newscript == NULL || newbitptr == NULL)
    {
        C_ReportError(-1);
        initprintf("%s:%d: out of memory: Aborted (%ud)\n",g_szScriptFileName,g_lineNumber,(unsigned)(g_scriptPtr-script));
        initprintf(tempbuf);
        g_numCompilerErrors++;
        return 1;
    }

    if (size >= osize)
    {
        Bmemset(&newscript[osize],0,(size-osize) * sizeof(intptr_t));
//        Bmemset(&bitptr[osize],0,size-osize);
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((osize+7)>>3));
    }
    else
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((size+7)>>3));

    Bfree(bitptr);
    bitptr = newbitptr;
    if (script != newscript)
    {
        initprintf("Relocating compiled code from to 0x%x to 0x%x\n", script, newscript);
        script = newscript;
    }

    g_scriptPtr = (intptr_t *)(script+oscriptPtr);
//    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    //initprintf("offset: %d\n",(unsigned)(g_scriptPtr-script));

    if (g_caseScriptPtr)
        g_caseScriptPtr = (intptr_t *)(script+ocaseScriptPtr);

    if (g_parsingEventPtr)
        g_parsingEventPtr = (intptr_t *)(script+oparsingEventPtr);

    if (g_parsingActorPtr)
        g_parsingActorPtr = (intptr_t *)(script+oparsingActorPtr);

    for (i=MAXSECTORS-1; i>=0; i--)
    {
        if (labelcode[i] && labeltype[i] != LABEL_DEFINE)
        {
            labelcode[i] += (intptr_t)&script[0];
        }
    }

    if (size >= osize)
    {
        for (i=g_scriptSize-(size-osize)-1; i>=0; i--)
            if (scriptptrs[i])
            {
                j = (intptr_t)script[i]+(intptr_t)&script[0];
                script[i] = j;
            }
    }
    else
    {
        for (i=g_scriptSize-1; i>=0; i--)
            if (scriptptrs[i])
            {
                j = (intptr_t)script[i]+(intptr_t)&script[0];
                script[i] = j;
            }
    }

    for (i=MAXTILES-1; i>=0; i--)
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

    for (i=MAXGAMEEVENTS-1; i>=0; i--)
        if (apScriptGameEvent[i])
        {
            j = (intptr_t)apScriptGameEvent[i]+(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)j;
        }

    Bfree(scriptptrs);
    return 0;
}

static inline int32_t ispecial(const char c)
{
    if (c == ' ' || c == 0x0d || c == '(' || c == ')' ||
            c == ',' || c == ';' || (c == 0x0a /*&& ++g_lineNumber*/))
        return 1;

    return 0;
}

#define C_NextLine() while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++

static int32_t C_SkipComments(void)
{
    do
    {
        switch (*textptr)
        {
        case '\n':
            g_lineNumber++;
        case ' ':
        case '\t':
        case '\r':
        case 0x1a:
            textptr++;
            break;
        case '/':
            switch (textptr[1])
            {
            case '/': // C++ style comment
                if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got comment.\n",g_szScriptFileName,g_lineNumber);
                C_NextLine();
                continue;
            case '*': // beginning of a C style comment
                if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got start of comment block.\n",g_szScriptFileName,g_lineNumber);
                do
                {
                    if (*textptr == '\n')
                        g_lineNumber++;
                    textptr++;
                }
                while (*textptr && (textptr[0] != '*' || textptr[1] != '/'));

                if (!*textptr)
                {
                    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
                        initprintf("%s:%d: debug: EOF in comment!\n",g_szScriptFileName,g_lineNumber);
                    C_ReportError(-1);
                    initprintf("%s:%d: error: found `/*' with no `*/'.\n",g_szScriptFileName,g_lineNumber);
                    g_parsingActorPtr = 0; g_processingState = g_numBraces = 0;
                    g_numCompilerErrors++;
                    continue;
                }

                if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got end of comment block.\n",g_szScriptFileName,g_lineNumber);

                textptr+=2;
                continue;
            }
            continue;

        default:
            if (ispecial(*textptr))
            {
                textptr++;
                continue;
            }
        case 0: // EOF
            return ((g_scriptPtr-script) > (g_scriptSize-32)) ? C_SetScriptSize(g_scriptSize<<1) : 0;
        }
    }
    while (1);
}

static void C_SetProjectile(int32_t lVar1, int32_t lLabelID, int32_t lVar2)
{
    switch (lLabelID)
    {
    case PROJ_WORKSLIKE:
        ProjectileData[lVar1].workslike=lVar2;
        break;

    case PROJ_SPAWNS:
        ProjectileData[lVar1].spawns=lVar2;
        break;

    case PROJ_SXREPEAT:
        ProjectileData[lVar1].sxrepeat=lVar2;
        break;

    case PROJ_SYREPEAT:
        ProjectileData[lVar1].syrepeat=lVar2;
        break;

    case PROJ_SOUND:
        ProjectileData[lVar1].sound=lVar2;
        break;

    case PROJ_ISOUND:
        ProjectileData[lVar1].isound=lVar2;
        break;

    case PROJ_VEL:
        ProjectileData[lVar1].vel=lVar2;
        break;

    case PROJ_EXTRA:
        ProjectileData[lVar1].extra=lVar2;
        break;

    case PROJ_DECAL:
        ProjectileData[lVar1].decal=lVar2;
        break;

    case PROJ_TRAIL:
        ProjectileData[lVar1].trail=lVar2;
        break;

    case PROJ_TXREPEAT:
        ProjectileData[lVar1].txrepeat=lVar2;
        break;

    case PROJ_TYREPEAT:
        ProjectileData[lVar1].tyrepeat=lVar2;
        break;

    case PROJ_TOFFSET:
        ProjectileData[lVar1].toffset=lVar2;
        break;

    case PROJ_TNUM:
        ProjectileData[lVar1].tnum=lVar2;
        break;

    case PROJ_DROP:
        ProjectileData[lVar1].drop=lVar2;
        break;

    case PROJ_CSTAT:
        ProjectileData[lVar1].cstat=lVar2;
        break;

    case PROJ_CLIPDIST:
        ProjectileData[lVar1].clipdist=lVar2;
        break;

    case PROJ_SHADE:
        ProjectileData[lVar1].shade=lVar2;
        break;

    case PROJ_XREPEAT:
        ProjectileData[lVar1].xrepeat=lVar2;
        break;

    case PROJ_YREPEAT:
        ProjectileData[lVar1].yrepeat=lVar2;
        break;

    case PROJ_PAL:
        ProjectileData[lVar1].pal=lVar2;
        break;

    case PROJ_EXTRA_RAND:
        ProjectileData[lVar1].extra_rand=lVar2;
        break;

    case PROJ_HITRADIUS:
        ProjectileData[lVar1].hitradius=lVar2;
        break;

    case PROJ_VEL_MULT:
        ProjectileData[lVar1].velmult=lVar2;
        break;

    case PROJ_OFFSET:
        ProjectileData[lVar1].offset=lVar2;
        break;

    case PROJ_BOUNCES:
        ProjectileData[lVar1].bounces=lVar2;
        break;

    case PROJ_BSOUND:
        ProjectileData[lVar1].bsound=lVar2;
        break;

    case PROJ_RANGE:
        ProjectileData[lVar1].range=lVar2;
        break;

    default:
        break;
    }

    //  DefaultProjectileData[lVar1] = ProjectileData[lVar1];
    Bmemcpy(&DefaultProjectileData[lVar1], &ProjectileData[lVar1], sizeof(ProjectileData[lVar1]));

    return;
}

static int32_t C_CheckEventSync(int32_t iEventID)
{
    if (g_parsingEventPtr || g_parsingActorPtr)
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

#define GetDefID(szGameLabel) hash_find(&h_gamevars,szGameLabel)
#define GetADefID(szGameLabel) hash_find(&h_arrays,szGameLabel)

static inline int32_t isaltok(const char c)
{
    return (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '\\' ||
            c == '*' || c == '-' || c == '_' || c == '.');
}

static inline int32_t C_GetLabelNameID(const memberlabel_t *pLabel, hashtable_t *tH, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int32_t l=-1;

    l = hash_findcase(tH,psz);
    if (l>=0) l= pLabel[l].lId;

    return l;
}

static inline int32_t C_GetLabelNameOffset(hashtable_t *tH, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the offset in the array for the label, or -1

    return hash_findcase(tH,psz);
}

static void C_GetNextLabelName(void)
{
    int32_t i;

    C_SkipComments();

    i = 0;
    while (ispecial(*textptr) == 0 && *textptr!='['&& *textptr!=']' && *textptr!='\t' && *textptr!='\n' && *textptr!='\r')
        label[(g_numLabels<<6)+(i++)] = *(textptr++);

    label[(g_numLabels<<6)+i] = 0;
    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: got label `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
}

static int32_t C_GetKeyword(void)
{
    int32_t i;
    char *temptextptr;

    C_SkipComments();

    temptextptr = textptr;

    while (isaltok(*temptextptr) == 0)
    {
        temptextptr++;
        if (*temptextptr == 0)
            return 0;
    }

    i = 0;
    while (isaltok(*temptextptr))
        tempbuf[i++] = *(temptextptr++);
    tempbuf[i] = 0;
    return hash_find(&h_keywords,tempbuf);
}

static int32_t C_GetNextKeyword(void) //Returns its code #
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0) // EOF
        return -1;

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

    i = hash_find(&h_keywords,tempbuf);
    if (i>=0)
    {
        if (i == CON_LEFTBRACE || i == CON_RIGHTBRACE || i == CON_NULLOP)
            *g_scriptPtr = i + (IFELSE_MAGIC<<12);
        else *g_scriptPtr = i + (g_lineNumber<<12);

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        textptr += l;
        g_scriptPtr++;

        if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
            initprintf("%s:%d: debug: translating keyword `%s'.\n",g_szScriptFileName,g_lineNumber,keyw[i]);
        return i;
    }

    textptr += l;

    if (tempbuf[0] == '{' && tempbuf[1] != 0)
    {
        C_ReportError(-1);
        initprintf("%s:%d: error: expected whitespace between `{' and `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf+1);
    }
    else if (tempbuf[0] == '}' && tempbuf[1] != 0)
    {
        C_ReportError(-1);
        initprintf("%s:%d: error: expected whitespace between `}' and `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf+1);
    }
    else C_ReportError(ERROR_EXPECTEDKEYWORD);
    g_numCompilerErrors++;
    return -1;
}

static void C_GetNextVarType(int32_t type)
{
    int32_t i=0,f=0;

    C_SkipComments();

    if (!type && !g_labelsOnly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
            initprintf("%s:%d: debug: accepted constant %d in place of gamevar.\n",g_szScriptFileName,g_lineNumber,atol(textptr));
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=MAXGAMEVARS;
        if (tolower(textptr[1])=='x')
            sscanf(textptr+2,"%" PRIxPTR "",g_scriptPtr);
        else
            *g_scriptPtr=atoi(textptr);
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        g_scriptPtr++;
#if 1
        while (!ispecial(*textptr) && *textptr != ']') textptr++;
#else
        C_GetNextLabelName();
#endif
        return;
    }
    else if ((*textptr == '-')/* && !isdigit(*(textptr+1))*/)
    {
        if (!type)
        {
            if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
                initprintf("%s:%d: debug: flagging gamevar as negative.\n",g_szScriptFileName,g_lineNumber,atol(textptr));
            f = (MAXGAMEVARS<<1);
        }
        else
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextLabelName();
            return;
        }

        textptr++;
    }
    C_GetNextLabelName();

    if (!g_skipKeywordCheck && hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_ISAKEYWORD);
        return;
    }

    C_SkipComments(); //skip comments and whitespace
    if ((*textptr == '['))     //read of array as a gamevar
    {
        int32_t lLabelID = -1;

        f |= (MAXGAMEVARS<<2);
        textptr++;
        i=GetADefID(label+(g_numLabels<<6));
        if (i < 0)
        {
            i=GetDefID(label+(g_numLabels<<6));
            if (i < g_iSpriteVarID || i > g_iActorVarID)
                i = -1;

            if (i < 0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return;
            }
            f &= ~(MAXGAMEVARS<<2); // not an array
            f |= (MAXGAMEVARS<<3);
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=(i|f);
        C_GetNextVarType(0);
        C_SkipComments();

        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return;
        }
        textptr++;

        //writing arrays in this way is not supported because it would require too many changes to other code

        if (type)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_INVALIDARRAYWRITE);
            return;
        }

        if (f & (MAXGAMEVARS<<3))
        {
            while (*textptr != '.')
            {
                if (*textptr == 0xa)
                    break;
                if (!*textptr)
                    break;

                textptr++;
            }

            if (*textptr != '.')
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_SYNTAXERROR);
                return;
            }
            textptr++;
            /// now pointing at 'xxx'
            C_GetNextLabelName();
            /*initprintf("found xxx label of '%s'\n",   label+(g_numLabels<<6));*/

            if (i == g_iSpriteVarID)
                lLabelID=C_GetLabelNameOffset(&actorH,Bstrtolower(label+(g_numLabels<<6)));
            else if (i == g_iSectorVarID)
                lLabelID=C_GetLabelNameOffset(&sectorH,Bstrtolower(label+(g_numLabels<<6)));
            else if (i == g_iWallVarID)
                lLabelID=C_GetLabelNameOffset(&wallH,Bstrtolower(label+(g_numLabels<<6)));
            else if (i == g_iPlayerVarID)
                lLabelID=C_GetLabelNameOffset(&playerH,Bstrtolower(label+(g_numLabels<<6)));
            else if (i == g_iActorVarID)
                lLabelID=GetDefID(label+(g_numLabels<<6));

            //printf("LabelID is %d\n",lLabelID);
            if (lLabelID == -1)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return;
            }

            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));

            if (i == g_iSpriteVarID)
            {
                *g_scriptPtr++=ActorLabels[lLabelID].lId;

                //printf("member's flags are: %02Xh\n",ActorLabels[lLabelID].flags);
                if (ActorLabels[lLabelID].flags & LABEL_HASPARM2)
                {
                    //printf("Member has PARM2\n");
                    // get parm2
                    // get the ID of the DEF
                    C_GetNextVarType(0);
                }
            }
            else if (i == g_iSectorVarID)
                *g_scriptPtr++=SectorLabels[lLabelID].lId;
            else if (i == g_iWallVarID)
                *g_scriptPtr++=SectorLabels[lLabelID].lId;
            else if (i == g_iPlayerVarID)
            {
                *g_scriptPtr++=PlayerLabels[lLabelID].lId;

                //printf("member's flags are: %02Xh\n",ActorLabels[lLabelID].flags);
                if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2)
                {
                    //printf("Member has PARM2\n");
                    // get parm2
                    // get the ID of the DEF
                    C_GetNextVarType(0);
                }
            }
            else if (i == g_iActorVarID)
                *g_scriptPtr++=lLabelID;
        }
        return;
    }
//    initprintf("not an array");
    i=GetDefID(label+(g_numLabels<<6));
    if (i<0)   //gamevar not found
    {
        if (!type && !g_labelsOnly)
        {
            //try looking for a define instead
            Bstrcpy(tempbuf,label+(g_numLabels<<6));
            i = hash_find(&h_labels,tempbuf);
            if (i>=0)
            {
                if (labeltype[i] & LABEL_DEFINE)
                {
                    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
                        initprintf("%s:%d: debug: accepted defined label `%s' instead of gamevar.\n",g_szScriptFileName,g_lineNumber,label+(i<<6));
                    bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                    *g_scriptPtr++=MAXGAMEVARS;
                    bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                    *g_scriptPtr++=labelcode[i];
                    return;
                }
            }
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEVAR);
            return;
        }
        g_numCompilerErrors++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        textptr++;
        return;

    }
    if (type == GAMEVAR_READONLY && aGameVars[i].dwFlags & GAMEVAR_READONLY)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (aGameVars[i].dwFlags & type)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }
    if ((aGameVars[i].dwFlags & GAMEVAR_SYNCCHECK) && g_parsingActorPtr && C_CheckEventSync(g_currentEvent))
    {
        C_ReportError(-1);
        initprintf("%s:%d: warning: found local gamevar `%s' used within %s; "
                   "expect multiplayer synchronization issues.\n",g_szScriptFileName,
                   g_lineNumber,label+(g_numLabels<<6),g_parsingEventPtr?"a synced event":"an actor");
        g_numCompilerWarnings++;
    }
    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: accepted gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));

    bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
    *g_scriptPtr++=(i|f);
}

#define C_GetNextVar() C_GetNextVarType(0)

static inline void C_GetManyVarsType(int32_t type, int32_t num)
{
    int32_t i;
    for (i=num-1; i>=0; i--)
        C_GetNextVarType(type);
}

#define C_GetManyVars(num) C_GetManyVarsType(0,num)

static int32_t C_GetNextValue(int32_t type)
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0) // EOF
        return -1;

    l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (!g_skipKeywordCheck && hash_find(&h_keywords,tempbuf /*label+(g_numLabels<<6)*/)>=0)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_ISAKEYWORD);
        textptr+=l;
    }

    i = hash_find(&h_labels,tempbuf);
    if (i>=0)
    {
        char *el,*gl;

        if (labeltype[i] & type)
        {
            if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
            {
                gl = (char *)C_GetLabelType(labeltype[i]);
                initprintf("%s:%d: debug: accepted %s label `%s'.\n",g_szScriptFileName,g_lineNumber,gl,label+(i<<6));
                Bfree(gl);
            }
            if (labeltype[i] != LABEL_DEFINE && labelcode[i] >= (intptr_t)&script[0] && labelcode[i] < (intptr_t)&script[g_scriptSize])
                bitptr[(g_scriptPtr-script)>>3] |= (BITPTR_POINTER<<((g_scriptPtr-script)&7));
            else bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *(g_scriptPtr++) = labelcode[i];
            textptr += l;
            return labeltype[i];
        }
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *(g_scriptPtr++) = 0;
        textptr += l;
        el = (char *)C_GetLabelType(type);
        gl = (char *)C_GetLabelType(labeltype[i]);
        C_ReportError(-1);
        initprintf("%s:%d: warning: expected %s, found %s.\n",g_szScriptFileName,g_lineNumber,el,gl);
        g_numCompilerWarnings++;
        Bfree(el);
        Bfree(gl);
        return -1;  // valid label name, but wrong type
    }

    if (isdigit(*textptr) == 0 && *textptr != '-')
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        g_numCompilerErrors++;
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr = 0;
        g_scriptPtr++;
        textptr+=l;
        return -1; // error!
    }

    if (isdigit(*textptr) && g_labelsOnly)
    {
        C_ReportError(WARNING_LABELSONLY);
        g_numCompilerWarnings++;
    }

    i = l-1;
    do
    {
        // FIXME: check for 0-9 A-F for hex
        if (textptr[0] == '0' && textptr[1] == 'x') break; // kill the warning for hex
        if (!isdigit(textptr[i--]))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: invalid character `%c' in definition!\n",g_szScriptFileName,g_lineNumber,textptr[i+1]);
            g_numCompilerWarnings++;
            break;
        }
    }
    while (i > 0);

    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: accepted constant %d.\n",g_szScriptFileName,g_lineNumber,atol(textptr));
    bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));

    if (tolower(textptr[1])=='x')
        sscanf(textptr+2,"%" PRIxPTR "",g_scriptPtr);
    else
        *g_scriptPtr = atol(textptr);

    g_scriptPtr++;

    textptr += l;

    return 0;   // literal value
}

static inline int32_t C_IntPow2(int32_t v)
{
    return ((v!=0) && (v&(v-1))==0);
}

static inline uint32_t C_Pow2IntLogBase2(int32_t v)
{
    static const uint32_t b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                 0xFF00FF00, 0xFFFF0000
                                };
    register uint32_t r = (v & b[0]) != 0;
    int32_t i = 4;

    for (; i > 0; i--)
        r |= ((v & b[i]) != 0) << i;

    return r;
}

static int32_t C_CheckMalformedBranch(intptr_t lastScriptPtr)
{
    switch (C_GetKeyword())
    {
    case CON_RIGHTBRACE:
    case CON_ENDA:
    case CON_ENDEVENT:
    case CON_ENDS:
    case CON_ELSE:
        g_scriptPtr = lastScriptPtr + &script[0];
        g_ifElseAborted = 1;
        C_ReportError(-1);
        g_numCompilerWarnings++;
        initprintf("%s:%d: warning: malformed `%s' branch\n",g_szScriptFileName,g_lineNumber,
                   keyw[*(g_scriptPtr) & 0xFFF]);
        return 1;
    }
    return 0;
}

static int32_t C_CheckEmptyBranch(int32_t tw, intptr_t lastScriptPtr)
{
    // ifrnd and ifhitweapon actually do something when the condition is executed
    if ((Bstrncmp(keyw[tw], "if", 2) && tw != CON_ELSE) ||
            tw == CON_IFRND || tw == CON_IFHITWEAPON)
    {
        g_ifElseAborted = 0;
        return 0;
    }

    if ((*(g_scriptPtr) & 0xFFF) != CON_NULLOP || *(g_scriptPtr)>>12 != IFELSE_MAGIC)
        g_ifElseAborted = 0;

    if (g_ifElseAborted)
    {
        C_ReportError(-1);
        g_numCompilerWarnings++;
        g_scriptPtr = lastScriptPtr + &script[0];
        initprintf("%s:%d: warning: empty `%s' branch\n",g_szScriptFileName,g_lineNumber,
                   keyw[*(g_scriptPtr) & 0xFFF]);
        *(g_scriptPtr) = (CON_NULLOP + (IFELSE_MAGIC<<12));
        return 1;
    }
    return 0;
}

static int32_t C_ParseCommand(void);

static int32_t C_CountCaseStatements()
{
    int32_t lCount;
    char *temptextptr = textptr;
    int32_t temp_ScriptLineNumber = g_lineNumber;
    intptr_t scriptoffset = (unsigned)(g_scriptPtr-script);
    intptr_t caseoffset = (unsigned)(g_caseScriptPtr-script);
//    int32_t i;

    g_numCases=0;
    g_caseScriptPtr=NULL;
    //Bsprintf(g_szBuf,"CSS: %.12s",textptr);
    //AddLog(g_szBuf);
    while (C_ParseCommand() == 0)
    {
        //Bsprintf(g_szBuf,"CSSL: %.20s",textptr);
        //AddLog(g_szBuf);
        ;
    }
    // since we processed the endswitch, we need to re-increment g_checkingSwitch
    g_checkingSwitch++;

    textptr=temptextptr;
    g_scriptPtr = (intptr_t *)(script+scriptoffset);

    g_lineNumber = temp_ScriptLineNumber;

    lCount=g_numCases;
    g_numCases=0;
    g_caseScriptPtr = (intptr_t *)(script+caseoffset);
    g_numCases = 0;
    return lCount;
}

static int32_t C_ParseCommand(void)
{
    int32_t i, j=0, k=0, done, tw, otw = g_lastKeyword;
    char *temptextptr;
    intptr_t *tempscrptr = NULL;

    if (quitevent)
    {
        initprintf("Aborted.\n");
        G_Shutdown();
        exit(0);
    }

    if (g_numCompilerErrors > 63 || (*textptr == '\0') || (*(textptr+1) == '\0'))
        return 1;

    if (g_scriptDebug)
        C_ReportError(-1);

    if (g_checkingSwitch > 0)
    {
        //Bsprintf(g_szBuf,"PC(): '%.25s'",textptr);
        //AddLog(g_szBuf);
    }
    g_lastKeyword = tw = C_GetNextKeyword();
    //    Bsprintf(tempbuf,"%s",keyw[tw]);
    //    AddLog(tempbuf);

    if (C_SkipComments())
        return 1;

    switch (tw)
    {
    default:
    case -1:
        return 0; //End
    case CON_STATE:
        if (g_parsingActorPtr == 0 && g_processingState == 0)
        {
            C_GetNextLabelName();
            g_scriptPtr--;
            labelcode[g_numLabels] = (intptr_t) g_scriptPtr;
            labeltype[g_numLabels] = LABEL_STATE;

            g_processingState = 1;
            Bsprintf(g_szCurrentBlockName,"%s",label+(g_numLabels<<6));
            hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
            g_numLabels++;
            return 0;
        }

        C_GetNextLabelName();

        if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        i = hash_find(&h_gamevars,label+(g_numLabels<<6));
        if (i>=0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        j = hash_find(&h_labels,label+(g_numLabels<<6));
        if (j>=0)
        {
            if (labeltype[j] & LABEL_STATE)
            {
                if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: accepted state label `%s'.\n",g_szScriptFileName,g_lineNumber,label+(j<<6));
                *g_scriptPtr = labelcode[j];
                if (labelcode[j] >= (intptr_t)&script[0] && labelcode[j] < (intptr_t)&script[g_scriptSize])
                    bitptr[(g_scriptPtr-script)>>3] |= (BITPTR_POINTER<<((g_scriptPtr-script)&7));
                else bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                g_scriptPtr++;
                return 0;
            }
            else
            {
                char *gl = (char *)C_GetLabelType(labeltype[j]);
                C_ReportError(-1);
                initprintf("%s:%d: warning: expected state, found %s.\n",g_szScriptFileName,g_lineNumber,gl);
                g_numCompilerWarnings++;
                Bfree(gl);
                *(g_scriptPtr-1) = CON_NULLOP; // get rid of the state, leaving a nullop to satisfy if conditions
                bitptr[(g_scriptPtr-script-1)>>3] &= ~(1<<((g_scriptPtr-script-1)&7));
                return 0;  // valid label name, but wrong type
            }
        }
        else
        {
            C_ReportError(-1);
            initprintf("%s:%d: error: state `%s' not found.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            g_numCompilerErrors++;
        }
        g_scriptPtr++;
        return 0;

    case CON_ENDS:
        if (g_processingState == 0)
        {
            C_ReportError(-1);
            initprintf("%s:%d: error: found `ends' without open `state'.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
        }
        //            else
        {
            if (g_numBraces > 0)
            {
                C_ReportError(ERROR_OPENBRACKET);
                g_numCompilerErrors++;
            }
            else if (g_numBraces < 0)
            {
                C_ReportError(ERROR_CLOSEBRACKET);
                g_numCompilerErrors++;
            }
            if (g_checkingSwitch > 0)
            {
                C_ReportError(ERROR_NOENDSWITCH);
                g_numCompilerErrors++;

                g_checkingSwitch = 0; // can't be checking anymore...
            }

            g_processingState = 0;
            Bsprintf(g_szCurrentBlockName,"(none)");
        }
        return 0;

    case CON_SETTHISPROJECTILE:
    case CON_SETPROJECTILE:
        if (!C_CheckEventSync(g_currentEvent))
            C_ReportError(WARNING_EVENTSYNC);
    case CON_GETTHISPROJECTILE:
    case CON_GETPROJECTILE:
    {
        int32_t lLabelID;

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
            g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameOffset(&projectileH,Bstrtolower(label+(g_numLabels<<6)));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=ProjectileLabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",PlayerLabels[lLabelID].flags);

        // now at target VAR...

        // get the ID of the DEF
        switch (tw)
        {
        case CON_SETPROJECTILE:
        case CON_SETTHISPROJECTILE:
            C_GetNextVar();
            break;
        default:
            C_GetNextVarType(GAMEVAR_READONLY);
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
            C_GetNextLabelName();
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr -= 3; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }

        C_GetNextLabelName();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined

        if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        //printf("Translating number  '%.20s'\n",textptr);
        C_GetNextValue(LABEL_DEFINE); // get initial value
        //printf("Done Translating number.  '%.20s'\n",textptr);

        C_GetNextValue(LABEL_DEFINE); // get flags
        //Bsprintf(g_szBuf,"Adding GameVar='%s', val=%l, flags=%lX",label+(g_numLabels<<6),
        //      *(g_scriptPtr-2), *(g_scriptPtr-1));
        //AddLog(g_szBuf);
        if ((*(g_scriptPtr-1)&GAMEVAR_USER_MASK)==3)
        {
            g_numCompilerWarnings++;
            *(g_scriptPtr-1)^=GAMEVAR_PERPLAYER;
            C_ReportError(WARNING_BADGAMEVAR);
        }
        Gv_NewVar(label+(g_numLabels<<6),*(g_scriptPtr-2),
                  (*(g_scriptPtr-1))
                  // can't define default or secret
                  & (~(GAMEVAR_DEFAULT | GAMEVAR_SECRET))
                 );
        //AddLog("Added gamevar");
        g_scriptPtr -= 3; // no need to save in script...
        return 0;

    case CON_GAMEARRAY:
        if (isdigit(*textptr) || (*textptr == '-'))
        {
            C_GetNextLabelName();
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr -= 2; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }
        C_GetNextLabelName();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined

        if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        i = hash_find(&h_gamevars,label+(g_numLabels<<6));
        if (i>=0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        C_GetNextValue(LABEL_DEFINE);
        Gv_NewArray(label+(g_numLabels<<6),*(g_scriptPtr-1));

        g_scriptPtr -= 2; // no need to save in script...
        return 0;


    case CON_DEFINE:
    {
        //printf("Got definition. Getting Label. '%.20s'\n",textptr);
        C_GetNextLabelName();
        //printf("Got label. '%.20s'\n",textptr);
        // Check to see it's already defined

        if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        i = hash_find(&h_gamevars,label+(g_numLabels<<6));
        if (i>=0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        i = hash_find(&h_labels,label+(g_numLabels<<6));
        if (i>=0)
        {
            /*            if (i >= g_numDefaultLabels)
                        {
                            g_numCompilerWarnings++;
                            C_ReportError(WARNING_DUPLICATEDEFINITION);
                        } */
        }

        //printf("Translating. '%.20s'\n",textptr);
        C_GetNextValue(LABEL_DEFINE);
        //printf("Translated. '%.20s'\n",textptr);
        if (i == -1)
        {
            //              printf("Defining Definition '%s' to be '%d'\n",label+(g_numLabels<<6),*(g_scriptPtr-1));
            hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
            labeltype[g_numLabels] = LABEL_DEFINE;
            labelcode[g_numLabels++] = *(g_scriptPtr-1);
            if (*(g_scriptPtr-1) >= 0 && *(g_scriptPtr-1) < MAXTILES && g_dynamicTileMapping)
                G_ProcessDynamicTileMapping(label+((g_numLabels-1)<<6),*(g_scriptPtr-1));
        }
        g_scriptPtr -= 2;
        return 0;
    }

    case CON_PALFROM:
        for (j=3; j>=0; j--)
        {
            if (C_GetKeyword() == -1)
                C_GetNextValue(LABEL_DEFINE);
            else break;
        }

        while (j>-1)
        {
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr = 0;
            g_scriptPtr++;
            j--;
        }
        return 0;

    case CON_MOVE:
        if (g_parsingActorPtr || g_processingState)
        {
            if (!C_CheckEventSync(g_currentEvent))
            {
                g_numCompilerWarnings++;
                C_ReportError(WARNING_EVENTSYNC);
            }

            if ((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1))
            {
                C_ReportError(-1);
                bitptr[(g_scriptPtr-script-1)>>3] &= ~(1<<((g_scriptPtr-script-1)&7));
                *(g_scriptPtr-1) = 0;
                initprintf("%s:%d: warning: expected a move, found a constant.\n",g_szScriptFileName,g_lineNumber);
                g_numCompilerWarnings++;
            }

            j = 0;
            while (C_GetKeyword() == -1)
            {
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
                j |= *g_scriptPtr;
            }
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr = j;

            g_scriptPtr++;
        }
        else
        {
            g_scriptPtr--;
            C_GetNextLabelName();
            // Check to see it's already defined

            if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_ISAKEYWORD);
                return 0;
            }

            if (hash_find(&h_gamevars,label+(g_numLabels<<6))>=0)
            {
                g_numCompilerWarnings++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            if ((i = hash_find(&h_labels,label+(g_numLabels<<6))) >= 0)
            {
                g_numCompilerWarnings++;
                initprintf("%s:%d: warning: duplicate move `%s' ignored.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            }

            if (i == -1)
            {
                hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
                labeltype[g_numLabels] = LABEL_MOVE;
                labelcode[g_numLabels++] = (intptr_t) g_scriptPtr;
            }

            for (j=1; j>=0; j--)
            {
                if (C_GetKeyword() >= 0) break;
                C_GetNextValue(LABEL_DEFINE);
            }

            for (k=j; k>=0; k--)
            {
                bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                *g_scriptPtr = 0;
                g_scriptPtr++;
            }
        }
        return 0;

    case CON_MUSIC:
    {
        // NOTE: this doesn't get stored in the PCode...

        // music 1 stalker.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
        //    thecall.mid ahgeez.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE); // Volume Number (0/4)
        g_scriptPtr--;

        k = *g_scriptPtr-1;

        if (k >= 0) // if it's background music
        {
            i = 0;
            // get the file name...
            while (C_GetKeyword() == -1)
            {
                C_SkipComments();
                j = 0;
                tempbuf[j] = '/';
                while (isaltok(*(textptr+j)))
                {
                    tempbuf[j+1] = textptr[j];
                    j++;
                }
                tempbuf[j+1] = '\0';

                if (MapInfo[(k*MAXLEVELS)+i].musicfn == NULL)
                    MapInfo[(k*MAXLEVELS)+i].musicfn = Bcalloc(Bstrlen(tempbuf)+1,sizeof(uint8_t));
                else if ((Bstrlen(tempbuf)+1) > sizeof(MapInfo[(k*MAXLEVELS)+i].musicfn))
                    MapInfo[(k*MAXLEVELS)+i].musicfn = Brealloc(MapInfo[(k*MAXLEVELS)+i].musicfn,(Bstrlen(tempbuf)+1));

                Bstrcpy(MapInfo[(k*MAXLEVELS)+i].musicfn,tempbuf);

                textptr += j;
                if (i > MAXLEVELS-1) break;
                i++;
            }
        }
        else
        {
            i = 0;
            while (C_GetKeyword() == -1)
            {
                C_SkipComments();
                j = 0;

                while (isaltok(*(textptr+j)))
                {
                    EnvMusicFilename[i][j] = textptr[j];
                    j++;
                }
                EnvMusicFilename[i][j] = '\0';

                textptr += j;
                if (i > MAXVOLUMES-1) break;
                i++;
            }
        }
    }
    return 0;

    case CON_INCLUDE:
        g_scriptPtr--;

        C_SkipComments();

        j = 0;
        while (isaltok(*textptr))
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';

        {
            int32_t temp_ScriptLineNumber;
            int32_t  temp_ifelse_check;
            char *origtptr, *mptr;
            char parentScriptFileName[255];
            int32_t fp;

            fp = kopen4loadfrommod(tempbuf,g_loadFromGroupOnly);
            if (fp < 0)
            {
                g_numCompilerErrors++;
                initprintf("%s:%d: error: could not find file `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf);
                return 0;
            }

            j = kfilelength(fp);

            mptr = (char *)Bmalloc(j+1);
            if (!mptr)
            {
                kclose(fp);
                g_numCompilerErrors++;
                initprintf("%s:%d: error: could not allocate %d bytes to include `%s'.\n",
                           g_lineNumber,g_szScriptFileName,j,tempbuf);
                return 0;
            }

            initprintf("Including: %s (%d bytes)\n",tempbuf, j);
            kread(fp, mptr, j);
            kclose(fp);
            mptr[j] = 0;

            if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
                textptr++;
            origtptr = textptr;

            Bstrcpy(parentScriptFileName, g_szScriptFileName);
            Bstrcpy(g_szScriptFileName, tempbuf);
            temp_ScriptLineNumber = g_lineNumber;
            g_lineNumber = 1;
            temp_ifelse_check = g_checkingIfElse;
            g_checkingIfElse = 0;

            textptr = mptr;

            C_SkipComments();

            do done = C_ParseCommand();
            while (!done);

            Bstrcpy(g_szScriptFileName, parentScriptFileName);
            g_totalLines += g_lineNumber;
            g_lineNumber = temp_ScriptLineNumber;
            g_checkingIfElse = temp_ifelse_check;

            textptr = origtptr;

            Bfree(mptr);
        }
        return 0;

    case CON_AI:
        if (g_parsingActorPtr || g_processingState)
        {
            if (!C_CheckEventSync(g_currentEvent))
            {
                C_ReportError(WARNING_EVENTSYNC);
                g_numCompilerWarnings++;
            }
            C_GetNextValue(LABEL_AI);
        }
        else
        {
            g_scriptPtr--;
            C_GetNextLabelName();

            if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_ISAKEYWORD);
                return 0;
            }

            i = hash_find(&h_gamevars,label+(g_numLabels<<6));
            if (i>=0)
            {
                g_numCompilerWarnings++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            i = hash_find(&h_labels,label+(g_numLabels<<6));
            if (i>=0)
            {
                g_numCompilerWarnings++;
                initprintf("%s:%d: warning: duplicate ai `%s' ignored.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            }

            if (i == -1)
            {
                labeltype[g_numLabels] = LABEL_AI;
                hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
                labelcode[g_numLabels++] = (intptr_t) g_scriptPtr;
            }

            for (j=0; j<3; j++)
            {
                if (C_GetKeyword() >= 0) break;
                if (j == 1)
                    C_GetNextValue(LABEL_ACTION);
                else if (j == 2)
                {
                    if ((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1))
                    {
                        C_ReportError(-1);
                        bitptr[(g_scriptPtr-script-1)>>3] &= ~(1<<((g_scriptPtr-script-1)&7));
                        *(g_scriptPtr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",g_szScriptFileName,g_lineNumber);
                        g_numCompilerWarnings++;
                    }
                    k = 0;
                    while (C_GetKeyword() == -1)
                    {
                        C_GetNextValue(LABEL_DEFINE);
                        g_scriptPtr--;
                        k |= *g_scriptPtr;
                    }
                    bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                    *g_scriptPtr = k;
                    g_scriptPtr++;
                    return 0;
                }
            }
            for (k=j; k<3; k++)
            {
                bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                *g_scriptPtr = 0;
                g_scriptPtr++;
            }
        }
        return 0;

    case CON_ACTION:
        if (g_parsingActorPtr || g_processingState)
        {
            if (!C_CheckEventSync(g_currentEvent))
            {
                C_ReportError(WARNING_EVENTSYNC);
                g_numCompilerWarnings++;
            }
            C_GetNextValue(LABEL_ACTION);
        }
        else
        {
            g_scriptPtr--;
            C_GetNextLabelName();
            // Check to see it's already defined

            if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_ISAKEYWORD);
                return 0;
            }

            i = hash_find(&h_gamevars,label+(g_numLabels<<6));
            if (i>=0)
            {
                g_numCompilerWarnings++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            i = hash_find(&h_labels,label+(g_numLabels<<6));
            if (i>=0)
            {
                g_numCompilerWarnings++;
                initprintf("%s:%d: warning: duplicate action `%s' ignored.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            }

            if (i == -1)
            {
                labeltype[g_numLabels] = LABEL_ACTION;
                labelcode[g_numLabels] = (intptr_t) g_scriptPtr;
                hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
                g_numLabels++;
            }

            for (j=4; j>=0; j--)
            {
                if (C_GetKeyword() >= 0) break;
                C_GetNextValue(LABEL_DEFINE);
            }
            for (k=j; k>=0; k--)
            {
                bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                *(g_scriptPtr++) = 0;
            }
        }
        return 0;

    case CON_ACTOR:
        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_numBraces = 0;
        g_scriptPtr--;
        g_parsingActorPtr = g_scriptPtr;

        C_SkipComments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            g_szCurrentBlockName[j] = textptr[j];
            j++;
        }
        g_szCurrentBlockName[j] = 0;
        C_GetNextValue(LABEL_DEFINE);
        //         Bsprintf(g_szCurrentBlockName,"%s",label+(g_numLabels<<6));
        g_scriptPtr--;
        actorscrptr[*g_scriptPtr] = g_parsingActorPtr;

        for (j=0; j<4; j++)
        {
            bitptr[(g_parsingActorPtr+j-script)>>3] &= ~(1<<((g_parsingActorPtr+j-script)&7));
            *(g_parsingActorPtr+j) = 0;
            if (j == 3)
            {
                j = 0;
                while (C_GetKeyword() == -1)
                {
                    C_GetNextValue(LABEL_DEFINE);
                    g_scriptPtr--;
                    j |= *g_scriptPtr;
                }
                bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                *g_scriptPtr = j;
                g_scriptPtr++;
                break;
            }
            else
            {
                if (C_GetKeyword() >= 0)
                {
                    for (i=4-j; i; i--)
                    {
                        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                        *(g_scriptPtr++) = 0;
                    }
                    break;
                }
                switch (j)
                {
                case 0:
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                case 1:
                    C_GetNextValue(LABEL_ACTION);
                    break;
                case 2:
                    if ((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1))
                    {
                        C_ReportError(-1);
                        bitptr[(g_scriptPtr-script-1)>>3] &= ~(1<<((g_scriptPtr-script-1)&7));
                        *(g_scriptPtr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",g_szScriptFileName,g_lineNumber);
                        g_numCompilerWarnings++;
                    }
                    break;
                }
                if (*(g_scriptPtr-1) >= (intptr_t)&script[0] && *(g_scriptPtr-1) < (intptr_t)&script[g_scriptSize])
                    bitptr[(g_parsingActorPtr+j-script)>>3] |= (BITPTR_POINTER<<((g_parsingActorPtr+j-script)&7));
                else bitptr[(g_parsingActorPtr+j-script)>>3] &= ~(1<<((g_parsingActorPtr+j-script)&7));
                *(g_parsingActorPtr+j) = *(g_scriptPtr-1);
            }
        }
        g_checkingIfElse = 0;
        return 0;

    case CON_ONEVENT:
        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_numBraces = 0;
        g_scriptPtr--;
        g_parsingEventPtr = g_scriptPtr;
        g_parsingActorPtr = g_scriptPtr;

        C_SkipComments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            g_szCurrentBlockName[j] = textptr[j];
            j++;
        }
        g_szCurrentBlockName[j] = 0;
//        g_labelsOnly = 1;
        C_GetNextValue(LABEL_DEFINE);
        g_labelsOnly = 0;
        g_scriptPtr--;
        j= *g_scriptPtr;  // type of event
        g_currentEvent = j;
        //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, g_parsingEventPtr);
        //AddLog(g_szBuf);
        if (j > MAXGAMEEVENTS-1 || j < 0)
        {
            initprintf("%s:%d: error: invalid event ID.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            return 0;
        }
        // if event has already been declared then store previous script location
        if (apScriptGameEvent[j])
        {
            previous_event =apScriptGameEvent[j];
        }
        apScriptGameEvent[j]=g_parsingEventPtr;

        g_checkingIfElse = 0;

        return 0;

    case CON_EVENTLOADACTOR:
        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_numBraces = 0;
        g_scriptPtr--;
        g_parsingActorPtr = g_scriptPtr;

        C_SkipComments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            g_szCurrentBlockName[j] = textptr[j];
            j++;
        }
        g_szCurrentBlockName[j] = 0;
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        actorLoadEventScrptr[*g_scriptPtr] = g_parsingActorPtr;

        g_checkingIfElse = 0;
        return 0;

    case CON_USERACTOR:
        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_numBraces = 0;
        g_scriptPtr--;
        g_parsingActorPtr = g_scriptPtr;

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;

        C_SkipComments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            g_szCurrentBlockName[j] = textptr[j];
            j++;
        }
        g_szCurrentBlockName[j] = 0;

        j = *g_scriptPtr;

        if (j > 2)
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: invalid useractor type.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerWarnings++;
            j = 0;
        }

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        actorscrptr[*g_scriptPtr] = g_parsingActorPtr;
        ActorType[*g_scriptPtr] = j;

        for (j=0; j<4; j++)
        {
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *(g_parsingActorPtr+j) = 0;
            if (j == 3)
            {
                j = 0;
                while (C_GetKeyword() == -1)
                {
                    C_GetNextValue(LABEL_DEFINE);
                    g_scriptPtr--;
                    j |= *g_scriptPtr;
                }
                bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                *g_scriptPtr = j;
                g_scriptPtr++;
                break;
            }
            else
            {
                if (C_GetKeyword() >= 0)
                {
                    for (i=4-j; i; i--)
                    {
                        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
                        *(g_scriptPtr++) = 0;
                    }
                    break;
                }
                switch (j)
                {
                case 0:
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                case 1:
                    C_GetNextValue(LABEL_ACTION);
                    break;
                case 2:
                    if ((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1))
                    {
                        C_ReportError(-1);
                        bitptr[(g_scriptPtr-script-1)>>3] &= ~(1<<((g_scriptPtr-script-1)&7));
                        *(g_scriptPtr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",g_szScriptFileName,g_lineNumber);
                        g_numCompilerWarnings++;
                    }
                    break;
                }
                if (*(g_scriptPtr-1) >= (intptr_t)&script[0] && *(g_scriptPtr-1) < (intptr_t)&script[g_scriptSize])
                    bitptr[(g_parsingActorPtr+j-script)>>3] |= (BITPTR_POINTER<<((g_parsingActorPtr+j-script)&7));
                else bitptr[(g_parsingActorPtr+j-script)>>3] &= ~(1<<((g_parsingActorPtr+j-script)&7));
                *(g_parsingActorPtr+j) = *(g_scriptPtr-1);
            }
        }
        g_checkingIfElse = 0;
        return 0;

    case CON_INSERTSPRITEQ:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
        return 0;

    case CON_QSPRINTF:
        C_GetManyVars(2);

        j = 0;

        while (C_GetKeyword() == -1 && j < 32)
            C_GetNextVar(), j++;

        *g_scriptPtr++ = CON_NULLOP + (g_lineNumber<<12);
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
    case CON_SAVENN:
    case CON_SAVE:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_ANGOFF:
    case CON_QUOTE:
    case CON_SOUND:
    case CON_GLOBALSOUND:
    case CON_SOUNDONCE:
    case CON_STOPSOUND:
        C_GetNextValue(LABEL_DEFINE);
        if (tw == CON_CSTAT)
        {
            if (*(g_scriptPtr-1) == 32767)
            {
                *(g_scriptPtr-1) = 32768;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat 32767, using 32768 instead.\n",g_szScriptFileName,g_lineNumber);
                g_numCompilerWarnings++;
            }
            else if ((*(g_scriptPtr-1) & 32) && (*(g_scriptPtr-1) & 16))
            {
                i = *(g_scriptPtr-1);
                *(g_scriptPtr-1) ^= 48;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat %d, using %d instead.\n",g_szScriptFileName,g_lineNumber,i,*(g_scriptPtr-1));
                g_numCompilerWarnings++;
            }
        }
        return 0;

    case CON_HITRADIUSVAR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
        C_GetManyVars(5);
        break;
    case CON_HITRADIUS:
        C_GetNextValue(LABEL_DEFINE);
        C_GetNextValue(LABEL_DEFINE);
        C_GetNextValue(LABEL_DEFINE);
    case CON_ADDAMMO:
    case CON_ADDWEAPON:
    case CON_SIZETO:
    case CON_SIZEAT:
    case CON_DEBRIS:
    case CON_ADDINVENTORY:
    case CON_GUTS:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
        C_GetNextValue(LABEL_DEFINE);
        C_GetNextValue(LABEL_DEFINE);
        break;

    case CON_ELSE:
        if (g_checkingIfElse)
        {
            intptr_t offset;
            intptr_t lastScriptPtr = g_scriptPtr - &script[0] - 1;

            g_ifElseAborted = 0;
            g_checkingIfElse--;

            if (C_CheckMalformedBranch(lastScriptPtr))
                return 0;

            tempscrptr = g_scriptPtr;
            offset = (unsigned)(tempscrptr-script);
            g_scriptPtr++; //Leave a spot for the fail location
            C_ParseCommand();

            if (C_CheckEmptyBranch(tw, lastScriptPtr))
                return 0;

            tempscrptr = (intptr_t *)script+offset;
            *tempscrptr = (intptr_t) g_scriptPtr;
            bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));
        }
        else
        {
            g_scriptPtr--;
            tempscrptr = g_scriptPtr;
            g_numCompilerWarnings++;
            C_ReportError(-1);

            initprintf("%s:%d: warning: found `else' with no `if'.\n",g_szScriptFileName,g_lineNumber);

            if (C_GetKeyword() == CON_LEFTBRACE)
            {
                C_GetNextKeyword();
                g_numBraces++;

                do
                    done = C_ParseCommand();
                while (done == 0);
            }
            else C_ParseCommand();

            g_scriptPtr = tempscrptr;
        }
        return 0;

    case CON_SETSECTOR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETSECTOR:
    {
        int32_t lLabelID;

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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameID(SectorLabels,&sectorH,Bstrtolower(label+(g_numLabels<<6)));

        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=lLabelID;

        // now at target VAR...

        // get the ID of the DEF
        if (tw==CON_GETSECTOR)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
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

        C_GetNextValue(LABEL_DEFINE); // get <type>
        C_GetNextValue(LABEL_DEFINE); // get maxdist

        switch (tw)
        {
        case CON_FINDNEARACTORZ:
        case CON_FINDNEARSPRITEZ:
            C_GetNextValue(LABEL_DEFINE);
        default:
            break;
        }

        // target var
        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }

    case CON_FINDNEARACTORVAR:
    case CON_FINDNEARACTOR3DVAR:
    case CON_FINDNEARSPRITEVAR:
    case CON_FINDNEARSPRITE3DVAR:
    case CON_FINDNEARACTORZVAR:
    case CON_FINDNEARSPRITEZVAR:
    {
        C_GetNextValue(LABEL_DEFINE); // get <type>

        // get the ID of the DEF
        C_GetNextVar();
        switch (tw)
        {
        case CON_FINDNEARACTORZVAR:
        case CON_FINDNEARSPRITEZVAR:
            C_GetNextVar();
        default:
            break;
        }
        // target var
        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }

    case CON_SQRT:
    {
        // syntax sqrt <invar> <outvar>
        // gets the sqrt of invar into outvar

        // get the ID of the DEF
        C_GetNextVar();
        // target var
        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }

    case CON_SETWALL:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETWALL:
    {
        int32_t lLabelID;

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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameID(WallLabels,&wallH,Bstrtolower(label+(g_numLabels<<6)));

        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=lLabelID;

        // now at target VAR...

        // get the ID of the DEF
        if (tw == CON_GETWALL)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_SETPLAYER:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETPLAYER:
    {
        int32_t lLabelID;

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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameOffset(&playerH,Bstrtolower(label+(g_numLabels<<6)));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=PlayerLabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",PlayerLabels[lLabelID].flags);
        if (PlayerLabels[lLabelID].flags & LABEL_HASPARM2)
        {
            //printf("Member has PARM2\n");
            // get parm2
            // get the ID of the DEF
            C_GetNextVar();
        }
        else
        {
            //printf("Member does not have Parm2\n");
        }

        // now at target VAR...

        // get the ID of the DEF
        if (tw==CON_GETPLAYER)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_SETINPUT:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETINPUT:
    {
        int32_t lLabelID;

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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameOffset(&inputH,Bstrtolower(label+(g_numLabels<<6)));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=InputLabels[lLabelID].lId;

        // now at target VAR...

        // get the ID of the DEF
        if (tw==CON_GETINPUT)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_SETUSERDEF:
    case CON_GETUSERDEF:
    {
        int32_t lLabelID;

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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameID(UserdefsLabels,&userdefH,Bstrtolower(label+(g_numLabels<<6)));

        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=lLabelID;

        // now at target VAR...

        // get the ID of the DEF
        if (tw==CON_GETUSERDEF)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_SETACTORVAR:
    case CON_SETPLAYERVAR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;

        if (g_scriptPtr[-1] == g_iThisActorID) // convert to "setvarvar"
        {
            g_scriptPtr--;
            g_scriptPtr[-1]=CON_SETVARVAR;
            if (tw == CON_SETACTORVAR || tw == CON_SETPLAYERVAR)
            {
                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextVar();
            }
            else
            {
                g_scriptPtr++;
                C_GetNextVar();
                g_scriptPtr-=2;
                C_GetNextVarType(GAMEVAR_READONLY);
                g_scriptPtr++;
            }
            break;
        }

        /// now pointing at 'xxx'

        // get the ID of the DEF
        C_GetNextLabelName();
        //printf("found label of '%s'\n",   label+(g_numLabels<<6));

        // Check to see if it's a keyword
        if (hash_find(&h_keywords,label+(g_numLabels<<6))>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        i=GetDefID(label+(g_numLabels<<6));
        //printf("Label '%s' ID is %d\n",label+(g_numLabels<<6), i);
        if (i<0)
        {
            // not a defined DEF
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEVAR);
            return 0;
        }
        if (aGameVars[i].dwFlags & GAMEVAR_READONLY)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_VARREADONLY);
            return 0;
        }

        switch (tw)
        {
        case CON_SETACTORVAR:
        {
            if (!(aGameVars[i].dwFlags & GAMEVAR_PERACTOR))
            {
                g_numCompilerErrors++;
                C_ReportError(-1);
                initprintf("%s:%d: error: variable `%s' is not per-actor.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
                return 0;
            }
            break;
        }
        case CON_SETPLAYERVAR:
        {
            if (!(aGameVars[i].dwFlags & GAMEVAR_PERPLAYER))
            {
                g_numCompilerErrors++;
                C_ReportError(-1);
                initprintf("%s:%d: error: variable `%s' is not per-player.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
                return 0;
            }
            break;
        }
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=i; // the ID of the DEF (offset into array...)

        switch (tw)
        {
        case CON_GETACTORVAR:
        case CON_GETPLAYERVAR:
            C_GetNextVarType(GAMEVAR_READONLY);
            break;
        default:
            C_GetNextVar();
            break;
        }
        break;
    }

    case CON_SETACTOR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETACTOR:
    {
        int32_t lLabelID;

        // syntax getwall[<var>].x <VAR>
        // gets the value of wall[<var>].xxx into <VAR>

        // now get name of .xxx
        while ((*textptr != '['))
            textptr++;

        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
        // now get name of .xxx

        while (*textptr != '.')
        {
            if (*textptr == 0xa)
                break;
            if (!*textptr)
                break;

            textptr++;
        }

        if (*textptr != '.')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'

        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameOffset(&actorH,Bstrtolower(label+(g_numLabels<<6)));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=ActorLabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",ActorLabels[lLabelID].flags);
        if (ActorLabels[lLabelID].flags & LABEL_HASPARM2)
        {
            //printf("Member has PARM2\n");
            // get parm2
            // get the ID of the DEF
            C_GetNextVar();
        }

        // now at target VAR...

        // get the ID of the DEF
        if (tw == CON_GETACTOR)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_GETTSPR:
    case CON_SETTSPR:
    {
        int32_t lLabelID;

        if (g_currentEvent != EVENT_ANIMATESPRITES)
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: found `%s' outside of EVENT_ANIMATESPRITES\n",g_szScriptFileName,g_lineNumber,tempbuf);
            g_numCompilerWarnings++;
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
        g_labelsOnly = 1;
        C_GetNextVar();
        g_labelsOnly = 0;
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
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
            return 0;
        }
        textptr++;
        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels<<6));

        lLabelID=C_GetLabelNameOffset(&tspriteH,Bstrtolower(label+(g_numLabels<<6)));
        //printf("LabelID is %d\n",lLabelID);
        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=TsprLabels[lLabelID].lId;

        //printf("member's flags are: %02Xh\n",ActorLabels[lLabelID].flags);

        // now at target VAR...

        // get the ID of the DEF
        if (tw == CON_GETTSPR)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_STOPACTORSOUND:
        C_GetManyVars(2);
        break;

    case CON_SECTOROFWALL:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextVar();
        return 0;

    case CON_GETTICKS:
        if (C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_REVEVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_GETCURRADDRESS:
        C_GetNextVarType(GAMEVAR_READONLY);
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
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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
        C_GetNextVar();
        return 0;

    case CON_ENHANCED:
    {
        // don't store in pCode...
        g_scriptPtr--;
        //printf("We are enhanced, baby...\n");
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        if (*g_scriptPtr > BYTEVERSION_JF)
        {
            g_numCompilerWarnings++;
            initprintf("%s:%d: warning: need build %d, found build %d\n",g_szScriptFileName,g_lineNumber,k,BYTEVERSION_JF);
        }
        break;
    }

    case CON_DYNAMICREMAP:
    {
        g_scriptPtr--;
        if (g_dynamicTileMapping++)
        {
            initprintf("%s:%d: warning: duplicate dynamicremap statement\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerWarnings++;
        }
        else initprintf("Using dynamic tile remapping\n");
        break;
    }

    case CON_RANDVAR:
    case CON_ZSHOOT:
    case CON_EZSHOOT:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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

    {
        intptr_t *inst = g_scriptPtr-1;
        char *tptr = textptr;
        // syntax: [rand|add|set]var    <var1> <const1>
        // sets var1 to const1
        // adds const1 to var1 (const1 can be negative...)
        //printf("Found [add|set]var at line= %d\n",g_lineNumber);

        // get the ID of the DEF
        if (tw != CON_ZSHOOT && tw != CON_EZSHOOT)
            C_GetNextVarType(GAMEVAR_READONLY);
        else C_GetNextVar();

        C_GetNextValue(LABEL_DEFINE); // the number to check against...

        if (tw == CON_MULVAR && *(g_scriptPtr-1) == -1)
        {
            *inst = CON_INV;
            g_scriptPtr--;
            return 0;
        }

        if (tw == CON_DIVVAR || (tw == CON_MULVAR && *(g_scriptPtr-1) > 0))
        {
            int32_t i = *(g_scriptPtr-1);
            j = klabs(*(g_scriptPtr-1));

            if (C_IntPow2(j))
            {
                *inst = ((tw == CON_DIVVAR) ? CON_SHIFTVARR : CON_SHIFTVARL);
                *(g_scriptPtr-1) = C_Pow2IntLogBase2(j);
//                    initprintf("%s:%d: replacing multiply/divide with shift\n",g_szScriptFileName,g_lineNumber);

                if (i == j)
                    return 0;

                *g_scriptPtr++ = CON_INV + (g_lineNumber<<12);
                textptr = tptr;
                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
//                    initprintf("%s:%d: adding inversion\n",g_szScriptFileName,g_lineNumber);
            }
        }
    }
    return 0;
    case CON_WRITEARRAYTOFILE:
    case CON_READARRAYFROMFILE:
        C_GetNextLabelName();
        i=GetADefID(label+(g_numLabels<<6));
        if (i > (-1))
        {
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=i;
        }
        else
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEARRAY);
            return 1;
        }
        C_GetNextValue(LABEL_DEFINE);
        return 0;
    case CON_COPY:
        C_GetNextLabelName();
        i=GetADefID(label+(g_numLabels<<6));
        if (i > (-1))
        {
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=i;
        }
        else
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEARRAY);
            return 1;
        }
        C_SkipComments();// skip comments and whitespace
        if (*textptr != '[')

        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNO);
            return 1;
        }
        textptr++;
        C_GetNextVar();
        C_SkipComments();// skip comments and whitespace
        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return 1;
        }
        textptr++;
    case CON_SETARRAY:
        C_GetNextLabelName();
        i=GetADefID(label+(g_numLabels<<6));
        if (i > (-1))
        {
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=i;
        }
        else
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEARRAY);
            return 1;
        }

        C_SkipComments();// skip comments and whitespace
        if (*textptr != '[')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNO);
            return 1;
        }
        textptr++;
        C_GetNextVar();
        C_SkipComments();// skip comments and whitespace
        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return 1;
        }
        textptr++;
        C_GetNextVar();
        return 0;
    case CON_GETARRAYSIZE:
    case CON_RESIZEARRAY:
        C_GetNextLabelName();
        i=GetADefID(label+(g_numLabels<<6));
        if (i < 0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_NOTAGAMEARRAY);
            return 1;
        }

        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=i;

        C_SkipComments();
        C_GetNextVar();
        return 0;

    case CON_RANDVARVAR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextVar();
        return 0;

    case CON_SMAXAMMO:
    case CON_ADDWEAPONVAR:
    case CON_ACTIVATEBYSECTOR:
    case CON_OPERATESECTORS:
    case CON_OPERATEACTIVATORS:
    case CON_SSP:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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
            C_GetNextVarType(GAMEVAR_READONLY);
            break;
        default:
            C_GetNextVar();
            break;
        }

        // get the ID of the DEF
        if (tw == CON_GMAXAMMO)
            C_GetNextVarType(GAMEVAR_READONLY);
        else C_GetNextVar();

        switch (tw)
        {
        case CON_DIST:
        case CON_LDIST:
        case CON_GETANGLE:
        case CON_GETINCANGLE:
            C_GetNextVar();
            break;
        case CON_MULSCALE:
            C_GetManyVars(2);
            break;
        }
        return 0;

    case CON_FLASH:
    case CON_SAVEMAPSTATE:
    case CON_LOADMAPSTATE:
        return 0;

    case CON_DRAGPOINT:
    case CON_GETKEYNAME:
        C_GetManyVars(3);
        return 0;

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        C_GetManyVars(3);
        C_GetNextVarType(GAMEVAR_READONLY);
        return 0;

    case CON_DEFINEPROJECTILE:
    {
        int32_t y;
        int32_t z;

        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_scriptPtr--;

        C_GetNextValue(LABEL_DEFINE);
        j = *(g_scriptPtr-1);

        if (j > MAXTILES-1)
        {
            C_ReportError(ERROR_EXCEEDSMAXTILES);
            g_numCompilerErrors++;
        }

        C_GetNextValue(LABEL_DEFINE);
        y = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        z = *(g_scriptPtr-1);

        C_SetProjectile(j,y,z);
        SpriteFlags[j] |= SPRITE_PROJECTILE;
        return 0;
    }

    case CON_SPRITEFLAGS:
    {
        if (g_parsingActorPtr == 0 && g_processingState == 0)
        {
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (j > MAXTILES-1)
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_numCompilerErrors++;
            }

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            SpriteFlags[j] = *g_scriptPtr;

            return 0;
        }
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
        C_GetNextVar();
        return 0;
    }

    case CON_SPRITESHADOW:
    case CON_SPRITENVG:
    case CON_SPRITENOSHADE:
    case CON_SPRITENOPAL:
    case CON_PRECACHE:
    {
        if (g_processingState || g_parsingActorPtr)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_scriptPtr--;

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;

        if (j > MAXTILES-1)
        {
            C_ReportError(ERROR_EXCEEDSMAXTILES);
            g_numCompilerErrors++;
        }

        switch (tw)
        {
        case CON_SPRITESHADOW:
            SpriteFlags[*g_scriptPtr] |= SPRITE_SHADOW;
            break;
        case CON_SPRITENVG:
            SpriteFlags[*g_scriptPtr] |= SPRITE_NVG;
            break;
        case CON_SPRITENOSHADE:
            SpriteFlags[*g_scriptPtr] |= SPRITE_NOSHADE;
            break;
        case CON_SPRITENOPAL:
            SpriteFlags[*g_scriptPtr] |= SPRITE_NOPAL;
            break;
        case CON_PRECACHE:
            SpriteCacheList[*g_scriptPtr][0] = j;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            i = *g_scriptPtr;
            if (i > MAXTILES-1)
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_numCompilerErrors++;
            }
            SpriteCacheList[j][1] = i;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            i = *g_scriptPtr;
            SpriteCacheList[j][2] = i;
            break;
        }
        return 0;
    }

    case CON_IFACTORSOUND:
    case CON_IFVARVARG:
    case CON_IFVARVARL:
    case CON_IFVARVARE:
    case CON_IFVARVARN:
    case CON_IFVARVARAND:
    case CON_IFVARVAROR:
    case CON_IFVARVARXOR:
    case CON_IFVARVAREITHER:
    case CON_WHILEVARVARN:
    {
        intptr_t offset;
        intptr_t lastScriptPtr = g_scriptPtr - &script[0] - 1;

        g_ifElseAborted = 0;

        C_GetManyVars(2);

        if (C_CheckMalformedBranch(lastScriptPtr))
            return 0;

        tempscrptr = g_scriptPtr;
        offset = (unsigned)(g_scriptPtr-script);
        g_scriptPtr++; // Leave a spot for the fail location

        C_ParseCommand();

        if (C_CheckEmptyBranch(tw, lastScriptPtr))
            return 0;

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) g_scriptPtr;
        bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));

        if (tw != CON_WHILEVARVARN)
        {
            j = C_GetKeyword();

            if (j == CON_ELSE || j == CON_LEFTBRACE)
                g_checkingIfElse++;
        }
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
        C_GetNextValue(LABEL_DEFINE);
        return 0;

    case CON_IFVARL:
    case CON_IFVARG:
    case CON_IFVARE:
    case CON_IFVARN:
    case CON_IFVARAND:
    case CON_IFVAROR:
    case CON_IFVARXOR:
    case CON_IFVAREITHER:
    case CON_WHILEVARN:
    {
        intptr_t offset;
        intptr_t lastScriptPtr = (g_scriptPtr-script-1);

        g_ifElseAborted = 0;
        // get the ID of the DEF
        C_GetNextVar();
        C_GetNextValue(LABEL_DEFINE); // the number to check against...

        if (C_CheckMalformedBranch(lastScriptPtr))
            return 0;

        tempscrptr = g_scriptPtr;
        offset = (unsigned)(tempscrptr-script);
        g_scriptPtr++; //Leave a spot for the fail location

        C_ParseCommand();

        if (C_CheckEmptyBranch(tw, lastScriptPtr))
            return 0;

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) g_scriptPtr;
        bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));

        if (tw != CON_WHILEVARN)
        {
            j = C_GetKeyword();

            if (j == CON_ELSE || j == CON_LEFTBRACE)
                g_checkingIfElse++;
        }

        return 0;
    }
    case CON_ADDLOGVAR:

        // syntax: addlogvar <var>

        // prints the line number in the log file.
        /*        *g_scriptPtr=g_lineNumber;
                g_scriptPtr++; */

        // get the ID of the DEF
        C_GetNextVar();
        return 0;

    case CON_ROTATESPRITE16:
    case CON_ROTATESPRITE:
        if (g_parsingEventPtr == 0 && g_processingState == 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }

        // syntax:
        // int32_t x, int32_t y, int32_t z, short a, short tilenum, int8_t shade, char orientation, x1, y1, x2, y2
        // myospal adds char pal

        // get the ID of the DEFs

        C_GetManyVars(12);
        break;

    case CON_SHOWVIEW:
        if (g_parsingEventPtr == 0 && g_processingState == 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }

        C_GetManyVars(10);
        break;

    case CON_GETZRANGE:
        C_GetManyVars(4);
        C_GetManyVarsType(GAMEVAR_READONLY,4);
        C_GetManyVars(2);
        break;

    case CON_SECTSETINTERPOLATION:
    case CON_SECTCLEARINTERPOLATION:
        C_GetNextVar();
        break;

    case CON_CLIPMOVE:
    case CON_CLIPMOVENOSLIDE:
        // <retvar>,<x>,<y>,z,<sectnum>, xvect,yvect,walldist,floordist,ceildist,clipmask
        C_GetManyVarsType(GAMEVAR_READONLY,3);
        C_GetNextVar();
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetManyVars(6);
        break;

    case CON_CALCHYPOTENUSE:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetManyVars(2);
        break;

    case CON_LINEINTERSECT:
    case CON_RAYINTERSECT:
        // lineintersect x y z  x y z  x y  x y  <intx> <inty> <intz> <ret>
        // rayintersect x y z  vx vy vz  x y  x y  <intx> <inty> <intz> <ret>
        C_GetManyVars(10);
        C_GetManyVarsType(GAMEVAR_READONLY,4);
        break;

    case CON_HITSCAN:
    case CON_CANSEE:
        // get the ID of the DEF
        C_GetManyVars(tw==CON_CANSEE?8:7);
        C_GetManyVarsType(GAMEVAR_READONLY,tw==CON_CANSEE?1:6);
        if (tw==CON_HITSCAN) C_GetNextVar();
        break;

    case CON_CANSEESPR:
        C_GetManyVars(2);
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_NEARTAG:
        C_GetManyVars(5);
        C_GetManyVarsType(GAMEVAR_READONLY,4);
        C_GetManyVars(2);
        break;

    case CON_ROTATEPOINT:
        C_GetManyVars(5);
        C_GetManyVarsType(GAMEVAR_READONLY,2);
        break;

    case CON_GETTIMEDATE:
        C_GetManyVarsType(GAMEVAR_READONLY,8);
        break;

    case CON_MOVESPRITE:
    case CON_SETSPRITE:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
        C_GetManyVars(4);
        if (tw == CON_MOVESPRITE)
        {
            C_GetNextVar();
            C_GetNextVarType(GAMEVAR_READONLY);
        }
        break;

    case CON_MINITEXT:
    case CON_GAMETEXT:
    case CON_GAMETEXTZ:
    case CON_DIGITALNUMBER:
    case CON_DIGITALNUMBERZ:
        if (g_parsingEventPtr == 0 && g_processingState == 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }

        switch (tw)
        {
        case CON_GAMETEXTZ:
        case CON_DIGITALNUMBERZ:
            C_GetManyVars(1);
        case CON_GAMETEXT:
        case CON_DIGITALNUMBER:
            C_GetManyVars(6);
        default:
            C_GetManyVars(5);
            break;
        }
        break;

    case CON_UPDATESECTOR:
    case CON_UPDATESECTORZ:
        C_GetManyVars(2);
        if (tw==CON_UPDATESECTORZ)
            C_GetNextVar();
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_MYOS:
    case CON_MYOSPAL:
    case CON_MYOSX:
    case CON_MYOSPALX:
        if (g_parsingEventPtr == 0 && g_processingState == 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }

        // syntax:
        // int32_t x, int32_t y, short tilenum, int8_t shade, char orientation
        // myospal adds char pal

        C_GetManyVars(5);
        if (tw==CON_MYOSPAL || tw==CON_MYOSPALX)
        {
            // Parse: pal

            // get the ID of the DEF
            C_GetNextVar();
        }
        break;

    case CON_FINDPLAYER:
    case CON_FINDOTHERPLAYER:
    case CON_DISPLAYRAND:

        // syntax: displayrand <var>
        // gets rand (not game rand) into <var>

        // Get The ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_SWITCH:
    {
        intptr_t tempoffset;

        //AddLog("Got Switch statement");
        if (g_checkingSwitch)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: g_checkingSwitch=",__FILE__,__LINE__, g_checkingSwitch);
            //    AddLog(g_szBuf);
        }
        g_checkingSwitch++; // allow nesting (if other things work)
        // Get The ID of the DEF
        C_GetNextVar();

        tempscrptr= g_scriptPtr;
        tempoffset = (unsigned)(tempscrptr-script);
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=0; // leave spot for end location (for after processing)
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=0; // count of case statements
        g_caseScriptPtr=g_scriptPtr;        // the first case's pointer.
        bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
        *g_scriptPtr++=0; // leave spot for 'default' location (null if none)

        temptextptr=textptr;
        // probably does not allow nesting...

        //AddLog("Counting Case Statements...");

        j=C_CountCaseStatements();
//        initprintf("Done Counting Case Statements for switch %d: found %d.\n", g_checkingSwitch,j);
        g_scriptPtr+=j*2;
        C_SkipComments();
        g_scriptPtr-=j*2; // allocate buffer for the table
        tempscrptr = (intptr_t *)(script+tempoffset);

        //AddLog(g_szBuf);
        if (g_checkingSwitch>1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: g_checkingSwitch=",__FILE__,__LINE__, g_checkingSwitch);
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
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=0; // value check
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=0; // code offset
            C_SkipComments();
        }

        //Bsprintf(g_szBuf,"SWITCH1: '%.22s'",textptr);
        //AddLog(g_szBuf);

        g_numCases=0;
        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"SWITCH2: '%.22s'",textptr);
            //AddLog(g_szBuf);
        }
        tempscrptr = (intptr_t *)(script+tempoffset);

        //Bsprintf(g_szBuf,"SWITCHXX: '%.22s'",textptr);
        //AddLog(g_szBuf);
        // done processing switch.  clean up.
        if (g_checkingSwitch<1)
        {
            //    Bsprintf(g_szBuf,"ERROR::%s %d: g_checkingSwitch=%d",__FILE__,__LINE__, g_checkingSwitch);
            //    AddLog(g_szBuf);
        }
        g_numCases=0;
        if (tempscrptr)
        {
            intptr_t t,n;
            for (i=3; i<3+tempscrptr[1]*2-2; i+=2) // sort them
            {
                t=tempscrptr[i]; n=i;
                for (j=i+2; j<3+tempscrptr[1]*2; j+=2)
                    if (tempscrptr[j]<t) {t=tempscrptr[j]; n=j;}
                if (n!=i)
                {
                    t=tempscrptr[i  ]; tempscrptr[i  ]=tempscrptr[n  ]; tempscrptr[n  ]=t;
                    t=tempscrptr[i+1]; tempscrptr[i+1]=tempscrptr[n+1]; tempscrptr[n+1]=t;
                }
            }
//            for (j=3;j<3+tempscrptr[1]*2;j+=2)initprintf("%5d %8x\n",tempscrptr[j],tempscrptr[j+1]);
            tempscrptr[0]= (intptr_t)g_scriptPtr - (intptr_t)&script[0];    // save 'end' location
//            bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));
        }
        else
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
        }
        g_caseScriptPtr=NULL;
        // decremented in endswitch.  Don't decrement here...
        //                    g_checkingSwitch--; // allow nesting (maybe if other things work)
        tempscrptr=NULL;
        if (g_checkingSwitch)
        {
            //Bsprintf(g_szBuf,"ERROR::%s %d: g_checkingSwitch=%d",__FILE__,__LINE__, g_checkingSwitch);
            //AddLog(g_szBuf);
        }
        //AddLog("End of Switch statement");
    }
    break;

    case CON_CASE:
    {
        intptr_t tempoffset = 0;
        //AddLog("Found Case");

        if (g_checkingSwitch < 1)
        {
            g_numCompilerErrors++;
            C_ReportError(-1);
            initprintf("%s:%d: error: found `case' statement when not in switch\n",g_szScriptFileName,g_lineNumber);
            g_scriptPtr--;
            return 1;
        }

repeatcase:
        g_scriptPtr--; // don't save in code
        g_numCases++;
        //Bsprintf(g_szBuf,"case1: %.12s",textptr);
        //AddLog(g_szBuf);
        C_GetNextValue(LABEL_DEFINE);
        if (*textptr == ':')
            textptr++;
        //Bsprintf(g_szBuf,"case2: %.12s",textptr);
        //AddLog(g_szBuf);

        j= *(--g_scriptPtr);      // get value
        //Bsprintf(g_szBuf,"case: Value of case %d is %d",(int32_t)g_numCases,(int32_t)j);
        //AddLog(g_szBuf);
        if (g_caseScriptPtr)
        {
            for (i=(g_numCases/2)-1; i>=0; i--)
                if (g_caseScriptPtr[i*2+1]==j)
                {
                    g_numCompilerWarnings++;
                    C_ReportError(WARNING_DUPLICATECASE);
                    break;
                }
            //AddLog("Adding value to script");
            g_caseScriptPtr[g_numCases++]=j;   // save value
            g_caseScriptPtr[g_numCases]=(intptr_t)((intptr_t *)g_scriptPtr-&script[0]);  // save offset
        }
        //      j = C_GetKeyword();
        //Bsprintf(g_szBuf,"case3: %.12s",textptr);
        //AddLog(g_szBuf);

        if (C_GetKeyword() == CON_CASE)
        {
            //AddLog("Found Repeat Case");
            C_GetNextKeyword();    // eat 'case'
            goto repeatcase;
        }
        //Bsprintf(g_szBuf,"case4: '%.12s'",textptr);
        //AddLog(g_szBuf);
        tempoffset = (unsigned)(tempscrptr-script);

        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"case5 '%.25s'",textptr);
            //AddLog(g_szBuf);
            if (C_GetKeyword() == CON_CASE)
            {
                //AddLog("Found Repeat Case");
                C_GetNextKeyword();    // eat 'case'
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
        g_scriptPtr--;    // don't save

        C_SkipComments();

        if (*textptr == ':')
            textptr++;

        if (g_checkingSwitch<1)
        {
            g_numCompilerErrors++;
            C_ReportError(-1);
            initprintf("%s:%d: error: found `default' statement when not in switch\n",g_szScriptFileName,g_lineNumber);
            return 1;
        }
        if (g_caseScriptPtr && g_caseScriptPtr[0]!=0)
        {
            // duplicate default statement
            g_numCompilerErrors++;
            C_ReportError(-1);
            initprintf("%s:%d: error: multiple `default' statements found in switch\n",g_szScriptFileName,g_lineNumber);
        }
        if (g_caseScriptPtr)
        {
            g_caseScriptPtr[0]=(intptr_t)(g_scriptPtr-&script[0]);   // save offset
//            bitptr[(g_caseScriptPtr-script)>>3] |= (BITPTR_POINTER<<((g_caseScriptPtr-script)&7));
        }
        //Bsprintf(g_szBuf,"default: '%.22s'",textptr);
        //AddLog(g_szBuf);
        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"defaultParse: '%.22s'",textptr);
            //AddLog(g_szBuf);
            ;
        }
        break;

    case CON_ENDSWITCH:
        //AddLog("End Switch");
        g_checkingSwitch--;
        if (g_checkingSwitch < 0)
        {
            g_numCompilerErrors++;
            C_ReportError(-1);
            initprintf("%s:%d: error: found `endswitch' without matching `switch'\n",g_szScriptFileName,g_lineNumber);
        }
        return 1;      // end of block
        break;

    case CON_QSTRNCAT:
        C_GetManyVars(3);
        return 0;
    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
    case CON_ZSHOOTVAR:
    case CON_EZSHOOTVAR:
        if (!C_CheckEventSync(g_currentEvent))
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_EVENTSYNC);
        }
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
        C_GetManyVars(2);
        return 0;
    case CON_QSUBSTR:
        C_GetManyVars(4);
        return 0;
    case CON_SETACTORANGLE:
    case CON_SETPLAYERANGLE:
        if (!C_CheckEventSync(g_currentEvent))
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_EVENTSYNC);
        }
    case CON_GETANGLETOTARGET:
    case CON_GETACTORANGLE:
    case CON_GETPLAYERANGLE:
        // Syntax:   <command> <var>

        // get the ID of the DEF
        C_GetNextVar();
        return 0;

    case CON_ADDLOG:
        // syntax: addlog

        // prints the line number in the log file.
        /*        *g_scriptPtr=g_lineNumber;
                g_scriptPtr++; */
        return 0;

    case CON_IFRND:
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
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
    case CON_IFP:
    case CON_IFPINVENTORY:
    {
        intptr_t offset;
        intptr_t lastScriptPtr = (g_scriptPtr-&script[0]-1);

        g_ifElseAborted = 0;

        switch (tw)
        {
        case CON_IFAI:
            C_GetNextValue(LABEL_AI);
            break;
        case CON_IFACTION:
            C_GetNextValue(LABEL_ACTION);
            break;
        case CON_IFMOVE:
            if ((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1))
            {
                C_ReportError(-1);
                *(g_scriptPtr-1) = 0;
                initprintf("%s:%d: warning: expected a move, found a constant.\n",g_szScriptFileName,g_lineNumber);
                g_numCompilerWarnings++;
            }
            break;
        case CON_IFPINVENTORY:
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            break;
        case CON_IFP:
            j = 0;
            do
            {
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
                j |= *g_scriptPtr;
            }
            while (C_GetKeyword() == -1);
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr = j;
            g_scriptPtr++;
            break;
        case CON_IFSOUND:
        case CON_IFACTORSOUND:
            if (C_CheckEventSync(g_currentEvent))
            {
                C_ReportError(WARNING_REVEVENTSYNC);
                g_numCompilerWarnings++;
            }
        default:
            C_GetNextValue(LABEL_DEFINE);
            break;
        }

        if (C_CheckMalformedBranch(lastScriptPtr))
            return 0;

        tempscrptr = g_scriptPtr;
        offset = (unsigned)(tempscrptr-script);

        g_scriptPtr++; //Leave a spot for the fail location

        C_ParseCommand();

        if (C_CheckEmptyBranch(tw, lastScriptPtr))
            return 0;

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) g_scriptPtr;
        bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));

        j = C_GetKeyword();

        if (j == CON_ELSE || j == CON_LEFTBRACE)
            g_checkingIfElse++;

        return 0;
    }

    case CON_IFCLIENT:
    case CON_IFSERVER:
    case CON_IFONWATER:
    case CON_IFINWATER:
    case CON_IFACTORNOTSTAYPUT:
    case CON_IFCANSEE:
    case CON_IFHITWEAPON:
    case CON_IFSQUISHED:
    case CON_IFDEAD:
    case CON_IFCANSHOOTTARGET:
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
        intptr_t lastScriptPtr = (g_scriptPtr-&script[0]-1);

        g_ifElseAborted = 0;

        if (C_CheckMalformedBranch(lastScriptPtr))
            return 0;

        tempscrptr = g_scriptPtr;
        offset = (unsigned)(tempscrptr-script);

        g_scriptPtr++; //Leave a spot for the fail location

        C_ParseCommand();

        if (C_CheckEmptyBranch(tw, lastScriptPtr))
            return 0;

        tempscrptr = (intptr_t *)script+offset;
        *tempscrptr = (intptr_t) g_scriptPtr;
        bitptr[(tempscrptr-script)>>3] |= (BITPTR_POINTER<<((tempscrptr-script)&7));

        j = C_GetKeyword();

        if (j == CON_ELSE || j == CON_LEFTBRACE)
            g_checkingIfElse++;

        return 0;
    }

    case CON_LEFTBRACE:
        if (!(g_processingState || g_parsingActorPtr || g_parsingEventPtr))
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYNTAXERROR);
        }
        /*        if (C_GetKeyword() == CON_NULLOP)
                {
        //            initprintf("%s:%d: warning: 'nullop' statement has no effect\n",g_szScriptFileName,g_lineNumber);
                    C_GetNextKeyword();
                    g_scriptPtr--;
                }
                */
#if 0
        if (C_GetKeyword() == CON_RIGHTBRACE) // rewrite "{ }" into "nullop"
        {
//            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from left\n",g_szScriptFileName,g_lineNumber);
            *(--g_scriptPtr) = CON_NULLOP;
            C_GetNextKeyword();
            g_scriptPtr--;
            return 0;
        }
#endif
        g_numBraces++;

        do
            done = C_ParseCommand();
        while (done == 0);
        return 0;

    case CON_RIGHTBRACE:
        g_numBraces--;

        if ((*(g_scriptPtr-2)>>12) == (IFELSE_MAGIC) &&
                ((*(g_scriptPtr-2) & 0xFFF) == CON_LEFTBRACE)) // rewrite "{ }" into "nullop"
        {
//            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from right\n",g_szScriptFileName,g_lineNumber);
            *(g_scriptPtr-2) = CON_NULLOP + (IFELSE_MAGIC<<12);
            g_scriptPtr -= 2;

            if (C_GetKeyword() != CON_ELSE && (*(g_scriptPtr-2)&0xFFF) != CON_ELSE)
                g_ifElseAborted = 1;
            else g_ifElseAborted = 0;

            j = C_GetKeyword();

            if (g_checkingIfElse && j != CON_ELSE)
                g_checkingIfElse--;

            return 1;
        }

        if (g_numBraces < 0)
        {
            if (g_checkingSwitch)
            {
                C_ReportError(ERROR_NOENDSWITCH);
            }

            C_ReportError(-1);
            initprintf("%s:%d: error: found more `}' than `{'.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
        }
        if (g_checkingIfElse && j != CON_ELSE)
            g_checkingIfElse--;

        return 1;

    case CON_BETANAME:
        g_scriptPtr--;
        j = 0;
        C_NextLine();
        return 0;

    case CON_DEFINEVOLUMENAME:
        g_scriptPtr--;

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;
        C_SkipComments();

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",
                       g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            EpisodeNames[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(EpisodeNames[j])-1)
            {
                initprintf("%s:%d: warning: truncating volume name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,sizeof(EpisodeNames[j])-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        g_numVolumes = j+1;
        EpisodeNames[j][i] = '\0';
        return 0;

    case CON_DEFINEGAMEFUNCNAME:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;
        C_SkipComments();

        if (j < 0 || j > NUMGAMEFUNCTIONS-1)
        {
            initprintf("%s:%d: error: function number exceeds number of game functions.\n",
                       g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            gamefunctions[j][i] = *textptr;
            keydefaults[j*3][i] = *textptr;
            textptr++,i++;
            if (*textptr != 0x0a && *textptr != 0x0d && ispecial(*textptr))
            {
                initprintf("%s:%d: warning: invalid character in function name.\n",
                           g_szScriptFileName,g_lineNumber);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
            if (i >= MAXGAMEFUNCLEN-1)
            {
                initprintf("%s:%d: warning: truncating function name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,MAXGAMEFUNCLEN);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        gamefunctions[j][i] = '\0';
        keydefaults[j*3][i] = '\0';
        hash_add(&h_gamefuncs,gamefunctions[j],j,0);
        {
            char *str = Bstrtolower(Bstrdup(gamefunctions[j]));
            hash_add(&h_gamefuncs,str,j,0);
            Bfree(str);
        }

        return 0;

    case CON_DEFINESKILLNAME:
        g_scriptPtr--;

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;
        C_SkipComments();

        if (j < 0 || j > 4)
        {
            initprintf("%s:%d: error: skill number exceeds maximum skill count.\n",
                       g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            SkillNames[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(SkillNames[j])-1)
            {
                initprintf("%s:%d: warning: truncating skill name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,sizeof(SkillNames[j])-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        SkillNames[j][i] = '\0';
        return 0;

    case CON_SETGAMENAME:
    {
        char gamename[32];
        g_scriptPtr--;

        C_SkipComments();

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            gamename[i] = *textptr;
            textptr++,i++;
            if (i >= (signed)sizeof(gamename)-1)
            {
                initprintf("%s:%d: warning: truncating game name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,sizeof(gamename)-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        gamename[i] = '\0';
        g_gameNamePtr = Bstrdup(gamename);
        Bsprintf(tempbuf,"%s - " APPNAME,g_gameNamePtr);
        wm_setapptitle(tempbuf);
    }
    return 0;

    case CON_SETDEFNAME:
    {
        g_scriptPtr--;
        C_SkipComments();

        j = 0;
        while (isaltok(*textptr))
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';
        g_defNamePtr = Bstrdup(tempbuf);
        initprintf("Using DEF file: %s.\n",g_defNamePtr);
    }
    return 0;

    case CON_SETCFGNAME:
    {
        g_scriptPtr--;
        C_SkipComments();

        j = 0;
        while (isaltok(*textptr))
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';
        if (Bstrcmp(setupfilename,SETUPFILENAME) == 0) // not set to something else via -cfg
        {
            char temp[BMAX_PATH];
            struct stat st;
            int32_t fullscreen = ud.config.ScreenMode;
            int32_t xdim = ud.config.ScreenWidth, ydim = ud.config.ScreenHeight, bpp = ud.config.ScreenBPP;
            int32_t usemouse = ud.config.UseMouse, usejoy = ud.config.UseJoystick;
#if defined(POLYMOST) && defined(USE_OPENGL)
            int32_t glrm = glrendmode;
#endif

            if (Bstat(g_modDir, &st) < 0)
            {
                if (errno == ENOENT)     // path doesn't exist
                {
                    if (Bmkdir(g_modDir, S_IRWXU) < 0)
                    {
                        OSD_Printf("Failed to create configuration file directory %s\n", g_modDir);
                        return 0;
                    }
                    else OSD_Printf("Created configuration file directory %s\n", g_modDir);
                }
                else
                {
                    // another type of failure
                    return 0;
                }
            }
            else if ((st.st_mode & S_IFDIR) != S_IFDIR)
            {
                // directory isn't a directory
                return 0;
            }

            Bstrcpy(temp,tempbuf);
            CONFIG_WriteSetup();
            if (g_modDir[0] != '/')
                Bsprintf(setupfilename,"%s/",g_modDir);
            else setupfilename[0] = 0;
            Bstrcat(setupfilename,temp);

            initprintf("Using config file '%s'.\n",setupfilename);

            CONFIG_ReadSetup();

            ud.config.ScreenMode = fullscreen;
            ud.config.ScreenWidth = xdim;
            ud.config.ScreenHeight = ydim;
            ud.config.ScreenBPP = bpp;
            ud.config.UseMouse = usemouse;
            ud.config.UseJoystick = usejoy;
#if defined(POLYMOST) && defined(USE_OPENGL)
            glrendmode = glrm;
#endif

        }
    }
    return 0;

    case CON_DEFINEGAMETYPE:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;

        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        GametypeFlags[j] = *g_scriptPtr;

        C_SkipComments();

        if (j < 0 || j > MAXGAMETYPES-1)
        {
            initprintf("%s:%d: error: gametype number exceeds maximum gametype count.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }
        g_numGametypes = j+1;

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            GametypeNames[j][i] = toupper(*textptr);
            textptr++,i++;
            if (i >= (signed)sizeof(GametypeNames[j])-1)
            {
                initprintf("%s:%d: warning: truncating gametype name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,sizeof(GametypeNames[j])-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        GametypeNames[j][i] = '\0';
        return 0;

    case CON_DEFINELEVELNAME:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        j = *g_scriptPtr;
        C_GetNextValue(LABEL_DEFINE);
        g_scriptPtr--;
        k = *g_scriptPtr;
        C_SkipComments();

        if (j < 0 || j > MAXVOLUMES-1)
        {
            initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }
        if (k < 0 || k > MAXLEVELS-1)
        {
            initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
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
                initprintf("%s:%d: error: level file name exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,BMAX_PATH);
                g_numCompilerErrors++;
                while (*textptr != ' ' && *textptr != '\t') textptr++;
                break;
            }
        }
        tempbuf[i+1] = '\0';

        Bcorrectfilename(tempbuf,0);

        if (MapInfo[j *MAXLEVELS+k].filename == NULL)
            MapInfo[j *MAXLEVELS+k].filename = Bcalloc(Bstrlen(tempbuf)+1,sizeof(uint8_t));
        else if ((Bstrlen(tempbuf)+1) > sizeof(MapInfo[j*MAXLEVELS+k].filename))
            MapInfo[j *MAXLEVELS+k].filename = Brealloc(MapInfo[j*MAXLEVELS+k].filename,(Bstrlen(tempbuf)+1));

        Bstrcpy(MapInfo[j*MAXLEVELS+k].filename,tempbuf);

        C_SkipComments();

        MapInfo[j *MAXLEVELS+k].partime =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*REALGAMETICSPERSEC*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*REALGAMETICSPERSEC);

        textptr += 5;
        while (*textptr == ' '  || *textptr == '\t') textptr++;

        MapInfo[j *MAXLEVELS+k].designertime =
            (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*REALGAMETICSPERSEC*60)+
            (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*REALGAMETICSPERSEC);

        textptr += 5;
        while (*textptr == ' '  || *textptr == '\t') textptr++;

        i = 0;

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            tempbuf[i] = Btoupper(*textptr);
            textptr++,i++;
            if (i >= 32)
            {
                initprintf("%s:%d: warning: truncating level name to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,32);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }

        tempbuf[i] = '\0';

        if (MapInfo[j *MAXLEVELS+k].name == NULL)
            MapInfo[j *MAXLEVELS+k].name = Bcalloc(Bstrlen(tempbuf)+1,sizeof(uint8_t));
        else if ((Bstrlen(tempbuf)+1) > sizeof(MapInfo[j*MAXLEVELS+k].name))
            MapInfo[j *MAXLEVELS+k].name = Brealloc(MapInfo[j*MAXLEVELS+k].name,(Bstrlen(tempbuf)+1));

        /*         initprintf("level name string len: %d\n",Bstrlen(tempbuf)); */

        Bstrcpy(MapInfo[j*MAXLEVELS+k].name,tempbuf);

        return 0;

    case CON_DEFINEQUOTE:
    case CON_REDEFINEQUOTE:
        if (tw == CON_DEFINEQUOTE)
        {
            g_scriptPtr--;
        }

        C_GetNextValue(LABEL_DEFINE);

        k = *(g_scriptPtr-1);

        if (k >= MAXQUOTES)
        {
            initprintf("%s:%d: error: quote number exceeds limit of %d.\n",g_szScriptFileName,g_lineNumber,MAXQUOTES);
            g_numCompilerErrors++;
        }

        if (ScriptQuotes[k] == NULL)
            ScriptQuotes[k] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
        if (!ScriptQuotes[k])
        {
            ScriptQuotes[k] = NULL;
            Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(uint8_t) * MAXQUOTELEN);
            G_GameExit(tempbuf);
        }

        if (tw == CON_DEFINEQUOTE)
            g_scriptPtr--;

        i = 0;

        while (*textptr == ' ' || *textptr == '\t')
            textptr++;

        if (tw == CON_REDEFINEQUOTE)
        {
            if (ScriptQuoteRedefinitions[g_numQuoteRedefinitions] == NULL)
                ScriptQuoteRedefinitions[g_numQuoteRedefinitions] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
            if (!ScriptQuoteRedefinitions[g_numQuoteRedefinitions])
            {
                ScriptQuoteRedefinitions[g_numQuoteRedefinitions] = NULL;
                Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(uint8_t) * MAXQUOTELEN);
                G_GameExit(tempbuf);
            }
        }

        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
            /*
                        if (*textptr == '%' && *(textptr+1) == 's')
                        {
                            initprintf("%s:%d: error: quote text contains string identifier.\n",g_szScriptFileName,g_lineNumber);
                            g_numCompilerErrors++;
                            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
                            break;
                        }
            */
            if (tw == CON_DEFINEQUOTE)
                *(ScriptQuotes[k]+i) = *textptr;
            else
                *(ScriptQuoteRedefinitions[g_numQuoteRedefinitions]+i) = *textptr;
            textptr++,i++;
            if (i >= MAXQUOTELEN-1)
            {
                initprintf("%s:%d: warning: truncating quote text to %d characters.\n",g_szScriptFileName,g_lineNumber,MAXQUOTELEN-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        if (tw == CON_DEFINEQUOTE)
            *(ScriptQuotes[k]+i) = '\0';
        else
        {
            *(ScriptQuoteRedefinitions[g_numQuoteRedefinitions]+i) = '\0';
            bitptr[(g_scriptPtr-script)>>3] &= ~(BITPTR_POINTER<<((g_scriptPtr-script)&7));
            *g_scriptPtr++=g_numQuoteRedefinitions;
            g_numQuoteRedefinitions++;
        }
        return 0;

    case CON_CHEATKEYS:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        CheatKeys[0] = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        CheatKeys[1] = *(g_scriptPtr-1);
        g_scriptPtr -= 2;
        return 0;

    case CON_DEFINECHEAT:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        k = *(g_scriptPtr-1);

        if (k > 25)
        {
            initprintf("%s:%d: error: cheat redefinition attempts to redefine nonexistent cheat.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            C_NextLine();
            break;
        }
        g_scriptPtr--;
        i = 0;
        while (*textptr == ' ' || *textptr == '\t')
            textptr++;
        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ')
        {
            CheatStrings[k][i] = *textptr;
            textptr++,i++;
            if (i >= (signed)sizeof(CheatStrings[k])-1)
            {
                initprintf("%s:%d: warning: truncating cheat string to %d characters.\n",
                           g_szScriptFileName,g_lineNumber,MAXCHEATLEN,sizeof(CheatStrings[k])-1);
                g_numCompilerWarnings++;
                C_NextLine();
                break;
            }
        }
        CheatStrings[k][i] = '\0';
        return 0;

    case CON_DEFINESOUND:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        k = *(g_scriptPtr-1);
        if (k >= MAXSOUNDS)
        {
            initprintf("%s:%ld: error: exceeded sound limit of %ld.\n",g_szScriptFileName,g_lineNumber,MAXSOUNDS);
            g_numCompilerErrors++;
        }
        g_scriptPtr--;
        i = 0;
        C_SkipComments();

        if (g_sounds[k].filename == NULL)
            g_sounds[k].filename = Bcalloc(BMAX_PATH,sizeof(uint8_t));
        if (!g_sounds[k].filename)
        {
            Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte buffer.",sizeof(uint8_t) * BMAX_PATH);
            G_GameExit(tempbuf);
        }

        if (*textptr == '\"')
        {
            textptr++;
            while (*textptr && *textptr != '\"')
            {
                g_sounds[k].filename[i++] = *textptr++;
                if (i >= BMAX_PATH)
                {
                    initprintf("%s:%d: error: sound filename exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,BMAX_PATH);
                    g_numCompilerErrors++;
                    C_SkipComments();
                    break;
                }
            }
            textptr++;
        }
        else while (*textptr != ' ' && *textptr != '\t' && *textptr != '\r' && *textptr != '\n')
            {
                g_sounds[k].filename[i++] = *textptr++;
                if (i >= BMAX_PATH)
                {
                    initprintf("%s:%d: error: sound filename exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,BMAX_PATH);
                    g_numCompilerErrors++;
                    C_SkipComments();
                    break;
                }
            }
        g_sounds[k].filename[i] = '\0';

        C_GetNextValue(LABEL_DEFINE);
        g_sounds[k].ps = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        g_sounds[k].pe = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        g_sounds[k].pr = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        g_sounds[k].m = *(g_scriptPtr-1);
        C_GetNextValue(LABEL_DEFINE);
        g_sounds[k].vo = *(g_scriptPtr-1);
        g_scriptPtr -= 5;

        if (k > g_maxSoundPos)
            g_maxSoundPos = k;
        return 0;

    case CON_ENDEVENT:

        if (g_parsingEventPtr == 0)
        {
            C_ReportError(-1);
            initprintf("%s:%d: error: found `endevent' without open `onevent'.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
        }
        if (g_numBraces > 0)
        {
            C_ReportError(ERROR_OPENBRACKET);
            g_numCompilerErrors++;
        }
        if (g_numBraces < 0)
        {
            C_ReportError(ERROR_CLOSEBRACKET);
            g_numCompilerErrors++;
        }
        // if event has already been declared then put a jump in instead
        if (previous_event)
        {
            g_scriptPtr--;
            *(g_scriptPtr++) = CON_JUMP;
            *(g_scriptPtr++) = MAXGAMEVARS;
            *(g_scriptPtr++) = previous_event-script;
            *(g_scriptPtr++) = CON_ENDEVENT;
            previous_event = NULL;
        }
        g_parsingEventPtr = g_parsingActorPtr = 0;
        g_currentEvent = -1;
        Bsprintf(g_szCurrentBlockName,"(none)");
        return 0;

    case CON_ENDA:
        if (g_parsingActorPtr == 0)
        {
            C_ReportError(-1);
            initprintf("%s:%d: error: found `enda' without open `actor'.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
        }
        if (g_numBraces > 0)
        {
            C_ReportError(ERROR_OPENBRACKET);
            g_numCompilerErrors++;
        }
        if (g_numBraces < 0)
        {
            C_ReportError(ERROR_CLOSEBRACKET);
            g_numCompilerErrors++;
        }
        g_parsingActorPtr = 0;
        Bsprintf(g_szCurrentBlockName,"(none)");
        return 0;

    case CON_RETURN:
    case CON_BREAK:
        if (g_checkingSwitch)
        {
            if (otw == CON_BREAK)
            {
                C_ReportError(-1);
                initprintf("%s:%d: warning: duplicate `break'.\n",g_szScriptFileName, g_lineNumber);
                g_numCompilerWarnings++;
                g_scriptPtr--;
                return 0;
            }
            return 1;
        }
        return 0;

    case CON_SCRIPTSIZE:
        g_scriptPtr--;
        C_GetNextValue(LABEL_DEFINE);
        j = *(g_scriptPtr-1);
        g_scriptPtr--;
        C_SkipComments();
        return C_SetScriptSize(j);

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
        if (!C_CheckEventSync(g_currentEvent))
        {
            C_ReportError(WARNING_EVENTSYNC);
            g_numCompilerWarnings++;
        }
    case CON_NULLOP:
        if (tw == CON_NULLOP)
        {
            if (C_GetKeyword() != CON_ELSE)
            {
                C_ReportError(-1);
                g_numCompilerWarnings++;
                initprintf("%s:%d: warning: `nullop' found without `else'\n",g_szScriptFileName,g_lineNumber);
                g_scriptPtr--;
                g_ifElseAborted = 1;
            }
        }
    case CON_STOPALLSOUNDS:
        return 0;
    case CON_GAMESTARTUP:
    {
        int32_t params[30];

        g_scriptPtr--;
        for (j = 0; j < 30; j++)
        {
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            params[j] = *g_scriptPtr;

            if (j != 25) continue;

            if (C_GetKeyword() != -1)
            {
//                initprintf("Duke Nukem 3D v1.3D style CON files detected.\n");
                break;
            }
            else
            {
                g_scriptVersion = 14;
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
        g_impactDamage = params[j++];
        g_maxPlayerHealth = g_player[0].ps->max_player_health = g_player[0].ps->max_shield_amount = params[j++];
        g_startArmorAmount = params[j++];
        g_actorRespawnTime = params[j++];
        g_itemRespawnTime = params[j++];
        g_playerFriction = params[j++];
        if (g_scriptVersion == 14) g_spriteGravity = params[j++];
        g_rpgBlastRadius = params[j++];
        g_pipebombBlastRadius = params[j++];
        g_shrinkerBlastRadius = params[j++];
        g_tripbombBlastRadius = params[j++];
        g_morterBlastRadius = params[j++];
        g_bouncemineBlastRadius = params[j++];
        g_seenineBlastRadius = params[j++];
        g_player[0].ps->max_ammo_amount[PISTOL_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[SHOTGUN_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[CHAINGUN_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[RPG_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[HANDBOMB_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[SHRINKER_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[TRIPBOMB_WEAPON] = params[j++];
        g_player[0].ps->max_ammo_amount[FREEZE_WEAPON] = params[j++];
        if (g_scriptVersion == 14) g_player[0].ps->max_ammo_amount[GROW_WEAPON] = params[j++];
        g_damageCameras = params[j++];
        g_numFreezeBounces = params[j++];
        g_freezerSelfDamage = params[j++];
        if (g_scriptVersion == 14)
        {
            g_spriteDeleteQueueSize = params[j++];
            if (g_spriteDeleteQueueSize > 1024) g_spriteDeleteQueueSize = 1024;
            else if (g_spriteDeleteQueueSize < 0) g_spriteDeleteQueueSize = 0;

            g_tripbombLaserMode = params[j++];
        }
    }
    return 0;
    }
    return 0;
}

/* Anything added with C_AddDefinition() cannot be overwritten in the CONs */

static void C_AddDefinition(const char *lLabel,int32_t lValue,int32_t lType)
{
    Bstrcpy(label+(g_numLabels<<6),lLabel);
    labeltype[g_numLabels] = lType;
    hash_add(&h_labels,label+(g_numLabels<<6),g_numLabels,0);
    labelcode[g_numLabels++] = lValue;
    g_numDefaultLabels++;
}

static void C_AddDefaultDefinitions(void)
{
    C_AddDefinition("EVENT_AIMDOWN",EVENT_AIMDOWN,LABEL_DEFINE);
    C_AddDefinition("EVENT_AIMUP",EVENT_AIMUP,LABEL_DEFINE);
    C_AddDefinition("EVENT_ANIMATESPRITES",EVENT_ANIMATESPRITES,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHANGEWEAPON",EVENT_CHANGEWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETBOOT",EVENT_CHEATGETBOOT,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETFIRSTAID",EVENT_CHEATGETFIRSTAID,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETHEAT",EVENT_CHEATGETHEAT,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETHOLODUKE",EVENT_CHEATGETHOLODUKE,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETJETPACK",EVENT_CHEATGETJETPACK,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETSCUBA",EVENT_CHEATGETSCUBA,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETSHIELD",EVENT_CHEATGETSHIELD,LABEL_DEFINE);
    C_AddDefinition("EVENT_CHEATGETSTEROIDS",EVENT_CHEATGETSTEROIDS,LABEL_DEFINE);
    C_AddDefinition("EVENT_CROUCH",EVENT_CROUCH,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYCROSSHAIR",EVENT_DISPLAYCROSSHAIR,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYREST",EVENT_DISPLAYREST,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYBONUSSCREEN",EVENT_DISPLAYBONUSSCREEN,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYMENU",EVENT_DISPLAYMENU,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYMENUREST",EVENT_DISPLAYMENUREST,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYLOADINGSCREEN",EVENT_DISPLAYLOADINGSCREEN,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYROOMS",EVENT_DISPLAYROOMS,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYSBAR",EVENT_DISPLAYSBAR,LABEL_DEFINE);
    C_AddDefinition("EVENT_DISPLAYWEAPON",EVENT_DISPLAYWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_DOFIRE",EVENT_DOFIRE,LABEL_DEFINE);
    C_AddDefinition("EVENT_DRAWWEAPON",EVENT_DRAWWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_EGS",EVENT_EGS,LABEL_DEFINE);
    C_AddDefinition("EVENT_ENTERLEVEL",EVENT_ENTERLEVEL,LABEL_DEFINE);
    C_AddDefinition("EVENT_FAKEDOMOVETHINGS",EVENT_FAKEDOMOVETHINGS,LABEL_DEFINE);
    C_AddDefinition("EVENT_FIRE",EVENT_FIRE,LABEL_DEFINE);
    C_AddDefinition("EVENT_FIREWEAPON",EVENT_FIREWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_GAME",EVENT_GAME,LABEL_DEFINE);
    C_AddDefinition("EVENT_GETAUTOAIMANGLE",EVENT_GETAUTOAIMANGLE,LABEL_DEFINE);
    C_AddDefinition("EVENT_GETLOADTILE",EVENT_GETLOADTILE,LABEL_DEFINE);
    C_AddDefinition("EVENT_GETMENUTILE",EVENT_GETMENUTILE,LABEL_DEFINE);
    C_AddDefinition("EVENT_GETSHOTRANGE",EVENT_GETSHOTRANGE,LABEL_DEFINE);
    C_AddDefinition("EVENT_HOLODUKEOFF",EVENT_HOLODUKEOFF,LABEL_DEFINE);
    C_AddDefinition("EVENT_HOLODUKEON",EVENT_HOLODUKEON,LABEL_DEFINE);
    C_AddDefinition("EVENT_HOLSTER",EVENT_HOLSTER,LABEL_DEFINE);
    C_AddDefinition("EVENT_INCURDAMAGE",EVENT_INCURDAMAGE,LABEL_DEFINE);
    C_AddDefinition("EVENT_INIT",EVENT_INIT,LABEL_DEFINE);
    C_AddDefinition("EVENT_INVENTORY",EVENT_INVENTORY,LABEL_DEFINE);
    C_AddDefinition("EVENT_INVENTORYLEFT",EVENT_INVENTORYLEFT,LABEL_DEFINE);
    C_AddDefinition("EVENT_INVENTORYRIGHT",EVENT_INVENTORYRIGHT,LABEL_DEFINE);
    C_AddDefinition("EVENT_JUMP",EVENT_JUMP,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOGO",EVENT_LOGO,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOOKDOWN",EVENT_LOOKDOWN,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOOKLEFT",EVENT_LOOKLEFT,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOOKRIGHT",EVENT_LOOKRIGHT,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOOKUP",EVENT_LOOKUP,LABEL_DEFINE);
    C_AddDefinition("EVENT_MOVEBACKWARD",EVENT_MOVEBACKWARD,LABEL_DEFINE);
    C_AddDefinition("EVENT_MOVEFORWARD",EVENT_MOVEFORWARD,LABEL_DEFINE);
    C_AddDefinition("EVENT_NEXTWEAPON",EVENT_NEXTWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_PREVIOUSWEAPON",EVENT_PREVIOUSWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_PRESSEDFIRE",EVENT_PRESSEDFIRE,LABEL_DEFINE);
    C_AddDefinition("EVENT_PROCESSINPUT",EVENT_PROCESSINPUT,LABEL_DEFINE);
    C_AddDefinition("EVENT_QUICKKICK",EVENT_QUICKKICK,LABEL_DEFINE);
    C_AddDefinition("EVENT_RESETINVENTORY",EVENT_RESETINVENTORY,LABEL_DEFINE);
    C_AddDefinition("EVENT_RESETPLAYER",EVENT_RESETPLAYER,LABEL_DEFINE);
    C_AddDefinition("EVENT_RESETWEAPONS",EVENT_RESETWEAPONS,LABEL_DEFINE);
    C_AddDefinition("EVENT_RETURNTOCENTER",EVENT_RETURNTOCENTER,LABEL_DEFINE);
    C_AddDefinition("EVENT_SELECTWEAPON",EVENT_SELECTWEAPON,LABEL_DEFINE);
    C_AddDefinition("EVENT_SOARDOWN",EVENT_SOARDOWN,LABEL_DEFINE);
    C_AddDefinition("EVENT_SOARUP",EVENT_SOARUP,LABEL_DEFINE);
    C_AddDefinition("EVENT_SPAWN",EVENT_SPAWN,LABEL_DEFINE);
    C_AddDefinition("EVENT_STRAFELEFT",EVENT_STRAFELEFT,LABEL_DEFINE);
    C_AddDefinition("EVENT_STRAFERIGHT",EVENT_STRAFERIGHT,LABEL_DEFINE);
    C_AddDefinition("EVENT_SWIMDOWN",EVENT_SWIMDOWN,LABEL_DEFINE);
    C_AddDefinition("EVENT_SWIMUP",EVENT_SWIMUP,LABEL_DEFINE);
    C_AddDefinition("EVENT_TURNAROUND",EVENT_TURNAROUND,LABEL_DEFINE);
    C_AddDefinition("EVENT_TURNLEFT",EVENT_TURNLEFT,LABEL_DEFINE);
    C_AddDefinition("EVENT_TURNRIGHT",EVENT_TURNRIGHT,LABEL_DEFINE);
    C_AddDefinition("EVENT_USE",EVENT_USE,LABEL_DEFINE);
    C_AddDefinition("EVENT_USEJETPACK",EVENT_USEJETPACK,LABEL_DEFINE);
    C_AddDefinition("EVENT_USEMEDKIT",EVENT_USEMEDKIT,LABEL_DEFINE);
    C_AddDefinition("EVENT_USENIGHTVISION",EVENT_USENIGHTVISION,LABEL_DEFINE);
    C_AddDefinition("EVENT_USESTEROIDS",EVENT_USESTEROIDS,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY10",EVENT_WEAPKEY10,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY1",EVENT_WEAPKEY1,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY2",EVENT_WEAPKEY2,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY3",EVENT_WEAPKEY3,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY4",EVENT_WEAPKEY4,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY5",EVENT_WEAPKEY5,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY6",EVENT_WEAPKEY6,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY7",EVENT_WEAPKEY7,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY8",EVENT_WEAPKEY8,LABEL_DEFINE);
    C_AddDefinition("EVENT_WEAPKEY9",EVENT_WEAPKEY9,LABEL_DEFINE);
    C_AddDefinition("EVENT_KILLIT",EVENT_KILLIT,LABEL_DEFINE);
    C_AddDefinition("EVENT_LOADACTOR",EVENT_LOADACTOR,LABEL_DEFINE);
    C_AddDefinition("EVENT_NEWGAME",EVENT_NEWGAME,LABEL_DEFINE);

    C_AddDefinition("STR_MAPNAME",STR_MAPNAME,LABEL_DEFINE);
    C_AddDefinition("STR_MAPFILENAME",STR_MAPFILENAME,LABEL_DEFINE);
    C_AddDefinition("STR_PLAYERNAME",STR_PLAYERNAME,LABEL_DEFINE);
    C_AddDefinition("STR_VERSION",STR_VERSION,LABEL_DEFINE);
    C_AddDefinition("STR_GAMETYPE",STR_GAMETYPE,LABEL_DEFINE);
    C_AddDefinition("STR_VOLUMENAME",STR_VOLUMENAME,LABEL_DEFINE);

    C_AddDefinition("NO",0,LABEL_DEFINE|LABEL_ACTION|LABEL_AI|LABEL_MOVE);

    C_AddDefinition("PROJ_BOUNCES",PROJ_BOUNCES,LABEL_DEFINE);
    C_AddDefinition("PROJ_BSOUND",PROJ_BSOUND,LABEL_DEFINE);
    C_AddDefinition("PROJ_CLIPDIST",PROJ_CLIPDIST,LABEL_DEFINE);
    C_AddDefinition("PROJ_CSTAT",PROJ_CSTAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_DECAL",PROJ_DECAL,LABEL_DEFINE);
    C_AddDefinition("PROJ_DROP",PROJ_DROP,LABEL_DEFINE);
    C_AddDefinition("PROJ_EXTRA",PROJ_EXTRA,LABEL_DEFINE);
    C_AddDefinition("PROJ_EXTRA_RAND",PROJ_EXTRA_RAND,LABEL_DEFINE);
    C_AddDefinition("PROJ_FLASH_COLOR",PROJ_FLASH_COLOR,LABEL_DEFINE);
    C_AddDefinition("PROJ_HITRADIUS",PROJ_HITRADIUS,LABEL_DEFINE);
    C_AddDefinition("PROJ_ISOUND",PROJ_ISOUND,LABEL_DEFINE);
    C_AddDefinition("PROJ_OFFSET",PROJ_OFFSET,LABEL_DEFINE);
    C_AddDefinition("PROJ_PAL",PROJ_PAL,LABEL_DEFINE);
    C_AddDefinition("PROJ_RANGE",PROJ_RANGE,LABEL_DEFINE);
    C_AddDefinition("PROJ_SHADE",PROJ_SHADE,LABEL_DEFINE);
    C_AddDefinition("PROJ_SOUND",PROJ_SOUND,LABEL_DEFINE);
    C_AddDefinition("PROJ_SPAWNS",PROJ_SPAWNS,LABEL_DEFINE);
    C_AddDefinition("PROJ_SXREPEAT",PROJ_SXREPEAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_SYREPEAT",PROJ_SYREPEAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_TNUM",PROJ_TNUM,LABEL_DEFINE);
    C_AddDefinition("PROJ_TOFFSET",PROJ_TOFFSET,LABEL_DEFINE);
    C_AddDefinition("PROJ_TRAIL",PROJ_TRAIL,LABEL_DEFINE);
    C_AddDefinition("PROJ_TXREPEAT",PROJ_TXREPEAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_TYREPEAT",PROJ_TYREPEAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_VEL_MULT",PROJ_VEL_MULT,LABEL_DEFINE);
    C_AddDefinition("PROJ_VEL",PROJ_VEL,LABEL_DEFINE);
    C_AddDefinition("PROJ_WORKSLIKE",PROJ_WORKSLIKE,LABEL_DEFINE);
    C_AddDefinition("PROJ_XREPEAT",PROJ_XREPEAT,LABEL_DEFINE);
    C_AddDefinition("PROJ_YREPEAT",PROJ_YREPEAT,LABEL_DEFINE);
}

#pragma pack(push,1)
static void C_InitProjectiles(void)
{
    int32_t i;

    typedef struct
    {
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
    } defaultprojectile_t;

    defaultprojectile_t DefaultProjectile =
    {
        1, -1, 2048, 0, 0, SMALLSMOKE, -1, -1, 600, BULLETHOLE, -1, 0, 0, 448, g_numFreezeBounces, PIPEBOMB_BOUNCE, 1,
        100, -1, -1, -1, -1, -1, -96, 18, 18, 0, 1, 32
    };

    // this will only happen if I forget to update this function...
    if (offsetof(projectile_t, filler) != sizeof(DefaultProjectile))
        G_GameExit("ERROR: C_InitProjectiles(): projectile_t mismatch!");

    for (i=MAXTILES-1; i>=0; i--)
        Bmemcpy(&ProjectileData[i],&DefaultProjectile,sizeof(projectile_t));

    Bmemcpy(&DefaultProjectileData[0], &ProjectileData[0], sizeof(ProjectileData));
}
#pragma pack(pop)

extern int32_t g_numObituaries;
extern int32_t g_numSelfObituaries;

void C_Compile(const char *filenam)
{
    char *mptr;
    int32_t i;
    int32_t fs,fp;
    int32_t startcompiletime;

    clearbuf(apScriptGameEvent,MAXGAMEEVENTS,0L);

    C_InitHashes();
    Gv_Init();
    C_InitProjectiles();

    fp = kopen4loadfrommod((char *)filenam,g_loadFromGroupOnly);
    if (fp == -1) // JBF: was 0
    {
        extern int32_t numgroupfiles;

        if (g_loadFromGroupOnly == 1 || numgroupfiles == 0)
        {
#ifdef WIN32
            Bsprintf(tempbuf,"Duke Nukem 3D game data was not found.  A valid copy of '%s' or other compatible data is needed to run EDuke32.\n\n"
                     "You can find '%s' in the 'DN3DINST' or 'ATOMINST' directory on your Duke Nukem 3D installation CD.\n\n"
                     "If you don't already own a copy of Duke or haven't seen your disc in years, don't worry -- you can download the full, registered "
                     "version of Duke Nukem 3D: Atomic Edition immediately for only $5.99 through our partnership with GOG.com.\n\n"
                     "Not a typo; it's less than 6 bucks.  Get Duke now?\n\n"
                     "(Clicking yes will bring you to our web store)",
                     g_grpNamePtr,g_grpNamePtr);

            if (wm_ynbox("Important - Duke Nukem 3D not found - EDuke32",tempbuf))
            {
                SHELLEXECUTEINFOA sinfo;
                char *p = "http://www.gog.com/en/gamecard/duke_nukem_3d_atomic_edition/pp/6c1e671f9af5b46d9c1a52067bdf0e53685674f7";

                Bmemset(&sinfo, 0, sizeof(sinfo));
                sinfo.cbSize = sizeof(sinfo);
                sinfo.fMask = SEE_MASK_CLASSNAME;
                sinfo.lpVerb = "open";
                sinfo.lpFile = p;
                sinfo.nShow = SW_SHOWNORMAL;
                sinfo.lpClass = "http";

                if (!ShellExecuteExA(&sinfo))
                    G_GameExit("Error launching default system browser!");
            }
            G_GameExit("");
#else
            Bsprintf(tempbuf,"Duke Nukem 3D game data was not found.  A valid copy of '%s' or other compatible data is needed to run EDuke32.\n"
                     "You can find '%s' in the 'DN3DINST' or 'ATOMINST' directory on your Duke Nukem 3D installation CD-ROM.\n\n"
                     "EDuke32 will now close.",
                     g_grpNamePtr,g_grpNamePtr);
            G_GameExit(tempbuf);
#endif
        }
        else
        {
            Bsprintf(tempbuf,"CON file `%s' missing.", filenam);
            G_GameExit(tempbuf);
        }

        //g_loadFromGroupOnly = 1;
        return; //Not there
    }

    fs = kfilelength(fp);

    initprintf("Compiling: %s (%d bytes)\n",filenam,fs);

    flushlogwindow = 0;

    startcompiletime = getticks();

    mptr = (char *)Bmalloc(fs+1);
    if (!mptr)
    {
        Bsprintf(tempbuf,"Failed allocating %d byte CON text buffer.", fs+1);
        G_GameExit(tempbuf);
    }

    mptr[fs] = 0;

    textptr = (char *) mptr;
    kread(fp,(char *)textptr,fs);
    kclose(fp);

    clearbuf(actorscrptr,MAXTILES,0L);  // JBF 20040531: MAXSPRITES? I think Todd meant MAXTILES...
    clearbuf(actorLoadEventScrptr,MAXTILES,0L); // I think this should be here...
    clearbufbyte(ActorType,MAXTILES,0L);
//    clearbufbyte(script,sizeof(script),0l); // JBF 20040531: yes? no?
    if (script != NULL)
        Bfree(script);

    script = Bcalloc(1,g_scriptSize * sizeof(intptr_t));
    bitptr = Bcalloc(1,(((g_scriptSize+7)>>3)+1) * sizeof(uint8_t));
//    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    g_numLabels = g_numDefaultLabels = 0;
    g_scriptPtr = script+1;
    g_numCompilerWarnings = 0;
    g_numCompilerErrors = 0;
    g_lineNumber = 1;
    g_totalLines = 0;

    C_AddDefaultDefinitions();

    Bstrcpy(g_szScriptFileName, filenam);   // JBF 20031130: Store currently compiling file name

    while (C_ParseCommand() == 0);

    flushlogwindow = 1;

    if (g_numCompilerErrors > 63)
        initprintf("fatal error: too many errors: Aborted\n");

    //*script = (intptr_t) g_scriptPtr;

    Bfree(mptr);
    mptr = NULL;

    if (g_numCompilerWarnings|g_numCompilerErrors)
        initprintf("Found %d warning(s), %d error(s).\n",g_numCompilerWarnings,g_numCompilerErrors);

    if (g_numCompilerErrors)
    {
        if (g_loadFromGroupOnly)
        {
            Bsprintf(buf,"Error compiling CON files.");
            G_GameExit(buf);
        }
        else
        {
            if (g_groupFileHandle != -1 && g_loadFromGroupOnly == 0)
            {
//                initprintf("Error(s) found in file `%s'.  Do you want to use the INTERNAL DEFAULTS (y/N)?\n",filenam);

                i=wm_ynbox("CON File Compilation Error", "Error(s) found in file `%s'. Do you want to use the "
                           "INTERNAL DEFAULTS?",filenam);
                if (i) i = 'y';
                if (i == 'y' || i == 'Y')
                {
                    initprintf(" Yes\n");
                    g_loadFromGroupOnly = 1;
                    return;
                }
                else
                {
#if (defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2))
                    while (!quitevent) // keep the window open so people can copy CON errors out of it
                        handleevents();
#endif
                    G_GameExit("");
                }
            }
        }
    }
    else
    {
        int32_t j=0, k=0, l=0;

        hash_free(&h_keywords);
        freehashnames();

        hash_free(&sectorH);
        hash_free(&wallH);
        hash_free(&userdefH);

        hash_free(&projectileH);
        hash_free(&playerH);
        hash_free(&inputH);
        hash_free(&actorH);
        hash_free(&tspriteH);

        g_totalLines += g_lineNumber;

        C_SetScriptSize(g_scriptPtr-script+8);

        initprintf("Script compiled in %dms, %ld*%db, version %s\n", getticks() - startcompiletime,
                   (unsigned)(g_scriptPtr-script), sizeof(intptr_t), (g_scriptVersion == 14?"1.4+":"1.3D"));

        initprintf("%ld/%ld labels, %d/%d variables\n", g_numLabels,
                   min((MAXSECTORS * sizeof(sectortype)/sizeof(int32_t)),
                       MAXSPRITES * sizeof(spritetype)/(1<<6)),
                   g_gameVarCount, MAXGAMEVARS);

        for (i=MAXQUOTES-1; i>=0; i--)
            if (ScriptQuotes[i])
                j++;
        for (i=MAXGAMEEVENTS-1; i>=0; i--)
            if (apScriptGameEvent[i])
                k++;
        for (i=MAXTILES-1; i>=0; i--)
            if (actorscrptr[i])
                l++;

        if (j) initprintf("%ld quotes, ", j);
        if (g_numQuoteRedefinitions) initprintf("%d strings, ", g_numQuoteRedefinitions);
        if (k) initprintf("%ld events, ", k);
        if (l) initprintf("%ld actors", l);

        initprintf("\n");

        for (i=127; i>=0; i--)
            if (ScriptQuotes[i] == NULL)
                ScriptQuotes[i] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));

        for (i=MAXQUOTELEN-7; i>=0; i--)
            if (Bstrncmp(&ScriptQuotes[13][i],"SPACE",5) == 0)
            {
                Bmemset(tempbuf,0,sizeof(tempbuf));
                Bstrncpy(tempbuf,ScriptQuotes[13],i);
                Bstrcat(tempbuf,"OPEN");
                Bstrcat(tempbuf,&ScriptQuotes[13][i+5]);
                Bstrncpy(ScriptQuotes[13],tempbuf,MAXQUOTELEN-1);
                i = MAXQUOTELEN-7;
            }

        // most of these are based on Blood, obviously
        {
            const char *PlayerObituaries[] =
            {
                "^02%s^02 beat %s^02 like a cur",
                "^02%s^02 broke %s",
                "^02%s^02 body bagged %s",
                "^02%s^02 boned %s^02 like a fish",
                "^02%s^02 castrated %s",
                "^02%s^02 creamed %s",
                "^02%s^02 crushed %s",
                "^02%s^02 destroyed %s",
                "^02%s^02 diced %s",
                "^02%s^02 disemboweled %s",
                "^02%s^02 erased %s",
                "^02%s^02 eviscerated %s",
                "^02%s^02 flailed %s",
                "^02%s^02 flattened %s",
                "^02%s^02 gave AnAl MaDnEsS to %s",
                "^02%s^02 gave %s^02 Anal Justice",
                "^02%s^02 hosed %s",
                "^02%s^02 hurt %s^02 real bad",
                "^02%s^02 killed %s",
                "^02%s^02 made dog meat out of %s",
                "^02%s^02 made mincemeat out of %s",
                "^02%s^02 manhandled %s",
                "^02%s^02 massacred %s",
                "^02%s^02 mutilated %s",
                "^02%s^02 murdered %s",
                "^02%s^02 neutered %s",
                "^02%s^02 punted %s",
                "^02%s^02 reamed %s",
                "^02%s^02 ripped %s^02 a new orifice",
                "^02%s^02 rocked %s",
                "^02%s^02 sent %s^02 to hell",
                "^02%s^02 shredded %s",
                "^02%s^02 slashed %s",
                "^02%s^02 slaughtered %s",
                "^02%s^02 sliced %s",
                "^02%s^02 smacked %s around",
                "^02%s^02 smashed %s",
                "^02%s^02 snuffed %s",
                "^02%s^02 sodomized %s",
                "^02%s^02 splattered %s",
                "^02%s^02 sprayed %s",
                "^02%s^02 squashed %s",
                "^02%s^02 throttled %s",
                "^02%s^02 toasted %s",
                "^02%s^02 vented %s",
                "^02%s^02 ventilated %s",
                "^02%s^02 wasted %s",
                "^02%s^02 wrecked %s",
            };

            const char *PlayerSelfObituaries[] =
            {
                "^02%s^02 is excrement",
                "^02%s^02 is hamburger",
                "^02%s^02 suffered scrotum separation",
                "^02%s^02 volunteered for population control",
                "^02%s^02 has suicided",
                "^02%s^02 bled out",
            };

            g_numObituaries = (sizeof(PlayerObituaries)/sizeof(PlayerObituaries[0]));
            for (i=g_numObituaries-1; i>=0; i--)
            {
                if (ScriptQuotes[i+OBITQUOTEINDEX] == NULL)
                {
                    ScriptQuotes[i+OBITQUOTEINDEX] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
                    Bstrcpy(ScriptQuotes[i+OBITQUOTEINDEX],PlayerObituaries[i]);
                }
            }

            g_numSelfObituaries = (sizeof(PlayerSelfObituaries)/sizeof(PlayerSelfObituaries[0]));
            for (i=g_numSelfObituaries-1; i>=0; i--)
            {
                if (ScriptQuotes[i+SUICIDEQUOTEINDEX] == NULL)
                {
                    ScriptQuotes[i+SUICIDEQUOTEINDEX] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
                    Bstrcpy(ScriptQuotes[i+SUICIDEQUOTEINDEX],PlayerSelfObituaries[i]);
                }
            }
        }
    }
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (g_parsingEventPtr || g_processingState || g_parsingActorPtr)
            initprintf("%s: In %s `%s':\n",g_szScriptFileName,g_parsingEventPtr?"event":g_parsingActorPtr?"actor":"state",g_szCurrentBlockName);
        else initprintf("%s: At top level:\n",g_szScriptFileName);
        Bstrcpy(g_szLastBlockName,g_szCurrentBlockName);
    }
    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%d: error: found more `}' than `{' before `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_EVENTONLY:
        initprintf("%s:%d: error: `%s' only valid during events.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        initprintf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",g_szScriptFileName,g_lineNumber,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%d: error: expected a keyword but found `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%d: error: found `%s' within %s.\n",g_szScriptFileName,g_lineNumber,tempbuf,g_parsingEventPtr?"an event":g_parsingActorPtr?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%d: error: did not find `endswitch' before `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%d: error: symbol `%s' is not a game definition.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a game variable.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_NOTAGAMEARRAY:
        initprintf("%s:%d: error: symbol `%s' is not a game array.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_GAMEARRAYBNC:
        initprintf("%s:%d: error: square brackets for index of game array not closed, expected ] found %c\n",g_szScriptFileName,g_lineNumber,*textptr);
        break;
    case ERROR_GAMEARRAYBNO:
        initprintf("%s:%d: error: square brackets for index of game array not opened, expected [ found %c\n",g_szScriptFileName,g_lineNumber,*textptr);
        break;
    case ERROR_INVALIDARRAYWRITE:
        initprintf("%s:%d: error: arrays can only be written to using `setarray' %c\n",g_szScriptFileName,g_lineNumber,*textptr);
        break;
    case ERROR_OPENBRACKET:
        initprintf("%s:%d: error: found more `{' than `}' before `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_SYMBOLNOTRECOGNIZED:
        initprintf("%s:%d: error: symbol `%s' is not recognized.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",g_szScriptFileName,g_lineNumber);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case WARNING_BADGAMEVAR:
        initprintf("%s:%ld: warning: variable `%s' should be either per-player OR per-actor, not both.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case WARNING_DUPLICATECASE:
        initprintf("%s:%ld: warning: duplicate case ignored.\n",g_szScriptFileName,g_lineNumber);
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate game definition `%s' ignored.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case WARNING_EVENTSYNC:
        initprintf("%s:%d: warning: found `%s' within a local event.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",g_szScriptFileName,g_lineNumber);
        break;
    case WARNING_NAMEMATCHESVAR:
        initprintf("%s:%d: warning: symbol `%s' already used for game variable.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
        break;
    case WARNING_REVEVENTSYNC:
        initprintf("%s:%d: warning: found `%s' outside of a local event.\n",g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    }
}
