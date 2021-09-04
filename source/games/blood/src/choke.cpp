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
#include "blood.h"
#include "choke.h"

BEGIN_BLD_NS

void CChoke::init(int a1, void(*a2)(PLAYER*))
{
	callback = a2;
	if (!qav && a1 != -1)
	{
		qav = getQAV(a1);
		if (!qav)
			I_Error("Could not load QAV %d\n", a1);
		qav->x = x;
		qav->y = y;
		duration = qav->duration;
		time = 0;
	}
}

void CChoke::animateChoke(int x, int y, int smoothratio)
{
	if (!qav)
		return;
	int myclock = PlayClock + MulScale(4, smoothratio, 16);
	qav->x = x;
	qav->y = y;
	int vd = myclock - time;
	time = myclock;
	duration -= vd;
	if (duration <= 0 || duration > qav->duration)
		duration = qav->duration;
	int vdi = qav->duration - duration;
	qav->Play(vdi - vd, vdi, -1, nullptr);
	// This originally overlaid the HUD but that simply doesn't work right with the HUD being a genuine overlay.
	// It also never adjusted for a reduced 3D view
	qav->Draw(vdi, 10, 0, 0, true);
}


void chokeCallback(PLAYER* pPlayer)
{
	int t = gGameOptions.nDifficulty + 2;
	if (pPlayer->handTime < 64)
		pPlayer->handTime = min(pPlayer->handTime + t, 64);
	if (pPlayer->handTime > (7 - gGameOptions.nDifficulty) * 5)
		pPlayer->blindEffect = min(pPlayer->blindEffect + t * 4, 128);
}

CChoke gChoke;

END_BLD_NS
