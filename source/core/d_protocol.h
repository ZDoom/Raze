/*
** d_protocol.h
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

#ifndef __D_PROTOCOL_H__
#define __D_PROTOCOL_H__

#include <stdint.h>
#include "packet.h"

// The IFF routines here all work with big-endian IDs, even if the host
// system is little-endian.
#define BIGE_ID(a,b,c,d)	((d)|((c)<<8)|((b)<<16)|((a)<<24))

#define FORM_ID		BIGE_ID('F','O','R','M')
#define ZDEM_ID		BIGE_ID('R','D','E','M')
#define ZDHD_ID		BIGE_ID('R','Z','H','D')
#define VARS_ID		BIGE_ID('V','A','R','S')
#define UINF_ID		BIGE_ID('U','I','N','F')
#define COMP_ID		BIGE_ID('C','O','M','P')
#define BODY_ID		BIGE_ID('B','O','D','Y')
#define NETD_ID		BIGE_ID('N','E','T','D')
#define WEAP_ID		BIGE_ID('W','E','A','P')

#define	ANGLE2SHORT(x)	((((x)/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*360)


struct zdemoheader_s {
	uint8_t	demovermajor;
	uint8_t	demoverminor;
	uint8_t	minvermajor;
	uint8_t	minverminor;
	uint8_t	map[8];
	unsigned int rngseed;
	uint8_t	consoleplayer;
};


// When transmitted, the above message is preceded by a uint8_t
// indicating which fields are actually present in the message.
enum
{
	UCMDF_BUTTONS		= 0x01,
	UCMDF_PITCH			= 0x02,
	UCMDF_YAW			= 0x04,
	UCMDF_FORWARDMOVE	= 0x08,
	UCMDF_SIDEMOVE		= 0x10,
	UCMDF_UPMOVE		= 0x20,
	UCMDF_ROLL			= 0x40,
};

// When changing the following enum, be sure to update Net_SkipCommand()
// and Net_DoCommand() in d_net.cpp.
enum EDemoCommand
{
	DEM_BAD,			//  0 Bad command
	DEM_USERCMD,
	DEM_EMPTYUSERCMD,
	DEM_GENERICCHEAT,
	DEM_GIVE,
	DEM_CHANGEMAP,
	DEM_ENDSCREENJOB,
	DEM_SAVEGAME,

	DEM_MAX
};

enum ECheat
{
	// must contain all cheats from all games
	CHT_NONE,
	CHT_GOD,
	CHT_GODON,
	CHT_GODOFF,
	CHT_NOCLIP,

	// Duke et.al.
	CHT_UNLOCK,
	CHT_CASHMAN,
	CHT_HYPER,
	CHT_KILL,
	CHT_BIKE,
	CHT_BOAT,
	CHT_TONY,
	CHT_VAN,
	CHT_RHETT,
	CHT_AARON,
	CHT_NOCHEAT,
	CHT_DRINK,
	CHT_SEASICK,
	CHT_KFC,
	CHT_MONSTERS,

	// Shadow Warrior
	CHT_FLY,

	// Blood 
	kCheatSatchel,
	kCheatKevorkian,
	kCheatMcGee,
	kCheatEdmark,
	kCheatKrueger,
	kCheatSterno,
	kCheat14, // quake effect, not used
	kCheatSpork,
	kCheatClarice,
	kCheatFrankenstein,
	kCheatCheeseHead,
	kCheatTequila,
	kCheatFunkyShoes,
	kCheatKeyMaster,
	kCheatOneRing,
	kCheatVoorhees,
	kCheatJoJo,
	kCheatGateKeeper,
	kCheatLaraCroft,
	kCheatHongKong,
	kCheatMontana,
	kCheatBunz,
	kCheatCousteau,
	kCheatForkYou,
	kCheatLieberMan,
	kCheatSpielberg,

	CHT_MAX
};

enum EGiveType
{
	GIVE_ALL,
	GIVE_AMMO,
	GIVE_ARMOR,
	GIVE_HEALTH,
	GIVE_INVENTORY,
	GIVE_ITEMS,
	GIVE_KEYS,
	GIVE_WEAPONS
};

typedef void(*NetCommandHandler)(int player, TArrayView<uint8_t>& stream, bool skip);

void Net_SetCommandHandler(EDemoCommand cmd, NetCommandHandler handler) noexcept;

void StartChunk (int id, TArrayView<uint8_t>& stream);
void FinishChunk (TArrayView<uint8_t>& stream);
void SkipChunk (TArrayView<uint8_t>& stream);

void UnpackUserCmd (InputPacket *ucmd, const InputPacket*basis, TArrayView<uint8_t>& stream);
void PackUserCmd (const InputPacket*ucmd, const InputPacket*basis, TArrayView<uint8_t>& stream);
void WriteUserCmdMessage (InputPacket*ucmd, const InputPacket*basis, TArrayView<uint8_t>& stream);

struct ticcmd_t;

void SkipTicCmd (TArrayView<uint8_t>& stream, int count);
void ReadTicCmd (TArrayView<uint8_t>& stream, int player, int tic);
void RunNetSpecs (int player, int buf);

#endif //__D_PROTOCOL_H__
