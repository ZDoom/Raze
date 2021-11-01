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

static double turnheldtime;

void updateTurnHeldAmt(double const scaleAdjust)
{
	turnheldtime += scaleAdjust * (120. / GameTicRate);
}

bool isTurboTurnTime()
{
	return turnheldtime >= 590. / GameTicRate;
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
Average: 1585.;

// Normal speed.
Blood:     92 / 4 * 30 = 690;
Duke:      15 * 2 * 30 = 900;
SW:  18 * 1.40625 * 40 = 1012.5; // Precisely, (((((12 + 6) * 12) + (((12 + 6) * 12) / 4)) * 3) / 32) * 40
Average: 867.5;

// Preamble.
Blood: N/A;
Duke:       5 * 2 * 30 = 300;
SW:   3 * 1.40625 * 40 = 168.75; // Precisely, ((((3 * 12) + ((3 * 12) / 4)) * 3) / 32) * 40
Average: 234.375;
Ratio: 867.5 / 234.375 = (2776. / 750.);

// Turbo turn time.
Blood:         24 * 30 = 720;
Duke:     128 / 8 * 30 = 450;
SW:       128 / 8 * 40 = 600;
Average: 590.;
*/

void processMovement(InputPacket* currInput, InputPacket* inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt, bool const allowstrafe, double const turnscale)
{
	// set up variables
	int const running = !!(inputBuffer->actions & SB_RUN);
	int const keymove = gi->playerKeyMove() << running;
	int const cntrlvelscale = g_gameType & GAMEFLAG_PSEXHUMED ? 8 : 1;
	float const mousevelscale = keymove / 160.f;
	double const hidspeed = ((running ? 1585. : 867.5) / GameTicRate) * BAngToDegree;

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
		double turnamount = hidspeed * turnscale;
		double preambleturn = turnamount * (750. / 2776.);

		// allow Exhumed to use its legacy values given the drastic difference from the other games.
		if ((g_gameType & GAMEFLAG_PSEXHUMED) && cl_exhumedoldturn)
		{
			preambleturn = turnamount = (running ? 12 : 8) * BAngToDegree;
		}

		if (buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !allowstrafe))
		{
			updateTurnHeldAmt(scaleAdjust);
			currInput->avel -= float(scaleAdjust * (isTurboTurnTime() ? turnamount : preambleturn));
		}
		else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !allowstrafe))
		{
			updateTurnHeldAmt(scaleAdjust);
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
				if (drink_amt & 1)
					currInput->svel += keymove;
				else
					currInput->svel -= keymove;
			}

			if (buttonMap.ButtonDown(gamefunc_Move_Backward))
			{
				currInput->fvel -= keymove;
				if (drink_amt & 1)
					currInput->svel -= keymove;
				else
					currInput->svel += keymove;
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
			double const amount = HorizToPitch(250. / GameTicRate);

			if (*actions & SB_AIM_DOWN)
				pitch -= scaleAdjust * amount;

			if (*actions & SB_AIM_UP)
				pitch += scaleAdjust * amount;
		}

		// this is the unlocked type
		if (*actions & (SB_LOOK_UP|SB_LOOK_DOWN))
		{
			*actions |= SB_CENTERVIEW;
			double const amount = HorizToPitch(500. / GameTicRate);

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
			horiz -= buildfhoriz(scaleAdjust * horiz.asbuildf() * (10. / GameTicRate));
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

void PlayerAngle::applyinput(float const avel, ESyncBits* actions, double const scaleAdjust)
{
	if (rotscrnang.asbam())
	{
		// return rotscrnang to 0
		rotscrnang -= buildfang(scaleAdjust * rotscrnang.signedbuildf() * (15. / GameTicRate));
		if (abs(rotscrnang.signedbam()) < (BAMUNIT >> 2)) rotscrnang = bamang(0);
	}

	if (look_ang.asbam())
	{
		// return look_ang to 0
		look_ang -= buildfang(scaleAdjust * look_ang.signedbuildf() * (7.5 / GameTicRate));
		if (abs(look_ang.signedbam()) < (BAMUNIT >> 2)) look_ang = bamang(0);
	}

	if (*actions & SB_LOOK_LEFT)
	{
		// start looking left
		look_ang -= buildfang(scaleAdjust * (4560. / GameTicRate));
		rotscrnang += buildfang(scaleAdjust * (720. / GameTicRate));
	}

	if (*actions & SB_LOOK_RIGHT)
	{
		// start looking right
		look_ang += buildfang(scaleAdjust * (4560. / GameTicRate));
		rotscrnang -= buildfang(scaleAdjust * (720. / GameTicRate));
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
			double add = scaleAdjust * ((!(*actions & SB_CROUCH) ? 3840. : 1920.) / GameTicRate);
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

void PlayerHorizon::calcviewpitch(vec2_t const pos, binangle const ang, bool const aimmode, bool const canslopetilt, int const cursectnum, double const scaleAdjust, bool const climbing)
{
	if (cl_slopetilting)
	{
		if (aimmode && canslopetilt) // If the floor is sloped
		{
			// Get a point, 512 (64 for Blood) units ahead of player's position
			int const shift = -(isBlood() ? 8 : 5);
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
					horizoff += q16horiz(xs_CRoundToInt(scaleAdjust * ((j - k) * (!isBlood() ? 160 : 1408))));
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
