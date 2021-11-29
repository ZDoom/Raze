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
bool ActorMoveHitReact(DSWActor* actor);
short ChooseActionNumber(short decision[]);
bool CanSeePlayer(DSWActor* actor);
int DoActorPickClosePlayer(DSWActor* actor);
int InitActorDecide(DSWActor* actor);
int DoActorDecide(DSWActor* actor);
int InitActorAlertNoise(DSWActor* actor);
int InitActorAmbientNoise(DSWActor* actor);
int InitActorAttackNoise(DSWActor* actor);
int InitActorPainNoise(DSWActor* actor);
int InitActorDieNoise(DSWActor* actor);
int InitActorExtra1Noise(DSWActor* actor);
int InitActorExtra2Noise(DSWActor* actor);
int InitActorExtra3Noise(DSWActor* actor);
int InitActorExtra4Noise(DSWActor* actor);
int InitActorExtra5Noise(DSWActor* actor);
int InitActorExtra6Noise(DSWActor* actor);
int InitActorMoveCloser(DSWActor* actor);
int DoActorCantMoveCloser(DSWActor* actor);
int DoActorMoveCloser(DSWActor* actor);
short FindTrackToPlayer(DSWActor* actor);
short FindTrackAwayFromPlayer(DSWActor* actor);
int InitActorRunAway(DSWActor* actor);
int InitActorRunToward(DSWActor* actor);
int InitActorAttack(DSWActor* actor);
int DoActorAttack(DSWActor* actor);
int InitActorEvade(DSWActor* actor);
int InitActorWanderAround(DSWActor* actor);
int InitActorFindPlayer(DSWActor* actor);
int InitActorDuck(DSWActor* actor);
int DoActorDuck(DSWActor* actor);
int DoActorMoveJump(DSWActor* actor);
int InitActorReposition(DSWActor* actor);
int DoActorReposition(DSWActor* actor);
int InitActorPause(DSWActor* actor);
int DoActorPause(DSWActor* actor);

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

void DoActorSetSpeed(DSWActor* actor, uint8_t speed);

END_SW_NS

#endif
