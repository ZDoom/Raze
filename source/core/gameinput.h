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

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);

	// Prototypes for applying input.
	void applyPitch(float const horz, ESyncBits* actions, double const scaleAdjust = 1);
	void applyYaw(float const avel, ESyncBits* actions, double const scaleAdjust = 1);

	// Prototypes for applying view.
	void doViewPitch(const DVector2& pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, bool const climbing = false);
	void doViewYaw(const ESyncBits actions);

	// General methods.
	void backupViewAngles() { PrevViewAngles = ViewAngles; }
	void setActor(DCoreActor* const actor) { pActor = actor; }

	// Angle getters.
	DAngle getPitchWithView()
	{
		return pActor->spr.Angles.Pitch + ViewAngles.Pitch;
	}
	DRotator lerpViewAngles(const double interpfrac)
	{
		return interpolatedvalue(PrevViewAngles, ViewAngles, interpfrac);
	}
	DRotator getRenderAngles(const double interpfrac)
	{
		return (!SyncInput() ? pActor->spr.Angles : pActor->interpolatedangles(interpfrac)) + lerpViewAngles(interpfrac);
	}

	// Draw code helpers.
	auto getCrosshairOffsets(const double interpfrac)
	{
		// Set up angles.
		const auto viewAngles = lerpViewAngles(interpfrac);
		const auto rotTangent = viewAngles.Roll.Tan();
		const auto yawTangent = clamp(viewAngles.Yaw, -DAngle90, DAngle90).Tan();
		const auto fovTangent = tan(r_fov * pi::pi() / 360.);

		// Return as pair with roll as the 2nd object since all callers inevitably need it.
		return std::make_pair(DVector2(160, 120 * -rotTangent) * -yawTangent / fovTangent, viewAngles.Roll);
	}
	auto getWeaponOffsets(const double interpfrac)
	{
		// Push the Y down a bit since the weapon is at the edge of the screen.
		auto offsets = getCrosshairOffsets(interpfrac); offsets.first.Y *= 4.;
		return offsets;
	}

	// Pitch methods.
	void lockPitch() { AngleLocks.Set(PITCH); }
	void unlockPitch() { AngleLocks.Clear(PITCH); }
	bool lockedPitch() { return AngleLocks[PITCH]; }
	void addPitch(const DAngle value) { updateAngle(PITCH, ClampViewPitch(pActor->spr.Angles.Pitch + value)); }
	void setPitch(const DAngle value, const bool backup = false) { updateAngle(PITCH, ClampViewPitch(value), backup); }

	// Yaw methods.
	void lockYaw() { AngleLocks.Set(YAW); }
	void unlockYaw() { AngleLocks.Clear(YAW); }
	bool lockedYaw() { return AngleLocks[YAW]; }
	void addYaw(const DAngle value) { updateAngle(YAW, pActor->spr.Angles.Yaw + value); }
	void setYaw(const DAngle value, const bool backup = false) { updateAngle(YAW, value, backup); }

	// Roll methods.
	void lockRoll() { AngleLocks.Set(ROLL); }
	void unlockRoll() { AngleLocks.Clear(ROLL); }
	bool lockedRoll() { return AngleLocks[ROLL]; }
	void addRoll(const DAngle value) { updateAngle(ROLL, pActor->spr.Angles.Roll + value); }
	void setRoll(const DAngle value, const bool backup = false) { updateAngle(ROLL, value, backup); }

	// Applicator of pending angle changes.
	void applyScaledAdjustments(const double scaleAdjust)
	{
		for (unsigned i = 0; i < MAXANGLES; i++)
		{
			if (Targets[i].Sgn())
			{
				// Calculate scaled amount of target and add to the accumlation buffer.
				DAngle addition = Targets[i] * scaleAdjust;
				AppliedAmounts[i] += addition;

				// Test whether we're now reached/exceeded our target.
				if (abs(AppliedAmounts[i]) >= abs(Targets[i]))
				{
					addition -= AppliedAmounts[i] - Targets[i];
					Targets[i] = AppliedAmounts[i] = nullAngle;
				}

				// Apply the scaled addition to the angle.
				pActor->spr.Angles[i] += addition;
			}
		}
	}

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
	DRotator Targets, AppliedAmounts;
	FixedBitArray<MAXANGLES> AngleLocks;
	DCoreActor* pActor;

	// Internal angle updater to reduce boilerplate from the public setters.
	void updateAngle(const unsigned angIndex, const DAngle value, const bool backup = false)
	{
		if (!SyncInput() && !backup)
		{
			Targets[angIndex] = deltaangle(pActor->spr.Angles[angIndex], value);
			AppliedAmounts[angIndex] = nullAngle;
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
