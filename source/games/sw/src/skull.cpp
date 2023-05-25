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
#include "misc.h"
#include "tags.h"
#include "ai.h"
#include "weapon.h"

BEGIN_SW_NS

int InitSpriteGrenade(DSWActor* actor);
int InitSpriteChemBomb(DSWActor*);
int InitFlashBomb(DSWActor* actor);
int InitCaltrops(DSWActor* actor);

//////////////////////
//
// SKULL Wait
//
//////////////////////


extern DAMAGE_DATA DamageData[];

#define SKULL_RATE 10

STATE s_SkullWait[1][1] =
{
    {
        {SPR_SKULL, 'A', SKULL_RATE, &AF(DoSkullWait), &s_SkullWait[0][0]},
    },
};


STATE* sg_SkullWait[] =
{
    &s_SkullWait[0][0],
};

ATTRIBUTE SkullAttrib =
{
    {60, 80, 100, 130},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {
        DIGI_AHAMBIENT, 0, 0, 0, DIGI_AHSCREAM,
        DIGI_AHEXPLODE,0,0,0,0
    }
};

//////////////////////
//
// SKULL for Serp God
//
//////////////////////

STATE s_SkullRing[1][1] =
{
    {
        {SPR_SKULL, 'A', SKULL_RATE, &AF(DoSerpRing), &s_SkullRing[0][0]},
    },
};


STATE* sg_SkullRing[] =
{
    &s_SkullRing[0][0],
};



//////////////////////
//
// SKULL Jump
//
//////////////////////

STATE s_SkullJump[1][1] =
{
    {
        {SPR_SKULL, 'A', SKULL_RATE, &AF(DoSkullJump), &s_SkullJump[0][0]},
    },
};


STATE* sg_SkullJump[] =
{
    &s_SkullJump[0][0],
};


//////////////////////
//
// SKULL Explode
//
//////////////////////

#define SKULL_EXPLODE_RATE 11

STATE s_SkullExplode[] =
{
    {SPR_SKULL_EXPLODE, 'A', 1,                  nullptr,  &s_SkullExplode[1]},
    {SPR_SKULL_EXPLODE, 'A', SF_QUICK_CALL,      &AF(DoDamageTest), &s_SkullExplode[2]},
    {SPR_SKULL_EXPLODE, 'A', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[3]},
    {SPR_SKULL_EXPLODE, 'B', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[4]},
    {SPR_SKULL_EXPLODE, 'C', SF_QUICK_CALL,      &AF(DoSkullSpawnShrap), &s_SkullExplode[5]},
    {SPR_SKULL_EXPLODE, 'C', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[6]},
    {SPR_SKULL_EXPLODE, 'D', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[7]},
    {SPR_SKULL_EXPLODE, 'E', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[8]},
    {SPR_SKULL_EXPLODE, 'F', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[9]},
    {SPR_SKULL_EXPLODE, 'G', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[10]},
    {SPR_SKULL_EXPLODE, 'H', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[11]},
    {SPR_SKULL_EXPLODE, 'I', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[12]},
    {SPR_SKULL_EXPLODE, 'J', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[13]},
    {SPR_SKULL_EXPLODE, 'K', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[14]},
    {SPR_SKULL_EXPLODE, 'L', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[15]},
    {SPR_SKULL_EXPLODE, 'M', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[16]},
    {SPR_SKULL_EXPLODE, 'N', SKULL_EXPLODE_RATE, nullptr,  &s_SkullExplode[17]},
    {SPR_SKULL_EXPLODE, 'N', SKULL_EXPLODE_RATE, &AF(DoSuicide),    &s_SkullExplode[17]}
};

STATE* sg_SkullExplode[] =
{
    s_SkullExplode,
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupSkull(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,SKULL_R0,s_SkullWait[0]);
        actor->user.Health = HEALTH_SKULL;
    }

    ChangeState(actor, s_SkullWait[0]);
    actor->user.__legacyState.Attrib = &SkullAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_SkullExplode;
    actor->user.__legacyState.Rot = sg_SkullWait;

    actor->user.ID = SKULL_R0;

    EnemyDefaults(actor, nullptr, nullptr);
    actor->clipdist = 12;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);
    actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actor->user.Radius = 400;

    if (ActorZOfBottom(actor) > actor->user.loz - 16)
    {
        auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
        actor->spr.pos.Z = actor->user.loz + tex->GetDisplayTopOffset();

        actor->user.loz = actor->spr.pos.Z;
        // leave 8 pixels above the ground
        actor->spr.pos.Z += ActorSizeToTop(actor) - 3;
    }
    else
    {
        actor->user.Counter = RANDOM_P2(2048);
        actor->user.pos.Z = actor->spr.pos.Z;
    }


    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSkull, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupSkull(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullMove(DSWActor* actor)
{
    auto vect = actor->spr.Angles.Yaw.ToVector() * actor->vel.X;
    double daz = actor->vel.Z;

    actor->user.coll = move_missile(actor, DVector3(vect, daz), 16, 16, CLIPMASK_MISSILE, ACTORMOVETICS);

    DoFindGroundPoint(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullBeginDeath(DSWActor* actor)
{
    int16_t i,num_ord=0;

    // Decrease for Serp God
    auto own = GetOwner(actor);
    if (own != nullptr && own->hasU())
        own->user.Counter--;

    // starts the explosion that does the actual damage

    switch (actor->spr.hitag)
    {
    case 1:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 2;
        if (num_ord > 3) num_ord = 3;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw += DAngle180 * (i & 1);
            InitSpriteChemBomb(actor);
        }
        break;

    case 2:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 5;
        if (num_ord > 10) num_ord = 10;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw = RandomAngle();
            InitCaltrops(actor);
        }
        break;

    case 3:
        UpdateSinglePlayKills(actor);
        InitFlashBomb(actor);
        break;

    case 4:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 5;
        if (num_ord > 10) num_ord = 10;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw += DAngle360 * i / num_ord;
            InitSpriteGrenade(actor);
        }
        break;
    default:
        SpawnMineExp(actor);
        for (i=0; i<3; i++)
        {
            actor->spr.Angles.Yaw = RandomAngle();
            InitPhosphorus(actor);
        }
        break;
    }

    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    
    actor->user.Tics = 0;
    actor->user.ID = SKULL_R0;
    actor->user.Radius = DamageData[DMG_SKULL_EXP].radius; //*DamageRadiusSkull;
    actor->user.OverlapZ = 64;
    change_actor_stat(actor, STAT_DEAD_ACTOR);
    actor->spr.shade = -40;

    SpawnLittleExp(actor);
    SetSuicide(actor);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullJump(DSWActor* actor)
{
    if(actor->vel.X != 0)
        DoSkullMove(actor);
    else
        actor->spr.Angles.Yaw += DAngle22_5 * 0.5 * ACTORMOVETICS;

    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoJump(actor);
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);

        // jump/fall type
        if(actor->vel.X != 0)
        {
            double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

            if (dist < 62.5 &&
                SpriteOverlapZ(actor, actor->user.targetActor, 32))
            {
                UpdateSinglePlayKills(actor);
                DoSkullBeginDeath(actor);
                return 0;
            }

            if (actor->spr.pos.Z > actor->user.loz - 36)
            {
                actor->spr.pos.Z = actor->user.loz - 36;
                UpdateSinglePlayKills(actor);
                DoSkullBeginDeath(actor);
                return 0;
            }
        }
        // non jumping type
        else
        {
            if (actor->user.jump_speed > 200)
            {
                UpdateSinglePlayKills(actor);
                DoSkullBeginDeath(actor);
            }
        }

    }
    else
    {
        UpdateSinglePlayKills(actor);
        DoSkullBeginDeath(actor);
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullBob(DSWActor* actor)
{
    // actor does a sine wave about actor->user.sz - this is the z mid point
    const int SKULL_BOB_AMT = 16;

    actor->user.Counter = (actor->user.Counter + (ACTORMOVETICS << 3) + (ACTORMOVETICS << 1)) & 2047;
    actor->spr.pos.Z = actor->user.pos.Z + SKULL_BOB_AMT * 1.5 * BobVal(actor->user.Counter);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullSpawnShrap(DSWActor* actor)
{
    SpawnShrap(actor, nullptr);

    //PlaySpriteSound(actor,attr_extra1,v3df_none);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSkullWait(DSWActor* actor)
{
    double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

    DoActorPickClosePlayer(actor);

    //if (dist < actor->user.active_range)
    //    return(0);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        PlaySound(DIGI_AHSCREAM, actor, v3df_none);
        actor->user.WaitTics = SEC(3) + RandomRange(360);
    }

    // below the floor type
    if (actor->spr.pos.Z > actor->user.loz)
    {
        // look for closest player every once in a while
        if (dist < 218.75)
        {
            actor->vel.X = 0;
            actor->user.jump_speed = -600;
            NewStateGroup(actor, sg_SkullJump);
            DoBeginJump(actor);
        }
    }
    else
    // above the floor type
    {
        actor->spr.Angles.Yaw += DAngle22_5 * 0.375 * ACTORMOVETICS;

        DoSkullBob(actor);

        if (dist < 500)
        {
            actor->spr.Angles.Yaw = (actor->user.targetActor->spr.pos - actor->spr.pos).Angle();
            actor->vel.X = 8 + RandomRangeF(16);
            actor->user.jump_speed = -700;
            NewStateGroup(actor, sg_SkullJump);
            DoBeginJump(actor);
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////
//
// BETTY Wait
//
//////////////////////


#define BETTY_RATE 10

STATE s_BettyWait[1][3] =
{
    {
        {SPR_BETTY, 'A', BETTY_RATE, &AF(DoBettyWait), &s_BettyWait[0][1]},
        {SPR_BETTY, 'B', BETTY_RATE, &AF(DoBettyWait), &s_BettyWait[0][2]},
        {SPR_BETTY, 'C', BETTY_RATE, &AF(DoBettyWait), &s_BettyWait[0][0]},
    },
};


STATE* sg_BettyWait[] =
{
    &s_BettyWait[0][0],
};

ATTRIBUTE BettyAttrib =
{
    {60, 80, 100, 130},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                 // MaxWeapons;
    {0, 0, 0, 0, 0, 0,0,0,0,0}
};

//////////////////////
//
// BETTY Jump
//
//////////////////////

STATE s_BettyJump[1][1] =
{
    {
        {SPR_BETTY, 'A', BETTY_RATE, &AF(DoBettyJump), &s_BettyJump[0][0]},
    },
};


STATE* sg_BettyJump[] =
{
    &s_BettyJump[0][0],
};


//////////////////////
//
// BETTY Explode
//
//////////////////////

#define BETTY_EXPLODE_RATE 11
#define BETTY_EXPLODE BETTY_R0

STATE s_BettyExplode[] =
{
    {SPR_BETTY, 'A', SF_QUICK_CALL,      &AF(DoDamageTest), &s_BettyExplode[1]},
    {SPR_BETTY, 'A', BETTY_EXPLODE_RATE, &AF(DoSuicide), &s_BettyExplode[0]}
};

STATE* sg_BettyExplode[] =
{
    s_BettyExplode,
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupBetty(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,BETTY_R0,s_BettyWait[0]);
        actor->user.Health = HEALTH_SKULL;
    }

    ChangeState(actor, s_BettyWait[0]);
    actor->user.__legacyState.Attrib = &BettyAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_BettyExplode;
    actor->user.__legacyState.Rot = sg_BettyWait;

    actor->user.ID = BETTY_R0;

    EnemyDefaults(actor, nullptr, nullptr);
    actor->clipdist = 12;
    actor->user.Flags |= (SPR_XFLIP_TOGGLE);
    actor->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actor->user.Radius = 400;

    if (ActorZOfBottom(actor) > actor->user.loz - 16)
    {
        auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
        actor->spr.pos.Z = actor->user.loz + tex->GetDisplayTopOffset();

        actor->user.loz = actor->spr.pos.Z;
        // leave 8 pixels above the ground
        actor->spr.pos.Z += ActorSizeToTop(actor) - 3;
    }
    else
    {
        actor->user.Counter = RANDOM_P2(2048);
        actor->user.pos.Z = actor->spr.pos.Z;
    }


    return 0;
}

DEFINE_ACTION_FUNCTION(DSWBetty, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupBetty(self);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettyMove(DSWActor* actor)
{
	return DoSkullMove(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettyBeginDeath(DSWActor* actor)
{
    int16_t i,num_ord=0;

    // starts the explosion that does the actual damage

    switch (actor->spr.hitag)
    {
    case 1:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 2;
        if (num_ord > 3) num_ord = 3;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw += DAngle180 * (i & 1);
            InitSpriteChemBomb(actor);
        }
        break;

    case 2:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 5;
        if (num_ord > 10) num_ord = 10;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw = RandomAngle();
            InitCaltrops(actor);
        }
        break;

    case 3:
        InitFlashBomb(actor);
        break;

    case 4:
        if (actor->spr.lotag) num_ord = actor->spr.lotag;
        else
            num_ord = 5;
        if (num_ord > 10) num_ord = 10;
        for (i=0; i<num_ord; i++)
        {
            actor->spr.Angles.Yaw += DAngle360 * i / num_ord;
            InitSpriteGrenade(actor);
        }
        break;
    default:
        for (i=0; i<5; i++)
        {
            actor->spr.Angles.Yaw = RandomAngle();
            InitPhosphorus(actor);
            SpawnMineExp(actor);
        }
        break;
    }

    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    
    actor->user.Tics = 0;
    actor->user.ID = BETTY_R0;
    actor->user.Radius = DamageData[DMG_SKULL_EXP].radius; //*DamageRadiusBetty;
    actor->user.OverlapZ = 64;
    change_actor_stat(actor, STAT_DEAD_ACTOR);
    actor->spr.shade = -40;

    SpawnLittleExp(actor);
    SetSuicide(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettyJump(DSWActor* actor)
{
    if(actor->vel.X != 0)
        DoBettyMove(actor);
    else
        actor->spr.Angles.Yaw += DAngle22_5 * 0.5 * ACTORMOVETICS;

    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoJump(actor);
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);

        // jump/fall type
        if(actor->vel.X != 0)
        {
            double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

            if (dist < 62.5 &&
                SpriteOverlapZ(actor, actor->user.targetActor, 32))
            {
                UpdateSinglePlayKills(actor);
                DoBettyBeginDeath(actor);
                return 0;
            }

            if (actor->spr.pos.Z > actor->user.loz - 36)
            {
                actor->spr.pos.Z = actor->user.loz - 36;
                UpdateSinglePlayKills(actor);
                DoBettyBeginDeath(actor);
                return 0;
            }
        }
        // non jumping type
        else
        {
            if (actor->user.jump_speed > 200)
            {
                UpdateSinglePlayKills(actor);
                DoBettyBeginDeath(actor);
            }
        }

    }
    else
    {
        UpdateSinglePlayKills(actor);
        DoBettyBeginDeath(actor);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettyBob(DSWActor* actor)
{
    // actor does a sine wave about actor->user.sz - this is the z mid point
    const int  BETTY_BOB_AMT = 16;

    actor->user.Counter = (actor->user.Counter + (ACTORMOVETICS << 3) + (ACTORMOVETICS << 1)) & 2047;
    actor->spr.pos.Z = actor->user.pos.Z + BETTY_BOB_AMT * 1.5 * BobVal(actor->user.Counter);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettySpawnShrap(DSWActor* actor)
{
    SpawnShrap(actor, nullptr);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoBettyWait(DSWActor* actor)
{
    double dist = (actor->spr.pos.XY() - actor->user.targetActor->spr.pos.XY()).Length();

    DoActorPickClosePlayer(actor);

    if ((actor->user.WaitTics -= ACTORMOVETICS) <= 0)
    {
        PlaySound(DIGI_MINEBEEP, actor, v3df_none);
        actor->user.WaitTics = SEC(3);
    }

    // below the floor type
    if (actor->spr.pos.Z > actor->user.loz)
    {
        // look for closest player every once in a while
        if (dist < 218.75)
        {
            actor->vel.X = 0;
            actor->user.jump_speed = -600;
            NewStateGroup(actor, sg_BettyJump);
            DoBeginJump(actor);
        }
    }
    else
    // above the floor type
    {
        actor->spr.Angles.Yaw += DAngle22_5 * 0.375 * ACTORMOVETICS;

        DoBettyBob(actor);

        if (dist < 8000)
        {
            actor->spr.Angles.Yaw = (actor->user.targetActor->spr.pos - actor->spr.pos).Angle();
            actor->vel.X = 8 + RandomRangeF(16);
            actor->user.jump_speed = -700;
            NewStateGroup(actor, sg_BettyJump);
            DoBeginJump(actor);
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

static saveable_data saveable_skull_data[] =
{
    SAVE_DATA(s_SkullWait),
    SAVE_DATA(sg_SkullWait),

    SAVE_DATA(SkullAttrib),

    SAVE_DATA(s_SkullRing),
    SAVE_DATA(sg_SkullRing),
    SAVE_DATA(s_SkullJump),
    SAVE_DATA(sg_SkullJump),
    SAVE_DATA(s_SkullExplode),
    SAVE_DATA(sg_SkullExplode),
    SAVE_DATA(s_BettyWait),
    SAVE_DATA(sg_BettyWait),

    SAVE_DATA(BettyAttrib),

    SAVE_DATA(s_BettyJump),
    SAVE_DATA(sg_BettyJump),
    SAVE_DATA(s_BettyExplode),
    SAVE_DATA(sg_BettyExplode),
};

saveable_module saveable_skull =
{
    // code
    nullptr, 0,

    // data
    saveable_skull_data,
    SIZ(saveable_skull_data)
};
END_SW_NS
