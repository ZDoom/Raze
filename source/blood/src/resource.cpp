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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compat.h"

#include "common_game.h"

#include "misc.h"

#if B_BIG_ENDIAN == 1
#include "qav.h"
#include "seq.h"
#include "sound.h"
#endif

BEGIN_BLD_NS



// Code left here for documentation. 
// It's unlikely to be needed by today's systems so unless needed I rather leave such hacks out of the file system.
#if B_BIG_ENDIAN == 1
void ByteSwapQAV(void *p)
{
	QAV *qav = (QAV*)p;
	qav->nFrames = B_LITTLE32(qav->nFrames);
	qav->ticksPerFrame = B_LITTLE32(qav->ticksPerFrame);
	qav->at10 = B_LITTLE32(qav->at10);
	qav->x = B_LITTLE32(qav->x);
	qav->y = B_LITTLE32(qav->y);
	qav->nSprite = B_LITTLE32(qav->nSprite);
	for (int i = 0; i < qav->nFrames; i++)
	{
		FRAMEINFO *pFrame = &qav->frames[i];
		SOUNDINFO *pSound = &pFrame->sound;
		pFrame->nCallbackId = B_LITTLE32(pFrame->nCallbackId);
		pSound->sound = B_LITTLE32(pSound->sound);
		for (int j = 0; j < 8; j++)
		{
			TILE_FRAME *pTile = &pFrame->tiles[j];
			pTile->picnum = B_LITTLE32(pTile->picnum);
			pTile->x = B_LITTLE32(pTile->x);
			pTile->y = B_LITTLE32(pTile->y);
			pTile->z = B_LITTLE32(pTile->z);
			pTile->stat = B_LITTLE32(pTile->stat);
			pTile->angle = B_LITTLE16(pTile->angle);
		}
	}
}

void ByteSwapSFX(void *p)
{
	SFX *pSFX = (SFX*)p;
	pSFX->relVol = B_LITTLE32(pSFX->relVol);
	pSFX->pitch = B_LITTLE32(pSFX->pitch);
	pSFX->pitchRange = B_LITTLE32(pSFX->pitchRange);
	pSFX->format = B_LITTLE32(pSFX->format);
	pSFX->loopStart = B_LITTLE32(pSFX->loopStart);
}

#endif


END_BLD_NS
