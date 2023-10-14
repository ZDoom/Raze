//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

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


///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////

#include "build.h"
#include "common_game.h"
#include "seq.h"
#include "view.h"
#include "endgame.h"
//#include "aicdud.h"
//#include "mmulti.h"
#include "nnexts.h"
#include "nnextsif.h"
#include "texinfo.h"

BEGIN_BLD_NS

enum
{
    kPushRange = 3,
    kCmdPush = 64,
    kCmdPop = 100,
};


enum ENUM_EVENT_OBJECT {
EVOBJ_SPRITE                    = OBJ_SPRITE,
EVOBJ_SECTOR                    = OBJ_SECTOR,
EVOBJ_WALL                      = OBJ_WALL,
};

enum ENUM_CONDITION_ERROR {
kErrInvalidObject               = 0,
kErrInvalidSerial,
kErrUnknownObject,
kErrInvalidArgsPass,
kErrObjectUnsupp,
kErrNotImplementedCond,
};

enum ENUM_CONDITION_TYPE {
CNON                            = 0,
CGAM,
CMIX,
CWAL,
CSEC,
CPLY,
CDUDG,
CDUDC,
CSPR,
};

struct CHECKFUNC_INFO
{
    bool (*pFunc)( void );              // function to call the condition
    unsigned int type           : 5;	// type of condition
    const char* name;                   // for errors output
};

struct CONDITION_INFO
{
    bool (*pFunc)( void );              // condition function
    uint16_t id;	// condition id
    uint8_t type;	// type of condition
    bool isBool;    // false = do comparison using Cmp()
    bool xReq;      // is x-object required?
    CHECKFUNC_INFO* pCaller  = nullptr;	// provided for easy access and must be set during init
};

struct TRACKING_CONDITION
{
    unsigned int id             : 16;   // x-sprite index of condition
    TArray<EventObject> objects;               // a dynamic list of objects it contains
};

static const char* gErrors[] =
{
    "Object #%d (objType: %d) is not a %s!",
    "%d is not condition serial!",
    "Unknown object type %d, index %d.",
    "Invalid arguments passed.",
    "Unsupported %s type %d, extra: %d.",
    "Condition is not implemented.",
};

/** GLOBAL coditions table
********************************************************************************/
TArray<TRCONDITION> gConditions;
static unsigned int gNumConditions = 0;



/** TRACKING(LOOPED) coditions list
********************************************************************************/
static TRACKING_CONDITION* gTrackingConditionsList = nullptr;
static unsigned int gTrackingConditionsListLength = 0;



/** Variables that is relative to current condition
********************************************************************************/
static DBloodActor* pCond = nullptr;		// current condition
static int arg1 = 0, arg2 = 0, arg3 = 0;							// arguments of current condition (data2, data3, data4)
static int cmpOp = 0;												// current comparison operator (condition sprite cstat)
static bool PUSH = false;											// current stack push status (kCmdNumberic)
static CONDITION_INFO* pEntry = nullptr;								// current condition db entry
static EVENT* pEvent = nullptr;										// current event

static DBloodPlayer* pPlayer = nullptr;										// player in the focus
static int objType = -1, objIndex = -1;						        // object in the focus
static DBloodActor* pSpr = nullptr;
static walltype* pWall = nullptr;
static XWALL* pXWall = nullptr;
static sectortype* pSect = nullptr;
static XSECTOR* pXSect = nullptr;
static bool xAvail = false;											// x-object indicator

static TArray<FGameTexture*> texSeenToClear;
static TArray<sectortype*> sectorSeenToClear;


/** INTERFACE functions
********************************************************************************/
static void UnSerialize(int nSerial, int* oType, int* oIndex);
static bool Cmp(int val, int nArg1, int nArg2);
static void Push(EventObject ev);
inline void Push(DBloodActor* a) { Push(EventObject(a)); }
inline void Push(walltype* a) { Push(EventObject(a)); }
inline void Push(sectortype* a) { Push(EventObject(a)); }
static void TriggerObject(int nSerial);
static void Error(const char* pFormat, ...);
static void ReceiveObjects(EVENT* pFrom);
static bool Cmp(int val);
static bool Cmp(EventObject ob1, EventObject ob2, EventObject ob3);
static bool DefaultResult();
static void Restore();

static bool CheckCustomDude();
static bool CheckGeneric();
static bool CheckSector();
static bool CheckSprite();
static bool CheckObject();
static bool CheckPlayer();
static bool CheckWall();
static bool CheckDude();



/** A LIST OF CONDITION FUNCTION CALLERS
********************************************************************************/
static CHECKFUNC_INFO gCheckFuncInfo[] =
{
    { CheckGeneric,				CGAM,		"Game"			},
    { CheckObject,				CMIX,		"Mixed"			},
    { CheckWall,				CWAL,		"Wall"          },
    { CheckSector, 				CSEC,		"Sector"        },
    { CheckPlayer,				CPLY,		"Player"        },
    { CheckDude,				CDUDG,		"Dude"          },
    { CheckCustomDude,			CDUDC,		"Custom Dude"   },
    { CheckSprite,				CSPR,		"Sprite"        },
    { CheckGeneric,				CNON,		"Unknown"       },
};

static int qsSortCheckFuncInfo(CHECKFUNC_INFO* ref1, CHECKFUNC_INFO* ref2)
{
    return ref1->type - ref2->type;
}



/** ERROR functions
********************************************************************************/
static bool errCondNotImplemented(void)
{
    Error(gErrors[kErrNotImplementedCond]);
    return false;
}


/** HELPER functions
********************************************************************************/
static bool helperChkSprite(int nSprite)
{
#if 0
    // inverse lookup table does not exist yet.
    if (!rngok(nSprite, 0, kMaxSprites)) return false;
    else if (PUSH) Push(EVOBJ_SPRITE, nSprite);
    return true;
#else
    return false;
#endif
}

static bool helperChkSprite(DBloodActor* nSprite)
{
    // inverse lookup table does not exist yet.
    if (!nSprite) return false;
    else if (PUSH) Push(nSprite);
    return true;
}

static bool helperChkSector(int nSect)
{
    if ((unsigned)nSect >= sector.Size()) return false;
    else if (PUSH) Push(&sector[nSect]);
    return true;
}

static bool helperChkWall(int nWall)
{
    if ((unsigned)nWall >= wall.Size()) return false;
    else if (PUSH) Push(&wall[nWall]);
    return true;
}

static bool helperCmpHeight(char ceil)
{
    double nHeigh1, nHeigh2;
    switch (pSect->type)
    {
        case kSectorZMotion:
        case kSectorRotate:
        case kSectorSlide:
        case kSectorSlideMarked:
        case kSectorRotateMarked:
        case kSectorRotateStep:
            if (ceil)
            {
                nHeigh1 = std::max(abs(pXSect->onCeilZ - pXSect->offCeilZ), 1/256.);
                nHeigh2 = abs(pSect->ceilingz - pXSect->offCeilZ);
            }
            else
            {
                nHeigh1 = std::max(abs(pXSect->onFloorZ - pXSect->offFloorZ), 1 / 256.);
                nHeigh2 = abs(pSect->floorz - pXSect->offFloorZ);
            }
            return Cmp((int((kPercFull * nHeigh2) / nHeigh1)));
        default:
            Error(gErrors[kErrObjectUnsupp], "sector", pSect->type, pSect->extra);
            return false;
    }
}

static bool helperCmpData(int nData)
{
    switch (objType)
    {
        case EVOBJ_WALL:
            return Cmp(pXWall->data);
        case EVOBJ_SPRITE:
            switch (nData)
            {
                case 1:		return Cmp(pSpr->xspr.data1);
                case 2:		return Cmp(pSpr->xspr.data2);
                case 3:		return Cmp(pSpr->xspr.data3);
                case 4:		return Cmp(pSpr->xspr.data4);
            }
            break;
        case EVOBJ_SECTOR:
            return Cmp(pXSect->data);
    }

    return Cmp(0);
}


static bool helperChkHitscan(int nWhat)
{
    DAngle nAng = pSpr->spr.Angles.Yaw.Normalized360();
    auto  nOldStat = pSpr->spr.cstat;
    int nHit = -1, nSlope = 0;
    int nMask = -1;

    if ((nOldStat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
        nSlope = spriteGetSlope(pSpr);

    pSpr->spr.cstat = 0;
    if (nOldStat & CSTAT_SPRITE_YCENTER)
        pSpr->spr.cstat |= CSTAT_SPRITE_YCENTER;

    if ((nOldStat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR)
        pSpr->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;

    switch (arg1)
    {
        case  0: 	nMask = CLIPMASK0 | CLIPMASK1; break;
        case  1: 	nMask = CLIPMASK0; break;
        case  2: 	nMask = CLIPMASK1; break;
    }

    double range = arg3 * 2;
    if ((pPlayer = getPlayer(pSpr)) != nullptr)
    {
        nHit = HitScan(pSpr, pPlayer->zWeapon, pPlayer->aim, arg1, range);
    }
    else if (pSpr->IsDudeActor())
    {
        nHit = HitScan(pSpr, pSpr->spr.pos.Z, DVector3(nAng.ToVector(), pSpr->dudeSlope), nMask, range);
    }
    else if ((nOldStat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLOPE)
    {
        nSlope = (nOldStat & CSTAT_SPRITE_YFLIP) ? (0x08000 - abs(nSlope)) : -(0x08000 - abs(nSlope));
        nHit = HitScan(pSpr, pSpr->spr.pos.Z, DVector3(pSpr->spr.Angles.Yaw.ToVector(), nSlope / 16384.), nMask, range);
    }
    else if ((nOldStat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR)
    {
        nSlope = (pSpr->spr.cstat & CSTAT_SPRITE_YFLIP) ? 8 : -8; // was 0x10000
        nHit = HitScan(pSpr, pSpr->spr.pos.Z, DVector3(pSpr->spr.Angles.Yaw.ToVector(), nSlope), nMask, range);
    }
    else
    {
        nHit = HitScan(pSpr, pSpr->spr.pos.Z, DVector3(pSpr->spr.Angles.Yaw.ToVector(), 0), nMask, range);
    }

    pSpr->spr.cstat = nOldStat;

    if (nHit < 0)
        return false;

    switch (nWhat)
    {
        case 1:	// ceil
            if (nHit != 1) return false;
            else if (PUSH) Push(gHitInfo.hitSector);
            return true;
        case 2:	// floor
            if (nHit != 2) return false;
            else if (PUSH) Push(gHitInfo.hitSector);
            return true;
        case 3:	// wall
            if (nHit != 0 && nHit != 4) return false;
            else if (PUSH) Push(gHitInfo.hitWall);
            return true;
        case 4:	// sprite
            if (nHit != 3) return false;
            else if (PUSH) Push(gHitInfo.actor());
            return true;
        case 5: // masked wall
            if (nHit != 4) return false;
            else if (PUSH) Push(gHitInfo.hitWall);
            return true;
    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}

static bool helperChkMarker(int nWhat)
{
    if (pSpr->xspr.dudeFlag4 && pSpr->xspr.target != nullptr && pSpr->xspr.target->GetType() == kMarkerPath)
    {
        switch (nWhat) {
            case 1:
                if (auto t = aiPatrolMarkerBusy(pSpr, pSpr->xspr.target))
                {
                    if (PUSH) Push(EventObject(t));
                    return true;
                }
                break;
            case 2:
                if (aiPatrolMarkerReached(pSpr))
                {
                    if (PUSH) Push(EventObject(pSpr->xspr.target));
                    return true;
                }
                break;
            default:
                Error(gErrors[kErrInvalidArgsPass]);
                break;
        }
    }

    return false;
};

static bool helperChkTarget(int nWhat)
{
    int t = 0;
    DBloodActor* pTrgt = pSpr->xspr.target;
    if (pTrgt)
    {
        DUDEINFO* pInfo = getDudeInfo(pSpr);
        double eyeAboveZ = (pInfo->eyeHeight * pSpr->spr.scale.Y);
        auto delta = pTrgt->spr.pos.XY() - pSpr->spr.pos.XY();

        switch (nWhat) {
            case 1:
                arg1 *= 512, arg2 *= 512;
                t = Cmp(int(delta.Length() * 16));
                break;
            case 2:
            case 3:
                t = cansee(pSpr->spr.pos, pSpr->sector(), pTrgt->spr.pos.plusZ(-eyeAboveZ), pTrgt->sector());
                if (nWhat == 3 && t > 0)
                {
                    DAngle absang = absangle(delta.Angle(), pSpr->spr.Angles.Yaw);
                    t = absang < (arg1 <= 0 ? pInfo->Periphery() : min(mapangle(arg1), DAngle360));
                }
                break;
            default:
                Error(gErrors[kErrInvalidArgsPass]);
                break;
        }
    }

    if (t <= 0) return false;
    else if (PUSH) Push(pSpr->xspr.target);
    return true;
};

static bool helperChkRoom(DBloodActor* t, int nWhat)
{
    if (t)
    {
        if (PUSH)
        {
            switch (nWhat)
            {
                case 1:
                    Push(t);
                    break;
                case 2:
                    Push(t->ownerActor);
                    break;
                default:
                    Push(t->ownerActor->sector());
                    break;
            }
        }

        return true;
    }

    return false;
}


/** GAME conditions
********************************************************************************/
static bool gamCmpLevelMin(void)        { return Cmp(gFrameCount / (kTicsPerSec * 60)); }
static bool gamCmpLevelSec(void)        { return Cmp((gFrameCount / kTicsPerSec) % 60); }
static bool gamCmpLevelMsec(void)       { return Cmp(((gFrameCount % kTicsPerSec) * 33) / 10); }
static bool gamCmpLevelTime(void)       { return Cmp(gFrameCount); }
static bool gamCmpKillsTotal(void)      { return Cmp(Level.kills.max); }
static bool gamCmpKillsDone(void)       { return Cmp(Level.kills.got); }
static bool gamCmpSecretsTotal(void)    { return Cmp(Level.secrets.max); }
static bool gamCmpSecretsDone(void)     { return Cmp(Level.secrets.got); }
static bool gamCmpVisibility(void)      { return Cmp(gVisibility); }
static bool gamChkGotpic(void)          
{ 
    auto tex = tileGetTexture(arg3);
    if (tex)
    {
        if (tex->isSeen(false)) // this is neither network not demo sync safe!
        {
            texSeenToClear.Push(tex);
            return true;
        }
    }
    return false;
}
static bool gamChkChance(void)          { return Chance((0x10000 * arg3) / kPercFull); }
static bool gamCmpRandom(void)          { return Cmp(nnExtRandom(arg1, arg2)); }
static bool gamCmpStatnumCount(void)    
{
    BloodStatIterator it(ClipRange(arg3, 0, kMaxStatus));
    int c = 0;
    while (it.Next()) c++;
    return Cmp(c); // compare counter of specific statnum sprites
}
static bool gamCmpNumsprites(void)      { return Cmp(Numsprites); }
static bool gamChkPlayerConnect(void)
{
    int i;
    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        auto pl = getPlayer(i);
        if (pl->pnum + 1 != arg3) continue;
        else if (PUSH) Push(pl->GetActor());
        return true;
    }

    return false;
}
static bool gamChkSector(void)      { return helperChkSector(arg1); }
static bool gamChkWall(void)        { return helperChkWall(arg1); }
static bool gamChkSprite(void)      { return helperChkSprite(arg1); }



/** SECTOR conditions
********************************************************************************/
static bool sectCmpVisibility(void)     { return Cmp(pSect->visibility); }
static bool sectChkShow2dSector(void)   { return show2dsector.Check(objIndex); }
static bool sectChkGotSector(void)
{
    if (pSect->exflags & SECTOREX_SEEN)// this is neither network not demo sync safe!
    {
        sectorSeenToClear.Push(pSect);
        return true;
    } 
    return false;
}
static bool sectCmpFloorSlope(void)     { return Cmp(pSect->floorheinum); }
static bool sectCmpCeilSlope(void)      { return Cmp(pSect->ceilingheinum); }
static bool sectChkSprTypeInSect(void)
{
    BloodSectIterator it(pSect);
    while(auto act = it.Next())
    {
        if (!Cmp(act->GetType())) continue;
        else if (PUSH) Push(act);
        return true;
    }
    return false;
}

static bool sectCmpFloorHeight(void)    { return helperCmpHeight(false); }
static bool sectCmpCeilHeight(void)     { return helperCmpHeight(true); }
static bool sectChkUnderwater(void)     { return pXSect->Underwater; }
static bool sectChkPaused(void)         { return !pXSect->restState; }
static bool sectCmpDepth(void)          { return Cmp(pXSect->Depth); }
static bool sectChkUpperRoom(void)      { return helperChkRoom(barrier_cast<DBloodActor*>(pSect->upperLink), arg3); }
static bool sectChkLowerRoom(void)      { return helperChkRoom(barrier_cast<DBloodActor*>(pSect->lowerLink), arg3); }



/** WALL conditions
********************************************************************************/
static bool wallCmpOverpicnum(void)     { return Cmp(legacyTileNum(pWall->overtexture)); }
static bool wallChkShow2dWall(void)     { return show2dwall.Check(objIndex); }
static bool wallChkIsMirror(void)
{
    return (pWall->portalflags == PORTAL_WALL_MIRROR);
}

static bool wallChkSector(void)         { return helperChkSector(pWall->sector); }
static bool wallChkNextSector(void)     { return helperChkSector(pWall->nextsector); }
static bool wallChkNextWallSector(void) { return helperChkSector(pWall->nextWall()->sector); }
static bool wallChkNextWall(void)       { return helperChkWall(pWall->nextwall); }
static bool wallChkPoint2(void)         { return helperChkWall(pWall->point2); }





/** MIXED OBJECT conditions
********************************************************************************/
static bool mixChkObjSect(void)         { return (objType == EVOBJ_SECTOR && pSect != nullptr); }
static bool mixChkObjWall(void)         { return (objType == EVOBJ_WALL && pWall != nullptr); }
static bool mixChkObjSpr(void)          { return (objType == EVOBJ_SPRITE && pSpr != nullptr); }
static bool mixChkXRange(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pWall->hasX();
        case EVOBJ_SPRITE:	return pSpr->hasX();
        case EVOBJ_SECTOR:	return pSect->hasX();
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixCmpLotag(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->type);
        case EVOBJ_SPRITE:	return Cmp(pSpr->GetType());
        case EVOBJ_SECTOR:	return Cmp(pSect->type);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpPicSurface(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(tilesurface(pWall->walltexture));
        case EVOBJ_SPRITE:	return Cmp(tilesurface(pSpr->spr.spritetexture()));
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:	return (Cmp(tilesurface(pSect->floortexture)) || Cmp(tilesurface(pSect->ceilingtexture)));
                case  1:	return Cmp(tilesurface(pSect->floortexture));
                case  2:	return Cmp(tilesurface(pSect->ceilingtexture));
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpPic(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(legacyTileNum(pWall->walltexture));
        case EVOBJ_SPRITE:	return Cmp(legacyTileNum(pSpr->spr.spritetexture()));
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:	return (Cmp(legacyTileNum(pSect->floortexture)) || Cmp(legacyTileNum(pSect->ceilingtexture)));
                case  1:	return Cmp(legacyTileNum(pSect->floortexture));
                case  2:	return Cmp(legacyTileNum(pSect->ceilingtexture));
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpPal(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->pal);
        case EVOBJ_SPRITE:	return Cmp(pSpr->spr.pal);
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:	return (Cmp(pSect->floorpal) || Cmp(pSect->ceilingpal));
                case  1:	return Cmp(pSect->floorpal);
                case  2:	return Cmp(pSect->ceilingpal);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpShade(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->shade);
        case EVOBJ_SPRITE:	return Cmp(pSpr->spr.shade);
        case EVOBJ_SECTOR:
            switch (arg3)
            {
                default:	return (Cmp(pSect->floorshade) || Cmp(pSect->ceilingshade));
                case  1:	return Cmp(pSect->floorshade);
                case  2:	return Cmp(pSect->ceilingshade);
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpCstat(void)
{
    switch (objType)
    {
    case EVOBJ_WALL: 	return (bool)((arg3) ? Cmp(pWall->cstat & EWallFlags::FromInt(arg3)) : (pWall->cstat & EWallFlags::FromInt(arg1)));
    case EVOBJ_SPRITE:	return (bool)((arg3) ? Cmp(pSpr->spr.cstat & ESpriteFlags::FromInt(arg3)) : (pSpr->spr.cstat & ESpriteFlags::FromInt(arg1)));
        case EVOBJ_SECTOR:	// !!!
            switch (arg3)
            {
            default:	return ((pSect->floorstat & ESectorFlags::FromInt(arg1)) || (pSect->ceilingstat & ESectorFlags::FromInt(arg1)));
                case  1:	return (pSect->floorstat & ESectorFlags::FromInt(arg1));
                case  2:	return (pSect->ceilingstat & ESectorFlags::FromInt(arg1));
            }
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpHitag(void) // This is the Blood flags
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return (bool)((arg3) ? Cmp(pWall->hitag & arg3) : (pWall->hitag & arg1));
        case EVOBJ_SPRITE:	return (bool)((arg3) ? Cmp(pSpr->spr.flags & arg3) : (pSpr->spr.flags & arg1));
        case EVOBJ_SECTOR:	return (bool)((arg3) ? Cmp(pSect->hitag & arg3) : (pSect->hitag & arg1));
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpXrepeat(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->xrepeat);
        case EVOBJ_SPRITE:	return Cmp(int(pSpr->spr.scale.X * INV_REPEAT_SCALE));
        case EVOBJ_SECTOR:	return Cmp(pSect->floorxpan());
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpXoffset(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->xpan());
        case EVOBJ_SPRITE:	return Cmp(pSpr->spr.xoffset);
        case EVOBJ_SECTOR:	return Cmp(pSect->ceilingxpan());
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpYrepeat(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->yrepeat);
        case EVOBJ_SPRITE:	return Cmp(int(pSpr->spr.scale.Y * INV_REPEAT_SCALE));
        case EVOBJ_SECTOR:	return Cmp(pSect->floorxpan());
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpYoffset(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pWall->ypan());
        case EVOBJ_SPRITE:	return Cmp(pSpr->spr.yoffset);
        case EVOBJ_SECTOR:	return Cmp(pSect->ceilingypan());
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpData1(void) { return helperCmpData(1); }
static bool mixCmpData2(void) { return helperCmpData(2); }
static bool mixCmpData3(void) { return helperCmpData(3); }
static bool mixCmpData4(void) { return helperCmpData(4); }
static bool mixCmpRXId(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pXWall->rxID);
        case EVOBJ_SPRITE:	return Cmp(pSpr->xspr.rxID);
        case EVOBJ_SECTOR:	return Cmp(pXSect->rxID);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixCmpTXId(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp(pXWall->txID);
        case EVOBJ_SPRITE:	return Cmp(pSpr->xspr.txID);
        case EVOBJ_SECTOR:	return Cmp(pXSect->txID);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixChkLock(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->locked;
        case EVOBJ_SPRITE:	return pSpr->xspr.locked;
        case EVOBJ_SECTOR:	return pXSect->locked;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixChkTriggerOn(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->triggerOn;
        case EVOBJ_SPRITE:	return pSpr->xspr.triggerOn;
        case EVOBJ_SECTOR:	return pXSect->triggerOn;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixChkTriggerOff(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->triggerOff;
        case EVOBJ_SPRITE:	return pSpr->xspr.triggerOff;
        case EVOBJ_SECTOR:	return pXSect->triggerOff;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixChkTriggerOnce(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->triggerOnce;
        case EVOBJ_SPRITE:	return pSpr->xspr.triggerOnce;
        case EVOBJ_SECTOR:	return pXSect->triggerOnce;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixChkIsTriggered(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->isTriggered;
        case EVOBJ_SPRITE:	return pSpr->xspr.isTriggered;
        case EVOBJ_SECTOR:	return pXSect->isTriggered;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixChkState(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->state;
        case EVOBJ_SPRITE:	return pSpr->xspr.state;
        case EVOBJ_SECTOR:	return pXSect->state;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixCmpBusy(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return Cmp((kPercFull * pXWall->busy) / 65536);
        case EVOBJ_SPRITE:	return Cmp((kPercFull * pSpr->xspr.busy) / 65536);
        case EVOBJ_SECTOR:	return Cmp((kPercFull * pXSect->busy) / 65536);
    }

    return Cmp(0);
}
/**---------------------------------------------------------------------------**/
static bool mixChkPlayerOnly(void)
{
    switch (objType)
    {
        case EVOBJ_WALL: 	return pXWall->dudeLockout;
        case EVOBJ_SPRITE:	return pSpr->xspr.DudeLockout;
        case EVOBJ_SECTOR:	return pXSect->dudeLockout;
    }

    return false;
}
/**---------------------------------------------------------------------------**/
static bool mixCmpSeqID(void) 
{
    switch (objType)
    {
    case EVOBJ_SPRITE:
        return Cmp(seqGetID(pSpr));
    case EVOBJ_WALL:
        switch (arg3)
        {
        default:	return seqGetID(SS_WALL, pWall) || seqGetID(SS_MASKED, pWall);
        case  1:	return seqGetID(SS_WALL, pWall);
        case  2:	return seqGetID(SS_MASKED, pWall);	// masked wall
        }
        break;
    case EVOBJ_SECTOR:
        switch (arg3)
        {
        default:	return seqGetID(SS_CEILING, pSect) || seqGetID(SS_FLOOR, pSect);
        case  1:	return seqGetID(SS_CEILING, pSect);
        case  2:	return seqGetID(SS_FLOOR, pSect);
        }
        break;
    }

    return Cmp(0);
}
static bool mixCmpSeqFrame(void)        
{
    switch (objType)
    {
    case EVOBJ_SPRITE:
        return Cmp(seqGetStatus(pSpr));
    case EVOBJ_WALL:
        switch (arg3)
        {
        default:	return seqGetStatus(SS_WALL, pWall) || seqGetStatus(SS_MASKED, pWall);
        case  1:	return seqGetStatus(SS_WALL, pWall);
        case  2:	return seqGetStatus(SS_MASKED, pWall);	// masked wall
        }
        break;
    case EVOBJ_SECTOR:
        switch (arg3)
        {
        default:	return seqGetStatus(SS_CEILING, pSect) || seqGetStatus(SS_FLOOR, pSect);
        case  1:	return seqGetStatus(SS_CEILING, pSect);
        case  2:	return seqGetStatus(SS_FLOOR, pSect);
        }
        break;
    }

    return Cmp(0);
}
static bool mixCmpObjIndex(void)        { return Cmp(objIndex); }
static bool mixCmpObjXIndex(void)       { return Cmp(objIndex); } // are the same. This makes no sense because behavior was undefined.


static int getDigitFromValue(int nVal, int nOffs)
{
    char t[16];
    int l = snprintf(t, 16, "%d", abs(nVal));
    if (nOffs < l && rngok(t[nOffs], 48, 58))
        return t[nOffs] - 48;

    return -1;
}

static bool mixCmpSerials(void)
{
    unsigned int i = 0, d;
    const EventObject* const serials = pCond->condition;
    EventObject t[3];
    
    while (i < 3)
    {
        d = getDigitFromValue(arg1, i);
        if (!rngok(d, 1, 4))
        {
            Error(gErrors[kErrInvalidArgsPass]);
            return false;
        }

        t[i++] = serials[d - 1];
    }
    
    return Cmp(t[0], t[1], t[2]);

}
static bool mixChkEventCauser(void)     { return helperChkSprite(pEvent->initiator); }
static bool mixCmpEventCmd(void)        { return Cmp(pEvent->cmd); }





/** SPRITE conditions
********************************************************************************/
static bool sprCmpAng(void)             { return Cmp((arg3 == 0) ? (pSpr->spr.Angles.Yaw.Buildang() & 2047) : pSpr->spr.Angles.Yaw.Buildang()); }
static bool sprChkShow2dSprite(void)    { return !!(pSpr->spr.cstat2 & CSTAT2_SPRITE_MAPPED); }
static bool sprCmpStatnum(void)         { return Cmp(pSpr->spr.statnum); }
static bool sprChkRespawn(void)         { return ((pSpr->spr.flags & kHitagRespawn) || pSpr->spr.statnum == kStatRespawn); }
static bool sprCmpSlope(void)           { return Cmp(spriteGetSlope(pSpr)); }
static bool sprCmpClipdist(void)        { return Cmp(pSpr->clipdist); }
static bool sprChkOwner(void)           { return helperChkSprite(pSpr->ownerActor); }
static bool sprChkSector(void)          { return helperChkSector(sectindex(pSpr->spr.sectp)); }
static bool sprCmpVelocityNew(void)
{
    switch (arg3)
    {
        case 0:		return (Cmp(FloatToFixed(pSpr->vel.X)) || Cmp(FloatToFixed(pSpr->vel.Y)) || Cmp(FloatToFixed(pSpr->vel.Z)));
        case 1:		return Cmp(FloatToFixed(pSpr->vel.X));
        case 2:		return Cmp(FloatToFixed(pSpr->vel.Y));
        case 3:		return Cmp(FloatToFixed(pSpr->vel.Z));
    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}
static bool sprCmpChkVelocity(void)
{
    if (arg3)
        return sprCmpVelocityNew();

    switch (arg1)
    {
        case 0:		return (pSpr->vel.isZero());
        case 1:		return (pSpr->vel.X != 0);
        case 2:		return (pSpr->vel.Y != 0);
        case 3:		return (pSpr->vel.Z != 0);
    }

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}

/**---------------------------------------------------------------------------**/
static bool sprChkUnderwater(void)      { return IsUnderwaterSector(pSpr->sector()); }
static bool sprChkDmgImmune(void)
{
    int i;
    if (arg1 == -1)
    {
        for (i = 0; i < kDmgMax; i++)
        {
            if (!nnExtIsImmune(pSpr, i, 0))
                return false;
        }

        return true;
    }

    return nnExtIsImmune(pSpr, arg1, 0);
}
/**---------------------------------------------------------------------------**/
static bool sprChkHitscanCeil(void)     { return helperChkHitscan(1); }
static bool sprChkHitscanFloor(void)    { return helperChkHitscan(2); }
static bool sprChkHitscanWall(void)     { return helperChkHitscan(3); }
static bool sprChkHitscanSpr(void)      { return helperChkHitscan(4); }
static bool sprChkHitscanMasked(void)   { return helperChkHitscan(5); }
static bool sprChkIsTarget(void)
{
    BloodStatIterator it(kStatDude);
    while(auto act = it.Next())
    {
        if (act != pSpr && act->hasX() && act->IsDudeActor())
        {
            if (act->xspr.health <= 0 || act->xspr.target != pSpr) continue;
            else if (PUSH) Push(act);
            return true;
        }
    }
    return false;
}
/**---------------------------------------------------------------------------**/
static bool sprCmpHealth(void)
{
    int t = 0;
    if (pSpr->IsDudeActor())
        t = (pSpr->xspr.sysData2 > 0) ? ClipRange(pSpr->xspr.sysData2 << 4, 1, 65535) : getDudeInfo(pSpr->GetType())->startHealth << 4;
    else if (pSpr->GetType() == kThingBloodChunks)
        return Cmp(0);
    else if (pSpr->IsThingActor())
        t = pSpr->IntVar("defhealth") << 4;

    return Cmp((kPercFull * pSpr->xspr.health) / ClipLow(t, 1));
}
/**---------------------------------------------------------------------------**/
static bool sprChkTouchCeil(void)
{
    if ((pSpr->hit.ceilhit.type != kHitSector)) return false;
    else if (PUSH) Push(pSpr->hit.ceilhit.hitSector);
    return true;
}
/**---------------------------------------------------------------------------**/
static bool sprChkTouchFloor(void)
{
    if ((pSpr->hit.florhit.type != kHitSector)) return false;
    else if (PUSH) Push(pSpr->hit.florhit.hitSector);
    return true;
}
/**---------------------------------------------------------------------------**/
static bool sprChkTouchWall(void)
{
    if ((pSpr->hit.hit.type != kHitWall)) return false;
    else if (PUSH) Push(pSpr->hit.hit.hitWall);
    return true;
}
/**---------------------------------------------------------------------------**/
static bool sprChkTouchSpite(void)
{
    DBloodActor* hit = nullptr;

    if (xAvail)
    {
        switch (arg3)
        {
        case 0:
        case 1:
            if (pSpr->hit.florhit.type == kHitSprite) hit = pSpr->hit.florhit.safeActor();
            if (arg3 || hit) break;
            [[fallthrough]];
        case 2:
            if (pSpr->hit.hit.type == kHitSprite) hit = pSpr->hit.hit.safeActor();
            if (arg3 || hit) break;
            [[fallthrough]];
        case 3:
            if (pSpr->hit.ceilhit.type == kHitSprite) hit = pSpr->hit.ceilhit.safeActor();
            break;
        }
    }

    // check if something touching this sprite
    if (hit == nullptr && pSpr->sector() != nullptr)
    {
        BloodSectIterator it(pSpr->sector());
        while (hit = it.Next())
        {
            if (!hit->hasX())
                continue;

            if (arg3 == 1 || !arg3)
            {
                if (hit->hit.ceilhit.type == kHitSprite && hit->hit.ceilhit.safeActor() == pSpr)
                    break;
            }

            if (arg3 == 2 || !arg3)
            {
                if (hit->hit.hit.type == kHitSprite && hit->hit.hit.safeActor() == pSpr)
                    break;
            }

            if (arg3 == 3 || !arg3)
            {
                if (hit->hit.florhit.type == kHitSprite && hit->hit.florhit.safeActor() == pSpr)
                    break;
            }
        }
    }

    if (hit == nullptr) return false;
    else if (PUSH) Push(hit);
    return true;
}
/**---------------------------------------------------------------------------**/
static bool sprCmpBurnTime(void)
{
    int t = (pSpr->IsDudeActor()) ? 2400 : 1200;
    if (!Cmp((kPercFull * pSpr->xspr.burnTime) / t)) return false;
    else if (PUSH && pSpr->xspr.burnSource) Push(pSpr->xspr.burnSource);
    return true;
}
/**---------------------------------------------------------------------------**/
static bool sprChkIsFlareStuck(void)
{
    BloodStatIterator it(kStatFlare);
    while(auto act = it.Next())
    {
        if (!act->hasX() || (act->spr.flags & kHitagFree))
            continue;

        if (act->xspr.target != pSpr) continue;
        else if (PUSH) Push(act);
        return true;
    }

    return false;
}
static bool sprCmpMass(void)        { return Cmp(getSpriteMassBySize(pSpr)); }
static bool sprChkTarget(void)      { return helperChkSprite(pSpr->xspr.target); }




/** GLOBAL DUDE functions
********************************************************************************/
static bool gdudChkHaveTargets(void)
{
    DBloodActor* targ = pSpr->xspr.target;
    if (!targ) return false;
    else if (!targ->IsDudeActor() && targ->GetType() != kMarkerPath) return false;
    else if (PUSH) Push(targ);
    return true;
};
static bool gdudChkInAiFight(void)          { return aiFightDudeIsAffected(pSpr); };
static bool gdudChkTargetDistance(void)     { return helperChkTarget(1); };
static bool gdudChkTargetCansee(void)       { return helperChkTarget(2); };
static bool gdudChkTargetCanseePerip(void)  { return helperChkTarget(3); };
static bool gdudChkFlagPatrol(void)         { return pSpr->xspr.dudeFlag4; };
static bool gdudChkFlagDeaf(void)           { return pSpr->xspr.dudeDeaf; };
static bool gdudChkFlagBlind(void)          { return pSpr->xspr.dudeGuard; };
static bool gdudChkFlagAlarm(void)          { return pSpr->xspr.dudeAmbush; };
static bool gdudChkFlagStealth(void)        { return ((pSpr->xspr.modernFlags & kDudeFlagStealth) != 0); };
static bool gdudChkMarkerBusy(void)         { return helperChkMarker(1); };
static bool gdudChkMarkerReached(void)      { return helperChkMarker(2); };
static bool gdudCmpSpotProgress(void)
{
    if (!pSpr->xspr.dudeFlag4 || !pSpr->xspr.target || pSpr->xspr.target->GetType() != kMarkerPath)	return Cmp(0);
    else if (!(pSpr->xspr.modernFlags & kDudeFlagStealth) || !valueIsBetween(pSpr->xspr.data3, 0, kMaxPatrolSpotValue))	return Cmp(0);
    else return Cmp((kPercFull * pSpr->xspr.data3) / kMaxPatrolSpotValue);
};
static bool gdudChkLockout(void)            { return getDudeInfo(pSpr->GetType())->lockOut; };
static bool gdudCmpAiStateType(void)        { return Cmp(pSpr->xspr.aiState->stateType); };
static bool gdudCmpAiStateTimer(void)       { return Cmp(pSpr->xspr.stateTimer); };





/** CUSTOM DUDE functions
********************************************************************************/
static bool cdudChkLeechThrown(void)
{
#pragma message("fix for custom dudes")
#if 0
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    if (!pDude->IsLeechBroken() && pDude->pXLeech)
        return helperChkSprite(pDude->pXLeech->reference);
#endif
    return false;
};
static bool cdudChkLeechDead(void)
{
#pragma message("fix" __FUNCTION__ "for custom dudes")
#if 0
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    if (pDude->IsLeechBroken()) return true;
    else if (PUSH && pDude->pXLeech) Push(EVOBJ_SPRITE, pDude->pXLeech->reference);
#endif
    return false;
};
static bool cdudCmpSummoned(void)
{
#pragma message("fix" __FUNCTION__ "for custom dudes")
#if 0
    IDLIST* pSlaves = cdudeGet(pSpr)->pSlaves;
    if (!pSlaves)
        return Cmp(0);

    return Cmp(pSlaves->Length());
#else
    return false;
#endif
};
static bool cdudChkIfAble(void)
{
#pragma message("fix" __FUNCTION__ "for custom dudes")
#if 0
    switch (arg3)
    {
        case 1: return false;
        case 2: return cdudeGet(pSpr)->CanBurn();
        case 3: return cdudeGet(pSpr)->CanCrouch();
        case 4: return cdudeGet(pSpr)->CanElectrocute();
        case 5: return false;
        case 6: return cdudeGet(pSpr)->CanRecoil();
        case 7: return cdudeGet(pSpr)->CanSwim();
        case 8: return cdudeGet(pSpr)->CanMove();
        default:
            Error(gErrors[kErrInvalidArgsPass]);
            break;
    }
#endif
    return false;
};
static bool cdudCmpDispersion(void)
{
#pragma message("fix" __FUNCTION__ "for custom dudes")
#if 0
    CUSTOMDUDE_WEAPON* pWeapon = cdudeGet(pSpr)->pWeapon;
    if (!pWeapon)
        return Cmp(0);

    return Cmp(pWeapon->dispersion[0]);
#else
    return false;
#endif
};





/** PLAYER functions
********************************************************************************/
static bool plyCmpConnected(void)
{
    if (!Cmp(pPlayer->pnum + 1)) return false;
    else return helperChkSprite(pPlayer->GetActor());
}
static bool plyCmpTeam(void)                { return Cmp(pPlayer->teamId + 1); }
static bool plyChkHaveKey(void)             { return (valueIsBetween(arg1, 0, 8) && pPlayer->hasKey[arg1 - 1]); }
static bool plyChkHaveWeapon(void)          { return (valueIsBetween(arg1, 0, 15) && pPlayer->hasWeapon[arg1 - 1]); }
static bool plyCmpCurWeapon(void)           { return Cmp(pPlayer->curWeapon); }
static bool plyCmpPackItemAmount(void)      { return (valueIsBetween(arg1, 0, 6) && Cmp(pPlayer->packSlots[arg1 - 1].curAmount)); }
static bool plyChkPackItemActive(void)      { return (valueIsBetween(arg1, 0, 6) && pPlayer->packSlots[arg1 - 1].isActive); }
static bool plyCmpPackItemSelect(void)      { return Cmp(pPlayer->packItemId + 1); }
static bool plyCmpPowerupAmount(void)
{
    int t;
    if (arg3 > 0 && arg3 <= (kMaxAllowedPowerup - (kMinAllowedPowerup << 1) + 1))
    {
        t = (kMinAllowedPowerup + arg3) - 1; // allowable powerups
        return Cmp(pPlayer->pwUpTime[t] / kPercFull);
    }

    Error("Unexpected powerup #%d", arg3);
    return false;
}
static bool plyChkKillerSprite(void)    { return helperChkSprite(pPlayer->fragger); }
static bool plyChkKeyPress(void)
{
    switch (arg1)
    {
		case 1:  return (pPlayer->cmd.ucmd.vel.X > 0);            // forward
		case 2:  return (pPlayer->cmd.ucmd.vel.X < 0);            // backward
		case 3:  return (pPlayer->cmd.ucmd.vel.Y < 0);             // left
		case 4:  return (pPlayer->cmd.ucmd.vel.Y > 0);             // right
		case 5:  return !!(pPlayer->cmd.ucmd.actions & SB_JUMP);       // jump
		case 6:  return !!(pPlayer->cmd.ucmd.actions & SB_CROUCH);     // crouch
		case 7:  return !!(pPlayer->cmd.ucmd.actions & SB_FIRE);      // normal fire weapon
		case 8:  return !!(pPlayer->cmd.ucmd.actions & SB_ALTFIRE);     // alt fire weapon
		case 9:  return !!(pPlayer->cmd.ucmd.actions & SB_OPEN);        // use
        default:
            Error("Specify a correct key!");
            return false;
    }
}
static bool plyChkRunning(void)             { return pPlayer->isRunning; }
static bool plyChkFalling(void)             { return pPlayer->fallScream; }
static bool plyCmpLifeMode(void)            { return Cmp(pPlayer->lifeMode + 1); }
static bool plyCmpPosture(void)             { return Cmp(pPlayer->posture + 1); }
static bool plyCmpKillsCount(void)          { return Cmp(pPlayer->fragCount); }
static bool plyChkAutoAimTarget(void)       { return helperChkSprite(pPlayer->aimTarget); }
static bool plyChkVoodooTarget(void)        { return helperChkSprite(pPlayer->voodooTarget); }

static bool plyCmpQavWeapon(void)           { return Cmp(pPlayer->weaponQav); }
static bool plyCmpQavScene(void)            { return Cmp(pPlayer->sceneQav); }
static bool plyChkGodMode(void)             { return (pPlayer->godMode || powerupCheck(pPlayer, kPwUpDeathMask)); }
static bool plyChkShrink(void)              { return isShrunk(pSpr); }
static bool plyChkGrown(void)               { return isGrown(pSpr); }
static bool plyCmpArmor(void)
{
    if (valueIsBetween(arg3, 0, 4))
        return Cmp((pPlayer->armor[arg3 - 1] * kPercFull) / 1600);

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}
static bool plyCmpAmmo(void)
{
    if (valueIsBetween(arg3, 0, 12))
        return Cmp(pPlayer->ammoCount[arg3 - 1]);

    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}
static bool plyChkSpriteItOwns(void)
{ 
    if (rngok(arg3, 0, kMaxStatus + 1))
    {
        BloodStatIterator it(arg3);
        while (auto act = it.Next())
        {
            if (!Cmp(act->GetType()) || act->ownerActor != pPlayer->GetActor()) continue;
            else if (PUSH) Push(act);
            return true;
        }

        return false;
    }
    
    Error(gErrors[kErrInvalidArgsPass]);
    return false;
}


/** TABLE OF CONDITION FUNCTIONS
********************************************************************************/
static CONDITION_INFO gConditionsList[] =
{
    { gamCmpLevelMin,			1,		CGAM,		false,		false },	// compare level minutes
    { gamCmpLevelSec,			2,		CGAM,		false,		false },	// compare level seconds
    { gamCmpLevelMsec,			3,		CGAM,		false,		false },	// compare level mseconds
    { gamCmpLevelTime,			4,		CGAM,		false,		false },	// compare level time (unsafe)
    { gamCmpKillsTotal,			5,		CGAM,		false,		false },	// compare current global kills counter
    { gamCmpKillsDone,			6,		CGAM,		false,		false },	// compare total global kills counter
    { gamCmpSecretsDone,		7,		CGAM,		false,		false },	// compare how many secrets found
    { gamCmpSecretsTotal,		8,		CGAM,		false,		false },	// compare total secrets
    { gamCmpVisibility,			20,		CGAM,		false,		false },	// compare global visibility value
    { gamChkGotpic,				21,		CGAM,		true,		false },	// check gotpic
    { gamChkChance,				30,		CGAM,		true,		false },	// check chance %
    { gamCmpRandom,				31,		CGAM,		false,		false },	// compare random
    { gamCmpStatnumCount,		47,		CGAM,		false,		false },	// compare counter of specific statnum sprites
    { gamCmpNumsprites,			48,		CGAM,		false,		false },	// compare counter of total sprites
    { gamChkSector,		        57,		CGAM,		true,		false },	// get sector N if possible
    { gamChkWall,		        58,		CGAM,		true,		false },	// get wall N if possible
    { gamChkSprite,		        59,		CGAM,		true,		false },	// get sprite N if possible
    { gamChkPlayerConnect,		60,		CGAM,		true,		false },	// check if player N connected
    /**--------------------------------------------------------------**/
    { mixChkObjSect,			100,	CMIX,		true,		false },
    { mixChkObjWall,			105,	CMIX,		true,		false },
    { mixChkObjSpr,				110,	CMIX,		true,		false },
    { mixChkXRange,				115,	CMIX,		true,		false },
    { mixCmpLotag,				120,	CMIX,		false,		false },
    { mixCmpPicSurface,			124,	CMIX,		false,		false },
    { mixCmpPic,				125,	CMIX,		false,		false },
    { mixCmpPal,				126,	CMIX,		false,		false },
    { mixCmpShade,				127,	CMIX,		false,		false },
    { mixCmpCstat,				128,	CMIX,		false,		false },
    { mixCmpHitag,				129,	CMIX,		false,		false },
    { mixCmpXrepeat,			130,	CMIX,		false,		false },
    { mixCmpXoffset,			131,	CMIX,		false,		false },
    { mixCmpYrepeat,			132,	CMIX,		false,		false },
    { mixCmpYoffset,			133,	CMIX,		false,		false },
    { mixCmpData1,				141,	CMIX,		false,		true  },
    { mixCmpData2,				142,	CMIX,		false,		true  },
    { mixCmpData3,				143,	CMIX,		false,		true  },
    { mixCmpData4,				144,	CMIX,		false,		true  },
    { mixCmpRXId,				150,	CMIX,		false,		true  },
    { mixCmpTXId,				151,	CMIX,		false,		true  },
    { mixChkLock,				152,	CMIX,		true,		true  },
    { mixChkTriggerOn,			153,	CMIX,		true,		true  },
    { mixChkTriggerOff,			154,	CMIX,		true,		true  },
    { mixChkTriggerOnce,		155,	CMIX,		true,		true  },
    { mixChkIsTriggered,		156,	CMIX,		true,		true  },
    { mixChkState,				157,	CMIX,		true,		true  },
    { mixCmpBusy,				158,	CMIX,		false,		true  },
    { mixChkPlayerOnly,			159,	CMIX,		true,		true  },
    { mixCmpSeqID,				170,	CMIX,		false,		true  },
    { mixCmpSeqFrame,			171,	CMIX,		false,		true  },
    { mixCmpObjIndex,			195,	CMIX,		false,		false },
    { mixCmpObjXIndex,			196,	CMIX,		false,		false },
    { mixCmpSerials,		    197,	CMIX,		false,		false },
    { mixChkEventCauser,		198,	CMIX,		true,		false },	// check event causer
    { mixCmpEventCmd,			199,	CMIX,		false,		false },	// this condition received N command?
    /**--------------------------------------------------------------**/
    { wallCmpOverpicnum,		200,	CWAL,		false,		false },
    { wallChkShow2dWall,		201,	CWAL,		true,		false },	// wall on the minimap
    { wallChkSector,			205,	CWAL,		true,		false },
    { wallChkIsMirror,			210,	CWAL,		true,		false },
    { wallChkNextSector,		215,	CWAL,		true,		false },
    { wallChkNextWall,			220,	CWAL,		true,		false },
    { wallChkPoint2,			221,	CWAL,		true,		false },
    { wallChkNextWallSector,	225,	CWAL,		true,		false },
    /**--------------------------------------------------------------**/
    { sectCmpVisibility,		300,	CSEC,		false,		false },	// compare visibility
    { sectChkShow2dSector,		301,	CSEC,		true,		false },	// sector on the minimap
    { sectChkGotSector,			302,	CSEC,		true,		false },	// sector on the screen
    { sectCmpFloorSlope,		305,	CSEC,		false,		false },	// compare floor slope
    { sectCmpCeilSlope,			306,	CSEC,		false,		false },	// compare ceil slope
    { sectChkSprTypeInSect,		310,	CSEC,		true,		false },	// check is sprite with lotag N in sector
    { sectChkUnderwater,		350,	CSEC,		true,		true  },	// sector is underwater?
    { sectCmpDepth,				351,	CSEC,		false,		true  },	// compare depth level
    { sectCmpFloorHeight,		355,	CSEC,		false,		true  },	// compare floor height (in %)
    { sectCmpCeilHeight,		356,	CSEC,		false,		true  },	// compare ceil height (in %)
    { sectChkPaused,			357,	CSEC,		true,		true  },	// this sector in movement?
    { sectChkUpperRoom,			358,	CSEC,		true,		false },	// get upper room sector or marker
    { sectChkLowerRoom,			359,	CSEC,		true,		false },	// get lower room sector or marker
    /**--------------------------------------------------------------**/
    { plyCmpConnected,			400,	CPLY,		false,		false },
    { plyCmpTeam,				401,	CPLY,		false,		false },
    { plyChkHaveKey,			402,	CPLY,		true,		false },
    { plyChkHaveWeapon,			403,	CPLY,		true,		false },
    { plyCmpCurWeapon,			404,	CPLY,		false,		false },
    { plyCmpPackItemAmount,		405,	CPLY,		false,		false },
    { plyChkPackItemActive,		406,	CPLY,		true,		false },
    { plyCmpPackItemSelect,		407,	CPLY,		false,		false },
    { plyCmpPowerupAmount,		408,	CPLY,		false,		false },
    { plyChkKillerSprite,		409,	CPLY,		true,		false },
    { plyChkKeyPress,			410,	CPLY,		true,		false },	// check keys pressed
    { plyChkRunning,			411,	CPLY,		true,		false },
    { plyChkFalling,			412,	CPLY,		true,		false },
    { plyCmpLifeMode,			413,	CPLY,		false,		false },
    { plyCmpPosture,			414,	CPLY,		false,		false },
    { plyChkSpriteItOwns,	    419,	CPLY,		true,		false },
    { plyCmpArmor,  			420,	CPLY,		false,		false },    // in %
    { plyCmpAmmo,  			    421,	CPLY,		false,		false },
    { plyCmpKillsCount,			430,	CPLY,		false,		false },
    { plyChkAutoAimTarget,		431,	CPLY,		true,		false },
    { plyChkVoodooTarget,		435,	CPLY,		true,		false },
    { plyCmpQavWeapon,			445,	CPLY,		false,		false },
    { plyCmpQavScene,			446,	CPLY,		false,		false },
    { plyChkGodMode,			447,	CPLY,		true,		false },
    { plyChkShrink,				448,	CPLY,		true,		false },
    { plyChkGrown,				449,	CPLY,		true,		false },
    /**--------------------------------------------------------------**/
    { gdudChkHaveTargets,		450,	CDUDG,		true,		true  },	// dude have any targets?
    { gdudChkInAiFight,			451,	CDUDG,		true,		true  },	// dude affected by ai fight?
    { gdudChkTargetDistance,	452,	CDUDG,		true,		true  },	// distance to the target in a range?
    { gdudChkTargetCansee,		453,	CDUDG,		true,		true  },	// is the target visible?
    { gdudChkTargetCanseePerip,	454,	CDUDG,		true,		true  },	// is the target visible with periphery?
    { gdudChkFlagPatrol,		455,	CDUDG,		true,		true  },
    { gdudChkFlagDeaf,			456,	CDUDG,		true,		true  },
    { gdudChkFlagBlind,			457,	CDUDG,		true,		true  },
    { gdudChkFlagAlarm,			458,	CDUDG,		true,		true  },
    { gdudChkFlagStealth,		459,	CDUDG,		true,		true  },
    { gdudChkMarkerBusy,		460,	CDUDG,		true,		true  },	// check if the marker is busy with another dude
    { gdudChkMarkerReached,		461,	CDUDG,		true,		true  },	// check if the marker is reached
    { gdudCmpSpotProgress,		462,	CDUDG,		false,		true  },	// compare spot progress value in %
    { gdudChkLockout,			465,	CDUDG,		true,		true  },	// dude allowed to interact with objects?
    { gdudCmpAiStateType,		466,	CDUDG,		false,		true  },
    { gdudCmpAiStateTimer,		467,	CDUDG,		false,		true  },
    /**--------------------------------------------------------------**/
    { cdudChkLeechThrown,		470,	CDUDC,		true,		false },	// life leech is thrown?
    { cdudChkLeechDead,			471,	CDUDC,		true,		false },	// life leech is destroyed?
    { cdudCmpSummoned,			472,	CDUDC,		false,		false },	// are required amount of dudes is summoned?
    { cdudChkIfAble,			473,	CDUDC,		true,		false },	// check if dude can...
    { cdudCmpDispersion,		474,	CDUDC,		false,		false },	// compare weapon dispersion
    /**--------------------------------------------------------------**/
    { sprCmpAng,				500,	CSPR,		false,		false },	// compare angle
    { sprChkShow2dSprite,		501,	CSEC,		true,		false },	// sprite on the minimap
    { sprCmpStatnum,			505,	CSPR,		false,		false },	// check statnum
    { sprChkRespawn,			506,	CSPR,		true,		false },	// check if on respawn list
    { sprCmpSlope,				507,	CSPR,		false,		false },	// compare slope
    { sprCmpClipdist,			510,	CSPR,		false,		false },	// compare clipdist
    { sprChkOwner,				515,	CSPR,		true,		false },	// check owner sprite
    { sprChkSector,				520,	CSPR,		true,		false },	// stays in a sector?
    { sprCmpChkVelocity,		525,	CSPR,		true,		false },	// check or compare velocity
    { sprCmpVelocityNew,		526,	CSPR,		false,		false },	// compare velocity
    { sprChkUnderwater,			530,	CSPR,		true,		false },	// sector of sprite is underwater?
    { sprChkDmgImmune,			531,	CSPR,		true,		false },	// check if immune to N dmgType
    { sprChkHitscanCeil,		535,	CSPR,		true,		false },	// hitscan: ceil?
    { sprChkHitscanFloor,		536,	CSPR,		true,		false },	// hitscan: floor?
    { sprChkHitscanWall,		537,	CSPR,		true,		false },	// hitscan: wall?
    { sprChkHitscanSpr,			538,	CSPR,		true,		false },	// hitscan: sprite?
    { sprChkHitscanMasked,		539,	CSPR,		true,		false },	// hitscan: masked wall?
    { sprChkIsTarget,			545,	CSPR,		true,		false },	// this sprite is a target of some dude?
    { sprCmpHealth,				550,	CSPR,		false,		true  },	// compare hp (in %)
    { sprChkTouchCeil,			555,	CSPR,		true,		true  },	// touching ceil of sector?
    { sprChkTouchFloor,			556,	CSPR,		true,		true  },	// touching floor of sector?
    { sprChkTouchWall,			557,	CSPR,		true,		true  },	// touching walls of sector?
    { sprChkTouchSpite,			558,	CSPR,		true,		false },	// touching another sprite? (allow no xAvail!)
    { sprCmpBurnTime,			565,	CSPR,		false,		true  },	// compare burn time (in %)
    { sprChkIsFlareStuck,		566,	CSPR,		true,		false },	// any flares stuck in this sprite?
    { sprChkTarget,		        569,	CSPR,		true,		true },	    // use with caution!
    { sprCmpMass,				570,	CSPR,		false,		true  },	// mass of the sprite in a range?
};



/** CONDITION INTERFACE FUNCTIONS
********************************************************************************/
static bool DefaultResult()     { return (pEntry->isBool) ? false : Cmp(0); }
static bool CheckGeneric()      { return pEntry->pFunc(); }
static bool CheckSector()
{
    if (objType == EVOBJ_SECTOR && rngok(objIndex, 0, sector.Size()))
    {
        pSect = &sector[objIndex];
        if (pSect->hasX())
        {
            pXSect = &pSect->xs();
            xAvail = true;
        }
        else
        {
            pXSect = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "sector");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckWall()
{
    if (objType == EVOBJ_WALL && rngok(objIndex, 0, wall.Size()))
    {
        pWall = &wall[objIndex];
        if (pWall->hasX())
        {
            pXWall = &pWall->xw();
            xAvail = true;
        }
        else
        {
            pXWall = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "wall");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckDude()
{
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        if ((!pSpr->IsDudeActor() && pSpr->type != kThingBloodChunks) || IsPlayerSprite(pSpr))
        {
            Error(gErrors[kErrInvalidObject], objIndex, objType, "dude");
            return false;
        }

        if (pSpr->extra > 0)
        {
            pXSpr = &xsprite[pSpr->extra];
            xAvail = true;
        }
        else
        {
            pXSpr = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "sprite");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckCustomDude()
{
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        switch (pSpr->type) {
            case kThingBloodChunks:
                if (pSpr->inittype == kDudeModernCustom) break;
                Error(gErrors[kErrInvalidObject], objIndex, objType, "custom dude");
                return false;
            case kDudeModernCustom:
                break;
            default:
                Error(gErrors[kErrInvalidObject], objIndex, objType, "custom dude");
                return false;
        }

        if (pSpr->extra > 0)
        {
            pXSpr = &xsprite[pSpr->extra];
            xAvail = true;
        }
        else
        {
            pXSpr = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "sprite");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckPlayer()
{
    int i;
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        for (i = connecthead; i >= 0; i = connectpoint2[i])
        {
            if (objIndex != gPlayer[i].nSprite) continue;
            pPlayer = &gPlayer[i];
            break;
        }

        // there is no point to check unlinked or disconnected players
        if (i < 0)
            return DefaultResult();

        if (pSpr->extra > 0)
        {
            pXSpr = &xsprite[pSpr->extra];
            xAvail = true;
        }
        else
        {
            pXSpr = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "player");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckSprite()
{
    if (objType == EVOBJ_SPRITE && rngok(objIndex, 0, kMaxSprites))
    {
        pSpr = &sprite[objIndex];
        if (pSpr->extra > 0)
        {
            pXSpr = &xsprite[pSpr->extra];
            xAvail = true;
        }
        else
        {
            pXSpr = nullptr;
            xAvail = false;
            if (pEntry->xReq)
                return DefaultResult();
        }
    }
    else
    {
        Error(gErrors[kErrInvalidObject], objIndex, objType, "sprite");
        return false;
    }

    return pEntry->pFunc();
}

static bool CheckObject()
{
    switch (objType)
    {
        case EVOBJ_WALL:    return CheckWall();
        case EVOBJ_SECTOR:  return CheckSector();
        case EVOBJ_SPRITE:  return CheckSprite();
    }

    // conditions can only work with objects in the switch anyway...
    Error(gErrors[kErrUnknownObject], objType, objIndex);
    return false;
}


static void Restore()
{
    int t;
    switch (pCond->xspr.command - kCmdPop)
    {
        case 0:
            pCond->spr.condition[0] = pCond->spr.condition[1];
            break;
        // additional slots leads to swapping
        // so object in the focus is not lost
        case 1:
            std::swap(pCond->spr.condition[0], pCond->spr.condition[2]);
            break;
        case 2:
            std::swap(pCond->spr.condition[0], pCond->spr.condition[3]);
            break;
    }
}

static void UnSerialize(int nSerial, int* oType, int* oIndex)
{
    if (rngok(nSerial, kSerialSector, kSerialWall))
    {
        *oIndex = nSerial - kSerialSector;
        *oType = EVOBJ_SECTOR;
    }
    else if (rngok(nSerial, kSerialWall, kSerialSprite))
    {
        *oIndex = nSerial - kSerialWall;
        *oType = EVOBJ_WALL;
    }
    else if (rngok(nSerial, kSerialSprite, kSerialMax))
    {
        *oIndex = nSerial - kSerialSprite;
        *oType = EVOBJ_SPRITE;
    }
    else
    {
        Error(gErrors[kErrInvalidSerial], nSerial);
    }
}

static bool Cmp(EventObject ob1, EventObject ob2, EventObject ob3)
{
    // What a f*cing mess... We cannot even report a setup error from here. :(
    if (ob1.isActor())
    {
        if (!ob2.isActor() || !ob3.isActor()) return false;
        return Cmp(ob1.actor()->time, ob2.actor()->time, ob3.actor()->time);
    }
    if (ob1.isWall())
    {
        if (!ob2.isWall() || !ob3.isWall()) return false;
        return Cmp(wallindex(ob1.wall()), wallindex(ob2.wall()), wallindex(ob3.wall()));
    }
    if (ob1.isSector())
    {
        if (!ob2.isSector() || !ob3.isSector()) return false;
        return Cmp(sectindex(ob1.sector()), sectindex(ob2.sector()), sectindex(ob3.sector()));
    }
    return false;
}


static bool Cmp(int val)
{
    if (cmpOp & 0x2000)
        return (cmpOp & 1) ? (val > arg1) : (val >= arg1); // blue sprite
    else if (cmpOp & 0x4000)
        return (cmpOp & 1) ? (val < arg1) : (val <= arg1); // green sprite
    else if (cmpOp & 1)
        return (val >= arg1 && val <= arg2);
    else
        return (val == arg1);
}

static bool Cmp(int val, int nArg1, int nArg2)
{
    arg1 = nArg1;
    arg2 = nArg2;

    return Cmp(val);
}

static void Error(const char* pFormat, ...)
{
    char buffer[512], buffer2[512], condType[32];
    strcpy(condType, (pEntry) ? gCheckFuncInfo[pEntry->type].name : "Unknown");
    strupr(condType);

    va_list args;
    va_start(args, pFormat);
    vsprintf(buffer2, pFormat, args);
    va_end(args);

    snprintf(buffer, 512,
        "\n"
        "ERROR IN %s CONDITION ID #%d:\n"
        "%s\n\n"
        "Debug information:\n"
        "--------------------------------------------\n"
        "Condition sprite  =  %d,  RX ID  =  %d,  TX ID  =  %d\n"
        "Arguments  =  %d,  %d,  %d\n"
        "Operator  =  %d\n",
        condType, pCond->xspr.data1, buffer2,
        pCond->spr.time,
        pCond->xspr.rxID,
        pCond->xspr.txID,
        arg1,
        arg2,
        arg3,
        cmpOp &= ~0x8000
    );

    I_Error(buffer);

}

static void ReceiveObjects(EVENT* pFrom)
{
    char srcIsCondition = false;

    objType = pFrom->type; objIndex = pFrom->index;
    if (objType == EVOBJ_SPRITE && pCond->spr.index != objIndex)
        srcIsCondition = (sprite[objIndex].statnum == kStatModernCondition);

    if (!srcIsCondition)
    {
        // save object serials in the "stack"
        pCond->spr.condition[0] = EventObject(objType, objIndex);
        pCond->spr.condition[1] = pCond->spr.condition[0];
        pCond->spr.condition[2] = pCond->spr.condition[0];
        pCond->spr.condition[3] = pCond->spr.condition[0];
    }
    else
    {
        // or grab serials of objects from previous conditions
        pCond->spr.condition[0] = xsprite[sprite[objIndex].extra].condition[0];
        pCond->spr.condition[1] = xsprite[sprite[objIndex].extra].condition[1];
        pCond->spr.condition[2] = xsprite[sprite[objIndex].extra].condition[2];
        pCond->spr.condition[3] = xsprite[sprite[objIndex].extra].condition[3];
    }
}

static void TriggerObject(int nSerial)
{
    int oType, oIndex;
    UnSerialize(nSerial, &oType, &oIndex);
    nnExtTriggerObject(oType, oIndex, pCond->xspr.command, pCond->spr.index);
}



/** EXTERNAL CODE
********************************************************************************/
void conditionsInit(bool bSaveLoad)
{
    unsigned int i, j;

    gConditions.Clear();
    if (!gNumConditions)
    {
        if (gConditions)
        {
            Bfree(gConditions);
            gConditions = nullptr;
        }

        // sort out *condition function callers* list the right way
        qsort(gCheckFuncInfo, LENGTH(gCheckFuncInfo), sizeof(gCheckFuncInfo[0]), (int(*)(const void*, const void*))qsSortCheckFuncInfo);

        for (i = 0; i < LENGTH(gConditionsList); i++)
        {
            CONDITION_INFO* pTemp = &gConditionsList[i];

            // check for duplicates
            for (j = 0; j < LENGTH(gConditionsList); j++)
            {
                if (i != j)
                    dassert(pTemp->id != gConditionsList[j].id);
            }

            // find the highest condition id
            if (pTemp->id > gNumConditions)
                gNumConditions = pTemp->id;
        }

        // allocate the list
        gNumConditions++;
        gConditions = (CONDITION_INFO*)Bmalloc(sizeof(CONDITION_INFO) * gNumConditions);
        dassert(gConditions != nullptr);

        // dummy template for everything
        for (i = 0; i < gNumConditions; i++)
        {
            CONDITION_INFO* pTemp = &gConditions[i];
            pTemp->pFunc = errCondNotImplemented; pTemp->type = CNON;
            pTemp->pCaller = &gCheckFuncInfo[pTemp->type];
            pTemp->isBool = false;
            pTemp->xReq = false;
            pTemp->id = i;
        }

        for (i = 0; i < LENGTH(gConditionsList); i++)
        {
            CONDITION_INFO* pTemp = &gConditionsList[i];

            // proper caller info for each function
            for (j = 0; j < LENGTH(gCheckFuncInfo); j++)
            {
                if (gCheckFuncInfo[j].type != pTemp->type) continue;
                pTemp->pCaller = &gCheckFuncInfo[j];
                break;
            }

            dassert(j < LENGTH(gCheckFuncInfo));

            // sort out condition list the right way
            memcpy(&gConditions[pTemp->id], pTemp, sizeof(gConditionsList[0]));
        }
    }

    // allocate tracking conditions and collect objects for it
    conditionsTrackingAlloc(bSaveLoad);
}

void conditionsTrackingAlloc(bool bSaveLoad)
{
    int nCount = 0, i, j, s, e;

    conditionsTrackingClear();
    for (i = headspritestat[kStatModernCondition]; i >= 0; i = nextspritestat[i])
    {
        spritetype* pSprite = &sprite[i]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
        if (pSprite->extra <= 0 || !pXSprite->busyTime || pXSprite->isTriggered)
            continue;

        gTrackingConditionsList = (TRACKING_CONDITION*)Brealloc(gTrackingConditionsList, (nCount + 1) * sizeof(TRACKING_CONDITION));
        if (!gTrackingConditionsList)
            break;

        TRACKING_CONDITION* pTr = &gTrackingConditionsList[nCount];
        pTr->objects = new(OBJECT_LIST);

        if (pXSprite->rxID >= kChannelUser)
        {
            for (j = 0; j < numsectors; j++)
            {
                for (s = headspritesect[j]; s >= 0; s = nextspritesect[s])
                {
                    spritetype* pObj = &sprite[s];
                    if (pObj->extra <= 0 || pObj->index == pSprite->index)
                        continue;

                    XSPRITE* pXObj = &xsprite[pObj->extra];
                    if (pXObj->txID != pXSprite->rxID)
                        continue;

                    // check exceptions
                    switch (pObj->type)
                    {
                        case kModernCondition:
                        case kModernConditionFalse:
                        case kSwitchToggle:
                        case kSwitchOneWay:
                            break;
                        case kModernPlayerControl:
                            if (pObj->statnum == kStatModernPlayerLinker && bSaveLoad)
                            {
                                // assign player sprite after savegame loading
                                pTr->objects->Add(EVOBJ_SPRITE, pXObj->condition[3]);
                                break;
                            }
                            [[fallthrough]];
                        default:
                            pTr->objects->Add(EVOBJ_SPRITE, pObj->index);
                            break;
                    }
                }

                s = sector[j].wallptr; e = s + sector[j].wallnum - 1;
                while (s <= e)
                {
                    walltype* pObj = &wall[s];
                    if (pObj->extra > 0 && xwall[pObj->extra].txID == pXSprite->rxID)
                    {
                        // check exceptions
                        switch (pObj->type)
                        {
                            case kSwitchToggle:
                            case kSwitchOneWay:
                                break;
                            default:
                                pTr->objects->Add(EVOBJ_WALL, s);
                                break;
                        }
                    }

                    s++;
                }

                sectortype* pObj = &sector[j];
                if (pObj->extra > 0 && xsector[pObj->extra].txID == pXSprite->rxID)
                    pTr->objects->Add(EVOBJ_SECTOR, j);
            }
        }

        // allow self tracking
        if (!pTr->objects->Length())
            pTr->objects->Add(EVOBJ_SPRITE, pSprite->index);

        pTr->id = pSprite->extra;
        nCount++;
    }

    if (!nCount)
        conditionsTrackingClear();

    gTrackingConditionsListLength = nCount;
}

void conditionsTrackingClear()
{
    int i = gTrackingConditionsListLength;
    if (gTrackingConditionsList)
    {
        while (--i >= 0)
        {
            TRACKING_CONDITION* pTr = &gTrackingConditionsList[i];
            if (pTr->objects)
            {
                delete(pTr->objects);
                pTr->objects = nullptr;
            }
        }

        Bfree(gTrackingConditionsList);
    }

    gTrackingConditionsList = nullptr;
    gTrackingConditionsListLength = 0;

}

void conditionsTrackingProcess()
{
    EVENT evn; OBJECT* o;
    evn.funcID = kCallbackMax; evn.cmd = kCmdOn; evn.causer = kCauserGame;
    TRACKING_CONDITION* pTr;
    XSPRITE* pXSpr;
    
    int i = gTrackingConditionsListLength;
    int t;

    while (--i >= 0)
    {
        pTr = &gTrackingConditionsList[i];  pXSpr = &xsprite[pTr->id];
        if (pSpr->xspr.locked || pSpr->xspr.isTriggered || ++pSpr->xspr.busy < pSpr->xspr.busyTime)
            continue;

        pSpr->xspr.busy = 0;
        t = pTr->objects->Length(); o = pTr->objects->Ptr();
        while (--t >= 0)
        {
            evn.type = o->type; evn.index = o->index;
            useCondition(&sprite[pSpr->xspr.reference], pXSpr, &evn);
            o++;
        }
    }
}

void conditionsLinkPlayer(XSPRITE* pXCtrl, PLAYER* pPlay)
{
    int i = gTrackingConditionsListLength;
    int t;
    OBJECT* o;

    // search for player control sprite and replace it with actual player sprite
    while (--i >= 0)
    {
        TRACKING_CONDITION* pTr = &gTrackingConditionsList[i];
        XSPRITE* pXTrack = &xsprite[pTr->id];
        if (pXTrack->rxID != pXCtrl->txID) continue;
        if ((t = pTr->objects->Length()) <= 0)
            continue;

        o = pTr->objects->Ptr();
        while (--t >= 0)
        {
            if (o->type == OBJ_SPRITE && o->index == pXCtrl->reference)
            {
                o->index = pPlay->nSprite;
                break;
            }

            o++;
        }
    }
}

// this updates index of object in all conditions
void conditionsUpdateIndex(int oType, int oldIndex, int newIndex)
{
    int i = gTrackingConditionsListLength;
    int t;
    OBJECT* o;

    // update index in tracking conditions first
    while (--i >= 0)
    {
        TRACKING_CONDITION* pTr = &gTrackingConditionsList[i];
        if ((t = pTr->objects->Length()) <= 0)
            continue;

        o = pTr->objects->Ptr();
        while (--t >= 0)
        {
            if (o->type == oType && o->index == oldIndex)
            {
                o->index = newIndex;
                break;
            }

            o++;
        }
    }


    int oldSerial = EventObject(oType, oldIndex);
    int newSerial = EventObject(oType, newIndex);

    // then update serials everywhere
    for (i = headspritestat[kStatModernCondition]; i >= 0; i = nextspritestat[i])
    {
        XSPRITE* pXSpr = &xsprite[sprite[i].extra];
        if (pSpr->xspr.condition[0] == oldSerial)        pSpr->xspr.condition[0] = newSerial;
        if (pSpr->xspr.condition[1] == oldSerial)        pSpr->xspr.condition[1] = newSerial;
        if (pSpr->xspr.condition[2] == oldSerial)        pSpr->xspr.condition[2] = newSerial;
        if (pSpr->xspr.condition[3] == oldSerial)       pSpr->xspr.condition[3] = newSerial;
    }


    return;
}

// for showing errors in external code
void conditionsError(XSPRITE* pXSprite, const char* pFormat, ...)
{
    char buffer[512];

    pCond = &sprite[pXSprite->reference]; pXCond = pXSprite;
    arg1 = pCond->xspr.data2, arg2 = pCond->xspr.data3, arg3 = pCond->xspr.data4;
    cmpOp = pCond->spr.cstat;
    pEntry = nullptr;

    va_list args;
    va_start(args, pFormat);
    vsprintf(buffer, pFormat, args);
    va_end(args);

    Error(buffer);
}


void useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT* pEvn)
{
    // if it's a tracking condition, it must ignore all the commands sent from objects
    if (pXSource->busyTime && pEvn->funcID != kCallbackMax)
        return;

    bool ok = false;
    bool delayBefore = false, flag8 = false;
    pCond = pSource; pXCond = pXSource;
    pEntry = nullptr;

    if (pCond->xspr.waitTime)
    {
        delayBefore     = (pCond->spr.flags & kModernTypeFlag64);
        flag8           = (pCond->spr.flags & kModernTypeFlag8);

        if (pCond->xspr.restState == 0)
        {
            if (delayBefore)
            {
                // start waiting
                evPost(pCond->spr.index, EVOBJ_SPRITE, EVTIME2TICKS(pCond->xspr.waitTime), (COMMAND_ID)kCmdRepeat, pEvn->causer);
                pCond->xspr.restState = 1;

                if (flag8)
                    ReceiveObjects(pEvn); // receive current objects and hold it

                return;
            }
            else
            {
                ReceiveObjects(pEvn); // receive current objects and continue
            }
        }
        else if (pEvn->cmd == kCmdRepeat)
        {
            // finished the waiting
            if (delayBefore)
                pCond->xspr.restState = 0;
        }
        else
        {
            if ((delayBefore && !flag8) || (!delayBefore && flag8))
                ReceiveObjects(pEvn); // continue receiving actual objects while waiting

            return;
        }
    }
    else
    {
        ReceiveObjects(pEvn); // receive current objects and continue
    }

    if (pCond->xspr.restState == 0)
    {
        if (!pCond->xspr.data1) ok = true;
        else if (rngok(pCond->xspr.data1, 0, gNumConditions))
        {
            cmpOp = pCond->spr.cstat;       PUSH = rngok(pCond->xspr.command, kCmdPush, kCmdPush + kPushRange);
            arg1 = pCond->xspr.data2;		arg2 = pCond->xspr.data3;
            arg3 = pCond->xspr.data4;		pEvent = pEvn;

            UnSerialize(pCond->spr.condition[0], &objType, &objIndex);
            pEntry = &gConditions[pCond->xspr.data1];
            ok = pEntry->pCaller->pFunc();
        }
        else
        {
            errCondNotImplemented();
            return;
        }

        if ((ok) ^ (pCond->spr.type == kModernConditionFalse))
        {
            pCond->xspr.state = 1;
            if (pCond->xspr.waitTime && !delayBefore) // delay after checking
            {
                // start waiting
                evPost(pCond->spr.index, EVOBJ_SPRITE, EVTIME2TICKS(pCond->xspr.waitTime), (COMMAND_ID)kCmdRepeat, pEvn->causer);
                pCond->xspr.restState = 1;
                return;
            }
        }
        else
        {
            pCond->xspr.state = 0;
        }
    }
    else if (pEvn->cmd == kCmdRepeat)
    {
        pCond->xspr.restState = 0;
    }
    else
    {
        return;
    }

    if (pCond->xspr.state)
    {
        // trigger once per result?
        if (pCond->spr.flags & kModernTypeFlag4)
        {
            if (pCond->xspr.unused2)
                return;

            pCond->xspr.unused2 = 1;
        }

        if (pCond->xspr.triggerOnce)
        #ifdef CONDITIONS_USE_BUBBLE_ACTION
            conditionsBubble(pXCond, conditionsSetIsTriggered, true);
        #else
            conditionsSetIsTriggered(pXCond, true);
        #endif

        if (rngok(pCond->xspr.command, kCmdPop, kCmdPop + kPushRange))
            Restore(); // change focus to the saved object

        // send command to rx bucket
        if (pCond->xspr.txID)
        {
            UnSerialize(pCond->spr.condition[0], &objType, &objIndex);
            evSend(pCond->spr.index, EVOBJ_SPRITE, pCond->xspr.txID, (COMMAND_ID)pCond->xspr.command, (objType == EVOBJ_SPRITE) ? objIndex : kCauserGame);
        }

        if (pCond->spr.flags)
        {
            // send it for object that currently in the focus
            if (pCond->spr.flags & kModernTypeFlag1)
            {
                TriggerObject(pCond->spr.condition[0]);
                if ((pCond->spr.flags & kModernTypeFlag2) && pCond->spr.condition[0] == pCond->spr.condition[1])
                    return;
            }

            // send it for initial object
            if (pCond->spr.flags & kModernTypeFlag2)
            {
                TriggerObject(pCond->spr.condition[1]);
            }
        }
    }
    else
    {
        pCond->xspr.unused2 = 0;
    }
}

#ifdef CONDITIONS_USE_BUBBLE_ACTION
void conditionsBubble(XSPRITE* pXStart, void(*pActionFunc)(XSPRITE*, int), int nValue)
{
    int i, j;

    pActionFunc(pXStart, nValue);

    // perform action for whole branch from bottom to top while there is no forks
    for (i = headspritestat[kStatModernCondition]; i >= 0; i = nextspritestat[i])
    {
        XSPRITE* pXSprite = &xsprite[sprite[i].extra];
        if (pXSprite->txID != pXStart->rxID)
            continue;

        for (j = headspritestat[kStatModernCondition]; j >= 0; j = nextspritestat[j])
        {
            XSPRITE* pXSprite2 = &xsprite[sprite[j].extra];
            if (pXSprite2->rxID == pXStart->rxID && pXSprite2->txID != pXStart->txID)
                break; // fork found
        }

        if (j < 0)
            conditionsBubble(pXSprite, pActionFunc, nValue);
    }
}
void conditionsSetIsTriggered(DBloodActor* actor, int nValue)
{
    actor->xspr.isTriggered = nValue;
    evKillActor(actor);
}

void conditionsSetIsLocked(DBloodActor* actor, int nValue)
{ 
    actor->xspr.locked        = nValue;
    actor->xspr.restState     = 0;
    actor->xspr.state         = 0;
    evKillActor(actor);
    if (actor->xspr.busyTime)
        actor->xspr.busy = 0;
}
#endif

END_BLD_NS
