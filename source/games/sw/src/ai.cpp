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
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"

// temp - should be moved
#include "ai.h"

#include "network.h"

BEGIN_SW_NS

bool DropAhead(DSWActor* actor, double min_height);

VMFunction* ChooseAction(DECISION decision[]);


#define CHOOSE2(value) (RANDOM_P2(1024) < (value))

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ActorMoveHitReact(DSWActor* actor)
{
    // Should only return true if there is a reaction to what was hit that
    // would cause the calling function to abort

    auto coll = actor->user.coll;
    if (coll.type == kHitSprite)
    {
        auto hitActor = coll.actor();
        if (hitActor->hasU() && hitActor->user.PlayerP)
        {
            // if you ran into a player - call close range functions
            DoActorPickClosePlayer(actor);
            auto action = ChooseAction(actor->user.Personality->TouchTarget);
            actor->callFunction(action);
        }
    }
    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ActorFlaming(DSWActor* actor)
{
    auto flame = actor->user.flameActor;
    if (flame != nullptr)
    {
        if (ActorSizeZ(flame) > ActorSizeZ(actor) * 0.75)
            return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoActorSetSpeed(DSWActor* actor, uint8_t speed)
{
    if (actor->spr.cstat & (CSTAT_SPRITE_RESTORE))
        return;

    ASSERT(actor->user.__legacyState.Attrib);

    actor->user.speed = speed;

    int vel = actor->user.__legacyState.Attrib->Speed[speed];
    if (ActorFlaming(actor))
        vel = (vel * 3) >> 1;

    actor->vel.X = vel * maptoworld;
}

//---------------------------------------------------------------------------
/*
  !AIC - Does a table lookup based on a random value from 0 to 1023.
  These tables are defined at the top of all actor files such as ninja.c,
  goro.c etc.
*/
//---------------------------------------------------------------------------

VMFunction* ChooseAction(DECISION decision[])
{
    // !JIM! Here is an opportunity for some AI, instead of randomness!
    int random_value = RANDOM_P2(1024<<5)>>5;

    for (int i = 0; true; i++)
    {
        ASSERT(i < 10);

        if (random_value <= decision[i].range)
        {
            return *decision[i].action;
        }
    }
}

int ChooseNoise(DECISIONB decision[])
{
    // !JIM! Here is an opportunity for some AI, instead of randomness!
    int random_value = RANDOM_P2(1024 << 5) >> 5;

    for (int i = 0; true; i++)
    {
        ASSERT(i < 10);

        if (random_value <= decision[i].range)
        {
            return decision[i].noise;
        }
    }
}


//---------------------------------------------------------------------------
/*
  !AIC - Sometimes just want the offset of the action
*/
//---------------------------------------------------------------------------

int ChooseActionNumber(int16_t decision[])
{
    int random_value = RANDOM_P2(1024<<5)>>5;

    for (int i = 0; true; i++)
    {
        if (random_value <= decision[i])
        {
            return i;
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorNoise(DSWActor* actor, int noise)
{
    if (noise == attr_alert)
    {
        if (!actor->hasU() || actor->user.DidAlert) // This only allowed once
            return 0;
    }
    PlaySpriteSound(actor, noise, v3df_follow);
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool CanSeePlayer(DSWActor* actor)
{
    return actor->user.targetActor && FAFcansee(ActorVectOfTop(actor), actor->sector(), ActorUpperVect(actor->user.targetActor), actor->user.targetActor->sector());
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int CanHitPlayer(DSWActor* actor)
{
    HitInfo hit{};
    DVector3 vect;
    // if actor can still see the player

    DSWActor* targ = actor->user.targetActor;
    DVector3 apos = actor->spr.pos.plusZ(-ActorSizeZ(actor) * 0.5);
    DVector3 tpos = targ->spr.pos.plusZ(-ActorSizeZ(targ) * 0.5);
    auto vec = (tpos - apos).Unit() * 1024;

    FAFhitscan(apos, actor->sector(), vec, hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
        return false;

    if (hit.actor() == actor->user.targetActor)
        return true;

    return false;
}

//---------------------------------------------------------------------------
/*
  !AIC - Pick a nearby player to be the actors target
*/
//---------------------------------------------------------------------------

int DoActorPickClosePlayer(DSWActor* actor)
{
    int pnum;
    SWPlayer* pp;
    // if actor can still see the player
    bool found = false;
    int i;

    double near_dist = MAX_ACTIVE_RANGE;
    double dist;

    if (actor->user.ID == ZOMBIE_RUN_R0 && gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
        goto TARGETACTOR;

    // Set initial target to Player 0
    actor->user.targetActor = Player->GetActor();

    if (actor->user.Flags2 & (SPR2_DONT_TARGET_OWNER))
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            if (GetOwner(actor) == pp->GetActor())
                continue;

            actor->user.targetActor = pp->GetActor();
            break;
        }
    }


    // Set initial target to the closest player
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        // Zombies don't target their masters!
        if (actor->user.Flags2 & (SPR2_DONT_TARGET_OWNER))
        {
            if (GetOwner(actor) == pp->GetActor())
                continue;

            if (!PlayerTakeDamage(pp, actor))
                continue;

            // if co-op don't hurt teammate
            // if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE && !gNet.HurtTeammate && actor->user.spal == pp->actor->spr.spal)
            //    continue;
        }

        dist = (actor->spr.pos - pp->GetActor()->getPosWithOffsetZ()).Length();

        if (dist < near_dist)
        {
            near_dist = dist;
            actor->user.targetActor = pp->GetActor();
        }
    }

    // see if you can find someone close that you can SEE
    near_dist = MAX_ACTIVE_RANGE;
    found = false;
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        // Zombies don't target their masters!
        if (actor->user.Flags2 & (SPR2_DONT_TARGET_OWNER))
        {
            if (GetOwner(actor) == pp->GetActor())
                continue;

            if (!PlayerTakeDamage(pp, actor))
                continue;
        }

        dist = (actor->spr.pos - pp->GetActor()->getPosWithOffsetZ()).Length();

        DSWActor* plActor = pp->GetActor();

        if (dist < near_dist && FAFcansee(ActorVectOfTop(actor), actor->sector(), ActorUpperVect(plActor), plActor->sector()))
        {
            near_dist = dist;
            actor->user.targetActor = pp->GetActor();
            found = true;
        }
    }


TARGETACTOR:
    // this is only for Zombies right now
    // zombie target other actors
    if (!found && (actor->user.Flags2 & SPR2_DONT_TARGET_OWNER))
    {
        near_dist = MAX_ACTIVE_RANGE;
        SWStatIterator it(STAT_ENEMY);
        while (auto itActor = it.Next())
        {
            if (itActor == actor || !itActor->hasU())
                continue;

            if ((itActor->user.Flags & (SPR_SUICIDE | SPR_DEAD)))
                continue;

            dist = (actor->spr.pos - itActor->spr.pos).Length();

            if (dist < near_dist && FAFcansee(ActorVectOfTop(actor), actor->sector(), ActorUpperVect(itActor), itActor->sector()))
            {
                near_dist = dist;
                actor->user.targetActor = itActor;
            }
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DSWActor* GetPlayerSpriteNum(DSWActor* actor)
{
    int pnum;
    SWPlayer* pp;

    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        if (pp->GetActor() == actor->user.targetActor)
        {
            return pp->GetActor();
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorOperate(DSWActor* actor)
{
    HitInfo near{};
    double z[2];
    unsigned int i;

    if (actor->user.ID == HORNET_RUN_R0 || actor->user.ID == EEL_RUN_R0 || actor->user.ID == BUNNY_RUN_R0)
        return false;

    if (actor->checkStateGroup(NAME_Sit) || actor->checkStateGroup(NAME_Stand))
        return false;

    if ((actor->user.WaitTics -= ACTORMOVETICS) > 0)
        return false;

    z[0] = -ActorSizeZ(actor) + 5;
    z[1] = -(ActorSizeZ(actor) * 0.5);

    for (i = 0; i < SIZ(z); i++)
    {
        neartag(actor->spr.pos.plusZ(z[i]), actor->sector(), actor->spr.Angles.Yaw, near, 64., NT_Lotag | NT_Hitag | NT_NoSpriteCheck);
    }

    if (near.hitSector != nullptr)
    {
        if (OperateSector(near.hitSector, false))
        {
            actor->user.WaitTics = 2 * 120;

            actor->setStateGroup(NAME_Sit);
        }
    }

    return true;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DECISION GenericFlaming[] =
{
    {30, &AF(InitActorAttack)},
    {512, &AF(InitActorRunToward)},
    {1024, &AF(InitActorRunAway)},
};

/*
 !AIC KEY - This routine decides what the actor will do next.  It is not called
 every time through the loop.  This would be too slow.  It is only called when
 the actor needs to know what to do next such as running into something or being
 targeted.  It makes decisions based on the distance and viewablity of its target
 (actor->user.targetActor).  When it figures out the situatation with its target it calls
 ChooseAction which does a random table lookup to decide what action to initialize.
 Once this action is initialized it will be called until it can't figure out what to
 do anymore and then this routine is called again.
*/

VMFunction* DoActorActionDecide(DSWActor* actor)
{
    VMFunction* action;
    bool ICanSee=false;

    // REMINDER: This function is not even called if SpriteControl doesn't let
    // it get called

    ASSERT(actor->user.Personality);

    actor->user.Dist = 0;
    action = AF(InitActorDecide);

    // target is gone.
    if (actor->user.targetActor == nullptr)
    {
        return action;
    }

    if (actor->user.Flags & (SPR_JUMPING | SPR_FALLING))
    {
        //CON_Message("Jumping or falling");
        return action;
    }

    // everybody on fire acts like this
    if (ActorFlaming(actor))
    {
        action = ChooseAction(&GenericFlaming[0]);
        //CON_Message("On Fire");
        return action;
    }

    ICanSee = CanSeePlayer(actor);  // Only need to call once
    // But need the result multiple times

    // !AIC KEY - If aware of player - var is changed in SpriteControl
    if (actor->user.Flags & (SPR_ACTIVE))
    {

        // Try to operate stuff
        DoActorOperate(actor);

        // if far enough away and cannot see the player
        double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

        if (dist > 1875 && !ICanSee)
        {
            // Enemy goes inactive - he is still allowed to roam about for about
            // 5 seconds trying to find another player before his active_range is
            // bumped down
            actor->user.Flags &= ~(SPR_ACTIVE);

            // You've lost the player - now decide what to do
            action = ChooseAction(actor->user.Personality->LostTarget);
            //CON_Message("LostTarget");
            return action;
        }


        auto pActor = GetPlayerSpriteNum(actor);
        // check for short range attack possibility
        if ((dist < CloseRangeDist(actor, actor->user.targetActor) && ICanSee) ||
            (pActor && pActor->hasU() && pActor->user.WeaponNum == WPN_FIST && actor->user.ID != RIPPER2_RUN_R0 && actor->user.ID != RIPPER_RUN_R0))
        {
            if ((actor->user.ID == COOLG_RUN_R0 && (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)) || (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                action = ChooseAction(actor->user.Personality->Evasive);
            else
                action = ChooseAction(actor->user.Personality->CloseRange);
            //CON_Message("CloseRange");
            return action;
        }

        // if player is facing me and I'm being attacked
        if (Facing(actor, actor->user.targetActor) && (actor->user.Flags & SPR_ATTACKED) && ICanSee)
        {
            // if I'm a target - at least one missile comming at me
            if (actor->user.Flags & (SPR_TARGETED))
            {
                // not going to evade, reset the target bit
                actor->user.Flags &= ~(SPR_TARGETED);        // as far as actor
                // knows, its not a
                // target any more
                if (actor->hasState(NAME_Duck) && RANDOM_P2(1024<<8)>>8 < 100)
                    action = AF(InitActorDuck);
                else
                {
                    if ((actor->user.ID == COOLG_RUN_R0 && (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)) || (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                        action = ChooseAction(actor->user.Personality->Evasive);
                    else
                        action = ChooseAction(actor->user.Personality->Battle);
                }
                //CON_Message("Battle 1");
                return action;
            }
            // if NOT a target - don't bother with evasive action and start
            // fighting
            else
            {
                if ((actor->user.ID == COOLG_RUN_R0 && (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)) || (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                    action = ChooseAction(actor->user.Personality->Evasive);
                else
                    action = ChooseAction(actor->user.Personality->Battle);
                //CON_Message("Battle 2");
                return action;
            }

        }
        // if player is NOT facing me he is running or unaware of actor
        else if (ICanSee)
        {
            if ((actor->user.ID == COOLG_RUN_R0 && (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)) || (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                action = ChooseAction(actor->user.Personality->Evasive);
            else
                action = ChooseAction(actor->user.Personality->Offense);
            //CON_Message("Offense");
            return action;
        }
        else
        {
            // You've lost the player - now decide what to do
            action = ChooseAction(actor->user.Personality->LostTarget);
            //CON_Message("Close but cant see, LostTarget");
            return action;
        }
    }
    // Not active - not aware of player and cannot see him
    else
    {
        // try and find another player
        // pick a closeby player as the (new) target
        if (actor->spr.hitag != TAG_SWARMSPOT)
            DoActorPickClosePlayer(actor);

        // if close by
		double const dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();
        if (dist < 937.5 || ICanSee)
        {
            if ((Facing(actor, actor->user.targetActor) && dist < 625) || ICanSee)
            {
                DoActorOperate(actor);

                // Don't let player completely sneek up behind you
                action = ChooseAction(actor->user.Personality->Surprised);
                //CON_Message("Surprised");
                if (!actor->user.DidAlert && ICanSee)
                {
                    DoActorNoise(actor, attr_alert);
                    actor->user.DidAlert = true;
                }
                return action;

            }
            else
            {
                // Player has not seen actor, to be fair let him know actor
                // are there
                ;
                DoActorNoise(actor, ChooseNoise(actor->user.Personality->Broadcast));
                return action;
            }
        }
    }

    //CON_Message("Couldn't resolve decide, &AF(InitActorDecide)");
    return action;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorDecide(DSWActor* actor)
{
    actor->setActionDecide();
    return DoActorDecide(actor);
}

int InitActorSetDecide(DSWActor* actor)
{
    actor->setActionDecide();
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorDecide(DSWActor* actor)
{
    VMFunction* actor_action;

    // See what to do next

    actor_action = DoActorActionDecide(actor);

    // Fix for the GenericFlaming bug for actors that don't have attack states
    if (actor_action == AF(InitActorAttack) && actor->user.WeaponNum == 0)
        return 0;   // Just let the actor do as it was doing before in this case

    // Target is gone.
    if (actor->user.targetActor == nullptr)
        return 0;

    // zombie is attacking a player
    if (actor_action == AF(InitActorAttack) && actor->user.ID == ZOMBIE_RUN_R0 && actor->user.targetActor->user.PlayerP)
    {
        // Don't let zombies shoot at master
        if (GetOwner(actor) == actor->user.targetActor)
            return 0;

        // if this player cannot take damage from this zombie(weapon) return out
        if (!PlayerTakeDamage(actor->user.targetActor->user.PlayerP, actor))
            return 0;
    }

    ASSERT(actor_action != nullptr);

    if (actor_action != AF(InitActorDecide))
    {
        // NOT staying put
        actor->callFunction(actor_action);
    }
    else
    {
        // Actually staying put
        actor->setStateGroup(NAME_Stand);
    }

    return 0;
}

//---------------------------------------------------------------------------
/*
  !AIC KEY - Routines handle moving toward the player.
*/
//---------------------------------------------------------------------------

int InitActorMoveCloser(DSWActor* actor)
{
    actor->user.ActorActionFunc = AF(DoActorMoveCloser);

    if (!actor->checkStateGroup(NAME_Run))
        actor->setStateGroup(NAME_Run);

    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorCantMoveCloser(DSWActor* actor)
{
    actor->user.track = FindTrackToPlayer(actor);

    if (actor->user.track >= 0)
    {
        auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
        actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();

        DoActorSetSpeed(actor, MID_SPEED);
        actor->user.Flags |= (SPR_FIND_PLAYER);

        actor->setActionDecide();
        actor->setStateGroup(NAME_Run);
    }
    else
    {
        // Try to move closer
        InitActorReposition(actor);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorMoveCloser(DSWActor* actor)
{
    // if cannot move the sprite
    if (!move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0)))
    {
        if (ActorMoveHitReact(actor))
            return 0;

        DoActorCantMoveCloser(actor);
        return 0;
    }

    // Do a noise if ok
    DoActorNoise(actor, ChooseNoise(actor->user.Personality->Broadcast));

    // after moving a ways check to see if player is still in sight
    if (actor->user.DistCheck > 34.375)
    {
        actor->user.DistCheck = 0;

        // If player moved out of sight
        if (!CanSeePlayer(actor))
        {
            // stay put and choose another option
            InitActorDecide(actor);
            return 0;
        }
        else
        {
            // turn to face player
            actor->spr.Angles.Yaw = (actor->user.targetActor->spr.pos - actor->spr.pos).Angle();
        }
    }

    // Should be a random value test
    if (actor->user.Dist > 32 * 3)
    {
        InitActorDecide(actor);
    }

    return 0;

}

//---------------------------------------------------------------------------
/*
  !AIC - Find tracks of different types.  Toward target, away from target, etc.
*/
//---------------------------------------------------------------------------

int FindTrackToPlayer(DSWActor* actor)
{
    int point, track_dir, track;
    int i, size;
    const uint16_t* type;
    double zdiff;

    static const uint16_t PlayerAbove[] =
    {
        BIT(TT_LADDER),
        BIT(TT_STAIRS),
        BIT(TT_JUMP_UP),
        BIT(TT_TRAVERSE),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    static const uint16_t PlayerBelow[] =
    {
        BIT(TT_JUMP_DOWN),
        BIT(TT_STAIRS),
        BIT(TT_TRAVERSE),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    static const uint16_t PlayerOnLevel[] =
    {
        BIT(TT_DUCK_N_SHOOT),
        BIT(TT_HIDE_N_SHOOT),
        BIT(TT_TRAVERSE),
        BIT(TT_EXIT),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    zdiff = ActorUpperZ(actor->user.targetActor) - (actor->spr.pos.Z - ActorSizeZ(actor) + 8);

    if (abs(zdiff) <= 20)
    {
        type = PlayerOnLevel;
        size = SIZ(PlayerOnLevel);
    }
    else
    {
        if (zdiff < 0)
        {
            type = PlayerAbove;
            size = SIZ(PlayerAbove);
        }
        else
        {
            type = PlayerBelow;
            size = SIZ(PlayerBelow);
        }
    }


    for (i = 0; i < size; i++)
    {
        track = ActorFindTrack(actor, 1, type[i], &point, &track_dir);

        if (track >= 0)
        {
            actor->user.point = point;
            actor->user.track_dir = track_dir;
            Track[track].flags |= (TF_TRACK_OCCUPIED);

            return track;
        }
    }

    return -1;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int FindTrackAwayFromPlayer(DSWActor* actor)
{
    int point, track_dir, track;
    unsigned int i;

    static const int16_t RunAwayTracks[] =
    {
        BIT(TT_EXIT),
        BIT(TT_LADDER),
        BIT(TT_TRAVERSE),
        BIT(TT_STAIRS),
        BIT(TT_JUMP_UP),
        BIT(TT_JUMP_DOWN),
        BIT(TT_DUCK_N_SHOOT),
        BIT(TT_HIDE_N_SHOOT),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    for (i = 0; i < SIZ(RunAwayTracks); i++)
    {
        track = ActorFindTrack(actor, -1, RunAwayTracks[i], &point, &track_dir);

        if (track >= 0)
        {
            actor->user.point = point;
            actor->user.track_dir = track_dir;
            Track[track].flags |= (TF_TRACK_OCCUPIED);

            return track;
        }
    }

    return -1;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int FindWanderTrack(DSWActor* actor)
{
    int point, track_dir, track;
    unsigned int i;

    static const int16_t WanderTracks[] =
    {
        BIT(TT_DUCK_N_SHOOT),
        BIT(TT_HIDE_N_SHOOT),
        BIT(TT_WANDER),
        BIT(TT_JUMP_DOWN),
        BIT(TT_JUMP_UP),
        BIT(TT_TRAVERSE),
        BIT(TT_STAIRS),
        BIT(TT_LADDER),
        BIT(TT_EXIT),
        BIT(TT_OPERATE)
    };

    for (i = 0; i < SIZ(WanderTracks); i++)
    {
        track = ActorFindTrack(actor, -1, WanderTracks[i], &point, &track_dir);

        if (track >= 0)
        {
            actor->user.point = point;
            actor->user.track_dir = track_dir;
            Track[track].flags |= (TF_TRACK_OCCUPIED);

            return track;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorRunAway(DSWActor* actor)
{
    actor->setActionDecide();
    actor->setStateGroup(NAME_Run);

    actor->user.track = FindTrackAwayFromPlayer(actor);

    if (actor->user.track >= 0)
    {
        auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
        actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        DoActorSetSpeed(actor, FAST_SPEED);
        actor->user.Flags |= (SPR_RUN_AWAY);
    }
    else
    {
        actor->user.Flags |= (SPR_RUN_AWAY);
        InitActorReposition(actor);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorRunToward(DSWActor* actor)
{
    actor->setActionDecide();
    actor->setStateGroup(NAME_Run);

    InitActorReposition(actor);
    DoActorSetSpeed(actor, FAST_SPEED);

    return 0;
}

//---------------------------------------------------------------------------
/*
  !AIC - Where actors do their attacks.  There is some special case code throughout
  these.  Both close and long range attacks are handled here by transitioning to
  the correct attack state.
*/
//---------------------------------------------------------------------------

int InitActorAttack(DSWActor* actor)
{
    // zombie is attacking a player
    if (actor->user.ID == ZOMBIE_RUN_R0 && actor->user.targetActor->hasU() && actor->user.targetActor->user.PlayerP)
    {
        // Don't let zombies shoot at master
        if (GetOwner(actor) == actor->user.targetActor)
            return 0;

        // if this player cannot take damage from this zombie(weapon) return out
        if (!PlayerTakeDamage(actor->user.targetActor->user.PlayerP, actor))
            return 0;
    }

    if ((actor->user.targetActor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT))
    {
        InitActorRunAway(actor);
        return 0;
    }

    if (actor->user.targetActor->hasU() && actor->user.targetActor->user.Health <= 0)
    {
        DoActorPickClosePlayer(actor);
        InitActorReposition(actor);
        return 0;
    }

    if (!CanHitPlayer(actor))
    {
        InitActorReposition(actor);
        return 0;
    }

    // if the guy you are after is dead, look for another and
    // reposition
    if (actor->user.targetActor->hasU() && actor->user.targetActor->user.PlayerP &&
        (actor->user.targetActor->user.PlayerP->Flags & PF_DEAD))
    {
        DoActorPickClosePlayer(actor);
        InitActorReposition(actor);
        return 0;
    }

    actor->user.ActorActionFunc = AF(DoActorAttack);

    // move into standing frame
    //actor->setStateGroup(NAME_Stand);

    // face player when attacking
    actor->spr.Angles.Yaw = (actor->user.targetActor->spr.pos - actor->spr.pos).Angle();

    // If it's your own kind, lay off!
    if (actor->user.ID == actor->user.targetActor->user.ID && !actor->user.targetActor->user.PlayerP)
    {
        InitActorRunAway(actor);
        return 0;
    }

    // Hari Kari for Ninja's
    if (actor->hasState(NAME_Death2))
    {
        const int SUICIDE_HEALTH_VALUE = 38;

        if (actor->user.Health < SUICIDE_HEALTH_VALUE)
        {
            if (CHOOSE2(100))
            {
                actor->setActionDecide();
                actor->setStateGroup(NAME_Death2);
                return 0;
            }
        }
    }


    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorAttack(DSWActor* actor)
{
    int rand_num;

    DoActorNoise(actor, ChooseNoise(actor->user.Personality->Broadcast));

    double dist =(actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

    auto pActor = GetPlayerSpriteNum(actor);
    if ((actor->user.__legacyState.ActorActionSet->CloseAttack[0] && dist < CloseRangeDist(actor, actor->user.targetActor)) ||
        (pActor && pActor->hasU() && pActor->user.WeaponNum == WPN_FIST))      // JBF: added null check
    {
        rand_num = ChooseActionNumber(actor->user.__legacyState.ActorActionSet->CloseAttackPercent);

        actor->setStateGroup(NAME_CloseAttack, rand_num);
    }
    else
    {
        ASSERT(actor->user.WeaponNum != 0);

        rand_num = ChooseActionNumber(actor->user.__legacyState.ActorActionSet->AttackPercent);

        ASSERT(rand_num < actor->user.WeaponNum);

        actor->setStateGroup(NAME_Attack, rand_num);
        actor->setActionDecide();
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorEvade(DSWActor* actor)
{
    // Evade is same thing as run away except when you get to the end of the track
    // you stop and take up the fight again.

    actor->setActionDecide();
    actor->setStateGroup(NAME_Run);

    actor->user.track = FindTrackAwayFromPlayer(actor);

    if (actor->user.track >= 0)
    {
        auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
        actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        DoActorSetSpeed(actor, FAST_SPEED);
        // NOT doing a RUN_AWAY
        actor->user.Flags &= ~(SPR_RUN_AWAY);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorWanderAround(DSWActor* actor)
{
    actor->setActionDecide();
    actor->setStateGroup(NAME_Run);

    DoActorPickClosePlayer(actor);

    actor->user.track = FindWanderTrack(actor);

    if (actor->user.track >= 0)
    {
        auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
        actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        DoActorSetSpeed(actor, NORM_SPEED);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorFindPlayer(DSWActor* actor)
{
    actor->setActionDecide();
    actor->setStateGroup(NAME_Run);

    actor->user.track = FindTrackToPlayer(actor);

    if (actor->user.track >= 0)
    {
        auto tp = Track[actor->user.track].TrackPoint + actor->user.point;
        actor->spr.Angles.Yaw = (tp->pos - actor->spr.pos).Angle();
        DoActorSetSpeed(actor, MID_SPEED);
        actor->user.Flags |= (SPR_FIND_PLAYER);

        actor->setActionDecide();
        actor->setStateGroup(NAME_Run);
    }
    else
    {
        InitActorReposition(actor);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int InitActorDuck(DSWActor* actor)
{
    if (!actor->hasState(NAME_Duck))
    {
        actor->setActionDecide();
        return 0;
    }

    actor->user.ActorActionFunc = AF(DoActorDuck);
    actor->setStateGroup(NAME_Duck);

	double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).LengthSquared();

    if (dist > 500*500)
    {
        actor->user.WaitTics = 190;
    }
    else
    {
        //actor->user.WaitTics = 120;
        actor->user.WaitTics = 60;
    }


    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorDuck(DSWActor* actor)
{
    if ((actor->user.WaitTics -= ACTORMOVETICS) < 0)
    {
        actor->setStateGroup(NAME_Rise);
        actor->setActionDecide();
        actor->user.Flags &= ~(SPR_TARGETED);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorMoveJump(DSWActor* actor)
{
	move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0));

    if (!(actor->user.Flags & (SPR_JUMPING|SPR_FALLING)))
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

Collision move_scan(DSWActor* actor, DAngle ang, double dst, DVector3& stop)
{
    uint32_t cliptype = CLIPMASK_ACTOR;

    DSWActor* highActor;
    DSWActor* lowActor;
    sectortype* lo_sectp,* hi_sectp, *ssp;


    // moves out a bit but keeps the sprites original postion/sector.

    // save off position info
	auto pos = actor->spr.pos;
    auto sang = actor->spr.Angles.Yaw;
    auto loz = actor->user.loz;
    auto hiz = actor->user.hiz;
    lowActor = actor->user.lowActor;
    highActor = actor->user.highActor;
    lo_sectp = actor->user.lo_sectp;
    hi_sectp = actor->user.hi_sectp;
    ssp = actor->sector();

    // do the move
    actor->spr.Angles.Yaw = ang;
	auto vec = ang.ToVector() * dst;

    Collision ret = move_sprite(actor, DVector3(vec, 0), actor->user.ceiling_dist, actor->user.floor_dist, cliptype, 1);
    // move_sprite DOES do a getzrange point?

    // should I look down with a FAFgetzrange to see where I am?

    // remember where it stopped
    stop = actor->spr.pos;

    // reset position information
	actor->spr.pos = pos;
    actor->spr.Angles.Yaw = sang;
    actor->user.loz = loz;
    actor->user.hiz = hiz;
    actor->user.lowActor = lowActor;
    actor->user.highActor = highActor;
    actor->user.lo_sectp = lo_sectp;
    actor->user.hi_sectp = hi_sectp;
    ChangeActorSect(actor, ssp);

    return ret;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

enum
{
    TOWARD = 1,
    AWAY = -1
};

DAngle FindNewAngle(DSWActor* actor, int dir, double DistToMove)
{
    static const int16_t toward_angle_delta[4][9] =
    {
        { -160, -384, 160, 384, -256, 256, -512, 512, -99},
        { -384, -160, 384, 160, -256, 256, -512, 512, -99},
        { 160, 384, -160, -384, 256, -256, 512, -512, -99},
        { 384, 160, -384, -160, 256, -256, 512, -512, -99}
    };

    static const int16_t away_angle_delta[4][8] =
    {
        { -768, 768, -640, 640, -896, 896, 1024, -99},
        { 768, -768, 640, -640, -896, 896, 1024, -99},
        { 896, -896, -768, 768, -640, 640, 1024, -99},
        { 896, -896, 768, -768, 640, -640, 1024, -99}
    };


    const int16_t* adp = nullptr;

    DAngle new_ang;
    DAngle save_ang = -minAngle;
    bool save_set = false;
    
    int set;
    // start out with mininum distance that will be accepted as a move
    double save_dist = 31.25;

    // if on fire, run shorter distances
    if (ActorFlaming(actor))
        DistToMove *= .375;

    // Find angle to from the player
    auto oang = (actor->user.targetActor->spr.pos - actor->spr.pos).Angle();

    // choose a random angle array
    switch (dir)
    {
    case TOWARD:
        set = RANDOM_P2(4<<8)>>8;
        adp = &toward_angle_delta[set][0];
        break;
    case AWAY:
        set = RANDOM_P2(4<<8)>>8;
        if (CanHitPlayer(actor))
        {
            adp = &toward_angle_delta[set][0];
        }
        else
        {
            adp = &away_angle_delta[set][0];
        }
        break;
    default:
        Printf("FindNewAngle called with dir=%d!\n",dir);
        return nullAngle;
    }

    for (; *adp != -99; adp++)
    {
        new_ang = oang + mapangle(* adp);

#if 1
        // look directly ahead for a ledge
        if (!(actor->user.Flags & (SPR_NO_SCAREDZ | SPR_JUMPING | SPR_FALLING | SPR_SWIMMING | SPR_DEAD)))
        {
            actor->spr.Angles.Yaw = new_ang;
            if (DropAhead(actor, actor->user.lo_step))
            {
                actor->spr.Angles.Yaw = oang;
                continue;
            }
            actor->spr.Angles.Yaw = oang;
        }
#endif

        DVector3 stop;

        // check to see how far we can move
        auto ret = move_scan(actor, new_ang, DistToMove, stop);
        double dist = (actor->spr.pos.XY() - stop.XY()).Length();

        if (ret.type == kHitNone)
        {
            // cleanly moved in new direction without hitting something
            actor->user.TargetDist = dist;
            return new_ang;
        }
        else
        {
            // hit something

            if (dist > save_dist)
            {
                save_ang = new_ang;
                save_set = true;
                save_dist = dist;
            }
        }
    }

    if (save_set)
    {
        actor->user.TargetDist = save_dist;

        // If actor moved to the TargetDist it would look like he was running
        // into things.

        // To keep this from happening make the TargetDist is less than the
        // point you would hit something

        if (actor->user.TargetDist > 250)
            actor->user.TargetDist -= 218.75;

        actor->spr.Angles.Yaw = save_ang;
        return save_ang;
    }

    return -minAngle;
}


//---------------------------------------------------------------------------
/*
  !AIC KEY - Reposition code is called throughout this file.  What this does is
  pick a new direction close to the target direction (or away from the target
  direction if running away) and a distance to move in and tries to move there
  with move_scan(). If it hits something it will try again.  No movement is
  actually acomplished here. This is just testing for clear paths to move in.
  Location variables that are changed are saved and reset.  FindNewAngle() and
  move_scan() are two routines (above) that go with this.  This is definately
  not called every time through the loop.  It would be majorly slow.
*/
//---------------------------------------------------------------------------

int InitActorReposition(DSWActor* actor)
{
    DAngle ang;
    int rnum;
    double dist;

    static const int16_t AwayDist[8] =
    {
        17000 / 16,
        20000 / 16,
        26000 / 16,
        26000 / 16,
        26000 / 16,
        32000 / 16,
        32000 / 16,
        42000 / 16
    };

    static const int16_t TowardDist[8] =
    {
        10000 / 16,
        15000 / 16,
        20000 / 16,
        20000 / 16,
        25000 / 16,
        30000 / 16,
        35000 / 16,
        40000 / 16
    };

    static const int16_t PlayerDist[8] =
    {
        2000 / 16,
        3000 / 16,
        3000 / 16,
        5000 / 16,
        5000 / 16,
        5000 / 16,
        9000 / 16,
        9000 / 16
    };

    actor->user.Dist = 0;

    rnum = RANDOM_P2(8<<8)>>8;
	dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

    if (dist < PlayerDist[rnum] || (actor->user.Flags & SPR_RUN_AWAY))
    {
        rnum = RANDOM_P2(8<<8)>>8;
        ang = FindNewAngle(actor, AWAY, AwayDist[rnum]);
        if (ang == -minAngle)
        {
            actor->user.Vis = 8;
            InitActorPause(actor);
            return 0;
        }

        actor->spr.Angles.Yaw = ang;
        DoActorSetSpeed(actor, FAST_SPEED);
        actor->user.Flags &= ~(SPR_RUN_AWAY);
    }
    else
    {
        // try to move toward player
        rnum = RANDOM_P2(8<<8)>>8;
        ang = FindNewAngle(actor, TOWARD, TowardDist[rnum]);
        if (ang == -minAngle)
        {
            // try to move away from player
            rnum = RANDOM_P2(8<<8)>>8;
            ang = FindNewAngle(actor, AWAY, AwayDist[rnum]);
            if (ang == -minAngle)
            {
                actor->user.Vis = 8;
                InitActorPause(actor);
                return 0;
            }
        }
        else
        {
            // pick random speed to move toward the player
            if (RANDOM_P2(1024) < 512)
                DoActorSetSpeed(actor, NORM_SPEED);
            else
                DoActorSetSpeed(actor, MID_SPEED);
        }

        actor->spr.Angles.Yaw = ang;
    }


    actor->user.ActorActionFunc = AF(DoActorReposition);
    if (!(actor->user.Flags & SPR_SWIMMING))
        actor->setStateGroup(NAME_Run);

    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorReposition(DSWActor* actor)
{
    // still might hit something and have to handle it.
    if (!move_actor(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, 0)))
    {
        if (ActorMoveHitReact(actor))
            return 0;

        actor->user.Vis = 6;
        InitActorPause(actor);
        return 0;
    }

    // if close to target distance do a Decision again
    if (actor->user.TargetDist < 3.125)
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

int InitActorPause(DSWActor* actor)
{
    actor->user.ActorActionFunc = AF(DoActorPause);

    actor->callAction();

    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoActorPause(DSWActor* actor)
{
    // Using Vis instead of WaitTics, var name sucks, but it's the same type
    // WaitTics is used by too much other actor code and causes problems here
    if ((actor->user.Vis -= ACTORMOVETICS) < 0)
    {
        actor->setActionDecide();
        actor->user.Flags &= ~(SPR_TARGETED);
    }

    return 0;
}


END_SW_NS
