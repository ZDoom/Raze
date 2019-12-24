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

#include "compat.h"
#include "duke3d.h"
#include "osdcmds.h"
#include "savegame.h"
#include "demo.h"

#include "menus.h"
#include "cheats.h"
#include "gamecvars.h"
#include "menu/menu.h"
#include "version.h"
#include "../../glbackend/glbackend.h"

BEGIN_RR_NS

#if 0


    case MENU_CREDITS31:
        l = 7;

        mgametextcenter(origin.x, origin.y + ((55-l)<<16), "Developer");
        creditsminitext(origin.x + (160<<16), origin.y + ((60+10-l)<<16), "Alexey \"Nuke.YKT\" Skrybykin", 8);

        mgametextcenter(origin.x, origin.y + ((85-l)<<16), "Tester & support");
        creditsminitext(origin.x + (160<<16), origin.y + ((90+10-l)<<16), "Sergey \"Maxi Clouds\" Skrybykin", 8);

        mgametextcenter(origin.x, origin.y + ((115-l)<<16), "Special thanks to");
        creditsminitext(origin.x + (160<<16), origin.y + ((120+10-l)<<16), "Evan \"Hendricks266\" Ramos", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+20-l)<<16), "Richard \"TerminX\" Gobeille", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+30-l)<<16), "\"NY00123\"", 8);
        creditsminitext(origin.x + (160<<16), origin.y + ((120+40-l)<<16), "\"MetHy\"", 8);

        break;


#endif

END_RR_NS
