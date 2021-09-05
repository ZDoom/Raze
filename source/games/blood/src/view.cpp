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
#include "gamefuncs.h"
#include "v_draw.h"
#include "precache.h"
#include "render.h"
#include "razefont.h"


EXTERN_CVAR(Bool, testnewrenderer)
BEGIN_BLD_NS

FixedBitArray<kMaxSprites> gInterpolateSprite;
VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

double gInterpolate;

int gScreenTilt;

void viewBackupView(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->angle = pPlayer->angle.ang;
    pView->x = pPlayer->pSprite->x;
    pView->y = pPlayer->pSprite->y;
    pView->viewz = pPlayer->zView;
    pView->weaponZ = pPlayer->zWeapon-pPlayer->zView-0xc00;
    pView->horiz = pPlayer->horizon.horiz;
    pView->horizoff = pPlayer->horizon.horizoff;
    pView->at2c = pPlayer->slope;
    pView->bobHeight = pPlayer->bobHeight;
    pView->bobWidth = pPlayer->bobWidth;
    pView->shakeBobY = pPlayer->swayHeight;
    pView->shakeBobX = pPlayer->swayWidth;
    pView->look_ang = pPlayer->angle.look_ang;
    pView->rotscrnang = pPlayer->angle.rotscrnang;
    pPlayer->angle.backup();
    pPlayer->horizon.backup();
}

void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->x += pPlayer->pSprite->x-oldpos->x;
    pView->y += pPlayer->pSprite->y-oldpos->y;
    pView->viewz += pPlayer->pSprite->z-oldpos->z;
}

void viewDrawText(FFont* pFont, const char *pString, int x, int y, int nShade, int nPalette, int position, char shadow)
{
    if (!pString) return;

    //y += pFont->yoff;

	if (position == 1) x -= pFont->StringWidth(pString) / 2;
    if (position == 2) x -= pFont->StringWidth(pString);


	if (shadow)
	{
		DrawText(twod, pFont, CR_UNTRANSLATED, x+1, y+1, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, 0.5, TAG_DONE);
	}
	DrawText(twod, pFont, CR_NATIVEPAL, x, y, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, nPalette),
			 DTA_Color, shadeToLight(nShade), TAG_DONE);

}

GameStats GameInterface::getStats()
{
	return { gKillMgr.Kills, gKillMgr.TotalKills, gSecretMgr.Founds, gSecretMgr.Total, gFrameCount / kTicsPerSec, gPlayer[myconnectindex].fragCount };
}

void viewDrawAimedPlayerName(void)
{
    if (!cl_idplayers || (gView->aim.dx == 0 && gView->aim.dy == 0))
        return;

    int hit = HitScan(gView->actor(), gView->zView, gView->aim.dx, gView->aim.dy, gView->aim.dz, CLIPMASK0, 512);
    if (hit == 3)
    {
        if (gHitInfo.hitactor && gHitInfo.hitactor->IsPlayerActor())
        {
            spritetype* pSprite = &gHitInfo.hitactor->s();
            int nPlayer = pSprite->type-kDudePlayer1;
            const char* szName = PlayerName(nPlayer);
            int nPalette = (gPlayer[nPlayer].teamId&3)+11;
            viewDrawText(DigiFont, szName, 160, 125, -128, nPalette, 1, 1);
        }
    }
}

static TArray<uint8_t> lensdata;
int *lensTable;

extern int dword_172CE0[16][3];

void viewInit(void)
{
    Printf("Initializing status bar\n");

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
        dword_172CE0[i][0] = MulScale(wrand(), 2048, 16);
        dword_172CE0[i][1] = MulScale(wrand(), 2048, 16);
        dword_172CE0[i][2] = MulScale(wrand(), 2048, 16);
    }
}

int othercameradist = 1280;
int othercameraclock;

void CalcOtherPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fixed_t zm, int smoothratio)
{
    int vX = MulScale(-Cos(nAng), 1280, 30);
    int vY = MulScale(-Sin(nAng), 1280, 30);
    int vZ = FixedToInt(MulScale(zm, 1280, 3))-(16<<8);
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
    if (abs(vX)+abs(vY) > abs(dX)+abs(dY))
    {
        *vsectnum = nHSector;
        dX -= Sgn(vX)<<6;
        dY -= Sgn(vY)<<6;
        int nDist;
        if (abs(vX) > abs(vY))
        {
            nDist = ClipHigh(DivScale(dX,vX, 16), othercameradist);
        }
        else
        {
            nDist = ClipHigh(DivScale(dY,vY, 16), othercameradist);
        }
        othercameradist = nDist;
    }
    *pX += MulScale(vX, othercameradist, 16);
    *pY += MulScale(vY, othercameradist, 16);
    *pZ += MulScale(vZ, othercameradist, 16);
	int myclock = PlayClock + MulScale(4, smoothratio, 16);
    othercameradist = ClipHigh(othercameradist+((myclock-othercameraclock)<<10), 65536);
    othercameraclock = myclock;
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
        int timer = PlayClock*4;
		if (powerCount < 512)
		{
			int powerScale = IntToFixed(powerCount) / 512;
			tilt1 = MulScale(tilt1, powerScale, 16);
			tilt2 = MulScale(tilt2, powerScale, 16);
			pitch = MulScale(pitch, powerScale, 16);
		}
		int sin2 = Sin(2*timer) >> 1;
		int sin3 = Sin(3*timer) >> 1;
		gScreenTilt = MulScale(sin2+sin3,tilt1, 30);
		int sin4 = Sin(4*timer) >> 1;
		deliriumTurn = MulScale(sin3+sin4,tilt2, 30);
		int sin5 = Sin(5*timer) >> 1;
		deliriumPitch = MulScale(sin4+sin5,pitch, 30);
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

void viewUpdateShake(int& cX, int& cY, int& cZ, binangle& cA, fixedhoriz& cH, double& pshakeX, double& pshakeY)
{
    auto doEffect = [&](const int& effectType)
    {
        if (effectType)
        {
            int nValue = ClipHigh(effectType * 8, 2000);
            cH += buildhoriz(QRandom2(nValue >> 8));
            cA += buildang(QRandom2(nValue >> 8));
            cX += QRandom2(nValue >> 4);
            cY += QRandom2(nValue >> 4);
            cZ += QRandom2(nValue);
            pshakeX += QRandom2(nValue);
            pshakeY += QRandom2(nValue);
        }
    };
    doEffect(gView->flickerEffect);
    doEffect(gView->quakeEffect);
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
    int x = interpolatedvalue(pView->x, pSprite->x, gInterpolate);
    int y = interpolatedvalue(pView->y, pSprite->y, gInterpolate);
    int ang = (!SyncInput() ? gView->angle.sum() : gView->angle.interpolatedsum(gInterpolate)).asbuild();
    DrawOverheadMap(x, y, ang, gInterpolate);
    if (tm)
        setViewport(hud_size);
}

void SetupView(int &cX, int& cY, int& cZ, binangle& cA, fixedhoriz& cH, int& nSectnum, double& zDelta, double& shakeX, double& shakeY, binangle& rotscrnang)
{
    int bobWidth, bobHeight;
    
    nSectnum = gView->pSprite->sectnum;
    if (numplayers > 1 && gView == gMe && gPrediction && gMe->pXSprite->health > 0)
    {
        nSectnum = predict.sectnum;
        cX = interpolatedvalue(predictOld.x, predict.x, gInterpolate);
        cY = interpolatedvalue(predictOld.y, predict.y, gInterpolate);
        cZ = interpolatedvalue(predictOld.viewz, predict.viewz, gInterpolate);
        zDelta = interpolatedvaluef(predictOld.weaponZ, predict.weaponZ, gInterpolate);
        bobWidth = interpolatedvalue(predictOld.bobWidth, predict.bobWidth, gInterpolate);
        bobHeight = interpolatedvalue(predictOld.bobHeight, predict.bobHeight, gInterpolate);
        shakeX = interpolatedvaluef(predictOld.shakeBobX, predict.shakeBobX, gInterpolate);
        shakeY = interpolatedvaluef(predictOld.shakeBobY, predict.shakeBobY, gInterpolate);

        if (!SyncInput())
        {
            cA = bamang(predict.angle.asbam() + predict.look_ang.asbam());
            cH = predict.horiz + predict.horizoff;
            rotscrnang = predict.rotscrnang;
        }
        else
        {
            cA = interpolatedangle(predictOld.angle + predictOld.look_ang, predict.angle + predict.look_ang, gInterpolate);
            cH = interpolatedhorizon(predictOld.horiz + predictOld.horizoff, predict.horiz + predict.horizoff, gInterpolate);
            rotscrnang = interpolatedangle(predictOld.rotscrnang, predict.rotscrnang, gInterpolate);
        }
    }
    else
    {
        VIEW* pView = &gPrevView[gViewIndex];
        cX = interpolatedvalue(pView->x, gView->pSprite->x, gInterpolate);
        cY = interpolatedvalue(pView->y, gView->pSprite->y, gInterpolate);
        cZ = interpolatedvalue(pView->viewz, gView->zView, gInterpolate);
        zDelta = interpolatedvaluef(pView->weaponZ, gView->zWeapon - gView->zView - (12 << 8), gInterpolate);
        bobWidth = interpolatedvalue(pView->bobWidth, gView->bobWidth, gInterpolate);
        bobHeight = interpolatedvalue(pView->bobHeight, gView->bobHeight, gInterpolate);
        shakeX = interpolatedvaluef(pView->shakeBobX, gView->swayWidth, gInterpolate);
        shakeY = interpolatedvaluef(pView->shakeBobY, gView->swayHeight, gInterpolate);

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

    viewUpdateShake(cX, cY, cZ, cA, cH, shakeX, shakeY);
    cH += buildhoriz(MulScale(0x40000000 - Cos(gView->tiltEffect << 2), 30, 30));
    if (gViewPos == 0)
    {
        if (cl_viewhbob)
        {
            cX -= MulScale(bobWidth, Sin(cA.asbuild()), 30) >> 4;
            cY += MulScale(bobWidth, Cos(cA.asbuild()), 30) >> 4;
        }
        if (cl_viewvbob)
        {
            cZ += bobHeight;
        }
        cZ += xs_CRoundToInt(cH.asq16() / 6553.6);
        cameradist = -1;
        cameraclock = PlayClock + MulScale(4, (int)gInterpolate, 16);
    }
    else
    {
        calcChaseCamPos((int*)&cX, (int*)&cY, (int*)&cZ, gView->pSprite, &nSectnum, cA, cH, gInterpolate);
    }
    CheckLink((int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum);
}

void renderCrystalBall()
{
#if 0
    int tmp = (PlayClock / 240) % (gNetPlayers - 1);
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
    //othercameraclock = PlayClock + MulScale(4, (int)gInterpolate, 16);;
    if (!tileData(4079))
    {
        TileFiles.tileCreate(4079, 128, 128);
    }
    //renderSetTarget(4079, 128, 128);
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
    uint8_t v14 = 0;
    if (IsUnderwaterSector(vcc))
    {
        v14 = 10;
    }
    drawrooms(vd8, vd4, vd0, v50, v54, vcc);
    viewProcessSprites(vd8, vd4, vd0, v50, gInterpolate);
    renderDrawMasks();
    renderRestoreTarget();
#endif
}

void render3DViewPolymost(int nSectnum, int cX, int cY, int cZ, binangle cA, fixedhoriz cH);

void viewDrawScreen(bool sceneonly)
{
    int nPalette = 0;
	
	if (testgotpic(2342, true))
	{
		FireProcess();
	}

    if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
    {
        gInterpolate = I_GetTimeFrac() * MaxSmoothRatio;
    }
    else gInterpolate = MaxSmoothRatio;
    pm_smoothratio = (int)gInterpolate;

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

        int cX, cY, cZ;
        binangle cA;
        fixedhoriz cH;
        int nSectnum;
        double zDelta;
        double shakeX, shakeY;
        binangle rotscrnang;
        SetupView(cX, cY, cZ, cA, cH, nSectnum, zDelta, shakeX, shakeY, rotscrnang);

        binangle tilt = interpolatedangle(buildang(gScreenTiltO), buildang(gScreenTilt), gInterpolate);
        uint8_t v14 = 0;
        uint8_t v10 = 0;
        bool bDelirium = powerupCheck(gView, kPwUpDeliriumShroom) > 0;
        static bool bDeliriumOld = false;
        //int tiltcs, tiltdim;
        uint8_t otherview = powerupCheck(gView, kPwUpCrystalBall) > 0;
        if (tilt.asbam() || bDelirium)
        {
            rotscrnang = tilt;
        }
        else if (otherview && gNetPlayers > 1)
        {
#if 0    
            renderCrystalBall();
#endif
        }
        else
        {
            othercameraclock = PlayClock + MulScale(4, (int)gInterpolate, 16);
        }

        if (!bDelirium)
        {
            deliriumTilt = 0;
            deliriumTurn = 0;
            deliriumPitch = 0;
        }
        int brightness = 0;

        BloodStatIterator it(kStatExplosion);
        while (auto actor = it.Next())
        {
            if (actor->hasX() && gotsector[actor->s().sectnum])
            {
                brightness += actor->x().data3 * 32;
            }
        }
        it.Reset(kStatProjectile);
        while (auto actor = it.Next())
        {
            spritetype* pSprite = &actor->s();
            switch (pSprite->type) {
            case kMissileFlareRegular:
            case kMissileTeslaAlt:
            case kMissileFlareAlt:
            case kMissileTeslaRegular:
                if (gotsector[pSprite->sectnum]) brightness += 256;
                break;
            }
        }
        g_visibility = (int32_t)(ClipLow(gVisibility - 32 * gView->visibility - brightness, 0));
        cA += interpolatedangle(buildang(deliriumTurnO), buildang(deliriumTurn), gInterpolate);

        int ceilingZ, floorZ;
        getzsofslope(nSectnum, cX, cY, &ceilingZ, &floorZ);
        if (cZ >= floorZ)
        {
            cZ = floorZ - (getUpperLink(nSectnum) ? 0 : (8 << 8));
        }
        if (cZ <= ceilingZ)
        {
            cZ = ceilingZ + (getLowerLink(nSectnum) ? 0 : (8 << 8));
        }
        cH = q16horiz(ClipRange(cH.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));

        if ((tilt.asbam() || bDelirium) && !sceneonly)
        {
            if (gDeliriumBlur)
            {
                // todo: Set up a blurring postprocessing shader.
                //const float fBlur = pow(1.f/3.f, 30.f/g_frameRate);
                //g lAccum(GL _MULT, fBlur);
                //g lAccum(GL _ACCUM, 1.f-fBlur);
                //g lAccum(GL _RETURN, 1.f);
            }
        }

        if (testnewrenderer)
        {
            fixedhoriz deliriumPitchI = q16horiz(interpolatedvalue(IntToFixed(deliriumPitchO), IntToFixed(deliriumPitch), gInterpolate));
            int bakCstat = gView->pSprite->cstat;
            gView->pSprite->cstat |= (gViewPos == 0) ? CSTAT_SPRITE_INVISIBLE : CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANSLUCENT_INVERT;
            render_drawrooms(gView->pSprite, { cX, cY, cZ }, nSectnum, cA, cH + deliriumPitchI, rotscrnang, gInterpolate);
            gView->pSprite->cstat = bakCstat;
        }
        else
        {
            renderSetRollAngle((float)rotscrnang.asbuildf());
            render3DViewPolymost(nSectnum, cX, cY, cZ, cA, cH);
        }
        bDeliriumOld = bDelirium && gDeliriumBlur;

        int nClipDist = gView->pSprite->clipdist << 2;
        int vec, vf4;
        Collision c1, c2;
        GetZRange(gView->actor(), &vf4, &c1, &vec, &c2, nClipDist, 0);
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
        hudDraw(gView, nSectnum, shakeX, shakeY, zDelta, basepal, gInterpolate);
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

    viewDrawAimedPlayerName();
    if (paused)
    {
        auto text = GStrings("TXTB_PAUSED");
        viewDrawText(PickBigFont(text), text, 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        FStringf gTempStr("] %s [", PlayerName(gView->nPlayer));
        viewDrawText(OriginalSmallFont, gTempStr, 160, 10, 0, 0, 1, 0);
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
        gMe->pSprite->x, gMe->pSprite->y, gMe->pSprite->z, gMe->pSprite->ang * BAngToDegree);

    return out;
}


bool GameInterface::DrawAutomapPlayer(int x, int y, int z, int a, double const smoothratio)
{
    // [MR]: Confirm that this is correct as math doesn't match the variable names.
    int nCos = z * -bsin(a);
    int nSin = z * -bcos(a);

    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        PLAYER* pPlayer = &gPlayer[i];
        spritetype* pSprite = pPlayer->pSprite;
        int x1 = pSprite->x - x;
        int y1 = pSprite->y - y;
        int pa = (pSprite->ang - a) & 2047;
        if (i == gView->nPlayer || gGameOptions.nGameType == 1)
        {
            int nTile = pSprite->picnum;
            int ceilZ, floorZ;
            Collision ceilHit, floorHit;
            GetZRange(gView->actor(), &ceilZ, &ceilHit, &floorZ, &floorHit, (pSprite->clipdist << 2) + 16, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
            int nTop, nBottom;
            GetSpriteExtents(pSprite, &nTop, &nBottom);
            int nScale = (pSprite->yrepeat + ((floorZ - nBottom) >> 8)) * z;
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
