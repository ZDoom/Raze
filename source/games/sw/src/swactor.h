#pragma once

// included by game.h

BEGIN_SW_NS

struct Personality;

FTextureID picFromState(FState* s)
{
	auto& spritedef = SpriteDefs[s->sprite];
	unsigned framenum = s->Frame - 'A';
	if (framenum >= spritedef.numframes) return FNullTextureID();
	auto& frame = SpriteFrames[spritedef.spriteframes + framenum];
	return frame.Texture[0];
}


class DSWActor : public DCoreActor
{
	DECLARE_CLASS(DSWActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:

	bool hasUser;
	USER user;
	walltype* tempwall;	// transient, to replace a hack using a 16 bit sprite field.
	TObjPtr<DSWActor*> ownerActor;
	FTextureID texparam, texparam2;	// some special variants of ST1 need this...

	// These should probably be elsewhere later but for now this is most convenient.
	int16_t CloseAttackPercent[MAX_ACTOR_CLOSE_ATTACK];
	int16_t AttackPercent[MAX_ACTOR_ATTACK];

	DSWActor() = default;

	bool hasU() { return hasUser; }


	void allocUser() 
	{ 
		hasUser = true;
	}

	void clearUser()
	{
		hasUser = false;
		user.Clear();
	}

	void Serialize(FSerializer& arc) override;
	// wrappers to static class data. Must be stored in the meta data, but will require better means of access than what's currently available.
	Personality* getPersonality();
	int16_t* getCloseAttackPercent() { return CloseAttackPercent; }
	int16_t* getAttackPercent() { return AttackPercent; }

	// wrappers that hide legacy implementation details.
	void ChangeStateEnd();
	void clearActionFunc() { user.ActorActionFunc = nullptr; }
	void setActionDecide();
	void setStateGroup(FName label, int substate = 0, bool noset = false);	// substate is only valid for Attack and CloseAttack
	bool checkStateGroup(FName label, int substate = 0);
	bool hasState(FName label, int substate = 0);
	void callAction();
	void callStateAction();
	void setPicFromState() { spr.setspritetexture(picFromState(user.__legacyState.State)); }
};

inline void UpdateChangeXY(DSWActor* actor)
{
	actor->user.change.XY() = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;
}

inline void UpdateChange(DSWActor* actor, double zfactor = 1.0)
{
	UpdateChangeXY(actor);
	actor->user.change.Z = actor->vel.Z * zfactor;
}


// subclassed to add a game specific actor() method

// Iterator wrappers that return an actor pointer, not an index.
using SWStatIterator = TStatIterator<DSWActor>;
using SWSectIterator = TSectIterator<DSWActor>;
using SWSpriteIterator = TSpriteIterator<DSWActor>;


END_SW_NS
