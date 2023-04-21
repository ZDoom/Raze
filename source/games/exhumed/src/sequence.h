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
    int16_t nSeqId;
    int16_t nFlags;
};

struct SeqFrameChunk
{
    int16_t xpos;
    int16_t ypos;
    int16_t picnum;
    int16_t flags;
};

struct SeqFrame
{
    int16_t sound;
    int16_t flags;
    TArray<SeqFrameChunk> chunks;
};

using SeqFrameArray = TArray<SeqFrame>;
using SeqArray = TArray<SeqFrameArray>;

extern int16_t frames;

extern int16_t nShadowWidth;
extern int16_t nFlameHeight;

extern int16_t nPilotLightFrame;
extern int16_t nPilotLightCount;

void seq_LoadSequences();
int seq_GetFrameSound(int val, int edx);
void seq_MoveSequence(DExhumedActor* actor, int16_t nSeq, int16_t nFrame);

int seq_GetSeqPicnum2(int16_t nSeq, int16_t nFrame);
int seq_GetSeqPicnum(int16_t nSeq, int16_t edx, int16_t ebx);
void seq_DrawStatusSequence(int16_t nSequence, uint16_t edx, int16_t ebx);

int seq_DrawGunSequence(int nSeqOffset, int16_t dx, double xOffs, double yOffs, int nShade, int nPal, DAngle angle, bool align = false);
void seq_PlotSequence(const int nSprite, const FName seqFile, const int16_t seqIndex, const int16_t frameIndex, const int16_t nFlags);
int seq_PlotArrowSequence(int nSprite, int16_t nSeq, int nVal);
void seq_DrawPilotLightSeq(double xOffset, double yOffset);

int getSeqFromId(const int nSeqFileId, const int nSeq = 0);
int getSeqFrame(const int nSeq, const int nFrame = 0);
int getSeqFrameCount(const int nSeq);
int getSeqFrameChunk(const int nFrame);
int getSeqFrameFlags(const int nFrame);
int getSeqFrameChunkCount(const int nFrame);
int getSeqFrameChunkPosX(const int nChunk);
int getSeqFrameChunkPosY(const int nChunk);
int getSeqFrameChunkPicnum(const int nChunk);
int getSeqFrameChunkFlags(const int nChunk);

const SeqFrameArray& getSequence(const FName nSeqFile, const unsigned nSeqIndex = 0);

END_PS_NS

