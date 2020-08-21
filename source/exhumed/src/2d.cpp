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
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "ps_input.h"
#include "view.h"
#include "raze_sound.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "v_font.h"
#include "texturemanager.h"
#include "gamestate.h"
#include "multipatchtexture.h"
#include "screenjob.h"
#include "sequence.h"
#include "v_draw.h"
#include "m_random.h"

#include <string>

#include <assert.h>

BEGIN_PS_NS

int SyncScreenJob()
{
    while (gamestate == GS_INTERMISSION || gamestate == GS_INTRO)
    {
        UpdateSounds();
        HandleAsync();
        updatePauseStatus();
        D_ProcessEvents();
        ControlInfo info;
        CONTROL_GetInput(&info);
        C_RunDelayedCommands();

        RunScreenJobFrame();	// This handles continuation through its completion callback.
        videoNextPage();
    }
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
        /* probably not such a good idea after all...
        while (left <= right)
        {
            bool didit = false;
            if (!rowset[left]) left++, didit = true;
            if (!rowset[right]) right--, didit = true;
            if (!didit) break;
        }*/
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawAbs(int tile, double x, double y, int shade = 0)
{
    DrawTexture(twod, tileGetTexture(tile), x, y, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, DTA_Color, shadeToLight(shade), TAG_DONE);
}

void DrawRel(int tile, double x, double y, int shade = 0)
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
    if (totalclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic = (int)totalclock + 4;

        if (!nLogoTile)
            nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;

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
                plasma_C[i] = (nSmokeLeft + rand() % logoWidth) << 16;
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
            int badOffset = (pC >> 16) < nSmokeLeft || (pC >> 16) >= nSmokeRight;

            const uint8_t* ptr3 = (logopix + ((pC >> 16) - nSmokeLeft) * tilesiz[nLogoTile].y);

            plasma_C[j] += plasma_B[j];

            if ((pB > 0 && (plasma_C[j] >> 16) >= nSmokeRight) || (pB < 0 && (plasma_C[j] >> 16) <= nSmokeLeft))
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

            uint8_t* v28 = plasmapix + (80 * (plasma_C[j] >> 16));
            v28[nSmokeOffset] = 175;
        }

        tileInvalidate(nPlasmaTile, -1, -1);

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
    int dword_9AB5F = ((int)totalclock / 16) & 3;

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
    int String_Copyright;
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
        String_Copyright = FindGString("COPYRIGHT");
        a = gString[String_Copyright];
        b = gString[String_Copyright + 1];
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
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 24, a, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
            nStringWidth = SmallFont->StringWidth(b);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 16, b, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);

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

    jobs[job++] = { Create<DImageScreen>(tileGetTexture(EXHUMED ? kTileBMGLogo : kTilePIELogo), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { Create<DLobotomyScreen>(tileGetTexture(seq_GetSeqPicnum(kSeqScreens, 0, 0)), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { PlayMovie("book.mov") };
    jobs[job++] = { Create<DMainTitle>() };

    RunScreenJob(jobs, job, completion);

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

static int gLevelNew; // this is needed to get the chosen level out of the map screen class

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
		int totalclock = int(clock * 120 / 1'000'000'000);

		twod->ClearScreen();
		
		if ((totalclock - startTime) / kTimerTicks)
		{
			nIdleSeconds++;
			startTime = totalclock;
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
					int nFireFrame = (((int)totalclock >> 4) & 3);
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
			
			int t = ((((int)totalclock & 16) >> 4));
			
			int nTile = mapNamePlaques[i].tiles[t].nTile;
			
			int nameX = mapNamePlaques[i].xPos + mapNamePlaques[i].tiles[t].xOffs;
			int nameY = mapNamePlaques[i].yPos + mapNamePlaques[i].tiles[t].yOffs + curYPos + screenY;
			
			// Draw level name plaque
			DrawAbs(nTile, nameX, nameY);
			
			int8_t shade = 96;
			
			if (nLevelNew == i)
			{
				shade = (Sin(16 * (int)totalclock) + 31) >> 8;
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
			if (totalclock - runtimer >= (kTimerTicks / 32)) {
				curYPos += var_2C;
				runtimer = (int)totalclock;
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
		gLevelNew = nLevelNew;
		return skiprequest? -1 : nIdleSeconds < 12? 1 : 0;
	}

	bool ProcessInput() override
	{
		if (inputState.GetKeyStatus(sc_UpArrow))
		{
			inputState.ClearKeyStatus(sc_UpArrow);
			
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
		
		if (inputState.GetKeyStatus(sc_DownArrow))
		{
			inputState.ClearKeyStatus(sc_DownArrow);
			
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

 void menu_DrawTheMap(int nLevel, int nLevelNew, int nLevelBest, std::function<void(int)> completion)
 {
	 if (nLevel > kMap20 || nLevelNew > kMap20) // max single player levels
	 {
		 completion(-1);
		 return;
	 }
     nLevelBest = kMap20;

	 if (nLevel < 1) nLevel = 1;
	 if (nLevelNew < 1) nLevelNew = nLevel;
	 
	  auto mycompletion = [=](bool)
	  {
		  completion(gLevelNew+1);
	  };
	  // 0-offset the level numbers
	 gLevelNew = nLevelNew;
	 JobDesc job = { Create<DMapScreen>(nLevel-1, nLevelNew-1, nLevelBest-1) };
	 RunScreenJob(&job, 1, mycompletion);
 }


END_PS_NS
