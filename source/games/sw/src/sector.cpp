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
void DoPlayerBeginForceJump(PLAYERp);

short FindNextSectorByTag(short sectnum, int tag);
short LevelSecrets;
bool TestVatorMatchActive(short match);
bool TestSpikeMatchActive(short match);
bool TestRotatorMatchActive(short match);
bool TestSlidorMatchActive(short match);
int PlayerCheckDeath(PLAYERp, DSWActor*);
void DoVatorOperate(PLAYERp, short);
void DoVatorMatch(PLAYERp pp, short match);
void DoRotatorOperate(PLAYERp, short);
void DoRotatorMatch(PLAYERp pp, short match, bool);
void DoSlidorOperate(PLAYERp, short);
void DoSlidorMatch(PLAYERp pp, short match, bool);

void KillMatchingCrackSprites(short match);
int DoTrapReset(short match);
int DoTrapMatch(short match);

PLAYERp GlobPlayerP;

TPointer<SECT_USER> SectUser[MAXSECTORS];
TPointer<USER> User[MAXSPRITES];

ANIM Anim[MAXANIM];
short AnimCnt = 0;

SINE_WAVE_FLOOR SineWaveFloor[MAX_SINE_WAVE][21];
SINE_WALL SineWall[MAX_SINE_WALL][MAX_SINE_WALL_POINTS];
SPRING_BOARD SpringBoard[20];

void SetSectorWallBits(short sectnum, int bit_mask, bool set_sectwall, bool set_nextwall)
{
    short wall_num, start_wall;

    wall_num = start_wall = sector[sectnum].wallptr;

    do
    {
        if (set_sectwall)
            SET(wall[wall_num].extra, bit_mask);

        if (set_nextwall)
        {
            uint16_t const nextwall = wall[wall_num].nextwall;
            if (validWallIndex(nextwall))
                SET(wall[nextwall].extra, bit_mask);
        }

        wall_num = wall[wall_num].point2;
    }
    while (wall_num != start_wall);

}

void WallSetupDontMove(void)
{
    SPRITEp spu, spl;
    WALLp wallp;

    SWStatIterator it(STAT_WALL_DONT_MOVE_UPPER);
    while (auto iActor = it.Next())
    {
        spu = &iActor->s();
        SWStatIterator it1(STAT_WALL_DONT_MOVE_LOWER);
        while (auto jActor = it1.Next())
        {
            spl = &jActor->s();

            if (spu->lotag == spl->lotag)
            {
                for (wallp = &wall[0]; wallp < &wall[numwalls]; wallp++)
                {
                    if (wallp->x < spl->x && wallp->x > spu->x && wallp->y < spl->y && wallp->y > spu->y)
                    {
                        SET(wallp->extra, WALLFX_DONT_MOVE);
                    }
                }
            }
        }
    }
}

static void WallSetupLoop(WALLp wp, int16_t lotag, int16_t extra)
{
    // set first wall
    {
        SET(wp->extra, extra);
        uint16_t const nextwall = wp->nextwall;
        if (validWallIndex(nextwall))
            SET(wall[nextwall].extra, extra);
    }

    // Travel all the way around loop setting wall bits
    for (uint16_t wall_num = wp->point2;
         wall[wall_num].lotag != lotag;
         wall_num = wall[wall_num].point2)
    {
        SET(wall[wall_num].extra, extra);
        uint16_t const nextwall = wall[wall_num].nextwall;
        if (validWallIndex(nextwall))
            SET(wall[nextwall].extra, extra);
    }
}

void WallSetup(void)
{
    short i = 0;
    short NextSineWall = 0;
    WALLp wp;

    WallSetupDontMove();

    memset(SineWall, -1, sizeof(SineWall));

    extern int x_min_bound, y_min_bound, x_max_bound, y_max_bound;

    for (wp = &wall[0]; wp < &wall[numwalls]; wp++)
    {
        if (wp->picnum == FAF_PLACE_MIRROR_PIC)
            wp->picnum = FAF_MIRROR_PIC;

        if (wall[i].picnum == FAF_PLACE_MIRROR_PIC+1)
            wall[i].picnum = FAF_MIRROR_PIC+1;

        // this overwrites the lotag so it needs to be called LAST - its down there
        // SetupWallForBreak(wp);

        switch (wp->lotag)
        {
        case TAG_WALL_LOOP_DONT_SPIN:
        {
            WallSetupLoop(wp, TAG_WALL_LOOP_DONT_SPIN, WALLFX_LOOP_DONT_SPIN);
            break;
        }

        case TAG_WALL_LOOP_DONT_SCALE:
        {
            WallSetupLoop(wp, TAG_WALL_LOOP_DONT_SCALE, WALLFX_DONT_SCALE);
            wp->lotag = 0;
            break;
        }

        case TAG_WALL_LOOP_OUTER_SECONDARY:
        {
            // make sure it's a red wall
            if (validWallIndex(wp->nextwall))
            {
                WallSetupLoop(wp, TAG_WALL_LOOP_OUTER_SECONDARY, WALLFX_LOOP_OUTER | WALLFX_LOOP_OUTER_SECONDARY);
            }
            else
            {
                Printf(PRINT_HIGH, "one-sided wall %d in loop setup\n", i);
            }
            break;
        }

        case TAG_WALL_LOOP_OUTER:
        {
            // make sure it's a red wall
            if (validWallIndex(wp->nextwall))
            {
                WallSetupLoop(wp, TAG_WALL_LOOP_OUTER, WALLFX_LOOP_OUTER);
            }
            else
            {
                Printf(PRINT_HIGH, "one-sided wall %d in loop setup\n", i);
            }
            wp->lotag = 0;
            break;
        }

        case TAG_WALL_DONT_MOVE:
        {
            // set first wall
            SET(wp->extra, WALLFX_DONT_MOVE);
            break;
        }

        case TAG_WALL_LOOP_SPIN_2X:
        {
            WallSetupLoop(wp, TAG_WALL_LOOP_SPIN_2X, WALLFX_LOOP_SPIN_2X);
            break;
        }

        case TAG_WALL_LOOP_SPIN_4X:
        {
            WallSetupLoop(wp, TAG_WALL_LOOP_SPIN_4X, WALLFX_LOOP_SPIN_4X);
            break;
        }

        case TAG_WALL_LOOP_REVERSE_SPIN:
        {
            WallSetupLoop(wp, TAG_WALL_LOOP_REVERSE_SPIN, WALLFX_LOOP_REVERSE_SPIN);
            break;
        }

        case TAG_WALL_SINE_Y_BEGIN:
        case TAG_WALL_SINE_X_BEGIN:
        {
            short wall_num, cnt, num_points, type, tag_end;
            SINE_WALLp sw;
            short range = 250, speed = 3, peak = 0;

            tag_end = wp->lotag + 2;

            type = wp->lotag - TAG_WALL_SINE_Y_BEGIN;


            // count up num_points
            for (wall_num = i, num_points = 0;
                 num_points < MAX_SINE_WALL_POINTS && wall[wall_num].lotag != tag_end;
                 wall_num = wall[wall_num].point2, num_points++)
            {
                if (num_points == 0)
                {
                    if (wall[wall_num].hitag)
                        range = wall[wall_num].hitag;
                }
                else if (num_points == 1)
                {
                    if (wall[wall_num].hitag)
                        speed = wall[wall_num].hitag;
                }
                else if (num_points == 2)
                {
                    if (wall[wall_num].hitag)
                        peak = wall[wall_num].hitag;
                }
            }

            if (peak)
                num_points = peak;

            for (wall_num = i, cnt = 0;
                 cnt < MAX_SINE_WALL_POINTS && wall[wall_num].lotag != tag_end;
                 wall_num = wall[wall_num].point2, cnt++)
            {
                // set the first on up
                sw = &SineWall[NextSineWall][cnt];

                sw->type = type;
                sw->wall = wall_num;
                sw->speed_shift = speed;
                sw->range = range;

                // don't allow bullet holes/stars
                SET(wall[wall_num].extra, WALLFX_DONT_STICK);

                if (!sw->type)
                    sw->orig_xy = wall[wall_num].y - (sw->range >> 2);
                else
                    sw->orig_xy = wall[wall_num].x - (sw->range >> 2);

                sw->sintable_ndx = cnt * (2048 / num_points);
            }

            NextSineWall++;

            ASSERT(NextSineWall < MAX_SINE_WALL);

        }
        }

        // this overwrites the lotag so it needs to be called LAST
        SetupWallForBreak(wp);
    }
}


void SectorLiquidSet(short i)
{
    SECT_USERp sectu;

    // ///////////////////////////////////
    //
    // CHECK for pics that mean something
    //
    // ///////////////////////////////////

    if (sector[i].floorpicnum >= 300 && sector[i].floorpicnum <= 307)
    {
        sectu = GetSectUser(i);

        SET(sector[i].extra, SECTFX_LIQUID_WATER);
    }
    else if (sector[i].floorpicnum >= 320 && sector[i].floorpicnum <= 343)
    {
        sectu = GetSectUser(i);

        SET(sector[i].extra, SECTFX_LIQUID_WATER);
    }
    else if (sector[i].floorpicnum >= 780 && sector[i].floorpicnum <= 794)
    {
        sectu = GetSectUser(i);

        SET(sector[i].extra, SECTFX_LIQUID_WATER);
    }
    else if (sector[i].floorpicnum >= 890 && sector[i].floorpicnum <= 897)
    {
        sectu = GetSectUser(i);

        SET(sector[i].extra, SECTFX_LIQUID_WATER);
    }
    else if (sector[i].floorpicnum >= 175 && sector[i].floorpicnum <= 182)
    {
        sectu = GetSectUser(i);

        SET(sector[i].extra, SECTFX_LIQUID_LAVA);
        if (!sectu->damage)
            sectu->damage = 40;
    }
}

void SectorSetup(void)
{
    short i = 0, tag;
    short NextSineWave = 0;

    short ndx;

    WallSetup();

    for (ndx = 0; ndx < MAX_SECTOR_OBJECTS; ndx++)
    {
        memset(&SectorObject[ndx], -1, sizeof(SectorObject[0]));
        // 0 pointers
        //memset(&SectorObject[ndx].sectp, nullptr, sizeof(SectorObject[0].sectp));
        SectorObject[ndx].PreMoveAnimator = nullptr;
        SectorObject[ndx].PostMoveAnimator = nullptr;
        SectorObject[ndx].Animator = nullptr;
        SectorObject[ndx].controller = nullptr;
        SectorObject[ndx].sp_child = nullptr;
        SectorObject[ndx].xmid = INT32_MAX;
    }

    memset(SineWaveFloor, -1, sizeof(SineWaveFloor));
    memset(SpringBoard, -1, sizeof(SpringBoard));

    LevelSecrets = 0;

    for (i = 0; i < numsectors; i++)
    {
        tag = sector[i].lotag;

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

        if (TEST(sector[i].extra, SECTFX_SINK))
        {
            SectorLiquidSet(i);
        }

        if (TEST(sector[i].floorstat, FLOOR_STAT_PLAX))
        {
            // don't do a z adjust for FAF area
            if (sector[i].floorpicnum != FAF_PLACE_MIRROR_PIC)
            {
                SET(sector[i].extra, SECTFX_Z_ADJUST);
            }
        }

        if (TEST(sector[i].ceilingstat, CEILING_STAT_PLAX))
        {
            // don't do a z adjust for FAF area
            if (sector[i].ceilingpicnum != FAF_PLACE_MIRROR_PIC)
            {
                SET(sector[i].extra, SECTFX_Z_ADJUST);
            }
        }

        // ///////////////////////////////////
        //
        // CHECK for sector/sprite objects
        //
        // ///////////////////////////////////

        if (tag >= TAG_OBJECT_CENTER && tag < TAG_OBJECT_CENTER + 100)
        {
            SetupSectorObject(i, tag);
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
            SetSectorWallBits(i, WALLFX_DONT_STICK, true, true);
            break;

        case TAG_SINE_WAVE_FLOOR:
        case TAG_SINE_WAVE_CEILING:
        case TAG_SINE_WAVE_BOTH:
        {
            SINE_WAVE_FLOOR *swf;
            short near_sect = i, base_sect = i;
            uint16_t swf_ndx = 0;
            short cnt = 0, sector_cnt;
            int range;
            int range_diff = 0;
            int wave_diff = 0;
            short peak_dist = 0;
            short speed_shift = 3;
            short num;

            num = (tag - TAG_SINE_WAVE_FLOOR) / 20;

            // set the first on up
            swf = &SineWaveFloor[NextSineWave][swf_ndx];

            swf->flags = 0;

            switch (num)
            {
            case 0:
                SET(swf->flags, SINE_FLOOR);
                if (TEST(sector[base_sect].floorstat, FLOOR_STAT_SLOPE))
                {
                    SET(swf->flags, SINE_SLOPED);
                }
                break;
            case 1:
                SET(swf->flags, SINE_CEILING);
                break;
            case 2:
                SET(swf->flags, SINE_FLOOR | SINE_CEILING);
                break;
            }


            swf->sector = near_sect;
            ASSERT(sector[swf->sector].hitag != 0);
            swf->range = range = Z(sector[swf->sector].hitag);
            swf->floor_origz = sector[swf->sector].floorz - (range >> 2);
            swf->ceiling_origz = sector[swf->sector].ceilingz - (range >> 2);

            // look for the rest by distance
            for (swf_ndx = 1, sector_cnt = 1; true; swf_ndx++)
            {
                // near_sect = FindNextSectorByTag(base_sect,
                // TAG_SINE_WAVE_FLOOR + swf_ndx);
                near_sect = FindNextSectorByTag(base_sect, tag + swf_ndx);

                if (near_sect >= 0)
                {
                    swf = &SineWaveFloor[NextSineWave][swf_ndx];

                    if (swf_ndx == 1 && sector[near_sect].hitag)
                        range_diff = sector[near_sect].hitag;
                    else if (swf_ndx == 2 && sector[near_sect].hitag)
                        speed_shift = sector[near_sect].hitag;
                    else if (swf_ndx == 3 && sector[near_sect].hitag)
                        peak_dist = sector[near_sect].hitag;

                    swf->sector = near_sect;
                    swf->floor_origz = sector[swf->sector].floorz - (range >> 2);
                    swf->ceiling_origz = sector[swf->sector].ceilingz - (range >> 2);
                    range -= range_diff;
                    swf->range = range;

                    base_sect = swf->sector;
                    sector_cnt++;
                }
                else
                    break;
            }


            ASSERT(swf_ndx <= SIZ(SineWaveFloor[NextSineWave]));

            // more than 6 waves and something in high tag - set up wave
            // dissapate
            if (sector_cnt > 8 && sector[base_sect].hitag)
            {
                wave_diff = sector[base_sect].hitag;
            }

            // setup the sintable_ndx based on the actual number of
            // sectors (swf_ndx)
            for (swf = &SineWaveFloor[NextSineWave][0], cnt = 0; swf->sector >= 0 && swf < (SINE_WAVE_FLOORp)&SineWaveFloor[SIZ(SineWaveFloor)]; swf++, cnt++)
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

                    swf->range -= wave_diff;

                    wave_diff += wave_diff;

                    if (swf->range < Z(4))
                        swf->range = Z(4);

                    // reset origz's based on new range
                    swf->floor_origz = sector[swf->sector].floorz - (swf->range >> 2);
                    swf->ceiling_origz = sector[swf->sector].ceilingz - (swf->range >> 2);
                }
            }

            NextSineWave++;

            ASSERT(NextSineWave < MAX_SINE_WAVE);

            break;
        }
        }
    }
}

void SectorMidPoint(short sectnum, int *xmid, int *ymid, int *zmid)
{
    short startwall, endwall, j;
    int xsum = 0, ysum = 0;
    WALLp wp;

    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum - 1;

    for (wp = &wall[startwall], j = startwall; j <= endwall; wp++, j++)
    {
        xsum += wp->x;
        ysum += wp->y;
    }

    *xmid = xsum / (endwall - startwall + 1);
    *ymid = ysum / (endwall - startwall + 1);

    *zmid = DIV2(sector[sectnum].floorz + sector[sectnum].ceilingz);
}


void DoSpringBoard(PLAYERp pp/*, short sectnum*/)
{

    pp->jump_speed = -sector[pp->cursectnum].hitag;
    DoPlayerBeginForceJump(pp);
    return;
}


void DoSpringBoardDown(void)
{
    unsigned sb;
    SPRING_BOARD *sbp;

    for (sb = 0; sb < SIZ(SpringBoard); sb++)
    {
        sbp = &SpringBoard[sb];

        // if empty set up an entry to close the sb later
        if (sbp->Sector != -1)
        {
            if ((sbp->TimeOut -= synctics) <= 0)
            {
                int destz;

                destz = sector[nextsectorneighborz(sbp->Sector, sector[sbp->Sector].floorz, 1, 1)].floorz;

                AnimSet(ANIM_Floorz, sbp->Sector, nullptr, destz, 256);

                sector[sbp->Sector].lotag = TAG_SPRING_BOARD;

                sbp->Sector = -1;
            }
        }
    }


    return;
}

short FindNextSectorByTag(short sectnum, int tag)
{
    short next_sectnum, startwall, endwall, j;

    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum - 1;

    for (j = startwall; j <= endwall; j++)
    {
        next_sectnum = wall[j].nextsector;

        if (next_sectnum >= 0)
        {
            if (sector[next_sectnum].lotag == tag)
            {
                return next_sectnum;
            }
        }
    }

    return -1;

}


int SectorDistance(short sect1, int sect2)
{
    short wallnum1, wallnum2;

    if (sect1 < 0 || sect2 < 0)
        return 9999999;

    wallnum1 = sector[sect1].wallptr;
    wallnum2 = sector[sect2].wallptr;

    // return the distance between the two sectors.
    return Distance(wall[wallnum1].x, wall[wallnum1].y, wall[wallnum2].x, wall[wallnum2].y);
}


int SectorDistanceByMid(short sect1, int sect2)
{
    int sx1, sy1, sx2, sy2, trash;

    SectorMidPoint(sect1, &sx1, &sy1, &trash);
    SectorMidPoint(sect2, &sx2, &sy2, &trash);

    // return the distance between the two sectors.
    return Distance(sx1, sy1, sx2, sy2);
}

short DoSpawnActorTrigger(short match)
{
    short spawn_count = 0;
    SPRITEp sp;

    SWStatIterator it(STAT_SPAWN_TRIGGER);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->hitag == match)
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

int OperateSector(short sectnum, short player_is_operating)
{
    PLAYERp pp = GlobPlayerP;

    // Don't let actors operate locked or secret doors
    if (!player_is_operating)
    {
        SPRITEp fsp;

        if (SectUser[sectnum].Data() && SectUser[sectnum]->stag == SECT_LOCK_DOOR)
            return false;

        SWSectIterator it(sectnum);
        while (auto actor = it.Next())
        {
            fsp = &actor->s();

            if (SectUser[fsp->sectnum].Data() && SectUser[fsp->sectnum]->stag == SECT_LOCK_DOOR)
                return false;

            if (fsp->statnum == STAT_VATOR && SP_TAG1(fsp) == SECT_VATOR && TEST_BOOL7(fsp))
                return false;
            if (fsp->statnum == STAT_ROTATOR && SP_TAG1(fsp) == SECT_ROTATOR && TEST_BOOL7(fsp))
                return false;
            if (fsp->statnum == STAT_SLIDOR && SP_TAG1(fsp) == SECT_SLIDOR && TEST_BOOL7(fsp))
                return false;

        }
    }

    //CON_Message("Operating sectnum %d",sectnum);
    //CON_Message("stag = %d, lock = %d",SectUser[sectnum]->stag,SECT_LOCK_DOOR);

    switch (sector[sectnum].lotag)
    {

    case TAG_VATOR:
        DoVatorOperate(pp, sectnum);
        return true;

    case TAG_ROTATOR:
        DoRotatorOperate(pp, sectnum);
        return true;

    case TAG_SLIDOR:
        DoSlidorOperate(pp, sectnum);
        return true;
    }

    return false;
}

int 
OperateWall(short wallnum, short player_is_operating)
{
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

short AnimateSwitch(SPRITEp sp, short tgt_value)
{
    // if the value is not ON or OFF
    // then it is a straight toggle

    switch (sp->picnum)
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

        sp->picnum += 1;

        // if the tgt_value should be true
        // flip it again - recursive but only once
        if (tgt_value == false)
        {
            AnimateSwitch(sp, tgt_value);
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

        sp->picnum -= 1;

        if (tgt_value == int(true))
        {
            AnimateSwitch(sp, tgt_value);
            return true;
        }

        return false;
    }
    return false;
}


void SectorExp(DSWActor* actor, short sectnum, short orig_ang, int zh)
{
    SPRITEp sp = &actor->s();
    USERp u = actor->u();
    SPRITEp exp;
    USERp eu;
    int x,y,z;

    RESET(sp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL|CSTAT_SPRITE_ALIGNMENT_FLOOR);
    SectorMidPoint(sectnum, &x, &y, &z);
    sp->ang = orig_ang;
    sp->x = x;
    sp->y = y;
    sp->z = z;

    // randomize the explosions
    sp->ang += RANDOM_P2(256) - 128;
    sp->x += RANDOM_P2(1024) - 512;
    sp->y += RANDOM_P2(1024) - 512;
    sp->z = zh;

    // setup vars needed by SectorExp
    ChangeActorSect(actor, sectnum);
    getzsofslope(sp->sectnum, sp->x, sp->y, &u->hiz, &u->loz);

    // spawn explosion
    auto explosion = SpawnSectorExp(actor);
    if (!explosion) return;
    exp = &explosion->s();
    eu = explosion->u();

    exp->xrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    exp->yrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    eu->xchange = MOVEx(92, exp->ang);
    eu->ychange = MOVEy(92, exp->ang);
}


void DoExplodeSector(short match)
{
    short orig_ang;
    int zh;

    SECTORp sectp;

    orig_ang = 0; //sp->ang;

    SWStatIterator it(STAT_EXPLODING_CEIL_FLOOR);
    while (auto actor = it.Next())
    {
        auto esp = &actor->s();

        if (match != esp->lotag)
            continue;

        if (!actor->hasU())
            /*u = */SpawnUser(actor, 0, nullptr);

        sectp = &sector[esp->sectnum];

        sectp->ceilingz -= Z(SP_TAG4(esp));

        if (SP_TAG5(esp))
        {
            sectp->floorheinum = SP_TAG5(esp);
            SET(sectp->floorstat, FLOOR_STAT_SLOPE);
        }

        if (SP_TAG6(esp))
        {
            sectp->ceilingheinum = SP_TAG6(esp);
            SET(sectp->ceilingstat, CEILING_STAT_SLOPE);
        }

        for (zh = sectp->ceilingz; zh < sectp->floorz; zh += Z(60))
        {
            SectorExp(actor, esp->sectnum, orig_ang, zh + Z(RANDOM_P2(64)) - Z(32));
        }

        // don't need it any more
        KillActor(actor);
    }
}


int DoSpawnSpot(DSWActor* actor)
{
    USER* u = actor->u();

    if ((u->WaitTics -= synctics) < 0)
    {
        change_actor_stat(actor, STAT_SPAWN_SPOT);
        SpawnShrap(actor, nullptr);

        if (u->LastDamage == 1)
        {
            KillActor(actor);
            return 0;
        }
    }

    return 0;
}

// spawns shrap when killing an object
void DoSpawnSpotsForKill(short match)
{
    SPRITEp sp;
    USERp u;

    if (match < 0)
        return;

    SWStatIterator it(STAT_SPAWN_SPOT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        // change the stat num and set the delay correctly to call SpawnShrap
        if (sp->hitag == SPAWN_SPOT && sp->lotag == match)
        {
            u = actor->u();
            change_actor_stat(actor, STAT_NO_STATE);
            u->ActorActionFunc = DoSpawnSpot;
            u->WaitTics = SP_TAG5(sp) * 15;
            SetActorZ(actor, &sp->pos);
            // setting for Killed
            u->LastDamage = 1;
        }
    }
}

// spawns shrap when damaging an object
void DoSpawnSpotsForDamage(short match)
{
    SPRITEp sp;
    USERp u;

    if (match < 0)
        return;

    SWStatIterator it(STAT_SPAWN_SPOT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        // change the stat num and set the delay correctly to call SpawnShrap

        if (sp->hitag == SPAWN_SPOT && sp->lotag == match)
        {
            u = actor->u();
            change_actor_stat(actor, STAT_NO_STATE);
            u->ActorActionFunc = DoSpawnSpot;
            u->WaitTics = SP_TAG7(sp) * 15;
            // setting for Damaged
            u->LastDamage = 0;
        }
    }
}

void DoSoundSpotMatch(short match, short sound_num, short sound_type)
{
    SPRITEp sp;
    int flags;
    short snd2play;

    //sound_type is not used

    sound_num--;

    ASSERT(sound_num >= 0);

    SWStatIterator it(STAT_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (SP_TAG2(sp) == match && !TEST_BOOL6(sp))
        {
            short snd[3];

            snd[0] = SP_TAG13(sp); // tag4 is copied to tag13
            snd[1] = SP_TAG5(sp);
            snd[2] = SP_TAG6(sp);

            snd2play = 0;
            flags = 0;

            if (TEST_BOOL2(sp))
                flags = v3df_follow|v3df_nolookup|v3df_init;

            // play once and only once
            if (TEST_BOOL1(sp))
                SET_BOOL6(sp);

            // don't pan
            if (TEST_BOOL4(sp))
                flags |= v3df_dontpan;
            // add doppler
            if (TEST_BOOL5(sp))
                flags |= v3df_doppler;
            // random
            if (TEST_BOOL3(sp))
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

            if (TEST_BOOL7(sp))
            {
                PLAYERp pp = GlobPlayerP;

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

void DoSoundSpotStopSound(short match)
{

    SWStatIterator it(STAT_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();

        // found match and is a follow type
        if (SP_TAG2(sp) == match && TEST_BOOL2(sp))
        {
            DeleteNoSoundOwner(actor);
        }
    }
}

void DoStopSoundSpotMatch(short match)
{
    SPRITEp sp;

    SWStatIterator it(STAT_STOP_SOUND_SPOT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (SP_TAG2(sp) == match)
        {
            DoSoundSpotStopSound(SP_TAG5(sp));
        }
    }
}


bool TestKillSectorObject(SECTOR_OBJECTp sop)
{
    if (TEST(sop->flags, SOBJ_KILLABLE))
    {
        KillMatchingCrackSprites(sop->match_event);
        // get new sectnums
        CollapseSectorObject(sop, sop->xmid, sop->ymid);
        DoSpawnSpotsForKill(sop->match_event);
        KillSectorObjectSprites(sop);
        return true;
    }

    return false;
}

short DoSectorObjectKillMatch(short match)
{
    SECTOR_OBJECTp sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (SO_EMPTY(sop))
            continue;

        if (sop->match_event == match)
            return TestKillSectorObject(sop);
    }

    return false;
}


bool SearchExplodeSectorMatch(short match)
{
    // THIS IS ONLY CALLED FROM DoMatchEverything
    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();

        if (sp->hitag == match)
        {
            KillMatchingCrackSprites(match);
            DoExplodeSector(match);
            return true;
        }
    }

    return false;
}

void KillMatchingCrackSprites(short match)
{
    SPRITEp sp;

    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->hitag == match)
        {
            if (TEST(SP_TAG8(sp), BIT(2)))
                continue;

            KillActor(actor);
        }
    }
}

void WeaponExplodeSectorInRange(DSWActor* wActor)
{
    SPRITEp wp = &wActor->s();
    USERp wu = wActor->u();
    SPRITEp sp;
    int dist;
    int radius;

    SWStatIterator it(STAT_SPRITE_HIT_MATCH);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        // test to see if explosion is close to crack sprite
        dist = FindDistance3D(wp->x - sp->x, wp->y - sp->y, wp->z - sp->z);

        if (sp->clipdist == 0)
            continue;

        radius = (((int)sp->clipdist) << 2) * 8;

        if ((unsigned int)dist > (wu->Radius/2) + radius)
            continue;

        if (!FAFcansee(wp->x,wp->y,wp->z,wp->sectnum,sp->x,sp->y,sp->z,sp->sectnum))
            continue;


        // pass in explosion type
        MissileHitMatch(wActor, WPN_ROCKET, actor);
    }
}


void ShootableSwitch(DSWActor* actor)
{
    SPRITEp sp = &actor->s();

    switch (sp->picnum)
    {
    case SWITCH_SHOOTABLE_1:
        //RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(actor, false);
        sp->picnum = SWITCH_SHOOTABLE_1 + 1;
        break;
    case SWITCH_FUSE:
    case SWITCH_FUSE + 1:
        RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(actor, false);
        sp->picnum = SWITCH_FUSE + 2;
        break;
    }
}

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

    int del_x = 0,del_y = 0;
    unsigned stat;

    while (true)
    {
        DSWActor* found = nullptr;

        // search for a DELETE_SPRITE with same match tag
        SWStatIterator it(STAT_DELETE_SPRITE);
        while (auto actor = it.Next())
        {
            auto sp = &actor->s();
            if (sp->lotag == match)
            {
                found = actor;
                del_x = sp->x;
                del_y = sp->y;
                break;
            }
        }

        if (found == nullptr)
            return;

        for (stat = 0; stat < SIZ(StatList); stat++)
        {
            SWStatIterator it(StatList[stat]);
            while (auto actor = it.Next())
            {
                auto sp = &actor->s();
                if (del_x == sp->x && del_y == sp->y)
                {
                    // special case lighting delete of Fade On/off after fades
                    if (StatList[stat] == STAT_LIGHTING)
                    {
                        // set shade to darkest and then kill it
                        sp->shade = int8_t(SP_TAG6(sp));
                        sp->pal = 0;
                        SectorLightShade(actor, sp->shade);
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

void DoChangorMatch(short match)
{
    SPRITEp sp;
    SECTORp sectp;

    SWStatIterator it(STAT_CHANGOR);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();
        sectp = &sector[sp->sectnum];

        if (SP_TAG2(sp) != match)
            continue;

        if (TEST_BOOL1(sp))
        {
            sectp->ceilingpicnum = SP_TAG4(sp);
            sectp->ceilingz += Z(SP_TAG5(sp));
            sectp->ceilingheinum += SP_TAG6(sp);

            if (sectp->ceilingheinum)
                SET(sectp->ceilingstat, CEILING_STAT_SLOPE);
            else
                RESET(sectp->ceilingstat, CEILING_STAT_SLOPE);

            sectp->ceilingshade += SP_TAG7(sp);
            sectp->ceilingpal += SP_TAG8(sp);
        }
        else
        {
            sectp->floorpicnum = SP_TAG4(sp);
            sectp->floorz += Z(SP_TAG5(sp));
            sectp->floorheinum += SP_TAG6(sp);

            if (sectp->floorheinum)
                SET(sectp->floorstat, FLOOR_STAT_SLOPE);
            else
                RESET(sectp->floorstat, FLOOR_STAT_SLOPE);

            sectp->floorshade += SP_TAG7(sp);
            sectp->floorpal += SP_TAG8(sp);
        }

        sectp->visibility += SP_TAG9(sp);

        // if not set then go ahead and kill it
        if (TEST_BOOL2(sp) == 0)
        {
            KillActor(actor);
        }
    }
}

void DoMatchEverything(PLAYERp pp, short match, short state)
{
    PLAYERp bak;

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

bool ComboSwitchTest(short combo_type, short match)
{
    int state;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();

        if (sp->lotag == combo_type && sp->hitag == match)
        {
            // dont toggle - get the current state
            state = AnimateSwitch(sp, 999);

            // if any one is not set correctly then switch is not set
            if (state != SP_TAG3(sp))
            {
                return false;
            }
        }
    }

    return true;
}

// NOTE: switches are always wall sprites
int OperateSprite(DSWActor* actor, short player_is_operating)
{
    SPRITEp sp = &actor->s();
    USERp u = actor->u();
    PLAYERp pp = nullptr;
    short state;
    short key_num=0;
    extern STATE s_Pachinko1Operate[];
    extern STATE s_Pachinko2Operate[];
    extern STATE s_Pachinko3Operate[];
    extern STATE s_Pachinko4Operate[];

    if (Prediction)
        return false;

    if (sp->picnum == ST1)
        return false;

    if (player_is_operating)
    {
        pp = GlobPlayerP;

        if (!FAFcansee(pp->posx, pp->posy, pp->posz, pp->cursectnum, sp->x, sp->y, sp->z - DIV2(SPRITEp_SIZE_Z(sp)), sp->sectnum))
            return false;
    }

    switch (sp->lotag)
    {
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    {
        short choose_snd;

        u->FlagOwner = 1;
        u->WaitTics = SEC(4);

        if (pp != Player+myconnectindex) return true;

        choose_snd = STD_RANDOM_RANGE(1000);
        if (sp->lotag == CARGIRL_R0)
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
        else if (sp->lotag == MECHANICGIRL_R0)
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
        else if (sp->lotag == SAILORGIRL_R0)
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
        else if (sp->lotag == PRUNEGIRL_R0)
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
        else if (sp->lotag == TOILETGIRL_R0)
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
        if (u->WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        u->WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko1Operate);

        return true;

    case PACHINKO2:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        u->WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko2Operate);

        return true;

    case PACHINKO3:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        u->WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko3Operate);

        return true;

    case PACHINKO4:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return true;

        PlaySound(DIGI_PFLIP, actor, v3df_none);
        u->WaitTics = SEC(3) + SEC(RandomRange(10));
        ChangeState(actor,s_Pachinko4Operate);

        return true;

    case SWITCH_LOCKED:
        key_num = sp->hitag;
        if (pp->HasKey[key_num - 1])
        {
            int i;
            for (i=0; i<numsectors; i++)
            {
                if (SectUser[i].Data() && SectUser[i]->stag == SECT_LOCK_DOOR && SectUser[i]->number == key_num)
                    SectUser[i]->number = 0;  // unlock all doors of this type
            }
            UnlockKeyLock(key_num, actor);
        }

        return true;

    case TAG_COMBO_SWITCH_EVERYTHING:

        // change the switch state
        AnimateSwitch(sp, -1);
        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, sp->hitag))
        {
            DoMatchEverything(pp, sp->hitag, true);
        }

        return true;

    case TAG_COMBO_SWITCH_EVERYTHING_ONCE:

        // change the switch state
        AnimateSwitch(sp, -1);
        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, sp->hitag))
        {
            DoMatchEverything(pp, sp->hitag, true);
        }

        sp->lotag = 0;
        sp->hitag = 0;
        return true;

    case TAG_SWITCH_EVERYTHING:
        state = AnimateSwitch(sp, -1);
        DoMatchEverything(pp, sp->hitag, state);
        return true;

    case TAG_SWITCH_EVERYTHING_ONCE:
        state = AnimateSwitch(sp, -1);
        DoMatchEverything(pp, sp->hitag, state);
        sp->lotag = 0;
        sp->hitag = 0;
        return true;

    case TAG_LIGHT_SWITCH:

        state = AnimateSwitch(sp, -1);
        DoLightingMatch(sp->hitag, state);
        return true;

    case TAG_SPRITE_SWITCH_VATOR:
    {
        // make sure all vators are inactive before allowing
        // to repress switch
        if (!TestVatorMatchActive(sp->hitag))
            DoVatorMatch(pp, sp->hitag);

        if (!TestSpikeMatchActive(sp->hitag))
            DoSpikeMatch(sp->hitag);

        if (!TestRotatorMatchActive(sp->hitag))
            DoRotatorMatch(pp, sp->hitag, false);

        if (!TestSlidorMatchActive(sp->hitag))
            DoSlidorMatch(pp, sp->hitag, false);

        return true;
    }

    case TAG_LEVEL_EXIT_SWITCH:
    {
        AnimateSwitch(sp, -1);

        PlaySound(DIGI_BIGSWITCH, actor, v3df_none);

		MapRecord *map;
        if (sp->hitag)
            map = FindMapByLevelNum(sp->hitag);
        else
            map = FindNextMap(currentLevel);
		ChangeLevel(map, g_nextskill);

        return true;
    }

    case TAG_SPRITE_GRATING:
    {
        USERp u;

        change_actor_stat(actor, STAT_NO_STATE);

        u = SpawnUser(actor, 0, nullptr);

        u->ActorActionFunc = DoGrating;

        sp->lotag = 0;
        sp->hitag /= 2;

        return true;
    }

    case TAG_SO_SCALE_SWITCH:
        AnimateSwitch(sp, -1);
        DoSectorObjectSetScale(sp->hitag);
        return true;

    case TAG_SO_SCALE_ONCE_SWITCH:
        AnimateSwitch(sp, -1);
        DoSectorObjectSetScale(sp->hitag);
        sp->lotag = 0;
        sp->hitag = 0;
        return true;

    case TAG_SO_EVENT_SWITCH:
    {
        state = AnimateSwitch(sp, -1);

        DoMatchEverything(nullptr, sp->hitag, state);

        sp->hitag = 0;
        sp->lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, actor, v3df_none);
        break;
    }

    case TAG_ROTATE_SO_SWITCH:
    {
        short so_num;
        SECTOR_OBJECTp sop;

        so_num = sp->hitag;

        ASSERT(so_num <= 20);
        ASSERT(SectorObject[so_num].num_sectors != -1);

        AnimateSwitch(sp, -1);

        sop = &SectorObject[so_num];

        sop->ang_tgt = NORM_ANGLE(sop->ang_tgt + 512);

        PlaySound(DIGI_BIGSWITCH, actor, v3df_none);

        return true;

        break;
    }
    }

    return false;
}

int DoTrapReset(short match)
{
    SPRITEp sp;
    USERp u;

    SWStatIterator it(STAT_TRAP);
    while (auto actor = it.Next())
    {
        sp = &actor->s();
        u = actor->u();

        if (sp->lotag != match)
            continue;

        // if correct type and matches
        if (sp->hitag == FIREBALL_TRAP)
            u->WaitTics = 0;

        // if correct type and matches
        if (sp->hitag == BOLT_TRAP)
            u->WaitTics = 0;

        // if correct type and matches
        if (sp->hitag == SPEAR_TRAP)
            u->WaitTics = 0;
    }
    return 0;
}

int DoTrapMatch(short match)
{
    SPRITEp sp;
    USERp u;

    // may need to be reset to fire immediately

    SWStatIterator it(STAT_TRAP);
    while (auto actor = it.Next())
    {
        sp = &actor->s();
        u = actor->u();

        if (sp->lotag != match)
            continue;

        // if correct type and matches
        if (sp->hitag == FIREBALL_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitFireballTrap(actor);
            }
        }

        // if correct type and matches
        if (sp->hitag == BOLT_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitBoltTrap(actor);
            }
        }

        // if correct type and matches
        if (sp->hitag == SPEAR_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitSpearTrap(actor);
            }
        }
    }
    return 0;
}


void OperateTripTrigger(PLAYERp pp)
{
    if (Prediction)
        return;

    if (pp->cursectnum < 0)
        return;

    SECTORp sectp = &sector[pp->cursectnum];

    // old method
    switch (sector[pp->cursectnum].lotag)
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
        if (pp == Player+myconnectindex)
            PlayerSound(DIGI_ANCIENTSECRET, v3df_dontpan|v3df_doppler|v3df_follow,pp);

        SECRET_Trigger(pp->cursectnum);

        PutStringInfo(pp, GStrings("TXTS_SECRET"));
        // always give to the first player
        Player->SecretsFound++;
        sectp->lotag = 0;
        sectp->hitag = 0;
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
        int dist;
        int i;
        SPRITEp sp;
        USERp u;

        dist = sectp->hitag;

        SWStatIterator it(STAT_ENEMY);
        while (auto actor = it.Next())
        {
            sp = &actor->s();
            u = actor->u();

            if (TEST(u->Flags, SPR_WAIT_FOR_TRIGGER))
            {
                if (Distance(sp->x, sp->y, pp->posx, pp->posy) < dist)
                {
                    u->targetActor = pp->Actor();
                    RESET(u->Flags, SPR_WAIT_FOR_TRIGGER);
                }
            }
        }

        break;
    }

    case TAG_TRIGGER_MISSILE_TRAP:
    {
        // reset traps so they fire immediately
        DoTrapReset(sector[pp->cursectnum].hitag);
        break;
    }

    case TAG_TRIGGER_EXPLODING_SECTOR:
    {
        DoMatchEverything(nullptr, sector[pp->cursectnum].hitag, -1);
        break;
    }

    case TAG_SPAWN_ACTOR_TRIGGER:
    {
        DoMatchEverything(nullptr, sector[pp->cursectnum].hitag, -1);

        sector[pp->cursectnum].hitag = 0;
        sector[pp->cursectnum].lotag = 0;
        break;
    }

    case TAG_SO_EVENT_TRIGGER:
    {
        DoMatchEverything(nullptr, sector[pp->cursectnum].hitag, -1);

        sector[pp->cursectnum].hitag = 0;
        sector[pp->cursectnum].lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, pp, v3df_none);
        break;
    }
    }
}

void OperateContinuousTrigger(PLAYERp pp)
{
    if (Prediction)
        return;

    if (pp->cursectnum < 0)
        return;

    switch (sector[pp->cursectnum].lotag)
    {
    case TAG_TRIGGER_MISSILE_TRAP:
    {
        DoTrapMatch(sector[pp->cursectnum].hitag);

        break;
    }
    }
}


short PlayerTakeSectorDamage(PLAYERp pp)
{
    SECT_USERp sectu = SectUser[pp->cursectnum].Data();
    USERp u = pp->Actor()->u();

    // the calling routine must make sure sectu exists
    if ((u->DamageTics -= synctics) < 0)
    {
        u->DamageTics = DAMAGE_TIME;

        PlayerUpdateHealth(pp, -sectu->damage);
        PlayerCheckDeath(pp, nullptr);
    }
    return 0;
}

// Needed in order to see if Player should grunt if he can't find a wall to operate on
// If player is too far away, don't grunt
enum { PLAYER_SOUNDEVENT_TAG = 900 };
bool NearThings(PLAYERp pp)
{
    short neartagsect, neartagwall, neartagsprite;
    int neartaghitdist;


    // Check player's current sector for triggered sound
    if (sector[pp->cursectnum].hitag == PLAYER_SOUNDEVENT_TAG)
    {
        if (pp == Player+myconnectindex)
            PlayerSound(sector[pp->cursectnum].lotag, v3df_follow|v3df_dontpan,pp);
        return false;
    }

    neartag(pp->posx, pp->posy, pp->posz, pp->cursectnum, pp->angle.ang.asbuild(),
            &neartagsect, &neartagwall, &neartagsprite,
            &neartaghitdist, 1024L, NTAG_SEARCH_LO_HI, nullptr);


    // hit a sprite? Check to see if it has sound info in it!
    // This can work with any sprite!
    if (neartagsprite >= 0)
    {
        SPRITEp sp = &sprite[neartagsprite];

        // Go through list of cases
        if (sp->hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(sp->lotag, v3df_follow|v3df_dontpan,pp);
        }
        return false;   // Return false so he doesn't grunt
    }

    if (neartagwall >= 0)
    {
        // Check player's current sector for triggered sound
        if (wall[neartagwall].hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(wall[neartagwall].lotag, v3df_follow|v3df_dontpan,pp);
            return false;   // We are playing a sound so don't return true
        }
        return true;
    }
    // This only gets called if nothing else worked, check for nearness to a wall
    {
        HITINFO hitinfo;
        short dang = pp->angle.ang.asbuild();

        FAFhitscan(pp->posx, pp->posy, pp->posz - Z(30), pp->cursectnum,    // Start position
                   bcos(dang),  // X vector of 3D ang
                   bsin(dang),  // Y vector of 3D ang
                   0,           // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
            return false;

        if (Distance(hitinfo.pos.x, hitinfo.pos.y, pp->posx, pp->posy) > 1500)
            return false;

        // hit a sprite?
        if (hitinfo.hitactor != nullptr)
            return false;

        if (neartagsect >= 0)
            return true;

        if (hitinfo.wall >= 0)
        {
            WALLp wp;

            wp =  &wall[hitinfo.wall];

            // Near a plain old vanilla wall.  Can't do anything but grunt.
            if (!TEST(wp->extra, WALLFX_DONT_STICK) && pp == Player+myconnectindex)
            {
                if (STD_RANDOM_RANGE(1000) > 970)
                    PlayerSound(DIGI_HITTINGWALLS, v3df_follow|v3df_dontpan,pp);
                else
                    PlayerSound(DIGI_SEARCHWALL, v3df_follow|v3df_dontpan,pp);
            }

            return true;
        }

        return false;
    }

}

short nti_cnt;

void NearTagList(NEAR_TAG_INFOp ntip, PLAYERp pp, int z, int dist, int type, int count)
{
    short save_lotag, save_hitag;
    short neartagsector, neartagwall, neartagsprite;
    int neartaghitdist;


    neartag(pp->posx, pp->posy, z, pp->cursectnum, pp->angle.ang.asbuild(),
            &neartagsector, &neartagwall, &neartagsprite,
            &neartaghitdist, dist, type, nullptr);

    if (neartagsector >= 0)
    {
        // save off values
        save_lotag = sector[neartagsector].lotag;
        save_hitag = sector[neartagsector].hitag;

        ntip->dist = neartaghitdist;
        ntip->sectnum = neartagsector;
        ntip->wallnum = -1;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        sector[neartagsector].lotag = 0;
        sector[neartagsector].hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        sector[neartagsector].lotag = save_lotag;
        sector[neartagsector].hitag = save_hitag;
    }
    else if (neartagwall >= 0)
    {
        // save off values
        save_lotag = wall[neartagwall].lotag;
        save_hitag = wall[neartagwall].hitag;

        ntip->dist = neartaghitdist;
        ntip->sectnum = -1;
        ntip->wallnum = neartagwall;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;
        
        if (nti_cnt >= count)
            return;

        // remove them
        wall[neartagwall].lotag = 0;
        wall[neartagwall].hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        wall[neartagwall].lotag = save_lotag;
        wall[neartagwall].hitag = save_hitag;
    }
    else if (neartagsprite >= 0)
    {
        auto actor = &swActors[neartagsprite];
        auto sp = &actor->s();
        // save off values
        save_lotag = sp->lotag;
        save_hitag = sp->hitag;

        ntip->dist = neartaghitdist;
        ntip->sectnum = -1;
        ntip->wallnum = -1;
        ntip->actor = actor;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        sp->lotag = 0;
        sp->hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        sp->lotag = save_lotag;
        sp->hitag = save_hitag;
    }
    else
    {
        ntip->dist = -1;
        ntip->sectnum = -1;
        ntip->wallnum = -1;
        ntip->actor = nullptr;
        nti_cnt++;
        ntip++;

        return;
    }
}

void BuildNearTagList(NEAR_TAG_INFOp ntip, int size, PLAYERp pp, int z, int dist, int type, int count)
{
    memset(ntip, -1, size);
    nti_cnt = 0;
    NearTagList(ntip, pp, z, dist, type, count);
}


int DoPlayerGrabStar(PLAYERp pp)
{
    SPRITEp sp = nullptr;
    int i;

    // MUST check exact z's of each star or it will never work
    for (i = 0; i < MAX_STAR_QUEUE; i++)
    {
        if (StarQueue[i] != nullptr)
        {
            sp = &StarQueue[i]->s();

            if (FindDistance3D(sp->x - pp->posx, sp->y - pp->posy, sp->z - pp->posz + Z(12)) < 500)
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
        if (TEST(pp->WpnFlags, BIT(WPN_STAR)))
            return true;
        SET(pp->WpnFlags, BIT(WPN_STAR));
        InitWeaponStar(pp);
        return true;
    }

    return false;
}



void PlayerOperateEnv(PLAYERp pp)
{
    bool found;

    if (Prediction || !pp->Actor())
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

            BuildNearTagList(nti, sizeof(nti), pp, pp->posz, 2048L, NTAG_SEARCH_LO_HI, 8);

            found = false;

            // try and find a sprite
            for (nt_ndx = 0; nti[nt_ndx].dist >= 0; nt_ndx++)
            {
                if (nti[nt_ndx].actor != nullptr && nti[nt_ndx].dist < 1024 + 768)
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
                int z[3];
                unsigned i;
                NEAR_TAG_INFO nti[16];
                short nt_ndx;
                auto psp = &pp->Actor()->s();

                z[0] = psp->z - SPRITEp_SIZE_Z(psp) - Z(10);
                z[1] = psp->z;
                z[2] = DIV2(z[0] + z[1]);

                for (i = 0; i < SIZ(z); i++)
                {
                    BuildNearTagList(nti, sizeof(nti), pp, z[i], 1024 + 768L, NTAG_SEARCH_LO_HI, 8);

                    for (nt_ndx = 0; nti[nt_ndx].dist >= 0; nt_ndx++)
                    {
                        if (nti[nt_ndx].actor != nullptr && nti[nt_ndx].dist < 1024 + 768)
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
                int neartaghitdist;
                short neartagsector, neartagwall;

                neartaghitdist = nti[0].dist;
                neartagsector = nti[0].sectnum;
                neartagwall = nti[0].wallnum;

                if (neartagsector >= 0 && neartaghitdist < 1024)
                {
                    if (OperateSector(neartagsector, true))
                    {
                        // Release the key
                        pp->KeyPressBits &= ~SB_OPEN;
                    }
                }

                if (neartagwall >= 0 && neartaghitdist < 1024)
                {
                    if (OperateWall(neartagwall, true))
                    {
                        pp->KeyPressBits &= ~SB_OPEN;
                    }
                }
            }

            //
            // Trigger operations
            //

			switch (sector[pp->cursectnum].lotag)
            {
            case TAG_VATOR:
                DoVatorOperate(pp, pp->cursectnum);
                DoSpikeOperate(pp->cursectnum);
                DoRotatorOperate(pp, pp->cursectnum);
                DoSlidorOperate(pp, pp->cursectnum);
                break;
            case TAG_SPRING_BOARD:
                DoSpringBoard(pp/*, pp->cursectnum*/);
                pp->KeyPressBits &= ~SB_OPEN;
                break;
            case TAG_DOOR_ROTATE:
                if (OperateSector(pp->cursectnum, true))
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

    SECT_USERp sectu;
    if (pp->cursectnum >= 0 && (sectu = SectUser[pp->cursectnum].Data()) && sectu->damage)
    {
        SECTORp sectp = &sector[pp->cursectnum];
        if (TEST(sectu->flags, SECTFU_DAMAGE_ABOVE_SECTOR))
        {
            PlayerTakeSectorDamage(pp);
        }
        else if ((SPRITEp_BOS(&pp->Actor()->s()) >= sectp->floorz) && !TEST(pp->Flags, PF_DIVING))
        {
            PlayerTakeSectorDamage(pp);
        }
    }
    else
    {
        USERp u = pp->Actor()->u();
        u->DamageTics = 0;
    }


    // ////////////////////////////
    //
    // Trigger stuff
    //
    // ////////////////////////////

    OperateContinuousTrigger(pp);

    // just changed sectors
    if (pp->lastcursectnum != pp->cursectnum)
    {
        OperateTripTrigger(pp);

        if (pp->cursectnum >= 0 && TEST(sector[pp->cursectnum].extra, SECTFX_WARP_SECTOR))
        {
            if (!TEST(pp->Flags2, PF2_TELEPORTED))
            {
                DoPlayerWarpTeleporter(pp);
            }
        }

        RESET(pp->Flags2, PF2_TELEPORTED);
    }
}



void DoSineWaveFloor(void)
{
    SINE_WAVE_FLOOR *swf;
    int newz;
    int wave;
    int flags;

    for (wave = 0; wave < MAX_SINE_WAVE; wave++)
    {
        for (swf = &SineWaveFloor[wave][0], flags = swf->flags; swf->sector >= 0 && swf < &SineWaveFloor[wave][SIZ(SineWaveFloor[wave])]; swf++)
        {

            swf->sintable_ndx = NORM_ANGLE(swf->sintable_ndx + (synctics << swf->speed_shift));

            if (TEST(flags, SINE_FLOOR))
            {
                newz = swf->floor_origz + MulScale(swf->range, bsin(swf->sintable_ndx), 14);
                sector[swf->sector].floorz = newz;
            }

            if (TEST(flags, SINE_CEILING))
            {
                newz = swf->ceiling_origz + MulScale(swf->range, bsin(swf->sintable_ndx), 14);
                sector[swf->sector].ceilingz = newz;
            }

        }
    }

    /*  SLOPED SIN-WAVE FLOORS:

    It's best to program sloped sin-wave floors in 2 steps:
       1.  First set the floorz of the floor as the sin code normally does it.
       2.  Adjust the slopes by calling alignflorslope once for each sector.

    Note:  For this to work, the first wall of each sin-wave sector must be
           aligned on the same side of each sector for the entire strip.
    */

    for (wave = 0; wave < MAX_SINE_WAVE; wave++)
    {
        for (swf = &SineWaveFloor[wave][0], flags = swf->flags; swf->sector >= 0 && swf < &SineWaveFloor[wave][SIZ(SineWaveFloor[wave])]; swf++)
        {
            if (!TEST(sector[swf->sector].floorstat, FLOOR_STAT_SLOPE))
                continue;

            if (TEST(flags, SINE_SLOPED))
            {
                WALLp wal;
                if (sector[swf->sector].wallnum == 4)
                {
                    //Set wal to the wall on the opposite side of the sector
                    wal = &wall[sector[swf->sector].wallptr+2];

                    //Pass (Sector, x, y, z)
                    alignflorslope(swf->sector,wal->x,wal->y,
                                   sector[wal->nextsector].floorz);
                }
            }
        }
    }
}


void DoSineWaveWall(void)
{
    SINE_WALL *sw;
    int New;
    short sw_num;

    for (sw_num = 0; sw_num < MAX_SINE_WAVE; sw_num++)
    {
        for (sw = &SineWall[sw_num][0]; sw->wall >= 0 && sw < &SineWall[sw_num][MAX_SINE_WALL_POINTS]; sw++)
        {
            // move through the sintable
            sw->sintable_ndx = NORM_ANGLE(sw->sintable_ndx + (synctics << sw->speed_shift));

            if (!sw->type)
            {
                New = sw->orig_xy + MulScale(sw->range, bsin(sw->sintable_ndx), 14);
                // wall[sw->wall].y = New;
                dragpoint(sw->wall, wall[sw->wall].x, New);
            }
            else
            {
                New = sw->orig_xy + MulScale(sw->range, bsin(sw->sintable_ndx), 14);
                // wall[sw->wall].x = New;
                dragpoint(sw->wall, New, wall[sw->wall].y);
            }
        }
    }
}

void DoAnim(int numtics)
{
    int i, animval;

    for (i = AnimCnt - 1; i >= 0; i--)
    {
        animval = Anim[i].Addr();

        // if LESS THAN goal
        if (animval < Anim[i].goal)
        {
            // move it
            animval += (numtics * PIXZ(Anim[i].vel));

            Anim[i].vel += Anim[i].vel_adj * numtics;

            // if the other way make it equal
            if (animval > Anim[i].goal)
                animval = Anim[i].goal;
        }

        // if GREATER THAN goal
        if (animval > Anim[i].goal)
        {
            animval -= (numtics * PIXZ(Anim[i].vel));

            Anim[i].vel += Anim[i].vel_adj * numtics;

            if (animval < Anim[i].goal)
                animval = Anim[i].goal;
        }

        Anim[i].Addr() =animval;

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

    //DSPRINTF(ds, "Deleted a Anim");
    MONO_PRINT(ds);

}


short AnimSet(int animtype, int animindex, DSWActor* animactor, fixed_t thegoal, int thevel)
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
    Anim[j].vel = Z(thevel);
    Anim[j].vel_adj = 0;
    Anim[j].callback = nullptr;
    Anim[j].callbackdata = nullptr;

    if (j == AnimCnt)
        AnimCnt++;

    return j;
}

short AnimSetCallback(short anim_ndx, ANIM_CALLBACKp call, SECTOR_OBJECTp data)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].callback = call;
    Anim[anim_ndx].callbackdata = data;

    return anim_ndx;
}

short AnimSetVelAdj(short anim_ndx, short vel_adj)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].vel_adj = vel_adj;

    return anim_ndx;
}



void DoPanning(void)
{
    int nx, ny;
    int i;
    SPRITEp sp;
    SECTORp sectp;
    WALLp wallp;


    SWStatIterator it(STAT_FLOOR_PAN);
    while (auto actor = it.Next())
    {
        sp = &actor->s();
        sectp = &sector[sp->sectnum];

        nx = MulScale(sp->xvel, bcos(sp->ang), 20);
        ny = MulScale(sp->xvel, bsin(sp->ang), 20);

        sectp->addfloorxpan((float)nx);
        sectp->addfloorypan((float)ny);
    }

    it.Reset(STAT_CEILING_PAN);
    while (auto actor = it.Next())
    {
        sp = &actor->s();
        sectp = &sector[sp->sectnum];

        nx = MulScale(sp->xvel, bcos(sp->ang), 20);
        ny = MulScale(sp->xvel, bsin(sp->ang), 20);

        sectp->addceilingxpan((float)nx);
        sectp->addceilingypan((float)ny);
    }

    it.Reset(STAT_WALL_PAN);
    while (auto actor = it.Next())
    {
        sp = &actor->s();
        wallp = &wall[sp->owner];

        nx = MulScale(sp->xvel, bcos(sp->ang), 20);
        ny = MulScale(sp->xvel, bsin(sp->ang), 20);

        wallp->addxpan((float)nx);
        wallp->addypan((float)ny);
    }
}


void DoSector(void)
{
    SECTOR_OBJECTp sop;
    bool riding;
    int sync_flag;
    short pnum;
    int min_dist,dist,a,b,c;
    PLAYERp pp;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {

        if (SO_EMPTY(sop))
            continue;


        riding = false;
        min_dist = 999999;

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
                DISTANCE(pp->posx, pp->posy, sop->xmid, sop->ymid, dist, a, b, c);
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
        if ((sync_flag = TEST(sop->flags, SOBJ_SYNC1|SOBJ_SYNC2)) != 0)
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
            if (min_dist < 15000)
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
