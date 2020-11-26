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

double getincanglef(double a, double na)
{
	a = fmod(a, 2048.);
	na = fmod(na, 2048.);

	if(fabs(a-na) >= 1024)
	{
		if(na > 1024) na -= 2048;
		if(a > 1024) a -= 2048;
	}

	return na-a;
}

fixed_t getincangleq16(fixed_t a, fixed_t na)
{
	a &= 0x7FFFFFF;
	na &= 0x7FFFFFF;

	if(abs(a-na) >= IntToFixed(1024))
	{
		if(na > IntToFixed(1024)) na -= IntToFixed(2048);
		if(a > IntToFixed(1024)) a -= IntToFixed(2048);
	}

	return na-a;
}

lookangle getincanglebam(binangle a, binangle na)
{
	int64_t cura = a.asbam() & 0xFFFFFFFF;
	int64_t newa = na.asbam() & 0xFFFFFFFF;

	if(abs(cura-newa) >= BAngToBAM(1024))
	{
		if(newa > BAngToBAM(1024)) newa -= BAngToBAM(2048);
		if(cura > BAngToBAM(1024)) cura -= BAngToBAM(2048);
	}

	return bamlook(newa-cura);
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
	double const angtodegscale = 360. / 2048.;
	double const hidspeed = ((running ? 1585. : 867.5) / GameTicRate) * angtodegscale;

	// process mouse and initial controller input.
	if (buttonMap.ButtonDown(gamefunc_Strafe) && allowstrafe)
		currInput->svel -= xs_CRoundToInt((hidInput->mousemovex * mousevelscale) + (scaleAdjust * (hidInput->dyaw / 60) * keymove * cntrlvelscale));
	else
		currInput->avel += hidInput->mouseturnx + (scaleAdjust * hidInput->dyaw * hidspeed * turnscale);

	if (!(inputBuffer->actions & SB_AIMMODE))
		currInput->horz -= hidInput->mouseturny;
	else
		currInput->fvel -= xs_CRoundToInt(hidInput->mousemovey * mousevelscale);

	if (invertmouse)
		currInput->horz = -currInput->horz;

	if (invertmousex)
		currInput->avel = -currInput->avel;

	// process remaining controller input.
	currInput->horz -= scaleAdjust * hidInput->dpitch * hidspeed;
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
		static double turnheldtime;
		int const turnheldamt = 120 / GameTicRate;
		double const turboturntime = 590. / GameTicRate;
		double turnamount = hidspeed * turnscale;
		double preambleturn = turnamount * (750. / 2776.);

		// allow Exhumed to use its legacy values given the drastic difference from the other games.
		if ((g_gameType & GAMEFLAG_PSEXHUMED) && cl_exhumedoldturn)
		{
			preambleturn = turnamount = (running ? 12 : 8) * angtodegscale;
		}

		if (buttonMap.ButtonDown(gamefunc_Turn_Left) || (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !allowstrafe))
		{
			turnheldtime += scaleAdjust * turnheldamt;
			currInput->avel -= scaleAdjust * (turnheldtime >= turboturntime ? turnamount : preambleturn);
		}
		else if (buttonMap.ButtonDown(gamefunc_Turn_Right) || (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !allowstrafe))
		{
			turnheldtime += scaleAdjust * turnheldamt;
			currInput->avel += scaleAdjust * (turnheldtime >= turboturntime ? turnamount : preambleturn);
		}
		else
		{
			turnheldtime = 0;
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

void sethorizon(fixedhoriz* horiz, float const horz, ESyncBits* actions, double const scaleAdjust)
{
	// Store current horizon as true pitch.
	double pitch = horiz->aspitch();

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

	// clamp pitch after processing
	pitch = clamp(pitch, -90, 90);

	// return to center if conditions met.
	if ((*actions & SB_CENTERVIEW) && !(*actions & (SB_LOOK_UP|SB_LOOK_DOWN)))
	{
		if (abs(pitch) > 0.1375)
		{
			// move pitch back to 0
			pitch += -scaleAdjust * pitch * (9. / GameTicRate);
		}
		else
		{
			// not looking anymore because pitch is back at 0
			pitch = 0;
			*actions &= ~SB_CENTERVIEW;
		}
	}

	// clamp before returning
	*horiz = q16horiz(clamp(PitchToHoriz(pitch), gi->playerHorizMin(), gi->playerHorizMax()));
}

//---------------------------------------------------------------------------
//
// Player's angle function, called from game's ticker or from gi->GetInput() as required.
//
//---------------------------------------------------------------------------

void applylook(PlayerAngle* angle, float const avel, ESyncBits* actions, double const scaleAdjust)
{
	// return q16rotscrnang to 0 and set to 0 if less than a quarter of a unit
	angle->rotscrnang -= bamlook(xs_CRoundToInt(scaleAdjust * angle->rotscrnang.asbam() * (15. / GameTicRate)));
	if (abs(angle->rotscrnang.asbam()) < (BAMUNIT >> 2)) angle->rotscrnang = bamlook(0);

	// return q16look_ang to 0 and set to 0 if less than a quarter of a unit
	angle->look_ang -= bamlook(xs_CRoundToInt(scaleAdjust * angle->look_ang.asbam() * (7.5 / GameTicRate)));
	if (abs(angle->look_ang.asbam()) < (BAMUNIT >> 2)) angle->look_ang = bamlook(0);

	if (*actions & SB_LOOK_LEFT)
	{
		// start looking left
		angle->look_ang -= bamlook(xs_CRoundToInt(scaleAdjust * (4560. / GameTicRate) * BAMUNIT));
		angle->rotscrnang += bamlook(xs_CRoundToInt(scaleAdjust * (720. / GameTicRate) * BAMUNIT));
	}

	if (*actions & SB_LOOK_RIGHT)
	{
		// start looking right
		angle->look_ang += bamlook(xs_CRoundToInt(scaleAdjust * (4560. / GameTicRate) * BAMUNIT));
		angle->rotscrnang -= bamlook(xs_CRoundToInt(scaleAdjust * (720. / GameTicRate) * BAMUNIT));
	}

	if (*actions & SB_TURNAROUND)
	{
		if (angle->spin.asbam() == 0)
		{
			// currently not spinning, so start a spin
			angle->spin = buildlook(-1024);
		}
		*actions &= ~SB_TURNAROUND;
	}

	if (angle->spin.asbam() < 0)
	{
		// return spin to 0
		lookangle add = bamlook(xs_CRoundToUInt(scaleAdjust * ((!(*actions & SB_CROUCH) ? 3840. : 1920.) / GameTicRate) * BAMUNIT));
		angle->spin += add;
		if (angle->spin.asbam() > 0)
		{
			// Don't overshoot our target. With variable factor this is possible.
			add -= angle->spin;
			angle->spin = bamlook(0);
		}
		angle->ang += bamang(add.asbam());
	}

	if (avel)
	{
		// add player's input
		angle->ang += degang(avel);
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
			.EndObject();

		if (arc.isReading())
		{
			w.oang = w.ang;
			w.olook_ang = w.look_ang;
			w.orotscrnang = w.rotscrnang;
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
			.EndObject();

		if (arc.isReading())
		{
			w.ohoriz = w.horiz;
			w.ohorizoff = w.horizoff;
			w.resetadjustment();
		}
	}
	return arc;
}
