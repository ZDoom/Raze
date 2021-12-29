#pragma once

// included by game.h

BEGIN_SW_NS


class DSWActor : public DCoreActor
{
	DECLARE_CLASS(DSWActor, DCoreActor)
	HAS_OBJECT_POINTERS

public:

	bool hasUser;
	USER user;
	walltype* tempwall;	// transient, to replace a hack using a 16 bit sprite field.
	TObjPtr<DSWActor*> ownerActor;

	DSWActor() = default;
	DSWActor& operator=(const DSWActor& other) = default;

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
};


// subclassed to add a game specific actor() method

// Iterator wrappers that return an actor pointer, not an index.
using SWStatIterator = TStatIterator<DSWActor>;
using SWSectIterator = TSectIterator<DSWActor>;
using SWSpriteIterator = TSpriteIterator<DSWActor>;


END_SW_NS
