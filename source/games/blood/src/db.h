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

BEGIN_BLD_NS


enum
{
	kMaxXSprites = 16384,
	kMaxXWalls = 512,
	kMaxXSectors = 512
};

enum
{
	kAttrMove = 0x0001, // is affected by movement physics
	kAttrGravity = 0x0002, // is affected by gravity
	kAttrFalling = 0x0004, // in z motion
	kAttrAiming = 0x0008,
	kAttrRespawn = 0x0010,
	kAttrFree = 0x0020,
	kAttrSmoke = 0x0100, // receives tsprite smoke/steam 
	};


// by NoOne: functions to quckly check range of specifical arrays
inline bool xspriRangeIsFine(int nXindex) {
    return (nXindex >= 0 && nXindex < kMaxXSprites);
}

inline bool xsectRangeIsFine(int nXindex) {
    return (nXindex >= 0 && nXindex < kMaxXSectors);
}

inline bool xwallRangeIsFine(int nXindex) {
    return (nXindex >= 0 && nXindex < kMaxXWalls);
}
#pragma pack(push, 1)

struct AISTATE;

struct XSPRITE {
    
    AISTATE* aiState;                   // ai
    union
    {
        uint32_t flags;
        struct {
            unsigned int state : 1;             // State 0
            unsigned int triggerOn : 1;         // going ON
            unsigned int triggerOff : 1;        // going OFF
            unsigned int restState : 1;         // restState
            unsigned int Interrutable : 1;      // Interruptable
            unsigned int Decoupled : 1;         // Decoupled
            unsigned int triggerOnce : 1;       // 1-shot
            unsigned int isTriggered : 1;       // works in case if triggerOnce selected
            unsigned int Push : 1;              // Push
            unsigned int Vector : 1;            // Vector
            unsigned int Impact : 1;            // Impact
            unsigned int Pickup : 1;            // Pickup
            unsigned int Touch : 1;             // Touch
            unsigned int Sight : 1;             // Sight
            unsigned int Proximity : 1;         // Proximity
            unsigned int lS : 1;                // Single
            unsigned int lB : 1;                // Bloodbath
            unsigned int lT : 1;                // Launch Team
            unsigned int lC : 1;                // Coop
            unsigned int DudeLockout : 1;       // DudeLockout
            unsigned int locked : 1;            // Locked
            unsigned int dudeDeaf : 1;          // dudeDeaf
            unsigned int dudeAmbush : 1;        // dudeAmbush
            unsigned int dudeGuard : 1;         // dudeGuard
            unsigned int dudeFlag4 : 1;         // unused
            unsigned int wave : 2;              // Wave
            unsigned int medium : 2;            // medium
            unsigned int respawn : 2;           // Respawn option
            unsigned int unused2 : 1;           // (new) patrol state
       };
    };
    int32_t targetX;          // target x
    int32_t targetY;          // target y
    int32_t targetZ;          // target z
    int32_t sysData1;            // used to keep here various system data, so user can't change it in map editor
    int32_t sysData2;            //
    int32_t scale;                   // used for scaling SEQ size on sprites
    uint32_t physAttr;         // currently used by additional physics sprites to keep it's attributes.
    uint32_t health;
    uint32_t busy;

    int16_t reference;
    int16_t data1;            // Data 1
    int16_t data2;            // Data 2
    int16_t data3;            // Data 3
    int16_t target;           // target sprite
    int16_t burnSource;
    uint16_t txID;             // TX ID
    uint16_t rxID;             // RX ID
    uint16_t command;           // Cmd
    uint16_t busyTime;         // busyTime
    uint16_t waitTime;         // waitTime
    uint16_t data4;            // Data 4
    uint16_t goalAng;          // Dude goal ang
    uint16_t burnTime;
    uint16_t height;
    uint16_t stateTimer;       // ai timer

    uint8_t respawnPending;    // respawnPending
    uint8_t dropMsg;           // Drop Item
    uint8_t key;               // Key
    uint8_t lSkill;            // Launch 12345
    uint8_t lockMsg;           // Lock msg
    int8_t dodgeDir;          // Dude dodge direction
    uint8_t unused1;            // modern flags
    uint8_t unused3;           // something about sight checks
    uint8_t unused4;           // patrol turn delay

};

struct XSECTOR {
 
    union
    {
        uint64_t flags;
        struct {
            unsigned int state : 1;             // State
            unsigned int triggerOn : 1;         // Send at ON
            unsigned int triggerOff : 1;        // Send at OFF
            unsigned int restState : 1;
            unsigned int interruptable : 1;     // Interruptable
            unsigned int reTriggerA : 1;        // OFF->ON wait
            unsigned int reTriggerB : 1;        // ON->OFF wait
            unsigned int shadeAlways : 1;       // Lighting shadeAlways
            unsigned int shadeFloor : 1;        // Lighting floor
            unsigned int shadeCeiling : 1;      // Lighting ceiling
            unsigned int shadeWalls : 1;        // Lighting walls
            unsigned int panAlways : 1;         // Pan always
            unsigned int panFloor : 1;          // Pan floor
            unsigned int panCeiling : 1;        // Pan ceiling
            unsigned int Drag : 1;              // Pan drag
            unsigned int Underwater : 1;        // Underwater
            unsigned int decoupled : 1;         // Decoupled
            unsigned int triggerOnce : 1;       // 1-shot
            unsigned int isTriggered : 1;
            unsigned int Push : 1;              // Push
            unsigned int Vector : 1;            // Vector
            unsigned int Reserved : 1;          // Reserved
            unsigned int Enter : 1;             // Enter
            unsigned int Exit : 1;              // Exit
            unsigned int Wallpush : 1;          // WallPush
            unsigned int color : 1;             // Color Lights
            unsigned int stopOn : 1;
            unsigned int stopOff : 1;
            unsigned int Crush : 1;             // Crush
            unsigned int locked : 1;            // Locked
            unsigned int windAlways : 1;        // Wind always
            unsigned int dudeLockout : 1;
            unsigned int bobAlways : 1;         // Motion always
            unsigned int bobFloor : 1;          // Motion bob floor
            unsigned int bobCeiling : 1;        // Motion bob ceiling
            unsigned int bobRotate : 1;         // Motion rotate
            unsigned int unused1 : 1;           // (new) pause motion
            
        };
    };
    
    uint32_t busy;
    int32_t offCeilZ;
    int32_t onCeilZ;
    int32_t offFloorZ;
    int32_t onFloorZ;
    uint32_t windVel;          // Wind vel (changed from 10 bit to use higher velocity values)

    uint16_t reference;
    uint16_t data;             // Data
    uint16_t txID;             // TX ID
    uint16_t rxID;             // RX ID
    uint16_t busyTimeA;        // OFF->ON busyTime
    uint16_t waitTimeA;        // OFF->ON waitTime
    uint16_t panAngle;         // Motion angle
    uint16_t busyTimeB;        // ON->OFF busyTime
    uint16_t waitTimeB;        // ON->OFF waitTime
    uint16_t marker0;
    uint16_t marker1;
    uint16_t windAng;          // Wind ang
    uint16_t bobTheta;         // Motion Theta
    int16_t bobSpeed;           // Motion speed

    uint8_t busyWaveA;         // OFF->ON wave
    uint8_t busyWaveB;         // ON->OFF wave
    uint8_t command;           // Cmd
    int8_t amplitude;           // Lighting amplitude
    uint8_t freq;              // Lighting freq
    uint8_t phase;             // Lighting phase
    uint8_t wave;              // Lighting wave
    int8_t shade;               // Lighting value
    uint8_t panVel;            // Motion speed
    uint8_t Depth;             // Depth
    uint8_t Key;               // Key
    uint8_t ceilpal;           // Ceil pal2
    uint8_t damageType;        // DamageType
    uint8_t floorpal;          // Floor pal2
    uint8_t bobZRange;         // Motion Z range
};

struct XWALL {
    
    union
    {
        uint32_t flags;
        struct {
            unsigned int state : 1;             // State
            unsigned int triggerOn : 1;         // going ON
            unsigned int triggerOff : 1;        // going OFF
            unsigned int restState : 1;         // restState
            unsigned int interruptable : 1;     // Interruptable
            unsigned int panAlways : 1;         // panAlways
            unsigned int decoupled : 1;         // Decoupled
            unsigned int triggerOnce : 1;       // 1-shot
            unsigned int isTriggered : 1;
            unsigned int triggerPush : 1;       // Push
            unsigned int triggerVector : 1;     // Vector
            unsigned int triggerTouch : 1;      // by NoOne: renamed from Reserved to Touch as it works with Touch now.
            unsigned int locked : 1;            // Locked
            unsigned int dudeLockout : 1;       // DudeLockout
        };
    };
    uint32_t busy;

    int16_t reference;
    int16_t data;               // Data
    uint16_t txID;             // TX ID
    uint16_t rxID;             // RX ID
    uint16_t busyTime;         // busyTime
    uint16_t waitTime;         // waitTime
    
    uint8_t command;           // Cmd
    int8_t panXVel;           // panX
    int8_t panYVel;           // panY
    uint8_t key;               // Key
};

struct MAPSIGNATURE {
    char signature[4];
    short version;
};

struct MAPHEADER  {
    int x; // x
    int y; // y
    int z; // z
    short ang; // ang
    short sect; // sect
    short pskybits; // pskybits
    int visibility; // visibility
    int mattid; // song id, Matt
    char parallax; // parallaxtype
    int revision; // map revision
    short numsectors; // numsectors
    short numwalls; // numwalls
    short numsprites; // numsprites
};

struct MAPHEADER2 {
    char name[64];
    int numxsprites; // xsprite size
    int numxwalls; // xwall size
    int numxsectors; // xsector size
    char pad[52];
};

struct SPRITEHIT 
{
    int hit, ceilhit, florhit;
};

#pragma pack(pop)

extern unsigned short gStatCount[kMaxStatus + 1];;

extern bool drawtile2048, encrypted;
extern MAPHEADER2 byte_19AE44;

extern XSPRITE xsprite[kMaxXSprites];
extern XSECTOR xsector[kMaxXSectors];
extern XWALL xwall[kMaxXWalls];

extern FixedBitArray<MAXSPRITES> activeXSprites;

extern SPRITEHIT gSpriteHit[kMaxXSprites];

extern char qsector_filler[kMaxSectors];

extern int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

extern int gVisibility;
extern int gMapRev, gMattId, gSkyCount;
extern const char *gItemText[];
extern const char *gAmmoText[];
extern const char *gWeaponText[];

extern unsigned short nextXSprite[kMaxXSprites];
extern int XWallsUsed, XSectorsUsed;

static inline int GetWallType(int nWall)
{
    return wall[nWall].type;
}

template<typename T> void GetSpriteExtents(T const * const pSprite, int *top, int *bottom)
{
    *top = *bottom = pSprite->z;
    if ((pSprite->cstat & 0x30) != 0x20)
    {
        int height = tileHeight(pSprite->picnum);
        int center = height / 2 + tileTopOffset(pSprite->picnum);
        *top -= (pSprite->yrepeat << 2)*center;
        *bottom += (pSprite->yrepeat << 2)*(height - center);
    }
}

#ifdef POLYMER
#pragma pack(push, 1)
struct PolymerLight_t {
    int16_t lightId, lightmaxrange;
    _prlight* lightptr;
    uint8_t lightcount;
};
#pragma pack(pop)

extern PolymerLight_t gPolymerLight[kMaxSprites];

void DeleteLight(int32_t s);

#endif

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
void InitFreeList(unsigned short* pList, int nCount, FixedBitArray<MAXSPRITES>& activeXSprites);
void InsertFree(unsigned short *pList, int nIndex);
unsigned short dbInsertXSprite(int nSprite);
void dbDeleteXSprite(int nXSprite);
unsigned short dbInsertXWall(int nWall);
unsigned short dbInsertXSector(int nSector);
void dbInit(void);
void PropagateMarkerReferences(void);
unsigned int dbReadMapCRC(const char *pPath);
void dbLoadMap(const char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector, unsigned int *pCRC);

END_BLD_NS
