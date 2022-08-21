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


BEGIN_BLD_NS

VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

double gInterpolate;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewBackupView(int nPlayer)
{
	PLAYER* pPlayer = &gPlayer[nPlayer];
	VIEW* pView = &gPrevView[nPlayer];
	pView->angle = pPlayer->angle.ang;
	pView->x = pPlayer->actor->int_pos().X;
	pView->y = pPlayer->actor->int_pos().Y;
	pView->viewz = pPlayer->zView;
	pView->weaponZ = pPlayer->zWeapon - pPlayer->zView - 0xc00;
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewCorrectViewOffsets(int nPlayer, vec3_t const* oldpos)
{
	PLAYER* pPlayer = &gPlayer[nPlayer];
	VIEW* pView = &gPrevView[nPlayer];
	pView->x += pPlayer->actor->int_pos().X - oldpos->X;
	pView->y += pPlayer->actor->int_pos().Y - oldpos->Y;
	pView->viewz += pPlayer->actor->int_pos().Z - oldpos->Z;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewDrawText(FFont* pFont, const char* pString, int x, int y, int nShade, int nPalette, int position, bool shadow)
{
	if (!pString) return;

	//y += pFont->yoff;

	if (position == 1) x -= pFont->StringWidth(pString) / 2;
	if (position == 2) x -= pFont->StringWidth(pString);


	if (shadow)
	{
		DrawText(twod, pFont, CR_UNTRANSLATED, x + 1, y + 1, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, 0.5, TAG_DONE);
	}
	DrawText(twod, pFont, CR_NATIVEPAL, x, y, pString, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, nPalette),
		DTA_Color, shadeToLight(nShade), TAG_DONE);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

GameStats GameInterface::getStats()
{
	return { gKillMgr.Kills, gKillMgr.TotalKills, gSecretMgr.Founds, gSecretMgr.Total, gFrameCount / kTicsPerSec, gPlayer[myconnectindex].fragCount };
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewDrawAimedPlayerName(void)
{
	if (!cl_idplayers || (gView->aim.dx == 0 && gView->aim.dy == 0))
		return;

	int hit = HitScan(gView->actor, gView->zView, gView->aim.dx, gView->aim.dy, gView->aim.dz, CLIPMASK0, 512);
	if (hit == 3)
	{
		auto actor = gHitInfo.actor();
		if (actor && actor->IsPlayerActor())
		{
			int nPlayer = actor->spr.type - kDudePlayer1;
			const char* szName = PlayerName(nPlayer);
			int nPalette = (gPlayer[nPlayer].teamId & 3) + 11;
			viewDrawText(DigiFont, szName, 160, 125, -128, nPalette, 1, 1);
		}
	}
}

static TArray<uint8_t> lensdata;
int* lensTable;

extern int dword_172CE0[16][3];

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewInit(void)
{
	Printf("Initializing status bar\n");

	lensdata = fileSystem.LoadFile("lens.dat");
	assert(lensdata.Size() == kLensSize * kLensSize * sizeof(int));

	lensTable = (int*)lensdata.Data();
#if WORDS_BIGENDIAN
	for (int i = 0; i < kLensSize * kLensSize; i++)
	{
		lensTable[i] = LittleLong(lensTable[i]);
	}
#endif
	uint8_t* data = TileFiles.tileCreate(4077, kLensSize, kLensSize);
	memset(data, TRANSPARENT_INDEX, kLensSize * kLensSize);

	for (int i = 0; i < 16; i++)
	{
		dword_172CE0[i][0] = MulScale(wrand(), 2048, 16);
		dword_172CE0[i][1] = MulScale(wrand(), 2048, 16);
		dword_172CE0[i][2] = MulScale(wrand(), 2048, 16);
	}
}

int othercameradist = 1280;
int othercameraclock;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

#if 0
void CalcOtherPosition(DBloodActor* actor, int* pX, int* pY, int* pZ, sectortype** vsectnum, int nAng, fixed_t zm, int smoothratio) // currently unused
{
	int vX = MulScale(-Cos(nAng), 1280, 30);
	int vY = MulScale(-Sin(nAng), 1280, 30);
	int vZ = FixedToInt(MulScale(zm, 1280, 3)) - (16 << 8);
	int bakCstat = pSprite->cstat;
	pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
	assert(validSectorIndex(*vsectnum));
	FindSector(*pX, *pY, *pZ, vsectnum);
	int nHSector;
	int hX, hY;
	vec3_t pos = { *pX, *pY, *pZ };
	hitscan(&pos, *vsectnum, vX, vY, vZ, &hitdata, CLIPMASK1);
	nHSector = hitdata.sect;
	hX = hitdata.pos.x;
	hY = hitdata.pos.y;
	int dX = hX - *pX;
	int dY = hY - *pY;
	if (abs(vX) + abs(vY) > abs(dX) + abs(dY))
	{
		*vsectnum = nHSector;
		dX -= Sgn(vX) << 6;
		dY -= Sgn(vY) << 6;
		int nDist;
		if (abs(vX) > abs(vY))
		{
			nDist = ClipHigh(DivScale(dX, vX, 16), othercameradist);
		}
		else
		{
			nDist = ClipHigh(DivScale(dY, vY, 16), othercameradist);
		}
		othercameradist = nDist;
	}
	*pX += MulScale(vX, othercameradist, 16);
	*pY += MulScale(vY, othercameradist, 16);
	*pZ += MulScale(vZ, othercameradist, 16);
	int myclock = PlayClock + MulScale(4, smoothratio, 16);
	othercameradist = ClipHigh(othercameradist + ((myclock - othercameraclock) << 10), 65536);
	othercameraclock = myclock;
	assert(validSectorIndex(*vsectnum));
	FindSector(*pX, *pY, *pZ, vsectnum);
	pSprite->cstat = bakCstat;
}
#endif

//---------------------------------------------------------------------------
//
// by NoOne: show warning msgs in game instead of throwing errors (in some cases)
//
//---------------------------------------------------------------------------

void viewSetSystemMessage(const char* pMessage, ...) {
	char buffer[1024]; va_list args; va_start(args, pMessage);
	vsprintf(buffer, pMessage, args);

	Printf(PRINT_HIGH | PRINT_NOTIFY, "%s\n", buffer); // print it also in console
}

void viewSetMessage(const char* pMessage, const char* color, const MESSAGE_PRIORITY priority)
{
	int printlevel = priority <= MESSAGE_PRIORITY_NORMAL ? PRINT_LOW : priority < MESSAGE_PRIORITY_SYSTEM ? PRINT_MEDIUM : PRINT_HIGH;
	Printf(printlevel | PRINT_NOTIFY, "%s%s\n", color? color : TEXTCOLOR_TAN, pMessage);
}

void viewSetErrorMessage(const char* pMessage)
{
	Printf(PRINT_BOLD | PRINT_NOTIFY, "%s\n", pMessage);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoLensEffect(void)
{
	// To investigate whether this can be implemented as a shader effect.
	auto d = tileData(4077);
	assert(d != NULL);
	auto s = tilePtr(4079);
	assert(s != NULL);
	for (int i = 0; i < kLensSize * kLensSize; i++, d++)
		if (lensTable[i] >= 0)
			*d = s[lensTable[i]];
	TileFiles.InvalidateTile(4077);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

int deliriumTilt, deliriumPitch;
int deliriumPitchO;
DAngle deliriumTurnO, deliriumTurn;
DAngle gScreenTiltO, gScreenTilt;

int gShowFrameRate = 1;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void viewUpdateDelirium(void)
{
	gScreenTiltO = gScreenTilt;
	deliriumTurnO = deliriumTurn;
	deliriumPitchO = deliriumPitch;
	int powerCount;
	if ((powerCount = powerupCheck(gView, kPwUpDeliriumShroom)) != 0)
	{
		int tilt1 = 170, tilt2 = 170, pitch = 20;
		int timer = PlayClock * 2;
		if (powerCount < 512)
		{
			int powerScale = IntToFixed(powerCount) / 512;
			tilt1 = MulScale(tilt1, powerScale, 16);
			tilt2 = MulScale(tilt2, powerScale, 16);
			pitch = MulScale(pitch, powerScale, 16);
		}
		int sin2 = Sin(2 * timer) >> 1;
		int sin3 = Sin(3 * timer) >> 1;
		gScreenTilt = DAngle::fromBuild(MulScale(sin2 + sin3, tilt1, 30));
		int sin4 = Sin(4 * timer) >> 1;
		deliriumTurn = DAngle::fromBuild(MulScale(sin3 + sin4, tilt2, 30));
		int sin5 = Sin(5 * timer) >> 1;
		deliriumPitch = MulScale(sin4 + sin5, pitch, 30);
		return;
	}
	gScreenTilt = gScreenTilt.Normalized180();
	if (gScreenTilt > nullAngle)
	{
		gScreenTilt -= DAngle::fromBuild(8);
		if (gScreenTilt < nullAngle)
			gScreenTilt = nullAngle;
	}
	else if (gScreenTilt < nullAngle)
	{
		gScreenTilt += DAngle::fromBuild(8);
		if (gScreenTilt >= nullAngle)
			gScreenTilt = nullAngle;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void viewUpdateShake(int& cX, int& cY, int& cZ, DAngle& cA, fixedhoriz& cH, double& pshakeX, double& pshakeY)
{
	auto doEffect = [&](const int& effectType)
	{
		if (effectType)
		{
			int nValue = ClipHigh(effectType * 8, 2000);
			cH += buildhoriz(QRandom2(nValue >> 8));
			cA += DAngle::fromBuild(QRandom2(nValue >> 8));
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DrawMap(DBloodActor* view)
{
	int tm = 0;
	if (viewport3d.Left() > 0)
	{
		setViewport(Hud_Stbar);
		tm = 1;
	}
	VIEW* pView = &gPrevView[gViewIndex];
	int x = interpolatedvalue(pView->x, view->int_pos().X, gInterpolate);
	int y = interpolatedvalue(pView->y, view->int_pos().Y, gInterpolate);
	int ang = (!SyncInput() ? gView->angle.sum() : gView->angle.interpolatedsum(gInterpolate)).Buildang();
	DrawOverheadMap(x, y, ang, gInterpolate);
	if (tm)
		setViewport(hud_size);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetupView(int& cX, int& cY, int& cZ, DAngle& cA, fixedhoriz& cH, sectortype*& pSector, double& zDelta, double& shakeX, double& shakeY, DAngle& rotscrnang)
{
	int bobWidth, bobHeight;

	pSector = gView->actor->sector();
#if 0
	if (numplayers > 1 && gView == gMe && gPrediction && gMe->actor->xspr.health > 0)
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
#endif
	{
		VIEW* pView = &gPrevView[gViewIndex];
		cX = interpolatedvalue(pView->x, gView->actor->int_pos().X, gInterpolate);
		cY = interpolatedvalue(pView->y, gView->actor->int_pos().Y, gInterpolate);
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
			cX -= MulScale(bobWidth, Sin(cA.Buildang()), 30) >> 4;
			cY += MulScale(bobWidth, Cos(cA.Buildang()), 30) >> 4;
		}
		if (cl_viewvbob)
		{
			cZ += bobHeight;
		}
		cZ += int(cH.asq16() * (1. / 6553.6));
		cameradist = -1;
		cameraclock = PlayClock + MulScale(4, (int)gInterpolate, 16);
	}
	else
	{
		calcChaseCamPos((int*)&cX, (int*)&cY, (int*)&cZ, gView->actor, &pSector, cA, cH, gInterpolate);
	}
	if (pSector != nullptr)
		CheckLink((int*)&cX, (int*)&cY, (int*)&cZ, &pSector);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
	int vd8 = pOther->actor->spr.x;
	int vd4 = pOther->actor->spr.y;
	int vd0 = pOther->zView;
	int vcc = pOther->actor->spr.sectnum;
	int v50 = pOther->actor->spr.angle;
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
	CalcOtherPosition(pOther->actor, &vd8, &vd4, &vd0, &vcc, v50, 0, (int)gInterpolate);
	CheckLink(&vd8, &vd4, &vd0, &vcc);
	uint8_t v14 = 0;
	if (IsUnderwaterSector(vcc))
	{
		v14 = 10;
	}
	drawrooms(vd8, vd4, vd0, v50, v54, vcc);
	viewProcessSprites(vd8, vd4, vd0, v50, gInterpolate);
	renderDrawMasks();
#endif
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void viewDrawScreen(bool sceneonly)
{
	if (testgotpic(2342, true))
	{
		FireProcess();
	}

	if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
	{
		gInterpolate = !cl_interpolate || cl_capfps ? MaxSmoothRatio : I_GetTimeFrac() * MaxSmoothRatio;
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
				if (gView->actor->xspr.medium == kMediumWater) basepal = 1;
				else if (gView->actor->xspr.medium == kMediumGoo) basepal = 3;
				else basepal = 2;
			}
		}
		UpdateDacs(basepal);
		UpdateBlend();

		int cX, cY, cZ;
		DAngle cA, rotscrnang;
		fixedhoriz cH;
		sectortype* pSector;
		double zDelta;
		double shakeX, shakeY;
		SetupView(cX, cY, cZ, cA, cH, pSector, zDelta, shakeX, shakeY, rotscrnang);

		DAngle tilt = interpolatedangle(gScreenTiltO, gScreenTilt, gInterpolate);
		bool bDelirium = powerupCheck(gView, kPwUpDeliriumShroom) > 0;
		static bool bDeliriumOld = false;
		//int tiltcs, tiltdim;
		uint8_t otherview = powerupCheck(gView, kPwUpCrystalBall) > 0;
		if (tilt.Degrees() || bDelirium)
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
			deliriumTurn = DAngle::fromDeg(0.);
			deliriumPitch = 0;
		}
		int brightness = 0;

		BloodStatIterator it(kStatExplosion);
		while (auto actor = it.Next())
		{
			if (actor->hasX() && gotsector[actor->sectno()])
			{
				brightness += actor->xspr.data3 * 32;
			}
		}
		it.Reset(kStatProjectile);
		while (auto actor = it.Next())
		{
			switch (actor->spr.type) {
			case kMissileFlareRegular:
			case kMissileTeslaAlt:
			case kMissileFlareAlt:
			case kMissileTeslaRegular:
				if (gotsector[actor->sectno()]) brightness += 256;
				break;
			}
		}
		g_relvisibility = (int32_t)(ClipLow(gVisibility - 32 * gView->visibility - brightness, 0)) - g_visibility;
		cA += interpolatedangle(deliriumTurnO, deliriumTurn, gInterpolate);

		if (pSector != nullptr)
		{
			int ceilingZ, floorZ;
			getzsofslopeptr(pSector, cX, cY, &ceilingZ, &floorZ);
			if ((cZ > floorZ - (1 << 8)) && (pSector->upperLink == nullptr)) // clamp to floor
			{
				cZ = floorZ - (1 << 8);
			}
			if ((cZ < ceilingZ + (1 << 8)) && (pSector->lowerLink == nullptr)) // clamp to ceiling
			{
				cZ = ceilingZ + (1 << 8);
			}
		}

		cH = q16horiz(ClipRange(cH.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));

		if ((tilt.Degrees() || bDelirium) && !sceneonly)
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

		if (!sceneonly) hudDraw(gView, pSector, shakeX, shakeY, zDelta, basepal, gInterpolate);
		fixedhoriz deliriumPitchI = q16horiz(interpolatedvalue(IntToFixed(deliriumPitchO), IntToFixed(deliriumPitch), gInterpolate));
		auto bakCstat = gView->actor->spr.cstat;
		gView->actor->spr.cstat |= (gViewPos == 0) ? CSTAT_SPRITE_INVISIBLE : CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
		render_drawrooms(gView->actor, { cX, cY, cZ }, sectnum(pSector), cA, cH + deliriumPitchI, rotscrnang, gInterpolate);
		gView->actor->spr.cstat = bakCstat;
		bDeliriumOld = bDelirium && gDeliriumBlur;

		int nClipDist = gView->actor->spr.clipdist << 2;
		int vec, vf4;
		Collision c1, c2;
		GetZRange(gView->actor, &vf4, &c1, &vec, &c2, nClipDist, 0);
		if (sceneonly) return;
		double look_anghalf = gView->angle.look_anghalf(gInterpolate);
		DrawCrosshair(kCrosshairTile, gView->actor->xspr.health >> 4, -look_anghalf, 0, 2);
#if 0 // This currently does not work. May have to be redone as a hardware effect.
		if (v4 && gNetPlayers > 1)
		{
			DoLensEffect();
			r otatesprite(IntToFixed(280), IntToFixed(35), 53248, 512, 4077, v10, v14, 512 + 6, gViewX0, gViewY0, gViewX1, gViewY1);
			r otatesprite(IntToFixed(280), IntToFixed(35), 53248, 0, 1683, v10, 0, 512 + 35, gViewX0, gViewY0, gViewX1, gViewY1);
		}
#endif

#if 0
		int tmpSect = nSectnum;
		if ((vf0 & 0xc000) == 0x4000)
		{
			tmpSect = vf0 & (kMaxWalls - 1);
		}
		int v8 = byte_1CE5C2 > 0 && (sector[tmpSect].ceilingstat & CSTAT_SECTOR_SKY);
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
	}
	UpdateDacs(0, true);    // keep the view palette active only for the actual 3D view and its overlays.
	if (automapMode != am_off)
	{
		DrawMap(gView->actor);
	}
	UpdateStatusBar();

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::GenerateSavePic()
{
	viewDrawScreen(true);
	return true;
}

std::pair<DVector3, DAngle> GameInterface::GetCoordinates()
{
	if (!gMe || !gMe->actor) return std::make_pair(DVector3(DBL_MAX, 0, 0), nullAngle);
	return std::make_pair(gMe->actor->spr.pos, gMe->actor->spr.angle);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::DrawAutomapPlayer(int mx, int my, int x, int y, int z, int a, double const smoothratio)
{
	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		PLAYER* pPlayer = &gPlayer[i];
		auto actor = pPlayer->actor;

		int xvect = -bsin(a) * z;
		int yvect = -bcos(a) * z;
		int ox = mx - x;
		int oy = my - y;
		int x1 = DMulScale(ox, xvect, -oy, yvect, 16);
		int y1 = DMulScale(oy, xvect, ox, yvect, 16);
		int xx = twod->GetWidth() / 2. + x1 / 4096.;
		int yy = twod->GetHeight() / 2. + y1 / 4096.;

		if (i == gView->nPlayer || gGameOptions.nGameType == 1)
		{
			int nTile = actor->spr.picnum;
			int ceilZ, floorZ;
			Collision ceilHit, floorHit;
			GetZRange(gView->actor, &ceilZ, &ceilHit, &floorZ, &floorHit, (actor->spr.clipdist << 2) + 16, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
			int nTop, nBottom;
			GetActorExtents(actor, &nTop, &nBottom);
			int nScale = (actor->spr.yrepeat + ((floorZ - nBottom) >> 8)) * z;
			nScale = ClipRange(nScale, 8000, 65536 << 1);
			// Players on automap
			double xsize = twod->GetWidth() / 2. + x1 / double(1 << 12);
			double ysize = twod->GetHeight() / 2. + y1 / double(1 << 12);
			// This very likely needs fixing later
			DrawTexture(twod, tileGetTexture(nTile, true), xx, yy, DTA_ClipLeft, viewport3d.Left(), DTA_ClipTop, viewport3d.Top(), DTA_ScaleX, z / 1536., DTA_ScaleY, z / 1536., DTA_CenterOffset, true,
				DTA_ClipRight, viewport3d.Right(), DTA_ClipBottom, viewport3d.Bottom(), DTA_Alpha, (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT ? 0.5 : 1.), TAG_DONE);
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
