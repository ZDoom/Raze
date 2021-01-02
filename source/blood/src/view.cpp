//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "v_font.h"

#include "blood.h"
#include "choke.h"
#include "zstring.h"
#include "razemenu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "statusbar.h"
#include "automap.h"
#include "v_draw.h"
#include "glbackend/glbackend.h"

BEGIN_BLD_NS

FixedBitArray<kMaxSprites> gInterpolateSprite;
VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

double gInterpolate;

int gScreenTilt;

FFont *gFont[kFontNum];

void FontSet(int id, int tile, int space)
{
	if (id < 0 || id >= kFontNum || tile < 0 || tile >= kMaxTiles)
		return;

	GlyphSet glyphs;
	for (int i = 1; i < 96; i++)
	{
		auto tex = tileGetTexture(tile + i);
		if (tex && tex->isValid() && tex->GetTexelWidth() > 0 && tex->GetTexelHeight() > 0)
		{
			glyphs.Insert(i + 32, tex);
			tex->SetOffsetsNotForFont();
		}

	}
	const char *names[] = { "smallfont", "bigfont", "gothfont", "smallfont2", "digifont"};
	const char *defs[] = { "defsmallfont", "defbigfont", nullptr, "defsmallfont2", nullptr};
	FFont ** ptrs[] = { &SmallFont, &BigFont, nullptr, &SmallFont2, nullptr};
	gFont[id] =	new ::FFont(names[id], nullptr, defs[id], 0, 0, 0, 0, tileWidth(tile), false, false, false, &glyphs);
	gFont[id]->SetKerning(space);
	if (ptrs[id]) *ptrs[id] = gFont[id];
}

void viewBackupView(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at30 = pPlayer->angle.ang;
    pView->at50 = pPlayer->pSprite->x;
    pView->at54 = pPlayer->pSprite->y;
    pView->at38 = pPlayer->zView;
    pView->at34 = pPlayer->zWeapon-pPlayer->zView-0xc00;
    pView->at24 = pPlayer->horizon.horiz;
    pView->at28 = pPlayer->horizon.horizoff;
    pView->at2c = pPlayer->slope;
    pView->at8 = pPlayer->bobHeight;
    pView->atc = pPlayer->bobWidth;
    pView->at18 = pPlayer->swayHeight;
    pView->at1c = pPlayer->swayWidth;
    pView->look_ang = pPlayer->angle.look_ang;
    pView->rotscrnang = pPlayer->angle.rotscrnang;
    pPlayer->angle.backup();
    pPlayer->horizon.backup();
}

void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at50 += pPlayer->pSprite->x-oldpos->x;
    pView->at54 += pPlayer->pSprite->y-oldpos->y;
    pView->at38 += pPlayer->pSprite->z-oldpos->z;
}

void viewDrawText(int nFont, const char *pString, int x, int y, int nShade, int nPalette, int position, char shadow, unsigned int nStat, uint8_t alpha)
{
    if (nFont < 0 || nFont >= kFontNum || !pString) return;
    FFont *pFont = gFont[nFont];

    //y += pFont->yoff;

	if (position == 1) x -= pFont->StringWidth(pString) / 2;
    if (position == 2) x -= pFont->StringWidth(pString);


	if (shadow)
	{
		DrawText(twod, pFont, CR_UNDEFINED, x+1, y+1, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, 0.5, TAG_DONE);
	}
	DrawText(twod, pFont, CR_UNDEFINED, x, y, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, nPalette),
			 DTA_Color, shadeToLight(nShade), DTA_Alpha, alpha / 255., TAG_DONE);

}

void InitStatusBar(void)
{
    if (r_precache) PrecacheHardwareTextures(2200);
}
GameStats GameInterface::getStats()
{
	return { gKillMgr.Kills, gKillMgr.TotalKills, gSecretMgr.Founds, gSecretMgr.Total, gFrameCount / kTicsPerSec, gPlayer[myconnectindex].fragCount };
}

void viewDrawAimedPlayerName(void)
{
    if (!cl_idplayers || (gView->aim.dx == 0 && gView->aim.dy == 0))
        return;

    int hit = HitScan(gView->pSprite, gView->pSprite->z, gView->aim.dx, gView->aim.dy, gView->aim.dz, CLIPMASK0, 512);
    if (hit == 3)
    {
        spritetype* pSprite = &sprite[gHitInfo.hitsprite];
        if (IsPlayerSprite(pSprite))
        {
            int nPlayer = pSprite->type-kDudePlayer1;
            const char* szName = PlayerName(nPlayer);
            int nPalette = (gPlayer[nPlayer].teamId&3)+11;
            viewDrawText(4, szName, 160, 125, -128, nPalette, 1, 1);
        }
    }
}

static TArray<uint8_t> lensdata;
int *lensTable;

extern int dword_172CE0[16][3];

void viewInit(void)
{
    Printf("Initializing status bar\n");
    InitStatusBar();
    FontSet(0, 4096, 0);
    FontSet(1, 4192, 1);
    FontSet(2, 4288, 1);
    FontSet(3, 4384, 1);
    FontSet(4, 4480, 0);

    lensdata = fileSystem.LoadFile("lens.dat");
    assert(lensdata.Size() == kLensSize * kLensSize * sizeof(int));

    lensTable = (int*)lensdata.Data();
#if B_BIG_ENDIAN == 1
    for (int i = 0; i < kLensSize*kLensSize; i++)
    {
        lensTable[i] = LittleLong(lensTable[i]);
    }
#endif
    uint8_t *data = TileFiles.tileCreate(4077, kLensSize, kLensSize);
    memset(data, TRANSPARENT_INDEX, kLensSize*kLensSize);

    for (int i = 0; i < 16; i++)
    {
        dword_172CE0[i][0] = mulscale16(wrand(), 2048);
        dword_172CE0[i][1] = mulscale16(wrand(), 2048);
        dword_172CE0[i][2] = mulscale16(wrand(), 2048);
    }
}

int othercameradist = 1280;
int cameradist = -1;
int othercameraclock, cameraclock;

void CalcOtherPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fixed_t zm, int smoothratio)
{
    int vX = mulscale30(-Cos(nAng), 1280);
    int vY = mulscale30(-Sin(nAng), 1280);
    int vZ = FixedToInt(mulscale(zm, 1280, 3))-(16<<8);
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    assert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    short nHSector;
    int hX, hY;
    vec3_t pos = {*pX, *pY, *pZ};
    hitdata_t hitdata;
    hitscan(&pos, *vsectnum, vX, vY, vZ, &hitdata, CLIPMASK1);
    nHSector = hitdata.sect;
    hX = hitdata.pos.x;
    hY = hitdata.pos.y;
    int dX = hX-*pX;
    int dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        int nDist;
        if (klabs(vX) > klabs(vY))
        {
            nDist = ClipHigh(divscale16(dX,vX), othercameradist);
        }
        else
        {
            nDist = ClipHigh(divscale16(dY,vY), othercameradist);
        }
        othercameradist = nDist;
    }
    *pX += mulscale16(vX, othercameradist);
    *pY += mulscale16(vY, othercameradist);
    *pZ += mulscale16(vZ, othercameradist);
	int myclock = gFrameClock + mulscale16(4, smoothratio);
    othercameradist = ClipHigh(othercameradist+((myclock-othercameraclock)<<10), 65536);
    othercameraclock = myclock;
    assert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

void CalcPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fixed_t zm, int smoothratio)
{
    int vX = mulscale30(-Cos(nAng), 1280);
    int vY = mulscale30(-Sin(nAng), 1280);
    int vZ = FixedToInt(mulscale(zm, 1280, 3))-(16<<8);
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    assert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    short nHSector;
    int hX, hY;
    hitscangoal.x = hitscangoal.y = 0x1fffffff;
    vec3_t pos = { *pX, *pY, *pZ };
    hitdata_t hitdata;
    hitscan(&pos, *vsectnum, vX, vY, vZ, &hitdata, CLIPMASK1);
    nHSector = hitdata.sect;
    hX = hitdata.pos.x;
    hY = hitdata.pos.y;
    int dX = hX-*pX;
    int dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        int nDist;
        if (klabs(vX) > klabs(vY))
        {
            nDist = ClipHigh(divscale16(dX,vX), cameradist);
        }
        else
        {
            nDist = ClipHigh(divscale16(dY,vY), cameradist);
        }
        cameradist = nDist;
    }
    *pX += mulscale16(vX, cameradist);
    *pY += mulscale16(vY, cameradist);
    *pZ += mulscale16(vZ, cameradist);
	int myclock = gFrameClock + mulscale16(4, smoothratio);
    cameradist = ClipHigh(cameradist+((myclock-cameraclock)<<10), 65536);
    cameraclock = myclock;
    assert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

// by NoOne: show warning msgs in game instead of throwing errors (in some cases)
void viewSetSystemMessage(const char* pMessage, ...) {
    char buffer[1024]; va_list args; va_start(args, pMessage);
    vsprintf(buffer, pMessage, args);
    
    Printf(PRINT_HIGH | PRINT_NOTIFY, "%s\n", buffer); // print it also in console
}

void viewSetMessage(const char *pMessage, const int pal, const MESSAGE_PRIORITY priority)
{
	int printlevel = priority <= MESSAGE_PRIORITY_NORMAL ? PRINT_LOW : priority < MESSAGE_PRIORITY_SYSTEM ? PRINT_MEDIUM : PRINT_HIGH;
    Printf(printlevel|PRINT_NOTIFY, "%s\n", pMessage);
}

void viewSetErrorMessage(const char *pMessage)
{
	Printf(PRINT_BOLD|PRINT_NOTIFY, "%s\n", pMessage);
}

void DoLensEffect(void)
{
	// To investigate whether this can be implemented as a shader effect.
    auto d = tileData(4077);
    assert(d != NULL);
	auto s = tilePtr(4079);
    assert(s != NULL);
    for (int i = 0; i < kLensSize*kLensSize; i++, d++)
        if (lensTable[i] >= 0)
            *d = s[lensTable[i]];
    TileFiles.InvalidateTile(4077);
}

void UpdateDacs(int nPalette, bool bNoTint)
{
    gLastPal = 0;
    auto& tint = lookups.tables[MAXPALOOKUPS - 1];
    tint.tintFlags = 0;
    switch (nPalette)
    {
    case 0:
    default:
        tint.tintColor.r = 255;
        tint.tintColor.g = 255;
        tint.tintColor.b = 255;
        break;
    case 1:
        tint.tintColor.r = 132;
        tint.tintColor.g = 164;
        tint.tintColor.b = 255;
        break;
    case 2:
        tint.tintColor.r = 255;
        tint.tintColor.g = 126;
        tint.tintColor.b = 105;
        break;
    case 3:
        tint.tintColor.r = 162;
        tint.tintColor.g = 186;
        tint.tintColor.b = 15;
        break;
    case 4:
        tint.tintColor.r = 255;
        tint.tintColor.g = 255;
        tint.tintColor.b = 255;
        break;
    }
    videoSetPalette(nPalette);
}

void UpdateBlend()
{
    int nRed = 0;
    int nGreen = 0;
    int nBlue = 0;

    nRed += gView->pickupEffect;
    nGreen += gView->pickupEffect;
    nBlue -= gView->pickupEffect;

    nRed += ClipHigh(gView->painEffect, 85) * 2;
    nGreen -= ClipHigh(gView->painEffect, 85) * 3;
    nBlue -= ClipHigh(gView->painEffect, 85) * 3;

    nRed -= gView->blindEffect;
    nGreen -= gView->blindEffect;
    nBlue -= gView->blindEffect;

    nRed -= gView->chokeEffect >> 6;
    nGreen -= gView->chokeEffect >> 5;
    nBlue -= gView->chokeEffect >> 6;

    nRed = ClipRange(nRed, -255, 255);
    nGreen = ClipRange(nGreen, -255, 255);
    nBlue = ClipRange(nBlue, -255, 255);

    videoTintBlood(nRed, nGreen, nBlue);
}

uint8_t otherMirrorGotpic[2];
uint8_t bakMirrorGotpic[2];
// int gVisibility;

int deliriumTilt, deliriumTurn, deliriumPitch;
int gScreenTiltO, deliriumTurnO, deliriumPitchO;

int gShowFrameRate = 1;

void viewUpdateDelirium(void)
{
    gScreenTiltO = gScreenTilt;
    deliriumTurnO = deliriumTurn;
    deliriumPitchO = deliriumPitch;
	int powerCount;
	if ((powerCount = powerupCheck(gView, kPwUpDeliriumShroom)) != 0)
	{
		int tilt1 = 170, tilt2 = 170, pitch = 20;
        int timer = gFrameClock*4;
		if (powerCount < 512)
		{
			int powerScale = IntToFixed(powerCount) / 512;
			tilt1 = mulscale16(tilt1, powerScale);
			tilt2 = mulscale16(tilt2, powerScale);
			pitch = mulscale16(pitch, powerScale);
		}
		int sin2 = costable[(2*timer-512)&2047] / 2;
		int sin3 = costable[(3*timer-512)&2047] / 2;
		gScreenTilt = mulscale30(sin2+sin3,tilt1);
		int sin4 = costable[(4*timer-512)&2047] / 2;
		deliriumTurn = mulscale30(sin3+sin4,tilt2);
		int sin5 = costable[(5*timer-512)&2047] / 2;
		deliriumPitch = mulscale30(sin4+sin5,pitch);
		return;
	}
	gScreenTilt = ((gScreenTilt+1024)&2047)-1024;
	if (gScreenTilt > 0)
	{
		gScreenTilt -= 8;
		if (gScreenTilt < 0)
			gScreenTilt = 0;
	}
	else if (gScreenTilt < 0)
	{
		gScreenTilt += 8;
		if (gScreenTilt >= 0)
			gScreenTilt = 0;
	}
}

int shakeHoriz, shakeAngle, shakeX, shakeY, shakeZ, shakeBobX, shakeBobY;

void viewUpdateShake(void)
{
    shakeHoriz = 0;
    shakeAngle = 0;
    shakeX = 0;
    shakeY = 0;
    shakeZ = 0;
    shakeBobX = 0;
    shakeBobY = 0;
    if (gView->flickerEffect)
    {
        int nValue = ClipHigh(gView->flickerEffect * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
    if (gView->quakeEffect)
    {
        int nValue = ClipHigh(gView->quakeEffect * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
}


int gLastPal = 0;

int32_t g_frameRate;

static void DrawMap(spritetype* pSprite)
{
    int tm = 0;
    if (windowxy1.x > 0)
    {
        setViewport(Hud_Stbar);
        tm = 1;
    }
    VIEW* pView = &gPrevView[gViewIndex];
    int x = interpolate(pView->at50, pSprite->x, gInterpolate);
    int y = interpolate(pView->at54, pSprite->y, gInterpolate);
    int ang = (!SyncInput() ? gView->angle.sum() : gView->angle.interpolatedsum(gInterpolate)).asbuild();
    DrawOverheadMap(x, y, ang, gInterpolate);
    if (tm)
        setViewport(hud_size);
}



void viewDrawScreen(bool sceneonly)
{
    int nPalette = 0;
	
	if (TestBitString(gotpic, 2342))
	{
		FireProcess();
		ClearBitString(gotpic, 2342);
	}

    if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
    {
        gInterpolate = I_GetTimeFrac() * MaxSmoothRatio;
    }
    else gInterpolate = MaxSmoothRatio;

    if (cl_interpolate)
    {
        DoInterpolations(gInterpolate / MaxSmoothRatio);
    }

    if (automapMode != am_full)
    {
        DoSectorLighting();
    }
    if (automapMode == am_off)
    {
        int basepal = 0;
        if (powerupCheck(gView, kPwUpDeathMask) > 0) basepal = 4;
        else if (powerupCheck(gView, kPwUpReflectShots) > 0) basepal = 1;
        else if (gView->isUnderwater) {
            if (gView->nWaterPal) basepal = gView->nWaterPal;
            else {
                if (gView->pXSprite->medium == kMediumWater) basepal = 1;
                else if (gView->pXSprite->medium == kMediumGoo) basepal = 3;
                else basepal = 2;
            }
        }
        UpdateDacs(basepal);
        UpdateBlend();

        int yxAspect = yxaspect;
        int viewingRange = viewingrange;
        videoSetCorrectedAspect();

        int v1 = xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.)));

        renderSetAspect(v1, yxaspect);

        int cX, cY, cZ, v74, v8c;
        lookangle rotscrnang;
        binangle cA;
        fixedhoriz cH;
        double zDelta, v4c, v48;
        int nSectnum = gView->pSprite->sectnum;
        if (numplayers > 1 && gView == gMe && gPrediction && gMe->pXSprite->health > 0)
        {
            nSectnum = predict.at68;
            cX = interpolate(predictOld.at50, predict.at50, gInterpolate);
            cY = interpolate(predictOld.at54, predict.at54, gInterpolate);
            cZ = interpolate(predictOld.at38, predict.at38, gInterpolate);
            zDelta = finterpolate(predictOld.at34, predict.at34, gInterpolate);
            v74 = interpolate(predictOld.atc, predict.atc, gInterpolate);
            v8c = interpolate(predictOld.at8, predict.at8, gInterpolate);
            v4c = finterpolate(predictOld.at1c, predict.at1c, gInterpolate);
            v48 = finterpolate(predictOld.at18, predict.at18, gInterpolate);

            if (!SyncInput())
            {
                cA = bamang(predict.at30.asbam() + predict.look_ang.asbam());
                cH = predict.at24 + predict.at28;
                rotscrnang = predict.rotscrnang;
            }
            else
            {
                uint32_t oang = predictOld.at30.asbam() + predictOld.look_ang.asbam();
                uint32_t ang = predict.at30.asbam() + predict.look_ang.asbam();
                cA = interpolateangbin(oang, ang, gInterpolate);

                fixed_t ohoriz = (predictOld.at24 + predictOld.at28).asq16();
                fixed_t horiz = (predict.at24 + predict.at28).asq16();
                cH = q16horiz(interpolate(ohoriz, horiz, gInterpolate));

                rotscrnang = interpolateanglook(predictOld.rotscrnang.asbam(), predict.rotscrnang.asbam(), gInterpolate);
            }
        }
        else
        {
            VIEW* pView = &gPrevView[gViewIndex];
            cX = interpolate(pView->at50, gView->pSprite->x, gInterpolate);
            cY = interpolate(pView->at54, gView->pSprite->y, gInterpolate);
            cZ = interpolate(pView->at38, gView->zView, gInterpolate);
            zDelta = finterpolate(pView->at34, gView->zWeapon - gView->zView - (12 << 8), gInterpolate);
            v74 = interpolate(pView->atc, gView->bobWidth, gInterpolate);
            v8c = interpolate(pView->at8, gView->bobHeight, gInterpolate);
            v4c = finterpolate(pView->at1c, gView->swayWidth, gInterpolate);
            v48 = finterpolate(pView->at18, gView->swayHeight, gInterpolate);

            if (!SyncInput())
            {
                cA = gView->angle.sum();
                cH = gView->horizon.sum();
                rotscrnang = gView->angle.rotscrnang;
            }
            else
            {
                cA = gView->angle.interpolatedsum(gInterpolate);
                cH = gView->horizon.interpolatedsum(gInterpolate);
                rotscrnang = gView->angle.interpolatedrotscrn(gInterpolate);
            }
        }

        viewUpdateShake();
        cH += buildhoriz(shakeHoriz);
        cA += buildang(shakeAngle);
        cX += shakeX;
        cY += shakeY;
        cZ += shakeZ;
        v4c += shakeBobX;
        v48 += shakeBobY;
        cH += buildhoriz(mulscale30(0x40000000 - Cos(gView->tiltEffect << 2), 30));
        if (gViewPos == 0)
        {
            if (cl_viewhbob)
            {
                cX -= mulscale30(v74, Sin(cA.asbuild())) >> 4;
                cY += mulscale30(v74, Cos(cA.asbuild())) >> 4;
            }
            if (cl_viewvbob)
            {
                cZ += v8c;
            }
            cZ += xs_CRoundToInt(cH.asq16() / 6553.6);
            cameradist = -1;
            cameraclock = gFrameClock +mulscale16(4, (int)gInterpolate);
        }
        else
        {
            CalcPosition(gView->pSprite, (int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum, cA.asbuild(), cH.asq16(), (int)gInterpolate);
        }
        CheckLink((int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum);
        int v78 = interpolateang(gScreenTiltO, gScreenTilt, gInterpolate);
        uint8_t v14 = 0;
        uint8_t v10 = 0;
        bool bDelirium = powerupCheck(gView, kPwUpDeliriumShroom) > 0;
        static bool bDeliriumOld = false;
        //int tiltcs, tiltdim;
        uint8_t v4 = powerupCheck(gView, kPwUpCrystalBall) > 0;
#ifdef USE_OPENGL
        renderSetRollAngle(rotscrnang.asbuildf());
#endif
        if (v78 || bDelirium)
        {
            renderSetRollAngle(v78);
        }
        else if (v4 && gNetPlayers > 1)
        {
#if 0       // needs to be redone for pure hardware rendering.
            int tmp = (gFrameClock / 240) % (gNetPlayers - 1);
            int i = connecthead;
            while (1)
            {
                if (i == gViewIndex)
                    i = connectpoint2[i];
                if (tmp == 0)
                    break;
                i = connectpoint2[i];
                tmp--;
            }
            PLAYER* pOther = &gPlayer[i];
            //othercameraclock = gFrameClock + mulscale16(4, (int)gInterpolate);;
            if (!tileData(4079))
            {
                TileFiles.tileCreate(4079, 128, 128);
            }
            r enderSetTarget(4079, 128, 128);
            renderSetAspect(65536, 78643);
            int vd8 = pOther->pSprite->x;
            int vd4 = pOther->pSprite->y;
            int vd0 = pOther->zView;
            int vcc = pOther->pSprite->sectnum;
            int v50 = pOther->pSprite->ang;
            int v54 = 0;
            if (pOther->flickerEffect)
            {
                int nValue = ClipHigh(pOther->flickerEffect * 8, 2000);
                v54 += QRandom2(nValue >> 8);
                v50 += QRandom2(nValue >> 8);
                vd8 += QRandom2(nValue >> 4);
                vd4 += QRandom2(nValue >> 4);
                vd0 += QRandom2(nValue);
            }
            if (pOther->quakeEffect)
            {
                int nValue = ClipHigh(pOther->quakeEffect * 8, 2000);
                v54 += QRandom2(nValue >> 8);
                v50 += QRandom2(nValue >> 8);
                vd8 += QRandom2(nValue >> 4);
                vd4 += QRandom2(nValue >> 4);
                vd0 += QRandom2(nValue);
            }
            CalcOtherPosition(pOther->pSprite, &vd8, &vd4, &vd0, &vcc, v50, 0, (int)gInterpolate);
            CheckLink(&vd8, &vd4, &vd0, &vcc);
            if (IsUnderwaterSector(vcc))
            {
                v14 = 10;
            }
            memcpy(bakMirrorGotpic, gotpic + 510, 2);
            memcpy(gotpic + 510, otherMirrorGotpic, 2);
            g_visibility = (int32_t)(ClipLow(gVisibility - 32 * pOther->visibility, 0));
            int vc4, vc8;
            getzsofslope(vcc, vd8, vd4, &vc8, &vc4);
            if (vd0 >= vc4)
            {
                vd0 = vc4 - (gUpperLink[vcc] >= 0 ? 0 : (8 << 8));
            }
            if (vd0 <= vc8)
            {
                vd0 = vc8 + (gLowerLink[vcc] >= 0 ? 0 : (8 << 8));
            }
            v54 = ClipRange(v54, -200, 200);
        RORHACKOTHER:
            int ror_status[16];
            for (int i = 0; i < 16; i++)
                ror_status[i] = TestBitString(gotpic, 4080 + i);
            yax_preparedrawrooms();
            DrawMirrors(vd8, vd4, vd0, IntToFixed(v50), IntToFixed(v54), gInterpolate, -1);
            drawrooms(vd8, vd4, vd0, v50, v54, vcc);
            yax_drawrooms(viewProcessSprites, vcc, 0, gInterpolate);
            bool do_ror_hack = false;
            for (int i = 0; i < 16; i++)
                if (ror_status[i] != TestBitString(gotpic, 4080 + i))
                    do_ror_hack = true;
            if (do_ror_hack)
            {
                spritesortcnt = 0;
                goto RORHACKOTHER;
            }
            memcpy(otherMirrorGotpic, gotpic+510, 2);
            memcpy(gotpic+510, bakMirrorGotpic, 2);
            viewProcessSprites(vd8, vd4, vd0, v50, gInterpolate);
            renderDrawMasks();
            renderRestoreTarget();
#endif
        }
        else
        {
            othercameraclock = gFrameClock + mulscale16(4, (int)gInterpolate);
        }

        if (!bDelirium)
        {
            deliriumTilt = 0;
            deliriumTurn = 0;
            deliriumPitch = 0;
        }
        int unk = 0;

        int nSprite;
        StatIterator it(kStatExplosion);
        while ((nSprite = it.NextIndex()) >= 0)
        {
            spritetype* pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            assert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE* pXSprite = &xsprite[nXSprite];
            if (TestBitString(gotsector, pSprite->sectnum))
            {
                unk += pXSprite->data3 * 32;
            }
        }
        it.Reset(kStatProjectile);
        while ((nSprite = it.NextIndex()) >= 0)
        {
            spritetype* pSprite = &sprite[nSprite];
            switch (pSprite->type) {
            case kMissileFlareRegular:
            case kMissileTeslaAlt:
            case kMissileFlareAlt:
            case kMissileTeslaRegular:
                if (TestBitString(gotsector, pSprite->sectnum)) unk += 256;
                break;
            }
        }
        g_visibility = (int32_t)(ClipLow(gVisibility - 32 * gView->visibility - unk, 0));
        cA += q16ang(interpolateangfix16(IntToFixed(deliriumTurnO), IntToFixed(deliriumTurn), gInterpolate));
        int vfc, vf8;
        getzsofslope(nSectnum, cX, cY, &vfc, &vf8);
        if (cZ >= vf8)
        {
            cZ = vf8 - (gUpperLink[nSectnum] >= 0 ? 0 : (8 << 8));
        }
        if (cZ <= vfc)
        {
            cZ = vfc + (gLowerLink[nSectnum] >= 0 ? 0 : (8 << 8));
        }
        cH = q16horiz(ClipRange(cH.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));
    RORHACK:
        int ror_status[16];
        for (int i = 0; i < 16; i++)
            ror_status[i] = TestBitString(gotpic, 4080 + i);
        fixed_t deliriumPitchI = interpolate(IntToFixed(deliriumPitchO), IntToFixed(deliriumPitch), gInterpolate);
        DrawMirrors(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, gInterpolate, gViewIndex);
        int bakCstat = gView->pSprite->cstat;
        if (gViewPos == 0)
        {
            gView->pSprite->cstat |= 32768;
        }
        else
        {
            gView->pSprite->cstat |= 514;
        }

        renderDrawRoomsQ16(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, nSectnum);
        viewProcessSprites(cX, cY, cZ, cA.asbuild(), gInterpolate);
        bool do_ror_hack = false;
        for (int i = 0; i < 16; i++)
            if (ror_status[i] != TestBitString(gotpic, 4080 + i))
                do_ror_hack = true;
        if (do_ror_hack)
        {
            gView->pSprite->cstat = bakCstat;
            spritesortcnt = 0;
            goto RORHACK;
        }
        sub_5571C(1);
        int nSpriteSortCnt = spritesortcnt;
        renderDrawMasks();
        spritesortcnt = nSpriteSortCnt;
        sub_5571C(0);
        sub_557C4(cX, cY, gInterpolate);
        renderDrawMasks();
        gView->pSprite->cstat = bakCstat;

        if ((v78 || bDelirium) && !sceneonly)
        {
            if (gDeliriumBlur)
            {
                // todo: Implement using modern techniques instead of relying on deprecated old stuff that isn't well supported anymore.
                /* names broken up so that searching for GL keywords won't find them anymore
                if (!bDeliriumOld)
                {
                    g lAccum(GL_LOAD, 1.f);
                }
                else
                {
                    const float fBlur = pow(1.f/3.f, 30.f/g_frameRate);
                    g lAccum(GL _MULT, fBlur);
                    g lAccum(GL _ACCUM, 1.f-fBlur);
                    g lAccum(GL _RETURN, 1.f);
                }
                */
            }
        }

        bDeliriumOld = bDelirium && gDeliriumBlur;

        renderSetAspect(viewingRange, yxAspect);
        int nClipDist = gView->pSprite->clipdist << 2;
        int ve8, vec, vf0, vf4;
        GetZRange(gView->pSprite, &vf4, &vf0, &vec, &ve8, nClipDist, 0);
        if (sceneonly) return;
#if 0
        int tmpSect = nSectnum;
        if ((vf0 & 0xc000) == 0x4000)
        {
            tmpSect = vf0 & (kMaxWalls - 1);
        }
        int v8 = byte_1CE5C2 > 0 && (sector[tmpSect].ceilingstat & 1);
        if (gWeather.at12d8 > 0 || v8)
        {
            gWeather.Draw(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitch, gWeather.at12d8);
            if (v8)
            {
                gWeather.at12d8 = ClipRange(delta * 8 + gWeather.at12d8, 0, 4095);
            }
            else
            {
                gWeather.at12d8 = ClipRange(gWeather.at12d8 - delta * 64, 0, 4095);
            }
        }
#endif
        hudDraw(gView, nSectnum, v4c, v48, zDelta, basepal, gInterpolate);
    }
    UpdateDacs(0, true);    // keep the view palette active only for the actual 3D view and its overlays.
    if (automapMode != am_off)
    {
        DrawMap (gView->pSprite);
    }
    UpdateStatusBar();
    int zn = ((gView->zWeapon-gView->zView-(12<<8))>>7)+220;
    PLAYER *pPSprite = &gPlayer[gMe->pSprite->type-kDudePlayer1];
    if (IsPlayerSprite(gMe->pSprite) && pPSprite->hand == 1)
    {
        gChoke.animateChoke(160, zn, (int)gInterpolate);
    }
#if 0
    if (drawtile_2048)
    {
        DrawStatSprite(2048, xdim-15, 20);
    }
#endif

    viewDrawAimedPlayerName();
    if (paused)
    {
        viewDrawText(1, GStrings("TXTB_PAUSED"), 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        FStringf gTempStr("] %s [", PlayerName(gView->nPlayer));
        viewDrawText(0, gTempStr, 160, 10, 0, 0, 1, 0);
    }
    if (cl_interpolate)
    {
        RestoreInterpolations();
    }
}

bool GameInterface::GenerateSavePic()
{
    viewDrawScreen(true);
    return true;
}

FString GameInterface::GetCoordString()
{
    FString out;

    out.Format("pos= %d, %d, %d - angle = %2.3f",
        gMe->pSprite->x, gMe->pSprite->y, gMe->pSprite->z, gMe->pSprite->ang * (360./2048));

    return out;
}


bool GameInterface::DrawAutomapPlayer(int x, int y, int z, int a, double const smoothratio)
{
    // [MR]: Confirm that this is correct as math doesn't match the variable names.
    int nCos = z * -bsin(a);
    int nSin = z * -bcos(a);
    int nCos2 = mulscale16(nCos, yxaspect);
    int nSin2 = mulscale16(nSin, yxaspect);
    int nPSprite = gView->pSprite->index;

    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        PLAYER* pPlayer = &gPlayer[i];
        spritetype* pSprite = pPlayer->pSprite;
        int px = pSprite->x - x;
        int py = pSprite->y - y;
        int pa = (pSprite->ang - a) & 2047;
        int x1 = dmulscale16(px, nCos, -py, nSin);
        int y1 = dmulscale16(py, nCos2, px, nSin2);
        if (i == gView->nPlayer || gGameOptions.nGameType == 1)
        {
            int nTile = pSprite->picnum;
            int ceilZ, ceilHit, floorZ, floorHit;
            GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, (pSprite->clipdist << 2) + 16, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
            int nTop, nBottom;
            GetSpriteExtents(pSprite, &nTop, &nBottom);
            int nScale = mulscale((pSprite->yrepeat + ((floorZ - nBottom) >> 8)) * z, yxaspect, 16);
            nScale = ClipRange(nScale, 8000, 65536 << 1);
            // Players on automap
            double x = xdim / 2. + x1 / double(1 << 12);
            double y = ydim / 2. + y1 / double(1 << 12);
            // This very likely needs fixing later
            DrawTexture(twod, tileGetTexture(nTile, true), x, y, DTA_ClipLeft, windowxy1.x, DTA_ClipTop, windowxy1.y, DTA_ScaleX, z/1536., DTA_ScaleY, z/1536., DTA_CenterOffset, true,
                DTA_ClipRight, windowxy2.x + 1, DTA_ClipBottom, windowxy2.y + 1, DTA_Alpha, (pSprite->cstat & 2 ? 0.5 : 1.), TAG_DONE);
        }
    }
    return true;
}

void SerializeView(FSerializer& arc)
{
    if (arc.BeginObject("view"))
    {
        arc("screentilt", gScreenTilt)
            ("deliriumtilt", deliriumTilt)
            ("deliriumturn", deliriumTurn)
            ("deliriumpitch", deliriumPitch)
            .EndObject();
    }
}


END_BLD_NS
