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
#include "engine.h"
#include "player.h"
#include "anims.h"
#include "status.h"
#include "exhumed.h"
#include "sequence.h"
#include "init.h"
#include "names.h"
#include "items.h"
#include "view.h"
#include "trigdat.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "typedefs.h"

BEGIN_PS_NS

short nMaskY;
static short nAnimsFree = 0;

short statusmask[MAXXDIM];

short message_timer = 0;
char message_text[80];
int magicperline;
int airperline;
int healthperline;
int nAirFrames;
int nCounter;
int nCounterDest;

short nStatusSeqOffset;
short nItemFrames;

int laststatusx;
int laststatusy;

int16_t nItemSeq;
short nDigit[3];

short nMagicFrames;
short nHealthLevel;
short nItemFrame;
short nMeterRange;
short nMagicLevel;
short nHealthFrame;
short nMagicFrame;

short statusx;
short statusy;
short nHealthFrames;

short airframe;

int16_t nFirstAnim;
int16_t nLastAnim;
short nItemAltSeq;

short airpages = 0;

short ammodelay = 3;

short nCounterBullet = -1;


// 8 bytes
struct statusAnim
{
    int16_t s1;
    int16_t s2;
//    int16_t nPage;
    int8_t nPrevAnim;
    int8_t nNextAnim;
};

#define kMaxStatusAnims		50

statusAnim StatusAnim[kMaxStatusAnims];
uint8_t StatusAnimsFree[kMaxStatusAnims];
uint8_t StatusAnimFlags[kMaxStatusAnims];

short nItemSeqOffset[] = {91, 72, 76, 79, 68, 87, 83};

short word_9AD54[kMaxPlayers] = {0, 0, 0, 0, 0, 0, 0, 0};
int dword_9AD64[kMaxPlayers] = {0, 0, 0, 0, 0, 0, 0, 0};

void SetCounterDigits();
void SetItemSeq();
void SetItemSeq2(int nSeqOffset);
void DestroyStatusAnim(short nAnim);


int BuildStatusAnim(int val, int nFlags)
{
    // destroy this anim if it already exists
    for (int i = nFirstAnim; i >= 0; i = StatusAnim[i].nPrevAnim)
    {
        if (StatusAnim[i].s1 == val) {
            DestroyStatusAnim(i);
            break;
        }
    }

    if (nAnimsFree <= 0) {
        return -1;
    }

    nAnimsFree--;

    uint8_t nStatusAnim = StatusAnimsFree[nAnimsFree];

    StatusAnim[nStatusAnim].nPrevAnim = -1;
    StatusAnim[nStatusAnim].nNextAnim = nLastAnim;

    if (nLastAnim < 0) {
        nFirstAnim = nStatusAnim;
    }
    else {
        StatusAnim[nLastAnim].nPrevAnim = nStatusAnim;
    }

    nLastAnim = nStatusAnim;

    StatusAnim[nStatusAnim].s1 = val;
    StatusAnim[nStatusAnim].s2 = 0;
    StatusAnimFlags[nStatusAnim] = nFlags;
    return nStatusAnim;
}

void RefreshStatus()
{
    short nLives = nPlayerLives[nLocalPlayer];
    if (nLives < 0 || nLives > kMaxPlayerLives) {
        I_Error("illegal value for nPlayerLives #%d\n", nLocalPlayer);
    }

    // draws the red dots that indicate the lives amount
    BuildStatusAnim(145 + (2 * nLives), 0);

    uint16_t nKeys = PlayerList[nLocalPlayer].keys;

    int val = 37;

    for (int i = 0; i < 4; i++)
    {
        if (nKeys & 0x1000) {
            BuildStatusAnim(val, 0);
        }

        nKeys >>= 1;
        val += 2;
    }

    SetPlayerItem(nLocalPlayer, nPlayerItem[nLocalPlayer]);
    SetHealthFrame(0);
    SetMagicFrame();
    SetAirFrame();
}

void InitStatus()
{
    nStatusSeqOffset = SeqOffsets[kSeqStatus];
    nHealthFrames = SeqSize[nStatusSeqOffset + 1];
    int nPicNum   = seq_GetSeqPicnum(kSeqStatus, 1, 0);
    nMagicFrames  = SeqSize[nStatusSeqOffset + 129];
    nHealthFrame  = 0;
    nMagicFrame   = 0;
    nHealthLevel  = 0;
    nMagicLevel   = 0;
    nMeterRange   = tilesiz[nPicNum].y;
    magicperline  = 1000 / nMeterRange;
    healthperline = 800 / nMeterRange;
    nAirFrames = SeqSize[nStatusSeqOffset + 133];
    airperline = 100 / nAirFrames;
    nCounter   = 0;
    nCounterDest = 0;

    memset(nDigit, 0, sizeof(nDigit));

    SetCounter(0);
    SetHealthFrame(0);
    SetMagicFrame();

    for (int i = 0; i < kMaxStatusAnims; i++) {
        StatusAnimsFree[i] = i;
    }

    nLastAnim  = -1;
    nFirstAnim = -1;
    nItemSeq   = -1;
    nAnimsFree = kMaxStatusAnims;
    statusx    = xdim - 320;
    textpages  = 0;
    message_timer = 0;
    statusy = ydim - 200;
}

void MoveStatusAnims()
{
    for (int i = nFirstAnim; i >= 0; i = StatusAnim[i].nPrevAnim)
    {
        seq_MoveSequence(-1, nStatusSeqOffset + StatusAnim[i].s1, StatusAnim[i].s2);

        StatusAnim[i].s2++;

        short nSize = SeqSize[nStatusSeqOffset + StatusAnim[i].s1];

        if (StatusAnim[i].s2 >= nSize)
        {
            if (StatusAnimFlags[i] & 0x10) {
                StatusAnim[i].s2 = 0;
            }
            else {
                StatusAnim[i].s2 = nSize - 1; // restart it
            }
        }
    }
}

void DestroyStatusAnim(short nAnim)
{
    int8_t nPrev = StatusAnim[nAnim].nPrevAnim;
    int8_t nNext = StatusAnim[nAnim].nNextAnim;

    if (nNext >= 0) {
        StatusAnim[nNext].nPrevAnim = nPrev;
    }

    if (nPrev >= 0) {
        StatusAnim[nPrev].nNextAnim = nNext;
    }

    if (nAnim == nFirstAnim) {
        nFirstAnim = nPrev;
    }

    if (nAnim == nLastAnim) {
        nLastAnim = nNext;
    }

    StatusAnimsFree[nAnimsFree] = (uint8_t)nAnim;
    nAnimsFree++;
}

void DrawStatusAnims()
{
    for (int i = nFirstAnim; i >= 0; i = StatusAnim[i].nPrevAnim)
    {
        int nSequence = nStatusSeqOffset + StatusAnim[i].s1;

        //seq_DrawStatusSequence(nSequence, StatusAnim[i].s2, 0);

/*
        if (StatusAnim[nAnim].s2 >= (SeqSize[nSequence] - 1))
        {
            if (!(StatusAnimFlags[nAnim] & 0x10))
            {
                StatusAnim[nAnim].nPage--;
                if (StatusAnim[nAnim].nPage <= 0) {
                    DestroyStatusAnim(nAnim);
                }
            }
        }
*/
    }
}

void SetMagicFrame()
{
    nMagicLevel = (1000 - PlayerList[nLocalPlayer].nMagic) / magicperline;

    if (nMagicLevel >= nMeterRange) {
        nMagicLevel = nMeterRange - 1;
    }

    if (nMagicLevel < 0) {
        nMagicLevel = 0;
    }

    SetItemSeq();
}

void SetHealthFrame(short nVal)
{
    nHealthLevel = (800 - PlayerList[nLocalPlayer].nHealth) / healthperline;

    if (nHealthLevel >= nMeterRange ) {
        nHealthLevel = nMeterRange - 1;
    }

    if (nHealthLevel < 0) {
        nHealthLevel = 0;
    }

    if (nVal < 0) {
        BuildStatusAnim(4, 0);
    }
}

void SetAirFrame()
{
    airframe = PlayerList[nLocalPlayer].nAir / airperline;

    if (airframe >= nAirFrames)
    {
        airframe = nAirFrames - 1;
    }
    else if (airframe < 0)
    {
        airframe = 0;
    }
}

void SetCounter(short nVal)
{
    if (nVal <= 999)
    {
        if (nVal < 0) {
            nVal = 0;
        }
    }
    else {
        nVal = 999;
    }

    nCounterDest = nVal;
}

void SetCounterImmediate(short nVal)
{
    SetCounter(nVal);
    nCounter = nCounterDest;

    SetCounterDigits();
}

void SetCounterDigits()
{
    nDigit[2] = 3 * (nCounter / 100 % 10);
    nDigit[1] = 3 * (nCounter / 10 % 10);
    nDigit[0] = 3 * (nCounter % 10);
}

void SetItemSeq()
{
    short nItem = nPlayerItem[nLocalPlayer];
    if (nItem < 0)
    {
        nItemSeq = -1;
        return;
    }

    short nOffset = nItemSeqOffset[nItem];

    SetItemSeq2(nOffset);
}

void SetItemSeq2(int nSeqOffset)
{
    short nItem = nPlayerItem[nLocalPlayer];

    if (nItemMagic[nItem] <= PlayerList[nLocalPlayer].nMagic) {
        nItemAltSeq = 0;
    }
    else {
        nItemAltSeq = 2;
    }

    nItemFrame = 0;
    nItemSeq = nSeqOffset + nItemAltSeq;
    nItemFrames = SeqSize[nItemSeq + nStatusSeqOffset];
}

void SetPlayerItem(short nPlayer, short nItem)
{
    nPlayerItem[nPlayer] = nItem;

    if (nPlayer == nLocalPlayer)
    {
        SetItemSeq();
        if (nItem >= 0) {
            BuildStatusAnim(156 + (2 * PlayerList[nLocalPlayer].items[nItem]), 0);
        }
    }
}

void SetNextItem(int nPlayer)
{
    short nItem = nPlayerItem[nPlayer];

    int i;

    for (i = 6; i > 0; i--)
    {
        nItem++;
        if (nItem == 6)
            nItem = 0;

        if (PlayerList[nPlayer].items[nItem] != 0)
            break;
    }

    if (i > 0) {
        SetPlayerItem(nPlayer, nItem);
    }
}

void SetPrevItem(int nPlayer)
{
    if (nPlayerItem[nPlayer] == -1)
        return;

    int nItem = nPlayerItem[nPlayer];

    int i;

    for (i = 6; i > 0; i--)
    {
        nItem--;
        if (nItem < 0)
            nItem = 5;

        if (PlayerList[nPlayer].items[nItem] != 0)
            break;
    }

    if (i > 0) {
        SetPlayerItem(nPlayer, nItem);
    }
}

void MoveStatus()
{
    if (nItemSeq >= 0)
    {
        nItemFrame++;

        if (nItemFrame >= nItemFrames)
        {
            if (nItemSeq == 67) {
                SetItemSeq();
            }
            else
            {
                nItemSeq -= nItemAltSeq;

                if (nItemAltSeq || totalmoves & 0x1F)
                {
                    if (nItemSeq < 2) {
                        nItemAltSeq = 0;
                    }
                }
                else
                {
                    nItemAltSeq = 1;
                }

                nItemFrame = 0;
                nItemSeq += nItemAltSeq;
                nItemFrames = SeqSize[nStatusSeqOffset + nItemSeq];
            }
        }
    }

    if (message_timer)
    {
        message_timer -= 4;
        if (message_timer <= 0)
        {
            if (screensize > 0) {
                textpages = numpages;
            }

            message_timer = 0;
        }
    }

    nHealthFrame++;
    if (nHealthFrame >= nHealthFrames) {
        nHealthFrame = 0;
    }

    nMagicFrame++;
    if (nMagicFrame >= nMagicFrames) {
        nMagicFrame = 0;
    }

    MoveStatusAnims();

    if (nCounter == nCounterDest)
    {
        nCounter = nCounterDest;
        ammodelay = 3;
        return;
    }
    else
    {
        ammodelay--;
        if (ammodelay > 0) {
            return;
        }
    }

    int eax = nCounterDest - nCounter;

    if (eax <= 0)
    {
        if (eax >= -30)
        {
            for (int i = 0; i < 3; i++)
            {
                nDigit[i]--;

                if (nDigit[i] < 0)
                {
                    nDigit[i] += 30;
                }

                if (nDigit[i] < 27) {
                    break;
                }
            }
        }
        else
        {
            nCounter += (nCounterDest - nCounter) >> 1;
            SetCounterDigits();
            return;
        }
    }
    else
    {
        if (eax <= 30)
        {
            for (int i = 0; i < 3; i++)
            {
                nDigit[i]++;

                if (nDigit[i] <= 27) {
                    break;
                }

                if (nDigit[i] >= 30) {
                    nDigit[i] -= 30;
                }
            }
        }
        else
        {
            nCounter += (nCounterDest - nCounter) >> 1;
            SetCounterDigits();
            return;
        }
    }

    if (!(nDigit[0] % 3)) {
        nCounter = nDigit[0] / 3 + 100 * (nDigit[2] / 3) + 10 * (nDigit[1] / 3);
    }

    eax = nCounterDest - nCounter;
    if (eax < 0) {
        eax = -eax;
    }

    ammodelay = 4 - (eax >> 1);
    if (ammodelay < 1) {
        ammodelay = 1;
    }
}

void UnMaskStatus()
{
#if 0
    for (int i = 0; i < xdim; i++) {
        startdmost[i] = ydim;
    }
#endif
}

void MaskStatus()
{
#if 0
    for (int i = 0; i < xdim; i++)
    {
        short bx = startdmost[i];
        short cx = statusmask[i];

        if (bx > cx) {
            startdmost[i] = cx;
        }
    }
#endif
}

void LoadStatus()
{
#if 0
    int i;
    short nSize;
    short tmp;
    short buffer[1024];
//	memset(buffer, 0, sizeof(buffer)); // bjd - added by me

    for (i = 0; i < xdim; i++) {
        statusmask[i] = ydim;
    }

    nMaskY = ydim;

    int hStatus = kopen4load("status.msk", 1);
    if (!hStatus) {
        return;
    }

    kread(hStatus, &nSize, sizeof(nSize));

    int nCount = nSize >> 1;

    kread(hStatus, &tmp, sizeof(tmp));
    kread(hStatus, buffer, nSize);

    kclose(hStatus);

    short *pStatusMask = statusmask;

    for (i = 0; i < nCount; i++)
    {
        int v8 = ydim - ((ydim * buffer[i]) / 200);
        *pStatusMask++ = ydim - v8;

        if (bHiRes) {
            *pStatusMask++ = ydim - v8;
        }

        if (ydim - v8 < nMaskY) {
            nMaskY = ydim - v8;
        }
    }
#endif
}

void StatusMessage(int messageTime, const char *fmt, ...)
{
    message_timer = messageTime;

    va_list args;
    va_start(args, fmt);

    vsprintf(message_text, fmt, args);

    if (screensize > 0) {
        textpages = numpages;
    }
}

void DrawStatus()
{
    char numberBuf[10] = {0};
    char stringBuf[20] = {0};
    char coordBuf[50] = {0}; // not sure of the size for this?

    if (!bFullScreen && nNetTime)
    {
        // bjd - commenting out this check seems to fix the black status bar at 320x200 resolution
//		if (bHiRes) {
            NoClip();
//		}

        // draw the main bar itself
        seq_DrawStatusSequence(nStatusSeqOffset, 0, 0);

        seq_DrawStatusSequence(nStatusSeqOffset + 128, 0, 0);
        seq_DrawStatusSequence(nStatusSeqOffset + 127, 0, 0);
        seq_DrawStatusSequence(nStatusSeqOffset + 1, nHealthFrame, nHealthLevel);
        seq_DrawStatusSequence(nStatusSeqOffset + 129, nMagicFrame, nMagicLevel);
        seq_DrawStatusSequence(nStatusSeqOffset + 125, 0, 0); // draw ankh on health pool
        seq_DrawStatusSequence(nStatusSeqOffset + 130, 0, 0); // draw health pool frame (top)
        seq_DrawStatusSequence(nStatusSeqOffset + 131, 0, 0); // magic pool frame (bottom)

        if (nItemSeq >= 0) {
            seq_DrawStatusSequence(nItemSeq + nStatusSeqOffset, nItemFrame, 0);
        }

        // draws health level dots, animates breathing lungs and other things
        DrawStatusAnims();

        // draw the blue air level meter when underwater (but not responsible for animating the breathing lungs otherwise)
        if (airpages)
        {
            seq_DrawStatusSequence(nStatusSeqOffset + 133, airframe, 0);
            // airpages--;
        }

        // draw compass
        seq_DrawStatusSequence(nStatusSeqOffset + 35, ((inita + 128) & kAngleMask) >> 8, 0);

        /*
        if (bCoordinates)
        {
            sprintf(numberBuf, "%i", lastfps);
            // char *cFPS = itoa(lastfps, numberBuf, 10);
            printext(xdim - 20, nViewTop, numberBuf, kTile159, -1);
        }
        */

        // draw ammo count
        seq_DrawStatusSequence(nStatusSeqOffset + 44, nDigit[2], 0);
        seq_DrawStatusSequence(nStatusSeqOffset + 45, nDigit[1], 0);
        seq_DrawStatusSequence(nStatusSeqOffset + 46, nDigit[0], 0);

        // bjd - commenting out this check seems to fix the black status bar at 320x200 resolution
//		if (bHiRes) {
            Clip();
//		}
    }

    if (nNetPlayerCount)
    {
        NoClip();

        int shade;

        if ((int)totalclock / kTimerTicks & 1) {
            shade = -100;
        }
        else {
            shade = 127;
        }

        int nTile = kTile3593;

        int x = 320 / (nTotalPlayers + 1);

        for (int i = 0; i < nTotalPlayers; i++)
        {
            int nScore = nPlayerScore[i];
            if (word_9AD54[i] == nScore)
            {
                int v9 = dword_9AD64[i];
                if (v9 && v9 <= (int)totalclock) {
                    dword_9AD64[i] = 0;
                }
            }
            else
            {
                word_9AD54[i] = nScore;
                dword_9AD64[i] = (int)totalclock + 30;
            }

            overwritesprite(x, 7, nTile, 0, 3, kPalNormal);

            if (i != nLocalPlayer) {
                shade = -100;
            }

            sprintf(stringBuf, "%d", nPlayerScore[i]);
            int nStringLen = MyGetStringWidth(stringBuf);

            myprintext(x - (nStringLen / 2), 4, stringBuf, shade);

            x *= 2;
            nTile++;
        }

        if (nNetTime >= 0)
        {
            int y = nViewTop;

            if (nNetTime)
            {
                int v12 = (nNetTime + 29) / 30 % 60;
                int v13 = (nNetTime + 29) / 1800;
                nNetTime += 29;

                sprintf(stringBuf, "%d.%02d", v13, v12);

                if (bHiRes) {
                    y = nViewTop / 2;
                }

                if (nViewTop <= 0) {
                    y += 20;
                }
                else {
                    y += 15;
                }

                nNetTime -= 29;
            }
            else
            {
                y = 100;
                strcpy(stringBuf, "GAME OVER");
            }

            int nLenString = MyGetStringWidth(stringBuf);
            myprintext((320 - nLenString) / 2, y, stringBuf, 0);
        }

        Clip();
    }

    if (bCoordinates)
    {
        int nSprite = PlayerList[nLocalPlayer].nSprite;

        int x = (nViewLeft + nViewRight) / 2;

        snprintf(coordBuf, 50, "X %d", (int)sprite[nSprite].x.cast());
        printext(x, nViewTop + 1, coordBuf, kTile159, 255);

        snprintf(coordBuf, 50, "Y %d", sprite[nSprite].y.cast());
        printext(x, nViewTop + 10, coordBuf, kTile159, 255);
    }

    if (bHolly)
    {
        snprintf(message_text, 80, "HOLLY: %s", sHollyStr);
        printext(0, 0, message_text, kTile159, 255);
    }
    else if (nSnakeCam < 0)
    {
        if (message_timer) {
            printext(0, 0, message_text, kTile159, 255);
        }
    }
    else
    {
        printext(0, 0, "S E R P E N T   C A M", kTile159, 255);
    }
}

// I'm not sure this really needs to be saved.
static SavegameHelper sgh("status",
    SV(nMaskY),
    SV(nAnimsFree),
    SV(message_timer),
    SV(magicperline),
    SV(airperline),
    SV(healthperline),
    SV(nAirFrames),
    SV(nCounter),
    SV(nCounterDest),
    SV(nStatusSeqOffset),
    SV(nItemFrames),
    SV(laststatusx),
    SV(laststatusy),
    SV(nItemSeq),
    SV(nMagicFrames),
    SV(nHealthLevel),
    SV(nItemFrame),
    SV(nMeterRange),
    SV(nMagicLevel),
    SV(nHealthFrame),
    SV(nMagicFrame),
    SV(statusx),
    SV(statusy),
    SV(nHealthFrames),
    SV(airframe),
    SV(nFirstAnim),
    SV(nLastAnim),
    SV(nItemAltSeq),
    SV(airpages),
    SV(ammodelay),
    SV(nCounterBullet),
    SA(statusmask),
    SA(message_text),
    SA(nDigit),
    SA(StatusAnim),
    SA(StatusAnimsFree),
    SA(StatusAnimFlags),
    SA(nItemSeqOffset),
    SA(word_9AD54),
    SA(dword_9AD64),
    nullptr);


END_PS_NS
