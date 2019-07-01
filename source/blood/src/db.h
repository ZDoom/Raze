//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#pragma once

#define kMaxXSprites 2048
#define kMaxXWalls 512
#define kMaxXSectors 512

#pragma pack(push, 1)

struct AISTATE;

struct XSPRITE {
    //int at0;
    unsigned int atb_2 : 2; // unused	//
    unsigned int atb_6 : 1; // unused	// let's use these to add more data 
    unsigned int ate_5 : 2; // unused	// fields in the future? must be signed also
    unsigned int at1a_2 : 6; // unused	//

    signed   int reference : 14; // at0_0
    unsigned int state : 1;  // State 0
    unsigned int busy : 17;
    unsigned int txID : 10; // TX ID
    unsigned int rxID : 10; // RX ID
    unsigned int command : 8; // Cmd
    unsigned int triggerOn : 1; // going ON
    unsigned int triggerOff : 1; // going OFF
    unsigned int busyTime : 12; // busyTime
    unsigned int waitTime : 12; // waitTime
    unsigned int restState : 1; // restState
    unsigned int Interrutable : 1; // Interruptable

    unsigned int respawnPending : 2; // respawnPending

    signed int dropMsg : 10; // Drop Item
    unsigned int Decoupled : 1; // Decoupled
    unsigned int triggerOnce : 1; // 1-shot
    unsigned int isTriggered : 1; // works in case if triggerOnce selected

    unsigned int key : 3; // Key
    unsigned int wave : 2; // Wave
    unsigned int Push: 1; // Push
    unsigned int Vector : 1; // Vector
    unsigned int Impact : 1; // Impact
    unsigned int Pickup : 1; // Pickup
    unsigned int Touch : 1; // Touch
    unsigned int Sight : 1; // Sight
    unsigned int Proximity : 1; // Proximity
    unsigned int lSkill : 5; // Launch 12345
    unsigned int lS : 1; // Single
    unsigned int lB : 1; // Bloodbath
    unsigned int lT : 1; // Launch Team
    unsigned int lC : 1; // Coop
    unsigned int DudeLockout : 1; // DudeLockout
    signed   int data1 : 16; // Data 1
    signed   int data2 : 16; // Data 2
    signed   int data3 : 16; // Data 3
    unsigned int data4 : 16; // Data 4
    unsigned int locked : 1; // Locked
    unsigned int medium : 2; // medium
    unsigned int respawn : 2; // Respawn option
    unsigned int lockMsg : 8; // Lock msg
    unsigned int health : 20; // 1c_0
    unsigned int dudeDeaf : 1; // dudeDeaf
    unsigned int dudeAmbush : 1; // dudeAmbush
    unsigned int dudeGuard : 1; // dudeGuard
    unsigned int dudeFlag4 : 1; // DF reserved
    signed   int target : 16; // target sprite
    signed   int targetX : 32; // target x
    signed   int targetY : 32; // target y
    signed   int targetZ : 32; // target z
    unsigned int goalAng : 11; // Dude goal ang
    signed   int dodgeDir : 2; // Dude dodge direction
    unsigned int burnTime : 16;
    signed   int burnSource : 16;
    unsigned int height : 16;
    unsigned int stateTimer : 16; // ai timer
    AISTATE *aiState; // ai
    signed int txIndex : 10; // used by kGDXSequentialTX to keep current TX ID index
    signed int cumulDamage : 16; // for dudes
    signed int scale; // used for scaling SEQ size on sprites
};

struct XSECTOR {
    signed int reference : 14;
    unsigned int state : 1; // State 0
    unsigned int busy : 17;
    unsigned int data : 16; // Data
    unsigned int txID : 10; // TX ID
    unsigned int rxID : 10; // RX ID
    unsigned int at7_2 : 3; // OFF->ON wave
    unsigned int at7_5 : 3; // ON->OFF wave

    unsigned int command : 8; // Cmd 0
    unsigned int triggerOn : 1; // Send at ON
    unsigned int triggerOff : 1; // Send at OFF
    unsigned int busyTimeA : 12; // OFF->ON busyTime
    unsigned int waitTimeA : 12; // OFF->ON waitTime
    unsigned int atd_4 : 1;
    unsigned int interruptable : 1; // Interruptable

    unsigned int atf_6 : 1; // OFF->ON wait
    unsigned int atf_7 : 1; // ON->OFF wait
    signed int amplitude : 8; // Lighting amplitude
    unsigned int freq : 8; // Lighting freq
    unsigned int phase : 8; // Lighting phase
    unsigned int wave : 4; // Lighting wave
    unsigned int shadeAlways : 1; // Lighting shadeAlways
    unsigned int shadeFloor : 1; // Lighting floor
    unsigned int shadeCeiling : 1; // Lighting ceiling
    unsigned int shadeWalls : 1; // Lighting walls
    signed int shade : 8; // Lighting value
    unsigned int panAlways : 1; // Pan always
    unsigned int panFloor : 1; // Pan floor
    unsigned int panCeiling : 1; // Pan ceiling
    unsigned int Drag : 1; // Pan drag
    unsigned int panVel : 8; // Motion speed
    unsigned int panAngle : 11; // Motion angle
    unsigned int Underwater : 1; // Underwater
    unsigned int Depth : 3; // Depth
    unsigned int at16_3 : 1;
    unsigned int decoupled : 1; // Decoupled
    unsigned int triggerOnce : 1; // 1-shot
    unsigned int at16_6 : 1;
    unsigned int Key : 3; // Key
    unsigned int Push : 1; // Push
    unsigned int Vector : 1; // Vector
    unsigned int Reserved : 1; // Reserved
    unsigned int Enter : 1; // Enter
    unsigned int Exit : 1; // Exit
    unsigned int Wallpush : 1; // WallPush
    unsigned int color : 1; // Color Lights
    unsigned int at18_1 : 1;
    unsigned int busyTimeB : 12; // ON->OFF busyTime
    unsigned int waitTimeB : 12; // ON->OFF waitTime
    unsigned int at1b_2 : 1;
    unsigned int at1b_3 : 1;
    unsigned int ceilpal : 4; // Ceil pal2
    signed int at1c_0 : 32;
    signed int at20_0 : 32;
    signed int at24_0 : 32;
    signed int at28_0 : 32;
    unsigned int at2c_0 : 16;
    unsigned int at2e_0 : 16;
    unsigned int Crush : 1; // Crush
    unsigned int at30_1 : 8; // Ceiling x panning frac
    unsigned int at31_1 : 8; // Ceiling y panning frac
    unsigned int at32_1 : 8; // Floor x panning frac
    unsigned int damageType : 3; // DamageType
    unsigned int floorpal : 4; // Floor pal2
    unsigned int at34_0 : 8; // Floor y panning frac
    unsigned int locked : 1; // Locked
    unsigned int windVel; // Wind vel (by NoOne: changed from 10 bit to use higher velocity values)
    unsigned int windAng : 11; // Wind ang
    unsigned int windAlways : 1; // Wind always
    unsigned int at37_7 : 1;
    unsigned int bobTheta : 11; // Motion Theta
    unsigned int bobZRange : 5; // Motion Z range
    signed int bobSpeed : 12; // Motion speed
    unsigned int bobAlways : 1; // Motion always
    unsigned int bobFloor : 1; // Motion bob floor
    unsigned int bobCeiling : 1; // Motion bob ceiling
    unsigned int bobRotate : 1; // Motion rotate
}; // 60(0x3c) bytes

struct XWALL {
    signed int reference : 14;
    unsigned int state : 1; // State
    unsigned int busy : 17;
    signed int data : 16; // Data
    unsigned int txID : 10; // TX ID
    unsigned int at7_2 : 6; // unused
    unsigned int rxID : 10; // RX ID
    unsigned int command : 8; // Cmd
    unsigned int triggerOn : 1; // going ON
    unsigned int triggerOff : 1; // going OFF
    unsigned int busyTime : 12; // busyTime
    unsigned int waitTime : 12; // waitTime
    unsigned int restState : 1; // restState
    unsigned int interruptable : 1; // Interruptable
    unsigned int panAlways : 1; // panAlways
    signed   int panXVel : 8; // panX
    signed   int panYVel : 8; // panY
    unsigned int decoupled : 1; // Decoupled
    unsigned int triggerOnce : 1; // 1-shot
    unsigned int isTriggered : 1;
    unsigned int key : 3; // Key 
    unsigned int triggerPush : 1; // Push
    unsigned int triggerVector : 1; // Vector
    unsigned int triggerReserved : 1; // Reserved
    unsigned int at11_0 : 2; // unused
    unsigned int xpanFrac : 8; // x panning frac
    unsigned int ypanFrac : 8; // y panning frac
    unsigned int locked : 1; // Locked
    unsigned int dudeLockout : 1; // DudeLockout
    unsigned int at13_4 : 4; // unused;
    unsigned int at14_0 : 32; // unused
}; // 24(0x18) bytes

struct MAPSIGNATURE {
    char signature[4];
    short version;
};

struct MAPHEADER  {
    int at0; // x
    int at4; // y
    int at8; // z
    short atc; // ang
    short ate; // sect
    short at10; // pskybits
    int at12; // visibility
    int at16; // song id, Matt
    char at1a; // parallaxtype
    int at1b; // map revision
    short at1f; // numsectors
    short at21; // numwalls
    short at23; // numsprites
};

struct MAPHEADER2 {
    char at0[64];
    int at40; // xsprite size
    int at44; // xwall size
    int at48; // xsector size
    char pad[52];
};

struct SPRITEHIT {
    int hit, ceilhit, florhit;
};

#pragma pack(pop)

extern unsigned short gStatCount[kMaxStatus + 1];;

extern bool byte_1A76C6, byte_1A76C7, byte_1A76C8;
extern MAPHEADER2 byte_19AE44;

extern XSPRITE xsprite[kMaxXSprites];
extern XSECTOR xsector[kMaxXSectors];
extern XWALL xwall[kMaxXWalls];

extern SPRITEHIT gSpriteHit[kMaxXSprites];

extern char qsprite_filler[kMaxSprites], qsector_filler[kMaxSectors];

extern int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

extern int gVisibility;
extern int gMapRev, gSongId, gSkyCount;
extern const char *gItemText[];
extern const char *gAmmoText[];
extern const char *gWeaponText[];

extern unsigned short nextXSprite[kMaxXSprites];
extern unsigned short nextXWall[kMaxXWalls];
extern unsigned short nextXSector[kMaxXSectors];

#ifdef YAX_ENABLE
static inline bool yax_hasnextwall(int nWall)
{
    return yax_getnextwall(nWall, YAX_CEILING) >= 0 || yax_getnextwall(nWall, YAX_FLOOR) >= 0;
}
#endif

static inline int GetWallType(int nWall)
{
#ifdef YAX_ENABLE
    if (yax_hasnextwall(nWall))
        return 0;
#endif
    return wall[nWall].lotag;
}

inline void GetSpriteExtents(spritetype *pSprite, int *top, int *bottom)
{
    *top = *bottom = pSprite->z;
    if ((pSprite->cstat & 0x30) != 0x20)
    {
        int height = tilesiz[pSprite->picnum].y;
        int center = height / 2 + picanm[pSprite->picnum].yofs;
        *top -= (pSprite->yrepeat << 2)*center;
        *bottom += (pSprite->yrepeat << 2)*(height - center);
    }
}

void InsertSpriteSect(int nSprite, int nSector);
void RemoveSpriteSect(int nSprite);
void InsertSpriteStat(int nSprite, int nStat);
void RemoveSpriteStat(int nSprite);
void qinitspritelists(void);
int InsertSprite(int nSector, int nStat);
int qinsertsprite(short nSector, short nStat);
int DeleteSprite(int nSprite);
int qdeletesprite(short nSprite);
int ChangeSpriteSect(int nSprite, int nSector);
int qchangespritesect(short nSprite, short nSector);
int ChangeSpriteStat(int nSprite, int nStatus);
int qchangespritestat(short nSprite, short nStatus);
void InitFreeList(unsigned short *pList, int nCount);
void InsertFree(unsigned short *pList, int nIndex);
unsigned short dbInsertXSprite(int nSprite);
void dbDeleteXSprite(int nXSprite);
unsigned short dbInsertXWall(int nWall);
void dbDeleteXWall(int nXWall);
unsigned short dbInsertXSector(int nSector);
void dbDeleteXSector(int nXSector);
void dbXSpriteClean(void);
void dbXWallClean(void);
void dbXSectorClean(void);
void dbInit(void);
void PropagateMarkerReferences(void);
unsigned int dbReadMapCRC(const char *pPath);
int dbLoadMap(const char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector, unsigned int *pCRC);
