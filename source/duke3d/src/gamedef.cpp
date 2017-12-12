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

#include "duke3d.h"
#include "namesdyn.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "common.h"
#include "common_game.h"
#include "cheats.h"

#include "osd.h"

int32_t g_scriptVersion = 13; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files

char g_scriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling

int32_t g_totalLines,g_lineNumber;
char g_szBuf[1024];

#if !defined LUNATIC
static char g_szCurrentBlockName[256] = "(none)", g_szLastBlockName[256] = "NULL";
static int32_t g_checkingIfElse, g_processingState, g_lastKeyword = -1;

// The pointer to the start of the case table in a switch statement.
// First entry is 'default' code.
static intptr_t *g_caseScriptPtr;
static intptr_t previous_event;
static int32_t g_numCases = 0, g_checkingCase = 0;
static int32_t g_checkingSwitch = 0, g_currentEvent = -1;
static int32_t g_labelsOnly = 0, g_skipKeywordCheck = 0, g_dynamicTileMapping = 0, g_dynamicSoundMapping = 0;
static int32_t g_numBraces = 0;

static int32_t C_ParseCommand(int32_t loop);
static int32_t C_SetScriptSize(int32_t size);
#endif

int32_t g_numXStrings = 0;

#ifdef LUNATIC
weapondata_t g_playerWeapon[MAXPLAYERS][MAX_WEAPONS];
#endif

#if !defined LUNATIC
static intptr_t g_parsingActorPtr;
static intptr_t g_parsingEventPtr;
static intptr_t g_parsingEventBreakPtr;
static intptr_t apScriptGameEventEnd[MAXEVENTS];
static char *textptr;
#endif

int32_t g_errorCnt,g_warningCnt;

extern int32_t g_maxSoundPos;

enum
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
};

#if !defined LUNATIC
static char *C_GetLabelType(int32_t type)
{
    int32_t i;
    char x[64];

    const char *LabelTypeText[] =
    {
        "define",
        "state",
        "actor",
        "action",
        "ai",
        "move"
    };

    x[0] = 0;
    for (i=0; i<6; i++)
    {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, LabelTypeText[i]);
    }

    return Xstrdup(x);
}

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
    { "addlog",                 CON_ADDLOG },
    { "addlogvar",              CON_ADDLOGVAR },
    { "addphealth",             CON_ADDPHEALTH },
    { "addstrength",            CON_ADDSTRENGTH },
    { "addvar",                 CON_ADDVAR },
    { "addvarvar",              CON_ADDVARVAR },
    { "addweapon",              CON_ADDWEAPON },
    { "addweaponvar",           CON_ADDWEAPONVAR },
    { "ai",                     CON_AI },
    { "andvar",                 CON_ANDVAR },
    { "andvarvar",              CON_ANDVARVAR },
    { "angoff",                 CON_ANGOFF },
    { "angoffvar",              CON_ANGOFFVAR },
    { "appendevent",            CON_APPENDEVENT },
    { "betaname",               CON_BETANAME },
    { "break",                  CON_BREAK },
    { "cactor",                 CON_CACTOR },
    { "calchypotenuse",         CON_CALCHYPOTENUSE },
    { "cansee",                 CON_CANSEE },
    { "canseespr",              CON_CANSEESPR },
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
    { "debris",                 CON_DEBRIS },
    { "debug",                  CON_DEBUG },
    { "default",                CON_DEFAULT },
    { "define",                 CON_DEFINE },
    { "definecheat",            CON_DEFINECHEAT },
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
    { "eqspawnvar",             CON_EQSPAWNVAR },
    { "eshootvar",              CON_ESHOOTVAR },
    { "espawnvar",              CON_ESPAWNVAR },
    { "eventloadactor",         CON_EVENTLOADACTOR },
    { "ezshootvar",             CON_EZSHOOTVAR },
    { "fall",                   CON_FALL },
    { "findnearactor3dvar",     CON_FINDNEARACTOR3DVAR },
    { "findnearactorvar",       CON_FINDNEARACTORVAR },
    { "findnearactorzvar",      CON_FINDNEARACTORZVAR },
    { "findnearsprite3dvar",    CON_FINDNEARSPRITE3DVAR },
    { "findnearspritevar",      CON_FINDNEARSPRITEVAR },
    { "findnearspritezvar",     CON_FINDNEARSPRITEZVAR },
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
    { "gettimedate",            CON_GETTIMEDATE },
    { "gettspr",                CON_GETTSPR },
    { "getuserdef",             CON_GETUSERDEF },
    { "getwall",                CON_GETWALL },
    { "getzrange",              CON_GETZRANGE },
    { "globalsound",            CON_GLOBALSOUND },
    { "globalsoundvar",         CON_GLOBALSOUNDVAR },
    { "gmaxammo",               CON_GMAXAMMO },
    { "guniqhudid",             CON_GUNIQHUDID },
    { "guts",                   CON_GUTS },
    { "headspritesect",         CON_HEADSPRITESECT },
    { "headspritestat",         CON_HEADSPRITESTAT },
    { "hitradius",              CON_HITRADIUS },
    { "hitradiusvar",           CON_HITRADIUSVAR },
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
    { "ifvarand",               CON_IFVARAND },
    { "ifvarboth",              CON_IFVARBOTH },
    { "ifvare",                 CON_IFVARE },
    { "ifvareither",            CON_IFVAREITHER },
    { "ifvarg",                 CON_IFVARG },
    { "ifvarge",                CON_IFVARGE },
    { "ifvarl",                 CON_IFVARL },
    { "ifvarle",                CON_IFVARLE },
    { "ifvarn",                 CON_IFVARN },
    { "ifvaror",                CON_IFVAROR },
    { "ifvarvarand",            CON_IFVARVARAND },
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
    { "pstomp",                 CON_PSTOMP },
    { "qgetsysstr",             CON_QGETSYSSTR },
    { "qspawnvar",              CON_QSPAWNVAR },
    { "qsprintf",               CON_QSPRINTF },
    { "qstrcat",                CON_QSTRCAT },
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
    { "shootvar",               CON_SHOOTVAR },
    { "showview",               CON_SHOWVIEW },
    { "showviewunbiased",       CON_SHOWVIEWUNBIASED },
    { "sin",                    CON_SIN },
    { "sizeat",                 CON_SIZEAT },
    { "sizeto",                 CON_SIZETO },
    { "sleeptime",              CON_SLEEPTIME },
    { "smaxammo",               CON_SMAXAMMO },
    { "sound",                  CON_SOUND },
    { "soundonce",              CON_SOUNDONCE },
    { "soundoncevar",           CON_SOUNDONCEVAR },
    { "soundvar",               CON_SOUNDVAR },
    { "spawn",                  CON_SPAWN },
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
    { "starttrackvar",          CON_STARTTRACKVAR },
    { "state",                  CON_STATE },
    { "stopactorsound",         CON_STOPACTORSOUND },
    { "stopallmusic",           CON_STOPALLMUSIC },
    { "stopallsounds",          CON_STOPALLSOUNDS },
    { "stopsound",              CON_STOPSOUND },
    { "stopsoundvar",           CON_STOPSOUNDVAR },
    { "strength",               CON_STRENGTH },
    { "subvar",                 CON_SUBVAR },
    { "subvarvar",              CON_SUBVARVAR },
    { "switch",                 CON_SWITCH },
    { "time",                   CON_TIME },
    { "tip",                    CON_TIP },
    { "tossweapon",             CON_TOSSWEAPON },
    { "undefinegamefunc",       CON_UNDEFINEGAMEFUNC },
    { "undefinelevel",          CON_UNDEFINELEVEL },
    { "undefineskill",          CON_UNDEFINESKILL },
    { "undefinevolume",         CON_UNDEFINEVOLUME },
    { "updatesector",           CON_UPDATESECTOR },
    { "updatesectorz",          CON_UPDATESECTORZ },
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
    { "zshootvar",              CON_ZSHOOTVAR },
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
    { "ck",                     CON_CHEATKEYS },

    { "qputs",                  CON_REDEFINEQUOTE },

    { "espawn",                 CON_ESPAWNVAR },
    { "qspawn",                 CON_QSPAWNVAR },
    { "eqspawn",                CON_EQSPAWNVAR },

    { "eshoot",                 CON_ESHOOTVAR },
    { "zshoot",                 CON_ZSHOOTVAR },
    { "ezshoot",                CON_EZSHOOTVAR },
    { "shoot",                  CON_SHOOTVAR },

    { "findnearactor",          CON_FINDNEARACTORVAR },
    { "findnearactor3d",        CON_FINDNEARACTOR3DVAR },
    { "findnearactorz",         CON_FINDNEARACTORZVAR },

    { "findnearsprite",         CON_FINDNEARSPRITEVAR },
    { "findnearsprite3d",       CON_FINDNEARSPRITE3DVAR },
    { "findnearspritez",        CON_FINDNEARSPRITEZVAR },
};

char const * VM_GetKeywordForID(int32_t id)
{
    // could be better but this is only called for diagnostics, ayy lmao
    for (tokenmap_t const & keyword : vm_keywords)
        if (keyword.val == id)
            return keyword.token;

    return nullptr;
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
#ifdef LUNATIC
    "EVENT_ANIMATEALLSPRITES",
#endif
};

#if !defined LUNATIC
const memberlabel_t SectorLabels[]=
{
    { "wallptr",         SECTOR_WALLPTR,         0, 0 },
    { "wallnum",         SECTOR_WALLNUM,         0, 0 },
    { "ceilingz",        SECTOR_CEILINGZ,        0, 0 },
    { "ceilingzgoal",    SECTOR_CEILINGZGOAL,    0, 0 },
    { "ceilingzvel",     SECTOR_CEILINGZVEL,     0, 0 },
    { "floorz",          SECTOR_FLOORZ,          0, 0 },
    { "floorzgoal",      SECTOR_FLOORZGOAL,      0, 0 },
    { "floorzvel",       SECTOR_FLOORZVEL,       0, 0 },
    { "ceilingstat",     SECTOR_CEILINGSTAT,     0, 0 },
    { "floorstat",       SECTOR_FLOORSTAT,       0, 0 },
    { "ceilingpicnum",   SECTOR_CEILINGPICNUM,   0, 0 },
    { "ceilingslope",    SECTOR_CEILINGSLOPE,    0, 0 },
    { "ceilingshade",    SECTOR_CEILINGSHADE,    0, 0 },
    { "ceilingpal",      SECTOR_CEILINGPAL,      0, 0 },
    { "ceilingxpanning", SECTOR_CEILINGXPANNING, 0, 0 },
    { "ceilingypanning", SECTOR_CEILINGYPANNING, 0, 0 },
    { "floorpicnum",     SECTOR_FLOORPICNUM,     0, 0 },
    { "floorslope",      SECTOR_FLOORSLOPE,      0, 0 },
    { "floorshade",      SECTOR_FLOORSHADE,      0, 0 },
    { "floorpal",        SECTOR_FLOORPAL,        0, 0 },
    { "floorxpanning",   SECTOR_FLOORXPANNING,   0, 0 },
    { "floorypanning",   SECTOR_FLOORYPANNING,   0, 0 },
    { "visibility",      SECTOR_VISIBILITY,      0, 0 },
    { "fogpal",          SECTOR_FOGPAL,          0, 0 }, // formerly filler
    { "alignto",         SECTOR_FOGPAL,          0, 0 }, // formerly filler
    { "lotag",           SECTOR_LOTAG,           0, 0 },
    { "hitag",           SECTOR_HITAG,           0, 0 },
    { "extra",           SECTOR_EXTRA,           0, 0 },
    { "ceilingbunch",    SECTOR_CEILINGBUNCH,    0, 0 },
    { "floorbunch",      SECTOR_FLOORBUNCH,      0, 0 },
    { "ulotag",          SECTOR_ULOTAG,          0, 0 },
    { "uhitag",          SECTOR_UHITAG,          0, 0 },
    { NULL,             -1,                      0, 0  }     // END OF LIST
};

const memberlabel_t WallLabels[]=
{
    { "x",          WALL_X,          0, 0 },
    { "y",          WALL_Y,          0, 0 },
    { "point2",     WALL_POINT2,     0, 0 },
    { "nextwall",   WALL_NEXTWALL,   0, 0 },
    { "nextsector", WALL_NEXTSECTOR, 0, 0 },
    { "cstat",      WALL_CSTAT,      0, 0 },
    { "picnum",     WALL_PICNUM,     0, 0 },
    { "overpicnum", WALL_OVERPICNUM, 0, 0 },
    { "shade",      WALL_SHADE,      0, 0 },
    { "pal",        WALL_PAL,        0, 0 },
    { "xrepeat",    WALL_XREPEAT,    0, 0 },
    { "yrepeat",    WALL_YREPEAT,    0, 0 },
    { "xpanning",   WALL_XPANNING,   0, 0 },
    { "ypanning",   WALL_YPANNING,   0, 0 },
    { "lotag",      WALL_LOTAG,      0, 0 },
    { "hitag",      WALL_HITAG,      0, 0 },
    { "extra",      WALL_EXTRA,      0, 0 },
    { "ulotag",     WALL_ULOTAG,     0, 0 },
    { "uhitag",     WALL_UHITAG,     0, 0 },
    { "blend",      WALL_BLEND,      0, 0 },
    { NULL,         -1,              0, 0  }     // END OF LIST
};

const memberlabel_t ActorLabels[]=
{
    { "x",              ACTOR_X,                      0, 0 },
    { "y",              ACTOR_Y,                      0, 0 },
    { "z",              ACTOR_Z,                      0, 0 },
    { "cstat",          ACTOR_CSTAT,                  0, 0 },
    { "picnum",         ACTOR_PICNUM,                 0, 0 },
    { "shade",          ACTOR_SHADE,                  0, 0 },
    { "pal",            ACTOR_PAL,                    0, 0 },
    { "clipdist",       ACTOR_CLIPDIST,               0, 0 },
//    { "filler",       ACTOR_DETAIL,                 0, 0 },
    { "blend",          ACTOR_DETAIL,                 0, 0 },
    { "xrepeat",        ACTOR_XREPEAT,                0, 0 },
    { "yrepeat",        ACTOR_YREPEAT,                0, 0 },
    { "xoffset",        ACTOR_XOFFSET,                0, 0 },
    { "yoffset",        ACTOR_YOFFSET,                0, 0 },
    { "sectnum",        ACTOR_SECTNUM,                0, 0 },
    { "statnum",        ACTOR_STATNUM,                0, 0 },
    { "ang",            ACTOR_ANG,                    0, 0 },
    { "owner",          ACTOR_OWNER,                  0, 0 },
    { "xvel",           ACTOR_XVEL,                   0, 0 },
    { "yvel",           ACTOR_YVEL,                   0, 0 },
    { "zvel",           ACTOR_ZVEL,                   0, 0 },
    { "lotag",          ACTOR_LOTAG,                  0, 0 },
    { "hitag",          ACTOR_HITAG,                  0, 0 },
    { "extra",          ACTOR_EXTRA,                  0, 0 },

    // ActorExtra labels...
    { "htcgg",          ACTOR_HTCGG,                  0, 0 },
    { "htpicnum",       ACTOR_HTPICNUM,               0, 0 },
    { "htang",          ACTOR_HTANG,                  0, 0 },
    { "htextra",        ACTOR_HTEXTRA,                0, 0 },
    { "htowner",        ACTOR_HTOWNER,                0, 0 },
    { "htmovflag",      ACTOR_HTMOVFLAG,              0, 0 },
    { "httempang",      ACTOR_HTTEMPANG,              0, 0 },
    { "htactorstayput", ACTOR_HTACTORSTAYPUT,         0, 0 },
    { "htdispicnum",    ACTOR_HTDISPICNUM,            0, 0 },
    { "httimetosleep",  ACTOR_HTTIMETOSLEEP,          0, 0 },
    { "htfloorz",       ACTOR_HTFLOORZ,               0, 0 },
    { "htceilingz",     ACTOR_HTCEILINGZ,             0, 0 },
    { "htlastvx",       ACTOR_HTLASTVX,               0, 0 },
    { "htlastvy",       ACTOR_HTLASTVY,               0, 0 },
    { "htbposx",        ACTOR_HTBPOSX,                0, 0 },
    { "htbposy",        ACTOR_HTBPOSY,                0, 0 },
    { "htbposz",        ACTOR_HTBPOSZ,                0, 0 },
    { "htg_t",          ACTOR_HTG_T,                  LABEL_HASPARM2, 10 },

    // model flags

    { "angoff",         ACTOR_ANGOFF,                 0, 0 },
    { "pitch",          ACTOR_PITCH,                  0, 0 },
    { "roll",           ACTOR_ROLL,                   0, 0 },
    { "mdxoff",         ACTOR_MDXOFF,                 0, 0 },
    { "mdyoff",         ACTOR_MDYOFF,                 0, 0 },
    { "mdzoff",         ACTOR_MDZOFF,                 0, 0 },
    { "mdflags",        ACTOR_MDFLAGS,                0, 0 },
    { "xpanning",       ACTOR_XPANNING,               0, 0 },
    { "ypanning",       ACTOR_YPANNING,               0, 0 },

    { "htflags",        ACTOR_HTFLAGS,                0, 0 },

    { "alpha",          ACTOR_ALPHA,                  0, 0 },

    { "ulotag",         ACTOR_ULOTAG,                 0, 0 },
    { "uhitag",         ACTOR_UHITAG,                 0, 0 },

    { "isvalid",        ACTOR_ISVALID,                0, 0 },
// aliases:
    { "movflags",       ACTOR_HITAG,                  0, 0 },
    { "detail",         ACTOR_DETAIL,                 0, 0 },  // deprecated name for 'blend'

    { NULL,             -1,                           0, 0  }     // END OF LIST
};

const memberlabel_t TsprLabels[]=
{
    // tsprite access

    { "tsprx",        ACTOR_X,        0, 0 },
    { "tspry",        ACTOR_Y,        0, 0 },
    { "tsprz",        ACTOR_Z,        0, 0 },
    { "tsprcstat",    ACTOR_CSTAT,    0, 0 },
    { "tsprpicnum",   ACTOR_PICNUM,   0, 0 },
    { "tsprshade",    ACTOR_SHADE,    0, 0 },
    { "tsprpal",      ACTOR_PAL,      0, 0 },
    { "tsprclipdist", ACTOR_CLIPDIST, 0, 0 },
//    { "tsprfiller", ACTOR_DETAIL,   0, 0 },
    { "tsprblend",    ACTOR_DETAIL,   0, 0 },
    { "tsprxrepeat",  ACTOR_XREPEAT,  0, 0 },
    { "tspryrepeat",  ACTOR_YREPEAT,  0, 0 },
    { "tsprxoffset",  ACTOR_XOFFSET,  0, 0 },
    { "tspryoffset",  ACTOR_YOFFSET,  0, 0 },
    { "tsprsectnum",  ACTOR_SECTNUM,  0, 0 },
    { "tsprstatnum",  ACTOR_STATNUM,  0, 0 },
    { "tsprang",      ACTOR_ANG,      0, 0 },
    { "tsprowner",    ACTOR_OWNER,    0, 0 },
#if 1
    { "tsprxvel",     ACTOR_XVEL,     0, 0 },
    { "tspryvel",     ACTOR_YVEL,     0, 0 },
    { "tsprzvel",     ACTOR_ZVEL,     0, 0 },
    { "tsprlotag",    ACTOR_LOTAG,    0, 0 },
    { "tsprhitag",    ACTOR_HITAG,    0, 0 },
    { "tsprextra",    ACTOR_EXTRA,    0, 0 },
#endif
// aliases:
    { "tsprdetail",   ACTOR_DETAIL,   0, 0 },  // deprecated name for 'tsprblend'

    { NULL,           -1,             0, 0  }     // END OF LIST
};

const memberlabel_t PlayerLabels[]=
{
    { "zoom",                  PLAYER_ZOOM,                  0, 0 },
    { "exitx",                 PLAYER_EXITX,                 0, 0 },
    { "exity",                 PLAYER_EXITY,                 0, 0 },
    { "loogiex",               PLAYER_LOOGIEX,               LABEL_HASPARM2, 64 },
    { "loogiey",               PLAYER_LOOGIEY,               LABEL_HASPARM2, 64 },
    { "numloogs",              PLAYER_NUMLOOGS,              0, 0 },
    { "loogcnt",               PLAYER_LOOGCNT,               0, 0 },
    { "posx",                  PLAYER_POSX,                  0, 0 },
    { "posy",                  PLAYER_POSY,                  0, 0 },
    { "posz",                  PLAYER_POSZ,                  0, 0 },
    { "horiz",                 PLAYER_HORIZ,                 0, 0 },
    { "ohoriz",                PLAYER_OHORIZ,                0, 0 },
    { "ohorizoff",             PLAYER_OHORIZOFF,             0, 0 },
    { "invdisptime",           PLAYER_INVDISPTIME,           0, 0 },
    { "bobposx",               PLAYER_BOBPOSX,               0, 0 },
    { "bobposy",               PLAYER_BOBPOSY,               0, 0 },
    { "oposx",                 PLAYER_OPOSX,                 0, 0 },
    { "oposy",                 PLAYER_OPOSY,                 0, 0 },
    { "oposz",                 PLAYER_OPOSZ,                 0, 0 },
    { "pyoff",                 PLAYER_PYOFF,                 0, 0 },
    { "opyoff",                PLAYER_OPYOFF,                0, 0 },
    { "posxv",                 PLAYER_POSXV,                 0, 0 },
    { "posyv",                 PLAYER_POSYV,                 0, 0 },
    { "poszv",                 PLAYER_POSZV,                 0, 0 },
    { "last_pissed_time",      PLAYER_LAST_PISSED_TIME,      0, 0 },
    { "truefz",                PLAYER_TRUEFZ,                0, 0 },
    { "truecz",                PLAYER_TRUECZ,                0, 0 },
    { "player_par",            PLAYER_PLAYER_PAR,            0, 0 },
    { "visibility",            PLAYER_VISIBILITY,            0, 0 },
    { "bobcounter",            PLAYER_BOBCOUNTER,            0, 0 },
    { "weapon_sway",           PLAYER_WEAPON_SWAY,           0, 0 },
    { "pals_time",             PLAYER_PALS_TIME,             0, 0 },
    { "randomflamex",          PLAYER_RANDOMFLAMEX,          0, 0 },
    { "crack_time",            PLAYER_CRACK_TIME,            0, 0 },
    { "aim_mode",              PLAYER_AIM_MODE,              0, 0 },
    { "ang",                   PLAYER_ANG,                   0, 0 },
    { "oang",                  PLAYER_OANG,                  0, 0 },
    { "angvel",                PLAYER_ANGVEL,                0, 0 },
    { "cursectnum",            PLAYER_CURSECTNUM,            0, 0 },
    { "look_ang",              PLAYER_LOOK_ANG,              0, 0 },
    { "last_extra",            PLAYER_LAST_EXTRA,            0, 0 },
    { "subweapon",             PLAYER_SUBWEAPON,             0, 0 },
    { "ammo_amount",           PLAYER_AMMO_AMOUNT,           LABEL_HASPARM2, MAX_WEAPONS },
    { "wackedbyactor",         PLAYER_WACKEDBYACTOR,         0, 0 },
    { "frag",                  PLAYER_FRAG,                  0, 0 },
    { "fraggedself",           PLAYER_FRAGGEDSELF,           0, 0 },
    { "curr_weapon",           PLAYER_CURR_WEAPON,           0, 0 },
    { "last_weapon",           PLAYER_LAST_WEAPON,           0, 0 },
    { "tipincs",               PLAYER_TIPINCS,               0, 0 },
    { "horizoff",              PLAYER_HORIZOFF,              0, 0 },
    { "wantweaponfire",        PLAYER_WANTWEAPONFIRE,        0, 0 },
    { "holoduke_amount",       PLAYER_HOLODUKE_AMOUNT,       0, 0 },
    { "newowner",              PLAYER_NEWOWNER,              0, 0 },
    { "hurt_delay",            PLAYER_HURT_DELAY,            0, 0 },
    { "hbomb_hold_delay",      PLAYER_HBOMB_HOLD_DELAY,      0, 0 },
    { "jumping_counter",       PLAYER_JUMPING_COUNTER,       0, 0 },
    { "airleft",               PLAYER_AIRLEFT,               0, 0 },
    { "knee_incs",             PLAYER_KNEE_INCS,             0, 0 },
    { "access_incs",           PLAYER_ACCESS_INCS,           0, 0 },
    { "fta",                   PLAYER_FTA,                   0, 0 },
    { "ftq",                   PLAYER_FTQ,                   0, 0 },
    { "access_wallnum",        PLAYER_ACCESS_WALLNUM,        0, 0 },
    { "access_spritenum",      PLAYER_ACCESS_SPRITENUM,      0, 0 },
    { "kickback_pic",          PLAYER_KICKBACK_PIC,          0, 0 },
    { "got_access",            PLAYER_GOT_ACCESS,            0, 0 },
    { "weapon_ang",            PLAYER_WEAPON_ANG,            0, 0 },
    { "firstaid_amount",       PLAYER_FIRSTAID_AMOUNT,       0, 0 },
    { "somethingonplayer",     PLAYER_SOMETHINGONPLAYER,     0, 0 },
    { "on_crane",              PLAYER_ON_CRANE,              0, 0 },
    { "i",                     PLAYER_I,                     0, 0 },
    { "one_parallax_sectnum",  PLAYER_ONE_PARALLAX_SECTNUM,  0, 0 },
    { "over_shoulder_on",      PLAYER_OVER_SHOULDER_ON,      0, 0 },
    { "random_club_frame",     PLAYER_RANDOM_CLUB_FRAME,     0, 0 },
    { "fist_incs",             PLAYER_FIST_INCS,             0, 0 },
    { "one_eighty_count",      PLAYER_ONE_EIGHTY_COUNT,      0, 0 },
    { "cheat_phase",           PLAYER_CHEAT_PHASE,           0, 0 },
    { "dummyplayersprite",     PLAYER_DUMMYPLAYERSPRITE,     0, 0 },
    { "extra_extra8",          PLAYER_EXTRA_EXTRA8,          0, 0 },
    { "quick_kick",            PLAYER_QUICK_KICK,            0, 0 },
    { "heat_amount",           PLAYER_HEAT_AMOUNT,           0, 0 },
    { "actorsqu",              PLAYER_ACTORSQU,              0, 0 },
    { "timebeforeexit",        PLAYER_TIMEBEFOREEXIT,        0, 0 },
    { "customexitsound",       PLAYER_CUSTOMEXITSOUND,       0, 0 },
    { "weaprecs",              PLAYER_WEAPRECS,              LABEL_HASPARM2, MAX_WEAPONS },
    { "weapreccnt",            PLAYER_WEAPRECCNT,            0, 0 },
    { "interface_toggle_flag", PLAYER_INTERFACE_TOGGLE_FLAG, 0, 0 },
    { "rotscrnang",            PLAYER_ROTSCRNANG,            0, 0 },
    { "dead_flag",             PLAYER_DEAD_FLAG,             0, 0 },
    { "show_empty_weapon",     PLAYER_SHOW_EMPTY_WEAPON,     0, 0 },
    { "scuba_amount",          PLAYER_SCUBA_AMOUNT,          0, 0 },
    { "jetpack_amount",        PLAYER_JETPACK_AMOUNT,        0, 0 },
    { "steroids_amount",       PLAYER_STEROIDS_AMOUNT,       0, 0 },
    { "shield_amount",         PLAYER_SHIELD_AMOUNT,         0, 0 },
    { "holoduke_on",           PLAYER_HOLODUKE_ON,           0, 0 },
    { "pycount",               PLAYER_PYCOUNT,               0, 0 },
    { "weapon_pos",            PLAYER_WEAPON_POS,            0, 0 },
    { "frag_ps",               PLAYER_FRAG_PS,               0, 0 },
    { "transporter_hold",      PLAYER_TRANSPORTER_HOLD,      0, 0 },
    { "clipdist",              PLAYER_CLIPDIST,              0, 0 },
    { "last_full_weapon",      PLAYER_LAST_FULL_WEAPON,      0, 0 },
    { "footprintshade",        PLAYER_FOOTPRINTSHADE,        0, 0 },
    { "boot_amount",           PLAYER_BOOT_AMOUNT,           0, 0 },
    { "scream_voice",          PLAYER_SCREAM_VOICE,          0, 0 },
    { "gm",                    PLAYER_GM,                    0, 0 },
    { "on_warping_sector",     PLAYER_ON_WARPING_SECTOR,     0, 0 },
    { "footprintcount",        PLAYER_FOOTPRINTCOUNT,        0, 0 },
    { "hbomb_on",              PLAYER_HBOMB_ON,              0, 0 },
    { "jumping_toggle",        PLAYER_JUMPING_TOGGLE,        0, 0 },
    { "rapid_fire_hold",       PLAYER_RAPID_FIRE_HOLD,       0, 0 },
    { "on_ground",             PLAYER_ON_GROUND,             0, 0 },
    { "name",                  PLAYER_NAME,                  LABEL_ISSTRING, 32 },
    { "inven_icon",            PLAYER_INVEN_ICON,            0, 0 },
    { "buttonpalette",         PLAYER_BUTTONPALETTE,         0, 0 },
    { "jetpack_on",            PLAYER_JETPACK_ON,            0, 0 },
    { "spritebridge",          PLAYER_SPRITEBRIDGE,          0, 0 },
    { "lastrandomspot",        PLAYER_LASTRANDOMSPOT,        0, 0 },
    { "scuba_on",              PLAYER_SCUBA_ON,              0, 0 },
    { "footprintpal",          PLAYER_FOOTPRINTPAL,          0, 0 },
    { "heat_on",               PLAYER_HEAT_ON,               0, 0 },
    { "holster_weapon",        PLAYER_HOLSTER_WEAPON,        0, 0 },
    { "falling_counter",       PLAYER_FALLING_COUNTER,       0, 0 },
    { "gotweapon",             PLAYER_GOTWEAPON,             LABEL_HASPARM2, MAX_WEAPONS },
    { "refresh_inventory",     PLAYER_REFRESH_INVENTORY,     0, 0 },
    { "palette",               PLAYER_PALETTE,               0, 0 },
    { "toggle_key_flag",       PLAYER_TOGGLE_KEY_FLAG,       0, 0 },
    { "knuckle_incs",          PLAYER_KNUCKLE_INCS,          0, 0 },
    { "walking_snd_toggle",    PLAYER_WALKING_SND_TOGGLE,    0, 0 },
    { "palookup",              PLAYER_PALOOKUP,              0, 0 },
    { "hard_landing",          PLAYER_HARD_LANDING,          0, 0 },
    { "max_secret_rooms",      PLAYER_MAX_SECRET_ROOMS,      0, 0 },
    { "secret_rooms",          PLAYER_SECRET_ROOMS,          0, 0 },
    { "pals",                  PLAYER_PALS,                  LABEL_HASPARM2, 3 },
    { "max_actors_killed",     PLAYER_MAX_ACTORS_KILLED,     0, 0 },
    { "actors_killed",         PLAYER_ACTORS_KILLED,         0, 0 },
    { "return_to_center",      PLAYER_RETURN_TO_CENTER,      0, 0 },
    { "runspeed",              PLAYER_RUNSPEED,              0, 0 },
    { "sbs",                   PLAYER_SBS,                   0, 0 },
    { "reloading",             PLAYER_RELOADING,             0, 0 },
    { "auto_aim",              PLAYER_AUTO_AIM,              0, 0 },
    { "movement_lock",         PLAYER_MOVEMENT_LOCK,         0, 0 },
    { "sound_pitch",           PLAYER_SOUND_PITCH,           0, 0 },
    { "weaponswitch",          PLAYER_WEAPONSWITCH,          0, 0 },
    { "team",                  PLAYER_TEAM,                  0, 0 },
    { "max_player_health",     PLAYER_MAX_PLAYER_HEALTH,     0, 0 },
    { "max_shield_amount",     PLAYER_MAX_SHIELD_AMOUNT,     0, 0 },
    { "max_ammo_amount",       PLAYER_MAX_AMMO_AMOUNT,       LABEL_HASPARM2, MAX_WEAPONS },
    { "last_quick_kick",       PLAYER_LAST_QUICK_KICK,       0, 0 },
    { "autostep",              PLAYER_AUTOSTEP,              0, 0 },
    { "autostep_sbw",          PLAYER_AUTOSTEP_SBW,          0, 0 },
    { "hudpal",                PLAYER_HUDPAL,                0, 0 },
    { "index",                 PLAYER_INDEX,                 0, 0 },
    { "connected",             PLAYER_CONNECTED,             0, 0 },
    { "frags",                 PLAYER_FRAGS,                 LABEL_HASPARM2, MAXPLAYERS },
    { "deaths",                PLAYER_DEATHS,                0, 0 },
    { NULL,                    -1,                           0, 0  }     // END OF LIST
};

const memberlabel_t ProjectileLabels[]=
{
    { "workslike",  PROJ_WORKSLIKE,   0, 0 },
    { "spawns",     PROJ_SPAWNS,      0, 0 },
    { "sxrepeat",   PROJ_SXREPEAT,    0, 0 },
    { "syrepeat",   PROJ_SYREPEAT,    0, 0 },
    { "sound",      PROJ_SOUND,       0, 0 },
    { "isound",     PROJ_ISOUND,      0, 0 },
    { "vel",        PROJ_VEL,         0, 0 },
    { "extra",      PROJ_EXTRA,       0, 0 },
    { "decal",      PROJ_DECAL,       0, 0 },
    { "trail",      PROJ_TRAIL,       0, 0 },
    { "txrepeat",   PROJ_TXREPEAT,    0, 0 },
    { "tyrepeat",   PROJ_TYREPEAT,    0, 0 },
    { "toffset",    PROJ_TOFFSET,     0, 0 },
    { "tnum",       PROJ_TNUM,        0, 0 },
    { "drop",       PROJ_DROP,        0, 0 },
    { "cstat",      PROJ_CSTAT,       0, 0 },
    { "clipdist",   PROJ_CLIPDIST,    0, 0 },
    { "shade",      PROJ_SHADE,       0, 0 },
    { "xrepeat",    PROJ_XREPEAT,     0, 0 },
    { "yrepeat",    PROJ_YREPEAT,     0, 0 },
    { "pal",        PROJ_PAL,         0, 0 },
    { "extra_rand", PROJ_EXTRA_RAND,  0, 0 },
    { "hitradius",  PROJ_HITRADIUS,   0, 0 },
    { "velmult",    PROJ_MOVECNT,     0, 0 },
    { "offset",     PROJ_OFFSET,      0, 0 },
    { "bounces",    PROJ_BOUNCES,     0, 0 },
    { "bsound",     PROJ_BSOUND,      0, 0 },
    { "range",      PROJ_RANGE,       0, 0 },
    { "flashcolor", PROJ_FLASH_COLOR, 0, 0 },
    { "userdata",   PROJ_USERDATA,    0, 0 },
    { NULL,         -1,               0, 0  }     // END OF LIST
};

const memberlabel_t UserdefsLabels[]=
{
    //   { "<null>", 1,                                          0, 0 },
    { "god",                    USERDEFS_GOD,                    0, 0 },
    { "warp_on",                USERDEFS_WARP_ON,                0, 0 },
    { "cashman",                USERDEFS_CASHMAN,                0, 0 },
    { "eog",                    USERDEFS_EOG,                    0, 0 },
    { "showallmap",             USERDEFS_SHOWALLMAP,             0, 0 },
    { "show_help",              USERDEFS_SHOW_HELP,              0, 0 },
    { "scrollmode",             USERDEFS_SCROLLMODE,             0, 0 },
    { "clipping",               USERDEFS_CLIPPING,               0, 0 },
    { "user_name",              USERDEFS_USER_NAME,              LABEL_HASPARM2, MAXPLAYERS },
    { "ridecule",               USERDEFS_RIDECULE,               LABEL_HASPARM2 | LABEL_ISSTRING, 10 },
    { "savegame",               USERDEFS_SAVEGAME,               LABEL_HASPARM2 | LABEL_ISSTRING, 10 },
    { "pwlockout",              USERDEFS_PWLOCKOUT,              LABEL_ISSTRING, 128 },
    { "rtsname;",               USERDEFS_RTSNAME,                LABEL_ISSTRING, 128 },
    { "overhead_on",            USERDEFS_OVERHEAD_ON,            0, 0 },
    { "last_overhead",          USERDEFS_LAST_OVERHEAD,          0, 0 },
    { "showweapons",            USERDEFS_SHOWWEAPONS,            0, 0 },

    { "pause_on",               USERDEFS_PAUSE_ON,               0, 0 },
    { "from_bonus",             USERDEFS_FROM_BONUS,             0, 0 },
    { "camerasprite",           USERDEFS_CAMERASPRITE,           0, 0 },
    { "last_camsprite",         USERDEFS_LAST_CAMSPRITE,         0, 0 },
    { "last_level",             USERDEFS_LAST_LEVEL,             0, 0 },
    { "secretlevel",            USERDEFS_SECRETLEVEL,            0, 0 },
    { "playerbest",             USERDEFS_PLAYERBEST,             0, 0 },

    { "const_visibility",       USERDEFS_CONST_VISIBILITY,       0, 0 },
    { "uw_framerate",           USERDEFS_UW_FRAMERATE,           0, 0 },
    { "camera_time",            USERDEFS_CAMERA_TIME,            0, 0 },
    { "folfvel",                USERDEFS_FOLFVEL,                0, 0 },
    { "folavel",                USERDEFS_FOLAVEL,                0, 0 },
    { "folx",                   USERDEFS_FOLX,                   0, 0 },
    { "foly",                   USERDEFS_FOLY,                   0, 0 },
    { "fola",                   USERDEFS_FOLA,                   0, 0 },
    { "reccnt",                 USERDEFS_RECCNT,                 0, 0 },

    { "m_origin_x",             USERDEFS_M_ORIGIN_X,             0, 0 },
    { "m_origin_y",             USERDEFS_M_ORIGIN_Y,             0, 0 },

    { "usevoxels",              USERDEFS_USEVOXELS,              0, 0 },
    { "usehightile",            USERDEFS_USEHIGHTILE,            0, 0 },
    { "usemodels",              USERDEFS_USEMODELS,              0, 0 },

    { "entered_name",           USERDEFS_ENTERED_NAME,           0, 0 },
    { "screen_tilting",         USERDEFS_SCREEN_TILTING,         0, 0 },
    { "shadows",                USERDEFS_SHADOWS,                0, 0 },
    { "fta_on",                 USERDEFS_FTA_ON,                 0, 0 },
    { "executions",             USERDEFS_EXECUTIONS,             0, 0 },
    { "auto_run",               USERDEFS_AUTO_RUN,               0, 0 },
    { "coords",                 USERDEFS_COORDS,                 0, 0 },
    { "tickrate",               USERDEFS_TICKRATE,               0, 0 },
    { "m_coop",                 USERDEFS_M_COOP,                 0, 0 },
    { "coop",                   USERDEFS_COOP,                   0, 0 },
    { "screen_size",            USERDEFS_SCREEN_SIZE,            0, 0 },
    { "lockout",                USERDEFS_LOCKOUT,                0, 0 },
    { "crosshair",              USERDEFS_CROSSHAIR,              0, 0 },
    { "playerai",               USERDEFS_PLAYERAI,               0, 0 },
    { "respawn_monsters",       USERDEFS_RESPAWN_MONSTERS,       0, 0 },
    { "respawn_items",          USERDEFS_RESPAWN_ITEMS,          0, 0 },
    { "respawn_inventory",      USERDEFS_RESPAWN_INVENTORY,      0, 0 },
    { "recstat",                USERDEFS_RECSTAT,                0, 0 },
    { "monsters_off",           USERDEFS_MONSTERS_OFF,           0, 0 },
    { "brightness",             USERDEFS_BRIGHTNESS,             0, 0 },
    { "m_respawn_items",        USERDEFS_M_RESPAWN_ITEMS,        0, 0 },
    { "m_respawn_monsters",     USERDEFS_M_RESPAWN_MONSTERS,     0, 0 },
    { "m_respawn_inventory",    USERDEFS_M_RESPAWN_INVENTORY,    0, 0 },
    { "m_recstat",              USERDEFS_M_RECSTAT,              0, 0 },
    { "m_monsters_off",         USERDEFS_M_MONSTERS_OFF,         0, 0 },
    { "detail",                 USERDEFS_DETAIL,                 0, 0 },
    { "m_ffire",                USERDEFS_M_FFIRE,                0, 0 },
    { "ffire",                  USERDEFS_FFIRE,                  0, 0 },
    { "m_player_skill",         USERDEFS_M_PLAYER_SKILL,         0, 0 },
    { "m_level_number",         USERDEFS_M_LEVEL_NUMBER,         0, 0 },
    { "m_volume_number",        USERDEFS_M_VOLUME_NUMBER,        0, 0 },
    { "multimode",              USERDEFS_MULTIMODE,              0, 0 },
    { "player_skill",           USERDEFS_PLAYER_SKILL,           0, 0 },
    { "level_number",           USERDEFS_LEVEL_NUMBER,           0, 0 },
    { "volume_number",          USERDEFS_VOLUME_NUMBER,          0, 0 },
    { "m_marker",               USERDEFS_M_MARKER,               0, 0 },
    { "marker",                 USERDEFS_MARKER,                 0, 0 },
    { "mouseflip",              USERDEFS_MOUSEFLIP,              0, 0 },
    { "statusbarscale",         USERDEFS_STATUSBARSCALE,         0, 0 },
    { "drawweapon",             USERDEFS_DRAWWEAPON,             0, 0 },
    { "mouseaiming",            USERDEFS_MOUSEAIMING,            0, 0 },
    { "weaponswitch",           USERDEFS_WEAPONSWITCH,           0, 0 },
    { "democams",               USERDEFS_DEMOCAMS,               0, 0 },
    { "color",                  USERDEFS_COLOR,                  0, 0 },
    { "msgdisptime",            USERDEFS_MSGDISPTIME,            0, 0 },
    { "statusbarmode",          USERDEFS_STATUSBARMODE,          0, 0 },
    { "m_noexits",              USERDEFS_M_NOEXITS,              0, 0 },
    { "noexits",                USERDEFS_NOEXITS,                0, 0 },
    { "autovote",               USERDEFS_AUTOVOTE,               0, 0 },
    { "automsg",                USERDEFS_AUTOMSG,                0, 0 },
    { "idplayers",              USERDEFS_IDPLAYERS,              0, 0 },
    { "team",                   USERDEFS_TEAM,                   0, 0 },
    { "viewbob",                USERDEFS_VIEWBOB,                0, 0 },
    { "weaponsway",             USERDEFS_WEAPONSWAY,             0, 0 },
    { "angleinterpolation",     USERDEFS_ANGLEINTERPOLATION,     0, 0 },
    { "obituaries",             USERDEFS_OBITUARIES,             0, 0 },
    { "levelstats",             USERDEFS_LEVELSTATS,             0, 0 },
    { "crosshairscale",         USERDEFS_CROSSHAIRSCALE,         0, 0 },
    { "althud",                 USERDEFS_ALTHUD,                 0, 0 },
    { "display_bonus_screen",   USERDEFS_DISPLAY_BONUS_SCREEN,   0, 0 },
    { "show_level_text",        USERDEFS_SHOW_LEVEL_TEXT,        0, 0 },
    { "weaponscale",            USERDEFS_WEAPONSCALE,            0, 0 },
    { "textscale",              USERDEFS_TEXTSCALE,              0, 0 },
    { "runkey_mode",            USERDEFS_RUNKEY_MODE,            0, 0 },
    { "musictoggle",            USERDEFS_MUSICTOGGLE,            0, 0 },
    { "gametypeflags",          USERDEFS_GAMETYPEFLAGS,          0, 0 },
    { "m_gametypeflags",        USERDEFS_M_GAMETYPEFLAGS,        0, 0 },
    { "globalflags",            USERDEFS_GLOBALFLAGS,            0, 0 },
    { "globalgameflags",        USERDEFS_GLOBALGAMEFLAGS,        0, 0 },
    { "vm_player",              USERDEFS_VM_PLAYER,              0, 0 },
    { "vm_sprite",              USERDEFS_VM_SPRITE,              0, 0 },
    { "vm_distance",            USERDEFS_VM_DISTANCE,            0, 0 },
    { "soundtoggle",            USERDEFS_SOUNDTOGGLE,            0, 0 },
    { "gametext_tracking",      USERDEFS_GAMETEXT_TRACKING,      0, 0 },
    { "mgametext_tracking",     USERDEFS_MGAMETEXT_TRACKING,     0, 0 },
    { "menutext_tracking",      USERDEFS_MENUTEXT_TRACKING,      0, 0 },
    { "maxspritesonscreen",     USERDEFS_MAXSPRITESONSCREEN,     0, 0 },
    { "screenarea_x1",          USERDEFS_SCREENAREA_X1,          0, 0 },
    { "screenarea_y1",          USERDEFS_SCREENAREA_Y1,          0, 0 },
    { "screenarea_x2",          USERDEFS_SCREENAREA_X2,          0, 0 },
    { "screenarea_y2",          USERDEFS_SCREENAREA_Y2,          0, 0 },
    { "screenfade",             USERDEFS_SCREENFADE,             0, 0 },
    { NULL,                     -1,                              0, 0  } // END OF LIST
};

const memberlabel_t InputLabels[]=
{
    { "avel",    INPUT_AVEL,    0, 0 },
    { "horz",    INPUT_HORZ,    0, 0 },
    { "fvel",    INPUT_FVEL,    0, 0 },
    { "svel",    INPUT_SVEL,    0, 0 },
    { "bits",    INPUT_BITS,    0, 0 },
    { "extbits", INPUT_EXTBITS, 0, 0 },
    { NULL,      -1,            0, 0  }     // END OF LIST
};

const memberlabel_t TileDataLabels[]=
{
    // tilesiz[]
    { "xsize",      TILEDATA_XSIZE,      0, 0 },
    { "ysize",      TILEDATA_YSIZE,      0, 0 },

    // picanm[]
    { "animframes", TILEDATA_ANIMFRAMES, 0, 0 },
    { "xoffset",    TILEDATA_XOFFSET,    0, 0 },
    { "yoffset",    TILEDATA_YOFFSET,    0, 0 },
    { "animspeed",  TILEDATA_ANIMSPEED,  0, 0 },
    { "animtype",   TILEDATA_ANIMTYPE,   0, 0 },

    // g_tile[]
    { "gameflags",  TILEDATA_GAMEFLAGS,  0, 0 },

    { NULL,         -1,                  0, 0  }     // END OF LIST
};

const memberlabel_t PalDataLabels[]=
{
    // g_noFloorPal[]
    { "nofloorpal", PALDATA_NOFLOORPAL, 0, 0 },

    { NULL,         -1,                 0, 0  }     // END OF LIST
};

const tokenmap_t iter_tokens [] =
{
    { "allsprites",      ITER_ALLSPRITES },
    { "allsectors",      ITER_ALLSECTORS },
    { "allwalls",        ITER_ALLWALLS },
    { "activelights",    ITER_ACTIVELIGHTS },
    { "drawnsprites",    ITER_DRAWNSPRITES },
    { "spritesofsector", ITER_SPRITESOFSECTOR },
    { "spritesofstatus", ITER_SPRITESOFSTATUS },
    { "loopofwall",      ITER_LOOPOFWALL },
    { "wallsofsector",   ITER_WALLSOFSECTOR },
    { "range",           ITER_RANGE },
    // vvv alternatives go here vvv
    { "lights",          ITER_ACTIVELIGHTS },
    { "sprofsec",        ITER_SPRITESOFSECTOR },
    { "sprofstat",       ITER_SPRITESOFSTATUS },
    { "walofsec",        ITER_WALLSOFSECTOR },
    { NULL,              -1 }     // END OF LIST
};

#endif

char *bitptr; // pointer to bitmap of which bytecode positions contain pointers
#define BITPTR_SET(x) (bitptr[(x)>>3] |= (1<<((x)&7)))
#define BITPTR_CLEAR(x) (bitptr[(x)>>3] &= ~(1<<((x)&7)))
#define BITPTR_IS_POINTER(x) (bitptr[(x)>>3] & (1<<((x) &7)))

#if !defined LUNATIC
hashtable_t h_gamevars    = { MAXGAMEVARS>>1, NULL };
hashtable_t h_arrays      = { MAXGAMEARRAYS>>1, NULL };
hashtable_t h_labels      = { 11264>>1, NULL };

static hashtable_t h_keywords   = { CON_END>>1, NULL };
static hashtable_t h_iter       = { ITER_END>>1, NULL };

static hashtable_t h_sector     = { SECTOR_END>>1, NULL };
static hashtable_t h_wall       = { WALL_END>>1, NULL };
static hashtable_t h_userdef    = { USERDEFS_END>>1, NULL };

static hashtable_t h_projectile = { PROJ_END>>1, NULL };
static hashtable_t h_player     = { PLAYER_END>>1, NULL };
static hashtable_t h_input      = { INPUT_END>>1, NULL };
static hashtable_t h_actor      = { ACTOR_END>>1, NULL };
static hashtable_t h_tsprite    = { ACTOR_END>>1, NULL };

static hashtable_t h_tiledata   = { TILEDATA_END>>1, NULL };
static hashtable_t h_paldata    = { PALDATA_END>>1, NULL };

static hashtable_t * const tables[] = {
    &h_gamevars,   &h_arrays, &h_labels, &h_keywords, &h_sector,  &h_wall,     &h_userdef,
    &h_projectile, &h_player, &h_input,  &h_actor,    &h_tsprite, &h_tiledata, &h_paldata,
    &h_iter
};

static hashtable_t * const tables_free [] = {
    &h_labels, &h_keywords, &h_sector,  &h_wall,     &h_userdef,
    &h_projectile, &h_player, &h_input,  &h_actor,    &h_tsprite, &h_tiledata, &h_paldata,
    &h_iter
};

#define STRUCT_HASH_SETUP(table, labels) do { for (i=0; labels[i].lId >= 0; i++) hash_add(&table, labels[i].name, i, 0); } while (0)

void C_InitHashes()
{
    uint32_t i;

    for (i=0; i < ARRAY_SIZE(tables); i++)
        hash_init(tables[i]);

    inithashnames();
    initsoundhashnames();

    for (tokenmap_t const & keyword : vm_keywords)
        hash_add(&h_keywords, keyword.token, keyword.val, 0);

    STRUCT_HASH_SETUP(h_sector, SectorLabels);
    STRUCT_HASH_SETUP(h_wall, WallLabels);
    STRUCT_HASH_SETUP(h_userdef, UserdefsLabels);
    STRUCT_HASH_SETUP(h_projectile, ProjectileLabels);
    STRUCT_HASH_SETUP(h_player, PlayerLabels);
    STRUCT_HASH_SETUP(h_input, InputLabels);
    STRUCT_HASH_SETUP(h_actor, ActorLabels);
    STRUCT_HASH_SETUP(h_tsprite, TsprLabels);
    STRUCT_HASH_SETUP(h_tiledata, TileDataLabels);
    STRUCT_HASH_SETUP(h_paldata, PalDataLabels);

    for (i=0; iter_tokens[i].val >=0; i++)
        hash_add(&h_iter, iter_tokens[i].token, iter_tokens[i].val, 0);
}

#undef STRUCT_HASH_SETUP

// "magic" number for { and }, overrides line number in compiled code for later detection
#define IFELSE_MAGIC 31337
static int32_t g_ifElseAborted;

static int32_t C_SetScriptSize(int32_t newsize)
{
    intptr_t const oscript = (intptr_t)apScript;
    intptr_t *newscript;
    intptr_t i, j;
    int32_t osize = g_scriptSize;
    char *scriptptrs;
    char *newbitptr;

    scriptptrs = (char *)Xcalloc(1, g_scriptSize * sizeof(uint8_t));

    for (i=g_scriptSize-1; i>=0; i--)
    {
        if (BITPTR_IS_POINTER(i))
        {
            if (EDUKE32_PREDICT_FALSE((intptr_t)apScript[i] < (intptr_t)&apScript[0] || (intptr_t)apScript[i] >= (intptr_t)&apScript[g_scriptSize]))
            {
                g_errorCnt++;
                buildprint("Internal compiler error at ", i, " (0x", hex(i), ")\n");
                VM_ScriptInfo(&apScript[i], 16);
            }

            scriptptrs[i] = 1;
            apScript[i] -= (intptr_t)&apScript[0];
        }
        else scriptptrs[i] = 0;
    }

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);

    newscript = (intptr_t *)Xrealloc(apScript, newsize * sizeof(intptr_t));
    newbitptr = (char *)Xcalloc(1,(((newsize+7)>>3)+1) * sizeof(uint8_t));

    if (newsize >= osize)
    {
        Bmemset(&newscript[0]+osize,0,(newsize-osize) * sizeof(uint8_t));
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((osize+7)>>3));
    }
    else
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((newsize+7)>>3));

    Bfree(bitptr);
    bitptr = newbitptr;
    if (apScript != newscript)
    {
        buildprint("Relocating compiled code from to 0x", hex((intptr_t)apScript), " to 0x", hex((intptr_t)newscript), "\n");
        apScript = newscript;
    }

    g_scriptSize = newsize;
    g_scriptPtr = apScript + (intptr_t)g_scriptPtr - oscript;

    if (g_caseScriptPtr)
        g_caseScriptPtr = apScript + (intptr_t)g_caseScriptPtr - oscript;

    for (i=(((newsize>=osize)?osize:newsize))-1; i>=0; i--)
        if (scriptptrs[i])
        {
            j = (intptr_t)apScript[i]+(intptr_t)&apScript[0];
            apScript[i] = j;
        }

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);

    Bfree(scriptptrs);
    return 0;
}

static inline int32_t ispecial(const char c)
{
    return (c == ' ' || c == 0x0d || c == '(' || c == ')' ||
        c == ',' || c == ';' || (c == 0x0a /*&& ++g_lineNumber*/));
}

static inline void C_NextLine(void)
{
    while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        textptr++;
}

static inline void C_SkipSpace(void)
{
    while (*textptr == ' ' || *textptr == '\t')
        textptr++;
}

static int32_t C_SkipComments(void)
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
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got comment.\n",g_scriptFileName,g_lineNumber);
                C_NextLine();
                continue;
            case '*': // beginning of a C style comment
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
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
                    g_parsingActorPtr = g_processingState = g_numBraces = 0;
                    g_errorCnt++;
                    continue;
                }

                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got end of comment block.\n",g_scriptFileName,g_lineNumber);

                textptr+=2;
                continue;
            default:
                C_ReportError(-1);
                initprintf("%s:%d: error: malformed comment.\n", g_scriptFileName, g_lineNumber);
                C_NextLine();
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
            return ((g_scriptPtr-apScript) > (g_scriptSize-32)) ? C_SetScriptSize(g_scriptSize<<1) : 0;
        }
    }
    while (1);
}

#define GetDefID(szGameLabel) hash_find(&h_gamevars,szGameLabel)
#define GetADefID(szGameLabel) hash_find(&h_arrays,szGameLabel)

static inline int32_t isaltok(const char c)
{
    return (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '\\' || c == '*' || c == '-' || c == '_' ||
            c == '.');
}

static inline int32_t C_IsLabelChar(const char c, int32_t const i)
{
    return (isalnum(c) || c == '_' || c == '*' || c == '?' || (i > 0 && (c == '+' || c == '-' || c == '.')));
}

static inline int32_t C_GetLabelNameID(const memberlabel_t *pLabel, hashtable_t const * const table, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int32_t l = hash_findcase(table, psz);
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
        label[(g_labelCnt<<6)+(i++)] = *(textptr++);

    label[(g_labelCnt<<6)+i] = 0;

    if (!(g_errorCnt|g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: label `%s'.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
}

static int32_t C_GetKeyword(void)
{
    int32_t i;
    char *temptextptr;

    C_SkipComments();

    temptextptr = textptr;

    if (*temptextptr == 0) // EOF
        return -2;

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
        return -2;

    l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (EDUKE32_PREDICT_TRUE((i = hash_find(&h_keywords,tempbuf)) >= 0))
    {
        if (i == CON_LEFTBRACE || i == CON_RIGHTBRACE || i == CON_NULLOP)
            *g_scriptPtr = i + (IFELSE_MAGIC<<12);
        else *g_scriptPtr = i + (g_lineNumber<<12);

        BITPTR_CLEAR(g_scriptPtr-apScript);
        textptr += l;
        g_scriptPtr++;

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
    int64_t x;
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
    int32_t i=0,f=0;

    C_SkipComments();

    if (!type && !g_labelsOnly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        BITPTR_CLEAR(g_scriptPtr-apScript);

        *g_scriptPtr++ = MAXGAMEVARS;

        if (tolower(textptr[1])=='x')  // hex constants
            *g_scriptPtr = parse_hex_constant(textptr+2);
        else
            *g_scriptPtr = parse_decimal_number();

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: constant %ld in place of gamevar.\n",
                       g_scriptFileName,g_lineNumber,(long)*g_scriptPtr);

        BITPTR_CLEAR(g_scriptPtr-apScript);
        g_scriptPtr++;
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
        f = (MAXGAMEVARS<<1);

        textptr++;
    }

    C_GetNextLabelName();

    if (EDUKE32_PREDICT_FALSE(!g_skipKeywordCheck && hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_ISAKEYWORD);
        return;
    }

    C_SkipComments(); //skip comments and whitespace
    if (*textptr == '[')     //read of array as a gamevar
    {
        int32_t labelNum = -1;

        f |= (MAXGAMEVARS<<2);
        textptr++;
        i=GetADefID(label+(g_labelCnt<<6));
        if (i < 0)
        {
            i=GetDefID(label+(g_labelCnt<<6));
            if ((unsigned) (i - g_structVarIDs) >= NUMQUICKSTRUCTS)
                i = -1;

            if (EDUKE32_PREDICT_FALSE(i < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return;
            }
            f &= ~(MAXGAMEVARS<<2); // not an array
            f |= (MAXGAMEVARS<<3);
        }

        BITPTR_CLEAR(g_scriptPtr-apScript);
        *g_scriptPtr++=(i|f);

        if ((f & (MAXGAMEVARS<<3)) && i - g_structVarIDs == STRUCT_USERDEF)
        {
            // userdef doesn't really have an array index
            while (*textptr != ']')
            {
                if (*textptr == 0xa)
                    break;
                if (!*textptr)
                    break;

                textptr++;
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *g_scriptPtr++ = 0; // help out the VM by inserting a dummy index
        }
        else
        {
            if (*textptr == ']')
            {
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++ = g_thisActorVarID;
            }
            else
                C_GetNextVarType(0);

            C_SkipComments();
        }

        if (EDUKE32_PREDICT_FALSE(*textptr != ']'))
        {
            g_errorCnt++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return;
        }
        textptr++;

        //writing arrays in this way is not supported because it would require too many changes to other code

        if (EDUKE32_PREDICT_FALSE(type))
        {
            g_errorCnt++;
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

            switch (i - g_structVarIDs)
            {
            case STRUCT_SPRITE:
                labelNum=C_GetLabelNameOffset(&h_actor,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_SECTOR:
                labelNum=C_GetLabelNameOffset(&h_sector,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_WALL:
                labelNum=C_GetLabelNameOffset(&h_wall,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_PLAYER:
                labelNum=C_GetLabelNameOffset(&h_player,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_ACTORVAR:
            case STRUCT_PLAYERVAR:
                labelNum=GetDefID(label+(g_labelCnt<<6));
                break;
            case STRUCT_TSPR:
                labelNum=C_GetLabelNameOffset(&h_tsprite,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_PROJECTILE:
            case STRUCT_THISPROJECTILE:
                labelNum=C_GetLabelNameOffset(&h_projectile,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_USERDEF:
                labelNum=C_GetLabelNameOffset(&h_userdef,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_INPUT:
                labelNum=C_GetLabelNameOffset(&h_input,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_TILEDATA:
                labelNum=C_GetLabelNameOffset(&h_tiledata,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            case STRUCT_PALDATA:
                labelNum=C_GetLabelNameOffset(&h_paldata,Bstrtolower(label+(g_labelCnt<<6)));
                break;
            }

            if (EDUKE32_PREDICT_FALSE(labelNum == -1))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAMEMBER);
                return;
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);

            switch (i - g_structVarIDs)
            {
            case STRUCT_SPRITE:
                *g_scriptPtr++=ActorLabels[labelNum].lId;

                if (ActorLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVarType(0);
                break;
            case STRUCT_SECTOR:
                *g_scriptPtr++=SectorLabels[labelNum].lId;
                break;
            case STRUCT_WALL:
                *g_scriptPtr++=WallLabels[labelNum].lId;
                break;
            case STRUCT_PLAYER:
                *g_scriptPtr++=PlayerLabels[labelNum].lId;

                if (PlayerLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVarType(0);
                break;
            case STRUCT_ACTORVAR:
            case STRUCT_PLAYERVAR:
                *g_scriptPtr++=labelNum;
                break;
            case STRUCT_TSPR:
                *g_scriptPtr++=TsprLabels[labelNum].lId;
                break;
            case STRUCT_PROJECTILE:
            case STRUCT_THISPROJECTILE:
                *g_scriptPtr++=ProjectileLabels[labelNum].lId;
                break;
            case STRUCT_USERDEF:
                *g_scriptPtr++=UserdefsLabels[labelNum].lId;
                break;
            case STRUCT_INPUT:
                *g_scriptPtr++=InputLabels[labelNum].lId;
                break;
            case STRUCT_TILEDATA:
                *g_scriptPtr++=TileDataLabels[labelNum].lId;
                break;
            case STRUCT_PALDATA:
                *g_scriptPtr++=PalDataLabels[labelNum].lId;
                break;
            }
        }
        return;
    }
//    initprintf("not an array");
    i=GetDefID(label+(g_labelCnt<<6));
    if (i<0)   //gamevar not found
    {
        if (!type && !g_labelsOnly)
        {
            //try looking for a define instead
            Bstrcpy(tempbuf,label+(g_labelCnt<<6));
            i = hash_find(&h_labels,tempbuf);
            if (EDUKE32_PREDICT_TRUE(i>=0))
            {
                if (EDUKE32_PREDICT_TRUE(labeltype[i] & LABEL_DEFINE))
                {
                    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
                        initprintf("%s:%d: debug: label `%s' in place of gamevar.\n",g_scriptFileName,g_lineNumber,label+(i<<6));
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr++ = MAXGAMEVARS;
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr++ = labelcode[i];
                    return;
                }
            }
            g_errorCnt++;
            C_ReportError(ERROR_NOTAGAMEVAR);
            return;
        }
        g_errorCnt++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        textptr++;
        return;

    }
    if (EDUKE32_PREDICT_FALSE(type == GAMEVAR_READONLY && aGameVars[i].flags & GAMEVAR_READONLY))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (EDUKE32_PREDICT_FALSE(aGameVars[i].flags & type))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }

    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: gamevar `%s'.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));

    BITPTR_CLEAR(g_scriptPtr-apScript);
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

    if (EDUKE32_PREDICT_FALSE(!g_skipKeywordCheck && hash_find(&h_keywords,tempbuf /*label+(g_numLabels<<6)*/)>=0))
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
            if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
            {
                char *gl = C_GetLabelType(labeltype[i]);
                initprintf("%s:%d: debug: %s label `%s'.\n",g_scriptFileName,g_lineNumber,gl,label+(i<<6));
                Bfree(gl);
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *(g_scriptPtr++) = labelcode[i];

            textptr += l;
            return labeltype[i];
        }

        BITPTR_CLEAR(g_scriptPtr-apScript);
        *(g_scriptPtr++) = 0;
        textptr += l;
        char *el = C_GetLabelType(type);
        char *gl = C_GetLabelType(labeltype[i]);
        C_ReportError(-1);
        initprintf("%s:%d: warning: expected %s, found %s.\n",g_scriptFileName,g_lineNumber,el,gl);
        g_warningCnt++;
        Bfree(el);
        Bfree(gl);
        return -1;  // valid label name, but wrong type
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) == 0 && *textptr != '-'))
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        g_errorCnt++;
        BITPTR_CLEAR(g_scriptPtr-apScript);
        *g_scriptPtr = 0;
        g_scriptPtr++;
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

    BITPTR_CLEAR(g_scriptPtr-apScript);

    if (textptr[0] == '0' && tolower(textptr[1])=='x')
        *g_scriptPtr = parse_hex_constant(textptr+2);
    else
        *g_scriptPtr = parse_decimal_number();

    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: constant %ld.\n",
                   g_scriptFileName,g_lineNumber,(long)*g_scriptPtr);

    g_scriptPtr++;

    textptr += l;

    return 0;   // literal value
}

static int32_t C_GetStructureIndexes(int32_t const labelsonly, hashtable_t const * const table)
{
    C_SkipComments();

    if (EDUKE32_PREDICT_FALSE(*textptr++ != '['))
    {
        g_errorCnt++;
        C_ReportError(ERROR_SYNTAXERROR);
        return -1;
    }

    C_SkipComments();

    if (*textptr == ']')
    {
        BITPTR_CLEAR(g_scriptPtr-apScript);
        *g_scriptPtr++ = g_thisActorVarID;
    }
    else
    {
        g_labelsOnly = labelsonly;
        C_GetNextVar();
        g_labelsOnly = 0;
    }

    textptr++;

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

    int32_t const labelNum = C_GetLabelNameOffset(table, Bstrtolower(label + (g_labelCnt << 6)));

    if (EDUKE32_PREDICT_FALSE(labelNum == -1))
    {
        g_errorCnt++;
        C_ReportError(ERROR_NOTAMEMBER);
        return -1;
    }

    return labelNum;
}

static inline int32_t C_IntPow2(int32_t const v)
{
    return ((v!=0) && (v&(v-1))==0);
}

static inline uint32_t C_Pow2IntLogBase2(int32_t const v)
{
    static const uint32_t b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                 0xFF00FF00, 0xFFFF0000
                                };

    uint32_t r = (v & b[0]) != 0;

    for (bssize_t i = 4; i > 0; i--)
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
        g_scriptPtr = lastScriptPtr + &apScript[0];
        g_ifElseAborted = 1;
        C_ReportError(-1);
        g_warningCnt++;
        initprintf("%s:%d: warning: malformed `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        return 1;
    }
    return 0;
}

static int32_t C_CheckEmptyBranch(int32_t tw, intptr_t lastScriptPtr)
{
    // ifrnd and the others actually do something when the condition is executed
    if ((Bstrncmp(VM_GetKeywordForID(tw), "if", 2) && tw != CON_ELSE) ||
            tw == CON_IFRND || tw == CON_IFHITWEAPON || tw == CON_IFCANSEE || tw == CON_IFCANSEETARGET ||
            tw == CON_IFPDISTL || tw == CON_IFPDISTG || tw == CON_IFGOTWEAPONCE)
    {
        g_ifElseAborted = 0;
        return 0;
    }

    if ((*(g_scriptPtr) & VM_INSTMASK) != CON_NULLOP || *(g_scriptPtr)>>12 != IFELSE_MAGIC)
        g_ifElseAborted = 0;

    if (EDUKE32_PREDICT_FALSE(g_ifElseAborted))
    {
        C_ReportError(-1);
        g_warningCnt++;
        g_scriptPtr = lastScriptPtr + &apScript[0];
        initprintf("%s:%d: warning: empty `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        *(g_scriptPtr) = (CON_NULLOP + (IFELSE_MAGIC<<12));
        return 1;
    }
    return 0;
}

static int32_t C_CountCaseStatements()
{
    char *temptextptr = textptr;
    int32_t temp_ScriptLineNumber = g_lineNumber;
    intptr_t scriptoffset = (unsigned)(g_scriptPtr-apScript);
    intptr_t caseoffset = (unsigned)(g_caseScriptPtr-apScript);
//    int32_t i;

    g_numCases=0;
    g_caseScriptPtr=NULL;
    //Bsprintf(g_szBuf,"CSS: %.12s",textptr);
    //AddLog(g_szBuf);
    C_ParseCommand(1);
    // since we processed the endswitch, we need to re-increment g_checkingSwitch
    g_checkingSwitch++;

    textptr=temptextptr;
    g_scriptPtr = (intptr_t *)(apScript+scriptoffset);

    g_lineNumber = temp_ScriptLineNumber;

    int32_t const lCount=g_numCases;
    g_numCases=0;
    g_caseScriptPtr = (intptr_t *)(apScript+caseoffset);
    g_numCases = 0;
    return lCount;
}

static void C_Include(const char *confile)
{
    int32_t fp = kopen4loadfrommod(confile,g_loadFromGroupOnly);

    if (EDUKE32_PREDICT_FALSE(fp < 0))
    {
        g_errorCnt++;
        initprintf("%s:%d: error: could not find file `%s'.\n",g_scriptFileName,g_lineNumber,confile);
        return;
    }

    int32_t j = kfilelength(fp);

    char *mptr = (char *)Xmalloc(j+1);

    initprintf("Including: %s (%d bytes)\n",confile, j);

    kread(fp, mptr, j);
    kclose(fp);
    mptr[j] = 0;

    if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
        textptr++;

    char *origtptr = textptr;
    char parentScriptFileName[255];

    Bstrcpy(parentScriptFileName, g_scriptFileName);
    Bstrcpy(g_scriptFileName, confile);

    int32_t temp_ScriptLineNumber = g_lineNumber;
    g_lineNumber = 1;

    int32_t temp_ifelse_check = g_checkingIfElse;
    g_checkingIfElse = 0;

    textptr = mptr;

    C_SkipComments();
    C_ParseCommand(1);

    Bstrcpy(g_scriptFileName, parentScriptFileName);

    g_totalLines += g_lineNumber;
    g_lineNumber = temp_ScriptLineNumber;
    g_checkingIfElse = temp_ifelse_check;

    textptr = origtptr;

    Bfree(mptr);
}
#endif  // !defined LUNATIC

#ifdef _WIN32
static void check_filename_case(const char *fn)
{
    int32_t fp;
    if ((fp = kopen4loadfrommod(fn, g_loadFromGroupOnly)) >= 0)
        kclose(fp);
}
#else
static void check_filename_case(const char *fn) { UNREFERENCED_PARAMETER(fn); }
#endif

void G_DoGameStartup(const int32_t *params)
{
    int j = 0;

    ud.const_visibility               = params[j++];
    g_impactDamage                    = params[j++];
    g_player[0].ps->max_shield_amount = params[j++];
    g_player[0].ps->max_player_health = g_player[0].ps->max_shield_amount;
    g_maxPlayerHealth                 = g_player[0].ps->max_player_health;
    g_startArmorAmount                = params[j++];
    g_actorRespawnTime                = params[j++];
    g_itemRespawnTime                 = (g_scriptVersion >= 11) ? params[j++] : g_actorRespawnTime;

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

    g_player[0].ps->max_ammo_amount[PISTOL_WEAPON]     = params[j++];
    g_player[0].ps->max_ammo_amount[SHOTGUN_WEAPON]    = params[j++];
    g_player[0].ps->max_ammo_amount[CHAINGUN_WEAPON]   = params[j++];
    g_player[0].ps->max_ammo_amount[RPG_WEAPON]        = params[j++];
    g_player[0].ps->max_ammo_amount[HANDBOMB_WEAPON]   = params[j++];
    g_player[0].ps->max_ammo_amount[SHRINKER_WEAPON]   = params[j++];
    g_player[0].ps->max_ammo_amount[DEVISTATOR_WEAPON] = params[j++];
    g_player[0].ps->max_ammo_amount[TRIPBOMB_WEAPON]   = params[j++];

    if (g_scriptVersion >= 13)
    {
        g_player[0].ps->max_ammo_amount[FREEZE_WEAPON] = params[j++];

        if (g_scriptVersion >= 14)
            g_player[0].ps->max_ammo_amount[GROW_WEAPON] = params[j++];

        g_damageCameras     = params[j++];
        g_numFreezeBounces  = params[j++];
        g_freezerSelfDamage = params[j++];

        if (g_scriptVersion >= 14)
        {
            g_deleteQueueSize   = clamp(params[j++], 0, 1024);
            g_tripbombLaserMode = params[j++];
        }
    }
}

void C_DefineMusic(int volumeNum, int levelNum, const char *fileName)
{
    Bassert((unsigned)volumeNum < MAXVOLUMES+1);
    Bassert((unsigned)levelNum < MAXLEVELS);

    map_t *const pMapInfo = &g_mapInfo[(MAXLEVELS*volumeNum)+levelNum];

    Bfree(pMapInfo->musicfn);
    pMapInfo->musicfn = dup_filename(fileName);
    check_filename_case(pMapInfo->musicfn);
}

#ifdef LUNATIC
void C_DefineSound(int32_t sndidx, const char *fn, int32_t args[5])
{
    Bassert((unsigned)sndidx < MAXSOUNDS);

    {
        sound_t *const snd = &g_sounds[sndidx];

        Bfree(snd->filename);
        snd->filename = dup_filename(fn);
        check_filename_case(snd->filename);

        snd->ps = args[0];
        snd->pe = args[1];
        snd->pr = args[2];
        snd->m = args[3] & ~SF_ONEINST_INTERNAL;
        if (args[3] & SF_LOOP)
            snd->m |= SF_ONEINST_INTERNAL;
        snd->vo = args[4];

        if (sndidx > g_maxSoundPos)
            g_maxSoundPos = sndidx;
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

        Bfree(map->filename);
        map->filename = dup_filename(fn);

        // TODO: truncate to 32 chars?
        Bfree(map->name);
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
int32_t g_numProjectiles = 0;

EDUKE32_STATIC_ASSERT(sizeof(projectile_t) == sizeof(DefaultProjectile));

void C_AllocProjectile(int32_t j)
{
    g_tile[j].proj = (projectile_t *)Xmalloc(2 * sizeof(projectile_t));
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
        g_numProjectiles += 2;
    }

    projectile_t * const proj = g_tile[j].proj;

    switch (what)
    {
    case PROJ_WORKSLIKE:
        proj->workslike = val; break;
    case PROJ_SPAWNS:
        proj->spawns = val; break;
    case PROJ_SXREPEAT:
        proj->sxrepeat = val; break;
    case PROJ_SYREPEAT:
        proj->syrepeat = val; break;
    case PROJ_SOUND:
        proj->sound = val; break;
    case PROJ_ISOUND:
        proj->isound = val; break;
    case PROJ_VEL:
        proj->vel = val; break;
    case PROJ_EXTRA:
        proj->extra = val; break;
    case PROJ_DECAL:
        proj->decal = val; break;
    case PROJ_TRAIL:
        proj->trail = val; break;
    case PROJ_TXREPEAT:
        proj->txrepeat = val; break;
    case PROJ_TYREPEAT:
        proj->tyrepeat = val; break;
    case PROJ_TOFFSET:
        proj->toffset = val; break;
    case PROJ_TNUM:
        proj->tnum = val; break;
    case PROJ_DROP:
        proj->drop = val; break;
    case PROJ_CSTAT:
        proj->cstat = val; break;
    case PROJ_CLIPDIST:
        proj->clipdist = val; break;
    case PROJ_SHADE:
        proj->shade = val; break;
    case PROJ_XREPEAT:
        proj->xrepeat = val; break;
    case PROJ_YREPEAT:
        proj->yrepeat = val; break;
    case PROJ_PAL:
        proj->pal = val; break;
    case PROJ_EXTRA_RAND:
        proj->extra_rand = val; break;
    case PROJ_HITRADIUS:
        proj->hitradius = val; break;
    case PROJ_MOVECNT:
        proj->movecnt = val; break;
    case PROJ_OFFSET:
        proj->offset = val; break;
    case PROJ_BOUNCES:
        proj->bounces = val; break;
    case PROJ_BSOUND:
        proj->bsound = val; break;
    case PROJ_RANGE:
        proj->range = val; break;
    case PROJ_FLASH_COLOR:
        proj->flashcolor = val; break;
    case PROJ_USERDATA:
        proj->userdata = val; break;
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
    for (bssize_t i = 0; i < 128; i++) C_AllocQuote(i);

#ifdef EDUKE32_TOUCH_DEVICES
    apStrings[QUOTE_DEAD] = 0;
#else
    char const * const replacement_USE = "USE";
    if (!Bstrstr(apStrings[QUOTE_DEAD], replacement_USE))
    {
        C_ReplaceQuoteSubstring(QUOTE_DEAD, "SPACE", replacement_USE);
        C_ReplaceQuoteSubstring(QUOTE_DEAD, "OPEN", replacement_USE);
        C_ReplaceQuoteSubstring(QUOTE_DEAD, "ANY BUTTON", replacement_USE);
    }
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

    char temp[BMAX_PATH];
    struct Bstat st;

    int32_t fullscreen = ud.config.ScreenMode;
    int32_t xdim = ud.config.ScreenWidth, ydim = ud.config.ScreenHeight, bpp = ud.config.ScreenBPP;
    int32_t usemouse = ud.config.UseMouse, usejoy = ud.config.UseJoystick;
#ifdef USE_OPENGL
    int32_t glrm = glrendmode;
#endif

    if (Bstrcmp(g_setupFileName, SETUPFILENAME) != 0) // set to something else via -cfg
        return;

    if (Bstat(g_modDir, &st) < 0)
    {
        if (errno == ENOENT)     // path doesn't exist
        {
            if (Bmkdir(g_modDir, S_IRWXU) < 0)
            {
                OSD_Printf("Failed to create configuration file directory %s\n", g_modDir);
                return;
            }
            else OSD_Printf("Created configuration file directory %s\n", g_modDir);
        }
        else
        {
            // another type of failure
            return;
        }
    }
    else if ((st.st_mode & S_IFDIR) != S_IFDIR)
    {
        // directory isn't a directory
        return;
    }

    // XXX: Back up 'cfgname' as it may be the global 'tempbuf'.
    Bstrncpyz(temp, cfgname, sizeof(temp));
    CONFIG_WriteSetup(1);

    if (g_modDir[0] != '/')
        Bsnprintf(g_setupFileName, sizeof(g_setupFileName), "%s/%s", g_modDir, temp);
    else
        Bstrncpyz(g_setupFileName, temp, sizeof(g_setupFileName));

    initprintf("Using config file \"%s\".\n", g_setupFileName);

    CONFIG_ReadSetup();

    ud.config.ScreenMode = fullscreen;
    ud.config.ScreenWidth = xdim;
    ud.config.ScreenHeight = ydim;
    ud.config.ScreenBPP = bpp;
    ud.config.UseMouse = usemouse;
    ud.config.UseJoystick = usejoy;
#ifdef USE_OPENGL
    glrendmode = glrm;
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
    BITPTR_CLEAR(g_scriptPtr-apScript);
    *g_scriptPtr++ = value;
}

static void C_FillEventBreakStackWithJump(intptr_t *breakPtr, intptr_t destination)
{
    while (breakPtr)
    {
        breakPtr = apScript + (intptr_t)breakPtr;
        intptr_t const tempPtr = *breakPtr;
        BITPTR_CLEAR(breakPtr-apScript);
        *breakPtr = destination;
        breakPtr = (intptr_t *)tempPtr;
    }
}

static int32_t C_ParseCommand(int32_t loop)
{
    int32_t i, j=0, k=0, tw;

    do
    {
        if (EDUKE32_PREDICT_FALSE(g_errorCnt > 63 || (*textptr == '\0') || (*(textptr+1) == '\0') || C_SkipComments()))
            return 1;

        if (EDUKE32_PREDICT_FALSE(g_scriptDebug))
            C_ReportError(-1);

        int32_t const otw = g_lastKeyword;

        switch ((g_lastKeyword = tw = C_GetNextKeyword()))
        {
        default:
        case -1:
        case -2:
            return 1; //End
        case CON_DEFSTATE:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
                continue;
            }
            goto DO_DEFSTATE;
        case CON_STATE:
            if (!g_parsingActorPtr && g_processingState == 0)
            {
DO_DEFSTATE:
                C_GetNextLabelName();
                g_scriptPtr--;
                labelcode[g_labelCnt] = g_scriptPtr-apScript;
                labeltype[g_labelCnt] = LABEL_STATE;

                g_processingState = 1;
                Bsprintf(g_szCurrentBlockName,"%s",label+(g_labelCnt<<6));

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_gamevars,label+(g_labelCnt<<6))>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                g_labelCnt++;
                continue;
            }

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE((j = hash_find(&h_labels,label+(g_labelCnt<<6))) < 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: state `%s' not found.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
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
                Bfree(gl);
                *(g_scriptPtr-1) = CON_NULLOP; // get rid of the state, leaving a nullop to satisfy if conditions
                BITPTR_CLEAR(g_scriptPtr-apScript-1);
                continue;  // valid label name, but wrong type
            }

            if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                initprintf("%s:%d: debug: state label `%s'.\n", g_scriptFileName, g_lineNumber, label+(j<<6));
            *g_scriptPtr = (intptr_t) (apScript+labelcode[j]);

            // 'state' type labels are always script addresses, as far as I can see
            BITPTR_SET(g_scriptPtr-apScript);

            g_scriptPtr++;
            continue;

        case CON_ENDS:
            if (EDUKE32_PREDICT_FALSE(g_processingState == 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `ends' without open `state'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }
            //            else
            {
                if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
                {
                    C_ReportError(ERROR_OPENBRACKET);
                    g_errorCnt++;
                }
                else if (EDUKE32_PREDICT_FALSE(g_numBraces < 0))
                {
                    C_ReportError(ERROR_CLOSEBRACKET);
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
            }
            continue;

        case CON_SETTHISPROJECTILE:
        case CON_SETPROJECTILE:
        case CON_GETTHISPROJECTILE:
        case CON_GETPROJECTILE:
            {
                int32_t const labelNum = C_GetStructureIndexes(tw == CON_SETTHISPROJECTILE || tw == CON_GETTHISPROJECTILE, &h_projectile);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=ProjectileLabels[labelNum].lId;

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
                C_GetNextLabelName();
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                C_GetNextValue(LABEL_DEFINE);
                j = 0;
                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);
                C_FinishBitOr(j);
                g_scriptPtr -= 3; // we complete the process anyways just to skip past the fucked up section
                continue;
            }

            g_scriptPtr--;

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords, label+(g_labelCnt<<6))>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_VARMASKSKEYWORD);
                hash_delete(&h_keywords, label+(g_labelCnt<<6));
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

            Gv_NewVar(label+(g_labelCnt<<6), defaultValue, varFlags);
            continue;
        }

        case CON_GAMEARRAY:
        {
            if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) || (*textptr == '-')))
            {
                C_GetNextLabelName();
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                C_GetNextValue(LABEL_DEFINE);
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr -= 2; // we complete the process anyways just to skip past the fucked up section
                continue;
            }
            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_ARRAYMASKSKEYWORD);
                hash_delete(&h_keywords, label+(g_labelCnt<<6));
            }

            i = hash_find(&h_gamevars,label+(g_labelCnt<<6));
            if (EDUKE32_PREDICT_FALSE(i>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            C_GetNextValue(LABEL_DEFINE);

            char const * const arrayName = label+(g_labelCnt<<6);
            int32_t arrayFlags = 0;

            while (C_GetKeyword() == -1)
                C_BitOrNextValue(&arrayFlags);

            C_FinishBitOr(arrayFlags);

            arrayFlags = *(g_scriptPtr-1);
            g_scriptPtr--;

            Gv_NewArray(arrayName, NULL, *(g_scriptPtr-1), arrayFlags);

            g_scriptPtr -= 2; // no need to save in script...
            continue;
        }

        case CON_DEFINE:
            {
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                C_GetNextValue(LABEL_DEFINE);

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (i>=0)
                {
                    // if (i >= g_numDefaultLabels)

                    if (EDUKE32_PREDICT_FALSE(labelcode[i] != *(g_scriptPtr-1)))
                    {
                        g_warningCnt++;
                        initprintf("%s:%d: warning: ignored redefinition of `%s' to %d (old: %d).\n",g_scriptFileName,
                                   g_lineNumber,label+(g_labelCnt<<6), (int32_t)(*(g_scriptPtr-1)), labelcode[i]);
                    }
                }
                else
                {
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    labeltype[g_labelCnt] = LABEL_DEFINE;
                    labelcode[g_labelCnt++] = *(g_scriptPtr-1);
                    if (*(g_scriptPtr-1) >= 0 && *(g_scriptPtr-1) < MAXTILES && g_dynamicTileMapping)
                        G_ProcessDynamicTileMapping(label+((g_labelCnt-1)<<6),*(g_scriptPtr-1));
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
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++ = 0;
                j--;
            }
            continue;

        case CON_MOVE:
            if (g_parsingActorPtr || g_processingState)
            {
                if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                {
                    C_ReportError(-1);
                    BITPTR_CLEAR(g_scriptPtr-apScript-1);
                    *(g_scriptPtr-1) = 0;
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

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_gamevars,label+(g_labelCnt<<6))>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                if (EDUKE32_PREDICT_FALSE((i = hash_find(&h_labels,label+(g_labelCnt<<6))) >= 0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate move `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
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
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr = 0;
                    g_scriptPtr++;
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
            if (g_parsingActorPtr || g_processingState)
            {
                C_GetNextValue(LABEL_AI);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate ai `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_AI;
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
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
                            (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                        {
                            C_ReportError(-1);
                            BITPTR_CLEAR(g_scriptPtr-apScript-1);
                            *(g_scriptPtr-1) = 0;
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
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr = 0;
                    g_scriptPtr++;
                }
            }
            continue;

        case CON_ACTION:
            if (g_parsingActorPtr || g_processingState)
            {
                C_GetNextValue(LABEL_ACTION);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();
                // Check to see it's already defined

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_gamevars,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    C_ReportError(WARNING_NAMEMATCHESVAR);
                }

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate action `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_ACTION;
                    labelcode[g_labelCnt] = g_scriptPtr-apScript;
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    g_labelCnt++;
                }

                for (j=ACTION_PARAM_COUNT-1; j>=0; j--)
                {
                    if (C_GetKeyword() != -1) break;
                    C_GetNextValue(LABEL_DEFINE);
                }
                for (k=j; k>=0; k--)
                {
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *(g_scriptPtr++) = 0;
                }
            }
            continue;

        case CON_ACTOR:
        case CON_USERACTOR:
        case CON_EVENTLOADACTOR:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_parsingActorPtr = g_scriptPtr - apScript;

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

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;

            if (EDUKE32_PREDICT_FALSE((unsigned)*g_scriptPtr >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                continue;
            }

            if (tw == CON_EVENTLOADACTOR)
            {
                g_tile[*g_scriptPtr].loadPtr = apScript + g_parsingActorPtr;
                g_checkingIfElse = 0;
                continue;
            }

            g_tile[*g_scriptPtr].execPtr = apScript + g_parsingActorPtr;

            if (tw == CON_USERACTOR)
            {
                if (j & 1)
                    g_tile[*g_scriptPtr].flags |= SFLAG_BADGUY;

                if (j & 2)
                    g_tile[*g_scriptPtr].flags |= (SFLAG_BADGUY|SFLAG_BADGUYSTAYPUT);

                if (j & 4)
                    g_tile[*g_scriptPtr].flags |= SFLAG_ROTFIXED;
            }

            for (j=0; j<4; j++)
            {
                BITPTR_CLEAR(g_parsingActorPtr+j);
                *((apScript+j)+g_parsingActorPtr) = 0;
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
                            BITPTR_CLEAR(g_scriptPtr-apScript);
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
                        // XXX: LABEL_MOVE|LABEL_DEFINE, what is this shit? compatibility?
                        // yep, it sure is :(
                        if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                        {
                            C_ReportError(-1);
                            BITPTR_CLEAR(g_scriptPtr-apScript-1);
                            *(g_scriptPtr-1) = 0;
                            initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                            g_warningCnt++;
                        }
                        break;
                    }
                    if (*(g_scriptPtr-1) >= (intptr_t)&apScript[0] && *(g_scriptPtr-1) < (intptr_t)&apScript[g_scriptSize])
                        BITPTR_SET(g_parsingActorPtr+j);
                    else BITPTR_CLEAR(g_parsingActorPtr+j);
                    *((apScript+j)+g_parsingActorPtr) = *(g_scriptPtr-1);
                }
            }
            g_checkingIfElse = 0;
            continue;

        case CON_ONEVENT:
        case CON_APPENDEVENT:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_parsingEventPtr = g_parsingActorPtr = g_scriptPtr - apScript;

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
            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXEVENTS-1))
            {
                initprintf("%s:%d: error: invalid event ID.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                continue;
            }
            // if event has already been declared then store previous script location
            if (!apScriptEvents[j])
            {
                apScriptEvents[j] = g_parsingEventPtr;
            }
            else if (tw == CON_ONEVENT)
            {
                previous_event = apScriptEvents[j];
                apScriptEvents[j] = g_parsingEventPtr;
            }
            else // if (tw == CON_APPENDEVENT)
            {
                intptr_t *previous_event_end = apScript + apScriptGameEventEnd[j];
                BITPTR_CLEAR(previous_event_end-apScript);
                *(previous_event_end++) = CON_JUMP | (g_lineNumber << 12);
                BITPTR_CLEAR(previous_event_end-apScript);
                *(previous_event_end++) = MAXGAMEVARS;
                C_FillEventBreakStackWithJump((intptr_t *)*previous_event_end, g_parsingEventPtr);
                BITPTR_CLEAR(previous_event_end-apScript);
                *(previous_event_end++) = g_parsingEventPtr;
            }

            g_checkingIfElse = 0;

            continue;

        case CON_QSPRINTF:
            C_GetManyVars(2);

            j = 0;

            while (C_GetKeyword() == -1 && j < 32)
                C_GetNextVar(), j++;

            *g_scriptPtr++ = CON_NULLOP + (g_lineNumber<<12);
            continue;

        case CON_CSTAT:
            C_GetNextValue(LABEL_DEFINE);

            if (EDUKE32_PREDICT_FALSE(*(g_scriptPtr-1) == 32767))
            {
                *(g_scriptPtr-1) = 32768;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat 32767, using 32768 instead.\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
            else if (EDUKE32_PREDICT_FALSE((*(g_scriptPtr-1) & 48) == 48))
            {
                i = *(g_scriptPtr-1);
                *(g_scriptPtr-1) ^= 48;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat %d, using %d instead.\n",g_scriptFileName,g_lineNumber,i,(int32_t)(*(g_scriptPtr-1)));
                g_warningCnt++;
            }
            continue;

        case CON_SCREENPAL:
            C_GetManyVars(4);
            continue;

        case CON_HITRADIUSVAR:
        case CON_DRAWLINE256:
            C_GetManyVars(5);
            continue;

        case CON_DRAWLINERGB:
            C_GetManyVars(6);
            continue;

        case CON_HITRADIUS:
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            fallthrough__;
        case CON_ADDAMMO:
        case CON_ADDWEAPON:
        case CON_SIZETO:
        case CON_SIZEAT:
        case CON_DEBRIS:
        case CON_ADDINVENTORY:
        case CON_GUTS:
            C_GetNextValue(LABEL_DEFINE);
            fallthrough__;
        case CON_STRENGTH:
        case CON_ADDPHEALTH:
        case CON_SPAWN:
        case CON_COUNT:
        case CON_ENDOFGAME:
        case CON_ENDOFLEVEL:
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
        case CON_ANGOFF:
        case CON_QUOTE:
        case CON_SOUND:
        case CON_GLOBALSOUND:
        case CON_SOUNDONCE:
        case CON_STOPSOUND:
            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_ELSE:
            {
                if (EDUKE32_PREDICT_FALSE(!g_checkingIfElse))
                {
                    g_scriptPtr--;
                    intptr_t *tempscrptr = g_scriptPtr;
                    g_warningCnt++;
                    C_ReportError(-1);

                    initprintf("%s:%d: warning: found `else' with no `if'.\n", g_scriptFileName, g_lineNumber);

                    if (C_GetKeyword() == CON_LEFTBRACE)
                    {
                        C_GetNextKeyword();
                        g_numBraces++;

                        C_ParseCommand(1);
                    }
                    else C_ParseCommand(0);

                    g_scriptPtr = tempscrptr;

                    continue;
                }

                intptr_t const lastScriptPtr = g_scriptPtr - apScript - 1;

                g_ifElseAborted = 0;
                g_checkingIfElse--;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t const offset = (unsigned) (g_scriptPtr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = (intptr_t *) apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                continue;
            }

        case CON_SETSECTOR:
        case CON_GETSECTOR:
            {
                int32_t const labelNum = C_GetStructureIndexes(1, &h_sector);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=SectorLabels[labelNum].lId;

                C_GetNextVarType((tw == CON_GETSECTOR) ? GAMEVAR_READONLY : 0);
                continue;
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
                continue;
            }

        case CON_SETWALL:
        case CON_GETWALL:
            {
                int32_t const labelNum = C_GetStructureIndexes(1, &h_wall);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=WallLabels[labelNum].lId;

                C_GetNextVarType((tw == CON_GETWALL) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETPLAYER:
        case CON_GETPLAYER:
            {
                int32_t const labelNum = C_GetStructureIndexes(1, &h_player);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=PlayerLabels[labelNum].lId;

                if (PlayerLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVarType((tw == CON_GETPLAYER) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETINPUT:
        case CON_GETINPUT:
            {
                int32_t const labelNum = C_GetStructureIndexes(1, &h_input);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=InputLabels[labelNum].lId;

                C_GetNextVarType(tw == CON_GETINPUT ? GAMEVAR_READONLY : 0);
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

                int32_t const labelNum=C_GetLabelNameID(UserdefsLabels,&h_userdef,Bstrtolower(label+(g_labelCnt<<6)));

                if (EDUKE32_PREDICT_FALSE(labelNum == -1))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_NOTAMEMBER);
                    continue;
                }
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=labelNum;

                C_GetNextVarType((tw == CON_GETUSERDEF) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_SETACTORVAR:
        case CON_SETPLAYERVAR:
        case CON_GETACTORVAR:
        case CON_GETPLAYERVAR:
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
                    continue;
                }

                /// now pointing at 'xxx'

                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i=GetDefID(label+(g_labelCnt<<6));

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
                            initprintf("%s:%d: error: variable `%s' is not per-actor.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                            continue;
                        }
                        break;
                case CON_SETPLAYERVAR:
                        if (EDUKE32_PREDICT_FALSE(!(aGameVars[i].flags & GAMEVAR_PERPLAYER)))
                        {
                            g_errorCnt++;
                            C_ReportError(-1);
                            initprintf("%s:%d: error: variable `%s' is not per-player.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                            continue;
                        }
                        break;
                }

                BITPTR_CLEAR(g_scriptPtr-apScript);
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
                continue;
            }

        case CON_SETACTOR:
        case CON_GETACTOR:
            {
                int32_t const labelNum = C_GetStructureIndexes(1, &h_actor);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=ActorLabels[labelNum].lId;

                if (ActorLabels[labelNum].flags & LABEL_HASPARM2)
                    C_GetNextVar();

                C_GetNextVarType((tw == CON_GETACTOR) ? GAMEVAR_READONLY : 0);
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
                int32_t const labelNum = C_GetStructureIndexes(1, &h_tsprite);

                if (labelNum == -1)
                    continue;

                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=TsprLabels[labelNum].lId;

                C_GetNextVarType((tw == CON_GETTSPR) ? GAMEVAR_READONLY : 0);
                continue;
            }

        case CON_RANDVARVAR:
        case CON_SETVARVAR:
        case CON_ADDVARVAR:
        case CON_SUBVARVAR:
        case CON_MULVARVAR:
        case CON_DIVVARVAR:
        case CON_MODVARVAR:
        case CON_ANDVARVAR:
        case CON_ORVARVAR:
        case CON_XORVARVAR:
        case CON_SHIFTVARVARL:
        case CON_SHIFTVARVARR:
        case CON_DISPLAYRANDVARVAR:
        case CON_SIN:
        case CON_COS:
        case CON_QSTRLEN:
        case CON_HEADSPRITESTAT:
        case CON_PREVSPRITESTAT:
        case CON_NEXTSPRITESTAT:
        case CON_HEADSPRITESECT:
        case CON_PREVSPRITESECT:
        case CON_NEXTSPRITESECT:
        case CON_SECTOROFWALL:
            C_GetNextVarType(GAMEVAR_READONLY);
            fallthrough__;
        case CON_ADDLOGVAR:
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
        case CON_JUMP:
        case CON_CMENU:
        case CON_SOUNDVAR:
        case CON_GLOBALSOUNDVAR:
        case CON_STOPSOUNDVAR:
        case CON_SCREENSOUND:
        case CON_SOUNDONCEVAR:
        case CON_ANGOFFVAR:
        case CON_CHECKAVAILWEAPON:
        case CON_CHECKAVAILINVEN:
        case CON_GUNIQHUDID:
        case CON_SAVEGAMEVAR:
        case CON_USERQUOTE:
        case CON_ECHO:
        case CON_STARTTRACKVAR:
        case CON_CLEARMAPSTATE:
        case CON_ACTIVATECHEAT:
        case CON_SETGAMEPALETTE:
        case CON_SECTSETINTERPOLATION:
        case CON_SECTCLEARINTERPOLATION:
        case CON_SETACTORANGLE:
        case CON_SETPLAYERANGLE:
        case CON_SETMUSICPOSITION:
        case CON_STARTCUTSCENE:
        case CON_RESETPLAYERFLAGS:
        case CON_MOVESECTOR:
            C_GetNextVar();
            continue;

        case CON_SQRT:
            C_GetNextVar();
            fallthrough__;
        case CON_FINDPLAYER:
        case CON_FINDOTHERPLAYER:
        case CON_DISPLAYRAND:
        case CON_READGAMEVAR:
        case CON_GETANGLETOTARGET:
        case CON_GETACTORANGLE:
        case CON_GETPLAYERANGLE:
        case CON_GETTICKS:
        case CON_GETCURRADDRESS:
        case CON_GETMUSICPOSITION:
        case CON_KLABS:
        case CON_INV:
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_CLAMP:
        case CON_GETCLOSESTCOL:
        case CON_CALCHYPOTENUSE:
            C_GetNextVarType(GAMEVAR_READONLY);
            fallthrough__;
        case CON_CHANGESPRITESTAT:
        case CON_CHANGESPRITESECT:
        case CON_ZSHOOTVAR:
        case CON_EZSHOOTVAR:
        case CON_GETPNAME:
        case CON_STARTLEVEL:
        case CON_QSTRCAT:
        case CON_QSTRCPY:
        case CON_QGETSYSSTR:
        case CON_ACTORSOUND:
        case CON_STOPACTORSOUND:
        case CON_STARTTRACKSLOT:
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
                initprintf("Using dynamic sound remapping\n");

            g_dynamicSoundMapping = 1;
#else
            {
                initprintf("%s:%d: warning: dynamic sound remapping is disabled in this build\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
#endif
            continue;

        case CON_RANDVAR:
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

                C_GetNextVarType(GAMEVAR_READONLY);

                C_GetNextValue(LABEL_DEFINE); // the number to check against...

                if (tw == CON_DIVVAR || tw == CON_MULVAR)
                {
                    int32_t i = *(g_scriptPtr-1);
                    j = klabs(i);

                    if (i == -1)
                    {
                        *inst = CON_INV+(g_lineNumber<<12);
                        g_scriptPtr--;
                        continue;
                    }

                    if (C_IntPow2(j))
                    {
                        *inst = ((tw == CON_DIVVAR) ? CON_SHIFTVARR : CON_SHIFTVARL)+(g_lineNumber<<12);
                        *(g_scriptPtr-1) = C_Pow2IntLogBase2(j);
                        //                    initprintf("%s:%d: replacing multiply/divide with shift\n",g_szScriptFileName,g_lineNumber);

                        if (i == j)
                            continue;

                        *g_scriptPtr++ = CON_INV+(g_lineNumber<<12);
                        textptr = tptr;
                        C_GetNextVarType(GAMEVAR_READONLY);
                        C_GetNextValue(LABEL_DEFINE);
                        g_scriptPtr--;
                        //                    initprintf("%s:%d: adding inversion\n",g_szScriptFileName,g_lineNumber);
                    }
                }
            }
            continue;

        case CON_WRITEARRAYTOFILE:
        case CON_READARRAYFROMFILE:
            C_GetNextLabelName();
            i=GetADefID(label+(g_labelCnt<<6));
            if (EDUKE32_PREDICT_FALSE(i < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return 1;
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *g_scriptPtr++=i;

            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_COPY:
            C_GetNextLabelName();
            i=GetADefID(label+(g_labelCnt<<6));
            if (EDUKE32_PREDICT_FALSE(i < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return 1;
            }
            BITPTR_CLEAR(g_scriptPtr-apScript);
            *g_scriptPtr++=i;
            C_SkipComments();// skip comments and whitespace
            if (EDUKE32_PREDICT_FALSE(*textptr != '['))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNO);
                return 1;
            }
            textptr++;
            C_GetNextVar();
            C_SkipComments();// skip comments and whitespace
            if (EDUKE32_PREDICT_FALSE(*textptr != ']'))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNC);
                return 1;
            }
            textptr++;
            fallthrough__;
        case CON_SETARRAY:
            C_GetNextLabelName();
            i=GetADefID(label+(g_labelCnt<<6));
            if (EDUKE32_PREDICT_FALSE(i < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return 1;
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *g_scriptPtr++=i;

            if (EDUKE32_PREDICT_FALSE(aGameArrays[i].flags & GAMEARRAY_READONLY))
            {
                C_ReportError(ERROR_ARRAYREADONLY);
                g_errorCnt++;
                return 1;
            }

            C_SkipComments();// skip comments and whitespace
            if (EDUKE32_PREDICT_FALSE(*textptr != '['))
            {
                g_errorCnt++;
                C_ReportError(ERROR_GAMEARRAYBNO);
                return 1;
            }
            textptr++;
            C_GetNextVar();
            C_SkipComments();// skip comments and whitespace
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
        case CON_RESIZEARRAY:
            C_GetNextLabelName();
            i=GetADefID(label+(g_labelCnt<<6));
            if (EDUKE32_PREDICT_FALSE(i < 0))
            {
                g_errorCnt++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return 1;
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *g_scriptPtr++ = i;
            if (tw==CON_RESIZEARRAY && (aGameArrays[i].flags & (GAMEARRAY_READONLY|GAMEARRAY_SYSTEM)))
            {
                C_ReportError(-1);
                initprintf("can't resize system array `%s'.", label+(g_labelCnt<<6));
                return 1;
            }

            C_SkipComments();
            C_GetNextVarType(tw==CON_GETARRAYSIZE ? GAMEVAR_READONLY : 0);
            continue;

        case CON_SMAXAMMO:
        case CON_ADDWEAPONVAR:
        case CON_ACTIVATEBYSECTOR:
        case CON_OPERATESECTORS:
        case CON_OPERATEACTIVATORS:
        case CON_SSP:
        case CON_GMAXAMMO:
        case CON_DIST:
        case CON_LDIST:
        case CON_GETINCANGLE:
        case CON_GETANGLE:
        case CON_MULSCALE:
        case CON_DIVSCALE:
        case CON_SCALEVAR:
        case CON_SETASPECT:
            // get the ID of the DEF
            switch (tw)
            {
            case CON_DIST:
            case CON_LDIST:
            case CON_GETANGLE:
            case CON_GETINCANGLE:
            case CON_MULSCALE:
            case CON_DIVSCALE:
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
            case CON_LDIST:
            case CON_GETANGLE:
            case CON_GETINCANGLE:
                C_GetNextVar();
                break;
            case CON_MULSCALE:
            case CON_DIVSCALE:
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
            *(g_scriptPtr-1) = CON_OPERATEACTIVATORS;
            C_GetNextValue(LABEL_DEFINE);
            *g_scriptPtr++ = 0;
            continue;

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
        case CON_UPDATESECTORZ:
            C_GetManyVars(3);
            C_GetNextVarType(GAMEVAR_READONLY);
            continue;

        case CON_DEFINEPROJECTILE:
            {
                int32_t y, z;

                if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
                {
                    C_ReportError(ERROR_FOUNDWITHIN);
                    g_errorCnt++;
                }

                g_scriptPtr--;

                C_GetNextValue(LABEL_DEFINE);
                j = *(g_scriptPtr-1);

                C_GetNextValue(LABEL_DEFINE);
                y = *(g_scriptPtr-1);
                C_GetNextValue(LABEL_DEFINE);
                z = *(g_scriptPtr-1);

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
                {
                    C_ReportError(ERROR_EXCEEDSMAXTILES);
                    g_errorCnt++;
                    continue;
                }

                C_DefineProjectile(j, y, z);
                continue;
            }

        case CON_SPRITEFLAGS:
            if (!g_parsingActorPtr && g_processingState == 0)
            {
                g_scriptPtr--;

                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
                j = *g_scriptPtr;

                int32_t flags = 0;
                do
                    C_BitOrNextValue(&flags);
                while (C_GetKeyword() == -1);

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
                {
                    C_ReportError(ERROR_EXCEEDSMAXTILES);
                    g_errorCnt++;
                    continue;
                }

                g_tile[j].flags = flags;

                continue;
            }
            C_GetNextVar();
            continue;

        case CON_SPRITESHADOW:
        case CON_SPRITENVG:
        case CON_SPRITENOSHADE:
        case CON_SPRITENOPAL:
        case CON_PRECACHE:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
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
        case CON_IFVARVARG:
        case CON_IFVARVARL:
        case CON_IFVARVARE:
        case CON_IFVARVARLE:
        case CON_IFVARVARGE:
        case CON_IFVARVARBOTH:
        case CON_IFVARVARN:
        case CON_IFVARVARAND:
        case CON_IFVARVAROR:
        case CON_IFVARVARXOR:
        case CON_IFVARVAREITHER:
        case CON_WHILEVARVARN:
        case CON_WHILEVARVARL:
            {
                intptr_t offset;
                intptr_t lastScriptPtr = g_scriptPtr - &apScript[0] - 1;

                g_ifElseAborted = 0;

                C_GetManyVars(2);

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                offset = (unsigned)(g_scriptPtr-apScript);
                g_scriptPtr++; // Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                if (tw != CON_WHILEVARVARN)
                {
                    j = C_GetKeyword();

                    if (j == CON_ELSE || j == CON_LEFTBRACE)
                        g_checkingIfElse++;
                }
                continue;
            }

        case CON_STARTTRACK:
            // one parameter (track#)
            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_IFVARL:
        case CON_IFVARLE:
        case CON_IFVARG:
        case CON_IFVARGE:
        case CON_IFVARE:
        case CON_IFVARN:
        case CON_IFVARAND:
        case CON_IFVAROR:
        case CON_IFVARXOR:
        case CON_IFVARBOTH:
        case CON_IFVAREITHER:
        case CON_WHILEVARN:
        case CON_WHILEVARL:
            {
                intptr_t lastScriptPtr = (g_scriptPtr-apScript-1);

                g_ifElseAborted = 0;
                // get the ID of the DEF
                C_GetNextVar();
                C_GetNextValue(LABEL_DEFINE); // the number to check against...

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = g_scriptPtr;
                intptr_t offset = (unsigned)(tempscrptr-apScript);
                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                if (tw != CON_WHILEVARN && tw != CON_WHILEVARL)
                {
                    j = C_GetKeyword();

                    if (j == CON_ELSE || j == CON_LEFTBRACE)
                        g_checkingIfElse++;
                }

                continue;
            }

        case CON_FOR:  // special-purpose iteration
        {
            C_GetNextVarType(GAMEVAR_READONLY);

            C_GetNextLabelName();

            int const iterType = hash_find(&h_iter, label + (g_labelCnt << 6));

            if (iterType < 0)
            {
                C_CUSTOMERROR("unknown iteration type `%s'.", label + (g_labelCnt << 6));
                return 1;
            }
            *g_scriptPtr++ = iterType;

            if (iterType >= ITER_SPRITESOFSECTOR)
                C_GetNextVar();

            intptr_t offset = g_scriptPtr-apScript;
            g_scriptPtr++; //Leave a spot for the location to jump to after completion

            C_ParseCommand(0);

            intptr_t *tscrptr = (intptr_t *) apScript+offset;
            *tscrptr = (g_scriptPtr-apScript)-offset;  // relative offset
            continue;
        }

        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr && g_processingState == 0))
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
            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr && g_processingState == 0))
            {
                C_ReportError(ERROR_EVENTONLY);
                g_errorCnt++;
            }

            C_GetManyVars(13);
            continue;

        case CON_SHOWVIEW:
        case CON_SHOWVIEWUNBIASED:
            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr && g_processingState == 0))
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

        case CON_MINITEXT:
        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
        case CON_SCREENTEXT:
            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr && g_processingState == 0))
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
            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr && g_processingState == 0))
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
                intptr_t tempoffset;

                g_checkingSwitch++; // allow nesting (if other things work)
                C_GetNextVar();

                intptr_t *tempscrptr= g_scriptPtr;
                tempoffset = (unsigned)(tempscrptr-apScript);
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=0; // leave spot for end location (for after processing)
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=0; // count of case statements
                g_caseScriptPtr=g_scriptPtr;        // the first case's pointer.
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=0; // leave spot for 'default' location (null if none)

//                temptextptr=textptr;
                // probably does not allow nesting...

                j=C_CountCaseStatements();
                //        initprintf("Done Counting Case Statements for switch %d: found %d.\n", g_checkingSwitch,j);
                g_scriptPtr+=j*2;
                C_SkipComments();
                g_scriptPtr-=j*2; // allocate buffer for the table
                tempscrptr = (intptr_t *)(apScript+tempoffset);

                //AddLog(g_szBuf);

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
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr++=0; // value check
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr++=0; // code offset
                    C_SkipComments();
                }

                g_numCases=0;
                C_ParseCommand(1);
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
                        intptr_t n = i;

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
                            t                 = tempscrptr[i];
                            tempscrptr[i]     = tempscrptr[n];
                            tempscrptr[n]     = t;
                            t                 = tempscrptr[i + 1];
                            tempscrptr[i + 1] = tempscrptr[n + 1];
                            tempscrptr[n + 1] = t;
                        }
                    }
                    //            for (j=3;j<3+tempscrptr[1]*2;j+=2)initprintf("%5d %8x\n",tempscrptr[j],tempscrptr[j+1]);
                    tempscrptr[0]= (intptr_t)g_scriptPtr - (intptr_t)&apScript[0];    // save 'end' location
                    //            BITPTR_POINTER_SET(tempscrptr-script);
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
                intptr_t *tempscrptr = NULL;

                g_checkingCase++;
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

                if (g_caseScriptPtr)
                {
                    if (tw == CON_DEFAULT)
                    {
                        if (EDUKE32_PREDICT_FALSE(g_caseScriptPtr && g_caseScriptPtr[0]!=0))
                        {
                            // duplicate default statement
                            g_errorCnt++;
                            C_ReportError(-1);
                            initprintf("%s:%d: error: multiple `default' statements found in switch\n", g_scriptFileName, g_lineNumber);
                        }
                        g_caseScriptPtr[0]=(intptr_t) (g_scriptPtr-&apScript[0]);   // save offset
                    }
                    else
                    {
                        for (i=(g_numCases/2)-1; i>=0; i--)
                            if (EDUKE32_PREDICT_FALSE(g_caseScriptPtr[i*2+1]==j))
                            {
                                g_warningCnt++;
                                C_ReportError(WARNING_DUPLICATECASE);
                                break;
                            }
                        g_caseScriptPtr[g_numCases++]=j;
                        g_caseScriptPtr[g_numCases]=(intptr_t) ((intptr_t *) g_scriptPtr-&apScript[0]);
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

                while (C_ParseCommand(0) == 0)
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
            if (g_caseScriptPtr)
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

        case CON_QSTRNCAT:
        case CON_DRAGPOINT:
        case CON_GETKEYNAME:
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

        case CON_IFRND:
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
        case CON_IFPLAYERSL:
        case CON_IFCUTSCENE:
            {
                intptr_t offset;
                intptr_t lastScriptPtr = (g_scriptPtr-&apScript[0]-1);

                g_ifElseAborted = 0;

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
                    if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                    {
                        C_ReportError(-1);
                        *(g_scriptPtr-1) = 0;
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

                intptr_t *tempscrptr = g_scriptPtr;
                offset = (unsigned)(tempscrptr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                j = C_GetKeyword();

                if (j == CON_ELSE || j == CON_LEFTBRACE)
                    g_checkingIfElse++;

                continue;
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
        case CON_IFPLAYBACKON:
            {
                intptr_t offset;
                intptr_t lastScriptPtr = (g_scriptPtr-&apScript[0]-1);

                g_ifElseAborted = 0;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = g_scriptPtr;
                offset = (unsigned)(tempscrptr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                j = C_GetKeyword();

                if (j == CON_ELSE || j == CON_LEFTBRACE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_LEFTBRACE:
            if (EDUKE32_PREDICT_FALSE(!(g_processingState || g_parsingActorPtr || g_parsingEventPtr)))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
            }
            g_numBraces++;

            C_ParseCommand(1);
            continue;

        case CON_RIGHTBRACE:
            g_numBraces--;

            if ((*(g_scriptPtr-2)>>12) == (IFELSE_MAGIC) &&
                ((*(g_scriptPtr-2) & VM_INSTMASK) == CON_LEFTBRACE)) // rewrite "{ }" into "nullop"
            {
                //            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from right\n",g_szScriptFileName,g_lineNumber);
                *(g_scriptPtr-2) = CON_NULLOP + (IFELSE_MAGIC<<12);
                g_scriptPtr -= 2;

                if (C_GetKeyword() != CON_ELSE && (*(g_scriptPtr-2) & VM_INSTMASK) != CON_ELSE)
                    g_ifElseAborted = 1;
                else g_ifElseAborted = 0;

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
            C_NextLine();
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
                C_NextLine();
                continue;
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)k > MAXLEVELS-1))
            {
                initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
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
                C_NextLine();
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
                C_NextLine();
                continue;
            }

            C_UndefineVolume(j);
            continue;

        case CON_DEFINEVOLUMENAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_SkipSpace();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
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
                    C_NextLine();
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
                C_NextLine();
                continue;
            }

            C_DefineVolumeFlags(j, k);
            continue;

        case CON_DEFINEGAMEFUNCNAME:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_SkipSpace();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > NUMGAMEFUNCTIONS-1))
            {
                initprintf("%s:%d: error: function number exceeds number of game functions.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
                continue;
            }

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                gamefunctions[j][i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(*textptr != 0x0a && *textptr != 0x0d && ispecial(*textptr)))
                {
                    initprintf("%s:%d: warning: invalid character in function name.\n",
                        g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                    C_NextLine();
                    break;
                }
                if (EDUKE32_PREDICT_FALSE(i >= MAXGAMEFUNCLEN-1))
                {
                    initprintf("%s:%d: warning: truncating function name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,MAXGAMEFUNCLEN);
                    g_warningCnt++;
                    C_NextLine();
                    break;
                }
            }
            gamefunctions[j][i] = '\0';
            hash_add(&h_gamefuncs,gamefunctions[j],j,0);
            {
                char *str = Bstrtolower(Xstrdup(gamefunctions[j]));
                hash_add(&h_gamefuncs,str,j,0);
                Bfree(str);
            }

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
                C_NextLine();
                continue;
            }

            gamefunctions[j][0] = '\0';

            continue;

        case CON_DEFINESKILLNAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_SkipSpace();

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSKILLS))
            {
                initprintf("%s:%d: error: skill number exceeds maximum skill count %d.\n",
                           g_scriptFileName,g_lineNumber, MAXSKILLS);
                g_errorCnt++;
                C_NextLine();
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
                    C_NextLine();
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
                        C_NextLine();
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
                C_NextLine();
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
                    C_NextLine();
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
                C_NextLine();
                continue;
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)k > MAXLEVELS-1))
            {
                initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
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
                    C_SkipSpace();
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
            C_SkipSpace();

            // cheap hack, 0.99 doesn't have the 3D Realms time
            if (*(textptr+2) == ':')
            {
                g_mapInfo[j *MAXLEVELS+k].designertime =
                    (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*REALGAMETICSPERSEC*60)+
                    (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*REALGAMETICSPERSEC);

                textptr += 5;
                C_SkipSpace();
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
                    C_NextLine();
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

            k = *(g_scriptPtr-1);

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXQUOTES))
            {
                initprintf("%s:%d: error: quote number exceeds limit of %d.\n",g_scriptFileName,g_lineNumber,MAXQUOTES);
                g_errorCnt++;
            }
            else
            {
                C_AllocQuote(k);
            }

            if (tw == CON_DEFINEQUOTE)
                g_scriptPtr--;

            i = 0;

            C_SkipSpace();

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
                    C_NextLine();
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
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++=g_numXStrings;
                g_numXStrings++;
            }
            continue;

        case CON_CHEATKEYS:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            CheatKeys[0] = *(g_scriptPtr-1);
            C_GetNextValue(LABEL_DEFINE);
            CheatKeys[1] = *(g_scriptPtr-1);
            g_scriptPtr -= 2;
            continue;

        case CON_DEFINECHEAT:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            k = *(g_scriptPtr-1);

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= NUMCHEATS))
            {
                initprintf("%s:%d: error: cheat redefinition attempts to redefine nonexistent cheat.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
                continue;
            }
            g_scriptPtr--;
            i = 0;
            C_SkipSpace();
            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0 && *textptr != ' ')
            {
                CheatStrings[k][i] = Btolower(*textptr);
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= (signed)sizeof(CheatStrings[k])-1))
                {
                    initprintf("%s:%d: warning: truncating cheat string to %d characters.\n",
                        g_scriptFileName,g_lineNumber,(signed)sizeof(CheatStrings[k])-1);
                    g_warningCnt++;
                    C_NextLine();
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

            k = *(g_scriptPtr-1);
            if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXSOUNDS))
            {
                initprintf("%s:%d: error: exceeded sound limit of %d.\n",g_scriptFileName,g_lineNumber,MAXSOUNDS);
                g_errorCnt++;
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
            g_sounds[k].ps = *(g_scriptPtr-1);
            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].pe = *(g_scriptPtr-1);
            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].pr = *(g_scriptPtr-1);

            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].m = *(g_scriptPtr-1) & ~SF_ONEINST_INTERNAL;
            if (*(g_scriptPtr-1) & 1)
                g_sounds[k].m |= SF_ONEINST_INTERNAL;

            C_GetNextValue(LABEL_DEFINE);
            g_sounds[k].vo = *(g_scriptPtr-1);
            g_scriptPtr -= 5;

            if (k > g_maxSoundPos)
                g_maxSoundPos = k;

            if (k >= 0 && k < MAXSOUNDS && g_dynamicSoundMapping && j >= 0 && (labeltype[j] & LABEL_DEFINE))
                G_ProcessDynamicSoundMapping(label+(j<<6),k);
            continue;

        case CON_ENDEVENT:

            if (EDUKE32_PREDICT_FALSE(!g_parsingEventPtr))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `endevent' without open `onevent'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
            {
                C_ReportError(ERROR_OPENBRACKET);
                g_errorCnt++;
            }
            else if (EDUKE32_PREDICT_FALSE(g_numBraces < 0))
            {
                C_ReportError(ERROR_CLOSEBRACKET);
                g_errorCnt++;
            }
            // if event has already been declared then put a jump in instead
            if (previous_event)
            {
                g_scriptPtr--;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = CON_JUMP | (g_lineNumber << 12);
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = MAXGAMEVARS;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = previous_event;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = CON_ENDEVENT | (g_lineNumber << 12);

                C_FillEventBreakStackWithJump((intptr_t *)g_parsingEventBreakPtr, previous_event);

                previous_event = 0;
            }
            else
            {
                // pad space for the next potential appendevent
                apScriptGameEventEnd[g_currentEvent] = (g_scriptPtr-1)-apScript;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = CON_ENDEVENT | (g_lineNumber << 12);
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = g_parsingEventBreakPtr;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = CON_ENDEVENT | (g_lineNumber << 12);
            }

            g_parsingEventBreakPtr = g_parsingEventPtr = g_parsingActorPtr = 0;
            g_currentEvent = -1;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_ENDA:
            if (EDUKE32_PREDICT_FALSE(!g_parsingActorPtr || g_parsingEventPtr))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `enda' without open `actor'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                g_parsingEventPtr = 0;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces != 0))
            {
                C_ReportError(g_numBraces > 0 ? ERROR_OPENBRACKET : ERROR_CLOSEBRACKET);
                g_errorCnt++;
            }
            g_parsingActorPtr = 0;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_RETURN:
            if (g_checkingSwitch)
            {
                g_checkingCase = 0;
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

                g_checkingCase = 0;
                return 1;
            }
            else if (g_parsingEventPtr)
            {
                g_scriptPtr--;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = CON_JUMP | (g_lineNumber << 12);
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *(g_scriptPtr++) = MAXGAMEVARS;
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr = g_parsingEventBreakPtr;
                g_parsingEventBreakPtr = g_scriptPtr++ - apScript;
            }
            continue;

        case CON_SCRIPTSIZE:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            j = *(g_scriptPtr-1);
            g_scriptPtr--;
            C_SkipComments();
            if (C_SetScriptSize(j)) return 1;
            continue;

        case CON_SHADETO:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            continue;

        case CON_ADDLOG:
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
        case CON_SPGETLOTAG:
        case CON_SPGETHITAG:
        case CON_SECTGETLOTAG:
        case CON_SECTGETHITAG:
        case CON_GETTEXTUREFLOOR:
        case CON_GETTEXTURECEILING:
        case CON_INSERTSPRITEQ:
        case CON_STOPALLSOUNDS:
        case CON_STOPALLMUSIC:
        case CON_STARTSCREEN:
            continue;

        case CON_NULLOP:
            if (EDUKE32_PREDICT_FALSE(C_GetKeyword() != CON_ELSE))
            {
                C_ReportError(-1);
                g_warningCnt++;
                initprintf("%s:%d: warning: `nullop' found without `else'\n",g_scriptFileName,g_lineNumber);
                g_scriptPtr--;
                g_ifElseAborted = 1;
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
    Bstrcpy(label+(g_labelCnt<<6),lLabel);
    labeltype[g_labelCnt] = lType;
    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
    labelcode[g_labelCnt++] = lValue;
    g_defaultLabelCnt++;
}

// KEEPINSYNC lunatic/con_lang.lua
static void C_AddDefaultDefinitions(void)
{
    for (int i=0; i<MAXEVENTS; i++)
        C_AddDefinition(EventNames[i], i, LABEL_DEFINE);

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

    tokenmap_t predefined[] = {
        { "STAT_DEFAULT", STAT_DEFAULT },
        { "STAT_ACTOR", STAT_ACTOR },
        { "STAT_ZOMBIEACTOR", STAT_ZOMBIEACTOR },
        { "STAT_EFFECTOR", STAT_EFFECTOR },
        { "STAT_PROJECTILE", STAT_PROJECTILE },
        { "STAT_MISC", STAT_MISC },
        { "STAT_STANDABLE", STAT_STANDABLE },
        { "STAT_LOCATOR", STAT_LOCATOR },
        { "STAT_ACTIVATOR", STAT_ACTIVATOR },
        { "STAT_TRANSPORT", STAT_TRANSPORT },
        { "STAT_PLAYER", STAT_PLAYER },
        { "STAT_FX", STAT_FX },
        { "STAT_FALLER", STAT_FALLER },
        { "STAT_DUMMYPLAYER", STAT_DUMMYPLAYER },
        { "STAT_LIGHT", STAT_LIGHT },

        { "SFLAG_SHADOW", SFLAG_SHADOW },
        { "SFLAG_NVG", SFLAG_NVG },
        { "SFLAG_NOSHADE", SFLAG_NOSHADE },
        { "SFLAG_BADGUY", SFLAG_BADGUY },
        { "SFLAG_NOPAL", SFLAG_NOPAL },
        { "SFLAG_NOEVENTS", SFLAG_NOEVENTCODE },
        { "SFLAG_NOLIGHT", SFLAG_NOLIGHT },
        { "SFLAG_USEACTIVATOR", SFLAG_USEACTIVATOR },
        { "SFLAG_NOCLIP", SFLAG_NOCLIP },
        { "SFLAG_SMOOTHMOVE", SFLAG_SMOOTHMOVE },
        { "SFLAG_NOTELEPORT", SFLAG_NOTELEPORT },
        { "SFLAG_NODAMAGEPUSH", SFLAG_NODAMAGEPUSH },
        { "SFLAG_NOWATERDIP", SFLAG_NOWATERDIP },
        { "SFLAG_HURTSPAWNBLOOD", SFLAG_HURTSPAWNBLOOD },
        { "SFLAG_GREENSLIMEFOOD", SFLAG_GREENSLIMEFOOD },
        { "SFLAG_REALCLIPDIST", SFLAG_REALCLIPDIST },
        { "SFLAG_WAKEUPBADGUYS", SFLAG_WAKEUPBADGUYS },

        { "STR_MAPNAME", STR_MAPNAME },
        { "STR_MAPFILENAME", STR_MAPFILENAME },
        { "STR_PLAYERNAME", STR_PLAYERNAME },
        { "STR_VERSION", STR_VERSION },
        { "STR_GAMETYPE", STR_GAMETYPE },
        { "STR_VOLUMENAME", STR_VOLUMENAME },
        { "STR_YOURTIME", STR_YOURTIME },
        { "STR_PARTIME", STR_PARTIME },
        { "STR_DESIGNERTIME", STR_DESIGNERTIME },
        { "STR_BESTTIME", STR_BESTTIME },

        { "MAXSTATUS", MAXSTATUS },
        { "MAXSPRITES", MAXSPRITES },
        { "MAX_WEAPONS", MAX_WEAPONS },
        { "MAXSPRITESONSCREEN", MAXSPRITESONSCREEN },
        { "MAXTILES", MAXTILES },

        { "PROJ_BOUNCES", PROJ_BOUNCES },
        { "PROJ_BSOUND", PROJ_BSOUND },
        { "PROJ_CLIPDIST", PROJ_CLIPDIST },
        { "PROJ_CSTAT", PROJ_CSTAT },
        { "PROJ_DECAL", PROJ_DECAL },
        { "PROJ_DROP", PROJ_DROP },
        { "PROJ_EXTRA", PROJ_EXTRA },
        { "PROJ_EXTRA_RAND", PROJ_EXTRA_RAND },
        { "PROJ_FLASH_COLOR", PROJ_FLASH_COLOR },
        { "PROJ_HITRADIUS", PROJ_HITRADIUS },
        { "PROJ_ISOUND", PROJ_ISOUND },
        { "PROJ_OFFSET", PROJ_OFFSET },
        { "PROJ_PAL", PROJ_PAL },
        { "PROJ_RANGE", PROJ_RANGE },
        { "PROJ_SHADE", PROJ_SHADE },
        { "PROJ_SOUND", PROJ_SOUND },
        { "PROJ_SPAWNS", PROJ_SPAWNS },
        { "PROJ_SXREPEAT", PROJ_SXREPEAT },
        { "PROJ_SYREPEAT", PROJ_SYREPEAT },
        { "PROJ_TNUM", PROJ_TNUM },
        { "PROJ_TOFFSET", PROJ_TOFFSET },
        { "PROJ_TRAIL", PROJ_TRAIL },
        { "PROJ_TXREPEAT", PROJ_TXREPEAT },
        { "PROJ_TYREPEAT", PROJ_TYREPEAT },
        { "PROJ_USERDATA", PROJ_USERDATA },
        { "PROJ_VEL_MULT", PROJ_MOVECNT },
        { "PROJ_VEL", PROJ_VEL },
        { "PROJ_WORKSLIKE", PROJ_WORKSLIKE },
        { "PROJ_XREPEAT", PROJ_XREPEAT },
        { "PROJ_YREPEAT", PROJ_YREPEAT },

        { "GAMEVAR_PERPLAYER", GAMEVAR_PERPLAYER },
        { "GAMEVAR_PERACTOR", GAMEVAR_PERACTOR },
        { "GAMEVAR_NODEFAULT", GAMEVAR_NODEFAULT },
        { "GAMEVAR_NORESET", GAMEVAR_NORESET },
        { "GAMEVAR_NOMULTI", GAMEVAR_NOMULTI },

        { "GAMEARRAY_RESTORE", GAMEARRAY_RESTORE },
        { "GAMEARRAY_INT16", GAMEARRAY_INT16 },
        { "GAMEARRAY_INT8", GAMEARRAY_INT8 },
        { "GAMEARRAY_UINT16", GAMEARRAY_UINT16 },
        { "GAMEARRAY_UINT8", GAMEARRAY_UINT8 },
        { "GAMEARRAY_BOOLEAN", GAMEARRAY_BITMAP },
    };

    for (unsigned i = 0; i < ARRAY_SIZE(predefined); i++)
        C_AddDefinition(predefined[i].token, predefined[i].val, LABEL_DEFINE);

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

    for (bssize_t i=MAXTILES-1; i>=0; i--)
    {
        if (g_tile[i].proj)
            *g_tile[i].proj = DefaultProjectile;

        if (g_tile[i].defproj)
            *g_tile[i].defproj = DefaultProjectile;
    }
}

#if !defined LUNATIC
extern int32_t g_numObituaries;
extern int32_t g_numSelfObituaries;

static char const * C_ScriptVersionString(int32_t version)
{
    switch (version)
    {
#ifndef EDUKE32_STANDALONE
    case 9:
        return ", v0.99 compatibility mode";
    case 10:
        return ", v1.0 compatibility mode";
    case 11:
        return ", v1.1 compatibility mode";
    case 13:
        return ", v1.3D compatibility mode";
#endif
    default:
        return "";
    }
}

void C_PrintStats(void)
{
    initprintf("%d/%d labels, %d/%d variables, %d/%d arrays\n", g_labelCnt,
        (int32_t) min((MAXSECTORS * sizeof(sectortype)/sizeof(int32_t)),
            MAXSPRITES * sizeof(spritetype)/(1<<6)),
        g_gameVarCount, MAXGAMEVARS, g_gameArrayCount, MAXGAMEARRAYS);

    int i, j;

    for (i=MAXQUOTES-1, j=g_numXStrings; i>=0; i--)
    {
        if (apStrings[i])
            j++;
    }

    if (j) initprintf("%d strings, ", j);

    for (i=MAXEVENTS-1, j=0; i>=0; i--)
    {
        if (apScriptEvents[i])
            j++;
    }
    if (j) initprintf("%d events, ", j);

    for (i=MAXTILES-1, j=0; i>=0; i--)
    {
        if (g_tile[i].execPtr)
            j++;
    }
    if (j) initprintf("%d actors", j);

    initprintf("\n");
}

void C_Compile(const char *fileName)
{
    Bmemset(apScriptEvents, 0, sizeof(apScriptEvents));
    Bmemset(apScriptGameEventEnd, 0, sizeof(apScriptGameEventEnd));

    for (int i=MAXTILES-1; i>=0; i--)
        Bmemset(&g_tile[i], 0, sizeof(tiledata_t));

    C_InitHashes();
    Gv_Init();
    C_InitProjectiles();

    int kFile = kopen4loadfrommod(fileName,g_loadFromGroupOnly);

    if (kFile == -1) // JBF: was 0
    {
        if (g_loadFromGroupOnly == 1 || numgroupfiles == 0)
        {
#ifndef EDUKE32_STANDALONE
            char const *gf = G_GrpFile();
            Bsprintf(tempbuf,"Required game data was not found.  A valid copy of \"%s\" or other compatible data is needed to run EDuke32.\n\n"
                     "You can find \"%s\" in the \"DN3DINST\" or \"ATOMINST\" directory on your Duke Nukem 3D installation disc.\n\n"
                     "Please copy \"%s\" to your game directory and restart EDuke32!", gf, gf, gf);
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

    flushlogwindow = 0;

    uint32_t const startcompiletime = getticks();

    char * mptr = (char *)Xmalloc(kFileLen+1);
    mptr[kFileLen] = 0;

    textptr = (char *) mptr;
    kread(kFile,(char *)textptr,kFileLen);
    kclose(kFile);

    Bfree(apScript);

    apScript = (intptr_t *)Xcalloc(1, g_scriptSize * sizeof(intptr_t));
    bitptr   = (char *)Xcalloc(1, (((g_scriptSize + 7) >> 3) + 1) * sizeof(uint8_t));
    //    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    g_labelCnt        = 0;
    g_defaultLabelCnt = 0;
    g_scriptPtr       = apScript + 3;  // move permits constants 0 and 1; moveptr[1] would be script[2] (reachable?)
    g_warningCnt      = 0;
    g_errorCnt        = 0;
    g_lineNumber      = 1;
    g_totalLines      = 0;

    C_AddDefaultDefinitions();

    Bstrcpy(g_scriptFileName, fileName);

    C_ParseCommand(1);

    for (int i=0; i < g_scriptModulesNum; ++i)
    {
        C_Include(g_scriptModules[i]);
        Bfree(g_scriptModules[i]);
    }
    DO_FREE_AND_NULL(g_scriptModules);
    g_scriptModulesNum = 0;

    flushlogwindow = 1;

    if (g_errorCnt > 63)
        initprintf("fatal error: too many errors: Aborted\n");

    //*script = (intptr_t) g_scriptPtr;

    DO_FREE_AND_NULL(mptr);

    if (g_warningCnt || g_errorCnt)
        initprintf("Found %d warning(s), %d error(s).\n", g_warningCnt, g_errorCnt);

    if (g_errorCnt)
    {
        Bsprintf(buf, "Error compiling CON files.");
        G_GameExit(buf);
    }

    for (int i = 0; i < MAXEVENTS; ++i)
    {
        if (!apScriptGameEventEnd[i])
            continue;

        intptr_t *eventEnd = apScript + apScriptGameEventEnd[i];
        // C_FillEventBreakStackWithEndEvent
        intptr_t *breakPtr = (intptr_t*)*(eventEnd + 2);
        while (breakPtr)
        {
            breakPtr = apScript + (intptr_t)breakPtr;
            *(breakPtr-2) = CON_ENDEVENT | (g_lineNumber << 12);
            breakPtr = (intptr_t*)*breakPtr;
        }
    }

    g_totalLines += g_lineNumber;

    C_SetScriptSize(g_scriptPtr-apScript+8);

    initprintf("Script compiled in %dms, %ld bytes%s\n", getticks() - startcompiletime,
                (unsigned long)(g_scriptPtr-apScript), C_ScriptVersionString(g_scriptVersion));

    for (unsigned i=0; i < ARRAY_SIZE(tables_free); i++)
        hash_free(tables_free[i]);

    freehashnames();
    freesoundhashnames();

    if (g_scriptDebug)
        C_PrintStats();

    C_InitQuotes();
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (g_parsingEventPtr || g_processingState || g_parsingActorPtr)
            initprintf("%s: In %s `%s':\n",g_scriptFileName,g_parsingEventPtr?"event":g_parsingActorPtr?"actor":"state",g_szCurrentBlockName);
        else initprintf("%s: At top level:\n",g_scriptFileName);
        Bstrcpy(g_szLastBlockName,g_szCurrentBlockName);
    }
    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%d: error: found more `}' than `{' before `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
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
        initprintf("%s:%d: error: found `%s' within %s.\n",g_scriptFileName,g_lineNumber,tempbuf,g_parsingEventPtr?"an event":g_parsingActorPtr?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%d: error: did not find `endswitch' before `%s'.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%d: error: symbol `%s' is not a definition.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a variable.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_NOTAGAMEARRAY:
        initprintf("%s:%d: error: symbol `%s' is not an array.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
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
    case ERROR_OPENBRACKET:
        initprintf("%s:%d: error: found more `{' than `}' before `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_NOTAMEMBER:
        initprintf("%s:%d: error: symbol `%s' is not a valid structure member.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",g_scriptFileName,g_lineNumber);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_ARRAYREADONLY:
        initprintf("%s:%d: error: array `%s' is read-only.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case WARNING_BADGAMEVAR:
        initprintf("%s:%d: warning: variable `%s' should be either per-player OR per-actor, not both.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case WARNING_DUPLICATECASE:
        initprintf("%s:%d: warning: duplicate case ignored.\n",g_scriptFileName,g_lineNumber);
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate definition `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case WARNING_EVENTSYNC:
        initprintf("%s:%d: warning: found `%s' within a local event.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",g_scriptFileName,g_lineNumber);
        break;
    case WARNING_NAMEMATCHESVAR:
        initprintf("%s:%d: warning: symbol `%s' already used for variable.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case WARNING_VARMASKSKEYWORD:
        initprintf("%s:%d: warning: variable `%s' masks keyword.\n", g_scriptFileName, g_lineNumber, label+(g_labelCnt<<6));
        break;
    case WARNING_ARRAYMASKSKEYWORD:
        initprintf("%s:%d: warning: array `%s' masks keyword.\n", g_scriptFileName, g_lineNumber, label+(g_labelCnt<<6));
        break;
    }
}
#endif
