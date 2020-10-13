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
#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "ps_input.h"
#include "view.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"
#include "v_font.h"
#include "texturemanager.h"
#include "gamestate.h"
#include "multipatchtexture.h"
#include "screenjob.h"
#include "sequence.h"
#include "v_draw.h"
#include "m_random.h"
#include "gstrings.h"

#include <string>

#include <assert.h>

BEGIN_PS_NS

int selectedlevelnew;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitFonts()
{
    GlyphSet fontdata;
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.

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

    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.

    for (int i = 0; i < 26; i++)
    {
        fontdata.Insert('A' + i, tileGetTexture(3624 + i));
    }
    for (int i = 0; i < 10; i++)
    {
        fontdata.Insert('0' + i, tileGetTexture(3657 + i));
    }
    fontdata.Insert('!', tileGetTexture(3651));
    fontdata.Insert('"', tileGetTexture(3655));
    fontdata.Insert('\'', tileGetTexture(3654));
    fontdata.Insert('`', tileGetTexture(3654));
    fontdata.Insert('.', tileGetTexture(3650));
    fontdata.Insert(',', tileGetTexture(3653));
    fontdata.Insert('-', tileGetTexture(3656));
    fontdata.Insert('?', tileGetTexture(3652));
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    GlyphSet::Iterator it2(fontdata);
    while (it2.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    SmallFont2->SetKerning(1);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawAbs(int tile, double x, double y, int shade = 0)
{
    DrawTexture(twod, tileGetTexture(tile), x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, shadeToLight(shade), TAG_DONE);
}

void DrawRel(int tile, double x, double y, int shade)
{
    // This is slightly different than what the backend does here, but critical for some graphics.
    int offx = (tileWidth(tile) >> 1) + tileLeftOffset(tile);
    int offy = (tileHeight(tile) >> 1) + tileTopOffset(tile);
    DrawAbs(tile, x - offx, y - offy, shade);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

// this might be static within the DoPlasma function?
static uint8_t* PlasmaBuffer;
static int nPlasmaTile = kTile4092;
static int nLogoTile;
static unsigned int nSmokeBottom;
static unsigned int nSmokeRight;
static unsigned int nSmokeTop;
static unsigned int nSmokeLeft;
static int nextPlasmaTic;
static int plasma_A[5] = { 0 };
static int plasma_B[5] = { 0 };
static int plasma_C[5] = { 0 };
static FRandom rnd_plasma;

enum
{
    kPlasmaWidth = 320,
    kPlasmaHeight = 80,
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void menu_DoPlasma()
{
    int ptile = nPlasmaTile;
    int pclock = I_GetBuildTime();
    if (pclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic = pclock + 4;

        if (!nLogoTile)
            nLogoTile = GameLogo();

        if (!PlasmaBuffer)
        {
            auto pixels = TileFiles.tileCreate(kTile4092, kPlasmaWidth, kPlasmaHeight);
            memset(pixels, 96, kPlasmaWidth * kPlasmaHeight);

            PlasmaBuffer = TileFiles.tileCreate(kTile4093, kPlasmaWidth, kPlasmaHeight);
            memset(PlasmaBuffer, 96, kPlasmaWidth * kPlasmaHeight);

            nSmokeLeft = 160 - tilesiz[nLogoTile].x / 2;
            nSmokeRight = nSmokeLeft + tilesiz[nLogoTile].x;

            nSmokeTop = 40 - tilesiz[nLogoTile].y / 2;
            nSmokeBottom = nSmokeTop + tilesiz[nLogoTile].y - 1;

            for (int i = 0; i < 5; i++)
            {
                int logoWidth = tilesiz[nLogoTile].x;
                plasma_C[i] = IntToFixed(nSmokeLeft + rand() % logoWidth);
                plasma_B[i] = (rnd_plasma.GenRand32() % 327680) + 0x10000;

                if (rnd_plasma.GenRand32()&1) {
                    plasma_B[i] = -plasma_B[i];
                }

                plasma_A[i] = rnd_plasma.GenRand32() & 1;
            }
        }

        uint8_t* plasmapix = tileData(nPlasmaTile);
        uint8_t* r_ebx = plasmapix + 81;
        const uint8_t* r_edx = tileData(nPlasmaTile ^ 1) + 81; // flip between value of 4092 and 4093 with xor

        for (int x = 0; x < kPlasmaWidth - 2; x++)
        {
            for (int y = 0; y < kPlasmaHeight - 2; y++)
            {
                uint8_t al = *r_edx;

                if (al != 96)
                {
                    if (al > 158) {
                        *r_ebx = al - 1;
                    }
                    else {
                        *r_ebx = 96;
                    }
                }
                else
                {
                    if (rnd_plasma.GenRand32() & 1) {
                        *r_ebx = *r_edx;
                    }
                    else
                    {
                        uint8_t al = *(r_edx + 1);
                        uint8_t cl = *(r_edx - 1);

                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;
                        al = *(r_edx - 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx - 79);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = *(r_edx - 81);
                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *r_ebx = 96;
                        }
                        else
                        {
                            if (!(rnd_plasma.GenRand32() & 1)) 
                            {
                                cl--;
                            }

                            *r_ebx = cl;
                        }
                    }
                }

                // before restarting inner loop
                r_edx++;
                r_ebx++;
            }

            // before restarting outer loop
            r_edx += 2;
            r_ebx += 2;
        }

        auto logopix = tilePtr(nLogoTile);

        for (int j = 0; j < 5; j++)
        {
            int pB = plasma_B[j];
            int pC = plasma_C[j];
            int badOffset = FixedToInt(pC) < nSmokeLeft || FixedToInt(pC) >= nSmokeRight;

            const uint8_t* ptr3 = (logopix + (FixedToInt(pC) - nSmokeLeft) * tilesiz[nLogoTile].y);

            plasma_C[j] += plasma_B[j];

            if ((pB > 0 && FixedToInt(plasma_C[j]) >= nSmokeRight) || (pB < 0 && FixedToInt(plasma_C[j]) <= nSmokeLeft))
            {
                int esi = plasma_A[j];
                plasma_B[j] = -plasma_B[j];
                plasma_A[j] = esi == 0;
            }

            if (badOffset)
                continue;

            unsigned int nSmokeOffset = 0;

            if (plasma_A[j])
            {
                nSmokeOffset = nSmokeTop;

                while (nSmokeOffset < nSmokeBottom)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset++;
                    ptr3++;
                }
            }
            else
            {
                nSmokeOffset = nSmokeBottom;

                ptr3 += tilesiz[nLogoTile].y - 1;

                while (nSmokeOffset > nSmokeTop)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset--;
                    ptr3--;
                }
            }

            uint8_t* v28 = plasmapix + (80 * FixedToInt(plasma_C[j]));
            v28[nSmokeOffset] = 175;
        }

        TileFiles.InvalidateTile(nPlasmaTile);

        // flip between tile 4092 and 4093
        if (nPlasmaTile == kTile4092) {
            nPlasmaTile = kTile4093;
        }
        else if (nPlasmaTile == kTile4093) {
            nPlasmaTile = kTile4092;
        }
    }
    DrawAbs(ptile, 0, 0);
    DrawRel(nLogoTile, 160, 40);

    // draw the fire urn/lamp thingies
    int dword_9AB5F = (pclock / 16) & 3;

    DrawRel(kTile3512 + dword_9AB5F, 50, 150);
    DrawRel(kTile3512 + ((dword_9AB5F + 2) & 3), 270, 150);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DLobotomyScreen : public DImageScreen
{
public:
	DLobotomyScreen(FGameTexture *tex, int fade) : DImageScreen(tex, fade)
	{}
	
    int Frame(uint64_t clock, bool skiprequest) override
    {
		if (clock == 0) PlayLocalSound(StaticSound[kSoundJonLaugh2], 7000, false, CHANF_UI);
        if (skiprequest) StopLocalSound();
		return DImageScreen::Frame(clock, skiprequest);
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const short skullDurations[] = { 6, 25, 43, 50, 68, 78, 101, 111, 134, 158, 173, 230, 600 };

class DMainTitle : public DScreenJob
{
    const char* a;
    const char* b;
    int state = 0;
    int var_18;
    int var_4 = 0;
    int esi = 130;
    int nCount = 0;
    int start;


public:
    DMainTitle() : DScreenJob(fadein)
    {
        a = GStrings("TXT_EX_COPYRIGHT1");
        b = GStrings("TXT_EX_COPYRIGHT2");
        var_18 = skullDurations[0];
    }

    int Frame(uint64_t clock, bool skiprequest) override
    {
        int ticker = clock * 120 / 1'000'000'000;
        if (clock == 0)
        {
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
            playCDtrack(19, true);
        }
        if (clock > 1'000'000 && state == 0 && !soundEngine->IsSourcePlayingSomething(SOURCE_None, nullptr,CHAN_AUTO, -1))
        {
            if (time(0) & 0xF)
                PlayGameOverSound();
            else 
                PlayLocalSound(StaticSound[61], 0, false, CHANF_UI);
            state = 1;
            start = ticker;
        }

        twod->ClearScreen();

        menu_DoPlasma();

        DrawRel(kSkullHead, 160, 100);
        switch (state)
        {
        case 0:
            DrawRel(kSkullJaw, 161, 130);
            break;

        case 1:
        {
            int nStringWidth = SmallFont->StringWidth(a);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 24, a, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
            nStringWidth = SmallFont->StringWidth(b);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 16, b, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);

            if (ticker > var_18)
            {
                nCount++;
                if (nCount > 12) return 0;
                var_18 = start + skullDurations[nCount];
                var_4 = var_4 == 0;
            }

            short nTile = kSkullJaw;

            if (var_4)
            {
                if (esi >= 135) nTile = kTile3583;
                else esi += 5;
            }
            else if (esi <= 130) esi = 130;
            else esi -= 2;

            int y;

            if (nTile == kTile3583)
            {
                y = 131;
            }
            else
            {
                y = esi;
                if (y > 135) y = 135;
            }

            DrawRel(nTile, 161, y);
            break;
        }
        }
        return skiprequest? -1 : 1;
    }
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DScreenJob *PlayMovie(const char* fileName);

void DoTitle(CompletionFunc completion)
{
    JobDesc jobs[5];
    int job = 0;

    jobs[job++] = { Create<DImageScreen>(tileGetTexture(PublisherLogo()), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { Create<DLobotomyScreen>(tileGetTexture(seq_GetSeqPicnum(kSeqScreens, 0, 0)), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { PlayMovie("book.mov") };
    jobs[job++] = { Create<DMainTitle>() };

    RunScreenJob(jobs, job, completion, true, true);

}

//---------------------------------------------------------------------------
//
// pre-level map display
//
//---------------------------------------------------------------------------

 static const int8_t MapLevelOffsets[] = { 0, 50, 10, 20, 0, 45, -20, 20, 5, 0, -10, 10, 30, -20, 0, 20, 0, 0, 0, 0 };

 struct TILEFRAMEDEF
 {
	 short nTile;
	 short xOffs;
	 short yOffs;
 };

 // 22 bytes
 struct MapNamePlaque
 {
	 short xPos;
	 short yPos;
	 TILEFRAMEDEF tiles[2];
	 TILEFRAMEDEF text;
 };

 static const MapNamePlaque mapNamePlaques[] = {
	 { 100, 170, kTile3376, 0, 0, kTile3377, 0, 0, kTile3411, 18, 6 },
	 { 230, 10,  kTile3378, 0, 0, kTile3379, 0, 0, kTile3414, 18, 6 }, // DENDUR (level 2)
	 { 180, 125, kTile3380, 0, 0, kTile3381, 0, 0, kTile3417, 18, 6 }, // Kalabash
	 { 10,  95,  kTile3382, 0, 0, kTile3383, 0, 0, kTile3420, 18, 6 },
	 { 210, 160, kTile3384, 0, 0, kTile3385, 0, 0, kTile3423, 18, 6 },
	 { 10,  110, kTile3371, 0, 0, kTile3386, 0, 0, kTile3426, 18, 6 },
	 { 10,  50,  kTile3387, 0, 0, kTile3388, 0, 0, kTile3429, 18, 6 },
	 { 140, 0,   kTile3389, 0, 0, kTile3390, 0, 0, kTile3432, 18, 6 },
	 { 30,  20,  kTile3391, 0, 0, kTile3392, 0, 0, kTile3435, 18, 6 },
	 { 200, 150, kTile3409, 0, 0, kTile3410, 0, 0, kTile3418, 20, 4 },
	 { 145, 170, kTile3393, 0, 0, kTile3394, 0, 0, kTile3438, 18, 6 },
	 { 80,  80,  kTile3395, 0, 0, kTile3396, 0, 0, kTile3441, 18, 6 },
	 { 15,  0,   kTile3397, 0, 0, kTile3398, 0, 0, kTile3444, 18, 5 },
	 { 220, 35,  kTile3399, 0, 0, kTile3400, 0, 0, kTile3447, 18, 6 },
	 { 190, 40,  kTile3401, 0, 0, kTile3402, 0, 0, kTile3450, 18, 6 },
	 { 20,  130, kTile3403, 0, 0, kTile3404, 0, 0, kTile3453, 19, 6 },
	 { 220, 160, kTile3405, 0, 0, kTile3406, 0, 0, kTile3456, 18, 6 },
	 { 20,  10,  kTile3407, 0, 0, kTile3408, 0, 0, kTile3459, 18, 6 },
	 { 200, 10,  kTile3412, 0, 0, kTile3413, 0, 0, kTile3419, 18, 5 },
	 { 20,  10,  kTile3415, 0, 0, kTile3416, 0, 0, kTile3421, 19, 4 }
 };

 // 3 different types of fire, each with 4 frames
 static const TILEFRAMEDEF FireTiles[3][4] = {
	 {{ kTile3484,0,3 },{ kTile3485,0,0 },{ kTile3486,0,3 },{ kTile3487,0,0 }},
	 {{ kTile3488,1,0 },{ kTile3489,1,0 },{ kTile3490,0,1 },{ kTile3491,1,1 }},
	 {{ kTile3492,1,2 },{ kTile3493,1,0 },{ kTile3494,1,2 },{ kTile3495,1,0 }}
 };

 struct Fire
 {
	 short nFireType;
	 short xPos;
	 short yPos;
 };

 // 20 bytes
 struct MapFire
 {
	 short nFires;
	 Fire fires[3];
 };

 /*
  level 1 - 3 fires
  level 2 - 3 fires
  level 3 - 1 fire

 */

 static const MapFire MapLevelFires[] = {
	 3, {{0, 107, 95}, {1, 58,  140}, {2, 28,   38}},
	 3, {{2, 240,  0}, {0, 237,  32}, {1, 200,  30}},
	 2, {{2, 250, 57}, {0, 250,  43}, {2, 200,  70}},
	 2, {{1, 82,  59}, {2, 84,   16}, {0, 10,   95}},
	 2, {{2, 237, 50}, {1, 215,  42}, {1, 210,  50}},
	 3, {{0, 40,   7}, {1, 75,    6}, {2, 100,  10}},
	 3, {{0, 58,  61}, {1, 85,   80}, {2, 111,  63}},
	 3, {{0, 260, 65}, {1, 228,   0}, {2, 259,  15}},
	 2, {{0, 81,  38}, {2, 58,   38}, {2, 30,   20}},
	 3, {{0, 259, 49}, {1, 248,  76}, {2, 290,  65}},
	 3, {{2, 227, 66}, {0, 224,  98}, {1, 277,  30}},
	 2, {{0, 100, 10}, {2, 48,   76}, {2, 80,   80}},
	 3, {{0, 17,   2}, {1, 29,   49}, {2, 53,   28}},
	 3, {{0, 266, 42}, {1, 283,  99}, {2, 243, 108}},
	 2, {{0, 238, 19}, {2, 240,  92}, {2, 190,  40}},
	 2, {{0, 27,   0}, {1, 70,   40}, {0, 20,  130}},
	 3, {{0, 275, 65}, {1, 235,   8}, {2, 274,   6}},
	 3, {{0, 75,  45}, {1, 152, 105}, {2, 24,   68}},
	 3, {{0, 290, 25}, {1, 225,  63}, {2, 260, 110}},
	 0, {{1, 20,  10}, {1, 20,   10}, {1, 20,   10}}
 };

class DMapScreen : public DScreenJob
{
	int i;
	int x = 0;
	int var_2C = 0;
	int nIdleSeconds = 0;
	int startTime = 0;
	int runtimer = 0;
	
	int curYPos, destYPos;
	int nLevel, nLevelNew, nLevelBest;

public:
	DMapScreen(int nLevel_, int nLevelNew_, int nLevelBest_) : DScreenJob(fadein|fadeout), nLevel(nLevel_), nLevelNew(nLevelNew_), nLevelBest(nLevelBest_)
	{
		curYPos = MapLevelOffsets[nLevel] + (200 * (nLevel / 2));
		destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

		if (curYPos < destYPos) {
			var_2C = 2;
		}

		if (curYPos > destYPos) {
			var_2C = -2;
		}

		// Trim smoke in widescreen
#if 0
		vec2_t mapwinxy1 = windowxy1, mapwinxy2 = windowxy2;
		int32_t width = mapwinxy2.x - mapwinxy1.x + 1, height = mapwinxy2.y - mapwinxy1.y + 1;
		if (3 * width > 4 * height)
		{
			mapwinxy1.x += (width - 4 * height / 3) / 2;
			mapwinxy2.x -= (width - 4 * height / 3) / 2;
		}
#endif
	}
	
	int Frame(uint64_t clock, bool skiprequest) override
	
	{
		int currentclock = int(clock * 120 / 1'000'000'000);

		twod->ClearScreen();
		
		if ((currentclock - startTime) / kTimerTicks)
		{
			nIdleSeconds++;
			startTime = currentclock;
		}
		
		int tileY = curYPos;
		
		// Draw the background screens
		for (i = 0; i < 10; i++)
		{
			DrawAbs(kTile3353 + i, x, tileY);
			tileY -= 200;
		}
		
		// for each level - drawing the 'level completed' on-fire smoke markers
		for (i = 0; i < kMap20; i++)
		{
			int screenY = (i >> 1) * -200;
			
			if (nLevelBest >= i) // check if the player has finished this level
			{
				for (int j = 0; j < MapLevelFires[i].nFires; j++)
				{
					int nFireFrame = ((currentclock >> 4) & 3);
					assert(nFireFrame >= 0 && nFireFrame < 4);
					
					int nFireType = MapLevelFires[i].fires[j].nFireType;
					assert(nFireType >= 0 && nFireType < 3);
					
					int nTile = FireTiles[nFireType][nFireFrame].nTile;
					int smokeX = MapLevelFires[i].fires[j].xPos + FireTiles[nFireType][nFireFrame].xOffs;
					int smokeY = MapLevelFires[i].fires[j].yPos + FireTiles[nFireType][nFireFrame].yOffs + curYPos + screenY;
					
					// Use rotatesprite to trim smoke in widescreen
					DrawAbs(nTile, smokeX, smokeY);
					// Todo: mask out the sides of the screen if the background is not widescreen.
				}
			}
			
			int t = (((currentclock & 16) >> 4));
			
			int nTile = mapNamePlaques[i].tiles[t].nTile;
			
			int nameX = mapNamePlaques[i].xPos + mapNamePlaques[i].tiles[t].xOffs;
			int nameY = mapNamePlaques[i].yPos + mapNamePlaques[i].tiles[t].yOffs + curYPos + screenY;
			
			// Draw level name plaque
			DrawAbs(nTile, nameX, nameY);
			
			int8_t shade = 96;
			
			if (nLevelNew == i)
			{
				shade = (Sin(16 * currentclock) + 31) >> 8;
			}
			else if (nLevelBest >= i)
			{
				shade = 31;
			}
			
			int textY = mapNamePlaques[i].yPos + mapNamePlaques[i].text.yOffs + curYPos + screenY;
			int textX = mapNamePlaques[i].xPos + mapNamePlaques[i].text.xOffs;
			nTile = mapNamePlaques[i].text.nTile;
			
			// draw the text, alternating between red and black
			DrawAbs(nTile, textX, textY, shade);
		}
		
		if (curYPos != destYPos)
		{
			// scroll the map every couple of ms
			if (currentclock - runtimer >= (kTimerTicks / 32)) {
				curYPos += var_2C;
				runtimer = currentclock;
			}
			
			if (inputState.CheckAllInput())
			{
				if (var_2C < 8) {
					var_2C *= 2;
				}
				
			}
			
			if (curYPos > destYPos&& var_2C > 0) {
				curYPos = destYPos;
			}
			
			if (curYPos < destYPos && var_2C < 0) {
				curYPos = destYPos;
			}
			
			nIdleSeconds = 0;
		}
		selectedlevelnew = nLevelNew + 1;
		return skiprequest? -1 : nIdleSeconds < 12? 1 : 0;
	}

	bool ProcessInput() override
	{
		if (buttonMap.ButtonDown(gamefunc_Move_Forward))
		{
			buttonMap.ClearButton(gamefunc_Move_Forward);
			
			if (curYPos == destYPos && nLevelNew <= nLevelBest)
			{
				nLevelNew++;
				assert(nLevelNew < 20);
				
				destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));
				
				if (curYPos <= destYPos) {
					var_2C = 2;
				}
				else {
					var_2C = -2;
				}
				
				nIdleSeconds = 0;
			}
			return true;
		}
		
		if (buttonMap.ButtonDown(gamefunc_Move_Backward))
		{
			buttonMap.ClearButton(gamefunc_Move_Backward);

			if (curYPos == destYPos && nLevelNew > 0)
			{
				nLevelNew--;
				assert(nLevelNew >= 0);
				
				destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));
				
				if (curYPos <= destYPos) {
					var_2C = 2;
				}
				else {
					var_2C = -2;
				}
				
				nIdleSeconds = 0;
			}
			return true;
		}

		return false;
	}
};

 void menu_DrawTheMap(int nLevel, int nLevelNew, int nLevelBest, TArray<JobDesc> &jobs)
 {
	 if (nLevel > kMap20 || nLevelNew > kMap20) // max single player levels
	 {
		 return;
	 }
#ifdef _DEBUG
     nLevelBest = kMap20;
#endif

	 if (nLevel < 1) nLevel = 1;
	 if (nLevelNew < 1) nLevelNew = nLevel;
	 
     // 0-offset the level numbers
     jobs.Push( { Create<DMapScreen>(nLevel-1, nLevelNew-1, nLevelBest-1) });
 }

//---------------------------------------------------------------------------
//
// text overlay
//
//---------------------------------------------------------------------------

void TextOverlay::Start(double starttime)
{
    lastclock = starttime;
}

void TextOverlay::ComputeCinemaText()
{
    int i = 0;
    for (auto &line : screentext)
    { 
        int nWidth = SmallFont->StringWidth(line);
        nLeft[i++] = 160 - nWidth / 2;
    }

    nCrawlY = 199;
    nHeight = screentext.Size() * 10;
}

void TextOverlay::ReadyCinemaText(uint16_t nVal)
{
    FStringf label("TXT_EX_CINEMA%d", nVal);
    label = GStrings(label);
    screentext = label.Split("\n");
    ComputeCinemaText();
}

void TextOverlay::DisplayText()
{
    if (nHeight + nCrawlY > 0)
    {
        double y = nCrawlY;
        unsigned int i = 0;

        while (i < screentext.Size() && y <= 199)
        {
            if (y >= -10) {
                DrawText(twod, SmallFont, CR_UNDEFINED, nLeft[i], y, screentext[i], DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_BasePalettes, currentCinemaPalette), TAG_DONE);
            }

            i++;
            y += 10;
        }
    }
}

bool TextOverlay::AdvanceCinemaText(double clock)
{
    if (nHeight + nCrawlY > 0 || CDplaying())
    {
        nCrawlY-= (clock - lastclock) / 15.;   // do proper interpolation.
        lastclock = clock;
        return true;
    }

    return false;
}


 //---------------------------------------------------------------------------
//
// cinema
//
//---------------------------------------------------------------------------

enum EScenes
{
    CINEMA_BEFORE_LEVEL_5,
    CINEMA_AFTER_LEVEL_10,
    CINEMA_BEFORE_LEVEL_11,
    CINEMA_AFTER_LEVEL_15,
    CINEMA_LOSE_SCENE,
    CINEMA_AFTER_LEVEL_20,
};

struct CinemaDef
{
    short tile;
    short palette;
    short text;
    short track;
};

static CinemaDef cinemas[] = {
    { 3449, 3, 2, 2},
    { 3451, 5, 4, 3},
    { 3454, 1, 3, 4},
    { 3446, 7, 6, 6},
    { 3445, 4, 7, 7},
    { 3448, 6, 8, 8}
};

static const char * const cinpalfname[] = {
    "3454.pal",
    "3452.pal",
    "3449.pal",
    "3445.pal",
    "set.pal",
    "3448.pal",
    "3446.pal",
    "hsc1.pal",
    "2972.pal",
    "2973.pal",
    "2974.pal",
    "2975.pal",
    "2976.pal",
    "heli.pal",
    "2978.pal",
    "terror.pal"
};

void uploadCinemaPalettes()
{
    for (int i = 0; i < countof(cinpalfname); i++)
    {
        uint8_t palette[768] = {};
        auto hFile = fileSystem.OpenFileReader(cinpalfname[i]);
        if (hFile.isOpen())
            hFile.Read(palette, 768);
        for (auto& c : palette)
            c <<= 2;
        paletteSetColorTable(ANIMPAL+i, palette, false, true);
    }
}

//---------------------------------------------------------------------------
//
// cinema
//
//---------------------------------------------------------------------------

class DCinema : public DScreenJob
{
    TextOverlay text;
	short cinematile;
	int currentCinemaPalette;
    int edx;
    int check;

public:
	DCinema(int nVal, int checklevel = -1) : DScreenJob(fadein|fadeout)
	{
		if (nVal < 0 || nVal >5) return;
		cinematile = cinemas[nVal].tile;
		currentCinemaPalette = cinemas[nVal].palette;
        text.Start(0);
        text.ReadyCinemaText(cinemas[nVal].text);
        text.SetPalette(currentCinemaPalette);
        edx = cinemas[nVal].track;
        check = checklevel;
    }
	
    int Frame(uint64_t clock, bool skiprequest) override
    {

        if (clock == 0)
        {
            if (check > 0 && check != selectedlevelnew) return 0; // immediately abort if the player selected a different level on the map
            StopAllSounds();
            if (edx != -1)
            {
                playCDtrack(edx + 2, false);
            }
        }

        twod->ClearScreen();
        DrawTexture(twod, tileGetTexture(cinematile), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, TRANSLATION(Translation_BasePalettes, currentCinemaPalette), TAG_DONE);

        text.DisplayText();
        auto cont = text.AdvanceCinemaText(clock * (120. / 1'000'000'000));
        int ret = skiprequest ? -1 : cont ? 1 : 0;

        // quit the game if we've finished level 4 and displayed the advert text
        if (isShareware() && currentCinemaPalette == 3 && ret != 1) 
        {
            ExitGame();
        }
        return ret;
    }
};

//---------------------------------------------------------------------------
//
// last level cinema
//
//---------------------------------------------------------------------------

class DLastLevelCinema : public DScreenJob
{
    int var_24 = 16;
    int var_28 = 12;

    int ebp;
    int phase = 0;
    int nextclock = 4;
    unsigned int nStringTypeOn, nCharTypeOn;
    int screencnt = 0;

    TArray<FString> screentext;

public:
    DLastLevelCinema() : DScreenJob(fadein | fadeout) {}

private:
    void DoStatic(int a, int b)
    {
        auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);

        int v2 = 160 - a / 2;
        int v4 = 81 - b / 2;

        int var_18 = v2 + a;
        int v5 = v4 + b;

        auto pTile = (pixels + (200 * v2)) + v4;

        TileFiles.InvalidateTile(kTileLoboLaptop);

        while (v2 < var_18)
        {
            uint8_t* pStart = pTile;
            pTile += 200;

            int v7 = v4;

            while (v7 < v5)
            {
                *pStart = RandomBit() * 16;

                v7++;
                pStart++;
            }
            v2++;
        }
    }

    void Phase1()
    {
        if (var_24 >= 116)
        {
            if (var_28 < 192)
                var_28 += 20;
        }
        else
        {
            var_24 += 20;
        }

        DoStatic(var_28, var_24);
    }

    bool InitPhase2()
    {
        FStringf label("TXT_EX_LASTLEVEL%d", screencnt + 1);
        label = GStrings(label);
        screentext = label.Split("\n");
        if (screentext.Size() == 0) return false;

        nStringTypeOn = 0;
        nCharTypeOn = 0;

        ebp = screentext.Size() * 4;    // half height of the entire text
        ebp = 81 - ebp;                 // offset from the screen's center.

        auto tex = dynamic_cast<FRestorableTile*>(tileGetTexture(kTileLoboLaptop)->GetTexture()->GetImage());
        if (tex) tex->Reload();
        TileFiles.InvalidateTile(kTileLoboLaptop);
        return true;
    }

    bool Phase3()
    {
        DoStatic(var_28, var_24);

        if (var_28 > 20) {
            var_28 -= 20;
            return true;
        }

        if (var_24 > 20) {
            var_24 -= 20;
            return true;
        }
        return false;
    }

    void DisplayPhase2()
    {
        int yy = ebp;

        auto p = GStrings["REQUIRED_CHARACTERS"];
        if (1)//p && *p)
        {
            yy *= 2;
            for (int i = 0; i < nStringTypeOn; i++, yy += 10)
            {
                DrawText(twod, ConFont, CR_GREEN, 140, yy, screentext[i], DTA_FullscreenScale, FSMode_Fit640x400, TAG_DONE);
            }
            DrawText(twod, ConFont, CR_GREEN, 140, yy, screentext[nStringTypeOn], DTA_FullscreenScale, FSMode_Fit640x400, DTA_TextLen, nCharTypeOn, TAG_DONE);
        }
        else
        {
            for (int i = 0; i < nStringTypeOn; i++, yy += 8)
            {
                DrawText(twod, SmallFont2, CR_UNTRANSLATED, 70, yy, screentext[i], DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
            }
            DrawText(twod, SmallFont2, CR_UNTRANSLATED, 70, yy, screentext[nStringTypeOn], DTA_FullscreenScale, FSMode_Fit320x200, DTA_TextLen, nCharTypeOn, TAG_DONE);
        }
    }

    int Frame(uint64_t clock, bool skiprequest) override
    {
        if (clock == 0)
        {
            PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);
            phase = 1;
        }
        int currentclock = clock * 120 / 1'000'000'000;
        twod->ClearScreen();
        DrawTexture(twod, tileGetTexture(kTileLoboLaptop), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
        switch (phase)
        {
        case 1:
            if (currentclock >= nextclock)
            {
                Phase1();
                nextclock += 4;
            }
            if (skiprequest || currentclock >= 240)
            {
                InitPhase2();
                phase = 2;
                skiprequest = 0;
            }
            break;

        case 2:
            if (currentclock >= nextclock)
            {
                if (screentext[nStringTypeOn][nCharTypeOn] != ' ')
                    PlayLocalSound(StaticSound[kSound71], 0, false, CHANF_UI);

                nCharTypeOn++;
                nextclock += 4;
                if (screentext[nStringTypeOn][nCharTypeOn] == 0)
                {
                    nCharTypeOn = 0;
                    nStringTypeOn++;
                    if (nStringTypeOn >= screentext.Size())
                    {
                        nextclock = (kTimerTicks * (screentext.Size() + 2)) + currentclock;
                        phase = 3;
                    }

                }
            }
            DisplayPhase2();
            if (skiprequest)
            {
                nextclock = (kTimerTicks * (screentext.Size() + 2)) + currentclock;
                phase = 4;
            }
            break;

        case 3:
            DisplayPhase2();
            if (currentclock >= nextclock || skiprequest)
            {
                PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);
                phase = 4;
                nextclock = currentclock + 240;
                skiprequest = 0;
            }
            break;

        case 4:
            if (currentclock >= nextclock)
            {
                skiprequest |= !Phase3();
                nextclock += 4;
            }
            if (skiprequest || currentclock >= 240)
            {
                // Go to the next text page.
                if (screencnt != 2)
                {
                    screencnt++;
                    nextclock = currentclock + 240;
                    skiprequest = 0;
                    phase = 1;
                }
                else return skiprequest ? -1 : 0;
            }
        }
        return 1;
    }

};

//---------------------------------------------------------------------------
//
// Credits roll
//
//---------------------------------------------------------------------------

class DExCredits : public DScreenJob
{
    TArray<FString> credits;
    TArray<FString> pagelines;
    uint64_t page;
    uint64_t pagetime;

public:
    DExCredits()
    {
        auto textdata = fileSystem.LoadFile("credits.txt", 1);
        FString text = (char*)textdata.Data();
        text.Substitute("\r", "");
        credits = text.Split("\n\n");
    }

private:
    int Frame(uint64_t clock, bool skiprequest) override
    {
        if (clock == 0)
        {
            if (credits.Size() == 0) return 0;
            playCDtrack(19, false);
            pagetime = 0;
            page = -1;
        }
        if (clock >= pagetime || skiprequest)
        {
            page++;
            if (page < credits.Size())
                pagelines = credits[page].Split("\n");
            else
            {
                if (skiprequest || !CDplaying()) return 0;
                pagelines.Clear();
            }
            pagetime = clock + 2'000'000'000; // 
        }
        twod->ClearScreen();

        int y = 100 - ((10 * (pagelines.Size() - 1)) / 2);

        for (unsigned i = 0; i < pagelines.Size(); i++)
        {
            uint64_t ptime = (pagetime-clock) / 1'000'000;
            int light;

            if (ptime < 255) light = ptime;
            else if (ptime > 2000 - 255) light = 2000 - ptime;
            else light = 255;

            auto color = PalEntry(255, light, light, light);

            int nStringWidth = SmallFont->StringWidth(pagelines[i]);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, y, pagelines[i], DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, color, TAG_DONE);
            y += 10;
        }
        return 1;
    }
};

//---------------------------------------------------------------------------
//
// player died
//
//---------------------------------------------------------------------------

void DoGameOverScene(bool finallevel)
{
    JobDesc job;

    if (finallevel)
    {
        job = { Create<DCinema>(CINEMA_LOSE_SCENE) };
    }
    else
    {
        StopCD();
        PlayGameOverSound();
        job = { Create<DImageScreen>(tileGetTexture(kTile3591), DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff, TRANSLATION(Translation_BasePalettes, 16)) };
    }
    RunScreenJob(&job, 1, [](bool) { gameaction = ga_mainmenu; });
}


void DoAfterCinemaScene(int nLevel, TArray<JobDesc>& jobs)
{
    int scene = -1;
    if (nLevel == 10) scene = CINEMA_AFTER_LEVEL_10;
    if (nLevel == 15) scene = CINEMA_AFTER_LEVEL_15;
    if (nLevel == 20) scene = CINEMA_AFTER_LEVEL_20;
    if (scene > 0) jobs.Push({ Create<DCinema>(scene) });
    if (nLevel == 19) { jobs.Push({ Create<DLastLevelCinema>() }); selectedlevelnew = 20; }
    if (nLevel == 20) jobs.Push({ Create<DExCredits>() });
}

void DoBeforeCinemaScene(int nLevel, TArray<JobDesc>& jobs)
{
    if (nLevel == 5) jobs.Push({ Create<DCinema>(CINEMA_BEFORE_LEVEL_5) });
    else if (nLevel == 11) jobs.Push({ Create<DCinema>(CINEMA_BEFORE_LEVEL_11, 11) });
}

END_PS_NS
