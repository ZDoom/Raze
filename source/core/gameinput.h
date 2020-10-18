#pragma once

#include "m_fixed.h"
#include "binaryangle.h"
#include "gamecvars.h"
#include "packet.h"

int getincangle(int c, int n);
fixed_t getincangleq16(fixed_t c, fixed_t n);

struct PlayerHorizon
{
	fixedhoriz horiz, ohoriz, horizoff, ohorizoff;
	double adjustment, target;

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
		if (!cl_syncinput)
		{
			adjustment += value * FRACUNIT;
		}
		else
		{
			horiz += q16horiz(FloatToFixed(value));
		}
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(double value, bool backup = false)
	{
		if (!cl_syncinput)
		{
			target = value * FRACUNIT;
			if (target == 0) target += 1;
		}
		else
		{
			horiz = q16horiz(FloatToFixed(value));
			if (backup) ohoriz = horiz;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (target)
		{
			horiz += q16horiz(xs_CRoundToInt(scaleAdjust * (target - horiz.asq16())));

			if (abs(horiz.asq16() - target) < FRACUNIT)
			{
				horiz = q16horiz(target);
				target = 0;
			}
		}
		else if (adjustment)
		{
			horiz += q16horiz(xs_CRoundToInt(scaleAdjust * adjustment));
		}
	}

	fixedhoriz sum()
	{
		return horiz + horizoff;
	}

	fixedhoriz interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio * (1. / FRACUNIT);
		fixed_t const prev = (ohoriz + ohorizoff).asq16();
		fixed_t const curr = (horiz + horizoff).asq16();
		return q16horiz(prev + xs_CRoundToInt(ratio * (curr - prev)));
	}
};

struct PlayerAngle
{
	binangle ang, oang;
	lookangle look_ang, olook_ang, rotscrnang, orotscrnang, spin;
	double adjustment, target;

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

	void addadjustment(double value)
	{
		if (!cl_syncinput)
		{
			adjustment += value * BAMUNIT;
		}
		else
		{
			ang += bamang(xs_CRoundToUInt(value * BAMUNIT));
		}
	}

	void resetadjustment()
	{
		adjustment = 0;
	}

	void settarget(double value, bool backup = false)
	{
		if (!cl_syncinput)
		{
			target = value * BAMUNIT;
			if (target == 0) target += 1;
		}
		else
		{
			ang = bamang(xs_CRoundToUInt(value * BAMUNIT));
			if (backup) oang = ang;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (target)
		{
			ang += bamang(xs_CRoundToUInt(scaleAdjust * (target - ang.asbam())));

			if (abs(ang.asbam() - target) < BAMUNIT)
			{
				ang = bamang(target);
				target = 0;
			}
		}
		else if (adjustment)
		{
			ang += bamang(xs_CRoundToUInt(scaleAdjust * adjustment));
		}
	}

	binangle sum()
	{
		return bamang(ang.asbam() + look_ang.asbam());
	}

	binangle interpolatedsum(double const smoothratio)
	{
		double const ratio = smoothratio / FRACUNIT;
		uint32_t const dang = UINT32_MAX >> 1;
		int64_t const prev = oang.asbam() + olook_ang.asbam();
		int64_t const curr = ang.asbam() + look_ang.asbam();
		return bamang(prev + xs_CRoundToUInt(ratio * (((curr + dang - prev) & 0xFFFFFFFF) - dang)));
	}

	lookangle interpolatedrotscrn(double const smoothratio)
	{
		double const ratio = smoothratio / FRACUNIT;
		return bamlook(orotscrnang.asbam() + xs_CRoundToInt(ratio * (rotscrnang.asbam() - orotscrnang.asbam())));
	}
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);

void processMovement(InputPacket* currInput, InputPacket* inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
void sethorizon(fixedhoriz* horiz, float const horz, ESyncBits* actions, double const scaleAdjust);
void applylook(PlayerAngle* angle, float const avel, ESyncBits* actions, double const scaleAdjust, bool const crouching);

