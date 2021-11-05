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
#include "build.h"

CVARD(Bool, invertmousex, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "invert horizontal mouse movement")
CVARD(Bool, invertmouse, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "invert vertical mouse movement")

//---------------------------------------------------------------------------
//
// code fron gameexec/conrun
//
//---------------------------------------------------------------------------

int getincangle(int a, int na)
{
	a &= 2047;
	na &= 2047;

	if(abs(a-na) >= 1024)
	{
		if(na > 1024) na -= 2048;
		if(a > 1024) a -= 2048;
	}

	return na-a;
}

binangle getincanglebam(binangle a, binangle na)
{
	int64_t cura = a.asbam();
	int64_t newa = na.asbam();

	if(abs(cura-newa) > INT32_MAX)
	{
		if(newa > INT32_MAX) newa -= UINT32_MAX;
		if(cura > INT32_MAX) cura -= UINT32_MAX;
	}

	return bamang(uint32_t(newa-cura));
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
	turnheldtime += scaleAdjust * (double(BUILDTICRATE) / GameTicRate);
}

bool isTurboTurnTime()
{
	return turnheldtime >= double(TURBOTURNBASE) / GameTicRate;
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

enum
{
	RUNNINGTURNBASE = 1549,
	NORMALTURNBASE = 891,
	PREAMBLEBASE = 234,
};

void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt, bool const allowstrafe, double const turnscale)
{
	// set up variables
	int const running = !!(inputBuffer->actions & SB_RUN);
	int const keymove = gi->playerKeyMove() << running;
	float const mousevelscale = keymove * (1.f / 160.f);
	double const cntrlvelscale = g_gameType & GAMEFLAG_PSEXHUMED ? 8. : 1.;
	double const hidspeed = (double(running ? RUNNINGTURNBASE : NORMALTURNBASE) / GameTicRate) * BAngToDegree;

	// process mouse and initial controller input.
	if (buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe)
		currInput->svel -= xs_CRoundToInt((hidInput->mousemovex * mousevelscale) + (scaleAdjust * hidInput->dyaw * keymove * cntrlvelscale));
	else
		currInput->avel += float(hidInput->mouseturnx + (scaleAdjust * hidInput->dyaw * hidspeed * turnscale));

	if (!(inputBuffer->actions & SB_AIMMODE))
		currInput->horz -= hidInput->mouseturny;
	else
		currInput->fvel -= xs_CRoundToInt(hidInput->mousemovey * mousevelscale);

	if (invertmouse)
		currInput->horz = -currInput->horz;

	if (invertmousex)
		currInput->avel = -currInput->avel;

	// process remaining controller input.
	currInput->horz -= float(scaleAdjust * hidInput->dpitch * hidspeed);
	currInput->svel += xs_CRoundToInt(scaleAdjust * hidInput->dx * keymove * cntrlvelscale);
	currInput->fvel += xs_CRoundToInt(scaleAdjust * hidInput->dz * keymove * cntrlvelscale);

	// process keyboard turning keys.
	if (buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe)
	{
		if (abs(inputBuffer->svel) < keymove)
		{
			if (buttonMap.ButtonDown(gamefunc_Turn_Left))
				currInput->svel += keymove;

			if (buttonMap.ButtonDown(gamefunc_Turn_Right))
				currInput->svel -= keymove;
		}
	}
	else
	{
		bool const turnleft = buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !allowstrafe);
		bool const turnright = buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !allowstrafe);

		if (turnleft || turnright)
		{
			double const turnamount = hidspeed * turnscale;
			double const preambleturn = turnamount * (double(PREAMBLEBASE) / double(NORMALTURNBASE));

			updateTurnHeldAmt(scaleAdjust);

			if (turnleft)
				currInput->avel -= float(scaleAdjust * (isTurboTurnTime() ? turnamount : preambleturn));

			if (turnright)
				currInput->avel += float(scaleAdjust * (isTurboTurnTime() ? turnamount : preambleturn));
		}
		else
		{
			resetTurnHeldAmt();
		}
	}

	// process keyboard forward/side velocity keys.
	if (abs(inputBuffer->svel) < keymove)
	{
		if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && allowstrafe)
			currInput->svel += keymove;

		if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && allowstrafe)
			currInput->svel -= keymove;
	}
	if (abs(inputBuffer->fvel) < keymove)
	{
		if (isRR() && drink_amt >= 66 && drink_amt <= 87)
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
		else
		{
			if (buttonMap.ButtonDown(gamefunc_Move_Forward))
				currInput->fvel += keymove;

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
				currInput->fvel -= keymove;
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
	// Process only if movewment isn't locked.
	if (!movementlocked())
	{
		// Store current horizon as true pitch.
		double pitch = horiz.aspitch();

		if (horz)
		{
			*actions &= ~SB_CENTERVIEW;
			pitch += horz;
		}

		// this is the locked type
		if (*actions & (SB_AIM_UP|SB_AIM_DOWN))
		{
			*actions &= ~SB_CENTERVIEW;
			double const amount = HorizToPitch(double(AIMSPEED) / GameTicRate);

			if (*actions & SB_AIM_DOWN)
				pitch -= scaleAdjust * amount;

			if (*actions & SB_AIM_UP)
				pitch += scaleAdjust * amount;
		}

		// this is the unlocked type
		if (*actions & (SB_LOOK_UP|SB_LOOK_DOWN))
		{
			*actions |= SB_CENTERVIEW;
			double const amount = HorizToPitch(double(LOOKSPEED) / GameTicRate);

			if (*actions & SB_LOOK_DOWN)
				pitch -= scaleAdjust * amount;

			if (*actions & SB_LOOK_UP)
				pitch += scaleAdjust * amount;
		}

		// clamp before converting back to horizon
		horiz = q16horiz(clamp(PitchToHoriz(pitch), gi->playerHorizMin(), gi->playerHorizMax()));

		// return to center if conditions met.
		if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)) && horiz.asq16())
		{
			// move horiz back to 0
			horiz -= buildfhoriz(scaleAdjust * horiz.asbuildf() * (double(CNTRSPEED) / GameTicRate));
			if (abs(horiz.asq16()) < (FRACUNIT >> 2))
			{
				// not looking anymore because horiz is back at 0
				horiz = q16horiz(0);
				*actions &= ~SB_CENTERVIEW;
			}
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

enum
{
	LOOKROTRETBASE = 15,
	ROTATESPEED = 720,
	LOOKINGSPEED = 4560,
	SPINSTAND = 3840,
	SPINCROUCH = 1920,
};

void PlayerAngle::applyinput(float const avel, ESyncBits* actions, double const scaleAdjust)
{
	if (rotscrnang.asbam())
	{
		// return rotscrnang to 0
		rotscrnang -= buildfang(scaleAdjust * rotscrnang.signedbuildf() * (double(LOOKROTRETBASE) / GameTicRate));
		if (abs(rotscrnang.signedbam()) < (BAMUNIT >> 2)) rotscrnang = bamang(0);
	}

	if (look_ang.asbam())
	{
		// return look_ang to 0
		look_ang -= buildfang(scaleAdjust * look_ang.signedbuildf() * ((+LOOKROTRETBASE * 0.5) / GameTicRate));
		if (abs(look_ang.signedbam()) < (BAMUNIT >> 2)) look_ang = bamang(0);
	}

	if (*actions & SB_LOOK_LEFT)
	{
		// start looking left
		look_ang -= buildfang(scaleAdjust * (double(LOOKINGSPEED) / GameTicRate));
		rotscrnang += buildfang(scaleAdjust * (double(ROTATESPEED) / GameTicRate));
	}

	if (*actions & SB_LOOK_RIGHT)
	{
		// start looking right
		look_ang += buildfang(scaleAdjust * (double(LOOKINGSPEED) / GameTicRate));
		rotscrnang -= buildfang(scaleAdjust * (double(ROTATESPEED) / GameTicRate));
	}

	if (!movementlocked())
	{
		if (*actions & SB_TURNAROUND)
		{
			if (spin == 0)
			{
				// currently not spinning, so start a spin
				spin = -1024.;
			}
			*actions &= ~SB_TURNAROUND;
		}

		if (avel)
		{
			// add player's input
			ang += degang(avel);
			spin = 0;
		}

		if (spin < 0)
		{
			// return spin to 0
			double add = scaleAdjust * (double(!(*actions & SB_CROUCH) ? SPINSTAND : SPINCROUCH) / GameTicRate);
			spin += add;
			if (spin > 0)
			{
				// Don't overshoot our target. With variable factor this is possible.
				add -= spin;
				spin = 0;
			}
			ang += buildfang(add);
		}
	}
	else
	{
		spin = 0;
	}
}

//---------------------------------------------------------------------------
//
// Player's slope tilt when playing without a mouse and on a slope.
//
//---------------------------------------------------------------------------

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

void PlayerHorizon::calcviewpitch(vec2_t const pos, binangle const ang, bool const aimmode, bool const canslopetilt, int const cursectnum, double const scaleAdjust, bool const climbing)
{
	if (cl_slopetilting)
	{
		if (aimmode && canslopetilt) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			int const shift = -(isBlood() ? BLOODSINSHIFT : DEFSINSHIFT);
			int const x = pos.x + ang.bcos(shift);
			int const y = pos.y + ang.bsin(shift);
			int16_t tempsect = cursectnum;
			updatesector(x, y, &tempsect);

			if (tempsect >= 0) // If the new point is inside a valid sector...
			{
				// Get the floorz as if the new (x,y) point was still in
				// your sector
				int const j = getflorzofslope(cursectnum, pos.x, pos.y);
				int const k = getflorzofslope(tempsect, x, y);

				// If extended point is in same sector as you or the slopes
				// of the sector of the extended point and your sector match
				// closely (to avoid accidently looking straight out when
				// you're at the edge of a sector line) then adjust horizon
				// accordingly
				if (cursectnum == tempsect || (!isBlood() && abs(getflorzofslope(tempsect, x, y) - k) <= (4 << 8)))
				{
					horizoff += q16horiz(xs_CRoundToInt(scaleAdjust * ((j - k) * (!isBlood() ? DEFVIEWPITCH : BLOODVIEWPITCH))));
				}
			}
		}

		if (climbing)
		{
			// tilt when climbing but you can't even really tell it.
			if (horizoff.asq16() < IntToFixed(100))
				horizoff += q16horiz(xs_CRoundToInt(scaleAdjust * (((IntToFixed(100) - horizoff.asq16()) >> 3) + FRACUNIT)));
		}
		else
		{
			// Make horizoff grow towards 0 since horizoff is not modified when you're not on a slope.
			if (horizoff.asq16() > 0)
			{
				horizoff += q16horiz(xs_CRoundToInt(-scaleAdjust * ((horizoff.asq16() >> 3) + FRACUNIT)));
				if (horizoff.asq16() < 0) horizoff = q16horiz(0);
			}
			if (horizoff.asq16() < 0)
			{
				horizoff += q16horiz(xs_CRoundToInt(-scaleAdjust * ((horizoff.asq16() >> 3) - FRACUNIT)));
				if (horizoff.asq16() > 0) horizoff = q16horiz(0);
			}
		}
	}
}

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

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerPosition& w, PlayerPosition* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("pos", w.pos).EndObject();

		if (arc.isReading())
		{
			w.opos = w.pos;
		}
	}
	return arc;
}
