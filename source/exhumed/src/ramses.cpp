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


BEGIN_PS_NS

short cPupData[300];
uint8_t *Worktile;
int lHeadStartClock;
short* pPupData;
int lNextStateChange;
int nPixels;
int nHeadTimeStart;
short nHeadStage;
short curx[97 * 106];
short cury[97 * 106];
int8_t destvelx[97 * 106];
int8_t destvely[97 * 106];
uint8_t pixelval[97 * 106];
int8_t origy[97 * 106];
int8_t origx[97 * 106];
int8_t velx[97 * 106];
int8_t vely[97 * 106];
short nMouthTile;

short nPupData = 0;

short word_964E8 = 0;
short word_964EA = 0;
short word_964EC = 10;

short nSpiritRepeatX;
short nSpiritRepeatY;
short nSpiritSprite;
short nPixelsToShow;
short nTalkTime = 0;


void InitSpiritHead()
{
    char filename[20];

    nPixels = 0;

    nSpiritRepeatX = sprite[nSpiritSprite].xrepeat;
    nSpiritRepeatY = sprite[nSpiritSprite].yrepeat;

    tileLoad(kTileRamsesNormal); // Ramses Normal Head

    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum)
        {
            sprite[i].cstat |= 0x8000;
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
				pixelval[nPixels] = *(pGold + x * 106 + y);
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


    sprite[nSpiritSprite].yrepeat = 140;
    sprite[nSpiritSprite].xrepeat = 140;
    sprite[nSpiritSprite].picnum = kTileRamsesWorkTile;

    nHeadStage = 0;

    // work tile is twice as big as the normal head size
	Worktile = TileFiles.tileCreate(kTileRamsesWorkTile, kSpiritX * 2, kSpiritY * 2);

    sprite[nSpiritSprite].cstat &= 0x7FFF;

    nHeadTimeStart = (int)totalclock;

    memset(Worktile, TRANSPARENT_INDEX, WorktileSize);
    TileFiles.InvalidateTile(kTileRamsesWorkTile);

    nPixelsToShow = 0;

    fadecdaudio();

    int nTrack;

    if (levelnum == 1)
    {
        nTrack = 3;
    }
    else
    {
        nTrack = 7;
    }

    bSubTitles = playCDtrack(nTrack, false) == 0;

    StartSwirlies();

    sprintf(filename, "LEV%d.PUP", levelnum);
    lNextStateChange = (int)totalclock;
    lHeadStartClock = (int)totalclock;

	auto headfd = fileSystem.OpenFileReader(filename);
	if (!headfd.isOpen())
	{
		memset(cPupData, 0, sizeof(cPupData));
	}
	else
	{
		nPupData = headfd.Read(cPupData, sizeof(cPupData));
		pPupData = cPupData;
	}
    nMouthTile = 0;
    nTalkTime = 1;
}

void DimSector(short nSector)
{
    short startwall = sector[nSector].wallptr;
    short nWalls = sector[nSector].wallnum;

    for (int i = 0; i < nWalls; i++)
    {
        if (wall[startwall+i].shade < 40) {
            wall[startwall+i].shade++;
        }
    }

    if (sector[nSector].floorshade < 40) {
        sector[nSector].floorshade++;
    }

    if (sector[nSector].ceilingshade < 40) {
        sector[nSector].ceilingshade++;
    }
}

void CopyHeadToWorkTile(short nTile)
{
	const uint8_t* pSrc = tilePtr(nTile);
    uint8_t *pDest = &Worktile[212 * 49 + 53];

    for (int i = 0; i < kSpiritY; i++)
    {
        memcpy(pDest, pSrc, kSpiritX);

        pDest += 212;
        pSrc += kSpiritX;
    }
}

int DoSpiritHead()
{
    static short word_964E6 = 0;

    PlayerList[0].q16horiz = fix16_sadd(PlayerList[0].q16horiz, fix16_sdiv(fix16_ssub(nDestVertPan[0], PlayerList[0].q16horiz), fix16_from_int(4)));

    TileFiles.InvalidateTile(kTileRamsesWorkTile);

    if (nHeadStage < 2)
    {
        memset(Worktile, TRANSPARENT_INDEX, WorktileSize);
    }

    if (nHeadStage < 2 || nHeadStage != 5)
    {
        nPixelsToShow = ((int)totalclock - nHeadTimeStart) * 15;

        if (nPixelsToShow > nPixels) {
            nPixelsToShow = nPixels;
        }

        if (nHeadStage < 3)
        {
            UpdateSwirlies();

            if (sprite[nSpiritSprite].shade > -127) {
                sprite[nSpiritSprite].shade--;
            }

            word_964E6--;
            if (word_964E6 < 0)
            {
                DimSector(sprite[nSpiritSprite].sectnum);
                word_964E6 = 5;
            }

            if (!nHeadStage)
            {
                if (((int)totalclock - nHeadTimeStart) > 480)
                {
                    nHeadStage = 1;
                    nHeadTimeStart = (int)totalclock + 480;
                }

                for (int i = 0; i < nPixelsToShow; i++)
                {
                    if (destvely[i] >= 0)
                    {
                        vely[i]++;

                        if (vely[i] >= destvely[i])
                        {
                            destvely[i] = -(RandomSize(2) + 1);
                        }
                    }
                    else
                    {
                        vely[i]--;

                        if (vely[i] <= destvely[i])
                        {
                            destvely[i] = RandomSize(2) + 1;
                        }
                    }

                    if (destvelx[i] >= 0)
                    {
                        velx[i]++;

                        if (velx[i] >= destvelx[i])
                        {
                            destvelx[i] = -(RandomSize(2) + 1);
                        }
                    }
                    else
                    {
                        velx[i]--;

                        if (velx[i] <= destvelx[i])
                        {
                            destvelx[i] = RandomSize(2) + 1;
                        }
                    }

                    int esi = vely[i] + (cury[i] >> 8);

                    if (esi < kSpiritX)
                    {
                        if (esi < -105)
                        {
                            vely[i] = 0;
                            esi = 0;
                        }
                    }
                    else
                    {
                        vely[i] = 0;
                        esi = 0;
                    }

                    int ebx = velx[i] + (curx[i] >> 8);

                    if (ebx < kSpiritY)
                    {
                        if (ebx < -96)
                        {
                            velx[i] = 0;
                            ebx = 0;
                        }
                    }
                    else
                    {
                        velx[i] = 0;
                        ebx = 0;
                    }

                    curx[i] = ebx * 256;
                    cury[i] = esi * 256;

                    esi += (ebx + kSpiritY) * 212;

                    Worktile[kSpiritX + esi] = pixelval[i];
                }

                return 1;
            }
            else
            {
                if (nHeadStage != 1) {
                    return 1;
                }

                uint8_t nXRepeat = sprite[nSpiritSprite].xrepeat;
                if (nXRepeat > nSpiritRepeatX)
                {
                    sprite[nSpiritSprite].xrepeat -= 2;

                    nXRepeat = sprite[nSpiritSprite].xrepeat;
                    if (nXRepeat < nSpiritRepeatX)
                    {
                        sprite[nSpiritSprite].xrepeat = nSpiritRepeatX;
                    }
                }

                uint8_t nYRepeat = sprite[nSpiritSprite].yrepeat;
                if (nYRepeat > nSpiritRepeatY)
                {
                    sprite[nSpiritSprite].yrepeat -= 2;

                    nYRepeat = sprite[nSpiritSprite].yrepeat;
                    if (nYRepeat < nSpiritRepeatY)
                    {
                        sprite[nSpiritSprite].yrepeat = nSpiritRepeatY;
                    }
                }

                int esi = 0;

                for (int i = 0; i < nPixels; i++)
                {
                    int eax = (origx[i] << 8) - curx[i];
                    int ecx = eax;

                    if (eax)
                    {
                        if (eax < 0) {
                            eax = -eax;
                        }

                        if (eax < 8)
                        {
                            curx[i] = origx[i] << 8;
                            ecx = 0;
                        }
                        else {
                            ecx >>= 3;
                        }
                    }
                    else
                    {
                        ecx >>= 3;
                    }

                    int var_1C = (origy[i] << 8) - cury[i];
                    int ebp = var_1C;

                    if (var_1C)
                    {
                        eax = ebp;

                        if (eax < 0) {
                            eax = -eax;
                        }

                        if (eax < 8)
                        {
                            cury[i] = origy[i] << 8;
                            var_1C = 0;
                        }
                        else
                        {
                            var_1C >>= 3;
                        }
                    }
                    else
                    {
                        var_1C >>= 3;
                    }

                    if (var_1C || ecx)
                    {
                        curx[i] += ecx;
                        cury[i] += var_1C;

                        esi++;
                    }

                    ecx = (((curx[i] >> 8) + kSpiritY) * 212) + (cury[i] >> 8);

                    Worktile[kSpiritX + ecx] = pixelval[i];
                }

                if (((int)totalclock - lHeadStartClock) > 600)
                {
                    CopyHeadToWorkTile(kTileRamsesGold);
                }

                int eax = ((nPixels << 4) - nPixels) / 16;

                if (esi < eax)
                {
                    SoundBigEntrance();
                    AddGlow(sprite[nSpiritSprite].sectnum, 20);
                    AddFlash(
                        sprite[nSpiritSprite].sectnum,
                        sprite[nSpiritSprite].x,
                        sprite[nSpiritSprite].y,
                        sprite[nSpiritSprite].z,
                        128);

                    nHeadStage = 3;
                    TintPalette(255, 255, 255);
                    CopyHeadToWorkTile(kTileRamsesNormal);
                }

                return 1;
            }
        }
        else
        {
            FixPalette();

            if (!nPalDiff)
            {
                nFreeze = 2;
                nHeadStage++;
            }

            return 0;
        }
    }
    else
    {
        if (lNextStateChange <= (int)totalclock)
        {
            if (nPupData)
            {
                short nPupVal = *pPupData;
                pPupData++;
                nPupData -= 2;

                if (nPupData > 0)
                {
                    lNextStateChange = (nPupVal + lHeadStartClock) - 10;
                    nTalkTime = !nTalkTime;
                }
                else
                {
                    nTalkTime = 0;
                    nPupData = 0;
                }
            }
            else if (!bSubTitles)
            {
                if (!CDplaying())
                {
                    EndLevel = 2;
                }
            }
        }

        word_964E8--;
        if (word_964E8 <= 0)
        {
            word_964EA = RandomBit() * 2;
            word_964E8 = RandomSize(5) + 4;
        }

        int ebx = 592;
        word_964EC--;

        if (word_964EC < 3)
        {
            ebx = 593;
            if (word_964EC <= 0) {
                word_964EC = RandomSize(6) + 4;
            }
        }

        ebx += word_964EA;

        uint8_t *pDest = &Worktile[10441];
		const uint8_t* pSrc = tilePtr(ebx);

        for (int i = 0; i < kSpiritY; i++)
        {
            memcpy(pDest, pSrc, kSpiritX);

            pDest += 212;
            pSrc += kSpiritX;
        }

        if (nTalkTime)
        {
            if (nMouthTile < 2) {
                nMouthTile++;
            }
        }
        else if (nMouthTile != 0)
        {
            nMouthTile--;
        }

        if (nMouthTile)
        {
            short nTileSizeX = tilesiz[nMouthTile + 598].x;
            short nTileSizeY = tilesiz[nMouthTile + 598].y;

            uint8_t *pDest = &Worktile[212 * (kSpiritY - nTileSizeX / 2)] + (159 - nTileSizeY);
            const uint8_t *pSrc = tilePtr(nMouthTile + 598);

            while (nTileSizeX > 0)
            {
                memcpy(pDest, pSrc, nTileSizeY);

                nTileSizeX--;
                pDest += 212;
                pSrc += nTileSizeY;
            }
        }

        return 1;
    }

    return 0;
}

// This is only the static global data.
static SavegameHelper sgh("ramses",
    SA(cPupData),
    SV(nPupData),
    SV(nPixels),
    SA(curx),
    SA(cury),
    SA(destvelx),
    SA(destvely),
    SA(pixelval),
    SA(origy),
    SA(origx),
    SA(velx),
    SA(vely),
    SV(nMouthTile),
    SV(nSpiritSprite),
    SV(word_964E8),
    SV(word_964EA),
    SV(word_964EC),
    SV(nSpiritRepeatX),
    SV(nSpiritRepeatY),
    SV(nPixelsToShow),
    SV(nTalkTime),
    SV(lHeadStartClock),
    SV(lNextStateChange),
    SV(nHeadStage),
    SV(nHeadTimeStart)
    );

END_PS_NS
