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
    int8_t nType;
    int8_t shade;
    DExhumedActor* pActor;
    int nIndex;
    int next;
};

struct Glow
{
    short field_0;
    short field_2;
    int nSector;
    short field_6;
};

struct Flicker
{
    short field_0;
    int nSector;
    unsigned int field_4;
};

struct Flow
{
    int objindex;
    int type;
    int xdelta;
    int ydelta;
    int angcos;
    int angsin;
    int xacc;
    int yacc;
};


FreeListArray<Flash, kMaxFlashes> sFlash;

Glow sGlow[kMaxGlows];
Flicker sFlicker[kMaxFlickers];
Flow sFlowInfo[kMaxFlows];
int flickermask[kMaxFlickerMask];

short bTorch = 0;
short nFirstFlash = -1;
short nLastFlash = -1;
short nFlashDepth = 2;
short nFlowCount;
short nFlickerCount;
short nGlowCount;

int bDoFlicks = 0;
int bDoGlows = 0;


FSerializer& Serialize(FSerializer& arc, const char* keyname, Flash& w, Flash* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.nType)
            ("shade", w.shade)
            ("at1", w.nIndex)
            ("next", w.next)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Glow& w, Glow* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("at2", w.field_2)
            ("sector", w.nSector)
            ("at6", w.field_6)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Flicker& w, Flicker* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("at0", w.field_0)
            ("sector", w.nSector)
            ("at4", w.field_4)
            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Flow& w, Flow* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("objindex", w.objindex)
            ("type", w.type)
            ("xdelta", w.xdelta)
            ("ydelta", w.ydelta)
            ("atc", w.angcos)
            ("at10", w.angsin)
            ("xacc", w.xacc)
            ("yacc", w.yacc)
            .EndObject();
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

void AddFlash(int nSector, int x, int y, int z, int val)
{
    assert(nSector >= 0 && nSector < kMaxSectors);
    auto sectp = &sector[nSector];

    int var_28 = 0;
    int var_1C = val >> 8;

    if (var_1C >= nFlashDepth) {
        return;
    }

    unsigned int var_20 = val & 0x80;
    unsigned int var_18 = val & 0x40;

    val = ((var_1C + 1) << 8) | (val & 0xff);

    int var_14 = 0;

	for (auto& wal : wallsofsector(sectp))
    {
		auto average = wal.center();

        sectortype *pNextSector = NULL;
        if (wal.nextsector > -1)
            pNextSector = wal.nextSector();

        int ebx = -255;

        if (!var_18)
        {
            int x2 = x - average.x;
            if (x2 < 0) {
                x2 = -x2;
            }

            ebx = x2;

            int y2 = y - average.y;
            if (y2 < 0) {
                y2 = -y2;
            }

            ebx = ((y2 + ebx) >> 4) - 255;
        }

        if (ebx < 0)
        {
            var_14++;
            var_28 += ebx;

            if (wal.pal < 5)
            {
                if (!pNextSector || pNextSector->floorz < sectp->floorz)
                {
                    short nFlash = GrabFlash();
                    if (nFlash < 0) {
                        return;
                    }

                    sFlash[nFlash].nType = var_20 | 2;
                    sFlash[nFlash].shade = wal.shade;
					sFlash[nFlash].nIndex = wallnum(&wal);

                    wal.pal += 7;

                    ebx += wal.shade;
                    int eax = ebx;

                    if (ebx < -127) {
                        eax = -127;
                    }

                    wal.shade = eax;

                    if (!var_1C && !wal.overpicnum && pNextSector)
                    {
                        AddFlash(wal.nextsector, x, y, z, val);
                    }
                }
            }
        }
    }

    if (var_14 && sectp->floorpal < 4)
    {
        short nFlash = GrabFlash();
        if (nFlash < 0) {
            return;
        }

        sFlash[nFlash].nType = var_20 | 1;
        sFlash[nFlash].nIndex = nSector;
        sFlash[nFlash].shade = sectp->floorshade;

        sectp->floorpal += 7;

        int edx = sectp->floorshade + var_28;
        int eax = edx;

        if (edx < -127) {
            eax = -127;
        }

        sectp->floorshade = eax;

        if (!(sectp->ceilingstat & 1))
        {
            if (sectp->ceilingpal < 4)
            {
                short nFlash2 = GrabFlash();
                if (nFlash2 >= 0)
                {
                    sFlash[nFlash2].nType = var_20 | 3;
                    sFlash[nFlash2].nIndex = nSector;
                    sFlash[nFlash2].shade = sectp->ceilingshade;

                    sectp->ceilingpal += 7;

                    int edx = sectp->ceilingshade + var_28;
                    int eax = edx;

                    if (edx < -127) {
                        eax = -127;
                    }

                    sectp->ceilingshade = eax;
                }
            }
        }

        ExhumedSectIterator it(nSector);
        while (auto pActor = it.Next())
        {
			auto pSprite = &pActor->s();
            if (pSprite->pal < 4)
            {
                short nFlash3 = GrabFlash();
                if (nFlash3 >= 0)
                {
                    sFlash[nFlash3].nType = var_20 | 4;
                    sFlash[nFlash3].shade = pSprite->shade;
                    sFlash[nFlash3].nIndex = -1;
                    sFlash[nFlash3].pActor = pActor;

                    pSprite->pal += 7;

                    int eax = -255;

                    if (!var_18)
                    {
                        int xDiff = x - pSprite->x;
                        if (xDiff < 0) {
                            xDiff = -xDiff;
                        }

                        int yDiff = y - pSprite->y;
                        if (yDiff < 0) {
                            yDiff = -yDiff;
                        }

                        eax = ((xDiff + yDiff) >> 4) - 255;
                    }

                    if (eax < 0)
                    {
                        short shade = pSprite->shade + eax;
                        if (shade < -127) {
                            shade = -127;
                        }

                        pSprite->shade = (int8_t)shade;
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

        uint8_t type = sFlash[nFlash].nType & 0x3F;
        int nIndex = sFlash[nFlash].nIndex;

        if (sFlash[nFlash].nType & 0x80)
        {
            int flashtype = type - 1;
            assert(flashtype >= 0);

            int8_t *pShade = NULL;

            switch (flashtype)
            {
                case 0:
                {
                    assert(nIndex >= 0 && nIndex < kMaxSectors);

                    pShade = &sector[nIndex].floorshade;
                    break;
                }

                case 1:
                {
                    assert(nIndex >= 0 && nIndex < kMaxWalls);

                    pShade = &wall[nIndex].shade;
                    break;
                }

                case 2:
                {
                    assert(nIndex >= 0 && nIndex < kMaxSectors);

                    pShade = &sector[nIndex].ceilingshade;
                    break;
                }

                case 3:
                {
                    auto ac = sFlash[nFlash].pActor;
                    if (!ac) continue;
                    auto sp = &ac->s();
                    if (sp->pal >= 7)
                    {
                        pShade = &sp->shade;
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
            int maxshade = sFlash[nFlash].shade;

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
                sector[nIndex].floorpal -= 7;
                sector[nIndex].floorshade = sFlash[nFlash].shade;
                break;
            }

            case 1:
            {
                wall[nIndex].pal -= 7;
                wall[nIndex].shade = sFlash[nFlash].shade;
                break;
            }

            case 2:
            {
                sector[nIndex].ceilingpal -= 7;
                sector[nIndex].ceilingshade = sFlash[nFlash].shade;
                break;
            }

            case 3:
            {
                auto ac = sFlash[nFlash].pActor;
                auto sp = &ac->s();
                if (sp->pal >= 7)
                {
                    sp->pal -= 7;
                    sp->shade = sFlash[nFlash].shade;
                }

                break;
            }
        }

loc_1868A:

        if (edi != -1)
        {
            sFlash[edi].next = sFlash[nFlash].next;
        }

        if (nFlash == nFirstFlash)
        {
            nFirstFlash = sFlash[nFirstFlash].next;
        }

        if (nFlash == nLastFlash)
        {
            nLastFlash = edi;
        }
        sFlash.Release(nFlash);
    }
}

void AddGlow(int nSector, int nVal)
{
    if (nGlowCount >= kMaxGlows) {
        return;
    }

    sGlow[nGlowCount].field_6 = nVal;
    sGlow[nGlowCount].nSector = nSector;
    sGlow[nGlowCount].field_0 = -1;
    sGlow[nGlowCount].field_2 = 0;

    nGlowCount++;
}

// ok
void AddFlicker(int nSector, int nVal)
{
    if (nFlickerCount >= kMaxFlickers) {
        return;
    }

    sFlicker[nFlickerCount].field_0 = nVal;
    sFlicker[nFlickerCount].nSector = nSector;

    if (nVal >= 25) {
        nVal = 24;
    }

    sFlicker[nFlickerCount].field_4 = flickermask[nVal];

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
        sGlow[i].field_2++;

        int nSector =sGlow[i].nSector;
        auto sectp = &sector[nSector];
        short nShade = sGlow[i].field_0;

        if (sGlow[i].field_2 >= sGlow[i].field_6)
        {
            sGlow[i].field_2 = 0;
            sGlow[i].field_0 = -sGlow[i].field_0;
        }

        sectp->ceilingshade += nShade;
        sectp->floorshade   += nShade;

        int startwall = sectp->wallptr;
        int endwall = startwall + sectp->wallnum - 1;

        for (int nWall = startwall; nWall <= endwall; nWall++)
        {
            wall[nWall].shade += nShade;

            // CHECKME - ASM has edx decreasing here. why?
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
        int nSector =sFlicker[i].nSector;
        auto sectp = &sector[nSector];
 
        unsigned int eax = (sFlicker[i].field_4 & 1);
        unsigned int edx = (sFlicker[i].field_4 & 1) << 31;
        unsigned int ebp = sFlicker[i].field_4 >> 1;

        ebp |= edx;
        edx = ebp & 1;

        sFlicker[i].field_4 = ebp;

        if (edx ^ eax)
        {
            short shade;

            if (eax)
            {
                shade = sFlicker[i].field_0;
            }
            else
            {
                shade = -sFlicker[i].field_0;
            }

            sectp->ceilingshade += shade;
            sectp->floorshade += shade;

            int startwall = sectp->wallptr;
            int endwall = startwall + sectp->wallnum - 1;

            for (int nWall = endwall; nWall >= startwall; nWall--)
            {
                wall[nWall].shade += shade;

                // CHECKME - ASM has edx decreasing here. why?
            }
        }
    }
}

// nWall can also be passed in here via nSprite parameter - TODO - rename nSprite parameter :)
void AddFlow(int nIndex, int nSpeed, int b, int nAngle)
{
    if (nFlowCount >= kMaxFlows)
        return;

    short nFlow = nFlowCount;
    nFlowCount++;


    if (b < 2)
    {
        short nPic = sector[nIndex].floorpicnum;

        sFlowInfo[nFlow].xacc = (tileWidth(nPic) << 14) - 1;
        sFlowInfo[nFlow].yacc = (tileHeight(nPic) << 14) - 1;
        sFlowInfo[nFlow].angcos  = -bcos(nAngle) * nSpeed;
        sFlowInfo[nFlow].angsin = bsin(nAngle) * nSpeed;
        sFlowInfo[nFlow].objindex = nIndex;

        StartInterpolation(nIndex, b ? Interp_Sect_CeilingPanX : Interp_Sect_FloorPanX);
        StartInterpolation(nIndex, b ? Interp_Sect_CeilingPanY : Interp_Sect_FloorPanY);
    }
    else
    {
        StartInterpolation(nIndex, Interp_Wall_PanX);
        StartInterpolation(nIndex, Interp_Wall_PanY);

        int nAngle;

        if (b == 2) {
            nAngle = 512;
        }
        else {
            nAngle = 1536;
        }

        short nPic = wall[nIndex].picnum;

        sFlowInfo[nFlow].xacc = (tileWidth(nPic) * wall[nIndex].xrepeat) << 8;
        sFlowInfo[nFlow].yacc = (tileHeight(nPic) * wall[nIndex].yrepeat) << 8;
        sFlowInfo[nFlow].angcos = -bcos(nAngle) * nSpeed;
        sFlowInfo[nFlow].angsin = bsin(nAngle) * nSpeed;
        sFlowInfo[nFlow].objindex = nIndex;
    }

    sFlowInfo[nFlow].ydelta = 0;
    sFlowInfo[nFlow].xdelta = 0;
    sFlowInfo[nFlow].type = b;
}

void DoFlows()
{
    for (int i = 0; i < nFlowCount; i++)
    {
        sFlowInfo[i].xdelta += sFlowInfo[i].angcos;
        sFlowInfo[i].ydelta += sFlowInfo[i].angsin;

        switch (sFlowInfo[i].type)
        {
            case 0:
            {
                sFlowInfo[i].xdelta &= sFlowInfo[i].xacc;
                sFlowInfo[i].ydelta &= sFlowInfo[i].yacc;

                int nSector =sFlowInfo[i].objindex;
                sector[nSector].setfloorxpan(sFlowInfo[i].xdelta / 16384.f);
                sector[nSector].setfloorypan(sFlowInfo[i].ydelta / 16384.f);
                break;
            }

            case 1:
            {
                int nSector =sFlowInfo[i].objindex;

                sector[nSector].setceilingxpan(sFlowInfo[i].xdelta / 16384.f);
                sector[nSector].setceilingypan(sFlowInfo[i].ydelta / 16384.f);

                sFlowInfo[i].xdelta &= sFlowInfo[i].xacc;
                sFlowInfo[i].ydelta &= sFlowInfo[i].yacc;
                break;
            }

            case 2:
            {
                int nWall = sFlowInfo[i].objindex;

                wall[nWall].setxpan(sFlowInfo[i].xdelta / 16384.f);
                wall[nWall].setypan(sFlowInfo[i].ydelta / 16384.f);

                if (sFlowInfo[i].xdelta < 0)
                {
                    sFlowInfo[i].xdelta += sFlowInfo[i].xacc;
                }

                if (sFlowInfo[i].ydelta < 0)
                {
                    sFlowInfo[i].ydelta += sFlowInfo[i].yacc;
                }

                break;
            }

            case 3:
            {
                int nWall = sFlowInfo[i].objindex;

                wall[nWall].setxpan(sFlowInfo[i].xdelta / 16384.f);
                wall[nWall].setypan(sFlowInfo[i].ydelta / 16384.f);

                if (sFlowInfo[i].xdelta >= sFlowInfo[i].xacc)
                {
                    sFlowInfo[i].xdelta -= sFlowInfo[i].xacc;
                }

                if (sFlowInfo[i].ydelta >= sFlowInfo[i].yacc)
                {
                    sFlowInfo[i].ydelta -= sFlowInfo[i].yacc;
                }

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
        PlayLocalSound(StaticSound[kSoundTorchOn], 0);
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
