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

	// Player camera angles, not for direct manipulation within the playsim.
	DRotator RenderAngles;

	// Holder of current yaw spin state for the 180 degree turn.
	DAngle YawSpin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);

	// Prototypes.
	void doPitchKeys(InputPacket* const input);
	void doYawKeys(ESyncBits* actions);
	void doViewPitch(const bool canslopetilt, const bool climbing = false);
	void doViewYaw(const ESyncBits actions);

	// General methods.
	void initialize(DCoreActor* const actor, const DAngle viewyaw = nullAngle)
	{
		if (pActor = actor) RenderAngles = PrevLerpAngles = pActor->spr.Angles;
		PrevViewAngles.Yaw = ViewAngles.Yaw = viewyaw;
	}
	DAngle getPitchWithView()
	{
		return ClampViewPitch(pActor->spr.Angles.Pitch + ViewAngles.Pitch);
	}

	// Render angle functions.
	DRotator getRenderAngles(const double interpfrac)
	{
		// Get angles and return with clamped off pitch.
		auto angles = RenderAngles + interpolatedvalue(PrevViewAngles, ViewAngles, interpfrac);
		angles.Pitch = ClampViewPitch(angles.Pitch);
		return angles;
	}
	void updateRenderAngles(const double interpfrac)
	{
		// Apply the current interpolated angle state to the render angles.
		const auto lerpAngles = interpolatedvalue(pActor->PrevAngles, pActor->spr.Angles, interpfrac);
		RenderAngles += lerpAngles - PrevLerpAngles;
		PrevLerpAngles = lerpAngles;
	}
	void resetRenderAngles()
	{
		// Apply any last remaining ticrate angle updates and reset variables.
		RenderAngles += pActor->spr.Angles - PrevLerpAngles;
		PrevLerpAngles = pActor->spr.Angles = RenderAngles;
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
		// Push the Y down a bit since the weapon is at the edge of the screen.
		auto offsets = getCrosshairOffsets(interpfrac); offsets.first.Y *= 4.;
		return offsets;
	}

private:
	// Private data which should never be accessed publically.
	DRotator PrevLerpAngles;
	DCoreActor* pActor;
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
