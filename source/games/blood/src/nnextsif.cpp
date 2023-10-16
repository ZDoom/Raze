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

// these two arrays are needed to clear the 'seen' flag for checked objects
static TArray<FGameTexture*> texSeenToClear;
static TArray<sectortype*> sectorSeenToClear;


enum
{
	kPushRange = 3,
	kCmdPush = 64,
	kCmdPop = 100,
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

static const char* const gCondErrors[] =
{
	"Object #%d (o.type(): %d) is not a %s!",
	"%d is not condition serial!",
	"Unknown object type %d, index %d.",
	"Invalid arguments passed.",
	"Unsupported %s type %d, extra: %d.",
	"Condition is not implemented.",
};

/** GLOBAL coditions table
********************************************************************************/
struct CONDITION_INFO;
struct EvalContext final
{
	EventObject o = EventObject(nullptr); // focus object

	DBloodActor* pCond = nullptr;		// current condition
	int arg1 = 0, arg2 = 0, arg3 = 0;							// arguments of current condition (data2, data3, data4)
	int cmpOp = 0;												// current comparison operator (condition sprite cstat)
	CONDITION_INFO* pEntry = nullptr;								// current condition db entry
	EVENT* pEvent = nullptr;										// current event
	bool PUSH = false;											// current stack push status (kCmdNumberic)
	bool xAvail = false;											// x-object indicator

	DBloodPlayer* pPlayer = nullptr;										// player in the focus
	DBloodActor* pSpr = nullptr;
	walltype* pWall = nullptr;
	XWALL* pXWall = nullptr;
	sectortype* pSect = nullptr;
	XSECTOR* pXSect = nullptr;

	int type() const { return o.type(); }
	bool isActor() const { return o.isActor(); }
	bool isSector() const { return o.isSector(); }
	bool isWall() const { return o.isWall(); }
	/*
	DBloodActor* actor() const { return o.actor(); }
	sectortype* sector() const { return o.sector(); }
	walltype* wall() const { return o.wall(); }
	*/

	int oindex()
	{
		return o.type() == EventObject::Actor ? o.actor()->time : o.rawindex();
	}

	bool Cmp(EventObject ob1, EventObject ob2, EventObject ob3)
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


	bool Cmp(int val)
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

	bool Cmp(int val, int nArg1, int nArg2)
	{
		arg1 = nArg1;
		arg2 = nArg2;

		return Cmp(val);
	}


	void Push(EventObject o)
	{
		// focus on object
		pCond->condition[0] = o;
		if (pCond->xspr.command > kCmdPush)
		{
			// copy in additional slots
			switch (pCond->xspr.command - kCmdPush)
			{
			case 1:
				pCond->condition[2] = pCond->condition[0];
				break;
			case 2:
				pCond->condition[3] = pCond->condition[0];
				break;
			}
		}
	}

	inline void Push(DBloodActor* a) { Push(EventObject(a)); }
	inline void Push(walltype* a) { Push(EventObject(a)); }
	inline void Push(sectortype* a) { Push(EventObject(a)); }


	/** ERROR functions
	********************************************************************************/
	bool errCondNotImplemented(void)
	{
		Error(gCondErrors[kErrNotImplementedCond]);
		return false;
	}


	/** HELPER functions
	********************************************************************************/
	bool helperChkSprite(int nSprite)
	{
#if 0
		// inverse lookup table does not exist yet.
		if (!rngok(nSprite, 0, kMaxSprites)) return false;
		else if (PUSH) Push(EventObject::Actor, nSprite);
		return true;
#else
		return false;
#endif
	}

	bool helperChkSprite(DBloodActor* nSprite)
	{
		// inverse lookup table does not exist yet.
		if (!nSprite) return false;
		else if (PUSH) Push(nSprite);
		return true;
	}

	bool helperChkSector(int nSect)
	{
		if ((unsigned)nSect >= sector.Size()) return false;
		else if (PUSH) Push(&sector[nSect]);
		return true;
	}

	bool helperChkWall(int nWall)
	{
		if ((unsigned)nWall >= wall.Size()) return false;
		else if (PUSH) Push(&wall[nWall]);
		return true;
	}

	bool helperCmpHeight(bool ceil)
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
				nHeigh1 = std::max(abs(pXSect->onCeilZ - pXSect->offCeilZ), 1 / 256.);
				nHeigh2 = abs(pSect->ceilingz - pXSect->offCeilZ);
			}
			else
			{
				nHeigh1 = std::max(abs(pXSect->onFloorZ - pXSect->offFloorZ), 1 / 256.);
				nHeigh2 = abs(pSect->floorz - pXSect->offFloorZ);
			}
			return Cmp((int((kPercFull * nHeigh2) / nHeigh1)));
		default:
			Error(gCondErrors[kErrObjectUnsupp], "sector", pSect->type, pSect->extra);
			return false;
		}
	}

	bool helperCmpData(int nData)
	{
		switch (o.type())
		{
		case EventObject::Wall:
			return Cmp(pXWall->data);
		case EventObject::Actor:
			switch (nData)
			{
			case 1:		return Cmp(pSpr->xspr.data1);
			case 2:		return Cmp(pSpr->xspr.data2);
			case 3:		return Cmp(pSpr->xspr.data3);
			case 4:		return Cmp(pSpr->xspr.data4);
			}
			break;
		case EventObject::Sector:
			return Cmp(pXSect->data);
		}

		return Cmp(0);
	}


	bool helperChkHitscan(int nWhat)
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

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}

	bool helperChkMarker(int nWhat)
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
				Error(gCondErrors[kErrInvalidArgsPass]);
				break;
			}
		}

		return false;
	};

	bool helperChkTarget(int nWhat)
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
				Error(gCondErrors[kErrInvalidArgsPass]);
				break;
			}
		}

		if (t <= 0) return false;
		else if (PUSH) Push(pSpr->xspr.target);
		return true;
	};

	bool helperChkRoom(DBloodActor* t, int nWhat)
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
	bool gamCmpLevelMin(void) { return Cmp(gFrameCount / (kTicsPerSec * 60)); }
	bool gamCmpLevelSec(void) { return Cmp((gFrameCount / kTicsPerSec) % 60); }
	bool gamCmpLevelMsec(void) { return Cmp(((gFrameCount % kTicsPerSec) * 33) / 10); }
	bool gamCmpLevelTime(void) { return Cmp(gFrameCount); }
	bool gamCmpKillsTotal(void) { return Cmp(Level.kills.max); }
	bool gamCmpKillsDone(void) { return Cmp(Level.kills.got); }
	bool gamCmpSecretsTotal(void) { return Cmp(Level.secrets.max); }
	bool gamCmpSecretsDone(void) { return Cmp(Level.secrets.got); }
	bool gamCmpVisibility(void) { return Cmp(gVisibility); }
	bool gamChkGotpic(void)
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
	bool gamChkChance(void) { return Chance((0x10000 * arg3) / kPercFull); }
	bool gamCmpRandom(void) { return Cmp(nnExtRandom(arg1, arg2)); }
	bool gamCmpStatnumCount(void)
	{
		BloodStatIterator it(ClipRange(arg3, 0, kMaxStatus));
		int c = 0;
		while (it.Next()) c++;
		return Cmp(c); // compare counter of specific statnum sprites
	}
	bool gamCmpNumsprites(void) { return Cmp(Numsprites); }
	bool gamChkPlayerConnect(void)
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
	bool gamChkSector(void) { return helperChkSector(arg1); }
	bool gamChkWall(void) { return helperChkWall(arg1); }
	bool gamChkSprite(void) { return helperChkSprite(arg1); }



	/** SECTOR conditions
	********************************************************************************/
	bool sectCmpVisibility(void) { return Cmp(pSect->visibility); }
	bool sectChkShow2dSector(void) { return show2dsector.Check(sectindex(pSect)); }
	bool sectChkGotSector(void)
	{
		if (pSect->exflags & SECTOREX_SEEN)// this is neither network not demo sync safe!
		{
			sectorSeenToClear.Push(pSect);
			return true;
		}
		return false;
	}
	bool sectCmpFloorSlope(void) { return Cmp(pSect->floorheinum); }
	bool sectCmpCeilSlope(void) { return Cmp(pSect->ceilingheinum); }
	bool sectChkSprTypeInSect(void)
	{
		BloodSectIterator it(pSect);
		while (auto act = it.Next())
		{
			if (!Cmp(act->GetType())) continue;
			else if (PUSH) Push(act);
			return true;
		}
		return false;
	}

	bool sectCmpFloorHeight(void) { return helperCmpHeight(false); }
	bool sectCmpCeilHeight(void) { return helperCmpHeight(true); }
	bool sectChkUnderwater(void) { return pXSect->Underwater; }
	bool sectChkPaused(void) { return !pXSect->restState; }
	bool sectCmpDepth(void) { return Cmp(pXSect->Depth); }
	bool sectChkUpperRoom(void) { return helperChkRoom(barrier_cast<DBloodActor*>(pSect->upperLink), arg3); }
	bool sectChkLowerRoom(void) { return helperChkRoom(barrier_cast<DBloodActor*>(pSect->lowerLink), arg3); }



	/** WALL conditions
	********************************************************************************/
	bool wallCmpOverpicnum(void) { return Cmp(legacyTileNum(pWall->overtexture)); }
	bool wallChkShow2dWall(void) { return show2dwall.Check(wallindex(pWall)); }
	bool wallChkIsMirror(void)
	{
		return (pWall->portalflags == PORTAL_WALL_MIRROR);
	}

	bool wallChkSector(void) { return helperChkSector(pWall->sector); }
	bool wallChkNextSector(void) { return helperChkSector(pWall->nextsector); }
	bool wallChkNextWallSector(void) { return helperChkSector(pWall->nextWall()->sector); }
	bool wallChkNextWall(void) { return helperChkWall(pWall->nextwall); }
	bool wallChkPoint2(void) { return helperChkWall(pWall->point2); }





	/** MIXED OBJECT conditions
	********************************************************************************/
	bool mixChkObjSect(void) { return (o.type() == EventObject::Sector && pSect != nullptr); }
	bool mixChkObjWall(void) { return (o.type() == EventObject::Wall && pWall != nullptr); }
	bool mixChkObjSpr(void) { return (o.type() == EventObject::Actor && pSpr != nullptr); }
	bool mixChkXRange(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pWall->hasX();
		case EventObject::Actor:	return pSpr->hasX();
		case EventObject::Sector:	return pSect->hasX();
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpLotag(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->type);
		case EventObject::Actor:	return Cmp(pSpr->GetType());
		case EventObject::Sector:	return Cmp(pSect->type);
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpPicSurface(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(tilesurface(pWall->walltexture));
		case EventObject::Actor:	return Cmp(tilesurface(pSpr->spr.spritetexture()));
		case EventObject::Sector:
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
	bool mixCmpPic(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(legacyTileNum(pWall->walltexture));
		case EventObject::Actor:	return Cmp(legacyTileNum(pSpr->spr.spritetexture()));
		case EventObject::Sector:
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
	bool mixCmpPal(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->pal);
		case EventObject::Actor:	return Cmp(pSpr->spr.pal);
		case EventObject::Sector:
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
	bool mixCmpShade(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->shade);
		case EventObject::Actor:	return Cmp(pSpr->spr.shade);
		case EventObject::Sector:
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
	bool mixCmpCstat(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return (bool)((arg3) ? Cmp(pWall->cstat & EWallFlags::FromInt(arg3)) : (pWall->cstat & EWallFlags::FromInt(arg1)));
		case EventObject::Actor:	return (bool)((arg3) ? Cmp(pSpr->spr.cstat & ESpriteFlags::FromInt(arg3)) : (pSpr->spr.cstat & ESpriteFlags::FromInt(arg1)));
		case EventObject::Sector:	// !!!
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
	bool mixCmpHitag(void) // This is the Blood flags
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return (bool)((arg3) ? Cmp(pWall->hitag & arg3) : (pWall->hitag & arg1));
		case EventObject::Actor:	return (bool)((arg3) ? Cmp(pSpr->spr.flags & arg3) : (pSpr->spr.flags & arg1));
		case EventObject::Sector:	return (bool)((arg3) ? Cmp(pSect->hitag & arg3) : (pSect->hitag & arg1));
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpXrepeat(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->xrepeat);
		case EventObject::Actor:	return Cmp(int(pSpr->spr.scale.X * INV_REPEAT_SCALE));
		case EventObject::Sector:	return Cmp(pSect->floorxpan());
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpXoffset(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->xpan());
		case EventObject::Actor:	return Cmp(pSpr->spr.xoffset);
		case EventObject::Sector:	return Cmp(pSect->ceilingxpan());
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpYrepeat(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->yrepeat);
		case EventObject::Actor:	return Cmp(int(pSpr->spr.scale.Y * INV_REPEAT_SCALE));
		case EventObject::Sector:	return Cmp(pSect->floorxpan());
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpYoffset(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pWall->ypan());
		case EventObject::Actor:	return Cmp(pSpr->spr.yoffset);
		case EventObject::Sector:	return Cmp(pSect->ceilingypan());
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpData1(void) { return helperCmpData(1); }
	bool mixCmpData2(void) { return helperCmpData(2); }
	bool mixCmpData3(void) { return helperCmpData(3); }
	bool mixCmpData4(void) { return helperCmpData(4); }
	bool mixCmpRXId(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pXWall->rxID);
		case EventObject::Actor:	return Cmp(pSpr->xspr.rxID);
		case EventObject::Sector:	return Cmp(pXSect->rxID);
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpTXId(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp(pXWall->txID);
		case EventObject::Actor:	return Cmp(pSpr->xspr.txID);
		case EventObject::Sector:	return Cmp(pXSect->txID);
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkLock(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->locked;
		case EventObject::Actor:	return pSpr->xspr.locked;
		case EventObject::Sector:	return pXSect->locked;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkTriggerOn(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->triggerOn;
		case EventObject::Actor:	return pSpr->xspr.triggerOn;
		case EventObject::Sector:	return pXSect->triggerOn;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkTriggerOff(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->triggerOff;
		case EventObject::Actor:	return pSpr->xspr.triggerOff;
		case EventObject::Sector:	return pXSect->triggerOff;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkTriggerOnce(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->triggerOnce;
		case EventObject::Actor:	return pSpr->xspr.triggerOnce;
		case EventObject::Sector:	return pXSect->triggerOnce;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkIsTriggered(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->isTriggered;
		case EventObject::Actor:	return pSpr->xspr.isTriggered;
		case EventObject::Sector:	return pXSect->isTriggered;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkState(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->state;
		case EventObject::Actor:	return pSpr->xspr.state;
		case EventObject::Sector:	return pXSect->state;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpBusy(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return Cmp((kPercFull * pXWall->busy) / 65536);
		case EventObject::Actor:	return Cmp((kPercFull * pSpr->xspr.busy) / 65536);
		case EventObject::Sector:	return Cmp((kPercFull * pXSect->busy) / 65536);
		}

		return Cmp(0);
	}
	/**---------------------------------------------------------------------------**/
	bool mixChkPlayerOnly(void)
	{
		switch (o.type())
		{
		case EventObject::Wall: 	return pXWall->dudeLockout;
		case EventObject::Actor:	return pSpr->xspr.DudeLockout;
		case EventObject::Sector:	return pXSect->dudeLockout;
		}

		return false;
	}
	/**---------------------------------------------------------------------------**/
	bool mixCmpSeqID(void)
	{
		switch (o.type())
		{
		case EventObject::Actor:
			return Cmp(seqGetID(pSpr));
		case EventObject::Wall:
			switch (arg3)
			{
			default:	return seqGetID(SS_WALL, pWall) || seqGetID(SS_MASKED, pWall);
			case  1:	return seqGetID(SS_WALL, pWall);
			case  2:	return seqGetID(SS_MASKED, pWall);	// masked wall
			}
			break;
		case EventObject::Sector:
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
	bool mixCmpSeqFrame(void)
	{
		switch (o.type())
		{
		case EventObject::Actor:
			return Cmp(seqGetStatus(pSpr));
		case EventObject::Wall:
			switch (arg3)
			{
			default:	return seqGetStatus(SS_WALL, pWall) || seqGetStatus(SS_MASKED, pWall);
			case  1:	return seqGetStatus(SS_WALL, pWall);
			case  2:	return seqGetStatus(SS_MASKED, pWall);	// masked wall
			}
			break;
		case EventObject::Sector:
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
	bool mixCmpObjIndex(void) { return Cmp(oindex()); }
	bool mixCmpObjXIndex(void) { return mixCmpObjIndex(); } // are the same. This makes no sense because behavior was undefined.


	int getDigitFromValue(int nVal, int nOffs)
	{
		char t[16];
		int l = snprintf(t, 16, "%d", abs(nVal));
		if (nOffs < l && rngok(t[nOffs], 48, 58))
			return t[nOffs] - 48;

		return -1;
	}

	bool mixCmpSerials(void)
	{
		unsigned int i = 0, d;
		const EventObject* const serials = pCond->condition;
		EventObject t[3];

		while (i < 3)
		{
			d = getDigitFromValue(arg1, i);
			if (!rngok(d, 1, 4))
			{
				Error(gCondErrors[kErrInvalidArgsPass]);
				return false;
			}

			t[i++] = serials[d - 1];
		}

		return Cmp(t[0], t[1], t[2]);

	}
	bool mixChkEventCauser(void) { return helperChkSprite(pEvent->initiator); }
	bool mixCmpEventCmd(void) { return Cmp(pEvent->cmd); }





	/** SPRITE conditions
	********************************************************************************/
	bool sprCmpAng(void) { return Cmp((arg3 == 0) ? (pSpr->spr.Angles.Yaw.Buildang() & 2047) : pSpr->spr.Angles.Yaw.Buildang()); }
	bool sprChkShow2dSprite(void) { return !!(pSpr->spr.cstat2 & CSTAT2_SPRITE_MAPPED); }
	bool sprCmpStatnum(void) { return Cmp(pSpr->spr.statnum); }
	bool sprChkRespawn(void) { return ((pSpr->spr.flags & kHitagRespawn) || pSpr->spr.statnum == kStatRespawn); }
	bool sprCmpSlope(void) { return Cmp(spriteGetSlope(pSpr)); }
	bool sprCmpClipdist(void) { return Cmp(int(pSpr->clipdist * 16)); }
	bool sprChkOwner(void) { return helperChkSprite(pSpr->ownerActor); }
	bool sprChkSector(void) { return helperChkSector(sectindex(pSpr->spr.sectp)); }
	bool sprCmpVelocityNew(void)
	{
		switch (arg3)
		{
		case 0:		return (Cmp(FloatToFixed(pSpr->vel.X)) || Cmp(FloatToFixed(pSpr->vel.Y)) || Cmp(FloatToFixed(pSpr->vel.Z)));
		case 1:		return Cmp(FloatToFixed(pSpr->vel.X));
		case 2:		return Cmp(FloatToFixed(pSpr->vel.Y));
		case 3:		return Cmp(FloatToFixed(pSpr->vel.Z));
		}

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}
	bool sprCmpChkVelocity(void)
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

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}

	/**---------------------------------------------------------------------------**/
	bool sprChkUnderwater(void) { return IsUnderwaterSector(pSpr->sector()); }
	bool sprChkDmgImmune(void)
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
	bool sprChkHitscanCeil(void) { return helperChkHitscan(1); }
	bool sprChkHitscanFloor(void) { return helperChkHitscan(2); }
	bool sprChkHitscanWall(void) { return helperChkHitscan(3); }
	bool sprChkHitscanSpr(void) { return helperChkHitscan(4); }
	bool sprChkHitscanMasked(void) { return helperChkHitscan(5); }
	bool sprChkIsTarget(void)
	{
		BloodStatIterator it(kStatDude);
		while (auto act = it.Next())
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
	bool sprCmpHealth(void)
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
	bool sprChkTouchCeil(void)
	{
		if ((pSpr->hit.ceilhit.type != kHitSector)) return false;
		else if (PUSH) Push(pSpr->hit.ceilhit.hitSector);
		return true;
	}
	/**---------------------------------------------------------------------------**/
	bool sprChkTouchFloor(void)
	{
		if ((pSpr->hit.florhit.type != kHitSector)) return false;
		else if (PUSH) Push(pSpr->hit.florhit.hitSector);
		return true;
	}
	/**---------------------------------------------------------------------------**/
	bool sprChkTouchWall(void)
	{
		if ((pSpr->hit.hit.type != kHitWall)) return false;
		else if (PUSH) Push(pSpr->hit.hit.hitWall);
		return true;
	}
	/**---------------------------------------------------------------------------**/
	bool sprChkTouchSpite(void)
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
	bool sprCmpBurnTime(void)
	{
		int t = (pSpr->IsDudeActor()) ? 2400 : 1200;
		if (!Cmp((kPercFull * pSpr->xspr.burnTime) / t)) return false;
		else if (PUSH && pSpr->xspr.burnSource) Push(pSpr->xspr.burnSource);
		return true;
	}
	/**---------------------------------------------------------------------------**/
	bool sprChkIsFlareStuck(void)
	{
		BloodStatIterator it(kStatFlare);
		while (auto act = it.Next())
		{
			if (!act->hasX() || (act->spr.flags & kHitagFree))
				continue;

			if (act->xspr.target != pSpr) continue;
			else if (PUSH) Push(act);
			return true;
		}

		return false;
	}
	bool sprCmpMass(void) { return Cmp(getSpriteMassBySize(pSpr)); }
	bool sprChkTarget(void) { return helperChkSprite(pSpr->xspr.target); }




	/** GLOBAL DUDE functions
	********************************************************************************/
	bool gdudChkHaveTargets(void)
	{
		DBloodActor* targ = pSpr->xspr.target;
		if (!targ) return false;
		else if (!targ->IsDudeActor() && targ->GetType() != kMarkerPath) return false;
		else if (PUSH) Push(targ);
		return true;
	};
	bool gdudChkInAiFight(void) { return aiFightDudeIsAffected(pSpr); };
	bool gdudChkTargetDistance(void) { return helperChkTarget(1); };
	bool gdudChkTargetCansee(void) { return helperChkTarget(2); };
	bool gdudChkTargetCanseePerip(void) { return helperChkTarget(3); };
	bool gdudChkFlagPatrol(void) { return pSpr->xspr.dudeFlag4; };
	bool gdudChkFlagDeaf(void) { return pSpr->xspr.dudeDeaf; };
	bool gdudChkFlagBlind(void) { return pSpr->xspr.dudeGuard; };
	bool gdudChkFlagAlarm(void) { return pSpr->xspr.dudeAmbush; };
	bool gdudChkFlagStealth(void) { return ((pSpr->xspr.modernFlags & kDudeFlagStealth) != 0); };
	bool gdudChkMarkerBusy(void) { return helperChkMarker(1); };
	bool gdudChkMarkerReached(void) { return helperChkMarker(2); };
	bool gdudCmpSpotProgress(void)
	{
		if (!pSpr->xspr.dudeFlag4 || !pSpr->xspr.target || pSpr->xspr.target->GetType() != kMarkerPath)	return Cmp(0);
		else if (!(pSpr->xspr.modernFlags & kDudeFlagStealth) || !valueIsBetween(pSpr->xspr.data3, 0, kMaxPatrolSpotValue))	return Cmp(0);
		else return Cmp((kPercFull * pSpr->xspr.data3) / kMaxPatrolSpotValue);
	};
	bool gdudChkLockout(void) { return getDudeInfo(pSpr->GetType())->lockOut; };
	bool gdudCmpAiStateType(void) { return Cmp(pSpr->xspr.aiState->stateType); };
	bool gdudCmpAiStateTimer(void) { return Cmp(pSpr->xspr.stateTimer); };





	/** CUSTOM DUDE functions
	********************************************************************************/
	bool cdudChkLeechThrown(void)
	{
		DCustomDude* pDude = cdudeGet(pSpr);
		if (!pDude->IsLeechBroken() && pDude->pLeech)
			return helperChkSprite(pDude->pLeech);
		return false;
	};
	bool cdudChkLeechDead(void)
	{
		DCustomDude* pDude = cdudeGet(pSpr);
		if (pDude->IsLeechBroken()) return true;
		else if (PUSH && pDude->pLeech) Push(pDude->pLeech);
		return false;
	};
	bool cdudCmpSummoned(void)
	{
		auto& pSlaves = cdudeGet(pSpr)->pSlaves;
		return Cmp(pSlaves.Size());
	};
	bool cdudChkIfAble(void)
	{
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
			Error(gCondErrors[kErrInvalidArgsPass]);
			break;
		}
		return false;
	};
	bool cdudCmpDispersion(void)
	{
		CUSTOMDUDE_WEAPON* pWeapon = cdudeGet(pSpr)->pWeapon;
		if (!pWeapon)
			return Cmp(0);

		return Cmp(pWeapon->dispersion[0]);
	};





	/** PLAYER functions
	********************************************************************************/
	bool plyCmpConnected(void)
	{
		if (!Cmp(pPlayer->pnum + 1)) return false;
		else return helperChkSprite(pPlayer->GetActor());
	}
	bool plyCmpTeam(void) { return Cmp(pPlayer->teamId + 1); }
	bool plyChkHaveKey(void) { return (valueIsBetween(arg1, 0, 8) && pPlayer->hasKey[arg1 - 1]); }
	bool plyChkHaveWeapon(void) { return (valueIsBetween(arg1, 0, 15) && pPlayer->hasWeapon[arg1 - 1]); }
	bool plyCmpCurWeapon(void) { return Cmp(pPlayer->curWeapon); }
	bool plyCmpPackItemAmount(void) { return (valueIsBetween(arg1, 0, 6) && Cmp(pPlayer->packSlots[arg1 - 1].curAmount)); }
	bool plyChkPackItemActive(void) { return (valueIsBetween(arg1, 0, 6) && pPlayer->packSlots[arg1 - 1].isActive); }
	bool plyCmpPackItemSelect(void) { return Cmp(pPlayer->packItemId + 1); }
	bool plyCmpPowerupAmount(void)
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
	bool plyChkKillerSprite(void) { return helperChkSprite(pPlayer->fragger); }
	bool plyChkKeyPress(void)
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
	bool plyChkRunning(void) { return pPlayer->isRunning; }
	bool plyChkFalling(void) { return pPlayer->fallScream; }
	bool plyCmpLifeMode(void) { return Cmp(pPlayer->lifeMode + 1); }
	bool plyCmpPosture(void) { return Cmp(pPlayer->posture + 1); }
	bool plyCmpKillsCount(void) { return Cmp(pPlayer->fragCount); }
	bool plyChkAutoAimTarget(void) { return helperChkSprite(pPlayer->aimTarget); }
	bool plyChkVoodooTarget(void) { return helperChkSprite(pPlayer->voodooTarget); }

	bool plyCmpQavWeapon(void) { return Cmp(pPlayer->weaponQav); }
	bool plyCmpQavScene(void) { return Cmp(pPlayer->sceneQav); }
	bool plyChkGodMode(void) { return (pPlayer->godMode || powerupCheck(pPlayer, kPwUpDeathMask)); }
	bool plyChkShrink(void) { return isShrunk(pSpr); }
	bool plyChkGrown(void) { return isGrown(pSpr); }
	bool plyCmpArmor(void)
	{
		if (valueIsBetween(arg3, 0, 4))
			return Cmp((pPlayer->armor[arg3 - 1] * kPercFull) / 1600);

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}
	bool plyCmpAmmo(void)
	{
		if (valueIsBetween(arg3, 0, 12))
			return Cmp(pPlayer->ammoCount[arg3 - 1]);

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}
	bool plyChkSpriteItOwns(void)
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

		Error(gCondErrors[kErrInvalidArgsPass]);
		return false;
	}


	bool DefaultResult();
	bool CheckGeneric();
	bool CheckSector();
	bool CheckWall();
	bool CheckDude();
	bool CheckCustomDude();
	bool CheckPlayer();
	bool CheckSprite();
	bool CheckObject();


	/** CONDITION INTERFACE FUNCTIONS
	********************************************************************************/

	void Error(const char* pFormat, ...);

	void ReceiveObjects(EVENT* pFrom)
	{
		bool srcIsCondition = false;

		o = pFrom->target;
		if (o.type() == EventObject::Actor && pCond != o.actor())
			srcIsCondition = (o.actor()->spr.statnum == kStatModernCondition);

		if (!srcIsCondition)
		{
			// save object serials in the "stack"
			pCond->condition[0] =
			pCond->condition[1] =
			pCond->condition[2] =
			pCond->condition[3] = o;
		}
		else
		{
			// or grab serials of objects from previous conditions
			auto actor = o.actor();
			pCond->condition[0] = actor->condition[0];
			pCond->condition[1] = actor->condition[1];
			pCond->condition[2] = actor->condition[2];
			pCond->condition[3] = actor->condition[3];
		}
	}

};


struct CHECKFUNC_INFO
{
	bool (EvalContext::* pFunc)(void);              // function to call the condition
	unsigned int type;	// type of condition
	const char* name;                   // for errors output
};


struct CONDITION_INFO
{
	bool (EvalContext::* pFunc)(void);              // condition function
	uint16_t id;	// condition id
	uint8_t type;	// type of condition
	bool isBool;    // false = do comparison using Cmp()
	bool xReq;      // is x-object required?
	CHECKFUNC_INFO* pCaller = nullptr;	// provided for easy access and must be set during init
};



/** A LIST OF CONDITION FUNCTION CALLERS
********************************************************************************/
static CHECKFUNC_INFO gCheckFuncInfo[] =
{
	{ &EvalContext::CheckGeneric,				CGAM,		"Game"			},
	{ &EvalContext::CheckObject,				CMIX,		"Mixed"			},
	{ &EvalContext::CheckWall,		    		CWAL,		"Wall"          },
	{ &EvalContext::CheckSector, 				CSEC,		"Sector"        },
	{ &EvalContext::CheckPlayer,				CPLY,		"Player"        },
	{ &EvalContext::CheckDude,			    	CDUDG,		"Dude"          },
	{ &EvalContext::CheckCustomDude,			CDUDC,		"Custom Dude"   },
	{ &EvalContext::CheckSprite,				CSPR,		"Sprite"        },
	{ &EvalContext::CheckGeneric,				CNON,		"Unknown"       },
};

static int qsSortCheckFuncInfo(CHECKFUNC_INFO* ref1, CHECKFUNC_INFO* ref2)
{
	return ref1->type - ref2->type;
}

bool EvalContext::DefaultResult() { return (pEntry->isBool) ? false : Cmp(0); }
bool EvalContext::CheckGeneric() { return (this->*(pEntry->pFunc))(); }
bool EvalContext::CheckSector()
{
	if (o.type() == EventObject::Sector)
	{
		pSect = o.sector();
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
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "sector");
		return false;
	}
	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckWall()
{
	if (o.type() == EventObject::Wall)
	{
		pWall = o.wall();
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
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "wall");
		return false;
	}

	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckDude()
{
	if (o.type() == EventObject::Actor)
	{
		pSpr = o.actor();
		if ((!pSpr->IsDudeActor() && pSpr->GetType() != kThingBloodChunks) || pSpr->IsPlayerActor())
		{
			Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "dude");
			return false;
		}

		if (pSpr->hasX())
		{
			xAvail = true;
		}
		else
		{
			xAvail = false;
			if (pEntry->xReq)
				return DefaultResult();
		}
	}
	else
	{
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "sprite");
		return false;
	}

	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckCustomDude()
{
	if (o.type() == EventObject::Actor)
	{
		pSpr = o.actor();
		switch (pSpr->GetType())
		{
		case kThingBloodChunks:
			if (pSpr->spr.inittype == kDudeModernCustom) break;
			Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "custom dude");
			return false;
		case kDudeModernCustom:
			break;
		default:
			Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "custom dude");
			return false;
		}

		if (pSpr->hasX())
		{
			xAvail = true;
		}
		else
		{
			xAvail = false;
			if (pEntry->xReq)
				return DefaultResult();
		}
	}
	else
	{
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "sprite");
		return false;
	}

	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckPlayer()
{
	int i;
	if (o.type() == EventObject::Actor)
	{
		pSpr = o.actor();
		for (i = connecthead; i >= 0; i = connectpoint2[i])
		{
			if (pSpr != getPlayer(i)->GetActor()) continue;
			pPlayer = getPlayer(i);
			break;
		}

		// there is no point to check unlinked or disconnected players
		if (i < 0)
			return DefaultResult();

		if (pSpr->hasX())
		{
			xAvail = true;
		}
		else
		{
			xAvail = false;
			if (pEntry->xReq)
				return DefaultResult();
		}
	}
	else
	{
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "player");
		return false;
	}

	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckSprite()
{
	if (o.type() == EventObject::Actor)
	{
		pSpr = o.actor();
		if (pSpr->hasX())
		{
			xAvail = true;
		}
		else
		{
			xAvail = false;
			if (pEntry->xReq)
				return DefaultResult();
		}
	}
	else
	{
		Error(gCondErrors[kErrInvalidObject], oindex(), o.type(), "sprite");
		return false;
	}

	return (this->*(pEntry->pFunc))();
}

bool EvalContext::CheckObject()
{
	switch (o.type())
	{
	case EventObject::Wall:    return CheckWall();
	case EventObject::Sector:  return CheckSector();
	case EventObject::Actor:  return CheckSprite();
	}

	// conditions can only work with objects in the switch anyway...
	Error(gCondErrors[kErrUnknownObject], o.type(), oindex());
	return false;
}

void EvalContext::Error(const char* pFormat, ...)
{
	FString buffer2;

	FString condType = (pEntry) ? gCheckFuncInfo[pEntry->type].name : "Unknown";
	condType = condType.MakeUpper();

	va_list args;
	va_start(args, pFormat);
	buffer2.Format(pFormat, args);
	va_end(args);

	I_Error(
		"\n"
		"ERROR IN %s CONDITION ID #%d:\n"
		"%s\n\n"
		"Debug information:\n"
		"--------------------------------------------\n"
		"Condition sprite  =  %d,  RX ID  =  %d,  TX ID  =  %d\n"
		"Arguments  =  %d,  %d,  %d\n"
		"Operator  =  %d\n",
		condType, pCond->xspr.data1, buffer2.GetChars(),
		pCond->time,
		pCond->xspr.rxID,
		pCond->xspr.txID,
		arg1,
		arg2,
		arg3,
		cmpOp &= ~0x8000
	);

}


static TArray<CONDITION_INFO> gConditions;

/** TABLE OF CONDITION FUNCTIONS
********************************************************************************/
static CONDITION_INFO gConditionsList[] =
{
	{ &EvalContext::gamCmpLevelMin,	    		1,		CGAM,		false,		false },	// compare level minutes
	{ &EvalContext::gamCmpLevelSec,		    	2,		CGAM,		false,		false },	// compare level seconds
	{ &EvalContext::gamCmpLevelMsec,			3,		CGAM,		false,		false },	// compare level mseconds
	{ &EvalContext::gamCmpLevelTime,			4,		CGAM,		false,		false },	// compare level time (unsafe)
	{ &EvalContext::gamCmpKillsTotal,			5,		CGAM,		false,		false },	// compare current global kills counter
	{ &EvalContext::gamCmpKillsDone,			6,		CGAM,		false,		false },	// compare total global kills counter
	{ &EvalContext::gamCmpSecretsDone,		    7,		CGAM,		false,		false },	// compare how many secrets found
	{ &EvalContext::gamCmpSecretsTotal, 		8,		CGAM,		false,		false },	// compare total secrets
	{ &EvalContext::gamCmpVisibility,			20,		CGAM,		false,		false },	// compare global visibility value
	{ &EvalContext::gamChkGotpic,				21,		CGAM,		true,		false },	// check gotpic
	{ &EvalContext::gamChkChance,				30,		CGAM,		true,		false },	// check chance %
	{ &EvalContext::gamCmpRandom,				31,		CGAM,		false,		false },	// compare random
	{ &EvalContext::gamCmpStatnumCount,	    	47,		CGAM,		false,		false },	// compare counter of specific statnum sprites
	{ &EvalContext::gamCmpNumsprites,			48,		CGAM,		false,		false },	// compare counter of total sprites
	{ &EvalContext::gamChkSector,		        57,		CGAM,		true,		false },	// get sector N if possible
	{ &EvalContext::gamChkWall,		            58,		CGAM,		true,		false },	// get wall N if possible
	{ &EvalContext::gamChkSprite,		        59,		CGAM,		true,		false },	// get sprite N if possible
	{ &EvalContext::gamChkPlayerConnect,		60,		CGAM,		true,		false },	// check if player N connected
	/**--------------------------------------------------------------**/
	{ &EvalContext::mixChkObjSect,  			100,	CMIX,		true,		false },
	{ &EvalContext::mixChkObjWall,	    		105,	CMIX,		true,		false },
	{ &EvalContext::mixChkObjSpr,				110,	CMIX,		true,		false },
	{ &EvalContext::mixChkXRange,				115,	CMIX,		true,		false },
	{ &EvalContext::mixCmpLotag,				120,	CMIX,		false,		false },
	{ &EvalContext::mixCmpPicSurface,			124,	CMIX,		false,		false },
	{ &EvalContext::mixCmpPic,			    	125,	CMIX,		false,		false },
	{ &EvalContext::mixCmpPal,				    126,	CMIX,		false,		false },
	{ &EvalContext::mixCmpShade,				127,	CMIX,		false,		false },
	{ &EvalContext::mixCmpCstat,				128,	CMIX,		false,		false },
	{ &EvalContext::mixCmpHitag,				129,	CMIX,		false,		false },
	{ &EvalContext::mixCmpXrepeat,  			130,	CMIX,		false,		false },
	{ &EvalContext::mixCmpXoffset,	    		131,	CMIX,		false,		false },
	{ &EvalContext::mixCmpYrepeat,		    	132,	CMIX,		false,		false },
	{ &EvalContext::mixCmpYoffset,			    133,	CMIX,		false,		false },
	{ &EvalContext::mixCmpData1,				141,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpData2,				142,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpData3,				143,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpData4,				144,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpRXId, 				150,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpTXId,	    			151,	CMIX,		false,		true  },
	{ &EvalContext::mixChkLock,		    		152,	CMIX,		true,		true  },
	{ &EvalContext::mixChkTriggerOn,			153,	CMIX,		true,		true  },
	{ &EvalContext::mixChkTriggerOff,			154,	CMIX,		true,		true  },
	{ &EvalContext::mixChkTriggerOnce,	    	155,	CMIX,		true,		true  },
	{ &EvalContext::mixChkIsTriggered,		    156,	CMIX,		true,		true  },
	{ &EvalContext::mixChkState,				157,	CMIX,		true,		true  },
	{ &EvalContext::mixCmpBusy,		    		158,	CMIX,		false,		true  },
	{ &EvalContext::mixChkPlayerOnly,			159,	CMIX,		true,		true  },
	{ &EvalContext::mixCmpSeqID,				170,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpSeqFrame,		    	171,	CMIX,		false,		true  },
	{ &EvalContext::mixCmpObjIndex,			    195,	CMIX,		false,		false },
	{ &EvalContext::mixCmpObjXIndex,			196,	CMIX,		false,		false },
	{ &EvalContext::mixCmpSerials,	    	    197,	CMIX,		false,		false },
	{ &EvalContext::mixChkEventCauser,	    	198,	CMIX,		true,		false },	// check event causer
	{ &EvalContext::mixCmpEventCmd,			    199,	CMIX,		false,		false },	// this condition received N command?
	/**--------------------------------------------------------------**/
	{ &EvalContext::wallCmpOverpicnum,			200,	CWAL,		false,		false },
	{ &EvalContext::wallChkShow2dWall,			201,	CWAL,		true,		false },	// wall on the minimap
	{ &EvalContext::wallChkSector,				205,	CWAL,		true,		false },
	{ &EvalContext::wallChkIsMirror,			210,	CWAL,		true,		false },
	{ &EvalContext::wallChkNextSector,			215,	CWAL,		true,		false },
	{ &EvalContext::wallChkNextWall,			220,	CWAL,		true,		false },
	{ &EvalContext::wallChkPoint2,				221,	CWAL,		true,		false },
	{ &EvalContext::wallChkNextWallSector,		225,	CWAL,		true,		false },
	/**--------------------------------------------------------------**/
	{ &EvalContext::sectCmpVisibility,			300,	CSEC,		false,		false },	// compare visibility
	{ &EvalContext::sectChkShow2dSector,		301,	CSEC,		true,		false },	// sector on the minimap
	{ &EvalContext::sectChkGotSector,			302,	CSEC,		true,		false },	// sector on the screen
	{ &EvalContext::sectCmpFloorSlope,			305,	CSEC,		false,		false },	// compare floor slope
	{ &EvalContext::sectCmpCeilSlope,			306,	CSEC,		false,		false },	// compare ceil slope
	{ &EvalContext::sectChkSprTypeInSect,		310,	CSEC,		true,		false },	// check is sprite with lotag N in sector
	{ &EvalContext::sectChkUnderwater,			350,	CSEC,		true,		true  },	// sector is underwater?
	{ &EvalContext::sectCmpDepth,				351,	CSEC,		false,		true  },	// compare depth level
	{ &EvalContext::sectCmpFloorHeight,			355,	CSEC,		false,		true  },	// compare floor height (in %)
	{ &EvalContext::sectCmpCeilHeight,			356,	CSEC,		false,		true  },	// compare ceil height (in %)
	{ &EvalContext::sectChkPaused,				357,	CSEC,		true,		true  },	// this sector in movement?
	{ &EvalContext::sectChkUpperRoom,			358,	CSEC,		true,		false },	// get upper room sector or marker
	{ &EvalContext::sectChkLowerRoom,			359,	CSEC,		true,		false },	// get lower room sector or marker
	/**--------------------------------------------------------------**/
	{ &EvalContext::plyCmpConnected,			400,	CPLY,		false,		false },
	{ &EvalContext::plyCmpTeam,					401,	CPLY,		false,		false },
	{ &EvalContext::plyChkHaveKey,				402,	CPLY,		true,		false },
	{ &EvalContext::plyChkHaveWeapon,			403,	CPLY,		true,		false },
	{ &EvalContext::plyCmpCurWeapon,			404,	CPLY,		false,		false },
	{ &EvalContext::plyCmpPackItemAmount,		405,	CPLY,		false,		false },
	{ &EvalContext::plyChkPackItemActive,		406,	CPLY,		true,		false },
	{ &EvalContext::plyCmpPackItemSelect,		407,	CPLY,		false,		false },
	{ &EvalContext::plyCmpPowerupAmount,		408,	CPLY,		false,		false },
	{ &EvalContext::plyChkKillerSprite,			409,	CPLY,		true,		false },
	{ &EvalContext::plyChkKeyPress,				410,	CPLY,		true,		false },	// check keys pressed
	{ &EvalContext::plyChkRunning,				411,	CPLY,		true,		false },
	{ &EvalContext::plyChkFalling,				412,	CPLY,		true,		false },
	{ &EvalContext::plyCmpLifeMode,				413,	CPLY,		false,		false },
	{ &EvalContext::plyCmpPosture,				414,	CPLY,		false,		false },
	{ &EvalContext::plyChkSpriteItOwns,		    419,	CPLY,		true,		false },
	{ &EvalContext::plyCmpArmor,  				420,	CPLY,		false,		false },    // in %
	{ &EvalContext::plyCmpAmmo,  			    421,	CPLY,		false,		false },
	{ &EvalContext::plyCmpKillsCount,			430,	CPLY,		false,		false },
	{ &EvalContext::plyChkAutoAimTarget,		431,	CPLY,		true,		false },
	{ &EvalContext::plyChkVoodooTarget,			435,	CPLY,		true,		false },
	{ &EvalContext::plyCmpQavWeapon,			445,	CPLY,		false,		false },
	{ &EvalContext::plyCmpQavScene,				446,	CPLY,		false,		false },
	{ &EvalContext::plyChkGodMode,				447,	CPLY,		true,		false },
	{ &EvalContext::plyChkShrink,				448,	CPLY,		true,		false },
	{ &EvalContext::plyChkGrown,				449,	CPLY,		true,		false },
	/**--------------------------------------------------------------**/
	{ &EvalContext::gdudChkHaveTargets,			450,	CDUDG,		true,		true  },	// dude have any targets?
	{ &EvalContext::gdudChkInAiFight,			451,	CDUDG,		true,		true  },	// dude affected by ai fight?
	{ &EvalContext::gdudChkTargetDistance,		452,	CDUDG,		true,		true  },	// distance to the target in a range?
	{ &EvalContext::gdudChkTargetCansee,		453,	CDUDG,		true,		true  },	// is the target visible?
	{ &EvalContext::gdudChkTargetCanseePerip,	454,	CDUDG,		true,		true  },	// is the target visible with periphery?
	{ &EvalContext::gdudChkFlagPatrol,			455,	CDUDG,		true,		true  },
	{ &EvalContext::gdudChkFlagDeaf,			456,	CDUDG,		true,		true  },
	{ &EvalContext::gdudChkFlagBlind,			457,	CDUDG,		true,		true  },
	{ &EvalContext::gdudChkFlagAlarm,			458,	CDUDG,		true,		true  },
	{ &EvalContext::gdudChkFlagStealth,			459,	CDUDG,		true,		true  },
	{ &EvalContext::gdudChkMarkerBusy,			460,	CDUDG,		true,		true  },	// check if the marker is busy with another dude
	{ &EvalContext::gdudChkMarkerReached,		461,	CDUDG,		true,		true  },	// check if the marker is reached
	{ &EvalContext::gdudCmpSpotProgress,		462,	CDUDG,		false,		true  },	// compare spot progress value in %
	{ &EvalContext::gdudChkLockout,				465,	CDUDG,		true,		true  },	// dude allowed to interact with objects?
	{ &EvalContext::gdudCmpAiStateType,			466,	CDUDG,		false,		true  },
	{ &EvalContext::gdudCmpAiStateTimer,		467,	CDUDG,		false,		true  },
	/**--------------------------------------------------------------**/
	{ &EvalContext::cdudChkLeechThrown,			470,	CDUDC,		true,		false },	// life leech is thrown?
	{ &EvalContext::cdudChkLeechDead,			471,	CDUDC,		true,		false },	// life leech is destroyed?
	{ &EvalContext::cdudCmpSummoned,			472,	CDUDC,		false,		false },	// are required amount of dudes is summoned?
	{ &EvalContext::cdudChkIfAble,				473,	CDUDC,		true,		false },	// check if dude can...
	{ &EvalContext::cdudCmpDispersion,			474,	CDUDC,		false,		false },	// compare weapon dispersion
	/**--------------------------------------------------------------**/
	{ &EvalContext::sprCmpAng,					500,	CSPR,		false,		false },	// compare angle
	{ &EvalContext::sprChkShow2dSprite,			501,	CSEC,		true,		false },	// sprite on the minimap
	{ &EvalContext::sprCmpStatnum,				505,	CSPR,		false,		false },	// check statnum
	{ &EvalContext::sprChkRespawn,				506,	CSPR,		true,		false },	// check if on respawn list
	{ &EvalContext::sprCmpSlope,				507,	CSPR,		false,		false },	// compare slope
	{ &EvalContext::sprCmpClipdist,				510,	CSPR,		false,		false },	// compare clipdist
	{ &EvalContext::sprChkOwner,				515,	CSPR,		true,		false },	// check owner sprite
	{ &EvalContext::sprChkSector,				520,	CSPR,		true,		false },	// stays in a sector?
	{ &EvalContext::sprCmpChkVelocity,			525,	CSPR,		true,		false },	// check or compare velocity
	{ &EvalContext::sprCmpVelocityNew,			526,	CSPR,		false,		false },	// compare velocity
	{ &EvalContext::sprChkUnderwater,			530,	CSPR,		true,		false },	// sector of sprite is underwater?
	{ &EvalContext::sprChkDmgImmune,			531,	CSPR,		true,		false },	// check if immune to N dmgType
	{ &EvalContext::sprChkHitscanCeil,			535,	CSPR,		true,		false },	// hitscan: ceil?
	{ &EvalContext::sprChkHitscanFloor,			536,	CSPR,		true,		false },	// hitscan: floor?
	{ &EvalContext::sprChkHitscanWall,			537,	CSPR,		true,		false },	// hitscan: wall?
	{ &EvalContext::sprChkHitscanSpr,			538,	CSPR,		true,		false },	// hitscan: sprite?
	{ &EvalContext::sprChkHitscanMasked,		539,	CSPR,		true,		false },	// hitscan: masked wall?
	{ &EvalContext::sprChkIsTarget,				545,	CSPR,		true,		false },	// this sprite is a target of some dude?
	{ &EvalContext::sprCmpHealth,				550,	CSPR,		false,		true  },	// compare hp (in %)
	{ &EvalContext::sprChkTouchCeil,			555,	CSPR,		true,		true  },	// touching ceil of sector?
	{ &EvalContext::sprChkTouchFloor,			556,	CSPR,		true,		true  },	// touching floor of sector?
	{ &EvalContext::sprChkTouchWall,			557,	CSPR,		true,		true  },	// touching walls of sector?
	{ &EvalContext::sprChkTouchSpite,			558,	CSPR,		true,		false },	// touching another sprite? (allow no xAvail!)
	{ &EvalContext::sprCmpBurnTime,				565,	CSPR,		false,		true  },	// compare burn time (in %)
	{ &EvalContext::sprChkIsFlareStuck,			566,	CSPR,		true,		false },	// any flares stuck in this sprite?
	{ &EvalContext::sprChkTarget,		        569,	CSPR,		true,		true },	    // use with caution!
	{ &EvalContext::sprCmpMass,					570,	CSPR,		false,		true  },	// mass of the sprite in a range?
};



/** working data set
********************************************************************************/
struct TRACKING_CONDITION
{
	TObjPtr<DBloodActor*> condi;   // x-sprite index of condition
	TArray<EventObject> objects;               // a dynamic list of objects it contains
};

static TArray<TRACKING_CONDITION> gTrackingConditionsList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, TRACKING_CONDITION& w, TRACKING_CONDITION* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("condi", w.condi)
			("obj", w.objects)
			.EndObject();
	}
	return arc;
}

void conditionInitData();

void SerializeConditions(FSerializer& arc)
{
	// if this gets run before loading a map it needs to run the one-time init code before deserializing the data.
	if (arc.isReading() && gConditions.Size() == 0) conditionInitData();
	arc("conditions", gTrackingConditionsList);
}


static void TriggerObject(EventObject nSerial, DBloodActor* pCond)
{
	nnExtTriggerObject(nSerial, pCond->xspr.command, pCond);
}

/** EXTERNAL CODE
********************************************************************************/


// run once for first use
void conditionInitData()
{
	int gNumConditions = 0;
	// sort out *condition function callers* list the right way
	qsort(gCheckFuncInfo, countof(gCheckFuncInfo), sizeof(gCheckFuncInfo[0]), (int(*)(const void*, const void*))qsSortCheckFuncInfo);

	for (size_t i = 0; i < countof(gConditionsList); i++)
	{
		CONDITION_INFO* pTemp = &gConditionsList[i];

#ifdef _DEBUG
		// check for duplicates (only in debug, no need to repeat in release)
		for (size_t j = 0; j < countof(gConditionsList); j++)
		{
			if (i != j)
				assert(pTemp->id != gConditionsList[j].id);
		}
#endif

		// find the highest condition id
		if (pTemp->id > gNumConditions)
			gNumConditions = pTemp->id;
	}

	// allocate the list
	gNumConditions++;
	gConditions.Resize(gNumConditions);

	// dummy template for everything
	int i = 0;
	for (auto cond : gConditions)
	{
		cond.pFunc = &EvalContext::errCondNotImplemented;
		cond.type = CNON;
		cond.pCaller = &gCheckFuncInfo[cond.type];
		cond.isBool = false;
		cond.xReq = false;
		cond.id = i++;
	}

	for (auto& cond : gConditionsList)
	{
		// proper caller info for each function
		for (auto& func : gCheckFuncInfo)
		{
			if (func.type != cond.type) continue;
			cond.pCaller = &func;
			break;
		}

		// sort out condition list the right way
		gConditions[cond.id] = cond;
	}

}

void conditionsInit()
{
	if (gConditions.Size() == 0) conditionInitData();

	int nCount = 0;


	gTrackingConditionsList.Clear();
	BloodStatIterator it(kStatModernCondition);
	while(auto actor = it.Next())
	{
		if (!actor->hasX() || !actor->xspr.busyTime || actor->xspr.isTriggered)
			continue;

		gTrackingConditionsList.Reserve(1);

		TRACKING_CONDITION* pTr = &gTrackingConditionsList.Last();

		if (actor->xspr.rxID >= kChannelUser)
		{
			for (auto& sect : sector)
			{
				BloodSectIterator it2(&sect);
				while (auto actor2 = it2.Next())
				{
					if (!actor2->hasX() || actor2 == actor)
						continue;

					if (actor2->xspr.txID != actor->xspr.rxID)
						continue;

					// check exceptions
					switch (actor2->GetType())
					{
						case kModernCondition:
						case kModernConditionFalse:
						case kSwitchToggle:
						case kSwitchOneWay:
							break;
						default:
							pTr->objects.Push(EventObject(actor2));
							break;
					}
				}

				for(auto&wal : sect.walls)
				{
					if (wal.hasX() && wal.xw().txID == actor->xspr.rxID)
					{
						// check exceptions
						switch (wal.type)
						{
							case kSwitchToggle:
							case kSwitchOneWay:
								break;
							default:
								pTr->objects.Push(EventObject(&wal));
								break;
						}
					}
				}

				if (sect.hasX() && sect.xs().txID == actor->xspr.rxID)
					pTr->objects.Push(EventObject(&sect));
			}
		}

		// allow self tracking
		if (!pTr->objects.Size())
			pTr->objects.Push(EventObject(actor));

		pTr->condi = actor;
	}
}

void conditionsTrackingProcess()
{
	EVENT evn; 
	evn.funcID = nullptr; 
	evn.cmd = kCmdOn; 
	evn.initiator = nullptr;
	
	for(int i = gTrackingConditionsList.SSize() - 1; i >= 0; i--)
	{
		auto pTr = &gTrackingConditionsList[i];  
		auto pSpr = pTr->condi;
		if (pSpr->xspr.locked || pSpr->xspr.isTriggered || ++pSpr->xspr.busy < pSpr->xspr.busyTime)
			continue;

		pSpr->xspr.busy = 0;
		for(auto o : pTr->objects)
		{
			evn.target = o;
			useCondition(pSpr, &evn);
		}
	}
}

void conditionsLinkPlayer(DBloodActor* control, DBloodPlayer* pPlay)
{
	// search for player control sprite and replace it with actual player sprite
	for(auto& cond : gTrackingConditionsList)
	{
		DBloodActor* pTrack = cond.condi;
		if (pTrack->xspr.rxID != control->xspr.txID) continue;

		for(auto& o : cond.objects)
		{
			if (o.isActor() && o.actor() == control)
			{
				o = EventObject(pPlay->GetActor());
				break;
			}
		}
	}
}

// this updates index of object in all conditions
void conditionsUpdateIndex(EventObject Old, EventObject New)
{
	for (auto& cond : gTrackingConditionsList)
	{
		DBloodActor* pTrack = cond.condi;

		for (auto& o : cond.objects)
		{
			if (o == Old)
			{
				o = New;
				break;
			}
		}
	}

	// then update serials everywhere
	BloodStatIterator it(kStatModernCondition);
	while (auto actor = it.Next())
	{
		for (int i = 0; i < 4; i++)
		{
			if (actor->condition[i] == Old) actor->condition[i] = New;
		}
	}
}

void Restore(DBloodActor* pCond)
{
	switch (pCond->xspr.command - kCmdPop)
	{
	case 0:
		pCond->condition[0] = pCond->condition[1];
		break;
		// additional slots leads to swapping
		// so object in the focus is not lost
	case 1:
		std::swap(pCond->condition[0], pCond->condition[2]);
		break;
	case 2:
		std::swap(pCond->condition[0], pCond->condition[3]);
		break;
	}
}

void useCondition(DBloodActor* source, EVENT* pEvn)
{
	// if it's a tracking condition, it must ignore all the commands sent from objects
	if (source->xspr.busyTime && pEvn->funcID != nullptr)
		return;

	bool ok = false;
	bool delayBefore = false, flag8 = false;
	EvalContext ctx;
	
	ctx.pCond = source;
	if (source->xspr.waitTime)
	{
		delayBefore     = (source->spr.flags & kModernTypeFlag64);
		flag8           = (source->spr.flags & kModernTypeFlag8);

		if (source->xspr.restState == 0)
		{
			if (delayBefore)
			{
				// start waiting
				evPostActor(source, EVTIME2TICKS(source->xspr.waitTime), (COMMAND_ID)kCmdRepeat, pEvn->initiator);
				source->xspr.restState = 1;

				if (flag8)
					ctx.ReceiveObjects(pEvn); // receive current objects and hold it

				return;
			}
			else
			{
				ctx.ReceiveObjects(pEvn); // receive current objects and continue
			}
		}
		else if (pEvn->cmd == kCmdRepeat)
		{
			// finished the waiting
			if (delayBefore)
				source->xspr.restState = 0;
		}
		else
		{
			if ((delayBefore && !flag8) || (!delayBefore && flag8))
				ctx.ReceiveObjects(pEvn); // continue receiving actual objects while waiting

			return;
		}
	}
	else
	{
		ctx.ReceiveObjects(pEvn); // receive current objects and continue
	}

	if (source->xspr.restState == 0)
	{
		if (!source->xspr.data1) ok = true;
		else if (rngok(source->xspr.data1, 0, gConditions.Size()))
		{
			ctx.cmpOp = source->spr.cstat;       
			ctx.PUSH = rngok(source->xspr.command, kCmdPush, kCmdPush + kPushRange);
			ctx.arg1 = source->xspr.data2;		
			ctx.arg2 = source->xspr.data3;
			ctx.arg3 = source->xspr.data4;		
			ctx.pEvent = pEvn;

			ctx.o = source->condition[0];
			ctx.pEntry = &gConditions[source->xspr.data1];
			ok = (ctx.*(ctx.pEntry->pCaller->pFunc))();
		}
		else
		{
			ctx.errCondNotImplemented();
			return;
		}

		if (ok != (source->GetType() == kModernConditionFalse))
		{
			source->xspr.state = 1;
			if (source->xspr.waitTime && !delayBefore) // delay after checking
			{
				// start waiting
				evPostActor(source, EVTIME2TICKS(source->xspr.waitTime), (COMMAND_ID)kCmdRepeat, pEvn->initiator);
				source->xspr.restState = 1;
				return;
			}
		}
		else
		{
			source->xspr.state = 0;
		}
	}
	else if (pEvn->cmd == kCmdRepeat)
	{
		source->xspr.restState = 0;
	}
	else
	{
		return;
	}

	if (source->xspr.state)
	{
		// trigger once per result?
		if (source->spr.flags & kModernTypeFlag4)
		{
			if (source->xspr.patrolstate)
				return;

			source->xspr.patrolstate = 1;
		}

		if (source->xspr.triggerOnce)
		#ifdef CONDITIONS_USE_BUBBLE_ACTION
			conditionsBubble(source, conditionsSetIsTriggered, true);
		#else
			conditionsSetIsTriggered(source, true);
		#endif

		if (rngok(source->xspr.command, kCmdPop, kCmdPop + kPushRange))
			Restore(source); // change focus to the saved object

		// send command to rx bucket
		if (source->xspr.txID)
		{
			ctx.o = source->condition[0];
			evSendActor(source, source->xspr.txID, (COMMAND_ID)source->xspr.command, ctx.o.type() == EventObject::Actor ? ctx.o.actor() : nullptr);
		}

		if (source->spr.flags)
		{
			// send it for object that currently in the focus
			if (source->spr.flags & kModernTypeFlag1)
			{
				TriggerObject(source->condition[0], source);
				if ((source->spr.flags & kModernTypeFlag2) && source->condition[0] == source->condition[1])
					return;
			}

			// send it for initial object
			if (source->spr.flags & kModernTypeFlag2)
			{
				TriggerObject(source->condition[1], source);
			}
		}
	}
	else
	{
		source->xspr.patrolstate = 0;
	}
}

#ifdef CONDITIONS_USE_BUBBLE_ACTION
void conditionsBubble(DBloodActor* pStart, void(*pActionFunc)(DBloodActor*, int), int nValue)
{
	pActionFunc(pStart, nValue);

	// perform action for whole branch from bottom to top while there is no forks
	BloodStatIterator it(kStatModernCondition);
	while (auto actor = it.Next())
	{
		if (actor->xspr.txID != pStart->xspr.rxID)
			continue;

		BloodStatIterator it2(kStatModernCondition);
		while (auto actor2 = it2.Next())
		{
			if (actor2->xspr.rxID == pStart->xspr.rxID && actor2->xspr.txID != pStart->xspr.txID)
			{
				conditionsBubble(actor, pActionFunc, nValue);
				break; // fork found
			}
		}
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

void conditionListReset()
{
	gTrackingConditionsList.Clear();
}

#endif

END_BLD_NS
