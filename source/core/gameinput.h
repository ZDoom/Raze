#pragma once

#include "m_fixed.h"
#include "binaryangle.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "packet.h"

int getincangle(int a, int na);
double getincanglef(double a, double na);
fixed_t getincangleq16(fixed_t a, fixed_t na);
lookangle getincanglebam(binangle a, binangle na);

struct PlayerHorizon
{
	fixedhoriz horiz, ohoriz, horizoff, ohorizoff;

	void backup()
	{
		ohoriz = horiz;
		ohorizoff = horizoff;
	}

	void restore()
	{
		horiz = ohoriz;
		horizoff = ohorizoff;
	}

	void addadjustment(double value)
	{
		__addadjustment(q16horiz(FloatToFixed(value)));
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(int value, bool backup = false)
	{
		__settarget(buildhoriz(clamp(value, FixedToInt(gi->playerHorizMin()), FixedToInt(gi->playerHorizMax()))), backup);
	}

	void settarget(double value, bool backup = false)
	{
		__settarget(buildfhoriz(clamp(value, FixedToFloat(gi->playerHorizMin()), FixedToFloat(gi->playerHorizMax()))), backup);
	}

	void settarget(fixedhoriz value, bool backup = false)
	{
		__settarget(q16horiz(clamp(value.asq16(), gi->playerHorizMin(), gi->playerHorizMax())), backup);
	}

	bool targetset()
	{
		return target.asq16();
	}

	void processhelpers(double const scaleAdjust)
	{
		if (targetset())
		{
			auto delta = (target - horiz).asq16();

			if (abs(delta) > FRACUNIT)
			{
				horiz += q16horiz(xs_CRoundToInt(scaleAdjust * delta));
			}
			else
			{
				horiz = target;
				target = q16horiz(0);
			}
		}
		else if (adjustment)
		{
			horiz += q16horiz(xs_CRoundToInt(scaleAdjust * adjustment));
		}
	}

	fixedhoriz osum()
	{
		return ohoriz + ohorizoff;
	}

	fixedhoriz sum()
	{
		return horiz + horizoff;
	}

	fixedhoriz interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio * (1. / FRACUNIT);
		fixed_t const prev = osum().asq16();
		fixed_t const curr = sum().asq16();
		return q16horiz(prev + xs_CRoundToInt(ratio * (curr - prev)));
	}

private:
	fixedhoriz target;
	double adjustment;

	void __addadjustment(fixedhoriz value)
	{
		if (!SyncInput())
		{
			adjustment += value.asq16();
		}
		else
		{
			horiz += value;
		}
	}

	void __settarget(fixedhoriz value, bool backup = false)
	{
		if (!SyncInput() && !backup)
		{
			target = value;
			if (!targetset()) target = q16horiz(1);
		}
		else
		{
			horiz = value;
			if (backup) ohoriz = horiz;
		}
	}
};

struct PlayerAngle
{
	binangle ang, oang;
	lookangle look_ang, olook_ang, rotscrnang, orotscrnang, spin;

	void backup()
	{
		oang = ang;
		olook_ang = look_ang;
		orotscrnang = rotscrnang;
	}

	void restore()
	{
		ang = oang;
		look_ang = olook_ang;
		rotscrnang = orotscrnang;
	}

	void addadjustment(int value)
	{
		__addadjustment(buildang(value));
	}

	void addadjustment(double value)
	{
		__addadjustment(buildfang(value));
	}

	void addadjustment(lookangle value)
	{
		__addadjustment(bamang(value.asbam()));
	}

	void addadjustment(binangle value)
	{
		__addadjustment(value);
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(int value, bool backup = false)
	{
		__settarget(buildang(value & 2047), backup);
	}

	void settarget(double value, bool backup = false)
	{
		__settarget(buildfang(fmod(value, 2048)), backup);
	}

	void settarget(binangle value, bool backup = false)
	{
		__settarget(value, backup);
	}

	bool targetset()
	{
		return target.asbam();
	}

	void processhelpers(double const scaleAdjust)
	{
		if (targetset())
		{
			auto delta = getincanglebam(ang, target).asbam();

			if (delta > BAMUNIT)
			{
				ang += bamang(xs_CRoundToUInt(scaleAdjust * delta));
			}
			else
			{
				ang = target;
				target = bamang(0);
			}
		}
		else if (adjustment)
		{
			ang += bamang(xs_CRoundToUInt(scaleAdjust * adjustment));
		}
	}

	binangle osum()
	{
		return oang + olook_ang;
	}

	binangle sum()
	{
		return ang + look_ang;
	}

	binangle interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio * (1. / FRACUNIT);
		uint32_t const dang = UINT32_MAX >> 1;
		int64_t const prev = osum().asbam();
		int64_t const curr = sum().asbam();
		return bamang(prev + xs_CRoundToUInt(ratio * (((curr + dang - prev) & 0xFFFFFFFF) - dang)));
	}

	lookangle interpolatedlookang(double const smoothratio)
	{
		double const ratio = smoothratio * (1. / FRACUNIT);
		return bamlook(olook_ang.asbam() + xs_CRoundToInt(ratio * (look_ang - olook_ang).asbam()));
	}

	lookangle interpolatedrotscrn(double const smoothratio)
	{
		double const ratio = smoothratio * (1. / FRACUNIT);
		return bamlook(orotscrnang.asbam() + xs_CRoundToInt(ratio * (rotscrnang - orotscrnang).asbam()));
	}

	double look_anghalf(double const smoothratio)
	{
		return (!SyncInput() ? look_ang : interpolatedlookang(smoothratio)).asbam() * (0.5 / BAMUNIT); // Used within draw code for weapon and crosshair when looking left/right.
	}

private:
	binangle target;
	double adjustment;

	void __addadjustment(binangle value)
	{
		if (!SyncInput())
		{
			adjustment += value.asbam();
		}
		else
		{
			ang += value;
		}
	}

	void __settarget(binangle value, bool backup = false)
	{
		if (!SyncInput() && !backup)
		{
			target = value;
			if (!targetset()) target = bamang(1);
		}
		else
		{
			ang = value;
			if (backup) oang = ang;
		}
	}
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool const isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* currInput, InputPacket* inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
void sethorizon(fixedhoriz* horiz, float const horz, ESyncBits* actions, double const scaleAdjust = 1);
void applylook(PlayerAngle* angle, float const avel, ESyncBits* actions, double const scaleAdjust = 1);
void calcviewpitch(vec2_t const pos, fixedhoriz* horizoff, binangle const ang, bool const aimmode, bool const canslopetilt, int const cursectnum, double const scaleAdjust = 1, bool const climbing = false);
