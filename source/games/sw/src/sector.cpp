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
#include "misc.h"
#include "tags.h"
#include "sector.h"
#include "player.h"
#include "weapon.h"
#include "jtags.h"

#include "network.h"

#include "break.h"
#include "misc.h"
#include "sprite.h"
#include "light.h"
#include "gstrings.h"
#include "secrets.h"

BEGIN_SW_NS

#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32
#define DEFAULT_DOOR_SPEED 800

enum
{
    SINE_FLOOR = (1 << 0),
    SINE_CEILING = (1 << 1),
    SINE_SLOPED = BIT(3),
};

ANIMATOR DoGrating;
void DoPlayerBeginForceJump(PLAYER*);

sectortype* FindNextSectorByTag(sectortype* sect, int tag);
short LevelSecrets;
bool TestVatorMatchActive(short match);
bool TestSpikeMatchActive(short match);
bool TestRotatorMatchActive(short match);
bool TestSlidorMatchActive(short match);
int PlayerCheckDeath(PLAYER*, DSWActor*);
void DoVatorOperate(PLAYER*, sectortype*);
void DoVatorMatch(PLAYER* pp, short match);
void DoRotatorOperate(PLAYER*, sectortype*);
void DoRotatorMatch(PLAYER* pp, short match, bool);
void DoSlidorOperate(PLAYER*, sectortype*);
void DoSlidorMatch(PLAYER* pp, short match, bool);

void KillMatchingCrackSprites(short match);
int DoTrapReset(short match);
int DoTrapMatch(short match);

PLAYER* GlobPlayerP;

ANIM Anim[MAXANIM];
short AnimCnt = 0;

SINE_WAVE_FLOOR SineWaveFloor[MAX_SINE_WAVE][21];
SINE_WALL SineWall[MAX_SINE_WALL][MAX_SINE_WALL_POINTS];
SPRING_BOARD SpringBoard[20];

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetSectorWallBits(sectortype* sect, int bit_mask, bool set_sectwall, bool set_nextwall)
{
    auto start_wall = sect->firstWall();
    auto wall_num = start_wall;

    do
    {
        if (set_sectwall)
            wall_num->extra |= bit_mask;

        if (set_nextwall)
        {
            if (wall_num->twoSided())
                wall_num->nextWall()->extra |= bit_mask;
        }

        wall_num = wall_num->point2Wall();
    }
    while (wall_num != start_wall);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void WallSetupDontMove(void)
{
    walltype* wallp;

    SWStatIterator it(STAT_WALL_DONT_MOVE_UPPER);
    while (auto iActor = it.Next())
    {
        SWStatIterator it1(STAT_WALL_DONT_MOVE_LOWER);
        while (auto jActor = it1.Next())
        {
            if (iActor->spr.lotag == jActor->spr.lotag)
            {
                for(auto& wal : wall)
                {
                    if (wal.pos.X < jActor->spr.pos.X && wal.pos.X > iActor->spr.pos.X && wal.pos.Y < jActor->spr.pos.Y && wal.pos.Y > iActor->spr.pos.Y)
                    {
                        wal.extra |= WALLFX_DONT_MOVE;
                    }
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void WallSetupLoop(walltype* wp, int16_t lotag, int16_t extra)
{
    // set first wall
    {
        wp->extra |= extra;

        if (wp->twoSided())
            wp->nextWall()->extra |= extra;
    }

    // Travel all the way around loop setting wall bits
    for (auto wall_num = wp->point2Wall();
         wall_num->lotag != lotag;
         wall_num = wall_num->point2Wall())
    {
        wall_num->extra |= extra;
        if (wall_num->twoSided())
            wall_num->nextWall()->extra |= extra;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void WallSetup(void)
{
    short NextSineWall = 0;

    WallSetupDontMove();

    memset(SineWall, 0, sizeof(SineWall));

    extern int x_min_bound, y_min_bound, x_max_bound, y_max_bound;

    for (auto& wal : wall)
    {
        if (wal.picnum == FAF_PLACE_MIRROR_PIC)
            wal.picnum = FAF_MIRROR_PIC;

        if (wal.picnum == FAF_PLACE_MIRROR_PIC+1)
            wal.picnum = FAF_MIRROR_PIC+1;

        // this overwrites the lotag so it needs to be called LAST - its down there
        // SetupWallForBreak(wp);

        switch (wal.lotag)
        {
        case TAG_WALL_LOOP_DONT_SPIN:
        {
            WallSetupLoop(&wal, TAG_WALL_LOOP_DONT_SPIN, WALLFX_LOOP_DONT_SPIN);
            break;
        }

        case TAG_WALL_LOOP_DONT_SCALE:
        {
            WallSetupLoop(&wal, TAG_WALL_LOOP_DONT_SCALE, WALLFX_DONT_SCALE);
            wal.lotag = 0;
            break;
        }

        case TAG_WALL_LOOP_OUTER_SECONDARY:
        {
            // make sure it's a red wall
            if (wal.twoSided())
            {
                WallSetupLoop(&wal, TAG_WALL_LOOP_OUTER_SECONDARY, WALLFX_LOOP_OUTER | WALLFX_LOOP_OUTER_SECONDARY);
            }
            else
            {
                Printf(PRINT_HIGH, "one-sided wall %d in loop setup\n", wallnum(&wal));
            }
            break;
        }

        case TAG_WALL_LOOP_OUTER:
        {
            // make sure it's a red wall
            if (wal.twoSided())
            {
                WallSetupLoop(&wal, TAG_WALL_LOOP_OUTER, WALLFX_LOOP_OUTER);
            }
            else
            {
                Printf(PRINT_HIGH, "one-sided wall %d in loop setup\n", wallnum(&wal));
            }
            wal.lotag = 0;
            break;
        }

        case TAG_WALL_DONT_MOVE:
        {
            // set first wall
            wal.extra |= WALLFX_DONT_MOVE;
            break;
        }

        case TAG_WALL_LOOP_SPIN_2X:
        {
            WallSetupLoop(&wal, TAG_WALL_LOOP_SPIN_2X, WALLFX_LOOP_SPIN_2X);
            break;
        }

        case TAG_WALL_LOOP_SPIN_4X:
        {
            WallSetupLoop(&wal, TAG_WALL_LOOP_SPIN_4X, WALLFX_LOOP_SPIN_4X);
            break;
        }

        case TAG_WALL_LOOP_REVERSE_SPIN:
        {
            WallSetupLoop(&wal, TAG_WALL_LOOP_REVERSE_SPIN, WALLFX_LOOP_REVERSE_SPIN);
            break;
        }

        case TAG_WALL_SINE_Y_BEGIN:
        case TAG_WALL_SINE_X_BEGIN:
        {
            walltype* wall_num;
            int cnt, num_points, type, tag_end;
            SINE_WALL* sw;
            int range = 250, speed = 3, peak = 0;

            tag_end = wal.lotag + 2;

            type = wal.lotag - TAG_WALL_SINE_Y_BEGIN;


            // count up num_points
            for (wall_num = &wal, num_points = 0;
                 num_points < MAX_SINE_WALL_POINTS && wall_num->lotag != tag_end;
                 wall_num = wall_num->point2Wall(), num_points++)
            {
                if (num_points == 0)
                {
                    if (wall_num->hitag)
                        range = wall_num->hitag;
                }
                else if (num_points == 1)
                {
                    if (wall_num->hitag)
                        speed = wall_num->hitag;
                }
                else if (num_points == 2)
                {
                    if (wall_num->hitag)
                        peak = wall_num->hitag;
                }
            }

            if (peak)
                num_points = peak;

            for (wall_num = &wal, cnt = 0;
                 cnt < MAX_SINE_WALL_POINTS && wall_num->lotag != tag_end;
                 wall_num = wall_num->point2Wall(), cnt++)
            {
                // set the first on up
                sw = &SineWall[NextSineWall][cnt];

                sw->type = type;
                sw->wallp = wall_num;
                sw->speed_shift = speed;
                sw->Range = range * maptoworld;

                // don't allow bullet holes/stars
                wall_num->extra |= WALLFX_DONT_STICK;

                if (!sw->type)
                    sw->origXY = wall_num->pos.Y - (sw->Range * 0.25);
                else
                    sw->origXY = wall_num->pos.X - (sw->Range * 0.25);

                sw->sintable_ndx = cnt * (2048 / num_points);
            }

            NextSineWall++;

            ASSERT(NextSineWall < MAX_SINE_WALL);

        }
        }

        // this overwrites the lotag so it needs to be called LAST
        SetupWallForBreak(&wal);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SectorLiquidSet(sectortype* sectp)
{
    // ///////////////////////////////////
    //
    // CHECK for pics that mean something
    //
    // ///////////////////////////////////

    if (sectp->floorpicnum >= 300 && sectp->floorpicnum <= 307)
    {
        sectp->u_defined = true;
        sectp->extra |= (SECTFX_LIQUID_WATER);
    }
    else if (sectp->floorpicnum >= 320 && sectp->floorpicnum <= 343)
    {
        sectp->u_defined = true;
        sectp->extra |= (SECTFX_LIQUID_WATER);
    }
    else if (sectp->floorpicnum >= 780 && sectp->floorpicnum <= 794)
    {
        sectp->u_defined = true;
        sectp->extra |= (SECTFX_LIQUID_WATER);
    }
    else if (sectp->floorpicnum >= 890 && sectp->floorpicnum <= 897)
    {
        sectp->u_defined = true;
        sectp->extra |= (SECTFX_LIQUID_WATER);
    }
    else if (sectp->floorpicnum >= 175 && sectp->floorpicnum <= 182)
    {
        sectp->u_defined = true;
        sectp->extra |= (SECTFX_LIQUID_LAVA);
        if (!sectp->damage)
            sectp->damage = 40;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SectorSetup(void)
{
    int tag;
    int NextSineWave = 0;
    int ndx;

    WallSetup();

    for (ndx = 0; ndx < MAX_SECTOR_OBJECTS; ndx++)
    {
        memset(&SectorObject[ndx], -1, sizeof(SectorObject[0]));
        // 0 pointers
        memset(&SectorObject[ndx].sectp, 0, sizeof(SectorObject[0].sectp));
        memset(&SectorObject[ndx].so_actors, 0, sizeof(SectorObject[0].so_actors));
        SectorObject[ndx].match_event_actor = nullptr;
        SectorObject[ndx].PreMoveAnimator = nullptr;
        SectorObject[ndx].PostMoveAnimator = nullptr;
        SectorObject[ndx].Animator = nullptr;
        SectorObject[ndx].controller = nullptr;
        SectorObject[ndx].sp_child = nullptr;
        SectorObject[ndx].mid_sector = nullptr;
        SectorObject[ndx].op_main_sector = nullptr;
        SectorObject[ndx].morph_wall_point = nullptr;
        SectorObject[ndx].pmid.X = MAXSO;
        SectorObject[ndx].ang = SectorObject[ndx].ang_moving = SectorObject[ndx].ang_tgt = 
            SectorObject[ndx].ang_orig = SectorObject[ndx].last_ang = SectorObject[ndx].old_ang =
            SectorObject[ndx].spin_speed = SectorObject[ndx].save_spin_speed = SectorObject[ndx].spin_ang = nullAngle;

    }

    memset(SineWaveFloor, 0, sizeof(SineWaveFloor));
    memset(SpringBoard, 0, sizeof(SpringBoard));

    LevelSecrets = 0;

    for(auto&sect: sector)
    {
        auto const sectp = &sect;
        tag = sectp->lotag;

        // ///////////////////////////////////
        //
        // CHECK for pics that mean something
        //
        // ///////////////////////////////////

        // ///////////////////////////////////
        //
        // CHECK for flags
        //
        // ///////////////////////////////////

        if ((sectp->extra & SECTFX_SINK))
        {
            SectorLiquidSet(sectp);
        }

        if ((sectp->floorstat & CSTAT_SECTOR_SKY))
        {
            // don't do a z adjust for FAF area
            if (sectp->floorpicnum != FAF_PLACE_MIRROR_PIC)
            {
                sectp->extra |= (SECTFX_Z_ADJUST);
            }
        }

        if ((sectp->ceilingstat & CSTAT_SECTOR_SKY))
        {
            // don't do a z adjust for FAF area
            if (sectp->ceilingpicnum != FAF_PLACE_MIRROR_PIC)
            {
                sectp->extra |= (SECTFX_Z_ADJUST);
            }
        }

        // ///////////////////////////////////
        //
        // CHECK for sector/sprite objects
        //
        // ///////////////////////////////////

        if (tag >= TAG_OBJECT_CENTER && tag < TAG_OBJECT_CENTER + 100)
        {
            SetupSectorObject(sectp, tag);
        }

        // ///////////////////////////////////
        //
        // CHECK lo and hi tags
        //
        // ///////////////////////////////////

        switch (tag)
        {
        case TAG_SECRET_AREA_TRIGGER:
            LevelSecrets++;
            break;

        case TAG_DOOR_SLIDING:
            SetSectorWallBits(sectp, WALLFX_DONT_STICK, true, true);
            break;

        case TAG_SINE_WAVE_FLOOR:
        case TAG_SINE_WAVE_CEILING:
        case TAG_SINE_WAVE_BOTH:
        {
            SINE_WAVE_FLOOR *swf;
            uint16_t swf_ndx = 0;
            short cnt = 0, sector_cnt;
            double Range;
            int range_diff = 0;
            int wave_diff = 0;
            short peak_dist = 0;
            short speed_shift = 3;
            short num;

            num = (tag - TAG_SINE_WAVE_FLOOR) / 20;

            // set the first on up
            swf = &SineWaveFloor[NextSineWave][swf_ndx];
            if (tag != TAG_SINE_WAVE_CEILING) StartInterpolation(sectp, Interp_Sect_Floorz);
            if (tag != TAG_SINE_WAVE_FLOOR) StartInterpolation(sectp, Interp_Sect_Ceilingz);

            swf->flags = 0;

            switch (num)
            {
            case 0:
                swf->flags |= (SINE_FLOOR);
                if ((sectp->floorstat & CSTAT_SECTOR_SLOPE))
                {
                    swf->flags |= (SINE_SLOPED);
                }
                break;
            case 1:
                swf->flags |= (SINE_CEILING);
                break;
            case 2:
                swf->flags |= (SINE_FLOOR | SINE_CEILING);
                break;
            }


            swf->sectp = sectp;
            ASSERT(swf->sectp->hitag != 0);
            swf->Range = Range = swf->sectp->hitag;
            swf->floorOrigz = swf->sectp->floorz - (Range * 0.25);
            swf->ceilingOrigz = swf->sectp->ceilingz- (Range * 0.25);

            // look for the rest by distance
            auto near_sectp = sectp, base_sectp = sectp;
            for (swf_ndx = 1, sector_cnt = 1; true; swf_ndx++)
            {
                near_sectp = FindNextSectorByTag(base_sectp, tag + swf_ndx);

                if (near_sectp != nullptr)
                {
                    swf = &SineWaveFloor[NextSineWave][swf_ndx];

                    if (swf_ndx == 1 && near_sectp->hitag)
                        range_diff = near_sectp->hitag;
                    else if (swf_ndx == 2 && near_sectp->hitag)
                        speed_shift = near_sectp->hitag;
                    else if (swf_ndx == 3 && near_sectp->hitag)
                        peak_dist = near_sectp->hitag;

                    swf->sectp = near_sectp;
					swf->floorOrigz = swf->sectp->floorz - (Range * 0.25);
					swf->ceilingOrigz = swf->sectp->ceilingz- (Range * 0.25);
                    Range -= range_diff * zmaptoworld;
                    swf->Range = Range;

                    base_sectp = swf->sectp;
                    sector_cnt++;
                }
                else
                    break;
            }


            ASSERT(swf_ndx <= SIZ(SineWaveFloor[NextSineWave]));

            // more than 6 waves and something in high tag - set up wave
            // dissapate
            if (sector_cnt > 8 && base_sectp->hitag)
            {
                wave_diff = base_sectp->hitag;
            }

            // setup the sintable_ndx based on the actual number of
            // sectors (swf_ndx)
            for (swf = &SineWaveFloor[NextSineWave][0], cnt = 0; swf->sectp != 0 && swf < (SINE_WAVE_FLOOR*)&SineWaveFloor[SIZ(SineWaveFloor)]; swf++, cnt++)
            {
                if (peak_dist)
                    swf->sintable_ndx = cnt * (2048 / peak_dist);
                else
                    swf->sintable_ndx = cnt * (2048 / swf_ndx);

                swf->speed_shift = speed_shift;
            }

            // set up the a real wave that dissapates at the end
            if (wave_diff)
            {
                for (cnt = sector_cnt - 1; cnt >= 0; cnt--)
                {
                    // only do the last (actually the first) few for the
                    // dissapate
                    if (cnt > 8)
                        continue;

                    swf = &SineWaveFloor[NextSineWave][cnt];

                    swf->Range -= wave_diff * zmaptoworld;

                    wave_diff += wave_diff;

                    if (swf->Range < 4)
                        swf->Range = 4;

                    // reset origz's based on new range
                    swf->floorOrigz = swf->sectp->floorz - (swf->Range * 0.25);
                    swf->ceilingOrigz = swf->sectp->ceilingz - (swf->Range * 0.25);
                }
            }

            NextSineWave++;

            ASSERT(NextSineWave < MAX_SINE_WAVE);

            break;
        }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DVector3 SectorMidPoint(sectortype* sectp)
{
    DVector3 sum(0,0,0);

    for (auto& wal : wallsofsector(sectp))
    {
        sum += wal.pos;
    }
    sum /= sectp->wallnum;
    sum.Z = (sectp->floorz + sectp->ceilingz) * 0.5;
    return sum;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSpringBoard(PLAYER* pp)
{

    pp->jump_speed = -pp->cursector->hitag;
    DoPlayerBeginForceJump(pp);
    return;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSpringBoardDown(void)
{
    unsigned sb;
    SPRING_BOARD *sbp;

    for (sb = 0; sb < SIZ(SpringBoard); sb++)
    {
        sbp = &SpringBoard[sb];

        // if empty set up an entry to close the sb later
        if (sbp->sectp != nullptr)
        {
            if ((sbp->TimeOut -= synctics) <= 0)
            {
				auto destz = nextsectorneighborzptr(sbp->sectp, sbp->sectp->floorz, Find_FloorDown | Find_Safe)->floorz;
                AnimSet(ANIM_Floorz, sbp->sectp, destz, 256);

                sbp->sectp->lotag = TAG_SPRING_BOARD;

                sbp->sectp = nullptr;
            }
        }
    }


    return;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

sectortype* FindNextSectorByTag(sectortype* sect, int tag)
{
    for(auto& wal : wallsofsector(sect))
    {
        if (wal.twoSided())
        {
            if (wal.nextSector()->lotag == tag)
            {
                return wal.nextSector();
            }
        }
    }

    return nullptr;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short DoSpawnActorTrigger(short match)
{
    short spawn_count = 0;

    SWStatIterator it(STAT_SPAWN_TRIGGER);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == match)
        {
            if (ActorSpawn(actor))
            {
                DoSpawnTeleporterEffectPlace(actor);
                PlaySound(DIGI_PLAYER_TELEPORT, actor, v3df_none);
                spawn_count++;
            }
        }
    }

    return spawn_count;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int OperateSector(sectortype* sect, short player_is_operating)
{
    PLAYER* pp = GlobPlayerP;

    // Don't let actors operate locked or secret doors
    if (!player_is_operating)
    {
        if (sect->hasU() && sect->stag == SECT_LOCK_DOOR)
            return false;

        SWSectIterator it(sect);
        while (auto actor = it.Next())
        {
            auto fsect = actor->sector();

            if (fsect->hasU() && fsect->stag == SECT_LOCK_DOOR)
                return false;

            if (actor->spr.statnum == STAT_VATOR && SP_TAG1(actor) == SECT_VATOR && TEST_BOOL7(actor))
                return false;
            if (actor->spr.statnum == STAT_ROTATOR && SP_TAG1(actor) == SECT_ROTATOR && TEST_BOOL7(actor))
                return false;
            if (actor->spr.statnum == STAT_SLIDOR && SP_TAG1(actor) == SECT_SLIDOR && TEST_BOOL7(actor))
                return false;

        }
    }

    switch (sect->lotag)
    {

    case TAG_VATOR:
        DoVatorOperate(pp, sect);
        return true;

    case TAG_ROTATOR:
        DoRotatorOperate(pp, sect);
        return true;

    case TAG_SLIDOR:
        DoSlidorOperate(pp, sect);
        return true;
    }

    return false;
}

enum
{
    SWITCH_LEVER = 581,
    SWITCH_FUSE = 558,
    SWITCH_FLIP = 561,
    SWITCH_RED_CHAIN = 563,
    SWITCH_GREEN_CHAIN = 565,
    SWITCH_TOUCH = 567,
    SWITCH_DRAGON = 569,

    SWITCH_LIGHT = 551,
    SWITCH_1 = 575,
    SWITCH_3 = 579,

    SWITCH_SHOOTABLE_1 = 577,
    SWITCH_4 = 571,
    SWITCH_5 = 573,
    SWITCH_6 = 583,
    EXIT_SWITCH = 2470,

    SWITCH_SKULL = 553,
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int AnimateSwitch(DSWActor* actor, int tgt_value)
{
    // if the value is not ON or OFF
    // then it is a straight toggle

    switch (actor->spr.picnum)
    {
    // set to true/ON
    case SWITCH_SKULL:
    case SWITCH_LEVER:
    case SWITCH_LIGHT:
    case SWITCH_SHOOTABLE_1:
    case SWITCH_1:
    case SWITCH_3:
    case SWITCH_FLIP:
    case SWITCH_RED_CHAIN:
    case SWITCH_GREEN_CHAIN:
    case SWITCH_TOUCH:
    case SWITCH_DRAGON:
    case SWITCH_4:
    case SWITCH_5:
    case SWITCH_6:
    case EXIT_SWITCH:

        // dont toggle - return the current state
        if (tgt_value == 999)
            return false;

        actor->spr.picnum += 1;

        // if the tgt_value should be true
        // flip it again - recursive but only once
        if (tgt_value == false)
        {
            AnimateSwitch(actor, tgt_value);
            return false;
        }

        return true;

    // set to true
    case SWITCH_SKULL + 1:
    case SWITCH_LEVER + 1:
    case SWITCH_LIGHT + 1:
    case SWITCH_1 + 1:
    case SWITCH_3 + 1:
    case SWITCH_FLIP + 1:
    case SWITCH_RED_CHAIN + 1:
    case SWITCH_GREEN_CHAIN + 1:
    case SWITCH_TOUCH + 1:
    case SWITCH_DRAGON + 1:
    case SWITCH_SHOOTABLE_1 + 1:
    case SWITCH_4+1:
    case SWITCH_5+1:
    case SWITCH_6+1:
    case EXIT_SWITCH+1:

        // dont toggle - return the current state
        if (tgt_value == 999)
            return true;

        actor->spr.picnum -= 1;

        if (tgt_value == int(true))
        {
            AnimateSwitch(actor, tgt_value);
            return true;
        }

        return false;
    }
    return false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SectorExp(DSWActor* actor, sectortype* sectp, double zh)
{
    actor->spr.cstat &= ~(CSTAT_SPRITE_ALIGNMENT_WALL|CSTAT_SPRITE_ALIGNMENT_FLOOR);
    auto mid = SectorMidPoint(sectp);
    // randomize the explosions
	actor->spr.angle = RandomAngle(45) + DAngle22_5;
    actor->spr.pos = { mid.X + RANDOM_P2F(16, 4) - 16, mid.Y + RANDOM_P2F(64, 4) - 64, zh };
    
    // setup vars needed by SectorExp
    ChangeActorSect(actor, sectp);
    getzsofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y, &actor->user.hiz, &actor->user.loz);

    // spawn explosion
    auto exp = SpawnSectorExp(actor);
    if (!exp) return;

    exp->spr.xrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    exp->spr.yrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    exp->user.change.XY() = exp->spr.angle.ToVector(5.75);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoExplodeSector(short match)
{
    sectortype* sectp;

    SWStatIterator it(STAT_EXPLODING_CEIL_FLOOR);
    while (auto actor = it.Next())
    {
        if (match != actor->spr.lotag)
            continue;

        if (!actor->hasU())
            SpawnUser(actor, 0, nullptr);

        sectp = actor->sector();

        sectp->addceilingz(-SP_TAG4(actor));

        if (SP_TAG5(actor))
        {
            sectp->setfloorslope(SP_TAG5(actor));
        }

        if (SP_TAG6(actor))
        {
            sectp->setceilingslope(SP_TAG6(actor));
        }

        for (double zh = sectp->ceilingz; zh < sectp->floorz; zh += 60)
        {
            SectorExp(actor, actor->sector(), zh + RANDOM_P2(64) - 32);
        }

        // don't need it any more
        KillActor(actor);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoSpawnSpot(DSWActor* actor)
{
    if ((actor->user.WaitTics -= synctics) < 0)
    {
        change_actor_stat(actor, STAT_SPAWN_SPOT);
        SpawnShrap(actor, nullptr);

        if (actor->user.LastDamage == 1)
        {
            KillActor(actor);
            return 0;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
//
// spawns shrap when killing an object
//
//---------------------------------------------------------------------------

void DoSpawnSpotsForKill(short match)
{
    if (match < 0)
        return;

    SWStatIterator it(STAT_SPAWN_SPOT);
    while (auto actor = it.Next())
    {
        // change the stat num and set the delay correctly to call SpawnShrap
        if (actor->spr.hitag == SPAWN_SPOT && actor->spr.lotag == match)
        {
            change_actor_stat(actor, STAT_NO_STATE);
            actor->user.ActorActionFunc = DoSpawnSpot;
            actor->user.WaitTics = SP_TAG5(actor) * 15;
            SetActorZ(actor, actor->spr.pos);
            // setting for Killed
            actor->user.LastDamage = 1;
        }
    }
}

//---------------------------------------------------------------------------
//
// spawns shrap when damaging an object
//
//---------------------------------------------------------------------------

void DoSpawnSpotsForDamage(short match)
{
    if (match < 0)
        return;

    SWStatIterator it(STAT_SPAWN_SPOT);
    while (auto actor = it.Next())
    {
        // change the stat num and set the delay correctly to call SpawnShrap
        if (actor->spr.hitag == SPAWN_SPOT && actor->spr.lotag == match)
        {
            change_actor_stat(actor, STAT_NO_STATE);
            actor->user.ActorActionFunc = DoSpawnSpot;
            actor->user.WaitTics = SP_TAG7(actor) * 15;
            // setting for Damaged
            actor->user.LastDamage = 0;
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSoundSpotMatch(short match, short sound_num, short sound_type)
{
    int flags;
    short snd2play;

    //sound_type is not used

    sound_num--;

    ASSERT(sound_num >= 0);

    SWStatIterator it(STAT_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        if (SP_TAG2(actor) == match && !TEST_BOOL6(actor))
        {
            short snd[3];

            snd[0] = SP_TAG13(actor); // tag4 is copied to tag13
            snd[1] = SP_TAG5(actor);
            snd[2] = SP_TAG6(actor);

            snd2play = 0;
            flags = 0;

            if (TEST_BOOL2(actor))
                flags = v3df_follow|v3df_nolookup|v3df_init;

            // play once and only once
            if (TEST_BOOL1(actor))
                SET_BOOL6(actor);

            // don't pan
            if (TEST_BOOL4(actor))
                flags |= v3df_dontpan;
            // add doppler
            if (TEST_BOOL5(actor))
                flags |= v3df_doppler;
            // random
            if (TEST_BOOL3(actor))
            {
                if (snd[0] && snd[1])
                {
                    snd2play = snd[RandomRange(2)];
                }
                else if (snd[0] && snd[1] && snd[2])
                {
                    snd2play = snd[RandomRange(3)];
                }
            }
            else if (snd[sound_num])
            {
                snd2play = snd[sound_num];
            }

            if (snd2play <= 0)
                continue;

            if (TEST_BOOL7(actor))
            {
                PLAYER* pp = GlobPlayerP;

                if (pp)
                {
                    if (pp == Player+myconnectindex)
                        PlayerSound(snd2play, v3df_dontpan|v3df_follow,pp);
                }
            }
            else
            {
                PlaySound(snd2play, actor, flags);
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSoundSpotStopSound(short match)
{

    SWStatIterator it(STAT_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        // found match and is a follow type
        if (SP_TAG2(actor) == match && TEST_BOOL2(actor))
        {
            DeleteNoSoundOwner(actor);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoStopSoundSpotMatch(short match)
{
    SWStatIterator it(STAT_STOP_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        if (SP_TAG2(actor) == match)
        {
            DoSoundSpotStopSound(SP_TAG5(actor));
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool TestKillSectorObject(SECTOR_OBJECT* sop)
{
    if ((sop->flags & SOBJ_KILLABLE))
    {
        KillMatchingCrackSprites(sop->match_event);
        // get new sectnums
        CollapseSectorObject(sop, sop->pmid);
        DoSpawnSpotsForKill(sop->match_event);
        KillSectorObjectSprites(sop);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short DoSectorObjectKillMatch(short match)
{
    SECTOR_OBJECT* sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        if (sop->match_event == match)
            return TestKillSectorObject(sop);
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SearchExplodeSectorMatch(short match)
{
    // THIS IS ONLY CALLED FROM DoMatchEverything
    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == match)
        {
            KillMatchingCrackSprites(match);
            DoExplodeSector(match);
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

void KillMatchingCrackSprites(short match)
{
    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == match)
        {
            if (SP_TAG8(actor) & BIT(2))
                continue;

            KillActor(actor);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void WeaponExplodeSectorInRange(DSWActor* wActor)
{
    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        // test to see if explosion is close to crack sprite
        double dist = (wActor->spr.pos - actor->spr.pos).Length();

        if (actor->native_clipdist() == 0)
            continue;

        double radius = actor->fClipdist() * 8;

		if (dist > (wActor->user.fRadius()/2) + radius)
            continue;

        if (!FAFcansee(wActor->spr.pos, wActor->sector(), actor->spr.pos, actor->sector()))
            continue;


        // pass in explosion type
        MissileHitMatch(wActor, WPN_ROCKET, actor);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ShootableSwitch(DSWActor* actor)
{
    switch (actor->spr.picnum)
    {
    case SWITCH_SHOOTABLE_1:
        //actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(actor, false);
        actor->spr.picnum = SWITCH_SHOOTABLE_1 + 1;
        break;
    case SWITCH_FUSE:
    case SWITCH_FUSE + 1:
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(actor, false);
        actor->spr.picnum = SWITCH_FUSE + 2;
        break;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoDeleteSpriteMatch(short match)
{
    static short StatList[] =
    {
        STAT_DEFAULT,
        STAT_VATOR,
        STAT_SPIKE,
        STAT_TRAP,
        STAT_ITEM,
        STAT_LIGHTING,
        STAT_STATIC_FIRE,
        STAT_AMBIENT,
        STAT_FAF
    };

	DVector2 del = {0,0};
    unsigned stat;

    while (true)
    {
        DSWActor* found = nullptr;

        // search for a DELETE_SPRITE with same match tag
        SWStatIterator it(STAT_DELETE_SPRITE);
        while (auto actor = it.Next())
        {
            if (actor->spr.lotag == match)
            {
                found = actor;
                del = actor->spr.pos;
                break;
            }
        }

        if (found == nullptr)
            return;

        for (stat = 0; stat < SIZ(StatList); stat++)
        {
            it.Reset(StatList[stat]);
            while (auto actor = it.Next())
            {
                if (del == actor->spr.pos)
                {
                    // special case lighting delete of Fade On/off after fades
                    if (StatList[stat] == STAT_LIGHTING)
                    {
                        // set shade to darkest and then kill it
                        actor->spr.shade = int8_t(SP_TAG6(actor));
                        actor->spr.pal = 0;
                        SectorLightShade(actor, actor->spr.shade);
                        DiffuseLighting(actor);
                    }

                    SpriteQueueDelete(actor);
                    KillActor(actor);
                }
            }
        }

        // kill the DELETE_SPRITE
        KillActor(found);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoChangorMatch(short match)
{
    SWStatIterator it(STAT_CHANGOR);
    while (auto actor = it.Next())
    {
        auto sectp = actor->sector();

        if (SP_TAG2(actor) != match)
            continue;

        if (TEST_BOOL1(actor))
        {
            sectp->ceilingpicnum = SP_TAG4(actor);
            sectp->addceilingz(SP_TAG5(actor));
            sectp->ceilingheinum += SP_TAG6(actor);

            if (sectp->ceilingheinum)
                sectp->ceilingstat |= CSTAT_SECTOR_SLOPE;
            else
                sectp->ceilingstat &= ~CSTAT_SECTOR_SLOPE;

            sectp->ceilingshade += SP_TAG7(actor);
            sectp->ceilingpal += SP_TAG8(actor);
        }
        else
        {
            sectp->floorpicnum = SP_TAG4(actor);
            sectp->addfloorz(SP_TAG5(actor));
            sectp->floorheinum += SP_TAG6(actor);

            if (sectp->floorheinum)
                sectp->floorstat |= CSTAT_SECTOR_SLOPE;
            else
                sectp->floorstat &= ~CSTAT_SECTOR_SLOPE;

            sectp->floorshade += SP_TAG7(actor);
            sectp->floorpal += SP_TAG8(actor);
        }

        sectp->visibility += SP_TAG9(actor);

        // if not set then go ahead and kill it
        if (TEST_BOOL2(actor) == 0)
        {
            KillActor(actor);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoMatchEverything(PLAYER* pp, short match, short state)
{
    PLAYER* bak;

    bak = GlobPlayerP;
    GlobPlayerP = pp;
    // CAREFUL! pp == nullptr is a valid case for this routine
    DoStopSoundSpotMatch(match);
    DoSoundSpotMatch(match, 1, SOUND_EVERYTHING_TYPE);
    GlobPlayerP = bak;

    DoLightingMatch(match, state);

    DoQuakeMatch(match);

    // make sure all vators are inactive before allowing
    // to repress switch
    if (!TestVatorMatchActive(match))
        DoVatorMatch(pp, match);

    if (!TestSpikeMatchActive(match))
        DoSpikeMatch(match);

    if (!TestRotatorMatchActive(match))
        DoRotatorMatch(pp, match, false);

    if (!TestSlidorMatchActive(match))
        DoSlidorMatch(pp, match, false);

    DoSectorObjectKillMatch(match);
    DoSectorObjectSetScale(match);

    DoSOevent(match, state);
    DoSpawnActorTrigger(match);

    // this may or may not find an exploding sector
    SearchExplodeSectorMatch(match);

    CopySectorMatch(match);
    DoWallMoveMatch(match);
    DoSpawnSpotsForKill(match);

    DoTrapReset(match);
    DoTrapMatch(match);

    SpawnItemsMatch(match);
    DoChangorMatch(match);
    DoDeleteSpriteMatch(match);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ComboSwitchTest(short combo_type, short match)
{
    int state;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag == combo_type && actor->spr.hitag == match)
        {
            // dont toggle - get the current state
            state = AnimateSwitch(actor, 999);

            // if any one is not set correctly then switch is not set
            if (state != SP_TAG3(actor))
            {
                return false;
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//
// NOTE: switches are always wall sprites
//
//---------------------------------------------------------------------------

int OperateSprite(DSWActor* actor, short player_is_operating)
{
    PLAYER* pp = nullptr;
    short state;
    short key_num=0;
    extern STATE s_Pachinko1Operate[];
    extern STATE s_Pachinko2Operate[];
    extern STATE s_Pachinko3Operate[];
    extern STATE s_Pachinko4Operate[];

    if (Prediction)
        return false;

    if (actor->spr.picnum == ST1)
        return false;

    if (player_is_operating)
    {
        pp = GlobPlayerP;

        if (!FAFcansee(pp->pos, pp->cursector, actor->spr.pos.plusZ(ActorSizeZ(actor) * -0.5), actor->sector()))
            return false;
    }

    switch (actor->spr.lotag)
    {
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    {
        short choose_snd;

        actor->user.FlagOwner = 1;
        actor->user.WaitTics = SEC(4);

        if (pp != Player+myconnectindex) return true;

        choose_snd = StdRandomRange(1000);
        if (actor->spr.lotag == CARGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG44052, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45014, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG44068, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG45010, v3df_dontpan|v3df_follow,pp);
        }
        else if (actor->spr.lotag == MECHANICGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG44027, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG44038, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG44039, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG44048, v3df_dontpan|v3df_follow,pp);
        }
        else if (actor->spr.lotag == SAILORGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG45018, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45030, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG45033, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG45043, v3df_dontpan|v3df_follow,pp);
        }
        else if (actor->spr.lotag == PRUNEGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG45053, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45067, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG46005, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG46010, v3df_dontpan|v3df_follow,pp);
        }
        else if (actor->spr.lotag == TOILETGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_WHATYOUEATBABY, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_WHATDIEDUPTHERE, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_YOUGOPOOPOO, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_PULLMYFINGER, v3df_dontpan|v3df_follow,pp);
        }
        else
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_SOAPYOUGOOD, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_WASHWANG, v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_DROPSOAP, v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_REALTITS, v3df_dontpan|v3df_follow,pp);
        }
    }
        return true;

    case PACHINKO1:

        // Don't mess with it if it's already going
        if (actor->user.WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        actor->user.WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko1Operate);

        return true;

    case PACHINKO2:

        // Don't mess with it if it's already going
        if (actor->user.WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        actor->user.WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko2Operate);

        return true;

    case PACHINKO3:

        // Don't mess with it if it's already going
        if (actor->user.WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        actor->user.WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko3Operate);

        return true;

    case PACHINKO4:

        // Don't mess with it if it's already going
        if (actor->user.WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        actor->user.WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko4Operate);

        return true;

    case SWITCH_LOCKED:
        key_num = actor->spr.hitag;
        if (pp->HasKey[key_num - 1])
        {
            for(auto& sect: sector)
            {
                if (sect.hasU() && sect.stag == SECT_LOCK_DOOR && sect.number == key_num)
                    sect.number = 0;  // unlock all doors of this type
            }
            UnlockKeyLock(key_num, actor);
        }

        return true;

    case TAG_COMBO_SWITCH_EVERYTHING:

        // change the switch state
        AnimateSwitch(actor, -1);
        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, actor->spr.hitag))
        {
            DoMatchEverything(pp, actor->spr.hitag, true);
        }

        return true;

    case TAG_COMBO_SWITCH_EVERYTHING_ONCE:

        // change the switch state
        AnimateSwitch(actor, -1);
        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, actor->spr.hitag))
        {
            DoMatchEverything(pp, actor->spr.hitag, true);
        }

        actor->spr.lotag = 0;
        actor->spr.hitag = 0;
        return true;

    case TAG_SWITCH_EVERYTHING:
        state = AnimateSwitch(actor, -1);
        DoMatchEverything(pp, actor->spr.hitag, state);
        return true;

    case TAG_SWITCH_EVERYTHING_ONCE:
        state = AnimateSwitch(actor, -1);
        DoMatchEverything(pp, actor->spr.hitag, state);
        actor->spr.lotag = 0;
        actor->spr.hitag = 0;
        return true;

    case TAG_LIGHT_SWITCH:

        state = AnimateSwitch(actor, -1);
        DoLightingMatch(actor->spr.hitag, state);
        return true;

    case TAG_SPRITE_SWITCH_VATOR:
    {
        // make sure all vators are inactive before allowing
        // to repress switch
        if (!TestVatorMatchActive(actor->spr.hitag))
            DoVatorMatch(pp, actor->spr.hitag);

        if (!TestSpikeMatchActive(actor->spr.hitag))
            DoSpikeMatch(actor->spr.hitag);

        if (!TestRotatorMatchActive(actor->spr.hitag))
            DoRotatorMatch(pp, actor->spr.hitag, false);

        if (!TestSlidorMatchActive(actor->spr.hitag))
            DoSlidorMatch(pp, actor->spr.hitag, false);

        return true;
    }

    case TAG_LEVEL_EXIT_SWITCH:
    {
        AnimateSwitch(actor, -1);

        PlaySound(DIGI_BIGSWITCH, actor, v3df_none);

		MapRecord *map;
        if (actor->spr.hitag)
            map = FindMapByLevelNum(actor->spr.hitag);
        else
            map = FindNextMap(currentLevel);
		ChangeLevel(map, g_nextskill);

        return true;
    }

    case TAG_SPRITE_GRATING:
    {
        change_actor_stat(actor, STAT_NO_STATE);

        SpawnUser(actor, 0, nullptr);

        actor->user.ActorActionFunc = DoGrating;

        actor->spr.lotag = 0;
        actor->spr.hitag /= 2;

        return true;
    }

    case TAG_SO_SCALE_SWITCH:
        AnimateSwitch(actor, -1);
        DoSectorObjectSetScale(actor->spr.hitag);
        return true;

    case TAG_SO_SCALE_ONCE_SWITCH:
        AnimateSwitch(actor, -1);
        DoSectorObjectSetScale(actor->spr.hitag);
        actor->spr.lotag = 0;
        actor->spr.hitag = 0;
        return true;

    case TAG_SO_EVENT_SWITCH:
    {
        state = AnimateSwitch(actor, -1);

        DoMatchEverything(nullptr, actor->spr.hitag, state);

        actor->spr.hitag = 0;
        actor->spr.lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);
        break;
    }

    case TAG_ROTATE_SO_SWITCH:
    {
        short so_num;
        SECTOR_OBJECT* sop;

        so_num = actor->spr.hitag;

        ASSERT(so_num <= 20);
        ASSERT(SectorObject[so_num].num_sectors != -1);

        AnimateSwitch(actor, -1);

        sop = &SectorObject[so_num];

        sop->ang_tgt += DAngle90;

        PlaySound(DIGI_BIGSWITCH, actor, v3df_none);

        return true;

        break;
    }
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoTrapReset(short match)
{
    SWStatIterator it(STAT_TRAP);
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag != match)
            continue;

        // if correct type and matches
        if (actor->spr.hitag == FIREBALL_TRAP)
            actor->user.WaitTics = 0;

        // if correct type and matches
        if (actor->spr.hitag == BOLT_TRAP)
            actor->user.WaitTics = 0;

        // if correct type and matches
        if (actor->spr.hitag == SPEAR_TRAP)
            actor->user.WaitTics = 0;
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoTrapMatch(short match)
{
    // may need to be reset to fire immediately

    SWStatIterator it(STAT_TRAP);
    while (auto actor = it.Next())
    {
        if (actor->spr.lotag != match)
            continue;

        // if correct type and matches
        if (actor->spr.hitag == FIREBALL_TRAP)
        {
            actor->user.WaitTics -= synctics;

            if (actor->user.WaitTics <= 0)
            {
                actor->user.WaitTics = 1 * 120;
                InitFireballTrap(actor);
            }
        }

        // if correct type and matches
        if (actor->spr.hitag == BOLT_TRAP)
        {
            actor->user.WaitTics -= synctics;

            if (actor->user.WaitTics <= 0)
            {
                actor->user.WaitTics = 1 * 120;
                InitBoltTrap(actor);
            }
        }

        // if correct type and matches
        if (actor->spr.hitag == SPEAR_TRAP)
        {
            actor->user.WaitTics -= synctics;

            if (actor->user.WaitTics <= 0)
            {
                actor->user.WaitTics = 1 * 120;
                InitSpearTrap(actor);
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

void TriggerSecret(sectortype* sectp, PLAYER* pp)
{
    if (pp == Player + myconnectindex)
        PlayerSound(DIGI_ANCIENTSECRET, v3df_dontpan | v3df_doppler | v3df_follow, pp);

    SECRET_Trigger(sectnum(pp->cursector));

    PutStringInfo(pp, GStrings("TXTS_SECRET"));
    // always give to the first player
    Player->SecretsFound++;
    sectp->lotag = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateTripTrigger(PLAYER* pp)
{
    if (Prediction)
        return;

    if (!pp->insector())
        return;

    sectortype* sectp = pp->cursector;

    // old method
    switch (pp->cursector->lotag)
    {
    // same tag for sector as for switch
    case TAG_LEVEL_EXIT_SWITCH:
    {
		MapRecord *map;
        if (sectp->hitag)
            map = FindMapByLevelNum(sectp->hitag);
        else
            map = FindNextMap(currentLevel);
		ChangeLevel(map, g_nextskill);
        break;
    }

    case TAG_SECRET_AREA_TRIGGER:
        TriggerSecret(sectp, pp);
        sectp->hitag = 0; // intentionally not in TriggerSecret.
        break;

    case TAG_TRIGGER_EVERYTHING:
        DoMatchEverything(pp, sectp->hitag, -1);
        break;

    case TAG_TRIGGER_EVERYTHING_ONCE:
        DoMatchEverything(pp, sectp->hitag, -1);
        sectp->lotag = 0;
        sectp->hitag = 0;
        break;

    case TAG_SECTOR_TRIGGER_VATOR:
        if (!TestVatorMatchActive(sectp->hitag))
            DoVatorMatch(pp, sectp->hitag);
        if (!TestSpikeMatchActive(sectp->hitag))
            DoSpikeMatch(sectp->hitag);
        if (!TestRotatorMatchActive(sectp->hitag))
            DoRotatorMatch(pp, sectp->hitag, false);
        if (!TestSlidorMatchActive(sectp->hitag))
            DoSlidorMatch(pp, sectp->hitag, false);
        break;

    case TAG_LIGHT_TRIGGER:
        DoLightingMatch(sectp->hitag, -1);
        break;

    case TAG_SO_SCALE_TRIGGER:
        DoSectorObjectSetScale(sectp->hitag);
        break;

    case TAG_SO_SCALE_ONCE_TRIGGER:
        DoSectorObjectSetScale(sectp->hitag);
        sectp->lotag = 0;
        sectp->hitag = 0;
        break;

    case TAG_TRIGGER_ACTORS:
    {
        double dist = sectp->hitag * maptoworld;

        SWStatIterator it(STAT_ENEMY);
        while (auto actor = it.Next())
        {
            if (actor->user.Flags & (SPR_WAIT_FOR_TRIGGER))
            {
                if ((actor->spr.pos.XY() - pp->pos.XY()).Length() < dist)
                {
                    actor->user.targetActor = pp->actor;
                    actor->user.Flags &= ~(SPR_WAIT_FOR_TRIGGER);
                }
            }
        }

        break;
    }

    case TAG_TRIGGER_MISSILE_TRAP:
    {
        // reset traps so they fire immediately
        DoTrapReset(pp->cursector->hitag);
        break;
    }

    case TAG_TRIGGER_EXPLODING_SECTOR:
    {
        DoMatchEverything(nullptr, pp->cursector->hitag, -1);
        break;
    }

    case TAG_SPAWN_ACTOR_TRIGGER:
    {
        DoMatchEverything(nullptr, pp->cursector->hitag, -1);

        pp->cursector->hitag = 0;
        pp->cursector->lotag = 0;
        break;
    }

    case TAG_SO_EVENT_TRIGGER:
    {
        DoMatchEverything(nullptr, pp->cursector->hitag, -1);

        pp->cursector->hitag = 0;
        pp->cursector->lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, pp, v3df_none);
        break;
    }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateContinuousTrigger(PLAYER* pp)
{
    if (Prediction)
        return;

    if (!pp->insector())
        return;

    switch (pp->cursector->lotag)
    {
    case TAG_TRIGGER_MISSILE_TRAP:
    {
        DoTrapMatch(pp->cursector->hitag);

        break;
    }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short PlayerTakeSectorDamage(PLAYER* pp)
{
    auto sectu = pp->cursector;
    DSWActor* actor = pp->actor;

    // the calling routine must make sure sectu exists
    if ((actor->user.DamageTics -= synctics) < 0)
    {
        actor->user.DamageTics = DAMAGE_TIME;

        PlayerUpdateHealth(pp, -sectu->damage);
        PlayerCheckDeath(pp, nullptr);
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// Needed in order to see if Player should grunt if he can't find a wall to operate on
// If player is too far away, don't grunt
//
//---------------------------------------------------------------------------

enum { PLAYER_SOUNDEVENT_TAG = 900 };
bool NearThings(PLAYER* pp)
{
    HitInfo near;

    // Check player's current sector for triggered sound
    if (pp->cursector->hitag == PLAYER_SOUNDEVENT_TAG)
    {
        if (pp == Player+myconnectindex)
            PlayerSound(pp->cursector->lotag, v3df_follow|v3df_dontpan,pp);
        return false;
    }

    neartag(pp->pos, pp->cursector, pp->angle.ang, near, 1024, NTAG_SEARCH_LO_HI);


    // hit a sprite? Check to see if it has sound info in it!
    // This can work with any sprite!
    if (near.actor() != nullptr)
    {
        auto actor = near.actor();

        // Go through list of cases
        if (actor->spr.hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(actor->spr.lotag, v3df_follow|v3df_dontpan,pp);
        }
        return false;   // Return false so he doesn't grunt
    }

    if (near.hitWall != nullptr)
    {
        // Check player's current sector for triggered sound
        if (near.hitWall->hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(near.hitWall->lotag, v3df_follow|v3df_dontpan,pp);
            return false;   // We are playing a sound so don't return true
        }
        return true;
    }
    // This only gets called if nothing else worked, check for nearness to a wall
    {
        HitInfo hit{};

        FAFhitscan(pp->pos.plusZ(-30), pp->cursector, DVector3(pp->angle.ang.ToVector() * 1024, 0), hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
            return false;

        if ((hit.hitpos.XY() - pp->pos.XY()).Length() > 93.75)
            return false;

        // hit a sprite?
        if (hit.actor() != nullptr)
            return false;

        if (near.hitSector != nullptr)
            return true;

        if (hit.hitWall != nullptr)
        {
            // Near a plain old vanilla wall.  Can't do anything but grunt.
            if (!(hit.hitWall->extra & WALLFX_DONT_STICK) && pp == Player+myconnectindex)
            {
                if (StdRandomRange(1000) > 970)
                    PlayerSound(DIGI_HITTINGWALLS, v3df_follow|v3df_dontpan,pp);
                else
                    PlayerSound(DIGI_SEARCHWALL, v3df_follow|v3df_dontpan,pp);
            }

            return true;
        }

        return false;
    }

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short nti_cnt;

void NearTagList(NEAR_TAG_INFO* ntip, PLAYER* pp, double z, double dist, int type, int count)
{
    short save_lotag, save_hitag;
    HitInfo near;


    neartag(DVector3(pp->pos.XY(), z), pp->cursector, pp->angle.ang, near, dist, type);

    if (near.hitSector != nullptr)
    {
        auto ntsec = near.hitSector;
        // save off values
        save_lotag = ntsec->lotag;
        save_hitag = ntsec->hitag;

        ntip->Dist = near.hitpos.X;
        ntip->sectp = ntsec;
        ntip->wallp = nullptr;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        ntsec->lotag = 0;
        ntsec->hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        ntsec->lotag = save_lotag;
        ntsec->hitag = save_hitag;
    }
    else if (near.hitWall != nullptr)
    {
        auto ntwall = near.hitWall;
        // save off values
        save_lotag = ntwall->lotag;
        save_hitag = ntwall->hitag;

        ntip->Dist = near.hitpos.X;
        ntip->sectp = nullptr;
        ntip->wallp = ntwall;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        ntwall->lotag = 0;
        ntwall->hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        ntwall->lotag = save_lotag;
        ntwall->hitag = save_hitag;
    }
    else if (near.actor() != nullptr)
    {
        auto actor = near.actor();
        // save off values
        save_lotag = actor->spr.lotag;
        save_hitag = actor->spr.hitag;

        ntip->Dist = near.hitpos.X;
        ntip->sectp = nullptr;
        ntip->wallp = nullptr;
        ntip->actor = actor;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        actor->spr.lotag = 0;
        actor->spr.hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        actor->spr.lotag = save_lotag;
        actor->spr.hitag = save_hitag;
    }
    else
    {
        ntip->Dist = -1;
        ntip->sectp = nullptr;
        ntip->wallp = nullptr;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;

        return;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void BuildNearTagList(NEAR_TAG_INFO* ntip, int size, PLAYER* pp, double z, int dist, int type, int count)
{
    memset(ntip, -1, size);
    nti_cnt = 0;
    NearTagList(ntip, pp, z, dist, type, count);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int DoPlayerGrabStar(PLAYER* pp)
{
    int i;

    // MUST check exact z's of each star or it will never work
    for (i = 0; i < MAX_STAR_QUEUE; i++)
    {
        auto actor = StarQueue[i];
        if (actor != nullptr)
        {
            if ((actor->spr.pos - pp->pos).plusZ(12).Length() < 31.25)
            {
                break;
            }
        }
    }

    if (i < MAX_STAR_QUEUE)
    {
        // Pull a star out of wall and up your ammo
        PlayerUpdateAmmo(pp, WPN_STAR, 1);
        PlaySound(DIGI_ITEM, StarQueue[i], v3df_none);
        KillActor(StarQueue[i]);
        StarQueue[i] = nullptr;
        if (pp->WpnFlags & (BIT(WPN_STAR)))
            return true;
        pp->WpnFlags |= (BIT(WPN_STAR));
        InitWeaponStar(pp);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void PlayerOperateEnv(PLAYER* pp)
{
    bool found;

    if (Prediction || !pp->actor)
        return;

    //
    // Switch & door activations
    //

    if (pp->input.actions & SB_OPEN)
    {
        if (pp->KeyPressBits & SB_OPEN)
        {
            // if space bar pressed
            short nt_ndx;
            NEAR_TAG_INFO nti[16];

            if (DoPlayerGrabStar(pp))
            {
                pp->KeyPressBits &= ~SB_OPEN;
            }
            else
            {
                NearThings(pp); // Check for player sound specified in a level sprite
            }

            BuildNearTagList(nti, sizeof(nti), pp, pp->pos.Z, 128, NTAG_SEARCH_LO_HI, 8);

            found = false;

            // try and find a sprite
            for (nt_ndx = 0; nti[nt_ndx].Dist >= 0; nt_ndx++)
            {
                if (nti[nt_ndx].actor != nullptr && nti[nt_ndx].Dist < 64 + 48)
                {
                    if (OperateSprite(nti[nt_ndx].actor, true))
                    {
                        pp->KeyPressBits &= ~SB_OPEN;
                        found = true;
                    }
                }
            }

            // if not found look at different z positions
            if (!found)
            {
                double z[3];
                DSWActor* plActor = pp->actor;

                z[0] = plActor->spr.pos.Z - ActorSizeZ(plActor) - 10;
                z[1] = plActor->spr.pos.Z;
                z[2] = (z[0] + z[1]) * 0.5;

                for (unsigned i = 0; i < SIZ(z); i++)
                {
                    BuildNearTagList(nti, sizeof(nti), pp, z[i], 64 + 48, NTAG_SEARCH_LO_HI, 8);

                    for (nt_ndx = 0; nti[nt_ndx].Dist >= 0; nt_ndx++)
                    {
                        if (nti[nt_ndx].actor != nullptr && nti[nt_ndx].Dist < 64 + 48)
                        {
                            if (OperateSprite(nti[nt_ndx].actor, true))
                            {
                                pp->KeyPressBits &= ~SB_OPEN;
                                break;
                            }
                        }
                    }
                }
            }

            {
                double neartaghitdist;
                sectortype* neartagsector;

                neartaghitdist = nti[0].Dist;
                neartagsector = nti[0].sectp;

                if (neartagsector != nullptr && neartaghitdist < 64)
                {
                    if (OperateSector(neartagsector, true))
                    {
                        // Release the key
                        pp->KeyPressBits &= ~SB_OPEN;

                        // crude and very awful hack for wd secret area
                        if  (sector.Size() > 715 && pp->cursector == &sector[491] && currentLevel->levelNumber == 11 && sector[715].lotag == TAG_SECRET_AREA_TRIGGER) 
                        {
                            TriggerSecret(&sector[715], pp);
                        }

                    }
                }
            }

            //
            // Trigger operations
            //

			switch (pp->cursector->lotag)
            {
            case TAG_VATOR:
                DoVatorOperate(pp, pp->cursector);
                DoSpikeOperate(pp->cursector);
                DoRotatorOperate(pp, pp->cursector);
                DoSlidorOperate(pp, pp->cursector);
                break;
            case TAG_SPRING_BOARD:
                DoSpringBoard(pp);
                pp->KeyPressBits &= ~SB_OPEN;
                break;
            case TAG_DOOR_ROTATE:
                if (OperateSector(pp->cursector, true))
                    pp->KeyPressBits &= ~SB_OPEN;
                break;
            }
        }
    }
    else
    {
        // Reset the key when syncbit key is not in use
        pp->KeyPressBits |= SB_OPEN;
    }

    // ////////////////////////////
    //
    // Sector Damage
    //
    // ////////////////////////////

    sectortype* sectp = pp->cursector;
    if (pp->insector() && sectp->hasU() && sectp->damage)
    {
        if ((sectp->flags & SECTFU_DAMAGE_ABOVE_SECTOR))
        {
            PlayerTakeSectorDamage(pp);
        }
        else if ((ActorZOfBottom(pp->actor) >= sectp->floorz) && !(pp->Flags & PF_DIVING))
        {
            PlayerTakeSectorDamage(pp);
        }
    }
    else
    {
        pp->actor->user.DamageTics = 0;
    }


    // ////////////////////////////
    //
    // Trigger stuff
    //
    // ////////////////////////////

    OperateContinuousTrigger(pp);

    // just changed sectors
    if (pp->lastcursector != pp->cursector)
    {
        OperateTripTrigger(pp);

        if (pp->insector() && (pp->cursector->extra & SECTFX_WARP_SECTOR))
        {
            if (!(pp->Flags2 & PF2_TELEPORTED))
            {
                DoPlayerWarpTeleporter(pp);
            }
        }

        pp->Flags2 &= ~(PF2_TELEPORTED);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSineWaveFloor(void)
{
    SINE_WAVE_FLOOR *swf;
    int wave;
    int flags;

    for (wave = 0; wave < MAX_SINE_WAVE; wave++)
    {
        for (swf = &SineWaveFloor[wave][0], flags = swf->flags; swf->sectp != nullptr && swf < &SineWaveFloor[wave][SIZ(SineWaveFloor[wave])]; swf++)
        {

            swf->sintable_ndx = NORM_ANGLE(swf->sintable_ndx + (synctics << swf->speed_shift));

            if ((flags & SINE_FLOOR))
            {
                double newz = swf->floorOrigz + swf->Range * BobVal(swf->sintable_ndx);
                swf->sectp->setfloorz(newz);
            }

            if ((flags & SINE_CEILING))
            {
                double newz = swf->ceilingOrigz + swf->Range * BobVal(swf->sintable_ndx);
                swf->sectp->setceilingz(newz);
            }

        }
    }

    /*  SLOPED SIN-WAVE FLOORS:

    It's best to program sloped sin-wave floors in 2 steps:
       1.  First set the floor z of the floor as the sin code normally does it.
       2.  Adjust the slopes by calling alignflorslope once for each sector.

    Note:  For this to work, the first wall of each sin-wave sector must be
           aligned on the same side of each sector for the entire strip.
    */

    for (wave = 0; wave < MAX_SINE_WAVE; wave++)
    {
        for (swf = &SineWaveFloor[wave][0], flags = swf->flags; swf->sectp != nullptr && swf < &SineWaveFloor[wave][SIZ(SineWaveFloor[wave])]; swf++)
        {
            auto sect = swf->sectp;
            if (!(sect->floorstat & CSTAT_SECTOR_SLOPE))
                continue;

            if ((flags & SINE_SLOPED))
            {
                walltype* wal;
                if (sect->wallnum == 4)
                {
                    //Set wal to the wall on the opposite side of the sector
                    wal = sect->firstWall() + 2;

                    //Pass (Sector, x, y, z)
                    alignflorslope(sect,DVector3(wal->pos, wal->nextSector()->floorz));
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSineWaveWall(void)
{
    SINE_WALL *sw;
    double New;
    short sw_num;

    for (sw_num = 0; sw_num < MAX_SINE_WALL; sw_num++)
    {
        for (sw = &SineWall[sw_num][0]; sw->wallp != nullptr && sw < &SineWall[sw_num][MAX_SINE_WALL_POINTS]; sw++)
        {
            auto wal = sw->wallp;
            // move through the sintable
            sw->sintable_ndx = NORM_ANGLE(sw->sintable_ndx + (synctics << sw->speed_shift));

            if (!sw->type)
            {
                New = sw->origXY + sw->Range * BobVal(sw->sintable_ndx);
                dragpoint(wal, DVector2(wal->pos.X, New));
            }
            else
            {
                New = sw->origXY + sw->Range * BobVal(sw->sintable_ndx);
                dragpoint(wal, DVector2(New, wal->pos.Y));
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoAnim(int numtics)
{
    int i;

    for (i = AnimCnt - 1; i >= 0; i--)
    {
        double animval = Anim[i].getValue();

        // if LESS THAN goal
        if (animval < Anim[i].goal)
        {
            // move it
            animval += numtics * (Anim[i].vel);

            Anim[i].vel += Anim[i].vel_adj * numtics;

            // if the other way make it equal
            if (animval > Anim[i].goal)
                animval = Anim[i].goal;
        }

        // if GREATER THAN goal
        if (animval > Anim[i].goal)
        {
            animval -= numtics * (Anim[i].vel);

            Anim[i].vel += Anim[i].vel_adj * numtics;

            if (animval < Anim[i].goal)
                animval = Anim[i].goal;
        }

        Anim[i].setValue(animval);

        // EQUAL this entry has finished
        if (animval == Anim[i].goal)
        {
            ANIM_CALLBACKp acp = Anim[i].callback;

            // do a callback when done if not nullptr
            if (Anim[i].callback)
                (*Anim[i].callback)(&Anim[i], Anim[i].callbackdata);

            // only delete it if the callback has not changed
            // Logic here is that if the callback changed then something
            // else must be happening with it - dont delete it
            if (Anim[i].callback == acp)
            {
                // decrement the count
                AnimCnt--;

                // move the last entry to the current one to free the last
                // entry up
                Anim[i] = Anim[AnimCnt];
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void AnimClear(void)
{
    AnimCnt = 0;
}

short AnimGetGoal(int animtype, int animindex, DSWActor* animactor)
{
    int i, j;

    j = -1;
    for (i = 0; i < AnimCnt; i++)
    {
		if (animtype == Anim[i].animtype && animindex == Anim[i].animindex && animactor == Anim[i].animactor )
        {
            j = i;
            break;
        }
    }

    return j;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void AnimDelete(int animtype, int animindex, DSWActor* animactor)
{
    int i, j;

    j = -1;
    for (i = 0; i < AnimCnt; i++)
    {
		if (animtype == Anim[i].animtype && animindex == Anim[i].animindex && animactor == Anim[i].animactor )
        {
            j = i;
            break;
        }
    }

    if (j == -1)
        return;

    // decrement the count
    AnimCnt--;

    // move the last entry to the current one to free the last entry up
    Anim[j] = Anim[AnimCnt];
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int AnimSet(int animtype, int animindex, DSWActor* animactor, double thegoal, int thevel)
{
    int i, j;

    ASSERT(AnimCnt < MAXANIM - 1);

    j = AnimCnt;

    // look for existing animation and reset it
    for (i = 0; i < AnimCnt; i++)
    {
        if (animtype == Anim[i].animtype && animindex == Anim[i].animindex && animactor == Anim[i].animactor )
        {
            j = i;
            break;
        }
    }

    Anim[j].animtype = animtype;
    Anim[j].animindex = animindex;
	Anim[j].animactor = animactor;
    Anim[j].goal = thegoal;
    Anim[j].vel = thevel;
    Anim[j].vel_adj = 0;
    Anim[j].callback = nullptr;
    Anim[j].callbackdata = nullptr;

    if (j == AnimCnt)
        AnimCnt++;

    return j;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short AnimSetCallback(short anim_ndx, ANIM_CALLBACKp call, SECTOR_OBJECT* data)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].callback = call;
    Anim[anim_ndx].callbackdata = data;

    return anim_ndx;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

short AnimSetVelAdj(short anim_ndx, double vel_adj)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].vel_adj = vel_adj;

    return anim_ndx;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoPanning(void)
{
    int nx, ny;
    int i;
    sectortype* sectp;
    walltype* wallp;

    SWStatIterator it(STAT_FLOOR_PAN);
    while (auto actor = it.Next())
    {
        sectp = actor->sector();

		nx = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Cos() * 0.25);
		ny = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Sin() * 0.25);

        sectp->addfloorxpan((float)nx);
        sectp->addfloorypan((float)ny);
    }

    it.Reset(STAT_CEILING_PAN);
    while (auto actor = it.Next())
    {
        sectp = actor->sector();

        nx = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Cos() * 0.25);
        ny = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Sin() * 0.25);

        sectp->addceilingxpan((float)nx);
        sectp->addceilingypan((float)ny);
    }

    it.Reset(STAT_WALL_PAN);
    while (auto actor = it.Next())
    {
        wallp = actor->tempwall;

        nx = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Cos() * 0.25);
        ny = xs_CRoundToInt(actor->vel.X * actor->spr.angle.Sin() * 0.25);

        wallp->addxpan((float)nx);
        wallp->addypan((float)ny);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSector(void)
{
    SECTOR_OBJECT* sop;
    bool riding;
    int sync_flag;
    short pnum;
    PLAYER* pp;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {

        if (SO_EMPTY(sop))
            continue;


        riding = false;
        double min_dist = 999999;

        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            if (pp->sop_riding == sop)
            {
                riding = true;
                pp->sop_riding = nullptr;
                break;
            }
            else
            {
				double dist = (pp->pos.XY() - sop->pmid.XY()).Length();
                if (dist < min_dist)
                    min_dist = dist;
            }
        }

        if (sop->Animator)
        {
            (*sop->Animator)(sop);
            continue;
        }

        // force sync SOs to be updated regularly
        if ((sync_flag = (sop->flags & (SOBJ_SYNC1|SOBJ_SYNC2))) != 0)
        {
            if (sync_flag == SOBJ_SYNC1)
                MoveSectorObjects(sop, synctics);
            else
            {
                if (MoveSkip2 == 0)
                    MoveSectorObjects(sop, synctics*2);
            }

            continue;
        }

        if (riding)
        {
            // if riding move smoothly
            // update every time
            MoveSectorObjects(sop, synctics);
        }
        else
        {
            if (min_dist < 937.5)
            {
                // if close update every other time
                if (MoveSkip2 == 0)
                    MoveSectorObjects(sop, synctics * 2);
            }
            else
            {
                // if further update every 4th time
                if (MoveSkip4 == 0)
                    MoveSectorObjects(sop, synctics * 4);
            }
        }
    }

    DoPanning();
    DoLighting();
    DoSineWaveFloor();
    DoSineWaveWall();
    DoSpringBoardDown();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_code saveable_sector_code[] =
{
    SAVE_CODE(DoSpawnSpot),
};

saveable_module saveable_sector =
{
    // code
    saveable_sector_code,
    SIZ(saveable_sector_code),

    // data
    nullptr,
    0
};


END_SW_NS
