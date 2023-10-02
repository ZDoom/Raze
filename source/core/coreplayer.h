#pragma once

#include "packet.h"
#include "gameinput.h"

struct CorePlayer
{
	InputPacket input;
	PlayerAngles Angles;
	DCoreActor* actor;

	virtual DCoreActor* GetActor() = 0;
};
