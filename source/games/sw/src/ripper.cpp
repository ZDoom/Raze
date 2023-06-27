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
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "pal.h"
#include "sprite.h"
#include "misc.h"

BEGIN_SW_NS

DECISION RipperBattle[] =
{
    {748, &AF(InitActorMoveCloser)},
    {750, &AF(InitActorSetDecide)},
    {755, &AF(InitActorSetDecide)},
    {1024, &AF(InitActorAttack)}
};

DECISION RipperOffense[] =
{
    {700, &AF(InitActorMoveCloser)},
    {710, &AF(InitActorSetDecide)},
    {1024, &AF(InitActorAttack)}
};

DECISIONB RipperBroadcast[] =
{
    {3, attr_alert},
    {6, attr_ambient},
    {1024, 0}
};

DECISION RipperSurprised[] =
{
    {30, &AF(InitRipperHang)},
    {701, &AF(InitActorMoveCloser)},
    {1024, &AF(InitActorDecide)}
};

DECISION RipperEvasive[] =
{
    {6, &AF(InitRipperHang)},
    {1024, nullptr}
};

DECISION RipperLostTarget[] =
{
    {980, &AF(InitActorFindPlayer)},
    {1024, &AF(InitActorWanderAround)}
};

DECISION RipperCloseRange[] =
{
    {900,   &AF(InitActorAttack    )         },
    {1024,  &AF(InitActorReposition)         }
};

PERSONALITY RipperPersonality =
{
    RipperBattle,
    RipperOffense,
    RipperBroadcast,
    RipperSurprised,
    RipperEvasive,
    RipperLostTarget,
    RipperCloseRange,
    RipperCloseRange
};

ATTRIBUTE RipperAttrib =
{
    {200, 220, 240, 280},               // Speeds
    {5, 0, -2, -4},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_RIPPERAMBIENT, DIGI_RIPPERALERT, DIGI_RIPPERATTACK,
        DIGI_RIPPERPAIN, DIGI_RIPPERSCREAM, DIGI_RIPPERHEARTOUT,
        0,0,0,0
    }
};

//////////////////////
//
// RIPPER RUN
//
//////////////////////

#define RIPPER_RUN_RATE 16

FState s_RipperRun[] =
{
        {SPR_RIPPER_RUN, 'A', RIPPER_RUN_RATE | SF_TIC_ADJUST, &AF(DoRipperMove), &s_RipperRun[1]},
        {SPR_RIPPER_RUN, 'B', RIPPER_RUN_RATE | SF_TIC_ADJUST, &AF(DoRipperMove), &s_RipperRun[2]},
        {SPR_RIPPER_RUN, 'C', RIPPER_RUN_RATE | SF_TIC_ADJUST, &AF(DoRipperMove), &s_RipperRun[3]},
        {SPR_RIPPER_RUN, 'D', RIPPER_RUN_RATE | SF_TIC_ADJUST, &AF(DoRipperMove), &s_RipperRun[0]},
};

//////////////////////
//
// RIPPER STAND
//
//////////////////////

#define RIPPER_STAND_RATE 12

FState s_RipperStand[] =
{
        {SPR_RIPPER_STAND, 'A', RIPPER_STAND_RATE, &AF(DoRipperMove), &s_RipperStand[0]},
};


//////////////////////
//
// RIPPER SWIPE
//
//////////////////////

#define RIPPER_SWIPE_RATE 8

FState s_RipperSwipe[] =
{
        {SPR_RIPPER_SWIPE, 'A', RIPPER_SWIPE_RATE, &AF(NullRipper), &s_RipperSwipe[1]},
        {SPR_RIPPER_SWIPE, 'B', RIPPER_SWIPE_RATE, &AF(NullRipper), &s_RipperSwipe[2]},
        {SPR_RIPPER_SWIPE, 'B', 0 | SF_QUICK_CALL, &AF(InitRipperSlash), &s_RipperSwipe[3]},
        {SPR_RIPPER_SWIPE, 'C', RIPPER_SWIPE_RATE, &AF(NullRipper), &s_RipperSwipe[4]},
        {SPR_RIPPER_SWIPE, 'D', RIPPER_SWIPE_RATE, &AF(NullRipper), &s_RipperSwipe[5]},
        {SPR_RIPPER_SWIPE, 'D', 0 | SF_QUICK_CALL, &AF(InitRipperSlash), &s_RipperSwipe[6]},
        {SPR_RIPPER_SWIPE, 'D', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_RipperSwipe[7]},
        {SPR_RIPPER_SWIPE, 'D', RIPPER_SWIPE_RATE, &AF(DoRipperMove), &s_RipperSwipe[7]},
};


//////////////////////
//
// RIPPER SPEW
//
//////////////////////

#define RIPPER_SPEW_RATE 8

FState s_RipperSpew[] =
{
        {SPR_RIPPER_SWIPE, 'A', RIPPER_SPEW_RATE, &AF(NullRipper), &s_RipperSpew[1]},
        {SPR_RIPPER_SWIPE, 'B', RIPPER_SPEW_RATE, &AF(NullRipper), &s_RipperSpew[2]},
        {SPR_RIPPER_SWIPE, 'B', 0 | SF_QUICK_CALL, &AF(InitCoolgFire), &s_RipperSpew[3]},
        {SPR_RIPPER_SWIPE, 'C', RIPPER_SPEW_RATE, &AF(NullRipper), &s_RipperSpew[4]},
        {SPR_RIPPER_SWIPE, 'D', RIPPER_SPEW_RATE, &AF(NullRipper), &s_RipperSpew[5]},
        {SPR_RIPPER_SWIPE, 'D', 0 | SF_QUICK_CALL, &AF(InitActorDecide), &s_RipperSpew[6]},
        {SPR_RIPPER_SWIPE, 'D', RIPPER_SPEW_RATE, &AF(DoRipperMove), &s_RipperSpew[6]},
};


//////////////////////
//
// RIPPER HEART - show players heart
//
//////////////////////

#define RIPPER_HEART_RATE 14

FState s_RipperHeart[] =
{
        {SPR_RIPPER_HEART, 'A', RIPPER_HEART_RATE, &AF(DoRipperStandHeart), &s_RipperHeart[0]},
};

//////////////////////
//
// RIPPER HANG
//
//////////////////////

#define RIPPER_HANG_RATE 14

FState s_RipperHang[] =
{
        {SPR_RIPPER_HANG, 'A', RIPPER_HANG_RATE, &AF(DoRipperHang), &s_RipperHang[0]},
};


//////////////////////
//
// RIPPER PAIN
//
//////////////////////

#define RIPPER_PAIN_RATE 38

FState s_RipperPain[] =
{
        {SPR_RIPPER_JUMP, 'A', RIPPER_PAIN_RATE, &AF(DoRipperPain), &s_RipperPain[0]},
};

//////////////////////
//
// RIPPER JUMP
//
//////////////////////

#define RIPPER_JUMP_RATE 25

FState s_RipperJump[] =
{
        {SPR_RIPPER_JUMP, 'A', RIPPER_JUMP_RATE, &AF(NullRipper), &s_RipperJump[1]},
        {SPR_RIPPER_JUMP, 'B', RIPPER_JUMP_RATE, &AF(DoRipperMoveJump), &s_RipperJump[1]},
};


//////////////////////
//
// RIPPER FALL
//
//////////////////////

#define RIPPER_FALL_RATE 25

FState s_RipperFall[] =
{
        {SPR_RIPPER_FALL, 'A', RIPPER_FALL_RATE, &AF(DoRipperMoveJump), &s_RipperFall[0]},
};


//////////////////////
//
// RIPPER JUMP ATTACK
//
//////////////////////

#define RIPPER_JUMP_ATTACK_RATE 35
int DoRipperBeginJumpAttack(DSWActor* actor);

FState s_RipperJumpAttack[] =
{
        {SPR_RIPPER_JUMP, 'A', RIPPER_JUMP_ATTACK_RATE, &AF(NullRipper), &s_RipperJumpAttack[1]},
        {SPR_RIPPER_JUMP, 'A', 0 | SF_QUICK_CALL, &AF(DoRipperBeginJumpAttack), &s_RipperJumpAttack[2]},
        {SPR_RIPPER_JUMP, 'B', RIPPER_JUMP_ATTACK_RATE, &AF(DoRipperMoveJump), &s_RipperJumpAttack[2]},
};


//////////////////////
//
// RIPPER HANG_JUMP
//
//////////////////////

#define RIPPER_HANG_JUMP_RATE 20

FState s_RipperHangJump[] =
{
        {SPR_RIPPER_JUMP, 'A', RIPPER_HANG_JUMP_RATE, &AF(NullRipper), &s_RipperHangJump[1]},
        {SPR_RIPPER_JUMP, 'B', RIPPER_HANG_JUMP_RATE, &AF(DoRipperHangJF), &s_RipperHangJump[1]},
};


//////////////////////
//
// RIPPER HANG_FALL
//
//////////////////////

#define RIPPER_FALL_RATE 25

FState s_RipperHangFall[] =
{
        {SPR_RIPPER_FALL, 'A', RIPPER_FALL_RATE, &AF(DoRipperHangJF), &s_RipperHangFall[0]},
};


//////////////////////
//
// RIPPER DIE
//
//////////////////////

#define RIPPER_DIE_RATE 16

FState s_RipperDie[] =
{
    {SPR_RIPPER_DIE, 'A', RIPPER_DIE_RATE, &AF(NullRipper), &s_RipperDie[1]},
    {SPR_RIPPER_DIE, 'B', RIPPER_DIE_RATE, &AF(NullRipper), &s_RipperDie[2]},
    {SPR_RIPPER_DIE, 'C', RIPPER_DIE_RATE, &AF(NullRipper), &s_RipperDie[3]},
    {SPR_RIPPER_DIE, 'D', RIPPER_DIE_RATE, &AF(NullRipper), &s_RipperDie[4]},
    {SPR_RIPPER_DEAD, 'A', RIPPER_DIE_RATE, &AF(DoActorDebris), &s_RipperDie[4]},
};

#define RIPPER_DEAD_RATE 8

FState s_RipperDead[] =
{
    {SPR_RIPPER_DIE, 'C', RIPPER_DEAD_RATE, nullptr,  &s_RipperDead[1]},
    {SPR_RIPPER_DIE, 'D', RIPPER_DEAD_RATE, nullptr,  &s_RipperDead[2]},
    {SPR_RIPPER_DEAD, 'A', SF_QUICK_CALL, &AF(QueueFloorBlood), &s_RipperDead[3]},
    {SPR_RIPPER_DEAD, 'A', RIPPER_DEAD_RATE, &AF(DoActorDebris), &s_RipperDead[3]},
};

FState s_RipperDeathJump[] =
{
    {SPR_RIPPER_DIE, 'A', RIPPER_DIE_RATE, &AF(DoActorDeathMove), &s_RipperDeathJump[0]}
};

FState s_RipperDeathFall[] =
{
    {SPR_RIPPER_DIE, 'B', RIPPER_DIE_RATE, &AF(DoActorDeathMove), &s_RipperDeathFall[0]}
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


ACTOR_ACTION_SET RipperActionSet =
{
    s_RipperStand,
    s_RipperRun,
    s_RipperJump,
    s_RipperFall,
    nullptr,                               // s_RipperCrawl,
    nullptr,                               // s_RipperSwim,
    nullptr,                               // s_RipperFly,
    nullptr,                               // s_RipperRise,
    nullptr,                               // s_RipperSit,
    nullptr,                               // s_RipperLook,
    nullptr,                               // climb
    s_RipperPain,
    s_RipperDie,
    nullptr,                               // s_RipperHariKari,
    s_RipperDead,
    s_RipperDeathJump,
    s_RipperDeathFall,
    {s_RipperSwipe,s_RipperSpew},
    {800,1024},
    {s_RipperJumpAttack, s_RipperSpew},
    {400, 1024},
    {s_RipperHeart, s_RipperHang},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET RipperBrownActionSet =
{
    s_RipperStand,
    s_RipperRun,
    s_RipperJump,
    s_RipperFall,
    nullptr,                               // s_RipperCrawl,
    nullptr,                               // s_RipperSwim,
    nullptr,                               // s_RipperFly,
    nullptr,                               // s_RipperRise,
    nullptr,                               // s_RipperSit,
    nullptr,                               // s_RipperLook,
    nullptr,                               // climb
    s_RipperPain,                      // pain
    s_RipperDie,
    nullptr,                               // s_RipperHariKari,
    s_RipperDead,
    s_RipperDeathJump,
    s_RipperDeathFall,
    {s_RipperSwipe},
    {1024},
    {s_RipperJumpAttack, s_RipperSwipe},
    {800, 1024},
    {s_RipperHeart, s_RipperHang},
    nullptr,
    nullptr
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int SetupRipper(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor, RIPPER_RUN_R0, s_RipperRun[0]);
        actor->user.Health = HEALTH_RIPPER/2; // Baby rippers are weaker
    }

    ChangeState(actor, s_RipperRun[0]);
    actor->user.__legacyState.Attrib = &RipperAttrib;
    DoActorSetSpeed(actor, FAST_SPEED);
    actor->user.__legacyState.StateEnd = s_RipperDie;
    actor->user.__legacyState.Rot = s_RipperRun;
    actor->spr.scale = DVector2(1, 1);

    if (actor->spr.pal == PALETTE_BROWN_RIPPER)
    {
        EnemyDefaults(actor, &RipperBrownActionSet, &RipperPersonality);
        actor->spr.scale = DVector2(1.65625, 1.40625);

        if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
            actor->user.Health = HEALTH_MOMMA_RIPPER;

		actor->clipdist += 8;
    }
    else
    {
        EnemyDefaults(actor, &RipperActionSet, &RipperPersonality);
    }

    actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWRipper, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupRipper(self);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetJumpHeight(int jump_speed, int jump_grav)
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

int PickJumpSpeed(DSWActor* actor, int pix_height)
{
    actor->user.jump_speed = -600;
    actor->user.jump_grav = 8;

    while (true)
    {
        if (GetJumpHeight(actor->user.jump_speed, actor->user.jump_grav) > pix_height + 20)
            break;

        actor->user.jump_speed -= 100;

        ASSERT(actor->user.jump_speed > -3000);
    }

    return actor->user.jump_speed;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int PickJumpMaxSpeed(DSWActor* actor, short max_speed)
{
    ASSERT(max_speed < 0);

    actor->user.jump_speed = max_speed;
    actor->user.jump_grav = 8;

    double zh = ActorZOfTop(actor);

    while (true)
    {
        if (zh - GetJumpHeight(actor->user.jump_speed, actor->user.jump_grav) - 16 > actor->user.hiz)
            break;

        actor->user.jump_speed += 100;

        if (actor->user.jump_speed > -200)
            break;
    }

    return actor->user.jump_speed;
}


//---------------------------------------------------------------------------
//
// HANGING - Jumping/Falling/Stationary
//
//---------------------------------------------------------------------------

int InitRipperHang(DSWActor* actor)
{
    HitInfo hit{};
    bool Found = false;

    for (auto dang = nullAngle; dang < DAngle360; dang += DAngle22_5)
    {
        auto tang = actor->spr.Angles.Yaw + dang;

        FAFhitscan(actor->spr.pos.plusZ(-ActorSizeZ(actor)), actor->sector(), DVector3(tang.ToVector() * 1024, 0), hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
            continue;

        double dist = (actor->spr.pos.XY() - hit.hitpos.XY()).Length();

        if (hit.hitWall == nullptr || dist < 125 || dist > 437.5)
        {
            continue;
        }

        Found = true;
        actor->spr.Angles.Yaw = tang;
        break;
    }

    if (!Found)
    {
        InitActorDecide(actor);
        return 0;
    }

    NewStateGroup(actor, s_RipperHangJump);
    actor->user.__legacyState.StateFallOverride = s_RipperHangFall;
    DoActorSetSpeed(actor, FAST_SPEED);

    PickJumpMaxSpeed(actor, -800);

    actor->user.Flags |= (SPR_JUMPING);
    actor->user.Flags &= ~(SPR_FALLING);

    // set up individual actor jump gravity
    actor->user.jump_grav = 8;

    DoJump(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRipperHang(DSWActor* actor)
{
    if ((actor->user.WaitTics -= ACTORMOVETICS) > 0)
        return 0;

    NewStateGroup(actor, s_RipperJumpAttack);
    // move to the 2nd frame - past the pause frame
    actor->user.Tics += actor->user.__legacyState.State->Tics;
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoRipperMoveHang(DSWActor* actor)
{
    // if cannot move the sprite
    if (!move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0)))
    {
        if (actor->user.coll.type == kHitWall)
        {
            actor->setStateGroup(NAME_Special);
            actor->user.WaitTics = 2 + ((RANDOM_P2(4 << 8) >> 8) * 120);

            // hang flush with the wall
            actor->spr.Angles.Yaw = actor->user.coll.hitWall->delta().Angle() - DAngle90;

            return 0;
        }
    }

    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
int DoRipperQuickJump(DSWActor* actor);

int DoRipperHangJF(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipperQuickJump(actor))
            return 0;

        InitActorDecide(actor);
    }

    DoRipperMoveHang(actor);

    return 0;

}

//---------------------------------------------------------------------------
//
// JUMP ATTACK
//
//---------------------------------------------------------------------------

int DoRipperBeginJumpAttack(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;

    auto vec = (target->spr.pos.XY() - actor->spr.pos.XY()).Unit() * 8;
	Collision coll = move_sprite(actor, DVector3(vec, 0), actor->user.ceiling_dist, actor->user.floor_dist, CLIPMASK_ACTOR, ACTORMOVETICS);

    if (coll.type != kHitNone)
		actor->spr.Angles.Yaw += RandomAngle(DAngle45) + DAngle180 - DAngle22_5;
    else
		actor->spr.Angles.Yaw = vec.Angle() + RandomAngle(DAngle45) - DAngle22_5;

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
int DoRipperQuickJump(DSWActor* actor);

int DoRipperMoveJump(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipperQuickJump(actor))
            return 0;

        InitActorDecide(actor);
    }

    DoRipperMoveHang(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
// STD MOVEMENT
//
//---------------------------------------------------------------------------

int DoRipperQuickJump(DSWActor* actor)
{
    // Tests to see if ripper is on top of a player/enemy and then immediatly
    // does another jump

    DSWActor* low = actor->user.lowActor;
    if (low)
    {
        if ((low->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            NewStateGroup(actor, s_RipperJumpAttack);
            // move past the first state
            actor->user.Tics = 30;
            return true;
        }
    }
    return false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int NullRipper(DSWActor* actor)
{
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

int DoRipperPain(DSWActor* actor)
{
    NullRipper(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        InitActorDecide(actor);
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRipperRipHeart(DSWActor* actor)
{
    DSWActor* target = actor->user.targetActor;

    NewStateGroup(actor, s_RipperHeart);
    actor->user.WaitTics = 6 * 120;

    // player face ripper
    target->spr.Angles.Yaw = (actor->spr.pos - target->spr.pos).Angle();
    return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRipperStandHeart(DSWActor* actor)
{
    NullRipper(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
        NewStateGroup(actor, s_RipperRun);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void RipperHatch(DSWActor* actor)
{
	const int MAX_RIPPERS = 1;

    for (int i = 0; i < MAX_RIPPERS; i++)
    {
        auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
        ClearOwner(actorNew);
        actorNew->spr.pos = actor->spr.pos;
        actorNew->spr.scale = DVector2(1, 1);
        actorNew->spr.Angles.Yaw = RandomAngle();
        actorNew->spr.pal = 0;
        SetupRipper(actorNew);

        // make immediately active
        actorNew->user.Flags |= (SPR_ACTIVE);

        actorNew->setStateGroup(NAME_Jump);
        actorNew->user.ActorActionFunc = AF(DoActorMoveJump);
        DoActorSetSpeed(actorNew, FAST_SPEED);
        PickJumpMaxSpeed(actorNew, -600);

        actorNew->user.Flags |= (SPR_JUMPING);
        actorNew->user.Flags &= ~(SPR_FALLING);

        actorNew->user.jump_grav = 8;

        // if I didn't do this here they get stuck in the air sometimes
        DoActorZrange(actorNew);

        DoJump(actorNew);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoRipperMove(DSWActor* actor)
{
    if (actor->user.scale_speed)
    {
        DoScaleSprite(actor);
    }

    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        if (actor->user.Flags & (SPR_JUMPING))
            DoJump(actor);
        else
            DoFall(actor);
    }

    // if on a player/enemy sprite jump quickly
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (DoRipperQuickJump(actor))
            return 0;

        KeepActorOnFloor(actor);
    }

    if (actor->user.Flags & (SPR_SLIDING))
        DoActorSlide(actor);

    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    DoActorSectorDamage(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

#include "saveable.h"

static saveable_data saveable_ripper_data[] =
{
    SAVE_DATA(RipperPersonality),

    SAVE_DATA(RipperAttrib),

    SAVE_DATA(s_RipperRun),
    SAVE_DATA(s_RipperStand),
    SAVE_DATA(s_RipperSwipe),
    SAVE_DATA(s_RipperSpew),
    SAVE_DATA(s_RipperHeart),
    SAVE_DATA(s_RipperHang),
    SAVE_DATA(s_RipperPain),
    SAVE_DATA(s_RipperJump),
    SAVE_DATA(s_RipperFall),
    SAVE_DATA(s_RipperJumpAttack),
    SAVE_DATA(s_RipperHangJump),
    SAVE_DATA(s_RipperHangFall),
    SAVE_DATA(s_RipperDie),
    SAVE_DATA(s_RipperDead),
    SAVE_DATA(s_RipperDeathJump),
    SAVE_DATA(s_RipperDeathFall),

    SAVE_DATA(RipperActionSet),
    SAVE_DATA(RipperBrownActionSet),
};

saveable_module saveable_ripper =
{
    // code
    nullptr, 0,

    // data
    saveable_ripper_data,
    SIZ(saveable_ripper_data)
};
END_SW_NS
