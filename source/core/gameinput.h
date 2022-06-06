#pragma once

#include "m_fixed.h"
#include "binaryangle.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "packet.h"

int getincangle(int a, int na);
binangle getincanglebam(binangle a, binangle na);


//---------------------------------------------------------------------------
//
// Function for dividing an input value by current ticrate for angle/horiz scaling.
//
//---------------------------------------------------------------------------

inline double getTicrateScale(double const value)
{
	return value / GameTicRate;
}

inline double getPushScale(double const scaleAdjust)
{
	return (2. / 9.) * ((scaleAdjust < 1.) ? ((1. - scaleAdjust * 0.5) * 1.5) : (1.));
}



struct PlayerHorizon
{
	fixedhoriz horiz, ohoriz, horizoff, ohorizoff;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);

	// Prototypes for functions in gameinput.cpp.
	void applyinput(float const horz, ESyncBits* actions, double const scaleAdjust = 1);
	void calcviewpitch(vec2_t const pos, binangle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, double const scaleAdjust = 1, bool const climbing = false);

	// Interpolation helpers.
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

	// Commonly used getters.
	fixedhoriz osum() { return ohoriz + ohorizoff; }
	fixedhoriz sum() { return horiz + horizoff; }
	fixedhoriz interpolatedsum(double const smoothratio) { return interpolatedhorizon(osum(), sum(), smoothratio); }

	// Ticrate playsim adjustment helpers.
	void resetadjustment() { adjustment = 0; }
	bool targetset() { return target.asq16(); }

	// Input locking helpers.
	void lockinput() { inputdisabled = true; }
	void unlockinput() { inputdisabled = false; }
	bool movementlocked() {	return targetset() || inputdisabled; }

	// Draw code helpers.
	double horizsumfrac(double const smoothratio) { return (!SyncInput() ? sum() : interpolatedsum(smoothratio)).asbuildf() * (1. / 16.); }

	// Ticrate scale helpers.
	fixedhoriz getscaledhoriz(double const value, double const scaleAdjust = 1., fixedhoriz* const object = nullptr, double const push = 0.)
	{
		return buildfhoriz(scaleAdjust * (((object ? object->asbuildf() : 1.) * getTicrateScale(value)) + push));
	}
	void scaletozero(fixedhoriz& object, double const value, double const scaleAdjust, double const push = DBL_MAX)
	{
		if (object.asq16())
		{
			auto sgn = Sgn(object.asq16());
			object  -= getscaledhoriz(value, scaleAdjust, &object, push == DBL_MAX ? sgn * getPushScale(scaleAdjust) : push);
			if (sgn != Sgn(object.asq16())) object = q16horiz(0);
		}
	}

	// Ticrate playsim adjustment setters and processor.
	void addadjustment(fixedhoriz const value)
	{
		if (!SyncInput())
		{
			adjustment += value.asbuildf();
		}
		else
		{
			horiz += value;
		}
	}

	void settarget(fixedhoriz value, bool const backup = false)
	{
		// Clamp incoming variable because sometimes the caller can exceed bounds.
		value = q16horiz(clamp(value.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));

		if (!SyncInput() && !backup)
		{
			target = value.asq16() ? value : q16horiz(1);
		}
		else
		{
			horiz = value;
			if (backup) ohoriz = horiz;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (targetset())
		{
			auto delta = (target - horiz).asbuildf();

			if (abs(delta) > 1)
			{
				horiz += buildfhoriz(scaleAdjust * delta);
			}
			else
			{
				horiz = target;
				target = q16horiz(0);
			}
		}
		else if (adjustment)
		{
			horiz += buildfhoriz(scaleAdjust * adjustment);
		}
	}

private:
	fixedhoriz target;
	double adjustment;
	bool inputdisabled;
};

struct PlayerAngle
{
	binangle ang, oang, look_ang, olook_ang, rotscrnang, orotscrnang;
	double spin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);

	// Prototypes for functions in gameinput.cpp.
	void applyinput(float const avel, ESyncBits* actions, double const scaleAdjust = 1);

	// Interpolation helpers.
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

	// Commonly used getters.
	binangle osum() { return oang + olook_ang; }
	binangle sum() { return ang + look_ang; }
	binangle interpolatedsum(double const smoothratio) { return interpolatedangle(osum(), sum(), smoothratio); }
	binangle interpolatedlookang(double const smoothratio) { return interpolatedangle(olook_ang, look_ang, smoothratio); }
	binangle interpolatedrotscrn(double const smoothratio) { return interpolatedangle(orotscrnang, rotscrnang, smoothratio); }

	// Ticrate playsim adjustment helpers.
	void resetadjustment() { adjustment = 0; }
	bool targetset() { return target.asbam(); }

	// Input locking helpers.
	void lockinput() { inputdisabled = true; }
	void unlockinput() { inputdisabled = false; }
	bool movementlocked() { return targetset() || inputdisabled; }

	// Draw code helpers.
	double look_anghalf(double const smoothratio) { return (!SyncInput() ? look_ang : interpolatedlookang(smoothratio)).signedbuildf() * 0.5; }
	double looking_arc(double const smoothratio) { return fabs((!SyncInput() ? look_ang : interpolatedlookang(smoothratio)).signedbuildf()) * (1. / 9.); }

	// Ticrate scale helpers.
	binangle getscaledangle(double const value, double const scaleAdjust = 1., binangle* const object = nullptr, double const push = 0.)
	{
		return buildfang(scaleAdjust * (((object ? object->signedbuildf() : 1.) * getTicrateScale(value)) + push));
	}
	void scaletozero(binangle& object, double const value, double const scaleAdjust, double const push = DBL_MAX)
	{
		if (object.asbam())
		{
			auto sgn = Sgn(object.signedbam());
			object  -= getscaledangle(value, scaleAdjust, &object, push == DBL_MAX ? sgn * getPushScale(scaleAdjust) : push);
			if (sgn != Sgn(object.signedbam())) object = bamang(0);
		}
	}

	// Ticrate playsim adjustment setters and processor.
	void addadjustment(binangle const value)
	{
		if (!SyncInput())
		{
			adjustment += value.signedbuildf();
		}
		else
		{
			ang += value;
		}
	}

	void settarget(binangle const value, bool const backup = false)
	{
		if (!SyncInput() && !backup)
		{
			target = value.asbam() ? value : bamang(1);
		}
		else
		{
			ang = value;
			if (backup) oang = ang;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (targetset())
		{
			auto delta = getincanglebam(ang, target).signedbuildf();

			if (abs(delta) > 1)
			{
				ang += buildfang(scaleAdjust * delta);
			}
			else
			{
				ang = target;
				target = bamang(0);
			}
		}
		else if (adjustment)
		{
			ang += buildfang(scaleAdjust * adjustment);
		}
	}

private:
	binangle target;
	double adjustment;
	bool inputdisabled;
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
