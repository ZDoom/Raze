//-------------------------------------------------------------------------
/*
Copyright (C) 2017 EDuke32 developers and contributors

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

/// Preprocessor directives:
///
/// ITERATE_ON_UPDATE:  Every time the netcode updates the sprite linked lists, attempt to iterate through all of them.
/// SPRLIST_PRINT:      Print in the console every time a sprite's status in the link list changes due to <see cref="Net_UpdateSpriteLinkedLists">
/// PACKET_RECV_PRINT:  Print all received packets
/// NETCODE_DISABLE:    Remove pretty much all of the game's interaction with the code in this file.

#include "duke3d.h"
#include "game.h"
#include "gamedef.h"
#include "network.h"
#include "premap.h"
#include "savegame.h"
#include "input.h"

#include "enet/enet.h"
#include "lz4.h"
#include "crc32.h"

#include "vfs.h"

// Data needed even if netcode is disabled
ENetHost    *g_netServer     = NULL;
ENetHost    *g_netClient     = NULL;
ENetPeer    *g_netClientPeer = NULL;
enet_uint16 g_netPort       = 23513;
int32_t     g_netDisconnect = 0;
char        g_netPassword[32];
int32_t     g_networkMode       = NET_CLIENT;

// to support (gcc only) -f-strict-aliasing, the netcode needs to specify that its 32 bit chunks to and from the
// packet code should not be subject to strict aliasing optimizations
// it would appear that only GCC does this, Clang doesn't seem to and Microsoft Visual C++ definitely doesn't.
#ifdef __GNUC__
#define TYPE_PUNNED __attribute__((__may_alias__))
#else
#define TYPE_PUNNED
#endif

typedef TYPE_PUNNED int32_t NetChunk32;


// Unfortunately faketimerhandler needs extra "help" because the Build Engine source doesn't include network.h.
#ifdef NETCODE_DISABLE
void faketimerhandler(void)
{
    ;
}
#else
void faketimerhandler(void)
{
    if (g_netServer == NULL && g_netClient == NULL)
        return;

    enet_host_service(g_netServer ? g_netServer : g_netClient, NULL, 0);
}

static void Net_Disconnect(void);
static void Net_HandleClientPackets(void);
static void Net_HandleServerPackets(void);
#endif

void Net_GetPackets(void)
{
    timerUpdate();
    MUSIC_Update();
    S_Update();

    G_HandleSpecialKeys();

#ifndef NETCODE_DISABLE
    if (g_netDisconnect)
    {
        Net_Disconnect();
        g_netDisconnect = 0;

        if (g_gameQuit)
            G_GameExit(" ");

        return;
    }

    if (g_netServer)
    {
        Net_HandleClientPackets();
    }
    else if (g_netClient)
    {
        Net_HandleServerPackets();
    }
#endif
}

// (the rest of the file)
#ifndef NETCODE_DISABLE

// this attribute is for variables that are only used to highlight interesting conditions in the code
// when I'm in a debugger.
#define NET_DEBUG_VAR EDUKE32_UNUSED

// internal data
#define SNAPSHOTS_TO_SAVE 32
#define MAX_SNAPSHOT_ACTORS 256
#define MAX_SNAPSHOT_WALLS MAXWALLS      // 16384
#define MAX_SNAPSHOT_SECTORS MAXSECTORS  // 4096


////////////////////////////////////////////////////////////////////////////////
// Server Update Packets

#pragma pack(push, 1)
typedef struct serverupdate_s
{
    uint8_t header;
    uint8_t numplayers;
    input_t nsyn;
    int32_t seed;
    int16_t pause_on;
} serverupdate_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct serverplayerupdate_s
{
    uint8_t  extra;
    int16_t  cstat;
    uint16_t owner;
    uint16_t picnum;
    uint16_t gotweapon;
    uint8_t  kickback_pic;
    uint8_t  frags[MAXPLAYERS];
    int16_t  inv_amount[GET_MAX];
    int16_t  ammo_amount[MAX_WEAPONS];

    uint8_t  curr_weapon;
    uint8_t  last_weapon;
    uint8_t  wantweaponfire;
    uint8_t  weapon_pos;
    uint8_t  frag_ps;
    uint8_t  frag;
    uint8_t  fraggedself;
    uint8_t  last_extra;
    uint8_t  pal;
    uint16_t ping;
    uint16_t newowner;

    playerupdate_t player;
} serverplayerupdate_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct clientupdate_s
{
    uint8_t        header;
    int32_t        RevisionNumber;
    input_t        nsyn;
    playerupdate_t player;
} clientupdate_t;
#pragma pack(pop)


// Client player ready for world updates (player spawned and ready)
static int ClientPlayerReady = 0;

// all zero NetActor
static const netactor_t cNullNetActor = netactor_t();

// all zero NetWall
static const netWall_t cNullNetWall = netWall_t();

// all zero NetSector
static const netSector_t cNullNetSector = netSector_t();

// the revision number reserved for the state of the map when it was loaded from the .map file.
// NOTE: Don't confuse this with a revision index. Index 0 into the snapshot history is NOT the initial map state.
// g_MapStateHistory[0] could be revision 0, revision 64, revision 128, etc...
static const uint32_t cInitialMapStateRevisionNumber = 0;
// the revision array starts here instead of zero (may as well have the initial map state be revision 0)...
static const uint32_t cStartingRevisionIndex = 1;

static const int32_t cLocSprite_DeletedSpriteStat = MAXSTATUS;
#ifdef CURRENTLY_UNUSED
static const int32_t cNetSprite_DeletedSpriteStat = STAT_NETALLOC;
#endif

//[75]  When a client attempts to allocate a sprite during the game loop, which is not defined as a clientside sprite,
//      the insert will go through, and the sprite will be put on this list.
//
//      every time the client gets a new snapshot, this entire list will be deleted

static int32_t headscratchpadsprite = -1;
static int32_t nextscratchpadsprite[MAXSPRITES];


static char    recbuf[180];
static int32_t g_chatPlayer = -1;

// note that NETINDEX_BITS must be large enough to store the largest of
// MAXWALLS, MAXSECTORS, and MAXSPRITES, plus one bit so that the stop
// code won't be mistaken for a valid index
//
// as of 5/4/2016 MAXSPRITES is 16384, which technically only needs 14 bits but I'm going to
// set this to 16 + 1 to make it a more even number
//
// also NETINDEX_BITS should not exceed 32
#define NETINDEX_BITS (16 + 1)


// worst case: 2 bits per field per stuct if every field in every struct is changed
#define WORLD_CHANGEBITSSIZE                                                                                                               \
    (MAXWALLS * ARRAY_SIZE(WallFields) * 2) + (MAXSECTORS * ARRAY_SIZE(SectorFields) * 2) + (MAXSPRITES * ARRAY_SIZE(ActorFields) * 2)

//                              each changed entry has a netindex + stop codes                  bits to indicate whether a field
//                              changed/zeroed                 convert to bytes....
#define WORLD_OVERHEADSIZE (((MAXSECTORS + MAXWALLS + MAXSPRITES + 3) * NETINDEX_BITS + WORLD_CHANGEBITSSIZE) >> 8) + 1


typedef struct netField_s
{
    const char  *name;      // field name
    int32_t     offset;     // offset from the start of the entity struct
    int32_t     bits;       // field size

} netField_t;

#define	SECTF(x) #x,(int32_t)(size_t)&((netSector_t*)0)->x

static netField_t SectorFields[] =
{
    { SECTF(wallptr),                16 },
    { SECTF(wallnum),                16 },

    { SECTF(ceilingz),                   32 },
    { SECTF(floorz),                     32 },

    { SECTF(ceilingstat),           16 },
    { SECTF(floorstat),             16 },
    { SECTF(ceilingpicnum),         16 },
    { SECTF(ceilingheinum),         16 },

    { SECTF(ceilingshade),       8 },
    { SECTF(ceilingpal),         8 },
    { SECTF(ceilingxpanning),    8 },
    { SECTF(ceilingypanning),    8 },

    { SECTF(floorpicnum),           16 },
    { SECTF(floorheinum),           16 },

    { SECTF(floorshade),         8 },
    { SECTF(floorpal),           8 },
    { SECTF(floorxpanning),      8 },
    { SECTF(floorypanning),      8 },
    { SECTF(visibility),         8 },
    { SECTF(fogpal),             8 },

    { SECTF(lotag),                  16 },
    { SECTF(hitag),                  16 },
    { SECTF(extra),                  16 },

};

#undef SECTF


#define	WALLF(x) #x,(int32_t)(size_t)&((netWall_t*)0)->x

static netField_t WallFields[] =
{
    { WALLF(x),                          32 },
    { WALLF(y),                          32 },

    { WALLF(point2),                 16 },
    { WALLF(nextwall),               16 },
    { WALLF(nextsector),             16 },

    { WALLF(cstat),                  16 },
    { WALLF(picnum),                 16 },
    { WALLF(overpicnum),             16 },

    { WALLF(shade),              8 },
    { WALLF(pal),                8 },
    { WALLF(xrepeat),            8 },
    { WALLF(yrepeat),            8 },
    { WALLF(xpanning),           8 },
    { WALLF(ypanning),           8 },

    { WALLF(lotag),                  16 },
    { WALLF(hitag),                  16 },
    { WALLF(extra),                  16 },

};

#undef WALLF


#define	ACTF(x) #x,(int32_t)(size_t)&((netactor_t*)0)->x

static netField_t ActorFields[] =
{
    { ACTF(t_data_0),                   32 },
    { ACTF(t_data_1),                   32 },
    { ACTF(t_data_2),                   32 },
    { ACTF(t_data_3),                   32 },
    { ACTF(t_data_4),                   32 },
    { ACTF(t_data_5),                   32 },
    { ACTF(t_data_6),                   32 },
    { ACTF(t_data_7),                   32 },
    { ACTF(t_data_8),                   32 },
    { ACTF(t_data_9),                   32 },

#ifdef LUNATIC
    // need to update this section if LUNATIC is ever brought back in
    { ACTF(hvel),               16 },
    { ACTF(vvel),               16 },


    { ACTF(startframe),         16 },
    { ACTF(numframes),          16 },

    { ACTF(viewtype),           16 },
    { ACTF(incval),             16 },
    { ACTF(delay),              16 },
#endif

    { ACTF(flags),                      32 },

    { ACTF(bpos_x),                     32 },
    { ACTF(bpos_y),                     32 },
    { ACTF(bpos_z),                     32 },

    { ACTF(floorz),                     32 },
    { ACTF(ceilingz),                   32 },
    { ACTF(lastvx),                     32 },
    { ACTF(lastvy),                     32 },

    { ACTF(lasttransport),  8},

    { ACTF(picnum),             16 },
    { ACTF(ang),                16 },
    { ACTF(extra),              16 },
    { ACTF(owner),              16 },

    { ACTF(movflag),            16 },
    { ACTF(tempang),            16 },
    { ACTF(timetosleep),        16 },

    { ACTF(stayput),            16 },
    { ACTF(dispicnum),          16 },

#if defined LUNATIC
    { ACTF(movflags),           16 },
#endif

    { ACTF(cgg),            8},

    //------------------------------------------------------
    // sprite fields

    { ACTF(spr_x),                          32 },
    { ACTF(spr_y),                          32 },
    { ACTF(spr_z),                          32 },

    { ACTF(spr_cstat),          16 },

    { ACTF(spr_picnum),         16 },
    { ACTF(spr_shade),      8 },
    { ACTF(spr_pal),        8 },
    { ACTF(spr_clipdist),   8 },
    { ACTF(spr_blend),      8 },
    { ACTF(spr_xrepeat),    8 },
    { ACTF(spr_yrepeat),    8 },
    { ACTF(spr_xoffset),    8 },
    { ACTF(spr_yoffset),    8 },

    { ACTF(spr_sectnum),        16 },
    { ACTF(spr_statnum),        16 },

    { ACTF(spr_ang),            16 },
    { ACTF(spr_owner),          16 },
    { ACTF(spr_xvel),           16 },
    { ACTF(spr_yvel),           16 },
    { ACTF(spr_zvel),           16 },

    { ACTF(spr_lotag),          16 },
    { ACTF(spr_hitag),          16 },

    { ACTF(spr_extra),          16 },

    //--------------------------------------------------------------
    //spriteext fields

    { ACTF(ext_mdanimtims),                 32 },
    { ACTF(ext_mdanimcur),      16 },
    { ACTF(ext_angoff),         16 },
    { ACTF(ext_pitch),          16 },
    { ACTF(ext_roll),           16 },

    { ACTF(ext_offset_x),                   32 },
    { ACTF(ext_offset_y),                   32 },
    { ACTF(ext_offset_z),                   32 },

    { ACTF(ext_flags),      8 },
    { ACTF(ext_xpanning),   8 },
    { ACTF(ext_ypanning),   8 },
    { ACTF(ext_alpha),                                  0 }, // float

    //--------------------------------------------------------------
    //spritesmooth fields

    { ACTF(sm_smoothduration),                          0 }, // float
    { ACTF(sm_mdcurframe),      16 },
    { ACTF(sm_mdoldframe),      16 },
    { ACTF(sm_mdsmooth),        16 },



};
#undef ACTF

// actual game struct data size
#define WORLD_DATASIZE (MAXSECTORS * sizeof(netSector_t) + MAXWALLS * sizeof(netWall_t) + MAXSPRITES * sizeof(netactor_t))

// max packet array size
#define MAX_WORLDBUFFER WORLD_DATASIZE + WORLD_OVERHEADSIZE

#ifdef CURRENTLY_UNUSED
// Just so you can get an idea of how much memory the netcode needs...
static const int64_t cWORLD_DataSize     = WORLD_DATASIZE;
static const int64_t cWORLD_OverheadSize = WORLD_OVERHEADSIZE;
static const int64_t cWORLD_TotalSize    = MAX_WORLDBUFFER;

// ...it's pretty big for now (!)
static const int64_t SnapshotArraySize = sizeof(netmapstate_t) * NET_REVISIONS;
#endif


// both the client and server store their current revision number here,
// when the client sends their revision to the server,
// the server stores it in g_player[that player's index].revision
static uint32_t g_netMapRevisionNumber = 0;

// In addition to the client keeping track of what revision the server is using
// (g_netRevisionNumber), it also increments its own revision that represents
// what version of the game the client has interpolated to.
static uint32_t g_cl_InterpolatedRevision = 0;

static netmapstate_t g_mapStartState;

static netmapstate_t g_cl_InterpolatedMapStateHistory[NET_REVISIONS];

// note that the map state number is not an index into here,
// to get the index into this array out of a map state number, do <Map state number> % NET_REVISONS
static netmapstate_t g_mapStateHistory[NET_REVISIONS];
static uint8_t       tempnetbuf[MAX_WORLDBUFFER];

// Remember that this constant needs to be one bit longer than a struct index, so it can't be mistaken for a valid wall, sprite, or sector index
static const int32_t cSTOP_PARSING_CODE = ((1 << NETINDEX_BITS) - 1);

static uint32_t NET_75_CHECK;

// Externally available data / functions
int32_t     g_netPlayersWaiting = 0;
int32_t     g_netIndex          = 2;
newgame_t   pendingnewgame;
bool        g_enableClientInterpolationCheck = true;


// Internal functions
static void Net_ReadWorldUpdate(uint8_t *packetData, int32_t packetSize);


//Adds a sprite with index 'spriteIndex' to the netcode's internal scratch sprite list,
//this does NOT allocate a new sprite or insert it into the other arrays.
//
//ONLY use this to insert sprites inserted between snapshots that are NOT defined to be clientside.
static void Net_InsertScratchPadSprite(int spriteIndex)
{
    if (!g_netClient)
    {
        return;
    }

    Bassert(spriteIndex < MAXSPRITES);
    Bassert(spriteIndex >= 0);

    int16_t const oldHead = headscratchpadsprite;

    nextscratchpadsprite[spriteIndex] = oldHead;

    headscratchpadsprite = spriteIndex;

#ifdef SPRLIST_PRINT
    OSD_Printf("DEBUG: Inserted scratch pad sprite %d\n", spriteIndex);
#endif

}

static void Net_DeleteFromSect(int16_t spriteIndex)
{
    if(spriteIndex >= MAXSPRITES)
    {
        return;
    }

    if(sprite[spriteIndex].sectnum >= MAXSECTORS)
    {
        return;
    }

    do_deletespritesect(spriteIndex);
}

static void Net_DeleteFromStat(int16_t spriteIndex)
{
    if(spriteIndex >= MAXSPRITES)
    {
        return;
    }

    if(sprite[spriteIndex].statnum >= MAXSTATUS)
    {
        return;
    }

    do_deletespritestat(spriteIndex);
}

static void Net_DoDeleteSprite(int32_t spritenum)
{
    NET_75_CHECK++; // Need to add a check to Net_DoDeleteSprite so that the client does not give sprites that were not previously STAT_NETALLOC back to STAT_NETALLOC,
                    // otherwise the client is leaking non-STAT_NETALLOC sprites.
                    //
                    // may want to add a flag to actor_t for SFLAG_SCRATCHSPRITE and SFLAG_CLIENTSIDE

    if (sprite[spritenum].statnum == STAT_NETALLOC)
    {
        return;
    }

    changespritestat(spritenum, STAT_NETALLOC);

    Net_DeleteFromSect(spritenum);
    sprite[spritenum].sectnum = MAXSECTORS;
}

static void Net_InitScratchPadSpriteList()
{
    if (!g_netClient)
    {
        return;
    }

    headscratchpadsprite = -1;
    Bmemset(&nextscratchpadsprite[0], -1, sizeof(nextscratchpadsprite));
}

static void Net_DeleteAllScratchPadSprites()
{
    if (!g_netClient)
    {
        return;
    }

    int16_t spriteIndex = headscratchpadsprite;

    for (spriteIndex = headscratchpadsprite; spriteIndex >= 0; spriteIndex = nextscratchpadsprite[spriteIndex])
    {
        Net_DoDeleteSprite(spriteIndex);
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Deleted scratch pad sprite (set to STAT_NETALLOC) %d\n", spriteIndex);
#endif
    }

    Net_InitScratchPadSpriteList();
}

static void Net_Error_Disconnect(const char* message)
{
    OSD_Printf("Net_Error_Disconnect: %s\n", message);
    // Here I could longjmp to the game main loop and unload the map somehow
    //
    // If we go to C++ it may be a good idea to throw an exception
    //
    Bassert(0);
}

static void Net_InitNetActor(netactor_t *netActor)
{

    *(netActor) = cNullNetActor;

    netActor->netIndex = cSTOP_PARSING_CODE;
}

// Low level "Copy net structs to / from game structs" functions
//------------------------------------------------------------------------------
// Net -> Game Arrays
//------------------------------------------------------------------------------

static void Net_CopyWallFromNet(netWall_t* netWall, walltype* gameWall)
{
    // (convert data from 32 bit integers)

    Bassert(netWall);
    Bassert(gameWall);

    gameWall->point2 = netWall->point2;
    gameWall->nextwall = netWall->nextwall;
    gameWall->nextsector = netWall->nextsector;

    gameWall->cstat = netWall->cstat;

    gameWall->picnum = netWall->picnum;
    gameWall->overpicnum = netWall->overpicnum;

    gameWall->shade = netWall->shade;

    gameWall->pal = netWall->pal;
    gameWall->xrepeat = netWall->xrepeat;
    gameWall->yrepeat = netWall->yrepeat;
    gameWall->xpanning = netWall->xpanning;
    gameWall->ypanning = netWall->ypanning;

    gameWall->lotag = netWall->lotag;
    gameWall->hitag = netWall->hitag;

    gameWall->extra = netWall->extra;

    int positionChanged = (netWall->x != gameWall->x) ||
        (netWall->y != gameWall->y);

    if (positionChanged)
    {
        dragpoint(netWall->netIndex, netWall->x, netWall->y, 0);
    }

    // unfortunately I don't know of any fields that would be guaranteed to be nonzero in here for error checking purposes.
    NET_75_CHECK++; // I should initialize netWall_t fields to some nonsense value other than zero, or maybe check for netWall->netIndex == cSTOP_PARSING_CODE?


}

static void Net_CopySectorFromNet(netSector_t* netSector, sectortype* gameSector)
{
    Bassert(gameSector);
    Bassert(netSector);

    // (convert data from 32 bit integers)

    gameSector->wallptr = netSector->wallptr;
    gameSector->wallnum = netSector->wallnum;

    gameSector->ceilingz = netSector->ceilingz;
    gameSector->floorz = netSector->floorz;

    gameSector->ceilingstat = netSector->ceilingstat;
    gameSector->floorstat = netSector->floorstat;

    gameSector->ceilingpicnum = netSector->ceilingpicnum;
    gameSector->ceilingheinum = netSector->ceilingheinum;

    gameSector->ceilingshade = netSector->ceilingshade;

    gameSector->ceilingpal = netSector->ceilingpal;
    gameSector->ceilingxpanning = netSector->ceilingxpanning;
    gameSector->ceilingypanning = netSector->ceilingypanning;

    gameSector->floorpicnum = netSector->floorpicnum;
    gameSector->floorheinum = netSector->floorheinum;

    gameSector->floorshade = netSector->floorshade;

    gameSector->floorpal = netSector->floorpal;
    gameSector->floorxpanning = netSector->floorxpanning;
    gameSector->floorypanning = netSector->floorypanning;
    gameSector->visibility = netSector->visibility;
    gameSector->fogpal = netSector->fogpal;

    gameSector->lotag = netSector->lotag;
    gameSector->hitag = netSector->hitag;

    gameSector->extra = netSector->extra;

    // sanity check
    if (gameSector->wallnum <= 0)
    {
        Net_Error_Disconnect("Net_CopySectorFromNet: Invalid wallnum from server.");
    }

}


// Try to catch infinite loops in the sprite linked lists, unfortunately this rarely works.
EDUKE32_UNUSED static void Test_Iterate()
{
    int32_t watchdogIndex = 0;
    const int32_t cThreshold = MAXSPRITES * 2;

    for (int32_t statnum = 0; statnum < MAXSTATUS; statnum++)
    {
        watchdogIndex = 0;

        for (int32_t spriteIndex = headspritestat[statnum]; spriteIndex >= 0; spriteIndex = nextspritestat[spriteIndex])
        {
            watchdogIndex++;

            if (watchdogIndex > cThreshold)
            {
                Bassert(watchdogIndex <= cThreshold);
            }
        }
    }

    for (int32_t sectnum = 0; sectnum < MAXSECTORS; sectnum++)
    {
        watchdogIndex = 0;

        for (int32_t spriteIndex = headspritesect[sectnum]; spriteIndex >= 0; spriteIndex = nextspritesect[spriteIndex])
        {
            watchdogIndex++;

            if (watchdogIndex > cThreshold)
            {
                Bassert(watchdogIndex <= cThreshold);
            }
        }
    }
}

static void Net_UpdateSpriteLinkedLists(int16_t spriteIndex, const netactor_t* snapActor)
{
    if ((spriteIndex >= MAXSPRITES) || (spriteIndex < 0))
    {
        Net_Error_Disconnect("Can't update linked lists. Actor index invalid.");
    }

    int16_t oldGameStatnum = sprite[spriteIndex].statnum;

    //                                                              changes to game lists for this sprite:
    //
    //                                                                  stats       sectors
    //                                                              -------------------------
    // 1. Game sprite stat == snap sprite stat                      --  nothing     nothing
    //
    //
    // this leaks a STAT_NETALLOC
    // 2. game sprite <any stat>, snap sprite MAXSTATUS             --  delete      delete
    //
    // this leaks a client sprite
    // 3. game sprite MAXSTATUS, snap sprite STAT_NETALLOC or normal--  insert      insert
    //                           (implied in `else`)
    //
    // 4. game sprite STAT_NETALLOC, snap sprite Normal             --  change      insert
    //
    // 5. game sprite Normal, snap sprite STAT_NETALLOC             --  change      delete
    //
    // 6. game sprite Normal, snap sprite Normal                    --  change      change


    // [1]
    if (snapActor->spr_statnum == oldGameStatnum)
    {
        return;
    }

    bool snapActorIsNormalStat =    (snapActor->spr_statnum != MAXSTATUS)   && (snapActor->spr_statnum != STAT_NETALLOC);
    bool gameSpriteIsNormalStat =   (oldGameStatnum != MAXSTATUS)           && (oldGameStatnum != STAT_NETALLOC);



    // [2]
    if (                                                snapActor->spr_statnum == MAXSTATUS)
    {
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Sprite %d: Case 2 (delete game sprite)\n", spriteIndex);
#endif

        Net_DeleteFromStat(spriteIndex);
        Net_DeleteFromSect(spriteIndex);

        return;
    }

    // [3]
    else if (oldGameStatnum == MAXSTATUS)
    {
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Sprite %d: Case 3 (insert new game sprite)\n", spriteIndex);
#endif

        do_insertsprite_at_headofstat(spriteIndex, snapActor->spr_statnum);
        do_insertsprite_at_headofsect(spriteIndex, snapActor->spr_sectnum);

    }

    // [4]
    else if ((oldGameStatnum == STAT_NETALLOC) &&  snapActorIsNormalStat)
    {
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Sprite %d: Case 4 (STAT_NETALLOC to stat %d)\n", spriteIndex, snapActor->spr_statnum);
#endif

        changespritestat(spriteIndex, snapActor->spr_statnum);
        do_insertsprite_at_headofsect(spriteIndex, snapActor->spr_sectnum);

    }

    // [5]
    else if (gameSpriteIsNormalStat &&                  (snapActor->spr_statnum == STAT_NETALLOC))
    {
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Sprite %d: Case 5 (normal to STAT_NETALLOC)\n", spriteIndex);
#endif

        changespritestat(spriteIndex, snapActor->spr_statnum);
        Net_DeleteFromSect(spriteIndex);

    }

    // [6]
    else if(gameSpriteIsNormalStat &&                  snapActorIsNormalStat)
    {
#ifdef SPRLIST_PRINT
        OSD_Printf("DEBUG: Sprite %d: Case 6 (normal stat to normal stat)\n", spriteIndex);
#endif

        // note that these functions handle cases where the game sprite already has that stat/sectnum
        changespritestat(spriteIndex, snapActor->spr_statnum);
        changespritesect(spriteIndex, snapActor->spr_sectnum);
    }

#ifdef SPRLIST_PRINT
    OSD_Printf("DEBUG: Sprite %d next is %d, prev is %d, head of stat %d is %d\n", spriteIndex, nextspritestat[spriteIndex], prevspritestat[spriteIndex], snapActor->spr_statnum, headspritestat[snapActor->spr_statnum]);
#endif

    NET_DEBUG_VAR bool invalid = (
                        (nextspritesect[spriteIndex] == spriteIndex)
                        && (nextspritestat[spriteIndex] == spriteIndex)
                    );

    Bassert(!invalid);

#ifdef ITERATE_ON_UPDATE
    Test_Iterate();
#endif


}


static void Net_CopySpriteFromNet(const netactor_t* netActor, spritetype* gameSprite)
{
    Bassert(netActor);
    Bassert(gameSprite);

    // NOTE: Don't call Net_UpdateSpriteLinkedLists here, that should only be called
    //       if the sprite isn't already deleted in the game arrays.
    gameSprite->x = netActor->spr_x;
    gameSprite->y = netActor->spr_y;
    gameSprite->z = netActor->spr_z;

    // don't set statnum / sectnum here, that should be done in Net_UpdateSpriteLinkedLists,
    // otherwise it's harder than it has to be because most of the engine functions
    // asssume that if the sprite's stat / sectnum matches there's nothing to do as
    // far as the linked lists are concerned

    gameSprite->cstat = netActor->spr_cstat;
    gameSprite->picnum = netActor->spr_picnum;
    gameSprite->shade = netActor->spr_shade;
    gameSprite->pal = netActor->spr_pal;
    gameSprite->clipdist = netActor->spr_clipdist;
    gameSprite->blend = netActor->spr_blend;

    gameSprite->xrepeat = netActor->spr_xrepeat;
    gameSprite->yrepeat = netActor->spr_yrepeat;

    gameSprite->xoffset = netActor->spr_xoffset;
    gameSprite->yoffset = netActor->spr_yoffset;



    gameSprite->ang = netActor->spr_ang;
    gameSprite->owner = netActor->spr_owner;
    gameSprite->xvel = netActor->spr_xvel;
    gameSprite->yvel = netActor->spr_yvel;
    gameSprite->zvel = netActor->spr_zvel;

    gameSprite->lotag = netActor->spr_lotag;
    gameSprite->hitag = netActor->spr_hitag;

    gameSprite->extra = netActor->spr_extra;
}



static void Net_CopyActorFromNet(const netactor_t* netActor, actor_t *gameActor)
{
    // (convert data from 32 bit integers)

    Bassert(netActor);
    Bassert(gameActor);

    // This seemed to make enemy movements smoother.
    bool aiIDChanged = (gameActor->t_data[5] != netActor->t_data_5);

    // If the sprite is a CON sprite, don't overwrite AC_Action_Count
    bool isActor = G_HaveActor(netActor->spr_picnum);

    // Fixes ambient sound infinite sound replay glitch (stand in the outdoor area of E1L1, the "airplane noise" will get very loud and loop endlessly.
    bool isSoundActor = (DYNAMICTILEMAP(netActor->picnum) == MUSICANDSFX);

    if(!isSoundActor)
    {
        gameActor->t_data[0] = netActor->t_data_0;
        gameActor->t_data[1] = netActor->t_data_1;
        gameActor->t_data[4] = netActor->t_data_4;
        gameActor->t_data[5] = netActor->t_data_5;
    }


    // Prevents:
    // - Rotating sector stuttering
    // - Trains running backwards
    bool isSyncedSE =   (DYNAMICTILEMAP(netActor->picnum) == SECTOREFFECTOR) &&
                        (
                                (netActor->spr_lotag == SE_0_ROTATING_SECTOR)
                            ||  (netActor->spr_lotag == SE_1_PIVOT)
                            ||  (netActor->spr_lotag == SE_6_SUBWAY)
                            ||  (netActor->spr_lotag == SE_11_SWINGING_DOOR)
                            ||  (netActor->spr_lotag == SE_14_SUBWAY_CAR)
                            ||  (netActor->spr_lotag == SE_30_TWO_WAY_TRAIN)
                        );



    if (aiIDChanged || isSyncedSE)
    {
        gameActor->t_data[2] = netActor->t_data_2;
    }

    if (aiIDChanged || !isActor)
    {
        gameActor->t_data[3] = netActor->t_data_3;
    }



    gameActor->t_data[6] = netActor->t_data_6;
    gameActor->t_data[7] = netActor->t_data_7;
    gameActor->t_data[8] = netActor->t_data_8;
    gameActor->t_data[9] = netActor->t_data_9;

#ifdef LUNATIC

    gameActor->mv.hvel = netActor->hvel;
    gameActor->mv.vvel = netActor->vvel;

    gameActor->ac.startframe = netActor->startframe;
    gameActor->ac.numframes = netActor->numframes;
    gameActor->ac.viewtype = netActor->viewtype;
    gameActor->ac.incval = netActor->incval;
    gameActor->ac.delay = netActor->delay;

    gameActor->actiontics = netActor->actiontics;

#endif

    gameActor->flags = netActor->flags;

    gameActor->bpos.x = netActor->bpos_x;
    gameActor->bpos.y = netActor->bpos_y;
    gameActor->bpos.z = netActor->bpos_z;

    gameActor->floorz = netActor->floorz;
    gameActor->ceilingz = netActor->ceilingz;
    gameActor->lastv.x = netActor->lastvx;
    gameActor->lastv.y = netActor->lastvy;

    gameActor->lasttransport = netActor->lasttransport;

    //WARNING: both sprite and actor have these fields
    gameActor->picnum = netActor->picnum;
    gameActor->ang = netActor->ang;
    gameActor->extra = netActor->extra;
    gameActor->owner = netActor->owner;

    gameActor->movflag = netActor->movflag;
    gameActor->tempang = netActor->tempang;
    gameActor->timetosleep = netActor->timetosleep;

    gameActor->stayput = netActor->stayput;
    gameActor->dispicnum = netActor->dispicnum;

#if defined LUNATIC

    //WARNING: NOT the same as movflag
    gameActor->movflags = netActor->movflags;

#endif

    gameActor->cgg = netActor->cgg;
}

static void Net_CopySpriteExtFromNet(const netactor_t* netActor, spriteext_t* gameSprExt)
{
    Bassert(netActor);
    Bassert(gameSprExt);

    gameSprExt->mdanimtims = netActor->ext_mdanimtims;

    gameSprExt->mdanimcur = netActor->ext_mdanimcur;
    gameSprExt->angoff = netActor->ext_angoff;
    gameSprExt->pitch = netActor->ext_pitch;
    gameSprExt->roll = netActor->ext_roll;

    gameSprExt->offset.x = netActor->ext_offset_x;
    gameSprExt->offset.y = netActor->ext_offset_y;
    gameSprExt->offset.z = netActor->ext_offset_z;

    gameSprExt->flags = netActor->ext_flags;
    gameSprExt->xpanning = netActor->ext_xpanning;
    gameSprExt->ypanning = netActor->ext_ypanning;

    gameSprExt->alpha = netActor->ext_alpha;

}

static void Net_CopySpriteSmoothFromNet(const netactor_t* netActor, spritesmooth_t* gameSprSmooth)
{
    Bassert(netActor);
    Bassert(gameSprSmooth);

    gameSprSmooth->smoothduration = netActor->sm_smoothduration;

    gameSprSmooth->mdcurframe = netActor->sm_mdcurframe;
    gameSprSmooth->mdoldframe = netActor->sm_mdoldframe;

    gameSprSmooth->mdsmooth = netActor->sm_mdsmooth;
}

static void Net_CopyAllActorDataFromNet(const netactor_t* netActor, spritetype* gameSprite, actor_t* gameActor, spriteext_t* gameSprExt, spritesmooth_t* gameSprSmooth)
{
    Net_CopySpriteFromNet(netActor, gameSprite);
    Net_CopyActorFromNet(netActor, gameActor);
    Net_CopySpriteExtFromNet(netActor, gameSprExt);
    Net_CopySpriteSmoothFromNet(netActor, gameSprSmooth);

}

// Clients only.
static void Net_CopyPlayerSpriteFromNet(const netactor_t* netActor, spritetype* gameSprite)
{
    Bassert(netActor);
    Bassert(gameSprite);

    // We don't need to synchronize player position, ang, or sectnum, because P_ProcessInput
    // does that for all player sprites based on ps->pos and the player's input.

    gameSprite->cstat = netActor->spr_cstat;
    gameSprite->picnum = netActor->spr_picnum;
    gameSprite->shade = netActor->spr_shade;
    gameSprite->pal = netActor->spr_pal;
    gameSprite->clipdist = netActor->spr_clipdist;
    gameSprite->blend = netActor->spr_blend;

    gameSprite->xrepeat = netActor->spr_xrepeat;
    gameSprite->yrepeat = netActor->spr_yrepeat;

    gameSprite->xoffset = netActor->spr_xoffset;
    gameSprite->yoffset = netActor->spr_yoffset;

    gameSprite->owner = netActor->spr_owner;

    //xvel for player sprites is used for weapon bobbing, and is just set to the euclidean distance between
    //pos and bobpos, so there is no need to sync that.

    gameSprite->yvel = netActor->spr_yvel; // player index

                                           //zvel for player sprites is only used during the transition between air and water, I'm pretty sure that we don't think we need to sync this.

    gameSprite->lotag = netActor->spr_lotag;
    gameSprite->hitag = netActor->spr_hitag;

    gameSprite->extra = netActor->spr_extra;
}

// Similar to CopyAllActorData, but this one ignores some fields for the player sprite
static void Net_CopyPlayerActorDataFromNet(const netactor_t* netActor, spritetype* gameSprite, actor_t* gameActor, spriteext_t* gameSprExt, spritesmooth_t* gameSprSmooth)
{
    Net_CopyPlayerSpriteFromNet(netActor, gameSprite);
    Net_CopyActorFromNet(netActor, gameActor);
    Net_CopySpriteExtFromNet(netActor, gameSprExt);
    Net_CopySpriteSmoothFromNet(netActor, gameSprSmooth);

}

static void Net_CopyActorsToGameArrays(const netmapstate_t* srv_snapshot, const netmapstate_t* cl_snapshot)
{
    int32_t actorIndex = 0;
    int32_t actorCount = 0;

    // we need to clear out any sprites the client inserted between applying snapshots,
    // so that there aren't any sprite index conflicts.
    Net_DeleteAllScratchPadSprites();

    if ((srv_snapshot->maxActorIndex) < 0 || (srv_snapshot->maxActorIndex > MAXSPRITES))
    {
        Net_Error_Disconnect("Net_CopyActorsToGameArrays: Invalid number of actors in snapshot.");
    }

    for (actorIndex = 0; actorIndex < MAXSPRITES; actorIndex++)
    {
        const netactor_t*       srvActor = &(srv_snapshot->actor[actorIndex]);
        const netactor_t*       clActor  = &(cl_snapshot->actor[actorIndex]);

        int status = memcmp(srvActor, clActor, sizeof(netactor_t));

        if(status == 0)
        {
            if(g_enableClientInterpolationCheck)
            {
                continue;
            }
        }

        const netactor_t*       snapshotActor = srvActor;

        spritetype*       gameSprite = &(sprite[actorIndex]);
        actor_t*          gameActor = &(actor[actorIndex]);
        spriteext_t*      gameExt = &(spriteext[actorIndex]);
        spritesmooth_t*   gameSm = &(spritesmooth[actorIndex]);

        NET_DEBUG_VAR int32_t           DEBUG_GameSprOldStat = gameSprite->statnum;
        NET_DEBUG_VAR int32_t           DEBUG_NetSprOldStat = snapshotActor->spr_statnum;

        // NOTE: STAT_NETALLOC sprites ARE part if numsprites!We need to count STAT_NETALLOC sprites.
        bool              snapSpriteIsDeleted = (snapshotActor->spr_statnum == cLocSprite_DeletedSpriteStat);

        NET_75_CHECK++; // Need to make sure this will not effect swimming legs, holodukes, or start points negatively

        bool              isAnyOtherPlayerSprite = (snapshotActor->spr_picnum == APLAYER) && (snapshotActor->spr_yvel > 0);

        bool              isPlayer0Sprite = (actorIndex == g_player[0].ps->i);


        // it's better to let P_ProcessInput update some fields of the player's sprite.


        if (isPlayer0Sprite || isAnyOtherPlayerSprite)
        {
            NET_75_CHECK++; // Net_CopyPlayerActorDataFromNet() may be a good place to handle checking the player's new position
                            // this will also need updating when we support a dynamic number of player sprites...
            Net_CopyPlayerActorDataFromNet(snapshotActor, gameSprite, gameActor, gameExt, gameSm);

            continue;
        }

        Net_CopyAllActorDataFromNet(snapshotActor, gameSprite, gameActor, gameExt, gameSm);

        Net_UpdateSpriteLinkedLists(actorIndex, snapshotActor);


        if (!snapSpriteIsDeleted)
        {
            actorCount++;
        }

    }

    if (actorCount > MAXSPRITES)
    {
        Net_Error_Disconnect("Net_CopyActorsToGameArrays: Too many actors in snapshot.");
    }

    Numsprites = actorCount;
}


//-------------------------------------------------------------------------------------
// Game -> Net
//-------------------------------------------------------------------------------------

static void Net_CopyWallToNet(const walltype* gameWall, netWall_t* netWall, int16_t netIndex)
{
    Bassert(gameWall);
    Bassert(netWall);

    // (convert data to 32 bit integers)

    netWall->x = gameWall->x;
    netWall->y = gameWall->y;
    netWall->point2 = gameWall->point2;
    netWall->nextwall = gameWall->nextwall;
    netWall->nextsector = gameWall->nextsector;

    netWall->cstat = gameWall->cstat;
    netWall->picnum = gameWall->picnum;
    netWall->overpicnum = gameWall->overpicnum;
    netWall->shade = gameWall->shade;
    netWall->pal = gameWall->pal;

    netWall->xrepeat = gameWall->xrepeat;
    netWall->yrepeat = gameWall->yrepeat;
    netWall->xpanning = gameWall->xpanning;
    netWall->ypanning = gameWall->ypanning;
    netWall->lotag = gameWall->lotag;

    netWall->hitag = gameWall->hitag;
    netWall->extra = gameWall->extra;

    netWall->netIndex = netIndex;
}

static void Net_CopySectorToNet(const sectortype * gameSector, netSector_t* netSector, int16_t netIndex)
{
    Bassert(gameSector);
    Bassert(netSector);

    // (convert data to 32 bit integers)

    netSector->wallptr = gameSector->wallptr;
    netSector->wallnum = gameSector->wallnum;
    netSector->ceilingz = gameSector->ceilingz;
    netSector->floorz = gameSector->floorz;
    netSector->ceilingstat = gameSector->ceilingstat;

    netSector->floorstat = gameSector->floorstat;
    netSector->ceilingpicnum = gameSector->ceilingpicnum;
    netSector->ceilingheinum = gameSector->ceilingheinum;
    netSector->ceilingshade = gameSector->ceilingshade;
    netSector->ceilingpal = gameSector->ceilingpal;

    netSector->ceilingxpanning = gameSector->ceilingxpanning;
    netSector->ceilingypanning = gameSector->ceilingypanning;
    netSector->floorpicnum = gameSector->floorpicnum;
    netSector->floorheinum = gameSector->floorheinum;
    netSector->floorshade = gameSector->floorshade;

    netSector->floorpal = gameSector->floorpal;
    netSector->floorxpanning = gameSector->floorxpanning;
    netSector->floorypanning = gameSector->floorypanning;
    netSector->visibility = gameSector->visibility;
    netSector->fogpal = gameSector->fogpal;

    netSector->lotag = gameSector->lotag;
    netSector->hitag = gameSector->hitag;
    netSector->extra = gameSector->extra;

    netSector->netIndex = netIndex;

}

static void Net_CopySpriteToNet(const spritetype* gameSprite, netactor_t* netActor)
{
    Bassert(netActor);
    Bassert(gameSprite);

    netActor->spr_x = gameSprite->x;
    netActor->spr_y = gameSprite->y;
    netActor->spr_z = gameSprite->z;

    netActor->spr_cstat = gameSprite->cstat;
    netActor->spr_picnum = gameSprite->picnum;
    netActor->spr_shade = gameSprite->shade;
    netActor->spr_pal = gameSprite->pal;
    netActor->spr_clipdist = gameSprite->clipdist;
    netActor->spr_blend = gameSprite->blend;

    netActor->spr_xrepeat = gameSprite->xrepeat;
    netActor->spr_yrepeat = gameSprite->yrepeat;

    netActor->spr_xoffset = gameSprite->xoffset;
    netActor->spr_yoffset = gameSprite->yoffset;

    netActor->spr_sectnum = gameSprite->sectnum;
    netActor->spr_statnum = gameSprite->statnum;

    netActor->spr_ang = gameSprite->ang;
    netActor->spr_owner = gameSprite->owner;
    netActor->spr_xvel = gameSprite->xvel;
    netActor->spr_yvel = gameSprite->yvel;
    netActor->spr_zvel = gameSprite->zvel;

    netActor->spr_lotag = gameSprite->lotag;
    netActor->spr_hitag = gameSprite->hitag;

    netActor->spr_extra = gameSprite->extra;

}

static void Net_CopyActorToNet(const actor_t* gameActor, netactor_t *netActor)
{
    // (convert data from 32 bit integers)

    Bassert(gameActor);
    Bassert(netActor);

    netActor->t_data_0 = gameActor->t_data[0];
    netActor->t_data_1 = gameActor->t_data[1];
    netActor->t_data_2 = gameActor->t_data[2];
    netActor->t_data_3 = gameActor->t_data[3];
    netActor->t_data_4 = gameActor->t_data[4];
    netActor->t_data_5 = gameActor->t_data[5];
    netActor->t_data_6 = gameActor->t_data[6];
    netActor->t_data_7 = gameActor->t_data[7];
    netActor->t_data_8 = gameActor->t_data[8];
    netActor->t_data_9 = gameActor->t_data[9];

#ifdef LUNATIC

    netActor->hvel = gameActor->mv.hvel;
    netActor->vvel = gameActor->mv.vvel;

    netActor->startframe = gameActor->ac.startframe;
    netActor->numframes = gameActor->ac.numframes;
    netActor->viewtype = gameActor->ac.viewtype;
    netActor->incval = gameActor->ac.incval;
    netActor->delay = gameActor->ac.delay;

    netActor->actiontics = gameActor->actiontics;

#endif

    netActor->flags = gameActor->flags;

    netActor->bpos_x = gameActor->bpos.x;
    netActor->bpos_y = gameActor->bpos.y;
    netActor->bpos_z = gameActor->bpos.z;

    netActor->floorz = gameActor->floorz;
    netActor->ceilingz = gameActor->ceilingz;
    netActor->lastvx = gameActor->lastv.x;
    netActor->lastvy = gameActor->lastv.y;

    netActor->lasttransport = gameActor->lasttransport;

    //WARNING: both sprite and actor have these fields
    netActor->picnum = gameActor->picnum;
    netActor->ang = gameActor->ang;
    netActor->extra = gameActor->extra;
    netActor->owner = gameActor->owner;

    netActor->movflag = gameActor->movflag;
    netActor->tempang = gameActor->tempang;
    netActor->timetosleep = gameActor->timetosleep;

    netActor->stayput = gameActor->stayput;
    netActor->dispicnum = gameActor->dispicnum;

#if defined LUNATIC
    //WARNING: NOT the same as movflag
    netActor->movflags = gameActor->movflags;

#endif

    netActor->cgg = gameActor->cgg;
}

static void Net_CopySpriteExtToNet(const spriteext_t* gameSpriteExt, netactor_t* netActor)
{
    Bassert(gameSpriteExt);
    Bassert(netActor);

    netActor->ext_mdanimtims = gameSpriteExt->mdanimtims;

    netActor->ext_mdanimcur = gameSpriteExt->mdanimcur;
    netActor->ext_angoff = gameSpriteExt->angoff;
    netActor->ext_pitch = gameSpriteExt->pitch;
    netActor->ext_roll = gameSpriteExt->roll;

    netActor->ext_offset_x = gameSpriteExt->offset.x;
    netActor->ext_offset_y = gameSpriteExt->offset.y;
    netActor->ext_offset_z = gameSpriteExt->offset.z;

    netActor->ext_flags = gameSpriteExt->flags;
    netActor->ext_xpanning = gameSpriteExt->xpanning;
    netActor->ext_ypanning = gameSpriteExt->ypanning;

    netActor->ext_alpha = gameSpriteExt->alpha;
}

static void Net_CopySpriteSmoothToNet(const spritesmooth_t* gameSprSmooth, netactor_t* netActor)
{
    Bassert(gameSprSmooth);
    Bassert(netActor);

    netActor->sm_smoothduration = gameSprSmooth->smoothduration;
    netActor->sm_mdcurframe = gameSprSmooth->mdcurframe;
    netActor->sm_mdoldframe = gameSprSmooth->mdoldframe;
    netActor->sm_mdsmooth = gameSprSmooth->mdsmooth;

}

static void Net_CopyAllActorDataToNet(int32_t spriteIndex, const spritetype* gameSprite, const actor_t* gameActor, const spriteext_t* gameSprExt, const spritesmooth_t* gameSprSmooth, netactor_t* netActor)
{
    Net_CopySpriteToNet(gameSprite, netActor);
    Net_CopyActorToNet(gameActor, netActor);
    Net_CopySpriteExtToNet(gameSprExt, netActor);
    Net_CopySpriteSmoothToNet(gameSprSmooth, netActor);

    netActor->netIndex = spriteIndex;

}

static void Net_AddActorsToSnapshot(netmapstate_t* snapshot)
{

    int32_t gameIndex = 0;

    NET_75_CHECK++; // we may want to only send over sprites that are visible, this might be a good optimization
                    // to do later.

    NET_75_CHECK++; // Verify: Does the netcode need to worry about spriteext and spritesmooth beyond index (MAXSPRITES - 1)?

    NET_75_CHECK++; // may be able to significantly improve performance by using headspritestat[] etc. lists
                    // i.e., replace with for(all stat) { for(all sprites in stat) { } }, ignoring net Non Relevant Stats,
                    // also then ioSnapshot->maxActorIndex could be something much less than MAXSPRITES

    snapshot->maxActorIndex = 0;

    // note that Numsprites should NOT be the upper bound, if sprites are deleted in the middle
    // the max index to check will be > than Numsprites.
    for (gameIndex = 0; gameIndex < (MAXSPRITES); gameIndex++)
    {
        const spritetype*       gameSpr = &sprite[gameIndex];

        const actor_t*          gameAct = &actor[gameIndex];
        const spriteext_t*      gameExt = &spriteext[gameIndex];
        const spritesmooth_t*   gameSmooth = &spritesmooth[gameIndex];

        netactor_t*       netSprite = &snapshot->actor[gameIndex];


        Net_CopyAllActorDataToNet(gameIndex, gameSpr, gameAct, gameExt, gameSmooth, netSprite);

    }

    snapshot->maxActorIndex = MAXSPRITES;

}


static void Net_AddWorldToSnapshot(netmapstate_t* snapshot)
{
    int32_t index = 0;

    for (index = 0; index < numwalls; index++)
    {
        // on the off chance that numwalls somehow gets set to higher than MAXWALLS... somehow...
        Bassert(index < MAXWALLS);
        const	walltype*   gameWall = &wall[index];
        netWall_t*  snapshotWall = &snapshot->wall[index];

        Net_CopyWallToNet(gameWall, snapshotWall, index);


    }

    for (index = 0; index < numsectors; index++)
    {
        Bassert(index < MAXSECTORS);
        const	sectortype*  gameSector = &sector[index];
        netSector_t* snapshotSector = &snapshot->sector[index];

        Net_CopySectorToNet(gameSector, snapshotSector, index);

    }

    Net_AddActorsToSnapshot(snapshot);
}




//------------------------------------------------------------------------------------------------------------------------


static void P_RemovePlayer(int32_t p)
{
    // server obviously can't leave the game, and index 0 shows up for disconnect events from
    // players that haven't gotten far enough into the connection process to get a player ID

    if (p <= 0) return;

    g_player[p].playerquitflag = 0;

    Bsprintf(recbuf,"%s^00 is history!",g_player[p].user_name);
    G_AddUserQuote(recbuf);

    if (numplayers == 1)
        S_PlaySound(GENERIC_AMBIENCE17);

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        if (screenpeek == p)
            screenpeek = myconnectindex;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        P_QuickKill(g_player[p].ps);

        if (voting == p)
        {
            for (p=0; p<MAXPLAYERS; p++)
            {
                g_player[p].vote = 0;
                g_player[p].gotvote = 0;
            }
            voting = -1;
        }

        Bstrcpy(apStrings[QUOTE_RESERVED2],recbuf);
        g_player[myconnectindex].ps->ftq = QUOTE_RESERVED2;
        g_player[myconnectindex].ps->fta = 180;
    }
}

static void Net_SendNewPlayer(int32_t newplayerindex)
{
    packbuf[0] = PACKET_NUM_PLAYERS;
    packbuf[1] = numplayers;
    packbuf[2] = g_mostConcurrentPlayers;
    packbuf[3] = ud.multimode;
    packbuf[4] = newplayerindex;
    packbuf[5] = g_networkMode;
    packbuf[6] = myconnectindex;
    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 7, ENET_PACKET_FLAG_RELIABLE));

    Dbg_PacketSent(PACKET_NUM_PLAYERS);
}

static void Net_SendPlayerIndex(int32_t index, ENetPeer *peer)
{
    packbuf[0] = PACKET_PLAYER_INDEX;
    packbuf[1] = index;
    packbuf[2] = myconnectindex;
    enet_peer_send(peer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 3, ENET_PACKET_FLAG_RELIABLE));

    Dbg_PacketSent(PACKET_PLAYER_INDEX);
}

// sync a connecting player up with the current game state
static void Net_SyncPlayer(ENetEvent *event)
{
    int32_t newPlayerIndex, j;

    if (numplayers + g_netPlayersWaiting >= MAXPLAYERS)
    {
        enet_peer_disconnect_later(event->peer, DISC_SERVER_FULL);
        initprintf("Refused peer; server full.\n");
        return;
    }

    g_netPlayersWaiting++;

    S_PlaySound(DUKE_GETWEAPON2);

    // open a new slot if necessary and save off the resulting slot # for future reference
    for (TRAVERSE_CONNECT(newPlayerIndex))
    {
        if (g_player[newPlayerIndex].playerquitflag == 0)
        {
            break;
        }
    }

    if (newPlayerIndex == -1)
    {
        newPlayerIndex = g_mostConcurrentPlayers++;
    }

    NET_75_CHECK++; // is it necessary to se event->peer->data to the new player index in Net_SyncPlayer?
    event->peer->data = (void *)(intptr_t)newPlayerIndex;

    g_player[newPlayerIndex].netsynctime = (int32_t) totalclock;
    g_player[newPlayerIndex].playerquitflag = 1;

    NET_75_CHECK++; // Need to think of something better when finding a remaining slot for players.

    for (j = 0; j < g_mostConcurrentPlayers - 1; j++)
    {
        connectpoint2[j] = j + 1;
    }

    connectpoint2[g_mostConcurrentPlayers - 1] = -1;

    G_MaybeAllocPlayer(newPlayerIndex);

    g_netPlayersWaiting--;
    ++numplayers;
    ++ud.multimode;
    Net_SendNewPlayer(newPlayerIndex);
    Net_SendPlayerIndex(newPlayerIndex, event->peer);
    Net_SendClientInfo();
    Net_SendUserMapName();
    Net_SendNewGame(0, event->peer); // newly connecting player (Net_SyncPlayer)
}



static void display_betascreen(void)
{
    rotatesprite_fs(160 << 16, 100 << 16, 65536, 0, BETASCREEN, 0, 0, 2 + 8 + 64 + BGSTRETCH);

    rotatesprite_fs(160 << 16, (104) << 16, 60 << 10, 0, DUKENUKEM, 0, 0, 2 + 8);
    rotatesprite_fs(160 << 16, (129) << 16, 30 << 11, 0, THREEDEE, 0, 0, 2 + 8);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs(160 << 16, (151) << 16, 30 << 11, 0, PLUTOPAKSPRITE + 1, 0, 0, 2 + 8);
}


static void Net_Disconnect(void)
{
    if (g_netClient)
    {
        ENetEvent event;

        if (g_netClientPeer)
            enet_peer_disconnect_later(g_netClientPeer, 0);

        while (enet_host_service(g_netClient, &event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet)
                    enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                numplayers = g_mostConcurrentPlayers = ud.multimode = 1;
                myconnectindex = screenpeek = 0;
                G_BackToMenu();
                break;
            }
        }

        enet_peer_reset(g_netClientPeer);
        g_netClientPeer = NULL;
        enet_host_destroy(g_netClient);
        g_netClient = NULL;
        return;
    }

    if (g_netServer)
    {
        int32_t peerIndex;
        ENetEvent event;

        for (peerIndex = 0; peerIndex < (signed)g_netServer->peerCount; peerIndex++)
        {
            enet_peer_disconnect_later(&g_netServer->peers[peerIndex], DISC_SERVER_QUIT);
        }
        while (enet_host_service(g_netServer, &event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_RECEIVE:
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.packet)
                {
                    enet_packet_destroy(event.packet);
                }
                break;
            }
        }
        enet_host_destroy(g_netServer);
        g_netServer = NULL;
    }
}

// Only clients run this
static void Net_ReceiveDisconnect(ENetEvent *event)
{
    g_netDisconnect = 1;
    numplayers = g_mostConcurrentPlayers = ud.multimode = 1;
    myconnectindex = screenpeek = 0;
    G_BackToMenu();

    switch (event->data)
    {
    case DISC_BAD_PASSWORD:
        initprintf("Bad password.\n");
        return;
    case DISC_VERSION_MISMATCH:
        initprintf("Version mismatch.\n");
        return;
    case DISC_INVALID:
        initprintf("Invalid data detected.\n");
        return;
    case DISC_SERVER_QUIT:
        initprintf("The server is quitting.\n");
        return;
    case DISC_SERVER_FULL:
        initprintf("The server is full.\n");
        return;
    case DISC_KICKED:
        initprintf("You have been kicked from the server.\n");
        return;
    case DISC_BANNED:
        initprintf("You are banned from this server.\n");
        return;
    default:
        initprintf("Disconnected.\n");
        return;
    }
}

static void Net_SendAcknowledge(ENetPeer *client)
{
    if (!g_netServer)
        return;

    tempnetbuf[0] = PACKET_ACK;
    tempnetbuf[1] = myconnectindex;

    enet_peer_send(client, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));

    Dbg_PacketSent(PACKET_ACK);
}

static void Net_ExtractPlayerUpdate(playerupdate_t *update, int32_t type)
{
    const int32_t playerindex = update->playerindex;

    if (playerindex != myconnectindex)
    {
        g_player[playerindex].ps->pos            = update->pos;
        g_player[playerindex].ps->opos          = update->opos;
        g_player[playerindex].ps->vel           = update->vel;
        g_player[playerindex].ps->q16ang        = update->q16ang;
        g_player[playerindex].ps->q16horiz      = update->q16horiz;
        g_player[playerindex].ps->q16horizoff   = update->q16horizoff;
    }

    if (type == PACKET_MASTER_TO_SLAVE)
    {
        g_player[playerindex].ping           = update->ping;
        g_player[playerindex].ps->dead_flag  = update->deadflag;
        g_player[playerindex].playerquitflag = update->playerquitflag;
    }

}

// Server only
static void Net_ReceiveClientUpdate(ENetEvent *event)
{
    int32_t        playeridx;
    clientupdate_t update;

    if (event->packet->dataLength != sizeof(clientupdate_t))
    {
        return;
    }

    Bmemcpy(&update, (char *)event->packet->data, sizeof(clientupdate_t));

    playeridx = (int32_t)(intptr_t)event->peer->data;

    if (playeridx < 0 || playeridx >= MAXPLAYERS)
    {
        return;
    }

    g_player[playeridx].revision = update.RevisionNumber;
    inputfifo[0][playeridx]      = update.nsyn;

    Net_ExtractPlayerUpdate(&update.player, PACKET_SLAVE_TO_MASTER);
}

static void Net_Server_SetupPlayer(int playerindex)
{
    int16_t playerspriteindex;

    playerspriteindex = g_player[playerindex].ps->i;
    Bmemcpy(g_player[playerindex].ps, g_player[0].ps, sizeof(DukePlayer_t));

    g_player[playerindex].ps->i = playerspriteindex;
    changespritestat(playerspriteindex, STAT_PLAYER);

    g_player[playerindex].ps->last_extra = sprite[g_player[playerindex].ps->i].extra = g_player[playerindex].ps->max_player_health;
    sprite[g_player[playerindex].ps->i].cstat                                        = 1 + 256;
    actor[g_player[playerindex].ps->i].t_data[2]                                     = actor[g_player[playerindex].ps->i].t_data[3]
    = actor[g_player[playerindex].ps->i].t_data[4]                                   = 0;

    P_ResetPlayer(playerindex);
    Net_SpawnPlayer(playerindex);
}

static void Net_ReceiveChallenge(uint8_t *pbuf, int32_t packbufleng, ENetEvent *event)
{
    const uint16_t byteVersion = B_UNBUF16(&pbuf[1]);
    const uint16_t netVersion  = B_UNBUF16(&pbuf[3]);
    const uint32_t crc         = B_UNBUF32(&pbuf[5]);

    UNREFERENCED_PARAMETER(packbufleng);  // remove when this variable is used

    if (byteVersion != BYTEVERSION || netVersion != NETVERSION)
    {
        enet_peer_disconnect_later(event->peer, DISC_VERSION_MISMATCH);
        initprintf("Bad client protocol: version %u.%u\n", byteVersion, netVersion);
        return;
    }
    if (crc != Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0))
    {
        enet_peer_disconnect_later(event->peer, DISC_BAD_PASSWORD);
        initprintf("Bad password from client.\n");
        return;
    }

    Net_SyncPlayer(event);
}

static void Net_ReceiveMessage(uint8_t *pbuf, int32_t packbufleng)
{
    Bstrncpy(recbuf, (char *)pbuf + 2, packbufleng - 2);
    recbuf[packbufleng - 2] = 0;

    G_AddUserQuote(recbuf);
    S_PlaySound(EXITMENUSOUND);

    pus = pub = NUMPAGES;
}


static void Net_CheckForEnoughVotes()
{



    // Only the server can decide map changes
    if (!g_netServer || numplayers <= 1)
    {
        return;
    }

    int32_t requiredvotes;

    // If there are just two players, both of them deserve a vote
    if (numplayers == 2)
    {
        requiredvotes = 2;
    }
    else
    {
        // If more than two players, we need at least 50% of the players to vote
        // Which means that if there's an odd number of players, we'll need slightly more than 50% of the vote.
        requiredvotes = numplayers / 2;
        if (numplayers % 2 == 1)
        {
            requiredvotes++;
        }
    }

    int32_t numfor = 0;
    int32_t numagainst = 0;

    for (int32_t playerIndex = 0; playerIndex < MAXPLAYERS; playerIndex++)
    {
        if (g_player[playerIndex].gotvote)
        {
            if (g_player[playerIndex].vote)
            {
                numfor++;
            }
            else
            {
                numagainst++;
            }
        }
    }

    if (numfor >= requiredvotes)
    {
        Net_StartNewGame();
        Net_SendNewGame(1, NULL);  // map vote
    }
    else if (numagainst >= requiredvotes || (numfor + numagainst) == numplayers)
    {
        Net_SendMapVoteCancel(1);
    }
}


static void Net_ReceiveMapVote(uint8_t *pbuf)
{
    if (voting == myconnectindex && g_player[(uint8_t)pbuf[1]].gotvote == 0)
    {
        Bsprintf(tempbuf, "Confirmed vote from %s", g_player[(uint8_t)pbuf[1]].user_name);
        G_AddUserQuote(tempbuf);
    }

    if (!g_netServer)
    {
        return;
    }

    g_player[(uint8_t)pbuf[1]].gotvote = 1;
    g_player[(uint8_t)pbuf[1]].vote    = pbuf[2];
    Net_CheckForEnoughVotes();
}


static void Net_ReceiveMapVoteCancel(uint8_t *pbuf)
{
    // Ignore if we're not voting
    if (voting == -1)
    {
        return;
    }

    // Ignore cancellations from clients that did not initiate the map vote
    if (voting != pbuf[1] && voting != myconnectindex)
    {
        return;
    }

    if (voting == myconnectindex || voting != pbuf[1])
    {
        Bsprintf(tempbuf, "Vote Failed");
    }
    else if (voting == pbuf[1])
    {
        Bsprintf(tempbuf, "%s^00 has canceled the vote", g_player[voting].user_name);
    }

    G_AddUserQuote(tempbuf);

    if (g_netServer)
    {
        Net_SendMapVoteCancel(0);
    }

    voting = -1;
}

static void Net_ReceiveClientInfo(uint8_t *pbuf, int32_t packbufleng, int32_t fromserver)
{
    uint32_t byteIndex, j;
    int32_t  other = pbuf[packbufleng];

    for (byteIndex = 1; pbuf[byteIndex]; byteIndex++)
    {
        g_player[other].user_name[byteIndex - 1] = pbuf[byteIndex];
    }

    g_player[other].user_name[byteIndex - 1] = 0;
    byteIndex++;

    g_player[other].ps->aim_mode     = pbuf[byteIndex++];
    g_player[other].ps->auto_aim     = pbuf[byteIndex++];
    g_player[other].ps->weaponswitch = pbuf[byteIndex++];
    g_player[other].ps->palookup = g_player[other].pcolor = pbuf[byteIndex++];
    g_player[other].pteam                                 = pbuf[byteIndex++];

    for (j = byteIndex; byteIndex - j < 10; byteIndex++)
    {
        g_player[other].wchoice[byteIndex - j] = pbuf[byteIndex];
    }

    if (fromserver)
    {
        g_player[other].playerquitflag = 1;
    }
}

static void Net_ReceiveUserMapName(uint8_t *pbuf, int32_t packbufleng)
{
    NET_75_CHECK++;  // possible buffer bug here with a malicious client? if the file name didn't end in '\0', a hacker could "forget" to put '\0'
                     // at the end.

    Bstrcpy(boardfilename, (char *)pbuf + 1);
    boardfilename[packbufleng - 1] = 0;
    Bcorrectfilename(boardfilename, 0);
    if (boardfilename[0] != 0)
    {
        buildvfs_kfd i;
        if ((i = kopen4loadfrommod(boardfilename, 0)) == buildvfs_kfd_invalid)
        {
            Bmemset(boardfilename, 0, sizeof(boardfilename));
            Net_SendUserMapName();
        }
        else
        {
            kclose(i);
        }
    }

    if (ud.m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
        ud.m_level_number = 0;
}

static void Net_ExtractNewGame(newgame_t *newgame, int32_t menuonly)
{
    ud.m_level_number      = newgame->level_number;
    ud.m_volume_number     = newgame->volume_number;
    ud.m_player_skill      = newgame->player_skill;
    ud.m_monsters_off      = newgame->monsters_off;
    ud.m_respawn_monsters  = newgame->respawn_monsters;
    ud.m_respawn_items     = newgame->respawn_items;
    ud.m_respawn_inventory = newgame->respawn_inventory;
    ud.m_ffire             = newgame->ffire;
    ud.m_noexits           = newgame->noexits;
    ud.m_coop              = newgame->coop;

    if (!menuonly)
    {
        ud.level_number      = newgame->level_number;
        ud.volume_number     = newgame->volume_number;
        ud.player_skill      = newgame->player_skill;
        ud.monsters_off      = newgame->monsters_off;
        ud.respawn_monsters  = newgame->respawn_monsters;
        ud.respawn_monsters  = newgame->respawn_items;
        ud.respawn_inventory = newgame->respawn_inventory;
        ud.ffire             = newgame->ffire;
        ud.noexits           = newgame->noexits;
        ud.coop              = newgame->coop;
    }
}

static void Net_ReceiveMapVoteInitiate(uint8_t *pbuf)
{
    int32_t playerIndex;

    Bmemcpy(&pendingnewgame, pbuf, sizeof(newgame_t));
    Net_ExtractNewGame(&pendingnewgame, 1);

    voting       = pendingnewgame.connection;
    vote_episode = pendingnewgame.volume_number;
    vote_map     = pendingnewgame.level_number;

    Bsprintf(tempbuf, "%s^00 has called a vote to change map to %s (E%dL%d)", g_player[voting].user_name,
             g_mapInfo[(uint8_t)(vote_episode * MAXLEVELS + vote_map)].name, vote_episode + 1, vote_map + 1);
    G_AddUserQuote(tempbuf);

    Bsprintf(tempbuf, "Press F1 to Accept, F2 to Decline");
    G_AddUserQuote(tempbuf);

    for (playerIndex = MAXPLAYERS - 1; playerIndex >= 0; playerIndex--)
    {
        g_player[playerIndex].vote    = 0;
        g_player[playerIndex].gotvote = 0;
    }

    g_player[voting].gotvote = g_player[voting].vote = 1;
}


static void Net_ParsePacketCommon(uint8_t *pbuf, int32_t packbufleng, int32_t serverpacketp)
{
    switch (pbuf[0])
    {
    case PACKET_MESSAGE:
        Net_ReceiveMessage(pbuf, packbufleng);
        break;

    case PACKET_CLIENT_INFO:
        Net_ReceiveClientInfo(pbuf, packbufleng, serverpacketp);
        break;

    case PACKET_RTS:
        G_StartRTS(pbuf[1], 0);
        break;

    case PACKET_USER_MAP:
        Net_ReceiveUserMapName(pbuf, packbufleng);
        break;

    case PACKET_MAP_VOTE:
        Net_ReceiveMapVote(pbuf);
        break;

    case PACKET_MAP_VOTE_INITIATE: // call map vote
        Net_ReceiveMapVoteInitiate(pbuf);
        break;

    case PACKET_MAP_VOTE_CANCEL: // cancel map vote
        Net_ReceiveMapVoteCancel(pbuf);
        break;
    }
}


static void Net_ParseClientPacket(ENetEvent *event)
{
    uint8_t *pbuf        = event->packet->data;
    int32_t  packbufleng = event->packet->dataLength;

    int32_t other = pbuf[--packbufleng];

    NET_DEBUG_VAR enum DukePacket_t packetType = (enum DukePacket_t)pbuf[0];

#ifdef PACKET_RECV_PRINT
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
        case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
            Net_ReceiveClientUpdate(event);
            break;

        case PACKET_PLAYER_READY:
        {
            if ((other == 0) || (other == myconnectindex) || (other >= MAXPLAYERS))
            {
                break;
            }

            // At intermission, just set the ready flag and wait for the other players.
            if (g_player[myconnectindex].ps->gm & MODE_EOL)
            {
                g_player[other].ready = 1;
            }
            else
            {
                // otherwise if the server's in a map now, set up the player immediately.
                Net_Server_SetupPlayer(other);
            }


            break;
        }

        case PACKET_PLAYER_PING:
            if (g_player[myconnectindex].ps->gm & MODE_GAME)
            {
                packbuf[0] = PACKET_PLAYER_PING;
                packbuf[1] = myconnectindex;
                enet_peer_send(event->peer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));

                Dbg_PacketSent(PACKET_PLAYER_PING);
            }
            g_player[other].pingcnt++;
            break;

        case PACKET_AUTH:
            Net_ReceiveChallenge(pbuf, packbufleng, event);
            break;

        default:
            Net_ParsePacketCommon(pbuf, packbufleng, 0);
            break;
    }
}

static void Net_HandleClientPackets(void)
{
    ENetEvent event;

    // pull events from the wire into the packet queue without dispatching them, once per Net_GetPackets() call
    enet_host_service(g_netServer, NULL, 0);

    // dispatch any pending events from the local packet queue
    while (enet_host_check_events(g_netServer, &event) > 0)
    {
        const intptr_t playeridx = (intptr_t)event.peer->data;

        if (playeridx < 0 || playeridx >= MAXPLAYERS)
        {
            enet_peer_disconnect_later(event.peer, DISC_INVALID);
            buildprint("Invalid player id (", playeridx, ") from client.\n");
            continue;
        }

        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            char ipaddr[32];

            enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));

            OSD_Printf("A new client connected from %s:%u.\n", ipaddr, event.peer->address.port);

            //[75] Temporary: For now the netcode can't handle more players connecting than there are player starts.
            if (g_playerSpawnCnt <= ud.multimode)
            {
                OSD_Printf("Connection dropped: No player spawn point available for this new player.");
                break;
            }

            Net_SendAcknowledge(event.peer);
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE:
            Net_ParseClientPacket(&event);
            // broadcast takes care of enet_packet_destroy itself
            // we set the state to disconnected so enet_host_broadcast
            // doesn't send the player back his own packets
            if ((event.channelID == CHAN_GAMESTATE && event.packet->data[0] > PACKET_BROADCAST)
                || event.channelID == CHAN_CHAT)
            {
                const ENetPacket *pak = event.packet;

                event.peer->state = ENET_PEER_STATE_DISCONNECTED;
                enet_host_broadcast(g_netServer, event.channelID,
                    enet_packet_create(pak->data, pak->dataLength, pak->flags&ENET_PACKET_FLAG_RELIABLE));
                event.peer->state = ENET_PEER_STATE_CONNECTED;
            }

            enet_packet_destroy(event.packet);
            g_player[playeridx].ping = (event.peer->lastRoundTripTime + event.peer->roundTripTime) / 2;
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            numplayers--;
            ud.multimode--;

            P_RemovePlayer(playeridx);

            g_player[playeridx].revision = cInitialMapStateRevisionNumber;

            packbuf[0] = PACKET_PLAYER_DISCONNECTED;
            packbuf[1] = playeridx;
            packbuf[2] = numplayers;
            packbuf[3] = ud.multimode;
            packbuf[4] = g_mostConcurrentPlayers;
            packbuf[5] = myconnectindex;

            enet_host_broadcast(g_netServer, CHAN_GAMESTATE,
            enet_packet_create(&packbuf[0], 6, ENET_PACKET_FLAG_RELIABLE));

            initprintf("%s disconnected.\n", g_player[playeridx].user_name);
            event.peer->data = NULL;

            Dbg_PacketSent(PACKET_PLAYER_DISCONNECTED);

            break;

        default:
            break;
        }
    }
}


static void Net_ReceiveServerUpdate(ENetEvent *event)
{
    serverupdate_t       serverupdate;
    serverplayerupdate_t playerupdate;

    if (((event->packet->dataLength - sizeof(serverupdate_t)) % sizeof(serverplayerupdate_t)) != 0)
    {
        return;
    }

    uint8_t *updatebuf = (uint8_t *)event->packet->data;
    Bmemcpy(&serverupdate, updatebuf, sizeof(serverupdate_t));
    updatebuf += sizeof(serverupdate_t);
    inputfifo[0][0] = serverupdate.nsyn;
    ud.pause_on     = serverupdate.pause_on;

    ticrandomseed = serverupdate.seed;

    for (int playerIndex = 0; playerIndex < serverupdate.numplayers; ++playerIndex)
    {
        Bmemcpy(&playerupdate, updatebuf, sizeof(serverplayerupdate_t));
        updatebuf += sizeof(serverplayerupdate_t);

        Net_ExtractPlayerUpdate(&playerupdate.player, PACKET_MASTER_TO_SLAVE);

        g_player[playerIndex].ps->gotweapon = playerupdate.gotweapon;

        g_player[playerIndex].ps->kickback_pic = playerupdate.kickback_pic;

        Bmemcpy(g_player[playerIndex].frags, playerupdate.frags, sizeof(playerupdate.frags));
        Bmemcpy(g_player[playerIndex].ps->inv_amount, playerupdate.inv_amount, sizeof(playerupdate.inv_amount));
        Bmemcpy(g_player[playerIndex].ps->ammo_amount, playerupdate.ammo_amount, sizeof(playerupdate.ammo_amount));

        g_player[playerIndex].ps->curr_weapon    = playerupdate.curr_weapon;
        g_player[playerIndex].ps->last_weapon    = playerupdate.last_weapon;
        g_player[playerIndex].ps->wantweaponfire = playerupdate.wantweaponfire;
        g_player[playerIndex].ps->weapon_pos     = playerupdate.weapon_pos;
        g_player[playerIndex].ps->frag_ps        = playerupdate.frag_ps;
        g_player[playerIndex].ps->frag           = playerupdate.frag;
        g_player[playerIndex].ps->fraggedself    = playerupdate.fraggedself;
        g_player[playerIndex].ps->last_extra     = playerupdate.last_extra;
        g_player[playerIndex].ping               = playerupdate.ping;


        g_player[playerIndex].ps->newowner = playerupdate.newowner;
    }
}

// sends the version and a simple crc32 of the current password, all verified by the server before the connection can continue
static void Net_SendChallenge()
{
    if (!g_netClientPeer)
    {
        return;
    }

    tempnetbuf[0] = PACKET_AUTH;
    B_BUF16(&tempnetbuf[1], BYTEVERSION);
    B_BUF16(&tempnetbuf[3], NETVERSION);
    B_BUF32(&tempnetbuf[5], Bcrc32((uint8_t *)g_netPassword, Bstrlen(g_netPassword), 0));
    tempnetbuf[9] = myconnectindex;

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], 10, ENET_PACKET_FLAG_RELIABLE));

    Dbg_PacketSent(PACKET_AUTH);
}


static void Net_ReceiveAcknowledge(uint8_t *pbuf, int32_t packbufleng)
{
    UNREFERENCED_PARAMETER(pbuf);         // remove when this variable is used
    UNREFERENCED_PARAMETER(packbufleng);  // remove when this variable is used

    Net_SendChallenge();
}


// client only
static void Net_ReceiveNewGame(ENetEvent *event)
{
    ClientPlayerReady = 0;

    if ((vote_map + vote_episode + voting) != -3)
        G_AddUserQuote("Vote Succeeded");

    Bmemcpy(&pendingnewgame, event->packet->data, sizeof(newgame_t));
    Net_StartNewGame();

    packbuf[0] = PACKET_PLAYER_READY;
    packbuf[1] = myconnectindex;

    if (g_netClientPeer)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
        Dbg_PacketSent(PACKET_PLAYER_READY);

        if (g_netClient)
        {
            NET_75_CHECK++;  // are these extra calls to create initial snapshots necessary or is it sufficient to just have it in
                             // G_EnterLevel?
            /*

            Net_InitMapStateHistory();
            Net_AddWorldToInitialSnapshot();

            */
            ClientPlayerReady = 1;
        }
    }

    //[75] Note:    DON'T set the initial map state or initialize the map state history in the packet code,
    //              The client didn't load the map until G_EnterLevel

    g_player[myconnectindex].ps->gm = MODE_GAME;
    ready2send                      = 1;
}


static void Net_ReceiveNewPlayer(uint8_t *pbuf, int32_t packbufleng)
{
    int32_t i;

    UNREFERENCED_PARAMETER(packbufleng);  // remove when this variable is used

    numplayers              = pbuf[1];
    g_mostConcurrentPlayers = pbuf[2];
    ud.multimode            = pbuf[3];
    if (pbuf[4])  // ID of new player
    {
        g_player[pbuf[4]].playerquitflag = 1;

        if (!g_player[pbuf[4]].ps)
        {
            g_player[pbuf[4]].ps = (DukePlayer_t *)Xcalloc(1, sizeof(DukePlayer_t));
        }
        if (!g_player[pbuf[4]].input)
        {
            g_player[pbuf[4]].input = (input_t *)Xcalloc(1, sizeof(input_t));
        }
    }

    if (pbuf[5] == NET_DEDICATED_SERVER)
    {
        g_networkMode = NET_DEDICATED_CLIENT;
    }

    for (i = 0; i < g_mostConcurrentPlayers - 1; i++)
    {
        connectpoint2[i] = i + 1;
    }

    connectpoint2[g_mostConcurrentPlayers - 1] = -1;

    S_PlaySound(DUKE_GETWEAPON2);

    // myconnectindex is 0 until we get PACKET_PLAYER_INDEX
    if (myconnectindex != 0)
    {
        Net_SendClientInfo();
    }
}

static void Net_ReceivePlayerIndex(uint8_t *pbuf, int32_t packbufleng)
{
    UNREFERENCED_PARAMETER(packbufleng);  // remove when this variable is used

    myconnectindex                          = pbuf[1];
    g_player[myconnectindex].playerquitflag = 1;
    Net_SendClientInfo();
}


static void Net_ParseServerPacket(ENetEvent *event)
{
    uint8_t *pbuf = event->packet->data;
    int32_t packbufleng = event->packet->dataLength;
    // input_t *nsyn;

    NET_DEBUG_VAR enum DukePacket_t packetType = (enum DukePacket_t) pbuf[0];

    --packbufleng;

#ifdef PACKET_RECV_PRINT
    initprintf("Received Packet: type: %d : len %d\n", pbuf[0], packbufleng);
#endif
    switch (pbuf[0])
    {
    case PACKET_MASTER_TO_SLAVE:

        if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            return;
        }

        Net_ReceiveServerUpdate(event);
        break;

    case PACKET_NEW_GAME:
        Net_ReceiveNewGame(event);
        break;

    case PACKET_ACK:
        Net_ReceiveAcknowledge(pbuf, packbufleng);
        break;

    case PACKET_NUM_PLAYERS:
        Net_ReceiveNewPlayer(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_INDEX:
        Net_ReceivePlayerIndex(event->packet->data, event->packet->dataLength);
        break;

    case PACKET_PLAYER_DISCONNECTED:
        if ((g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            P_RemovePlayer(pbuf[1]);
        }

        numplayers = pbuf[2];
        ud.multimode = pbuf[3];
        g_mostConcurrentPlayers = pbuf[4];

        break;

    case PACKET_PLAYER_SPAWN:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            break;
        }

        P_ResetPlayer(pbuf[1]);
        Bmemcpy(&g_player[pbuf[1]].ps->pos.x, &pbuf[2], sizeof(vec3_t) * 2);
        Bmemcpy(&sprite[g_player[pbuf[1]].ps->i], &pbuf[2], sizeof(vec3_t));

        break;

    case PACKET_PLAYER_PING:
        g_player[0].pingcnt++;
        return;

    case PACKET_FRAG:
        if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        {
            break;
        }

        g_player[pbuf[1]].ps->frag_ps = pbuf[2];
        actor[g_player[pbuf[1]].ps->i].picnum = pbuf[3];
        ticrandomseed = B_UNBUF32(&pbuf[4]);
        P_FragPlayer(pbuf[1]);

        break;

        // [75]
    case PACKET_WORLD_UPDATE:
        Net_ReadWorldUpdate(pbuf, packbufleng);
        break;


    default:
        Net_ParsePacketCommon(pbuf, packbufleng, 1);
        break;
    }
}

static void Net_HandleServerPackets(void)
{
    ENetEvent event;

    enet_host_service(g_netClient, NULL, 0);

    while (enet_host_check_events(g_netClient, &event) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            Net_ReceiveDisconnect(&event);
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            Net_ParseServerPacket(&event);
        }

        enet_packet_destroy(event.packet);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Map Update Packets


//Insert a sprite from STAT_NETALLOC
static int32_t Net_DoInsertSprite(int32_t sect, int32_t stat)
{
    int32_t i = headspritestat[STAT_NETALLOC];

    // This means that we've run out of server-side actors
    if (i < 0)
    {
        Net_Error_Disconnect("Out of server side actors");
        return i;
    }

    changespritestat(i, stat);
    do_insertsprite_at_headofsect(i, sect);

    return i;
}


////////////////////////////////////////////////////////////////////////////////
// Player Updates

static void Net_FillPlayerUpdate(playerupdate_t *update, int32_t player)
{
    update->playerindex = player;

    update->pos = g_player[player].ps->pos;
    update->opos = g_player[player].ps->opos;
    update->vel = g_player[player].ps->vel;
    update->q16ang = g_player[player].ps->q16ang;
    update->q16horiz = g_player[player].ps->q16horiz;
    update->q16horizoff = g_player[player].ps->q16horizoff;
    update->ping = g_player[player].ping;
    update->deadflag = g_player[player].ps->dead_flag;
    update->playerquitflag = g_player[player].playerquitflag;
}





////////////////////////////////////////////////////////////////////////////////
// New Game Packets

// set all actors, walls, and sectors in a snapshot to their Null states.
static void Net_InitMapState(netmapstate_t* mapState)
{
    netactor_t  sprDefault = cNullNetActor;

    int32_t     index = 0;

    sprDefault.netIndex = cSTOP_PARSING_CODE;

    mapState->maxActorIndex = 0;

    // this code is just a (slow) memset as it is now,
    // it may be a good idea to use "baselines", which can reduce the amount
    // of delta encoding when a sprite is first added. This
    // could be a good optimization to consider later.
    for (index = 0; index < MAXSPRITES; index++)
    {
        mapState->actor[index] = sprDefault;
    }

    for (index = 0; index < MAXWALLS; index++)
    {
        mapState->wall[index] = cNullNetWall;
    }

    for (index = 0; index < MAXSECTORS; index++)
    {
        mapState->sector[index] = cNullNetSector;
    }

    // set the revision number to a valid but easy to identify number,
    // this is just to make it easier to debug.
    mapState->revisionNumber = 8675309;

}

// Both client and server execute this
static void Net_ResetPlayers()
{
    int32_t playerIndex;

    for (TRAVERSE_CONNECT(playerIndex))
    {
        P_ResetWeapons(playerIndex);
        P_ResetInventory(playerIndex);
        g_player[playerIndex].revision = cInitialMapStateRevisionNumber;
    }

}



static void Net_ResetPlayerReady()
{
    int32_t playerindex;

    for (TRAVERSE_CONNECT(playerindex))
    {
        g_player[playerindex].ready = 0;
    }
}




// Packet code
//---------------------------------------------------------------------------------------------------------------------------------


// enforce signed/unsigned and overflow behavior for WriteBits smaller than 32
// remember that all of the WriteX functions only take 32 bit integers as input...
#define PICKY_TYPECHECK 1

#define MAX_PACKET_STRING 1024

#define LOG_MESSAGE_BITS 1

#define NetNumberOfIndexes (1 << NETINDEX_BITS)
#define IndexesOK (MAXWALLS < NetNumberOfIndexes) && (MAXSPRITES < NetNumberOfIndexes) && (MAXSECTORS < NetNumberOfIndexes)

#if(!IndexesOK)
#error "network.cpp: game arrays are now too big to send over the network, please update NETINDEX_BITS to be the right length to store a wall, sprite, and sector index"
#endif

#define STRUCTINDEX_BITS 8 // hopefully no game structs ever have more than 255 fields....

#define DEBUG_CHANGEDINDEXES 1

#define	FLOAT_INT_BITS	13

// remember that the minimum negative number is the sign bit + all zeros
const int32_t cTruncInt_Min = -(1 << (FLOAT_INT_BITS - 1));
const int32_t cTruncInt_Max = (1 << (FLOAT_INT_BITS - 1)) - 1;

typedef struct NetBuffer_s
{
    uint8_t     *Data;
    int32_t     Bit;
    int32_t     ReadCurByte;
    int32_t     CurSize;
    int32_t     MaxSize; // set in NetBuffer_Init to the size of the byte array that this struct referrs to in Data
} NetBuffer_t;

// NOTE: does NOT fill byteArray with zeros
static void NetBuffer_Init(NetBuffer_t *buffer, uint8_t *byteArray, int32_t arrayLength)
{
    // fill all the entries of the netbuffer struct (reminder: the byte array is not part of the struct)
    Bmemset(buffer, 0, sizeof(*buffer));
    buffer->Data = byteArray;
    buffer->MaxSize = arrayLength;
}


///<summary>
///guarantees that the destination string will be nul terminated
///</summary>
static void SafeStrncpyz(char *dest, const char *src, int destsize)
{
    if (!dest || !src || (destsize <= 0))
    {

        Bassert(dest);
        Bassert(src);
        Bassert(destsize > 0);
        Net_Error_Disconnect("SafeStrncpyz: Invalid src, dest, or datasize.");
        return;
    }

    strncpy(dest, src, destsize - 1);
    dest[destsize - 1] = 0;
}

// Write a bit (bitValue) to a byte array (dataBuffer) at bit offset bitOffset,
// this function increments bitOffset for you.
static void PutBit(int32_t bitValue, uint8_t *dataBuffer, int32_t *bitOffset)
{
    int32_t bitOffsetValue = *bitOffset;

    const int32_t byteIndex = bitOffsetValue >> 3;
    const int32_t bitIndex = bitOffsetValue & 7;

    // reset this byte to 0 on the first bit
    if ((bitIndex) == 0)
    {
        dataBuffer[byteIndex] = 0;
    }

    dataBuffer[byteIndex] |= (bitValue << (bitIndex));


    bitOffsetValue++;
    *bitOffset = bitOffsetValue;
}

// Get a single bit (return value) from a byte array (dataBuffer) at bit offset bitOffset
// this function increments bitOffset for you.
static int32_t GetBit(uint8_t *dataBuffer, int32_t *bitOffset)
{
    int32_t bitValue = -1;

    int32_t bitOffsetValue = *bitOffset;

    const int32_t byteIndex = bitOffsetValue >> 3;
    const int32_t bitIndex = bitOffsetValue & 7;

    bitValue = (dataBuffer[byteIndex] >> bitIndex) & 0x1;

    bitOffsetValue++;
    *bitOffset = bitOffsetValue;

    return bitValue;
}

//Note: This function increments bitOffset for you
static void PutBits(int32_t value, uint8_t *dataBuffer, int32_t *bitOffset, int16_t numberOfBits)
{
    int16_t bitIndex;

    if (numberOfBits > 32)
    {
        Net_Error_Disconnect("PutBits: Attempted to write more than 32 bits to the buffer.");
        numberOfBits = 32;
    }

    for (bitIndex = 0; bitIndex < numberOfBits; bitIndex++)
    {
        int32_t bitValue = value & (1 << bitIndex);

        PutBit((bitValue != 0), dataBuffer, bitOffset);

    }

}

//Note: this function increments bitOffset for you
static int32_t GetBits(uint8_t *dataBuffer, int32_t *bitOffset, int16_t numberOfBits)
{
    int32_t value = 0;
    int16_t bitIndex;

    if (numberOfBits > 32)
    {
        Net_Error_Disconnect("GetBits: Attempted to read more than 32 bits from the buffer.");
        numberOfBits = 32;
    }

    for (bitIndex = 0; bitIndex < numberOfBits; bitIndex++)
    {
        int32_t tBitValue = GetBit(dataBuffer, bitOffset);

        value |= (tBitValue << bitIndex);

    }

    return value;
}


// only difference between this and GetBits is this one does not require you
// to pass in a pointer for the bit offset. Remember to not use semicolons in the watch list!
//
// use this to debug packets
//
// NOTE: Set to non-static just so the compiler won't optimize it out.
int32_t Dbg_GetBits(NetBuffer_t *netBuffer, int32_t bitOffset, int16_t numberOfBits)
{
    // I'm having GetBits modify a temp instead of the input parameter so that you can
    // still see what the input parameter was at the end of the function.
    int32_t bitOffsetValue = bitOffset;
    return GetBits(netBuffer->Data, &bitOffsetValue, numberOfBits);
}

// intended for use in the debugger
// the netcode will return (32 bit) floats in a variable with type int32, though the data is actually
// floating point data. This function is here to make it easier to verify a (full) float encoded as int32
// in the debugger.
// note: floats that are actually truncated integers will usually look something like this: 3.922e-39#DEN
EDUKE32_UNUSED static float Dbg_RawIntToFloat(NetChunk32 floatRawData)
{
    float* floatPtr = (float*)&floatRawData;

    return *(floatPtr);
}


static void NetBuffer_WriteBits(NetBuffer_t *netBuffer, int32_t data, int16_t numberOfBits)
{
    if (netBuffer->CurSize >= netBuffer->MaxSize)
    {
        Net_Error_Disconnect("NetBuffer_WriteBits: Buffer overrun.");
    }

    int32_t dataToWrite = data & (0xffffffff >> (32 - numberOfBits));

    PutBits(dataToWrite, netBuffer->Data, &netBuffer->Bit, numberOfBits);

    netBuffer->CurSize = (netBuffer->Bit >> 3) + 1;

}

EDUKE32_UNUSED static void NetBuffer_WriteUnsignedByte(NetBuffer_t *netBuffer, int32_t byte)
{
#ifdef PICKY_TYPECHECK
    if ((byte < 0) || byte > 255)
    {
        Net_Error_Disconnect("NetBuffer_WriteUnsignedByte: Value not valid.");
    }
#endif
    NetBuffer_WriteBits(netBuffer, byte, 8);
}

EDUKE32_UNUSED static void NetBuffer_WriteSignedByte(NetBuffer_t *netBuffer, int32_t byte)
{
#ifdef PICKY_TYPECHECK
    if ((byte < -128) || (byte > 127))
    {
        Net_Error_Disconnect("NetBuffer_WriteSignedByte: Value not valid.");
    }
#endif
    NetBuffer_WriteBits(netBuffer, byte, 8);
}


EDUKE32_UNUSED static void NetBuffer_WriteInt16(NetBuffer_t *netBuffer, int32_t word)
{
#ifdef PICKY_TYPECHECK
    const int16_t cLowVal = (int16_t)0x00008000;
    const int16_t cHighVal = (int16_t)0x00007fff;

    if ((word < cLowVal) || (word > cHighVal))
    {
        Net_Error_Disconnect("NetBuffer_WriteInt16: Value not valid.");
    }
#endif
    NetBuffer_WriteBits(netBuffer, word, 16);
}

EDUKE32_UNUSED static void NetBuffer_WriteUInt16(NetBuffer_t *netBuffer, int32_t word)
{
#ifdef PICKY_TYPECHECK
    const uint16_t cLowVal = (uint16_t)0x00000000;
    const uint16_t cHighVal = (uint16_t)0x0000ffff;

    if ((word < cLowVal) | (word > cHighVal))
    {
        Net_Error_Disconnect("NetBuffer_WriteUInt16: Value not valid.");
    }

#endif
    NetBuffer_WriteBits(netBuffer, word, 16);
}

static void NetBuffer_WriteDword(NetBuffer_t *netBuffer, int32_t dword)
{
    // don't care about signed/unsigned since the bit width matches
    NetBuffer_WriteBits(netBuffer, dword, 32);
}


//where length is the size in bytes
static void NetBuffer_WriteData(NetBuffer_t  *netBuffer, const void *data, int16_t length)
{
    int32_t byteIndex;
    for (byteIndex = 0; byteIndex < length; byteIndex++)
    {
        NetBuffer_WriteUnsignedByte(netBuffer, ((const int8_t *)data)[byteIndex]);
    }
}

EDUKE32_UNUSED static void NetBuffer_WriteString(NetBuffer_t *netBuffer, const char *stringToWrite)
{
    if (!stringToWrite)
    {
        // if stringToWrite is null, write a NUL ('\0') character
        NetBuffer_WriteData(netBuffer, "", 1);
    }

    else
    {
        int32_t     length; // length of input string
        char        tempStringBuffer[MAX_PACKET_STRING]; // temp storage

        length = strlen(stringToWrite);

        if (length >= MAX_PACKET_STRING)
        {
            Net_Error_Disconnect("NetBuffer_WriteString: String too long.");
        }
        SafeStrncpyz(tempStringBuffer, stringToWrite, sizeof(tempStringBuffer));

        NetBuffer_WriteData(netBuffer, tempStringBuffer, length + 1);
    }
}

static void NetBuffer_WriteDeltaFloat(NetBuffer_t *netBuffer, NetChunk32 rawFloatData)
{
    if (sizeof(float) != sizeof(int32_t))
    {
        Net_Error_Disconnect("Can't write float to buffer: sizeof(float) must be 4 bytes");
    }


    // interpret the FOUR BYTES of raw float data as a float
    float*	floatDataPtr = (float*) &(rawFloatData);
    float	floatValue = *(floatDataPtr);

    int32_t truncatedFloat = (int32_t)floatValue;

    if (floatValue == 0.0f)
    {
        NetBuffer_WriteBits(netBuffer, 0, 1);                                          // float is zeroed

    }
    else
    {
        NetBuffer_WriteBits(netBuffer, 1, 1);                                          // float is not zeroed

                                                                                       // check if this float can be sent as a FLOAT_INT_BITS sized (*signed*) integer with no data loss
        if (
            ((float)truncatedFloat == floatValue) &&
            (truncatedFloat >= cTruncInt_Min) &&
            (truncatedFloat <= cTruncInt_Max)
            )
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                                                 // send float as small integer
            NetBuffer_WriteBits(netBuffer, truncatedFloat + cTruncInt_Max, FLOAT_INT_BITS);      // float as small integer data

        }

        else
        {
            // send as full floating point value

            NetBuffer_WriteBits(netBuffer, 1, 1);                                          // send full float
            NetBuffer_WriteBits(netBuffer, rawFloatData, 32);                              // full raw float data

        }

    }

}

//-------------------------------------------------------------------------------------
// low level buffer reading functions

static int32_t NetBuffer_ReadBits(NetBuffer_t *netBuffer, int32_t numberOfBits)
{
    int32_t    value = GetBits(netBuffer->Data, &netBuffer->Bit, numberOfBits);

    netBuffer->ReadCurByte = (netBuffer->Bit >> 3) + 1;

    return value;
}


// returns -1 if no more characters are available
static int32_t NetBuffer_ReadByte(NetBuffer_t *netBuffer)
{
    int32_t    byte;

    byte = (int8_t)NetBuffer_ReadBits(netBuffer, 8);

    if (netBuffer->ReadCurByte > netBuffer->CurSize)
    {
        Net_Error_Disconnect("NetBuffer_ReadByte: Attempted to read beyond end of buffer current size.");
    }

    return byte;
}

EDUKE32_UNUSED static int32_t NetBuffer_ReadUnsignedByte(NetBuffer_t *netBuffer)
{
    int32_t    byte;

    byte = (uint8_t)NetBuffer_ReadBits(netBuffer, 8);

    if (netBuffer->ReadCurByte > netBuffer->CurSize)
    {
        Net_Error_Disconnect("NetBuffer_ReadUnsignedByte: Attempted to read beyond end of buffer current size.");
    }

    return byte;
}

EDUKE32_UNUSED static int32_t NetBuffer_ReadInt16(NetBuffer_t *netBuffer)
{
    int32_t    word;

    word = (int16_t)NetBuffer_ReadBits(netBuffer, 16);
    if (netBuffer->ReadCurByte > netBuffer->CurSize)
    {
        Net_Error_Disconnect("NetBuffer_ReadInt16: Attempted to read beyond end of buffer current size.");
    }

    return word;
}

EDUKE32_UNUSED static int32_t NetBuffer_ReadUInt16(NetBuffer_t *netBuffer)
{
    int32_t    word;

    word = (uint16_t)NetBuffer_ReadBits(netBuffer, 16);
    if (netBuffer->ReadCurByte > netBuffer->CurSize)
    {
        Net_Error_Disconnect("NetBuffer_ReadUInt16: Attempted to read beyond end of buffer current size.");
    }

    return word;
}

static int32_t NetBuffer_ReadDWord(NetBuffer_t *netBuffer)
{
    int32_t    dword;

    dword = NetBuffer_ReadBits(netBuffer, 32);
    if (netBuffer->ReadCurByte > netBuffer->CurSize)
    {
        Net_Error_Disconnect("NetBuffer_ReadDword: Attempted to read beyond end of buffer current size.");
    }

    return dword;
}

EDUKE32_UNUSED static void NetBuffer_ReadData(NetBuffer_t *netBuffer, void *data, int32_t dataLength)
{
    int32_t byteIndex;
    for (byteIndex = 0; byteIndex < dataLength; byteIndex++)
    {
        ((int8_t *)data)[byteIndex] = NetBuffer_ReadByte(netBuffer);
    }
}

// net struct -> Buffer functions
//----------------------------------------------------------------------------------------------------------

static void NetBuffer_WriteDeltaNetWall(NetBuffer_t *netBuffer, const netWall_t *from, const netWall_t *to)
{
    const   int32_t         cMaxStructs = MAXWALLS;
    const   int32_t         cFieldsInStruct = ARRAY_SIZE(WallFields);

    int32_t         fieldIndex,
                    maxChgIndex;

    netField_t      *fieldPtr;

    const int32_t   *fromField,
                    *toField;

    // all fields should be 32 bits to avoid any compiler packing issues
    // the "number" field is not part of the field list
    //
    // if this assert fails, check that all of the entries in net*_t are in the engine's type
    Bassert(cFieldsInStruct + 1 == (sizeof(*from) / 4));
    Bassert(to);
    Bassert(from);

    if (to->netIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("Netbuffer_WriteDeltaNetWall: Invalid To NetIndex");
        return;
    }

    maxChgIndex = 0;

    for (fieldIndex = 0, fieldPtr = WallFields; fieldIndex < cFieldsInStruct; fieldIndex++, fieldPtr++)
    {
        fromField   = (int32_t const *)((int8_t const *)from  + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to    + fieldPtr->offset);

        if (*fromField != *toField)
        {
            maxChgIndex = fieldIndex + 1;
        }
    }

    if (maxChgIndex == 0)  // no fields changed
    {
        return;     // write nothing at all
    }

    NetBuffer_WriteBits(netBuffer, to->netIndex, NETINDEX_BITS);

    NetBuffer_WriteBits(netBuffer, maxChgIndex, STRUCTINDEX_BITS);

    for (fieldIndex = 0, fieldPtr = WallFields; fieldIndex < maxChgIndex; fieldIndex++, fieldPtr++)
    {
        fromField   = (int32_t const *)((int8_t const *)from    + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to      + fieldPtr->offset);

        //                                                              // Bit(s) meaning
        //                                                              //-------------------
        if (*fromField == *toField)
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                       // field not changed
            continue;
        }

        NetBuffer_WriteBits(netBuffer, 1, 1);                           // field changed

        if (*toField == 0)
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                       // zero this field
        }
        else
        {
            NetBuffer_WriteBits(netBuffer, 1, 1);                       // don't zero this field
            NetBuffer_WriteBits(netBuffer, *toField, fieldPtr->bits);   // new field value
        }

    }
}

static void NetBuffer_WriteDeltaNetSector(NetBuffer_t *netBuffer, const netSector_t *from, const netSector_t *to)
{
    const   int32_t         cMaxStructs = MAXSECTORS;
    const   int32_t         cFieldsInStruct = ARRAY_SIZE(SectorFields);

    int32_t         fieldIndex,
                    maxChgIndex;

    netField_t      *fieldPtr;

    const int32_t       *fromField,
                        *toField;

    // all fields should be 32 bits to avoid any compiler packing issues
    // the "number" field is not part of the field list
    //
    // if this assert fails, check that all of the entries in net*_t are in the engine's type
    Bassert(cFieldsInStruct + 1 == (sizeof(*from) / 4));
    Bassert(to);
    Bassert(from);

    if (to->netIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("Netbuffer_WriteDeltaNetSector: Invalid To NetIndex");
        return;
    }

    maxChgIndex = 0;

    for (fieldIndex = 0, fieldPtr = SectorFields; fieldIndex < cFieldsInStruct; fieldIndex++, fieldPtr++)
    {
        fromField   = (int32_t const *)((int8_t const *)from    + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to      + fieldPtr->offset);

        if (*fromField != *toField)
        {
            maxChgIndex = fieldIndex + 1;
        }
    }

    if (maxChgIndex == 0)  // no fields changed
    {
        return;     // write nothing at all
    }

    NetBuffer_WriteBits(netBuffer, to->netIndex, NETINDEX_BITS);

    NetBuffer_WriteBits(netBuffer, maxChgIndex, STRUCTINDEX_BITS);

    for (fieldIndex = 0, fieldPtr = SectorFields; fieldIndex < maxChgIndex; fieldIndex++, fieldPtr++)
    {
        fromField   = (int32_t const *)((int8_t const *)from    + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to      + fieldPtr->offset);

        //                                                              // Bit(s) meaning
        //                                                              //-------------------
        if (*fromField == *toField)
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                       // field not changed
            continue;
        }

        NetBuffer_WriteBits(netBuffer, 1, 1);                           // field changed

        if (*toField == 0)
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                       // zero this field
        }
        else
        {
            NetBuffer_WriteBits(netBuffer, 1, 1);                       // don't zero this field
            NetBuffer_WriteBits(netBuffer, *toField, fieldPtr->bits);      // new field value
        }

    }
}

static void NetBuffer_WriteDeltaNetActor(NetBuffer_t* netBuffer, const netactor_t *from, const netactor_t* to, int8_t writeDeletedActors)
{
    const   int32_t         cMaxStructs = MAXSPRITES;
    const   int32_t         cFieldsInStruct = ARRAY_SIZE(ActorFields);

    int32_t         fieldIndex,
                    maxChgIndex;

    netField_t      *fieldPtr;

    const int32_t       *fromField,
                        *toField;

    if ((from == NULL) && (to == NULL))
    {
        // Actor was deleted in the "from" snapshot and it's still deleted in the "to" snasphot, don't write anything.
        return;
    }

    if (from == NULL)
    {
        // The actor was deleted in the "From" snapshot, but it's there in the "To" snapshot, so that means it has been inserted in the "To" snapshot.
        from = &cNullNetActor;
    }

    if (to == NULL)
    {
        // The actor was present in the "From" snapshot but it's now deleted in the "To" snapshot.
        NetBuffer_WriteBits(netBuffer, from->netIndex, NETINDEX_BITS);                      // {<NetIndex>}     sprite index
        NetBuffer_WriteBits(netBuffer, 1, 1);                                               // {<NetIndex>, 1}  sprite deleted
        return;
    }

    // all fields should be 32 bits to avoid any compiler packing issues
    // the "number" field is not part of the field list
    //
    // if this assert fails, check that all of the entries in net*_t are in the engine's type
    Bassert(cFieldsInStruct + 1 == (sizeof(*from) / 4));

    if (to->netIndex < 0 || to->netIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("Netbuffer_WriteDeltaNetActor: Invalid To NetIndex");
        return;
    }

    maxChgIndex = 0;

    for (fieldIndex = 0, fieldPtr = ActorFields; fieldIndex < cFieldsInStruct; fieldIndex++, fieldPtr++)
    {
        fromField   = (int32_t const *)((int8_t const *)from    + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to      + fieldPtr->offset);

        if (*fromField != *toField)
        {
            maxChgIndex = fieldIndex + 1;
        }
    }

    if (maxChgIndex == 0)
    {

        if (!writeDeletedActors)
        {
            // no fields changed, if we're not forcing a write for each actor then we're done
            return;
        }

        // write two bits for no change
        // as in, write {0,0} to indicate that the entity still exists,
        // but has not changed.
        NetBuffer_WriteBits(netBuffer, to->netIndex, NETINDEX_BITS);                    // {<NetIndex>}         sprite index
        NetBuffer_WriteBits(netBuffer, 0, 1);                                           // {<NetIndex>, 0}      sprite NOT deleted
        NetBuffer_WriteBits(netBuffer, 0, 1);                                           // {<NetIndex>, 0,0}    sprite has NOT changed.

        return;

    }

    // if we got to this point, the sprite / actor exists and has changed
    NetBuffer_WriteBits(netBuffer, to->netIndex, NETINDEX_BITS);                        // {<NetIndex>}         sprite index
    NetBuffer_WriteBits(netBuffer, 0, 1);                                               // {<NetIndex>, 0}      sprite/actor NOT deleted.
    NetBuffer_WriteBits(netBuffer, 1, 1);                                               // {<NetIndex>, 0,1}    sprite/actor HAS changed.

                                                                                        //--------------------------------------------------
                                                                                        // then...

    NetBuffer_WriteBits(netBuffer, maxChgIndex, STRUCTINDEX_BITS);						// Write Max change index

                                                                                        // For each field in struct...

    for (fieldIndex = 0, fieldPtr = ActorFields; fieldIndex < maxChgIndex; fieldIndex++, fieldPtr++)
    {
        const int8_t isFloatingPointField = (fieldPtr->bits == 0);

        fromField   = (int32_t const *)((int8_t const *)from    + fieldPtr->offset);
        toField     = (int32_t const *)((int8_t const *)to      + fieldPtr->offset);

        if (*fromField == *toField)
        {
            NetBuffer_WriteBits(netBuffer, 0, 1);                                       // {0}              field not changed
            continue;

        }

        NetBuffer_WriteBits(netBuffer, 1, 1);                                           // {1}              field changed

        if (isFloatingPointField)
        {
            int32_t rawFloatData = *(toField);

            NetBuffer_WriteDeltaFloat(netBuffer, rawFloatData);                         // {1, <special float behavior>}

        }
        else
        {
            if (*toField == 0)
            {
                NetBuffer_WriteBits(netBuffer, 0, 1);                                    // {1, 0}           Zero this field

            }
            else
            {
                NetBuffer_WriteBits(netBuffer, 1, 1);                                    // {1, 1}           don't zero this int field
                NetBuffer_WriteBits(netBuffer, *toField, fieldPtr->bits);                    // {1, 1, <value>}  new field value

            }

        }

    }

}


static void Net_WriteNetActorsToBuffer(NetBuffer_t* netBuffer, const netmapstate_t* from, const netmapstate_t* to)
{
    const netactor_t* fromActor = NULL;

    const netactor_t* toActor = NULL;

    int16_t     actorIndex = 0;

    int16_t     fromMaxIndex = 0;

    if (!from)
    {
        fromMaxIndex = 0;
    }
    else
    {
        fromMaxIndex = from->maxActorIndex;
    }

    while (
        actorIndex < to->maxActorIndex
        ||
        actorIndex < fromMaxIndex
        )
    {

        // load actor pointers using actor indexes
        if (actorIndex >= to->maxActorIndex)
        {
            toActor = NULL;
        }
        else
        {
            toActor = &to->actor[actorIndex];

            if (toActor->netIndex == cSTOP_PARSING_CODE)
            {
                toActor = NULL;
            }
        }


        if (actorIndex >= fromMaxIndex)
        {
            fromActor = NULL;
        }
        else
        {
            fromActor = &from->actor[actorIndex];

            if (fromActor->netIndex == cSTOP_PARSING_CODE)
            {
                fromActor = NULL;
            }
        }

        NetBuffer_WriteDeltaNetActor(netBuffer, fromActor, toActor, 0);

        actorIndex++;

    }
}


static void Net_WriteWorldToBuffer(NetBuffer_t* netBuffer, const netmapstate_t* fromSnapshot, const netmapstate_t* toSnapshot)
{
    int32_t index = 0;

    for (index = 0; index < numwalls; index++)
    {
        Bassert(index < MAXWALLS);

        const netWall_t* fromWall = &fromSnapshot->wall[index];
        const netWall_t* toWall = &toSnapshot->wall[index];

        NetBuffer_WriteDeltaNetWall(netBuffer, fromWall, toWall);

    }

    NetBuffer_WriteBits(netBuffer, cSTOP_PARSING_CODE, NETINDEX_BITS);



    for (index = 0; index < numsectors; index++)
    {
        Bassert(index < MAXSECTORS);

        const netSector_t* fromSector = &fromSnapshot->sector[index];
        const netSector_t* toSector = &toSnapshot->sector[index];

        NetBuffer_WriteDeltaNetSector(netBuffer, fromSector, toSector);
    }

    NetBuffer_WriteBits(netBuffer, cSTOP_PARSING_CODE, NETINDEX_BITS);

    Net_WriteNetActorsToBuffer(netBuffer, fromSnapshot, toSnapshot);

    NetBuffer_WriteBits(netBuffer, cSTOP_PARSING_CODE, NETINDEX_BITS); // end of actors/sprites

}


// buffer -> net struct functions
//----------------------------------------------------------------------------------------------------------

// I agonized over whether to just make a more generic NetBuffer_ReadDeltaWorldEntry since there's so little
// difference between walls and sectors for this function but my attempts ended up being much harder
// to read and probably more error prone than using "clipboard" inheritance, I really hope I made the right call here...

static void NetBuffer_ReadDeltaWall(NetBuffer_t *netBuffer, const netWall_t *from, netWall_t *to, uint16_t netIndex)
{
    int32_t         fieldIndex,
                    maxChgIndex;                               // the number of fields that changed in this delta struct from the server

    const   int32_t         cMaxStructs = MAXWALLS;
    const   int32_t         cStructFields = ARRAY_SIZE(WallFields);     // the number of fields in the full struct

    netField_t      *cFieldsArray = &(WallFields[0]);

    netField_t      *field;

    const NetChunk32      *fromField;
          NetChunk32      *toField;

    if (netIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaWall: Bad Netindex to read.");
        return;
    }

    if (from->netIndex != netIndex)
    {
        Net_Error_Disconnect("Unexpected from struct netIndex. from struct netIndex should match the netIndex parameter.");
        return;
    }

    maxChgIndex = NetBuffer_ReadBits(netBuffer, STRUCTINDEX_BITS);

    if (maxChgIndex  > cStructFields || maxChgIndex < 0)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaWall: Invalid delta field count from client.");
        return;
    }

    to->netIndex = netIndex;

    // for each field of the delta struct
    for (fieldIndex = 0, field = cFieldsArray; fieldIndex < maxChgIndex; fieldIndex++, field++)
    {
        fromField = (int32_t const *)((int8_t const *)from  + field->offset);
        toField   = (int32_t *)      ((int8_t *)      to    + field->offset);

        // no change to this field
        if (!NetBuffer_ReadBits(netBuffer, 1))
        {
            *toField = *fromField;

        }

        // field has changed
        else
        {
            // zero the field
            if (NetBuffer_ReadBits(netBuffer, 1) == 0)
            {
                *toField = 0;

            }

            // read the whole field
            else
            {
                *toField = NetBuffer_ReadBits(netBuffer, field->bits);

            }

        }
    }

    // if the delta for this struct doesn't include every field of the struct (likely)
    // just set every field after the end of this delta struct to the "from" value
    for (fieldIndex = maxChgIndex, field = &cFieldsArray[maxChgIndex]; fieldIndex < cStructFields; fieldIndex++, field++)
    {
        fromField   = (NetChunk32 const *)((int8_t const *)from + field->offset);
        toField     = (NetChunk32 *)      ((int8_t *)      to + field->offset);

        // no change on all the rest of the fields of the struct
        *toField = *fromField;
    }


}

static void NetBuffer_ReadDeltaSector(NetBuffer_t *netBuffer, const netSector_t *from, netSector_t *to, uint16_t netIndex)
{
    int32_t         fieldIndex,
                    maxChgIndex;                                    // the number of fields that changed in this delta struct from the server

    const   int32_t         cMaxStructs = MAXSECTORS;
    const   int32_t         cStructFields = ARRAY_SIZE(SectorFields);       // the number of fields in the full struct

    netField_t      *cFieldsArray = &(SectorFields[0]);

    netField_t      *field;

    const NetChunk32      *fromField;
          NetChunk32      *toField;


    if (netIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaSector: Bad Netindex to read.");
        return;
    }

    if (from->netIndex != netIndex)
    {
        Net_Error_Disconnect("Unexpected from struct netIndex. from struct netIndex should match the netIndex parameter.");
        return;
    }

    maxChgIndex = NetBuffer_ReadBits(netBuffer, STRUCTINDEX_BITS);

    if (maxChgIndex  > cStructFields || maxChgIndex < 0)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaSector: Invalid delta field count from client.");
        return;
    }

    to->netIndex = netIndex;

    // for each field of the delta struct
    for (fieldIndex = 0, field = cFieldsArray; fieldIndex < maxChgIndex; fieldIndex++, field++)
    {
        fromField = (int32_t const *)((int8_t const *)from + field->offset);
        toField   = (int32_t *)      ((int8_t *)      to + field->offset);

        // no change to this field
        if (!NetBuffer_ReadBits(netBuffer, 1))
        {
            *toField = *fromField;

        }

        // field has changed
        else
        {
            // zero the field
            if (NetBuffer_ReadBits(netBuffer, 1) == 0)
            {
                *toField = 0;

            }

            // read the whole field
            else
            {
                *toField = NetBuffer_ReadBits(netBuffer, field->bits);

            }

        }
    }

    // if the delta for this struct doesn't include every field of the struct (likely)
    // just set every field after the end of this delta struct to the "from" value
    for (fieldIndex = maxChgIndex, field = &cFieldsArray[maxChgIndex]; fieldIndex < cStructFields; fieldIndex++, field++)
    {
        // I am not sure if this is type punning or not, just to be safe
        fromField   = (NetChunk32 const *)((int8_t const *)from + field->offset);
        toField     = (NetChunk32 *)      ((int8_t *)      to + field->offset);

        // no change on all the rest of the fields of the struct
        *toField = *fromField;
    }


}



static void NetBuffer_ReadDeltaActor(NetBuffer_t* netBuffer, const netactor_t *from, netactor_t *to, uint16_t actorIndex)
{
    int32_t         fieldIndex,
                    maxChgIndex;                                   // the number of fields that changed in this delta struct from the server

    const	int32_t         cMaxStructs = MAXWALLS;
    const	int32_t         cStructFields = ARRAY_SIZE(ActorFields);        // the number of fields in the full struct

    netField_t      *cFieldsArray = &(ActorFields[0]);

    netField_t      *field;

    const NetChunk32      *fromField;
          NetChunk32      *toField;

    int32_t         removeActor;

    int32_t         actorChanged; // this is only used if the packet is sent with "force"

    int32_t         fromActorDeleted = (from->spr_sectnum == MAXSECTORS);

    int32_t         actorIndexOK = ((fromActorDeleted) || (from->netIndex == actorIndex));

    if (actorIndex >= cMaxStructs)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaActor: Bad Netindex to read.");
    }

    if (!actorIndexOK)
    {
        Net_Error_Disconnect("Unexpected from struct netIndex. from struct netIndex should match the netIndex parameter.");
        return;
    }


    removeActor = NetBuffer_ReadBits(netBuffer, 1);                                    // read actor deleted bit

                                                                                       // if this actor is being deleted, fill it with zeros and set its netIndex to STOP_PARSING_CODE
    if (removeActor == 1)
    {
        Net_InitNetActor(to);

        return;
    }

    actorChanged = NetBuffer_ReadBits(netBuffer, 1);                                     // read actor changed bit

    to->netIndex = actorIndex;

    if (actorChanged == 0)
    {
        *to = *from;

        if (to->netIndex == cSTOP_PARSING_CODE)
        {
            Net_Error_Disconnect("Read invalid netindex from delta actor buffer.");
        }

        return;
    }

    maxChgIndex = NetBuffer_ReadBits(netBuffer, STRUCTINDEX_BITS);                          // max change index

    if (maxChgIndex  > cStructFields || maxChgIndex < 0)
    {
        Net_Error_Disconnect("NetBuffer_ReadDeltaActor: Invalid delta field count from server.");
    }

    // for each field of the delta struct
    for (fieldIndex = 0, field = cFieldsArray; fieldIndex < maxChgIndex; fieldIndex++, field++)
    {
        int8_t isFloatField = (field->bits == 0);

        fromField = (int32_t const *)((int8_t const *)from + field->offset);
        toField   = (int32_t *)      ((int8_t *)      to + field->offset);

        // no change to this field
        if (!NetBuffer_ReadBits(netBuffer, 1))
        {
            *toField = *fromField;

        }

        // field has changed
        else
        {
            if (isFloatField)
            {
                int32_t notZeroed = NetBuffer_ReadBits(netBuffer, 1);

                if (notZeroed == 0)
                {
                    *(float *)toField = 0.0f;

                }

                else
                {
                    int32_t sentAsRealFloat = NetBuffer_ReadBits(netBuffer, 1);

                    if (sentAsRealFloat == 0)
                    {
                        // float was written as a truncated FLOAT_INT_BITS integer
                        NetChunk32 truncated = NetBuffer_ReadBits(netBuffer, FLOAT_INT_BITS);

                        // remove bias to allow this to be signed
                        truncated -= cTruncInt_Max;

                        *(float *)toField = (float)truncated;
                    }

                    else
                    {
                        // read the raw float data directly from the buffer
                        *toField = NetBuffer_ReadBits(netBuffer, 32);
                    }
                }
            }

            // not a float field
            else
            {

                // zero the field
                if (NetBuffer_ReadBits(netBuffer, 1) == 0)
                {
                    *toField = 0;

                }

                // read the whole field
                else
                {
                    *toField = NetBuffer_ReadBits(netBuffer, field->bits);

                }
            }

        }
    }

    // if the delta for this struct doesn't include every field of the struct (likely)
    // just set every field after the end of this delta struct to the "from" value
    for (fieldIndex = maxChgIndex, field = &cFieldsArray[maxChgIndex]; fieldIndex < cStructFields; fieldIndex++, field++)
    {
        // I am really not 100% sure if this is type punning or not, just to be safe.
        fromField   = (NetChunk32 const *)((int8_t const *)from + field->offset);
        toField     = (NetChunk32 *)      ((int8_t *)      to + field->offset);

        // no change on all the rest of the fields of the struct
        *toField = *fromField;
    }

    if (to->netIndex == cSTOP_PARSING_CODE)
    {
        Net_Error_Disconnect("Read invalid netindex from delta actor buffer.");
    }

}

// Using oldSnapshot as the "From" snapshot, parse the data in netBuffer as a diff from
// oldSnapshot to make newSnapshot.
static void Net_ParseWalls(NetBuffer_t *netBuffer, netmapstate_t *oldSnapshot, netmapstate_t *newSnapshot)
{

    int32_t         newSnapshotNetIndex = 0;
    int32_t         oldSnapshotNetIndex = 0;



    netWall_t       *oldSnapshotStruct = NULL;
    netWall_t       *newSnapshotStruct = NULL;

    const   int32_t         cMaxStructIndex = numwalls - 1;

    NET_DEBUG_VAR int32_t       DEBUG_NumberOfStructsInPacket  = 0;

    NET_DEBUG_VAR int32_t       DEBUG_FirstChangedIndex  = -1234;
    NET_DEBUG_VAR int32_t       DEBUG_LastChangedIndex  = -1234;



#if DEBUG_CHANGEDINDEXES
    NET_DEBUG_VAR int32_t       DEBUG_ChangedIndexes[NetNumberOfIndexes];

    memset(&DEBUG_ChangedIndexes, 0, sizeof(DEBUG_ChangedIndexes));

#endif

    //read each struct from the delta packet, until the NetIndex == the stop number
    while (1)
    {
        // read the netIndex of this struct
        newSnapshotNetIndex = NetBuffer_ReadBits(netBuffer, NETINDEX_BITS);

        if (newSnapshotNetIndex < 0)
        {
            Net_Error_Disconnect("Net_ParseWalls: Invalid netIndex from client.");
            return;
        }

        //================================================================
        // DEBUG ONLY

        if (DEBUG_FirstChangedIndex < 0)
        {
            DEBUG_FirstChangedIndex = newSnapshotNetIndex;
        }

        if (newSnapshotNetIndex != cSTOP_PARSING_CODE)
        {
            DEBUG_LastChangedIndex = newSnapshotNetIndex;
        }

        //----------------------------------------------------------------

        if (newSnapshotNetIndex == (cSTOP_PARSING_CODE))
        {
            break;
        }

        if (newSnapshotNetIndex > cMaxStructIndex)
        {
            // this assert is only here to test the STOP_PARSING_CODE because we need that code to work for actors/sprites,
            // since the number of walls/sectors does not change, it's safe to return here.
            Bassert(0);
            break;
        }

        newSnapshotStruct = &(newSnapshot->wall[newSnapshotNetIndex]);
        oldSnapshotStruct = &(oldSnapshot->wall[oldSnapshotNetIndex]);

        if (netBuffer->ReadCurByte > netBuffer->CurSize)
        {
            Net_Error_Disconnect("Net_ParseWalls: Reached end of buffer without finding a stop code.");
            return;
        }

        // index up to the point where the changed structs start
        while (oldSnapshotNetIndex < newSnapshotNetIndex)
        {
            // if it's not in the delta entity set, just use the previous snapshot's value
            newSnapshot->wall[oldSnapshotNetIndex] = oldSnapshot->wall[oldSnapshotNetIndex];
            oldSnapshotNetIndex++;
            oldSnapshotStruct = &(oldSnapshot->wall[oldSnapshotNetIndex]);

        }

        // the struct referred to by oldindex is the same struct as the one referred to by newindex,
        // compare the two structs
        if (oldSnapshotNetIndex == newSnapshotNetIndex)
        {
            NetBuffer_ReadDeltaWall(netBuffer, oldSnapshotStruct, newSnapshotStruct, newSnapshotNetIndex);

            oldSnapshotNetIndex++;

            DEBUG_NumberOfStructsInPacket++;

#if DEBUG_CHANGEDINDEXES
            DEBUG_ChangedIndexes[newSnapshotNetIndex] = 1;
#endif

        }

    }

    // No more walls changed for the new snapshot, but there's still walls in the old snapshot.
    //
    // all of those old walls are unchanged. Copy all of these into the new snapshot.
    while (oldSnapshotNetIndex < numwalls)
    {
        newSnapshot->wall[oldSnapshotNetIndex] = oldSnapshot->wall[oldSnapshotNetIndex];

        oldSnapshotNetIndex++;
    }

}

static void Net_ParseSectors(NetBuffer_t *netBuffer, netmapstate_t *oldSnapshot, netmapstate_t *newSnapshot)
{

    int32_t         newSnapshotNetIndex = 0;
    int32_t         oldSnapshotNetIndex = 0;



    netSector_t     *oldSnapshotStruct = NULL;
    netSector_t     *newSnapshotStruct = NULL;

    const   int32_t         cMaxStructIndex = numsectors - 1;

    NET_DEBUG_VAR int32_t       DEBUG_PrevNewIndex  = -1;

    NET_DEBUG_VAR int32_t       DEBUG_FirstChangedIndex  = -1234;
    NET_DEBUG_VAR int32_t       DEBUG_LastChangedIndex = -1234;



#if DEBUG_CHANGEDINDEXES
    NET_DEBUG_VAR int32_t       DEBUG_ChangedIndexes[NetNumberOfIndexes];

    memset(&DEBUG_ChangedIndexes, 0, sizeof(DEBUG_ChangedIndexes));

#endif

    //read each struct from the delta packet, until the NetIndex == the stop number
    while (1)
    {
        // read the netIndex of this struct
        newSnapshotNetIndex = NetBuffer_ReadBits(netBuffer, NETINDEX_BITS);

        //================================================================
        // DEBUG ONLY

        if (DEBUG_FirstChangedIndex < 0)
        {
            DEBUG_FirstChangedIndex = newSnapshotNetIndex;
        }

        if (newSnapshotNetIndex != cSTOP_PARSING_CODE)
        {
            DEBUG_LastChangedIndex = newSnapshotNetIndex;
        }

        //----------------------------------------------------------------


        if (newSnapshotNetIndex < DEBUG_PrevNewIndex)
        {
            Net_Error_Disconnect("Error: Sectors were recieved out of order.");
        }

        DEBUG_PrevNewIndex = newSnapshotNetIndex;

        if (newSnapshotNetIndex < 0)
        {
            Net_Error_Disconnect("Net_ParseSectors: Invalid netIndex from client.");
            return;
        }

        if (newSnapshotNetIndex == (cSTOP_PARSING_CODE))
        {
            break;
        }

        if (newSnapshotNetIndex > cMaxStructIndex)
        {
            // this assert is only here to test the STOP_PARSING_CODE because we need that code to work for actors/sprites,
            // since the number of walls/sectors does not change, it's safe to return here.
            //
            // also this can help catch corrupted packets
            Bassert(0);
            break;
        }

        newSnapshotStruct = &(newSnapshot->sector[newSnapshotNetIndex]);
        oldSnapshotStruct = &(oldSnapshot->sector[oldSnapshotNetIndex]);

        if (netBuffer->ReadCurByte > netBuffer->CurSize)
        {
            Net_Error_Disconnect("Net_ParseSectors: Reached end of buffer without finding a stop code.");
            return;
        }

        // index up to the point where the changed structs start
        while (oldSnapshotNetIndex < newSnapshotNetIndex)
        {
            // if it's not in the delta entity set, just use the previous snapshot's value
            newSnapshot->sector[oldSnapshotNetIndex] = oldSnapshot->sector[oldSnapshotNetIndex];
            oldSnapshotNetIndex++;
            oldSnapshotStruct = &(oldSnapshot->sector[oldSnapshotNetIndex]);
        }

        // the struct referred to by oldindex is the same struct as the one referred to by newindex,
        // compare the two structs
        if (oldSnapshotNetIndex == newSnapshotNetIndex)
        {
            NetBuffer_ReadDeltaSector(netBuffer, oldSnapshotStruct, newSnapshotStruct, newSnapshotNetIndex);

            oldSnapshotNetIndex++;

#if DEBUG_CHANGEDINDEXES
            DEBUG_ChangedIndexes[newSnapshotNetIndex] = 1;
#endif

        }

    }

    // No more sectors changed for the new snapshot, but there's still sectors in the old snapshot.
    //
    // all of those old sectors are unchanged. Copy all of these into the new snapshot.
    while (oldSnapshotNetIndex < numsectors)
    {
        newSnapshot->sector[oldSnapshotNetIndex] = oldSnapshot->sector[oldSnapshotNetIndex];

        oldSnapshotNetIndex++;
    }

}

static void Net_ParseActors(NetBuffer_t *netBuffer, const netmapstate_t* oldSnapshot, netmapstate_t* newSnapshot)
{

    // FROM PACKET: index into sprite[] (game arrays)
    int32_t         newActorIndex = 0;

    // INCREMENTED: index into sprite[] (game arrays)
    int32_t         oldActorIndex = 0;

    const   netactor_t      *oldSnapshotStruct = NULL;
    netactor_t              *newSnapshotStruct = NULL;

    const   int32_t         cActorIndex_OutOfOldFrameActors = 99999;

    NET_DEBUG_VAR int32_t   DEBUG_FirstChangedIndex  = -1234;
    NET_DEBUG_VAR int32_t   DEBUG_LastChangedIndex   = -1234;



#if DEBUG_CHANGEDINDEXES
    NET_DEBUG_VAR int32_t   DEBUG_ChangedIndexes[NetNumberOfIndexes];

    memset(&DEBUG_ChangedIndexes, 0, sizeof(DEBUG_ChangedIndexes));

#endif

    newSnapshot->maxActorIndex = 0;

    if (!oldSnapshot)
    {
        oldActorIndex = cActorIndex_OutOfOldFrameActors;
    }
    else
    {
        oldSnapshotStruct = &oldSnapshot->actor[oldActorIndex];
    }

    //read each struct from the delta packet, until the NetIndex == the stop number
    //i.e., for each actor in the actors section of the packet...
    while (1)
    {
        newActorIndex = NetBuffer_ReadBits(netBuffer, NETINDEX_BITS);

        //================================================================
        // DEBUG ONLY

        if (DEBUG_FirstChangedIndex < 0)
        {
            DEBUG_FirstChangedIndex = newActorIndex;
        }

        if (newActorIndex != cSTOP_PARSING_CODE)
        {
            DEBUG_LastChangedIndex = newActorIndex;
        }

        //----------------------------------------------------------------

        if (newActorIndex == cSTOP_PARSING_CODE)
        {
            //NOTE: This is okay, there is a while loop below to handle cases
            //      where there are unchanged actors after the last changed actor in the
            //      netbuffer.
            break;
        }

        if (netBuffer->ReadCurByte > netBuffer->CurSize)
        {
            Net_Error_Disconnect("Net_ParseActors: Reached end of buffer without finding a stop code.");
        }

        if ((newActorIndex < 0) || (newActorIndex > MAXSPRITES))
        {
            Net_Error_Disconnect("Net_ParseActors: Invalid netIndex from client.");
        }

        // loop up to the point where the structs changed between the old frame and the new frame start
        while (oldActorIndex < newActorIndex)
        {
            //actor is unchanged in the new snapshot, just *copy* the previous snapshot's value
            newSnapshot->actor[oldActorIndex] = oldSnapshot->actor[oldActorIndex];

            oldActorIndex++;

        }

        // NOTE that actors deleted for the new snapshot will be processed *here*. New actors that "fill in gaps"
        // (rather than being added to the end) in the actor list will be processed here too.
        if (oldActorIndex == newActorIndex)
        {
            oldSnapshotStruct = &(oldSnapshot->actor[oldActorIndex]);
            newSnapshotStruct = &(newSnapshot->actor[newActorIndex]);

            NetBuffer_ReadDeltaActor(netBuffer, oldSnapshotStruct, newSnapshotStruct, newActorIndex);

            oldActorIndex++;

#if DEBUG_CHANGEDINDEXES
            DEBUG_ChangedIndexes[newSnapshotStruct->netIndex] = 1;
#endif

            continue; // because we handled the newActorIndex read from the packet.
        }

        // This actor is a newly inserted actor, either to be put in a space between
        // two actors in the old snapshot, or after the end of the old snapshot.
        //
        // Note that the "no more oldframe entities" index constant runs this
        if (oldActorIndex > newActorIndex)
        {
            newSnapshotStruct = &(newSnapshot->actor[newActorIndex]);

            NetBuffer_ReadDeltaActor(netBuffer, &cNullNetActor, newSnapshotStruct, newActorIndex);

        }
    }

    // No more actors changed for the new snapshot, but there's still actors in the old snapshot.
    //
    // All of those old actors are unchanged. Copy all of these into the new snapshot.
    // Remember that deleting an actor counts as a "change".
    while (oldActorIndex < MAXSPRITES)
    {
        newSnapshot->actor[oldActorIndex] = oldSnapshot->actor[oldActorIndex];

        oldActorIndex++;

    }

    NET_75_CHECK++; // For now every snapshot will have MAXSPRITES entries
    newSnapshot->maxActorIndex = MAXSPRITES;
}



static void NetBuffer_ReadWorldSnapshotFromBuffer(NetBuffer_t* netBuffer, netmapstate_t* oldSnapshot, netmapstate_t* newSnapshot)
{
    // note that the order these functions are called should match the order that these structs are written to
    // by the server
    Net_ParseWalls(netBuffer, oldSnapshot, newSnapshot);
    Net_ParseSectors(netBuffer, oldSnapshot, newSnapshot);
    Net_ParseActors(netBuffer, oldSnapshot, newSnapshot);
}


static void Net_SendWorldUpdate(uint32_t fromRevisionNumber, uint32_t toRevisionNumber, int32_t sendToPlayerIndex)
{
    if (sendToPlayerIndex == myconnectindex)
    {
        return;
    }

    Bassert(MAX_WORLDBUFFER == ARRAY_SIZE(tempnetbuf));
    Bassert(NET_REVISIONS == ARRAY_SIZE(g_mapStateHistory));

    uint32_t        playerRevisionIsTooOld = (toRevisionNumber - fromRevisionNumber) > NET_REVISIONS;

    // to avoid the client thinking that revision 2 is older than revision 0xFFFF_FFFF,
    // send packets to take the client from the map's initial state until the client reports back
    // that it's beyond that rollover threshold.
    uint32_t        revisionInRolloverState = (fromRevisionNumber > toRevisionNumber);


    NetBuffer_t     buffer;
    NetBuffer_t*    bufferPtr = &buffer;

    // note: not enough stack memory to put the world data as a local variable
    uint8_t*        byteBuffer = &tempnetbuf[1];


    uint32_t        fromRevisionNumberToSend = 0x86753090;

    netmapstate_t*  toMapState = &g_mapStateHistory[toRevisionNumber % NET_REVISIONS];
    netmapstate_t*  fromMapState = NULL;

    NET_75_CHECK++; // during the rollover state it might be a good idea to init the map state history?
                    // maybe not? I do init map states before using them, so it might not be needed.


    if (playerRevisionIsTooOld || revisionInRolloverState)
    {
        fromMapState = &g_mapStartState;
        fromRevisionNumberToSend = cInitialMapStateRevisionNumber;
    }
    else
    {
        uint32_t tFromRevisionIndex = fromRevisionNumber % NET_REVISIONS;

        fromMapState = &g_mapStateHistory[tFromRevisionIndex];
        fromRevisionNumberToSend = fromRevisionNumber;
    }


    Bmemset(byteBuffer, 0, sizeof(tempnetbuf) - 1);

    tempnetbuf[0] = PACKET_WORLD_UPDATE;

    NetBuffer_Init(bufferPtr, byteBuffer, MAX_WORLDBUFFER);

    NetBuffer_WriteDword(bufferPtr, fromRevisionNumberToSend);
    NetBuffer_WriteDword(bufferPtr, toRevisionNumber);

    Net_WriteWorldToBuffer(bufferPtr, fromMapState, toMapState);

    if (sendToPlayerIndex > ((int32_t) g_netServer->peerCount))
    {
        Net_Error_Disconnect("No peer for player.");
        return;
    }

    // in the future we could probably use these flags for enet_peer_send, for the world updates
    EDUKE32_UNUSED const ENetPacketFlag optimizedFlags = (ENetPacketFlag)(ENET_PACKET_FLAG_UNSEQUENCED | ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);

    NET_75_CHECK++; // HACK: I Really need to keep the peer with the player instead of assuming that the peer index is the same as the (player index - 1)
    ENetPeer *const tCurrentPeer = &g_netServer->peers[sendToPlayerIndex - 1];
    enet_peer_send(tCurrentPeer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf, bufferPtr->CurSize + 1, 0));
    Dbg_PacketSent(PACKET_WORLD_UPDATE);


}

static void Net_CopySnapshotToGameArrays(netmapstate_t* srv_snapshot, netmapstate_t* cl_snapshot)
{
    int32_t index;

    // this isn't just copying the entries that "changed" into the game arrays,
    // remember that we need to correct any incorrect guesses the client made.
    // don't do memcpy either, e.g., sizeof(netWall_t) != sizeof(walltype)

    for (index = 0; index < numwalls; index++)
    {
        netWall_t*  srvWall = &(srv_snapshot->wall[index]);
        netWall_t*  clWall  = &(cl_snapshot->wall[index]);

        int status = memcmp(srvWall, clWall, sizeof(netWall_t));

        if(status == 0)
        {
            if(g_enableClientInterpolationCheck)
            {
                // only copy in the server's (old) value if the client's interpolation up to that point was incorrect
                continue;
            }
        }

        walltype*   gameWall = &(wall[index]);

        Net_CopyWallFromNet(srvWall, gameWall);

    }

    for (index = 0; index < numsectors; index++)
    {
        netSector_t*  srvSector = &(srv_snapshot->sector[index]);
        netSector_t*  clSector  = &(cl_snapshot->sector[index]);

        int status = memcmp(srvSector, clSector, sizeof(netSector_t));

        if(status == 0)
        {
            if(g_enableClientInterpolationCheck)
            {
                continue;
            }
        }

        sectortype*   gameSector = &(sector[index]);

        Net_CopySectorFromNet(srvSector, gameSector);
    }

    Net_CopyActorsToGameArrays(srv_snapshot, cl_snapshot);
}

// clients only
static void Net_ReadWorldUpdate(uint8_t* packetData, int32_t packetSize)
{
    if (!g_netClient)
    {
        return;
    }

    NetBuffer_t     buffer;
    NetBuffer_t*    bufferPtr = &buffer;
    uint8_t*        dataStartAddr = packetData + 1;

    NET_DEBUG_VAR int16_t   DEBUG_NoMapLoaded               = ((numwalls < 1) || (numsectors < 1));
    NET_DEBUG_VAR int32_t   DEBUG_InitialSnapshotNotSet     = (g_mapStartState.sector[0].wallnum <= 0);

    if (!ClientPlayerReady)
    {
        return;
    }

    NetBuffer_Init(bufferPtr, dataStartAddr, MAX_WORLDBUFFER);

    bufferPtr->CurSize = packetSize - 1;

    uint32_t packetFromRevisionNumber = NetBuffer_ReadDWord(bufferPtr);
    uint32_t packetToRevisionNumber = NetBuffer_ReadDWord(bufferPtr);

    uint32_t from_IsInitialState = (packetFromRevisionNumber == cInitialMapStateRevisionNumber);

    uint32_t clientRevisionIsTooOld = (packetToRevisionNumber - g_netMapRevisionNumber) > NET_REVISIONS;

    netmapstate_t* fromMapState = NULL;

    if (clientRevisionIsTooOld && !from_IsInitialState)
    {
        // this is actually not a major problem, an alternative way to handle this would be to just ignore the packet and hope the
        // server sends a diff from the initial revision eventually.
        //
        // NOTE: If you get this bug on connect it really is a problem, the client never seems to get updated to the initial revision when
        // this happens. It seems like if you don't wat enough time before connecting again after a crash you will get this to happen.
        //
        // actually I've seen that happen even when this error didn't happen.
        Net_Error_Disconnect("Internal Error: Net_ReadWorldUpdate(): Client From map state too old, but server did not send snapshot from initial state.");
        return;
    }

    if (from_IsInitialState)
    {
        // clients must always accept the initial map state as a snapshot,
        // If a client's current revision is too old, the server will send them
        // a snapshot to update them from the initial state instead of the
        // client's last known state.
        fromMapState = &g_mapStartState;
    }
    else if (packetToRevisionNumber <= g_netMapRevisionNumber)
    {
        // clients should just ignore revisions that are older than their current revision
        // (packets can arrive out of order). Note that the server will send cInitialMapStateRevisionNumber
        // as the "From" index when the revision counter rolls over.
        return;
    }
    else
    {
        fromMapState = &g_mapStateHistory[packetFromRevisionNumber % NET_REVISIONS];
    }


    netmapstate_t* toMapState = &g_mapStateHistory[packetToRevisionNumber % NET_REVISIONS];
    netmapstate_t* clMapState = &g_cl_InterpolatedMapStateHistory[packetToRevisionNumber % NET_REVISIONS];

    toMapState->revisionNumber = packetToRevisionNumber;

    NET_DEBUG_VAR uint32_t DEBUG_OldClientRevision = g_netMapRevisionNumber;

    Bassert(fromMapState);
    NetBuffer_ReadWorldSnapshotFromBuffer(bufferPtr, fromMapState, toMapState);

    g_netMapRevisionNumber = packetToRevisionNumber;

    Net_CopySnapshotToGameArrays(toMapState, clMapState);

}

// handles revision rollover
static uint32_t Net_GetNextRevisionNumber(uint32_t currentNumber)
{
    if(currentNumber == UINT32_MAX)
    {
        // we should be able to recover from this
        OSD_Printf("Net_SendMapUpdates(): [Note] Map state revision number overflow.");
        return cStartingRevisionIndex;
    }

    return (currentNumber + 1);
}

// ---------------------------------------------------------------------------------------------------------------
// Externally accessible functions
// ---------------------------------------------------------------------------------------------------------------



void Net_WaitForInitialSnapshot()
{
    while (g_netMapRevisionNumber < cStartingRevisionIndex)
    {
        G_HandleAsync();
    }
}

// this is mostly just here to put a breakpoint on, use conditional breakpoints to stop only on particular types of packets
int32_t Dbg_PacketSent(enum DukePacket_t iPacketType)
{
    return iPacketType + 1;
}

// If failed is true, that means the vote lost. Otherwise it was cancelled by the client who initiated it.
void Net_SendMapVoteCancel(int32_t failed)
{
    // Only the server or the client that initiated the vote can cancel the vote
    if (g_netClient && voting != myconnectindex)
    {
        return;
    }

    tempbuf[0] = PACKET_MAP_VOTE_CANCEL;
    tempbuf[1] = myconnectindex;

    // If we're forwarding a cancelled message, change the connection index to the one who cancelled it.
    if (g_netServer && !failed)
    {
        tempbuf[1] = voting;
    }

    voting = -1;

    Dbg_PacketSent(PACKET_MAP_VOTE_CANCEL);

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_SendMapVote(int32_t votefor)
{
    voting                           = -1;
    g_player[myconnectindex].gotvote = 1;
    g_player[myconnectindex].vote    = votefor;

    tempbuf[0] = PACKET_MAP_VOTE;
    tempbuf[1] = myconnectindex;
    tempbuf[2] = votefor;
    tempbuf[3] = myconnectindex;

    Dbg_PacketSent(PACKET_MAP_VOTE);

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 4, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&tempbuf[0], 4, ENET_PACKET_FLAG_RELIABLE));
    }

    Net_CheckForEnoughVotes();
}

void Net_SendMapVoteInitiate(void)
{
    newgame_t newgame;

    if (!g_netClient)
    {
        return;
    }

    voting = myconnectindex;

    newgame.header     = PACKET_MAP_VOTE_INITIATE;
    newgame.connection = myconnectindex;
    Net_FillNewGame(&newgame, 1);

    Dbg_PacketSent(PACKET_MAP_VOTE_INITIATE);

    enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
}


void Net_SendNewGame(int32_t frommenu, ENetPeer *peer)
{
    newgame_t newgame;

    newgame.header = PACKET_NEW_GAME;
    Net_FillNewGame(&newgame, frommenu);

    Dbg_PacketSent(PACKET_NEW_GAME);

    if (peer)
    {
        enet_peer_send(peer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
    }
    else
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&newgame, sizeof(newgame_t), ENET_PACKET_FLAG_RELIABLE));
    }

    Net_ResetPlayerReady();
}


void Net_FillNewGame(newgame_t *newgame, int32_t frommenu)
{
    if (frommenu)
    {
        newgame->level_number      = ud.m_level_number;
        newgame->volume_number     = ud.m_volume_number;
        newgame->player_skill      = ud.m_player_skill;
        newgame->monsters_off      = ud.m_monsters_off;
        newgame->respawn_monsters  = ud.m_respawn_monsters;
        newgame->respawn_items     = ud.m_respawn_items;
        newgame->respawn_inventory = ud.m_respawn_inventory;
        newgame->ffire             = ud.m_ffire;
        newgame->noexits           = ud.m_noexits;
        newgame->coop              = ud.m_coop;
    }
    else
    {
        newgame->level_number      = ud.level_number;
        newgame->volume_number     = ud.volume_number;
        newgame->player_skill      = ud.player_skill;
        newgame->monsters_off      = ud.monsters_off;
        newgame->respawn_monsters  = ud.respawn_monsters;
        newgame->respawn_items     = ud.respawn_items;
        newgame->respawn_inventory = ud.respawn_inventory;
        newgame->ffire             = ud.ffire;
        newgame->noexits           = ud.noexits;
        newgame->coop              = ud.coop;
    }
}

// store the client's history so that we can go back and look and see if the client interpolated properly
// (the server snapshots are always older than the client's current game tic)
void Net_StoreClientState(void)
{
    if(!g_netClient)
    {
        return;
    }

    // don't store client revisions if we don't even have a server revision
    if(g_netMapRevisionNumber < cStartingRevisionIndex)
    {
        return;
    }

    g_cl_InterpolatedRevision = Net_GetNextRevisionNumber(g_cl_InterpolatedRevision);

    netmapstate_t* currentMapState = &g_cl_InterpolatedMapStateHistory[g_cl_InterpolatedRevision % NET_REVISIONS];

    Net_InitMapState(currentMapState);
    Net_AddWorldToSnapshot(currentMapState);

    currentMapState->revisionNumber = g_cl_InterpolatedRevision;

}

void Net_SendMapUpdate(void)
{
    if (g_netClient || !g_netServer || numplayers < 2)
    {
        return;
    }

    g_netMapRevisionNumber = Net_GetNextRevisionNumber(g_netMapRevisionNumber);

    netmapstate_t* toMapState = &g_mapStateHistory[g_netMapRevisionNumber % NET_REVISIONS];

    Net_InitMapState(toMapState);
    Net_AddWorldToSnapshot(toMapState);

    toMapState->revisionNumber = g_netMapRevisionNumber;

    int32_t playerIndex = 0;

    for (TRAVERSE_CONNECT(playerIndex))
    {
        if (playerIndex == myconnectindex)
        {
            // there's no point in the server sending itself a snapshot.
            continue;
        }

        uint32_t playerRevisionNumber = g_player[playerIndex].revision;

        Net_SendWorldUpdate(playerRevisionNumber, g_netMapRevisionNumber, playerIndex);
    }

}


void DumpMapStateHistory()
{
    const char* fileName = NULL;

    if(g_netClient)
    {
        fileName = "CL_MapStates.bin";
    }
    else
    {
        fileName = "SRV_MapStates.bin";
    }

    FILE* mapStatesFile = fopen(fileName, "wb");

    // write the null map state (it should never, ever be changed, but just for completeness sake
    // fwrite(&NullMapState, sizeof(NullMapState), 1, mapStatesFile);

    fwrite(&g_mapStartState, sizeof(g_mapStartState), 1, mapStatesFile);

    fwrite(&g_mapStateHistory[0], sizeof(g_mapStateHistory), 1, mapStatesFile);

    OSD_Printf("Dumped map states to %s.\n", fileName);

    fclose(mapStatesFile);
    mapStatesFile = NULL;
}


void Net_SpawnPlayer(int32_t player)
{
    int32_t byteOffset    = 0;
    packbuf[byteOffset++] = PACKET_PLAYER_SPAWN;
    packbuf[byteOffset++] = player;

    Bmemcpy(&packbuf[byteOffset], &g_player[player].ps->pos.x, sizeof(vec3_t) * 2);
    byteOffset += sizeof(vec3_t) * 2;

    packbuf[byteOffset++] = 0;

    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], byteOffset, ENET_PACKET_FLAG_RELIABLE));

    Dbg_PacketSent(PACKET_PLAYER_SPAWN);
}




void Net_WaitForServer(void)
{
    int32_t serverReady = g_player[0].pingcnt;

    if (numplayers < 2 || g_netServer)
        return;

    P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, 8 + 2 + 1);

    do
    {
        if (quitevent || keystatus[1])
            G_GameExit("");

        if (G_FPSLimit())
        {
            display_betascreen();
            gametext_center_shade(170, "Waiting for server", 14);
            videoNextPage();
        }

        // XXX: this looks like something that should be rate limited...
        packbuf[0] = PACKET_PLAYER_PING;
        packbuf[1] = myconnectindex;

        if (g_netClientPeer)
        {
            enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], 2, ENET_PACKET_FLAG_RELIABLE));
            Dbg_PacketSent(PACKET_PLAYER_PING);
        }


        G_HandleAsync();

        if (g_player[0].pingcnt > serverReady)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8 + 2 + 1);
            return;
        }
    } while (1);
}

void Net_ResetPrediction(void)
{
    NET_75_CHECK++;  // this function is probably not needed
}

void Net_Connect(const char *srvaddr)
{
    ENetAddress address;
    ENetEvent   event;
    char *      addrstr = NULL;

    const int32_t cNumberOfRetries = 4;

    int32_t connectCount = cNumberOfRetries;

    char *oursrvaddr = Xstrdup(srvaddr);

    Net_Disconnect();

    g_netClient = enet_host_create(NULL, 1, CHAN_MAX, 0, 0);

    if (g_netClient == NULL)
    {
        initprintf("An error occurred while trying to create an ENet client host.\n");
        return;
    }

    addrstr = strtok(oursrvaddr, ":");
    enet_address_set_host(&address, addrstr);
    addrstr      = strtok(NULL, ":");
    address.port = addrstr == NULL ? g_netPort : Batoi(addrstr);

    g_netClientPeer = enet_host_connect(g_netClient, &address, CHAN_MAX, 0);

    if (g_netClientPeer == NULL)
    {
        initprintf("No available peers for initiating an ENet connection.\n");
        return;
    }

    for (connectCount = cNumberOfRetries; connectCount > 0; connectCount--)
    {
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        if (enet_host_service(g_netClient, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            initprintf("Connection to %s:%d succeeded.\n", oursrvaddr, address.port);
            Xfree(oursrvaddr);
            return;
        }
        else
        {
            /* Either the 5 seconds are up or a disconnect event was */
            /* received. Reset the peer in the event the 5 seconds   */
            /* had run out without any significant event.            */
            enet_peer_reset(g_netClientPeer);
            initprintf("Connection to %s:%d failed.\n", oursrvaddr, address.port);
        }
        initprintf(connectCount ? "Retrying...\n" : "Giving up connection attempt.\n");
    }

    // [75] note: it only gets here if there was an error
    Xfree(oursrvaddr);
    Net_Disconnect();
}

///< summary>
/// Record the current state of the game arrays to the initial snapshot
///</summary>
void Net_AddWorldToInitialSnapshot()
{
    Net_AddWorldToSnapshot(&g_mapStartState);
}

void Net_SendClientInfo(void)
{
    int32_t i, l;

    for (l = 0; (unsigned)l < sizeof(szPlayerName) - 1; l++) g_player[myconnectindex].user_name[l] = szPlayerName[l];

    if (numplayers < 2)
        return;

    tempnetbuf[0] = PACKET_CLIENT_INFO;
    l             = 1;

    // null terminated player name to send
    for (i = 0; szPlayerName[i]; i++)
    {
        tempnetbuf[l++] = szPlayerName[i];
    }
    tempnetbuf[l++] = 0;

    tempnetbuf[l++] = g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    tempnetbuf[l++] = g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    tempnetbuf[l++] = g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    tempnetbuf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

    tempnetbuf[l++] = g_player[myconnectindex].pteam = ud.team;

    for (i = 0; i < 10; i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        tempnetbuf[l++]                     = (uint8_t)g_player[0].wchoice[i];
    }

    tempnetbuf[l++] = myconnectindex;

    Dbg_PacketSent(PACKET_CLIENT_INFO);

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&tempnetbuf[0], l, ENET_PACKET_FLAG_RELIABLE));
    }
}

void Net_SendUserMapName(void)
{

    if (numplayers < 2)
        return;

    packbuf[0] = PACKET_USER_MAP;

    Bcorrectfilename(boardfilename, 0);

    // user map name is sent with a NUL at the end

    int32_t j = Bstrlen(boardfilename) + 1;
    Bmemcpy(&packbuf[1], boardfilename, j);
    j++;

    packbuf[j++] = myconnectindex;

    Dbg_PacketSent(PACKET_USER_MAP);

    if (g_netClient)
    {
        enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], j, ENET_PACKET_FLAG_RELIABLE));
    }
    else if (g_netServer)
    {
        enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(&packbuf[0], j, ENET_PACKET_FLAG_RELIABLE));
    }
}


// Insert a sprite from STAT_NETALLOC, and add it to the scratch pad list if we're a client.
int32_t Net_InsertSprite(int32_t sect, int32_t stat)
{
    if ((!g_netServer) && (!g_netClient))
    {
        return insertsprite(sect, stat);
    }

    int32_t i = Net_DoInsertSprite(sect, stat);

    if (i < 0)
    {
        return i;
    }

    if (g_netClient)
    {
        Net_InsertScratchPadSprite(i);
    }

    return i;
}

void Net_DeleteSprite(int32_t spritenum)
{
    // [75] later on, real clientside sprites (not scratchpad or server side) can be just deleted
    //      using deletesprite without worrying about linked lists or sprite indexes going out of sync.

    // [75] for most cases, only the server deletes sprites, clients just set their sprites invisible and to
    //      the null picnum so no CON gets executed

    if ((!g_netServer) && (!g_netClient))
    {
        deletesprite(spritenum);
        return;
    }

    if (g_netClient)
    {
        sprite[spritenum].cstat  = 32768;
        sprite[spritenum].picnum = 0;
        return;
    }

    Net_DoDeleteSprite(spritenum);
}


// Send various player related updates (server -> client)
void Net_SendServerUpdates(void)
{
    int16_t              i;
    uint8_t *            updatebuf;
    serverupdate_t       serverupdate;
    serverplayerupdate_t playerupdate;
    input_t *            osyn = (input_t *)&inputfifo[1][0];
    input_t *            nsyn = (input_t *)&inputfifo[0][0];

    ticrandomseed = randomseed;

    if (g_netServer)
    {
        Bmemcpy(&osyn[0], &nsyn[0], sizeof(input_t));
    }

    if (!g_netServer || numplayers < 2)
    {
        return;
    }

    serverupdate.header   = PACKET_MASTER_TO_SLAVE;
    serverupdate.seed     = ticrandomseed;
    serverupdate.nsyn     = *nsyn;
    serverupdate.pause_on = ud.pause_on;

    serverupdate.numplayers = 0;
    updatebuf               = tempnetbuf + sizeof(serverupdate_t);

    for (TRAVERSE_CONNECT(i))
    {
        if (g_player[i].playerquitflag == 0)
        {
            continue;
        }

        Net_FillPlayerUpdate(&playerupdate.player, i);

        playerupdate.gotweapon    = g_player[i].ps->gotweapon;
        playerupdate.kickback_pic = g_player[i].ps->kickback_pic;
        Bmemcpy(playerupdate.frags, g_player[i].frags, sizeof(playerupdate.frags));
        Bmemcpy(playerupdate.inv_amount, g_player[i].ps->inv_amount, sizeof(playerupdate.inv_amount));
        Bmemcpy(playerupdate.ammo_amount, g_player[i].ps->ammo_amount, sizeof(playerupdate.ammo_amount));

        playerupdate.curr_weapon    = g_player[i].ps->curr_weapon;
        playerupdate.last_weapon    = g_player[i].ps->last_weapon;
        playerupdate.wantweaponfire = g_player[i].ps->wantweaponfire;
        playerupdate.weapon_pos     = g_player[i].ps->weapon_pos;
        playerupdate.frag_ps        = g_player[i].ps->frag_ps;
        playerupdate.frag           = g_player[i].ps->frag;
        playerupdate.fraggedself    = g_player[i].ps->fraggedself;
        playerupdate.last_extra     = g_player[i].ps->last_extra;
        playerupdate.ping           = g_player[i].ping;
        playerupdate.newowner       = g_player[i].ps->newowner;

        Bmemcpy(updatebuf, &playerupdate, sizeof(serverplayerupdate_t));
        updatebuf += sizeof(serverplayerupdate_t);
        serverupdate.numplayers++;
    }

    if (serverupdate.numplayers == 0)
    {
        return;
    }

    Bmemcpy(tempnetbuf, &serverupdate, sizeof(serverupdate_t));

    enet_host_broadcast(
    g_netServer, CHAN_MOVE,
    enet_packet_create(&tempnetbuf[0], sizeof(serverupdate_t) + (serverupdate.numplayers * sizeof(serverplayerupdate_t)), 0));

    Dbg_PacketSent(PACKET_MASTER_TO_SLAVE);
}

void Net_SendClientUpdate(void)
{
    clientupdate_t update;
    update.header         = PACKET_SLAVE_TO_MASTER;
    update.RevisionNumber = g_netMapRevisionNumber;
    update.nsyn           = inputfifo[0][myconnectindex];

    Net_FillPlayerUpdate(&update.player, myconnectindex);

    enet_peer_send(g_netClientPeer, CHAN_MOVE, enet_packet_create(&update, sizeof(clientupdate_t), 0));

    Dbg_PacketSent(PACKET_SLAVE_TO_MASTER);
}


void Net_SendMessage(void)
{
    if (g_player[myconnectindex].ps->gm & MODE_SENDTOWHOM)
    {
        int32_t i, j;
        if (g_chatPlayer != -1 || ud.multimode < 3)
        {
            tempbuf[0] = PACKET_MESSAGE;
            tempbuf[2] = 0;
            recbuf[0]  = 0;

            if (ud.multimode < 3)
                g_chatPlayer = 2;

            if (typebuf[0] == '/' && Btoupper(typebuf[1]) == 'M' && Btoupper(typebuf[2]) == 'E')
            {
                Bstrcat(recbuf, "* ");
                i = 3, j = Bstrlen(typebuf);
                Bstrcpy(tempbuf, typebuf);
                while (i < j)
                {
                    typebuf[i - 3] = tempbuf[i];
                    i++;
                }
                typebuf[i - 3] = '\0';
                Bstrcat(recbuf, g_player[myconnectindex].user_name);
            }
            else
            {
                Bstrcat(recbuf, g_player[myconnectindex].user_name);
                Bstrcat(recbuf, ": ");
            }

            Bstrcat(recbuf, "^00");
            Bstrcat(recbuf, typebuf);
            j         = Bstrlen(recbuf);
            recbuf[j] = 0;
            Bstrcat(tempbuf + 2, recbuf);

            if (g_chatPlayer >= ud.multimode)
            {
                tempbuf[1]     = 255;
                tempbuf[j + 2] = myconnectindex;
                j++;
                if (g_netServer)
                    enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(&tempbuf[0], j + 2, 0));
                else if (g_netClient)
                    enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(&tempbuf[0], j + 2, 0));
                G_AddUserQuote(recbuf);
            }
            g_chatPlayer = -1;
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE | MODE_SENDTOWHOM);
        }
        else if (g_chatPlayer == -1)
        {
            j = 50;
            gametext_center(j, "Send message to...");
            j += 8;
            for (TRAVERSE_CONNECT(i))
            {
                if (i == myconnectindex)
                {
                    minitextshade((320 >> 1) - 40 + 1, j + 1, "A/ENTER - ALL", 26, 0, 2 + 8 + 16);
                    minitext((320 >> 1) - 40, j, "A/ENTER - ALL", 0, 2 + 8 + 16);
                    j += 7;
                }
                else
                {
                    Bsprintf(recbuf, "      %d - %s", i + 1, g_player[i].user_name);
                    minitextshade((320 >> 1) - 40 - 6 + 1, j + 1, recbuf, 26, 0, 2 + 8 + 16);
                    minitext((320 >> 1) - 40 - 6, j, recbuf, 0, 2 + 8 + 16);
                    j += 7;
                }
            }
            minitextshade((320 >> 1) - 40 - 4 + 1, j + 1, "    ESC - Abort", 26, 0, 2 + 8 + 16);
            minitext((320 >> 1) - 40 - 4, j, "    ESC - Abort", 0, 2 + 8 + 16);
            j += 7;

            mpgametext(mpgametext_x, ud.screen_size > 0 ? (200 - 45) << 16 : (200 - 8) << 16, typebuf, 0, 0, 0, 0);

            if (KB_KeyWaiting())
            {
                i = KB_GetCh();

                if (i == 'A' || i == 'a' || i == 13)
                    g_chatPlayer = ud.multimode;
                else if (i >= '1' || i <= (ud.multimode + '1'))
                    g_chatPlayer = i - '1';
                else
                {
                    g_chatPlayer = ud.multimode;
                    if (i == 27)
                    {
                        g_player[myconnectindex].ps->gm &= ~(MODE_TYPE | MODE_SENDTOWHOM);
                        g_chatPlayer = -1;
                    }
                    else
                        typebuf[0] = 0;
                }

                KB_ClearKeyDown(sc_1);
                KB_ClearKeyDown(sc_2);
                KB_ClearKeyDown(sc_3);
                KB_ClearKeyDown(sc_4);
                KB_ClearKeyDown(sc_5);
                KB_ClearKeyDown(sc_6);
                KB_ClearKeyDown(sc_7);
                KB_ClearKeyDown(sc_8);
                KB_ClearKeyDown(sc_A);
                KB_ClearKeyDown(sc_Escape);
                KB_ClearKeyDown(sc_Enter);
            }
        }
    }
    else
    {
        int32_t const hitstate = I_EnterText(typebuf, 120, 0);

        int32_t const y = ud.screen_size > 1 ? (200 - 58) << 16 : (200 - 35) << 16;

        int32_t const width     = mpgametextsize(typebuf, TEXT_LITERALESCAPE).x;
        int32_t const fullwidth = width + textsc((tilesiz[SPINNINGNUKEICON].x << 15) + (2 << 16));
        int32_t const text_x    = fullwidth >= (320 << 16) ? (320 << 16) - fullwidth : mpgametext_x;
        mpgametext(text_x, y, typebuf, 1, 2 | 8 | 16 | ROTATESPRITE_FULL16, 0, TEXT_YCENTER | TEXT_LITERALESCAPE);
        int32_t const cursor_x = text_x + width + textsc((tilesiz[SPINNINGNUKEICON].x << 14) + (1 << 16));
        rotatesprite_fs(cursor_x, y, textsc(32768), 0, SPINNINGNUKEICON + (((int32_t) totalclock >> 3) % 7),
                        4 - (sintable[((int32_t) totalclock << 4) & 2047] >> 11), 0, 2 | 8);

        if (hitstate == 1)
        {
            KB_ClearKeyDown(sc_Enter);
            if (Bstrlen(typebuf) == 0)
            {
                g_player[myconnectindex].ps->gm &= ~(MODE_TYPE | MODE_SENDTOWHOM);
                return;
            }
            if (ud.automsg)
            {
                if (SHIFTS_IS_PRESSED)
                    g_chatPlayer = -1;
                else
                    g_chatPlayer = ud.multimode;
            }
            g_player[myconnectindex].ps->gm |= MODE_SENDTOWHOM;
        }
        else if (hitstate == -1)
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE | MODE_SENDTOWHOM);
        else
            pub = NUMPAGES;
    }
}


void Net_InitMapStateHistory()
{
    int32_t mapStateIndex = 0;

    for (mapStateIndex = 0; mapStateIndex < NET_REVISIONS; mapStateIndex++)
    {
        netmapstate_t *mapState = &g_mapStateHistory[mapStateIndex];
        netmapstate_t *clState  = &g_cl_InterpolatedMapStateHistory[mapStateIndex];


        Net_InitMapState(mapState);
        Net_InitMapState(clState);
    }

    Net_InitMapState(&g_mapStartState);

    g_mapStartState.revisionNumber = cInitialMapStateRevisionNumber;

    g_netMapRevisionNumber    = cInitialMapStateRevisionNumber;  // Net_InitMapStateHistory()
    g_cl_InterpolatedRevision = cInitialMapStateRevisionNumber;
}

void Net_StartNewGame()
{
    Net_ResetPlayers();

    Net_ExtractNewGame(&pendingnewgame, 0);
    G_NewGame(ud.volume_number, ud.level_number, ud.player_skill);
    ud.coop = ud.m_coop;

    if (G_EnterLevel(MODE_GAME))
    {
        G_BackToMenu();
    }
}


void Net_NotifyNewGame()
{
    int32_t spriteIndex;
    int32_t statIndex;
    int32_t numSprites           = 0;
    int32_t numSpritesToNetAlloc = 0;

    if (!g_netServer && !g_netClient)
    {
        return;
    }

    // Grab the total number of sprites at level load
    for (statIndex = 0; statIndex < MAXSTATUS; ++statIndex)
    {
        spriteIndex = headspritestat[statIndex];
        for (; spriteIndex >= 0; spriteIndex = nextspritestat[spriteIndex])
        {
            numSprites++;
        }
    }

    // Take half of the leftover sprites and allocate them for the network's nefarious purposes.
    numSpritesToNetAlloc = (MAXSPRITES - numSprites) / 2;
    for (spriteIndex = 0; spriteIndex < numSpritesToNetAlloc; ++spriteIndex)
    {
        int32_t newSprite         = insertspritestat(STAT_NETALLOC);
        sprite[newSprite].sectnum = MAXSECTORS;
        Numsprites++;
    }

    //[75] Note:    DON'T set the initial map state or initialize the map state history in the packet code,
    //              The client didn't load the map until G_EnterLevel
}

#endif

//-------------------------------------------------------------------------------------------------
