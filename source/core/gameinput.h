#pragma once

#include "m_fixed.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "gamefuncs.h"
#include "packet.h"

struct PlayerHorizon
{
	DAngle horiz, ohoriz, horizoff, ohorizoff;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);

	// Prototypes for functions in gameinput.cpp.
	void applyinput(float const horz, ESyncBits* actions, double const scaleAdjust = 1);
	void calcviewpitch(const DVector2& pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, double const scaleAdjust = 1, bool const climbing = false);

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
	DAngle osum() { return ohoriz + ohorizoff; }
	DAngle sum() { return horiz + horizoff; }
	DAngle interpolatedsum(double const interpfrac) { return interpolatedvalue(osum(), sum(), interpfrac); }

	// Ticrate playsim adjustment helpers.
	void resetadjustment() { adjustment = nullAngle; }
	bool targetset() { return target.Sgn(); }

	// Input locking helpers.
	void lockinput() { inputdisabled = true; }
	void unlockinput() { inputdisabled = false; }
	bool movementlocked() {	return targetset() || inputdisabled; }

	// Ticrate playsim adjustment setters and processor.
	void addadjustment(DAngle const value)
	{
		if (!SyncInput())
		{
			adjustment += value;
		}
		else
		{
			horiz += value;
		}
	}

	void settarget(DAngle value, bool const backup = false)
	{
		// Clamp incoming variable because sometimes the caller can exceed bounds.
		value = ClampViewPitch(value);

		if (!SyncInput() && !backup)
		{
			target = value.Sgn() ? value : minAngle;
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
			auto delta = deltaangle(horiz, target);

			if (abs(delta).Degrees() > 0.45)
			{
				horiz += delta * scaleAdjust;
			}
			else
			{
				horiz = target;
				target = nullAngle;
			}
		}
		else if (adjustment.Sgn())
		{
			horiz += adjustment * scaleAdjust;
		}
	}

private:
	DAngle target, adjustment;
	bool inputdisabled;
};

struct PlayerAngle
{
	DAngle ang, oang, look_ang, olook_ang, rotscrnang, orotscrnang, spin;

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
	DAngle osum() { return oang + olook_ang; }
	DAngle sum() { return ang + look_ang; }
	DAngle interpolatedsum(double const interpfrac) { return interpolatedvalue(osum(), sum(), interpfrac); }
	DAngle interpolatedang(double const interpfrac) { return interpolatedvalue(oang, ang, interpfrac); }
	DAngle interpolatedlookang(double const interpfrac) { return interpolatedvalue(olook_ang, look_ang, interpfrac); }
	DAngle interpolatedrotscrn(double const interpfrac) { return interpolatedvalue(orotscrnang, rotscrnang, interpfrac); }
	DAngle renderlookang(double const interpfrac) { return !SyncInput() ? look_ang : interpolatedlookang(interpfrac); }

	// Ticrate playsim adjustment helpers.
	void resetadjustment() { adjustment = nullAngle; }
	bool targetset() { return target.Sgn(); }

	// Input locking helpers.
	void lockinput() { inputdisabled = true; }
	void unlockinput() { inputdisabled = false; }
	bool movementlocked() { return targetset() || inputdisabled; }

	// Draw code helpers. The logic where these are used rely heavily on Build's angle period.
	double look_anghalf(double const interpfrac) { return renderlookang(interpfrac).Normalized180().Degrees() * (128. / 45.); }
	double looking_arc(double const interpfrac) { return fabs(renderlookang(interpfrac).Normalized180().Degrees() * (1024. / 1620.)); }

	// Ticrate playsim adjustment setters and processor.
	void addadjustment(const DAngle value)
	{
		if (!SyncInput())
		{
			adjustment += value.Normalized180();
		}
		else
		{
			ang += value;
		}
	}

	void settarget(const DAngle value, bool const backup = false)
	{
		if (!SyncInput() && !backup)
		{
			target = value.Sgn() ? value : minAngle;
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
			auto delta = deltaangle(ang, target);

			if (abs(delta) > DAngleBuildToDeg)
			{
				ang += delta * scaleAdjust;
			}
			else
			{
				ang = target;
				target = nullAngle;
			}
		}
		else if (adjustment.Sgn())
		{
			ang += adjustment * scaleAdjust;
		}
	}

private:
	DAngle target, adjustment;
	bool inputdisabled;
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
