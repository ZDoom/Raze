//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#ifndef AI_H

#define AI_H

// Call functions based on a random range value
typedef struct
{
    short range;
    ANIMATORp action;
} DECISION, *DECISIONp;

// Personality structure
struct PERSONALITYstruct
{
    DECISIONp Battle;
    DECISIONp Offense;
    DECISIONp Broadcast;
    DECISIONp Surprised;
    DECISIONp Evasive;
    DECISIONp LostTarget;
    DECISIONp CloseRange;
    DECISIONp TouchTarget;
};

enum ActorStates { SLOW_SPEED, NORM_SPEED, MID_SPEED, FAST_SPEED, MAX_SPEED};

#define MAXATTRIBSNDS   11
typedef enum
{
    attr_ambient, attr_alert, attr_attack, attr_pain, attr_die,
    attr_extra1, attr_extra2, attr_extra3,attr_extra4,attr_extra5,
    attr_extra6
} ATTRIB_SNDS;

struct ATTRIBUTEstruct
{
    short Speed[MAX_SPEED];
    int8_t TicAdjust[MAX_SPEED];
    uint8_t MaxWeapons;
    /*ATTRIB_SNDS*/ int Sounds[MAXATTRIBSNDS];  // JBF: ATTRIB_SNDS? Somehow I don't think this is what was intended...
};

extern ATTRIBUTE DefaultAttrib;

// AI.C functions
void DebugMoveHit(short SpriteNum);
SWBOOL ActorMoveHitReact(short SpriteNum);
SWBOOL ActorFlaming(short SpriteNum);
void DoActorSetSpeed(short SpriteNum,uint8_t speed);
short ChooseActionNumber(short decision[]);
int DoActorNoise(ANIMATORp Action,short SpriteNum);
int CanSeePlayer(short SpriteNum);
int CanHitPlayer(short SpriteNum);
int DoActorPickClosePlayer(short SpriteNum);
int CloseRangeDist(SPRITEp sp1,SPRITEp sp2);
int InitActorDecide(short SpriteNum);
int DoActorDecide(short SpriteNum);
int InitActorAlertNoise(short SpriteNum);
int InitActorAmbientNoise(short SpriteNum);
int InitActorAttackNoise(short SpriteNum);
int InitActorPainNoise(short SpriteNum);
int InitActorDieNoise(short SpriteNum);
int InitActorExtra1Noise(short SpriteNum);
int InitActorExtra2Noise(short SpriteNum);
int InitActorExtra3Noise(short SpriteNum);
int InitActorExtra4Noise(short SpriteNum);
int InitActorExtra5Noise(short SpriteNum);
int InitActorExtra6Noise(short SpriteNum);
int InitActorMoveCloser(short SpriteNum);
int DoActorCantMoveCloser(short SpriteNum);
int DoActorMoveCloser(short SpriteNum);
short FindTrackToPlayer(USERp u);
short FindTrackAwayFromPlayer(USERp u);
short FindWanderTrack(USERp u);
int InitActorRunAway(short SpriteNum);
int InitActorRunToward(short SpriteNum);
int InitActorAttack(short SpriteNum);
int DoActorAttack(short SpriteNum);
int InitActorEvade(short SpriteNum);
int InitActorWanderAround(short SpriteNum);
int InitActorFindPlayer(short SpriteNum);
int InitActorDuck(short SpriteNum);
int DoActorDuck(short SpriteNum);
int DoActorMoveJump(short SpriteNum);
int move_scan(short SpriteNum,short ang,int dist,int *stopx,int *stopy,int *stopz,short *stopsect);
int FindNewAngle(short SpriteNum,signed char dir,int DistToMove);
int InitActorReposition(short SpriteNum);
int DoActorReposition(short SpriteNum);
int InitActorPause(short SpriteNum);
int DoActorPause(short SpriteNum);

/*
ANIMATOR
InitActorDecide,
InitActorMoveCloser,
InitActorAttack,
InitActorRunAway,
InitActorEvade,
InitActorWanderAround,
InitActorFindPlayer,
InitActorReposition,
InitActorPause,
InitActorDuck,
InitActorAmbientNoise,
InitActorAlertNoise,
InitActorAttackNoise,
InitActorPainNoise,
InitActorDieNoise,
InitActorExtra1Noise,
InitActorExtra2Noise,
InitActorExtra3Noise,
InitActorExtra4Noise,
InitActorExtra5Noise,
InitActorExtra6Noise;

ANIMATOR
DoActorDecide,
DoActorMoveCloser,
DoActorAttack,
DoActorRunAway,
DoActorWanderAround,
DoActorReposition,
DoActorPause,
DoActorDuck,
DoActorAmbientNoise,
DoActorAlertNoise,
DoActorAttackNoise,
DoActorPainNoise,
DoActorDieNoise,
DoActorExtra1Noise,
DoActorExtra2Noise,
DoActorExtra3Noise,
DoActorExtra4Noise,
DoActorExtra5Noise,
DoActorExtra6Noise;
*/

void DoActorSetSpeed(short SpriteNum, uint8_t speed);

#endif
