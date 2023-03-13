//-------------------------------------------------------------------------
/*
Copyright (C) 2019 Christoph Oelckers
Copyright (C) 2020 Mitchell Richters

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
//------------------------------------------------------------------------- 

#include "gamecontrol.h"
#include "gameinput.h"
#include "gamestruct.h"
#include "serializer.h"
#include "gamefuncs.h"

//---------------------------------------------------------------------------
//
// Static constants used throughout functions.
//
//---------------------------------------------------------------------------

enum
{
	BUILDTICRATE = 120,
	TURBOTURNBASE = 590,
};

static constexpr double YAW_TURNSPEEDS[3] = { 41.1987304, 156.555175, 272.24121 };
static constexpr double YAW_PREAMBLESCALE = YAW_TURNSPEEDS[0] / YAW_TURNSPEEDS[1];
static constexpr double YAW_LOOKINGSPEED = 801.5625;
static constexpr double YAW_ROTATESPEED = 63.28125;
static constexpr double YAW_LOOKRETURN = 7.5;
static constexpr double YAW_SPINSTAND = 675.;
static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
static constexpr double PITCH_LOOKSPEED = 222.83185;
static constexpr double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
static constexpr double PITCH_CENTERSPEED = 10.7375;
static constexpr double PITCH_HORIZOFFSPEED = 4.375;
static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 / 8.;
static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(-38.);
static constexpr DAngle PITCH_HORIZOFFPUSH = DAngle::fromDeg(0.4476);


//---------------------------------------------------------------------------
//
// Input scale helper functions.
//
//---------------------------------------------------------------------------

static inline double getTicrateScale(const double value)
{
	return value / GameTicRate;
}

static inline DAngle getscaledangle(const DAngle angle, const double scale, const DAngle push)
{
	return (angle.Normalized180() * getTicrateScale(scale)) + push;
}

static inline bool scaletozero(DAngle& angle, const double scale, const DAngle push = DAngle::fromDeg(32. / 465.))
{
	auto sgn = angle.Sgn();

	if (!sgn || sgn != (angle -= getscaledangle(angle, scale, push * sgn)).Sgn())
	{
		angle = nullAngle;
		return true;
	}
	return false;
}


//---------------------------------------------------------------------------
//
// Functions for determining whether its turbo turn time (turn key held for a number of tics).
//
//---------------------------------------------------------------------------

static double turnheldtime;

void updateTurnHeldAmt(double const scaleAdjust)
{
	turnheldtime += getTicrateScale(BUILDTICRATE) * scaleAdjust;
}

bool isTurboTurnTime()
{
	return turnheldtime >= getTicrateScale(TURBOTURNBASE);
}

void resetTurnHeldAmt()
{
	turnheldtime = 0;
}


//---------------------------------------------------------------------------
//
// Player's movement function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt, bool const allowstrafe, double const turnscale)
{
	// set up variables.
	int const keymove = 1 << int(!!(inputBuffer->actions & SB_RUN));
	float const hidspeed = float(getTicrateScale(YAW_TURNSPEEDS[2]) * turnscale);
	float const scaleAdjustf = float(scaleAdjust);

	// determine player input.
	auto const turning = buttonMap.ButtonDown(gamefunc_Turn_Right) - buttonMap.ButtonDown(gamefunc_Turn_Left);
	auto const moving = buttonMap.ButtonDown(gamefunc_Move_Forward) - buttonMap.ButtonDown(gamefunc_Move_Backward) + hidInput->dz * scaleAdjustf;
	auto const strafing = buttonMap.ButtonDown(gamefunc_Strafe_Right) - buttonMap.ButtonDown(gamefunc_Strafe_Left) - hidInput->dx * scaleAdjustf;

	// process player angle input.
	if (!(buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe))
	{
		float const turndir = clamp(turning + strafing * !allowstrafe, -1.f, 1.f);
		float const turnspeed = float(getTicrateScale(YAW_TURNSPEEDS[keymove]) * turnscale * (isTurboTurnTime() ? 1. : YAW_PREAMBLESCALE));
		currInput->avel += hidInput->mouseturnx + (hidInput->dyaw * hidspeed + turndir * turnspeed) * scaleAdjustf;
		if (turndir) updateTurnHeldAmt(scaleAdjust); else resetTurnHeldAmt();
	}
	else
	{
		currInput->svel += hidInput->mousemovex + (hidInput->dyaw + turning) * keymove * scaleAdjustf;
	}

	// process player pitch input.
	if (!(inputBuffer->actions & SB_AIMMODE))
		currInput->horz += hidInput->mouseturny + hidInput->dpitch * hidspeed * scaleAdjustf;
	else
		currInput->fvel -= hidInput->mousemovey + hidInput->dpitch * keymove * scaleAdjustf;

	// process movement input.
	currInput->fvel += moving * keymove;
	currInput->svel += strafing * keymove * allowstrafe;

	// process RR's drunk state.
	if (isRR() && drink_amt >= 66 && drink_amt <= 87)
		currInput->svel += drink_amt & 1 ? -currInput->fvel : currInput->fvel;

	// add collected input to game's local input accumulation packet.
	inputBuffer->fvel = clamp(inputBuffer->fvel + currInput->fvel, -(float)keymove, (float)keymove);
	inputBuffer->svel = clamp(inputBuffer->svel + currInput->svel, -(float)keymove, (float)keymove);
	inputBuffer->avel = clamp(inputBuffer->avel + currInput->avel, -179.f, 179.f);
	inputBuffer->horz = clamp(inputBuffer->horz + currInput->horz, -179.f, 179.f);
}


//---------------------------------------------------------------------------
//
// Adjust player's pitch by way of keyboard input.
//
//---------------------------------------------------------------------------

void PlayerAngles::doPitchKeys(ESyncBits* actions, const bool stopcentering)
{
	// Cancel return to center if conditions met.
	if (stopcentering) *actions &= ~SB_CENTERVIEW;

	// Process keyboard input.
	if (auto aiming = !!(*actions & SB_AIM_DOWN) - !!(*actions & SB_AIM_UP))
	{
		pActor->spr.Angles.Pitch += DAngle::fromDeg(getTicrateScale(PITCH_AIMSPEED)) * aiming;
		*actions &= ~SB_CENTERVIEW;
	}
	if (auto looking = !!(*actions & SB_LOOK_DOWN) - !!(*actions & SB_LOOK_UP))
	{
		pActor->spr.Angles.Pitch += DAngle::fromDeg(getTicrateScale(PITCH_LOOKSPEED)) * looking;
		*actions |= SB_CENTERVIEW;
	}

	// Do return to centre.
	if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
	{
		const auto pitch = abs(pActor->spr.Angles.Pitch);
		const auto scale = pitch > PITCH_CNTRSINEOFFSET ? (pitch - PITCH_CNTRSINEOFFSET).Cos() : 1.;
		if (scaletozero(pActor->spr.Angles.Pitch, PITCH_CENTERSPEED * scale))
			*actions &= ~SB_CENTERVIEW;
	}

	// clamp before we finish, factoring in the player's view pitch offset.
	const auto maximum = GetMaxPitch() - ViewAngles.Pitch * (ViewAngles.Pitch < nullAngle);
	const auto minimum = GetMinPitch() - ViewAngles.Pitch * (ViewAngles.Pitch > nullAngle);
	pActor->spr.Angles.Pitch = clamp(pActor->spr.Angles.Pitch, maximum, minimum);
}


//---------------------------------------------------------------------------
//
// Adjust player's yaw by way of keyboard input.
//
//---------------------------------------------------------------------------

void PlayerAngles::doYawKeys(ESyncBits* actions)
{
	if (*actions & SB_TURNAROUND)
	{
		if (YawSpin == nullAngle)
		{
			// currently not spinning, so start a spin
			YawSpin = -DAngle180;
		}
		*actions &= ~SB_TURNAROUND;
	}

	if (YawSpin < nullAngle)
	{
		// return spin to 0
		DAngle add = DAngle::fromDeg(getTicrateScale(!(*actions & SB_CROUCH) ? YAW_SPINSTAND : YAW_SPINCROUCH));
		YawSpin += add;
		if (YawSpin > nullAngle)
		{
			// Don't overshoot our target. With variable factor this is possible.
			add -= YawSpin;
			YawSpin = nullAngle;
		}
		pActor->spr.Angles.Yaw += add;
	}
}


//---------------------------------------------------------------------------
//
// Player's slope tilt when playing without a mouse and on a slope.
//
//---------------------------------------------------------------------------

void PlayerAngles::doViewPitch(const bool canslopetilt, const bool climbing)
{
	if (cl_slopetilting)
	{
		const auto actorsect = pActor->sector();
		if (actorsect && (actorsect->floorstat & CSTAT_SECTOR_SLOPE) && canslopetilt) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			const auto rotpt = pActor->spr.pos.XY() + pActor->spr.Angles.Yaw.ToVector() * (!isBlood() ? 32 : 4);
			auto tempsect = actorsect;
			updatesector(rotpt, &tempsect);

			if (tempsect != nullptr) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector, unless it's Blood.
				const double j = getflorzofslopeptr(actorsect, pActor->spr.pos.XY());
				const double k = getflorzofslopeptr(!isBlood() ? actorsect : tempsect, rotpt);

				// If extended point is in same sector as you or the slopes
				// of the sector of the extended point and your sector match
				// closely (to avoid accidently looking straight out when
				// you're at the edge of a sector line) then adjust horizon
				// accordingly
				if (actorsect == tempsect || (!isBlood() && abs(getflorzofslopeptr(tempsect, rotpt) - k) <= 4))
				{
					ViewAngles.Pitch -= maphoriz((j - k) * (!isBlood() ? 0.625 : 5.5));
				}
			}
		}
	}

	if (cl_slopetilting && climbing)
	{
		// tilt when climbing but you can't even really tell it.
		if (ViewAngles.Pitch > PITCH_HORIZOFFCLIMB)
			ViewAngles.Pitch += getscaledangle(deltaangle(ViewAngles.Pitch, PITCH_HORIZOFFCLIMB), PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
	}
	else
	{
		// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
		scaletozero(ViewAngles.Pitch, PITCH_HORIZOFFSPEED, PITCH_HORIZOFFPUSH);
	}

	// Clamp off against the maximum allowed pitch.
	ViewAngles.Pitch = ClampViewPitch(ViewAngles.Pitch);
}


//---------------------------------------------------------------------------
//
// Player's look left/right key angle handler.
//
//---------------------------------------------------------------------------

void PlayerAngles::doViewYaw(const ESyncBits actions)
{
	// Process angle return to zeros.
	scaletozero(ViewAngles.Yaw, YAW_LOOKRETURN);
	scaletozero(ViewAngles.Roll, YAW_LOOKRETURN);

	// Process keyboard input.
	if (auto looking = !!(actions & SB_LOOK_RIGHT) - !!(actions & SB_LOOK_LEFT))
	{
		ViewAngles.Yaw += DAngle::fromDeg(getTicrateScale(YAW_LOOKINGSPEED)) * looking;
		ViewAngles.Roll += DAngle::fromDeg(getTicrateScale(YAW_ROTATESPEED)) * looking;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("viewangles", w.ViewAngles)
			("spin", w.YawSpin)
			("actor", w.pActor)
			.EndObject();

		if (arc.isReading())
		{
			w.resetRenderAngles();
		}
	}
	return arc;
}
