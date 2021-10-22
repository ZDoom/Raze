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

#pragma once 

#include "compat.h"

BEGIN_PS_NS

enum {
    kSeqRothands = 0,
    kSeqSword,
    kSeqPistol,
    kSeqM60,
    kSeqFlamer,
    kSeqGrenade,
    kSeqCobra,
    kSeqBoneSaw,
    kSeqScramble,
    kSeqGlove,
    kSeqMummy,
    kSeqSkull,
    kSeqPoof,
    kSeqKapow,
    kSeqFireball,
    kSeqBubble,
    kSeqSpider,
    kSeqAnubis,
    kSeqAnuBall,
    kSeqFish,
    kSeqSnakehed,
    kSeqSnakBody,
    kSeqWasp,
    kSeqCobraPow,
    kSeqScorp,
    kSeqJoe, // player pic
    kSeqStatus,
    kSeqDead,
    kSeqDeadEx,
    kSeqAnuPoof,
    kSeqSkulPoof,
    kSeqBullet,
    kSeqShadow,
    kSeqGrenRoll,
    kSeqGrenBoom,
    kSeqSplash,
    kSeqGrenPow,
    kSeqSkulSrt,
    kSeqFirePoof,
    kSeqBloodHit,
    kSeqLion,
    kSeqItems,
    kSeqLavag,
    kSeqLsplash,
    kSeqLavaShot,
    kSeqSmokeBal,
    kSeqFirePot,
    kSeqRex,
    kSeqSet,
    kSeqQueen,
    kSeqRoach,
    kSeqHawk,
    kSeqSetGhost,
    kSeqSetGBlow,
    kSeqBizzTail,
    kSeqBizzPoof,
    kSeqQueenEgg,
    kSeqRoacShot,
    kSeqBackgrnd,
    kSeqScreens,
    kSeqArrow,
    kSeqFonts,
    kSeqDrips,
    kSeqFireTrap,
    kSeqMagic2,
    kSeqCreepy,
    kSeqSlider,
    kSeqRavolt, // 67
    kSeqEyeHit,
    kSeqFont2,
    kSeqSeeBubbl,
    kSeqBlood,
    kSeqDrum,
    kSeqPoof2,
    kSeqDeadBrn, // 74
    kSeqGrenBubb,
    kSeqRochfire,
    kSeqRat
};

struct actionSeq
{
    short a;
    short b;
};

extern short frames;

extern short SeqBase[];
extern short SeqSize[];
extern short SeqOffsets[];

extern short FrameFlag[];

extern short nShadowWidth;
extern short nFlameHeight;

extern short nPilotLightFrame;
extern short nPilotLightCount;

extern short ChunkYpos[];
extern short ChunkXpos[];
extern short ChunkPict[];
extern short ChunkFlag[];
extern short FrameSize[];
extern short FrameBase[];


void seq_LoadSequences();
int seq_GetFrameSound(int val, int edx);
void seq_MoveSequence(DExhumedActor* actor, short nSeq, short bx);

int seq_GetSeqPicnum2(short nSeq, short nFrame);
int seq_GetSeqPicnum(short nSeq, short edx, short ebx);
void seq_DrawStatusSequence(short nSequence, uint16_t edx, short ebx);

int seq_DrawGunSequence(int nSeqOffset, short dx, double xOffs, double yOffs, int nShade, int nPal);
short seq_GetFrameFlag(short val, short nFrame);
int seq_PlotSequence(short nSprite, short edx, short nFrame, short ecx);
int seq_PlotArrowSequence(short nSprite, short nSeq, int nVal);
void seq_DrawPilotLightSeq(double xOffset, double yOffset);

END_PS_NS

