#pragma once

#include "d_net.h"
#include "packet.h"
#include "gameinput.h"

struct CorePlayer
{
	ticcmd_t lastcmd, cmd;
	PlayerAngles Angles;
	DCoreActor* actor;

	virtual ~CorePlayer() { if (actor) actor->Destroy(); };
	virtual DCoreActor* GetActor() = 0;
};

extern CorePlayer* PlayerArray[MAXPLAYERS];
