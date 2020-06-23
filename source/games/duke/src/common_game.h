//
// Definitions of common game-only data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_GAME_H_
#define EDUKE32_COMMON_GAME_H_

#include "gamecontrol.h"

BEGIN_DUKE_NS


#define DUKE                (g_gameType & GAMEFLAG_DUKE)
#define RR                  (g_gameType & GAMEFLAG_RRALL)
#define RRRA                (g_gameType & GAMEFLAG_RRRA)
#define NAM                 (g_gameType & GAMEFLAG_NAM)
#define NAPALM              (g_gameType & GAMEFLAG_NAPALM)
#define WW2GI               (g_gameType & GAMEFLAG_WW2GI)
#define NAM_WW2GI           (g_gameType & (GAMEFLAG_NAM|GAMEFLAG_WW2GI))
#define SHAREWARE           (g_gameType & GAMEFLAG_SHAREWARE)
#define DEER                (g_gameType & GAMEFLAG_DEER)
//#define DUKEBETA            ((g_gameType & GAMEFLAG_DUKEBETA) == GAMEFLAG_DUKEBETA)
//#define IONMAIDEN           (g_gameType & GAMEFLAG_IONMAIDEN)

enum Games_t {
    GAME_DUKE = 0,
    GAME_RR,
    GAME_RRRA,
    GAME_NAM,
    GAME_NAPALM,
    //GAME_WW2GI,
    GAMECOUNT
};

enum basepal_t {
    BASEPAL = 0,
    WATERPAL,
    SLIMEPAL,
    TITLEPAL,
    DREALMSPAL,
    ENDINGPAL,  // 5
    ANIMPAL,    // not used anymore
    DRUGPAL,
    BASEPALCOUNT
};

#include "v_text.h"


extern int loaddefinitions_game(const char *fn, int32_t preload);

//////////

extern void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN);
extern void G_SetupGlobalPsky(void);

//////////

extern void genspriteremaps(void);

//////////

END_DUKE_NS

#endif
