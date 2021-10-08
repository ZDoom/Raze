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
BEGIN_SW_NS

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
bool ActorMoveHitReact(short SpriteNum);
bool ActorFlaming(short SpriteNum);
void DoActorSetSpeed(short SpriteNum,uint8_t speed);
short ChooseActionNumber(short decision[]);
int DoActorNoise(ANIMATORp Action,short SpriteNum);
bool CanSeePlayer(short SpriteNum);
int CanHitPlayer(short SpriteNum);
int DoActorPickClosePlayer(short SpriteNum);
int CloseRangeDist(SPRITEp sp1,SPRITEp sp2);
int InitActorDecide(USER* SpriteNum);
int DoActorDecide(USER* SpriteNum);
int InitActorAlertNoise(USER* SpriteNum);
int InitActorAmbientNoise(USER* SpriteNum);
int InitActorAttackNoise(USER* SpriteNum);
int InitActorPainNoise(USER* SpriteNum);
int InitActorDieNoise(USER* SpriteNum);
int InitActorExtra1Noise(USER* SpriteNum);
int InitActorExtra2Noise(USER* SpriteNum);
int InitActorExtra3Noise(USER* SpriteNum);
int InitActorExtra4Noise(USER* SpriteNum);
int InitActorExtra5Noise(USER* SpriteNum);
int InitActorExtra6Noise(USER* SpriteNum);
int InitActorMoveCloser(USER* SpriteNum);
int DoActorCantMoveCloser(USER* SpriteNum);
int DoActorMoveCloser(USER* SpriteNum);
short FindTrackToPlayer(USERp u);
short FindTrackAwayFromPlayer(USERp u);
short FindWanderTrack(USERp u);
int InitActorRunAway(USER* SpriteNum);
int InitActorRunToward(USER* SpriteNum);
int InitActorAttack(USER* SpriteNum);
int DoActorAttack(USER* SpriteNum);
int InitActorEvade(USER* SpriteNum);
int InitActorWanderAround(USER* SpriteNum);
int InitActorFindPlayer(USER* SpriteNum);
int InitActorDuck(USER* SpriteNum);
int DoActorDuck(USER* SpriteNum);
int DoActorMoveJump(USER* SpriteNum);
int move_scan(short SpriteNum,short ang,int dist,int *stopx,int *stopy,int *stopz,short *stopsect);
int FindNewAngle(short SpriteNum,signed char dir,int DistToMove);
int InitActorReposition(USER* SpriteNum);
int DoActorReposition(USER* SpriteNum);
int InitActorPause(USER* SpriteNum);
int DoActorPause(USER* SpriteNum);

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

END_SW_NS

#endif
