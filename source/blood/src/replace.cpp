//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "compat.h"
#include "common_game.h"
#include "crc32.h"

#include "globals.h"
#include "screen.h"

int qanimateoffs(int a1, int a2)
{
    int offset = 0;
    if (a1 >= 0 && a1 < kMaxTiles)
    {
        int frames = picanm[a1].num;
        if (frames > 0)
        {
            int vd;
            if ((a2&0xc000) == 0x8000)
                vd = (Bcrc32(&a2, 2, 0)+gFrameClock)>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
            else
                vd = gFrameClock>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
            switch (picanm[a1].sf&PICANM_ANIMTYPE_MASK)
            {
            case PICANM_ANIMTYPE_OSC:
                offset = vd % (2*frames);
                if (offset >= frames)
                    offset = 2*frames-offset;
                break;
            case PICANM_ANIMTYPE_FWD:
                offset = vd % (frames+1);
                break;
            case PICANM_ANIMTYPE_BACK:
                offset = -(vd % (frames+1));
                break;
            }
        }
    }
    return offset;
}

void qloadpalette()
{
    scrLoadPalette();
}

int32_t qgetpalookup(int32_t a1, int32_t a2)
{
    if (gFogMode)
        return ClipHigh(a1 >> 8, 15) * 16 + ClipRange(a2, 0, 15);
    else
        return ClipRange((a1 >> 8) + a2, 0, 63);
}