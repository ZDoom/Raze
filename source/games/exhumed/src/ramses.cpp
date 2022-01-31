//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2020 EDuke32 developers and contributors
Copyright (C) 2020 sirlemonhead, Nuke.YKT
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

#include "engine.h"
#include "exhumed.h"
#include "sound.h"
#include "view.h"
#include "names.h"
#include "aistuff.h"
#include "player.h"
#include "mapinfo.h"
#include "input.h"


BEGIN_PS_NS

int16_t cPupData[300];
uint8_t *Worktile;
int lHeadStartClock;
int16_t* pPupData;
int lNextStateChange;
int nPixels;
int nHeadTimeStart;
int nHeadStage;
int16_t curx[kSpiritY * kSpiritX];
int16_t cury[kSpiritY * kSpiritX];
int8_t destvelx[kSpiritY * kSpiritX];
int8_t destvely[kSpiritY * kSpiritX];
uint8_t pixelval[kSpiritY * kSpiritX];
int8_t origy[kSpiritY * kSpiritX];
int8_t origx[kSpiritY * kSpiritX];
int8_t velx[kSpiritY * kSpiritX];
int8_t vely[kSpiritY * kSpiritX];
int16_t nMouthTile;

int nPupData = 0;

int word_964E8 = 0;
int word_964EA = 0;
int word_964EC = 10;

int nSpiritRepeatX;
int nSpiritRepeatY;
TObjPtr<DExhumedActor*> pSpiritSprite;
int nPixelsToShow;
int nTalkTime = 0;


void InitSpiritHead()
{
    nPixels = 0;
    auto pSpiritSpr = pSpiritSprite;

    nSpiritRepeatX = pSpiritSpr->spr.xrepeat;
    nSpiritRepeatY = pSpiritSpr->spr.yrepeat;

    tileLoad(kTileRamsesNormal); // Ramses Normal Head

    ExhumedSpriteIterator it;
    while (auto act = it.Next())
    {
        if (act->spr.statnum)
        {
            act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        }
    }

	auto pTile = tilePtr(kTileRamsesNormal); // Ramses Normal Head
	auto pGold = tilePtr(kTileRamsesGold);
    for (int x = 0; x < 97; x++)
    {
        for (int y = 0; y < 106; y++)
        {
            if (*pTile != TRANSPARENT_INDEX)
            {
				pixelval[nPixels] = *(pGold + x * kSpiritX + y);
                origx[nPixels] = x - 48;
                origy[nPixels] = y - 53;
                curx[nPixels] = 0;
                cury[nPixels] = 0;
                vely[nPixels] = 0;
                velx[nPixels] = 0;

                destvelx[nPixels] = RandomSize(2) + 1;

                if (curx[nPixels] > 0) {
                    destvelx[nPixels] = -destvelx[nPixels];
                }

                destvely[nPixels] = RandomSize(2) + 1;

                if (cury[nPixels] > 0) {
                    destvely[nPixels] = -destvely[nPixels];
                }

                nPixels++;
            }

            pTile++;
        }
    }


    pSpiritSpr->spr.yrepeat = 140;
    pSpiritSpr->spr.xrepeat = 140;
    pSpiritSpr->spr.picnum = kTileRamsesWorkTile;

    nHeadStage = 0;

    // work tile is twice as big as the normal head size
	Worktile = TileFiles.tileCreate(kTileRamsesWorkTile, kSpiritY * 2, kSpiritX * 2);

    pSpiritSpr->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;

    nHeadTimeStart = PlayClock;

    memset(Worktile, TRANSPARENT_INDEX, WorktileSize);
    TileFiles.InvalidateTile(kTileRamsesWorkTile);

    nPixelsToShow = 0;

    fadecdaudio();

    int nTrack = currentLevel->ex_ramses_cdtrack;
    playCDtrack(nTrack, false);

    StartSwirlies();

    lNextStateChange = PlayClock;
    lHeadStartClock = PlayClock;

	auto headfd = fileSystem.OpenFileReader(currentLevel->ex_ramses_pup);
	if (!headfd.isOpen())
	{
		memset(cPupData, 0, sizeof(cPupData));
	}
	else
	{
		nPupData = (int)headfd.Read(cPupData, sizeof(cPupData));
		pPupData = cPupData;
	}
    nMouthTile = 0;
    nTalkTime = 1;
}

void DimSector(sectortype* pSector)
{
	for(auto& wal : wallsofsector(pSector))
    {
        if (wal.shade < 40) {
            wal.shade++;
        }
    }

    if (pSector->floorshade < 40) {
        pSector->floorshade++;
    }

    if (pSector->ceilingshade < 40) {
        pSector->ceilingshade++;
    }
}

void CopyHeadToWorkTile(int nTile)
{
	const uint8_t* pSrc = tilePtr(nTile);
    uint8_t *pDest = &Worktile[212 * 49 + 53];

    for (unsigned i = 0; i < kSpiritY; i++)
    {
        memcpy(pDest, pSrc, kSpiritX);

        pDest += 212;
        pSrc += kSpiritX;
    }
}

// This is based on BuildGDX's version of this function which was a lot less cryptic than PCExhumed's.
void DoSpiritHead() 
{
    static int dimSectCount = 0;
    auto pSpiritSpr = pSpiritSprite;

    sPlayerInput[0].actions |= SB_CENTERVIEW;
    TileFiles.InvalidateTile(kTileRamsesWorkTile);

    switch (nHeadStage) 
    {
    case 0:
    case 1:
        memset(Worktile, TRANSPARENT_INDEX, WorktileSize);
        break;
    case 5:
        if (lNextStateChange <= PlayClock) 
        {
            if (nPupData != 0) 
            {
                int clock = *pPupData++;
                nPupData -= 2;
                if (nPupData > 0) 
                {
                    lNextStateChange = lHeadStartClock + clock - 10;
                    nTalkTime = !nTalkTime;
                }
                else 
                {
                    nTalkTime = false;
                    nPupData = 0;
                }
            }
            else if (!bSubTitles)
            {
                if (!CDplaying())
                {
                    LevelFinished();
                    EndLevel = 1;
                }
            }
        }

        if (--word_964E8 <= 0) 
        {
            word_964EA = 2 * RandomBit();
            word_964E8 = RandomSize(5) + 4;
        }

        int tilenum = kTileRamsesNormal;
        if (--word_964EC < 3) 
        {
            tilenum = 593;
            if (word_964EC <= 0)
                word_964EC = RandomSize(6) + 4;
        }

        CopyHeadToWorkTile(word_964EA + tilenum);

        if (nTalkTime) 
        {
            if (nMouthTile < 2)
                nMouthTile++;
        }
        else if (nMouthTile != 0)
            nMouthTile--;

        if (nMouthTile != 0) 
        {
            int srctile = nMouthTile + 598;
            auto src = tilePtr(srctile);
            int sizx = tileWidth(srctile);
            int sizy = tileHeight(srctile);
            int workptr = 212 * (97 - sizx / 2) + 159 - sizy;
            int srcptr = 0;
            while (sizx > 0) 
            {
                memcpy(Worktile + workptr, src + srcptr, sizy);
                workptr += 212;
                srcptr += sizy;
                sizx--;
            }
        }
        return;
    }

    nPixelsToShow = 15 * (PlayClock - nHeadTimeStart);
    if (nPixelsToShow > nPixels)
        nPixelsToShow = nPixels;

    switch (nHeadStage) 
    {
    case 3:
        FixPalette();
        if (nPalDiff == 0) 
        {
            nFreeze = 2;
            nHeadStage++;
        }
        return;
    case 0:
    case 1:
    case 2:
        UpdateSwirlies();
        if (pSpiritSpr->spr.shade > -127)
            pSpiritSpr->spr.shade--;
        if (--dimSectCount < 0) 
        {
            DimSector(pSpiritSpr->sector());
            dimSectCount = 5;
        }

        if (nHeadStage == 0) 
        {
            if (PlayClock - nHeadTimeStart > 480) 
            {
                nHeadStage = 1;
                nHeadTimeStart = PlayClock + 480;
            }

            for (int i = 0; i < nPixelsToShow; i++) 
            {
                if (destvely[i] >= 0) 
                {
                    if (++vely[i] >= destvely[i]) 
                    {
                        destvely[i] = (int8_t)-(RandomSize(2) + 1);
                    }
                }
                else {
                    if (--vely[i] <= destvely[i]) 
                    {
                        destvely[i] = (int8_t)(RandomSize(2) + 1);
                    }
                }

                if (destvelx[i] >= 0) {
                    if (++velx[i] >= destvelx[i]) 
                    {
                        destvelx[i] = (int8_t)-(RandomSize(2) + 1);
                    }
                }
                else {
                    if (--velx[i] <= destvelx[i]) 
                    {
                        destvelx[i] = (int8_t)(RandomSize(2) + 1);
                    }
                }

                int x = (curx[i] >> 8) + velx[i];
                if (x < 97) 
                {
                    if (x < -96) 
                    {
                        x = 0;
                        velx[i] = 0;
                    }
                }
                else 
                {
                    x = 0;
                    velx[i] = 0;
                }

                int y = (cury[i] >> 8) + vely[i];
                if (y < 106) 
                {
                    if (y < -105) 
                    {
                        y = 0;
                        vely[i] = 0;
                    }
                }
                else 
                {
                    y = 0;
                    vely[i] = 0;
                }

                curx[i] = (int)(x << 8);
                cury[i] = (int)(y << 8);

                Worktile[212 * (x + 97) + 106 + y] = pixelval[i++];
            }
        }

        if (nHeadStage == 1) 
        {
            if (pSpiritSpr->spr.xrepeat > nSpiritRepeatX) 
            {
                pSpiritSpr->spr.xrepeat -= 2;
                if (pSpiritSpr->spr.xrepeat < nSpiritRepeatX)
                    pSpiritSpr->spr.xrepeat = (uint8_t)nSpiritRepeatX;
            }
            if (pSpiritSpr->spr.yrepeat > nSpiritRepeatY) 
            {
                pSpiritSpr->spr.yrepeat -= 2;
                if (pSpiritSpr->spr.yrepeat < nSpiritRepeatY)
                    pSpiritSpr->spr.yrepeat = (uint8_t)nSpiritRepeatY;
            }

            int nCount = 0;
            for (int i = 0; i < nPixels; i++) 
            {
                int dx, dy;
                if (origx[i] << 8 == curx[i] || abs((origx[i] << 8) - curx[i]) >= 8)
                    dx = ((origx[i] << 8) - curx[i]) >> 3;
                else {
                    dx = 0;
                    curx[i] = (int)(origx[i] << 8);
                }

                if (origy[i] << 8 == cury[i] || abs((origy[i] << 8) - cury[i]) >= 8)
                    dy = ((origy[i] << 8) - cury[i]) >> 3;
                else {
                    dy = 0;
                    cury[i] = (int)(origy[i] << 8);
                }

                if ((dx | dy) != 0) 
                {
                    curx[i] += dx;
                    cury[i] += dy;
                    nCount++;
                }

                Worktile[((cury[i] >> 8) + (212 * ((curx[i] >> 8) + 97))) + 106] = pixelval[i];
            }

            if (PlayClock - lHeadStartClock > 600)
                CopyHeadToWorkTile(590);

            if (nCount < (15 * nPixels) / 16) {
                SoundBigEntrance();
                AddGlow(pSpiritSpr->sector(), 20);
                AddFlash(pSpiritSpr->sector(), pSpiritSpr->int_pos().X, pSpiritSpr->int_pos().Y, pSpiritSpr->int_pos().Z, 128);
                nHeadStage = 3;
                TintPalette(255, 255, 255);
                CopyHeadToWorkTile(kTileRamsesNormal);
            }
        }
        break;
    }
}

END_PS_NS
