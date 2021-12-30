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
#include "cmdlib.h"
#include "serializer.h"

extern int gametic;


char *ReadString (uint8_t **stream)
{
	char *string = *((char **)stream);

	*stream += strlen (string) + 1;
	return copystring (string);
}

const char *ReadStringConst(uint8_t **stream)
{
	const char *string = *((const char **)stream);
	*stream += strlen (string) + 1;
	return string;
}

int ReadByte (uint8_t **stream)
{
	uint8_t v = **stream;
	*stream += 1;
	return v;
}

int ReadWord (uint8_t **stream)
{
	short v = (((*stream)[0]) << 8) | (((*stream)[1]));
	*stream += 2;
	return v;
}

int ReadLong (uint8_t **stream)
{
	int v = (((*stream)[0]) << 24) | (((*stream)[1]) << 16) | (((*stream)[2]) << 8) | (((*stream)[3]));
	*stream += 4;
	return v;
}

float ReadFloat (uint8_t **stream)
{
	union
	{
		int i;
		float f;
	} fakeint;
	fakeint.i = ReadLong (stream);
	return fakeint.f;
}

void WriteString (const char *string, uint8_t **stream)
{
	char *p = *((char **)stream);

	while (*string) {
		*p++ = *string++;
	}

	*p++ = 0;
	*stream = (uint8_t *)p;
}


void WriteByte (uint8_t v, uint8_t **stream)
{
	**stream = v;
	*stream += 1;
}

void WriteWord (short v, uint8_t **stream)
{
	(*stream)[0] = v >> 8;
	(*stream)[1] = v & 255;
	*stream += 2;
}

void WriteLong (int v, uint8_t **stream)
{
	(*stream)[0] = v >> 24;
	(*stream)[1] = (v >> 16) & 255;
	(*stream)[2] = (v >> 8) & 255;
	(*stream)[3] = v & 255;
	*stream += 4;
}

void WriteFloat (float v, uint8_t **stream)
{
	union
	{
		int i;
		float f;
	} fakeint;
	fakeint.f = v;
	WriteLong (fakeint.i, stream);
}

// Returns the number of bytes read
int UnpackUserCmd (InputPacket *ucmd, const InputPacket *basis, uint8_t **stream)
{
	uint8_t *start = *stream;
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

	flags = ReadByte (stream);

	if (flags)
	{
		// We can support up to 29 buttons, using from 0 to 4 bytes to store them.
		if (flags & UCMDF_BUTTONS)
			ucmd->actions = ESyncBits::FromInt(ReadLong(stream));
		if (flags & UCMDF_PITCH)
			ucmd->horz = ReadFloat(stream);
		if (flags & UCMDF_YAW)
			ucmd->avel = ReadFloat(stream);
		if (flags & UCMDF_FORWARDMOVE)
			ucmd->fvel = ReadWord (stream);
		if (flags & UCMDF_SIDEMOVE)
			ucmd->svel = ReadWord (stream);
	}

	return int(*stream - start);
}

// Returns the number of bytes written
int PackUserCmd (const InputPacket *ucmd, const InputPacket *basis, uint8_t **stream)
{
	uint8_t flags = 0;
	uint8_t *temp = *stream;
	uint8_t *start = *stream;
	InputPacket blank;

	if (basis == NULL)
	{
		memset (&blank, 0, sizeof(blank));
		basis = &blank;
	}

	WriteByte (0, stream);			// Make room for the packing bits

	if (ucmd->actions != basis->actions)
	{
		flags |= UCMDF_BUTTONS;
		WriteLong(ucmd->actions, stream);
	}
	if (ucmd->horz != basis->horz)
	{
		flags |= UCMDF_PITCH;
		WriteFloat (ucmd->horz, stream);
	}
	if (ucmd->avel != basis->avel)
	{
		flags |= UCMDF_YAW;
		WriteFloat (ucmd->avel, stream);
	}
	if (ucmd->fvel != basis->fvel)
	{
		flags |= UCMDF_FORWARDMOVE;
		WriteWord (ucmd->fvel, stream);
	}
	if (ucmd->svel != basis->svel)
	{
		flags |= UCMDF_SIDEMOVE;
		WriteWord (ucmd->svel, stream);
	}

	// Write the packing bits
	WriteByte (flags, &temp);

	return int(*stream - start);
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
			("horz", cmd.horz)
			("avel", cmd.avel)
			("fvel", cmd.fvel)
			("svwl", cmd.svel)
			.EndObject();
	}
	return arc;
}

int WriteUserCmdMessage (InputPacket *ucmd, const InputPacket *basis, uint8_t **stream)
{
	if (basis == NULL)
	{
		if (ucmd->actions != 0 ||
			ucmd->horz != 0 ||
			ucmd->avel != 0 ||
			ucmd->fvel != 0 ||
			ucmd->svel != 0)
		{
			WriteByte (DEM_USERCMD, stream);
			return PackUserCmd (ucmd, basis, stream) + 1;
		}
	}
	else
	if (ucmd->actions != basis->actions ||
		ucmd->horz != basis->horz ||
		ucmd->avel != basis->avel ||
		ucmd->fvel != basis->fvel ||
		ucmd->svel != basis->svel)
	{
		WriteByte (DEM_USERCMD, stream);
		return PackUserCmd (ucmd, basis, stream) + 1;
	}

	WriteByte (DEM_EMPTYUSERCMD, stream);
	return 1;
}


int SkipTicCmd (uint8_t **stream, int count)
{
	int i, skip;
	uint8_t *flow = *stream;

	for (i = count; i > 0; i--)
	{
		bool moreticdata = true;

		flow += 2;		// Skip consistancy marker
		while (moreticdata)
		{
			uint8_t type = *flow++;

			if (type == DEM_USERCMD)
			{
				moreticdata = false;
				skip = 1;
				if (*flow & UCMDF_PITCH)		skip += 4;
				if (*flow & UCMDF_YAW)			skip += 4;
				if (*flow & UCMDF_FORWARDMOVE)	skip += 2;
				if (*flow & UCMDF_SIDEMOVE)		skip += 2;
				if (*flow & UCMDF_UPMOVE)		skip += 4;
				if (*flow & UCMDF_ROLL)			skip += 4;
				if (*flow & UCMDF_BUTTONS)		skip += 4;
				flow += skip;
			}
			else if (type == DEM_EMPTYUSERCMD)
			{
				moreticdata = false;
			}
			else
			{
				Net_SkipCommand (type, &flow);
			}
		}
	}

	skip = int(flow - *stream);
	*stream = flow;

	return skip;
}

extern short consistency[MAXPLAYERS][BACKUPTICS];
void ReadTicCmd (uint8_t **stream, int player, int tic)
{
	int type;
	uint8_t *start;
	ticcmd_t *tcmd;

	int ticmod = tic % BACKUPTICS;

	tcmd = &netcmds[player][ticmod];
	tcmd->consistency = ReadWord (stream);

	start = *stream;

	while ((type = ReadByte (stream)) != DEM_USERCMD && type != DEM_EMPTYUSERCMD)
		Net_SkipCommand (type, stream);

	NetSpecs[player][ticmod].SetData (start, int(*stream - start - 1));

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
	uint8_t *stream;
	int len;

	if (gametic % ticdup == 0)
	{
		stream = NetSpecs[player][buf].GetData (&len);
		if (stream)
		{
			uint8_t *end = stream + len;
			while (stream < end)
			{
				int type = ReadByte (&stream);
				try
				{
					Net_DoCommand(type, &stream, player);
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

uint8_t *lenspot;

// Write the header of an IFF chunk and leave space
// for the length field.
void StartChunk (int id, uint8_t **stream)
{
	WriteLong (id, stream);
	lenspot = *stream;
	*stream += 4;
}

// Write the length field for the chunk and insert
// pad byte if the chunk is odd-sized.
void FinishChunk (uint8_t **stream)
{
	int len;

	if (!lenspot)
		return;

	len = int(*stream - lenspot - 4);
	WriteLong (len, &lenspot);
	if (len & 1)
		WriteByte (0, stream);

	lenspot = NULL;
}

// Skip past an unknown chunk. *stream should be
// pointing to the chunk's length field.
void SkipChunk (uint8_t **stream)
{
	int len;

	len = ReadLong (stream);
	*stream += len + (len & 1);
}
