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
#include "build.h"
#include "common.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "player.h"
#include "quake.h"
#include "weapon.h"
#include "jtags.h"

#include "net.h"

#include "break.h"
#include "track.h"
#include "sprite.h"
#include "common_game.h"
#include "light.h"
#include "text.h"

#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32
#define DEFAULT_DOOR_SPEED 800

short FindNextSectorByTag(short sectnum, int tag);
short LevelSecrets;
SWBOOL TestVatorMatchActive(short match);
SWBOOL TestSpikeMatchActive(short match);
SWBOOL TestRotatorMatchActive(short match);
SWBOOL TestSlidorMatchActive(short match);
int PlayerCheckDeath(PLAYERp, short);
short DoVatorOperate(PLAYERp, short);
short DoVatorMatch(PLAYERp pp, short match);
short DoRotatorOperate(PLAYERp, short);
short DoRotatorMatch(PLAYERp pp, short match, SWBOOL);
short DoSlidorOperate(PLAYERp, short);
short DoSlidorMatch(PLAYERp pp, short match, SWBOOL);

void KillMatchingCrackSprites(short match);
int DoTrapReset(short match);
int DoTrapMatch(short match);

PLAYERp GlobPlayerP;
#if 0
char lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2)], lavainc[LAVASIZ];
int lavanumdrops, lavanumframes;
int lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
int lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
int lavaradx[32][128], lavarady[32][128], lavaradcnt[32];
#endif

SECT_USERp SectUser[MAXSECTORS];
USERp User[MAXSPRITES];

ANIM Anim[MAXANIM];
short AnimCnt = 0;

SINE_WAVE_FLOOR SineWaveFloor[MAX_SINE_WAVE][21];
SINE_WALL SineWall[MAX_SINE_WALL][MAX_SINE_WALL_POINTS];
SPRING_BOARD SpringBoard[20];
int x_min_bound, y_min_bound, x_max_bound, y_max_bound;

void SetSectorWallBits(short sectnum, int bit_mask, SWBOOL set_sectwall, SWBOOL set_nextwall)
{
    short wall_num, start_wall;

    wall_num = start_wall = sector[sectnum].wallptr;

    do
    {
        if (set_sectwall)
            SET(wall[wall_num].extra, bit_mask);

        if (set_nextwall && wall[wall_num].nextwall >= 0)
            SET(wall[wall[wall_num].nextwall].extra, bit_mask);

        wall_num = wall[wall_num].point2;
    }
    while (wall_num != start_wall);

}

void WallSetupDontMove(void)
{
    int i,j,nexti,nextj;
    SPRITEp spu, spl;
    WALLp wallp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_DONT_MOVE_UPPER],i,nexti)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_DONT_MOVE_LOWER],j,nextj)
        {
            spu = &sprite[i];
            spl = &sprite[j];

            if (spu->lotag == spl->lotag)
            {
                for (wallp = wall; wallp < &wall[numwalls]; wallp++)
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

void
WallSetup(void)
{
    short i = 0;
    short NextSineWall = 0;
    WALLp wp;

    WallSetupDontMove();

    memset(SineWall, -1, sizeof(SineWall));

    x_min_bound = 999999;
    y_min_bound = 999999;
    x_max_bound = -999999;
    y_max_bound = -999999;


    for (wp = wall, i = 0; wp < &wall[numwalls]; i++, wp++)
    {
        if (wp->picnum == FAF_PLACE_MIRROR_PIC)
            wp->picnum = FAF_MIRROR_PIC;

        if (wall[i].picnum == FAF_PLACE_MIRROR_PIC+1)
            wall[i].picnum = FAF_MIRROR_PIC+1;

        // get map min and max coordinates
        x_min_bound = min(wp->x, x_min_bound);
        y_min_bound = min(wp->y, y_min_bound);
        x_max_bound = max(wp->x, x_max_bound);
        y_max_bound = max(wp->y, y_max_bound);

        // this overwrites the lotag so it needs to be called LAST - its down there
        // SetupWallForBreak(wp);

        switch (wp->lotag)
        {
        case TAG_WALL_LOOP_DONT_SPIN:
        {
            short wall_num, start_wall;

            // set first wall
            SET(wp->extra, WALLFX_LOOP_DONT_SPIN);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_DONT_SPIN);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_DONT_SPIN;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_DONT_SPIN);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_DONT_SPIN);
            }

            break;
        }

        case TAG_WALL_LOOP_DONT_SCALE:
        {
            short wall_num, start_wall;

            // set first wall
            SET(wp->extra, WALLFX_DONT_SCALE);
            SET(wall[wp->nextwall].extra, WALLFX_DONT_SCALE);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_DONT_SCALE;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_DONT_SCALE);
                if (wall[wall_num].nextwall >= 0)
                    SET(wall[wall[wall_num].nextwall].extra, WALLFX_DONT_SCALE);
            }

            wp->lotag = 0;

            break;
        }

        case TAG_WALL_LOOP_OUTER_SECONDARY:
        {
            short wall_num, start_wall;

            // make sure its a red wall
            ASSERT(wp->nextwall >= 0);

            // set first wall
            SET(wp->extra, WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_OUTER_SECONDARY;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_OUTER|WALLFX_LOOP_OUTER_SECONDARY);
            }

            break;
        }

        case TAG_WALL_LOOP_OUTER:
        {
            short wall_num, start_wall;

            // make sure its a red wall
            ASSERT(wp->nextwall >= 0);

            // set first wall
            SET(wp->extra, WALLFX_LOOP_OUTER);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_OUTER);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_OUTER;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_OUTER);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_OUTER);
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
            short wall_num, start_wall;

            // set first wall
            SET(wp->extra, WALLFX_LOOP_SPIN_2X);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_SPIN_2X);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_SPIN_2X;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_SPIN_2X);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_SPIN_2X);
            }

            break;
        }

        case TAG_WALL_LOOP_SPIN_4X:
        {
            short wall_num, start_wall;

            // set first wall
            SET(wp->extra, WALLFX_LOOP_SPIN_4X);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_SPIN_4X);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_SPIN_4X;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_SPIN_4X);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_SPIN_4X);
            }

            break;
        }

        case TAG_WALL_LOOP_REVERSE_SPIN:
        {
            short wall_num, start_wall;

            // set first wall
            SET(wp->extra, WALLFX_LOOP_REVERSE_SPIN);
            SET(wall[wp->nextwall].extra, WALLFX_LOOP_REVERSE_SPIN);

            // move the the next wall
            start_wall = wp->point2;

            // Travel all the way around loop setting wall bits
            for (wall_num = start_wall;
                 wall[wall_num].lotag != TAG_WALL_LOOP_REVERSE_SPIN;
                 wall_num = wall[wall_num].point2)
            {
                SET(wall[wall_num].extra, WALLFX_LOOP_REVERSE_SPIN);
                SET(wall[wall[wall_num].nextwall].extra, WALLFX_LOOP_REVERSE_SPIN);
            }

            break;
        }

        case TAG_WALL_SINE_Y_BEGIN:
        case TAG_WALL_SINE_X_BEGIN:
        {
            short wall_num, cnt, last_wall, num_points, type, tag_end;
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


void
SectorLiquidSet(short i)
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

void
SectorSetup(void)
{
    short i = 0, k, tag;
    short NextSineWave = 0, rotcnt = 0, swingcnt = 0;

    short startwall, endwall, j, ndx, door_sector;

    WallSetup();

    for (ndx = 0; ndx < MAX_SECTOR_OBJECTS; ndx++)
    {
        memset(&SectorObject[ndx], -1, sizeof(SectorObject[0]));
        // 0 pointers
        //memset(&SectorObject[ndx].sectp, NULL, sizeof(SectorObject[0].sectp));
        SectorObject[ndx].PreMoveAnimator = NULL;
        SectorObject[ndx].PostMoveAnimator = NULL;
        SectorObject[ndx].Animator = NULL;
        SectorObject[ndx].controller = NULL;
        SectorObject[ndx].sp_child = NULL;
        SectorObject[ndx].xmid = MAXLONG;
    }

    memset(SineWaveFloor, -1, sizeof(SineWaveFloor));
    memset(SpringBoard, -1, sizeof(SpringBoard));

    LevelSecrets = 0;

    //for (i = 0; i < MAX_SW_PLAYERS; i++)
    //    memset((Player + i)->HasKey, 0, sizeof((Player + i)->HasKey));

    for (i = 0; i < numsectors; i++)
    {
        tag = LOW_TAG(i);

// DOH! NOT AGAIN! :(
//ASSERT(wall[4568].lotag != 307);

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
            SetSectorWallBits(i, WALLFX_DONT_STICK, TRUE, TRUE);
            break;

        case TAG_SINE_WAVE_FLOOR:
        case TAG_SINE_WAVE_CEILING:
        case TAG_SINE_WAVE_BOTH:
        {
            SINE_WAVE_FLOOR *swf;
            short near_sect = i, base_sect = i;
            short swf_ndx = 0;
            short cnt = 0, sector_cnt;
            int range;
            int range_diff = 0;
            int wave_diff = 0;
            short peak_dist = 0;
            short speed_shift = 3;
            short num;

#define SINE_FLOOR (1<<0)
#define SINE_CEILING (1<<1)

            num = (tag - TAG_SINE_WAVE_FLOOR) / 20;

            // set the first on up
            swf = &SineWaveFloor[NextSineWave][swf_ndx];

            swf->flags = 0;

            switch (num)
            {
            case 0:
                SET(swf->flags, SINE_FLOOR);
#define SINE_SLOPED BIT(3)
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
            for (swf_ndx = 1, sector_cnt = 1; TRUE; swf_ndx++)
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

void
SectorMidPoint(short sectnum, int *xmid, int *ymid, int *zmid)
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


void
DoSpringBoard(PLAYERp pp, short sectnum)
{
    int sb;
    int i;
    void DoPlayerBeginForceJump(PLAYERp);

#if 0
    i = AnimGetGoal(&sector[sectnum].floorz);

    // if in motion return
    if (i >= 0)
        return;

    AnimSet(&sector[sectnum].floorz, sector[sectnum].floorz - Z(32), 512);

    for (sb = 0; sb < SIZ(SpringBoard); sb++)
    {
        // if empty set up an entry to close the sb later
        if (SpringBoard[sb].Sector == -1)
        {
            pp->jump_speed = -sector[pp->cursectnum].hitag;
            DoPlayerBeginForceJump(pp);

            SpringBoard[sb].Sector = sectnum;
            SpringBoard[sb].TimeOut = 1 * 120;

            sector[sectnum].lotag = 0;
        }
    }
#else
    pp->jump_speed = -sector[pp->cursectnum].hitag;
    DoPlayerBeginForceJump(pp);
#endif

    return;
}


void
DoSpringBoardDown(void)
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

                AnimSet(&sector[sbp->Sector].floorz, destz, 256);

                sector[sbp->Sector].lotag = TAG_SPRING_BOARD;

                sbp->Sector = -1;
            }
        }
    }


    return;
}

short
FindSectorByTag(int x, int y, int tag)
{
    short i = 0, near_sector = -1;
    int diff, near_diff = 9999999;
    short wallnum;

    for (i = 0; i < numsectors; i++)
    {
        if (LOW_TAG(i) == tag)
        {
            // get the delta of the door/elevator
            wallnum = sector[i].wallptr;

            // diff = labs(wall[wallnum].x - x) + labs(wall[wallnum].y - y);
            diff = Distance(wall[wallnum].x, wall[wallnum].y, x, y);

            // if the door/elevator is closer than the last save it off
            if (diff < near_diff)
            {
                near_diff = diff;
                near_sector = i;
            }
        }
    }

    return near_sector;

}

short
FindSectorByTag_Wall(short wallnum, int tag)
{
    return FindSectorByTag(wall[wallnum].x, wall[wallnum].y, tag);
}

short
FindSectorByTag_Sprite(short SpriteNum, int tag)
{
    return FindSectorByTag(sprite[SpriteNum].x, sprite[SpriteNum].y, tag);
}

#if 1
short
FindSectorMidByTag(short sectnum, int tag)
{
    short i = 0, near_sector = -1;
    int diff, near_diff = 9999999, x, y;
    int trash, fx, fy;

    // Get the mid x,y of the sector
    SectorMidPoint(sectnum, &x, &y, &trash);

    for (i = 0; i < numsectors; i++)
    {
        if (sector[i].lotag == tag)
        {
            // get the delta of the door/elevator
            SectorMidPoint(i, &fx, &fy, &trash);

            // diff = labs(wall[wallnum].x - x) + labs(wall[wallnum].y - y);
            diff = Distance(fx, fy, x, y);

            // if the door/elevator is closer than the last save it off
            if (diff < near_diff)
            {
                near_diff = diff;
                near_sector = i;
            }
        }
    }

    return near_sector;

}

#endif

short
FindNextSectorByTag(short sectnum, int tag)
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


int
SectorDistance(short sect1, int sect2)
{
    short wallnum1, wallnum2;

    if (sect1 < 0 || sect2 < 0)
        return 9999999;

    wallnum1 = sector[sect1].wallptr;
    wallnum2 = sector[sect2].wallptr;

    // return the distance between the two sectors.
    return Distance(wall[wallnum1].x, wall[wallnum1].y, wall[wallnum2].x, wall[wallnum2].y);
}


int
SectorDistanceByMid(short sect1, int sect2)
{
    int sx1, sy1, sx2, sy2, trash;

    SectorMidPoint(sect1, &sx1, &sy1, &trash);
    SectorMidPoint(sect2, &sx2, &sy2, &trash);

    // return the distance between the two sectors.
    return Distance(sx1, sy1, sx2, sy2);
}

short
DoSpawnActorTrigger(short match)
{
    int i, nexti, pnum;
    short spawn_count = 0, hidden;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPAWN_TRIGGER], i, nexti)
    {
        sp = &sprite[i];

        if (sp->hitag == match)
        {
            if (ActorSpawn(sp))
            {
                DoSpawnTeleporterEffectPlace(sp);
                PlaySound(DIGI_PLAYER_TELEPORT, &sp->x, &sp->y, &sp->z, v3df_none);
                spawn_count++;
            }
        }
    }

    return spawn_count;
}

int
OperateSector(short sectnum, short player_is_operating)
{
    PLAYERp pp = GlobPlayerP;

    // Don't let actors operate locked or secret doors
    if (!player_is_operating)
    {
        SPRITEp fsp;
        short match;
        short i,nexti;


        if (SectUser[sectnum] && SectUser[sectnum]->stag == SECT_LOCK_DOOR)
            return FALSE;

        TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
        {
            fsp = &sprite[i];

            if (SectUser[fsp->sectnum] && SectUser[fsp->sectnum]->stag == SECT_LOCK_DOOR)
                return FALSE;

            if (fsp->statnum == STAT_VATOR && SP_TAG1(fsp) == SECT_VATOR && TEST_BOOL7(fsp))
                return FALSE;
            if (fsp->statnum == STAT_ROTATOR && SP_TAG1(fsp) == SECT_ROTATOR && TEST_BOOL7(fsp))
                return FALSE;
            if (fsp->statnum == STAT_SLIDOR && SP_TAG1(fsp) == SECT_SLIDOR && TEST_BOOL7(fsp))
                return FALSE;

        }
    }

    //CON_Message("Operating sectnum %d",sectnum);
    //CON_Message("stag = %d, lock = %d",SectUser[sectnum]->stag,SECT_LOCK_DOOR);

    switch (LOW_TAG(sectnum))
    {

    case TAG_VATOR:
        DoVatorOperate(pp, sectnum);
        return TRUE;

    case TAG_ROTATOR:
        DoRotatorOperate(pp, sectnum);
        return TRUE;

    case TAG_SLIDOR:
        DoSlidorOperate(pp, sectnum);
        return TRUE;
    }

    return FALSE;
}

int
OperateWall(short wallnum, short player_is_operating)
{
    WALLp wallp = &wall[wallnum];

    switch (LOW_TAG_WALL(wallnum))
    {
    }

    return FALSE;
}

short
AnimateSwitch(SPRITEp sp, short tgt_value)
{
#define SWITCH_LEVER        581
#define SWITCH_FUSE         558
#define SWITCH_FLIP         561
#define SWITCH_RED_CHAIN    563
#define SWITCH_GREEN_CHAIN  565
#define SWITCH_TOUCH        567
#define SWITCH_DRAGON       569

#define SWITCH_LIGHT        551
#define SWITCH_1            575
#define SWITCH_3            579

#define SWITCH_SHOOTABLE_1  577
#define SWITCH_4            571
#define SWITCH_5            573
#define SWITCH_6            583
#define EXIT_SWITCH         2470

#define SWITCH_SKULL        553

    // if the value is not ON or OFF
    // then it is a straight toggle

    switch (sp->picnum)
    {
    // set to TRUE/ON
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
            return OFF;

        sp->picnum += 1;

        // if the tgt_value should be true
        // flip it again - recursive but only once
        if (tgt_value == OFF)
        {
            AnimateSwitch(sp, tgt_value);
            return OFF;
        }

        return ON;

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
            return ON;

        sp->picnum -= 1;

        if (tgt_value == ON)
        {
            AnimateSwitch(sp, tgt_value);
            return ON;
        }

        return OFF;
    }
    return OFF;
}


void
SectorExp(short SpriteNum, short sectnum, short orig_ang, int zh)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    SECT_USERp sectu = SectUser[sectnum];
    SECTORp sectp = &sector[sectnum];
    short explosion;
    SPRITEp exp;
    USERp eu;
    int x,y,z;

    RESET(sp->cstat, CSTAT_SPRITE_WALL|CSTAT_SPRITE_FLOOR);
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
    changespritesect(SpriteNum, sectnum);
    //setspritez(SpriteNum, (vec3_t *)sp);
    getzsofslope(sp->sectnum, sp->x, sp->y, &u->hiz, &u->loz);

    // spawn explosion
    explosion = SpawnSectorExp(SpriteNum);
    ASSERT(explosion >= 0);
    exp = &sprite[explosion];
    eu = User[explosion];

    exp->xrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    exp->yrepeat += (RANDOM_P2(32<<8)>>8) - 16;
    eu->xchange = MOVEx(92, exp->ang);
    eu->ychange = MOVEy(92, exp->ang);
}


void
DoExplodeSector(short match)
{
    short orig_ang;
    int zh;
    USERp u;
    short cf,nextcf;
    short ed,nexted;

    int ss, nextss;
    SECTORp dsectp, ssectp;
    SPRITEp src_sp, dest_sp;

    SPRITEp esp;
    SECTORp sectp;

    orig_ang = 0; //sp->ang;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_EXPLODING_CEIL_FLOOR], cf, nextcf)
    {
        esp = &sprite[cf];

        if (match != esp->lotag)
            continue;

        if (!User[cf])
            u = SpawnUser(cf, 0, NULL);

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
            SectorExp(cf, esp->sectnum, orig_ang, zh + Z(RANDOM_P2(64)) - Z(32));
        }

        // don't need it any more
        KillSprite(cf);
    }
}


int DoSpawnSpot(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    if ((u->WaitTics -= synctics) < 0)
    {
        change_sprite_stat(SpriteNum, STAT_SPAWN_SPOT);
        SpawnShrap(SpriteNum, -1);

        if (u->LastDamage == 1)
        {
            KillSprite(SpriteNum);
            return 0;
        }
    }

    return 0;
}

// spawns shrap when killing an object
void
DoSpawnSpotsForKill(short match)
{
    short sn, next_sn;
    SPRITEp sp;
    USERp u;

    if (match < 0)
        return;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPAWN_SPOT], sn, next_sn)
    {
        sp = &sprite[sn];

        // change the stat num and set the delay correctly to call SpawnShrap
        if (sp->hitag == SPAWN_SPOT && sp->lotag == match)
        {
            u = User[sn];
            change_sprite_stat(sn, STAT_NO_STATE);
            u->ActorActionFunc = DoSpawnSpot;
            u->WaitTics = SP_TAG5(sp) * 15;
            setspritez(sn, (vec3_t *)sp);
            // setting for Killed
            u->LastDamage = 1;
        }
    }
}

// spawns shrap when damaging an object
void
DoSpawnSpotsForDamage(short match)
{
    short sn, next_sn;
    SPRITEp sp;
    USERp u;

    if (match < 0)
        return;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPAWN_SPOT], sn, next_sn)
    {
        sp = &sprite[sn];

        // change the stat num and set the delay correctly to call SpawnShrap

        if (sp->hitag == SPAWN_SPOT && sp->lotag == match)
        {
            u = User[sn];
            change_sprite_stat(sn, STAT_NO_STATE);
            u->ActorActionFunc = DoSpawnSpot;
            u->WaitTics = SP_TAG7(sp) * 15;
            // setting for Damaged
            u->LastDamage = 0;
        }
    }
}

void
DoSoundSpotMatch(short match, short sound_num, short sound_type)
{
    short sn, next_sn;
    SPRITEp sp;
    int flags;
    short snd2play;

    //sound_type is not used

    sound_num--;

    ASSERT(sound_num >= 0);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SOUND_SPOT], sn, next_sn)
    {
        sp = &sprite[sn];

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
            //flags = v3df_follow|v3df_ambient|v3df_nolookup;

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
                    snd2play = snd[RANDOM_RANGE(2)];
                }
                else if (snd[0] && snd[1] && snd[2])
                {
                    snd2play = snd[RANDOM_RANGE(3)];
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

                ////DSPRINTF(ds,"PlayerSound %d",pp-Player);
                //MONO_PRINT(ds);
                //ASSERT(pp >= Player && pp <= Player+numplayers);

                if (pp)
                {
                    if (pp == Player+myconnectindex)
                        PlayerSound(snd2play,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
                }
            }
            else
            {
                PlaySound(snd2play, &sp->x, &sp->y, &sp->z, flags);

                //if (TEST(flags, v3df_follow)) // Just set it anyway
                Set3DSoundOwner(sn);
            }
        }
    }
}

void
DoSoundSpotStopSound(short match)
{
    short sn, next_sn;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SOUND_SPOT], sn, next_sn)
    {
        sp = &sprite[sn];

        // found match and is a follow type
        if (SP_TAG2(sp) == match && TEST_BOOL2(sp))
        {
            DeleteNoSoundOwner(sn);
        }
    }
}

void
DoStopSoundSpotMatch(short match)
{
    short sn, next_sn;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_STOP_SOUND_SPOT], sn, next_sn)
    {
        sp = &sprite[sn];

        if (SP_TAG2(sp) == match)
        {
            DoSoundSpotStopSound(SP_TAG5(sp));
        }
    }
}


SWBOOL TestKillSectorObject(SECTOR_OBJECTp sop)
{
    if (TEST(sop->flags, SOBJ_KILLABLE))
    {
        KillMatchingCrackSprites(sop->match_event);
        // get new sectnums
        CollapseSectorObject(sop, sop->xmid, sop->ymid);
        DoSpawnSpotsForKill(sop->match_event);
        KillSectorObjectSprites(sop);
        return TRUE;
    }

    return FALSE;
}

short
DoSectorObjectKillMatch(short match)
{
    SECTOR_OBJECTp sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (sop->xmid == MAXLONG)
            continue;

        if (sop->match_event == match)
            return TestKillSectorObject(sop);
    }

    return FALSE;
}


SWBOOL
SearchExplodeSectorMatch(short match)
{
    short i,nexti;

    // THIS IS ONLY CALLED FROM DoMatchEverything
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPRITE_HIT_MATCH], i, nexti)
    {
        SPRITEp sp = &sprite[i];

        if (sp->hitag == match)
        {
            KillMatchingCrackSprites(match);
            DoExplodeSector(match);
            return TRUE;
        }
    }

    return FALSE;
}

void
KillMatchingCrackSprites(short match)
{
    short i,nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPRITE_HIT_MATCH], i, nexti)
    {
        sp = &sprite[i];

        if (sp->hitag == match)
        {
            if (TEST(SP_TAG8(sp), BIT(2)))
                continue;

            KillSprite(i);
        }
    }
}

void
WeaponExplodeSectorInRange(short weapon)
{
    short i, nexti;
    SPRITEp wp = &sprite[weapon];
    USERp wu = User[weapon];
    SPRITEp sp;
    int dist;
    int radius;
    short match;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPRITE_HIT_MATCH], i, nexti)
    {
        sp = &sprite[i];

        // test to see if explosion is close to crack sprite
        dist = FindDistance3D(wp->x - sp->x, wp->y - sp->y, (wp->z - sp->z)>>4);

        if (sp->clipdist == 0)
            continue;

        radius = (((int)sp->clipdist) << 2) * 8;

        if ((unsigned int)dist > (wu->Radius/2) + radius)
            continue;

        if (!FAFcansee(wp->x,wp->y,wp->z,wp->sectnum,sp->x,sp->y,sp->z,sp->sectnum))
            continue;

        match = sp->hitag;
        // this and every other crack sprite of this type is now dead
        // don't use them
#if 0
        KillMatchingCrackSprites(match);
        DoExplodeSector(match);
        DoMatchEverything(NULL, match, -1);
#else
        // pass in explosion type
        MissileHitMatch(weapon, WPN_ROCKET, i);
#endif

        return;
    }
}


void
ShootableSwitch(short SpriteNum, short Weapon)
{
    SPRITEp sp = &sprite[SpriteNum];

    switch (sp->picnum)
    {
    case SWITCH_SHOOTABLE_1:
        //RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(SpriteNum, FALSE);
        sp->picnum = SWITCH_SHOOTABLE_1 + 1;
        break;
    case SWITCH_FUSE:
    case SWITCH_FUSE + 1:
        RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        OperateSprite(SpriteNum, FALSE);
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
    short i,nexti;
    unsigned stat;
    short found;

    while (TRUE)
    {
        found = -1;

        // search for a DELETE_SPRITE with same match tag
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_DELETE_SPRITE], i, nexti)
        {
            if (sprite[i].lotag == match)
            {
                found = i;
                del_x = sprite[i].x;
                del_y = sprite[i].y;
                break;
            }
        }

        if (found == -1)
            return;

        for (stat = 0; stat < SIZ(StatList); stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[StatList[stat]], i, nexti)
            {
                if (del_x == sprite[i].x && del_y == sprite[i].y)
                {
                    // special case lighting delete of Fade On/off after fades
                    if (StatList[stat] == STAT_LIGHTING)
                    {
                        // set shade to darkest and then kill it
                        sprite[i].shade = SPRITE_TAG6(i);
                        sprite[i].pal = 0;
                        SectorLightShade(&sprite[i], sprite[i].shade);
                        DiffuseLighting(&sprite[i]);
                    }

                    ////DSPRINTF(ds,"Delete Sprite stat %d, x %d, y %d",sprite[i].statnum, sprite[i].x, sprite[i].y);
                    //MONO_PRINT(ds);
                    SpriteQueueDelete(i);
                    KillSprite(i);
                }
            }
        }

        // kill the DELETE_SPRITE
        KillSprite(found);
    }
}

void
DoChangorMatch(short match)
{
    short sn, next_sn;
    SPRITEp sp;
    SECTORp sectp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CHANGOR], sn, next_sn)
    {
        sp = &sprite[sn];
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
            KillSprite(sn);
        }
    }
}

void DoMatchEverything(PLAYERp pp, short match, short state)
{
    PLAYERp bak;

    bak = GlobPlayerP;
    GlobPlayerP = pp;
    // CAREFUL! pp == NULL is a valid case for this routine
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
        DoSpikeMatch(pp, match);

    if (!TestRotatorMatchActive(match))
        DoRotatorMatch(pp, match, FALSE);

    if (!TestSlidorMatchActive(match))
        DoSlidorMatch(pp, match, FALSE);

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

SWBOOL ComboSwitchTest(short combo_type, short match)
{
    short i,nexti;
    SPRITEp sp;
    SWBOOL state;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_DEFAULT], i, nexti)
    {
        sp = &sprite[i];

        if (sp->lotag == combo_type && sp->hitag == match)
        {
            // dont toggle - get the current state
            state = AnimateSwitch(sp, 999);

            // if any one is not set correctly then switch is not set
            if (state != SP_TAG3(sp))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// NOTE: switches are always wall sprites
int
OperateSprite(short SpriteNum, short player_is_operating)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    PLAYERp pp = NULL;
    short state;
    short key_num=0;
    extern STATE s_Pachinko1Operate[];
    extern STATE s_Pachinko2Operate[];
    extern STATE s_Pachinko3Operate[];
    extern STATE s_Pachinko4Operate[];

    if (Prediction)
        return FALSE;

    if (sp->picnum == ST1)
        return FALSE;

    if (player_is_operating)
    {
        pp = GlobPlayerP;

        if (!FAFcansee(pp->posx, pp->posy, pp->posz, pp->cursectnum, sp->x, sp->y, sp->z - DIV2(SPRITEp_SIZE_Z(sp)), sp->sectnum))
            return FALSE;
    }

    switch (sp->lotag)
    {
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
        //if(RANDOM_RANGE(1000) < 500) return(TRUE);
        //if(u->FlagOwner == 0)
    {
        short choose_snd;

        u->FlagOwner = 1;
        u->WaitTics = SEC(4);

        if (pp != Player+myconnectindex) return TRUE;

        choose_snd = STD_RANDOM_RANGE(1000);
        if (sp->lotag == CARGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG44052,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45014,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG44068,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG45010,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
        else if (sp->lotag == MECHANICGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG44027,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG44038,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG44039,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG44048,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
        else if (sp->lotag == SAILORGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG45018,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45030,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG45033,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG45043,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
        else if (sp->lotag == PRUNEGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_JG45053,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_JG45067,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_JG46005,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_JG46010,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
        else if (sp->lotag == TOILETGIRL_R0)
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_WHATYOUEATBABY,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_WHATDIEDUPTHERE,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_YOUGOPOOPOO,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_PULLMYFINGER,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
        else
        {
            if (choose_snd > 700)
                PlayerSound(DIGI_SOAPYOUGOOD,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 500)
                PlayerSound(DIGI_WASHWANG,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else if (choose_snd > 250)
                PlayerSound(DIGI_DROPSOAP,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
            else
                PlayerSound(DIGI_REALTITS,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_follow,pp);
        }
    }
        return TRUE;

    case PACHINKO1:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return TRUE;

        PlaySound(DIGI_PFLIP,&sp->x,&sp->y,&sp->z,v3df_none);
        u->WaitTics = SEC(3) + SEC(RANDOM_RANGE(10));
        ChangeState(SpriteNum,s_Pachinko1Operate);

        return TRUE;

    case PACHINKO2:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return TRUE;

        PlaySound(DIGI_PFLIP,&sp->x,&sp->y,&sp->z,v3df_none);
        u->WaitTics = SEC(3) + SEC(RANDOM_RANGE(10));
        ChangeState(SpriteNum,s_Pachinko2Operate);

        return TRUE;

    case PACHINKO3:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return TRUE;

        PlaySound(DIGI_PFLIP,&sp->x,&sp->y,&sp->z,v3df_none);
        u->WaitTics = SEC(3) + SEC(RANDOM_RANGE(10));
        ChangeState(SpriteNum,s_Pachinko3Operate);

        return TRUE;

    case PACHINKO4:

        // Don't mess with it if it's already going
        if (u->WaitTics > 0) return TRUE;

        PlaySound(DIGI_PFLIP,&sp->x,&sp->y,&sp->z,v3df_none);
        u->WaitTics = SEC(3) + SEC(RANDOM_RANGE(10));
        ChangeState(SpriteNum,s_Pachinko4Operate);

        return TRUE;

    case SWITCH_LOCKED:
        key_num = sp->hitag;
        if (pp->HasKey[key_num - 1])
        {
            int i;
            for (i=0; i<numsectors; i++)
            {
                if (SectUser[i] && SectUser[i]->stag == SECT_LOCK_DOOR && SectUser[i]->number == key_num)
                    SectUser[i]->number = 0;  // unlock all doors of this type
            }
            UnlockKeyLock(key_num, SpriteNum);
        }

        return TRUE;

    case TAG_COMBO_SWITCH_EVERYTHING:

        // change the switch state
        AnimateSwitch(sp, -1);
        PlaySound(DIGI_REGULARSWITCH, &sp->x, &sp->y, &sp->z, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, sp->hitag))
        {
            DoMatchEverything(pp, sp->hitag, ON);
        }

        return TRUE;

    case TAG_COMBO_SWITCH_EVERYTHING_ONCE:

        // change the switch state
        AnimateSwitch(sp, -1);
        PlaySound(DIGI_REGULARSWITCH, &sp->x, &sp->y, &sp->z, v3df_none);

        if (ComboSwitchTest(TAG_COMBO_SWITCH_EVERYTHING, sp->hitag))
        {
            DoMatchEverything(pp, sp->hitag, ON);
        }

        sp->lotag = 0;
        sp->hitag = 0;
        return TRUE;

    case TAG_SWITCH_EVERYTHING:
        state = AnimateSwitch(sp, -1);
        DoMatchEverything(pp, sp->hitag, state);
        return TRUE;

    case TAG_SWITCH_EVERYTHING_ONCE:
        state = AnimateSwitch(sp, -1);
        DoMatchEverything(pp, sp->hitag, state);
        sp->lotag = 0;
        sp->hitag = 0;
        return TRUE;

    case TAG_LIGHT_SWITCH:

        state = AnimateSwitch(sp, -1);
        DoLightingMatch(sp->hitag, state);
        return TRUE;

    case TAG_SPRITE_SWITCH_VATOR:
    {
        // make sure all vators are inactive before allowing
        // to repress switch
        if (!TestVatorMatchActive(sp->hitag))
            DoVatorMatch(pp, sp->hitag);

        if (!TestSpikeMatchActive(sp->hitag))
            DoSpikeMatch(pp, sp->hitag);

        if (!TestRotatorMatchActive(sp->hitag))
            DoRotatorMatch(pp, sp->hitag, FALSE);

        if (!TestSlidorMatchActive(sp->hitag))
            DoSlidorMatch(pp, sp->hitag, FALSE);

        return TRUE;
    }

    case TAG_LEVEL_EXIT_SWITCH:
    {
        extern short Level;
        extern SWBOOL QuitFlag, ExitLevel, FinishedLevel;

        AnimateSwitch(sp, -1);

        PlaySound(DIGI_BIGSWITCH, &sp->x, &sp->y, &sp->z, v3df_none);

        if (sp->hitag)
            Level = sp->hitag;
        else
            Level++;
        ExitLevel = TRUE;
        FinishedLevel = TRUE;

        return TRUE;
    }

    case TAG_SPRITE_GRATING:
    {
        USERp u;
        ANIMATOR DoGrating;

        change_sprite_stat(SpriteNum, STAT_NO_STATE);

        u = SpawnUser(SpriteNum, 0, NULL);

        u->ActorActionFunc = DoGrating;

        sp->lotag = 0;
        sp->hitag /= 2;

        return TRUE;
    }

    case TAG_SO_SCALE_SWITCH:
        AnimateSwitch(sp, -1);
        DoSectorObjectSetScale(sp->hitag);
        return TRUE;

    case TAG_SO_SCALE_ONCE_SWITCH:
        AnimateSwitch(sp, -1);
        DoSectorObjectSetScale(sp->hitag);
        sp->lotag = 0;
        sp->hitag = 0;
        return TRUE;

    case TAG_SO_EVENT_SWITCH:
    {
        state = AnimateSwitch(sp, -1);

        DoMatchEverything(NULL, sp->hitag, state);

        sp->hitag = 0;
        sp->lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, &sp->x, &sp->y, &sp->z, v3df_none);
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

        PlaySound(DIGI_BIGSWITCH, &sp->x, &sp->y, &sp->z, v3df_none);

        return TRUE;

        break;
    }
    }

    return FALSE;
}

int DoTrapReset(short match)
{
    short i, nexti;
    SPRITEp sp;
    USERp u;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRAP], i, nexti)
    {
        sp = &sprite[i];
        u = User[i];

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
    short i, nexti;
    SPRITEp sp;
    USERp u;
    int InitFireballTrap(short SpriteNum);

    // may need to be reset to fire immediately

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRAP], i, nexti)
    {
        sp = &sprite[i];
        u = User[i];

        if (sp->lotag != match)
            continue;

        // if correct type and matches
        if (sp->hitag == FIREBALL_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitFireballTrap(i);
            }
        }

        // if correct type and matches
        if (sp->hitag == BOLT_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitBoltTrap(i);
            }
        }

        // if correct type and matches
        if (sp->hitag == SPEAR_TRAP)
        {
            u->WaitTics -= synctics;

            if (u->WaitTics <= 0)
            {
                u->WaitTics = 1 * 120;
                InitSpearTrap(i);
            }
        }
    }
    return 0;
}


void
OperateTripTrigger(PLAYERp pp)
{
    SECTORp sectp = &sector[pp->cursectnum];

    if (Prediction)
        return;

#if 0
    // new method
    if (TEST(sectp->extra, SECTFX_TRIGGER))
    {
        SPRITEp sp;

        TRAVERSE_SPRITE_SECT(headspritesect[*sectp],i,nexti)
        {
            sp = &sprite[i];

            if (sp->statnum == STAT_TRIGGER && SP_TAG7(sp) == 0)
            {
                switch (SP_TAG3(sp))
                {
                case 1: // Secret Area

                    break;
                }
            }
        }
    }
#endif

    // old method
    switch (LOW_TAG(pp->cursectnum))
    {
    // same tag for sector as for switch
    case TAG_LEVEL_EXIT_SWITCH:
    {
        extern short Level;
        extern SWBOOL QuitFlag, ExitLevel, FinishedLevel;

        if (sectp->hitag)
            Level = sectp->hitag;
        else
            Level++;
        ExitLevel = TRUE;
        FinishedLevel = TRUE;
        break;
    }

    case TAG_SECRET_AREA_TRIGGER:
        if (pp == Player+myconnectindex)
            PlayerSound(DIGI_ANCIENTSECRET, &pp->posx, &pp->posy, &pp->posz,
                        v3df_dontpan|v3df_doppler|v3df_follow,pp);

        sprintf(ds,"You found a secret area!");
        PutStringInfo(pp, ds);
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
            DoSpikeMatch(pp, sectp->hitag);
        if (!TestRotatorMatchActive(sectp->hitag))
            DoRotatorMatch(pp, sectp->hitag, FALSE);
        if (!TestSlidorMatchActive(sectp->hitag))
            DoSlidorMatch(pp, sectp->hitag, FALSE);
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
        short i, nexti;
        SPRITEp sp;
        USERp u;

        dist = sectp->hitag;

        TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            if (TEST(u->Flags, SPR_WAIT_FOR_TRIGGER))
            {
                if (Distance(sp->x, sp->y, pp->posx, pp->posy) < dist)
                {
                    u->tgt_sp = pp->SpriteP;
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
        DoMatchEverything(NULL, sector[pp->cursectnum].hitag, -1);
        break;
    }

    case TAG_SPAWN_ACTOR_TRIGGER:
    {
        DoMatchEverything(NULL, sector[pp->cursectnum].hitag, -1);

        sector[pp->cursectnum].hitag = 0;
        sector[pp->cursectnum].lotag = 0;
        break;
    }

    case TAG_SO_EVENT_TRIGGER:
    {
        DoMatchEverything(NULL, sector[pp->cursectnum].hitag, -1);

        sector[pp->cursectnum].hitag = 0;
        sector[pp->cursectnum].lotag = 0;

        PlaySound(DIGI_REGULARSWITCH, &pp->posx, &pp->posy, &pp->posz, v3df_none);
        break;
    }
    }
}

void
OperateContinuousTrigger(PLAYERp pp)
{
    if (Prediction)
        return;

    switch (LOW_TAG(pp->cursectnum))
    {
    case TAG_TRIGGER_MISSILE_TRAP:
    {
#if 1
        DoTrapMatch(sector[pp->cursectnum].hitag);
#else
        short i, nexti;
        SPRITEp sp;
        USERp u;
        int InitFireballTrap(short SpriteNum);

        TRAVERSE_SPRITE_STAT(headspritestat[STAT_TRAP], i, nexti)
        {
            sp = &sprite[i];
            u = User[i];

            // if correct type and matches
            if (sp->hitag == FIREBALL_TRAP && sp->lotag == sector[pp->cursectnum].hitag)
            {
                u->WaitTics -= synctics;

                if (u->WaitTics <= 0)
                {
                    u->WaitTics = 1 * 120;
                    InitFireballTrap(i);
                }
            }

            // if correct type and matches
            if (sp->hitag == BOLT_TRAP && sp->lotag == sector[pp->cursectnum].hitag)
            {
                u->WaitTics -= synctics;

                if (u->WaitTics <= 0)
                {
                    u->WaitTics = 1 * 120;
                    InitBoltTrap(i);
                }
            }

            // if correct type and matches
            if (sp->hitag == SPEAR_TRAP && sp->lotag == sector[pp->cursectnum].hitag)
            {
                u->WaitTics -= synctics;

                if (u->WaitTics <= 0)
                {
                    u->WaitTics = 1 * 120;
                    InitSpearTrap(i);
                }
            }
        }
#endif

        break;
    }
    }
}


short PlayerTakeSectorDamage(PLAYERp pp)
{
    SECT_USERp sectu = SectUser[pp->cursectnum];
    USERp u = User[pp->PlayerSprite];

    // the calling routine must make sure sectu exists
    if ((u->DamageTics -= synctics) < 0)
    {
        u->DamageTics = DAMAGE_TIME;

        PlayerUpdateHealth(pp, -sectu->damage);
        PlayerCheckDeath(pp, -1);
    }
    return 0;
}

// Needed in order to see if Player should grunt if he can't find a wall to operate on
// If player is too far away, don't grunt
#define PLAYER_SOUNDEVENT_TAG 900
SWBOOL NearThings(PLAYERp pp)
{
    short sectnum;
    short rndnum;
    int daz;
    short neartagsect, neartagwall, neartagsprite;
    int neartaghitdist;


    // Check player's current sector for triggered sound
    if (sector[pp->cursectnum].hitag == PLAYER_SOUNDEVENT_TAG)
    {
        if (pp == Player+myconnectindex)
            PlayerSound(sector[pp->cursectnum].lotag,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        return FALSE;
    }

    neartag(pp->posx, pp->posy, pp->posz, pp->cursectnum, pp->pang,
            &neartagsect, &neartagwall, &neartagsprite,
            &neartaghitdist, 1024L, NTAG_SEARCH_LO_HI, NULL);


    // hit a sprite? Check to see if it has sound info in it!
    // This can work with any sprite!
    if (neartagsprite >= 0)
    {
        SPRITEp sp = &sprite[neartagsprite];

        // Go through list of cases
        if (sp->hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(sp->lotag,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        }
        return FALSE;   // Return false so he doesn't grunt
    }

    if (neartagwall >= 0)
    {
        // Check player's current sector for triggered sound
        if (wall[neartagwall].hitag == PLAYER_SOUNDEVENT_TAG)
        {
            if (pp == Player+myconnectindex)
                PlayerSound(wall[neartagwall].lotag,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
            return FALSE;   // We are playing a sound so don't return true
        }
        return TRUE;
    }
    // This only gets called if nothing else worked, check for nearness to a wall
    {
        hitdata_t hitinfo = { { 0, 0, 0 }, 0, 0, 0 };
        short dang = pp->pang;

        FAFhitscan(pp->posx, pp->posy, pp->posz - Z(30), pp->cursectnum,    // Start position
                   sintable[NORM_ANGLE(dang + 512)],  // X vector of 3D ang
                   sintable[NORM_ANGLE(dang)],        // Y vector of 3D ang
                   0,                                 // Z vector of 3D ang
                   &hitinfo, CLIPMASK_MISSILE);

        if (hitinfo.sect < 0)
            return FALSE;

        if (Distance(hitinfo.pos.x, hitinfo.pos.y, pp->posx, pp->posy) > 1500)
            return FALSE;

        // hit a sprite?
        if (hitinfo.sprite >= 0)
            return FALSE;

        if (neartagsect >= 0)
            return TRUE;

        if (hitinfo.wall >= 0)
        {
            WALLp wp;

            wp =  &wall[hitinfo.wall];

            // Near a plain old vanilla wall.  Can't do anything but grunt.
            if (!TEST(wp->extra, WALLFX_DONT_STICK) && pp == Player+myconnectindex)
            {
                if (STD_RANDOM_RANGE(1000) > 970)
                    PlayerSound(DIGI_HITTINGWALLS,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
                else
                    PlayerSound(DIGI_SEARCHWALL,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
            }

            return TRUE;
        }

        return FALSE;
    }

}




#if 0  // Move to sector.h file because .def files could not find declaration!
typedef struct
{
    int dist;
    short sectnum, wallnum, spritenum;
} NEAR_TAG_INFO, *NEAR_TAG_INFOp;
#endif
short nti_cnt;

void
NearTagList(NEAR_TAG_INFOp ntip, PLAYERp pp, int z, int dist, int type, int count)
{
    short save_lotag, save_hitag;
    short neartagsector, neartagwall, neartagsprite;
    int neartaghitdist;


    neartag(pp->posx, pp->posy, z, pp->cursectnum, pp->pang,
            &neartagsector, &neartagwall, &neartagsprite,
            &neartaghitdist, dist, type, NULL);

    if (neartagsector >= 0)
    {
        // save off values
        save_lotag = sector[neartagsector].lotag;
        save_hitag = sector[neartagsector].hitag;

        ntip->dist = neartaghitdist;
        ntip->sectnum = neartagsector;
        ntip->wallnum = -1;
        ntip->spritenum = -1;
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
        ntip->spritenum = -1;
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
        // save off values
        save_lotag = sprite[neartagsprite].lotag;
        save_hitag = sprite[neartagsprite].hitag;

        ntip->dist = neartaghitdist;
        ntip->sectnum = -1;
        ntip->wallnum = -1;
        ntip->spritenum = neartagsprite;
        nti_cnt++;
        ntip++;

        if (nti_cnt >= count)
            return;

        // remove them
        sprite[neartagsprite].lotag = 0;
        sprite[neartagsprite].hitag = 0;

        NearTagList(ntip, pp, z, dist, type, count);

        // reset off values
        sprite[neartagsprite].lotag = save_lotag;
        sprite[neartagsprite].hitag = save_hitag;
    }
    else
    {
        ntip->dist = -1;
        ntip->sectnum = -1;
        ntip->wallnum = -1;
        ntip->spritenum = -1;
        nti_cnt++;
        ntip++;

        return;
    }
}

void
BuildNearTagList(NEAR_TAG_INFOp ntip, int size, PLAYERp pp, int z, int dist, int type, int count)
{
    memset(ntip, -1, size);
    nti_cnt = 0;
    NearTagList(ntip, pp, z, dist, type, count);
}


int DoPlayerGrabStar(PLAYERp pp)
{
    SPRITEp sp = NULL;
    int i;
    extern short StarQueue[MAX_STAR_QUEUE];

    // MUST check exact z's of each star or it will never work
    for (i = 0; i < MAX_STAR_QUEUE; i++)
    {
        if (StarQueue[i] >= 0)
        {
            sp = &sprite[StarQueue[i]];

            if (FindDistance3D(sp->x - pp->posx, sp->y - pp->posy, (sp->z - pp->posz + Z(12))>>4) < 500)
            {
                break;
            }
        }
    }

    if (i < MAX_STAR_QUEUE)
    {
        // Pull a star out of wall and up your ammo
        PlayerUpdateAmmo(pp, WPN_STAR, 1);
        PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_none);
        KillSprite(StarQueue[i]);
        StarQueue[i] = -1;
        if (TEST(pp->WpnFlags, BIT(WPN_STAR)))
            return TRUE;
        SET(pp->WpnFlags, BIT(WPN_STAR));
        InitWeaponStar(pp);
        return TRUE;
    }

    return FALSE;
}



void
PlayerOperateEnv(PLAYERp pp)
{
    SECT_USERp sectu = SectUser[pp->cursectnum];
    SECTORp sectp = &sector[pp->cursectnum];
    SWBOOL found;

    if (Prediction)
        return;

    ////DSPRINTF(ds,"dist %d sectnum %d wallnum %d spritenum %d",nti[nt_ndx].dist, nti[nt_ndx].sectnum, nti[nt_ndx].wallnum, nti[nt_ndx].spritenum);
    //MONO_PRINT(ds);

    //
    // Switch & door activations
    //

    if (TEST_SYNC_KEY(pp, SK_OPERATE))
    {
        if (FLAG_KEY_PRESSED(pp, SK_OPERATE))
        {
            // if space bar pressed
            short nt_ndx;
            NEAR_TAG_INFO nti[16];

            if (DoPlayerGrabStar(pp))
            {
                FLAG_KEY_RELEASE(pp, SK_OPERATE);
            }
            else
            {
                NearThings(pp); // Check for player sound specified in a level sprite
            }

            BuildNearTagList(nti, sizeof(nti), pp, pp->posz, 2048L, NTAG_SEARCH_LO_HI, 8);

            found = FALSE;

            // try and find a sprite
            for (nt_ndx = 0; nti[nt_ndx].dist >= 0; nt_ndx++)
            {
                if (nti[nt_ndx].spritenum >= 0 && nti[nt_ndx].dist < 1024 + 768)
                {
                    if (OperateSprite(nti[nt_ndx].spritenum, TRUE))
                    {
                        FLAG_KEY_RELEASE(pp, SK_OPERATE);
                        found = TRUE;
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


                z[0] = pp->SpriteP->z - SPRITEp_SIZE_Z(pp->SpriteP) - Z(10);
                z[1] = pp->SpriteP->z;
                z[2] = DIV2(z[0] + z[1]);

                for (i = 0; i < SIZ(z); i++)
                {
                    BuildNearTagList(nti, sizeof(nti), pp, z[i], 1024 + 768L, NTAG_SEARCH_LO_HI, 8);

                    for (nt_ndx = 0; nti[nt_ndx].dist >= 0; nt_ndx++)
                    {
                        if (nti[nt_ndx].spritenum >= 0 && nti[nt_ndx].dist < 1024 + 768)
                        {
                            if (OperateSprite(nti[nt_ndx].spritenum, TRUE))
                            {
                                FLAG_KEY_RELEASE(pp, SK_OPERATE);
                                break;
                            }
                        }
                    }
                }
            }

            {
                int neartaghitdist;
                short neartagsector, neartagsprite, neartagwall;

                neartaghitdist = nti[0].dist;
                neartagsector = nti[0].sectnum;
                neartagwall = nti[0].wallnum;
                neartagsprite = nti[0].spritenum;

                if (neartagsector >= 0 && neartaghitdist < 1024)
                {
                    if (OperateSector(neartagsector, TRUE))
                    {
                        // Release the key
                        FLAG_KEY_RELEASE(pp, SK_OPERATE);
                    }
                }

                if (neartagwall >= 0 && neartaghitdist < 1024)
                {
                    if (OperateWall(neartagwall, TRUE))
                    {
                        FLAG_KEY_RELEASE(pp, SK_OPERATE);
                    }
                }
            }

            //
            // Trigger operations
            //

            switch (LOW_TAG(pp->cursectnum))
            {
            case TAG_VATOR:
                DoVatorOperate(pp, pp->cursectnum);
                DoSpikeOperate(pp, pp->cursectnum);
                DoRotatorOperate(pp, pp->cursectnum);
                DoSlidorOperate(pp, pp->cursectnum);
                break;
            case TAG_SPRING_BOARD:
                DoSpringBoard(pp, pp->cursectnum);
                FLAG_KEY_RELEASE(pp, SK_OPERATE);
                break;
            case TAG_DOOR_ROTATE:
                if (OperateSector(pp->cursectnum, TRUE))
                    FLAG_KEY_RELEASE(pp, SK_OPERATE);
                break;
            }
        }
    }
    else
    {
        // Reset the key when syncbit key is not in use
        FLAG_KEY_RESET(pp, SK_OPERATE);
    }

    // ////////////////////////////
    //
    // Sector Damage
    //
    // ////////////////////////////

    if (sectu && sectu->damage)
    {
        if (TEST(sectu->flags, SECTFU_DAMAGE_ABOVE_SECTOR))
        {
            PlayerTakeSectorDamage(pp);
        }
        else if ((SPRITEp_BOS(pp->SpriteP) >= sectp->floorz) && !TEST(pp->Flags, PF_DIVING))
        {
            PlayerTakeSectorDamage(pp);
        }
    }
    else
    {
        USERp u = User[pp->PlayerSprite];
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

        if (TEST(sector[pp->cursectnum].extra, SECTFX_WARP_SECTOR))
        {
            if (!TEST(pp->Flags2, PF2_TELEPORTED))
            {
                DoPlayerWarpTeleporter(pp);
            }
        }

        RESET(pp->Flags2, PF2_TELEPORTED);
    }
}



void
DoSineWaveFloor(void)
{
    SINE_WAVE_FLOOR *swf;
    int newz;
    short wave;
    char flags;

    for (wave = 0; wave < MAX_SINE_WAVE; wave++)
    {
        for (swf = &SineWaveFloor[wave][0], flags = swf->flags; swf->sector >= 0 && swf < &SineWaveFloor[wave][SIZ(SineWaveFloor[wave])]; swf++)
        {

            swf->sintable_ndx = NORM_ANGLE(swf->sintable_ndx + (synctics << swf->speed_shift));

            if (TEST(flags, SINE_FLOOR))
            {
                newz = swf->floor_origz + ((swf->range * sintable[swf->sintable_ndx]) >> 14);
                sector[swf->sector].floorz = newz;
            }

            if (TEST(flags, SINE_CEILING))
            {
                newz = swf->ceiling_origz + ((swf->range * sintable[swf->sintable_ndx]) >> 14);
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


void
DoSineWaveWall(void)
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
                New = sw->orig_xy + ((sw->range * sintable[sw->sintable_ndx]) >> 14);
                // wall[sw->wall].y = New;
                dragpoint(sw->wall, wall[sw->wall].x, New, 0);
            }
            else
            {
                New = sw->orig_xy + ((sw->range * sintable[sw->sintable_ndx]) >> 14);
                // wall[sw->wall].x = New;
                dragpoint(sw->wall, New, wall[sw->wall].y, 0);
            }
        }
    }
}

void
DoAnim(int numtics)
{
    int i, animval;

    for (i = AnimCnt - 1; i >= 0; i--)
    {
        animval = *Anim[i].ptr;

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

        *Anim[i].ptr = animval;

        // EQUAL this entry has finished
        if (animval == Anim[i].goal)
        {
            ANIM_CALLBACKp acp = Anim[i].callback;

            // do a callback when done if not NULL
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

void
AnimClear(void)
{
    int i, animval;

#if 1
    AnimCnt = 0;
#else
    for (i = AnimCnt - 1; i >= 0; i--)
    {
        if (Anim[i].extra)
        {
            FreeMem(Anim[i].extra);
            Anim[i].extra = 0;
        }
    }

    AnimCnt = 0;
#endif
}

short
AnimGetGoal(int *animptr)
{
    int i, j;

    j = -1;
    for (i = 0; i < AnimCnt; i++)
    {
        if (animptr == Anim[i].ptr)
        {
            j = i;
            break;
        }
    }

    return j;
}

void
AnimDelete(int *animptr)
{
    int i, j;

    j = -1;
    for (i = 0; i < AnimCnt; i++)
    {
        if (animptr == Anim[i].ptr)
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


short
AnimSet(int *animptr, int thegoal, int thevel)
{
    int i, j;

    ASSERT(AnimCnt < MAXANIM - 1);

    j = AnimCnt;

    // look for existing animation and reset it
    for (i = 0; i < AnimCnt; i++)
    {
        if (animptr == Anim[i].ptr)
        {
            j = i;
            break;
        }
    }

    Anim[j].ptr = animptr;
    Anim[j].goal = thegoal;
    Anim[j].vel = Z(thevel);
    Anim[j].vel_adj = 0;
    Anim[j].callback = NULL;
    Anim[j].callbackdata = NULL;

    if (j == AnimCnt)
        AnimCnt++;

    return j;
}

short
AnimSetCallback(short anim_ndx, ANIM_CALLBACKp call, ANIM_DATAp data)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].callback = call;
    Anim[anim_ndx].callbackdata = data;

    return anim_ndx;
}

short
AnimSetVelAdj(short anim_ndx, short vel_adj)
{
    ASSERT(anim_ndx < AnimCnt);

    if (anim_ndx == -1)
        return anim_ndx;

    Anim[anim_ndx].vel_adj = vel_adj;

    return anim_ndx;
}


void initlava(void)
{
#if 0
    int x, y, z, r;
    int i;
    extern char tempbuf[];

//char lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2)], lavainc[LAVASIZ];
//int lavanumdrops, lavanumframes;
//int lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
//int lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
//int lavaradx[32][128], lavarady[32][128], lavaradcnt[32];


    UPDATE TO NEW CODE

    static int lavaradx[24][96];
    static int lavarady[24][96];

    for
    lavaradcnt[z] = 0;

    for (x = -16; x <= 16; x++)
        for (y = -16; y <= 16; y++)
        {
            // r should only be between 0 and 31
            // lavaradcnt[r] should be less 127
            r = ksqrt(x * x + y * y);
            lavaradx[r][lavaradcnt[r]] = x;
            lavarady[r][lavaradcnt[r]] = y;
            // this was causing an overwrite in ElevatorAuto structure
            lavaradcnt[r]++;
        }

//      for(z=0;z<16;z++)
//              lavadropsizlookup[z] = 8 / (ksqrt(z)+1);

    for (z = 0; z < 16; z++)
    {
        lavadropsizlookup[z] = 8 / (ksqrt(16 - z) + 1);
    }

    for (z = 0; z < LAVASIZ; z++)
    {
        lavainc[z] = klabs((((z ^ 17) >> 4) & 7) - 4) + 12;
    }

    lavanumdrops = 0;
    lavanumframes = 0;
#endif
}

void movelava(char *dapic)
{
#if 0
//    #define COLOR_OFFSET 192
#define COLOR_OFFSET LT_BROWN

    char dat, *ptr;
    int x, y, z, zx, dalavadropsiz, dadropsizlookup;
    intptr_t offs, offs2;
    int dalavax, dalavay;

    z = 3;
    if (lavanumdrops + z >= LAVAMAXDROPS)
        z = LAVAMAXDROPS - lavanumdrops - 1;
    while (z >= 0)
    {
        lavadropx[lavanumdrops] = (rand() & (LAVASIZ - 1));
        lavadropy[lavanumdrops] = (rand() & (LAVASIZ - 1));
        lavadropsiz[lavanumdrops] = 1;
        lavanumdrops++;
        z--;
    }

    z = lavanumdrops - 1;
    while (z >= 0)
    {
        dadropsizlookup = lavadropsizlookup[lavadropsiz[z]] * (((z & 1) << 1) - 1);
        dalavadropsiz = lavadropsiz[z];
        dalavax = lavadropx[z];
        dalavay = lavadropy[z];
        for (zx = lavaradcnt[lavadropsiz[z]] - 1; zx >= 0; zx--)
        {
            offs = (((lavaradx[dalavadropsiz][zx] + dalavax) & (LAVASIZ - 1)) << LAVALOGSIZ);
            offs += ((lavarady[dalavadropsiz][zx] + dalavay) & (LAVASIZ - 1));

            dapic[offs] += dadropsizlookup;

            if (dapic[offs] < COLOR_OFFSET)
                dapic[offs] = COLOR_OFFSET;
        }

        lavadropsiz[z]++;
        if (lavadropsiz[z] > 10)
        {
            lavanumdrops--;
            lavadropx[z] = lavadropx[lavanumdrops];
            lavadropy[z] = lavadropy[lavanumdrops];
            lavadropsiz[z] = lavadropsiz[lavanumdrops];
        }
        z--;
    }

    // Back up dapic with 1 pixel extra on each boundary
    // (to prevent anding for wrap-around)
    offs = ((intptr_t) dapic);
    offs2 = (LAVASIZ + 2) + 1 + ((intptr_t) lavabakpic);
    for (x = 0; x < LAVASIZ; x++)
    {
        copybuf(offs, offs2, LAVASIZ >> 2);
        offs += LAVASIZ;
        offs2 += LAVASIZ + 2;
    }
    for (y = 0; y < LAVASIZ; y++)
    {
        lavabakpic[y + 1] = dapic[y + ((LAVASIZ - 1) << LAVALOGSIZ)];
        lavabakpic[y + 1 + (LAVASIZ + 1) * (LAVASIZ + 2)] = dapic[y];
    }
    for (x = 0; x < LAVASIZ; x++)
    {
        lavabakpic[(x + 1) * (LAVASIZ + 2)] = dapic[(x << LAVALOGSIZ) + (LAVASIZ - 1)];
        lavabakpic[(x + 1) * (LAVASIZ + 2) + (LAVASIZ + 1)] = dapic[x << LAVALOGSIZ];
    }
    lavabakpic[0] = dapic[LAVASIZ * LAVASIZ - 1];
    lavabakpic[LAVASIZ + 1] = dapic[LAVASIZ * (LAVASIZ - 1)];
    lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 1)] = dapic[LAVASIZ - 1];
    lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2) - 1] = dapic[0];

    for (z = (LAVASIZ + 2) * (LAVASIZ + 2) - 4; z >= 0; z -= 4)
    {
        lavabakpic[z + 0] &= 31;
        lavabakpic[z + 1] &= 31;
        lavabakpic[z + 2] &= 31;
        lavabakpic[z + 3] &= 31;
    }

    for (x = LAVASIZ - 1; x >= 0; x--)
    {
        FAKETIMERHANDLER();
        offs = (x + 1) * (LAVASIZ + 2) + 1;
        ptr = (char *)((x << LAVALOGSIZ) + (intptr_t) dapic);

        zx = ((x + lavanumframes) & (LAVASIZ - 1));

        offs2 = LAVASIZ - 1;
        for (y = offs; y < offs + LAVASIZ; y++)
        {
            dat = lavainc[(offs2--) & zx];

            dat += lavabakpic[y - (LAVASIZ + 2) - 1];
            dat += lavabakpic[y - (LAVASIZ + 2)];
            dat += lavabakpic[y - (LAVASIZ + 2) + 1];
            dat += lavabakpic[y - 1];
            dat += lavabakpic[y + 1];
            dat += lavabakpic[y + (LAVASIZ + 2)];
            dat += lavabakpic[y + (LAVASIZ + 2) - 1];

            *ptr++ = (dat >> 3) + COLOR_OFFSET;
        }
    }

    lavanumframes++;
#endif
}


void
DoPanning(void)
{
    int nx, ny;
    short i,nexti;
    SPRITEp sp;
    SECTORp sectp;
    WALLp wallp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FLOOR_PAN], i, nexti)
    {
        sp = &sprite[i];
        sectp = &sector[sp->sectnum];

        nx = (((int) sintable[NORM_ANGLE(sp->ang + 512)]) * sp->xvel) >> 20;
        ny = (((int) sintable[sp->ang]) * sp->xvel) >> 20;

        sectp->floorxpanning += nx;
        sectp->floorypanning += ny;

        sectp->floorxpanning &= 255;
        sectp->floorypanning &= 255;
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CEILING_PAN], i, nexti)
    {
        sp = &sprite[i];
        sectp = &sector[sp->sectnum];

        nx = (((int) sintable[NORM_ANGLE(sp->ang + 512)]) * sp->xvel) >> 20;
        ny = (((int) sintable[sp->ang]) * sp->xvel) >> 20;

        sectp->ceilingxpanning += nx;
        sectp->ceilingypanning += ny;

        sectp->ceilingxpanning &= 255;
        sectp->ceilingypanning &= 255;
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALL_PAN], i, nexti)
    {
        sp = &sprite[i];
        wallp = &wall[sp->owner];

        nx = (((int) sintable[NORM_ANGLE(sp->ang + 512)]) * sp->xvel) >> 20;
        ny = (((int) sintable[sp->ang]) * sp->xvel) >> 20;

        wallp->xpanning += nx;
        wallp->ypanning += ny;

        wallp->xpanning &= 255;
        wallp->ypanning &= 255;
    }
}


void
DoSector(void)
{
    short i;
    SECTOR_OBJECTp sop;
    SWBOOL riding;
    extern SWBOOL DebugActorFreeze;
    int sync_flag;
    short pnum;
    int min_dist,dist,a,b,c;
    PLAYERp pp;

    if (DebugActorFreeze)
        return;

#if 0
    MoveSectorObjects(synctics);
#endif

#if 0
    if (MoveSkip2 == 0)
        MoveSectorObjects(synctics * 2);
#endif

#if 0
    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;
        pp->sop_riding = NULL;
    }

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        if (MoveSkip2 == 0)
            MoveSectorObjects(sop, synctics * 2);
    }
#endif

#if 1
    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {

        if (sop->xmid == MAXLONG || sop->xmid == MAXSO)
            continue;


        riding = FALSE;
        min_dist = 999999;

        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            if (pp->sop_riding == sop)
            {
                riding = TRUE;
                pp->sop_riding = NULL;
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
#endif

    DoPanning();
    DoLighting();
    DoSineWaveFloor();
    DoSineWaveWall();
    DoSpringBoardDown();
}

#if 0
int inside(int x, int y, short sectnum)
{
    WALLp wal;
    int i, x1, y1, x2, y2;
    char cnt;

    if ((sectnum < 0) || (sectnum >= numsectors))
        return -1;
    cnt = 0;

    wal = &wall[sector[sectnum].wallptr];
    for (i = sector[sectnum].wallnum; i > 0; i--)
    {
        y1 = wal->y - y;
        y2 = wall[wal->point2].y - y;
        if ((y1 ^ y2) < 0)
        {
            x1 = wal->x - x;
            x2 = wall[wal->point2].x - x;

            if ((x1 ^ x2) < 0)
                cnt ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
            else if (x1 >= 0)
                cnt ^= 1;
        }
        wal++;
    }
    return cnt;
}
}
#endif

#include "saveable.h"

static saveable_code saveable_sector_code[] =
{
    SAVE_CODE(WallSetupDontMove),
    SAVE_CODE(WallSetup),
    SAVE_CODE(SectorLiquidSet),
    SAVE_CODE(SectorSetup),
    SAVE_CODE(DoSpringBoard),
    SAVE_CODE(DoSpringBoardDown),
    SAVE_CODE(DoSpawnActorTrigger),
    SAVE_CODE(OperateSector),
    SAVE_CODE(OperateWall),
    SAVE_CODE(AnimateSwitch),
    SAVE_CODE(SectorExp),
    SAVE_CODE(DoExplodeSector),
    SAVE_CODE(DoSpawnSpot),
    SAVE_CODE(DoSpawnSpotsForKill),
    SAVE_CODE(DoSpawnSpotsForDamage),
    SAVE_CODE(DoSoundSpotMatch),
    SAVE_CODE(DoSoundSpotStopSound),
    SAVE_CODE(DoStopSoundSpotMatch),
    SAVE_CODE(DoSectorObjectKillMatch),
    SAVE_CODE(DoDeleteSpriteMatch),
    SAVE_CODE(DoChangorMatch),
    SAVE_CODE(DoMatchEverything),
    SAVE_CODE(DoTrapReset),
    SAVE_CODE(DoTrapMatch),
    SAVE_CODE(OperateTripTrigger),
    SAVE_CODE(OperateContinuousTrigger),
    SAVE_CODE(PlayerTakeSectorDamage),
    SAVE_CODE(NearThings),
    SAVE_CODE(NearTagList),
    SAVE_CODE(BuildNearTagList),
    SAVE_CODE(DoPlayerGrabStar),
    SAVE_CODE(PlayerOperateEnv),
    SAVE_CODE(DoSineWaveFloor),
    SAVE_CODE(DoSineWaveWall),
    SAVE_CODE(DoAnim),
    SAVE_CODE(AnimClear),
    SAVE_CODE(AnimGetGoal),
    SAVE_CODE(AnimDelete),
    SAVE_CODE(AnimSet),
    SAVE_CODE(AnimSetCallback),
    SAVE_CODE(AnimSetVelAdj),
    SAVE_CODE(DoPanning),
    SAVE_CODE(DoSector),
};

saveable_module saveable_sector =
{
    // code
    saveable_sector_code,
    SIZ(saveable_sector_code),

    // data
    NULL,
    0
};


