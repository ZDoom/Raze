/*
** d_protocol.cpp
** Basic network packet creation routines and simple IFF parsing
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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


#include "d_protocol.h"
#include "d_net.h"
#include "i_protocol.h"
#include "cmdlib.h"
#include "serializer.h"

extern int gametic;

void UnpackUserCmd (InputPacket *ucmd, const InputPacket *basis, TArrayView<uint8_t>& stream)
{
	uint8_t flags;

	if (basis != NULL)
	{
		if (basis != ucmd)
		{
			memcpy (ucmd, basis, sizeof(InputPacket));
		}
	}
	else
	{
		memset (ucmd, 0, sizeof(InputPacket));
	}

	flags = ReadInt8 (stream);

	if (flags)
	{
		// We can support up to 29 buttons, using from 0 to 4 bytes to store them.
		if (flags & UCMDF_BUTTONS)
			ucmd->actions = ESyncBits::FromInt(ReadInt32(stream));
		if (flags & UCMDF_PITCH)
			ucmd->ang.Pitch = DAngle::fromDeg(ReadFloat(stream));
		if (flags & UCMDF_YAW)
			ucmd->ang.Yaw = DAngle::fromDeg(ReadFloat(stream));
		if (flags & UCMDF_FORWARDMOVE)
			ucmd->vel.X = ReadFloat(stream);
		if (flags & UCMDF_SIDEMOVE)
			ucmd->vel.Y = ReadFloat(stream);
		if (flags & UCMDF_UPMOVE)
			ucmd->vel.Z = ReadFloat(stream);
		if (flags & UCMDF_ROLL)
			ucmd->ang.Roll = DAngle::fromDeg(ReadFloat(stream));
	}
}

void PackUserCmd (const InputPacket *ucmd, const InputPacket *basis, TArrayView<uint8_t>& stream)
{
	uint8_t flags = 0;
	auto packingBitsView = TArrayView(stream.Data(), 1);
	InputPacket blank;

	if (basis == NULL)
	{
		memset (&blank, 0, sizeof(blank));
		basis = &blank;
	}

	WriteInt8 (0, stream);			// Make room for the packing bits

	if (ucmd->actions != basis->actions)
	{
		flags |= UCMDF_BUTTONS;
		WriteInt32(ucmd->actions, stream);
	}
	if (ucmd->ang.Pitch != basis->ang.Pitch)
	{
		flags |= UCMDF_PITCH;
		WriteFloat ((float)ucmd->ang.Pitch.Degrees(), stream);
	}
	if (ucmd->ang.Yaw != basis->ang.Yaw)
	{
		flags |= UCMDF_YAW;
		WriteFloat ((float)ucmd->ang.Yaw.Degrees(), stream);
	}
	if (ucmd->vel.X != basis->vel.X)
	{
		flags |= UCMDF_FORWARDMOVE;
		WriteFloat ((float)ucmd->vel.X, stream);
	}
	if (ucmd->vel.Y != basis->vel.Y)
	{
		flags |= UCMDF_SIDEMOVE;
		WriteFloat ((float)ucmd->vel.Y, stream);
	}
	if (ucmd->vel.Z != basis->vel.Z)
	{
		flags |= UCMDF_UPMOVE;
		WriteFloat ((float)ucmd->vel.Z, stream);
	}
	if (ucmd->ang.Roll != basis->ang.Roll)
	{
		flags |= UCMDF_ROLL;
		WriteFloat ((float)ucmd->ang.Roll.Degrees(), stream);
	}

	// Write the packing bits
	WriteInt8 (flags, packingBitsView);
}

FSerializer &Serialize(FSerializer &arc, const char *key, ticcmd_t &cmd, ticcmd_t *def)
{
	if (arc.BeginObject(key))
	{
		arc("consistency", cmd.consistency)
			("ucmd", cmd.ucmd)
			.EndObject();
	}
	return arc;
}

FSerializer &Serialize(FSerializer &arc, const char *key, InputPacket &cmd, InputPacket *def)
{
	if (arc.BeginObject(key))
	{
		arc("actions", cmd.actions)
			("vel", cmd.vel)
			("ang", cmd.ang)
			.EndObject();
	}
	return arc;
}

void WriteUserCmdMessage (InputPacket *ucmd, const InputPacket *basis, TArrayView<uint8_t>& stream)
{
	if (basis == NULL)
	{
		if (ucmd->actions != 0 || !ucmd->vel.isZero() || !ucmd->ang.isZero())
		{
			WriteInt8 (DEM_USERCMD, stream);
			PackUserCmd (ucmd, basis, stream);
			return;
		}
	}
	else if (ucmd->actions != basis->actions || ucmd->vel != basis->vel || ucmd->ang != basis->ang)
	{
		WriteInt8 (DEM_USERCMD, stream);
		PackUserCmd (ucmd, basis, stream);
		return;
	}

	WriteInt8 (DEM_EMPTYUSERCMD, stream);
}


void SkipTicCmd (TArrayView<uint8_t>& stream, int count)
{
	for (int i = count; i > 0; i--)
	{
		bool moreticdata = true;

		AdvanceStream(stream, 2);		// Skip consistancy marker
		while (moreticdata)
		{
			uint8_t type = ReadInt8(stream);

			if (type == DEM_USERCMD)
			{
				moreticdata = false;
				int flags = ReadInt8(stream);
				size_t skip = 0;
				if (flags & UCMDF_PITCH)		skip += 4;
				if (flags & UCMDF_YAW)			skip += 4;
				if (flags & UCMDF_FORWARDMOVE)	skip += 4;
				if (flags & UCMDF_SIDEMOVE)		skip += 4;
				if (flags & UCMDF_UPMOVE)		skip += 4;
				if (flags & UCMDF_ROLL)			skip += 4;
				if (flags & UCMDF_BUTTONS)		skip += 4;
				AdvanceStream(stream, skip);
			}
			else if (type == DEM_EMPTYUSERCMD)
			{
				moreticdata = false;
			}
			else
			{
				Net_SkipCommand (type, stream);
			}
		}
	}
}

extern short consistency[MAXPLAYERS][BACKUPTICS];
void ReadTicCmd (TArrayView<uint8_t>& stream, int player, int tic)
{
	int type;
	uint8_t *start;
	ticcmd_t *tcmd;

	int ticmod = tic % BACKUPTICS;

	tcmd = &netcmds[player][ticmod];
	tcmd->consistency = ReadInt16 (stream);

	start = stream.Data();

	while ((type = ReadInt8 (stream)) != DEM_USERCMD && type != DEM_EMPTYUSERCMD)
		Net_SkipCommand (type, stream);

	NetSpecs[player][ticmod].SetData (start, int(stream.Data() - start - 1));

	if (type == DEM_USERCMD)
	{
		UnpackUserCmd (&tcmd->ucmd,
			tic ? &netcmds[player][(tic-1)%BACKUPTICS].ucmd : NULL, stream);
	}
	else
	{
		if (tic)
		{
			memcpy (&tcmd->ucmd, &netcmds[player][(tic-1)%BACKUPTICS].ucmd, sizeof(tcmd->ucmd));
		}
		else
		{
			memset (&tcmd->ucmd, 0, sizeof(tcmd->ucmd));
		}
	}
#if 0
	if (player==consoleplayer&&tic>BACKUPTICS)
		assert(consistancy[player][ticmod] == tcmd->consistancy);
#endif
}

void RunNetSpecs (int player, int buf)
{
	if (gametic % ticdup == 0)
	{
		auto stream = NetSpecs[player][buf].GetTArrayView ();
		if (stream.Size())
		{
			while (stream.Size())
			{
				int type = ReadInt8 (stream);
				try
				{
					Net_DoCommand(type, stream, player);
				}
				catch (...)
				{
					NetSpecs[player][buf].SetData(NULL, 0);
					throw;
				}
			}
#if 0
			if (!demorecording)
#endif
				NetSpecs[player][buf].SetData (NULL, 0);
		}
	}
}

uint8_t *lenspot = nullptr;

// Write the header of an IFF chunk and leave space
// for the length field.
void StartChunk (int id, TArrayView<uint8_t>& stream)
{
	WriteInt32(id, stream);
	lenspot = stream.Data();
	AdvanceStream(stream, 4);
}

// Write the length field for the chunk and insert
// pad byte if the chunk is odd-sized.
void FinishChunk (TArrayView<uint8_t>& stream)
{
	if (!lenspot)
		return;

	int len = int(stream.Data() - lenspot - 4);
	auto lenSpotView = TArrayView<uint8_t>(lenspot, 4);
	WriteInt32 (len, lenSpotView);
	if (len & 1)
		WriteInt8 (0, stream);

	lenspot = NULL;
}

// Skip past an unknown chunk. stream should be
// pointing to the chunk's length field.
void SkipChunk (TArrayView<uint8_t>& stream)
{
	int len = ReadInt32(stream);
	AdvanceStream(stream, len + (len & 1));
}
