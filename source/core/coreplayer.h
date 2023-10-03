#pragma once

#include "d_net.h"
#include "packet.h"
#include "gameinput.h"

class DCorePlayer : public DObject
{
	DECLARE_CLASS(DCorePlayer, DObject)
	HAS_OBJECT_POINTERS
protected:
	DCorePlayer() = default;
	void Clear()
	{
		memset(&lastcmd, 0, sizeof(lastcmd));
		memset(&cmd, 0, sizeof(cmd));
		memset(&Angles, 0, sizeof(Angles));
		actor = nullptr;
		pnum = 0;
	}

public:
	ticcmd_t lastcmd, cmd;
	PlayerAngles Angles;
	DCoreActor* actor;
	uint8_t pnum;

	DCorePlayer(uint8_t p) : pnum(p) {}
	void OnDestroy() override { if (actor) actor->Destroy(); actor = nullptr; }
	virtual DCoreActor* GetActor() = 0;
	void Serialize(FSerializer& arc) override;
};

extern DCorePlayer* PlayerArray[MAXPLAYERS];

inline ESyncBits GetPersistentActions()
{
	return PlayerArray[myconnectindex]->cmd.ucmd.actions & SB_CENTERVIEW;
}
