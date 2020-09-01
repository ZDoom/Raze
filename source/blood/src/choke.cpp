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

#include "build.h"
#include "compat.h"
#include "common_game.h"
#include "blood.h"
#include "choke.h"
#include "globals.h"
#include "levels.h"
#include "player.h"
#include "qav.h"

BEGIN_BLD_NS

void CChoke::init(int a1, void(*a2)(PLAYER*))
{
    at0 = NULL;
    at1c = a2;
    if (!at8 && a1 != -1)
    {
        at8 = getQAV(a1);
        if (!at8)
            ThrowError("Could not load QAV %d\n", a1);
        at8->nSprite = -1;
        at8->x = at14;
        at8->y = at18;
        //at8->Preload();
		atc = at8->at10;
		at10 = 0;
    }
}

void CChoke::animateChoke(int x, int y, int smoothratio)
{
    if (!at8)
        return;
	int myclock = gFrameClock +  mulscale16(4, smoothratio);
    at8->x = x;
    at8->y = y;
    int vd = myclock-at10;
    at10 = myclock;
    atc -= vd;
    if (atc <= 0 || atc > at8->at10)
        atc = at8->at10;
    int vdi = at8->at10-atc;
    at8->Play(vdi-vd, vdi, -1, NULL);
	// This originally overlaid the HUD but that simply doesn't work right with the HUD being a genuine overlay.
	// It also never adjusted for a reduced 3D view
    at8->Draw(vdi, 10, 0, 0, true);
}


void sub_84230(PLAYER *pPlayer)
{
    int t = gGameOptions.nDifficulty+2;
    if (pPlayer->handTime < 64)
        pPlayer->handTime = ClipHigh(pPlayer->handTime+t, 64);
    if (pPlayer->handTime > (7-gGameOptions.nDifficulty)*5)
        pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect+t*4, 128);
}

CChoke gChoke;

END_BLD_NS
