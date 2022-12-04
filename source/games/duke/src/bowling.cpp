//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_r.h"
#include "dukeactor.h"
#include "buildtiles.h"

BEGIN_DUKE_NS

void updatepindisplay(int tag, int pins)
{
	static const uint8_t pinx[] = { 64, 56, 72, 48, 64, 80, 40, 56, 72, 88 };
	static const uint8_t piny[] = { 48, 40, 40, 32, 32, 32, 24, 24, 24, 24 };

	if (tag < 1 || tag > 4) return;
	tag += RTILE_BOWLINGLANE1 - 1;
	if (TileFiles.tileMakeWritable(tag))
	{
		tileCopySection(RTILE_LANEPICBG, 0, 0, 128, 64, tag, 0, 0);
		for (int i = 0; i < 10; i++) if (pins & (1 << i))
			tileCopySection(RTILE_LANEPICS, 0, 0, 8, 8, tag, pinx[i] - 4, piny[i] - 10);
	}
}

void resetlanepics(void)
{
	if (!isRR()) return;
	for (int tag = 0; tag < 4; tag++)
	{
		int pic = tag + 1;
		if (pic == 0) continue;
		updatepindisplay(pic, 0xffff);
	}
}

END_DUKE_NS

