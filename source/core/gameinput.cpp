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
// Input scale helper functions.
//
//---------------------------------------------------------------------------

inline static double getTicrateScale(const double value)
{
	return value * (1. / GameTicRate);
}

inline static double getPushBuild(const double scaleAdjust)
{
	return 2. / 9. * (scaleAdjust < 1. ? (1. - scaleAdjust * 0.5) * 1.5 : 1.);
}

inline static DAngle getPushAngle(const double scaleAdjust)
{
	return DAngle::fromDeg(getPushBuild(scaleAdjust) * BAngToDegree);
}

inline static fixedhoriz getscaledhoriz(const double value, const double scaleAdjust, const fixedhoriz object, const double push)
{
	return buildfhoriz(scaleAdjust * ((object.asbuildf() * getTicrateScale(value)) + push));
}

inline static DAngle getscaledangle(const double value, const double scaleAdjust, const DAngle object, const DAngle push)
{
	return ((object.Normalized180() * getTicrateScale(value)) + push) * scaleAdjust;
}

inline static void scaletozero(fixedhoriz& object, const double value, const double scaleAdjust, const double push = DBL_MAX)
{
	if (auto sgn = Sgn(object.asq16()))
	{
		object  -= getscaledhoriz(value, scaleAdjust, object, push == DBL_MAX ? getPushBuild(scaleAdjust) * sgn : push);
		if (sgn != Sgn(object.asq16())) object = q16horiz(0);
	}
}

inline static void scaletozero(DAngle& object, const double value, const double scaleAdjust, const double push = DBL_MAX)
{
	if (auto sgn = object.Sgn())
	{
		object  -= getscaledangle(value, scaleAdjust, object, push == DBL_MAX ? getPushAngle(scaleAdjust) * sgn : DAngle::fromDeg(push));
		if (sgn != object.Sgn()) object = nullAngle;
	}
}


//---------------------------------------------------------------------------
//
// Functions for determining whether its turbo turn time (turn key held for a number of tics).
//
//---------------------------------------------------------------------------

/*
// Turbo turn time.
Blood:     24 * 30 = 720;
Duke: 120 / 8 * 30 = 450;
SW:   120 / 8 * 40 = 600;
Exhumed: N/A;
Average: 590.;
*/

enum
{
	BUILDTICRATE = 120,
	TURBOTURNBASE = 590,
};

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

/*
// Running speed.
Blood: 92 / 4 * 2 * 30 = 1380;
Duke:  15 * 2 * 2 * 30 = 1800;
SW:  28 * 1.40625 * 40 = 1575;   // Precisely, ((((28 * 12) + ((28 * 12) / 4)) * 3) / 32) * 40
Exhumed:   12 * 4 * 30 = 1440;
Average: 1548.75;

// Normal speed.
Blood:     92 / 4 * 30 = 690;
Duke:      15 * 2 * 30 = 900;
SW:  18 * 1.40625 * 40 = 1012.5; // Precisely, (((((12 + 6) * 12) + (((12 + 6) * 12) / 4)) * 3) / 32) * 40
Exhumed:    8 * 4 * 30 = 960;
Average: 890.625;

// Preamble.
Blood:   N/A;
Exhumed: N/A;
Duke:       5 * 2 * 30 = 300;
SW:   3 * 1.40625 * 40 = 168.75; // Precisely, ((((3 * 12) + ((3 * 12) / 4)) * 3) / 32) * 40
Average: 234.375;
*/

static constexpr double RUNNINGTURNBASE = 1548.75;
static constexpr double NORMALTURNBASE = 890.625;
static constexpr double PREAMBLEBASE = 234.375;
static constexpr double PREAMBLESCALE = PREAMBLEBASE / NORMALTURNBASE;

void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt, bool const allowstrafe, double const turnscale)
{
	// set up variables
	int const running = !!(inputBuffer->actions & SB_RUN);
	int const keymove = gi->playerKeyMove() << running;
	bool const strafing = buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe;
	float const mousevelscale = keymove * (1.f / 160.f);
	double const hidprescale = g_gameType & GAMEFLAG_PSEXHUMED ? 5. : 1.;
	double const hidspeed = getTicrateScale(RUNNINGTURNBASE) * turnscale * BAngToDegree;

	// process mouse and initial controller input.
	if (!strafing)
		currInput->avel += float(hidInput->mouseturnx + (scaleAdjust * hidInput->dyaw * hidspeed));
	else
		currInput->svel -= int16_t(((hidInput->mousemovex * mousevelscale) + (scaleAdjust * hidInput->dyaw * keymove)) * hidprescale);

	if (!(inputBuffer->actions & SB_AIMMODE))
		currInput->horz -= hidInput->mouseturny;
	else
		currInput->fvel -= int16_t(hidInput->mousemovey * mousevelscale * hidprescale);

	// process remaining controller input.
	currInput->horz -= float(scaleAdjust * hidInput->dpitch * hidspeed);
	currInput->svel += int16_t(scaleAdjust * hidInput->dx * keymove * hidprescale);
	currInput->fvel += int16_t(scaleAdjust * hidInput->dz * keymove * hidprescale);

	// process keyboard turning keys.
	if (!strafing)
	{
		bool const turnleft = buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !allowstrafe);
		bool const turnright = buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !allowstrafe);

		if (turnleft || turnright)
		{
			double const turnspeed = getTicrateScale(running ? RUNNINGTURNBASE : NORMALTURNBASE) * turnscale * BAngToDegree;
			float const turnamount = float(scaleAdjust * turnspeed * (isTurboTurnTime() ? 1. : PREAMBLESCALE));

			if (turnleft)
				currInput->avel -= turnamount;

			if (turnright)
				currInput->avel += turnamount;

			updateTurnHeldAmt(scaleAdjust);
		}
		else
		{
			resetTurnHeldAmt();
		}
	}
	else
	{
		if (buttonMap.ButtonDown(gamefunc_Turn_Left))
			currInput->svel += keymove;

		if (buttonMap.ButtonDown(gamefunc_Turn_Right))
			currInput->svel -= keymove;
	}

	// process keyboard side velocity keys.
	if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && allowstrafe)
		currInput->svel += keymove;

	if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && allowstrafe)
		currInput->svel -= keymove;

	// process keyboard forward velocity keys.
	if (!(isRR() && drink_amt >= 66 && drink_amt <= 87))
	{
		if (buttonMap.ButtonDown(gamefunc_Move_Forward))
			currInput->fvel += keymove;

		if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			currInput->fvel -= keymove;
	}
	else
	{
		if (buttonMap.ButtonDown(gamefunc_Move_Forward))
		{
			currInput->fvel += keymove;
			currInput->svel += drink_amt & 1 ? keymove : -keymove;
		}

		if (buttonMap.ButtonDown(gamefunc_Move_Backward))
		{
			currInput->fvel -= keymove;
			currInput->svel -= drink_amt & 1 ? keymove : -keymove;
		}
	}

	// add collected input to game's local input accumulation packet.
	inputBuffer->fvel = clamp(inputBuffer->fvel + currInput->fvel, -keymove, keymove);
	inputBuffer->svel = clamp(inputBuffer->svel + currInput->svel, -keymove, keymove);
	inputBuffer->avel += currInput->avel;
	inputBuffer->horz += currInput->horz;
}

//---------------------------------------------------------------------------
//
// Player's horizon function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

/*
// Aim speed.
Duke:      6 * 30 = 180;
SW: (16 / 2) * 40 = 320;
Average: 250.;

// Look speed.
Duke:     12 * 30 = 360;
SW:       16 * 40 = 640;
Average: 500.;

// Return to centre speed.
Duke: (1 / 3) * 30 = 10;
SW:   (1 / 4) * 40 = 10;
Average: 10.;
*/

enum
{
	AIMSPEED = 250,
	LOOKSPEED = 500,
	CNTRSPEED = 10,
};

void PlayerHorizon::applyinput(float const horz, ESyncBits* actions, double const scaleAdjust)
{
	// Process only if movement isn't locked.
	if (!movementlocked())
	{
		// Test if we have input to process.
		if (horz || *actions & (SB_AIM_UP | SB_AIM_DOWN | SB_LOOK_UP | SB_LOOK_DOWN))
		{
			// Store current horizon as true pitch.
			double pitch = horiz.aspitch();

			// Process mouse input.
			if (horz)
			{
				*actions &= ~SB_CENTERVIEW;
				pitch += horz;
			}

			// Process keyboard input.
			auto doKbdInput = [&](ESyncBits_ const up, ESyncBits_ const down, double const rate, bool const lock)
			{
				if (*actions & (up | down))
				{
					if (lock) *actions &= ~SB_CENTERVIEW; else *actions |= SB_CENTERVIEW;
					double const amount = scaleAdjust * HorizToPitch(getTicrateScale(rate));

					if (*actions & down)
						pitch -= amount;

					if (*actions & up)
						pitch += amount;
				}
			};
			doKbdInput(SB_AIM_UP, SB_AIM_DOWN, AIMSPEED, true);
			doKbdInput(SB_LOOK_UP, SB_LOOK_DOWN, LOOKSPEED, false);

			// clamp before converting back to horizon
			horiz = q16horiz(clamp(PitchToHoriz(pitch), gi->playerHorizMin(), gi->playerHorizMax()));
		}

		// return to center if conditions met.
		if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
		{
			scaletozero(horiz, CNTRSPEED, scaleAdjust);
			if (!horiz.asq16()) *actions &= ~SB_CENTERVIEW;
		}
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

/*
// Rotate return speed.
Duke: (1 / 2) * 30 = 15;

// Look return speed.
Duke: (1 / 4) * 30 = 7.5;

// Rotating speed.
Duke:      24 * 30 = 720;

// Looking speed.
Duke:     152 * 30 = 4560;

// Spin standing speed.
Duke:     128 * 30 = 3840;
Blood:    128 * 30 = 3840;

// Looking speed.
Blood:     64 * 30 = 1920;
*/

static constexpr double ROTRETURNSPEED  = (1. / 2.) * 30.;
static constexpr double LOOKRETURNSPEED = (1. / 4.) * 30.;

enum
{
	ROTATESPEED = 720,
	LOOKINGSPEED = 4560,
	SPINSTAND = 3840,
	SPINCROUCH = 1920,
};

void PlayerAngle::applyinput(float const avel, ESyncBits* actions, double const scaleAdjust)
{
	// Process angle return to zeros.
	scaletozero(rotscrnang, ROTRETURNSPEED, scaleAdjust);
	scaletozero(look_ang, LOOKRETURNSPEED, scaleAdjust);

	// Process keyboard input.
	auto doLookKeys = [&](ESyncBits_ const key, double const direction)
	{
		if (*actions & key)
		{
			look_ang += DAngle::fromDeg(getTicrateScale(LOOKINGSPEED) * scaleAdjust * direction * BAngToDegree);
			rotscrnang -= DAngle::fromDeg(getTicrateScale(ROTATESPEED) * scaleAdjust * direction * BAngToDegree);
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
			DAngle add = DAngle::fromDeg(getTicrateScale(!(*actions & SB_CROUCH) ? SPINSTAND : SPINCROUCH) * scaleAdjust * BAngToDegree);
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

/*
// Horizoff centre speed.
Duke: (1 / 8) * 30 = 3.75;
SW:   (1 / 8) * 40 = 5;
Average: 4.375;
*/

static constexpr double HORIZOFFSPEED = (1. / 8.) * 35.;

enum
{
	// Values used by Duke/SW, where this function originated from.
	DEFSINSHIFT = 5,
	DEFVIEWPITCH = 160,

	// Values used by Blood since it calculates differently to Duke/SW.
	BLOODSINSHIFT = 8,
	SINSHIFTDELTA = BLOODSINSHIFT - DEFSINSHIFT,
	BLOODVIEWPITCH = (0x4000 >> SINSHIFTDELTA) - (DEFVIEWPITCH << (SINSHIFTDELTA - 1)), // 1408.
};

void PlayerHorizon::calcviewpitch(vec2_t const pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, double const scaleAdjust, bool const climbing)
{
	if (cl_slopetilting && cursectnum != nullptr)
	{
		if (aimmode && canslopetilt) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			int const shift = isBlood() ? BLOODSINSHIFT : DEFSINSHIFT;
			int const x = pos.X + int(ang.Cos() * (1 << (BUILDSINBITS - shift)));
			int const y = pos.Y + int(ang.Sin() * (1 << (BUILDSINBITS - shift)));
			auto tempsect = cursectnum;
			updatesector(x, y, &tempsect);

			if (tempsect != nullptr) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector
				int const j = getflorzofslopeptr(cursectnum, pos.X, pos.Y);
				int const k = getflorzofslopeptr(tempsect, x, y);

				// If extended point is in same sector as you or the slopes
				// of the sector of the extended point and your sector match
				// closely (to avoid accidently looking straight out when
				// you're at the edge of a sector line) then adjust horizon
				// accordingly
				if (cursectnum == tempsect || (!isBlood() && abs(getflorzofslopeptr(tempsect, x, y) - k) <= (4 << 8)))
				{
					horizoff += q16horiz(fixed_t(scaleAdjust * ((j - k) * (!isBlood() ? DEFVIEWPITCH : BLOODVIEWPITCH))));
				}
			}
		}

		if (climbing)
		{
			// tilt when climbing but you can't even really tell it.
			if (horizoff.asq16() < IntToFixed(100)) horizoff += getscaledhoriz(HORIZOFFSPEED, scaleAdjust, buildhoriz(100) - horizoff, 1.);
		}
		else
		{
			// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
			scaletozero(horizoff, HORIZOFFSPEED, scaleAdjust, Sgn(horizoff.asq16()));
		}
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
