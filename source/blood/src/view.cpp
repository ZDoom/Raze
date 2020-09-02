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

#include "endgame.h"
#include "aistate.h"
#include "map2d.h"
#include "loadsave.h"
#include "sectorfx.h"
#include "choke.h"
#include "view.h"
#include "nnexts.h"
#include "zstring.h"
#include "menu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "glbackend/glbackend.h"

BEGIN_BLD_NS


VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

struct INTERPOLATE {
    void *pointer;
    int value;
    int value2;
    INTERPOLATE_TYPE type;
};

double gInterpolate;
int nInterpolations;
char gInterpolateSprite[(kMaxSprites+7)>>3];
char gInterpolateWall[(kMaxWalls+7)>>3];
char gInterpolateSector[(kMaxSectors+7)>>3];

#define kMaxInterpolations 16384

INTERPOLATE gInterpolation[kMaxInterpolations];

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
    pView->at30 = pPlayer->q16ang;
    pView->at50 = pPlayer->pSprite->x;
    pView->at54 = pPlayer->pSprite->y;
    pView->at38 = pPlayer->zView;
    pView->at34 = pPlayer->zWeapon-pPlayer->zView-0xc00;
    pView->at24 = pPlayer->q16horiz;
    pView->at28 = pPlayer->q16slopehoriz;
    pView->at2c = pPlayer->slope;
    pView->at8 = pPlayer->bobHeight;
    pView->atc = pPlayer->bobWidth;
    pView->at18 = pPlayer->swayHeight;
    pView->at1c = pPlayer->swayWidth;
}

void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at50 += pPlayer->pSprite->x-oldpos->x;
    pView->at54 += pPlayer->pSprite->y-oldpos->y;
    pView->at38 += pPlayer->pSprite->z-oldpos->z;
}

void viewClearInterpolations(void)
{
    nInterpolations = 0;
    memset(gInterpolateSprite, 0, sizeof(gInterpolateSprite));
    memset(gInterpolateWall, 0, sizeof(gInterpolateWall));
    memset(gInterpolateSector, 0, sizeof(gInterpolateSector));
}

void viewAddInterpolation(void *data, INTERPOLATE_TYPE type)
{
    if (nInterpolations == kMaxInterpolations)
        ThrowError("Too many interpolations");
    INTERPOLATE *pInterpolate = &gInterpolation[nInterpolations++];
    pInterpolate->pointer = data;
    pInterpolate->type = type;
    switch (type)
    {
    case INTERPOLATE_TYPE_INT:
        pInterpolate->value = *((int*)data);
        break;
    case INTERPOLATE_TYPE_SHORT:
        pInterpolate->value = *((short*)data);
        break;
    }
}

void CalcInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
        {
            pInterpolate->value2 = *((int*)pInterpolate->pointer);
            int newValue = interpolate(pInterpolate->value, *((int*)pInterpolate->pointer), gInterpolate);
            *((int*)pInterpolate->pointer) = newValue;
            break;
        }
        case INTERPOLATE_TYPE_SHORT:
        {
            pInterpolate->value2 = *((short*)pInterpolate->pointer);
            int newValue = interpolate(pInterpolate->value, *((short*)pInterpolate->pointer), gInterpolate);
            *((short*)pInterpolate->pointer) = newValue;
            break;
        }
        }
    }
}

void RestoreInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
            *((int*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        case INTERPOLATE_TYPE_SHORT:
            *((short*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        }
    }
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
	return { gKillMgr.Kills, gKillMgr.TotalKills, gSecretMgr.Founds, gSecretMgr.Total, gLevelTime / kTicsPerSec, gPlayer[myconnectindex].fragCount };
}

void viewDrawMapTitle(void)
{
    if (!hud_showmapname || M_Active())
        return;

    int const fadeStartTic = kTicsPerSec;
    int const fadeEndTic = int(1.5f*kTicsPerSec);
    if (gLevelTime > fadeEndTic)
        return;
    int const alpha = 255 - clamp((gLevelTime-fadeStartTic)*255/(fadeEndTic-fadeStartTic), 0, 255);

    if (alpha != 0)
    {
        viewDrawText(1, currentLevel->DisplayName(), 160, 50, -128, 0, 1, 1, 0, alpha);
    }
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
            char nPlayer = pSprite->type-kDudePlayer1;
            char* szName = gProfile[nPlayer].name;
            int nPalette = (gPlayer[nPlayer].teamId&3)+11;
            viewDrawText(4, szName, 160, 125, -128, nPalette, 1, 1);
        }
    }
}

static TArray<uint8_t> lensdata;
int *lensTable;

int gZoom = 1024;

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
    dassert(lensdata.Size() == kLensSize * kLensSize * sizeof(int));

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
    gViewMap.sub_25C38(0, 0, gZoom, 0);
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
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
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
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
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
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
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
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
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


char errMsg[256];

void viewSetErrorMessage(const char *pMessage)
{
    if (!pMessage)
    {
        strcpy(errMsg, "");
    }
    else
    {
        strcpy(errMsg, pMessage);
    }
}

void DoLensEffect(void)
{
	// To investigate whether this can be implemented as a shader effect.
    auto d = tileData(4077);
    dassert(d != NULL);
	auto s = tilePtr(4079);
    dassert(s != NULL);
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

char otherMirrorGotpic[2];
char bakMirrorGotpic[2];
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

void viewDrawScreen(bool sceneonly)
{
    int nPalette = 0;
    int defaultHoriz = r_horizcenter ? 100 : 90;

#ifdef USE_OPENGL
    polymostcenterhoriz = defaultHoriz;
#endif
    if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
    {
        gInterpolate = I_GetTimeFrac() * MaxSmoothRatio;
    }
    else gInterpolate = MaxSmoothRatio;

    if (cl_interpolate)
    {
        CalcInterpolations();
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

        int v1 = Blrintf(double(viewingrange) * tan(r_fov * (PI / 360.)));

        renderSetAspect(v1, yxaspect);
        int cX = gView->pSprite->x;
        int cY = gView->pSprite->y;
        int cZ = gView->zView;
        double zDelta = gView->zWeapon - gView->zView - (12 << 8);
        fixed_t cA;
        fixed_t q16horiz;
        fixed_t q16slopehoriz = gView->q16slopehoriz;
        int v74 = gView->bobWidth;
        int v8c = gView->bobHeight;
        double v4c = gView->swayWidth;
        double v48 = gView->swayHeight;
        int nSectnum = gView->pSprite->sectnum;
        if (cl_interpolate)
        {
            if (numplayers > 1 && gView == gMe && gPrediction && gMe->pXSprite->health > 0)
            {
                nSectnum = predict.at68;
                cX = interpolate(predictOld.at50, predict.at50, gInterpolate);
                cY = interpolate(predictOld.at54, predict.at54, gInterpolate);
                cZ = interpolate(predictOld.at38, predict.at38, gInterpolate);
                zDelta = finterpolate(predictOld.at34, predict.at34, gInterpolate);
                cA = interpolateangfix16(predictOld.at30, predict.at30, gInterpolate);
                q16horiz = interpolate(predictOld.at24, predict.at24, gInterpolate);
                q16slopehoriz = interpolate(predictOld.at28, predict.at28, gInterpolate);
                v74 = interpolate(predictOld.atc, predict.atc, gInterpolate);
                v8c = interpolate(predictOld.at8, predict.at8, gInterpolate);
                v4c = finterpolate(predictOld.at1c, predict.at1c, gInterpolate);
                v48 = finterpolate(predictOld.at18, predict.at18, gInterpolate);
            }
            else
            {
                VIEW* pView = &gPrevView[gViewIndex];
                cX = interpolate(pView->at50, cX, gInterpolate);
                cY = interpolate(pView->at54, cY, gInterpolate);
                cZ = interpolate(pView->at38, cZ, gInterpolate);
                zDelta = finterpolate(pView->at34, zDelta, gInterpolate);
                cA = interpolateangfix16(pView->at30, gView->q16ang, gInterpolate);
                q16horiz = interpolate(pView->at24, gView->q16horiz, gInterpolate);
                q16slopehoriz = interpolate(pView->at28, q16slopehoriz, gInterpolate);
                v74 = interpolate(pView->atc, v74, gInterpolate);
                v8c = interpolate(pView->at8, v8c, gInterpolate);
                v4c = finterpolate(pView->at1c, v4c, gInterpolate);
                v48 = finterpolate(pView->at18, v48, gInterpolate);
            }
        }
        if (!cl_syncinput && gView == gMe && (numplayers <= 1 || gPrediction) && gView->pXSprite->health != 0 && !VanillaMode())
        {
            int upAngle = 289;
            int downAngle = -347;
            fixed_t q16look;
            cA = gViewAngle;
            q16look = gViewLook;
            q16horiz = FloatToFixed(100.f * tanf(FixedToFloat(q16look) * fPI / 1024.f));
        }
        viewUpdateShake();
        q16horiz += IntToFixed(shakeHoriz);
        cA += IntToFixed(shakeAngle);
        cX += shakeX;
        cY += shakeY;
        cZ += shakeZ;
        v4c += shakeBobX;
        v48 += shakeBobY;
        q16horiz += IntToFixed(mulscale30(0x40000000 - Cos(gView->tiltEffect << 2), 30));
        if (gViewPos == 0)
        {
            if (cl_viewbob)
            {
                if (cl_viewhbob)
                {
                    cX -= mulscale30(v74, Sin(FixedToInt(cA))) >> 4;
                    cY += mulscale30(v74, Cos(FixedToInt(cA))) >> 4;
                }
                if (cl_viewvbob)
                {
                    cZ += v8c;
                }
            }
            if (cl_slopetilting)
            {
                q16horiz += q16slopehoriz;
            }
            cZ += FixedToInt(q16horiz * 10);
            cameradist = -1;
            cameraclock = gFrameClock +mulscale16(4, (int)gInterpolate);
        }
        else
        {
            CalcPosition(gView->pSprite, (int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum, FixedToInt(cA), q16horiz, (int)gInterpolate);
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
        renderSetRollAngle(0);
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
            //othercameraclock = gGameClock;
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
            DrawMirrors(vd8, vd4, vd0, IntToFixed(v50), IntToFixed(v54 + defaultHoriz), gInterpolate, -1);
            drawrooms(vd8, vd4, vd0, v50, v54 + defaultHoriz, vcc);
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
        int nSprite = headspritestat[kStatExplosion];
        int unk = 0;
        while (nSprite >= 0)
        {
            spritetype* pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE* pXSprite = &xsprite[nXSprite];
            if (TestBitString(gotsector, pSprite->sectnum))
            {
                unk += pXSprite->data3 * 32;
            }
            nSprite = nextspritestat[nSprite];
        }
        nSprite = headspritestat[kStatProjectile];
        while (nSprite >= 0) {
            spritetype* pSprite = &sprite[nSprite];
            switch (pSprite->type) {
            case kMissileFlareRegular:
            case kMissileTeslaAlt:
            case kMissileFlareAlt:
            case kMissileTeslaRegular:
                if (TestBitString(gotsector, pSprite->sectnum)) unk += 256;
                break;
            }
            nSprite = nextspritestat[nSprite];
        }
        g_visibility = (int32_t)(ClipLow(gVisibility - 32 * gView->visibility - unk, 0));
        cA = (cA + interpolateangfix16(IntToFixed(deliriumTurnO), IntToFixed(deliriumTurn), gInterpolate)) & 0x7ffffff;
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
        q16horiz = ClipRange(q16horiz, IntToFixed(-200), IntToFixed(200));
    RORHACK:
        int ror_status[16];
        for (int i = 0; i < 16; i++)
            ror_status[i] = TestBitString(gotpic, 4080 + i);
        fixed_t deliriumPitchI = interpolate(IntToFixed(deliriumPitchO), IntToFixed(deliriumPitch), gInterpolate);
        DrawMirrors(cX, cY, cZ, cA, q16horiz + IntToFixed(defaultHoriz) + deliriumPitchI, gInterpolate, gViewIndex);
        int bakCstat = gView->pSprite->cstat;
        if (gViewPos == 0)
        {
            gView->pSprite->cstat |= 32768;
        }
        else
        {
            gView->pSprite->cstat |= 514;
        }

        renderDrawRoomsQ16(cX, cY, cZ, cA, q16horiz + IntToFixed(defaultHoriz) + deliriumPitchI, nSectnum);
        viewProcessSprites(cX, cY, cZ, FixedToInt(cA), gInterpolate);
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
            if (videoGetRenderMode() == REND_POLYMOST && gDeliriumBlur)
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
            gWeather.Draw(cX, cY, cZ, cA, q16horiz + defaultHoriz + deliriumPitch, gWeather.at12d8);
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
        hudDraw(gView, nSectnum, defaultHoriz, v4c, v48, zDelta, basepal, (int)gInterpolate);
    }
    UpdateDacs(0, true);    // keep the view palette active only for the actual 3D view and its overlays.
    if (automapMode != am_off)
    {
        gViewMap.sub_25DB0(gView->pSprite);
    }
    UpdateStatusBar();
    int zn = ((gView->zWeapon-gView->zView-(12<<8))>>7)+220;
    PLAYER *pPSprite = &gPlayer[gMe->pSprite->type-kDudePlayer1];
    if (IsPlayerSprite(gMe->pSprite) && pPSprite->hand == 1)
    {
        //static int lastClock;
        gChoke.animateChoke(160, zn, (int)gInterpolate);
        //if ((gGameClock % 5) == 0 && gGameClock != lastClock)
        //{
        //    gChoke.swayV(pPSprite);
        //}
        //lastClock = gGameClock;
    }
#if 0
    if (byte_1A76C6)
    {
        DrawStatSprite(2048, xdim-15, 20);
    }
#endif

    viewDrawMapTitle();
    viewDrawAimedPlayerName();
    if (paused)
    {
        viewDrawText(1, GStrings("TXTB_PAUSED"), 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        FStringf gTempStr("] %s [", gProfile[gView->nPlayer].name);
        viewDrawText(0, gTempStr, 160, 10, 0, 0, 1, 0);
    }
    if (errMsg[0])
    {
        viewDrawText(0, errMsg, 160, 20, 0, 0, 1, 0);
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


#define LOW_FPS 60
#define SLOW_FRAME_TIME 20

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

FString GameInterface::GetCoordString()
{
    return "Player pos is unknown"; // todo: output at least something useful.
}


class ViewLoadSave : public LoadSave {
public:
    void Load(void);
    void Save(void);
};

static ViewLoadSave *myLoadSave;

static int messageTime;
static char message[256];

void ViewLoadSave::Load(void)
{
    Read(&messageTime, sizeof(messageTime));
    Read(message, sizeof(message));
    Read(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Read(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Read(&gScreenTilt, sizeof(gScreenTilt));
    Read(&deliriumTilt, sizeof(deliriumTilt));
    Read(&deliriumTurn, sizeof(deliriumTurn));
    Read(&deliriumPitch, sizeof(deliriumPitch));
}

void ViewLoadSave::Save(void)
{
    Write(&messageTime, sizeof(messageTime));
    Write(message, sizeof(message));
    Write(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Write(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Write(&gScreenTilt, sizeof(gScreenTilt));
    Write(&deliriumTilt, sizeof(deliriumTilt));
    Write(&deliriumTurn, sizeof(deliriumTurn));
    Write(&deliriumPitch, sizeof(deliriumPitch));
}

void ViewLoadSaveConstruct(void)
{
    myLoadSave = new ViewLoadSave();
}

END_BLD_NS
