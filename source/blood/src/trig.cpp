#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "pragmas.h"
#include "common_game.h"
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
#include "build.h"
#include "pragmas.h"
#include "common_game.h"
#include "trig.h"

BEGIN_BLD_NS

int costable[2048];

int OctantTable[8] = { 5, 6, 2, 1, 4, 7, 3, 0 };

int GetOctant(int x, int y)
{
    int vc = klabs(x)-klabs(y);
    return OctantTable[7-(x<0)-(y<0)*2-(vc<0)*4];
}

void RotateVector(int *dx, int *dy, int nAngle)
{
    int ox = *dx;
    int oy = *dy;
    *dx = dmulscale30r(ox, Cos(nAngle), -oy, Sin(nAngle));
    *dy = dmulscale30r(ox, Sin(nAngle), oy, Cos(nAngle));
}

void RotatePoint(int *x, int *y, int nAngle, int ox, int oy)
{
    int dx = *x-ox;
    int dy = *y-oy;
    *x = ox+dmulscale30r(dx, Cos(nAngle), -dy, Sin(nAngle));
    *y = oy+dmulscale30r(dx, Sin(nAngle), dy, Cos(nAngle));
}

void trigInit()
{
    auto fr = fileSystem.OpenFileReader("cosine.dat");
    auto len = fr.Read(costable, 2048);
    if (len != 2048)
        ThrowError("Cosine table incorrect size");
#if B_BIG_ENDIAN == 1
    for (int i = 0; i < 512; i++)
    {
        costable[i] = B_LITTLE32(costable[i]);
    }
#endif
    costable[512] = 0;
    for (int i = 513; i <= 1024; i++)
    {
        costable[i] = -costable[1024-i];
    }
    for (int i = 1025; i < 2048; i++)
    {
        costable[i] = costable[2048 - i];
    }
}

END_BLD_NS
