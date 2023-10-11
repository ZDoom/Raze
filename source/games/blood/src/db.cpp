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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "common_game.h"
#include "zstring.h"
#include "m_crc32.h"
#include "md4.h"
#include "automap.h"
#include "raze_sound.h"
#include "gamefuncs.h"
#include "hw_sections.h"
#include "sectorgeometry.h"
#include "psky.h"
#include "m_swap.h"

#include "blood.h"

BEGIN_BLD_NS

int gSkyCount;
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* InsertSprite(sectortype* pSector, int nStat, PClass* cls)
{
	if (cls == nullptr) cls = RUNTIME_CLASS(DBloodActor);
	auto act = static_cast<DBloodActor*>(::InsertActor(cls, pSector, nStat));
	act->spr.cstat = CSTAT_SPRITE_YCENTER;
	act->clipdist = 8;

	// default to full scale.
	if (act->spr.scale.isZero())
		act->spr.scale = DVector2(1, 1);
	return act;
}

int DeleteSprite(DBloodActor* actor)
{
#ifdef NOONE_EXTENSIONS
	for (auto& ctrl : gPlayerCtrl) if (ctrl.qavScene.initiator == actor) ctrl.qavScene.initiator = nullptr;
#endif

	actor->Destroy();
	return 0;
}


uint8_t gModernMap = 0;
int gVisibility;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void dbCrypt(char* pPtr, int nLength, int nKey)
{
	for (int i = 0; i < nLength; i++)
	{
		pPtr[i] = pPtr[i] ^ nKey;
		nKey++;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

unsigned int dbReadMapCRC(const char* pPath)
{
	FString mapname = pPath;
	DefaultExtension(mapname, ".map");
	auto fr = fileSystem.OpenFileReader(mapname.GetChars());

	if (!fr.isOpen())
	{
		Printf("Error opening map file %s", pPath);
		return -1;
	}

	MAPSIGNATURE header;
	fr.Read(&header, 6);
	if (memcmp(header.signature, "BLM\x1a", 4))
	{
		I_Error("%s: Map file corrupted.", mapname.GetChars());
	}
	int ver = LittleShort(header.version);
	if ((ver & 0xff00) == 0x600)
	{
	}
	else if ((ver & 0xff00) == 0x700)
	{
	}
	else
	{
		I_Error("%s: Map file is wrong version.", mapname.GetChars());
	}
	fr.Seek(-4, FileReader::SeekEnd);
	return fr.ReadInt32();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void dbLoadMap(const char* pPath, DVector3& pos, short* pAngle, sectortype** cursect, unsigned int* pCRC, BloodSpawnSpriteDef& sprites)
{
	const int nXSectorSize = 60;
	const int nXSpriteSize = 56;
	const int nXWallSize = 24;

	MAPHEADER2 xheader;
	int gMapRev, gMattId;

	int16_t tpskyoff[256];
	ClearAutomap();
#ifdef NOONE_EXTENSIONS
	gModernMap = false;
#endif

#ifdef NOONE_EXTENSIONS
	for (auto& ctrl : gPlayerCtrl) ctrl.qavScene.initiator = nullptr;
#endif


	FString mapname = pPath;
	DefaultExtension(mapname, ".map");
	auto fr = fileSystem.OpenFileReader(mapname.GetChars());

	if (!fr.isOpen())
	{
		I_Error("Error opening map file %s", mapname.GetChars());
	}
	MAPSIGNATURE header;
	fr.Read(&header, 6);
	if (memcmp(header.signature, "BLM\x1a", 4))
	{
		I_Error("%s: Map file corrupted", mapname.GetChars());
	}
	bool encrypted = 0;
	if ((LittleShort(header.version) & 0xff00) == 0x700) {
		encrypted = 1;

	currentLevel->featureflags = 0;

#ifdef NOONE_EXTENSIONS
		// indicate if the map requires modern features to work properly
		// for maps wich created in PMAPEDIT BETA13 or higher versions. Since only minor version changed,
		// the map is still can be loaded with vanilla BLOOD / MAPEDIT and should work in other ports too.
		int tmp = (header.version & 0x00ff);

		// get the modern features revision
		switch (tmp) {
		case 0x001:
			gModernMap = 1;
			currentLevel->featureflags = ~0;
			break;
		case 0x002:
			gModernMap = 2;
			currentLevel->featureflags = ~0;
			break;
		}
#endif

	}
	else {
		I_Error("%s: Map file is wrong version", mapname.GetChars());
	}

	MAPHEADER mapHeader;
	fr.Read(&mapHeader, 37/* sizeof(mapHeader)*/);
	if (mapHeader.mattid != 0 && mapHeader.mattid != 0x7474614d && mapHeader.mattid != 0x4d617474) {
		dbCrypt((char*)&mapHeader, sizeof(mapHeader), 0x7474614d);
	}

	mapHeader.x = LittleLong(mapHeader.x);
	mapHeader.y = LittleLong(mapHeader.y);
	mapHeader.z = LittleLong(mapHeader.z);
	mapHeader.ang = LittleShort(mapHeader.ang);
	mapHeader.sect = LittleShort(mapHeader.sect);
	mapHeader.pskybits = LittleShort(mapHeader.pskybits);
	mapHeader.visibility = LittleLong(mapHeader.visibility);
	mapHeader.mattid = LittleLong(mapHeader.mattid);
	mapHeader.revision = LittleLong(mapHeader.revision);
	mapHeader.numsectors = LittleShort(mapHeader.numsectors);
	mapHeader.numwalls = LittleShort(mapHeader.numwalls);
	mapHeader.numsprites = LittleShort(mapHeader.numsprites);

	pos = { mapHeader.x * maptoworld, mapHeader.y * maptoworld, mapHeader.z * zmaptoworld };
	*pAngle = mapHeader.ang;
	gVisibility = g_visibility = mapHeader.visibility;
	gMattId = mapHeader.mattid;
	if (encrypted)
	{
		if (!(mapHeader.mattid == 0x7474614d || mapHeader.mattid == 0x4d617474 || !mapHeader.mattid))
		{
			I_Error("%s: Corrupted Map file", mapname.GetChars());
		}
	}
	else if (mapHeader.mattid)
	{
		I_Error("%s: Corrupted Map file", mapname.GetChars());
	}
	gMapRev = mapHeader.revision;
	allocateMapArrays(mapHeader.numwalls, mapHeader.numsectors, mapHeader.numsprites);
	*cursect = validSectorIndex(mapHeader.sect)? &sector[mapHeader.sect] : nullptr;

	if (encrypted)
	{
		fr.Read(&xheader, 128);
		dbCrypt((char*)&xheader, 128, wall.Size());

		xheader.numxsprites = LittleLong(xheader.numxsprites);
		xheader.numxwalls = LittleLong(xheader.numxwalls);
		xheader.numxsectors = LittleLong(xheader.numxsectors);
	}
	else
	{
		memset(&xheader, 0, 128);
	}
	gSkyCount = 1 << mapHeader.pskybits;
	fr.Read(tpskyoff, gSkyCount * sizeof(tpskyoff[0]));
	if (encrypted)
	{
		dbCrypt((char*)tpskyoff, gSkyCount * sizeof(tpskyoff[0]), gSkyCount * 2);
	}
	for (int i = 0; i < ClipHigh(gSkyCount, MAXPSKYTILES); i++)
	{
		tpskyoff[i] = LittleShort(tpskyoff[i]);
	}

	defineSky(nullptr, mapHeader.pskybits, tpskyoff);

	for (unsigned i = 0; i < sector.Size(); i++)
	{
		sectortype* pSector = &sector[i];
		sectortypedisk load;
		fr.Read(&load, sizeof(sectortypedisk));
		if (encrypted)
		{
			dbCrypt((char*)&load, sizeof(sectortypedisk), gMapRev * sizeof(sectortypedisk));
		}
		pSector->walls.Set(&wall[LittleShort(load.wallptr)], LittleShort(load.wallnum));
		pSector->setzfrommap(LittleLong(load.ceilingz), LittleLong(load.floorz));
		pSector->ceilingstat = ESectorFlags::FromInt(LittleShort(load.ceilingstat));
		pSector->floorstat = ESectorFlags::FromInt(LittleShort(load.floorstat));
		pSector->setceilingtexture(tileGetTextureID(LittleShort(load.ceilingpic)));
		pSector->ceilingheinum = LittleShort(load.ceilingheinum);
		pSector->setfloortexture(tileGetTextureID(LittleShort(load.floorpic)));
		pSector->floorheinum = LittleShort(load.floorheinum);
		pSector->type = LittleShort(load.type);
		pSector->hitag = LittleShort(load.hitag);
		pSector->extra = LittleShort(load.extra);
		pSector->ceilingshade = load.ceilingshade;
		pSector->ceilingpal = load.ceilingpal;
		pSector->ceilingxpan_ = load.ceilingxpanning;
		pSector->ceilingypan_ = load.ceilingypanning;
		pSector->floorshade = load.floorshade;
		pSector->floorpal = load.floorpal;
		pSector->floorxpan_ = load.floorxpanning;
		pSector->floorypan_ = load.floorypanning;
		pSector->visibility = load.visibility;
		pSector->slopewallofs = load.fogpal;
		pSector->dirty = EDirty::AllDirty;
		pSector->exflags = 0;
		pSector->fogpal = 0;

		if (pSector->extra > 0)
		{
			uint8_t pBuffer[nXSectorSize];
			pSector->allocX();
			XSECTOR* pXSector = &pSector->xs();
			int nCount;
			if (!encrypted)
			{
				nCount = nXSectorSize;
			}
			else
			{
				nCount = xheader.numxsectors;
			}
			assert(nCount <= nXSectorSize);
			fr.Read(pBuffer, nCount);
			BitReader bitReader(pBuffer, nCount);
			/*pXSector->reference =*/ bitReader.getBitsSigned(14);
			pXSector->state = bitReader.getBits(1);
			pXSector->busy = bitReader.getBits(17);
			pXSector->data = bitReader.getBits(16);
			pXSector->txID = bitReader.getBits(10);
			pXSector->busyWaveA = bitReader.getBits(3);
			pXSector->busyWaveB = bitReader.getBits(3);
			pXSector->rxID = bitReader.getBits(10);
			pXSector->command = bitReader.getBits(8);
			pXSector->triggerOn = bitReader.getBits(1);
			pXSector->triggerOff = bitReader.getBits(1);
			pXSector->busyTimeA = bitReader.getBits(12);
			pXSector->waitTimeA = bitReader.getBits(12);
			pXSector->restState = bitReader.getBits(1);
			pXSector->interruptable = bitReader.getBits(1);
			pXSector->amplitude = bitReader.getBitsSigned(8);
			pXSector->freq = bitReader.getBits(8);
			pXSector->reTriggerA = bitReader.getBits(1);
			pXSector->reTriggerB = bitReader.getBits(1);
			pXSector->phase = bitReader.getBits(8);
			pXSector->wave = bitReader.getBits(4);
			pXSector->shadeAlways = bitReader.getBits(1);
			pXSector->shadeFloor = bitReader.getBits(1);
			pXSector->shadeCeiling = bitReader.getBits(1);
			pXSector->shadeWalls = bitReader.getBits(1);
			pXSector->shade = bitReader.getBitsSigned(8);
			pXSector->panAlways = bitReader.getBits(1);
			pXSector->panFloor = bitReader.getBits(1);
			pXSector->panCeiling = bitReader.getBits(1);
			pXSector->Drag = bitReader.getBits(1);
			pXSector->Underwater = bitReader.getBits(1);
			pXSector->Depth = bitReader.getBits(3);
			pXSector->panVel = bitReader.getBits(8);
			pXSector->panAngle = mapangle(bitReader.getBits(11));
			pXSector->pauseMotion = bitReader.getBits(1);
			pXSector->decoupled = bitReader.getBits(1);
			pXSector->triggerOnce = bitReader.getBits(1);
			pXSector->isTriggered = bitReader.getBits(1);
			pXSector->Key = bitReader.getBits(3);
			pXSector->Push = bitReader.getBits(1);
			pXSector->Vector = bitReader.getBits(1);
			pXSector->Reserved = bitReader.getBits(1);
			pXSector->Enter = bitReader.getBits(1);
			pXSector->Exit = bitReader.getBits(1);
			pXSector->Wallpush = bitReader.getBits(1);
			pXSector->color = bitReader.getBits(1);
			/*pXSector->unused2 =*/ bitReader.getBits(1);
			pXSector->busyTimeB = bitReader.getBits(12);
			pXSector->waitTimeB = bitReader.getBits(12);
			pXSector->stopOn = bitReader.getBits(1);
			pXSector->stopOff = bitReader.getBits(1);
			pXSector->ceilpal = bitReader.getBits(4);
			pXSector->offCeilZ = bitReader.getBitsSigned(32) * zmaptoworld;
			pXSector->onCeilZ = bitReader.getBitsSigned(32) * zmaptoworld;
			pXSector->offFloorZ = bitReader.getBitsSigned(32) * zmaptoworld;
			pXSector->onFloorZ = bitReader.getBitsSigned(32) * zmaptoworld;
			/*pXSector->marker0 =*/ bitReader.getBits(16);
			/*pXSector->marker1 =*/ bitReader.getBits(16);
			pXSector->Crush = bitReader.getBits(1);
			pSector->ceilingxpan_ += bitReader.getBits(8) / 256.f;
			pSector->ceilingypan_ += bitReader.getBits(8) / 256.f;
			pSector->floorxpan_ += bitReader.getBits(8) / 256.f;
			pXSector->damageType = bitReader.getBits(3);
			pXSector->floorpal = bitReader.getBits(4);
			pSector->floorypan_ += bitReader.getBits(8) / 256.f;
			pXSector->locked = bitReader.getBits(1);
			pXSector->windVel = bitReader.getBits(10);
			pXSector->windAng = mapangle(bitReader.getBits(11));
			pXSector->windAlways = bitReader.getBits(1);
			pXSector->dudeLockout = bitReader.getBits(1);
			pXSector->bobTheta = bitReader.getBits(11);
			pXSector->bobZRange = bitReader.getBits(5);
			pXSector->bobSpeed = bitReader.getBitsSigned(12);
			pXSector->bobAlways = bitReader.getBits(1);
			pXSector->bobFloor = bitReader.getBits(1);
			pXSector->bobCeiling = bitReader.getBits(1);
			pXSector->bobRotate = bitReader.getBits(1);
			pXSector->busy = IntToFixed(pXSector->state);

		}
	}
	for (unsigned i = 0; i < wall.Size(); i++)
	{
		walltype* pWall = &wall[i];
		walltypedisk load;
		fr.Read(&load, sizeof(walltypedisk));
		if (encrypted)
		{
			dbCrypt((char*)&load, sizeof(walltypedisk), (gMapRev * sizeof(sectortypedisk)) | 0x7474614d);
		}
		int x = LittleLong(load.x);
		int y = LittleLong(load.y);
		pWall->setPosFromMap(x, y);
		pWall->point2 = LittleShort(load.point2);
		pWall->nextwall = LittleShort(load.nextwall);
		pWall->nextsector = LittleShort(load.nextsector);
		pWall->cstat = EWallFlags::FromInt(LittleShort(load.cstat));
		pWall->setwalltexture(tileGetTextureID(LittleShort(load.wallpic)));
		if (load.overpic == 0) load.overpic = -1;
		pWall->setovertexture(tileGetTextureID(LittleShort(load.overpic)));
		pWall->type = LittleShort(load.type);
		pWall->hitag = LittleShort(load.hitag);
		pWall->extra = LittleShort(load.extra);
		pWall->shade = load.shade;
		pWall->pal = load.pal;
		pWall->xrepeat = load.xrepeat;
		pWall->xpan_ = load.xpanning;
		pWall->yrepeat = load.yrepeat;
		pWall->ypan_ = load.ypanning;

		if (pWall->extra > 0)
		{
			uint8_t pBuffer[nXWallSize];
			pWall->allocX();
			XWALL* pXWall = &pWall->xw();
			int nCount;
			if (!encrypted)
			{
				nCount = nXWallSize;
			}
			else
			{
				nCount = xheader.numxwalls;
			}
			assert(nCount <= nXWallSize);
			fr.Read(pBuffer, nCount);
			BitReader bitReader(pBuffer, nCount);
			/*pXWall->reference =*/ bitReader.getBitsSigned(14);
			pXWall->state = bitReader.getBits(1);
			pXWall->busy = bitReader.getBits(17);
			pXWall->data = bitReader.getBitsSigned(16);
			pXWall->txID = bitReader.getBits(10);
			bitReader.getBits(6);
			pXWall->rxID = bitReader.getBits(10);
			pXWall->command = bitReader.getBits(8);
			pXWall->triggerOn = bitReader.getBits(1);
			pXWall->triggerOff = bitReader.getBits(1);
			pXWall->busyTime = bitReader.getBits(12);
			pXWall->waitTime = bitReader.getBits(12);
			pXWall->restState = bitReader.getBits(1);
			pXWall->interruptable = bitReader.getBits(1);
			pXWall->panAlways = bitReader.getBits(1);
			pXWall->panVel.X = bitReader.getBitsSigned(8);
			pXWall->panVel.Y = bitReader.getBitsSigned(8);
			pXWall->decoupled = bitReader.getBits(1);
			pXWall->triggerOnce = bitReader.getBits(1);
			pXWall->isTriggered = bitReader.getBits(1);
			pXWall->key = bitReader.getBits(3);
			pXWall->triggerPush = bitReader.getBits(1);
			pXWall->triggerVector = bitReader.getBits(1);
			pXWall->triggerTouch = bitReader.getBits(1);
			bitReader.getBits(2);
			pWall->xpan_ += bitReader.getBits(8) / 256.f;
			pWall->ypan_ += bitReader.getBits(8) / 256.f;
			pXWall->locked = bitReader.getBits(1);
			pXWall->dudeLockout = bitReader.getBits(1);
			bitReader.getBits(4);
			bitReader.getBits(32);
			pXWall->busy = IntToFixed(pXWall->state);

		}
	}
	leveltimer = mapHeader.numsprites;
	sprites.sprites.Resize(mapHeader.numsprites);
	sprites.xspr.Resize(mapHeader.numsprites);
	for (int i = 0; i < mapHeader.numsprites; i++)
	{
		spritetypedisk load;
		fr.Read(&load, sizeof(spritetypedisk));
		if (encrypted) // What were these people thinking? :(
		{
			dbCrypt((char*)&load, sizeof(spritetypedisk), (gMapRev * sizeof(spritetypedisk)) | 0x7474614d);
		}
		auto pSprite = &sprites.sprites[i];
		*pSprite = {};
		pSprite->SetMapPos(LittleLong(load.x), LittleLong(load.y), LittleLong(load.z));
		pSprite->cstat = ESpriteFlags::FromInt(LittleShort(load.cstat));
		pSprite->picnum = LittleShort(load.pic);
		int secno = LittleShort(load.sectnum);
		pSprite->statnum = LittleShort(load.statnum);
		pSprite->Angles.Yaw = mapangle(LittleShort(load.ang));
		pSprite->intowner = LittleShort(load.owner);
		pSprite->xint = LittleShort(load.index);
		pSprite->yint = LittleShort(load.yvel);
		pSprite->inittype = LittleShort(load.inittype);
		pSprite->lotag = LittleShort(load.lotag);
		pSprite->flags = LittleShort(load.hitag);
		pSprite->extra = LittleShort(load.extra);
		pSprite->pal = load.pal;
		pSprite->clipdist = load.clipdist;
		pSprite->scale = DVector2(load.xrepeat * REPEAT_SCALE, load.yrepeat * REPEAT_SCALE);
		pSprite->xoffset = load.xoffset;
		pSprite->yoffset = load.yoffset;
		pSprite->detail = load.detail;
		pSprite->shade = load.shade;
		pSprite->blend = 0;
		validateSprite(*pSprite, secno, i);

		if (pSprite->extra > 0)
		{
			uint8_t pBuffer[nXSpriteSize];
			XSPRITE* pXSprite = &sprites.xspr[i];
			*pXSprite = {};
			int nCount;
			if (!encrypted)
			{
				nCount = nXSpriteSize;
			}
			else
			{
				nCount = xheader.numxsprites;
			}
			assert(nCount <= nXSpriteSize);
			fr.Read(pBuffer, nCount);
			BitReader bitReader(pBuffer, nCount);
			/*pXSprite->reference =*/ bitReader.getBitsSigned(14);
			pXSprite->state = bitReader.getBits(1);
			pXSprite->busy = bitReader.getBits(17);
			pXSprite->txID = bitReader.getBits(10);
			pXSprite->rxID = bitReader.getBits(10);
			pXSprite->command = bitReader.getBits(8);
			pXSprite->triggerOn = bitReader.getBits(1);
			pXSprite->triggerOff = bitReader.getBits(1);
			pXSprite->wave = bitReader.getBits(2);
			pXSprite->busyTime = bitReader.getBits(12);
			pXSprite->waitTime = bitReader.getBits(12);
			pXSprite->restState = bitReader.getBits(1);
			pXSprite->Interrutable = bitReader.getBits(1);
			pXSprite->modernFlags = bitReader.getBits(2);
			pXSprite->respawnPending = bitReader.getBits(2);
			pXSprite->patrolstate = bitReader.getBits(1);
			pXSprite->lT = bitReader.getBits(1);
			pXSprite->dropMsg = bitReader.getBits(8);
			pXSprite->Decoupled = bitReader.getBits(1);
			pXSprite->triggerOnce = bitReader.getBits(1);
			pXSprite->isTriggered = bitReader.getBits(1);
			pXSprite->key = bitReader.getBits(3);
			pXSprite->Push = bitReader.getBits(1);
			pXSprite->Vector = bitReader.getBits(1);
			pXSprite->Impact = bitReader.getBits(1);
			pXSprite->Pickup = bitReader.getBits(1);
			pXSprite->Touch = bitReader.getBits(1);
			pXSprite->Sight = bitReader.getBits(1);
			pXSprite->Proximity = bitReader.getBits(1);
			pXSprite->sightstuff = bitReader.getBits(2);
			pXSprite->lSkill = bitReader.getBits(5);
			pXSprite->lS = bitReader.getBits(1);
			pXSprite->lB = bitReader.getBits(1);
			pXSprite->lC = bitReader.getBits(1);
			pXSprite->DudeLockout = bitReader.getBits(1);
			pXSprite->data1 = bitReader.getBitsSigned(16);
			pXSprite->data2 = bitReader.getBitsSigned(16);
			pXSprite->data3 = bitReader.getBitsSigned(16);
			pXSprite->goalAng = mapangle(bitReader.getBits(11));
			pXSprite->dodgeDir = bitReader.getBitsSigned(2);
			pXSprite->locked = bitReader.getBits(1);
			pXSprite->medium = bitReader.getBits(2);
			pXSprite->respawn = bitReader.getBits(2);
			pXSprite->data4 = bitReader.getBits(16);
			pXSprite->patrolturndelay = bitReader.getBits(6);
			pXSprite->lockMsg = bitReader.getBits(8);
			pXSprite->health = bitReader.getBits(12);
			pXSprite->dudeDeaf = bitReader.getBits(1);
			pXSprite->dudeAmbush = bitReader.getBits(1);
			pXSprite->dudeGuard = bitReader.getBits(1);
			pXSprite->dudeFlag4 = bitReader.getBits(1);
			/*pXSprite->target_i = */ bitReader.getBitsSigned(16);
			int tx = bitReader.getBitsSigned(32);
			int ty = bitReader.getBitsSigned(32);
			int tz = bitReader.getBitsSigned(32);
			pXSprite->TargetPos = {tx * maptoworld, ty * maptoworld, tz * zmaptoworld };
			pXSprite->burnTime = bitReader.getBits(16);
			/*pXSprite->burnSource =*/ bitReader.getBitsSigned(16);
			pXSprite->height = bitReader.getBits(16);
			pXSprite->stateTimer = bitReader.getBits(16);
			pXSprite->aiState = NULL;
			bitReader.skipBits(32);
			pXSprite->busy = IntToFixed(pXSprite->state);
			if (!encrypted) {
				pXSprite->lT |= pXSprite->lB;
			}

#ifdef NOONE_EXTENSIONS
			// indicate if the map requires modern features to work properly
			// for maps wich created in different editors (include vanilla MAPEDIT) or in PMAPEDIT version below than BETA13
			if (!gModernMap && pXSprite->rxID == pXSprite->txID && pXSprite->command == kCmdModernFeaturesEnable)
			{
				// get the modern features revision
				switch (pXSprite->txID) {
				case kChannelMapModernRev1:
					gModernMap = 1;
					break;
				case kChannelMapModernRev2:
					gModernMap = 2;
					break;
				}
			}
#endif
		}
	}
	fixSectors();

	unsigned int nCRC = fr.ReadUInt32();

	fr.Seek(0, FileReader::SeekSet);
	auto buffer = fr.Read();
	uint8_t md4[16];
	md4once(buffer.data(), (unsigned)buffer.size(), md4);
	PostProcessLevel(md4, mapname, sprites);
	loadMapHack(mapname.GetChars(), md4, sprites);

	if (CalcCRC32(buffer.data(), (unsigned)buffer.size() - 4) != nCRC)
	{
		I_Error("%s: Map File does not match CRC", mapname.GetChars());
	}
	if (pCRC)
		*pCRC = nCRC;
	if (encrypted)
	{
		if (!(gMattId == 0x7474614d || gMattId == 0x4d617474 || !gMattId))
		{
			I_Error("%s: Corrupted Map file", mapname.GetChars());
		}
	}
	else if (gMattId != 0)
	{
		I_Error("%s: Corrupted Map file", mapname.GetChars());
	}

	if ((header.version & 0xff00) == 0x600)
	{
		switch (header.version & 0xff)
		{
		case 0:
			for (auto& sect : sector)
			{
				sectortype* pSector = &sect;
				if (pSector->hasX())
				{
					XSECTOR* pXSector = &pSector->xs();
					pXSector->busyTimeB = pXSector->busyTimeA;
					if (pXSector->busyTimeA > 0)
					{
						if (!pXSector->restState)
						{
							pXSector->reTriggerA = 1;
						}
						else
						{
							pXSector->waitTimeB = pXSector->waitTimeA;
							pXSector->waitTimeA = 0;
							pXSector->reTriggerB = 1;
						}
					}
				}
			}
			[[fallthrough]];
		case 1:
			for (auto& sect : sector)
			{
				sectortype* pSector = &sect;
				if (pSector->hasX())
				{
					XSECTOR* pXSector = &pSector->xs();
					pXSector->freq >>= 1;
				}
			}
			[[fallthrough]];
		case 2:
			break;

		}
	}

	setWallSectors();
	hw_CreateSections();
	sectionGeometry.SetSize(sections.Size());
	wallbackup = wall;
	sectorbackup = sector;
	validateStartSector(mapname.GetChars(), pos, cursect, mapHeader.numsectors, true);
}


END_BLD_NS

//---------------------------------------------------------------------------
//
// only used by the backup loader.
//
//---------------------------------------------------------------------------

void qloadboard(const char* filename, uint8_t flags, DVector3* dapos, int16_t* daang)
{
	Blood::BloodSpawnSpriteDef sprites;
	sectortype* sp;
	Blood::dbLoadMap(filename, *dapos, daang, &sp, nullptr, sprites);
}
