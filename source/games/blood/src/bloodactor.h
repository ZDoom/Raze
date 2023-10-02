#pragma once

#include "coreactor.h"

BEGIN_BLD_NS

class DBloodActor;

struct SPRITEHIT
{
	// These must use read barriers as they can live longer and need proper GC maintenance.
	Collision hit, ceilhit, florhit;
};

class DBloodActor : public DCoreActor
{
	DECLARE_CLASS(DBloodActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:
	double dudeSlope; // Q18.14 format
	bool hasx;
	XSPRITE xspr;
	SPRITEHIT hit;
	DUDEEXTRA dudeExtra;
	SPRITEMASS spriteMass;
	GENDUDEEXTRA genDudeExtra;
	TObjPtr<DBloodActor*> prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE
	TObjPtr<DBloodActor*> ownerActor;	// was previously stored in the sprite's owner field.
	DVector3 basePoint;
	EventObject condition[2];
	bool explosionhackflag; // this originally hijacked the target field which is not safe when working with pointers.

	// transient data (not written to savegame)
	int cumulDamage;
	bool interpolated;

	DBloodActor() = default;
	void Serialize(FSerializer& arc) override;
	size_t PropagateMark() override;

	bool hasX() { return hasx; }
	void addX() { hasx = true; }
	
	void SetOwner(DBloodActor* own)
	{
		ownerActor = own;
	}

	DBloodActor* GetOwner()
	{
		return ownerActor;
	}

	void SetTarget(DBloodActor* own)
	{
		xspr.target = own;
	}

	DBloodActor* GetTarget()
	{
		return xspr.target;
	}

	bool ValidateTarget(const char* func)
	{
		if (GetTarget() == nullptr)
		{
			Printf(PRINT_HIGH | PRINT_NOTIFY, "%s: invalid target in calling actor\n", func);
			return false;
		}
		return true;
	}

	void SetBurnSource(DBloodActor* own)
	{
		xspr.burnSource = own;
	}

	DBloodActor* GetBurnSource()
	{
		return xspr.burnSource;
	}

	void SetSpecialOwner() // nnext hackery
	{
		ownerActor = nullptr;
		spr.intowner = kMagicOwner;
	}

	bool GetSpecialOwner()
	{
		return  ownerActor == nullptr && (spr.intowner == kMagicOwner);
	}

	bool IsPlayerActor()
	{
		return spr.type >= kDudePlayer1 && spr.type <= kDudePlayer8;
	}

	bool IsDudeActor()
	{
		return spr.type >= kDudeBase && spr.type < kDudeMax;
	}

	bool IsThingActor()
	{
		return spr.type >= kThingBase && spr.type < kThingMax;
	}

	bool IsItemActor()
	{
		return spr.type >= kItemBase && spr.type < kItemMax;
	}

	bool IsWeaponActor()
	{
		return spr.type >= kItemWeaponBase && spr.type < kItemWeaponMax;
	}

	bool IsAmmoActor()
	{
		return spr.type >= kItemAmmoBase && spr.type < kItemAmmoMax;
	}
	
	bool isActive()
	{
		if (!hasX())
			return false;

		switch (xspr.aiState->stateType)
		{
		case kAiStateIdle:
		case kAiStateGenIdle:
		case kAiStateSearch:
		case kAiStateMove:
		case kAiStateOther:
			return false;
		default:
			return true;
		}
	}
	int GetType() const
	{
		return spr.type;
	}

};

struct BloodPlayer final : public CorePlayer
{
	DUDEINFO* pDudeInfo;
	uint8_t             newWeapon;
	int                 weaponQav;
	int                 qavCallback;
	bool                isRunning;
	int                 posture;   // stand, crouch, swim
	int                 sceneQav;  // by NoOne: used to keep qav id
	double              bobPhase;
	int                 bobAmp;
	double              bobHeight;
	double              bobWidth;
	double              obobHeight;
	double              obobWidth;
	int                 swayAmp;
	double              swayHeight;
	double              swayWidth;
	double              oswayHeight;
	double              oswayWidth;
	int                 nPlayer;  // Connect id
	int                 lifeMode;
	double              zView;
	double              ozView;
	double              zViewVel;
	double              zWeapon;
	double              ozWeapon;
	double              zWeaponVel;
	double              slope;
	bool                isUnderwater;
	bool                hasKey[8];
	int8_t              hasFlag;
	TObjPtr<DBloodActor*>        ctfFlagState[2];
	int                 damageControl[7];
	int8_t              curWeapon;
	int8_t              nextWeapon;
	int                 weaponTimer;
	int                 weaponState;
	int                 weaponAmmo;  //rename
	bool                hasWeapon[kWeapMax];
	int                 weaponMode[kWeapMax];
	int                 weaponOrder[2][kWeapMax];
	//int               at149[14];
	int                 ammoCount[12];
	bool                qavLoop;
	int                 qavLastTick;
	int                 qavTimer;
	int                 fuseTime;
	int                 throwTime;
	double              throwPower;
	DVector3            aim;  // world
	DVector3            relAim;  // relative
	TObjPtr<DBloodActor*>        aimTarget;  // aim target sprite
	int                 aimTargetsCount;
	TObjPtr<DBloodActor*>        aimTargets[16];
	int                 deathTime;
	int                 pwUpTime[kMaxPowerUps];
	int                 fragCount;
	int                 fragInfo[8];
	int                 teamId;
	TObjPtr<DBloodActor*>        fragger;
	int                 underwaterTime;
	int                 bubbleTime;
	int                 restTime;
	int                 kickPower;
	int                 laughCount;
	bool                godMode;
	bool                fallScream;
	bool                cantJump;
	int                 packItemTime;  // pack timer
	int                 packItemId;    // pack id 1: diving suit, 2: crystal ball, 3: beast vision 4: jump boots
	PACKINFO            packSlots[5];  // at325 [1]: diving suit, [2]: crystal ball, [3]: beast vision [4]: jump boots
	int                 armor[3];      // armor
	//int               at342;
	//int               at346;
	TObjPtr<DBloodActor*>        voodooTarget;
	int                 voodooTargets;  // --> useless
	int                 voodooVar1;     // --> useless
	int                 vodooVar2;      // --> useless
	int                 flickerEffect;
	int                 tiltEffect;
	int                 visibility;
	int                 painEffect;
	int                 blindEffect;
	int                 chokeEffect;
	int                 handTime;
	bool                hand;  // if true, there is hand start choking the player
	int                 pickupEffect;
	bool                flashEffect;  // if true, reduce pPlayer->visibility counter
	int                 quakeEffect;
	int                 player_par;
	int                 nWaterPal;
	POSTURE             pPosture[kModeMax][kPostureMax];

	inline DBloodActor* GetActor() override
	{
		return static_cast<DBloodActor*>(actor);
	}
};

extern BloodPlayer* PlayerArray[kMaxPlayers];

inline BloodPlayer* getPlayer(int index)
{
	return PlayerArray[index];
}

// subclassed to add a game specific actor() method

extern HitInfo gHitInfo;


// Iterator wrappers that return an actor pointer, not an index.

using BloodStatIterator = TStatIterator<DBloodActor>;
using BloodSectIterator = TSectIterator<DBloodActor>;
using BloodSpriteIterator = TSpriteIterator<DBloodActor>;

inline void GetActorExtents(DBloodActor* actor, double* top, double* bottom)
{
	GetSpriteExtents(&actor->spr, top, bottom);
}

inline bool CheckSector(const BitArray& bits, DBloodActor* act)
{
	return bits[act->sectno()];
}

inline bool IsTargetTeammate(DBloodActor* pSource, DBloodActor* pTarget)
{
	if (!pSource->IsPlayerActor())
		return false;
	BloodPlayer* pSourcePlayer = getPlayer(pSource->spr.type - kDudePlayer1);
	return IsTargetTeammate(pSourcePlayer, pTarget);
}

template<typename T>
void AdjustVelocity(DBloodActor *actor, T adjuster)
{
	double nCos = actor->spr.Angles.Yaw.Cos();
	double nSin = actor->spr.Angles.Yaw.Sin();
	double t1 = actor->vel.X * nCos + actor->vel.Y * nSin;
	double t2 = actor->vel.X * nSin - actor->vel.Y * nCos;
	adjuster(actor, t1, t2);
	actor->vel.X = t1 * nCos + t2 * nSin;
	actor->vel.Y = t1 * nSin - t2 * nCos;
}

// just so we don't have to type this out several dozen times
#define ADJUSTER [=](DBloodActor* actor, double& t1, double& t2)

END_BLD_NS
