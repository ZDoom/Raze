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
static constexpr DAngle YAW_LOOKINGSPEED = DAngle::fromDeg(801.5625);
static constexpr DAngle YAW_ROTATESPEED = DAngle::fromDeg(63.28125);
static constexpr DAngle YAW_LOOKRETURN = DAngle::fromDeg(7.5);
static constexpr DAngle YAW_SPINSTAND = DAngle::fromDeg(675.);
static constexpr DAngle YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
static constexpr DAngle PITCH_LOOKSPEED = DAngle::fromDeg(222.83185);
static constexpr DAngle PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
static constexpr DAngle PITCH_CENTERSPEED = DAngle::fromDeg(10.25);
static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle::fromDeg(90. + 11.25);
static constexpr DAngle PITCH_HORIZOFFSPEED = DAngle::fromDeg(1.95835);
static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(38.);
static constexpr DAngle PITCH_HORIZOFFPUSH = DAngle::fromDeg(0.4476);


//---------------------------------------------------------------------------
//
// Input scale helper functions.
//
//---------------------------------------------------------------------------

template<class T>
inline static T getTicrateScale(const T value)
{
	return T(value / GameTicRate);
}

inline static double getCorrectedScale(const double scaleAdjust)
{
	// When using the output of I_GetInputFrac() to scale input adjustments at framerate, deviations of over 100 ms can occur.
	// Below formula corrects the deviation, with 0.2125 being an average between an ideal value of 0.20 for 40Hz and 0.22 for 30Hz.
	// We use the average value here as the difference is under 5 ms and is not worth complicating the algorithm for such precision.
	return scaleAdjust < 1. ? scaleAdjust * (1. + 0.21 * (1. - scaleAdjust)) : scaleAdjust;
}

inline static DAngle getscaledangle(const DAngle value, const double scaleAdjust, const DAngle object, const DAngle push)
{
	return ((object.Normalized180() * getTicrateScale(value)) + push) * getCorrectedScale(scaleAdjust);
}

inline static void scaletozero(DAngle& object, const DAngle value, const double scaleAdjust, const DAngle push = DAngle::fromDeg(32. / 465.))
{
	if (auto sgn = object.Sgn())
	{
		object  -= getscaledangle(value, scaleAdjust, object, push * sgn);
		if (sgn != object.Sgn()) object = nullAngle;
	}
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
		currInput->horz += hidInput->mouseturny - hidInput->dpitch * hidspeed * scaleAdjustf;
	else
		currInput->fvel -= hidInput->mousemovey + hidInput->dpitch * keymove * scaleAdjustf;

	// process movement input.
	currInput->fvel += moving * keymove;
	currInput->svel += strafing * keymove * allowstrafe;
	if (isRR() && drink_amt >= 66 && drink_amt <= 87) currInput->svel += drink_amt & 1 ? -currInput->fvel : currInput->fvel;

	// add collected input to game's local input accumulation packet.
	inputBuffer->fvel = clamp<float>(inputBuffer->fvel + currInput->fvel, -(float)keymove, (float)keymove);
	inputBuffer->svel = clamp<float>(inputBuffer->svel + currInput->svel, -(float)keymove, (float)keymove);
	inputBuffer->avel += currInput->avel;
	inputBuffer->horz += currInput->horz;
}


//---------------------------------------------------------------------------
//
// Player's horizon function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void PlayerHorizon::applyinput(float const horz, ESyncBits* actions, double const scaleAdjust)
{
	// Process only if movement isn't locked.
	if (!movementlocked())
	{
		// Process mouse input.
		if (horz)
		{
			*actions &= ~SB_CENTERVIEW;
			horiz += DAngle::fromDeg(horz);
		}

		// Process keyboard input.
		auto doKbdInput = [&](ESyncBits_ const up, ESyncBits_ const down, DAngle const rate, bool const lock)
		{
			if (*actions & (up | down))
			{
				if (lock) *actions &= ~SB_CENTERVIEW; else *actions |= SB_CENTERVIEW;
				horiz += getTicrateScale(rate) * scaleAdjust * (!!(*actions & down) - !!(*actions & up));
			}
		};
		doKbdInput(SB_AIM_UP, SB_AIM_DOWN, PITCH_AIMSPEED, true);
		doKbdInput(SB_LOOK_UP, SB_LOOK_DOWN, PITCH_LOOKSPEED, false);

		// Do return to centre.
		if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
		{
			scaletozero(horiz, PITCH_CENTERSPEED * (PITCH_CNTRSINEOFFSET - abs(horiz)).Sin(), scaleAdjust);
			if (!horiz.Sgn()) *actions &= ~SB_CENTERVIEW;
		}

		// clamp before we finish, even if it's clamped in the drawer.
		horiz = ClampViewPitch(horiz);
	}
	else
	{
		*actions &= ~SB_CENTERVIEW;
	}
}


//---------------------------------------------------------------------------
//
// Player's angle function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void PlayerAngle::applyinput(float const avel, ESyncBits* actions, double const scaleAdjust)
{
	// Process angle return to zeros.
	scaletozero(rotscrnang, YAW_LOOKRETURN, scaleAdjust);
	scaletozero(look_ang, YAW_LOOKRETURN, scaleAdjust);

	// Process keyboard input.
	auto doLookKeys = [&](ESyncBits_ const key, double const direction)
	{
		if (*actions & key)
		{
			look_ang += getTicrateScale(YAW_LOOKINGSPEED) * getCorrectedScale(scaleAdjust) * direction;
			rotscrnang -= getTicrateScale(YAW_ROTATESPEED) * getCorrectedScale(scaleAdjust) * direction;
		}
	};
	doLookKeys(SB_LOOK_LEFT, -1);
	doLookKeys(SB_LOOK_RIGHT, 1);

	if (!movementlocked())
	{
		if (*actions & SB_TURNAROUND)
		{
			if (spin == nullAngle)
			{
				// currently not spinning, so start a spin
				spin = -DAngle180;
			}
			*actions &= ~SB_TURNAROUND;
		}

		if (avel)
		{
			// add player's input
			ang += DAngle::fromDeg(avel);
		}

		if (spin < nullAngle)
		{
			// return spin to 0
			DAngle add = getTicrateScale(!(*actions & SB_CROUCH) ? YAW_SPINSTAND : YAW_SPINCROUCH) * scaleAdjust;
			spin += add;
			if (spin > nullAngle)
			{
				// Don't overshoot our target. With variable factor this is possible.
				add -= spin;
				spin = nullAngle;
			}
			ang += add;
		}
	}
	else
	{
		spin = nullAngle;
	}
}


//---------------------------------------------------------------------------
//
// Player's slope tilt when playing without a mouse and on a slope.
//
//---------------------------------------------------------------------------

void PlayerHorizon::calcviewpitch(const DVector2& pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, double const scaleAdjust, bool const climbing)
{
	if (cl_slopetilting && cursectnum != nullptr)
	{
		if (aimmode && canslopetilt) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			auto rotpt = pos + ang.ToVector() * (isBlood() ? 4 : 32);
			auto tempsect = cursectnum;
			updatesector(rotpt, &tempsect);

			if (tempsect != nullptr) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector
				double const j = getflorzofslopeptr(cursectnum, pos);
				double const k = getflorzofslopeptr(tempsect, rotpt);

				// If extended point is in same sector as you or the slopes
				// of the sector of the extended point and your sector match
				// closely (to avoid accidently looking straight out when
				// you're at the edge of a sector line) then adjust horizon
				// accordingly
				if (cursectnum == tempsect || (!isBlood() && abs(getflorzofslopeptr(tempsect, rotpt) - k) <= 4))
				{
					horizoff -= maphoriz(scaleAdjust * ((j - k) * (!isBlood() ? 0.625 : 5.5)));
				}
			}
		}

		if (climbing)
		{
			// tilt when climbing but you can't even really tell it.
			if (horizoff > PITCH_HORIZOFFCLIMB) horizoff -= getscaledangle(PITCH_HORIZOFFSPEED, scaleAdjust, deltaangle(horizoff, PITCH_HORIZOFFCLIMB), PITCH_HORIZOFFPUSH);
		}
		else
		{
			// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
			scaletozero(horizoff, PITCH_HORIZOFFSPEED, scaleAdjust, PITCH_HORIZOFFPUSH);
		}

		// Clamp off against the maximum allowed pitch.
		horizoff = ClampViewPitch(horizoff);
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("ang", w.ang)
			("lookang", w.look_ang)
			("rotscrnang", w.rotscrnang)
			("spin", w.spin)
			("inputdisabled", w.inputdisabled)
			.EndObject();

		if (arc.isReading())
		{
			w.oang = w.ang;
			w.olook_ang = w.look_ang;
			w.orotscrnang = w.rotscrnang;
			w.inputdisabled = w.inputdisabled;
			w.resetadjustment();
		}
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("horiz", w.horiz)
			("horizoff", w.horizoff)
			("inputdisabled", w.inputdisabled)
			.EndObject();

		if (arc.isReading())
		{
			w.ohoriz = w.horiz;
			w.ohorizoff = w.horizoff;
			w.inputdisabled = w.inputdisabled;
			w.resetadjustment();
		}
	}
	return arc;
}
