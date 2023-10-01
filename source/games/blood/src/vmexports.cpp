//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2023 - Christoph Oelckers

This file is part of Raze

This is free software;you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation;either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "vm.h"
#include "ns.h"
#include "buildtiles.h"
#include "blood.h"
BEGIN_BLD_NS


DEFINE_FIELD_X(XSECTOR, XSECTOR, flags)
DEFINE_FIELD_X(XSECTOR, XSECTOR, flags2)
DEFINE_FIELD_X(XSECTOR, XSECTOR, marker0)
DEFINE_FIELD_X(XSECTOR, XSECTOR, marker1)
DEFINE_FIELD_X(XSECTOR, XSECTOR, basePath)
DEFINE_FIELD_X(XSECTOR, XSECTOR, actordata)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busy)
DEFINE_FIELD_X(XSECTOR, XSECTOR, offCeilZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, onCeilZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, offFloorZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, onFloorZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, windVel)
DEFINE_FIELD_X(XSECTOR, XSECTOR, data)
DEFINE_FIELD_X(XSECTOR, XSECTOR, txID)
DEFINE_FIELD_X(XSECTOR, XSECTOR, rxID)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyTimeA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, waitTimeA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, panAngle)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyTimeB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, waitTimeB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, windAng)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobTheta)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobSpeed)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyWaveA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyWaveB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, command)
DEFINE_FIELD_X(XSECTOR, XSECTOR, amplitude)
DEFINE_FIELD_X(XSECTOR, XSECTOR, freq)
DEFINE_FIELD_X(XSECTOR, XSECTOR, phase)
DEFINE_FIELD_X(XSECTOR, XSECTOR, wave)
DEFINE_FIELD_X(XSECTOR, XSECTOR, shade)
DEFINE_FIELD_X(XSECTOR, XSECTOR, panVel)
DEFINE_FIELD_X(XSECTOR, XSECTOR, Depth)
DEFINE_FIELD_X(XSECTOR, XSECTOR, Key)
DEFINE_FIELD_X(XSECTOR, XSECTOR, ceilpal)
DEFINE_FIELD_X(XSECTOR, XSECTOR, damageType)
DEFINE_FIELD_X(XSECTOR, XSECTOR, floorpal)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobZRange)

DEFINE_FIELD_X(XWALL, XWALL, flags)
DEFINE_FIELD_X(XWALL, XWALL, busy)
DEFINE_FIELD_X(XWALL, XWALL, data)
DEFINE_FIELD_X(XWALL, XWALL, txID)
DEFINE_FIELD_X(XWALL, XWALL, rxID)
DEFINE_FIELD_X(XWALL, XWALL, busyTime)
DEFINE_FIELD_X(XWALL, XWALL, waitTime)
DEFINE_FIELD_X(XWALL, XWALL, command)
DEFINE_FIELD_NAMED_X(XWALL, XWALL, panVel.X, panVelX) // VM does not support int vectors.
DEFINE_FIELD_NAMED_X(XWALL, XWALL, panVel.Y, panVelY)
DEFINE_FIELD_X(XWALL, XWALL, key)


void Blood_ChangeType(DBloodActor* self, PClassActor* type)
{
	self->ChangeType(type);
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, ChangeType, Blood_ChangeType)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_POINTER(type, PClassActor);
	self->ChangeType(type);
	return 0;
}

END_BLD_NS
