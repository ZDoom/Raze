//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// CONSOLE.C
// Handles all argument storing and user console variable modifications.
// Copyright (c) 1996 by Jim Norwood

#include "ns.h"

#include "build.h"

#include "mytypes.h"
#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"
#include "warp.h"
#include "quake.h"

#include "common_game.h"
#include "gamecontrol.h"
#include "trigger.h"

#include "savedef.h"
#include "menus.h"
#include "network.h"
#include "pal.h"

#include "weapon.h"
#include "text.h"
#include "jsector.h"

BEGIN_SW_NS

void CON_Bunny(void)
{
    PLAYERp pp = Player + myconnectindex;

    if (CommEnabled)
        return;

    pp->BunnyMode = !pp->BunnyMode;
    if (pp->BunnyMode)
        PutStringInfo(pp,"Bunny rockets enabled!");
    else
        PutStringInfo(pp,"Bunny rockets disabled!");
}

void CON_ShowMirror(void)
{
    char base[80];
    int16_t op1=0;

    // Format: showmirror [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help showmirror");
        return;
    }

    if (op1 < 0 || op1 > 9)
    {
        Printf("Mirror number is out of range!");
        return;
    }

    Printf("camera is the ST1 sprite used as the view spot");
    Printf("camspite is the SpriteNum of the drawtotile tile in editart");
    Printf("camspic is the tile number of the drawtotile in editart");
    Printf("iscamera is whether or not this mirror is a camera type");
    Printf(" ");
    Printf("mirror[%d].mirrorwall = %d",op1,mirror[op1].mirrorwall);
    Printf("mirror[%d].mirrorsector = %d",op1,mirror[op1].mirrorsector);
    Printf("mirror[%d].camera = %d",op1,mirror[op1].camera);
    Printf("mirror[%d].camsprite = %d",op1,mirror[op1].camsprite);
    Printf("mirror[%d].campic = %d",op1,mirror[op1].campic);
    Printf("mirror[%d].iscamera = %d",op1,mirror[op1].ismagic);
}


END_SW_NS
