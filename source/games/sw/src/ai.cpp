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

bool PlayerTakeDamage(PLAYERp, short);
ANIMATOR InitActorRunToward;
bool FAF_Sector(short);
bool DropAhead(short SpriteNum, short min_height);

ANIMATORp ChooseAction(DECISION decision[]);


//static short ZigZagDeltaAng[] = {-200, 200};

// Choose between two things
short AttackOrRun = 200;

#define CHOOSE2(value) (RANDOM_P2(1024) < (value))

int Distance(int x1, int y1, int x2, int y2)
{
    int min;

    if ((x2 = x2 - x1) < 0)
        x2 = -x2;

    if ((y2 = y2 - y1) < 0)
        y2 = -y2;

    if (x2 > y2)
        min = y2;
    else
        min = x2;

    return x2 + y2 - DIV2(min);
}


void DebugMoveHit(DSWActor* actor)
{
    //if (u->ret == -1)
    //    printf("Hit a ledge\n");
    //else
    /*
    SPRITEp sp;
    USERp u = actor->u();
    switch (TEST(u->ret, HIT_MASK))
        {
        case HIT_SPRITE:
            sp = &sprite[NORM_SPRITE(u->ret)];
            //DSPRINTF(ds, "Hit a Sprite %d, stat %d ", sp-sprite, (short)sp->statnum);
            if (sp->statnum == STAT_MISSILE)
                {
                //DSPRINTF(ds, "Monster hit bullet %d, stat %d ", sp-sprite, (short)sp->statnum);
                }
            else
                {
                //DSPRINTF(ds, "Hit a Sprite %d, stat %d ", sp-sprite, (short)sp->statnum);
                }
            break;
        case HIT_WALL:
            //DSPRINTF(ds, "Hit a Wall %d    ", NORM_WALL(u->ret));
            break;
        case HIT_SECTOR:
            //DSPRINTF(ds, "Hit a Sector %d  ", NORM_SECTOR(u->ret));
            break;
        }

    MONO_PRINT(ds);
    */
}


bool ActorMoveHitReact(DSWActor* actor)
{
    USERp u = actor->u();
    int SpriteNum = actor->GetSpriteIndex();

    // Should only return true if there is a reaction to what was hit that
    // would cause the calling function to abort

    auto coll = u->hitCode();
    if (coll.type == kHitSprite)
    {
        auto hitActor = coll.actor;
        if (hitActor->hasU() && hitActor->u()->PlayerP)
        {
            // if you ran into a player - call close range functions
            DoActorPickClosePlayer(actor);
            auto action = ChooseAction(u->Personality->TouchTarget);
            if (action)
            {
                (*action)(actor);
                return true;
            }
        }
    }
    return false;
}


bool ActorFlaming(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    if (u->flame >= 0)
    {
        int size;
        SPRITEp fp = &sprite[u->flame];

        size = SPRITEp_SIZE_Z(sp) - DIV4(SPRITEp_SIZE_Z(sp));

        //DSPRINTF(ds,"enemy size %d, flame size %d",size>>8,SPRITEp_SIZE_Z(fp)>>8);

        if (SPRITEp_SIZE_Z(fp) > size)
            return true;
    }

    return false;
}

void DoActorSetSpeed(DSWActor* actor, uint8_t speed)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    if (TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
        return;

    ASSERT(u->Attrib);

    u->speed = speed;

    if (ActorFlaming(actor))
        sp->xvel = u->Attrib->Speed[speed] + DIV2(u->Attrib->Speed[speed]);
    else
        sp->xvel = u->Attrib->Speed[speed];
}

/*
  !AIC - Does a table lookup based on a random value from 0 to 1023.
  These tables are defined at the top of all actor files such as ninja.c,
  goro.c etc.
*/

ANIMATORp ChooseAction(DECISION decision[])
{
    short random_value;
    short i;

    // !JIM! Here is an opportunity for some AI, instead of randomness!
    random_value = RANDOM_P2(1024<<5)>>5;

    for (i = 0; true; i++)
    {
        ASSERT(i < 10);

        if (random_value <= decision[i].range)
        {
            return decision[i].action;
        }
    }
}

/*
  !AIC - Sometimes just want the offset of the action
*/

short ChooseActionNumber(short decision[])
{
    short random_value;
    short i;

    random_value = RANDOM_P2(1024<<5)>>5;

    for (i = 0; true; i++)
    {
        if (random_value <= decision[i])
        {
            return i;
        }
    }
}

int DoActorNoise(ANIMATORp Action, DSWActor* actor)
{
    USERp u = actor->u();
    int SpriteNum = actor->GetSpriteIndex();

    if (Action == InitActorAmbientNoise)
    {
        PlaySpriteSound(SpriteNum, attr_ambient, v3df_follow);
//      MONO_PRINT("Ambient Sound");
    }
    else if (Action == InitActorAlertNoise)
    {
        if (u && !u->DidAlert) // This only allowed once
            PlaySpriteSound(SpriteNum, attr_alert, v3df_follow);
//      MONO_PRINT("Alert Sound");
    }
    else if (Action == InitActorAttackNoise)
    {
        PlaySpriteSound(SpriteNum, attr_attack, v3df_follow);
//      MONO_PRINT("Attack Sound");
    }
    else if (Action == InitActorPainNoise)
    {
        PlaySpriteSound(SpriteNum, attr_pain, v3df_follow);
//      MONO_PRINT("Pain Sound");
    }
    else if (Action == InitActorDieNoise)
    {
        PlaySpriteSound(SpriteNum, attr_die, v3df_none);
//      MONO_PRINT("Die Sound");
    }
    else if (Action == InitActorExtra1Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra1, v3df_follow);
    }
    else if (Action == InitActorExtra2Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra2, v3df_follow);
    }
    else if (Action == InitActorExtra3Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra3, v3df_follow);
    }
    else if (Action == InitActorExtra4Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra4, v3df_follow);
    }
    else if (Action == InitActorExtra5Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra5, v3df_follow);
    }
    else if (Action == InitActorExtra6Noise)
    {
        PlaySpriteSound(SpriteNum, attr_extra6, v3df_follow);
    }

    return 0;
}

bool CanSeePlayer(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    // if actor can still see the player
    int look_height = SPRITEp_TOS(sp);

    //if (FAF_Sector(sp->sectnum))
    //    return(true);

    if (u->tgt_sp() && FAFcansee(sp->x, sp->y, look_height, sp->sectnum, u->tgt_sp()->x, u->tgt_sp()->y, SPRITEp_UPPER(u->tgt_sp()), u->tgt_sp()->sectnum))
        return true;
    else
        return false;
}

int CanHitPlayer(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    hitdata_t hitinfo;
    int xvect,yvect,zvect;
    short ang,ret=false;
    // if actor can still see the player
    int zhs, zhh;

    //if (FAF_Sector(sp->sectnum))
    //    return(true);

    zhs = sp->z - DIV2(SPRITEp_SIZE_Z(sp));

    auto hp = u->tgt_sp();

    // get angle to target
    ang = getangle(hp->x - sp->x, hp->y - sp->y);

    // get x,yvect
    xvect = bcos(ang);
    yvect = bsin(ang);

    // get zvect
    zhh = hp->z - DIV2(SPRITEp_SIZE_Z(hp));
    if (hp->x - sp->x != 0)
        zvect = xvect * ((zhh - zhs)/(hp->x - sp->x));
    else if (hp->y - sp->y != 0)
        zvect = yvect * ((zhh - zhs)/(hp->y - sp->y));
    else
        return false;

    // so actors won't shoot straight up at you
    // need to be a bit of a distance away
    // before they have a valid shot
//    if (labs(zvect / FindDistance2D(hp->x - sp->x, hp->y - sp->y)) > 200)
//       return(false);

    FAFhitscan(sp->x, sp->y, zhs, sp->sectnum,
               xvect,
               yvect,
               zvect,
               &hitinfo, CLIPMASK_MISSILE);

    if (hitinfo.sect < 0)
        return false;

    if (hitinfo.sprite == u->tgt_sp() - sprite)
        return true;

    ////DSPRINTF(ds,"CanHit %s",ret ? "true" : "false");
    //MONO_PRINT(ds);

    return false;
}

/*
  !AIC - Pick a nearby player to be the actors target
*/

int DoActorPickClosePlayer(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    int SpriteNum = actor->GetSpriteIndex();
    int dist, near_dist = MAX_ACTIVE_RANGE, a,b,c;
    short pnum;
    PLAYERp pp;
    // if actor can still see the player
    int look_height = SPRITEp_TOS(sp);
    bool found = false;
    int i;

    if (u->ID == ZOMBIE_RUN_R0 && gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
        goto TARGETACTOR;

    // Set initial target to Player 0
    u->targetActor = Player->Actor();

    if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            if (sp->owner == pp->PlayerSprite)
                continue;

            u->targetActor = pp->Actor();
            break;
        }
    }


    // Set initial target to the closest player
    near_dist = MAX_ACTIVE_RANGE;
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        // Zombies don't target their masters!
        if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
        {
            if (sp->owner == pp->PlayerSprite)
                continue;

            if (!PlayerTakeDamage(pp, SpriteNum))
                continue;

            // if co-op don't hurt teammate
            // if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE && !gNet.HurtTeammate && u->spal == User[pp->PlayerSprite]->spal)
            //    continue;
        }

        DISTANCE(sp->x, sp->y, pp->posx, pp->posy, dist, a, b, c);

        if (dist < near_dist)
        {
            near_dist = dist;
            u->targetActor = pp->Actor();
        }
    }

    // see if you can find someone close that you can SEE
    near_dist = MAX_ACTIVE_RANGE;
    found = false;
    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        // Zombies don't target their masters!
        if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
        {
            if (sp->owner == pp->PlayerSprite)
                continue;

            if (!PlayerTakeDamage(pp, SpriteNum))
                continue;

            // if co-op don't hurt teammate
            //if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE && !gNet.HurtTeammate && u->spal == User[pp->PlayerSprite]->spal)
            //    continue;
        }

        DISTANCE(sp->x, sp->y, pp->posx, pp->posy, dist, a, b, c);

        //bool ICanSee = FAFcansee(sp->x, sp->y, look_height, sp->sectnum, pp->SpriteP->x, pp->SpriteP->y, SPRITEp_UPPER(pp->SpriteP), pp->SpriteP->sectnum);
        if (dist < near_dist && FAFcansee(sp->x, sp->y, look_height, sp->sectnum, pp->SpriteP->x, pp->SpriteP->y, SPRITEp_UPPER(pp->SpriteP), pp->SpriteP->sectnum))
        {
            near_dist = dist;
            u->targetActor = pp->Actor();
            found = true;
        }
    }


TARGETACTOR:
    // this is only for Zombies right now
    // zombie target other actors
    if (!found && TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
    {
        near_dist = MAX_ACTIVE_RANGE;
        StatIterator it(STAT_ENEMY);
        while ((i = it.NextIndex()) >= 0)
        {
            auto itActor = &swActors[i];
            if (i == SpriteNum)
                continue;

            if (TEST(User[i]->Flags, SPR_SUICIDE|SPR_DEAD))
                continue;

            DISTANCE(sp->x, sp->y, sprite[i].x, sprite[i].y, dist, a, b, c);

            if (dist < near_dist && FAFcansee(sp->x, sp->y, look_height, sp->sectnum, sprite[i].x, sprite[i].y, SPRITEp_UPPER(&sprite[i]), sprite[i].sectnum))
            {
                near_dist = dist;
                u->targetActor = itActor;
            }
        }
    }

    return 0;
}

int
GetPlayerSpriteNum(short SpriteNum)
{
    USERp u = User[SpriteNum].Data();
    short pnum;
    PLAYERp pp;

    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];

        if (pp->SpriteP == u->tgt_sp())
        {
            return pp->PlayerSprite;
        }
    }
    return 0;
}

int
CloseRangeDist(SPRITEp sp1, SPRITEp sp2)
{
    int clip1 = sp1->clipdist;
    int clip2 = sp2->clipdist;

    // add clip boxes and a fudge factor
#define DIST_CLOSE_RANGE 400

    return (clip1<<2) + (clip2<<2) + DIST_CLOSE_RANGE;
}

int DoActorOperate(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum].Data();
    short nearsector, nearwall, nearsprite;
    int nearhitdist;
    int z[2];
    unsigned int i;

    if (u->ID == HORNET_RUN_R0 || u->ID == EEL_RUN_R0 || u->ID == BUNNY_RUN_R0)
        return false;

    if (u->Rot == u->ActorActionSet->Sit || u->Rot == u->ActorActionSet->Stand)
        return false;

    if ((u->WaitTics -= ACTORMOVETICS) > 0)
        return false;

    //DSPRINTF(ds,"sp->x = %ld, sp->y = %ld, sp->sector = %d, tp->x = %ld, tp->y = %ld, tp->ang = %d\n",sp->x,sp->y,sp->sectnum,tpoint->x,tpoint->y,tpoint->ang);
    //MONO_PRINT(ds);

    z[0] = sp->z - SPRITEp_SIZE_Z(sp) + Z(5);
    z[1] = sp->z - DIV2(SPRITEp_SIZE_Z(sp));

    for (i = 0; i < SIZ(z); i++)
    {
        neartag(sp->x, sp->y, z[i], sp->sectnum, sp->ang,
                &nearsector, &nearwall, &nearsprite,
                &nearhitdist, 1024L, NTAG_SEARCH_LO_HI, nullptr);

        //DSPRINTF(ds,"nearsector = %d, nearwall = %d, nearsprite = %d hitdist == %ld\n",nearsector,nearwall,nearsprite,nearhitdist);
        //MONO_PRINT(ds);

#if 0  // Actors don't hit switches on a whim
        if (nearsprite >= 0 && nearhitdist < 1024)
        {
            if (OperateSprite(nearsprite, false))
            {
                u->WaitTics = 2 * 120;

                NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
            }
        }
#endif

    }

    if (nearsector >= 0 && nearhitdist < 1024)
    {
        if (OperateSector(nearsector, false))
        {
            u->WaitTics = 2 * 120;

            NewStateGroup(SpriteNum, u->ActorActionSet->Sit);
        }
    }

    if (nearwall >= 0 && nearhitdist < 1024)
    {
        if (OperateWall(nearwall, false))
        {
            u->WaitTics = 2 * 120;

            NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
        }
    }
    return true;

}

DECISION GenericFlaming[] =
{
    {30, InitActorAttack},
    {512, InitActorRunToward},
    {1024, InitActorRunAway},
};

/*
 !AIC KEY - This routine decides what the actor will do next.  It is not called
 every time through the loop.  This would be too slow.  It is only called when
 the actor needs to know what to do next such as running into something or being
 targeted.  It makes decisions based on the distance and viewablity of its target
 (u->tgt_sp()).  When it figures out the situatation with its target it calls
 ChooseAction which does a random table lookup to decide what action to initialize.
 Once this action is initialized it will be called until it can't figure out what to
 do anymore and then this routine is called again.
*/

ANIMATORp
DoActorActionDecide(short SpriteNum)
{
    auto actor = &swActors[SpriteNum];
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    int dist;
    ANIMATORp action;
    USERp pu=nullptr;
    bool ICanSee=false;

    // REMINDER: This function is not even called if SpriteControl doesn't let
    // it get called

    ASSERT(u->Personality);

    u->Dist = 0;
    action = InitActorDecide;

    // target is gone.
    if (u->targetActor == nullptr)
    {
        return action;
    }

    if (TEST(u->Flags, SPR_JUMPING | SPR_FALLING))
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
    if (TEST(u->Flags, SPR_ACTIVE))
    {
        // CODE BELOW = CRAP, DON'T USE IT OR IT MAKES ACTORS NOT ANIMATE SOMETIMES!!!!!
        // If target was actor, retarget to player it actor died
        // or just randomly give the target actor a break
        //if ((User[u->tgt_sp()-sprite] &&
        //    User[u->tgt_sp()-sprite]->Health <= 0) || RandomRange(1000) > 950)
        //    {
        //    DoActorPickClosePlayer(actor);
        //    InitActorReposition(SpriteNum);
        //    return(action);
        //    }

        // Try to operate stuff
        DoActorOperate(SpriteNum);

        // if far enough away and cannot see the player
        dist = Distance(sp->x, sp->y, u->tgt_sp()->x, u->tgt_sp()->y);

        if (dist > 30000 && !ICanSee)
        {
            // Enemy goes inactive - he is still allowed to roam about for about
            // 5 seconds trying to find another player before his active_range is
            // bumped down
            SetEnemyInactive(SpriteNum);

            // You've lost the player - now decide what to do
            action = ChooseAction(u->Personality->LostTarget);
            //CON_Message("LostTarget");
            return action;
        }


        pu = User[GetPlayerSpriteNum(SpriteNum)].Data();
        // check for short range attack possibility
        if ((dist < CloseRangeDist(sp, u->tgt_sp()) && ICanSee) ||
            (pu && pu->WeaponNum == WPN_FIST && u->ID != RIPPER2_RUN_R0 && u->ID != RIPPER_RUN_R0))
        {
            if ((u->ID == COOLG_RUN_R0 && TEST(sp->cstat, CSTAT_SPRITE_TRANSLUCENT)) || TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                action = ChooseAction(u->Personality->Evasive);
            else
                action = ChooseAction(u->Personality->CloseRange);
            //CON_Message("CloseRange");
            return action;
        }

        // if player is facing me and I'm being attacked
        if (FACING(sp, u->tgt_sp()) && TEST(u->Flags, SPR_ATTACKED) && ICanSee)
        {
            // if I'm a target - at least one missile comming at me
            if (TEST(u->Flags, SPR_TARGETED))
            {
                // not going to evade, reset the target bit
                RESET(u->Flags, SPR_TARGETED);        // as far as actor
                // knows, its not a
                // target any more
                if (u->ActorActionSet->Duck && RANDOM_P2(1024<<8)>>8 < 100)
                    action = InitActorDuck;
                else
                {
                    if ((u->ID == COOLG_RUN_R0 && TEST(sp->cstat, CSTAT_SPRITE_TRANSLUCENT)) || TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                        action = ChooseAction(u->Personality->Evasive);
                    else
                        action = ChooseAction(u->Personality->Battle);
                }
                //CON_Message("Battle 1");
                return action;
            }
            // if NOT a target - don't bother with evasive action and start
            // fighting
            else
            {
                if ((u->ID == COOLG_RUN_R0 && TEST(sp->cstat, CSTAT_SPRITE_TRANSLUCENT)) || TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                    action = ChooseAction(u->Personality->Evasive);
                else
                    action = ChooseAction(u->Personality->Battle);
                //CON_Message("Battle 2");
                return action;
            }

        }
        // if player is NOT facing me he is running or unaware of actor
        else if (ICanSee)
        {
            if ((u->ID == COOLG_RUN_R0 && TEST(sp->cstat, CSTAT_SPRITE_TRANSLUCENT)) || TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
                action = ChooseAction(u->Personality->Evasive);
            else
                action = ChooseAction(u->Personality->Offense);
            //CON_Message("Offense");
            return action;
        }
        else
        {
            // You've lost the player - now decide what to do
            action = ChooseAction(u->Personality->LostTarget);
            //CON_Message("Close but cant see, LostTarget");
            return action;
        }
    }
    // Not active - not aware of player and cannot see him
    else
    {
        // try and find another player
        // pick a closeby player as the (new) target
        if (sp->hitag != TAG_SWARMSPOT)
            DoActorPickClosePlayer(actor);

        // if close by
        dist = Distance(sp->x, sp->y, u->tgt_sp()->x, u->tgt_sp()->y);
        if (dist < 15000 || ICanSee)
        {
            if ((FACING(sp, u->tgt_sp()) && dist < 10000) || ICanSee)
            {
                DoActorOperate(SpriteNum);

                // Don't let player completely sneek up behind you
                action = ChooseAction(u->Personality->Surprised);
                //CON_Message("Surprised");
                if (!u->DidAlert && ICanSee)
                {
                    DoActorNoise(InitActorAlertNoise, actor);
                    u->DidAlert = true;
                }
                return action;

            }
            else
            {
                // Player has not seen actor, to be fair let him know actor
                // are there
                DoActorNoise(ChooseAction(u->Personality->Broadcast),actor);
                //CON_Message("Actor Noise");
                return action;
            }
        }
    }

    //CON_Message("Couldn't resolve decide, InitActorDecide");
    return action;
}

/*
  !AIC - Setup to do the decision
*/


int
InitActorDecide(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    // NOTE: It is possible to overflow the stack with too many calls to this
    // routine
    // Should use:
    // u->ActorActionFunc = DoActorDecide;
    // Instead of calling this function direcly

    // MONO_PRINT(strcpy(ds,"Init Actor Stay Put"));

    u->ActorActionFunc = DoActorDecide;

    DoActorDecide(actor);

    return 0;
}

int
DoActorDecide(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    ANIMATORp actor_action;

    // DoActorTest(SpriteNum);

    // See what to do next
    actor_action = DoActorActionDecide(SpriteNum);

    // Fix for the GenericFlaming bug for actors that don't have attack states
    if (actor_action == InitActorAttack && u->WeaponNum == 0)
        return 0;   // Just let the actor do as it was doing before in this case

    // Target is gone.
    if (u->targetActor == nullptr)
        return 0;

    // zombie is attacking a player
    if (actor_action == InitActorAttack && u->ID == ZOMBIE_RUN_R0 && User[u->tgt_sp()-sprite]->PlayerP)
    {
        // Don't let zombies shoot at master
        if (sp->owner == (u->tgt_sp() - sprite))
            return 0;

        // if this player cannot take damage from this zombie(weapon) return out
        if (!PlayerTakeDamage(User[u->tgt_sp()-sprite]->PlayerP, SpriteNum))
            return 0;
    }

    ASSERT(actor_action != nullptr);

    if (actor_action != InitActorDecide)
    {
        // NOT staying put
        (*actor_action)(actor);
        //CON_Message("DoActorDecide: NOT Staying put");
    }
    else
    {
        // Actually staying put
        NewStateGroup(SpriteNum, u->ActorActionSet->Stand);
        //CON_Message("DoActorDecide: Staying put");
    }

    return 0;
}

// Important note: The functions below are being checked for as state identifiers.
// But they are all identical content wise which makes MSVC merge them together into one.
// Assigning 'sw_snd_scratch' different values makes them different so that merging does not occur.
int sw_snd_scratch = 0;


int
InitActorAlertNoise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 1;
    // MONO_PRINT(strcpy(ds,"Init Actor Threat Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}


int
InitActorAmbientNoise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 2;

    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorAttackNoise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 3;

    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorPainNoise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 4;

    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorDieNoise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 5;

    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra1Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 6;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra2Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 7;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra3Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 8;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra4Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 9;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra5Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 10;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorExtra6Noise(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    sw_snd_scratch = 11;
    // MONO_PRINT(strcpy(ds,"Init Actor Move Noise"));

    // make some sort of noise here

    // InitActorDecide(SpriteNum);
    u->ActorActionFunc = DoActorDecide;

    return 0;
}


/*
  !AIC KEY - Routines handle moving toward the player.
*/

int
InitActorMoveCloser(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    //MONO_PRINT("Init Actor Move Closer\n");

    u->ActorActionFunc = DoActorMoveCloser;

    if (u->Rot != u->ActorActionSet->Run)
        NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    (*u->ActorActionFunc)(actor);

    return 0;
}

int
DoActorCantMoveCloser(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    //MONO_PRINT("Can't move closer\n");

    u->track = FindTrackToPlayer(u);

    if (u->track >= 0)
    {
        sp->ang = getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y);

        DoActorSetSpeed(actor, MID_SPEED);
        SET(u->Flags, SPR_FIND_PLAYER);

        u->ActorActionFunc = DoActorDecide;
        NewStateGroup(SpriteNum, u->ActorActionSet->Run);
        //MONO_PRINT("Trying to get to the track point\n");
    }
    else
    {
        // Try to move closer
        //MONO_PRINT("Move Closer - Trying to move around\n");

        InitActorReposition(actor);
    }
    return 0;
}

int
DoActorMoveCloser(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    nx = MulScale(sp->xvel, bcos(sp->ang), 14);
    ny = MulScale(sp->xvel, bsin(sp->ang), 14);

    // if cannot move the sprite
    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        DebugMoveHit(actor);

        if (ActorMoveHitReact(actor))
            return 0;

        DoActorCantMoveCloser(actor);
        return 0;
    }

    // Do a noise if ok
    DoActorNoise(ChooseAction(u->Personality->Broadcast), actor);

#if 0
    // evasion if targeted
    if (TEST(u->Flags, SPR_TARGETED))
    {
        ANIMATORp action;

        action = ChooseAction(u->Personality->Evasive);
        if (action)
        {
            (*action)(SpriteNum);
            return 0;
        }
    }
#endif

    // after moving a ways check to see if player is still in sight
    if (u->DistCheck > 550)
    {
        u->DistCheck = 0;

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
            sp->ang = getangle(u->tgt_sp()->x - sp->x, u->tgt_sp()->y - sp->y);
        }
    }

    // Should be a random value test
    if (u->Dist > 512 * 3)
    {
        InitActorDecide(actor);
    }

    return 0;

}

/*
  !AIC - Find tracks of different types.  Toward target, away from target, etc.
*/


short
FindTrackToPlayer(USERp u)
{
    SPRITEp sp = u->SpriteP;

    short point, track_dir, track;
    short i, *type, size;
    int zdiff;

    static short PlayerAbove[] =
    {
        BIT(TT_LADDER),
        BIT(TT_STAIRS),
        BIT(TT_JUMP_UP),
        BIT(TT_TRAVERSE),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    static short PlayerBelow[] =
    {
        BIT(TT_JUMP_DOWN),
        BIT(TT_STAIRS),
        BIT(TT_TRAVERSE),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    static short PlayerOnLevel[] =
    {
        BIT(TT_DUCK_N_SHOOT),
        BIT(TT_HIDE_N_SHOOT),
        BIT(TT_TRAVERSE),
        BIT(TT_EXIT),
        BIT(TT_OPERATE),
        BIT(TT_SCAN)
    };

    //MONO_PRINT("FindTrackToPlayer\n");

    zdiff = SPRITEp_UPPER(u->tgt_sp()) - (sp->z - SPRITEp_SIZE_Z(sp) + Z(8));

    if (labs(zdiff) <= Z(20))
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
        track = ActorFindTrack(u->SpriteNum, 1, type[i], &point, &track_dir);

        if (track >= 0)
        {
            u->point = point;
            u->track_dir = track_dir;
            SET(Track[track].flags, TF_TRACK_OCCUPIED);

            ////DSPRINTF(ds, "Found Track To Player\n");
            //MONO_PRINT(ds);

            return track;
        }
    }

    return -1;

}



short
FindTrackAwayFromPlayer(USERp u)
{
    short point, track_dir, track;
    unsigned int i;

    static short RunAwayTracks[] =
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

    //MONO_PRINT("FindTrackAwayFromPlayer\n");

    for (i = 0; i < SIZ(RunAwayTracks); i++)
    {
        track = ActorFindTrack(u->SpriteNum, -1, RunAwayTracks[i], &point, &track_dir);

        if (track >= 0)
        {
            u->point = point;
            u->track_dir = track_dir;
            SET(Track[track].flags, TF_TRACK_OCCUPIED);

            ////DSPRINTF(ds, "Found Run Away Track\n");
            //MONO_PRINT(ds);

            return track;
        }
        //MONO_PRINT("Did not find a run away track!\n");
    }

    return -1;

}


short
FindWanderTrack(USERp u)
{
    short point, track_dir, track;
    unsigned int i;

    static short WanderTracks[] =
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

    //MONO_PRINT("FindWanderTrack\n");

    for (i = 0; i < SIZ(WanderTracks); i++)
    {
        track = ActorFindTrack(u->SpriteNum, -1, WanderTracks[i], &point, &track_dir);

        if (track >= 0)
        {
            u->point = point;
            u->track_dir = track_dir;
            SET(Track[track].flags, TF_TRACK_OCCUPIED);

            return track;
        }
    }

    return -1;
}


int
InitActorRunAway(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    //MONO_PRINT("Init Actor RunAway\n");

    u->ActorActionFunc = DoActorDecide;
    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    u->track = FindTrackAwayFromPlayer(u);

    if (u->track >= 0)
    {
        sp->ang = NORM_ANGLE(getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y));
        DoActorSetSpeed(actor, FAST_SPEED);
        SET(u->Flags, SPR_RUN_AWAY);
        //MONO_PRINT("Actor running away on track\n");
    }
    else
    {
        SET(u->Flags, SPR_RUN_AWAY);
        InitActorReposition(actor);
        ////DSPRINTF(ds, "Actor RunAway\n");
        //MONO_PRINT(ds);
    }

    return 0;
}

int
InitActorRunToward(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    //MONO_PRINT("InitActorRunToward\n");

    u->ActorActionFunc = DoActorDecide;
    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    InitActorReposition(actor);
    DoActorSetSpeed(actor, FAST_SPEED);

    return 0;
}

/*
  !AIC - Where actors do their attacks.  There is some special case code throughout
  these.  Both close and long range attacks are handled here by transitioning to
  the correct attack state.
*/


int
InitActorAttack(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    // zombie is attacking a player
    if (u->ID == ZOMBIE_RUN_R0 && User[u->tgt_sp()-sprite]->PlayerP)
    {
        // Don't let zombies shoot at master
        if (sp->owner == (u->tgt_sp() - sprite))
            return 0;

        // if this player cannot take damage from this zombie(weapon) return out
        if (!PlayerTakeDamage(User[u->tgt_sp()-sprite]->PlayerP, SpriteNum))
            return 0;
    }

    if (TEST(sprite[u->tgt_sp()-sprite].cstat, CSTAT_SPRITE_TRANSLUCENT))
    {
        InitActorRunAway(actor);
        return 0;
    }

    if (User[u->tgt_sp()-sprite].Data() &&
        User[u->tgt_sp()-sprite]->Health <= 0)
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
    if (User[u->tgt_sp()-sprite].Data() &&
        User[u->tgt_sp()-sprite]->PlayerP &&
        TEST(User[u->tgt_sp()-sprite]->PlayerP->Flags, PF_DEAD))
    {
        DoActorPickClosePlayer(actor);
        InitActorReposition(actor);
        return 0;
    }

    u->ActorActionFunc = DoActorAttack;

    // move into standing frame
    //NewStateGroup(SpriteNum, u->ActorActionSet->Stand);

    // face player when attacking
    sp->ang = NORM_ANGLE(getangle(u->tgt_sp()->x - sp->x, u->tgt_sp()->y - sp->y));

    // If it's your own kind, lay off!
    if (u->ID == User[u->tgt_sp() - sprite]->ID && !User[u->tgt_sp() - sprite]->PlayerP)
    {
        InitActorRunAway(actor);
        return 0;
    }

#if 0
    // if low on health determine if needs to run away
    if (u->Health < 26)
    {
        if (CHOOSE2(AttackOrRun))
        {
            InitActorRunAway(SpriteNum);

            // could do a FindHealth here

            return 0;
        }

    }
#endif

    // Hari Kari for Ninja's
    if (u->ActorActionSet->Death2)
    {
        //#define SUICIDE_HEALTH_VALUE 26
#define SUICIDE_HEALTH_VALUE 38
        //#define SUICIDE_HEALTH_VALUE 50

        if (u->Health < SUICIDE_HEALTH_VALUE)
        {
            if (CHOOSE2(100))
            {
                u->ActorActionFunc = DoActorDecide;
                NewStateGroup(SpriteNum, u->ActorActionSet->Death2);
                return 0;
            }
        }
    }


    (*u->ActorActionFunc)(actor);

    return 0;
}


int
DoActorAttack(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
	USERp pu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short rand_num;
    int dist,a,b,c;

    DoActorNoise(ChooseAction(u->Personality->Broadcast),actor);

    DISTANCE(sp->x, sp->y, u->tgt_sp()->x, u->tgt_sp()->y, dist, a, b, c);

    pu = User[GetPlayerSpriteNum(SpriteNum)].Data();
    if ((u->ActorActionSet->CloseAttack[0] && dist < CloseRangeDist(sp, u->tgt_sp())) ||
        (pu && pu->WeaponNum == WPN_FIST))      // JBF: added null check
    {
        rand_num = ChooseActionNumber(u->ActorActionSet->CloseAttackPercent);

        NewStateGroup(SpriteNum, u->ActorActionSet->CloseAttack[rand_num]);
    }
    else
    {
        ASSERT(u->WeaponNum != 0);

        rand_num = ChooseActionNumber(u->ActorActionSet->AttackPercent);

        ASSERT(rand_num < u->WeaponNum);

        NewStateGroup(SpriteNum, u->ActorActionSet->Attack[rand_num]);
        u->ActorActionFunc = DoActorDecide;
    }

    //u->ActorActionFunc = DoActorDecide;

    return 0;
}

int
InitActorEvade(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    //MONO_PRINT("Init Actor Evade\n");

    // Evade is same thing as run away except when you get to the end of the
    // track
    // you stop and take up the fight again.

    u->ActorActionFunc = DoActorDecide;
    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    u->track = FindTrackAwayFromPlayer(u);

    if (u->track >= 0)
    {
        sp->ang = NORM_ANGLE(getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y));
        DoActorSetSpeed(actor, FAST_SPEED);
        // NOT doing a RUN_AWAY
        RESET(u->Flags, SPR_RUN_AWAY);
    }

    return 0;
}

int
InitActorWanderAround(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    //DSPRINTF(ds, "InitActorWanderAround\n");
    //MONO_PRINT(ds);

    u->ActorActionFunc = DoActorDecide;
    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    DoActorPickClosePlayer(actor);

    u->track = FindWanderTrack(u);

    if (u->track >= 0)
    {
        sp->ang = getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y);
        DoActorSetSpeed(actor, NORM_SPEED);
    }

    return 0;
}

int
InitActorFindPlayer(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int DoActorFindPlayer(short SpriteNum);

    //DSPRINTF(ds, "InitActorFindPlayer\n");
    //MONO_PRINT(ds);

    u->ActorActionFunc = DoActorDecide;
    NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    u->track = FindTrackToPlayer(u);

    if (u->track >= 0)
    {
        sp->ang = getangle((Track[u->track].TrackPoint + u->point)->x - sp->x, (Track[u->track].TrackPoint + u->point)->y - sp->y);
        DoActorSetSpeed(actor, MID_SPEED);
        SET(u->Flags, SPR_FIND_PLAYER);

        u->ActorActionFunc = DoActorDecide;
        NewStateGroup(SpriteNum, u->ActorActionSet->Run);
    }
    else
    {
        InitActorReposition(actor);
    }


    return 0;
}

int
InitActorDuck(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short dist;

//    MONO_PRINT(strcpy(ds, "Init Actor Duck"));

    if (!u->ActorActionSet->Duck)
    {
        u->ActorActionFunc = DoActorDecide;
        return 0;
    }

    u->ActorActionFunc = DoActorDuck;
    NewStateGroup(SpriteNum, u->ActorActionSet->Duck);

    dist = Distance(sp->x, sp->y, u->tgt_sp()->x, u->tgt_sp()->y);

    if (dist > 8000)
    {
        u->WaitTics = 190;
    }
    else
    {
        //u->WaitTics = 120;
        u->WaitTics = 60;
    }


    (*u->ActorActionFunc)(actor);

    return 0;
}

int
DoActorDuck(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    if ((u->WaitTics -= ACTORMOVETICS) < 0)
    {
        NewStateGroup(SpriteNum, u->ActorActionSet->Rise);
        u->ActorActionFunc = DoActorDecide;
        // InitActorDecide(SpriteNum);
        RESET(u->Flags, SPR_TARGETED);
    }

    return 0;
}

int
DoActorMoveJump(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    // Move while jumping

    nx = MulScale(sp->xvel, bcos(sp->ang), 14);
    ny = MulScale(sp->xvel, bsin(sp->ang), 14);

    move_actor(SpriteNum, nx, ny, 0L);

    if (!TEST(u->Flags, SPR_JUMPING|SPR_FALLING))
    {
        InitActorDecide(actor);
    }

    return 0;
}


int move_scan(short SpriteNum, short ang, int dist, int *stopx, int *stopy, int *stopz, short *stopsect)
{
    USERp u = User[SpriteNum].Data();
    SPRITEp sp = User[SpriteNum]->SpriteP;

    int nx,ny;
    uint32_t cliptype = CLIPMASK_ACTOR;
    int ret;

    short sang,ss;
    int x, y, z, loz, hiz;
    SPRITEp lo_sp, hi_sp;
    SECTORp lo_sectp, hi_sectp;


    // moves out a bit but keeps the sprites original postion/sector.

    // save off position info
    x = sp->x;
    y = sp->y;
    z = sp->z;
    sang = sp->ang;
    loz = u->loz;
    hiz = u->hiz;
    lo_sp = u->lo_sp;
    hi_sp = u->hi_sp;
    lo_sectp = u->lo_sectp;
    hi_sectp = u->hi_sectp;
    ss = sp->sectnum;

    // do the move
    sp->ang = ang;
    nx = MulScale(dist, bcos(sp->ang), 14);
    ny = MulScale(dist, bsin(sp->ang), 14);

    ret = move_sprite(SpriteNum, nx, ny, 0, u->ceiling_dist, u->floor_dist, cliptype, 1);
    // move_sprite DOES do a getzrange point?

    // should I look down with a FAFgetzrange to see where I am?

    // remember where it stopped
    *stopx = sp->x;
    *stopy = sp->y;
    *stopz = sp->z;
    *stopsect = sp->sectnum;

    // reset position information
    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->ang = sang;
    u->loz = loz;
    u->hiz = hiz;
    u->lo_sp = lo_sp;
    u->hi_sp = hi_sp;
    u->lo_sectp = lo_sectp;
    u->hi_sectp = hi_sectp;
    changespritesect(SpriteNum, ss);

    return ret;
}

#define TOWARD 1
#define AWAY -1

int
FindNewAngle(short SpriteNum, signed char dir, int DistToMove)
{
    auto actor = &swActors[SpriteNum];
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    static short toward_angle_delta[4][9] =
    {
        { -160, -384, 160, 384, -256, 256, -512, 512, -99},
        { -384, -160, 384, 160, -256, 256, -512, 512, -99},
        { 160, 384, -160, -384, 256, -256, 512, -512, -99},
        { 384, 160, -384, -160, 256, -256, 512, -512, -99}
    };

    static short away_angle_delta[4][8] =
    {
        { -768, 768, -640, 640, -896, 896, 1024, -99},
        { 768, -768, 640, -640, -896, 896, 1024, -99},
        { 896, -896, -768, 768, -640, 640, 1024, -99},
        { 896, -896, 768, -768, 640, -640, 1024, -99}
    };


    int16_t* adp = nullptr;

    short new_ang, oang;
    short save_ang = -1;
    unsigned short ret;
    int set;

    int dist, stopx, stopy, stopz;
    short stopsect;
    // start out with mininum distance that will be accepted as a move
    int save_dist = 500;

    // if on fire, run shorter distances
    if (ActorFlaming(actor))
        DistToMove = DIV4(DistToMove) + DIV8(DistToMove);

    // Find angle to from the player
    oang = NORM_ANGLE(getangle(u->tgt_sp()->x - sp->x, u->tgt_sp()->y - sp->y));

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
        printf("FindNewAngle called with dir=%d!\n",dir);
        return 0;
    }

    for (; *adp != -99; adp++)
    {
        new_ang = NORM_ANGLE(oang + *adp);

#if 1
        // look directly ahead for a ledge
        if (!TEST(u->Flags, SPR_NO_SCAREDZ | SPR_JUMPING | SPR_FALLING | SPR_SWIMMING | SPR_DEAD))
        {
            sp->ang = new_ang;
            if (DropAhead(SpriteNum, u->lo_step))
            {
                sp->ang = oang;
                continue;
            }
            sp->ang = oang;
        }
#endif

        // check to see how far we can move
        ret = move_scan(SpriteNum, new_ang, DistToMove, &stopx, &stopy, &stopz, &stopsect);

        if (ret == 0)
        {
            // cleanly moved in new direction without hitting something
            u->TargetDist = Distance(sp->x, sp->y, stopx, stopy);
            return new_ang;
        }
        else
        {
            // hit something
            dist = Distance(sp->x, sp->y, stopx, stopy);

            if (dist > save_dist)
            {
                save_ang = new_ang;
                save_dist = dist;
            }
        }
    }

    if (save_ang != -1)
    {
        u->TargetDist = save_dist;

        // If actor moved to the TargetDist it would look like he was running
        // into things.

        // To keep this from happening make the TargetDist is less than the
        // point you would hit something

        if (u->TargetDist > 4000)
            u->TargetDist -= 3500;

        sp->ang = save_ang;
        return save_ang;
    }

    return -1;
}


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

int
InitActorReposition(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short ang;
    int rnum;
    int dist;

    static int AwayDist[8] =
    {
        17000,
        20000,
        26000,
        26000,
        26000,
        32000,
        32000,
        42000
    };

    static int TowardDist[8] =
    {
        10000,
        15000,
        20000,
        20000,
        25000,
        30000,
        35000,
        40000
    };

    static int PlayerDist[8] =
    {
        2000,
        3000,
        3000,
        5000,
        5000,
        5000,
        9000,
        9000
    };

    //MONO_PRINT("InitActorReposition\n");

    u->Dist = 0;

    rnum = RANDOM_P2(8<<8)>>8;
    dist = Distance(sp->x, sp->y, u->tgt_sp()->x, u->tgt_sp()->y);

    if (dist < PlayerDist[rnum] || TEST(u->Flags, SPR_RUN_AWAY))
    {
        rnum = RANDOM_P2(8<<8)>>8;
        ang = FindNewAngle(SpriteNum, AWAY, AwayDist[rnum]);
        if (ang == -1)
        {
            u->Vis = 8;
            InitActorPause(actor);
            return 0;
        }

        sp->ang = ang;
        DoActorSetSpeed(actor, FAST_SPEED);
        RESET(u->Flags, SPR_RUN_AWAY);
    }
    else
    {
        // try to move toward player
        rnum = RANDOM_P2(8<<8)>>8;
        ang = FindNewAngle(SpriteNum, TOWARD, TowardDist[rnum]);
        if (ang == -1)
        {
            // try to move away from player
            rnum = RANDOM_P2(8<<8)>>8;
            ang = FindNewAngle(SpriteNum, AWAY, AwayDist[rnum]);
            if (ang == -1)
            {
                u->Vis = 8;
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

        sp->ang = ang;
    }


    u->ActorActionFunc = DoActorReposition;
    if (!TEST(u->Flags, SPR_SWIMMING))
        NewStateGroup(SpriteNum, u->ActorActionSet->Run);

    (*u->ActorActionFunc)(actor);

    return 0;
}

int
DoActorReposition(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int nx, ny;

    nx = MulScale(sp->xvel, bcos(sp->ang), 14);
    ny = MulScale(sp->xvel, bsin(sp->ang), 14);

    // still might hit something and have to handle it.
    if (!move_actor(SpriteNum, nx, ny, 0L))
    {
        if (ActorMoveHitReact(actor))
            return 0;

        u->Vis = 6;
        InitActorPause(actor);
        return 0;
    }

    // if close to target distance do a Decision again
    if (u->TargetDist < 50)
    {
        InitActorDecide(actor);
    }

    return 0;
}


int
InitActorPause(DSWActor* actor)
{
    USER* u = actor->u();
    int SpriteNum = u->SpriteNum;

    u->ActorActionFunc = DoActorPause;

    // !JIM! This makes actors not animate
    //if (!TEST(u->Flags, SPR_SWIMMING))
    //NewStateGroup(SpriteNum, u->ActorActionSet->Stand);

    (*u->ActorActionFunc)(actor);

    return 0;
}

int
DoActorPause(DSWActor* actor)
{
    USER* u = actor->u();
	int SpriteNum = u->SpriteNum;

    // Using Vis instead of WaitTics, var name sucks, but it's the same type
    // WaitTics is used by too much other actor code and causes problems here
    if ((u->Vis -= ACTORMOVETICS) < 0)
    {
        u->ActorActionFunc = DoActorDecide;
        RESET(u->Flags, SPR_TARGETED);
    }

    return 0;
}



#include "saveable.h"

static saveable_code saveable_ai_code[] =
{
    SAVE_CODE(InitActorDecide),
    SAVE_CODE(DoActorDecide),
    SAVE_CODE(InitActorAlertNoise),
    SAVE_CODE(InitActorAmbientNoise),
    SAVE_CODE(InitActorAttackNoise),
    SAVE_CODE(InitActorPainNoise),
    SAVE_CODE(InitActorDieNoise),
    SAVE_CODE(InitActorExtra1Noise),
    SAVE_CODE(InitActorExtra2Noise),
    SAVE_CODE(InitActorExtra3Noise),
    SAVE_CODE(InitActorExtra4Noise),
    SAVE_CODE(InitActorExtra5Noise),
    SAVE_CODE(InitActorExtra6Noise),
    SAVE_CODE(InitActorMoveCloser),
    SAVE_CODE(DoActorMoveCloser),
    SAVE_CODE(FindTrackToPlayer),
    SAVE_CODE(FindTrackAwayFromPlayer),
    SAVE_CODE(FindWanderTrack),
    SAVE_CODE(InitActorRunAway),
    SAVE_CODE(InitActorRunToward),
    SAVE_CODE(InitActorAttack),
    SAVE_CODE(DoActorAttack),
    SAVE_CODE(InitActorEvade),
    SAVE_CODE(InitActorWanderAround),
    SAVE_CODE(InitActorFindPlayer),
    SAVE_CODE(InitActorDuck),
    SAVE_CODE(DoActorDuck),
    SAVE_CODE(DoActorMoveJump),
    SAVE_CODE(InitActorReposition),
    SAVE_CODE(DoActorReposition),
    SAVE_CODE(DoActorPause)
};

static saveable_data saveable_ai_data[] =
{
    SAVE_DATA(GenericFlaming)
};

saveable_module saveable_ai =
{
    // code
    saveable_ai_code,
    SIZ(saveable_ai_code),

    // data
    saveable_ai_data,
    SIZ(saveable_ai_data)
};
END_SW_NS
