//
// Definitions of common game-only data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_GAME_H_
#define EDUKE32_COMMON_GAME_H_

#define GAMEFLAG_DUKE       0x00000001
#define GAMEFLAG_NAM        0x00000002
#define GAMEFLAG_NAPALM     0x00000004
#define GAMEFLAG_WW2GI      0x00000008
#define GAMEFLAG_ADDON      0x00000010
#define GAMEFLAG_SHAREWARE  0x00000020
#define GAMEFLAG_DUKEBETA   0x00000060 // includes 0x20 since it's a shareware beta

extern int32_t g_gameType;

#define DUKE                (g_gameType & GAMEFLAG_DUKE)
#define NAM                 (g_gameType & GAMEFLAG_NAM)
#define NAPALM              (g_gameType & GAMEFLAG_NAPALM)
#define WW2GI               (g_gameType & GAMEFLAG_WW2GI)
#define SHAREWARE           (g_gameType & GAMEFLAG_SHAREWARE)
#define DUKEBETA            ((g_gameType & GAMEFLAG_DUKEBETA) == GAMEFLAG_DUKEBETA)

enum Games_t {
    GAME_DUKE = 0,
    GAME_NAM,
    GAME_NAPALM,
    GAME_WW2GI,
    GAMECOUNT
};

enum instpath_t {
    INSTPATH_STEAM,
    INSTPATH_GOG,
    NUMINSTPATHS
};

extern const char *defaultgamegrp[GAMECOUNT];
extern const char *defaultdeffilename[GAMECOUNT];
extern const char *defaultconfilename;
extern const char *defaultgameconfilename[GAMECOUNT];

extern char *g_grpNamePtr;
extern char *g_scriptNamePtr;

extern const char *G_DefaultGrpFile(void);
extern const char *G_GrpFile(void);

extern const char *G_DefaultConFile(void);
extern const char *G_ConFile(void);

extern void clearGrpNamePtr(void);
extern void clearDefNamePtr(void);
extern void clearScriptNamePtr(void);

//////////

extern void G_InitMultiPsky(int32_t CLOUDYOCEAN__DYN, int32_t MOONSKY1__DYN, int32_t BIGORBIT1__DYN, int32_t LA__DYN);
extern void G_SetupGlobalPsky(void);

// Get the multi-psky index corresponding to a base tile number of the sky (for
// the game: the *static* value!), or 0 as the catch-all.
// KEEPINSYNC G_InitMultiPsky().
static inline int32_t MultiPsky_TileToIdx(int32_t tilenum)
{
    switch (tilenum)
    {
    default:
        return 0;
    case 78: // CLOUDYOCEAN__STATIC
        return 1;
    case 80: // MOONSKY1__STATIC:
        return 2;
    case 84: // BIGORBIT1__STATIC:
        return 3;
    case 89: // LA__STATIC:
        return 4;
    }
}

//////////

extern void G_ExtPreInit(void);

extern void G_AddSearchPaths(void);
extern void G_CleanupSearchPaths(void);

extern const char * G_GetInstallPath(int32_t insttype);

#endif
