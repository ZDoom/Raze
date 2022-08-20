//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "aistuff.h"
#include "player.h"
#include "engine.h"
#include "exhumed.h"
#include "sound.h"
#include "interpolate.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

enum
{
	kMaxFlashes			= 2000,
	kMaxFlickerMask		= 25,
	kMaxGlows			= 50,
	kMaxFlickers		= 100,
	kMaxFlows			= 375,
};

struct Flash
{
    union
    {
        walltype* pWall;
        sectortype* pSector;
        TObjPtr<DExhumedActor*> pActor;
    };
    int next;
    int8_t nType;
    int8_t shade;
};

struct Glow
{
    sectortype* pSector;

    int16_t nShade;
    int16_t nCounter;
    int16_t nThreshold;
};

struct Flicker
{
    sectortype* pSector;
    int16_t nShade;
    unsigned int nMask;
};

struct Flow
{
    union
    {
        walltype* pWall;
        sectortype* pSector;
    };
    int type;
    float angcos;
    float angsin;
};


FreeListArray<Flash, kMaxFlashes> sFlash;

Glow sGlow[kMaxGlows];
Flicker sFlicker[kMaxFlickers];
Flow sFlowInfo[kMaxFlows];
int flickermask[kMaxFlickerMask];

int bTorch = 0;
int nFirstFlash = -1;
int nLastFlash = -1;
int nFlashDepth = 2;
int nFlowCount;
int nFlickerCount;
int nGlowCount;

int bDoFlicks = 0;
int bDoGlows = 0;

size_t MarkLighting()
{
    for (int i = 0; i < kMaxFlashes; i++)
    {
        auto& f = sFlash[i];
        if (f.nType & 4) GC::Mark(f.pActor);
    }
    return kMaxFlashes;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Flash& w, Flash* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("type", w.nType)
            ("shade", w.shade)
            ("next", w.next);

        if (w.nType & 4) arc("index", w.pActor);
        else if (w.nType & 1) arc("index", w.pSector);
        else arc("index", w.pWall);
        arc.EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Glow& w, Glow* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("shade", w.nShade)
            ("counter", w.nCounter)
            ("sector", w.pSector)
            ("threshold", w.nThreshold)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Flicker& w, Flicker* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("shade", w.nShade)
            ("sector", w.pSector)
            ("mask", w.nMask)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Flow& w, Flow* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("type", w.type)
            ("angcos", w.angcos)
            ("angsin", w.angsin);
        if (w.type < 2) arc("index", w.pSector);
        else arc("index", w.pWall);
        arc.EndObject();
    }
    return arc;
}

void SerializeLighting(FSerializer& arc)
{
    if (arc.BeginObject("lighting"))
    {
        arc("flash", sFlash)
            ("glowcount", nGlowCount)
            .Array("glow", sGlow, nGlowCount)
            ("flickercount", nFlickerCount)
            .Array("flicker", sFlicker, nFlickerCount)
            ("flowcount", nFlowCount)
            .Array("flow", sFlowInfo, nFlowCount)
            .Array("flickermask", flickermask, countof(flickermask))
            ("torch", bTorch)
            ("firstflash", nFirstFlash)
            ("lastflash", nLastFlash)
            ("flashdepth", nFlashDepth)
            ("doflicks", bDoFlicks)
            ("doglows", bDoGlows)
            .EndObject();
    }
}

// done
int GrabFlash()
{
    int nFlash = sFlash.Get();
    if (nFlash < 0) {
        return -1;
    }

    sFlash[nFlash].next = -1;

    if (nLastFlash <= -1)
    {
        nFirstFlash = nFlash;
    }
    else
    {
        sFlash[nLastFlash].next = nFlash;
    }

    nLastFlash = nFlash;
    return nLastFlash;
}

void InitLights()
{
    int i;
    nFlickerCount = 0;

    for (i = 0; i < kMaxFlickerMask; i++) {
        flickermask[i] = RandomSize(0x1F) * 2;
    }

    nGlowCount = 0;
    nFlowCount = 0;
    bDoFlicks = false;
    bDoGlows  = false;

    sFlash.Clear();

    nFirstFlash = -1;
    nLastFlash  = -1;
}

void AddFlash(sectortype* pSector, const DVector3& pos, int val)
{
    int var_28 = 0;
    int var_1C = val >> 8;

    if (var_1C >= nFlashDepth) {
        return;
    }

    unsigned int var_20 = val & 0x80;
    unsigned int var_18 = val & 0x40;

    val = ((var_1C + 1) << 8) | (val & 0xff);

    int var_14 = 0;

	for (auto& wal : wallsofsector(pSector))
    {
		auto average = wal.center();

        sectortype *pNextSector = NULL;
        if (wal.twoSided())
            pNextSector = wal.nextSector();

        int walldist = -255;

        if (!var_18)
        {
            walldist = (int)sqrt(SquareDistToWall(pos.X, pos.Y, &wal)) - 255;
        }

        if (walldist < 0)
        {
            var_14++;
            var_28 += walldist;

            if (wal.pal < 5)
            {
                if (!pNextSector || pNextSector->floorz < pSector->floorz)
                {
                    int nFlash = GrabFlash();
                    if (nFlash < 0) {
                        return;
                    }

                    sFlash[nFlash].nType = var_20 | 2;
                    sFlash[nFlash].shade = wal.shade;
					sFlash[nFlash].pWall = &wal;

                    wal.pal += 7;

                    wal.shade = max( -127, wal.shade + walldist);

                    if (!var_1C && !wal.overpicnum && pNextSector)
                    {
                        AddFlash(pNextSector, pos, val);
                    }
                }
            }
        }
    }

    if (var_14 && pSector->floorpal < 4)
    {
        int nFlash = GrabFlash();
        if (nFlash < 0) {
            return;
        }

        sFlash[nFlash].nType = var_20 | 1;
        sFlash[nFlash].pSector = pSector;;
        sFlash[nFlash].shade = pSector->floorshade;

        pSector->floorpal += 7;

        int edx = pSector->floorshade + var_28;
        int eax = edx;

        if (edx < -127) {
            eax = -127;
        }

        pSector->floorshade = eax;

        if (!(pSector->ceilingstat & CSTAT_SECTOR_SKY))
        {
            if (pSector->ceilingpal < 4)
            {
                int nFlash2 = GrabFlash();
                if (nFlash2 >= 0)
                {
                    sFlash[nFlash2].nType = var_20 | 3;
                    sFlash[nFlash2].pSector = pSector;
                    sFlash[nFlash2].shade = pSector->ceilingshade;

                    pSector->ceilingpal += 7;

                    edx = pSector->ceilingshade + var_28;
                    eax = edx;

                    if (edx < -127) {
                        eax = -127;
                    }

                    pSector->ceilingshade = eax;
                }
            }
        }

        ExhumedSectIterator it(pSector);
        while (auto pActor = it.Next())
        {
            if (pActor->spr.pal < 4)
            {
                int nFlash3 = GrabFlash();
                if (nFlash3 >= 0)
                {
                    sFlash[nFlash3].nType = var_20 | 4;
                    sFlash[nFlash3].shade = pActor->spr.shade;
                    sFlash[nFlash3].pActor = pActor;

                    pActor->spr.pal += 7;

                    eax = -255;

                    if (!var_18)
                    {
                        double xDiff = fabs(pos.X - pActor->spr.pos.X);
                        double yDiff = fabs(pos.Y - pActor->spr.pos.Y);

                        eax += int(xDiff + yDiff);
                    }

                    if (eax < 0)
                    {
                        int shade = pActor->spr.shade + eax;
                        if (shade < -127) {
                            shade = -127;
                        }

                        pActor->spr.shade = (int8_t)shade;
                    }
                }
            }
        }
    }
}

void UndoFlashes()
{
    int var_24 = 0; // CHECKME - Watcom error "initializer for variable var_24 may not execute

    int edi = -1;

    for (int nFlash = nFirstFlash; nFlash >= 0; nFlash = sFlash[nFlash].next)
    {
        assert(nFlash < 2000 && nFlash >= 0);
        auto pFlash = &sFlash[nFlash];

        uint8_t type = pFlash->nType & 0x3F;

        if (pFlash->nType & 0x80)
        {
            int flashtype = type - 1;
            assert(flashtype >= 0);

            int8_t *pShade = NULL;

            switch (flashtype)
            {
                case 0:
                {
                    pShade = &pFlash->pSector->floorshade;
                    break;
                }

                case 1:
                {
                    pShade = &pFlash->pWall->shade;
                    break;
                }

                case 2:
                {
                    pShade = &pFlash->pSector->ceilingshade;
                    break;
                }

                case 3:
                {
                    DExhumedActor* ac = pFlash->pActor;
                    if (ac && ac->spr.pal >= 7)
                    {
                        pShade = &ac->spr.shade;
                    }
                    else {
                        goto loc_1868A;
                    }

                    break;
                }

                default:
                    break;
            }

            assert(pShade != NULL);

            int thisshade = (*pShade) + 6;
            int maxshade = pFlash->shade;

            if (thisshade < maxshade)
            {
                *pShade = (int8_t)thisshade;
                edi = nFlash;
                continue;
            }
        }

        // loc_185FE
        var_24 = type - 1; // CHECKME - Watcom error "initializer for variable var_24 may not execute
        assert(var_24 >= 0);

        switch (var_24)
        {
            default:
                break;

            case 0:
            {
                pFlash->pSector->floorpal -= 7;
                pFlash->pSector->floorshade = pFlash->shade;
                break;
            }

            case 1:
            {
                pFlash->pWall->pal -= 7;
                pFlash->pWall->shade = pFlash->shade;
                break;
            }

            case 2:
            {
                pFlash->pSector->ceilingpal -= 7;
                pFlash->pSector->ceilingshade = pFlash->shade;
                break;
            }

            case 3:
            {
                DExhumedActor* ac = pFlash->pActor;
                if (ac && ac->spr.pal >= 7)
                {
                    ac->spr.pal -= 7;
                    ac->spr.shade = pFlash->shade;
                }

                break;
            }
        }

loc_1868A:

        if (edi != -1)
        {
            sFlash[edi].next = pFlash->next;
        }

        if (nFlash == nFirstFlash)
        {
            nFirstFlash = sFlash[nFirstFlash].next;
        }

        if (nFlash == nLastFlash)
        {
            nLastFlash = edi;
        }
        pFlash->pActor = nullptr;
        sFlash.Release(nFlash);
    }
}

void AddGlow(sectortype* pSector, int nVal)
{
    if (nGlowCount >= kMaxGlows) {
        return;
    }

    sGlow[nGlowCount].nThreshold = nVal;
    sGlow[nGlowCount].pSector = pSector;
    sGlow[nGlowCount].nShade = -1;
    sGlow[nGlowCount].nCounter = 0;

    nGlowCount++;
}

// ok
void AddFlicker(sectortype* pSector, int nVal)
{
    if (nFlickerCount >= kMaxFlickers) {
        return;
    }

    sFlicker[nFlickerCount].nShade = nVal;
    sFlicker[nFlickerCount].pSector = pSector;

    if (nVal >= 25) {
        nVal = 24;
    }

    sFlicker[nFlickerCount].nMask = flickermask[nVal];

    nFlickerCount++;
}

void DoGlows()
{
    bDoGlows++;

    if (bDoGlows < 3) {
        return;
    }

    bDoGlows = 0;

    for (int i = 0; i < nGlowCount; i++)
    {
        sGlow[i].nCounter++;

        auto pSector = sGlow[i].pSector;
        int nShade = sGlow[i].nShade;

        if (sGlow[i].nCounter >= sGlow[i].nThreshold)
        {
            sGlow[i].nCounter = 0;
            sGlow[i].nShade = -sGlow[i].nShade;
        }

        pSector->ceilingshade += nShade;
        pSector->floorshade   += nShade;

		for(auto& wal : wallsofsector(pSector))
        {
            wal.shade += nShade;
        }
    }
}

void DoFlickers()
{
    bDoFlicks ^= 1;
    if (!bDoFlicks) {
        return;
    }

    for (int i = 0; i < nFlickerCount; i++)
    {
        auto pSector = sFlicker[i].pSector;

        unsigned int eax = (sFlicker[i].nMask & 1);
        unsigned int edx = (sFlicker[i].nMask & 1) << 31;
        unsigned int ebp = sFlicker[i].nMask >> 1;

        ebp |= edx;
        edx = ebp & 1;

        sFlicker[i].nMask = ebp;

        if (edx ^ eax)
        {
            int shade;

            if (eax)
            {
                shade = sFlicker[i].nShade;
            }
            else
            {
                shade = -sFlicker[i].nShade;
            }

            pSector->ceilingshade += shade;
            pSector->floorshade += shade;

			for(auto& wal : wallsofsector(pSector))
            {
                wal.shade += shade;
            }
        }
    }
}

void AddFlow(sectortype* pSector, int nSpeed, int b, int nAngle)
{
    if (nFlowCount >= kMaxFlows || b >= 2)
        return;

    int nFlow = nFlowCount;
    nFlowCount++;

    int nPic = pSector->floorpicnum;

    sFlowInfo[nFlow].angcos  = float(-cos(nAngle * BAngRadian) * nSpeed);
    sFlowInfo[nFlow].angsin = float(sin(nAngle * BAngRadian) * nSpeed);
    sFlowInfo[nFlow].pSector = pSector;

    StartInterpolation(pSector, b ? Interp_Sect_CeilingPanX : Interp_Sect_FloorPanX);
    StartInterpolation(pSector, b ? Interp_Sect_CeilingPanY : Interp_Sect_FloorPanY);

    sFlowInfo[nFlow].type = b;
}


void AddFlow(walltype* pWall, int nSpeed, int b, int nAngle)
{
    if (nFlowCount >= kMaxFlows || b < 2)
        return;

    int nFlow = nFlowCount;
    nFlowCount++;


    // only moves up or down
    StartInterpolation(pWall, Interp_Wall_PanY);

    int nPic = pWall->picnum;

    sFlowInfo[nFlow].angcos = 0;
    sFlowInfo[nFlow].angsin = b == 2 ? 1 : -1;
    sFlowInfo[nFlow].pWall = pWall;

    sFlowInfo[nFlow].type = b;
}

void DoFlows()
{
    for (int i = 0; i < nFlowCount; i++)
    {
        switch (sFlowInfo[i].type)
        {
            case 0:
            {
                auto pSector =sFlowInfo[i].pSector;
                pSector->addfloorxpan(sFlowInfo[i].angcos);
                pSector->addfloorypan(sFlowInfo[i].angsin);
                break;
            }

            case 1:
            {
                auto pSector = sFlowInfo[i].pSector;
                pSector->addceilingxpan(sFlowInfo[i].angcos);
                pSector->addceilingypan(sFlowInfo[i].angsin);
                break;
            }

            case 2:
            {
                auto pWall = sFlowInfo[i].pWall;
                pWall->addypan(sFlowInfo[i].angsin);
                break;
            }

            case 3:
            {
                auto pWall = sFlowInfo[i].pWall;
                pWall->addypan(sFlowInfo[i].angsin);
                break;
            }
        }
    }
}

void DoLights()
{
    DoFlickers();
    DoGlows();
    DoFlows();
}

void SetTorch(int nPlayer, int bTorchOnOff)
{
    if (bTorchOnOff == bTorch) {
        return;
    }

    if (nPlayer != nLocalPlayer) {
        return;
    }

    if (bTorchOnOff == 2) {
        bTorch = !bTorch;
    }
    else {
        bTorch = bTorchOnOff;
    }

    if (bTorch) {
        PlayLocalSound(StaticSound[kSoundTorchOn], 0, false, CHANF_UI);
    }

    const char* buf = bTorch ? "TXT_EX_TORCHLIT" : "TXT_EX_TORCHOUT";
    StatusMessage(150, GStrings(buf));
}

void BuildFlash(int nPlayer, int nVal)
{
    if (nPlayer == nLocalPlayer)
    {
        flash = nVal;
        flash = -nVal; // ???
    }
}

END_PS_NS
