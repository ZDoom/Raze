#pragma once

#include "coreactor.h"
#include "g_mapinfo.h"

BEGIN_BLD_NS

// access helpers to dudeinfo properties. We do not really want to go through the hashmap each time when reading them.
inline size_t o_seqStartID;
inline size_t o_Periphery;
inline size_t o_SeeDist;
inline size_t o_HearDist;
inline size_t o_MeleeDist;
inline size_t o_TurnRange;
inline size_t o_FrontSpeed;
inline size_t o_SideSpeed;
inline size_t o_ClipDist;
inline size_t o_startHealth;
inline size_t o_mass;
inline size_t o_eyeHeight;
inline size_t o_aimHeight;
inline size_t o_fleeHealth;
inline size_t o_alertChance;
inline size_t o_lockout;
inline size_t o_classflags;



struct SPRITEHIT
{
	// These must use read barriers as they can live longer and need proper GC maintenance.
	Collision hit, ceilhit, florhit;
};

class DBloodActor;

class DBloodActor : public DCoreActor
{
	DECLARE_CLASS(DBloodActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:
	double dudeSlope; // Q18.14 format
	bool hasx;
	bool explosionhackflag; // this originally hijacked the target field which is not safe when working with pointers.
	XSPRITE xspr;
	SPRITEHIT hit;
	DUDEEXTRA dudeExtra;
	TObjPtr<DBloodActor*> ownerActor;	// was previously stored in the sprite's owner field.

	// nnext stuff. For now not exported to scripting.
#ifdef NOONE_EXTENSIONS
	SPRITEMASS spriteMass;
	GENDUDEEXTRA genDudeExtra;
	EventObject condition[2];
	TObjPtr<DBloodActor*> prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE
#endif
	DVector3 basePoint;
	int16_t dmgControl[kDamageMax];    // combination of the ones in DUDEINFO, THINGINFO and GENDUDEEXTRA, needs to be modifiable

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
		return GetType() >= kDudePlayer1 && GetType() <= kDudePlayer8;
	}

	bool IsDudeActor()
	{
		return GetType() >= kDudeBase && GetType() < kDudeMax;
	}

	bool IsThingActor()
	{
		return GetType() >= kThingBase && GetType() < kThingMax;
	}

	bool IsItemActor()
	{
		return GetType() >= kItemBase && GetType() < kItemMax;
	}

	bool IsWeaponActor()
	{
		return GetType() >= kItemWeaponBase && GetType() < kItemWeaponMax;
	}

	bool IsAmmoActor()
	{
		return GetType() >= kItemAmmoBase && GetType() < kItemAmmoMax;
	}
	
#ifdef NOONE_EXTENSIONS
	bool isActive()
	{
		if (!hasX())
			return false;

		switch (xspr.aiState->Type)
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
#endif

	void ChangeType(PClass* newtype)
	{
		if (newtype->IsDescendantOf(RUNTIME_CLASS(DBloodActor)) && newtype->Size == RUNTIME_CLASS(DBloodActor)->Size && GetClass()->Size == RUNTIME_CLASS(DBloodActor)->Size)
		{
			// It sucks having to do this but the game heavily depends on being able to swap out the class type and often uses this to manage actor state.
			// We'll allow this only for classes that do not add their own data, though.
			SetClass(newtype);
			// we also must update the dmgControl array to match the new class
			auto startdamage = static_cast<DBloodActor*>(GetDefaultByType(newtype))->dmgControl;
			if (newtype->IsDescendantOf(BloodDudeBaseClass))
			{
				for (int j = 0; j < 7; j++)
					dmgControl[j] = (int16_t)MulScale(DudeDifficulty[gGameOptions.nDifficulty], startdamage[j], 8);
			}
			else
			{
				memcpy(dmgControl, startdamage, sizeof(dmgControl));
			}
			spr.lotag = static_cast<PClassActor*>(newtype)->ActorInfo()->TypeNum;
		}
	}

	// this is only temporary
	void ChangeType(int newtype)
	{
		auto cls = GetSpawnType(newtype);
		if (cls != nullptr) ChangeType(cls);
		spr.lotag = newtype;
	}
	
	int GetType() const
	{
		return spr.lotag;
	}


	// dudeinfo helpers.
	inline FName seqStartName() const
	{
		return NAME_None;
	}

	inline int seqStartID() const
	{
		return *(int*)(GetClass()->Meta + o_seqStartID);
	}

	inline int startHealth() const
	{
		return *(int*)(GetClass()->Meta + o_startHealth);
	}

	/* virtual */inline int mass() const // if we decide to add nnext, this will need a virtual override on the script side or some comparable means to branch off!
	{
		return *(int*)(GetClass()->Meta + o_mass);
	}

	inline int eyeHeight() const
	{
		return *(int*)(GetClass()->Meta + o_eyeHeight);
	}

	inline int aimHeight() const
	{
		return *(int*)(GetClass()->Meta + o_aimHeight);
	}

	inline int fleeHealth() const
	{
		return *(int*)(GetClass()->Meta + o_fleeHealth);
	}

	inline int alertChance() const
	{
		return *(int*)(GetClass()->Meta + o_alertChance);
	}

	inline int lockout() const
	{
		return *(int*)(GetClass()->Meta + o_lockout);
	}

	inline int classflags() const
	{
		return *(int*)(GetClass()->Meta + o_classflags);
	}

	/*
	int hinderDamage; // recoil damage
	int changeTarget; // chance to change target when attacked someone else
	int changeTargetKin; // chance to change target when attacked by same type
	int backSpeed; // backward speed (unused)
	*/


	inline double HearDist() const
	{
		return *(double*)(GetClass()->Meta + o_HearDist);
	}

	inline double SeeDist() const
	{
		return *(double*)(GetClass()->Meta + o_SeeDist);
	}

	inline double MeleeDist() const
	{
		return *(double*)(GetClass()->Meta + o_MeleeDist);
	}

	inline DAngle Periphery() const
	{
		return *(DAngle*)(GetClass()->Meta + o_Periphery);
	}

	inline double FrontSpeed() const
	{
		return *(double*)(GetClass()->Meta + o_FrontSpeed);
	}

	inline int FrontSpeedFixed() const
	{
		return FloatToFixed(*(double*)(GetClass()->Meta + o_FrontSpeed));
	}

	inline double SideSpeed() const
	{
		return *(double*)(GetClass()->Meta + o_SideSpeed);
	}

	inline int SideSpeedFixed() const
	{
		return FloatToFixed(*(double*)(GetClass()->Meta + o_SideSpeed));
	}

	inline DAngle TurnRange() const
	{
		return *(DAngle*)(GetClass()->Meta + o_TurnRange);
	}

	double fClipDist() const { return clipdist * 0.25; }

};


class DBloodPlayer final : public DCorePlayer
{
	DECLARE_CLASS(DBloodPlayer, DCorePlayer)
	HAS_OBJECT_POINTERS
	DBloodPlayer() = default;
public:
	DBloodPlayer(uint8_t p) : DCorePlayer(p) {}
	void Serialize(FSerializer& arc) override;
	void Clear()
	{
		Super::Clear();
		// Quick'n dirty clear
		memset(&startOfMem, 0, sizeof(DBloodPlayer) - myoffsetof(DBloodPlayer, startOfMem));
	}

	uint8_t				startOfMem; // only for Clear
	uint8_t             newWeapon;
	bool                isRunning;

	DUDEINFO* pDudeInfo;
	int                 weaponQav;
	int                 qavCallback;
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

inline DBloodPlayer* getPlayer(int index)
{
	return static_cast<DBloodPlayer*>(PlayerArray[index]);
}

struct PlayerSave
{
	int weaponQav;
	int8_t curWeapon;
	int weaponState;
	int weaponAmmo;
	int qavCallback;
	bool qavLoop;
	int weaponTimer;
	int8_t nextWeapon;
	int qavLastTick;
	int qavTimer;

	void CopyFromPlayer(DBloodPlayer* p)
	{
		weaponQav = p->weaponQav;
		curWeapon = p->curWeapon;
		weaponState = p->weaponState;
		weaponAmmo = p->weaponAmmo;
		qavCallback = p->qavCallback;
		qavLoop = p->qavLoop;
		weaponTimer = p->weaponTimer;
		nextWeapon = p->nextWeapon;
		qavLastTick = p->qavLastTick;
		qavTimer = p->qavTimer;

	}

	void CopyToPlayer(DBloodPlayer* p)
	{
		p->weaponQav = weaponQav;
		p->curWeapon = curWeapon;
		p->weaponState = weaponState;
		p->weaponAmmo = weaponAmmo;
		p->qavCallback = qavCallback;
		p->qavLoop = qavLoop;
		p->weaponTimer = weaponTimer;
		p->nextWeapon = nextWeapon;
		p->qavLastTick = qavLastTick;
		p->qavTimer = qavTimer;
	}
};

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
	auto pSourcePlayer = getPlayer(pSource);
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
