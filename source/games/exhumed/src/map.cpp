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

void DrawMap(double const interpfrac)
{
    if (!nFreeze && automapMode != am_off) 
    {
        auto pPlayerActor = PlayerList[nLocalPlayer].pActor;
        auto ang = !SyncInput() ? PlayerList[nLocalPlayer].angle.sum() : PlayerList[nLocalPlayer].angle.interpolatedsum(interpfrac);
        DrawOverheadMap(pPlayerActor->interpolatedpos(interpfrac).XY(), ang, interpfrac);
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

bool GameInterface::DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac)
{
    auto cangvect = cang.ToVector();

    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (i == nLocalPlayer)// || gGameOptions.nGameType == 1)
        {
            auto pPlayerActor = PlayerList[i].pActor;
            auto vect = OutAutomapVector(mxy - cpos, cangvect, czoom, xydim);

            DrawTexture(twod, tileGetTexture(pPlayerActor->spr.picnum /*+ ((PlayClock >> 4) & 3)*/, true), vect.X, vect.Y, DTA_ClipLeft, viewport3d.Left(), DTA_ClipTop, viewport3d.Top(), DTA_ScaleX, czoom * (2. / 3.), DTA_ScaleY, czoom * (2. / 3.), DTA_CenterOffset, true,
                DTA_ClipRight, viewport3d.Right(), DTA_ClipBottom, viewport3d.Bottom(), DTA_Alpha, (pPlayerActor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT ? 0.5 : 1.), TAG_DONE);
            break;
        }
    }
    return true;
}

END_PS_NS
