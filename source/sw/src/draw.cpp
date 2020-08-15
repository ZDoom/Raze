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

#define QUIET
#include "build.h"
#include "pragmas.h"


#include "names2.h"
#include "panel.h"
#include "game.h"
#include "quake.h"

#include "jsector.h"

#include "mytypes.h"
#include "gamecontrol.h"
#include "network.h"
#include "pal.h"
#include "player.h"
#include "jtags.h"
#include "parent.h"

#include "misc.h"
#include "reserve.h"

#include "menus.h"
#include "interp.h"
#include "interpso.h"
#include "sector.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "glbackend/glbackend.h"

BEGIN_SW_NS

static int OverlapDraw = FALSE;
extern SWBOOL QuitFlag, LocationInfo, ConPanel, SpriteInfo;
extern SWBOOL Voxel;
extern char buffer[];
SWBOOL DrawScreen;
extern short f_c;

extern short HelpPage;
extern short HelpPagePic[];
extern ParentalStruct aVoxelArray[MAXTILES];
extern int Follow_posx,Follow_posy;

int ConnectCopySprite(uspritetype const * tsp);
void PreDrawStackedWater(void);

#if 1
void
ShadeSprite(tspriteptr_t tsp)
{
    // set shade of sprite
    tsp->shade = sector[tsp->sectnum].floorshade - 25;

    if (tsp->shade > -3)
        tsp->shade = -3;

    if (tsp->shade < -30)
        tsp->shade = -30;
}
#else
#endif


short
GetRotation(short tSpriteNum, int viewx, int viewy)
{
    static short RotTable8[] = {0, 7, 6, 5, 4, 3, 2, 1};
    static short RotTable5[] = {0, 1, 2, 3, 4, 3, 2, 1};
    short rotation;

    tspriteptr_t tsp = &tsprite[tSpriteNum];
    USERp tu = User[tsp->owner];
    short angle2;

    if (tu->RotNum == 0)
        return 0;

    // Get which of the 8 angles of the sprite to draw (0-7)
    // rotation ranges from 0-7
    angle2 = getangle(tsp->x - viewx, tsp->y - viewy);
    rotation = ((tsp->ang + 3072 + 128 - angle2) & 2047);
    rotation = (rotation >> 8) & 7;

    if (tu->RotNum == 5)
    {
        if (TEST(tu->Flags, SPR_XFLIP_TOGGLE))
        {
            if (rotation <= 4)
            {
                // leave rotation alone
                RESET(tsp->cstat, CSTAT_SPRITE_XFLIP);
            }
            else
            {
                rotation = (8 - rotation);
                SET(tsp->cstat, CSTAT_SPRITE_XFLIP);    // clear x-flipping bit
            }
        }
        else
        {
            if (rotation > 3 || rotation == 0)
            {
                // leave rotation alone
                RESET(tsp->cstat, CSTAT_SPRITE_XFLIP);  // clear x-flipping bit
            }
            else
            {
                rotation = (8 - rotation);
                SET(tsp->cstat, CSTAT_SPRITE_XFLIP);    // set
            }
        }

        // Special case bunk
        if (tu->ID == TOILETGIRL_R0 || tu->ID == WASHGIRL_R0 || tu->ID == TRASHCAN ||
            tu->ID == CARGIRL_R0 || tu->ID == MECHANICGIRL_R0 || tu->ID == PRUNEGIRL_R0 ||
            tu->ID == SAILORGIRL_R0)
            RESET(tsp->cstat, CSTAT_SPRITE_XFLIP);  // clear x-flipping bit

        return RotTable5[rotation];
    }

    return RotTable8[rotation];

}

/*


!AIC - At draw time this is called for actor rotation.  GetRotation() is more
complex than needs to be in part because importing of actor rotations and x-flip
directions was not standardized.

*/

int
SetActorRotation(short tSpriteNum, int viewx, int viewy)
{
    tspriteptr_t tsp = &tsprite[tSpriteNum];
    USERp tu = User[tsp->owner];
    short StateOffset, Rotation;

    // don't modify ANY tu vars - back them up!
    STATEp State = tu->State;
    STATEp StateStart = tu->StateStart;

    if (tu->RotNum == 0)
        return 0;

    // Get the offset into the State animation
    StateOffset = State - StateStart;

    // Get the rotation angle
    Rotation = GetRotation(tSpriteNum, viewx, viewy);

    ASSERT(Rotation < 5);

    // Reset the State animation start based on the Rotation
    StateStart = tu->Rot[Rotation];

    // Set the sprites state
    State = StateStart + StateOffset;

    // set the picnum here - may be redundant, but we just changed states and
    // thats a big deal
    tsp->picnum = State->Pic;

    //sprintf(ds,"SetActorRotation:tsp->picnum: %d",tsp->picnum);
    //CON_Message(ds);

    /*

     !AIC KEY - For actor states EVERY rotation needs to have the same tics
     animators.  The only thing different can be the picnum.  If not then sync bugs
     will occur.  This code attempts to check to the best of its ability for this
     problem.  Should go away with shipped compile.

    */

#if DEBUG
    {
        short i;

        for (i = 0; i < tu->RotNum; i++)
        {
            STATEp TestStateStart, TestState;

            TestStateStart = tu->Rot[i];
            TestState = TestStateStart + StateOffset;

            ASSERT(State->Tics == TestState->Tics);
            ASSERT(State->Animator == TestState->Animator);
        }
    }
#endif
    return 0;
}

int
DoShadowFindGroundPoint(tspriteptr_t sp)
{
    // USES TSPRITE !!!!!
    USERp u = User[sp->owner];
    SPRITEp hsp;
    int ceilhit, florhit;
    int hiz, loz = u->loz;
    short save_cstat, bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    save_cstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(sp->x, sp->y, sp->z, sp->sectnum, &hiz, &ceilhit, &loz, &florhit);
    sp->cstat = save_cstat;

    ASSERT(TEST(florhit, HIT_SPRITE | HIT_SECTOR));

    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SPRITE:
    {
        hsp = &sprite[NORM_SPRITE(florhit)];

        if (TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            // found a sprite floor
            return loz;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            bak_cstat = hsp->cstat;
            RESET(hsp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            loz = DoShadowFindGroundPoint(sp);
            hsp->cstat = bak_cstat;
        }
        break;
    }

    case HIT_SECTOR:
        break;

    default:
        ASSERT(TRUE == FALSE);
        break;
    }

    return loz;
}

#if 0
#define GENERIC_SHADOW_PIC 66
extern SWBOOL bVoxelsOn;
void
DoVoxelShadow(SPRITEp tspr)
{
    // Check for voxels
    if (bVoxelsOn)
    {
        switch (tspr->picnum)
        {
        case ICON_STAR:             // 1793
        case ICON_UZI:              // 1797
        case ICON_UZIFLOOR:         // 1807
        case ICON_LG_UZI_AMMO:      // 1799
        case ICON_HEART:            // 1824
        case ICON_HEART_LG_AMMO:    // 1820
        case ICON_GUARD_HEAD:       // 1814
        case ICON_FIREBALL_LG_AMMO: // 3035
        case ICON_ROCKET:           // 1843
        case ICON_SHOTGUN:          // 1794
        case ICON_LG_ROCKET:        // 1796
        case ICON_LG_SHOTSHELL:     // 1823
        case ICON_MICRO_GUN:        // 1818
        case ICON_MICRO_BATTERY:    // 1800
        case ICON_GRENADE_LAUNCHER: // 1817
        case ICON_LG_GRENADE:       // 1831
        case ICON_LG_MINE:          // 1842
        case ICON_RAIL_GUN:         // 1811
        case ICON_RAIL_AMMO:        // 1812
        case ICON_SM_MEDKIT:        // 1802
        case ICON_MEDKIT:           // 1803
        case ICON_CHEMBOMB:         // 1808
        case ICON_FLASHBOMB:        // 1805
        case ICON_NUKE:             // 1809
        case ICON_CALTROPS:
        case ICON_BOOSTER:          // 1810
        case ICON_HEAT_CARD:        // 1819
        case ICON_REPAIR_KIT:       // 1813
        case ICON_EXPLOSIVE_BOX:    // 1801
        case ICON_ENVIRON_SUIT:     // 1837
        case ICON_FLY:              // 1782
        case ICON_CLOAK:            // 1826
        case ICON_NIGHT_VISION:     // 3031
        case ICON_NAPALM:           // 3046
        case ICON_RING:             // 3050
        //case ICON_GOROAMMO:       // 3035
        //case ICON_HEARTAMMO:      // 1820
        case ICON_RINGAMMO:         // 3054
        case ICON_NAPALMAMMO:       // 3058
        case ICON_GRENADE:          // 3059
        //case ICON_OXYGEN:         // 1800
        case ICON_ARMOR:            // 3030
        case BLUE_KEY:              // 1766
        case RED_KEY:               // 1770
        case GREEN_KEY:             // 1774
        case YELLOW_KEY:            // 1778
        case GOLD_SKELKEY:
        case SILVER_SKELKEY:
        case BRONZE_SKELKEY:
        case RED_SKELKEY:
        case BLUE_CARD:
        case RED_CARD:
        case GREEN_CARD:
        case YELLOW_CARD:
//              tspr->picnum = GENERIC_SHADOW_PIC;
            tspr->xrepeat = 0;  // For now, don't do voxel shadows
            tspr->yrepeat = 0;
//              tspr->xrepeat = 27;
//              tspr->yrepeat = 4;
            //tspr->z+=(sintable[(rotang*2)%2047]/16);
            break;
        }
    }
}
#endif

void
DoShadows(tspriteptr_t tsp, int viewz, SWBOOL mirror)
{
    tspriteptr_t New = &tsprite[spritesortcnt];
    USERp tu = User[tsp->owner];
    int ground_dist = 0;
    int view_dist = 0;
    int loz;
    short xrepeat;
    short yrepeat;
    short sectnum;

    sectnum = tsp->sectnum;
    // make sure its the correct sector
    // DoShadowFindGroundPoint calls FAFgetzrangepoint and this is sensitive
    //updatesectorz(tsp->x, tsp->y, tsp->z, &sectnum);
    updatesector(tsp->x, tsp->y, &sectnum);

    if (sectnum < 0)
    {
        return;
    }

    tsp->sectnum = sectnum;
    *New = *tsp;
    // shadow is ALWAYS draw last - status is priority
    New->statnum = MAXSTATUS;
    New->sectnum = sectnum;

    if ((tsp->yrepeat >> 2) > 4)
    {
        yrepeat = (tsp->yrepeat >> 2) - (SPRITEp_SIZE_Y(tsp) / 64) * 2;
        xrepeat = New->xrepeat;
    }
    else
    {
        yrepeat = New->yrepeat;
        xrepeat = New->xrepeat;
    }

    New->shade = 127;
    SET(New->cstat, CSTAT_SPRITE_TRANSLUCENT);

    loz = tu->loz;
    if (tu->lo_sp)
    {
        if (!TEST(tu->lo_sp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            loz = DoShadowFindGroundPoint(tsp);
        }
    }

#if 0
    if (SectUser[tsp->sectnum] && SectUser[tsp->sectnum]->depth)
    {
        loz -= Z(SectUser[tsp->sectnum]->depth);
    }
#endif

    // need to find the ground here
    New->z = loz;

    // if below or close to sprites z don't bother to draw it
    if ((viewz - loz) > -Z(8))
        return;

    // if close to shadows z shrink it
    view_dist = labs(loz - viewz) >> 8;
    if (view_dist < 32)
        view_dist = 256/view_dist;
    else
        view_dist = 0;

    // make shadow smaller depending on height from ground
    ground_dist = labs(loz - SPRITEp_BOS(tsp)) >> 8;
    ground_dist = DIV16(ground_dist);

    xrepeat = max(xrepeat - ground_dist - view_dist, 4);
    yrepeat = max(yrepeat - ground_dist - view_dist, 4);
    xrepeat = min(xrepeat, short(255));
    yrepeat = min(yrepeat, short(255));

    New->xrepeat = xrepeat;
    New->yrepeat = yrepeat;

    if (tilehasmodelorvoxel(tsp->picnum,tsp->pal))
    {
        New->yrepeat = 0;
        // cstat:    trans reverse
        // clipdist: tell mdsprite.cpp to use Z-buffer hacks to hide overdraw issues
        New->clipdist |= TSPR_FLAGS_MDHACK;
        New->cstat |= 512;
    }
    else
    {
        int const camang = mirror ? NORM_ANGLE(2048 - Player[screenpeek].siang) : Player[screenpeek].siang;
        vec2_t const ofs = { sintable[NORM_ANGLE(camang+512)]>>11, sintable[NORM_ANGLE(camang)]>>11};

        New->x += ofs.x;
        New->y += ofs.y;
    }

    // Check for voxel items and use a round generic pic if so
    //DoVoxelShadow(New);

    spritesortcnt++;
}

void
DoMotionBlur(tspritetype const * const tsp)
{
    USERp tu = User[tsp->owner];
    int nx,ny,nz = 0,dx,dy,dz;
    short i, ang;
    short xrepeat, yrepeat, repeat_adj = 0;
    int z_amt_per_pixel;

    ang = NORM_ANGLE(tsp->ang + 1024);

    if (tsp->xvel == 0)
    {
        return;
    }

    if (TEST(tsp->extra, SPRX_PLAYER_OR_ENEMY))
    {
        z_amt_per_pixel = (((int)-tu->jump_speed * ACTORMOVETICS)<<16)/tsp->xvel;
    }
    else
    {
        z_amt_per_pixel = (((int)-tsp->zvel)<<16)/tsp->xvel;
    }

    switch (tu->motion_blur_dist)
    {
    case 64:
        dx = nx = MOVEx(64, ang);
        dy = ny = MOVEy(64, ang);
        nz = (z_amt_per_pixel * 64)>>16;
        break;
    case 128:
        dx = nx = MOVEx(128, ang);
        dy = ny = MOVEy(128, ang);
        nz = (z_amt_per_pixel * 128)>>16;
        break;
    case 256:
        dx = nx = MOVEx(256, ang);
        dy = ny = MOVEy(256, ang);
        nz = (z_amt_per_pixel * 256)>>16;
        break;
    case 512:
        dx = nx = MOVEx(512, ang);
        dy = ny = MOVEy(512, ang);
        nz = (z_amt_per_pixel * 512)>>16;
        break;
    default:
        dx = nx = MOVEx(tu->motion_blur_dist, ang);
        dy = ny = MOVEy(tu->motion_blur_dist, ang);
        break;
    }

    dz = nz;

    xrepeat = tsp->xrepeat;
    yrepeat = tsp->yrepeat;

    switch (TEST(tu->Flags2, SPR2_BLUR_TAPER))
    {
    case 0:
        repeat_adj = 0;
        break;
    case SPR2_BLUR_TAPER_SLOW:
        repeat_adj = xrepeat / (tu->motion_blur_num*2);
        break;
    case SPR2_BLUR_TAPER_FAST:
        repeat_adj = xrepeat / tu->motion_blur_num;
        break;
    }

    for (i = 0; i < tu->motion_blur_num; i++)
    {
        tspriteptr_t New = &tsprite[spritesortcnt];
        *New = *tsp;
        SET(New->cstat, CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_TRANSLUCENT_INVERT);

        New->x += dx;
        New->y += dy;
        dx += nx;
        dy += ny;

        New->z += dz;
        dz += nz;

        New->xrepeat = xrepeat;
        New->yrepeat = yrepeat;

        xrepeat -= repeat_adj;
        yrepeat -= repeat_adj;

        spritesortcnt++;
    }

}

void SetVoxelSprite(SPRITEp sp, short pic)
{
    SET(sp->cstat, CSTAT_SPRITE_ALIGNMENT_SLAB);
    sp->picnum = pic;
}

void WarpCopySprite(void)
{
    SPRITEp sp1, sp2, sp;
    short sn, nsn;
    short sn2, nsn2;
    short spnum, next_spnum;
    int xoff,yoff,zoff;
    short match;
    short sect1, sect2;

    // look for the first one
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_WARP_COPY_SPRITE1], sn, nsn)
    {
        sp1 = &sprite[sn];
        match = sp1->lotag;

        // look for the second one
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_WARP_COPY_SPRITE2], sn2, nsn2)
        {
            sp = &sprite[sn2];

            if (sp->lotag == match)
            {
                sp2 = sp;
                sect1 = sp1->sectnum;
                sect2 = sp2->sectnum;

                TRAVERSE_SPRITE_SECT(headspritesect[sect1], spnum, next_spnum)
                {
                    if (&sprite[spnum] == sp1)
                        continue;

                    if (sprite[spnum].picnum == ST1)
                        continue;

                    tspriteptr_t New = renderAddTSpriteFromSprite(spnum);
                    New->statnum = 0;

                    xoff = sp1->x - New->x;
                    yoff = sp1->y - New->y;
                    zoff = sp1->z - New->z;

                    New->x = sp2->x - xoff;
                    New->y = sp2->y - yoff;
                    New->z = sp2->z - zoff;
                    New->sectnum = sp2->sectnum;
                }

                TRAVERSE_SPRITE_SECT(headspritesect[sect2], spnum, next_spnum)
                {
                    if (&sprite[spnum] == sp2)
                        continue;

                    if (sprite[spnum].picnum == ST1)
                        continue;

                    tspriteptr_t New = renderAddTSpriteFromSprite(spnum);
                    New->statnum = 0;

                    xoff = sp2->x - New->x;
                    yoff = sp2->y - New->y;
                    zoff = sp2->z - New->z;

                    New->x = sp1->x - xoff;
                    New->y = sp1->y - yoff;
                    New->z = sp1->z - zoff;
                    New->sectnum = sp1->sectnum;
                }
            }
        }
    }
}

void DoStarView(tspriteptr_t tsp, USERp tu, int viewz)
{
    extern STATE s_Star[], s_StarDown[];
    extern STATE s_StarStuck[], s_StarDownStuck[];
    int zdiff = viewz - tsp->z;

    if (labs(zdiff) > Z(24))
    {
        if (tu->StateStart == s_StarStuck)
            tsp->picnum = s_StarDownStuck[tu->State - s_StarStuck].Pic;
        else
            tsp->picnum = s_StarDown[tu->State - s_Star].Pic;

        if (zdiff > 0)
            SET(tsp->cstat, CSTAT_SPRITE_YFLIP);
    }
    else
    {
        if (zdiff > 0)
            SET(tsp->cstat, CSTAT_SPRITE_YFLIP);
    }
}

void
analyzesprites(int viewx, int viewy, int viewz, SWBOOL mirror)
{
    int tSpriteNum;
    short SpriteNum;
    int smr4, smr2;
    USERp tu;
    static int ang = 0;
    PLAYERp pp = Player + screenpeek;
    short newshade=0;


    ang = NORM_ANGLE(ang + 12);

    smr4 = smoothratio + (((int) MoveSkip4) << 16);
    smr2 = smoothratio + (((int) MoveSkip2) << 16);

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        SpriteNum = tsprite[tSpriteNum].owner;
        tspriteptr_t tsp = &tsprite[tSpriteNum];
        tu = User[SpriteNum];

        //if(tsp->statnum == STAT_GENERIC_QUEUE)
        //    Printf("tsp->pal = %d",tsp->pal);

#if 0
        // Brighten up the sprite if set somewhere else to do so
        if (tu && tu->Vis > 0)
        {
            short tmpshade; // Having this prevent overflow

            tmpshade = tsp->shade  - tu->Vis;
            if (tmpshade < -128) tmpshade = -128;

            tsp->shade = tmpshade;
            tu->Vis -= 8;
        }
#endif

        // don't draw these
        if (tsp->statnum >= STAT_DONT_DRAW)
        {
            tsp->owner = -1;
            continue;
        }

        // Diss any parentally locked sprites
        if (adult_lockout || Global_PLock)
        {
            if (aVoxelArray[tsp->picnum].Parental == 6145)
            {
                tsp->owner = -1;
                tu = NULL;
            }
            else if (aVoxelArray[tsp->picnum].Parental > 0)
            {
                ASSERT(aVoxelArray[tsp->picnum].Parental >= 0 && aVoxelArray[tsp->picnum].Parental < 6145);
                tsp->picnum=aVoxelArray[tsp->picnum].Parental; // Change the pic
            }
        }

        if (tu)
        {
            if (tsp->statnum != STAT_DEFAULT)
            {
                if (TEST(tu->Flags, SPR_SKIP4))
                {
                    if (tsp->statnum <= STAT_SKIP4_INTERP_END)
                    {
                        tsp->x = tu->ox + mulscale18(tsp->x - tu->ox, smr4);
                        tsp->y = tu->oy + mulscale18(tsp->y - tu->oy, smr4);
                        tsp->z = tu->oz + mulscale18(tsp->z - tu->oz, smr4);
                    }
                }

                if (TEST(tu->Flags, SPR_SKIP2))
                {
                    if (tsp->statnum <= STAT_SKIP2_INTERP_END)
                    {
                        tsp->x = tu->ox + mulscale17(tsp->x - tu->ox, smr2);
                        tsp->y = tu->oy + mulscale17(tsp->y - tu->oy, smr2);
                        tsp->z = tu->oz + mulscale17(tsp->z - tu->oz, smr2);
                    }
                }
            }

            // workaround for mines and floor decals beneath the floor
            if (tsp->picnum == BETTY_R0 || tsp->picnum == FLOORBLOOD1)
            {
                auto sp = (uspriteptr_t)&sprite[SpriteNum];
                int32_t const floorz = getflorzofslope(sp->sectnum, sp->x, sp->y);
                if (sp->z > floorz)
                    tsp->z = floorz;
            }

            if (r_shadows && TEST(tu->Flags, SPR_SHADOW))
            {
                DoShadows(tsp, viewz, mirror);
            }

            //#define UK_VERSION 1

            //#define DART_REPEAT 6
            //#define DART_PIC 2233
            if (sw_darts)
                if (tu->ID == 1793 || tsp->picnum == 1793)
                {
                    tsp->picnum = 2519;
                    tsp->xrepeat = 27;
                    tsp->yrepeat = 29;
                }

#define DART_PIC 2526
#define DART_REPEAT 16
            if (tu->ID == STAR1)
            {
                if (sw_darts)
                {
                    tsp->picnum = DART_PIC;
                    tsp->ang = NORM_ANGLE(tsp->ang - 512 - 24);
                    tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                    SET(tsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL);
                }
                else
                    DoStarView(tsp, tu, viewz);
            }

            // rotation
            if (tu->RotNum > 0)
                SetActorRotation(tSpriteNum, viewx, viewy);

            if (tu->motion_blur_num)
            {
                DoMotionBlur(tsp);
            }

            // set palette lookup correctly
            if (tsp->pal != sector[tsp->sectnum].floorpal)
            {
                if (sector[tsp->sectnum].floorpal == PALETTE_DEFAULT)
                {
                    // default pal for sprite is stored in tu->spal
                    // mostly for players and other monster types
                    tsp->pal = tu->spal;
                }
                else
                {
                    // if sector pal is something other than default
                    SECT_USERp sectu = SectUser[tsp->sectnum];
                    uint8_t pal = sector[tsp->sectnum].floorpal;
                    SWBOOL nosectpal=FALSE;

                    // sprite does not take on the new pal if sector flag is set
                    if (sectu && TEST(sectu->flags, SECTFU_DONT_COPY_PALETTE))
                    {
                        pal = PALETTE_DEFAULT;
                        nosectpal = TRUE;
                    }

                    //if(tu->spal == PALETTE_DEFAULT)
                    if (tsp->hitag != SECTFU_DONT_COPY_PALETTE && tsp->hitag != LUMINOUS
                        && !nosectpal
                        && pal != PALETTE_FOG && pal != PALETTE_DIVE &&
                        pal != PALETTE_DIVE_LAVA)
                        tsp->pal = pal;
                    else
                        tsp->pal = tu->spal;

                }
            }

            // Sprite debug information mode
            if (tsp->hitag == 9997)
            {
                tsp->pal = PALETTE_RED_LIGHTING;
                // Turn it off, it gets reset by PrintSpriteInfo
                sprite[tu->SpriteNum].hitag = 0;
            }
        }

        if (sw_darts)
            if (tsp->statnum == STAT_STAR_QUEUE)
            {
                tsp->picnum = DART_PIC;
                tsp->ang = NORM_ANGLE(tsp->ang - 512);
                tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                SET(tsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL);
            }

        // Call my sprite handler
        // Does autosizing and voxel handling
        JAnalyzeSprites(tsp);

        // only do this of you are a player sprite
        //if (tsp->statnum >= STAT_PLAYER0 && tsp->statnum < STAT_PLAYER0 + MAX_SW_PLAYERS)
        if (tu && tu->PlayerP)
        {
            // Shadow spell
            if (!TEST(tsp->cstat, CSTAT_SPRITE_TRANSLUCENT))
                ShadeSprite(tsp);

            // sw if its your playersprite
            //if ((Player + screenpeek)->PlayerSprite == SpriteNum)
            if ((Player + screenpeek)->PlayerSprite == tu->SpriteNum)
            {
                PLAYERp pp = Player + screenpeek;
                if (mirror || TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE|PF_VIEW_FROM_CAMERA))
                {
                    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
                        SET(tsp->cstat, CSTAT_SPRITE_TRANSLUCENT);

                    if (TEST(pp->Flags, PF_CLIMBING))
                    {
                        // move sprite forward some so he looks like he's
                        // climbing
                        tsp->x = pp->six + MOVEx(128 + 80, tsp->ang);
                        tsp->y = pp->siy + MOVEy(128 + 80, tsp->ang);
                    }
                    else
                    {
                        tsp->x = pp->six;
                        tsp->y = pp->siy;
                    }

                    tsp->z = tsp->z + pp->siz;
                    tsp->ang = pp->siang;
                    //continue;
                }
                else
                {
                    // dont draw your sprite
                    tsp->owner = -1;
                    //SET(tsp->cstat, CSTAT_SPRITE_INVISIBLE);
                }
            }
            else if (!PedanticMode) // Otherwise just interpolate the player sprite
            {
                PLAYERp pp = tu->PlayerP;
                tsp->x -= mulscale16(pp->posx - pp->oposx, 65536-smoothratio);
                tsp->y -= mulscale16(pp->posy - pp->oposy, 65536-smoothratio);
                tsp->z -= mulscale16(pp->posz - pp->oposz, 65536-smoothratio);
                tsp->ang -= fix16_to_int(mulscale16(pp->q16ang - pp->oq16ang, 65536-smoothratio));
            }
        }

        if (OverlapDraw && FAF_ConnectArea(tsp->sectnum) && tsp->owner >= 0)
        {
            EDUKE32_STATIC_ASSERT(sizeof(uspritetype) == sizeof(tspritetype)); // see TSPRITE_SIZE
            ConnectCopySprite((uspriteptr_t)tsp);
        }

        //
        // kens original sprite shade code he moved out of the engine
        //

        switch (tsp->statnum)
        {
        case STAT_ENEMY:
        case STAT_DEAD_ACTOR:
        case STAT_FAF_COPY:
            break;
        default:
            newshade = tsp->shade;
            newshade += 6;
            if (newshade > 127) newshade = 127;
            tsp->shade = newshade;
        }

        if (TEST(sector[tsp->sectnum].ceilingstat, CEILING_STAT_PLAX))
        {
            newshade = tsp->shade;
            newshade += sector[tsp->sectnum].ceilingshade;
            if (newshade > 127) newshade = 127;
            if (newshade < -128) newshade = -128;
            tsp->shade = newshade;
        }
        else
        {
            newshade = tsp->shade;
            newshade += sector[tsp->sectnum].floorshade;
            if (newshade > 127) newshade = 127;
            if (newshade < -128) newshade = -128;
            tsp->shade = newshade;
        }

        if (tsp->hitag == 9998)
            tsp->shade = 127; // Invisible enemy ninjas

        // Correct shades for luminous sprites
        if (tsp->hitag == LUMINOUS)
        {
            tsp->shade = -128;
        }

        if (pp->NightVision && TEST(tsp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            if (tu && tu->ID == TRASHCAN) continue; // Don't light up trashcan

            tsp->pal = PALETTE_ILLUMINATE;  // Make sprites REALLY bright green.
            tsp->shade = -128;
        }

        if (tu && tu->PlayerP)
        {
            if (TEST(tu->Flags2, SPR2_VIS_SHADING))
            {
                if ((Player + screenpeek)->PlayerSprite != tu->SpriteNum)
                {
                    if (!TEST(tu->PlayerP->Flags, PF_VIEW_FROM_OUTSIDE))
                    {
                        RESET(tsp->cstat, CSTAT_SPRITE_TRANSLUCENT);
                    }
                }

                tsp->shade = 12 - STD_RANDOM_RANGE(30);
            }
        }
    }

    WarpCopySprite();

}

#if 1
tspriteptr_t get_tsprite(short SpriteNum)
{
    int tSpriteNum;

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        if (tsprite[tSpriteNum].owner == SpriteNum)
            return &tsprite[tSpriteNum];
    }

    return NULL;
}

void
post_analyzesprites(void)
{
    int tSpriteNum;
    short SpriteNum;
    USERp tu;

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        SpriteNum = tsprite[tSpriteNum].owner;
        if (SpriteNum < 0) continue;    // JBF: verify this is safe
        tspriteptr_t tsp = &tsprite[tSpriteNum];
        tu = User[SpriteNum];

        if (tu)
        {
            if (tu->ID == FIREBALL_FLAMES && tu->Attach >= 0)
            {
                //uspritetype * const atsp = &sprite[tu->Attach];
                tspriteptr_t const atsp = get_tsprite(tu->Attach);

                if (!atsp)
                {
                    //DSPRINTF(ds,"Attach not found");
                    MONO_PRINT(ds);
                    continue;
                }

                tsp->x = atsp->x;
                tsp->y = atsp->y;
                // statnum is priority - draw this ALWAYS first at 0
                // statnum is priority - draw this ALWAYS last at MAXSTATUS
                if (TEST(atsp->extra, SPRX_BURNABLE))
                {
                    atsp->statnum = 1;
                    tsp->statnum = 0;
                }
                else
                    tsp->statnum = MAXSTATUS;
            }
        }
    }
}
#endif

static ClockTicks mapzoomclock;

void
ResizeView(PLAYERp pp)
{
    if (M_Active() || paused)
        return;

    if (dimensionmode == 2 || dimensionmode == 5 || dimensionmode == 6)
    {
        int32_t timepassed = (int32_t)(totalclock - mapzoomclock);
        mapzoomclock += timepassed;
        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
            zoom = max<int32_t>(zoom - mulscale7(timepassed * synctics, zoom), 48);

        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
            zoom = min<int32_t>(zoom + mulscale7(timepassed * synctics, zoom), 4096);

#if 0
        if (inputState.GetKeyStatus(sc_Escape))
        {
            extern SWBOOL ScrollMode2D;

			inputState.ClearKeyStatus(sc_Escape);
            dimensionmode = 3;
            ScrollMode2D = FALSE;
        }
#endif
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))      // &&
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);
            G_ChangeHudLayout(-1);
        }

        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen)) // &&
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);
            G_ChangeHudLayout(1);
        }
    }
}


void
BackView(int *nx, int *ny, int *nz, short *vsect, fix16_t *nq16ang, short horiz)
{
    vec3_t n = { *nx, *ny, *nz };
    SPRITEp sp;
    hitdata_t hitinfo;
    int i, vx, vy, vz, hx, hy;
    short bakcstat, daang;
    PLAYERp pp = &Player[screenpeek];
    short ang;

    ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);

    ang = fix16_to_int(*nq16ang) + pp->view_outside_dang;

    // Calculate the vector (nx,ny,nz) to shoot backwards
    vx = (sintable[NORM_ANGLE(ang + 1536)] >> 3);
    vy = (sintable[NORM_ANGLE(ang + 1024)] >> 3);
    vz = (horiz - 100) * 256L;

    // Player sprite of current view
    sp = &sprite[pp->PlayerSprite];

    bakcstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Make sure sector passed to FAFhitscan is correct
    //COVERupdatesector(*nx, *ny, vsect);

    hitscan(&n, *vsect, vx, vy, vz,
            &hitinfo, CLIPMASK_PLAYER);

    ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);

    sp->cstat = bakcstat;              // Restore cstat

    hx = hitinfo.pos.x - (*nx);
    hy = hitinfo.pos.y - (*ny);

    // If something is in the way, make pp->camera_dist lower if necessary
    if (klabs(vx) + klabs(vy) > klabs(hx) + klabs(hy))
    {
        if (hitinfo.wall >= 0)               // Push you a little bit off the wall
        {
            *vsect = hitinfo.sect;

            daang = getangle(wall[wall[hitinfo.wall].point2].x - wall[hitinfo.wall].x,
                             wall[wall[hitinfo.wall].point2].y - wall[hitinfo.wall].y);

            i = vx * sintable[daang] + vy * sintable[NORM_ANGLE(daang + 1536)];
            if (klabs(vx) > klabs(vy))
                hx -= mulscale28(vx, i);
            else
                hy -= mulscale28(vy, i);
        }
        else if (hitinfo.sprite < 0)        // Push you off the ceiling/floor
        {
            *vsect = hitinfo.sect;

            if (klabs(vx) > klabs(vy))
                hx -= (vx >> 5);
            else
                hy -= (vy >> 5);
        }
        else
        {
            SPRITEp hsp = &sprite[hitinfo.sprite];
            int flag_backup;

            // if you hit a sprite that's not a wall sprite - try again
            if (!TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                flag_backup = hsp->cstat;
                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);
                BackView(nx, ny, nz, vsect, nq16ang, horiz);
                hsp->cstat = flag_backup;
                return;
            }
            else
            {
                // same as wall calculation
                daang = NORM_ANGLE(sp->ang-512);

                i = vx * sintable[daang] + vy * sintable[NORM_ANGLE(daang + 1536)];
                if (klabs(vx) > klabs(vy))
                    hx -= mulscale28(vx, i);
                else
                    hy -= mulscale28(vy, i);
            }

        }

        if (klabs(vx) > klabs(vy))
            i = (hx << 16) / vx;
        else
            i = (hy << 16) / vy;

        if (i < pp->camera_dist)
            pp->camera_dist = i;
    }

    // Actually move you!  (Camerdist is 65536 if nothing is in the way)
    *nx = (*nx) + mulscale16(vx, pp->camera_dist);
    *ny = (*ny) + mulscale16(vy, pp->camera_dist);
    *nz = (*nz) + mulscale16(vz, pp->camera_dist);

    // Slowly increase pp->camera_dist until it reaches 65536
    // Synctics is a timer variable so it increases the same rate
    // on all speed computers
    pp->camera_dist = min(pp->camera_dist + (3 << 10), 65536);
    //pp->camera_dist = min(pp->camera_dist + (synctics << 10), 65536);

    // Make sure vsect is correct
    updatesectorz(*nx, *ny, *nz, vsect);

    *nq16ang = fix16_from_int(ang);
}

void
CircleCamera(int *nx, int *ny, int *nz, short *vsect, int *nq16ang, short horiz)
{
    vec3_t n = { *nx, *ny, *nz };
    SPRITEp sp;
    hitdata_t hitinfo;
    int i, vx, vy, vz, hx, hy;
    short bakcstat, daang;
    PLAYERp pp = &Player[screenpeek];
    short ang;

    ang = fix16_to_int(*nq16ang) + pp->circle_camera_ang;

    // Calculate the vector (nx,ny,nz) to shoot backwards
    vx = (sintable[NORM_ANGLE(ang + 1536)] >> 4);
    vy = (sintable[NORM_ANGLE(ang + 1024)] >> 4);

    // lengthen the vector some
    vx += DIV2(vx);
    vy += DIV2(vy);

    vz = (horiz - 100) * 256;

    // Player sprite of current view
    sp = &sprite[pp->PlayerSprite];

    bakcstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Make sure sector passed to hitscan is correct
    //COVERupdatesector(*nx, *ny, vsect);

    hitscan(&n, *vsect, vx, vy, vz,
            &hitinfo, CLIPMASK_MISSILE);

    sp->cstat = bakcstat;              // Restore cstat
    //ASSERT(hitinfo.sect >= 0);

    hx = hitinfo.pos.x - (*nx);
    hy = hitinfo.pos.y - (*ny);

    // If something is in the way, make pp->circle_camera_dist lower if necessary
    if (klabs(vx) + klabs(vy) > klabs(hx) + klabs(hy))
    {
        if (hitinfo.wall >= 0)               // Push you a little bit off the wall
        {
            *vsect = hitinfo.sect;

            daang = getangle(wall[wall[hitinfo.wall].point2].x - wall[hitinfo.wall].x,
                             wall[wall[hitinfo.wall].point2].y - wall[hitinfo.wall].y);

            i = vx * sintable[daang] + vy * sintable[NORM_ANGLE(daang + 1536)];
            if (klabs(vx) > klabs(vy))
                hx -= mulscale28(vx, i);
            else
                hy -= mulscale28(vy, i);
        }
        else if (hitinfo.sprite < 0)        // Push you off the ceiling/floor
        {
            *vsect = hitinfo.sect;

            if (klabs(vx) > klabs(vy))
                hx -= (vx >> 5);
            else
                hy -= (vy >> 5);
        }
        else
        {
            SPRITEp hsp = &sprite[hitinfo.sprite];
            int flag_backup;

            // if you hit a sprite that's not a wall sprite - try again
            if (!TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                flag_backup = hsp->cstat;
                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

                CircleCamera(nx, ny, nz, vsect, nq16ang, horiz);
                hsp->cstat = flag_backup;
                return;
            }
        }

        if (klabs(vx) > klabs(vy))
            i = (hx << 16) / vx;
        else
            i = (hy << 16) / vy;

        if (i < pp->circle_camera_dist)
            pp->circle_camera_dist = i;
    }

    // Actually move you!  (Camerdist is 65536 if nothing is in the way)
    *nx = (*nx) + ((vx * pp->circle_camera_dist) >> 16);
    *ny = (*ny) + ((vy * pp->circle_camera_dist) >> 16);
    *nz = (*nz) + ((vz * pp->circle_camera_dist) >> 16);

    // Slowly increase pp->circle_camera_dist until it reaches 65536
    // Synctics is a timer variable so it increases the same rate
    // on all speed computers
    pp->circle_camera_dist = min(pp->circle_camera_dist + (3 << 8), 65536);
    //pp->circle_camera_dist = min(pp->circle_camera_dist + (synctics << 10), 65536);

    // Make sure vsect is correct
    updatesectorz(*nx, *ny, *nz, vsect);

    *nq16ang = fix16_from_int(ang);
}

FString GameInterface::statFPS()
{
#define AVERAGEFRAMES 16
    static int frameval[AVERAGEFRAMES], framecnt = 0;
    int i;

    FString out;
    //if (LocationInfo)
    {

        i = (int32_t)totalclock;
        if (i != frameval[framecnt])
        {
            out.AppendFormat("FPS: %d\n", ((120 * AVERAGEFRAMES) / (i - frameval[framecnt])) + f_c);
            frameval[framecnt] = i;
        }

        framecnt = ((framecnt + 1) & (AVERAGEFRAMES - 1));
    }
    return out;
}

FString GameInterface::GetCoordString()
{
    PLAYERp pp = Player + myconnectindex;
    FString out;
    out.AppendFormat("POSX:%d ", pp->posx);
    out.AppendFormat("POSY:%d ", pp->posy);
    out.AppendFormat("POSZ:%d ", pp->posz);
    out.AppendFormat("ANG:%d\n", fix16_to_int(pp->camq16ang));

    return out;
}


void PrintSpriteInfo(PLAYERp pp)
{
#define Y_STEP 7
    int x = windowxy1.x+2;
    int y = windowxy1.y+2;
    SPRITEp sp;
    USERp u;

    //if (SpriteInfo && !LocationInfo)
    {
        short hit_sprite = DoPickTarget(pp->SpriteP, 32, 2);

        sp = &sprite[hit_sprite];
        u = User[hit_sprite];

        sp->hitag = 9997; // Special tag to make the actor glow red for one frame

        if (hit_sprite == -1)
        {
            Printf("SPRITENUM: NONE TARGETED\n");
            return;
        }
        else
            Printf("SPRITENUM:%d\n", hit_sprite);

        if (u)
        {
            Printf("ID:%d, ", u->ID);
            Printf("PALETTE:%d, ", u->spal);
            Printf("HEALTH:%d, ", u->Health);
            Printf("WAITTICS:%d, ", u->WaitTics);
            Printf("COUNTER:%d, ", u->Counter);
            Printf("COUNTER2:%d\n", u->Counter);
        }
        if (sp)
        {
            Printf("POSX:%d, ", TrackerCast(sp->x));
            Printf("POSY:%d, ", TrackerCast(sp->y));
            Printf("POSZ:%d,", TrackerCast(sp->z));
            Printf("ANG:%d\n", TrackerCast(sp->ang));
        }
    }
}


void SpriteSortList2D(int tx, int ty)
{
    SPRITEp sp;
    int i;
    int dist,a,b,c;

    spritesortcnt = 0;
    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS)
        {
            sp = &sprite[i];

            if (!TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE) &&
                (sp->xrepeat > 0) && (sp->yrepeat > 0) &&
                (spritesortcnt < MAXSPRITESONSCREEN))
            {
                DISTANCE(tx,ty,sp->x,sp->y,dist,a,b,c);

                if (dist < 22000)
                {
                    renderAddTSpriteFromSprite(i);
                }
            }
        }
    }
}

void DrawCheckKeys(PLAYERp pp)
{
    if (ConPanel) return;

    if (!InputMode)
        ResizeView(pp);
}

void DrawCrosshair(PLAYERp pp)
{
    extern SWBOOL CameraTestMode;

    if (cl_crosshair && !(CameraTestMode) && !TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE) && dimensionmode != 6)
    {
        int32_t a = 2326;

        double crosshair_scale = cl_crosshairscale * .01;
        if (isRR()) crosshair_scale *= .5;

        DrawTexture(twod, tileGetTexture(a), 160, 100, DTA_Color, shadeToLight(10),
            DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, crosshair_scale, DTA_ScaleY, crosshair_scale, DTA_CenterOffsetRel, true,
            DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1, TAG_DONE);
    }
}

void CameraView(PLAYERp pp, int *tx, int *ty, int *tz, short *tsectnum, fix16_t *tq16ang, fix16_t *tq16horiz)
{
    int i,nexti;
    short ang;
    SPRITEp sp;
    SWBOOL found_camera = FALSE;
    SWBOOL player_in_camera = FALSE;
    SWBOOL FAFcansee_test;
    SWBOOL ang_test;

    if (pp == &Player[screenpeek])
    {
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_DEMO_CAMERA], i, nexti)
        {
            sp = &sprite[i];

            ang = getangle(*tx - sp->x, *ty - sp->y);
            ang_test  = GetDeltaAngle(sp->ang, ang) < sp->lotag;

            FAFcansee_test =
                (FAFcansee(sp->x, sp->y, sp->z, sp->sectnum, *tx, *ty, *tz, pp->cursectnum) ||
                 FAFcansee(sp->x, sp->y, sp->z, sp->sectnum, *tx, *ty, *tz + SPRITEp_SIZE_Z(pp->SpriteP), pp->cursectnum));

            player_in_camera = ang_test && FAFcansee_test;

            if (player_in_camera || pp->camera_check_time_delay > 0)
            {

                // if your not in the camera but are still looking
                // make sure that only the last camera shows you

                if (!player_in_camera && pp->camera_check_time_delay > 0)
                {
                    if (pp->last_camera_sp != sp)
                        continue;
                }

                switch (sp->clipdist)
                {
                case 1:
                    pp->last_camera_sp = sp;
                    CircleCamera(tx, ty, tz, tsectnum, tq16ang, 100);
                    found_camera = TRUE;
                    break;

                default:
                {
                    int xvect,yvect,zvect,zdiff;

                    pp->last_camera_sp = sp;

                    xvect = sintable[NORM_ANGLE(ang + 512)] >> 3;
                    yvect = sintable[NORM_ANGLE(ang)] >> 3;

                    zdiff = sp->z - *tz;
                    if (labs(sp->x - *tx) > 1000)
                        zvect = scale(xvect, zdiff, sp->x - *tx);
                    else if (labs(sp->y - *ty) > 1000)
                        zvect = scale(yvect, zdiff, sp->y - *ty);
                    else if (sp->x - *tx != 0)
                        zvect = scale(xvect, zdiff, sp->x - *tx);
                    else if (sp->y - *ty != 0)
                        zvect = scale(yvect, zdiff, sp->y - *ty);
                    else
                        zvect = 0;

                    // new horiz to player
                    *tq16horiz = fix16_from_int(100 - (zvect/256));
                    *tq16horiz = fix16_max(*tq16horiz, fix16_from_int(PLAYER_HORIZ_MIN));
                    *tq16horiz = fix16_min(*tq16horiz, fix16_from_int(PLAYER_HORIZ_MAX));

                    //DSPRINTF(ds,"xvect %d,yvect %d,zvect %d,tq16horiz %d",xvect,yvect,zvect,*tq16horiz);
                    MONO_PRINT(ds);

                    *tq16ang = fix16_from_int(ang);
                    *tx = sp->x;
                    *ty = sp->y;
                    *tz = sp->z;
                    *tsectnum = sp->sectnum;

                    found_camera = TRUE;
                    break;
                }
                }
            }

            if (found_camera)
                break;
        }
    }

    // if you player_in_camera you definately have a camera
    if (player_in_camera)
    {
        pp->camera_check_time_delay = 120/2;
        SET(pp->Flags, PF_VIEW_FROM_CAMERA);

        ASSERT(found_camera);
    }
    else
    // if you !player_in_camera you still might have a camera
    // for a split second
    {
        if (found_camera)
        {
            SET(pp->Flags, PF_VIEW_FROM_CAMERA);
        }
        else
        {
            pp->circle_camera_ang = 0;
            pp->circle_camera_dist = CIRCLE_CAMERA_DIST_MIN;
            RESET(pp->Flags, PF_VIEW_FROM_CAMERA);
        }
    }
}

void
PreDraw(void)
{
    short i, nexti;

    PreDrawStackedWater();

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FLOOR_SLOPE_DONT_DRAW], i, nexti)
    {
        RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_SLOPE);
    }
}

void
PostDraw(void)
{
    short i, nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FLOOR_SLOPE_DONT_DRAW], i, nexti)
    {
        SET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_SLOPE);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF_COPY], i, nexti)
    {
        if (User[i])
        {
            FreeMem(User[i]);
            User[i] = NULL;
        }

#if DEBUG
        SPRITEp sp = &sprite[i];
        short statnum = sp->statnum;
        short sectnum = sp->sectnum;
        memset(sp, 0xCC, sizeof(SPRITE));
        sp->statnum = statnum;
        sp->sectnum = sectnum;
#endif

        deletesprite(i);
    }
}

int CopySprite(uspritetype const * tsp, short newsector)
{
    short New;
    SPRITEp sp;

    New = COVERinsertsprite(newsector, STAT_FAF_COPY);
    sp = &sprite[New];

    sp->x = tsp->x;
    sp->y = tsp->y;
    sp->z = tsp->z;
    sp->cstat = tsp->cstat;
    sp->picnum = tsp->picnum;
    sp->pal = tsp->pal;
    sp->xrepeat = tsp->xrepeat;
    sp->yrepeat = tsp->yrepeat;
    sp->xoffset = tsp->xoffset;
    sp->yoffset = tsp->yoffset;
    sp->ang = tsp->ang;
    sp->xvel = tsp->xvel;
    sp->yvel = tsp->yvel;
    sp->zvel = tsp->zvel;
    sp->shade = tsp->shade;

    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    return New;
}

int ConnectCopySprite(uspritetype const * tsp)
{
    short newsector;
    int testz;

    if (FAF_ConnectCeiling(tsp->sectnum))
    {
        newsector = tsp->sectnum;
        testz = SPRITEp_TOS(tsp) - Z(10);

        if (testz < sector[tsp->sectnum].ceilingz)
            updatesectorz(tsp->x, tsp->y, testz, &newsector);

        if (newsector >= 0 && newsector != tsp->sectnum)
        {
            return CopySprite(tsp, newsector);
        }
    }

    if (FAF_ConnectFloor(tsp->sectnum))
    {
        newsector = tsp->sectnum;
        testz = SPRITEp_BOS(tsp) + Z(10);

        if (testz > sector[tsp->sectnum].floorz)
            updatesectorz(tsp->x, tsp->y, testz, &newsector);

        if (newsector >= 0 && newsector != tsp->sectnum)
        {
            return CopySprite(tsp, newsector);
        }
    }

    return -1;
}


void PreDrawStackedWater(void)
{
    short si,snexti;
    short i,nexti;
    SPRITEp sp;
    USERp u,nu;
    short New;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CEILING_FLOOR_PIC_OVERRIDE], si, snexti)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[sprite[si].sectnum], i, nexti)
        {
            if (User[i])
            {
                if (sprite[i].statnum == STAT_ITEM)
                    continue;

                if (sprite[i].statnum <= STAT_DEFAULT || sprite[i].statnum > STAT_PLAYER0 + MAX_SW_PLAYERS)
                    continue;

                // code so that a copied sprite will not make another copy
                if (User[i]->xchange == -989898)
                    continue;

                sp = &sprite[i];
                u = User[i];

                New = ConnectCopySprite((uspritetype const *)sp);
                if (New >= 0)
                {
                    // spawn a user
                    User[New] = nu = (USERp)CallocMem(sizeof(USER), 1);
                    ASSERT(nu != NULL);

                    nu->xchange = -989898;

                    // copy everything reasonable from the user that
                    // analyzesprites() needs to draw the image
                    nu->State = u->State;
                    nu->Rot = u->Rot;
                    nu->StateStart = u->StateStart;
                    nu->StateEnd = u->StateEnd;
                    nu->ox = u->ox;
                    nu->oy = u->oy;
                    nu->oz = u->oz;
                    nu->Flags = u->Flags;
                    nu->Flags2 = u->Flags2;
                    nu->RotNum = u->RotNum;
                    nu->ID = u->ID;

                    // set these to other sprite for players draw
                    nu->SpriteNum = i;
                    nu->SpriteP = sp;

                    nu->PlayerP = u->PlayerP;
                    nu->spal = u->spal;
                }
            }
        }
    }
}


void FAF_DrawRooms(int x, int y, int z, fix16_t q16ang, fix16_t q16horiz, short sectnum)
{
    short i,nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CEILING_FLOOR_PIC_OVERRIDE], i, nexti)
    {
        if (SPRITE_TAG3(i) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].ceilingpicnum;
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].ceilingstat;
            //SET(sector[sprite[i].sectnum].ceilingstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].ceilingstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].floorpicnum;
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].floorstat;
            //SET(sector[sprite[i].sectnum].floorstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].floorstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }

    renderDrawRoomsQ16(x,y,z,q16ang,q16horiz,sectnum);

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CEILING_FLOOR_PIC_OVERRIDE], i, nexti)
    {
        // manually set gotpic
        if (TEST_GOTSECTOR(sprite[i].sectnum))
        {
            SET_GOTPIC(FAF_MIRROR_PIC);
        }

        if (SPRITE_TAG3(i) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].ceilingstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].floorstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }
}

short ScreenSavePic = FALSE;

SWBOOL PicInView(short, SWBOOL);
void DoPlayerDiveMeter(PLAYERp pp);
void MoveScrollMode2D(PLAYERp pp);

void
drawscreen(PLAYERp pp)
{
    extern SWBOOL CameraTestMode;
    int tx, ty, tz;
    fix16_t tq16horiz, tq16ang;
    short tsectnum;
    short i,j;
    int bob_amt = 0;
    int quake_z, quake_x, quake_y;
    short quake_ang;
    extern SWBOOL FAF_DebugView;
    PLAYERp camerapp;                       // prediction player if prediction is on, else regular player

    // last valid stuff
    static short lv_sectnum = -1;
    static int lv_x, lv_y, lv_z;

    int const viewingRange = viewingrange;

    DrawScreen = TRUE;
    PreDraw();
    // part of new border refresh method
    if (!ScreenSavePic)
    {
        SetBorder(pp);
    }

    PreUpdatePanel();


    smoothratio = CalcSmoothRatio(totalclock, ototalclock, 120 / synctics);
    if (paused && !ReloadPrompt) // The checks were brought over from domovethings
        smoothratio = 65536;

    if (!ScreenSavePic)
    {
        dointerpolations(smoothratio);                      // Stick at beginning of drawscreen
        short_dointerpolations(smoothratio);                      // Stick at beginning of drawscreen
        if (cl_sointerpolation)
            so_dointerpolations(smoothratio);                           // Stick at beginning of drawscreen
    }

    // TENSW: when rendering with prediction, the only thing that counts should
    // be the predicted player.
    if (PredictionOn && CommEnabled && pp == Player+myconnectindex)
        camerapp = ppp;
    else
        camerapp = pp;

    tx = camerapp->oposx + mulscale16(camerapp->posx - camerapp->oposx, smoothratio);
    ty = camerapp->oposy + mulscale16(camerapp->posy - camerapp->oposy, smoothratio);
    tz = camerapp->oposz + mulscale16(camerapp->posz - camerapp->oposz, smoothratio);
    // TODO: It'd be better to check pp->input.q16angvel instead, problem is that
    // it's been repurposed for the q16ang diff while tying input to framerate
    if (PedanticMode || (pp != Player+myconnectindex) ||
        (TEST(pp->Flags, PF_DEAD) && (loc.q16angvel == 0)))
    {
        tq16ang = camerapp->oq16ang + mulscale16(NORM_Q16ANGLE(camerapp->q16ang + fix16_from_int(1024) - camerapp->oq16ang) - fix16_from_int(1024), smoothratio);
        tq16horiz = camerapp->oq16horiz + mulscale16(camerapp->q16horiz - camerapp->oq16horiz, smoothratio);
    }
    else if (cl_sointerpolation && !CommEnabled)
    {
        tq16ang = camerapp->oq16ang + mulscale16(((pp->camq16ang + fix16_from_int(1024) - camerapp->oq16ang) & 0x7FFFFFF) - fix16_from_int(1024), smoothratio);
        tq16horiz = camerapp->oq16horiz + mulscale16(pp->camq16horiz - camerapp->oq16horiz, smoothratio);
    }
    else
    {
        tq16ang = pp->camq16ang;
        tq16horiz = pp->camq16horiz;
    }
    tsectnum = camerapp->cursectnum;

    //ASSERT(tsectnum >= 0 && tsectnum <= MAXSECTORS);
    // if updatesectorz is to sensitive try COVERupdatesector
    //updatesectorz(tx, ty, tz, &tsectnum);

    COVERupdatesector(tx, ty, &tsectnum);

    if (tsectnum < 0)
    {
#if 0
        // if we hit an invalid sector move to the last valid position for drawing
        tsectnum = lv_sectnum;
        tx = lv_x;
        ty = lv_y;
        tz = lv_z;
#endif
    }
    else
    {
        // last valid stuff
        lv_sectnum = tsectnum;
        lv_x = tx;
        lv_y = ty;
        lv_z = tz;
    }

    // with "last valid" code this should never happen
    // ASSERT(tsectnum >= 0 && tsectnum <= MAXSECTORS);

    if (pp->sop_riding || pp->sop_control)
    {
        if (pp->sop_control &&
            (!cl_sointerpolation || (CommEnabled && !pp->sop_remote)))
        {
            tx = pp->posx;
            ty = pp->posy;
            tz = pp->posz;
            tq16ang = pp->q16ang;
        }
        tsectnum = pp->cursectnum;
        updatesectorz(tx, ty, tz, &tsectnum);
    }

    pp->six = tx;
    pp->siy = ty;
    pp->siz = tz - pp->posz;
    pp->siang = fix16_to_int(tq16ang);

    QuakeViewChange(camerapp, &quake_z, &quake_x, &quake_y, &quake_ang);
    VisViewChange(camerapp, &g_visibility);
    tz = tz + quake_z;
    tx = tx + quake_x;
    ty = ty + quake_y;
    //tq16horiz = tq16horiz + fix16_from_int(quake_x);
    tq16ang = fix16_from_int(NORM_ANGLE(fix16_to_int(tq16ang) + quake_ang));

    if (pp->sop_remote)
    {
        if (TEST_BOOL1(pp->remote_sprite))
            tq16ang = fix16_from_int(pp->remote_sprite->ang);
        else
            tq16ang = GetQ16AngleFromVect(pp->sop_remote->xmid - tx, pp->sop_remote->ymid - ty);
    }

    //if (TEST(camerapp->Flags, PF_VIEW_FROM_OUTSIDE))
    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        BackView(&tx, &ty, &tz, &tsectnum, &tq16ang, fix16_to_int(tq16horiz));
    }
    else
    {
        bob_amt = camerapp->bob_amt;

        if (CameraTestMode)
        {
            CameraView(camerapp, &tx, &ty, &tz, &tsectnum, &tq16ang, &tq16horiz);
        }
    }

    if (!TEST(pp->Flags, PF_VIEW_FROM_CAMERA|PF_VIEW_FROM_OUTSIDE))
    {
        tz += bob_amt;
        tz += PedanticMode ? camerapp->bob_z :
                             pp->obob_z + mulscale16(pp->bob_z - pp->obob_z, smoothratio);

        // recoil only when not in camera
        //tq16horiz = tq16horiz + fix16_from_int(camerapp->recoil_horizoff);
        tq16horiz = tq16horiz + fix16_from_int(pp->recoil_horizoff);
        tq16horiz = fix16_max(tq16horiz, fix16_from_int(PLAYER_HORIZ_MIN));
        tq16horiz = fix16_min(tq16horiz, fix16_from_int(PLAYER_HORIZ_MAX));
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        videoSetCorrectedAspect();
    }

    renderSetAspect(Blrintf(float(viewingrange) * tanf(r_fov * (fPI/360.f))), yxaspect);

    if (dimensionmode != 6)// && !ScreenSavePic)
    {
        // Cameras must be done before the main loop.
        JS_DrawCameras(pp, tx, ty, tz);
    }

    OverlapDraw = TRUE;
    DrawOverlapRoom(tx, ty, tz, tq16ang, tq16horiz, tsectnum);
    OverlapDraw = FALSE;

    if (dimensionmode != 6)// && !ScreenSavePic)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tq16ang, tq16horiz);
        JS_DrawMirrors(pp, tx, ty, tz, tq16ang, tq16horiz);
    }

    // TODO: This call is redundant if the tiled overhead map is shown, but the
    // HUD elements should be properly outputted with hardware rendering first.
    if (!FAF_DebugView)
        FAF_DrawRooms(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

    analyzesprites(tx, ty, tz, FALSE);
    post_analyzesprites();
    renderDrawMasks();

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        renderSetAspect(viewingRange, divscale16(ydim * 8, xdim * 5));
    }

    UpdateStatusBar(totalclock);

    UpdatePanel();

#define SLIME 2305
    // Only animate lava if its picnum is on screen
    // gotpic is a bit array where the tile number's bit is set
    // whenever it is drawn (ceilings, walls, sprites, etc.)
#if 0	// This needs a different implementation.
    if ((gotpic[SLIME >> 3] & (1 << (SLIME & 7))) > 0)
    {
        gotpic[SLIME >> 3] &= ~(1 << (SLIME & 7));

        if (waloff[SLIME])
            movelava((char *) waloff[SLIME]);
    }
#endif


    i = pp->cursectnum;

    if (i >= 0)
    {
        show2dsector.Set(i);
        walltype *wal = &wall[sector[i].wallptr];
        for (j=sector[i].wallnum; j>0; j--,wal++)
        {
            i = wal->nextsector;
            if (i < 0) continue;
            if (wal->cstat&0x0071) continue;
            uint16_t const nextwall = wal->nextwall;
            if (nextwall < MAXWALLS && wall[nextwall].cstat&0x0071) continue;
            if (sector[i].lotag == 32767) continue;
            if (sector[i].ceilingz >= sector[i].floorz) continue;
            show2dsector.Set(i);
        }
    }

    if ((dimensionmode == 5 || dimensionmode == 6) && pp == Player+myconnectindex)
    {
        extern SWBOOL ScrollMode2D;

        if (ScrollMode2D)
        {
            tx = Follow_posx;
            ty = Follow_posy;
        }

        for (j = 0; j < MAXSPRITES; j++)
        {
            // Don't show sprites tagged with 257
            if (sprite[j].lotag == 257)
            {
                if (TEST(sprite[j].cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR))
                {
                    RESET(sprite[j].cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR);
                    sprite[j].owner = -2;
                }
            }
        }

        if (dimensionmode == 6)
        {
            // only clear the actual window.
            twod->AddColorOnlyQuad(windowxy1.x, windowxy1.y, (windowxy2.x + 1) - windowxy1.x, (windowxy2.y + 1) - windowxy1.y, 0xff000000);
            renderDrawMapView(tx, ty, zoom, fix16_to_int(tq16ang));
        }

        // Draw the line map on top of texture 2d map or just stand alone
        drawoverheadmap(tx, ty, zoom, fix16_to_int(tq16ang));
    }

    for (j = 0; j < MAXSPRITES; j++)
    {
        // Don't show sprites tagged with 257
        if (sprite[j].lotag == 257 && sprite[j].owner == -2)
            SET(sprite[j].cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR);
    }


    // if doing a screen save don't need to process the rest
    if (ScreenSavePic)
    {
        DrawScreen = FALSE;
        return;
    }

    //PrintLocationInfo(pp);
    //PrintSpriteInfo(pp);

#if SYNC_TEST
    SyncStatMessage();
#endif

    DrawCrosshair(pp);



    DoPlayerDiveMeter(pp); // Do the underwater breathing bar

    // Boss Health Meter, if Boss present
    BossHealthMeter();

	//if (!M_Active())

    videoNextPage();

#if SYNC_TEST
    SyncStatMessage();
#endif

    // certain input is done here - probably shouldn't be
    DrawCheckKeys(pp);

    restoreinterpolations();                 // Stick at end of drawscreen
    short_restoreinterpolations();                 // Stick at end of drawscreen
    if (cl_sointerpolation)
        so_restoreinterpolations();                       // Stick at end of drawscreen

    if (paused && !M_Active())
    {
        MNU_DrawString(160, 100, "Game Paused", 0, 0, 0);
    }

    if (!CommEnabled && TEST(pp->Flags, PF_DEAD))
    {
        if (ReloadPrompt)
        {
            ReloadPrompt = FALSE;
        }
    }

    PostDraw();
    DrawScreen = FALSE;
}

bool GameInterface::GenerateSavePic()
{
    ScreenSavePic = TRUE;
    drawscreen(Player + myconnectindex);
    ScreenSavePic = FALSE;
    return true;
}


END_SW_NS
