//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include <string.h>
#include "player.h"
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "v_2ddrawer.h"

BEGIN_PS_NS


short bShowTowers = false;

void GrabMap()
{
    for (int i = 0; i < numsectors; i++) {
        MarkSectorSeen(i);
    }
}


void UpdateMap()
{
    if (sector[initsect].ceilingpal != 3 || (nPlayerTorch[nLocalPlayer] != 0)) {
        MarkSectorSeen(initsect);
    }
}

void DrawMap()
{
    if (!nFreeze && automapMode != am_off) 
    {
        DrawOverheadMap(initx, inity, inita);
    }
}
END_PS_NS
