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
#include "engine.h"
#ifndef __WATCOMC__
#include <cstdlib>
#include <cmath>
#else
#include <stdlib.h>
#endif

BEGIN_PS_NS

// 100% done
int AngleDiff(DAngle a, DAngle b)
{
    int diff = (b - a).Buildang();

    if (diff > 1024) {
        diff = 2048 - diff;
    }
    return diff;
}

int AngleDelta(int a, int b, int c)
{
    int diff = b - a;

    if (diff >= 0)
    {
        if (diff > 1024) {
            diff = -(2048 - diff);
        }
    }
    else if (diff < -1024)
    {
        diff += 2048;
    }

    if (abs(diff) > c)
    {
        if (diff < 0) {
            return -diff;
        }

        diff = c;
    }
    return diff;
}
END_PS_NS
