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
#include "ns.h"

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "sprite.h"
#include "weapon.h"
#include "misc.h"
#include "texinfo.h"

BEGIN_SW_NS

int Bunny_Count = 0;

DECISION BunnyBattle[] =
{
    {748, &AF(InitActorMoveCloser)},
    {750, &AF(InitActorSetDecide)},
    {760, &AF(InitActorSetDecide)},
    {1024, &AF(InitActorMoveCloser)}
};

DECISION BunnyOffense[] =
{
    {600, &AF(InitActorMoveCloser)},
    {700, &AF(InitActorSetDecide)},
    {1024, &AF(InitActorMoveCloser)}
};

DECISIONB BunnyBroadcast[] =
{
    {21, attr_alert},
    {51, attr_ambient},
    {1024, 0}
};

DECISIONB BunnyBroadcast2[] =
{
    {500,  0},
    {1020, 0},
    {1024, attr_ambient}
};

DECISION BunnySurprised[] =
{
    {500, &AF(InitActorRunAway)},
    {701, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorDecide)}
};

DECISION BunnyEvasive[] =
{
    {500,  &AF(InitActorWanderAround)},
    {1020, &AF(InitActorRunAway)},
    {1024, &AF(InitActorSetDecide)}
};

DECISION BunnyLostTarget[] =
{
    {900, &AF(InitActorFindPlayer)},
    {1024, &AF(InitActorWanderAround)}
};

DECISION BunnyCloseRange[] =
{
    {1024,  &AF(InitActorAttack)             },
};

DECISION BunnyWander[] =
{
    {1024, &AF(InitActorReposition)}
};

PERSONALITY WhiteBunnyPersonality =
{
    BunnyBattle,
    BunnyOffense,
    BunnyBroadcast,
    BunnySurprised,
    BunnyEvasive,
    BunnyLostTarget,
    BunnyCloseRange,
    BunnyCloseRange
};

PERSONALITY BunnyPersonality =
{
    BunnyEvasive,
    BunnyEvasive,
    BunnyBroadcast2,
    BunnyWander,
    BunnyWander,
    BunnyWander,
    BunnyEvasive,
    BunnyEvasive
};

ATTRIBUTE BunnyAttrib =
{
    {100, 120, 140, 180},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_BUNNYAMBIENT, 0, DIGI_BUNNYATTACK,
        DIGI_BUNNYATTACK, DIGI_BUNNYDIE2, 0,
        0,0,0,0
    }
};

ATTRIBUTE WhiteBunnyAttrib =
{
    {200, 220, 340, 380},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_BUNNYAMBIENT, 0, DIGI_BUNNYATTACK,
        DIGI_BUNNYATTACK, DIGI_BUNNYDIE2, 0,
        0,0,0,0
    }
};

//////////////////////
//
// BUNNY RUN
//
//////////////////////

#define BUNNY_RUN_RATE 10

FState s_BunnyRun[] =
{
        {SPR_BUNNY_RUN, 'A', BUNNY_RUN_RATE | SF_TIC_ADJUST, &AF(DoBunnyMove), &s_BunnyRun[1]},
        {SPR_BUNNY_RUN, 'B', BUNNY_RUN_RATE | SF_TIC_ADJUST, &AF(DoBunnyMove), &s_BunnyRun[2]},
        {SPR_BUNNY_RUN, 'C', BUNNY_RUN_RATE | SF_TIC_ADJUST, &AF(DoBunnyMove), &s_BunnyRun[3]},
        {SPR_BUNNY_RUN, 'D', BUNNY_RUN_RATE | SF_TIC_ADJUST, &AF(DoBunnyMove), &s_BunnyRun[4]},
        {SPR_BUNNY_RUN, 'E', SF_QUICK_CALL,                &AF(DoBunnyGrowUp), &s_BunnyRun[5]},
        {SPR_BUNNY_RUN, 'E', BUNNY_RUN_RATE | SF_TIC_ADJUST, &AF(DoBunnyMove), &s_BunnyRun[0]},
};


//////////////////////
//
// BUNNY STAND
//
//////////////////////

#define BUNNY_STAND_RATE 12

FState s_BunnyStand[] =
{
        {SPR_BUNNY_RUN, 'A', BUNNY_STAND_RATE, &AF(DoBunnyEat), &s_BunnyStand[1]},
        {SPR_BUNNY_RUN, 'E', SF_QUICK_CALL, &AF(DoBunnyGrowUp), &s_BunnyStand[2]},
        {SPR_BUNNY_RUN, 'E', BUNNY_STAND_RATE, &AF(DoBunnyEat), &s_BunnyStand[0]},
};


//////////////////////
//
// BUNNY GET LAYED
//
//////////////////////

#define BUNNY_SCREW_RATE 16

FState s_BunnyScrew[] =
{
        {SPR_BUNNY_RUN, 'A', BUNNY_SCREW_RATE, &AF(DoBunnyScrew), &s_BunnyScrew[1]},
        {SPR_BUNNY_RUN, 'C', BUNNY_SCREW_RATE, &AF(DoBunnyScrew), &s_BunnyScrew[0]},
};


//////////////////////
//
// BUNNY SWIPE
//
//////////////////////

#define BUNNY_SWIPE_RATE 8

FState s_BunnySwipe[] =
{
        {SPR_BUNNY_SWIPE, 'A', BUNNY_SWIPE_RATE, &AF(NullBunny), &s_BunnySwipe[1]},
        {SPR_BUNNY_SWIPE, 'B', BUNNY_SWIPE_RATE, &AF(NullBunny), &s_BunnySwipe[2]},
        {SPR_BUNNY_SWIPE, 'B', 0 | SF_QUICK_CALL, &AF(InitBunnySlash), &s_BunnySwipe[3]},
        {SPR_BUNNY_SWIPE, 'C', BUNNY_SWIPE_RATE, &AF(NullBunny), &s_BunnySwipe[4]},
        {SPR_BUNNY_SWIPE, 'D', BUNNY_SWIPE_RATE, &AF(NullBunny), &s_BunnySwipe[5]},
        {SPR_BUNNY_SWIPE, 'D', 0 | SF_QUICK_CALL, &AF(InitBunnySlash), &s_BunnySwipe[6]},
        {SPR_BUNNY_SWIPE, 'D', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_BunnySwipe[7]},
        {SPR_BUNNY_SWIPE, 'D', BUNNY_SWIPE_RATE, &AF(DoBunnyMove), &s_BunnySwipe[7]},
};


//////////////////////
//
// BUNNY HEART - show players heart
//
//////////////////////

#define BUNNY_HEART_RATE 14

FState s_BunnyHeart[] =
{
        {SPR_BUNNY_SWIPE, 'A', BUNNY_HEART_RATE, &AF(DoBunnyStandKill), &s_BunnyHeart[0]},
};


//////////////////////
//
// BUNNY PAIN
//
//////////////////////

#define BUNNY_PAIN_RATE 38

FState s_BunnyPain[] =
{
        {SPR_BUNNY_SWIPE, 'A', BUNNY_PAIN_RATE, &AF(DoBunnyPain), &s_BunnyPain[0]},
};

//////////////////////
//
// BUNNY JUMP
//
//////////////////////

#define BUNNY_JUMP_RATE 25

FState s_BunnyJump[] =
{
        {SPR_BUNNY_RUN, 'B', BUNNY_JUMP_RATE, &AF(DoBunnyMoveJump), &s_BunnyJump[1]},
        {SPR_BUNNY_RUN, 'C', BUNNY_JUMP_RATE, &AF(DoBunnyMoveJump), &s_BunnyJump[1]},
};


//////////////////////
//
// BUNNY FALL
//
//////////////////////

#define BUNNY_FALL_RATE 25

FState s_BunnyFall[] =
{
        {SPR_BUNNY_RUN, 'D', BUNNY_FALL_RATE, &AF(DoBunnyMoveJump), &s_BunnyFall[0]},
};


//////////////////////
//
// BUNNY JUMP ATTACK
//
//////////////////////

#define BUNNY_JUMP_ATTACK_RATE 35
int DoBunnyBeginJumpAttack(DSWActor* actor);

FState s_BunnyJumpAttack[] =
{
        {SPR_BUNNY_RUN, 'B', BUNNY_JUMP_ATTACK_RATE, &AF(NullBunny), &s_BunnyJumpAttack[1]},
        {SPR_BUNNY_RUN, 'B', 0 | SF_QUICK_CALL, &AF(DoBunnyBeginJumpAttack), &s_BunnyJumpAttack[2]},
        {SPR_BUNNY_RUN, 'C', BUNNY_JUMP_ATTACK_RATE, &AF(DoBunnyMoveJump), &s_BunnyJumpAttack[2]},
};


//////////////////////
//
// BUNNY DIE
//
//////////////////////

#define BUNNY_DIE_RATE 16

FState s_BunnyDie[] =
{
    {SPR_BUNNY_DIE, 'A', BUNNY_DIE_RATE, &AF(NullBunny), &s_BunnyDie[1]},
    {SPR_BUNNY_DIE, 'A', SF_QUICK_CALL,  &AF(BunnySpew), &s_BunnyDie[2]},
    {SPR_BUNNY_DIE, 'B', BUNNY_DIE_RATE, &AF(NullBunny), &s_BunnyDie[3]},
    {SPR_BUNNY_DIE, 'C', BUNNY_DIE_RATE, &AF(NullBunny), &s_BunnyDie[4]},
    {SPR_BUNNY_DIE, 'C', BUNNY_DIE_RATE, &AF(NullBunny), &s_BunnyDie[5]},
    {SPR_BUNNY_DEAD, 'A', BUNNY_DIE_RATE, &AF(DoActorDebris), &s_BunnyDie[5]},
};

#define BUNNY_DEAD_RATE 8

FState s_BunnyDead[] =
{
    {SPR_BUNNY_DIE, 'A', BUNNY_DEAD_RATE, nullptr,  &s_BunnyDie[1]},
    {SPR_BUNNY_DIE, 'A', SF_QUICK_CALL,  &AF(BunnySpew), &s_BunnyDie[2]},
    {SPR_BUNNY_DIE, 'B', BUNNY_DEAD_RATE, nullptr,  &s_BunnyDead[3]},
    {SPR_BUNNY_DIE, 'C', BUNNY_DEAD_RATE, nullptr,  &s_BunnyDead[4]},
    {SPR_BUNNY_DEAD, 'A', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_BunnyDead[5]},
    {SPR_BUNNY_DEAD, 'A', BUNNY_DEAD_RATE, &AF(DoActorDebris), &s_BunnyDead[5]},
};

FState s_BunnyDeathJump[] =
{
    {SPR_BUNNY_DIE, 'A', BUNNY_DIE_RATE, &AF(DoActorDeathMove), &s_BunnyDeathJump[0]}
};

FState s_BunnyDeathFall[] =
{
    {SPR_BUNNY_DIE, 'B', BUNNY_DIE_RATE, &AF(DoActorDeathMove), &s_BunnyDeathFall[0]}
};



/*
FState* *Stand[MAX_WEAPONS];
FState* *Run;
FState* *Jump;
FState* *Fall;
FState* *Crawl;
FState* *Swim;
FState* *Fly;
FState* *Rise;
FState* *Sit;
FState* *Look;
FState* *Climb;
FState* *Pain;
FState* *Death1;
FState* *Death2;
FState* *Dead;
FState* *DeathJump;
FState* *DeathFall;
FState* *CloseAttack[2];
FState* *Attack[6];
FState* *Special[2];
*/

ACTOR_ACTION_SET BunnyActionSet =
{
    s_BunnyStand,
    s_BunnyRun,
    s_BunnyJump,
    s_BunnyFall,
    nullptr,                               // s_BunnyCrawl,
    nullptr,                               // s_BunnySwim,
    nullptr,                               // s_BunnyFly,
    nullptr,                               // s_BunnyRise,
    nullptr,                               // s_BunnySit,
    nullptr,                               // s_BunnyLook,
    nullptr,                               // climb
    s_BunnyPain,
    s_BunnyDie,
    nullptr,
    s_BunnyDead,
    s_BunnyDeathJump,
    s_BunnyDeathFall,
    {nullptr},
    {1024},
    {nullptr},
    {1024},
    {s_BunnyHeart, s_BunnyRun},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET BunnyWhiteActionSet =
{
    s_BunnyStand,
    s_BunnyRun,
    s_BunnyJump,
    s_BunnyFall,
    nullptr,                               // s_BunnyCrawl,
    nullptr,                               // s_BunnySwim,
    nullptr,                               // s_BunnyFly,
    nullptr,                               // s_BunnyRise,
    nullptr,                               // s_BunnySit,
    nullptr,                               // s_BunnyLook,
    nullptr,                               // climb
    s_BunnyPain,                       // pain
    s_BunnyDie,
    nullptr,
    s_BunnyDead,
    s_BunnyDeathJump,
    s_BunnyDeathFall,
    {s_BunnySwipe},
    {1024},
//    {s_BunnyJumpAttack, s_BunnySwipe},
//    {800, 1024},
    {s_BunnySwipe},
    {1024},
    {s_BunnyHeart, s_BunnySwipe},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupBunny(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, BUNNY_RUN_R0, &s_BunnyRun[0]);
        actor->user.Health = 10;
    }

    Bunny_Count++;

    actor->user.ShellNum = 0; // Not Pregnant right now
    actor->user.FlagOwner = 0;

	actor->clipdist = 9.25;

    if (actor->spr.pal == PALETTE_PLAYER1)
    {
        EnemyDefaults(actor, &BunnyWhiteActionSet, &WhiteBunnyPersonality);
        actor->user.__legacyState.Attrib = &WhiteBunnyAttrib;
        actor->spr.scale = DVector2(1.5, 1.40625);

        actor->clipdist = 12.5;

        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = 60;
    }
    else if (actor->spr.pal == PALETTE_PLAYER8) // Male Rabbit
    {
        EnemyDefaults(actor, &BunnyActionSet, &BunnyPersonality);
        actor->user.__legacyState.Attrib = &BunnyAttrib;

		if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = 20;
        actor->user.Flag1 = 0;
    }
    else
    {
        // Female Rabbit
        EnemyDefaults(actor, &BunnyActionSet, &BunnyPersonality);
        actor->user.__legacyState.Attrib = &BunnyAttrib;
        actor->user.spal = actor->spr.pal = PALETTE_PLAYER0;
        actor->user.Flag1 = SEC(5);
        //actor->spr.shade = 0; // darker
    }

    actor->setStateGroup(NAME_Run);
    actor->setPicFromState();
    actor->user.__legacyState.StateEnd = s_BunnyDie;
    

    DoActorSetSpeed(actor, FAST_SPEED);

    actor->user.Flags |= (SPR_XFLIP_TOGGLE);


    actor->user.zclip = 16;
    actor->user.floor_dist = 8;
    actor->user.ceiling_dist = 8;
    actor->user.lo_step = 16;

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetBunnyJumpHeight(int jump_speed, int jump_grav)
{
    int jump_iterations;
    int height;

    jump_speed = abs(jump_speed);

    jump_iterations = jump_speed / (jump_grav * ACTORMOVETICS);

    height = jump_speed * jump_iterations * ACTORMOVETICS;

    return height >> 9;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int PickBunnyJumpSpeed(DSWActor* actor, int pix_height)
{
    ASSERT(pix_height < 128);

    actor->user.jump_speed = -600;
    actor->user.jump_grav = 8;

    while (true)
    {
        if (GetBunnyJumpHeight(actor->user.jump_speed, actor->user.jump_grav) > pix_height + 20)
            break;

        actor->user.jump_speed -= 100;

        ASSERT(actor->user.jump_speed > -3000);
    }

    return actor->user.jump_speed;
}

//---------------------------------------------------------------------------
//
// JUMP ATTACK
//
//---------------------------------------------------------------------------

int DoBunnyBeginJumpAttack(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;

    DAngle tang = (target->spr.pos - actor->spr.pos).Angle();

    Collision coll = move_sprite(actor, DVector3(tang.ToVector() * 8, 0), actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_ACTOR, ACTORMOVETICS);

	auto rndang = RandomAngle(DAngle45) - DAngle22_5;
    if (coll.type != kHitNone)
		actor->spr.Angles.Yaw += DAngle180 + rndang; 
	else
		actor->spr.Angles.Yaw = tang + rndang;

    DoActorSetSpeed(actor, FAST_SPEED);

    PickJumpMaxSpeed(actor, -400); // was -800

    actor->user.Flags |= (SPR_JUMPING);
    actor->user.Flags &= ~(SPR_FALLING);

    // set up individual actor jump gravity
    actor->user.jump_grav = 17; // was 8

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actor);

    DoJump(actor);

    return 0;
}

int DoBunnyMoveJump(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0));

        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    DoActorZrange(actor);

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        InitActorDecide(actor);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoPickCloseBunny(DSWActor* actor)
{
    double near_dist = 62.5;

    // if actor can still see the player
    bool ICanSee = false;

    SWStatIterator it(STAT_ENEMY);
    while (auto itActor = it.Next())
    {
        if (actor == itActor) continue;

        if (itActor->user.ID != BUNNY_RUN_R0) continue;

        double dist = (itActor->spr.pos.XY() - actor->spr.pos.XY()).Length();

        if (dist > near_dist) continue;

        ICanSee = FAFcansee(ActorVectOfTop(actor), actor->sector(), ActorUpperVect(itActor), itActor->sector());

        if (ICanSee && dist < near_dist && itActor->user.ID == BUNNY_RUN_R0)
        {
            near_dist = dist;
            actor->user.targetActor = itActor;
            actor->user.lowActor = itActor;
            //Bunny_Result = i;
            return;
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyQuickJump(DSWActor* actor)
{
    if (actor->user.spal != PALETTE_PLAYER8) return false;

    if (!actor->user.lowActor&& actor->user.spal == PALETTE_PLAYER8 && MoveSkip4)
        DoPickCloseBunny(actor);

    // Random Chance of like sexes fighting
    DSWActor* hitActor = actor->user.lowActor;
    if (hitActor)
    {
        if (!hitActor->hasU() || hitActor->user.ID != BUNNY_RUN_R0) return false;


        // Not mature enough yet
        if (actor->spr.scale.X != 1 || actor->spr.scale.Y != 1) return false;
        if (hitActor->spr.scale.X != 1 || hitActor->spr.scale.Y != 1) return false;

        // Kill a rival
        // Only males fight
        if (hitActor->user.spal == actor->spr.pal && RandomRange(1000) > 995)
        {
            if (actor->user.spal == PALETTE_PLAYER8 && hitActor->user.spal == PALETTE_PLAYER8)
            {
                PlaySound(DIGI_BUNNYATTACK, actor, v3df_follow);
                PlaySound(DIGI_BUNNYDIE2, hitActor, v3df_follow);
                hitActor->user.Health = 0;

                // Blood fountains
                InitBloodSpray(hitActor, true,-1);

                if (SpawnShrap(hitActor, actor))
                {
                    SetSuicide(hitActor);
                }
                else
                    DoActorDie(hitActor, actor, 0);

                Bunny_Count--; // Bunny died

                actor->user.lowActor = nullptr;
                return true;
            }
        }
    }

    // Get layed!
    hitActor = actor->user.lowActor;
    if (hitActor && actor->user.spal == PALETTE_PLAYER8) // Only males check this
    {
        if (!hitActor->hasU() || hitActor->user.ID != BUNNY_RUN_R0) return false;

        // Not mature enough to mate yet
		if (actor->spr.scale.X != 1 || actor->spr.scale.Y != 1) return false;
		if (hitActor->spr.scale.X != 1 || hitActor->spr.scale.Y != 1) return false;

        if (hitActor->user.ShellNum <= 0 && hitActor->user.WaitTics <= 0 && actor->user.WaitTics <= 0)
        {
            if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
            {
                DSWPlayer* pp = nullptr;

                if (RandomRange(1000) < 995 && hitActor->user.spal != PALETTE_PLAYER0) return false;

                DoActorPickClosePlayer(actor);

                if (actor->user.targetActor->user.PlayerP)
                    pp = actor->user.targetActor->user.PlayerP;

                if (hitActor->user.spal != PALETTE_PLAYER0)
                {
                    if (hitActor->user.Flag1 > 0) return false;
                    hitActor->user.FlagOwner = 1; // FAG!
                    hitActor->user.Flag1 = SEC(10);
                    if (pp)
                    {
                        int choose_snd;
                        static const int fagsnds[] = {DIGI_FAGRABBIT1,DIGI_FAGRABBIT2,DIGI_FAGRABBIT3};

                        if (pp == getPlayer(myconnectindex))
                        {
                            choose_snd = StdRandomRange(2<<8)>>8;
                            if (FAFcansee(ActorVectOfTop(actor),actor->sector(),pp->GetActor()->getPosWithOffsetZ(), pp->cursector) && Facing(actor, actor->user.targetActor))
                                PlayerSound(fagsnds[choose_snd], v3df_doppler|v3df_follow|v3df_dontpan,pp);
                        }
                    }
                }
                else
                {
                    if (pp && RandomRange(1000) > 200)
                    {
                        int choose_snd;
                        static const int straightsnds[] = {DIGI_RABBITHUMP1,DIGI_RABBITHUMP2, DIGI_RABBITHUMP3,DIGI_RABBITHUMP4};

                        if (pp == getPlayer(myconnectindex))
                        {
                            choose_snd = StdRandomRange(3<<8)>>8;
                            if (FAFcansee(ActorVectOfTop(actor), actor->sector(), pp->GetActor()->getPosWithOffsetZ(), pp->cursector) && Facing(actor, actor->user.targetActor))
                                PlayerSound(straightsnds[choose_snd], v3df_doppler | v3df_follow | v3df_dontpan, pp);
                        }
                    }
                }

                actor->spr.pos.XY() = hitActor->spr.pos.XY();
                actor->spr.Angles.Yaw = hitActor->spr.Angles.Yaw;
                actor->spr.Angles.Yaw += DAngle180;
                HelpMissileLateral(actor, 2000);
                actor->spr.Angles.Yaw = hitActor->spr.Angles.Yaw;

                NewStateGroup(actor, s_BunnyScrew);
                NewStateGroup(hitActor, s_BunnyScrew);
                actor->user.WaitTics = hitActor->user.WaitTics = SEC(10);  // Mate for this long
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullBunny(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    DoActorSectorDamage(actor);

    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyPain(DSWActor* actor)
{
    NullBunny(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);
    return 0;
}

int DoBunnyRipHeart(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;

    NewStateGroup(actor, s_BunnyHeart);
    actor->user.WaitTics = 6 * 120;

    // player face bunny
    target->spr.Angles.Yaw = (actor->spr.pos - target->spr.pos).Angle();
    return 0;
}

int DoBunnyStandKill(DSWActor* actor)
{
    NullBunny(actor);

    // Growl like the bad ass bunny you are!
    if (RandomRange(1000) > 800)
        PlaySound(DIGI_BUNNYATTACK, actor, v3df_none);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(actor, s_BunnyRun);
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void BunnyHatch(DSWActor* actor)
{
    const int MAX_BUNNYS = 1;

    for (int i = 0; i < MAX_BUNNYS; i++)
    {
        auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
        actorNew->spr.pos = actor->spr.pos;
		actorNew->spr.scale = DVector2(0.46875, 0.375);  // Baby size
        actorNew->spr.Angles.Yaw = RandomAngle();
        actorNew->spr.pal = 0;
        SetupBunny(actorNew);
        actorNew->spr.shade = actor->spr.shade;

        // make immediately active
        actorNew->user.Flags |= (SPR_ACTIVE);
        if (RandomRange(1000) > 500) // Boy or Girl?
            actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER0; // Girl
        else
        {
            actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER8; // Boy
            // Oops, mommy died giving birth to a boy
            if (RandomRange(1000) > 500)
            {
                actor->user.Health = 0;
                Bunny_Count--; // Bunny died

                // Blood fountains
                InitBloodSpray(actor, true, -1);

                if (SpawnShrap(actor, actorNew))
                {
                    SetSuicide(actor);
                }
                else
                    DoActorDie(actor, actorNew, 0);
            }
        }

        actorNew->user.ShellNum = 0; // Not Pregnant right now

        actorNew->setStateGroup(NAME_Jump);
        actorNew->user.ActorActionFunc = AF(DoActorMoveJump);
        DoActorSetSpeed(actorNew, FAST_SPEED);
        PickJumpMaxSpeed(actorNew, -600);

        actorNew->user.Flags |= (SPR_JUMPING);
        actorNew->user.Flags &= ~(SPR_FALLING);

        actorNew->user.jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(actorNew);

        DoActorJump(actorNew);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DSWActor* BunnyHatch2(DSWActor* actor)
{
    auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
    actorNew->spr.pos = actor->spr.pos;
	actorNew->spr.scale = DVector2(0.46875, 0.375);  // Baby size
    actorNew->spr.Angles.Yaw = RandomAngle();
    actorNew->spr.pal = 0;
    SetupBunny(actorNew);
    actorNew->spr.shade = actor->spr.shade;

    // make immediately active
    actorNew->user.Flags |= (SPR_ACTIVE);
    if (RandomRange(1000) > 500) // Boy or Girl?
    {
        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER0; // Girl
        actorNew->user.Flag1 = SEC(5);
    }
    else
    {
        actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER8; // Boy
        actorNew->user.Flag1 = 0;
    }

    actorNew->user.ShellNum = 0; // Not Pregnant right now

    actorNew->setStateGroup(NAME_Jump);
    actorNew->user.ActorActionFunc = AF(DoActorMoveJump);
    DoActorSetSpeed(actorNew, FAST_SPEED);
    if (TEST_BOOL3(actor))
    {
        PickJumpMaxSpeed(actorNew, -600-RandomRange(600));
        actorNew->spr.scale = DVector2(1, 1);
        actorNew->vel.X = 9.375 + RandomRangeF(62.5);
        actorNew->user.Health = 1; // Easy to pop. Like shootn' skeet.
		actorNew->spr.Angles.Yaw += RandomAngle(22.5) - RandomAngle(22.5);
    }
    else
        PickJumpMaxSpeed(actorNew, -600);

    actorNew->user.Flags |= (SPR_JUMPING);
    actorNew->user.Flags &= ~(SPR_FALLING);

    actorNew->user.jump_grav = 8;
    actorNew->user.FlagOwner = 0;

    actorNew->user.active_range = 75000; // Set it far

    // if I didn't do this here they get stuck in the air sometimes
    DoActorZrange(actorNew);

    DoActorJump(actorNew);

    return actorNew;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyMove(DSWActor* actor)
{
    // Parental lock crap
    if (actor->spr.cstat & (CSTAT_SPRITE_INVISIBLE))
        actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE); // Turn em' back on

    // Sometimes they just won't die!
    if (actor->user.Health <= 0)
        SetSuicide(actor);

    if (actor->user.scale_speed)
    {
        DoScaleSprite(actor);
    }

    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        DoBunnyQuickJump(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if (RandomRange(1000) > 985 && actor->spr.pal != PALETTE_PLAYER1 && actor->user.track < 0)
    {
        if (tileflags(actor->sector()->floortexture) & TFLAG_BUNNYFRIENDLY)
        {
            NewStateGroup(actor, s_BunnyStand);
        }
        else
        {
			actor->spr.Angles.Yaw = RandomAngle();
            actor->user.jump_speed = -350;
            DoActorBeginJump(actor);
            actor->user.ActorActionFunc = AF(DoActorMoveJump);
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int BunnySpew(DSWActor* actor)
{
    InitBloodSpray(actor, true, -1);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyEat(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        DoBunnyQuickJump(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if (tileflags(actor->sector()->floortexture) & TFLAG_BUNNYFRIENDLY)
    {
        if (RandomRange(1000) > 970)
            NewStateGroup(actor, s_BunnyRun);
    }
    else
    {
        NewStateGroup(actor,s_BunnyRun);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyScrew(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoActorJump(actor);
        else
            DoActorFall(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    // stay on floor unless doing certain things
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    if (RandomRange(1000) > 990) // Bunny sex sounds
    {
         PlaySound(DIGI_BUNNYATTACK, actor, v3df_follow);
    }

    actor->user.WaitTics -= ACTORMOVETICS;

    if ((actor->user.FlagOwner || actor->user.spal == PALETTE_PLAYER0) && actor->user.WaitTics > 0) // Keep Girl still
        NewStateGroup(actor,s_BunnyScrew);

    if (actor->user.spal == PALETTE_PLAYER0 && actor->user.WaitTics <= 0) // Female has baby
    {
        actor->user.Flag1 = SEC(5); // Count down to babies
        actor->user.ShellNum = 1; // She's pregnant now
    }

    if (actor->user.WaitTics <= 0)
    {
        actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE); // Turn em' back on
        actor->user.FlagOwner = 0;
        NewStateGroup(actor,s_BunnyRun);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoBunnyGrowUp(DSWActor* actor)
{
    if (actor->spr.pal == PALETTE_PLAYER1) return 0;   // Don't bother white bunnies

    if ((actor->user.Counter -= ACTORMOVETICS) <= 0)
    {
		actor->spr.scale.X += (REPEAT_SCALE);
		actor->spr.scale.Y += (REPEAT_SCALE);
        if ((actor->spr.scale.X) > 1) actor->spr.scale.X = (1);
		if ((actor->spr.scale.Y) > 1) actor->spr.scale.Y = (1);
        actor->user.Counter = 60;
    }

    // Don't go homo too much!
    if (actor->spr.pal != PALETTE_PLAYER0 && actor->user.Flag1 > 0)
        actor->user.Flag1 -= ACTORMOVETICS;

    // Gestation period for female rabbits
    if (actor->spr.pal == PALETTE_PLAYER0 && actor->user.ShellNum > 0)
    {
        if ((actor->user.Flag1 -= ACTORMOVETICS) <= 0)
        {
            if (Bunny_Count < 20)
            {
                PlaySound(DIGI_BUNNYDIE2, actor, v3df_follow);
                BunnyHatch(actor); // Baby time
            }
            actor->user.ShellNum = 0; // Not pregnent anymore
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

#include "saveable.h"

static saveable_data saveable_bunny_data[] =
{
    SAVE_DATA(WhiteBunnyPersonality),
    SAVE_DATA(BunnyPersonality),

    SAVE_DATA(WhiteBunnyAttrib),
    SAVE_DATA(BunnyAttrib),

    SAVE_DATA(s_BunnyRun),
    SAVE_DATA(s_BunnyStand),
    SAVE_DATA(s_BunnyScrew),
    SAVE_DATA(s_BunnySwipe),
    SAVE_DATA(s_BunnyHeart),
    SAVE_DATA(s_BunnyPain),
    SAVE_DATA(s_BunnyJump),
    SAVE_DATA(s_BunnyFall),
    SAVE_DATA(s_BunnyJumpAttack),
    SAVE_DATA(s_BunnyDie),
    SAVE_DATA(s_BunnyDead),
    SAVE_DATA(s_BunnyDeathJump),
    SAVE_DATA(s_BunnyDeathFall),
    SAVE_DATA(BunnyActionSet),
    SAVE_DATA(BunnyWhiteActionSet),
};

saveable_module saveable_bunny =
{
    // code
    nullptr,
    0,

    // data
    saveable_bunny_data,
    SIZ(saveable_bunny_data)
};
END_SW_NS
