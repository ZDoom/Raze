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
    int16_t a;
    int16_t b;
};

extern int16_t frames;

extern int16_t SeqBase[];
extern int16_t SeqSize[];
extern int16_t SeqOffsets[];
extern int16_t FrameFlag[];

extern int16_t nShadowWidth;
extern int16_t nFlameHeight;

extern int16_t nPilotLightFrame;
extern int16_t nPilotLightCount;

extern int16_t ChunkYpos[];
extern int16_t ChunkXpos[];
extern int16_t ChunkPict[];
extern int16_t ChunkFlag[];
extern int16_t FrameSize[];
extern int16_t FrameBase[];


void seq_LoadSequences();
int seq_GetFrameSound(int val, int edx);
void seq_MoveSequence(DExhumedActor* actor, int16_t nSeq, int16_t bx);

int seq_GetSeqPicnum2(int16_t nSeq, int16_t nFrame);
int seq_GetSeqPicnum(int16_t nSeq, int16_t edx, int16_t ebx);
void seq_DrawStatusSequence(int16_t nSequence, uint16_t edx, int16_t ebx);

int seq_DrawGunSequence(int nSeqOffset, int16_t dx, double xOffs, double yOffs, int nShade, int nPal, bool align = false);
int16_t seq_GetFrameFlag(int16_t val, int16_t nFrame);
int seq_PlotSequence(int nSprite, int16_t edx, int16_t nFrame, int16_t ecx);
int seq_PlotArrowSequence(int nSprite, int16_t nSeq, int nVal);
void seq_DrawPilotLightSeq(double xOffset, double yOffset);

END_PS_NS

