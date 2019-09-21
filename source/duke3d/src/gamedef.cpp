//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "gamedef.h"

#include "cheats.h"
#include "common.h"
#include "common_game.h"
#include "crc32.h"
#include "debugbreak.h"
#include "duke3d.h"
#include "gameexec.h"
#include "namesdyn.h"
#include "osd.h"
#include "savegame.h"

#include "vfs.h"

#define LINE_NUMBER (g_lineNumber << 12)

int32_t g_scriptVersion = 13; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files

char g_scriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling

int32_t g_totalLines;
int32_t g_lineNumber;
uint32_t g_scriptcrc;
char g_szBuf[1024];

#if !defined LUNATIC
static char *textptr;

static char g_szCurrentBlockName[64] = "(none)";
static char g_szLastBlockName[64] = "NULL";

static bool g_checkingCase;
static bool g_dynamicSoundMapping;
static bool g_dynamicTileMapping;
static bool g_labelsOnly;
static bool g_processingState;
static bool g_skipBranch;

static int g_checkingIfElse;
static int g_checkingSwitch;
static int g_lastKeyword = -1;
static int g_numBraces;
static int g_numCases;

static intptr_t apScriptGameEventEnd[MAXEVENTS];
static intptr_t g_scriptActorOffset;
static intptr_t g_scriptEventBreakOffset;
static intptr_t g_scriptEventChainOffset;
static intptr_t g_scriptEventOffset;

// The pointer to the start of the case table in a switch statement.
// First entry is 'default' code.
static intptr_t *g_caseTablePtr;

static bool C_ParseCommand(bool loop = false);
static void C_SetScriptSize(int32_t newsize);
#endif

int32_t g_errorCnt;
int32_t g_warningCnt;
int32_t g_numXStrings;

#ifdef LUNATIC
weapondata_t g_playerWeapon[MAXPLAYERS][MAX_WEAPONS];
#endif

#if !defined LUNATIC
static char *C_GetLabelType(int const type)
{
    static tokenmap_t const LabelType[] =
    {
        { "action", LABEL_ACTION },
        { "actor",  LABEL_ACTOR },
        { "ai",     LABEL_AI },
        { "define", LABEL_DEFINE },
        { "event",  LABEL_EVENT },
        { "move",   LABEL_MOVE },
        { "state",  LABEL_STATE },
    };

    char x[64] = {};

    for (auto &label : LabelType)
    {
        if ((type & label.val) != label.val)
            continue;

        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, label.token);

        if (type == label.val)
            break;
    }

    return Xstrdup(x);
}

static hashtable_t h_keywords   = { CON_END>>1, NULL };
static hashtable_t h_iter       = { ITER_END>>1, NULL };

static hashtable_t *const tables[] = {
    &h_arrays,
    &h_gamevars,
    &h_iter,
    &h_keywords,
    &h_labels,
};

static hashtable_t *const tables_free[] = {
    &h_iter,
    &h_keywords,
    &h_labels,
};

static tokenmap_t const vm_keywords[] =
{
    { "action",                 CON_ACTION },
    { "activate",               CON_ACTIVATE },
    { "activatebysector",       CON_ACTIVATEBYSECTOR },
    { "activatecheat",          CON_ACTIVATECHEAT },
    { "actor",                  CON_ACTOR },
    { "actorsound",             CON_ACTORSOUND },
    { "addammo",                CON_ADDAMMO },
    { "addinventory",           CON_ADDINVENTORY },
    { "addkills",               CON_ADDKILLS },
    { "addlog",                 CON_ADDLOGVAR },
    { "addlogvar",              CON_ADDLOGVAR },
    { "addphealth",             CON_ADDPHEALTH },
    { "addstrength",            CON_ADDSTRENGTH },
    { "addvar",                 CON_ADDVAR },
    { "addvarvar",              CON_ADDVARVAR },
    { "addweapon",              CON_ADDWEAPON },
    { "addweaponvar",           CON_ADDWEAPON },
    { "ai",                     CON_AI },
    { "andvar",                 CON_ANDVAR },
    { "andvarvar",              CON_ANDVARVAR },
    { "angoff",                 CON_ANGOFF },
    { "angoffvar",              CON_ANGOFF },
    { "appendevent",            CON_APPENDEVENT },
    { "betaname",               CON_BETANAME },
    { "break",                  CON_BREAK },
    { "cactor",                 CON_CACTOR },
    { "calchypotenuse",         CON_CALCHYPOTENUSE },
    { "cansee",                 CON_CANSEE },
    { "canseespr",              CON_CANSEESPR },
    { "capia",                  CON_CAPIA },
    { "capis",                  CON_CAPIS },
    { "case",                   CON_CASE },
    { "changespritesect",       CON_CHANGESPRITESECT },
    { "changespritestat",       CON_CHANGESPRITESTAT },
    { "cheatkeys",              CON_CHEATKEYS },
    { "checkactivatormotion",   CON_CHECKACTIVATORMOTION },
    { "checkavailinven",        CON_CHECKAVAILINVEN },
    { "checkavailweapon",       CON_CHECKAVAILWEAPON },
    { "clamp",                  CON_CLAMP },
    { "clearmapstate",          CON_CLEARMAPSTATE },
    { "clipdist",               CON_CLIPDIST },
    { "clipmove",               CON_CLIPMOVE },
    { "clipmovenoslide",        CON_CLIPMOVENOSLIDE },
    { "cmenu",                  CON_CMENU },
    { "copy",                   CON_COPY },
    { "cos",                    CON_COS },
    { "count",                  CON_COUNT },
    { "cstat",                  CON_CSTAT },
    { "cstator",                CON_CSTATOR },
    { "damageeventtile",        CON_DAMAGEEVENTTILE },
    { "damageeventtilerange",   CON_DAMAGEEVENTTILERANGE },
    { "debris",                 CON_DEBRIS },
    { "debug",                  CON_DEBUG },
    { "default",                CON_DEFAULT },
    { "define",                 CON_DEFINE },
    { "definecheat",            CON_DEFINECHEAT },
    { "definecheatdescription", CON_DEFINECHEATDESCRIPTION },
    { "definegamefuncname",     CON_DEFINEGAMEFUNCNAME },
    { "definegametype",         CON_DEFINEGAMETYPE },
    { "definelevelname",        CON_DEFINELEVELNAME },
    { "defineprojectile",       CON_DEFINEPROJECTILE },
    { "definequote",            CON_DEFINEQUOTE },
    { "defineskillname",        CON_DEFINESKILLNAME },
    { "definesound",            CON_DEFINESOUND },
    { "definevolumeflags",      CON_DEFINEVOLUMEFLAGS },
    { "definevolumename",       CON_DEFINEVOLUMENAME },
    { "defstate",               CON_DEFSTATE },
    { "digitalnumber",          CON_DIGITALNUMBER },
    { "digitalnumberz",         CON_DIGITALNUMBERZ },
    { "displayrand",            CON_DISPLAYRAND },
    { "displayrandvar",         CON_DISPLAYRANDVAR },
    { "displayrandvarvar",      CON_DISPLAYRANDVARVAR },
    { "dist",                   CON_DIST },
    { "divr",                   CON_DIVR },
    { "divrd",                  CON_DIVVARVAR }, // div round toward zero -- alias until proven otherwise
    { "divru",                  CON_DIVRU },
    { "divscale",               CON_DIVSCALE },
    { "divvar",                 CON_DIVVAR },
    { "divvarvar",              CON_DIVVARVAR },
    { "dragpoint",              CON_DRAGPOINT },
    { "drawline256",            CON_DRAWLINE256 },
    { "drawlinergb",            CON_DRAWLINERGB },
    { "dynamicremap",           CON_DYNAMICREMAP },
    { "dynamicsoundremap",      CON_DYNAMICSOUNDREMAP },
    { "echo",                   CON_ECHO },
    { "else",                   CON_ELSE },
    { "enda",                   CON_ENDA },
    { "endevent",               CON_ENDEVENT },
    { "endofgame",              CON_ENDOFGAME },
    { "endoflevel",             CON_ENDOFLEVEL },
    { "ends",                   CON_ENDS },
    { "endswitch",              CON_ENDSWITCH },
    { "enhanced",               CON_ENHANCED },
    { "eqspawnvar",             CON_EQSPAWN },
    { "eshootvar",              CON_ESHOOT },
    { "espawnvar",              CON_ESPAWN },
    { "eventloadactor",         CON_EVENTLOADACTOR },
    { "ezshootvar",             CON_EZSHOOT },
    { "fall",                   CON_FALL },
    { "findnearactor3dvar",     CON_FINDNEARACTOR3D },
    { "findnearactorvar",       CON_FINDNEARACTOR },
    { "findnearactorzvar",      CON_FINDNEARACTORZ },
    { "findnearsprite3dvar",    CON_FINDNEARSPRITE3D },
    { "findnearspritevar",      CON_FINDNEARSPRITE },
    { "findnearspritezvar",     CON_FINDNEARSPRITEZ },
    { "findotherplayer",        CON_FINDOTHERPLAYER },
    { "findplayer",             CON_FINDPLAYER },
    { "flash",                  CON_FLASH },
    { "for",                    CON_FOR },
    { "gamearray",              CON_GAMEARRAY },
    { "gamestartup",            CON_GAMESTARTUP },
    { "gametext",               CON_GAMETEXT },
    { "gametextz",              CON_GAMETEXTZ },
    { "gamevar",                CON_GAMEVAR },
    { "getactor",               CON_GETACTOR },
    { "getactorangle",          CON_GETACTORANGLE },
    { "getactorvar",            CON_GETACTORVAR },
    { "getangle",               CON_GETANGLE },
    { "getangletotarget",       CON_GETANGLETOTARGET },
    { "getarraysize",           CON_GETARRAYSIZE },
    { "getceilzofslope",        CON_GETCEILZOFSLOPE },
    { "getclosestcol",          CON_GETCLOSESTCOL },
    { "getcurraddress",         CON_GETCURRADDRESS },
    { "getflorzofslope",        CON_GETFLORZOFSLOPE },
    { "getgamefuncbind",        CON_GETGAMEFUNCBIND },
    { "getincangle",            CON_GETINCANGLE },
    { "getinput",               CON_GETINPUT },
    { "getkeyname",             CON_GETKEYNAME },
    { "getlastpal",             CON_GETLASTPAL },
    { "getmusicposition",       CON_GETMUSICPOSITION },
    { "getplayer",              CON_GETPLAYER },
    { "getplayerangle",         CON_GETPLAYERANGLE },
    { "getplayervar",           CON_GETPLAYERVAR },
    { "getpname",               CON_GETPNAME },
    { "getprojectile",          CON_GETPROJECTILE },
    { "getsector",              CON_GETSECTOR },
    { "gettextureceiling",      CON_GETTEXTURECEILING },
    { "gettexturefloor",        CON_GETTEXTUREFLOOR },
    { "getthisprojectile",      CON_GETTHISPROJECTILE },
    { "getticks",               CON_GETTICKS },
    { "gettiledata",            CON_GETTILEDATA }, // OldMP compat.
    { "gettimedate",            CON_GETTIMEDATE },
    { "gettspr",                CON_GETTSPR },
    { "getuserdef",             CON_GETUSERDEF },
    { "getwall",                CON_GETWALL },
    { "getzrange",              CON_GETZRANGE },
    { "globalsound",            CON_GLOBALSOUND },
    { "globalsoundvar",         CON_GLOBALSOUND },
    { "gmaxammo",               CON_GMAXAMMO },
    { "guniqhudid",             CON_GUNIQHUDID },
    { "guts",                   CON_GUTS },
    { "headspritesect",         CON_HEADSPRITESECT },
    { "headspritestat",         CON_HEADSPRITESTAT },
    { "hitradius",              CON_HITRADIUS },
    { "hitradiusvar",           CON_HITRADIUS },
    { "hitscan",                CON_HITSCAN },
    { "ifaction",               CON_IFACTION },
    { "ifactioncount",          CON_IFACTIONCOUNT },
    { "ifactor",                CON_IFACTOR },
    { "ifactornotstayput",      CON_IFACTORNOTSTAYPUT },
    { "ifactorsound",           CON_IFACTORSOUND },
    { "ifai",                   CON_IFAI },
    { "ifangdiffl",             CON_IFANGDIFFL },
    { "ifawayfromwall",         CON_IFAWAYFROMWALL },
    { "ifbulletnear",           CON_IFBULLETNEAR },
    { "ifcansee",               CON_IFCANSEE },
    { "ifcanseetarget",         CON_IFCANSEETARGET },
    { "ifcanshoottarget",       CON_IFCANSHOOTTARGET },
    { "ifceilingdistl",         CON_IFCEILINGDISTL },
    { "ifclient",               CON_IFCLIENT },
    { "ifcount",                CON_IFCOUNT },
    { "ifcutscene",             CON_IFCUTSCENE },
    { "ifdead",                 CON_IFDEAD },
    { "iffloordistl",           CON_IFFLOORDISTL },
    { "ifgapzl",                CON_IFGAPZL },
    { "ifgotweaponce",          CON_IFGOTWEAPONCE },
    { "ifhitspace",             CON_IFHITSPACE },
    { "ifhitweapon",            CON_IFHITWEAPON },
    { "ifinouterspace",         CON_IFINOUTERSPACE },
    { "ifinspace",              CON_IFINSPACE },
    { "ifinwater",              CON_IFINWATER },
    { "ifmove",                 CON_IFMOVE },
    { "ifmultiplayer",          CON_IFMULTIPLAYER },
    { "ifnosounds",             CON_IFNOSOUNDS },
    { "ifnotmoving",            CON_IFNOTMOVING },
    { "ifonwater",              CON_IFONWATER },
    { "ifoutside",              CON_IFOUTSIDE },
    { "ifp",                    CON_IFP },
    { "ifpdistg",               CON_IFPDISTG },
    { "ifpdistl",               CON_IFPDISTL },
    { "ifphealthl",             CON_IFPHEALTHL },
    { "ifpinventory",           CON_IFPINVENTORY },
    { "ifplaybackon",           CON_IFPLAYBACKON },
    { "ifplayersl",             CON_IFPLAYERSL },
    { "ifrespawn",              CON_IFRESPAWN },
    { "ifrnd",                  CON_IFRND },
    { "ifserver",               CON_IFSERVER },
    { "ifsound",                CON_IFSOUND },
    { "ifspawnedby",            CON_IFSPAWNEDBY },
    { "ifspritepal",            CON_IFSPRITEPAL },
    { "ifsquished",             CON_IFSQUISHED },
    { "ifstrength",             CON_IFSTRENGTH },
    { "ifvara",                 CON_IFVARA },
    { "ifvarae",                CON_IFVARAE },
    { "ifvarand",               CON_IFVARAND },
    { "ifvarb",                 CON_IFVARB },
    { "ifvarbe",                CON_IFVARBE },
    { "ifvarboth",              CON_IFVARBOTH },
    { "ifvare",                 CON_IFVARE },
    { "ifvareither",            CON_IFVAREITHER },
    { "ifvarg",                 CON_IFVARG },
    { "ifvarge",                CON_IFVARGE },
    { "ifvarl",                 CON_IFVARL },
    { "ifvarle",                CON_IFVARLE },
    { "ifvarn",                 CON_IFVARN },
    { "ifvaror",                CON_IFVAROR },
    { "ifvarvara",              CON_IFVARVARA },
    { "ifvarvarae",             CON_IFVARVARAE },
    { "ifvarvarand",            CON_IFVARVARAND },
    { "ifvarvarb",              CON_IFVARVARB },
    { "ifvarvarbe",             CON_IFVARVARBE },
    { "ifvarvarboth",           CON_IFVARVARBOTH },
    { "ifvarvare",              CON_IFVARVARE },
    { "ifvarvareither",         CON_IFVARVAREITHER },
    { "ifvarvarg",              CON_IFVARVARG },
    { "ifvarvarge",             CON_IFVARVARGE },
    { "ifvarvarl",              CON_IFVARVARL },
    { "ifvarvarle",             CON_IFVARVARLE },
    { "ifvarvarn",              CON_IFVARVARN },
    { "ifvarvaror",             CON_IFVARVAROR },
    { "ifvarvarxor",            CON_IFVARVARXOR },
    { "ifvarxor",               CON_IFVARXOR },
    { "ifwasweapon",            CON_IFWASWEAPON },
    { "include",                CON_INCLUDE },
    { "includedefault",         CON_INCLUDEDEFAULT },
    { "inittimer",              CON_INITTIMER },
    { "insertspriteq",          CON_INSERTSPRITEQ },
    { "inv",                    CON_INV },
    { "jump",                   CON_JUMP },
    { "killit",                 CON_KILLIT },
    { "klabs",                  CON_KLABS },
    { "ldist",                  CON_LDIST },
    { "lineintersect",          CON_LINEINTERSECT },
    { "loadmapstate",           CON_LOADMAPSTATE },
    { "lockplayer",             CON_LOCKPLAYER },
    { "lotsofglass",            CON_LOTSOFGLASS },
    { "mail",                   CON_MAIL },
    { "mikesnd",                CON_MIKESND },
    { "minitext",               CON_MINITEXT },
    { "modvar",                 CON_MODVAR },
    { "modvarvar",              CON_MODVARVAR },
    { "money",                  CON_MONEY },
    { "move",                   CON_MOVE },
    { "movesector",             CON_MOVESECTOR },
    { "movesprite",             CON_MOVESPRITE },
    { "mulscale",               CON_MULSCALE },
    { "mulvar",                 CON_MULVAR },
    { "mulvarvar",              CON_MULVARVAR },
    { "music",                  CON_MUSIC },
    { "myos",                   CON_MYOS },
    { "myospal",                CON_MYOSPAL },
    { "myospalx",               CON_MYOSPALX },
    { "myosx",                  CON_MYOSX },
    { "neartag",                CON_NEARTAG },
    { "nextsectorneighborz",    CON_NEXTSECTORNEIGHBORZ },
    { "nextspritesect",         CON_NEXTSPRITESECT },
    { "nextspritestat",         CON_NEXTSPRITESTAT },
    { "nullop",                 CON_NULLOP },
    { "onevent",                CON_ONEVENT },
    { "operate",                CON_OPERATE },
    { "operateactivators",      CON_OPERATEACTIVATORS },
    { "operatemasterswitches",  CON_OPERATEMASTERSWITCHES },
    { "operaterespawns",        CON_OPERATERESPAWNS },
    { "operatesectors",         CON_OPERATESECTORS },
    { "orvar",                  CON_ORVAR },
    { "orvarvar",               CON_ORVARVAR },
    { "palfrom",                CON_PALFROM },
    { "paper",                  CON_PAPER },
    { "pkick",                  CON_PKICK },
    { "precache",               CON_PRECACHE },
    { "prevspritesect",         CON_PREVSPRITESECT },
    { "prevspritestat",         CON_PREVSPRITESTAT },
    { "preloadtrackslotforswap", CON_PRELOADTRACKSLOTFORSWAP },
    { "pstomp",                 CON_PSTOMP },
    { "qgetsysstr",             CON_QGETSYSSTR },
    { "qspawnvar",              CON_QSPAWN },
    { "qsprintf",               CON_QSPRINTF },
    { "qstrcat",                CON_QSTRCAT },
    { "qstrcmp",                CON_QSTRCMP },
    { "qstrcpy",                CON_QSTRCPY },
    { "qstrdim",                CON_QSTRDIM },
    { "qstrlen",                CON_QSTRLEN },
    { "qstrncat",               CON_QSTRNCAT },
    { "qsubstr",                CON_QSUBSTR },
    { "quake",                  CON_QUAKE },
    { "quote",                  CON_QUOTE },
    { "randvar",                CON_RANDVAR },
    { "randvarvar",             CON_RANDVARVAR },
    { "rayintersect",           CON_RAYINTERSECT },
    { "readarrayfromfile",      CON_READARRAYFROMFILE },
    { "readgamevar",            CON_READGAMEVAR },
    { "redefinequote",          CON_REDEFINEQUOTE },
    { "resetactioncount",       CON_RESETACTIONCOUNT },
    { "resetcount",             CON_RESETCOUNT },
    { "resetplayer",            CON_RESETPLAYER },
    { "resetplayerflags",       CON_RESETPLAYERFLAGS },
    { "resizearray",            CON_RESIZEARRAY },
    { "respawnhitag",           CON_RESPAWNHITAG },
    { "return",                 CON_RETURN },
    { "rotatepoint",            CON_ROTATEPOINT },
    { "rotatesprite",           CON_ROTATESPRITE },
    { "rotatesprite16",         CON_ROTATESPRITE16 },
    { "rotatespritea",          CON_ROTATESPRITEA },
    { "save",                   CON_SAVE },
    { "savegamevar",            CON_SAVEGAMEVAR },
    { "savemapstate",           CON_SAVEMAPSTATE },
    { "savenn",                 CON_SAVENN },
    { "scalevar",               CON_SCALEVAR },
    { "screenpal",              CON_SCREENPAL },
    { "screensound",            CON_SCREENSOUND },
    { "screentext",             CON_SCREENTEXT },
    { "scriptsize",             CON_SCRIPTSIZE },
    { "sectclearinterpolation", CON_SECTCLEARINTERPOLATION },
    { "sectgethitag",           CON_SECTGETHITAG },
    { "sectgetlotag",           CON_SECTGETLOTAG },
    { "sectorofwall",           CON_SECTOROFWALL },
    { "sectsetinterpolation",   CON_SECTSETINTERPOLATION },
    { "setactor",               CON_SETACTOR },
    { "setactorangle",          CON_SETACTORANGLE },
    { "setactorsoundpitch",     CON_SETACTORSOUNDPITCH },
    { "setactorvar",            CON_SETACTORVAR },
    { "setarray",               CON_SETARRAY },
    { "setaspect",              CON_SETASPECT },
    { "setcfgname",             CON_SETCFGNAME },
    { "setdefname",             CON_SETDEFNAME },
    { "setgamename",            CON_SETGAMENAME },
    { "setgamepalette",         CON_SETGAMEPALETTE },
    { "setinput",               CON_SETINPUT },
    { "setmusicposition",       CON_SETMUSICPOSITION },
    { "setplayer",              CON_SETPLAYER },
    { "setplayerangle",         CON_SETPLAYERANGLE },
    { "setplayervar",           CON_SETPLAYERVAR },
    { "setprojectile",          CON_SETPROJECTILE },
    { "setsector",              CON_SETSECTOR },
    { "setsprite",              CON_SETSPRITE },
    { "setthisprojectile",      CON_SETTHISPROJECTILE },
    { "settiledata",            CON_SETTILEDATA },
    { "settspr",                CON_SETTSPR },
    { "setuserdef",             CON_SETUSERDEF },
    { "setvar",                 CON_SETVAR },
    { "setvarvar",              CON_SETVARVAR },
    { "setwall",                CON_SETWALL },
    { "shadeto",                CON_SHADETO },
    { "shiftvarl",              CON_SHIFTVARL },
    { "shiftvarr",              CON_SHIFTVARR },
    { "shiftvarvarl",           CON_SHIFTVARVARL },
    { "shiftvarvarr",           CON_SHIFTVARVARR },
    { "shootvar",               CON_SHOOT },
    { "showview",               CON_SHOWVIEW },
    { "showviewunbiased",       CON_SHOWVIEWUNBIASED },
    { "showviewq16",            CON_SHOWVIEWQ16 },
    { "showviewq16unbiased",    CON_SHOWVIEWQ16UNBIASED },
    { "sin",                    CON_SIN },
    { "sizeat",                 CON_SIZEAT },
    { "sizeto",                 CON_SIZETO },
    { "sleeptime",              CON_SLEEPTIME },
    { "smaxammo",               CON_SMAXAMMO },
    { "sound",                  CON_SOUND },
    { "soundonce",              CON_SOUNDONCE },
    { "soundoncevar",           CON_SOUNDONCE },
    { "soundvar",               CON_SOUND },
    { "spawn",                  CON_SPAWN },
    { "spawnceilingglass",      CON_SPAWNCEILINGGLASS },
    { "spawnwallstainedglass",  CON_SPAWNWALLSTAINEDGLASS },
    { "spawnwallglass",         CON_SPAWNWALLGLASS },
    { "spgethitag",             CON_SPGETHITAG },
    { "spgetlotag",             CON_SPGETLOTAG },
    { "spriteflags",            CON_SPRITEFLAGS },
    { "spritenopal",            CON_SPRITENOPAL },
    { "spritenoshade",          CON_SPRITENOSHADE },
    { "spritenvg",              CON_SPRITENVG },
    { "spritepal",              CON_SPRITEPAL },
    { "spriteshadow",           CON_SPRITESHADOW },
    { "sqrt",                   CON_SQRT },
    { "ssp",                    CON_SSP },
    { "startcutscene",          CON_STARTCUTSCENE },
    { "startlevel",             CON_STARTLEVEL },
    { "startscreen",            CON_STARTSCREEN },
    { "starttrack",             CON_STARTTRACK },
    { "starttrackslot",         CON_STARTTRACKSLOT },
    { "starttrackvar",          CON_STARTTRACK },
    { "state",                  CON_STATE },
    { "stopactorsound",         CON_STOPACTORSOUND },
    { "stopallmusic",           CON_STOPALLMUSIC },
    { "stopallsounds",          CON_STOPALLSOUNDS },
    { "stopsound",              CON_STOPSOUND },
    { "stopsoundvar",           CON_STOPSOUND },
    { "strength",               CON_STRENGTH },
    { "subvar",                 CON_SUBVAR },
    { "subvarvar",              CON_SUBVARVAR },
    { "switch",                 CON_SWITCH },
    { "swaparrays",             CON_SWAPARRAYS },
    { "swaptrackslot",          CON_SWAPTRACKSLOT },
    { "time",                   CON_TIME },
    { "tip",                    CON_TIP },
    { "tossweapon",             CON_TOSSWEAPON },
    { "undefinecheat",          CON_UNDEFINECHEAT },
    { "undefinegamefunc",       CON_UNDEFINEGAMEFUNC },
    { "undefinelevel",          CON_UNDEFINELEVEL },
    { "undefineskill",          CON_UNDEFINESKILL },
    { "undefinevolume",         CON_UNDEFINEVOLUME },
    { "updatesector",           CON_UPDATESECTOR },
    { "updatesectorz",          CON_UPDATESECTORZ },
    { "updatesectorneighbor",  CON_UPDATESECTORNEIGHBOR },
    { "updatesectorneighborz", CON_UPDATESECTORNEIGHBORZ },
    { "useractor",              CON_USERACTOR },
    { "userquote",              CON_USERQUOTE },
    { "wackplayer",             CON_WACKPLAYER },
    { "whilevarl",              CON_WHILEVARL },
    { "whilevarn",              CON_WHILEVARN },
    { "whilevarvarl",           CON_WHILEVARVARL },
    { "whilevarvarn",           CON_WHILEVARVARN },
    { "writearraytofile",       CON_WRITEARRAYTOFILE },
    { "xorvar",                 CON_XORVAR },
    { "xorvarvar",              CON_XORVARVAR },
    { "zshootvar",              CON_ZSHOOT },
    { "{",                      CON_LEFTBRACE },
    { "}",                      CON_RIGHTBRACE },

    { "#define",                CON_DEFINE },
    { "#include",               CON_INCLUDE },
    { "al",                     CON_ADDLOGVAR },
    { "var",                    CON_GAMEVAR },
    { "array",                  CON_GAMEARRAY },
    { "shiftl",                 CON_SHIFTVARVARL },
    { "shiftr",                 CON_SHIFTVARVARR },
    { "rand",                   CON_RANDVARVAR },
    { "set",                    CON_SETVARVAR },
    { "add",                    CON_ADDVARVAR },
    { "sub",                    CON_SUBVARVAR },
    { "mul",                    CON_MULVARVAR },
    { "div",                    CON_DIVVARVAR },
    { "mod",                    CON_MODVARVAR },
    { "and",                    CON_ANDVARVAR },
    { "or",                     CON_ORVARVAR },
    { "xor",                    CON_XORVARVAR },
    { "ifa",                    CON_IFVARVARA },
    { "ifae",                   CON_IFVARVARAE },
    { "ifb",                    CON_IFVARVARB },
    { "ifbe",                   CON_IFVARVARBE },
    { "ifl",                    CON_IFVARVARL },
    { "ifle",                   CON_IFVARVARLE },
    { "ifg",                    CON_IFVARVARG },
    { "ifge",                   CON_IFVARVARGE },
    { "ife",                    CON_IFVARVARE },
    { "ifn",                    CON_IFVARVARN },
    { "ifand",                  CON_IFVARVARAND },
    { "ifor",                   CON_IFVARVAROR },
    { "ifxor",                  CON_IFVARVARXOR },
    { "ifeither",               CON_IFVARVAREITHER },
    { "ifboth",                 CON_IFVARVARBOTH },
    { "whilen",                 CON_WHILEVARVARN },
    { "whilel",                 CON_WHILEVARVARL },
    { "abs",                    CON_KLABS },

    { "getp",                   CON_GETPLAYER },
    { "getpv",                  CON_GETPLAYERVAR },
    { "gets",                   CON_GETSECTOR },
    { "geta",                   CON_GETACTOR },
    { "getav",                  CON_GETACTORVAR },
    { "getw",                   CON_GETWALL },
    { "getu",                   CON_GETUSERDEF },
    { "geti",                   CON_GETINPUT },

    { "setp",                   CON_SETPLAYER },
    { "setpv",                  CON_SETPLAYERVAR },
    { "sets",                   CON_SETSECTOR },
    { "seta",                   CON_SETACTOR },
    { "setav",                  CON_SETACTORVAR },
    { "setw",                   CON_SETWALL },
    { "setu",                   CON_SETUSERDEF },
    { "seti",                   CON_SETINPUT },

    { "string",                 CON_DEFINEQUOTE },
    { "print",                  CON_QUOTE },

    { "dc",                     CON_DEFINECHEAT },
    { "dcd",                    CON_DEFINECHEATDESCRIPTION },
    { "udc",                    CON_UNDEFINECHEAT },
    { "ck",                     CON_CHEATKEYS },

    { "qputs",                  CON_REDEFINEQUOTE },

    { "espawn",                 CON_ESPAWN },
    { "qspawn",                 CON_QSPAWN },
    { "eqspawn",                CON_EQSPAWN },

    { "eshoot",                 CON_ESHOOT },
    { "zshoot",                 CON_ZSHOOT },
    { "ezshoot",                CON_EZSHOOT },
    { "shoot",                  CON_SHOOT },

    { "findnearactor",          CON_FINDNEARACTOR },
    { "findnearactor3d",        CON_FINDNEARACTOR3D },
    { "findnearactorz",         CON_FINDNEARACTORZ },

    { "findnearsprite",         CON_FINDNEARSPRITE },
    { "findnearsprite3d",       CON_FINDNEARSPRITE3D },
    { "findnearspritez",        CON_FINDNEARSPRITEZ },
};

static const vec2_t varvartable[] =
{
    { CON_IFVARVARA,         CON_IFVARA },
    { CON_IFVARVARAE,        CON_IFVARAE },
    { CON_IFVARVARAND,       CON_IFVARAND },
    { CON_IFVARVARB,         CON_IFVARB },
    { CON_IFVARVARBE,        CON_IFVARBE },
    { CON_IFVARVARBOTH,      CON_IFVARBOTH },
    { CON_IFVARVARE,         CON_IFVARE },
    { CON_IFVARVAREITHER,    CON_IFVAREITHER },
    { CON_IFVARVARG,         CON_IFVARG },
    { CON_IFVARVARGE,        CON_IFVARGE },
    { CON_IFVARVARL,         CON_IFVARL },
    { CON_IFVARVARLE,        CON_IFVARLE },
    { CON_IFVARVARN,         CON_IFVARN },
    { CON_IFVARVAROR,        CON_IFVAROR },
    { CON_IFVARVARXOR,       CON_IFVARXOR },

    { CON_ADDVARVAR,         CON_ADDVAR },
    { CON_ANDVARVAR,         CON_ANDVAR },
    { CON_DISPLAYRANDVARVAR, CON_DISPLAYRANDVAR },
    { CON_DIVVARVAR,         CON_DIVVAR },
    { CON_MODVARVAR,         CON_MODVAR },
    { CON_MULVARVAR,         CON_MULVAR },
    { CON_ORVARVAR,          CON_ORVAR },
    { CON_RANDVARVAR,        CON_RANDVAR },
    { CON_SETVARVAR,         CON_SETVAR },
    { CON_SHIFTVARVARL,      CON_SHIFTVARL },
    { CON_SHIFTVARVARR,      CON_SHIFTVARR },
    { CON_SUBVARVAR,         CON_SUBVAR },
    { CON_WHILEVARVARL,      CON_WHILEVARL },
    { CON_WHILEVARVARN,      CON_WHILEVARN },
    { CON_XORVARVAR,         CON_XORVAR },
};

#ifdef CON_DISCRETE_VAR_ACCESS
static const vec2_t globalvartable[] =
{
    { CON_IFVARA,         CON_IFVARA_GLOBAL },
    { CON_IFVARAE,        CON_IFVARAE_GLOBAL },
    { CON_IFVARAND,       CON_IFVARAND_GLOBAL },
    { CON_IFVARB,         CON_IFVARB_GLOBAL },
    { CON_IFVARBE,        CON_IFVARBE_GLOBAL },
    { CON_IFVARBOTH,      CON_IFVARBOTH_GLOBAL },
    { CON_IFVARE,         CON_IFVARE_GLOBAL },
    { CON_IFVAREITHER,    CON_IFVAREITHER_GLOBAL },
    { CON_IFVARG,         CON_IFVARG_GLOBAL },
    { CON_IFVARGE,        CON_IFVARGE_GLOBAL },
    { CON_IFVARL,         CON_IFVARL_GLOBAL },
    { CON_IFVARLE,        CON_IFVARLE_GLOBAL },
    { CON_IFVARN,         CON_IFVARN_GLOBAL },
    { CON_IFVAROR,        CON_IFVAROR_GLOBAL },
    { CON_IFVARXOR,       CON_IFVARXOR_GLOBAL },
    { CON_WHILEVARL,      CON_WHILEVARL_GLOBAL },
    { CON_WHILEVARN,      CON_WHILEVARN_GLOBAL },

    { CON_ADDVAR,         CON_ADDVAR_GLOBAL },
    { CON_ANDVAR,         CON_ANDVAR_GLOBAL },
    { CON_DIVVAR,         CON_DIVVAR_GLOBAL },
    { CON_MODVAR,         CON_MODVAR_GLOBAL },
    { CON_MULVAR,         CON_MULVAR_GLOBAL },
    { CON_ORVAR,          CON_ORVAR_GLOBAL },
    { CON_RANDVAR,        CON_RANDVAR_GLOBAL },
    { CON_SETVAR,         CON_SETVAR_GLOBAL },
    { CON_SHIFTVARL,      CON_SHIFTVARL_GLOBAL },
    { CON_SHIFTVARR,      CON_SHIFTVARR_GLOBAL },
    { CON_SUBVAR,         CON_SUBVAR_GLOBAL },
    { CON_XORVAR,         CON_XORVAR_GLOBAL },
};

static const vec2_t playervartable[] =
{
    { CON_IFVARA,         CON_IFVARA_PLAYER },
    { CON_IFVARAE,        CON_IFVARAE_PLAYER },
    { CON_IFVARAND,       CON_IFVARAND_PLAYER },
    { CON_IFVARB,         CON_IFVARB_PLAYER },
    { CON_IFVARBE,        CON_IFVARBE_PLAYER },
    { CON_IFVARBOTH,      CON_IFVARBOTH_PLAYER },
    { CON_IFVARE,         CON_IFVARE_PLAYER },
    { CON_IFVAREITHER,    CON_IFVAREITHER_PLAYER },
    { CON_IFVARG,         CON_IFVARG_PLAYER },
    { CON_IFVARGE,        CON_IFVARGE_PLAYER },
    { CON_IFVARL,         CON_IFVARL_PLAYER },
    { CON_IFVARLE,        CON_IFVARLE_PLAYER },
    { CON_IFVARN,         CON_IFVARN_PLAYER },
    { CON_IFVAROR,        CON_IFVAROR_PLAYER },
    { CON_IFVARXOR,       CON_IFVARXOR_PLAYER },
    { CON_WHILEVARL,      CON_WHILEVARL_PLAYER },
    { CON_WHILEVARN,      CON_WHILEVARN_PLAYER },

    { CON_ADDVAR,         CON_ADDVAR_PLAYER },
    { CON_ANDVAR,         CON_ANDVAR_PLAYER },
    { CON_DIVVAR,         CON_DIVVAR_PLAYER },
    { CON_MODVAR,         CON_MODVAR_PLAYER },
    { CON_MULVAR,         CON_MULVAR_PLAYER },
    { CON_ORVAR,          CON_ORVAR_PLAYER },
    { CON_RANDVAR,        CON_RANDVAR_PLAYER },
    { CON_SETVAR,         CON_SETVAR_PLAYER },
    { CON_SHIFTVARL,      CON_SHIFTVARL_PLAYER },
    { CON_SHIFTVARR,      CON_SHIFTVARR_PLAYER },
    { CON_SUBVAR,         CON_SUBVAR_PLAYER },
    { CON_XORVAR,         CON_XORVAR_PLAYER },
};

static const vec2_t actorvartable[] =
{
    { CON_IFVARA,         CON_IFVARA_ACTOR },
    { CON_IFVARAE,        CON_IFVARAE_ACTOR },
    { CON_IFVARAND,       CON_IFVARAND_ACTOR },
    { CON_IFVARB,         CON_IFVARB_ACTOR },
    { CON_IFVARBE,        CON_IFVARBE_ACTOR },
    { CON_IFVARBOTH,      CON_IFVARBOTH_ACTOR },
    { CON_IFVARE,         CON_IFVARE_ACTOR },
    { CON_IFVAREITHER,    CON_IFVAREITHER_ACTOR },
    { CON_IFVARG,         CON_IFVARG_ACTOR },
    { CON_IFVARGE,        CON_IFVARGE_ACTOR },
    { CON_IFVARL,         CON_IFVARL_ACTOR },
    { CON_IFVARLE,        CON_IFVARLE_ACTOR },
    { CON_IFVARN,         CON_IFVARN_ACTOR },
    { CON_IFVAROR,        CON_IFVAROR_ACTOR },
    { CON_IFVARXOR,       CON_IFVARXOR_ACTOR },
    { CON_WHILEVARL,      CON_WHILEVARL_ACTOR },
    { CON_WHILEVARN,      CON_WHILEVARN_ACTOR },

    { CON_ADDVAR,         CON_ADDVAR_ACTOR },
    { CON_ANDVAR,         CON_ANDVAR_ACTOR },
    { CON_DIVVAR,         CON_DIVVAR_ACTOR },
    { CON_MODVAR,         CON_MODVAR_ACTOR },
    { CON_MULVAR,         CON_MULVAR_ACTOR },
    { CON_ORVAR,          CON_ORVAR_ACTOR },
    { CON_RANDVAR,        CON_RANDVAR_ACTOR },
    { CON_SETVAR,         CON_SETVAR_ACTOR },
    { CON_SHIFTVARL,      CON_SHIFTVARL_ACTOR },
    { CON_SHIFTVARR,      CON_SHIFTVARR_ACTOR },
    { CON_SUBVAR,         CON_SUBVAR_ACTOR },
    { CON_XORVAR,         CON_XORVAR_ACTOR },
};
#endif

static inthashtable_t h_varvar = { NULL, INTHASH_SIZE(ARRAY_SIZE(varvartable)) };
#ifdef CON_DISCRETE_VAR_ACCESS
static inthashtable_t h_globalvar = { NULL, INTHASH_SIZE(ARRAY_SIZE(globalvartable)) };
static inthashtable_t h_playervar = { NULL, INTHASH_SIZE(ARRAY_SIZE(playervartable)) };
static inthashtable_t h_actorvar = { NULL, INTHASH_SIZE(ARRAY_SIZE(actorvartable)) };
#endif

static inthashtable_t *const inttables[] = {
    &h_varvar,
#ifdef CON_DISCRETE_VAR_ACCESS
    &h_globalvar,
    &h_playervar,
    &h_actorvar,
#endif
};


const tokenmap_t iter_tokens [] =
{
    { "allsprites",       ITER_ALLSPRITES },
    { "allspritesbystat", ITER_ALLSPRITESBYSTAT },
    { "allspritesbysect", ITER_ALLSPRITESBYSECT },
    { "allsectors",       ITER_ALLSECTORS },
    { "allwalls",         ITER_ALLWALLS },
    { "activelights",     ITER_ACTIVELIGHTS },
    { "drawnsprites",     ITER_DRAWNSPRITES },
    { "spritesofsector",  ITER_SPRITESOFSECTOR },
    { "spritesofstatus",  ITER_SPRITESOFSTATUS },
    { "loopofwall",       ITER_LOOPOFWALL },
    { "wallsofsector",    ITER_WALLSOFSECTOR },
    { "range",            ITER_RANGE },
    // vvv alternatives go here vvv
    { "lights",           ITER_ACTIVELIGHTS },
    { "sprofsec",         ITER_SPRITESOFSECTOR },
    { "sprofstat",        ITER_SPRITESOFSTATUS },
    { "walofsec",         ITER_WALLSOFSECTOR },
};

char const * VM_GetKeywordForID(int32_t id)
{
    // could be better but this is only called for diagnostics, ayy lmao
    for (tokenmap_t const & keyword : vm_keywords)
        if (keyword.val == id)
            return keyword.token;

    return "<unknown>";
}
#endif

// KEEPINSYNC with enum GameEvent_t and lunatic/con_lang.lua
const char *EventNames[MAXEVENTS] =
{
    "EVENT_INIT",
    "EVENT_ENTERLEVEL",
    "EVENT_RESETWEAPONS",
    "EVENT_RESETINVENTORY",
    "EVENT_HOLSTER",
    "EVENT_LOOKLEFT",
    "EVENT_LOOKRIGHT",
    "EVENT_SOARUP",
    "EVENT_SOARDOWN",
    "EVENT_CROUCH",
    "EVENT_JUMP",
    "EVENT_RETURNTOCENTER",
    "EVENT_LOOKUP",
    "EVENT_LOOKDOWN",
    "EVENT_AIMUP",
    "EVENT_FIRE",
    "EVENT_CHANGEWEAPON",
    "EVENT_GETSHOTRANGE",
    "EVENT_GETAUTOAIMANGLE",
    "EVENT_GETLOADTILE",
    "EVENT_CHEATGETSTEROIDS",
    "EVENT_CHEATGETHEAT",
    "EVENT_CHEATGETBOOT",
    "EVENT_CHEATGETSHIELD",
    "EVENT_CHEATGETSCUBA",
    "EVENT_CHEATGETHOLODUKE",
    "EVENT_CHEATGETJETPACK",
    "EVENT_CHEATGETFIRSTAID",
    "EVENT_QUICKKICK",
    "EVENT_INVENTORY",
    "EVENT_USENIGHTVISION",
    "EVENT_USESTEROIDS",
    "EVENT_INVENTORYLEFT",
    "EVENT_INVENTORYRIGHT",
    "EVENT_HOLODUKEON",
    "EVENT_HOLODUKEOFF",
    "EVENT_USEMEDKIT",
    "EVENT_USEJETPACK",
    "EVENT_TURNAROUND",
    "EVENT_DISPLAYWEAPON",
    "EVENT_FIREWEAPON",
    "EVENT_SELECTWEAPON",
    "EVENT_MOVEFORWARD",
    "EVENT_MOVEBACKWARD",
    "EVENT_TURNLEFT",
    "EVENT_TURNRIGHT",
    "EVENT_STRAFELEFT",
    "EVENT_STRAFERIGHT",
    "EVENT_WEAPKEY1",
    "EVENT_WEAPKEY2",
    "EVENT_WEAPKEY3",
    "EVENT_WEAPKEY4",
    "EVENT_WEAPKEY5",
    "EVENT_WEAPKEY6",
    "EVENT_WEAPKEY7",
    "EVENT_WEAPKEY8",
    "EVENT_WEAPKEY9",
    "EVENT_WEAPKEY10",
    "EVENT_DRAWWEAPON",
    "EVENT_DISPLAYCROSSHAIR",
    "EVENT_DISPLAYREST",
    "EVENT_DISPLAYSBAR",
    "EVENT_RESETPLAYER",
    "EVENT_INCURDAMAGE",
    "EVENT_AIMDOWN",
    "EVENT_GAME",
    "EVENT_PREVIOUSWEAPON",
    "EVENT_NEXTWEAPON",
    "EVENT_SWIMUP",
    "EVENT_SWIMDOWN",
    "EVENT_GETMENUTILE",
    "EVENT_SPAWN",
    "EVENT_LOGO",
    "EVENT_EGS",
    "EVENT_DOFIRE",
    "EVENT_PRESSEDFIRE",
    "EVENT_USE",
    "EVENT_PROCESSINPUT",
    "EVENT_FAKEDOMOVETHINGS",
    "EVENT_DISPLAYROOMS",
    "EVENT_KILLIT",
    "EVENT_LOADACTOR",
    "EVENT_DISPLAYBONUSSCREEN",
    "EVENT_DISPLAYMENU",
    "EVENT_DISPLAYMENUREST",
    "EVENT_DISPLAYLOADINGSCREEN",
    "EVENT_ANIMATESPRITES",
    "EVENT_NEWGAME",
    "EVENT_SOUND",
    "EVENT_CHECKTOUCHDAMAGE",
    "EVENT_CHECKFLOORDAMAGE",
    "EVENT_LOADGAME",
    "EVENT_SAVEGAME",
    "EVENT_PREGAME",
    "EVENT_CHANGEMENU",
    "EVENT_DAMAGEHPLANE",
    "EVENT_ACTIVATECHEAT",
    "EVENT_DISPLAYINACTIVEMENU",
    "EVENT_DISPLAYINACTIVEMENUREST",
    "EVENT_CUTSCENE",
    "EVENT_DISPLAYCURSOR",
    "EVENT_DISPLAYLEVELSTATS",
    "EVENT_DISPLAYCAMERAOSD",
    "EVENT_DISPLAYROOMSCAMERA",
    "EVENT_DISPLAYSTART",
    "EVENT_WORLD",
    "EVENT_PREWORLD",
    "EVENT_PRELEVEL",
    "EVENT_DISPLAYSPIT",
    "EVENT_DISPLAYFIST",
    "EVENT_DISPLAYKNEE",
    "EVENT_DISPLAYKNUCKLES",
    "EVENT_DISPLAYSCUBA",
    "EVENT_DISPLAYTIP",
    "EVENT_DISPLAYACCESS",
    "EVENT_MOVESECTOR",
    "EVENT_MOVEEFFECTORS",
    "EVENT_DISPLAYOVERHEADMAPTEXT",
    "EVENT_PRELOADGAME",
    "EVENT_POSTSAVEGAME",
    "EVENT_PRECUTSCENE",
    "EVENT_SKIPCUTSCENE",
    "EVENT_SCREEN",
    "EVENT_DISPLAYROOMSEND",
    "EVENT_DISPLAYEND",
    "EVENT_OPENMENUSOUND",
    "EVENT_RECOGSOUND",
    "EVENT_UPDATESCREENAREA",
    "EVENT_DISPLAYBORDER",
    "EVENT_SETDEFAULTS",
    "EVENT_MAINMENUSCREEN",
    "EVENT_NEWGAMESCREEN",
    "EVENT_ENDLEVELSCREEN",
    "EVENT_EXITGAMESCREEN",
    "EVENT_EXITPROGRAMSCREEN",
    "EVENT_ALTFIRE",
    "EVENT_ALTWEAPON",
    "EVENT_DISPLAYOVERHEADMAPPLAYER",
    "EVENT_MENUCURSORLEFT",
    "EVENT_MENUCURSORRIGHT",
    "EVENT_MENUCURSORSHADE",
    "EVENT_MENUSHADESELECTED",
    "EVENT_PLAYLEVELMUSICSLOT",
    "EVENT_CONTINUELEVELMUSICSLOT",
    "EVENT_DISPLAYPOINTER",
    "EVENT_LASTWEAPON",
    "EVENT_DAMAGESPRITE",
    "EVENT_POSTDAMAGESPRITE",
    "EVENT_DAMAGEWALL",
    "EVENT_DAMAGEFLOOR",
    "EVENT_DAMAGECEILING",
    "EVENT_DISPLAYROOMSCAMERATILE",
    "EVENT_RESETGOTPICS",
    "EVENT_VALIDATESTART",
    "EVENT_NEWGAMECUSTOM",
    "EVENT_INITCOMPLETE",
    "EVENT_CAPIR",
#ifdef LUNATIC
    "EVENT_ANIMATEALLSPRITES",
#endif
};

uint8_t *bitptr; // pointer to bitmap of which bytecode positions contain pointers

#define BITPTR_SET(x) bitmap_set(bitptr, x)
#define BITPTR_CLEAR(x) bitmap_clear(bitptr, x)
#define BITPTR_IS_POINTER(x) bitmap_test(bitptr, x)

#if !defined LUNATIC
hashtable_t h_arrays   = { MAXGAMEARRAYS >> 1, NULL };
hashtable_t h_gamevars = { MAXGAMEVARS >> 1, NULL };
hashtable_t h_labels   = { 11264 >> 1, NULL };

static void C_SetScriptSize(int32_t newsize)
{
    for (int i = 0; i < g_scriptSize - 1; ++i)
    {
        if (BITPTR_IS_POINTER(i))
        {
            if (EDUKE32_PREDICT_FALSE(apScript[i] < (intptr_t)apScript || apScript[i] >= (intptr_t)g_scriptPtr))
            {
                g_errorCnt++;
                buildprint("Internal compiler error at ", i, " (0x", hex(i), ")\n");
                VM_ScriptInfo(&apScript[i], 16);
            }
            else
                apScript[i] -= (intptr_t)apScript;
        }
    }

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);

    auto newscript = (intptr_t *)Xrealloc(apScript, newsize * sizeof(intptr_t));
    bitptr = (uint8_t *)Xrealloc(bitptr, (((newsize + 7) >> 3) + 1) * sizeof(uint8_t));

    if (newsize > g_scriptSize)
        Bmemset(&newscript[g_scriptSize], 0, (newsize - g_scriptSize) * sizeof(intptr_t));

    if (apScript != newscript)
    {
        buildprint("Relocated compiled code from 0x", hex((intptr_t)apScript), " to 0x", hex((intptr_t)newscript), "\n");
        g_scriptPtr = g_scriptPtr - apScript + newscript;
        apScript    = newscript;
    }

    int const smallestSize = min(g_scriptSize, newsize);

    for (int i = 0; i < smallestSize - 1; ++i)
    {
        if (BITPTR_IS_POINTER(i))
            apScript[i] += (intptr_t)apScript;
    }

    g_scriptSize = newsize;

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);
}

static inline bool ispecial(const char c)
{
    return (c == ' ' || c == 0x0d || c == '(' || c == ')' ||
        c == ',' || c == ';' || (c == 0x0a /*&& ++g_lineNumber*/));
}

static inline void scriptSkipLine(void)
{
    while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        textptr++;
}

static inline void scriptSkipSpaces(void)
{
    while (*textptr == ' ' || *textptr == '\t')
        textptr++;
}

static void C_SkipComments(void)
{
    do
    {
        switch (*textptr)
        {
        case '\n':
            g_lineNumber++;
            fallthrough__;
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
                if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    initprintf("%s:%d: debug: got comment.\n",g_scriptFileName,g_lineNumber);
                scriptSkipLine();
                continue;
            case '*': // beginning of a C style comment
                if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    initprintf("%s:%d: debug: got start of comment block.\n",g_scriptFileName,g_lineNumber);
                do
                {
                    if (*textptr == '\n')
                        g_lineNumber++;
                    textptr++;
                }
                while (*textptr && (textptr[0] != '*' || textptr[1] != '/'));

                if (EDUKE32_PREDICT_FALSE(!*textptr))
                {
                    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
                        initprintf("%s:%d: debug: EOF in comment!\n",g_scriptFileName,g_lineNumber);
                    C_ReportError(-1);
                    initprintf("%s:%d: error: found `/*' with no `*/'.\n",g_scriptFileName,g_lineNumber);
                    g_scriptActorOffset = g_numBraces = g_processingState = 0;
                    g_errorCnt++;
                    continue;
                }

                if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    initprintf("%s:%d: debug: got end of comment block.\n",g_scriptFileName,g_lineNumber);

                textptr+=2;
                continue;
            default:
                C_ReportError(-1);
                initprintf("%s:%d: error: malformed comment.\n", g_scriptFileName, g_lineNumber);
                scriptSkipLine();
                g_errorCnt++;
                continue;
            }
            break;

        default:
            if (ispecial(*textptr))
            {
                textptr++;
                continue;
            }
            fallthrough__;
        case 0: // EOF
            return;
        }
    }
    while (1);
}

static inline int GetDefID(char const *label) { return hash_find(&h_gamevars, label); }
static inline int GetADefID(char const *label) { return hash_find(&h_arrays, label); }

#define LAST_LABEL (label+(g_labelCnt<<6))
static inline bool isaltok(const char c)
{
    return (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '\\' || c == '*' || c == '-' || c == '_' ||
            c == '.');
}

static inline bool C_IsLabelChar(const char c, int32_t const i)
{
    return (isalnum(c) || c == '_' || c == '*' || c == '?' || (i > 0 && (c == '+' || c == '-')));
}

static inline int32_t C_GetLabelNameID(const memberlabel_t *pLabel, hashtable_t const * const table, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int const l = hash_findcase(table, psz);
    return (l >= 0) ? pLabel[l].lId : -1;
}

static inline int32_t C_GetLabelNameOffset(hashtable_t const * const table, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the offset in the array for the label, or -1

    return hash_findcase(table, psz);
}

static void C_GetNextLabelName(void)
{
    int32_t i = 0;

    C_SkipComments();

//    while (ispecial(*textptr) == 0 && *textptr!='['&& *textptr!=']' && *textptr!='\t' && *textptr!='\n' && *textptr!='\r')
    while (C_IsLabelChar(*textptr, i))
    {
        if (i < (1<<6)-1)
            label[(g_labelCnt<<6) + (i++)] = *textptr;
        textptr++;
    }

    label[(g_labelCnt<<6)+i] = 0;

    if (!(g_errorCnt|g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: label `%s'.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
}

static inline void scriptWriteValue(int32_t const value)
{
    BITPTR_CLEAR(g_scriptPtr-apScript);
    *g_scriptPtr++ = value;
}

// addresses passed to these functions must be within the block of memory pointed to by apScript
static inline void scriptWriteAtOffset(int32_t const value, intptr_t * const addr)
{
    BITPTR_CLEAR(addr-apScript);
    *(addr) = value;
}

static inline void scriptWritePointer(intptr_t const value, intptr_t * const addr)
{
    BITPTR_SET(addr-apScript);
    *(addr) = value;
}

static int32_t C_GetNextGameArrayName(void)
{
    C_GetNextLabelName();
    int32_t const i = GetADefID(LAST_LABEL);
    if (EDUKE32_PREDICT_FALSE(i < 0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_NOTAGAMEARRAY);
        return -1;
    }

    scriptWriteValue(i);
    return i;
}

static int C_GetKeyword(void)
{
    C_SkipComments();

    char *temptextptr = textptr;

    if (*temptextptr == 0) // EOF
        return -2;

    while (isaltok(*temptextptr) == 0)
    {
        temptextptr++;
        if (*temptextptr == 0)
            return 0;
    }

    int i = 0;

    while (isaltok(*temptextptr))
        tempbuf[i++] = *(temptextptr++);
    tempbuf[i] = 0;

    return hash_find(&h_keywords,tempbuf);
}

static int C_GetNextKeyword(void) //Returns its code #
{
    C_SkipComments();

    if (*textptr == 0) // EOF
        return -2;

    int l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    int i;
    if (EDUKE32_PREDICT_TRUE((i = hash_find(&h_keywords,tempbuf)) >= 0))
    {
        if (i == CON_LEFTBRACE || i == CON_RIGHTBRACE || i == CON_NULLOP)
            scriptWriteValue(i | (VM_IFELSE_MAGIC<<12));
        else scriptWriteValue(i | LINE_NUMBER);

        textptr += l;
        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: keyword `%s'.\n", g_scriptFileName, g_lineNumber, tempbuf);
        return i;
    }

    textptr += l;
    g_errorCnt++;

    if (EDUKE32_PREDICT_FALSE((tempbuf[0] == '{' || tempbuf[0] == '}') && tempbuf[1] != 0))
    {
        C_ReportError(-1);
        initprintf("%s:%d: error: expected whitespace between `%c' and `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf[0],tempbuf+1);
    }
    else C_ReportError(ERROR_EXPECTEDKEYWORD);

    return -1;
}

static int32_t parse_decimal_number(void)  // (textptr)
{
    // decimal constants -- this is finicky business
    int64_t num = strtoll(textptr, NULL, 10);  // assume long long to be int64_t

    if (EDUKE32_PREDICT_TRUE(num >= INT32_MIN && num <= INT32_MAX))
    {
        // all OK
    }
    else if (EDUKE32_PREDICT_FALSE(num > INT32_MAX && num <= UINT32_MAX))
    {
        // Number interpreted as uint32, but packed as int32 (on 32-bit archs)
        // (CON code in the wild exists that does this).  Note that such conversion
        // is implementation-defined (C99 6.3.1.3) but GCC does the 'expected' thing.
#if 0
        initprintf("%s:%d: warning: number greater than INT32_MAX converted to a negative one.\n",
                   g_szScriptFileName,g_lineNumber);
        g_numCompilerWarnings++;
#endif
    }
    else
    {
        // out of range, this is arguably worse

        initprintf("%s:%d: warning: number out of the range of a 32-bit integer encountered.\n",
                   g_scriptFileName,g_lineNumber);
        g_warningCnt++;
    }

    return (int32_t)num;
}

static int32_t parse_hex_constant(const char *hexnum)
{
    uint64_t x;
    sscanf(hexnum, "%" PRIx64 "", &x);

    if (EDUKE32_PREDICT_FALSE(x > UINT32_MAX))
    {
        initprintf(g_scriptFileName, ":", g_lineNumber, ": warning: number 0x", hex(x), " truncated to 32 bits.\n");
        g_warningCnt++;
    }

    return x;
}

static void C_GetNextVarType(int32_t type)
{
    int32_t id    = 0;
    int32_t flags = 0;

    auto varptr = g_scriptPtr;

    C_SkipComments();

    if (!type && !g_labelsOnly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        scriptWriteValue(GV_FLAG_CONSTANT);

        if (tolower(textptr[1])=='x')  // hex constants
            scriptWriteValue(parse_hex_constant(textptr+2));
        else
            scriptWriteValue(parse_decimal_number());

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: constant %ld in place of gamevar.\n", g_scriptFileName, g_lineNumber, (long)(g_scriptPtr[-1]));
#if 1
        while (!ispecial(*textptr) && *textptr != ']') textptr++;
#else
        C_GetNextLabelName();
#endif
        return;
    }
    else if (*textptr == '-'/* && !isdigit(*(textptr+1))*/)
    {
        if (EDUKE32_PREDICT_FALSE(type))
        {
            g_errorCnt++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextLabelName();
            return;
        }

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: flagging gamevar as negative.\n", g_scriptFileName, g_lineNumber); //,Batol(textptr));

        flags = GV_FLAG_NEGATIVE;
        textptr++;
    }

    C_GetNextLabelName();

    if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_ISAKEYWORD);
        return;
    }

    C_SkipComments();

    if (*textptr == '[' || *textptr == '.')     //read of array as a gamevar
    {
        flags |= GV_FLAG_ARRAY;
        if (*textptr != '.') textptr++;
        id=GetADefID(LAST_LABEL);

        if (id < 0)
        {
            id=GetDefID(LAST_LABEL);
            if ((unsigned) (id - g_structVarIDs) >= NUMQUICKSTRUCTS)
                id = -1;

            if (EDUKE32_PREDICT_FALSE(id < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return;
            }

            flags &= ~GV_FLAG_ARRAY; // not an array
            flags |= GV_FLAG_STRUCT;
        }

        scriptWriteValue(id|flags);

        if ((flags & GV_FLAG_STRUCT) && id - g_structVarIDs == STRUCT_USERDEF)
        {
            // userdef doesn't really have an array index
            while (*textptr != '.')
            {
                if (*textptr == 0xa || *textptr == 0)
                    break;

                textptr++;
            }

            scriptWriteValue(0); // help out the VM by inserting a dummy index
        }
        else
        {
            // allow "[]" or "." but not "[."
            if (*textptr == ']' || (*textptr == '.' && textptr[-1] != '['))
            {
                scriptWriteValue(g_thisActorVarID);
            }
            else
                C_GetNextVarType(0);

            C_SkipComments();
        }

        if (EDUKE32_PREDICT_FALSE(*textptr != ']' && *textptr != '.'))
        {
            g_errorCnt++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return;
        }

        if (*textptr != '.') textptr++;

        //writing arrays in this way is not supported because it would require too many changes to other code

        if (EDUKE32_PREDICT_FALSE(type))
        {
            g_errorCnt++;
            C_ReportError(ERROR_INVALIDARRAYWRITE);
            return;
        }

        if (flags & GV_FLAG_STRUCT)
        {
            while (*textptr != '.')
            {
                if (*textptr == 0xa || !*textptr)
                    break;

                textptr++;
            }

            if (EDUKE32_PREDICT_FALSE(*textptr != '.'))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                return;
            }
            textptr++;
            /// now pointing at 'xxx'
            C_GetNextLabelName();
            /*initprintf("found xxx label of \"%s\"\n",   label+(g_numLabels<<6));*/

            int32_t labelNum = -1;

            switch (id - g_structVarIDs)
            {
                case STRUCT_SPRITE:         labelNum = C_GetLabelNameOffset(&h_actor,      Bstrtolower(LAST_LABEL)); break;
                case STRUCT_SECTOR:         labelNum = C_GetLabelNameOffset(&h_sector,     Bstrtolower(LAST_LABEL)); break;
                case STRUCT_WALL:           labelNum = C_GetLabelNameOffset(&h_wall,       Bstrtolower(LAST_LABEL)); break;
                case STRUCT_PLAYER:         labelNum = C_GetLabelNameOffset(&h_player,     Bstrtolower(LAST_LABEL)); break;
                case STRUCT_TSPR:           labelNum = C_GetLabelNameOffset(&h_tsprite,    Bstrtolower(LAST_LABEL)); break;
                case STRUCT_PROJECTILE:
                case STRUCT_THISPROJECTILE: labelNum = C_GetLabelNameOffset(&h_projectile, Bstrtolower(LAST_LABEL)); break;
                case STRUCT_USERDEF:        labelNum = C_GetLabelNameOffset(&h_userdef,    Bstrtolower(LAST_LABEL)); break;
                case STRUCT_INPUT:          labelNum = C_GetLabelNameOffset(&h_input,      Bstrtolower(LAST_LABEL)); break;
                case STRUCT_TILEDATA:       labelNum = C_GetLabelNameOffset(&h_tiledata,   Bstrtolower(LAST_LABEL)); break;
                case STRUCT_PALDATA:        labelNum = C_GetLabelNameOffset(&h_paldata,    Bstrtolower(LAST_LABEL)); break;

                case STRUCT_ACTORVAR:
                case STRUCT_PLAYERVAR:      labelNum = GetDefID(LAST_LABEL); break;
            }

            if (labelNum == -1)
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAMEMBER);
                return;
            }

            switch (id - g_structVarIDs)
            {
            case STRUCT_SPRITE:
                {
                    auto const &label = ActorLabels[labelNum];

                    scriptWriteValue(label.lId);

                    Bassert((*varptr & (MAXGAMEVARS-1)) == g_structVarIDs + STRUCT_SPRITE);

                    if (label.flags & LABEL_HASPARM2)
                        C_GetNextVarType(0);
                    else if (label.offset != -1 && (label.flags & LABEL_READFUNC) == 0)
                    {
                        if (labelNum >= ACTOR_SPRITEEXT_BEGIN)
                            *varptr = (*varptr & ~(MAXGAMEVARS-1)) + g_structVarIDs + STRUCT_SPRITEEXT_INTERNAL__;
                        else if (labelNum >= ACTOR_STRUCT_BEGIN)
                            *varptr = (*varptr & ~(MAXGAMEVARS-1)) + g_structVarIDs + STRUCT_ACTOR_INTERNAL__;
                        else
                            *varptr = (*varptr & ~(MAXGAMEVARS-1)) + g_structVarIDs + STRUCT_SPRITE_INTERNAL__;
                    }
                }

                break;
            case STRUCT_SECTOR:
                scriptWriteValue(SectorLabels[labelNum].lId);
                break;
            case STRUCT_WALL:
                scriptWriteValue(WallLabels[labelNum].lId);
                break;
            case STRUCT_PLAYER:
                scriptWriteValue(PlayerLabels[labelNum].lId);

                if (PlayerLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVarType(0);
                break;
            case STRUCT_ACTORVAR:
            case STRUCT_PLAYERVAR:
                scriptWriteValue(labelNum);
                break;
            case STRUCT_TSPR:
                scriptWriteValue(TsprLabels[labelNum].lId);
                break;
            case STRUCT_PROJECTILE:
            case STRUCT_THISPROJECTILE:
                scriptWriteValue(ProjectileLabels[labelNum].lId);
                break;
            case STRUCT_USERDEF:
                scriptWriteValue(UserdefsLabels[labelNum].lId);

                if (UserdefsLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVarType(0);
                break;
            case STRUCT_INPUT:
                scriptWriteValue(InputLabels[labelNum].lId);
                break;
            case STRUCT_TILEDATA:
                scriptWriteValue(TileDataLabels[labelNum].lId);
                break;
            case STRUCT_PALDATA:
                scriptWriteValue(PalDataLabels[labelNum].lId);
                break;
            }
        }
        return;
    }

    id=GetDefID(LAST_LABEL);
    if (id<0)   //gamevar not found
    {
        if (EDUKE32_PREDICT_TRUE(!type && !g_labelsOnly))
        {
            //try looking for a define instead
            Bstrcpy(tempbuf,LAST_LABEL);
            id = hash_find(&h_labels,tempbuf);

            if (EDUKE32_PREDICT_TRUE(id>=0 && labeltype[id] & LABEL_DEFINE))
            {
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
                    initprintf("%s:%d: debug: label `%s' in place of gamevar.\n",g_scriptFileName,g_lineNumber,label+(id<<6));

                scriptWriteValue(GV_FLAG_CONSTANT);
                scriptWriteValue(labelcode[id]);
                return;
            }
        }

        g_errorCnt++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        return;
    }

    if (EDUKE32_PREDICT_FALSE(type == GAMEVAR_READONLY && aGameVars[id].flags & GAMEVAR_READONLY))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (EDUKE32_PREDICT_FALSE(aGameVars[id].flags & type))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }

    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
        initprintf("%s:%d: debug: gamevar `%s'.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);

    scriptWriteValue(id|flags);
}

#define C_GetNextVar() C_GetNextVarType(0)

static FORCE_INLINE void C_GetManyVarsType(int32_t type, int num)
{
    for (; num>0; --num)
        C_GetNextVarType(type);
}

#define C_GetManyVars(num) C_GetManyVarsType(0,num)

// returns:
//  -1 on EOF or wrong type or error
//   0 if literal value
//   LABEL_* (>0) if that type and matched
//
// *g_scriptPtr will contain the value OR 0 if wrong type or error
static int32_t C_GetNextValue(int32_t type)
{
    C_SkipComments();

    if (*textptr == 0) // EOF
        return -1;

    int32_t l = 0;

    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,tempbuf /*label+(g_numLabels<<6)*/)>=0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_ISAKEYWORD);
        textptr+=l;
    }

    int32_t i = hash_find(&h_labels,tempbuf);

    if (i>=0)
    {
        if (EDUKE32_PREDICT_TRUE(labeltype[i] & type))
        {
            if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
            {
                char *gl = C_GetLabelType(labeltype[i]);
                initprintf("%s:%d: debug: %s label `%s'.\n",g_scriptFileName,g_lineNumber,gl,label+(i<<6));
                Xfree(gl);
            }

            scriptWriteValue(labelcode[i]);

            textptr += l;
            return labeltype[i];
        }

        scriptWriteValue(0);
        textptr += l;
        char * const el = C_GetLabelType(type);
        char * const gl = C_GetLabelType(labeltype[i]);
        C_ReportError(-1);
        initprintf("%s:%d: warning: expected %s, found %s.\n",g_scriptFileName,g_lineNumber,el,gl);
        g_warningCnt++;
        Xfree(el);
        Xfree(gl);
        return -1;  // valid label name, but wrong type
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) == 0 && *textptr != '-'))
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        g_errorCnt++;
        scriptWriteValue(0);
        textptr+=l;
        if (!l) textptr++;
        return -1; // error!
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) && g_labelsOnly))
    {
        C_ReportError(WARNING_LABELSONLY);
        g_warningCnt++;
    }

    i = l-1;
    do
    {
        // FIXME: check for 0-9 A-F for hex
        if (textptr[0] == '0' && textptr[1] == 'x') break; // kill the warning for hex
        if (EDUKE32_PREDICT_FALSE(!isdigit(textptr[i--])))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: invalid character `%c' in definition!\n",g_scriptFileName,g_lineNumber,textptr[i+1]);
            g_warningCnt++;
            break;
        }
    }
    while (i > 0);

    if (textptr[0] == '0' && tolower(textptr[1])=='x')
        scriptWriteValue(parse_hex_constant(textptr+2));
    else
        scriptWriteValue(parse_decimal_number());

    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
        initprintf("%s:%d: debug: constant %ld.\n", g_scriptFileName, g_lineNumber, (long)g_scriptPtr[-1]);

    textptr += l;

    return 0;   // literal value
}

static int C_GetStructureIndexes(bool const labelsonly, hashtable_t const * const table)
{
    C_SkipComments();

    if (EDUKE32_PREDICT_FALSE(*textptr != '[' && *textptr != '.'))
    {
        g_errorCnt++;
        C_ReportError(ERROR_SYNTAXERROR);
        return -1;
    }

    if (*textptr != '.') textptr++;

    C_SkipComments();

    if (*textptr == ']' || *textptr == '.')
    {
        scriptWriteValue(g_thisActorVarID);
    }
    else
    {
        g_labelsOnly = labelsonly;
        C_GetNextVar();
        g_labelsOnly = 0;
    }

    if (*textptr != '.') textptr++;

    C_SkipComments();

    // now get name of .xxx

    if (EDUKE32_PREDICT_FALSE(*textptr++ != '.'))
    {
        g_errorCnt++;
        C_ReportError(ERROR_SYNTAXERROR);
        return -1;
    }

    if (!table)
        return 0;

    // .xxx

    C_GetNextLabelName();

    int const labelNum = C_GetLabelNameOffset(table, Bstrtolower(LAST_LABEL));

    if (EDUKE32_PREDICT_FALSE(labelNum == -1))
    {
        g_errorCnt++;
        C_ReportError(ERROR_NOTAMEMBER);
        return -1;
    }

    return labelNum;
}

#ifdef CURRENTLY_UNUSED
static FORCE_INLINE bool C_IntPow2(int32_t const v)
{
    return ((v!=0) && (v&(v-1))==0);
}

static inline uint32_t C_Pow2IntLogBase2(int32_t const v)
{
    static constexpr uint32_t b[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };

    uint32_t r = (v & b[0]) != 0;

    for (int i = 0; i < ARRAY_SSIZE(b); ++i)
        r |= ((v & b[i]) != 0) << i;

    return r;
}
#endif

static bool C_CheckMalformedBranch(intptr_t lastScriptPtr)
{
    switch (C_GetKeyword())
    {
    case CON_RIGHTBRACE:
    case CON_ENDA:
    case CON_ENDEVENT:
    case CON_ENDS:
    case CON_ELSE:
        g_scriptPtr = lastScriptPtr + apScript;
        g_skipBranch = true;
        C_ReportError(-1);
        g_warningCnt++;
        initprintf("%s:%d: warning: malformed `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        return true;
    }
    return false;
}

static bool C_CheckEmptyBranch(int tw, intptr_t lastScriptPtr)
{
    // ifrnd and the others actually do something when the condition is executed
    if ((Bstrncmp(VM_GetKeywordForID(tw), "if", 2) && tw != CON_ELSE) ||
            tw == CON_IFRND || tw == CON_IFHITWEAPON || tw == CON_IFCANSEE || tw == CON_IFCANSEETARGET ||
            tw == CON_IFPDISTL || tw == CON_IFPDISTG || tw == CON_IFGOTWEAPONCE)
    {
        g_skipBranch = false;
        return false;
    }

    if ((*(g_scriptPtr) & VM_INSTMASK) != CON_NULLOP || *(g_scriptPtr)>>12 != VM_IFELSE_MAGIC)
        g_skipBranch = false;

    if (EDUKE32_PREDICT_FALSE(g_skipBranch))
    {
        C_ReportError(-1);
        g_warningCnt++;
        g_scriptPtr = lastScriptPtr + apScript;
        initprintf("%s:%d: warning: empty `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        scriptWriteAtOffset(CON_NULLOP | (VM_IFELSE_MAGIC<<12), g_scriptPtr);
        return true;
    }

    return false;
}

static int C_CountCaseStatements()
{
    char *const    temptextptr      = textptr;
    int const      backupLineNumber = g_lineNumber;
    int const      backupNumCases   = g_numCases;
    intptr_t const casePtrOffset    = g_caseTablePtr - apScript;
    intptr_t const scriptPtrOffset  = g_scriptPtr - apScript;

    g_numCases = 0;
    g_caseTablePtr = NULL;
    C_ParseCommand(true);

    // since we processed the endswitch, we need to re-increment g_checkingSwitch
    g_checkingSwitch++;

    int const numCases = g_numCases;

    textptr        = temptextptr;
    g_lineNumber   = backupLineNumber;
    g_numCases     = backupNumCases;
    g_caseTablePtr = apScript + casePtrOffset;
    g_scriptPtr    = apScript + scriptPtrOffset;

    return numCases;
}

static void C_Include(const char *confile)
{
    buildvfs_kfd fp = kopen4loadfrommod(confile, g_loadFromGroupOnly);

    if (EDUKE32_PREDICT_FALSE(fp == buildvfs_kfd_invalid))
    {
        g_errorCnt++;
        initprintf("%s:%d: error: could not find file `%s'.\n",g_scriptFileName,g_lineNumber,confile);
        return;
    }

    int32_t const len = kfilelength(fp);
    char *mptr = (char *)Xmalloc(len+1);

    initprintf("Including: %s (%d bytes)\n",confile, len);

    kread(fp, mptr, len);
    kclose(fp);

    mptr[len] = 0;
    g_scriptcrc = Bcrc32(mptr, len, g_scriptcrc);

    if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
        textptr++;

    char * const origtptr = textptr;
    char parentScriptFileName[BMAX_PATH];

    Bstrcpy(parentScriptFileName, g_scriptFileName);
    Bstrcpy(g_scriptFileName, confile);

    int const temp_ScriptLineNumber = g_lineNumber;
    g_lineNumber = 1;

    int const temp_ifelse_check = g_checkingIfElse;
    g_checkingIfElse = 0;

    textptr = mptr;

    C_SkipComments();
    C_ParseCommand(true);

    Bstrcpy(g_scriptFileName, parentScriptFileName);

    g_totalLines += g_lineNumber;
    g_lineNumber = temp_ScriptLineNumber;
    g_checkingIfElse = temp_ifelse_check;

    textptr = origtptr;

    Xfree(mptr);
}
#endif  // !defined LUNATIC

#ifdef _WIN32
static void check_filename_case(const char *fn)
{
    buildvfs_kfd fp;
    if ((fp = kopen4loadfrommod(fn, g_loadFromGroupOnly)) != buildvfs_kfd_invalid)
        kclose(fp);
}
#else
static void check_filename_case(const char *fn) { UNREFERENCED_PARAMETER(fn); }
#endif

void G_DoGameStartup(const int32_t *params)
{
    auto &p0 = *g_player[0].ps;
    int j = 0;

    ud.const_visibility  = params[j++];
    g_impactDamage       = params[j++];

    p0.max_shield_amount = params[j];
    p0.max_player_health = params[j];
    g_maxPlayerHealth    = params[j++];

    g_startArmorAmount   = params[j++];
    g_actorRespawnTime   = params[j++];
    g_itemRespawnTime    = (g_scriptVersion >= 11) ? params[j++] : g_actorRespawnTime;

    if (g_scriptVersion >= 11)
        g_playerFriction = params[j++];

    if (g_scriptVersion >= 14)
        g_spriteGravity = params[j++];

    if (g_scriptVersion >= 11)
    {
        g_rpgRadius        = params[j++];
        g_pipebombRadius   = params[j++];
        g_shrinkerRadius   = params[j++];
        g_tripbombRadius   = params[j++];
        g_morterRadius     = params[j++];
        g_bouncemineRadius = params[j++];
        g_seenineRadius    = params[j++];
    }

    p0.max_ammo_amount[PISTOL_WEAPON]     = params[j++];
    p0.max_ammo_amount[SHOTGUN_WEAPON]    = params[j++];
    p0.max_ammo_amount[CHAINGUN_WEAPON]   = params[j++];
    p0.max_ammo_amount[RPG_WEAPON]        = params[j++];
    p0.max_ammo_amount[HANDBOMB_WEAPON]   = params[j++];
    p0.max_ammo_amount[SHRINKER_WEAPON]   = params[j++];
    p0.max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
    p0.max_ammo_amount[TRIPBOMB_WEAPON]   = params[j++];

    if (g_scriptVersion >= 13)
    {
        p0.max_ammo_amount[FREEZE_WEAPON] = params[j++];

        if (g_scriptVersion >= 14)
            p0.max_ammo_amount[GROW_WEAPON] = params[j++];

        g_damageCameras     = params[j++];
        g_numFreezeBounces  = params[j++];
        g_freezerSelfDamage = params[j++];

        if (g_scriptVersion >= 14)
        {
            g_deleteQueueSize   = clamp(params[j++], 0, ARRAY_SSIZE(SpriteDeletionQueue));
            g_tripbombLaserMode = params[j++];
        }
    }
}

void C_DefineMusic(int volumeNum, int levelNum, const char *fileName)
{
    Bassert((unsigned)volumeNum < MAXVOLUMES+1);
    Bassert((unsigned)levelNum < MAXLEVELS);

    if (strcmp(fileName, "/.") == 0)
        return;

    map_t *const pMapInfo = &g_mapInfo[(MAXLEVELS*volumeNum)+levelNum];

    Xfree(pMapInfo->musicfn);
    pMapInfo->musicfn = dup_filename(fileName);
    check_filename_case(pMapInfo->musicfn);
}

#ifdef LUNATIC
void C_DefineSound(int32_t sndidx, const char *fn, int32_t args[5])
{
    Bassert((unsigned)sndidx < MAXSOUNDS);

    {
        sound_t *const snd = &g_sounds[sndidx];

        Xfree(snd->filename);
        snd->filename = dup_filename(fn);
        check_filename_case(snd->filename);

        snd->ps = args[0];
        snd->pe = args[1];
        snd->pr = args[2];
        snd->m = args[3] & ~SF_ONEINST_INTERNAL;
        if (args[3] & SF_LOOP)
            snd->m |= SF_ONEINST_INTERNAL;
        snd->vo = args[4];

        if (sndidx > g_highestSoundIdx)
            g_highestSoundIdx = sndidx;
    }
}

void C_DefineQuote(int32_t qnum, const char *qstr)
{
    C_AllocQuote(qnum);
    Bstrncpyz(apStrings[qnum], qstr, MAXQUOTELEN);
}

void C_DefineVolumeName(int32_t vol, const char *name)
{
    Bassert((unsigned)vol < MAXVOLUMES);
    Bstrncpyz(g_volumeNames[vol], name, sizeof(g_volumeNames[vol]));
    g_volumeCnt = vol+1;
}

void C_DefineSkillName(int32_t skill, const char *name)
{
    Bassert((unsigned)skill < MAXSKILLS);
    Bstrncpyz(g_skillNames[skill], name, sizeof(g_skillNames[skill]));
    g_skillCnt = max(g_skillCnt, skill+1);  // TODO: bring in line with C-CON?
}

void C_DefineLevelName(int32_t vol, int32_t lev, const char *fn,
                       int32_t partime, int32_t designertime,
                       const char *levelname)
{
    Bassert((unsigned)vol < MAXVOLUMES);
    Bassert((unsigned)lev < MAXLEVELS);

    {
        map_t *const map = &g_mapInfo[(MAXLEVELS*vol)+lev];

        Xfree(map->filename);
        map->filename = dup_filename(fn);

        // TODO: truncate to 32 chars?
        Xfree(map->name);
        map->name = Xstrdup(levelname);

        map->partime = REALGAMETICSPERSEC * partime;
        map->designertime = REALGAMETICSPERSEC * designertime;
    }
}

void C_DefineGameFuncName(int32_t idx, const char *name)
{
    assert((unsigned)idx < NUMGAMEFUNCTIONS);

    Bstrncpyz(gamefunctions[idx], name, MAXGAMEFUNCLEN);

    hash_add(&h_gamefuncs, gamefunctions[idx], idx, 0);
}

void C_DefineGameType(int32_t idx, int32_t flags, const char *name)
{
    Bassert((unsigned)idx < MAXGAMETYPES);

    g_gametypeFlags[idx] = flags;
    Bstrncpyz(g_gametypeNames[idx], name, sizeof(g_gametypeNames[idx]));
    g_gametypeCnt = idx+1;
}
#endif

void C_DefineVolumeFlags(int32_t vol, int32_t flags)
{
    Bassert((unsigned)vol < MAXVOLUMES);

    g_volumeFlags[vol] = flags;
}

void C_UndefineVolume(int32_t vol)
{
    Bassert((unsigned)vol < MAXVOLUMES);

    for (bssize_t i = 0; i < MAXLEVELS; i++)
        C_UndefineLevel(vol, i);

    g_volumeNames[vol][0] = '\0';

    g_volumeCnt = 0;
    for (bssize_t i = MAXVOLUMES-1; i >= 0; i--)
    {
        if (g_volumeNames[i][0])
        {
            g_volumeCnt = i+1;
            break;
        }
    }
}

void C_UndefineSkill(int32_t skill)
{
    Bassert((unsigned)skill < MAXSKILLS);

    g_skillNames[skill][0] = '\0';

    g_skillCnt = 0;
    for (bssize_t i = MAXSKILLS-1; i >= 0; i--)
    {
        if (g_skillNames[i][0])
        {
            g_skillCnt = i+1;
            break;
        }
    }
}

void C_UndefineLevel(int32_t vol, int32_t lev)
{
    Bassert((unsigned)vol < MAXVOLUMES);
    Bassert((unsigned)lev < MAXLEVELS);

    map_t *const map = &g_mapInfo[(MAXLEVELS*vol)+lev];

    DO_FREE_AND_NULL(map->filename);
    DO_FREE_AND_NULL(map->name);
    map->partime = 0;
    map->designertime = 0;
}

LUNATIC_EXTERN int32_t C_SetDefName(const char *name)
{
    clearDefNamePtr();
    g_defNamePtr = dup_filename(name);
    if (g_defNamePtr)
        initprintf("Using DEF file: %s.\n", g_defNamePtr);
    return (g_defNamePtr==NULL);
}

defaultprojectile_t DefaultProjectile;

EDUKE32_STATIC_ASSERT(sizeof(projectile_t) == sizeof(DefaultProjectile));

void C_AllocProjectile(int32_t j)
{
    g_tile[j].proj = (projectile_t *)Xrealloc(g_tile[j].proj, 2 * sizeof(projectile_t));
    g_tile[j].defproj = g_tile[j].proj + 1;
}

void C_FreeProjectile(int32_t j)
{
    DO_FREE_AND_NULL(g_tile[j].proj);
    g_tile[j].defproj = NULL;
}


LUNATIC_EXTERN void C_DefineProjectile(int32_t j, int32_t what, int32_t val)
{
    if (g_tile[j].proj == NULL)
    {
        C_AllocProjectile(j);
        *g_tile[j].proj = DefaultProjectile;
    }

    projectile_t * const proj = g_tile[j].proj;

    switch (what)
    {
        case PROJ_WORKSLIKE:   proj->workslike  = val; break;
        case PROJ_SPAWNS:      proj->spawns     = val; break;
        case PROJ_SXREPEAT:    proj->sxrepeat   = val; break;
        case PROJ_SYREPEAT:    proj->syrepeat   = val; break;
        case PROJ_SOUND:       proj->sound      = val; break;
        case PROJ_ISOUND:      proj->isound     = val; break;
        case PROJ_VEL:         proj->vel        = val; break;
        case PROJ_EXTRA:       proj->extra      = val; break;
        case PROJ_DECAL:       proj->decal      = val; break;
        case PROJ_TRAIL:       proj->trail      = val; break;
        case PROJ_TXREPEAT:    proj->txrepeat   = val; break;
        case PROJ_TYREPEAT:    proj->tyrepeat   = val; break;
        case PROJ_TOFFSET:     proj->toffset    = val; break;
        case PROJ_TNUM:        proj->tnum       = val; break;
        case PROJ_DROP:        proj->drop       = val; break;
        case PROJ_CSTAT:       proj->cstat      = val; break;
        case PROJ_CLIPDIST:    proj->clipdist   = val; break;
        case PROJ_SHADE:       proj->shade      = val; break;
        case PROJ_XREPEAT:     proj->xrepeat    = val; break;
        case PROJ_YREPEAT:     proj->yrepeat    = val; break;
        case PROJ_PAL:         proj->pal        = val; break;
        case PROJ_EXTRA_RAND:  proj->extra_rand = val; break;
        case PROJ_HITRADIUS:   proj->hitradius  = val; break;
        case PROJ_MOVECNT:     proj->movecnt    = val; break;
        case PROJ_OFFSET:      proj->offset     = val; break;
        case PROJ_BOUNCES:     proj->bounces    = val; break;
        case PROJ_BSOUND:      proj->bsound     = val; break;
        case PROJ_RANGE:       proj->range      = val; break;
        case PROJ_FLASH_COLOR: proj->flashcolor = val; break;
        case PROJ_USERDATA:    proj->userdata   = val; break;
        default: break;
    }

    *g_tile[j].defproj = *proj;

    g_tile[j].flags |= SFLAG_PROJECTILE;
}

int32_t C_AllocQuote(int32_t qnum)
{
    Bassert((unsigned)qnum < MAXQUOTES);

    if (apStrings[qnum] == NULL)
    {
        apStrings[qnum] = (char *)Xcalloc(MAXQUOTELEN,sizeof(uint8_t));
        return 1;
    }

    return 0;
}

#ifndef EDUKE32_TOUCH_DEVICES
static void C_ReplaceQuoteSubstring(const size_t q, char const * const query, char const * const replacement)
{
    size_t querylength = Bstrlen(query);

    for (bssize_t i = MAXQUOTELEN - querylength - 2; i >= 0; i--)
        if (Bstrncmp(&apStrings[q][i], query, querylength) == 0)
        {
            Bmemset(tempbuf, 0, sizeof(tempbuf));
            Bstrncpy(tempbuf, apStrings[q], i);
            Bstrcat(tempbuf, replacement);
            Bstrcat(tempbuf, &apStrings[q][i + querylength]);
            Bstrncpy(apStrings[q], tempbuf, MAXQUOTELEN - 1);
            i = MAXQUOTELEN - querylength - 2;
        }
}
#endif

void C_InitQuotes(void)
{
    for (int i = 0; i < 128; i++) C_AllocQuote(i);

#ifdef EDUKE32_TOUCH_DEVICES
    apStrings[QUOTE_DEAD] = 0;
#else
    char const * const OpenGameFunc = gamefunctions[gamefunc_Open];
    C_ReplaceQuoteSubstring(QUOTE_DEAD, "SPACE", OpenGameFunc);
    C_ReplaceQuoteSubstring(QUOTE_DEAD, "OPEN", OpenGameFunc);
    C_ReplaceQuoteSubstring(QUOTE_DEAD, "USE", OpenGameFunc);
#endif

    // most of these are based on Blood, obviously
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

    EDUKE32_STATIC_ASSERT(OBITQUOTEINDEX + ARRAY_SIZE(PlayerObituaries)-1 < MAXQUOTES);
    EDUKE32_STATIC_ASSERT(SUICIDEQUOTEINDEX + ARRAY_SIZE(PlayerSelfObituaries)-1 < MAXQUOTES);

    g_numObituaries = ARRAY_SIZE(PlayerObituaries);
    for (bssize_t i = g_numObituaries - 1; i >= 0; i--)
    {
        if (C_AllocQuote(i + OBITQUOTEINDEX))
            Bstrcpy(apStrings[i + OBITQUOTEINDEX], PlayerObituaries[i]);
    }

    g_numSelfObituaries = ARRAY_SIZE(PlayerSelfObituaries);
    for (bssize_t i = g_numSelfObituaries - 1; i >= 0; i--)
    {
        if (C_AllocQuote(i + SUICIDEQUOTEINDEX))
            Bstrcpy(apStrings[i + SUICIDEQUOTEINDEX], PlayerSelfObituaries[i]);
    }
}

LUNATIC_EXTERN void C_SetCfgName(const char *cfgname)
{
    if (Bstrcmp(g_setupFileName, cfgname) == 0) // no need to do anything if name is the same
        return;

    if (Bstrcmp(g_setupFileName, SETUPFILENAME) != 0) // set to something else via -cfg
        return;

    ud_setup_t const config = ud.setup;
#ifdef POLYMER
    int const renderMode = glrendmode;
#endif

    if (!buildvfs_isdir(g_modDir))
    {
        if (buildvfs_mkdir(g_modDir, S_IRWXU) != 0)
        {
            OSD_Printf("Failed to create directory \"%s\"!\n", g_modDir);
            return;
        }
        else
            OSD_Printf("Created configuration file directory %s\n", g_modDir);
    }

    // XXX: Back up 'cfgname' as it may be the global 'tempbuf'.
    char *temp = Xstrdup(cfgname);

    CONFIG_WriteSetup(1);

    if (g_modDir[0] != '/')
        Bsnprintf(g_setupFileName, sizeof(g_setupFileName), "%s/%s", g_modDir, temp);
    else
        Bstrncpyz(g_setupFileName, temp, sizeof(g_setupFileName));

    DO_FREE_AND_NULL(temp);

    initprintf("Using config file \"%s\".\n", g_setupFileName);

    CONFIG_ReadSetup();

    ud.setup = config;
#ifdef POLYMER
    glrendmode = renderMode;
#endif
}

#if !defined LUNATIC
static inline void C_BitOrNextValue(int32_t *valptr)
{
    C_GetNextValue(LABEL_DEFINE);
    g_scriptPtr--;
    *valptr |= *g_scriptPtr;
}

static inline void C_FinishBitOr(int32_t value)
{
    scriptWriteValue(value);
}

static void C_FillEventBreakStackWithJump(intptr_t *breakPtr, intptr_t destination)
{
    while (breakPtr)
    {
        breakPtr = apScript + (intptr_t)breakPtr;
        intptr_t const tempPtr = *breakPtr;
        scriptWriteAtOffset(destination, breakPtr);
        breakPtr = (intptr_t *)tempPtr;
    }
}

static void scriptUpdateOpcodeForVariableType(intptr_t *ins)
{
    int opcode = -1;

#ifdef CON_DISCRETE_VAR_ACCESS
    if (ins[1] < MAXGAMEVARS)
    {
        switch (aGameVars[ins[1] & (MAXGAMEVARS - 1)].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))
        {
            case 0:
                opcode = inthash_find(&h_globalvar, *ins & VM_INSTMASK);
                break;
            case GAMEVAR_PERACTOR:
                opcode = inthash_find(&h_actorvar, *ins & VM_INSTMASK);
                break;
            case GAMEVAR_PERPLAYER:
                opcode = inthash_find(&h_playervar, *ins & VM_INSTMASK);
                break;
        }
    }
#endif

    if (opcode != -1)
    {
        if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
        {
            initprintf("%s:%d: %s -> %s for var %s\n", g_scriptFileName, g_lineNumber,
                        VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode), aGameVars[ins[1] & (MAXGAMEVARS-1)].szLabel);
        }

        scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
    }
}

static bool C_ParseCommand(bool loop /*= false*/)
{
    int32_t i, j=0, k=0, tw;

    do
    {
        if (EDUKE32_PREDICT_FALSE(g_errorCnt > 63 || (*textptr == '\0') || (*(textptr+1) == '\0')))
            return 1;

        if ((g_scriptPtr - apScript) > (g_scriptSize - 4096) && g_caseTablePtr == NULL)
            C_SetScriptSize(g_scriptSize << 1);

        if (EDUKE32_PREDICT_FALSE(g_scriptDebug))
            C_ReportError(-1);

        int const otw = g_lastKeyword;

        C_SkipComments();

        switch ((g_lastKeyword = tw = C_GetNextKeyword()))
        {
        default:
        case -1:
        case -2:
            return 1; //End
        case CON_DEFSTATE:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
                continue;
            }
            goto DO_DEFSTATE;
        case CON_STATE:
            if (!g_scriptActorOffset && g_processingState == 0)
            {
DO_DEFSTATE:
                C_GetNextLabelName();
                g_scriptPtr--;
                labelcode[g_labelCnt] = g_scriptPtr-apScript;
                labeltype[g_labelCnt] = LABEL_STATE;

                g_processingState = 1;
                Bsprintf(g_szCurrentBlockName,"%s",LAST_LABEL);

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_gamevars,LAST_LABEL)>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
                g_labelCnt++;
                continue;
            }

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE((j = hash_find(&h_labels,LAST_LABEL)) < 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: state `%s' not found.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                g_errorCnt++;
                g_scriptPtr++;
                continue;
            }

            if (EDUKE32_PREDICT_FALSE((labeltype[j] & LABEL_STATE) != LABEL_STATE))
            {
                char *gl = (char *) C_GetLabelType(labeltype[j]);
                C_ReportError(-1);
                initprintf("%s:%d: warning: expected state, found %s.\n", g_scriptFileName, g_lineNumber, gl);
                g_warningCnt++;
                Xfree(gl);
                scriptWriteAtOffset(CON_NULLOP, &g_scriptPtr[-1]); // get rid of the state, leaving a nullop to satisfy if conditions
                continue;  // valid label name, but wrong type
            }

            if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                initprintf("%s:%d: debug: state label `%s'.\n", g_scriptFileName, g_lineNumber, label+(j<<6));

            // 'state' type labels are always script addresses, as far as I can see
            scriptWritePointer((intptr_t)(apScript+labelcode[j]), g_scriptPtr++);
            continue;

        case CON_ENDS:
            if (EDUKE32_PREDICT_FALSE(g_processingState == 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `ends' without open `state'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }

            if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
            {
                C_ReportError(ERROR_NOTTOPLEVEL);
                g_errorCnt++;
            }

            if (EDUKE32_PREDICT_FALSE(g_checkingSwitch > 0))
            {
                C_ReportError(ERROR_NOENDSWITCH);
                g_errorCnt++;

                g_checkingSwitch = 0; // can't be checking anymore...
            }

            g_processingState = 0;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_GETPROJECTILE:
        case CON_GETTHISPROJECTILE:
        case CON_SETPROJECTILE:
        case CON_SETTHISPROJECTILE:
            {
                int const labelNum = C_GetStructureIndexes(tw == CON_SETTHISPROJECTILE || tw == CON_GETTHISPROJECTILE, &h_projectile);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(ProjectileLabels[labelNum].lId);

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
                continue;
            }

        case CON_GAMEVAR:
        {
            // syntax: gamevar <var1> <initial value> <flags>
            // defines var1 and sets initial value.
            // flags are used to define usage
            // (see top of this files for flags)

            if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) || (*textptr == '-')))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                scriptSkipLine();
                continue;
            }

            g_scriptPtr--;

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords, LAST_LABEL)>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_VARMASKSKEYWORD);
                hash_delete(&h_keywords, LAST_LABEL);
            }

            int32_t defaultValue = 0;
            int32_t varFlags     = 0;

            if (C_GetKeyword() == -1)
            {
                C_GetNextValue(LABEL_DEFINE); // get initial value
                defaultValue = *(--g_scriptPtr);

                j = 0;

                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);

                C_FinishBitOr(j);
                varFlags = *(--g_scriptPtr);

                if (EDUKE32_PREDICT_FALSE((*(g_scriptPtr)&GAMEVAR_USER_MASK)==(GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR)))
                {
                    g_warningCnt++;
                    varFlags ^= GAMEVAR_PERPLAYER;
                    C_ReportError(WARNING_BADGAMEVAR);
                }
            }

            Gv_NewVar(LAST_LABEL, defaultValue, varFlags);
            continue;
        }

        case CON_GAMEARRAY:
        {
            if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) || (*textptr == '-')))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                scriptSkipLine();
                continue;
            }
            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_ARRAYMASKSKEYWORD);
                hash_delete(&h_keywords, LAST_LABEL);
            }

            i = hash_find(&h_gamevars,LAST_LABEL);
            if (EDUKE32_PREDICT_FALSE(i>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            C_GetNextValue(LABEL_DEFINE);

            char const * const arrayName = LAST_LABEL;
            int32_t arrayFlags = 0;

            while (C_GetKeyword() == -1)
                C_BitOrNextValue(&arrayFlags);

            C_FinishBitOr(arrayFlags);

            arrayFlags = g_scriptPtr[-1];
            g_scriptPtr--;

            Gv_NewArray(arrayName, NULL, g_scriptPtr[-1], arrayFlags);

            g_scriptPtr -= 2; // no need to save in script...
            continue;
        }

        case CON_DEFINE:
            {
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,LAST_LABEL);
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                C_GetNextValue(LABEL_DEFINE);

                i = hash_find(&h_labels,LAST_LABEL);
                if (i>=0)
                {
                    // if (i >= g_numDefaultLabels)

                    if (EDUKE32_PREDICT_FALSE(labelcode[i] != g_scriptPtr[-1]))
                    {
                        g_warningCnt++;
                        initprintf("%s:%d: warning: ignored redefinition of `%s' to %d (old: %d).\n",g_scriptFileName,
                                   g_lineNumber,LAST_LABEL, (int32_t)(g_scriptPtr[-1]), labelcode[i]);
                    }
                }
                else
                {
                    hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
                    labeltype[g_labelCnt] = LABEL_DEFINE;
                    labelcode[g_labelCnt++] = g_scriptPtr[-1];
                    if (g_scriptPtr[-1] >= 0 && g_scriptPtr[-1] < MAXTILES && g_dynamicTileMapping)
                        G_ProcessDynamicTileMapping(label+((g_labelCnt-1)<<6),g_scriptPtr[-1]);
                }
                g_scriptPtr -= 2;
                continue;
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
                scriptWriteValue(0);
                j--;
            }
            continue;

        case CON_MOVE:
            if (g_scriptActorOffset || g_processingState)
            {
                if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (g_scriptPtr[-1] != 0) && (g_scriptPtr[-1] != 1)))
                {
                    C_ReportError(-1);
                    scriptWriteAtOffset(0, &g_scriptPtr[-1]);
                    initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                }

                j = 0;
                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);

                C_FinishBitOr(j);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();
                // Check to see it's already defined

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_gamevars,LAST_LABEL)>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                if (EDUKE32_PREDICT_FALSE((i = hash_find(&h_labels,LAST_LABEL)) >= 0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate move `%s' ignored.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                }
                else
                {
                    hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
                    labeltype[g_labelCnt] = LABEL_MOVE;
                    labelcode[g_labelCnt++] = g_scriptPtr-apScript;
                }

                for (j=1; j>=0; j--)
                {
                    if (C_GetKeyword() != -1) break;
                    C_GetNextValue(LABEL_DEFINE);
                }

                for (k=j; k>=0; k--)
                {
                    scriptWriteValue(0);
                }
            }
            continue;

        case CON_MUSIC:
            {
                // NOTE: this doesn't get stored in the PCode...

                // music 1 stalker.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
                //    thecall.mid ahgeez.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
                g_scriptPtr--;
                C_GetNextValue(LABEL_DEFINE); // Volume Number (0/4)
                g_scriptPtr--;

                k = *g_scriptPtr-1;  // 0-based volume number. -1 or MAXVOLUMES: "special"
                if (k == -1)
                    k = MAXVOLUMES;

                if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXVOLUMES+1)) // if it's not background or special music
                {
                    g_errorCnt++;
                    C_ReportError(-1);
                    initprintf("%s:%d: error: volume number must be between 0 and MAXVOLUMES+1=%d.\n",
                               g_scriptFileName, g_lineNumber, MAXVOLUMES+1);
                    continue;

                }

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

                    C_DefineMusic(k, i, tempbuf);

                    textptr += j;

                    if (i >= MAXLEVELS)
                        break;
                    i++;
                }
            }
            continue;

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

            C_Include(tempbuf);
            continue;

        case CON_INCLUDEDEFAULT:
            C_SkipComments();
            C_Include(G_DefaultConFile());
            continue;

        case CON_AI:
            if (g_scriptActorOffset || g_processingState)
            {
                C_GetNextValue(LABEL_AI);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,LAST_LABEL);
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                i = hash_find(&h_labels,LAST_LABEL);
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate ai `%s' ignored.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_AI;
                    hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
                    labelcode[g_labelCnt++] = g_scriptPtr-apScript;
                }

                for (j=0; j<3; j++)
                {
                    if (C_GetKeyword() != -1) break;
                    if (j == 1)
                        C_GetNextValue(LABEL_ACTION);
                    else if (j == 2)
                    {
                        if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) &&
                            (g_scriptPtr[-1] != 0) && (g_scriptPtr[-1] != 1)))
                        {
                            C_ReportError(-1);
                            scriptWriteAtOffset(0, &g_scriptPtr[-1]);
                            initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                            g_warningCnt++;
                        }

                        k = 0;
                        while (C_GetKeyword() == -1)
                            C_BitOrNextValue(&k);

                        C_FinishBitOr(k);
                        j = 666;
                        break;
                    }
                }

                if (j == 666)
                    continue;

                for (k=j; k<3; k++)
                {
                    scriptWriteValue(0);
                }
            }
            continue;

        case CON_ACTION:
            if (g_scriptActorOffset || g_processingState)
            {
                C_GetNextValue(LABEL_ACTION);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();
                // Check to see it's already defined

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,LAST_LABEL);
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                i = hash_find(&h_labels,LAST_LABEL);
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate action `%s' ignored.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_ACTION;
                    labelcode[g_labelCnt] = g_scriptPtr-apScript;
                    hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
                    g_labelCnt++;
                }

                for (j=ACTION_PARAM_COUNT-1; j>=0; j--)
                {
                    if (C_GetKeyword() != -1) break;
                    C_GetNextValue(LABEL_DEFINE);
                }
                for (k=j; k>=0; k--)
                {
                    scriptWriteValue(0);
                }
            }
            continue;

        case CON_ACTOR:
        case CON_USERACTOR:
        case CON_EVENTLOADACTOR:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_scriptActorOffset = g_scriptPtr - apScript;

            if (tw == CON_USERACTOR)
            {
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
            }

            // save the actor name w/o consuming it
            C_SkipComments();
            j = 0;
            while (isaltok(*(textptr+j)))
            {
                g_szCurrentBlockName[j] = textptr[j];
                j++;
            }
            g_szCurrentBlockName[j] = 0;

            j = hash_find(&h_labels, g_szCurrentBlockName);

            if (j != -1)
                labeltype[j] |= LABEL_ACTOR;

            if (tw == CON_USERACTOR)
            {
                j = *g_scriptPtr;

                if (EDUKE32_PREDICT_FALSE(j > 6 || (j&3)==3))
                {
                    C_ReportError(-1);
                    initprintf("%s:%d: warning: invalid useractor type. Must be 0, 1, 2"
                               " (notenemy, enemy, enemystayput) or have 4 added (\"doesn't move\").\n",
                               g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                    j = 0;
                }
            }

            C_GetNextValue(LABEL_ACTOR);
            g_scriptPtr--;

            if (EDUKE32_PREDICT_FALSE((unsigned)*g_scriptPtr >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                continue;
            }

            if (tw == CON_EVENTLOADACTOR)
            {
                g_tile[*g_scriptPtr].loadPtr = apScript + g_scriptActorOffset;
                g_checkingIfElse = 0;
                continue;
            }

            g_tile[*g_scriptPtr].execPtr = apScript + g_scriptActorOffset;

            if (tw == CON_USERACTOR)
            {
                if (j & 1) g_tile[*g_scriptPtr].flags |= SFLAG_BADGUY;
                if (j & 2) g_tile[*g_scriptPtr].flags |= (SFLAG_BADGUY|SFLAG_BADGUYSTAYPUT);
                if (j & 4) g_tile[*g_scriptPtr].flags |= SFLAG_ROTFIXED;
            }

            for (j=0; j<4; j++)
            {
                scriptWriteAtOffset(0, apScript+g_scriptActorOffset+j);
                if (j == 3)
                {
                    j = 0;
                    while (C_GetKeyword() == -1)
                        C_BitOrNextValue(&j);

                    C_FinishBitOr(j);
                    break;
                }
                else
                {
                    if (C_GetKeyword() != -1)
                    {
                        for (i=4-j; i; i--)
                        {
                            scriptWriteValue(0);
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
                        // XXX: LABEL_MOVE|LABEL_DEFINE, what is this shit? compatibility?
                        // yep, it sure is :(
                        if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (g_scriptPtr[-1] != 0) && (g_scriptPtr[-1] != 1)))
                        {
                            C_ReportError(-1);
                            scriptWriteAtOffset(0, &g_scriptPtr[-1]);
                            initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                            g_warningCnt++;
                        }
                        break;
                    }

                    if (g_scriptPtr[-1] >= (intptr_t)apScript && g_scriptPtr[-1] < (intptr_t)&apScript[g_scriptSize])
                        scriptWritePointer(g_scriptPtr[-1], apScript + g_scriptActorOffset + j);
                    else
                        scriptWriteAtOffset(g_scriptPtr[-1], apScript + g_scriptActorOffset + j);
                }
            }
            g_checkingIfElse = 0;
            continue;

        case CON_ONEVENT:
        case CON_APPENDEVENT:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_scriptEventOffset = g_scriptActorOffset = g_scriptPtr - apScript;

            C_SkipComments();
            j = 0;
            while (isaltok(*(textptr+j)))
            {
                g_szCurrentBlockName[j] = textptr[j];
                j++;
            }
            g_szCurrentBlockName[j] = 0;
            //        g_labelsOnly = 1;
            C_GetNextValue(LABEL_EVENT);
            g_labelsOnly = 0;
            g_scriptPtr--;
            j= *g_scriptPtr;  // type of event
            g_currentEvent = j;
            //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, g_parsingEventPtr);
            //AddLog(g_szBuf);
            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXEVENTS-1))
            {
                initprintf("%s:%d: error: invalid event ID.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                continue;
            }
            // if event has already been declared then store previous script location
            if (!apScriptEvents[j])
            {
                apScriptEvents[j] = g_scriptEventOffset;
            }
            else if (tw == CON_ONEVENT)
            {
                g_scriptEventChainOffset = apScriptEvents[j];
                apScriptEvents[j] = g_scriptEventOffset;
            }
            else // if (tw == CON_APPENDEVENT)
            {
                auto previous_event_end = apScript + apScriptGameEventEnd[j];
                scriptWriteAtOffset(CON_JUMP | LINE_NUMBER, previous_event_end++);
                scriptWriteAtOffset(GV_FLAG_CONSTANT, previous_event_end++);
                C_FillEventBreakStackWithJump((intptr_t *)*previous_event_end, g_scriptEventOffset);
                scriptWriteAtOffset(g_scriptEventOffset, previous_event_end++);
            }

            g_checkingIfElse = 0;

            continue;

        case CON_QSPRINTF:
            C_GetManyVars(2);

            j = 0;

            while (C_GetKeyword() == -1 && j < 32)
                C_GetNextVar(), j++;

            scriptWriteValue(CON_NULLOP | LINE_NUMBER);
            continue;

        case CON_CSTAT:
            C_GetNextValue(LABEL_DEFINE);

            if (EDUKE32_PREDICT_FALSE(g_scriptPtr[-1] == 32767))
            {
                g_scriptPtr[-1] = 32768;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat 32767, using 32768 instead.\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
            else if (EDUKE32_PREDICT_FALSE((g_scriptPtr[-1] & 48) == 48))
            {
                i = g_scriptPtr[-1];
                g_scriptPtr[-1] ^= 48;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat %d, using %d instead.\n",g_scriptFileName,g_lineNumber,i,(int32_t)(g_scriptPtr[-1]));
                g_warningCnt++;
            }
            continue;

        case CON_SCREENPAL:
            C_GetManyVars(4);
            continue;

        case CON_HITRADIUS:
        case CON_DRAWLINE256:
            C_GetManyVars(5);
            continue;

        case CON_DRAWLINERGB:
            C_GetManyVars(6);
            continue;

        case CON_ADDAMMO:
        case CON_ADDINVENTORY:
        case CON_DEBRIS:
        case CON_GUTS:
        case CON_SIZEAT:
        case CON_SIZETO:
            C_GetNextValue(LABEL_DEFINE);
            fallthrough__;
        case CON_ADDKILLS:
        case CON_ADDPHEALTH:
        case CON_ADDSTRENGTH:
        case CON_CACTOR:
        case CON_CLIPDIST:
        case CON_COUNT:
        case CON_CSTATOR:
        case CON_DEBUG:
        case CON_ENDOFGAME:
        case CON_ENDOFLEVEL:
        case CON_LOTSOFGLASS:
        case CON_MAIL:
        case CON_MONEY:
        case CON_PAPER:
        case CON_SAVE:
        case CON_SAVENN:
        case CON_SLEEPTIME:
        case CON_SPAWN:
        case CON_SPRITEPAL:
        case CON_STRENGTH:
            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_QUOTE:
            C_GetNextValue(LABEL_DEFINE);
            if (EDUKE32_PREDICT_FALSE(((unsigned)g_scriptPtr[-1] >= MAXQUOTES) || apStrings[g_scriptPtr[-1]] == NULL))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: invalid quote\n", g_scriptFileName, g_lineNumber);
            }
            continue;

        case CON_ELSE:
            {
                if (EDUKE32_PREDICT_FALSE(!g_checkingIfElse))
                {
                    g_scriptPtr--;
                    auto const tempscrptr = g_scriptPtr;
                    g_warningCnt++;
                    C_ReportError(-1);

                    initprintf("%s:%d: warning: found `else' with no `if'\n", g_scriptFileName, g_lineNumber);

                    if (C_GetKeyword() == CON_LEFTBRACE)
                    {
                        C_GetNextKeyword();
                        g_numBraces++;

                        C_ParseCommand(true);
                    }
                    else C_ParseCommand();

                    g_scriptPtr = tempscrptr;

                    continue;
                }

                intptr_t const lastScriptPtr = &g_scriptPtr[-1] - apScript;

                g_skipBranch = false;
                g_checkingIfElse--;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t const offset = (unsigned) (g_scriptPtr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand();

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = (intptr_t *) apScript+offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                continue;
            }

        case CON_SETSECTOR:
        case CON_GETSECTOR:
            {
                int const labelNum = C_GetStructureIndexes(1, &h_sector);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(SectorLabels[labelNum].lId);

                C_GetNextVarType((tw == CON_GETSECTOR) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_FINDNEARACTOR3D:
        case CON_FINDNEARACTOR:
        case CON_FINDNEARACTORZ:
        case CON_FINDNEARSPRITE3D:
        case CON_FINDNEARSPRITE:
        case CON_FINDNEARSPRITEZ:
            {
                C_GetNextValue(LABEL_DEFINE); // get <type>

                // get the ID of the DEF
                C_GetNextVar();
                switch (tw)
                {
                case CON_FINDNEARACTORZ:
                case CON_FINDNEARSPRITEZ:
                    C_GetNextVar();
                default:
                    break;
                }
                // target var
                // get the ID of the DEF
                C_GetNextVarType(GAMEVAR_READONLY);
                continue;
            }

        case CON_SETWALL:
        case CON_GETWALL:
            {
                int const labelNum = C_GetStructureIndexes(1, &h_wall);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(WallLabels[labelNum].lId);

                C_GetNextVarType((tw == CON_GETWALL) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETPLAYER:
        case CON_GETPLAYER:
            {
                int const labelNum = C_GetStructureIndexes(1, &h_player);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(PlayerLabels[labelNum].lId);

                if (PlayerLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVarType((tw == CON_GETPLAYER) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETINPUT:
        case CON_GETINPUT:
            {
                int const labelNum = C_GetStructureIndexes(1, &h_input);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(InputLabels[labelNum].lId);

                C_GetNextVarType(tw == CON_GETINPUT ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETTILEDATA:
        case CON_GETTILEDATA:
        {
            int const labelNum = C_GetStructureIndexes(0, &h_tiledata);

            if (labelNum == -1)
                continue;

            scriptWriteValue(TileDataLabels[labelNum].lId);

            C_GetNextVarType((tw == CON_GETTILEDATA) ? GAMEVAR_READONLY : 0);
            continue;
        }

        case CON_SETUSERDEF:
        case CON_GETUSERDEF:
            {
                // now get name of .xxx
                while (*textptr != '.')
                {
                    if (*textptr == 0xa || !*textptr)
                        break;

                    textptr++;
                }

                if (EDUKE32_PREDICT_FALSE(*textptr!='.'))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_SYNTAXERROR);
                    continue;
                }
                textptr++;
                C_GetNextLabelName();

                int const labelNum=C_GetLabelNameID(UserdefsLabels,&h_userdef,Bstrtolower(LAST_LABEL));

                if (EDUKE32_PREDICT_FALSE(labelNum == -1))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_NOTAMEMBER);
                    continue;
                }
                scriptWriteValue(labelNum);

                if (UserdefsLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVarType((tw == CON_GETUSERDEF) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETACTOR:
            {
                intptr_t * const ins = &g_scriptPtr[-1];
                int const labelNum = C_GetStructureIndexes(1, &h_actor);

                if (labelNum == -1)
                    continue;

                Bassert((*ins & VM_INSTMASK) == CON_SETACTOR);

                auto const &label = ActorLabels[labelNum];

                if (label.offset != -1 && (label.flags & (LABEL_WRITEFUNC|LABEL_HASPARM2)) == 0)
                {
                    if (labelNum >= ACTOR_SPRITEEXT_BEGIN)
                        *ins = CON_SETSPRITEEXT;
                    else if (labelNum >= ACTOR_STRUCT_BEGIN)
                        *ins = CON_SETACTORSTRUCT;
                    else
                        *ins = CON_SETSPRITESTRUCT;
                }

                scriptWriteValue(label.lId);

                if (label.flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVar();
                continue;
            }

        case CON_GETACTOR:
            {
                intptr_t * const ins = &g_scriptPtr[-1];
                int const labelNum = C_GetStructureIndexes(1, &h_actor);

                if (labelNum == -1)
                    continue;

                Bassert((*ins & VM_INSTMASK) == CON_GETACTOR);

                auto const &label = ActorLabels[labelNum];

                if (label.offset != -1 && (label.flags & (LABEL_READFUNC|LABEL_HASPARM2)) == 0)
                {
                    if (labelNum >= ACTOR_SPRITEEXT_BEGIN)
                        *ins = CON_GETSPRITEEXT;
                    else if (labelNum >= ACTOR_STRUCT_BEGIN)
                        *ins = CON_GETACTORSTRUCT;
                    else
                        *ins = CON_GETSPRITESTRUCT;
                }

                scriptWriteValue(label.lId);

                if (label.flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVarType(GAMEVAR_READONLY);
                continue;
            }

        case CON_GETTSPR:
        case CON_SETTSPR:
            {
#if 0
                if (unlikely(g_currentEvent != EVENT_ANIMATESPRITES))
                {
                    C_ReportError(-1);
                    initprintf("%s:%d: warning: found `%s' outside of EVENT_ANIMATESPRITES\n",g_szScriptFileName,g_lineNumber,tempbuf);
                    g_numCompilerWarnings++;
                }
#endif
                int const labelNum = C_GetStructureIndexes(1, &h_tsprite);

                if (labelNum == -1)
                    continue;

                scriptWriteValue(TsprLabels[labelNum].lId);

                C_GetNextVarType((tw == CON_GETTSPR) ? GAMEVAR_READONLY : 0);
                continue;
            }
        case CON_ADDLOGVAR:
            g_labelsOnly = 1;
            C_GetNextVar();
            g_labelsOnly = 0;
            continue;

        case CON_COS:
        case CON_DIVR:
        case CON_DIVRU:
        case CON_HEADSPRITESECT:
        case CON_HEADSPRITESTAT:
        case CON_NEXTSPRITESECT:
        case CON_NEXTSPRITESTAT:
        case CON_PREVSPRITESECT:
        case CON_PREVSPRITESTAT:
        case CON_QSTRLEN:
        case CON_SECTOROFWALL:
        case CON_SIN:
            C_GetNextVarType(GAMEVAR_READONLY);
            fallthrough__;
        case CON_ACTIVATECHEAT:
        case CON_ANGOFF:
        case CON_CAPIA:
        case CON_CHECKACTIVATORMOTION:
        case CON_CHECKAVAILINVEN:
        case CON_CHECKAVAILWEAPON:
        case CON_CLEARMAPSTATE:
        case CON_CMENU:
        case CON_ECHO:
        case CON_EQSPAWN:
        case CON_ESHOOT:
        case CON_ESPAWN:
        case CON_GLOBALSOUND:
        case CON_GUNIQHUDID:
        case CON_INITTIMER:
        case CON_JUMP:
        case CON_LOCKPLAYER:
        case CON_MOVESECTOR:
        case CON_OPERATEMASTERSWITCHES:
        case CON_OPERATERESPAWNS:
        case CON_QSPAWN:
        case CON_QUAKE:
        case CON_RESETPLAYERFLAGS:
        case CON_SAVEGAMEVAR:
        case CON_SCREENSOUND:
        case CON_SECTCLEARINTERPOLATION:
        case CON_SECTSETINTERPOLATION:
        case CON_SETACTORANGLE:
        case CON_SETGAMEPALETTE:
        case CON_SETMUSICPOSITION:
        case CON_SETPLAYERANGLE:
        case CON_SHOOT:
        case CON_SOUNDONCE:
        case CON_SOUND:
        case CON_STARTCUTSCENE:
        case CON_STARTTRACK:
        case CON_STOPSOUND:
        case CON_TIME:
        case CON_USERQUOTE:
            C_GetNextVar();
            continue;

        case CON_SQRT:
            C_GetNextVar();
            fallthrough__;
        case CON_DISPLAYRAND:
        case CON_FINDOTHERPLAYER:
        case CON_FINDPLAYER:
        case CON_GETACTORANGLE:
        case CON_GETANGLETOTARGET:
        case CON_GETCURRADDRESS:
        case CON_GETMUSICPOSITION:
        case CON_GETPLAYERANGLE:
        case CON_GETTICKS:
        case CON_INV:
        case CON_KLABS:
        case CON_READGAMEVAR:
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_CALCHYPOTENUSE:
        case CON_CLAMP:
        case CON_GETCLOSESTCOL:
            C_GetNextVarType(GAMEVAR_READONLY);
            fallthrough__;
        case CON_ACTORSOUND:
        case CON_CAPIS:
        case CON_CHANGESPRITESECT:
        case CON_CHANGESPRITESTAT:
        case CON_EZSHOOT:
        case CON_GETPNAME:
        case CON_PRELOADTRACKSLOTFORSWAP:
        case CON_QGETSYSSTR:
        case CON_QSTRCAT:
        case CON_QSTRCPY:
        case CON_SPAWNCEILINGGLASS:
        case CON_SPAWNWALLGLASS:
        case CON_SPAWNWALLSTAINEDGLASS:
        case CON_STARTLEVEL:
        case CON_STARTTRACKSLOT:
        case CON_STOPACTORSOUND:
        case CON_SWAPTRACKSLOT:
        case CON_ZSHOOT:
        case CON_GETGAMEFUNCBIND:
            C_GetManyVars(2);
            continue;

        case CON_ENHANCED:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            if (EDUKE32_PREDICT_FALSE(*g_scriptPtr > BYTEVERSION_EDUKE32))
            {
                g_warningCnt++;
                initprintf("%s:%d: warning: need build %d, found build %d\n",g_scriptFileName,g_lineNumber,k,BYTEVERSION_EDUKE32);
            }
            continue;

        case CON_DYNAMICREMAP:
            g_scriptPtr--;
            if (EDUKE32_PREDICT_FALSE(g_dynamicTileMapping))
            {
                initprintf("%s:%d: warning: duplicate dynamicremap statement\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
#ifdef DYNTILEREMAP_ENABLE
#ifdef DEBUGGINGAIDS
                else
                    initprintf("Using dynamic tile remapping\n");
#endif
            g_dynamicTileMapping = 1;
#else
            else
            {
                initprintf("%s:%d: warning: dynamic tile remapping is disabled in this build\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
#endif
            continue;

        case CON_DYNAMICSOUNDREMAP:
            g_scriptPtr--;
            if (EDUKE32_PREDICT_FALSE(g_dynamicSoundMapping))
            {
                initprintf("%s:%d: warning: duplicate dynamicsoundremap statement\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
            else
#ifdef DYNSOUNDREMAP_ENABLE
#ifdef DEBUGGINGAIDS
                initprintf("Using dynamic sound remapping\n");
#endif

            g_dynamicSoundMapping = 1;
#else
            {
                initprintf("%s:%d: warning: dynamic sound remapping is disabled in this build\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
#endif
            continue;

        case CON_ADDVAR:
        case CON_ANDVAR:
        case CON_DISPLAYRANDVAR:
        case CON_DIVVAR:
        case CON_MODVAR:
        case CON_MULVAR:
        case CON_ORVAR:
        case CON_RANDVAR:
        case CON_SETVAR:
        case CON_SHIFTVARL:
        case CON_SHIFTVARR:
        case CON_SUBVAR:
        case CON_XORVAR:
setvar:
        {
            auto ins = &g_scriptPtr[-1];

            C_GetNextVarType(GAMEVAR_READONLY);
            C_GetNextValue(LABEL_DEFINE);

            // replace divides and multiplies by 0 with an error asking if the user is stupid
            if (ins[2] == 0 && (tw == CON_MODVAR || tw == CON_MULVAR || tw == CON_DIVVAR))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: divide or multiply by zero! What are you doing?\n", g_scriptFileName, g_lineNumber);
                continue;
            }
            else if (tw == CON_DIVVAR || tw == CON_MULVAR)
            {
                auto const i = ins[2];
                // replace multiplies or divides by 1 with nullop
                if (i == 1)
                {
                    int constexpr const opcode = CON_NULLOP;

                    if (!g_errorCnt && !g_warningCnt && g_scriptDebug > 1)
                    {
                        initprintf("%s:%d: %s -> %s\n", g_scriptFileName, g_lineNumber,
                                   VM_GetKeywordForID(tw), VM_GetKeywordForID(opcode));
                    }

                    scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                    g_scriptPtr = &ins[1];
                }
                // replace multiplies or divides by -1 with inversion
                else if (i == -1)
                {
                    int constexpr const opcode = CON_INV;

                    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    {
                        initprintf("%s:%d: %s -> %s\n", g_scriptFileName, g_lineNumber,
                                   VM_GetKeywordForID(tw), VM_GetKeywordForID(opcode));
                    }

                    scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                    g_scriptPtr--;
                }
            }
            // replace instructions with special versions for specific var types
            scriptUpdateOpcodeForVariableType(ins);
            continue;
        }

        case CON_ADDVARVAR:
        case CON_ANDVARVAR:
        case CON_DISPLAYRANDVARVAR:
        case CON_DIVVARVAR:
        case CON_MODVARVAR:
        case CON_MULVARVAR:
        case CON_ORVARVAR:
        case CON_RANDVARVAR:
        case CON_SETVARVAR:
        case CON_SHIFTVARVARL:
        case CON_SHIFTVARVARR:
        case CON_SUBVARVAR:
        case CON_XORVARVAR:
            {
setvarvar:
                auto ins = &g_scriptPtr[-1];
                auto tptr = textptr;
                int const lnum = g_lineNumber;

                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextVar();

                int const opcode = inthash_find(&h_varvar, *ins & VM_INSTMASK);

                if (ins[2] == GV_FLAG_CONSTANT && opcode != -1)
                {
                    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    {
                        initprintf("%s:%d: %s -> %s\n", g_scriptFileName, g_lineNumber,
                                    VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                    }

                    tw = opcode;
                    scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                    g_scriptPtr = &ins[1];
                    textptr = tptr;
                    g_lineNumber = lnum;
                    goto setvar;
                }

                continue;
            }

        case CON_GETACTORVAR:
        case CON_GETPLAYERVAR:
        case CON_SETACTORVAR:
        case CON_SETPLAYERVAR:
            {
                // syntax [gs]etactorvar[<var>].<varx> <VAR>
                // gets the value of the per-actor variable varx into VAR

                if (C_GetStructureIndexes(1, NULL) == -1)
                    continue;

                if (g_scriptPtr[-1] == g_thisActorVarID) // convert to "setvarvar"
                {
                    g_scriptPtr--;
                    g_scriptPtr[-1]=CON_SETVARVAR;
                    if (tw == CON_SETACTORVAR || tw == CON_SETPLAYERVAR)
                    {
                        tw = inthash_find(&h_varvar, tw);
                        goto setvarvar;
                    }
                    else
                    {
                        g_scriptPtr++;
                        C_GetNextVar();
                        g_scriptPtr-=2;
                        C_GetNextVarType(GAMEVAR_READONLY);
                        g_scriptPtr++;
                    }
                    continue;
                }

                /// now pointing at 'xxx'

                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i=GetDefID(LAST_LABEL);

                if (EDUKE32_PREDICT_FALSE(i<0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_NOTAGAMEVAR);
                    continue;
                }
                if (EDUKE32_PREDICT_FALSE(aGameVars[i].flags & GAMEVAR_READONLY))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_VARREADONLY);
                    continue;
                }

                switch (tw)
                {
                case CON_SETACTORVAR:
                    if (EDUKE32_PREDICT_FALSE(!(aGameVars[i].flags & GAMEVAR_PERACTOR)))
                    {
                        g_errorCnt++;
                        C_ReportError(-1);
                        initprintf("%s:%d: error: variable `%s' is not per-actor.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                        continue;
                    }
                    break;
                case CON_SETPLAYERVAR:
                    if (EDUKE32_PREDICT_FALSE(!(aGameVars[i].flags & GAMEVAR_PERPLAYER)))
                    {
                        g_errorCnt++;
                        C_ReportError(-1);
                        initprintf("%s:%d: error: variable `%s' is not per-player.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
                        continue;
                    }
                    break;
                }

                scriptWriteValue(i); // the ID of the DEF (offset into array...)

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
                continue;
            }

        case CON_WRITEARRAYTOFILE:
        case CON_READARRAYFROMFILE:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;

            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_COPY:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;

            C_SkipComments();

            if (EDUKE32_PREDICT_FALSE(*textptr != '['))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNO);
                return 1;
            }
            textptr++;
            C_GetNextVar();
            C_SkipComments();
            if (EDUKE32_PREDICT_FALSE(*textptr != ']'))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNC);
                return 1;
            }
            textptr++;
            fallthrough__;
        case CON_SETARRAY:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;

            if (EDUKE32_PREDICT_FALSE(aGameArrays[i].flags & GAMEARRAY_READONLY))
            {
                C_ReportError(ERROR_ARRAYREADONLY);
                g_errorCnt++;
                return 1;
            }

            C_SkipComments();
            if (EDUKE32_PREDICT_FALSE(*textptr != '['))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNO);
                return 1;
            }
            textptr++;
            C_GetNextVar();
            C_SkipComments();
            if (EDUKE32_PREDICT_FALSE(*textptr != ']'))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNC);
                return 1;
            }
            textptr++;
            C_GetNextVar();
            continue;

        case CON_GETARRAYSIZE:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;
            C_SkipComments();
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_RESIZEARRAY:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;

            if (aGameArrays[i].flags & (GAMEARRAY_READONLY|GAMEARRAY_SYSTEM))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: can't resize system array `%s'.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
                return 1;
            }

            C_SkipComments();
            C_GetNextVarType(0);
            continue;

        case CON_SWAPARRAYS:
            i = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(i < 0))
                return 1;

            if (aGameArrays[i].flags & (GAMEARRAY_READONLY|GAMEARRAY_SYSTEM|GAMEARRAY_VARSIZE))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: can't swap system array `%s'.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
                return 1;
            }

            C_SkipComments();

            tw = C_GetNextGameArrayName();
            if (EDUKE32_PREDICT_FALSE(tw < 0))
                return 1;

            if (aGameArrays[tw].flags & (GAMEARRAY_READONLY|GAMEARRAY_SYSTEM|GAMEARRAY_VARSIZE))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: can't swap system array `%s'.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
                return 1;
            }

            if ((aGameArrays[i].flags & GAMEARRAY_STORAGE_MASK) != (aGameArrays[tw].flags & GAMEARRAY_STORAGE_MASK))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: can't swap arrays of different storage classes.\n", g_scriptFileName, g_lineNumber);
                return 1;
            }

            continue;

        case CON_ACTIVATEBYSECTOR:
        case CON_ADDWEAPON:
        case CON_DIST:
        case CON_DIVSCALE:
        case CON_GETANGLE:
        case CON_GETINCANGLE:
        case CON_GMAXAMMO:
        case CON_LDIST:
        case CON_MULSCALE:
        case CON_OPERATEACTIVATORS:
        case CON_OPERATESECTORS:
        case CON_SCALEVAR:
        case CON_SETASPECT:
        case CON_SMAXAMMO:
        case CON_SSP:
            // get the ID of the DEF
            switch (tw)
            {
            case CON_DIST:
            case CON_DIVSCALE:
            case CON_GETANGLE:
            case CON_GETINCANGLE:
            case CON_LDIST:
            case CON_MULSCALE:
            case CON_SCALEVAR:
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
            case CON_GETANGLE:
            case CON_GETINCANGLE:
            case CON_LDIST:
                C_GetNextVar();
                break;
            case CON_DIVSCALE:
            case CON_MULSCALE:
            case CON_SCALEVAR:
                C_GetManyVars(2);
                break;
            }
            continue;

        case CON_FLASH:
        case CON_SAVEMAPSTATE:
        case CON_LOADMAPSTATE:
            if (tw != CON_FLASH)
            {
                if (EDUKE32_PREDICT_FALSE(g_currentEvent == EVENT_ANIMATESPRITES))
                {
                    initprintf("%s:%d: warning: found `%s' inside EVENT_ANIMATESPRITES\n",
                               g_scriptFileName,g_lineNumber,tempbuf);
                    g_warningCnt++;
                }
            }
            continue;

        case CON_ACTIVATE:
            g_scriptPtr[-1] = CON_OPERATEACTIVATORS | LINE_NUMBER;
            C_GetNextValue(LABEL_DEFINE);
            scriptWriteValue(0);
            continue;

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
        case CON_UPDATESECTORZ:
        case CON_UPDATESECTORNEIGHBORZ:
            C_GetManyVars(3);
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_DEFINEPROJECTILE:
            {
                int32_t y, z;

                if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
                {
                    C_ReportError(ERROR_FOUNDWITHIN);
                    g_errorCnt++;
                }

                g_scriptPtr--;

                C_GetNextValue(LABEL_DEFINE);
                j = g_scriptPtr[-1];
                g_scriptPtr--;

                C_GetNextValue(LABEL_DEFINE);
                y = g_scriptPtr[-1];
                g_scriptPtr--;

                C_GetNextValue(LABEL_DEFINE);
                z = g_scriptPtr[-1];
                g_scriptPtr--;

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
                {
                    C_ReportError(ERROR_EXCEEDSMAXTILES);
                    g_errorCnt++;
                    continue;
                }

                C_DefineProjectile(j, y, z);
                continue;
            }

        case CON_DAMAGEEVENTTILE:
        {
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            j = g_scriptPtr[-1];
            g_scriptPtr--;

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                continue;
            }

            g_tile[j].flags |= SFLAG_DAMAGEEVENT;

            continue;
        }

        case CON_DAMAGEEVENTTILERANGE:
        {
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            i = g_scriptPtr[-1];
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            j = g_scriptPtr[-1];
            g_scriptPtr--;

            if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXTILES || (unsigned)j >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                continue;
            }

            for (tiledata_t * t = g_tile + i, * t_end = g_tile + j; t <= t_end; ++t)
                t->flags |= SFLAG_DAMAGEEVENT;

            continue;
        }

        case CON_SPRITEFLAGS:
            if (!g_scriptActorOffset && g_processingState == 0)
            {
                g_scriptPtr--;
                auto tmpscrptr = g_scriptPtr;

                C_GetNextValue(LABEL_DEFINE);
                j = g_scriptPtr[-1];

                int32_t flags = 0;
                do
                    C_BitOrNextValue(&flags);
                while (C_GetKeyword() == -1);

                g_scriptPtr = tmpscrptr;

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
                {
                    C_ReportError(ERROR_EXCEEDSMAXTILES);
                    g_errorCnt++;
                    continue;
                }

                g_tile[j].flags = flags;
            }
            else C_GetNextVar();
            continue;

        case CON_PRECACHE:
        case CON_SPRITENOPAL:
        case CON_SPRITENOSHADE:
        case CON_SPRITENVG:
        case CON_SPRITESHADOW:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_scriptActorOffset))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            switch (tw)
            {
            case CON_SPRITESHADOW:
                g_tile[*g_scriptPtr].flags |= SFLAG_SHADOW;
                break;
            case CON_SPRITENVG:
                g_tile[*g_scriptPtr].flags |= SFLAG_NVG;
                break;
            case CON_SPRITENOSHADE:
                g_tile[*g_scriptPtr].flags |= SFLAG_NOSHADE;
                break;
            case CON_SPRITENOPAL:
                g_tile[*g_scriptPtr].flags |= SFLAG_NOPAL;
                break;
            case CON_PRECACHE:
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
                i = *g_scriptPtr;
                if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXTILES))
                {
                    C_ReportError(ERROR_EXCEEDSMAXTILES);
                    g_errorCnt++;
                    scriptSkipLine();
                    continue;
                }
                g_tile[j].cacherange = i;

                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
                if (*g_scriptPtr)
                    g_tile[j].flags |= SFLAG_CACHE;

                break;
            }
            continue;

        case CON_IFACTORSOUND:
        case CON_IFVARVARA:
        case CON_IFVARVARAE:
        case CON_IFVARVARAND:
        case CON_IFVARVARB:
        case CON_IFVARVARBE:
        case CON_IFVARVARBOTH:
        case CON_IFVARVARE:
        case CON_IFVARVAREITHER:
        case CON_IFVARVARG:
        case CON_IFVARVARGE:
        case CON_IFVARVARL:
        case CON_IFVARVARLE:
        case CON_IFVARVARN:
        case CON_IFVARVAROR:
        case CON_IFVARVARXOR:
        case CON_WHILEVARVARL:
        case CON_WHILEVARVARN:
            {
                auto const ins = &g_scriptPtr[-1];
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;
                auto const lasttextptr = textptr;
                int const lnum = g_lineNumber;

                g_skipBranch = false;

                C_GetNextVar();
                auto const var = g_scriptPtr;
                C_GetNextVar();

                if (*var == GV_FLAG_CONSTANT)
                {
                    int const opcode = inthash_find(&h_varvar, tw);

                    if (opcode != -1)
                    {
                        if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                        {
                            initprintf("%s:%d: replacing %s with %s\n", g_scriptFileName, g_lineNumber,
                                       VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                        }

                        scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                        tw = opcode;
                        g_scriptPtr = &ins[1];
                        textptr = lasttextptr;
                        g_lineNumber = lnum;
                        goto ifvar;
                    }
                }

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                auto const offset = g_scriptPtr - apScript;
                g_scriptPtr++; // Leave a spot for the fail location

                C_ParseCommand();

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                if (tw != CON_WHILEVARVARN && tw != CON_WHILEVARVARL)
                {
                    j = C_GetKeyword();

                    if (j == CON_ELSE)
                        g_checkingIfElse++;
                }
                continue;
            }

        case CON_IFVARA:
        case CON_IFVARAE:
        case CON_IFVARAND:
        case CON_IFVARB:
        case CON_IFVARBE:
        case CON_IFVARBOTH:
        case CON_IFVARE:
        case CON_IFVAREITHER:
        case CON_IFVARG:
        case CON_IFVARGE:
        case CON_IFVARL:
        case CON_IFVARLE:
        case CON_IFVARN:
        case CON_IFVAROR:
        case CON_IFVARXOR:
        case CON_WHILEVARL:
        case CON_WHILEVARN:
            {
ifvar:
                auto const ins = &g_scriptPtr[-1];
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;

                g_skipBranch = false;

                C_GetNextVar();
                C_GetNextValue(LABEL_DEFINE);

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                scriptUpdateOpcodeForVariableType(ins);

                auto const offset = g_scriptPtr - apScript;
                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand();

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                if (tw != CON_WHILEVARN && tw != CON_WHILEVARL)
                {
                    j = C_GetKeyword();

                    if (j == CON_ELSE)
                        g_checkingIfElse++;
                }

                continue;
            }

        case CON_FOR:  // special-purpose iteration
        {
            C_GetNextVarType(GAMEVAR_READONLY);
            C_GetNextLabelName();

            int const iterType = hash_find(&h_iter, LAST_LABEL);

            if (EDUKE32_PREDICT_FALSE(iterType < 0))
            {
                C_CUSTOMERROR("unknown iteration type `%s'.", LAST_LABEL);
                return 1;
            }

            scriptWriteValue(iterType);

            if (iterType >= ITER_SPRITESOFSECTOR)
                C_GetNextVar();

            intptr_t const offset = g_scriptPtr-apScript;
            g_scriptPtr++; //Leave a spot for the location to jump to after completion

            C_ParseCommand();

            // write relative offset
            auto const tscrptr = (intptr_t *) apScript+offset;
            scriptWriteAtOffset((g_scriptPtr-apScript)-offset, tscrptr);
            continue;
        }

        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
            }

            // syntax:
            // int32_t x, int32_t y, int32_t z, short a, short tilenum, int8_t shade, char orientation, x1, y1, x2, y2
            // myospal adds char pal

            // get the ID of the DEFs

            C_GetManyVars(12);
            continue;

        case CON_ROTATESPRITEA:
            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
            }

            C_GetManyVars(13);
            continue;

        case CON_SHOWVIEW:
        case CON_SHOWVIEWUNBIASED:
        case CON_SHOWVIEWQ16:
        case CON_SHOWVIEWQ16UNBIASED:
            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
            }

            C_GetManyVars(10);
            continue;

        case CON_GETZRANGE:
            C_GetManyVars(4);
            C_GetManyVarsType(GAMEVAR_READONLY,4);
            C_GetManyVars(2);
            continue;

        case CON_CLIPMOVE:
        case CON_CLIPMOVENOSLIDE:
            // <retvar>,<x>,<y>,z,<sectnum>, xvect,yvect,walldist,floordist,ceildist,clipmask
            C_GetManyVarsType(GAMEVAR_READONLY,3);
            C_GetNextVar();
            C_GetNextVarType(GAMEVAR_READONLY);
            C_GetManyVars(6);
            continue;

        case CON_LINEINTERSECT:
        case CON_RAYINTERSECT:
            // lineintersect x y z  x y z  x y  x y  <intx> <inty> <intz> <ret>
            // rayintersect x y z  vx vy vz  x y  x y  <intx> <inty> <intz> <ret>
            C_GetManyVars(10);
            C_GetManyVarsType(GAMEVAR_READONLY,4);
            continue;

        case CON_HITSCAN:
        case CON_CANSEE:
            // get the ID of the DEF
            C_GetManyVars(tw==CON_CANSEE?8:7);
            C_GetManyVarsType(GAMEVAR_READONLY,tw==CON_CANSEE?1:6);
            if (tw==CON_HITSCAN) C_GetNextVar();
            continue;

        case CON_CANSEESPR:
        case CON_UPDATESECTOR:
        case CON_UPDATESECTORNEIGHBOR:
        case CON_QSTRCMP:
            C_GetManyVars(2);
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_NEARTAG:
            C_GetManyVars(5);
            C_GetManyVarsType(GAMEVAR_READONLY,4);
            C_GetManyVars(2);
            continue;

        case CON_ROTATEPOINT:
            C_GetManyVars(5);
            C_GetManyVarsType(GAMEVAR_READONLY,2);
            continue;

        case CON_GETTIMEDATE:
            C_GetManyVarsType(GAMEVAR_READONLY,8);
            continue;

        case CON_MOVESPRITE:
            C_GetManyVars(5);
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
        case CON_MINITEXT:
        case CON_SCREENTEXT:
            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
            }

            switch (tw)
            {
            case CON_SCREENTEXT:
                C_GetManyVars(8);
                fallthrough__;
            case CON_GAMETEXTZ:
            case CON_DIGITALNUMBERZ:
                C_GetManyVars(1);
                fallthrough__;
            case CON_GAMETEXT:
            case CON_DIGITALNUMBER:
                C_GetManyVars(6);
                fallthrough__;
            default:
                C_GetManyVars(5);
                break;
            }
            continue;

        case CON_MYOS:
        case CON_MYOSPAL:
        case CON_MYOSX:
        case CON_MYOSPALX:
            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
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
            continue;

        case CON_SWITCH:
            {
                g_checkingSwitch++; // allow nesting (if other things work)
                C_GetNextVar();

                intptr_t const tempoffset = (unsigned)(g_scriptPtr-apScript);

                scriptWriteValue(0); // leave spot for end location (for after processing)
                scriptWriteValue(0); // count of case statements

                auto const backupCaseScriptPtr = g_caseTablePtr;
                g_caseTablePtr=g_scriptPtr;        // the first case's pointer.

                int const backupNumCases = g_numCases;

                scriptWriteValue(0); // leave spot for 'default' location (null if none)

//                temptextptr=textptr;
                // probably does not allow nesting...

                j=C_CountCaseStatements();
                //        initprintf("Done Counting Case Statements for switch %d: found %d.\n", g_checkingSwitch,j);
                g_scriptPtr+=j*2;
                C_SkipComments();
                g_scriptPtr-=j*2; // allocate buffer for the table
                auto tempscrptr = (intptr_t *)(apScript+tempoffset);

                //AddLog(g_szBuf);

                if (j<0)
                {
                    return 1;
                }

                if (tempscrptr)
                {
                    // save count of cases
                    scriptWriteAtOffset(j, &tempscrptr[1]);
                }
                else
                {
                    //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
                    //AddLog(g_szBuf);
                }

                while (j--)
                {
                    // leave room for statements

                    scriptWriteValue(0); // value check
                    scriptWriteValue(0); // code offset
                    C_SkipComments();
                }

                g_numCases=0;
                C_ParseCommand(true);
                tempscrptr = (intptr_t *)(apScript+tempoffset);

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
                    for (i = 3; i < 3 + tempscrptr[1] * 2 - 2; i += 2)  // sort them
                    {
                        intptr_t t = tempscrptr[i];
                        int n = i;

                        for (j = i + 2; j < 3 + tempscrptr[1] * 2; j += 2)
                        {
                            if (tempscrptr[j] < t)
                            {
                                t = tempscrptr[j];
                                n = j;
                            }
                        }

                        if (n != i)
                        {
                            swapptr(&tempscrptr[i],   &tempscrptr[n]);
                            swapptr(&tempscrptr[i+1], &tempscrptr[n+1]);
                        }
                    }
                    //            for (j=3;j<3+tempscrptr[1]*2;j+=2)initprintf("%5d %8x\n",tempscrptr[j],tempscrptr[j+1]);

                    // save 'end' location
                    scriptWriteAtOffset((intptr_t)g_scriptPtr - (intptr_t)apScript, tempscrptr);
                }
                else
                {
                    //Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__);
                    //AddLog(g_szBuf);
                }
                g_caseTablePtr=backupCaseScriptPtr;
                g_numCases=backupNumCases;
                //AddLog("End of Switch statement");
            }
            continue;

        case CON_CASE:
        case CON_DEFAULT:
            {
                if (EDUKE32_PREDICT_FALSE(g_checkingSwitch < 1))
                {
                    g_errorCnt++;
                    C_ReportError(-1);
                    initprintf("%s:%d: error: found `%s' statement when not in switch\n", g_scriptFileName,
                               g_lineNumber, tw == CON_CASE ? "case" : "default");
                    g_scriptPtr--;
                    return 1;
                }

                intptr_t tempoffset = 0;
                intptr_t *tempscrptr = g_scriptPtr;

                g_checkingCase = true;
repeatcase:
                g_scriptPtr--;

                C_SkipComments();

                if (tw == CON_CASE)
                {
                    g_numCases++;
                    C_GetNextValue(LABEL_ANY);
                    j= *(--g_scriptPtr);
                }

                C_SkipComments();

                if (*textptr == ':')
                    textptr++;

                C_SkipComments();

                if (g_caseTablePtr)
                {
                    if (tw == CON_DEFAULT)
                    {
                        if (EDUKE32_PREDICT_FALSE(g_caseTablePtr[0] != 0))
                        {
                            // duplicate default statement
                            g_errorCnt++;
                            C_ReportError(-1);
                            initprintf("%s:%d: error: multiple `default' statements found in switch\n", g_scriptFileName, g_lineNumber);
                        }
                        g_caseTablePtr[0]=(intptr_t) (g_scriptPtr-apScript);   // save offset
                    }
                    else
                    {
                        for (i=(g_numCases/2)-1; i>=0; i--)
                            if (EDUKE32_PREDICT_FALSE(g_caseTablePtr[i*2+1]==j))
                            {
                                g_warningCnt++;
                                C_ReportError(WARNING_DUPLICATECASE);
                                break;
                            }
                        g_caseTablePtr[g_numCases++]=j;
                        g_caseTablePtr[g_numCases]=(intptr_t) ((intptr_t *) g_scriptPtr-apScript);
                    }
                }

                j = C_GetKeyword();

                if (j == CON_CASE || j == CON_DEFAULT)
                {
                    //AddLog("Found Repeat Case");
                    C_GetNextKeyword();    // eat keyword
                    tw = j;
                    goto repeatcase;
                }

                tempoffset = (unsigned)(tempscrptr-apScript);

                while (C_ParseCommand() == 0)
                {
                    j = C_GetKeyword();

                    if (j == CON_CASE || j == CON_DEFAULT)
                    {
                        C_GetNextKeyword();    // eat keyword
                        tempscrptr = (intptr_t *)(apScript+tempoffset);
                        tw = j;
                        goto repeatcase;
                    }
                }

                continue;
            }

        case CON_ENDSWITCH:
            //AddLog("End Switch");
            if (g_caseTablePtr)
            {
                if (EDUKE32_PREDICT_FALSE(g_checkingCase))
                {
                    g_errorCnt++;
                    C_ReportError(-1);
                    initprintf("%s:%d: error: found `endswitch' before `break' or `return'\n", g_scriptFileName, g_lineNumber);
                }
            }

            if (EDUKE32_PREDICT_FALSE(--g_checkingSwitch < 0))
            {
                g_errorCnt++;
                C_ReportError(-1);
                initprintf("%s:%d: error: found `endswitch' without matching `switch'\n", g_scriptFileName, g_lineNumber);
            }
            return 1;      // end of block

        case CON_DRAGPOINT:
        case CON_GETKEYNAME:
        case CON_QSTRNCAT:
        case CON_SETACTORSOUNDPITCH:
            C_GetManyVars(3);
            continue;

        case CON_QSTRDIM:
            C_GetManyVarsType(GAMEVAR_READONLY, 2);
            C_GetManyVars(16);
            continue;

        case CON_QSUBSTR:
        case CON_SETSPRITE:
        case CON_NEXTSECTORNEIGHBORZ:
            C_GetManyVars(4);
            continue;

        case CON_IFACTION:
        case CON_IFACTIONCOUNT:
        case CON_IFACTOR:
        case CON_IFAI:
        case CON_IFANGDIFFL:
        case CON_IFCEILINGDISTL:
        case CON_IFCOUNT:
        case CON_IFCUTSCENE:
        case CON_IFFLOORDISTL:
        case CON_IFGAPZL:
        case CON_IFGOTWEAPONCE:
        case CON_IFMOVE:
        case CON_IFP:
        case CON_IFPDISTG:
        case CON_IFPDISTL:
        case CON_IFPHEALTHL:
        case CON_IFPINVENTORY:
        case CON_IFPLAYERSL:
        case CON_IFRND:
        case CON_IFSOUND:
        case CON_IFSPAWNEDBY:
        case CON_IFSPRITEPAL:
        case CON_IFSTRENGTH:
        case CON_IFWASWEAPON:
            {
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;

                g_skipBranch = false;

                switch (tw)
                {
                case CON_IFCUTSCENE:
                    C_GetNextVar();
                    break;
                case CON_IFAI:
                    C_GetNextValue(LABEL_AI);
                    break;
                case CON_IFACTION:
                    C_GetNextValue(LABEL_ACTION);
                    break;
                case CON_IFMOVE:
                    if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (g_scriptPtr[-1] != 0) && (g_scriptPtr[-1] != 1)))
                    {
                        C_ReportError(-1);
                        g_scriptPtr[-1] = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                        g_warningCnt++;
                    }
                    break;
                case CON_IFPINVENTORY:
                    C_GetNextValue(LABEL_DEFINE);
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                case CON_IFP:
                    j = 0;
                    do
                        C_BitOrNextValue(&j);
                    while (C_GetKeyword() == -1);
                    C_FinishBitOr(j);
                    break;
                case CON_IFSOUND:
                case CON_IFACTORSOUND:
                default:
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                }

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t const offset = (unsigned)(g_scriptPtr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand();

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = (intptr_t *)apScript+offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                j = C_GetKeyword();

                if (j == CON_ELSE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_IFACTORNOTSTAYPUT:
        case CON_IFAWAYFROMWALL:
        case CON_IFBULLETNEAR:
        case CON_IFCANSEE:
        case CON_IFCANSEETARGET:
        case CON_IFCANSHOOTTARGET:
        case CON_IFCLIENT:
        case CON_IFDEAD:
        case CON_IFHITSPACE:
        case CON_IFHITWEAPON:
        case CON_IFINOUTERSPACE:
        case CON_IFINSPACE:
        case CON_IFINWATER:
        case CON_IFMULTIPLAYER:
        case CON_IFNOSOUNDS:
        case CON_IFNOTMOVING:
        case CON_IFONWATER:
        case CON_IFOUTSIDE:
        case CON_IFPLAYBACKON:
        case CON_IFRESPAWN:
        case CON_IFSERVER:
        case CON_IFSQUISHED:
            {
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;

                g_skipBranch = false;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t const offset = (unsigned)(g_scriptPtr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand();

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = (intptr_t *)apScript+offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                j = C_GetKeyword();

                if (j == CON_ELSE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_LEFTBRACE:
            if (EDUKE32_PREDICT_FALSE(!(g_processingState || g_scriptActorOffset || g_scriptEventOffset)))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
            }
            g_numBraces++;

            C_ParseCommand(true);
            continue;

        case CON_RIGHTBRACE:
            g_numBraces--;

            if ((g_scriptPtr[-2]>>12) == (VM_IFELSE_MAGIC) &&
                ((g_scriptPtr[-2] & VM_INSTMASK) == CON_LEFTBRACE)) // rewrite "{ }" into "nullop"
            {
                //            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from right\n",g_szScriptFileName,g_lineNumber);
                g_scriptPtr[-2] = CON_NULLOP | (VM_IFELSE_MAGIC<<12);
                g_scriptPtr -= 2;

                if (C_GetKeyword() != CON_ELSE && (g_scriptPtr[-2] & VM_INSTMASK) != CON_ELSE)
                    g_skipBranch = true;
                else g_skipBranch = false;

                j = C_GetKeyword();

                if (g_checkingIfElse && j != CON_ELSE)
                    g_checkingIfElse--;

                return 1;
            }

            if (EDUKE32_PREDICT_FALSE(g_numBraces < 0))
            {
                if (g_checkingSwitch)
                {
                    C_ReportError(ERROR_NOENDSWITCH);
                }

                C_ReportError(-1);
                initprintf("%s:%d: error: found more `}' than `{'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }

            if (g_checkingIfElse && j != CON_ELSE)
                g_checkingIfElse--;

            return 1;

        case CON_BETANAME:
            g_scriptPtr--;
            j = 0;
            scriptSkipLine();
            continue;


        case CON_UNDEFINELEVEL:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            k = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)k > MAXLEVELS-1))
            {
                initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            C_UndefineLevel(j, k);
            continue;

        case CON_UNDEFINESKILL:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSKILLS))
            {
                initprintf("%s:%d: error: skill number exceeds maximum skill count %d.\n",
                           g_scriptFileName,g_lineNumber, MAXSKILLS);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            C_UndefineSkill(j);
            continue;

        case CON_UNDEFINEVOLUME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            C_UndefineVolume(j);
            continue;

        case CON_DEFINEVOLUMENAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            scriptSkipSpaces();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                g_volumeNames[j][i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(g_volumeNames[j])-1))
                {
                    initprintf("%s:%d: warning: truncating volume name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,(int32_t)sizeof(g_volumeNames[j])-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }
            g_volumeCnt = j+1;
            g_volumeNames[j][i] = '\0';
            continue;

        case CON_DEFINEVOLUMEFLAGS:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            k = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            C_DefineVolumeFlags(j, k);
            continue;

        case CON_DEFINEGAMEFUNCNAME:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            scriptSkipSpaces();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > NUMGAMEFUNCTIONS-1))
            {
                initprintf("%s:%d: error: function number exceeds number of game functions.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            i = 0;

            hash_delete(&h_gamefuncs, gamefunctions[j]);

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                gamefunctions[j][i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(*textptr != 0x0a && *textptr != 0x0d && ispecial(*textptr)))
                {
                    initprintf("%s:%d: warning: invalid character in function name.\n",
                        g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
                if (EDUKE32_PREDICT_FALSE(i >= MAXGAMEFUNCLEN-1))
                {
                    initprintf("%s:%d: warning: truncating function name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,MAXGAMEFUNCLEN);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }
            gamefunctions[j][i] = '\0';
            hash_add(&h_gamefuncs,gamefunctions[j],j,0);

            continue;

        case CON_UNDEFINEGAMEFUNC:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j > NUMGAMEFUNCTIONS-1))
            {
                initprintf("%s:%d: error: function number exceeds number of game functions.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            hash_delete(&h_gamefuncs, gamefunctions[j]);

            gamefunctions[j][0] = '\0';

            continue;

        case CON_DEFINESKILLNAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            scriptSkipSpaces();

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSKILLS))
            {
                initprintf("%s:%d: error: skill number exceeds maximum skill count %d.\n",
                           g_scriptFileName,g_lineNumber, MAXSKILLS);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                g_skillNames[j][i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(g_skillNames[j])-1))
                {
                    initprintf("%s:%d: warning: truncating skill name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,(int32_t)sizeof(g_skillNames[j])-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }

            g_skillNames[j][i] = '\0';

            for (i=0; i<MAXSKILLS; i++)
                if (g_skillNames[i][0] == 0)
                    break;
            g_skillCnt = i;

            continue;

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
                    if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(gamename)-1))
                    {
                        initprintf("%s:%d: warning: truncating game name to %d characters.\n",
                            g_scriptFileName,g_lineNumber,(int32_t)sizeof(gamename)-1);
                        g_warningCnt++;
                        scriptSkipLine();
                        break;
                    }
                }
                gamename[i] = '\0';
                g_gameNamePtr = Xstrdup(gamename);
                G_UpdateAppTitle();
            }
            continue;

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

                C_SetDefName(tempbuf);
            }
            continue;

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

                C_SetCfgName(tempbuf);
            }
            continue;

        case CON_DEFINEGAMETYPE:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            g_gametypeFlags[j] = *g_scriptPtr;

            C_SkipComments();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXGAMETYPES-1))
            {
                initprintf("%s:%d: error: gametype number exceeds maximum gametype count.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }
            g_gametypeCnt = j+1;

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                g_gametypeNames[j][i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(g_gametypeNames[j])-1))
                {
                    initprintf("%s:%d: warning: truncating gametype name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,(int32_t)sizeof(g_gametypeNames[j])-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }
            g_gametypeNames[j][i] = '\0';
            continue;

        case CON_DEFINELEVELNAME:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            k = *g_scriptPtr;
            C_SkipComments();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)k > MAXLEVELS-1))
            {
                initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            i = 0;

            tempbuf[i] = '/';

            while (*textptr != ' ' && *textptr != '\t' && *textptr != 0x0a)
            {
                tempbuf[i+1] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= BMAX_PATH))
                {
                    initprintf("%s:%d: error: level file name exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,BMAX_PATH);
                    g_errorCnt++;
                    scriptSkipSpaces();
                    break;
                }
            }
            tempbuf[i+1] = '\0';

            Bcorrectfilename(tempbuf,0);

            if (g_mapInfo[j *MAXLEVELS+k].filename == NULL)
                g_mapInfo[j *MAXLEVELS+k].filename = (char *)Xcalloc(Bstrlen(tempbuf)+1,sizeof(uint8_t));
            else if ((Bstrlen(tempbuf)+1) > sizeof(g_mapInfo[j*MAXLEVELS+k].filename))
                g_mapInfo[j *MAXLEVELS+k].filename = (char *)Xrealloc(g_mapInfo[j*MAXLEVELS+k].filename,(Bstrlen(tempbuf)+1));

            Bstrcpy(g_mapInfo[j*MAXLEVELS+k].filename,tempbuf);

            C_SkipComments();

            g_mapInfo[j *MAXLEVELS+k].partime =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*REALGAMETICSPERSEC*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*REALGAMETICSPERSEC);

            textptr += 5;
            scriptSkipSpaces();

            // cheap hack, 0.99 doesn't have the 3D Realms time
            if (*(textptr+2) == ':')
            {
                g_mapInfo[j *MAXLEVELS+k].designertime =
                    (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*REALGAMETICSPERSEC*60)+
                    (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*REALGAMETICSPERSEC);

                textptr += 5;
                scriptSkipSpaces();
            }
            else if (g_scriptVersion == 10) g_scriptVersion = 9;

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                tempbuf[i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= 32))
                {
                    initprintf("%s:%d: warning: truncating level name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,32);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }

            tempbuf[i] = '\0';

            if (g_mapInfo[j*MAXLEVELS+k].name == NULL)
                g_mapInfo[j*MAXLEVELS+k].name = (char *)Xcalloc(Bstrlen(tempbuf)+1,sizeof(uint8_t));
            else if ((Bstrlen(tempbuf)+1) > sizeof(g_mapInfo[j*MAXLEVELS+k].name))
                g_mapInfo[j *MAXLEVELS+k].name = (char *)Xrealloc(g_mapInfo[j*MAXLEVELS+k].name,(Bstrlen(tempbuf)+1));

            /*         initprintf("level name string len: %d\n",Bstrlen(tempbuf)); */

            Bstrcpy(g_mapInfo[j*MAXLEVELS+k].name,tempbuf);

            continue;

        case CON_DEFINEQUOTE:
        case CON_REDEFINEQUOTE:
            if (tw == CON_DEFINEQUOTE)
            {
                g_scriptPtr--;
            }

            C_GetNextValue(LABEL_DEFINE);

            k = g_scriptPtr[-1];

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXQUOTES))
            {
                initprintf("%s:%d: error: quote number exceeds limit of %d.\n",g_scriptFileName,g_lineNumber,MAXQUOTES);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }
            else
            {
                C_AllocQuote(k);
            }

            if (tw == CON_DEFINEQUOTE)
                g_scriptPtr--;

            i = 0;

            scriptSkipSpaces();

            if (tw == CON_REDEFINEQUOTE)
            {
                if (apXStrings[g_numXStrings] == NULL)
                    apXStrings[g_numXStrings] = (char *)Xcalloc(MAXQUOTELEN,sizeof(uint8_t));
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
                    *(apStrings[k]+i) = *textptr;
                else
                    *(apXStrings[g_numXStrings]+i) = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= MAXQUOTELEN-1))
                {
                    initprintf("%s:%d: warning: truncating quote text to %d characters.\n",g_scriptFileName,g_lineNumber,MAXQUOTELEN-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }

            if (tw == CON_DEFINEQUOTE)
            {
                if ((unsigned)k < MAXQUOTES)
                    *(apStrings[k]+i) = '\0';
            }
            else
            {
                *(apXStrings[g_numXStrings]+i) = '\0';
                scriptWriteValue(g_numXStrings++);
            }
            continue;

        case CON_DEFINECHEATDESCRIPTION:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);

            k = g_scriptPtr[-1];

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= NUMCHEATS))
            {
                initprintf("%s:%d: error: cheat number exceeds limit of %d.\n",g_scriptFileName,g_lineNumber,NUMCHEATS);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            g_scriptPtr--;

            i = 0;

            scriptSkipSpaces();

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                *(CheatDescriptions[k]+i) = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= MAXCHEATDESC-1))
                {
                    initprintf("%s:%d: warning: truncating cheat text to %d characters.\n",g_scriptFileName,g_lineNumber,MAXCHEATDESC-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }

            *(CheatDescriptions[k]+i) = '\0';
            continue;

        case CON_CHEATKEYS:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            CheatKeys[0] = g_scriptPtr[-1];
            C_GetNextValue(LABEL_DEFINE);
            CheatKeys[1] = g_scriptPtr[-1];
            g_scriptPtr -= 2;
            continue;

        case CON_UNDEFINECHEAT:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= NUMCHEATS))
            {
                initprintf("%s:%d: error: cheat undefinition attempts to undefine nonexistent cheat.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }

            CheatStrings[j][0] = '\0';
            continue;

        case CON_DEFINECHEAT:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            k = g_scriptPtr[-1];

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= NUMCHEATS))
            {
                initprintf("%s:%d: error: cheat redefinition attempts to redefine nonexistent cheat.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                scriptSkipLine();
                continue;
            }
            g_scriptPtr--;
            i = 0;
            scriptSkipSpaces();
            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ')
            {
                CheatStrings[k][i] = Btolower(*textptr);
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(CheatStrings[k])-1))
                {
                    initprintf("%s:%d: warning: truncating cheat string to %d characters.\n",
                        g_scriptFileName,g_lineNumber,(signed)sizeof(CheatStrings[k])-1);
                    g_warningCnt++;
                    scriptSkipLine();
                    break;
                }
            }
            CheatStrings[k][i] = '\0';
            continue;

        case CON_DEFINESOUND:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);

            // Ideally we could keep the value of i from C_GetNextValue() instead of having to hash_find() again.
            // This depends on tempbuf remaining in place after C_GetNextValue():
            j = hash_find(&h_labels,tempbuf);

            k = g_scriptPtr[-1];
            if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXSOUNDS-1))
            {
                initprintf("%s:%d: error: sound index exceeds limit of %d.\n",g_scriptFileName,g_lineNumber, MAXSOUNDS-1);
                g_errorCnt++;
                k = MAXSOUNDS-1;
            }
            else if (EDUKE32_PREDICT_FALSE(g_sounds[k].filename != NULL))
            {
                initprintf("%s:%d: warning: sound %d already defined (%s)\n",g_scriptFileName,g_lineNumber,k,g_sounds[k].filename);
                g_warningCnt++;
            }

            g_scriptPtr--;
            i = 0;
            C_SkipComments();

            if (g_sounds[k].filename == NULL)
                g_sounds[k].filename = (char *)Xcalloc(BMAX_PATH,sizeof(uint8_t));

            if (*textptr == '\"')
            {
                textptr++;
                while (*textptr && *textptr != '\"')
                {
                    g_sounds[k].filename[i++] = *textptr++;
                    if (EDUKE32_PREDICT_FALSE(i >= BMAX_PATH-1))
                    {
                        initprintf("%s:%d: error: sound filename exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,BMAX_PATH-1);
                        g_errorCnt++;
                        C_SkipComments();
                        break;
                    }
                }
                textptr++;
            }
            else while (*textptr != ' ' && *textptr != '\t' && *textptr != '\r' && *textptr != '\n')
            {
                g_sounds[k].filename[i++] = *textptr++;
                if (EDUKE32_PREDICT_FALSE(i >= BMAX_PATH-1))
                {
                    initprintf("%s:%d: error: sound filename exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,BMAX_PATH-1);
                    g_errorCnt++;
                    C_SkipComments();
                    break;
                }
            }
            g_sounds[k].filename[i] = '\0';

            check_filename_case(g_sounds[k].filename);

            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].ps = g_scriptPtr[-1];
            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].pe = g_scriptPtr[-1];
            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].pr = g_scriptPtr[-1];

            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].m = g_scriptPtr[-1] & ~SF_ONEINST_INTERNAL;
            if (g_scriptPtr[-1] & SF_LOOP)
                g_sounds[k].m |= SF_ONEINST_INTERNAL;

            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].vo = g_scriptPtr[-1];
            g_scriptPtr -= 5;

            g_sounds[k].volume = 1.f;

            if (k > g_highestSoundIdx)
                g_highestSoundIdx = k;

            if (g_dynamicSoundMapping && j >= 0 && (labeltype[j] & LABEL_DEFINE))
                G_ProcessDynamicSoundMapping(label+(j<<6), k);
            continue;

        case CON_ENDEVENT:

            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `endevent' without open `onevent'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
            {
                C_ReportError(ERROR_NOTTOPLEVEL);
                g_errorCnt++;
            }
            // if event has already been declared then put a jump in instead
            if (g_scriptEventChainOffset)
            {
                g_scriptPtr--;
                scriptWriteValue(CON_JUMP | LINE_NUMBER);
                scriptWriteValue(GV_FLAG_CONSTANT);
                scriptWriteValue(g_scriptEventChainOffset);
                scriptWriteValue(CON_ENDEVENT | LINE_NUMBER);

                C_FillEventBreakStackWithJump((intptr_t *)g_scriptEventBreakOffset, g_scriptEventChainOffset);

                g_scriptEventChainOffset = 0;
            }
            else
            {
                // pad space for the next potential appendevent
                apScriptGameEventEnd[g_currentEvent] = &g_scriptPtr[-1] - apScript;
                scriptWriteValue(CON_ENDEVENT | LINE_NUMBER);
                scriptWriteValue(g_scriptEventBreakOffset);
                scriptWriteValue(CON_ENDEVENT | LINE_NUMBER);
            }

            g_scriptEventBreakOffset = g_scriptEventOffset = g_scriptActorOffset = 0;
            g_currentEvent = -1;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_ENDA:
            if (EDUKE32_PREDICT_FALSE(!g_scriptActorOffset || g_scriptEventOffset))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `enda' without open `actor'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                g_scriptEventOffset = 0;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
            {
                C_ReportError(ERROR_NOTTOPLEVEL);
                g_errorCnt++;
            }
            g_scriptActorOffset = 0;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_RETURN:
            if (g_checkingSwitch)
            {
                g_checkingCase = false;
                return 1;
            }
            continue;

        case CON_BREAK:
            if (g_checkingSwitch)
            {
                if (EDUKE32_PREDICT_FALSE(otw == CON_BREAK))
                {
                    C_ReportError(-1);
                    initprintf("%s:%d: warning: duplicate `break'.\n",g_scriptFileName, g_lineNumber);
                    g_warningCnt++;
                    g_scriptPtr--;
                    continue;
                }

                g_checkingCase = false;
                return 1;
            }
            else if (g_scriptEventOffset)
            {
                g_scriptPtr--;
                scriptWriteValue(CON_JUMP | LINE_NUMBER);
                scriptWriteValue(GV_FLAG_CONSTANT);
                scriptWriteValue(g_scriptEventBreakOffset);
                g_scriptEventBreakOffset = &g_scriptPtr[-1] - apScript;
            }
            continue;

        case CON_SCRIPTSIZE:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            j = g_scriptPtr[-1];
            g_scriptPtr--;
            C_SkipComments();
            C_SetScriptSize(j);
            continue;

        case CON_SHADETO:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            continue;

        case CON_FALL:
        case CON_GETLASTPAL:
        case CON_GETTEXTURECEILING:
        case CON_GETTEXTUREFLOOR:
        case CON_INSERTSPRITEQ:
        case CON_KILLIT:
        case CON_MIKESND:
        case CON_OPERATE:
        case CON_PKICK:
        case CON_PSTOMP:
        case CON_RESETACTIONCOUNT:
        case CON_RESETCOUNT:
        case CON_RESETPLAYER:
        case CON_RESPAWNHITAG:
        case CON_SECTGETHITAG:
        case CON_SECTGETLOTAG:
        case CON_SPGETHITAG:
        case CON_SPGETLOTAG:
        case CON_STARTSCREEN:
        case CON_STOPALLMUSIC:
        case CON_STOPALLSOUNDS:
        case CON_TIP:
        case CON_TOSSWEAPON:
        case CON_WACKPLAYER:
            continue;

        case CON_NULLOP:
            if (EDUKE32_PREDICT_FALSE(C_GetKeyword() != CON_ELSE))
            {
                C_ReportError(-1);
                g_warningCnt++;
                initprintf("%s:%d: warning: `nullop' found without `else'\n",g_scriptFileName,g_lineNumber);
                g_scriptPtr--;
                g_skipBranch = true;
            }
            continue;

        case CON_GAMESTARTUP:
            {
                int32_t params[31];

                g_scriptPtr--;
                for (j = 0; j < 31; j++)
                {
                    C_GetNextValue(LABEL_DEFINE);
                    g_scriptPtr--;
                    params[j] = *g_scriptPtr;

                    if (j != 12 && j != 21 && j != 25 && j != 29) continue;

                    if (C_GetKeyword() != -1)
                    {
                        if (j == 12)
                            g_scriptVersion = 10;
                        else if (j == 21)
                            g_scriptVersion = 11;
                        else if (j == 25)
                            g_scriptVersion = 13;
                        else if (j == 29)
                            g_scriptVersion = 14;
                        break;
                    }
                    else
                        g_scriptVersion = 16;
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

                G_DoGameStartup(params);
            }
            continue;
        }
    }
    while (loop);

    return 0;
}

/* Anything added with C_AddDefinition() cannot be overwritten in the CONs */
static void C_AddDefinition(const char *lLabel,int32_t lValue,int32_t lType)
{
    Bstrcpy(LAST_LABEL,lLabel);
    labeltype[g_labelCnt] = lType;
    hash_add(&h_labels,LAST_LABEL,g_labelCnt,0);
    labelcode[g_labelCnt++] = lValue;
}

// KEEPINSYNC lunatic/con_lang.lua
static void C_AddDefaultDefinitions(void)
{
    for (int i=0; i<MAXEVENTS; i++)
        C_AddDefinition(EventNames[i], i, LABEL_DEFINE|LABEL_EVENT);

#if 0
    for (int i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        int32_t j;

        if (gamefunctions[i][0] == '\0')
            continue;

        // if (!Bstrcmp(gamefunctions[i],"Show_Console")) continue;

        j = Bsprintf(tempbuf,"GAMEFUNC_%s", gamefunctions[i]);

        for (; j>=0; j--)
            tempbuf[j] = Btoupper(tempbuf[j]);

        C_AddDefinition(tempbuf, i, LABEL_DEFINE);
    }
#endif

    static tokenmap_t predefined[] =
    {
        { "CLIPMASK0",         CLIPMASK0 },
        { "CLIPMASK1",         CLIPMASK1 },

        { "GAMEARRAY_BOOLEAN", GAMEARRAY_BITMAP },
        { "GAMEARRAY_INT16",   GAMEARRAY_INT16 },
        { "GAMEARRAY_INT8",    GAMEARRAY_INT8 },
        { "GAMEARRAY_RESTORE", GAMEARRAY_RESTORE },
        { "GAMEARRAY_UINT16",  GAMEARRAY_UINT16 },
        { "GAMEARRAY_UINT8",   GAMEARRAY_UINT8 },

        { "GAMEVAR_NODEFAULT", GAMEVAR_NODEFAULT },
        { "GAMEVAR_NOMULTI",   GAMEVAR_NOMULTI },
        { "GAMEVAR_NORESET",   GAMEVAR_NORESET },
        { "GAMEVAR_PERACTOR",  GAMEVAR_PERACTOR },
        { "GAMEVAR_PERPLAYER", GAMEVAR_PERPLAYER },
        { "GAMEVAR_SERIALIZE", GAMEVAR_SERIALIZE },

        { "MAX_WEAPONS",        MAX_WEAPONS },
        { "MAXSPRITES",         MAXSPRITES },
        { "MAXSPRITESONSCREEN", MAXSPRITESONSCREEN },
        { "MAXSTATUS",          MAXSTATUS },
        { "MAXTILES",           MAXTILES },

        { "PROJ_BOUNCES",     PROJ_BOUNCES },
        { "PROJ_BSOUND",      PROJ_BSOUND },
        { "PROJ_CLIPDIST",    PROJ_CLIPDIST },
        { "PROJ_CSTAT",       PROJ_CSTAT },
        { "PROJ_DECAL",       PROJ_DECAL },
        { "PROJ_DROP",        PROJ_DROP },
        { "PROJ_EXTRA",       PROJ_EXTRA },
        { "PROJ_EXTRA_RAND",  PROJ_EXTRA_RAND },
        { "PROJ_FLASH_COLOR", PROJ_FLASH_COLOR },
        { "PROJ_HITRADIUS",   PROJ_HITRADIUS },
        { "PROJ_ISOUND",      PROJ_ISOUND },
        { "PROJ_OFFSET",      PROJ_OFFSET },
        { "PROJ_PAL",         PROJ_PAL },
        { "PROJ_RANGE",       PROJ_RANGE },
        { "PROJ_SHADE",       PROJ_SHADE },
        { "PROJ_SOUND",       PROJ_SOUND },
        { "PROJ_SPAWNS",      PROJ_SPAWNS },
        { "PROJ_SXREPEAT",    PROJ_SXREPEAT },
        { "PROJ_SYREPEAT",    PROJ_SYREPEAT },
        { "PROJ_TNUM",        PROJ_TNUM },
        { "PROJ_TOFFSET",     PROJ_TOFFSET },
        { "PROJ_TRAIL",       PROJ_TRAIL },
        { "PROJ_TXREPEAT",    PROJ_TXREPEAT },
        { "PROJ_TYREPEAT",    PROJ_TYREPEAT },
        { "PROJ_USERDATA",    PROJ_USERDATA },
        { "PROJ_VEL",         PROJ_VEL },
        { "PROJ_VEL_MULT",    PROJ_MOVECNT },
        { "PROJ_WORKSLIKE",   PROJ_WORKSLIKE },
        { "PROJ_XREPEAT",     PROJ_XREPEAT },
        { "PROJ_YREPEAT",     PROJ_YREPEAT },

        { "SFLAG_BADGUY",          SFLAG_BADGUY },
        { "SFLAG_DAMAGEEVENT",     SFLAG_DAMAGEEVENT },
        { "SFLAG_GREENSLIMEFOOD",  SFLAG_GREENSLIMEFOOD },
        { "SFLAG_HURTSPAWNBLOOD",  SFLAG_HURTSPAWNBLOOD },
        { "SFLAG_NOCLIP",          SFLAG_NOCLIP },
        { "SFLAG_NODAMAGEPUSH",    SFLAG_NODAMAGEPUSH },
        { "SFLAG_NOEVENTS",        SFLAG_NOEVENTCODE },
        { "SFLAG_NOLIGHT",         SFLAG_NOLIGHT },
        { "SFLAG_NOPAL",           SFLAG_NOPAL },
        { "SFLAG_NOSHADE",         SFLAG_NOSHADE },
        { "SFLAG_NOTELEPORT",      SFLAG_NOTELEPORT },
        { "SFLAG_NOWATERDIP",      SFLAG_NOWATERDIP },
        { "SFLAG_NVG",             SFLAG_NVG },
        { "SFLAG_REALCLIPDIST",    SFLAG_REALCLIPDIST },
        { "SFLAG_SHADOW",          SFLAG_SHADOW },
        { "SFLAG_SMOOTHMOVE",      SFLAG_SMOOTHMOVE },
        { "SFLAG_USEACTIVATOR",    SFLAG_USEACTIVATOR },
        { "SFLAG_WAKEUPBADGUYS",   SFLAG_WAKEUPBADGUYS },
        { "SFLAG_NOWATERSECTOR",   SFLAG_NOWATERSECTOR },
        { "SFLAG_QUEUEDFORDELETE", SFLAG_QUEUEDFORDELETE },

        { "STAT_ACTIVATOR",   STAT_ACTIVATOR },
        { "STAT_ACTOR",       STAT_ACTOR },
        { "STAT_DEFAULT",     STAT_DEFAULT },
        { "STAT_DUMMYPLAYER", STAT_DUMMYPLAYER },
        { "STAT_EFFECTOR",    STAT_EFFECTOR },
        { "STAT_FALLER",      STAT_FALLER },
        { "STAT_FX",          STAT_FX },
        { "STAT_LIGHT",       STAT_LIGHT },
        { "STAT_LOCATOR",     STAT_LOCATOR },
        { "STAT_MISC",        STAT_MISC },
        { "STAT_PLAYER",      STAT_PLAYER },
        { "STAT_PROJECTILE",  STAT_PROJECTILE },
        { "STAT_STANDABLE",   STAT_STANDABLE },
        { "STAT_TRANSPORT",   STAT_TRANSPORT },
        { "STAT_ZOMBIEACTOR", STAT_ZOMBIEACTOR },

        { "STR_BESTTIME",        STR_BESTTIME },
        { "STR_DESIGNERTIME",    STR_DESIGNERTIME },
        { "STR_GAMETYPE",        STR_GAMETYPE },
        { "STR_MAPFILENAME",     STR_MAPFILENAME },
        { "STR_MAPNAME",         STR_MAPNAME },
        { "STR_PARTIME",         STR_PARTIME },
        { "STR_PLAYERNAME",      STR_PLAYERNAME },
        { "STR_USERMAPFILENAME", STR_USERMAPFILENAME },
        { "STR_VERSION",         STR_VERSION },
        { "STR_VOLUMENAME",      STR_VOLUMENAME },
        { "STR_YOURTIME",        STR_YOURTIME },
    };

    for (auto & def : predefined)
        C_AddDefinition(def.token, def.val, LABEL_DEFINE);

    C_AddDefinition("NO", 0, LABEL_DEFINE | LABEL_ACTION | LABEL_AI | LABEL_MOVE);
}
#endif

void C_InitProjectiles(void)
{
    defaultprojectile_t const Projectile =
    {
        // workslike, cstat, hitradius, range, flashcolor;
        // spawns, sound, isound, vel, decal, trail, tnum, drop;
        // offset, bounces, bsound, toffset, extra, extra_rand;
        // sxrepeat, syrepeat, txrepeat, tyrepeat;
        // shade, xrepeat, yrepeat, pal;
        // movecnt, clipdist, filler[2], userdata;

        // XXX: The default projectie seems to mimic a union of hard-coded ones.

        1, -1, 2048, 0, 0,
        (int16_t)SMALLSMOKE, -1, -1, 600, (int16_t)BULLETHOLE, -1, 0, 0,
        448, (int16_t)g_numFreezeBounces, (int16_t)PIPEBOMB_BOUNCE, 1, 100, -1,
        -1, -1, -1, -1,
        -96, 18, 18, 0,
        1, 32, { 0, 0 }, 0,
    };

    DefaultProjectile = Projectile;

    for (auto & tile : g_tile)
    {
        if (tile.proj)
            *tile.proj = DefaultProjectile;

        if (tile.defproj)
            *tile.defproj = DefaultProjectile;
    }
}

#if !defined LUNATIC
static char const * C_ScriptVersionString(int32_t version)
{
#ifdef EDUKE32_STANDALONE
    UNREFERENCED_PARAMETER(version);
#else
    switch (version)
    {
    case 9:
        return ", v0.99 compatibility mode";
    case 10:
        return ", v1.0 compatibility mode";
    case 11:
        return ", v1.1 compatibility mode";
    case 13:
        return ", v1.3D compatibility mode";
    }
#endif
    return "";
}

void C_PrintStats(void)
{
    initprintf("%d/%d labels, %d/%d variables, %d/%d arrays\n", g_labelCnt,
        (int32_t) min((MAXSECTORS * sizeof(sectortype)/sizeof(int32_t)),
            MAXSPRITES * sizeof(spritetype)/(1<<6)),
        g_gameVarCount, MAXGAMEVARS, g_gameArrayCount, MAXGAMEARRAYS);

    int cnt = g_numXStrings;

    for (auto &ptr : apStrings)
        if (ptr)
            cnt++;

    if (cnt) initprintf("%d strings, ", cnt);
    cnt = 0;

    for (auto & apScriptEvent : apScriptEvents)
        if (apScriptEvent)
            cnt++;

    if (cnt) initprintf("%d events, ", cnt);
    cnt = 0;

    for (auto & tile : g_tile)
        if (tile.execPtr)
            cnt++;

    if (cnt) initprintf("%d actors", cnt);
    initprintf("\n");
}

// TODO: add some kind of mapping between the table and the struct holding the tokens
void scriptInitTables()
{
    for (auto table : tables)
        hash_init(table);

    for (auto table : inttables)
        inthash_init(table);

    for (auto &keyword : vm_keywords)
        hash_add(&h_keywords, keyword.token, keyword.val, 0);

    for (auto &iter_token : iter_tokens)
        hash_add(&h_iter, iter_token.token, iter_token.val, 0);

    for (auto &varvar : varvartable)
        inthash_add(&h_varvar, varvar.x, varvar.y, 0);

#ifdef CON_DISCRETE_VAR_ACCESS
    for (auto &globalvar : globalvartable)
        inthash_add(&h_globalvar, globalvar.x, globalvar.y, 0);

    for (auto &playervar : playervartable)
        inthash_add(&h_playervar, playervar.x, playervar.y, 0);

    for (auto &actorvar : actorvartable)
        inthash_add(&h_actorvar, actorvar.x, actorvar.y, 0);
#endif
}

void C_Compile(const char *fileName)
{
    Bmemset(apScriptEvents, 0, sizeof(apScriptEvents));
    Bmemset(apScriptGameEventEnd, 0, sizeof(apScriptGameEventEnd));

    for (auto & i : g_tile)
        Bmemset(&i, 0, sizeof(tiledata_t));

    for (double & actorMinMs : g_actorMinMs)
        actorMinMs = 1e308;

    scriptInitTables();
    scriptInitStructTables();

    Gv_Init();
    C_InitProjectiles();

    buildvfs_kfd kFile = kopen4loadfrommod(fileName, g_loadFromGroupOnly);

    if (kFile == buildvfs_kfd_invalid) // JBF: was 0
    {
        if (g_loadFromGroupOnly == 1 || numgroupfiles == 0)
        {
#ifndef EDUKE32_STANDALONE
            char const *gf = G_GrpFile();
            Bsprintf(tempbuf,"Required game data was not found.  A valid copy of \"%s\" or other compatible data is needed to run EDuke32.\n\n"
                     "You must copy \"%s\" to your game directory before continuing!", gf, gf);
            G_GameExit(tempbuf);
#else
            G_GameExit(" ");
#endif
        }
        else
        {
            Bsprintf(tempbuf,"CON file `%s' missing.", fileName);
            G_GameExit(tempbuf);
        }

        //g_loadFromGroupOnly = 1;
        return; //Not there
    }

    int const kFileLen = kfilelength(kFile);

    initprintf("Compiling: %s (%d bytes)\n", fileName, kFileLen);

    g_logFlushWindow = 0;

    uint32_t const startcompiletime = timerGetTicks();

    char * mptr = (char *)Xmalloc(kFileLen+1);
    mptr[kFileLen] = 0;

    textptr = (char *)mptr;
    kread(kFile, (char *)textptr, kFileLen);
    kclose(kFile);

    g_scriptcrc = Bcrc32(NULL, 0, 0L);
    g_scriptcrc = Bcrc32(textptr, kFileLen, g_scriptcrc);

    Xfree(apScript);

    apScript = (intptr_t *)Xcalloc(1, g_scriptSize * sizeof(intptr_t));
    bitptr   = (uint8_t *)Xcalloc(1, (((g_scriptSize + 7) >> 3) + 1) * sizeof(uint8_t));

    g_errorCnt   = 0;
    g_labelCnt   = 0;
    g_lineNumber = 1;
    g_scriptPtr  = apScript + 3;  // move permits constants 0 and 1; moveptr[1] would be script[2] (reachable?)
    g_totalLines = 0;
    g_warningCnt = 0;

    Bstrcpy(g_scriptFileName, fileName);

    C_AddDefaultDefinitions();
    C_ParseCommand(true);

    for (char * m : g_scriptModules)
    {
        C_Include(m);
        free(m);
    }
    g_scriptModules.clear();

    g_logFlushWindow = 1;

    if (g_errorCnt > 63)
        initprintf("fatal error: too many errors: Aborted\n");

    //*script = (intptr_t) g_scriptPtr;

    DO_FREE_AND_NULL(mptr);

    if (g_warningCnt || g_errorCnt)
    {
        initprintf("Found %d warning(s), %d error(s).\n", g_warningCnt, g_errorCnt);

        if (g_errorCnt)
        {
            Bsprintf(buf, "Error compiling CON files.");
            G_GameExit(buf);
        }
    }

    for (intptr_t i : apScriptGameEventEnd)
    {
        if (!i)
            continue;

        auto const eventEnd = apScript + i;
        auto breakPtr = (intptr_t*)*(eventEnd + 2);

        while (breakPtr)
        {
            breakPtr = apScript + (intptr_t)breakPtr;
            scriptWriteAtOffset(CON_ENDEVENT | LINE_NUMBER, breakPtr-2);
            breakPtr = (intptr_t*)*breakPtr;
        }
    }

    g_totalLines += g_lineNumber;

    C_SetScriptSize(g_scriptPtr-apScript+8);

    initprintf("Compiled %d bytes in %ums%s\n", (int)((intptr_t)g_scriptPtr - (intptr_t)apScript),
               timerGetTicks() - startcompiletime, C_ScriptVersionString(g_scriptVersion));

    for (auto i : tables_free)
        hash_free(i);

    for (auto i : inttables)
        inthash_free(i);

    freehashnames();
    freesoundhashnames();

    if (g_scriptDebug)
        C_PrintStats();

    C_InitQuotes();
}

void C_ReportError(int error)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (g_scriptEventOffset || g_processingState || g_scriptActorOffset)
            initprintf("%s: In %s `%s':\n",g_scriptFileName,g_scriptEventOffset?"event":g_scriptActorOffset?"actor":"state",g_szCurrentBlockName);
        else initprintf("%s: At top level:\n",g_scriptFileName);
        Bstrcpy(g_szLastBlockName,g_szCurrentBlockName);
    }
    switch (error)
    {
    case ERROR_NOTTOPLEVEL:
        initprintf("%s:%d: error: `%s' not at top level within script.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_EVENTONLY:
        initprintf("%s:%d: error: keyword `%s' only available during events.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        initprintf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",g_scriptFileName,g_lineNumber,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%d: error: expected a keyword but found `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%d: error: found `%s' within %s.\n",g_scriptFileName,g_lineNumber,tempbuf,g_scriptEventOffset?"an event":g_scriptActorOffset?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%d: error: did not find `endswitch' before `%s'.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%d: error: symbol `%s' is not a definition.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a variable.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_NOTAGAMEARRAY:
        initprintf("%s:%d: error: symbol `%s' is not an array.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_GAMEARRAYBNC:
        initprintf("%s:%d: error: malformed array index: expected ], found %c\n",g_scriptFileName,g_lineNumber,*textptr);
        break;
    case ERROR_GAMEARRAYBNO:
        initprintf("%s:%d: error: malformed array index: expected [, found %c\n",g_scriptFileName,g_lineNumber,*textptr);
        break;
    case ERROR_INVALIDARRAYWRITE:
        initprintf("%s:%d: error: arrays can only be written to using `setarray'.\n",g_scriptFileName,g_lineNumber);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_NOTAMEMBER:
        initprintf("%s:%d: error: symbol `%s' is not a valid structure member.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",g_scriptFileName,g_lineNumber);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_ARRAYREADONLY:
        initprintf("%s:%d: error: array `%s' is read-only.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_BADGAMEVAR:
        initprintf("%s:%d: warning: variable `%s' should be either per-player OR per-actor, not both.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_DUPLICATECASE:
        initprintf("%s:%d: warning: duplicate case ignored.\n",g_scriptFileName,g_lineNumber);
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate definition `%s' ignored.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_EVENTSYNC:
        initprintf("%s:%d: warning: found `%s' within a local event.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",g_scriptFileName,g_lineNumber);
        break;
    case WARNING_NAMEMATCHESVAR:
        initprintf("%s:%d: warning: symbol `%s' already used for variable.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_VARMASKSKEYWORD:
        initprintf("%s:%d: warning: variable `%s' masks keyword.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
        break;
    case WARNING_ARRAYMASKSKEYWORD:
        initprintf("%s:%d: warning: array `%s' masks keyword.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
        break;
    }
}
#endif
