//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

This is free software; you can redistribute it and/or
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

struct ChunkFrame // this wraps the internal (mis-)representation of the chunk data.
{
    TextureID tex;
    int x, y;
    int flags;

    native void GetChunkFrame(int nFrameBase);
}

class ExhumedStatusBar : RazeStatusBar
{
	HUDFont textfont, numberFont;

	int keyanims[4];
	int airframe, lungframe;

	int nSelectedItem;
	int nHealthLevel;
	int nMagicLevel;
	int nMeterRange;
	int nHurt;
	int nHealthFrame;
	int nMagicFrame;
	int nItemAltSeq;
	int nItemSeq;
	int nItemFrames;
	int nItemFrame;
	int totalmoves;


	int nCounter;
	int nCounterDest;
	int nDigit[3];
	int ammodelay;
	int nLastWeapon;

	enum EConst
	{
		KeySeq = 36,
	}

	override void Init()
	{
		textfont = HUDFont.Create(SmallFont, 1, Mono_Off, 1, 1);
		numberFont = HUDFont.Create(BigFont, 0, Mono_Off, 1, 1);

		let nPicNum = GetStatusSequencePic(0, 0);
		let siz = TexMan.GetScaledSize(nPicNum);
		nMeterRange = siz.Y;
		Reset();
	}

	override void Reset()
	{
		airframe = lungframe = nHurt = nHealthFrame = nMagicFrame = nItemAltSeq = nItemFrames = nItemFrame = nCounter = 0;

		nDigit[0] = nDigit[1] = nDigit[2] = 0;
		nHealthLevel = -1;
		nMagicLevel = -1;
		nSelectedItem = -1;
		nItemSeq = -1;
		ammodelay = 3;
		nLastWeapon = -1;
		totalmoves = 0;
		nCounterDest = 0;
	}

    //---------------------------------------------------------------------------
    //
    // draws a sequence animation to the status bar
    //
    //---------------------------------------------------------------------------

    void DrawStatusSequence(int nSequence, int frameindex, double yoffset, double xoffset = 0, bool trueadjust = false)
    {
		int nFrameBase, nFrameSize;
		[nFrameBase, nFrameSize] = Exhumed.GetStatusSequence(nSequence, frameindex);

        for(; nFrameSize > 0; nFrameSize--, nFrameBase++)
        {
            int flags = 0;
            ChunkFrame chunk;
            chunk.GetChunkFrame(nFrameBase);

            double x = chunk.x + xoffset;
            double y = chunk.y + yoffset;

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

            if (chunk.flags & 3)
            {
                // This is hard to align with bad offsets, so skip that treatment for mirrored elements.
                flags |= DI_ITEM_RELCENTER;
            }
            else
            {
				let tsiz = TexMan.GetScaledSize(chunk.tex);
				if (trueadjust)
				{
					x -= tsiz.x * 0.5;
					y -= tsiz.y * 0.5;
				}
				else
				{
					x -= int(tsiz.x) / 2;
					y -= int(tsiz.y) / 2;
				}
                flags |= DI_ITEM_OFFSETS;
            }

            if (chunk.flags & 1)
                flags |= DI_MIRROR;
            if (chunk.flags & 2)
                flags |= DI_MIRRORY;

            DrawTexture(chunk.tex, (x, y), flags);
        }
    }

	//---------------------------------------------------------------------------
	//
	// draws a sequence animation to the status bar
	//
	//---------------------------------------------------------------------------

    TextureID GetStatusSequencePic(int nSequence, int frameindex)
    {
		int nFrameBase = Exhumed.GetStatusSequence(nSequence, frameindex);
		ChunkFrame chunk;
		chunk.GetChunkFrame(nFrameBase);
        return chunk.tex;
    }

	//---------------------------------------------------------------------------
	//
	// Frag display - very ugly and may have to be redone if multiplayer support gets added.
	//
	//---------------------------------------------------------------------------

	void DrawMulti()
	{
		/* 
		char stringBuf[30];
		if (netgame)
		{
			BeginHUD(1, false, 320, 200);

			int shade;

			if ((PlayClock / 120) & 1)
				shade = -100;
			else 
				shade = 127;

			int nTile = 3593;

			int xx = 320 / (nTotalPlayers + 1);
			int x = xx - 160;

			for (int i = 0; i < nTotalPlayers; i++)
			{
				DrawImage(String.Format("#%05d", nTile)), (x, 7), DI_ITEM_CENTER);

				if (i != nLocalPlayer) {
					shade = -100;
				}

				sprintf(stringBuf, "%d", nPlayerScore[i]);
				DrawString(this, textfont, stringBuf, x, 0, DI_ITEM_TOP | DI_TEXT_ALIGN_CENTER, i != nLocalPlayer ? CR_UNTRANSLATED : CR_GOLD, 1, -1, 0, 1, 1);
				x += xx;
				nTile++;
			}
		}
		*/
	}

	//==========================================================================
	//
	// Fullscreen HUD variant #1
	//
	//==========================================================================

	int ItemTimer(ExhumedPlayer pPlayer, int num)
	{
		switch (num) {
		case 1: //Scarab item
			return (pPlayer.invincibility * 100) / 900;
		case 3: //Hand item
			return (pPlayer.nDouble * 100) / 1350;
		case 5: //Mask
			return (pPlayer.nMaskAmount * 100) / 1350;
		case 4: //Invisible
			return (pPlayer.nInvisible * 100) / 900;
		case 2: //Torch
			return (pPlayer.nTorch * 100) / 900;
		}

		return -1;
	}

	void DrawHUD2(ExhumedPlayer pp)
	{
		BeginHUD(1, false, 320, 200);

		String format;
		TextureID img;
		double imgScale;
		double baseScale = numberFont.mFont.GetHeight() * 0.75;


		//
		// Health
		//
		img = GetStatusSequencePic(125, 0);
		let size = TexMan.GetScaledSize(img);
		imgScale = baseScale / size.Y;
		DrawTexture(img, (1.5, -1), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		if (!hud_flashing || pp.nHealth > 150 || (PlayClock & 32))
		{
			int s = -8;
			if (hud_flashing && pp.nHealth > 800)
				s += Raze.bsin(PlayClock << 5, -10);
			int intens = clamp(255 - 4 * s, 0, 255);
			format = String.Format("%d", pp.nHealth >> 3);
			DrawString(numberFont, format, (13, -numberFont.mFont.GetHeight() + 3), DI_TEXT_ALIGN_LEFT, Font.CR_UNTRANSLATED, intens / 255.);
		}

		//
		// Air
		//
		if (pp.isUnderwater())
		{
			img = GetStatusSequencePic(133, airframe);
			let size = TexMan.GetScaledSize(img);
			imgScale = baseScale / size.Y;
			DrawTexture(img, (-4, -22), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
		}

		//
		// Magic
		//
		if (nItemSeq >= 0)
		{
			img = GetStatusSequencePic(nItemSeq, nItemFrame);
			//int tile = GetStatusSequenceTile(nItemSeq, nItemFrame);
			//int tile2 = tile;
			//if (tile2 > 744 && tile2 < 751) tile2 = 744;

			let size = TexMan.GetScaledSize(img);
			imgScale = baseScale / size.Y;
			DrawTexture(img, (70, -1), DI_ITEM_CENTER_BOTTOM, scale:(imgScale, imgScale));

			format = String.Format("%d", pp.nMagic / 10);

			int nItem = pp.nItem;
			int timer = ItemTimer(pp, nItem);
			if (timer > 0)
			{
				format.AppendFormat("/%d", timer);
			}
			DrawString(numberFont, format, (79.5, -numberFont.mFont.GetHeight() + 3), DI_TEXT_ALIGN_LEFT);
		}
		//
		// Weapon
		//
		int weapon = pp.nCurrentWeapon;
		int ammo = nCounterDest;
		if (ammo > 0) // wicon > 0
		{
			if (weapon == kWeaponPistol && cl_showmagamt)
			{
				int clip = CalcMagazineAmount(ammo, 6, Exhumed.GetPistolClip() == 0);
				format = String.Format("%d/%d", clip, ammo - clip);
			}
			else if (weapon == kWeaponM60 && cl_showmagamt)
			{
				int clip = CalcMagazineAmount(ammo, 100, Exhumed.GetPlayerClip() == 0);
				format = String.Format("%d/%d", clip, ammo - clip);
			}
			else
			{
				format = String.Format("%d", ammo);
			}
			/* non-implemented weapon icon.
			int wicon = 0;// ammo_sprites[weapon];
			img = tileGetTexture(wicon);
			imgScale = baseScale / img.GetDisplayHeight();
			let imgX = 21.125;
			let strlen = format.Len();

			if (strlen > 1)
			{
				imgX += (imgX * 0.855) * (strlen - 1);
			}
			*/

			if ((!hud_flashing || PlayClock & 32 || ammo > 10))// (DamageData[weapon].max_ammo / 10)))
			{
				DrawString(numberFont, format, (-3, -numberFont.mFont.GetHeight() + 3), DI_TEXT_ALIGN_RIGHT);
			}

			//DrawGraphic(img, (-imgX, -1), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
		}

		//
		// keys
		//

		int nKeys = pp.keys;
		int x = -164;

		for (int i = 0; i < 4; i++)
		{
			if (nKeys & (0x1000 << i))
			{
				DrawImage(String.Format("KeyIcon%d", i+1), (x, -2), DI_ITEM_LEFT_BOTTOM);
			}
			x += 20;
		}
	}



	//---------------------------------------------------------------------------
	//
	// draw the full status bar
	//
	//---------------------------------------------------------------------------

	void DrawStatus(ExhumedPlayer pp)
	{
		if (hud_size <= Hud_StbarOverlay)
		{
			// draw the main bar itself
			BeginStatusBar(false, 320, 200, 40);
			if (hud_size == Hud_StbarOverlay) Set43ClipRect();
			DrawImage("TileStatusBar", (160, 200), DI_ITEM_CENTER_BOTTOM);
			screen.ClearClipRect();
		}
		else if (hud_size == Hud_Mini)
		{
			BeginHUD(1, false, 320, 200);
			DrawImage("hud_l", (0, 0), DI_ITEM_LEFT_BOTTOM | DI_SCREEN_LEFT_BOTTOM);
			DrawImage("hud_r", (0, 0), DI_ITEM_RIGHT_BOTTOM | DI_SCREEN_RIGHT_BOTTOM);
		}
		else if (hud_size == Hud_full)
		{
			DrawHUD2(pp);
			return;
		}

		for (int i = 0; i < 4; i++)
		{
			if (pp.keys & (4096 << i))
			{
				DrawStatusSequence(KeySeq + 2 * i, keyanims[i], 0.5, 0.5, true);
			}
		}

		//if (/*!bFullScreen &&*/ nNetTime)
		{
			DrawStatusSequence(127, 0, 0);
			DrawStatusSequence(129, nMagicFrame, nMagicLevel);
			DrawStatusSequence(131, 0, 0); // magic pool frame (bottom)

			DrawStatusSequence(128, 0, 0);
			DrawStatusSequence(1, nHealthFrame, nHealthLevel);
			DrawStatusSequence(125, 0, 0); // draw ankh on health pool
			DrawStatusSequence(130, 0, 0); // draw health pool frame (top)

			// Item on the magic pool
			if (nItemSeq >= 0) DrawStatusSequence(nItemSeq, nItemFrame, 1);

			// draw the blue air level meter when underwater
			if (pp.isUnderwater()) DrawStatusSequence(133, airframe, 0, 0.5);
			else DrawStatusSequence(132, lungframe, 0);

			// item count dots.
			if (nSelectedItem >= 0)
			{
				int count = pp.items[nSelectedItem];
				DrawStatusSequence(156 + 2 * count, 0, 0);
			}

			// life dots.
			DrawStatusSequence(145 + (2 * pp.nLives), 0, 0);

			if (nHurt > 0) DrawStatusSequence(4, nHurt - 1, 0);

			// draw compass
			if (hud_size <= Hud_StbarOverlay) DrawStatusSequence(35, ((pp.GetAngle() + 128) & Raze.kAngleMask) >> 8, 0, 0.5, true);

			//if (hud_size < Hud_full)
			{
				// draw ammo count
				DrawStatusSequence(44, nDigit[2], 0, 0.5, true);
				DrawStatusSequence(45, nDigit[1], 0, 0.5, true);
				DrawStatusSequence(46, nDigit[0], 0, 0.5, true);
			}
		}

		DrawMulti();
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void DoLevelStats(int bottomy, Summaryinfo summary)
	{
		StatsPrintInfo stats;

		stats.fontscale = 1.;
		stats.altspacing = stats.spacing = SmallFont.GetHeight();
		stats.screenbottomspace = bottomy;
		stats.statfont = SmallFont;
		stats.letterColor = TEXTCOLOR_RED;
		stats.standardColor = TEXTCOLOR_UNTRANSLATED; 

		if (automapMode == am_full)
		{
			stats.statfont = Raze.PickSmallFont();
			PrintAutomapInfo(stats, true);
		}
		else if (automapMode == am_off && hud_stats) 
		{
			stats.completeColor = TEXTCOLOR_DARKGREEN;
			PrintLevelStats(stats, summary);
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetItemSeq(ExhumedPlayer pp)
	{
		static const int nItemSeqOffset[] = { 91, 72, 76, 79, 68, 87, 83 };
		static const int nItemMagic[] = { 500, 1000, 100, 500, 400, 200, 700, 0 };

		int nItem = pp.nItem;
		if (nItem < 0)
		{
			nItemSeq = -1;
			return;
		}

		if (nItemMagic[nItem] <= pp.nMagic) nItemAltSeq = 0;
		else nItemAltSeq = 2;

		nItemFrame = 0;
		nItemSeq = nItemSeqOffset[nItem] + nItemAltSeq;
		nItemFrames = Exhumed.SizeofStatusSequence(nItemSeq);
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetMagicFrame(ExhumedPlayer pp)
	{
		int magicperline = 1000 / nMeterRange;

		int newMagicLevel = (1000 - pp.nMagic) / magicperline;
		newMagicLevel = clamp(newMagicLevel, 0, nMeterRange - 1);
		if (newMagicLevel != nMagicLevel || nItemFrames == 0) SetItemSeq(pp);
		nMagicLevel = newMagicLevel;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetHealthFrame(ExhumedPlayer pp)
	{
		if (nHurt)
		{
			nHurt++;
			if (nHurt > Exhumed.SizeofStatusSequence(4)) nHurt = 0;
		}

		int healthperline = 800 / nMeterRange;

		int newHealthLevel = (800 - pp.nHealth) / healthperline;
		newHealthLevel = clamp(newHealthLevel, 0, nMeterRange - 1);
		if (newHealthLevel > nHealthLevel) nHurt = 1;
		nHealthLevel = newHealthLevel;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetCounter(int nVal)
	{
		nCounterDest = clamp(nVal, 0, 999);
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetCounterImmediate(int nVal)
	{
		SetCounter(nVal);
		nCounter = nCounterDest;
		SetCounterDigits();
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetCounterDigits()
	{
		nDigit[2] = 3 * (nCounter / 100 % 10);
		nDigit[1] = 3 * (nCounter / 10 % 10);
		nDigit[0] = 3 * (nCounter % 10);
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void UpdateCounter(ExhumedPlayer pp)
	{
		int nWeapon = pp.nCurrentWeapon;

		if (nWeapon < 0)
		{
			SetCounterImmediate(0);
		}
		else
		{
			int thiscount;

			if (nWeapon >= kWeaponSword && nWeapon <= kWeaponRing)
				thiscount = pp.nAmmo[nWeapon];
			else
				thiscount = 0;

			if (nWeapon != nLastWeapon) SetCounterImmediate(thiscount);
			else SetCounter(thiscount);
		}
		nLastWeapon = nWeapon;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void MoveStatus(ExhumedPlayer pp)
	{
		if (nItemSeq >= 0)
		{
			nItemFrame++;

			if (nItemFrame >= nItemFrames)
			{
				if (nItemSeq == 76) {
					SetItemSeq(pp);
				}
				else
				{
					nItemSeq -= nItemAltSeq;

					if (nItemAltSeq || (totalmoves & 0x1F))
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
					nItemFrames = Exhumed.SizeofStatusSequence(nItemSeq);
				}
			}
		}

		nHealthFrame++;
		if (nHealthFrame >= Exhumed.SizeofStatusSequence(1)) nHealthFrame = 0;

		nMagicFrame++;
		if (nMagicFrame >= Exhumed.SizeofStatusSequence(129)) nMagicFrame = 0;

		if (nCounter == nCounterDest)
		{
			nCounter = nCounterDest;
			ammodelay = 3;
			return;
		}
		else
		{
			ammodelay--;
			if (ammodelay > 0)
				return;
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

					if (nDigit[i] <= 27)
						break;

					if (nDigit[i] >= 30)
						nDigit[i] -= 30;
				}
			}
			else
			{
				nCounter += (nCounterDest - nCounter) >> 1;
				SetCounterDigits();
				return;
			}
		}

		if (!(nDigit[0] % 3)) 
			nCounter = nDigit[0] / 3 + 100 * (nDigit[2] / 3) + 10 * (nDigit[1] / 3);

		eax = nCounterDest - nCounter;
		if (eax < 0)
			eax = -eax;

		ammodelay = max(1, 4 - (eax >> 1));
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void SetPlayerItem(ExhumedPlayer pp)
	{
		if (nSelectedItem != pp.nItem)
		{
			nSelectedItem = pp.nItem;
			SetItemSeq(pp);
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override void Tick()
	{
		totalmoves++;
		let pp = Exhumed.GetViewPlayer();
		SetMagicFrame(pp);
		SetHealthFrame(pp);
		SetPlayerItem(pp);
		UpdateCounter(pp);
		MoveStatus(pp);
		for (int i = 0; i < 4; i++)
		{
			int seq = KeySeq + 2 * i;
			if (pp.keys & (4096 << i))
			{
				if (keyanims[i] < Exhumed.SizeofStatusSequence(seq) - 1)
				{
					Exhumed.MoveStatusSequence(seq, 0);   // this plays the pickup sound.
					keyanims[i]++;
				}
			}
			else
			{
				keyanims[i] = 0;
			}
		}

		if (pp.isUnderwater())
		{

			int nAirFrames = Exhumed.SizeofStatusSequence(133);
			int airperline = 100 / nAirFrames;

			airframe = pp.nAir / airperline;

			if (airframe >= nAirFrames)
			{
				airframe = nAirFrames - 1;
			}
			else if (airframe < 0)
			{
				airframe = 0;
			}
			lungframe = 0;
		}
		else
		{
			int size = Exhumed.SizeofStatusSequence(132);
			if (++lungframe == size) lungframe = 0;
		}
	}


	override void UpdateStatusBar(SummaryInfo info)
	{
		if (hud_size <= Hud_full)
		{
			DrawStatus(Exhumed.GetViewPlayer());
		}
		DoLevelStats(hud_size == Hud_Nothing ? 0 : hud_size == Hud_full ? 20 : 45, info);
	}
}
