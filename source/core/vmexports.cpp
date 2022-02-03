/*
** vmexports.cpp
**
** global script exports
**
**---------------------------------------------------------------------------
** Copyright 2022 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "maptypes.h"
#include "vm.h"

//=============================================================================
//
// internal sector struct - no longer identical with on-disk format
//
//=============================================================================

DEFINE_FIELD_NAMED_X(sectortype, sectortype, ceilingxpan_, ceilingxpan)
DEFINE_FIELD_NAMED_X(sectortype, sectortype, ceilingypan_, ceilingypan)
DEFINE_FIELD_NAMED_X(sectortype, sectortype, floorxpan_, floorxpan)
DEFINE_FIELD_NAMED_X(sectortype, sectortype, floorypan_, floorypan)
DEFINE_FIELD_X(sectortype, sectortype, wallptr)
DEFINE_FIELD_X(sectortype, sectortype, wallnum)
DEFINE_FIELD_X(sectortype, sectortype, ceilingstat)
DEFINE_FIELD_X(sectortype, sectortype, floorstat)
DEFINE_FIELD_X(sectortype, sectortype, lotag)
DEFINE_FIELD_X(sectortype, sectortype, type)
DEFINE_FIELD_X(sectortype, sectortype, hitag)
DEFINE_FIELD_X(sectortype, sectortype, extra)
DEFINE_FIELD_X(sectortype, sectortype, ceilingshade)
DEFINE_FIELD_X(sectortype, sectortype, ceilingpal)
DEFINE_FIELD_X(sectortype, sectortype, floorshade)
DEFINE_FIELD_X(sectortype, sectortype, floorpal)
DEFINE_FIELD_X(sectortype, sectortype, visibility)
DEFINE_FIELD_X(sectortype, sectortype, fogpal)
DEFINE_FIELD_X(sectortype, sectortype, exflags)
DEFINE_FIELD_X(sectortype, sectortype, floorz)
DEFINE_FIELD_X(sectortype, sectortype, ceilingz)

DEFINE_FIELD_NAMED_X(walltype, walltype, xpan_, xpan)
DEFINE_FIELD_NAMED_X(walltype, walltype, ypan_, ypan)
DEFINE_FIELD_X(walltype, walltype, pos)
DEFINE_FIELD_X(walltype, walltype, point2)
DEFINE_FIELD_X(walltype, walltype, nextwall)
DEFINE_FIELD_X(walltype, walltype, sector)
DEFINE_FIELD_X(walltype, walltype, nextsector)
DEFINE_FIELD_X(walltype, walltype, cstat)
DEFINE_FIELD_X(walltype, walltype, lotag)
DEFINE_FIELD_X(walltype, walltype, type)
DEFINE_FIELD_X(walltype, walltype, hitag)
DEFINE_FIELD_X(walltype, walltype, extra)
DEFINE_FIELD_X(walltype, walltype, shade)
DEFINE_FIELD_X(walltype, walltype, pal)
DEFINE_FIELD_X(walltype, walltype, xrepeat)
DEFINE_FIELD_X(walltype, walltype, yrepeat)

DEFINE_FIELD_NAMED_X(tspritetype, tspritetype, sectp, sector)
DEFINE_FIELD_X(tspritetype, tspritetype, cstat)
DEFINE_FIELD_X(tspritetype, tspritetype, statnum)
DEFINE_FIELD_X(tspritetype, tspritetype, ang)
DEFINE_FIELD_X(tspritetype, tspritetype, xvel)
DEFINE_FIELD_X(tspritetype, tspritetype, yvel)
DEFINE_FIELD_X(tspritetype, tspritetype, zvel)
DEFINE_FIELD_X(tspritetype, tspritetype, lotag)
DEFINE_FIELD_X(tspritetype, tspritetype, hitag)
DEFINE_FIELD_X(tspritetype, tspritetype, extra)
DEFINE_FIELD_X(tspritetype, tspritetype, detail)
DEFINE_FIELD_X(tspritetype, tspritetype, shade)
DEFINE_FIELD_X(tspritetype, tspritetype, pal)
DEFINE_FIELD_X(tspritetype, tspritetype, clipdist)
DEFINE_FIELD_X(tspritetype, tspritetype, blend)
DEFINE_FIELD_X(tspritetype, tspritetype, xrepeat)
DEFINE_FIELD_X(tspritetype, tspritetype, yrepeat)
DEFINE_FIELD_X(tspritetype, tspritetype, xoffset)
DEFINE_FIELD_X(tspritetype, tspritetype, yoffset)
DEFINE_FIELD_X(tspritetype, tspritetype, ownerActor)
DEFINE_FIELD_X(tspritetype, tspritetype, time)

DEFINE_GLOBAL(wall)
DEFINE_GLOBAL(sector)

void sector_setfloorz(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setfloorz(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setfloorz, sector_setfloorz)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(z);
	self->setfloorz(z);
	return 0;
}

void sector_setceilingz(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setceilingz(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setceilingz, sector_setceilingz)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(z);
	self->setceilingz(z);
	return 0;
}

void sector_addfloorz(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addfloorz(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addfloorz, sector_addfloorz)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(z);
	self->addfloorz(z);
	return 0;
}

void sector_addceilingz(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addceilingz(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addceilingz, sector_addceilingz)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(z);
	self->addceilingz(z);
	return 0;
}

void sector_setfloorxpan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setfloorxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setfloorxpan, sector_setfloorxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(xpan);
	self->setfloorxpan(xpan);
	return 0;
}

void sector_setceilingxpan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setceilingxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setceilingxpan, sector_setceilingxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(xpan);
	self->setceilingxpan(xpan);
	return 0;
}

void sector_addfloorxpan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addfloorxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addfloorxpan, sector_addfloorxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(xpan);
	self->addfloorxpan(xpan);
	return 0;
}

void sector_addceilingxpan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addceilingxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addceilingxpan, sector_addceilingxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(xpan);
	self->addceilingxpan(xpan);
	return 0;
}

void sector_setfloorypan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setfloorypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setfloorypan, sector_setfloorypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(ypan);
	self->setfloorypan(ypan);
	return 0;
}

void sector_setceilingypan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setceilingypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setceilingypan, sector_setceilingypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(ypan);
	self->setceilingypan(ypan);
	return 0;
}

void sector_addfloorypan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addfloorypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addfloorypan, sector_addfloorypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(ypan);
	self->addfloorypan(ypan);
	return 0;
}

void sector_addceilingypan(sectortype* sect, double val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->addceilingypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, addceilingypan, sector_addceilingypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_FLOAT(ypan);
	self->addceilingypan(ypan);
	return 0;
}


void sector_setfloorslope(sectortype* sect, int val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setfloorslope(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setfloorslope, sector_setfloorslope)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_INT(slope);
	self->setfloorslope(slope);
	return 0;
}

void sector_setceilingslope(sectortype* sect, int val)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	sect->setceilingslope(val);
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, setceilingslope, sector_setceilingslope)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	PARAM_INT(slope);
	self->setceilingslope(slope);
	return 0;
}

int sector_floorslope(sectortype* sect)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	return sect->getfloorslope();
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, floorslope, sector_floorslope)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	ACTION_RETURN_INT(self->getfloorslope());
}

int sector_ceilingslope(sectortype* sect)
{
	if (!sect) ThrowAbortException(X_READ_NIL, nullptr);
	return sect->getceilingslope();
}

DEFINE_ACTION_FUNCTION_NATIVE(_sectortype, ceilingslope, sector_ceilingslope)
{
	PARAM_SELF_STRUCT_PROLOGUE(sectortype);
	ACTION_RETURN_INT(self->getceilingslope());
}


//=============================================================================

void wall_setxpan(walltype* wal, double val)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	wal->setxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, setxpan, wall_setxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	PARAM_FLOAT(xpan);
	self->setxpan(xpan);
	return 0;
}

void wall_addxpan(walltype* wal, double val)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	wal->addxpan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, addxpan, wall_addxpan)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	PARAM_FLOAT(xpan);
	self->addxpan(xpan);
	return 0;
}

void wall_setypan(walltype* wal, double val)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	wal->setypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, setypan, wall_setypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	PARAM_FLOAT(ypan);
	self->setypan(ypan);
	return 0;
}

void wall_addypan(walltype* wal, double val)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	wal->addypan(float(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, addypan, wall_addypan)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	PARAM_FLOAT(ypan);
	self->addypan(ypan);
	return 0;
}

sectortype* wall_nextsector(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->nextSector();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, nextsectorp, wall_nextsector)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_POINTER(self->nextSector());
}

sectortype* wall_sector(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->sectorp();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, sectorp, wall_sector)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_POINTER(self->sectorp());
}

walltype* wall_nextwall(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->nextWall();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, nextwallp, wall_nextwall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_POINTER(self->nextWall());
}

walltype* wall_lastwall(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->lastWall();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, lastwall, wall_lastwall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_POINTER(self->lastWall());
}

walltype* wall_point2wall(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->point2Wall();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, point2wall, wall_point2wall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_POINTER(self->point2Wall());
}

double wall_deltax(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->deltax();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, deltax, wall_point2wall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_FLOAT(self->deltax());
}

double wall_deltay(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->deltay();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, deltay, wall_point2wall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_FLOAT(self->deltay());
}

double wall_length(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->Length();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, length, wall_point2wall)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_FLOAT(self->Length());
}

void wall_move(walltype* wal, double x, double y)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	wal->movexy(x, y);
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, move, wall_move)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	self->movexy(x, y);
	return 0;
}

int wall_twosided(walltype* wal)
{
	if (!wal) ThrowAbortException(X_READ_NIL, nullptr);
	return wal->twoSided();
}

DEFINE_ACTION_FUNCTION_NATIVE(_walltype, twosided, wall_twosided)
{
	PARAM_SELF_STRUCT_PROLOGUE(walltype);
	ACTION_RETURN_BOOL(self->twoSided());
}
