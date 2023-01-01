//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software, you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program, if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source, 1996 - Todd Replogle
Prepared for public release, 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"

BEGIN_DUKE_NS

void initactorflags_r()
{
	gs.weaponsandammosprites[0] = RedneckCrossbowClass;
	gs.weaponsandammosprites[1] = RedneckRiflegunClass;
	gs.weaponsandammosprites[2] = RedneckBlasterammoClass;
	gs.weaponsandammosprites[3] = RedneckDynamiteAmmoClass;
	gs.weaponsandammosprites[4] = RedneckDynamiteAmmoClass;
	gs.weaponsandammosprites[5] = RedneckCowpieClass;
	gs.weaponsandammosprites[6] = RedneckWhiskeyClass;
	gs.weaponsandammosprites[7] = RedneckPorkRindsClass;
	gs.weaponsandammosprites[8] = RedneckMoonshineClass;
	gs.weaponsandammosprites[9] = RedneckDynamiteAmmoClass;
	gs.weaponsandammosprites[10] = RedneckDynamiteAmmoClass;
	gs.weaponsandammosprites[11] = RedneckCrossbowClass;
	gs.weaponsandammosprites[12] = RedneckDynamiteAmmoClass;
	gs.weaponsandammosprites[13] = RedneckTitgunClass;
	gs.weaponsandammosprites[14] = RedneckTitAmmoClass;

	gs.gutsscale = 0.125;
}

END_DUKE_NS
