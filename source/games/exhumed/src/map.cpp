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


bool bShowTowers = false;

void GrabMap()
{
    for(auto&sec: sector)
        MarkSectorSeen(&sec);
}


void UpdateMap()
{
    if (initsectp->ceilingpal != 3 || (PlayerList[nLocalPlayer].nTorch != 0)) {
        MarkSectorSeen(initsectp);
    }
}

void DrawMap(double const smoothratio)
{
    if (!nFreeze && automapMode != am_off) 
    {
        auto pPlayerActor = PlayerList[nLocalPlayer].pActor;

        int x = pPlayerActor->interpolatedx(smoothratio);
        int y = pPlayerActor->interpolatedy(smoothratio);
        int ang = (!SyncInput() ? PlayerList[nLocalPlayer].angle.sum() : PlayerList[nLocalPlayer].angle.interpolatedsum(smoothratio)).asbuild();
        DrawOverheadMap(x, y, ang, smoothratio);
    }
}

void GetActorExtents(DExhumedActor* actor, int* top, int* bottom)
{
    *top = *bottom = actor->int_pos().Z;
    if ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_FLOOR)
    {
        int height = tileHeight(actor->spr.picnum);
        int center = height / 2 + tileTopOffset(actor->spr.picnum);
        *top -= (actor->spr.yrepeat << 2) * center;
        *bottom += (actor->spr.yrepeat << 2) * (height - center);
    }
}

bool GameInterface::DrawAutomapPlayer(int mx, int my, int x, int y, int z, int a, double const smoothratio)
{
    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
		auto pPlayerActor = PlayerList[i].pActor;

        int xvect = -bsin(a) * z;
        int yvect = -bcos(a) * z;
        int ox = mx - x;
        int oy = my - y;
        int x1 = DMulScale(ox, xvect, -oy, yvect, 16);
        int y1 = DMulScale(oy, xvect, ox, yvect, 16);
        int xx = twod->GetWidth() / 2. + x1 / 4096.;
        int yy = twod->GetHeight() / 2. + y1 / 4096.;

        if (i == nLocalPlayer)// || gGameOptions.nGameType == 1)
        {
            int nTile = pPlayerActor->spr.picnum;
            int ceilZ, floorZ;
            Collision ceilHit, floorHit;
            getzrange(pPlayerActor->int_pos(), pPlayerActor->sector(), &ceilZ, ceilHit, &floorZ, floorHit, (pPlayerActor->spr.clipdist << 2) + 16, CLIPMASK0);
            int nTop, nBottom;
            GetActorExtents(pPlayerActor, &nTop, &nBottom);
            int nScale = (pPlayerActor->spr.yrepeat + ((floorZ - nBottom) >> 8)) * z;
            nScale = clamp(nScale, 8000, 65536 << 1);
            // This very likely needs fixing later
            DrawTexture(twod, tileGetTexture(nTile /*+ ((PlayClock >> 4) & 3)*/, true), xx, yy, DTA_ClipLeft, viewport3d.Left(), DTA_ClipTop, viewport3d.Top(), DTA_ScaleX, z / 1536., DTA_ScaleY, z / 1536., DTA_CenterOffset, true,
                DTA_ClipRight, viewport3d.Right(), DTA_ClipBottom, viewport3d.Bottom(), DTA_Alpha, (pPlayerActor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT ? 0.5 : 1.), TAG_DONE);
            break;
        }
    }
    return true;
}

END_PS_NS
