
#ifndef __sequence_h__
#define __sequence_h__

#include "typedefs.h"

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

extern short laststatustile;

extern int laststatusx;
extern int laststatusy;

void seq_LoadSequences();
void seq_MoveSequence(short nSprite, short nSeq, short bx);
int seq_GetSeqPicnum2(short nSeq, short nFrame);
int seq_GetSeqPicnum(short nSeq, short edx, short ebx);
void seq_DrawStatusSequence(short nSequence, ushort edx, short ebx);

int seq_DrawGunSequence(int nSeqOffset, short dx, int xOffs, int yOffs, int nShade, int nPal);
short seq_GetFrameFlag(short val, short nFrame);
int seq_PlotSequence(short nSprite, short edx, short nFrame, short ecx);
int seq_PlotArrowSequence(short nSprite, short nSeq, int nVal);
void seq_DrawPilotLightSeq(int xOffset, int yOffset);

#endif
