#pragma once

#include "serializer.h"
#include "gamefuncs.h"

struct PlayerAngles
{
	// Player viewing angles, separate from the camera.
	DRotator PrevViewAngles, ViewAngles;

	// Holder of current yaw spin state for the 180 degree turn.
	DAngle YawSpin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);
	friend void getInput(const double scaleAdjust, PlayerAngles* const plrAngles, InputPacket* packet);

	// Prototypes.
	void doPitchKeys(InputPacket* const input);
	void doYawKeys(InputPacket* const input);
	void doViewPitch(const bool canslopetilt, const bool climbing = false);
	void doViewYaw(InputPacket* const input);

	// General methods.
	void initialize(DCoreActor* const actor, const DAngle viewyaw = nullAngle)
	{
		if ((pActor = actor)) CameraAngles = PrevLerpAngles = pActor->spr.Angles;
		PrevViewAngles.Yaw = ViewAngles.Yaw = viewyaw;
	}
	DAngle getPitchWithView()
	{
		return ClampViewPitch(pActor->spr.Angles.Pitch + ViewAngles.Pitch);
	}

	// Render angle functions.
	const DRotator& getCameraAngles() const
	{
		return CameraAngles;
	}
	DRotator getRenderAngles(const double interpfrac)
	{
		// Get angles and return with clamped off pitch.
		auto angles = CameraAngles + interpolatedvalue(PrevViewAngles, ViewAngles, interpfrac);
		angles.Pitch = ClampViewPitch(angles.Pitch);
		return angles;
	}
	void updateCameraAngles(const double interpfrac)
	{
		// Apply the current interpolated angle state to the render angles.
		const auto lerpAngles = interpolatedvalue(pActor->PrevAngles, pActor->spr.Angles, interpfrac);
		CameraAngles += lerpAngles - PrevLerpAngles;
		PrevLerpAngles = lerpAngles;
	}
	void resetCameraAngles()
	{
		// Apply any last remaining ticrate angle updates and reset variables.
		CameraAngles += pActor->spr.Angles - PrevLerpAngles;
		PrevLerpAngles = pActor->spr.Angles = CameraAngles;
		PrevViewAngles = ViewAngles;
	}

	// Draw code helpers.
	auto getCrosshairOffsets(const double interpfrac)
	{
		// Set up angles and return as pair with roll as the 2nd object since all callers inevitably need it.
		const auto viewAngles = interpolatedvalue(PrevViewAngles, ViewAngles, interpfrac);
		return std::make_pair(DVector2(160, 120 * -viewAngles.Roll.Tan()) * -viewAngles.Yaw.Tan() / tan(r_fov * pi::pi() / 360.), viewAngles.Roll);
	}
	auto getWeaponOffsets(const double interpfrac)
	{
		// Push the Y down a bit since the weapon is at the edge of the screen. Also null roll for now.
		auto offsets = getCrosshairOffsets(interpfrac); offsets.first.Y *= 4.; offsets.second = nullAngle;
		return offsets;
	}

private:
	// Private data which should never be accessed publicly.
	DRotator PrevLerpAngles, CameraAngles;
	DCoreActor* pActor;
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);


void updateTurnHeldAmt(const double scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void clearLocalInputBuffer();
void processVehicleInput(HIDInput* const hidInput, InputPacket* const inputBuffer, InputPacket* const currInput, const double scaleAdjust, const float baseVel, const float velScale, const bool canMove, const bool canTurn, const bool attenuate);
void getInput(const double scaleAdjust, PlayerAngles* const plrAngles, InputPacket* packet = nullptr);
