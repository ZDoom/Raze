//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "demo.h"
#include "screens.h"
#include "baselayer.h"
#include "cmdline.h"
#include "m_argv.h"

BEGIN_DUKE_NS

int32_t g_fakeMultiMode = 0;

void G_CheckCommandLine()
{
	auto val = Args->CheckValue("-skill");
	if (val)
	{
		ud.m_player_skill = ud.player_skill = clamp((int)strtol(val, nullptr, 0), 0, 5);
		if (ud.m_player_skill == 4) ud.m_respawn_monsters = ud.respawn_monsters = 1;
	}
	val = Args->CheckValue("-respawn");
	if (val)
	{
		if (*val == '1') ud.m_respawn_monsters = 1;
		else if (*val == '2') ud.m_respawn_items = 1;
		else if (*val == '3') ud.m_respawn_inventory = 1;
		else
		{
			ud.m_respawn_monsters = 1;
			ud.m_respawn_items = 1;
			ud.m_respawn_inventory = 1;
		}
		Printf("Respawn on.\n");
	}
}
END_DUKE_NS
