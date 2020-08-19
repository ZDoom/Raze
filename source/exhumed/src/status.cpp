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
#include "aistuff.h"
#include "status.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "view.h"
#include "v_2ddrawer.h"
#include "multipatchtexture.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "v_draw.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_PS_NS


void InitFonts()
{
    GlyphSet fontdata;
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.

    auto tile = tileGetTexture(159);
    auto pixels = tile->GetTexture()->Get8BitPixels(false);
    // Convert the sheet font to a proportional font by checking the actual character sizes.
    for (int i = 1; i < 128; i++)
    {
        int xpos = (i % 16) * 8;
        int ypos = (i / 16) * 8;
        bool rowset[8]{};
        for (int x = 0; x < 8; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                int pixel = pixels[xpos + x + 128 * (ypos + y)];
                if (pixel)
                {
                    rowset[x] = true;
                    break;
                }
            }
        }
        int left = 0;
        int right = 7;
        while (left <= right)
        {
            bool didit = false;
            if (!rowset[left]) left++, didit = true;
            if (!rowset[right]) right--, didit = true;
            if (!didit) break;
        }
        if (left < right)
        {
            xpos += left;
            int width = right - left + 1;
            int height = 8;

            TArray<TexPartBuild> part(1, true);
            part[0].OriginX = -xpos;
            part[0].OriginY = -ypos;
            part[0].TexImage = static_cast<FImageTexture*>(tile->GetTexture());
            FMultiPatchTexture* image = new FMultiPatchTexture(width, height, part, false, false);
            FImageTexture* tex = new FImageTexture(image);
            auto gtex = MakeGameTexture(tex, nullptr, ETextureType::FontChar);
            gtex->SetWorldPanning(true);
            gtex->SetOffsets(0, 0, 0);
            gtex->SetOffsets(1, 0, 0);
            TexMan.AddGameTexture(gtex);
            fontdata.Insert(i, gtex);
        }
    }
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    SmallFont2->SetKerning(1);
    fontdata.Clear();

    for (int i = 0; i < 26; i++)
    {
        fontdata.Insert('A' + i, tileGetTexture(3522 + i));
    }
    for (int i = 0; i < 10; i++)
    {
        fontdata.Insert('0' + i, tileGetTexture(3555 + i));
    }
    fontdata.Insert('.', tileGetTexture(3548));
    fontdata.Insert('!', tileGetTexture(3549));
    fontdata.Insert('?', tileGetTexture(3550));
    fontdata.Insert(',', tileGetTexture(3551));
    fontdata.Insert('`', tileGetTexture(3552));
    fontdata.Insert('"', tileGetTexture(3553));
    fontdata.Insert('-', tileGetTexture(3554));
    fontdata.Insert('_', tileGetTexture(3554));
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    GlyphSet::Iterator it(fontdata);
    GlyphSet::Pair* pair;
    while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    SmallFont->SetKerning(1);
    fontdata.Clear();

}


// All this must be moved into the status bar once it is made persistent!
const int kMaxStatusAnims = 50;

short word_9AD54[kMaxPlayers] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int dword_9AD64[kMaxPlayers] = { 0, 0, 0, 0, 0, 0, 0, 0 };

short nStatusSeqOffset;
short nHealthFrames;
short nMagicFrames;

short nHealthLevel;
short nMagicLevel;
short nHealthFrame;
short nMagicFrame;

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

short nItemFrames;

int16_t nItemSeq;
short nDigit[3];

short nItemFrame;
short nMeterRange;

short statusx;
short statusy;

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


statusAnim StatusAnim[kMaxStatusAnims];
uint8_t StatusAnimsFree[kMaxStatusAnims];
uint8_t StatusAnimFlags[kMaxStatusAnims];

short nItemSeqOffset[] = {91, 72, 76, 79, 68, 87, 83};

void SetCounterDigits();
void SetItemSeq();
void SetItemSeq2(int nSeqOffset);
void DestroyStatusAnim(short nAnim);


void InitStatus()
{
    nStatusSeqOffset = SeqOffsets[kSeqStatus];
    nHealthFrames = SeqSize[nStatusSeqOffset + 1];
    int nPicNum = seq_GetSeqPicnum(kSeqStatus, 1, 0);
    nMagicFrames = SeqSize[nStatusSeqOffset + 129];
    nHealthFrame = 0;
    nMagicFrame = 0;
    nHealthLevel = 0;
    nMagicLevel = 0;
    nMeterRange = tilesiz[nPicNum].y;
    magicperline = 1000 / nMeterRange;
    healthperline = 800 / nMeterRange;
    nAirFrames = SeqSize[nStatusSeqOffset + 133];
    airperline = 100 / nAirFrames;
    nCounter = 0;
    nCounterDest = 0;

    memset(nDigit, 0, sizeof(nDigit));

    SetCounter(0);
    SetHealthFrame(0);
    SetMagicFrame();

    for (int i = 0; i < kMaxStatusAnims; i++) {
        StatusAnimsFree[i] = i;
    }

    nLastAnim = -1;
    nFirstAnim = -1;
    nItemSeq = -1;
    nAnimsFree = kMaxStatusAnims;
    statusx = xdim - 320;
    message_timer = 0;
    statusy = ydim - 200;
}


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
        //Error("illegal value for nPlayerLives #%d\n", nLocalPlayer);
        nLives = 0;
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


void DrawSnakeCamStatus()
{
    printext(0, 0, "S E R P E N T   C A M", kTile159, 255);
}

class DExhumedStatusBar : public DBaseStatusBar
{
    DHUDFont textfont;

public:
    DExhumedStatusBar()
    {
        textfont = { SmallFont2, 1, Off, 1, 1 };
    }

private:

    //---------------------------------------------------------------------------
    //
    // draws a sequence animation to the status bar
    //
    //---------------------------------------------------------------------------

    void DrawStatusSequence(short nSequence, uint16_t edx, short ebx, int xoffset = 0)
    {
        edx += SeqBase[nSequence];

        short nFrameBase = FrameBase[edx];
        int16_t nFrameSize = FrameSize[edx];

        while (1)
        {
            nFrameSize--;
            if (nFrameSize < 0)
                break;

            int flags = DI_ITEM_RELCENTER;

            int x = ChunkXpos[nFrameBase];
            int y = ChunkYpos[nFrameBase] + ebx;

            if (hud_size <= Hud_StbarOverlay)
            {
                x += 160;
                y += 100;
            }
            else
            {
                if (x < 0)
                {
                    x += 160;
                    flags |= DI_SCREEN_LEFT_BOTTOM;
                }
                else if (x > 0)
                {
                    x -= 159; // graphics do not match up precisely.
                    flags |= DI_SCREEN_RIGHT_BOTTOM;
                }
                y -= 100;
                if (hud_size == Hud_full)
                {
                    x += xoffset;
                }
            }

            int tile = ChunkPict[nFrameBase];

            short chunkFlag = ChunkFlag[nFrameBase];

            if (chunkFlag & 1) flags |= DI_MIRROR;
            if (chunkFlag & 2) flags |= DI_MIRRORY;

            DrawGraphic(tileGetTexture(tile), x, y, flags, 1, -1, -1, 1, 1);
            //overwritesprite(x, y, tile, 0, nStat, 0);
            nFrameBase++;
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatusAnims()
    {
        for (int i = nFirstAnim; i >= 0; i = StatusAnim[i].nPrevAnim)
        {
            int nSequence = nStatusSeqOffset + StatusAnim[i].s1;

            int xoffs = 0;
            if (StatusAnim[i].s1 == 132) xoffs = -32;

            DrawStatusSequence(nSequence, StatusAnim[i].s2, 0, xoffs);

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


    //---------------------------------------------------------------------------
    //
    // Frag display - very ugly and may have to be redone if multiplayer suppoer gets added.
    //
    //---------------------------------------------------------------------------

    void DrawMulti()
    {
        char stringBuf[30];
        if (nNetPlayerCount)
        {
            BeginHUD(320, 200, 1);

            int shade;

            if ((int)totalclock / kTimerTicks & 1) {
                shade = -100;
            }
            else {
                shade = 127;
            }

            int nTile = kTile3593;

            int xx = 320 / (nTotalPlayers + 1);
            int x = xx - 160;

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

                DrawGraphic(tileGetTexture(nTile), x, 7, DI_ITEM_TOP|DI_MIRROR|DI_MIRRORY, 1, -1, -1, 1, 1);

                if (i != nLocalPlayer) {
                    shade = -100;
                }

                sprintf(stringBuf, "%d", nPlayerScore[i]);
                SBar_DrawString(this, &textfont, stringBuf, x, 0, DI_ITEM_TOP|DI_TEXT_ALIGN_CENTER, i != nLocalPlayer ? CR_UNTRANSLATED : CR_GOLD, 1, -1, 0, 1, 1);
                x += xx;
                nTile++;
            }

            if (nNetTime >= 0)
            {
                int y = 0;

                if (nNetTime)
                {
                    int v12 = (nNetTime + 29) / 30 % 60;
                    int v13 = (nNetTime + 29) / 1800;
                    nNetTime += 29;

                    sprintf(stringBuf, "%d.%02d", v13, v12);

                    y += 20;
                    nNetTime -= 29;
                    SBar_DrawString(this, &textfont, stringBuf, 0, 10, DI_ITEM_TOP | DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, -1, 0, 1, 1);
                }
            }

        }
    }

    //---------------------------------------------------------------------------
    //
    // draw the full status bar
    //
    //---------------------------------------------------------------------------

    void DrawStatus()
    {
        BeginStatusBar(320, 200, 40);
        char numberBuf[10] = { 0 };
        char stringBuf[20] = { 0 };
        char coordBuf[50] = { 0 }; // not sure of the size for this?

        if (hud_size <= Hud_StbarOverlay)
        {
            // draw the main bar itself
            BeginStatusBar(320, 200, 40);
            DrawStatusSequence(nStatusSeqOffset, 0, 0);
        }
        else if (hud_size == Hud_Mini)
        {
            auto lh = TexMan.GetGameTextureByName("hud_l");
            auto rh = TexMan.GetGameTextureByName("hud_r");
            BeginHUD(320, 200, 1);
            if (lh) DrawGraphic(lh, 0, 0, DI_ITEM_LEFT_BOTTOM | DI_SCREEN_LEFT_BOTTOM, 1, -1, -1, 1, 1);
            if (rh) DrawGraphic(rh, 0, 0, DI_ITEM_RIGHT_BOTTOM | DI_SCREEN_RIGHT_BOTTOM, 1, -1, -1, 1, 1);
        }
        else if (hud_size == Hud_full)
        {
            BeginHUD(320, 200, 1);
        }

        if (/*!bFullScreen &&*/ nNetTime)
        {
            DrawStatusSequence(nStatusSeqOffset + 127, 0, 0, -4);
            DrawStatusSequence(nStatusSeqOffset + 129, nMagicFrame, nMagicLevel, -4);
            DrawStatusSequence(nStatusSeqOffset + 131, 0, 0, -4); // magic pool frame (bottom)

            DrawStatusSequence(nStatusSeqOffset + 128, 0, 0, 4);
            DrawStatusSequence(nStatusSeqOffset + 1, nHealthFrame, nHealthLevel, 4);
            DrawStatusSequence(nStatusSeqOffset + 125, 0, 0, 4); // draw ankh on health pool
            DrawStatusSequence(nStatusSeqOffset + 130, 0, 0, 4); // draw health pool frame (top)

            if (nItemSeq >= 0) {
                DrawStatusSequence(nItemSeq + nStatusSeqOffset, nItemFrame, 0);
            }
            // draw the blue air level meter when underwater (but not responsible for animating the breathing lungs otherwise)
            DrawStatusSequence(nStatusSeqOffset + 133, airframe, 0, -32);

            // draws health level dots, animates breathing lungs and other things
            DrawStatusAnims();


            // draw compass
            if (hud_size <= Hud_StbarOverlay) DrawStatusSequence(nStatusSeqOffset + 35, ((inita + 128) & kAngleMask) >> 8, 0);

            //if (hud_size < Hud_full)
            {
                // draw ammo count
                DrawStatusSequence(nStatusSeqOffset + 44, nDigit[2], 0, -35);
                DrawStatusSequence(nStatusSeqOffset + 45, nDigit[1], 0, -35);
                DrawStatusSequence(nStatusSeqOffset + 46, nDigit[0], 0, -35);
            }
        }

        DrawMulti();

#if 0
        if (bCoordinates)
        {
            int nSprite = PlayerList[nLocalPlayer].nSprite;

            int x = 160;

            snprintf(coordBuf, 50, "X %d", (int)sprite[nSprite].x);
            printext(x, 1, coordBuf, kTile159, 255);

            snprintf(coordBuf, 50, "Y %d", sprite[nSprite].y);
            printext(x, 10, coordBuf, kTile159, 255);
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
#endif
    }

public:
    void Draw()
    {
        if (hud_size <= Hud_full)
        {
            DrawStatus();
        }
    }
};

void UpdateFrame()
{
    auto tex = tileGetTexture(nBackgroundPic);

    twod->AddFlatFill(0, 0, xdim, windowxy1.y - 3, tex);
    twod->AddFlatFill(0, windowxy2.y + 4, xdim, ydim, tex);
    twod->AddFlatFill(0, windowxy1.y - 3, windowxy1.x - 3, windowxy2.y + 4, tex);
    twod->AddFlatFill(windowxy2.x + 4, windowxy1.y - 3, xdim, windowxy2.y + 4, tex);

    twod->AddFlatFill(windowxy1.x - 3, windowxy1.y - 3, windowxy1.x, windowxy2.y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.x, windowxy1.y - 3, windowxy2.x + 4, windowxy1.y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, windowxy2.x + 4, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.x - 3, windowxy2.y + 1, windowxy2.x + 1, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
}

void StatusMessage(int messageTime, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VPrintf(PRINT_NOTIFY, fmt, ap);
    Printf(PRINT_NOTIFY, "\n");
    va_end(ap);
}


void DrawStatusBar()
{
    if (hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }
    DExhumedStatusBar sbar;
    sbar.Draw();
}


#if 0
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
    nullptr);
#endif

END_PS_NS
