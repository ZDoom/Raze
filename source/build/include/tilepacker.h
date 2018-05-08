/*
 * tilepacker.h
 *  A k-d tree based bin packer that organizes rectangular tiles to fit
 *  neatly into one texture.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#ifndef TILEPACKER_H_
#define TILEPACKER_H_

#define MAXTILESHEETS 64
#define MAXPACKEDTILES MAXTILES+1

typedef struct
{
    uint32_t u, v, width, height;
} TileRect;

typedef struct
{
    uint32_t tilesheetID;
    TileRect rect;
} Tile;

// Initialize the specified tilesheet
// tilesheetId must be less than MAXTILESHEETS
// Re-initializing an existing tilesheet will discard all of its contents
void tilepacker_initTilesheet(uint32_t tilesheetID, uint32_t tilesheetWidth, uint32_t tilesheetHeight);

// Adds a tile into sorted collection to be packed
// uid can be any unique key -- picnums are used for indexed colour textures
void tilepacker_addTile(uint32_t tileUID, uint32_t tileWidth, uint32_t tileHeight);

// Packs the sorted collection of tiles that have been added with tilepacker_addTile()
// Returns true if all nodes could be packed succesfully into the specified tilesheet
// tilepacker_pack can be called again with a different tilesheetId to pack the remaining tiles
char tilepacker_pack(uint32_t tilesheetID);

// Discard the rejected tiles so that they will not be carried over to the next call to tilepacker_pack
void tilepacker_discardRejects();

// Sets pOutput to contain the Tile information for a packed tile with the given tileUID
// Returns true if the Tile has been packed, false otherwise
// If pOutput is NULL, the function solely returns whether or not the Tile has been packed
char tilepacker_getTile(uint32_t tileUID, Tile *pOutput);

// Returns true if the Tile with tileUID has been packed, false otherwise
static inline char tilepacker_isTilePacked(uint32_t tileUID)
{
    return tilepacker_getTile(tileUID, NULL);
};

#endif /* TILEPACKER_H_ */
