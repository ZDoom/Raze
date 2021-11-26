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
#include "automap.h"
#include "v_draw.h"

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
    if (sector[initsect].ceilingpal != 3 || (PlayerList[nLocalPlayer].nTorch != 0)) {
        MarkSectorSeen(initsect);
    }
}

void DrawMap(double const smoothratio)
{
    if (!nFreeze && automapMode != am_off) 
    {
        auto pPlayerActor = PlayerList[nLocalPlayer].Actor();
        auto psp = &pPlayerActor->s();

        int x = psp->interpolatedx(smoothratio);
        int y = psp->interpolatedy(smoothratio);
        int ang = (!SyncInput() ? PlayerList[nLocalPlayer].angle.sum() : PlayerList[nLocalPlayer].angle.interpolatedsum(smoothratio)).asbuild();
        DrawOverheadMap(x, y, ang, smoothratio);
    }
}

template<typename T> void GetSpriteExtents(T const* const pSprite, int* top, int* bottom)
{
    *top = *bottom = pSprite->z;
    if ((pSprite->cstat & 0x30) != 0x20)
    {
        int height = tileHeight(pSprite->picnum);
        int center = height / 2 + tileTopOffset(pSprite->picnum);
        *top -= (pSprite->yrepeat << 2) * center;
        *bottom += (pSprite->yrepeat << 2) * (height - center);
    }
}

bool GameInterface::DrawAutomapPlayer(int mx, int my, int x, int y, int z, int a, double const smoothratio)
{
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
		auto pPlayerActor = PlayerList[i].Actor();
        spritetype* pSprite = &pPlayerActor->s();

        int xvect = -bsin(a) * z;
        int yvect = -bcos(a) * z;
        int ox = mx - x;
        int oy = my - y;
        int x1 = DMulScale(ox, xvect, -oy, yvect, 16);
        int y1 = DMulScale(oy, xvect, ox, yvect, 16);
        int xx = xdim / 2. + x1 / 4096.;
        int yy = ydim / 2. + y1 / 4096.;

        if (i == nLocalPlayer)// || gGameOptions.nGameType == 1)
        {
            int nTile = pSprite->picnum;
            int ceilZ, ceilHit, floorZ, floorHit;
            getzrange(&pSprite->pos, pSprite->sectnum, &ceilZ, &ceilHit, &floorZ, &floorHit, (pSprite->clipdist << 2) + 16, CLIPMASK0);
            int nTop, nBottom;
            GetSpriteExtents(pSprite, &nTop, &nBottom);
            int nScale = (pSprite->yrepeat + ((floorZ - nBottom) >> 8)) * z;
            nScale = clamp(nScale, 8000, 65536 << 1);
            // Players on automap
            double x = xdim / 2. + x1 / double(1 << 12);
            double y = ydim / 2. + y1 / double(1 << 12);
            // This very likely needs fixing later
            DrawTexture(twod, tileGetTexture(nTile /*+ ((PlayClock >> 4) & 3)*/, true), xx, yy, DTA_ClipLeft, windowxy1.x, DTA_ClipTop, windowxy1.y, DTA_ScaleX, z / 1536., DTA_ScaleY, z / 1536., DTA_CenterOffset, true,
                DTA_ClipRight, windowxy2.x + 1, DTA_ClipBottom, windowxy2.y + 1, DTA_Alpha, (pSprite->cstat & 2 ? 0.5 : 1.), TAG_DONE);
            break;
        }
    }
    return true;
}

END_PS_NS
