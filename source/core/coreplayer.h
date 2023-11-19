#pragma once

#include "d_net.h"
#include "packet.h"
#include "gamestate.h"
#include "gamefuncs.h"

class DCorePlayer : public DObject
{
	DECLARE_CLASS(DCorePlayer, DObject)
	HAS_OBJECT_POINTERS
protected:
	DCorePlayer() = default;
	void Clear()
	{
		CameraAngles = PrevLerpAngles = PrevViewAngles = ViewAngles = {};
		PrevRollVel = RollVel = 0;
		YawSpin = nullAngle;
		CameraPos = {};
		memset(&lastcmd, 0, sizeof(lastcmd));
		memset(&cmd, 0, sizeof(cmd));
		actor = nullptr;
		pnum = 0;
	}

public:
	DRotator PrevLerpAngles, CameraAngles;
	DRotator PrevViewAngles, ViewAngles;
	double PrevRollVel, RollVel;
	DAngle YawSpin;
	DVector3 CameraPos;
	ticcmd_t lastcmd, cmd;
	DCoreActor* actor;
	uint8_t pnum;

	DCorePlayer(uint8_t p) : pnum(p) {}
	void OnDestroy() override { if (actor) actor->Destroy(); actor = nullptr; }
	void Serialize(FSerializer& arc) override;

	// All overridable methods.
	virtual DCoreActor* GetActor() = 0;
	virtual bool canSlopeTilt() const { return false; }
	virtual unsigned getCrouchFlags() const = 0;
	virtual double GetMaxInputVel() const = 0;
	virtual const DVector2& GetInputVelocity() const { return actor->vel.XY(); }

	// Angle prototypes.
	void doPitchInput();
	void doYawInput();
	void doViewPitch(const bool climbing = false);
	void doViewYaw();
	void doRollInput(const bool bUnderwater);

	// Angle methods.
	void InitAngles(const DAngle viewyaw = nullAngle)
	{
		PrevLerpAngles = CameraAngles = actor->spr.Angles;
		PrevViewAngles = ViewAngles = { nullAngle, viewyaw, nullAngle };
		PrevRollVel = RollVel = 0;
		YawSpin = nullAngle;
	}

	DAngle getPitchWithView()
	{
		return ClampViewPitch(actor->spr.Angles.Pitch + ViewAngles.Pitch);
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
		const auto lerpAngles = interpolatedvalue(actor->PrevAngles, actor->spr.Angles, interpfrac);
		CameraAngles += lerpAngles - PrevLerpAngles;
		PrevLerpAngles = lerpAngles;
	}

	void resetCameraAngles()
	{
		if (actor != nullptr)
		{
			// Apply any last remaining ticrate angle updates and reset variables.
			CameraAngles += actor->spr.Angles - PrevLerpAngles;
			PrevLerpAngles = actor->spr.Angles = CameraAngles;
			PrevViewAngles = ViewAngles;
		}
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
};

extern DCorePlayer* PlayerArray[MAXPLAYERS];

inline double getTicrateScale(const double value)
{
	return value / GameTicRate;
}

inline DAngle getTicrateAngle(const double value)
{
	return DAngle::fromDeg(getTicrateScale(value));
}

inline DAngle getscaledangle(const DAngle angle, const double scale, const DAngle push)
{
	return (angle.Normalized180() * getTicrateScale(scale)) + push;
}

inline bool scaletozero(DAngle& angle, const double scale, const DAngle push = DAngle::fromDeg(7646143. / 110386328.))
{
	const auto sgn = angle.Sgn();

	if (!sgn || sgn != (angle -= getscaledangle(angle, scale, push * sgn)).Sgn())
	{
		angle = nullAngle;
		return true;
	}
	return false;
}
