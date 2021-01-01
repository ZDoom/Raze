#pragma once

#include "d_protocol.h"
#include "packet.h"

struct ticcmd_t
{
	InputPacket	ucmd;
	uint16_t	consistency;	// checks for net game
};

