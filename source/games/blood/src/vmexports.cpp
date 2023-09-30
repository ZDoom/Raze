//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2023 - Christoph Oelckers

This file is part of Raze

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "vm.h"
#include "ns.h"
#include "buildtiles.h"
#include "blood.h"
BEGIN_BLD_NS



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
