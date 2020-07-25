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

void ByteSwapSEQ(void *p)
{
	Seq *pSeq = (Seq*)p;
	pSeq->version = B_LITTLE16(pSeq->version);
	pSeq->nFrames = B_LITTLE16(pSeq->nFrames);
	pSeq->at8 = B_LITTLE16(pSeq->at8);
	pSeq->ata = B_LITTLE16(pSeq->ata);
	pSeq->atc = B_LITTLE32(pSeq->atc);
	for (int i = 0; i < pSeq->nFrames; i++)
	{
		SEQFRAME *pFrame = &pSeq->frames[i];
		BitReader bitReader((char *)pFrame, sizeof(SEQFRAME));
		SEQFRAME swapFrame;
		swapFrame.tile = bitReader.readUnsigned(12);
		swapFrame.at1_4 = bitReader.readBit();
		swapFrame.at1_5 = bitReader.readBit();
		swapFrame.at1_6 = bitReader.readBit();
		swapFrame.at1_7 = bitReader.readBit();
		swapFrame.at2_0 = bitReader.readUnsigned(8);
		swapFrame.at3_0 = bitReader.readUnsigned(8);
		swapFrame.at4_0 = bitReader.readSigned(8);
		swapFrame.at5_0 = bitReader.readUnsigned(5);
		swapFrame.at5_5 = bitReader.readBit();
		swapFrame.at5_6 = bitReader.readBit();
		swapFrame.at5_7 = bitReader.readBit();
		swapFrame.at6_0 = bitReader.readBit();
		swapFrame.at6_1 = bitReader.readBit();
		swapFrame.at6_2 = bitReader.readBit();
		swapFrame.at6_3 = bitReader.readBit();
		swapFrame.at6_4 = bitReader.readBit();
		swapFrame.tile2 = bitReader.readUnsigned(4);
		swapFrame.soundRange = bitReader.readUnsigned(4);
		swapFrame.surfaceSound = bitReader.readBit();
		swapFrame.reserved = bitReader.readUnsigned(2);
		*pFrame = swapFrame;
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
