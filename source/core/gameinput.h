#pragma once

#include "serializer.h"
#include "gamefuncs.h"

inline double getTicrateScale(const double value)
{
	return value / GameTicRate;
}

struct HIDInput
{
	float       joyaxes[NUM_JOYAXIS];
	FVector2    mouse;
};

class GameInput
{
	enum
	{
		BUILDTICRATE = 120,
		TURBOTURNBASE = 590,
	};

	static constexpr float MOUSESCALE = (1.f / 16.f);
	static constexpr double YAW_TURNSPEEDS[3] = { 41.1987304, 156.555175, 272.24121 };
	static constexpr double YAW_PREAMBLESCALE = YAW_TURNSPEEDS[0] / YAW_TURNSPEEDS[1];

	// Internal variables when generating a packet.
	InputPacket inputBuffer;
	double turnheldtime;
	int WeaponToSend;
	int dpad_lock;
	ESyncBits ActionsToSend;

	// Turn speed doubling after x amount of tics.
	void updateTurnHeldAmt(const double scaleAdjust)
	{
		turnheldtime += getTicrateScale(BUILDTICRATE) * scaleAdjust;
	}
	bool isTurboTurnTime()
	{
		return turnheldtime >= getTicrateScale(TURBOTURNBASE);
	}

	// Prototypes for private member functions.
	void ApplyGlobalInput(HIDInput* const hidInput);

public:
	// Bit sender updates.
	void SendWeapon(const int weapon)
	{
		WeaponToSend = weapon;
	}
	void SendAction(const ESyncBits action)
	{
		ActionsToSend |= action;
	}

	// Clear all values within this object.
	void Clear()
	{
		inputBuffer = {};
		ActionsToSend = 0;
		WeaponToSend = 0;
		dpad_lock = 0;
		turnheldtime = 0;
	}

	// Prototypes for large member functions.
	void processMovement(HIDInput* const hidInput, InputPacket* const currInput, const double scaleAdjust, const int drink_amt = 0, const bool allowstrafe = true, const double turnscale = 1.);
	void processVehicle(HIDInput* const hidInput, InputPacket* const currInput, const double scaleAdjust, const float baseVel, const float velScale, const bool canMove, const bool canTurn, const bool attenuate);
	void getInput(const double scaleAdjust, PlayerAngles* const plrAngles, InputPacket* packet = nullptr);
};

struct PlayerAngles
{
	// Player viewing angles, separate from the camera.
	DRotator PrevViewAngles, ViewAngles;

	// Holder of current yaw spin state for the 180 degree turn.
	DAngle YawSpin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);
	friend void GameInput::getInput(const double scaleAdjust, PlayerAngles* const plrAngles, InputPacket* packet);

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

	// Constants used throughout input functions.
	static constexpr double YAW_LOOKINGSPEED = 801.5625;
	static constexpr double YAW_ROTATESPEED = 63.28125;
	static constexpr double YAW_LOOKRETURN = 7.5;
	static constexpr double YAW_SPINSTAND = 675.;
	static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
	static constexpr double PITCH_LOOKSPEED = 222.83185;
	static constexpr double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
	static constexpr double PITCH_CENTERSPEED = 10.7375;
	static constexpr double PITCH_HORIZOFFSPEED = 4.375;
	static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 / 8.;
	static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(-38.);
	static constexpr DAngle PITCH_HORIZOFFPUSH = DAngle::fromDeg(0.4476);
};

extern GameInput gameInput;

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);
void processCrouchToggle(bool& toggle, ESyncBits& actions, const bool crouchable, const bool disabletoggle);
