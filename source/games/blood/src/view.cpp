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

VIEWPOS gViewPos;
int gViewIndex;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewBackupView(int nPlayer)
{
	PLAYER* pPlayer = &gPlayer[nPlayer];
	pPlayer->ozView = pPlayer->zView;
	pPlayer->ozWeapon = pPlayer->zWeapon - pPlayer->zView - 12;
	pPlayer->obobHeight = pPlayer->bobHeight;
	pPlayer->obobWidth = pPlayer->bobWidth;
	pPlayer->oswayHeight = pPlayer->swayHeight;
	pPlayer->oswayWidth = pPlayer->swayWidth;
	pPlayer->actor->backuploc();
	pPlayer->actor->interpolated = true;
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

void viewDrawAimedPlayerName(PLAYER* pPlayer)
{
	if (!cl_idplayers || (pPlayer->flt_aim().XY().isZero()))
		return;

	int hit = HitScan(pPlayer->actor, pPlayer->zView, pPlayer->flt_aim(), CLIPMASK0, 512);
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

extern DAngle random_angles[16][3];

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void viewInit(void)
{
	Printf("Initializing status bar\n");

	for (int i = 0; i < 16; i++)
	{
		random_angles[i][0] = RandomAngle();
		random_angles[i][1] = RandomAngle();
		random_angles[i][2] = RandomAngle();
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
	// the lens effect depends on the software renderer and easy availability of the pixel data. 
	// If this ever gets redone for hardware rendering it needs very different handling on the GPU side.
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

void UpdateBlend(PLAYER* pPlayer)
{
	int nRed = 0;
	int nGreen = 0;
	int nBlue = 0;

	nRed += pPlayer->pickupEffect;
	nGreen += pPlayer->pickupEffect;
	nBlue -= pPlayer->pickupEffect;

	nRed += ClipHigh(pPlayer->painEffect, 85) * 2;
	nGreen -= ClipHigh(pPlayer->painEffect, 85) * 3;
	nBlue -= ClipHigh(pPlayer->painEffect, 85) * 3;

	nRed -= pPlayer->blindEffect;
	nGreen -= pPlayer->blindEffect;
	nBlue -= pPlayer->blindEffect;

	nRed -= pPlayer->chokeEffect >> 6;
	nGreen -= pPlayer->chokeEffect >> 5;
	nBlue -= pPlayer->chokeEffect >> 6;

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

void viewUpdateDelirium(PLAYER* pPlayer)
{
	gScreenTiltO = gScreenTilt;
	deliriumTurnO = deliriumTurn;
	deliriumPitchO = deliriumPitch;
	int powerCount;
	if ((powerCount = powerupCheck(pPlayer, kPwUpDeliriumShroom)) != 0)
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
		double sin2 = BobVal(2 * timer);
		double sin3 = BobVal(3 * timer);
		double sin4 = BobVal(4 * timer);
		double sin5 = BobVal(5 * timer);
		gScreenTilt = DAngle::fromBuild((sin2 + sin3) * tilt1 * 0.5);
		deliriumTurn = DAngle::fromBuild((sin3 + sin4) * tilt2 * 0.5);
		deliriumPitch = int((sin4 + sin5) * pitch * 0.5);
		return;
	}
	gScreenTilt = gScreenTilt.Normalized180();
	if (gScreenTilt > nullAngle)
	{
		gScreenTilt -= mapangle(8);
		if (gScreenTilt < nullAngle)
			gScreenTilt = nullAngle;
	}
	else if (gScreenTilt < nullAngle)
	{
		gScreenTilt += mapangle(8);
		if (gScreenTilt >= nullAngle)
			gScreenTilt = nullAngle;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void viewUpdateShake(PLAYER* pPlayer, DVector3& cPos, DRotator& cAngles, double& pshakeX, double& pshakeY)
{
	auto doEffect = [&](const int& effectType)
	{
		if (effectType)
		{
			int nValue = ClipHigh(effectType * 8, 2000);
			cAngles.Pitch -= maphoriz(QRandom2F(nValue * (1. / 256.)));
			cAngles.Yaw += DAngle::fromDeg(QRandom2F(nValue * (360. / 524288.)));
			cPos.X += QRandom2F(nValue * maptoworld) * maptoworld;
			cPos.Y += QRandom2F(nValue * maptoworld) * maptoworld;
			cPos.Z += QRandom2F(nValue) * zmaptoworld;
			pshakeX += QRandom2F(nValue) * (1. / 256.);
			pshakeY += QRandom2F(nValue) * (1. / 256.);
		}
	};
	doEffect(pPlayer->flickerEffect);
	doEffect(pPlayer->quakeEffect);

	cAngles.Pitch -= DAngle::fromDeg((1 - BobVal((pPlayer->tiltEffect << 2) + 512)) * 13.2);
}


int gLastPal = 0;

int32_t g_frameRate;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DrawMap(PLAYER* pPlayer, const double interpfrac)
{
	int tm = 0;
	if (viewport3d.Left() > 0)
	{
		setViewport(Hud_Stbar);
		tm = 1;
	}
	DrawOverheadMap(pPlayer->actor->interpolatedpos(interpfrac).XY(), pPlayer->Angles.getRenderAngles(interpfrac).Yaw, interpfrac);
	if (tm)
		setViewport(hud_size);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void SetupView(PLAYER* pPlayer, DVector3& cPos, DRotator& cAngles, sectortype*& pSector, double& zDelta, double& shakeX, double& shakeY, const double interpfrac)
{
	double bobWidth, bobHeight;

	pSector = pPlayer->actor->sector();
#if 0
	if (numplayers > 1 && pPlayer == gMe && gPrediction && gMe->actor->xspr.health > 0)
	{
		nSectnum = predict.sectnum;
		cX = interpolatedvalue(predictOld.x, predict.x, interpfrac);
		cY = interpolatedvalue(predictOld.y, predict.y, interpfrac);
		cZ = interpolatedvalue(predictOld.viewz, predict.viewz, interpfrac);
		zDelta = interpolatedvalue(predictOld.weaponZ, predict.weaponZ, interpfrac);
		bobWidth = interpolatedvalue(predictOld.bobWidth, predict.bobWidth, interpfrac);
		bobHeight = interpolatedvalue(predictOld.bobHeight, predict.bobHeight, interpfrac);
		shakeX = interpolatedvalue(predictOld.shakeBobX, predict.shakeBobX, interpfrac);
		shakeY = interpolatedvalue(predictOld.shakeBobY, predict.shakeBobY, interpfrac);

		if (!SyncInput())
		{
			cAngles.Yaw = bamang(predict.angle.asbam() + predict.look_ang.asbam());
			cAngles.Pitch = predict.horiz + predict.horizoff;
			cAngles.Roll = predict.cAngles.Roll;
		}
		else
		{
			cAngles.Yaw = interpolatedvalue(predictOld.angle + predictOld.look_ang, predict.angle + predict.look_ang, interpfrac);
			cAngles.Pitch = interpolatedvalue(predictOld.horiz + predictOld.horizoff, predict.horiz + predict.horizoff, interpfrac);
			cAngles.Roll = interpolatedvalue(predictOld.rotscrnang, predict.rotscrnang, interpfrac);
		}
	}
	else
#endif
	{
		cPos = pPlayer->actor->getRenderPos(interpfrac);
		cAngles = pPlayer->Angles.getRenderAngles(interpfrac);
		zDelta = interpolatedvalue(pPlayer->ozWeapon, pPlayer->zWeapon - pPlayer->zView - 12, interpfrac);
		bobWidth = interpolatedvalue(pPlayer->obobWidth, pPlayer->bobWidth, interpfrac);
		bobHeight = interpolatedvalue(pPlayer->obobHeight, pPlayer->bobHeight, interpfrac);
		shakeX = interpolatedvalue(pPlayer->oswayWidth, pPlayer->swayWidth, interpfrac);
		shakeY = interpolatedvalue(pPlayer->oswayHeight, pPlayer->swayHeight, interpfrac);
	}

	viewUpdateShake(pPlayer, cPos, cAngles, shakeX, shakeY);

	if (gViewPos == 0)
	{
		if (cl_viewhbob)
		{
			cPos.XY() -= cAngles.Yaw.ToVector().Rotated90CW() * bobWidth;
		}
		if (cl_viewvbob)
		{
			cPos.Z += bobHeight;
		}
		cPos.Z -= interpolatedvalue(0., 10., cAngles.Pitch / DAngle90);
	}
	else
	{
		calcChaseCamPos(cPos, pPlayer->actor, &pSector, cAngles, interpfrac, 80.);
	}
	if (pSector != nullptr)
		CheckLink(cPos, &pSector);
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
	PLAYER* pPlayer = &gPlayer[gViewIndex];

	FireProcess();

	double interpfrac;

	if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
	{
		interpfrac = !cl_interpolate || cl_capfps ? 1. : I_GetTimeFrac() * 1.;
	}
	else interpfrac = 1.;

	// update render angles.
	pPlayer->Angles.updateRenderAngles(interpfrac);

	if (cl_interpolate)
	{
		DoInterpolations(interpfrac);
	}

	if (automapMode != am_full)
	{
		DoSectorLighting();
	}
	if (automapMode == am_off)
	{
		int basepal = 0;
		if (powerupCheck(pPlayer, kPwUpDeathMask) > 0) basepal = 4;
		else if (powerupCheck(pPlayer, kPwUpReflectShots) > 0) basepal = 1;
		else if (pPlayer->isUnderwater) {
			if (pPlayer->nWaterPal) basepal = pPlayer->nWaterPal;
			else {
				if (pPlayer->actor->xspr.medium == kMediumWater) basepal = 1;
				else if (pPlayer->actor->xspr.medium == kMediumGoo) basepal = 3;
				else basepal = 2;
			}
		}
		UpdateDacs(basepal);
		UpdateBlend(pPlayer);

		DVector3 cPos;
		DRotator cAngles;
		sectortype* pSector;
		double zDelta;
		double shakeX, shakeY;
		SetupView(pPlayer, cPos, cAngles, pSector, zDelta, shakeX, shakeY, interpfrac);

		DAngle tilt = interpolatedvalue(gScreenTiltO, gScreenTilt, interpfrac);
		bool bDelirium = powerupCheck(pPlayer, kPwUpDeliriumShroom) > 0;
		static bool bDeliriumOld = false;
		//int tiltcs, tiltdim;
		uint8_t otherview = powerupCheck(pPlayer, kPwUpCrystalBall) > 0;
		if (tilt.Degrees() || bDelirium)
		{
			cAngles.Roll = -tilt;
		}
		else if (otherview && gNetPlayers > 1)
		{
#if 0    
			renderCrystalBall();
#endif
		}
		else
		{
			othercameraclock = PlayClock + int(4 * interpfrac);
		}

		if (!bDelirium)
		{
			deliriumTilt = 0;
			deliriumTurn = nullAngle;
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
		g_relvisibility = (int32_t)(ClipLow(gVisibility - 32 * pPlayer->visibility - brightness, 0)) - g_visibility;
		cAngles.Yaw += interpolatedvalue(deliriumTurnO, deliriumTurn, interpfrac);

		if (pSector != nullptr)
		{
			double ceilingZ, floorZ;
			calcSlope(pSector, cPos, &ceilingZ, &floorZ);
			if ((cPos.Z > floorZ - 1) && (pSector->upperLink == nullptr)) // clamp to floor
			{
				cPos.Z = floorZ - 1;
			}
			if ((cPos.Z < ceilingZ + 1) && (pSector->lowerLink == nullptr)) // clamp to ceiling
			{
				cPos.Z = ceilingZ + 1;
			}
		}

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

		if (!sceneonly) hudDraw(pPlayer, pSector, shakeX, shakeY, zDelta, cAngles.Roll, basepal, interpfrac);
		DAngle deliriumPitchI = interpolatedvalue(maphoriz(deliriumPitchO), maphoriz(deliriumPitch), interpfrac);
		auto bakCstat = pPlayer->actor->spr.cstat;
		pPlayer->actor->spr.cstat |= (gViewPos == 0) ? CSTAT_SPRITE_INVISIBLE : CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
		cAngles.Pitch -= deliriumPitchI;
		render_drawrooms(pPlayer->actor, cPos, pSector, cAngles, interpfrac);
		pPlayer->actor->spr.cstat = bakCstat;
		bDeliriumOld = bDelirium && gDeliriumBlur;

		if (sceneonly) return;
		auto offsets = pPlayer->Angles.getCrosshairOffsets(interpfrac);
		DrawCrosshair(kCrosshairTile, pPlayer->actor->xspr.health >> 4, offsets.first.X, offsets.first.Y, 2, offsets.second);
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
			gWeather.Draw(cX, cY, cZ, cAngles.Yaw.Tan() * (1 << 23), cAngles.Pitch.Tan() * (1 << 23) + deliriumPitch, gWeather.at12d8);
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

	MarkSectorSeen(pPlayer->actor->sector());

	if (automapMode != am_off)
	{
		DrawMap(pPlayer, interpfrac);
	}
	UpdateStatusBar(pPlayer);

	viewDrawAimedPlayerName(pPlayer);
	if (paused)
	{
		auto text = GStrings("TXTB_PAUSED");
		viewDrawText(PickBigFont(text), text, 160, 10, 0, 0, 1, 0);
	}
	else if (pPlayer->nPlayer != myconnectindex)
	{
		FStringf gTempStr("] %s [", PlayerName(pPlayer->nPlayer));
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
	PLAYER* pPlayer = &gPlayer[myconnectindex];
	if (!pPlayer->actor) return std::make_pair(DVector3(DBL_MAX, 0, 0), nullAngle);
	return std::make_pair(pPlayer->actor->spr.pos, pPlayer->actor->spr.Angles.Yaw);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac)
{
	auto cangvect = cang.ToVector();

	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		if (i == gViewIndex || gGameOptions.nGameType == 1)
		{
			auto actor = gPlayer[i].actor;
			auto vect = OutAutomapVector(mxy - cpos, cangvect, czoom, xydim);

			DrawTexture(twod, tileGetTexture(actor->spr.picnum, true), vect.X, vect.Y, DTA_ClipLeft, viewport3d.Left(), DTA_ClipTop, viewport3d.Top(), DTA_ScaleX, czoom * (2. / 3.), DTA_ScaleY, czoom * (2. / 3.), DTA_CenterOffset, true,
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
