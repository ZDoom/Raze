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

#ifndef gamedef_h_
#define gamedef_h_

#include "actors.h"
#include "build.h"  // hashtable_t
#include "cheats.h"
#include "common.h"  // tokenlist
#include "player.h"  // projectile_t

BEGIN_RR_NS

enum
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
    LABEL_EVENT  = 0x40,
};

#define LABEL_HASPARM2  1
#define LABEL_ISSTRING  2

// "magic" number for { and }, overrides line number in compiled code for later detection
#define VM_IFELSE_MAGIC 31337
#define VM_INSTMASK 0xfff
#define VM_DECODE_LINE_NUMBER(xxx) ((int)((xxx) >> 12))

#define C_CUSTOMERROR(Text, ...)                                                                                                           \
    do                                                                                                                                     \
    {                                                                                                                                      \
        C_ReportError(-1);                                                                                                                 \
        Printf("%s:%d: error: " Text "\n", g_scriptFileName, g_lineNumber, ##__VA_ARGS__);                                             \
        g_errorCnt++;                                                                                                                      \
    } while (0)

#define C_CUSTOMWARNING(Text, ...)                                                                                                         \
    do                                                                                                                                     \
    {                                                                                                                                      \
        C_ReportError(-1);                                                                                                                 \
        Printf("%s:%d: warning: " Text "\n", g_scriptFileName, g_lineNumber, ##__VA_ARGS__);                                           \
        g_warningCnt++;                                                                                                                    \
    } while (0)

extern intptr_t const * insptr;
extern void VM_ScriptInfo(intptr_t const *ptr, int range);

extern hashtable_t h_gamevars;
extern hashtable_t h_labels;

extern int32_t g_aimAngleVarID;   // var ID of "AUTOAIMANGLE"
extern int32_t g_angRangeVarID;   // var ID of "ANGRANGE"
extern int32_t g_returnVarID;     // var ID of "RETURN"
extern int32_t g_weaponVarID;     // var ID of "WEAPON"
extern int32_t g_worksLikeVarID;  // var ID of "WORKSLIKE"
extern int32_t g_zRangeVarID;     // var ID of "ZRANGE"

#include "events_defs.h"
extern intptr_t apScriptEvents[MAXEVENTS];

extern char g_scriptFileName[BMAX_PATH];

extern const uint32_t CheatFunctionFlags[];
extern const uint8_t  CheatFunctionIDs[];

extern int32_t g_errorCnt;
extern int32_t g_lineNumber;
extern int32_t g_scriptVersion;
extern int32_t g_totalLines;
extern int32_t g_warningCnt;
extern uint32_t g_scriptcrc;
extern int32_t otherp;

extern intptr_t *g_scriptPtr;

typedef struct
{
    const char *name;
    int lId, flags, maxParm2;
} memberlabel_t;

extern const memberlabel_t ActorLabels[];
extern const memberlabel_t InputLabels[];
extern const memberlabel_t PalDataLabels[];
extern const memberlabel_t PlayerLabels[];
extern const memberlabel_t ProjectileLabels[];
extern const memberlabel_t SectorLabels[];
extern const memberlabel_t TileDataLabels[];
extern const memberlabel_t TsprLabels[];
extern const memberlabel_t UserdefsLabels[];
extern const memberlabel_t WallLabels[];

int32_t C_AllocQuote(int32_t qnum);
void C_InitQuotes(void);

extern int32_t g_numProjectiles;

typedef struct {
    int spriteNum;
    int playerNum;
    int playerDist;
    int flags;

    union {
        spritetype * pSprite;
        uspritetype *pUSprite;
    };

    int32_t *     pData;
    DukePlayer_t *pPlayer;
    actor_t *     pActor;
} vmstate_t;

extern vmstate_t vm;

void G_DoGameStartup(const int32_t *params);
void C_DefineMusic(int volumeNum, int levelNum, const char *fileName);

void C_DefineVolumeFlags(int32_t vol, int32_t flags);
void C_UndefineVolume(int32_t vol);
void C_UndefineSkill(int32_t skill);
void C_UndefineLevel(int32_t vol, int32_t lev);
void C_ReportError(int32_t iError);
void C_Compile(const char *filenam);

extern int32_t g_errorLineNum;
extern int32_t g_tw;

typedef struct {
    const char* token;
    int32_t val;
} tokenmap_t;

extern char const * VM_GetKeywordForID(int32_t id);

enum ScriptError_t
{
    ERROR_CLOSEBRACKET,
    ERROR_EXCEEDSMAXTILES,
    ERROR_EXPECTEDKEYWORD,
    ERROR_FOUNDWITHIN,
    ERROR_ISAKEYWORD,
    ERROR_OPENBRACKET,
    ERROR_NOTAGAMEVAR,
    ERROR_PARAMUNDEFINED,
    ERROR_SYNTAXERROR,
    ERROR_VARREADONLY,
    ERROR_VARTYPEMISMATCH,
    WARNING_BADGAMEVAR,
    WARNING_DUPLICATEDEFINITION,
    WARNING_LABELSONLY,
    WARNING_VARMASKSKEYWORD,
};

enum ScriptKeywords_t
{
    CON_ELSE,               // 0
    CON_ACTOR,              // 1
    CON_ADDAMMO,            // 2
    CON_IFRND,              // 3
    CON_ENDA,               // 4
    CON_IFCANSEE,           // 5
    CON_IFHITWEAPON,        // 6
    CON_ACTION,             // 7
    CON_IFPDISTL,           // 8
    CON_IFPDISTG,           // 9
    CON_DEFINELEVELNAME,    // 10
    CON_STRENGTH,           // 11
    CON_BREAK,              // 12
    CON_SHOOT,              // 13
    CON_PALFROM,            // 14
    CON_SOUND,              // 15
    CON_FALL,               // 16
    CON_STATE,              // 17
    CON_ENDS,               // 18
    CON_DEFINE,             // 19
    CON_COMMENT,            // 20 deprecated
    CON_IFAI,               // 21
    CON_KILLIT,             // 22
    CON_ADDWEAPON,          // 23
    CON_AI,                 // 24
    CON_ADDPHEALTH,         // 25
    CON_IFDEAD,             // 26
    CON_IFSQUISHED,         // 27
    CON_SIZETO,             // 28
    CON_LEFTBRACE,          // 29
    CON_RIGHTBRACE,         // 30
    CON_SPAWN,              // 31
    CON_MOVE,               // 32
    CON_IFWASWEAPON,        // 33
    CON_IFACTION,           // 34
    CON_IFACTIONCOUNT,      // 35
    CON_RESETACTIONCOUNT,   // 36
    CON_DEBRIS,             // 37
    CON_PSTOMP,             // 38
    CON_BLOCKCOMMENT,       // 39 deprecated
    CON_CSTAT,              // 40
    CON_IFMOVE,             // 41
    CON_RESETPLAYER,        // 42
    CON_IFONWATER,          // 43
    CON_IFINWATER,          // 44
    CON_IFCANSHOOTTARGET,   // 45
    CON_IFCOUNT,            // 46
    CON_RESETCOUNT,         // 47
    CON_ADDINVENTORY,       // 48
    CON_IFACTORNOTSTAYPUT,  // 49
    CON_HITRADIUS,          // 50
    CON_IFP,                // 51
    CON_COUNT,              // 52
    CON_IFACTOR,            // 53
    CON_MUSIC,              // 54
    CON_INCLUDE,            // 55
    CON_IFSTRENGTH,         // 56
    CON_DEFINESOUND,        // 57
    CON_GUTS,               // 58
    CON_IFSPAWNEDBY,        // 59
    CON_GAMESTARTUP,        // 60
    CON_WACKPLAYER,         // 61
    CON_IFGAPZL,            // 62
    CON_IFHITSPACE,         // 63
    CON_IFOUTSIDE,          // 64
    CON_IFMULTIPLAYER,      // 65
    CON_OPERATE,            // 66
    CON_IFINSPACE,          // 67
    CON_DEBUG,              // 68
    CON_ENDOFGAME,          // 69
    CON_IFBULLETNEAR,       // 70
    CON_IFRESPAWN,          // 71
    CON_IFFLOORDISTL,       // 72
    CON_IFCEILINGDISTL,     // 73
    CON_SPRITEPAL,          // 74
    CON_IFPINVENTORY,       // 75
    CON_BETANAME,           // 76
    CON_CACTOR,             // 77
    CON_IFPHEALTHL,         // 78
    CON_DEFINEQUOTE,        // 79
    CON_QUOTE,              // 80
    CON_IFINOUTERSPACE,     // 81
    CON_IFNOTMOVING,        // 82
    CON_RESPAWNHITAG,       // 83
    CON_TIP,                // 84
    CON_IFSPRITEPAL,        // 85
    CON_MONEY,              // 86
    CON_SOUNDONCE,          // 87
    CON_ADDKILLS,           // 88
    CON_STOPSOUND,          // 89
    CON_IFAWAYFROMWALL,     // 90
    CON_IFCANSEETARGET,     // 91
    CON_GLOBALSOUND,        // 92
    CON_LOTSOFGLASS,        // 93
    CON_IFGOTWEAPONCE,      // 94
    CON_GETLASTPAL,         // 95
    CON_PKICK,              // 96
    CON_MIKESND,            // 97
    CON_USERACTOR,          // 98
    CON_SIZEAT,             // 99
    CON_ADDSTRENGTH,        // 100
    CON_CSTATOR,            // 101
    CON_MAIL,               // 102
    CON_PAPER,              // 103
    CON_TOSSWEAPON,         // 104
    CON_SLEEPTIME,          // 105
    CON_NULLOP,             // 106
    CON_DEFINEVOLUMENAME,   // 107
    CON_DEFINESKILLNAME,    // 108
    CON_IFNOSOUNDS,         // 109
    CON_CLIPDIST,           // 110
    CON_IFANGDIFFL,         // 111
    CON_IFNOCOVER,          // 112
    CON_IFHITTRUCK,         // 113
    CON_IFTIPCOW,           // 114
    CON_ISDRUNK,            // 115
    CON_ISEAT,              // 116
    CON_DESTROYIT,          // 117
    CON_LARRYBIRD,          // 118
    CON_STRAFELEFT,         // 119
    CON_STRAFERIGHT,        // 120
    CON_IFACTORHEALTHG,     // 121
    CON_IFACTORHEALTHL,     // 122
    CON_SLAPPLAYER,         // 123
    CON_IFPDRUNK,           // 124
    CON_TEARITUP,           // 125
    CON_SMACKBUBBA,         // 126
    CON_SOUNDTAGONCE,       // 127
    CON_SOUNDTAG,           // 128
    CON_IFSOUNDID,          // 129
    CON_IFSOUNDDIST,        // 130
    CON_IFONMUD,            // 131
    CON_IFCOOP,             // 132
    CON_IFMOTOFAST,         // 133
    CON_IFWIND,             // 134
    CON_SMACKSPRITE,        // 135
    CON_IFONMOTO,           // 136
    CON_IFONBOAT,           // 137
    CON_FAKEBUBBA,          // 138
    CON_MAMATRIGGER,        // 139
    CON_MAMASPAWN,          // 140
    CON_MAMAQUAKE,          // 141
    CON_MAMAEND,            // 142
    CON_NEWPIC,             // 143
    CON_GARYBANJO,          // 144
    CON_MOTOLOOPSND,        // 145
    CON_IFSIZEDOWN,         // 146
    CON_RNDMOVE,            // 147
    CON_GAMEVAR,            // 148
    CON_IFVARL,             // 149
    CON_IFVARG,             // 150
    CON_SETVARVAR,          // 151
    CON_SETVAR,             // 152
    CON_ADDVARVAR,          // 153
    CON_ADDVAR,             // 154
    CON_IFVARVARL,          // 155
    CON_IFVARVARG,          // 156
    CON_ADDLOGVAR,          // 157
    CON_ONEVENT,            // 158
    CON_ENDEVENT,           // 159
    CON_IFVARE,             // 160
    CON_IFVARVARE,          // 161
    CON_IFFINDNEWSPOT,      // 162
    CON_LEAVETRAX,          // 163
    CON_LEAVEDROPPINGS,     // 164
    CON_DEPLOYBIAS,         // 165
    CON_IFPUPWIND,          // 166
    CON_END
};
// KEEPINSYNC with the keyword list in lunatic/con_lang.lua

END_RR_NS

#endif // gamedef_h_
