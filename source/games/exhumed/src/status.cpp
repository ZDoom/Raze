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
#include "automap.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_PS_NS



// All this must be moved into the status bar once it is made persistent!
const int kMaxStatusAnims = 50;

short nStatusSeqOffset;
short nHealthFrames;
short nMagicFrames;

short nHealthLevel;
short nMagicLevel;
short nHealthFrame;
short nMagicFrame;

short nMaskY;

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
    uint8_t StatusAnimFlags;
};

FreeListArray<statusAnim, kMaxStatusAnims> StatusAnim;

short nItemSeqOffset[] = {91, 72, 76, 79, 68, 87, 83};

void SetCounterDigits();
void SetItemSeq();
void SetItemSeq2(int nSeqOffset);
void DestroyStatusAnim(short nAnim);

FSerializer& Serialize(FSerializer& arc, const char* keyname, statusAnim& w, statusAnim* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("s1", w.s1)
            ("s2", w.s2)
            ("prev", w.nPrevAnim)
            ("next", w.nNextAnim)
            ("flags", w.StatusAnimFlags)
            .EndObject();
    }
    return arc;
}


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
    nMeterRange = tileHeight(nPicNum);
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

    StatusAnim.Clear();

    nLastAnim = -1;
    nFirstAnim = -1;
    nItemSeq = -1;
}

int ItemTimer(int num, int plr) 
{
    switch (num) {
    case 1: //Scarab item
        return (PlayerList[plr].invincibility * 100) / 900;
    case 3: //Hand item
        return (nPlayerDouble[plr] * 100) / 1350;
    case 5: //Mask
        return (PlayerList[plr].nMaskAmount * 100) / 1350;
    case 4: //Invisible
        return (nPlayerInvisible[plr] * 100) / 900;
    case 2: //Torch
        return (nPlayerTorch[plr] * 100) / 900;
    }

    return -1;
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

    int nStatusAnim = StatusAnim.Get();

    StatusAnim[nStatusAnim].nPrevAnim = -1;
    StatusAnim[nStatusAnim].nNextAnim = (int8_t)nLastAnim;

    if (nLastAnim < 0) {
        nFirstAnim = nStatusAnim;
    }
    else {
        StatusAnim[nLastAnim].nPrevAnim = (int8_t)nStatusAnim;
    }

    nLastAnim = nStatusAnim;

    StatusAnim[nStatusAnim].s1 = val;
    StatusAnim[nStatusAnim].s2 = 0;
    StatusAnim[nStatusAnim].StatusAnimFlags = nFlags;
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
            if (StatusAnim[i].StatusAnimFlags & 0x10) {
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

    StatusAnim.Release(nAnim);
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


class DExhumedStatusBar : public DBaseStatusBar
{
    DECLARE_CLASS(DExhumedStatusBar, DBaseStatusBar)
    HAS_OBJECT_POINTERS

    TObjPtr<DHUDFont*> textfont, numberFont;
    int keyanims[4];

    enum EConst
    {
        KeySeq = 36,
    };

public:
    DExhumedStatusBar()
    {
        textfont = Create<DHUDFont>(SmallFont, 1, Off, 1, 1 );
        numberFont = Create<DHUDFont>(BigFont, 0, Off, 1, 1 );
    }

private:

    //---------------------------------------------------------------------------
    //
    // draws a sequence animation to the status bar
    //
    //---------------------------------------------------------------------------

    void DrawStatusSequence(short nSequence, uint16_t edx, double ebx, double xoffset = 0)
    {
        edx += SeqBase[nSequence];

        short nFrameBase = FrameBase[edx];
        int16_t nFrameSize = FrameSize[edx];

        while (1)
        {
            nFrameSize--;
            if (nFrameSize < 0)
                break;

            int flags = 0;

            double x = ChunkXpos[nFrameBase] + xoffset;
            double y = ChunkYpos[nFrameBase] + ebx;

            if (hud_size <= Hud_StbarOverlay)
            {
                x += 161;
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
            }

            int tile = ChunkPict[nFrameBase];

            short chunkFlag = ChunkFlag[nFrameBase];

            if (chunkFlag & 3)
            {
                // This is hard to align with bad offsets, so skip that treatment for mirrored elements.
                flags |= DI_ITEM_RELCENTER;
            }
            else
            {
                x -= tileWidth(tile) * .5;
                y -= tileHeight(tile) * .5;
                flags |= DI_ITEM_OFFSETS;
            }

            if (chunkFlag & 1)
                flags |= DI_MIRROR;
            if (chunkFlag & 2)
                flags |= DI_MIRRORY;

            DrawGraphic(tileGetTexture(tile), x, y, flags, 1, -1, -1, 1, 1);
            nFrameBase++;
        }
    }

    //---------------------------------------------------------------------------
    //
    // draws a sequence animation to the status bar
    //
    //---------------------------------------------------------------------------

    FGameTexture * GetStatusSequencePic(short nSequence, uint16_t edx)
    {
        edx += SeqBase[nSequence];
        int nFrameBase = FrameBase[edx];
        return tileGetTexture(ChunkPict[nFrameBase]);
    }

    int GetStatusSequenceTile(short nSequence, uint16_t edx)
    {
        edx += SeqBase[nSequence];
        int nFrameBase = FrameBase[edx];
        return ChunkPict[nFrameBase];
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
            DrawStatusSequence(nSequence, StatusAnim[i].s2, 0, 0);
        }
    }


    //---------------------------------------------------------------------------
    //
    // Frag display - very ugly and may have to be redone if multiplayer support gets added.
    //
    //---------------------------------------------------------------------------

    void DrawMulti()
    {
        char stringBuf[30];
        if (nNetPlayerCount)
        {
            BeginHUD(320, 200, 1);

            int shade;

            if ((PlayClock / 120) & 1) {
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
                DrawGraphic(tileGetTexture(nTile), x, 7, DI_ITEM_CENTER, 1, -1, -1, 1, 1);

                if (i != nLocalPlayer) {
                    shade = -100;
                }

                sprintf(stringBuf, "%d", nPlayerScore[i]);
                SBar_DrawString(this, textfont, stringBuf, x, 0, DI_ITEM_TOP|DI_TEXT_ALIGN_CENTER, i != nLocalPlayer ? CR_UNTRANSLATED : CR_GOLD, 1, -1, 0, 1, 1);
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
                    SBar_DrawString(this, textfont, stringBuf, 0, 10, DI_ITEM_TOP | DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, -1, 0, 1, 1);
                }
            }

        }
    }

    //==========================================================================
    //
    // Fullscreen HUD variant #1
    //
    //==========================================================================

    void DrawHUD2()
    {
        BeginHUD(320, 200, 1);

        auto pp = &PlayerList[nLocalPlayer];

        FString format;
        FGameTexture* img;
        double imgScale;
        double baseScale = numberFont->mFont->GetHeight() * 0.75;

        
        //
        // Health
        //
        img = GetStatusSequencePic(nStatusSeqOffset + 125, 0);
        imgScale = baseScale / img->GetDisplayHeight();
        DrawGraphic(img, 1.5, -1, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, imgScale, imgScale);

        if (!althud_flashing || pp->nHealth > 150 || (PlayClock & 32))
        {
            int s = -8;
            if (althud_flashing && pp->nHealth > 800)
                s += bsin(PlayClock << 5, -10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", pp->nHealth >> 3);
            SBar_DrawString(this, numberFont, format, 13, -numberFont->mFont->GetHeight()+3, DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }
		
		//
		// Air
		//
		if (SectFlag[nPlayerViewSect[nLocalPlayer]] & kSectUnderwater)
		{
			img = GetStatusSequencePic(nStatusSeqOffset + 133, airframe);
            imgScale = baseScale / img->GetDisplayHeight();
            DrawGraphic(img, -4, -22, DI_ITEM_RIGHT_BOTTOM, 1., -1, -1, imgScale, imgScale);
		}

        //
        // Magic
        //
        if (nItemSeq >= 0)
        {
            int tile = GetStatusSequenceTile(nItemSeq + nStatusSeqOffset, nItemFrame);
            int tile2 = tile;
            if (tile2 > 744 && tile2 < 751) tile2 = 744;

            imgScale = baseScale / tileHeight(tile2);
            DrawGraphic(tileGetTexture(tile), 70, -1, DI_ITEM_CENTER_BOTTOM, 1., -1, -1, imgScale, imgScale);



            format.Format("%d", pp->nMagic / 10);

            short nItem = nPlayerItem[nLocalPlayer];
            int timer = ItemTimer(nItem, nLocalPlayer);
            if (timer > 0)
            {
                format.AppendFormat("/%d", timer);
            }
            SBar_DrawString(this, numberFont, format, 79.5, -numberFont->mFont->GetHeight() + 3, DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }
        //
        // Weapon
        //
        const short ammo_sprites[] = { -1, -1 };

        int weapon = pp->nCurrentWeapon;
        int wicon = 0;// ammo_sprites[weapon];
        int ammo = nCounterDest;// pp->WpnAmmo[weapon];
        if (ammo > 0) // wicon > 0
        {
            format.Format("%d", ammo);
            img = tileGetTexture(wicon);
            imgScale = baseScale / img->GetDisplayHeight();
            auto imgX = 21.125;
            auto strlen = format.Len();

            if (strlen > 1)
            {
                imgX += (imgX * 0.855) * (strlen - 1);
            }

            if ((!althud_flashing || PlayClock & 32 || ammo > 10))// (DamageData[weapon].max_ammo / 10)))
            {
                SBar_DrawString(this, numberFont, format, -3, -numberFont->mFont->GetHeight()+3, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
            }

            //DrawGraphic(img, -imgX, -1, DI_ITEM_RIGHT_BOTTOM, 1, -1, -1, imgScale, imgScale);
        }

#if 0
        //
        // Selected inventory item
        //
        img = tileGetTexture(icons[pp->InventoryNum]);
        imgScale = baseScale / img->GetDisplayHeight();
        int x = 165;
        DrawGraphic(img, x, -1, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, imgScale, imgScale);

        PlayerUpdateInventoryState(pp, x + 3.0, -18.0, 1, 1);
        PlayerUpdateInventoryPercent(pp, x + 3.5, -20.5, 1, 1);
#endif


        //
        // keys
        //

        uint16_t nKeys = PlayerList[nLocalPlayer].keys;

        int val = 675;
        int x = -134;

        for (int i = 0; i < 4; i++)
        {
            if (nKeys & 0x1000) 
            {
                auto tex = tileGetTexture(val);
                if (tex && tex->isValid())
                {
                    DrawGraphic(tex, x, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);
                }
            }

            nKeys >>= 1;
            val += 4;
            x += 20;
        }
    }



    //---------------------------------------------------------------------------
    //
    // draw the full status bar
    //
    //---------------------------------------------------------------------------

    void DrawStatus()
    {
        if (hud_size <= Hud_StbarOverlay)
        {
            // draw the main bar itself
            BeginStatusBar(320, 200, 40);
            if (hud_size == Hud_StbarOverlay) Set43ClipRect();
            DrawGraphic(tileGetTexture(kTileStatusBar), 160, 200, DI_ITEM_CENTER_BOTTOM, 1, -1, -1, 1, 1);
            twod->ClearClipRect();
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
            DrawHUD2();
            return;
        }

        for (int i = 0; i < 4; i++)
        {
            if (PlayerList[nLocalPlayer].keys & (4096 << i))
            {
                DrawStatusSequence(nStatusSeqOffset + KeySeq + 2 * i, keyanims[i], 0.5, 0.5);
            }
        }

        if (/*!bFullScreen &&*/ nNetTime)
        {
            DrawStatusSequence(nStatusSeqOffset + 127, 0, 0);
            DrawStatusSequence(nStatusSeqOffset + 129, nMagicFrame, nMagicLevel);
            DrawStatusSequence(nStatusSeqOffset + 131, 0, 0); // magic pool frame (bottom)

            DrawStatusSequence(nStatusSeqOffset + 128, 0, 0);
            DrawStatusSequence(nStatusSeqOffset + 1, nHealthFrame, nHealthLevel);
            DrawStatusSequence(nStatusSeqOffset + 125, 0, 0); // draw ankh on health pool
            DrawStatusSequence(nStatusSeqOffset + 130, 0, 0); // draw health pool frame (top)

            if (nItemSeq >= 0) {
                DrawStatusSequence(nItemSeq + nStatusSeqOffset, nItemFrame, 1);
            }

            // draws health level dots, animates breathing lungs and other things
            DrawStatusAnims();

            // draw the blue air level meter when underwater (but not responsible for animating the breathing lungs otherwise)
			if (SectFlag[nPlayerViewSect[nLocalPlayer]] & kSectUnderwater)
			{
				DrawStatusSequence(nStatusSeqOffset + 133, airframe, 0, 0.5);
			}


            // draw compass
            if (hud_size <= Hud_StbarOverlay) DrawStatusSequence(nStatusSeqOffset + 35, ((inita + 128) & kAngleMask) >> 8, 0, 0.5);

            //if (hud_size < Hud_full)
            {
                // draw ammo count
                DrawStatusSequence(nStatusSeqOffset + 44, nDigit[2], 0, 0.5);
                DrawStatusSequence(nStatusSeqOffset + 45, nDigit[1], 0, 0.5);
                DrawStatusSequence(nStatusSeqOffset + 46, nDigit[0], 0, 0.5);
            }
        }

        DrawMulti();

        if (nSnakeCam >= 0)
        {
            BeginHUD(320, 200, 1);
            SBar_DrawString(this, textfont, "S E R P E N T   C A M", 0, 0, DI_TEXT_ALIGN_CENTER | DI_SCREEN_CENTER_TOP, CR_UNTRANSLATED, 1, -1, 0, 1, 1);
        }
    }

    //---------------------------------------------------------------------------
    //
    //
    //
    //---------------------------------------------------------------------------

    void PrintLevelStats(int bottomy)
    {
        FLevelStats stats{};
        stats.fontscale = 1.;
        stats.spacing = SmallFont->GetHeight();
        stats.screenbottomspace = bottomy;
        stats.font = SmallFont;
        stats.letterColor = CR_RED;
        stats.standardColor = CR_UNTRANSLATED;
        stats.time = Scale(PlayClock, 1000, 120);

        if (automapMode == am_full)
        {
            DBaseStatusBar::PrintAutomapInfo(stats, true);
        }
        else if (hud_stats)
        {

            stats.completeColor = CR_DARKGREEN;
            stats.kills = nCreaturesKilled;
            stats.maxkills = nCreaturesTotal;
            stats.frags = -1;
            stats.secrets = 0;
            stats.maxsecrets = 0;

            DBaseStatusBar::PrintLevelStats(stats);
        }
    }


    void Tick() override
    {
        for (int i = 0; i < 4; i++)
        {
            int seq = nStatusSeqOffset + KeySeq + 2 * i;
            if (PlayerList[nLocalPlayer].keys & (4096 << i))
            {
                if (keyanims[i] < SeqSize[seq] - 1)
                {
                    seq_MoveSequence(-1, seq, 0);   // this plays the pickup sound.
                    keyanims[i]++;
                }
            }
            else
            {
                keyanims[i] = 0;
            }
        }
    }


public:
    void UpdateStatusBar()
    {
        Tick(); // temporary.
        if (hud_size <= Hud_full)
        {
            DrawStatus();
        }
        PrintLevelStats(hud_size == Hud_Nothing ? 0 : hud_size == Hud_full? 22 : 40);
    }
};

IMPLEMENT_CLASS(DExhumedStatusBar, false, true)
IMPLEMENT_POINTERS_START(DExhumedStatusBar)
IMPLEMENT_POINTER(textfont)
IMPLEMENT_POINTER(numberFont)
IMPLEMENT_POINTERS_END

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
    if (nFreeze == 2) return; // Hide when Ramses is talking.
    if (hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }
    StatusBar->UpdateStatusBar();
}

END_PS_NS
