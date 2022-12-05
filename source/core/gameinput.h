#pragma once

#include "m_fixed.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "gamefuncs.h"
#include "packet.h"

struct PlayerAngles
{
	// Player viewing angles, separate from the camera.
	DRotator PrevViewAngles, ViewAngles;

	// Holder of current yaw spin state for the 180 degree turn.
	DAngle YawSpin;

	// Temporary wrappers.
	DAngle& ZzHORIZON() { return pActor->spr.Angles.Pitch; }
	DAngle& ZzOLDHORIZON() { return pActor->PrevAngles.Pitch; }
	DAngle& ZzANGLE() { return pActor->spr.Angles.Yaw; }
	DAngle& ZzOLDANGLE() { return pActor->PrevAngles.Yaw; }

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);

	// Prototypes for applying input.
	void applyPitch(float const horz, ESyncBits* actions, double const scaleAdjust = 1);
	void applyYaw(float const avel, ESyncBits* actions, double const scaleAdjust = 1);

	// Prototypes for applying view.
	void doViewPitch(const DVector2& pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, bool const climbing = false);
	void doViewYaw(const ESyncBits actions);

	// General methods.
	void resetAdjustments() { Adjustments = {}; }
	void backupViewAngles() { PrevViewAngles = ViewAngles; }
	void setActor(DCoreActor* const actor) { pActor = actor; }

	// Angle getters.
	DRotator lerpViewAngles(const double interpfrac)
	{
		return interpolatedvalue(PrevViewAngles, ViewAngles, interpfrac);
	}
	DRotator getRenderAngles(const double interpfrac)
	{
		return (!SyncInput() ? pActor->spr.Angles : pActor->interpolatedangles(interpfrac)) + lerpViewAngles(interpfrac);
	}

	// Pitch methods.
	void lockPitch() { AngleLocks.Set(PITCH); }
	void unlockPitch() { AngleLocks.Clear(PITCH); }
	bool lockedPitch() { return Targets.Pitch.Sgn() || AngleLocks[PITCH]; }
	void addPitch(const DAngle value) { addAngle(PITCH, value); }
	void setPitch(const DAngle value, const bool backup = false) { setAngle(PITCH, ClampViewPitch(value), backup); }

	// Yaw methods.
	void lockYaw() { AngleLocks.Set(YAW); }
	void unlockYaw() { AngleLocks.Clear(YAW); }
	bool lockedYaw() { return Targets.Yaw.Sgn() || AngleLocks[YAW]; }
	void addYaw(const DAngle value) { addAngle(YAW, value); }
	void setYaw(const DAngle value, const bool backup = false) { setAngle(YAW, value, backup); }

	// Roll methods.
	void lockRoll() { AngleLocks.Set(ROLL); }
	void unlockRoll() { AngleLocks.Clear(ROLL); }
	bool lockedRoll() { return Targets.Roll.Sgn() || AngleLocks[ROLL]; }
	void addRoll(const DAngle value) { addAngle(ROLL, value); }
	void setRoll(const DAngle value, const bool backup = false) { setAngle(ROLL, value, backup); }

	// Applicator of pending angle changes.
	void applyScaledAdjustments(const double scaleAdjust)
	{
		for (unsigned i = 0; i < MAXANGLES; i++)
		{
			if (Targets[i].Sgn())
			{
				const auto delta = deltaangle(pActor->spr.Angles[i], Targets[i]);

				if (abs(delta.Degrees()) > BAngToDegree)
				{
					pActor->spr.Angles[i] += delta * scaleAdjust;
				}
				else
				{
					pActor->spr.Angles[i] = Targets[i];
					Targets[i] = nullAngle;
				}
			}
			else if (Adjustments[i].Sgn())
			{
				pActor->spr.Angles[i] += Adjustments[i] * scaleAdjust;
			}
		}
	}

	// Miscellaneous helpers.
	double angLOOKANGHALF(double const interpfrac) { return angLERPLOOKANG(interpfrac).Normalized180().Degrees() * (128. / 45.); }
	double angLOOKINGARC(double const interpfrac) { return fabs(angLERPLOOKANG(interpfrac).Normalized180().Degrees() * (1024. / 1620.)); }

	// Crosshair x/y offsets based on look_ang's tangent.
	DVector2 angCROSSHAIROFFSETS(const double interpfrac)
	{
		return DVector2(159.72, 145.5 * -angLERPROTSCRN(interpfrac).Sin()) * -angLERPLOOKANG(interpfrac).Tan() * (1. / tan(r_fov * pi::pi() / 360.));
	}

	// Weapon x/y offsets based on the above.
	DVector2 angWEAPONOFFSETS(const double interpfrac)
	{
		auto offsets = angCROSSHAIROFFSETS(interpfrac); offsets.Y = abs(offsets.Y) * 4.;
		return offsets;
	}


	// Legacy, to be removed.
	DAngle horizSUM(const double interpfrac = 1) { return ZzHORIZON() + interpolatedvalue(PrevViewAngles.Pitch, ViewAngles.Pitch, interpfrac); }
	DAngle horizLERPSUM(double const interpfrac) { return interpolatedvalue(ZzOLDHORIZON() + PrevViewAngles.Pitch, ZzHORIZON() + ViewAngles.Pitch, interpfrac); }
	DAngle angSUM(const double interpfrac) { return ZzANGLE() + angLERPLOOKANG(interpfrac); }
	DAngle angLERPSUM(double const interpfrac) { return interpolatedvalue(ZzOLDANGLE() + PrevViewAngles.Yaw, ZzANGLE() + ViewAngles.Yaw, interpfrac); }
	DAngle angLERPANG(double const interpfrac) { return interpolatedvalue(ZzOLDANGLE(), ZzANGLE(), interpfrac); }
	DAngle angLERPLOOKANG(double const interpfrac) { return interpolatedvalue(PrevViewAngles.Yaw, ViewAngles.Yaw, interpfrac); }
	DAngle angLERPROTSCRN(double const interpfrac) { return interpolatedvalue(PrevViewAngles.Roll, ViewAngles.Roll, interpfrac); }

private:
	// DRotator indices.
	enum : unsigned
	{
		PITCH,
		YAW,
		ROLL,
		MAXANGLES,
	};

	// Private data which should never be accessed publically.
	DRotator Targets, Adjustments;
	FixedBitArray<MAXANGLES> AngleLocks;
	DCoreActor* pActor;

	void addAngle(const unsigned angIndex, const DAngle value)
	{
		if (!SyncInput())
		{
			Adjustments[angIndex] += value.Normalized180();
		}
		else
		{
			pActor->spr.Angles[angIndex] += value;
		}
	}

	void setAngle(const unsigned angIndex, const DAngle value, const bool backup)
	{
		if (!SyncInput() && !backup)
		{
			Targets[angIndex] = value.Sgn() ? value : minAngle;
		}
		else
		{
			pActor->spr.Angles[angIndex] = value;
			if (backup) pActor->PrevAngles[angIndex] = pActor->spr.Angles[angIndex];
		}
	}
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
