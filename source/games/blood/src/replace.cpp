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
#include "ns.h"	// Must come before everything else!

#include "blood.h"
#include "m_crc32.h"


BEGIN_BLD_NS

int qanimateoffs(int a1, int a2)
{
    int offset = 0;
    if (a1 >= 0 && a1 < kMaxTiles)
    {
        int frames = picanm[a1].num;
        if (frames > 0)
        {
            int const frameClock = PlayClock;
            int vd;
            if ((a2&0xc000) == 0x8000)
                vd = (Bcrc32(&a2, 2, 0)+frameClock)>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
            else
                vd = frameClock>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
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

END_BLD_NS
