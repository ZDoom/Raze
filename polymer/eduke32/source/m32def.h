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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef m32def_h_
#define m32def_h_

// the parsing routines aren't good at error recovery yet...
#define ABORTERRCNT 1

// these two are for m32def.c
#define C_CUSTOMERROR(Text, ...) do { \
    C_ReportError(-1);                                                  \
    initprintf("%s:%d: error: " Text "\n", g_szScriptFileName, g_lineNumber, ## __VA_ARGS__); \
    C_PrintErrorPosition();  \
    g_numCompilerErrors++; \
    } while (0)
#define C_CUSTOMWARNING(Text, ...) do { \
    C_ReportError(-1);                                                  \
    initprintf("%s:%d: warning: " Text "\n", g_szScriptFileName, g_lineNumber, ## __VA_ARGS__); \
    C_PrintErrorPosition();    \
    g_numCompilerWarnings++; \
    } while (0)

extern void C_PrintErrorPosition();
extern void C_ReportError(int32_t iError);

extern char g_szScriptFileName[BMAX_PATH];
extern int32_t g_totalLines, g_lineNumber;
extern int32_t g_numCompilerErrors, g_numCompilerWarnings;

extern int32_t g_didDefineSomething;

extern instype *g_scriptPtr;

void C_Compile(const char *filenameortext, int32_t isfilename);
void C_CompilationInfo(void);

void registerMenuFunction(const char *funcname, int32_t stateidx);
void M32_PostScriptExec(void);

typedef struct
{
    int32_t ofs;  // offset into script[]
    int32_t codesize;
    uint16_t numlocals;  // number of local int32_t vars to allocate
    char name[MAXLABELLEN];
} statesinfo_t;

extern statesinfo_t *statesinfo;
extern int32_t g_stateCount;


typedef struct
{
    const char *name;
    int16_t lId;
    int16_t flags;  // 1: read-only
    int32_t min, max;
} memberlabel_t;

extern const memberlabel_t SectorLabels[];
extern const memberlabel_t WallLabels[];
extern const memberlabel_t SpriteLabels[];
extern const memberlabel_t LightLabels[];


typedef struct {
    const char* token;
    int32_t val;
} tokenmap_t;

extern const tokenmap_t iter_tokens[];

enum vmflags
{
    VMFLAG_RETURN = 1,
    VMFLAG_BREAK = 2,
    VMFLAG_ERROR = 4,
};

enum miscvmflags
{
    VMFLAG_MISC_UPDATEHL = 1,
    VMFLAG_MISC_UPDATEHLSECT = 2,
    VMFLAG_MISC_INTERACTIVE = 4,
};

typedef struct {
    int32_t spriteNum;
    // VM state: either ==0 (top-level), >=1 and < MAXEVENTS+1 (event),
    // or >= MAXEVENTS+1 and < MAXEVENTS+1+g_stateCount (state)
    int32_t g_st;
    union {
        spritetype * pSprite;
        uspritetype *pUSprite;
    };
    uint32_t flags;  // g_errorFlag, g_returnFlag;

    // 1:updatehighlight, 2:updatehighlightsector, 4:interactive (from menu)?
    uint32_t miscflags;
} vmstate_t;

extern vmstate_t vm;
extern vmstate_t vm_default;


extern int32_t g_errorLineNum;
extern int32_t g_tw;
extern const char *keyw[];

enum SystemString_t {
    STR_MAPFILENAME,
    STR_VERSION,
};

enum ScriptError_t
{
    ERROR_CLOSEBRACKET,
    ERROR_EVENTONLY,
//    ERROR_EXCEEDSMAXTILES,
    ERROR_EXPECTEDKEYWORD,
    ERROR_FOUNDWITHIN,
    ERROR_ISAKEYWORD,
    ERROR_NOENDSWITCH,
    ERROR_NOTAGAMEDEF,
    ERROR_NOTAGAMEVAR,
    ERROR_NOTAGAMEARRAY,
    ERROR_GAMEARRAYBNC,
    ERROR_GAMEARRAYBNO,
    ERROR_INVALIDARRAYWRITE,
    ERROR_EXPECTEDSIMPLEVAR,
    ERROR_OPENBRACKET,
    ERROR_PARAMUNDEFINED,
    ERROR_SYMBOLNOTRECOGNIZED,
    ERROR_SYNTAXERROR,
    ERROR_VARREADONLY,
    ERROR_ARRAYREADONLY,
    ERROR_VARTYPEMISMATCH,
    ERROR_LABELINUSE,
//    WARNING_BADGAMEVAR,
    WARNING_DUPLICATECASE,
    WARNING_DUPLICATEDEFINITION,
    WARNING_LABELSONLY,
    WARNING_NAMEMATCHESVAR,
//    WARNING_CONSTANTBITSIZE,
    WARNING_OUTSIDEDRAWSPRITE,
};


enum SectorLabel_t
{
    SECTOR_WALLPTR,
    SECTOR_WALLNUM,
    SECTOR_CEILINGZ,
    SECTOR_FLOORZ,
    SECTOR_CEILINGSTAT,
    SECTOR_FLOORSTAT,
    SECTOR_CEILINGPICNUM,
    SECTOR_CEILINGSLOPE,
    SECTOR_CEILINGSHADE,
    SECTOR_CEILINGPAL,
    SECTOR_CEILINGXPANNING,
    SECTOR_CEILINGYPANNING,
    SECTOR_FLOORPICNUM,
    SECTOR_FLOORSLOPE,
    SECTOR_FLOORSHADE,
    SECTOR_FLOORPAL,
    SECTOR_FLOORXPANNING,
    SECTOR_FLOORYPANNING,
    SECTOR_VISIBILITY,
    SECTOR_FOGPAL,
    SECTOR_LOTAG,
    SECTOR_HITAG,
    SECTOR_EXTRA,
    SECTOR_END
};

enum WallLabel_t
{
    WALL_X,
    WALL_Y,
    WALL_POINT2,
    WALL_NEXTWALL,
    WALL_NEXTSECTOR,
    WALL_CSTAT,
    WALL_PICNUM,
    WALL_OVERPICNUM,
    WALL_SHADE,
    WALL_PAL,
    WALL_XREPEAT,
    WALL_YREPEAT,
    WALL_XPANNING,
    WALL_YPANNING,
    WALL_LOTAG,
    WALL_HITAG,
    WALL_EXTRA,
    WALL_END
};

enum SpriteLabel_t
{
    SPRITE_X,  // 0
    SPRITE_Y,
    SPRITE_Z,
    SPRITE_CSTAT,
    SPRITE_PICNUM,
    SPRITE_SHADE,  // 5
    SPRITE_PAL,
    SPRITE_CLIPDIST,
    SPRITE_BLEND,
    SPRITE_XREPEAT,
    SPRITE_YREPEAT,  // 10
    SPRITE_XOFFSET,
    SPRITE_YOFFSET,
    SPRITE_SECTNUM,
    SPRITE_STATNUM,
    SPRITE_ANG,  // 15
    SPRITE_OWNER,
    SPRITE_XVEL,
    SPRITE_YVEL,
    SPRITE_ZVEL,
    SPRITE_LOTAG,  // 20
    SPRITE_HITAG,
    SPRITE_EXTRA,

    LIGHT_X,  // must be first here
    LIGHT_Y,
    LIGHT_Z,  // 25
    LIGHT_HORIZ,
    LIGHT_RANGE,
    LIGHT_ANGLE,
    LIGHT_FADERADIUS,
    LIGHT_RADIUS,  // 30
    LIGHT_SECTOR,
    LIGHT_R,
    LIGHT_G,
    LIGHT_B,
    LIGHT_PRIORITY,  // 35
    LIGHT_TILENUM,
    LIGHT_MINSHADE,
    LIGHT_MAXSHADE,
    LIGHT_ACTIVE,

    SPRITE_END
};

enum IterationTypes_t
{
    ITER_ALLSPRITES,
    ITER_ALLSECTORS,
    ITER_ALLWALLS,
    ITER_ACTIVELIGHTS,
    ITER_SELSPRITES,
    ITER_SELSECTORS,
    ITER_SELWALLS,
    ITER_DRAWNSPRITES,
// ---
    ITER_SPRITESOFSECTOR,
    ITER_WALLSOFSECTOR,
    ITER_LOOPOFWALL,
    ITER_RANGE,
    ITER_END
};

enum ScriptKeywords_t
{
// basic commands
    CON_NULLOP,
    CON_DEFINE,
    CON_INCLUDE,
    CON_DEFSTATE,
    CON_ENDS,
    CON_STATE,
    CON_ONEVENT,
    CON_ENDEVENT,
    CON_GAMEVAR,

// control flow (except if*)
    CON_ELSE,
    CON_RETURN,
    CON_BREAK,
    CON_SWITCH,
    CON_CASE,
    CON_DEFAULT,
    CON_ENDSWITCH,
    CON_GETCURRADDRESS,
    CON_JUMP,
    CON_LEFTBRACE,
    CON_RIGHTBRACE,

#if 0
// more basic commands
    CON_SETSECTOR,
    CON_GETSECTOR,
    CON_SETWALL,
    CON_GETWALL,
    CON_SETSPRITE, //+
    CON_GETSPRITE, //+
    CON_GETTSPR,
    CON_SETTSPR,
#endif

// arrays
    CON_GAMEARRAY,
    CON_SETARRAY,
    CON_GETARRAYSIZE,
    CON_RESIZEARRAY,
    CON_COPY,
///    CON_WRITEARRAYTOFILE,
///    CON_READARRAYFROMFILE,

// var ops
    CON_RANDVAR,
    CON_DISPLAYRANDVAR,
    CON_SETVAR,
    CON_ADDVAR,
    CON_SUBVAR,
    CON_MULVAR,
    CON_DIVVAR,
    CON_MODVAR,
    CON_ANDVAR,
    CON_ORVAR,
    CON_XORVAR,
    CON_SHIFTVARL,
    CON_SHIFTVARR,
// varvar ops
    CON_RANDVARVAR,
    CON_DISPLAYRANDVARVAR,
    CON_SETVARVAR,
    CON_ADDVARVAR,
    CON_SUBVARVAR,
    CON_MULVARVAR,
    CON_DIVVARVAR,
    CON_MODVARVAR,
    CON_ANDVARVAR,
    CON_ORVARVAR,
    CON_XORVARVAR,
    CON_SHIFTVARVARL,
    CON_SHIFTVARVARR,
    CON_SIN,
    CON_COS,

// random
    CON_DISPLAYRAND,

// other math
    CON_ITOF,
    CON_FTOI,
    CON_CLAMP,
    CON_INV,
    CON_SQRT,
    CON_MULSCALE,
    CON_DIVSCALE,
    CON_SCALEVAR,
    CON_DIST,
    CON_LDIST,
    CON_CALCHYPOTENUSE,
    CON_GETANGLE,
    CON_GETINCANGLE,
    CON_A2XY,
    CON_AH2XYZ,

    CON_COLLECTSECTORS,
    CON_SORT,
    CON_FOR,

// if & while var
    CON_IFVARL,
    CON_IFVARLE,
    CON_IFVARG,
    CON_IFVARGE,
    CON_IFVARE,
    CON_IFVARN,
    CON_IFVARAND,
    CON_IFVAROR,
    CON_IFVARXOR,
    CON_IFVAREITHER,
    CON_IFVARBOTH,
    CON_WHILEVARN,
    CON_WHILEVARL,

// if & while varvar
    CON_IFVARVARL,
    CON_IFVARVARLE,
    CON_IFVARVARG,
    CON_IFVARVARGE,
    CON_IFVARVARE,
    CON_IFVARVARN,
    CON_IFVARVARAND,
    CON_IFVARVAROR,
    CON_IFVARVARXOR,
    CON_IFVARVAREITHER,
    CON_IFVARVARBOTH,
    CON_WHILEVARVARN,
    CON_WHILEVARVARL,

// other if*
    CON_IFHITKEY,
    CON_IFHOLDKEY,
    CON_IFRND,

// if* using current sprite
    CON_IFANGDIFFL,
    CON_IFSPRITEPAL,
    CON_IFHIGHLIGHTED,
    CON_IFACTOR,
    CON_IFSOUND,
    CON_IFPDISTL,
    CON_IFPDISTG,
///    CON_IFGAPZL,
///    CON_IFFLOORDISTL,
///    CON_IFCEILINGDISTL,

    CON_IFINSIDE,

    CON_IFEITHERALT,
    CON_IFEITHERCTRL,
    CON_IFEITHERSHIFT,
    CON_IFAWAYFROMWALL,
    CON_IFCANSEE,
    CON_IFONWATER,
    CON_IFINWATER,
    CON_IFOUTSIDE,
///    CON_IFHITSPACE,
///    CON_IFINSPACE,
///    CON_IFINOUTERSPACE,
///    CON_IFCANSEETARGET,
    CON_IFNOSOUNDS,
    CON_IFIN3DMODE,
    CON_IFAIMINGSPRITE,
    CON_IFAIMINGWALL,
    CON_IFAIMINGSECTOR,
    CON_IFINTERACTIVE,

// keyboard
    CON_RESETKEY,
    CON_SETKEY,

// BUILD functions
    CON_INSERTSPRITE,
    CON_DUPSPRITE,
    CON_TDUPSPRITE,
    CON_DELETESPRITE,
    CON_GETSPRITELINKTYPE,
    CON_LASTWALL,
    CON_UPDATECURSECTNUM,
    CON_UPDATESECTOR,
    CON_UPDATESECTORZ,
    CON_GETZRANGE,
    CON_CLIPMOVE,
    CON_LINEINTERSECT,
    CON_RAYINTERSECT,
    CON_HITSCAN,
    CON_CANSEE,
    CON_CANSEESPR,
    CON_NEARTAG,
    CON_ROTATEPOINT,
    CON_DRAGPOINT,
    CON_GETCEILZOFSLOPE,
    CON_GETFLORZOFSLOPE,
    CON_ALIGNCEILSLOPE,
    CON_ALIGNFLORSLOPE,
    CON_BSETSPRITE,  // was CON_SETSPRITE
    CON_SETFIRSTWALL,
    CON_CHANGESPRITESTAT,
    CON_CHANGESPRITESECT,
    CON_HEADSPRITESTAT,
    CON_PREVSPRITESTAT,
    CON_NEXTSPRITESTAT,
    CON_HEADSPRITESECT,
    CON_PREVSPRITESECT,
    CON_NEXTSPRITESECT,
    CON_SECTOROFWALL,
    CON_FIXREPEATS,
    CON_GETCLOSESTCOL,

// stuff
    CON_UPDATEHIGHLIGHT,
    CON_UPDATEHIGHLIGHTSECTOR,
    CON_SETHIGHLIGHT,
    CON_SETHIGHLIGHTSECTOR,
    CON_ADDLOGVAR,
    CON_ADDLOG,
    CON_DEBUG,

// strings
    CON_DEFINEQUOTE,
    CON_REDEFINEQUOTE,
    CON_PRINT,
    CON_QUOTE,
    CON_ERRORINS,
    CON_PRINTMESSAGE16,
    CON_PRINTMESSAGE256,
    CON_PRINTEXT256,
    CON_PRINTEXT16,
    CON_DRAWLABEL,
    CON_GETNUMBER16,
    CON_GETNUMBER256,
    CON_GETNUMBERFROMUSER,
    CON_QSPRINTF,
    CON_QSTRCAT,
    CON_QSTRCPY,
    CON_QSTRLEN,
//    CON_QGETSYSSTR,
    CON_QSTRNCAT,
    CON_QSUBSTR,

// findnear*
    CON_FINDNEARSPRITE,
    CON_FINDNEARSPRITEVAR,
    CON_FINDNEARSPRITE3D,
    CON_FINDNEARSPRITE3DVAR,
    CON_FINDNEARSPRITEZ,
    CON_FINDNEARSPRITEZVAR,

    CON_GETTICKS,
    CON_GETTIMEDATE,
    CON_SETASPECT,

// vvv stuff using current sprite
    CON_SETI,
    CON_SIZEAT,
    CON_CSTAT,
    CON_CSTATOR,
    CON_CLIPDIST,
    CON_SPRITEPAL,
    CON_CACTOR,
    CON_SPGETLOTAG,
    CON_SPGETHITAG,
    CON_SECTGETLOTAG,
    CON_SECTGETHITAG,
    CON_GETTEXTUREFLOOR,
    CON_GETTEXTURECEILING,
///    CON_KILLIT,

// left to define later/undecided

// sound
    CON_SOUNDVAR,
    CON_SOUNDONCEVAR,
    CON_STOPALLSOUNDS,
    CON_STOPSOUNDVAR,
    CON_GLOBALSOUNDVAR,
    CON_GETSOUNDFLAGS,
///    CON_SOUND,
///    CON_SOUNDONCE,
///    CON_STOPSOUND,
///    CON_GLOBALSOUND,

// drawing
///    CON_MYOS,
///    CON_MYOSPAL,
///    CON_MYOSX,
///    CON_MYOSPALX,
///    CON_MINITEXT,
///    CON_GAMETEXT,
///    CON_DIGITALNUMBER,
///    CON_SHOWVIEW,
///    CON_GAMETEXTZ,
///    CON_DIGITALNUMBERZ,
    CON_DRAWLINE16,
    CON_DRAWLINE16B,
    CON_DRAWLINE16Z,
    CON_DRAWCIRCLE16,
    CON_DRAWCIRCLE16B,
    CON_DRAWCIRCLE16Z,
    CON_ROTATESPRITEA,
    CON_ROTATESPRITE16,
    CON_ROTATESPRITE,
    CON_SETGAMEPALETTE,

///    CON_TIME,
///    CON_GETANGLETOTARGET,
///    CON_ANGOFF,
///    CON_ANGOFFVAR,
///    CON_PRECACHE,
///    CON_SAVEGAMEVAR,
///    CON_READGAMEVAR,
///    CON_SETDEFNAME,
///    CON_SETCFGNAME,

    CON_END
};

#endif
