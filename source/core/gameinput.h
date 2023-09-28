#pragma once

#include "serializer.h"
#include "gamefuncs.h"

enum : unsigned
{
	CS_CANCROUCH = 1,
	CS_DISABLETOGGLE = 2,
};

enum : unsigned
{
	VEH_CANMOVE = 1,
	VEH_CANTURN = 2,
	VEH_SCALETURN = 4,
};

inline double getTicrateScale(const double value)
{
	return value / GameTicRate;
}

class GameInput
{
	enum
	{
		BUILDTICRATE = 120,
		TURBOTURNBASE = 590,
	};

	static constexpr double YAW_TURNSPEEDS[3] = { 234.375 * (360. / 2048.), 890.625 * (360. / 2048.), 1548.75 * (360. / 2048.) };
	static constexpr float MOUSE_SCALE = (1.f / 16.f);

	// Input received from the OS.
	float joyAxes[NUM_JOYAXIS];
	FVector2 mouseInput;	

	// Internal variables when generating a packet.
	InputPacket inputBuffer;
	double turnheldtime;
	int WeaponToSend;
	int dpad_lock;
	ESyncBits ActionsToSend;

	// Turn speed doubling after x amount of tics.
	void updateTurnHeldAmt(const float scaleAdjust)
	{
		turnheldtime += getTicrateScale(BUILDTICRATE) * scaleAdjust;
	}
	bool isTurboTurnTime()
	{
		return turnheldtime >= getTicrateScale(TURBOTURNBASE);
	}

	// Prototypes for private member functions.
	void processInputBits();

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
		memset(this, 0, sizeof(*this));
	}

	// Receives mouse input from OS for processing.
	void MouseAddToPos(float x, float y)
	{
		mouseInput.X += x;
		mouseInput.Y += y;
	}

	// Prototypes for large member functions.
	void processMovement(PlayerAngles* const plrAngles, const float scaleAdjust, const int drink_amt = 0, const bool allowstrafe = true, const float turnscale = 1.f);
	void processVehicle(PlayerAngles* const plrAngles, const float scaleAdjust, const float baseVel, const float velScale, const unsigned flags);
	void getInput(const double scaleAdjust, InputPacket* packet = nullptr);
	void resetCrouchToggle();
};

struct PlayerAngles
{
	// Player viewing angles, separate from the camera.
	DRotator PrevViewAngles, ViewAngles;

	// Strafe roll counter, to be incremented/managed by the game's velocity handler.
	double PrevStrafeVel, StrafeVel;

	// Holder of current yaw spin state for the 180 degree turn.
	DAngle YawSpin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);
	friend void GameInput::processMovement(PlayerAngles* const plrAngles, const float scaleAdjust, const int drink_amt, const bool allowstrafe, const float turnscale);
	friend void GameInput::processVehicle(PlayerAngles* const plrAngles, const float scaleAdjust, const float baseVel, const float velScale, const unsigned flags);

	// Prototypes.
	void doPitchInput(InputPacket* const input);
	void doYawInput(InputPacket* const input);
	void doViewPitch(const bool canslopetilt, const bool climbing = false);
	void doViewYaw(InputPacket* const input);
	void doRollInput(InputPacket* const input, const DVector2& nVelVect, const double nMaxVel, const bool bUnderwater);

	// General methods.
	void initialize(DCoreActor* const actor, const DAngle viewyaw = nullAngle)
	{
		memset(this, 0, sizeof(*this));
		pActor = actor;
		CameraAngles = PrevLerpAngles = pActor->spr.Angles;
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
	static constexpr double ROLL_TILTAVELSCALE = (1966426. / 12000000.);
	static constexpr double ROLL_TILTRETURN = 15.;
	static constexpr double YAW_LOOKINGSPEED = 801.5625;
	static constexpr double YAW_ROTATESPEED = 63.28125;
	static constexpr double YAW_LOOKRETURN = 7.5;
	static constexpr double YAW_SPINSTAND = 675.;
	static constexpr double YAW_SPINCROUCH = YAW_SPINSTAND * 0.5;
	static constexpr double PITCH_LOOKSPEED = (269426662. / 1209103.);
	static constexpr double PITCH_AIMSPEED = PITCH_LOOKSPEED * 0.5;
	static constexpr double PITCH_CENTERSPEED = 10.7375;
	static constexpr double PITCH_HORIZOFFSPEED = 4.375;
	static constexpr DAngle PITCH_CNTRSINEOFFSET = DAngle90 / 8.;
	static constexpr DAngle PITCH_HORIZOFFCLIMB = DAngle::fromDeg(-127076387. / 3344227.);
	static constexpr double PITCH_HORIZOFFPUSH = (14115687. / 31535389.);
};

extern GameInput gameInput;

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngles& w, PlayerAngles* def);
bool scaletozero(DAngle& angle, const double scale, const double push = (7646143. / 110386328.));
