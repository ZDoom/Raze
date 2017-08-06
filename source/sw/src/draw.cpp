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
#define QUIET
#include "build.h"
#include "pragmas.h"
#include "cache1d.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "quake.h"
#include "vis.h"

#include "jsector.h"

#include "mytypes.h"
#include "control.h"
#include "function.h"
#include "net.h"
#include "pal.h"
#include "player.h"
#include "jtags.h"
#include "parent.h"

#include "cache.h"
#include "reserve.h"

#include "text.h"
#include "menus.h"
#include "interp.h"
#include "sector.h"

static int OverlapDraw = FALSE;
extern SWBOOL QuitFlag, LocationInfo, ConPanel, SpriteInfo, PauseKeySet;
extern SWBOOL Voxel;
extern char tempbuf[];
extern char buffer[];
SWBOOL DrawScreen;
extern short f_c;

extern SWBOOL HelpInputMode;
extern short HelpPage;
extern short HelpPagePic[];
extern ParentalStruct aVoxelArray[MAXTILES];
extern SWBOOL RedrawScreen;
SWBOOL RedrawCompass=FALSE;
extern int Follow_posx,Follow_posy;

int ConnectCopySprite(uspritetype const * tsp);
void PreDrawStackedWater(void);
void DrawCompass(PLAYERp pp);

#if 1
void
ShadeSprite(uspritetype * tsp)
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
    extern short screenpeek;

    uspritetype * tsp = &tsprite[tSpriteNum];
    USERp tu = User[tsp->owner];
    PLAYERp pp = Player + screenpeek;
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
    uspritetype * tsp = &tsprite[tSpriteNum];
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
DoShadowFindGroundPoint(uspritetype * sp)
{
    // USES TSPRITE !!!!!
    USERp u = User[sp->owner];
    SPRITEp hsp;
    int ceilhit, florhit;
    int hiz, loz = u->loz;
    short save_cstat, bak_cstat;
    SWBOOL found = FALSE;

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

        if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR))
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
    }

    case HIT_SECTOR:
    {
        return loz;
    }

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
DoShadows(uspritetype * tsp, int viewz)
{
    uspritetype * New = &tsprite[spritesortcnt];
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
        //int cz,fz;
        //getzsofslope(tsp->sectnum, tsp->x, tsp->y, &cz, &fz);
        ////DSPRINTF(ds,"Shad sect !fnd x%d, y%d, z%d, sect%d, cz %d, fz %d", tsp->x, tsp->y, tsp->z, tsp->sectnum, cz, fz);
        //MONO_PRINT(ds);
        return;
    }

    tsp->sectnum = sectnum;
    memcpy(New, tsp, sizeof(SPRITE));
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
        if (!TEST(tu->lo_sp->cstat, CSTAT_SPRITE_WALL | CSTAT_SPRITE_FLOOR))
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
    xrepeat = min(xrepeat, 255);
    yrepeat = min(yrepeat, 255);

    New->xrepeat = xrepeat;
    New->yrepeat = yrepeat;

    // Check for voxel items and use a round generic pic if so
    //DoVoxelShadow(New);

    spritesortcnt++;
}

void
DoMotionBlur(uspritetype const * tsp)
{
    uspritetype * New;
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
        New = &tsprite[spritesortcnt];
        memcpy(New, tsp, sizeof(SPRITE));
        SET(New->cstat, CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_TRANS_FLIP);

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
    SET(sp->cstat, CSTAT_SPRITE_SLAB);
    sp->picnum = pic;
}

void WarpCopySprite(void)
{
    SPRITEp sp1, sp2, sp;
    uspritetype * New;
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

                    New = &tsprite[spritesortcnt];
                    memcpy(New, &sprite[spnum], sizeof(SPRITE));
                    spritesortcnt++;
                    New->owner = spnum;
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

                    New = &tsprite[spritesortcnt];
                    memcpy(New, &sprite[spnum], sizeof(SPRITE));
                    spritesortcnt++;
                    New->owner = spnum;
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

void DoStarView(uspritetype * tsp, USERp tu, int viewz)
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
    int tSpriteNum, j, k;
    short SpriteNum, pnum;
    int smr4, smr2;
    uspritetype * tsp;
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
        tsp = &tsprite[tSpriteNum];
        tu = User[SpriteNum];

        //if(tsp->statnum == STAT_GENERIC_QUEUE)
        //    CON_ConMessage("tsp->pal = %d",tsp->pal);

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
        if (gs.ParentalLock || Global_PLock)
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

            if (gs.Shadows && TEST(tu->Flags, SPR_SHADOW))
            {
                DoShadows(tsp, viewz);
            }

            //#define UK_VERSION 1

            //#define DART_REPEAT 6
            //#define DART_PIC 2233
            if (useDarts)
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
                if (useDarts)
                {
                    tsp->picnum = DART_PIC;
                    tsp->ang = NORM_ANGLE(tsp->ang - 512 - 24);
                    tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                    SET(tsp->cstat, CSTAT_SPRITE_WALL);
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

        if (useDarts)
            if (tsp->statnum == STAT_STAR_QUEUE)
            {
                tsp->picnum = DART_PIC;
                tsp->ang = NORM_ANGLE(tsp->ang - 512);
                tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                SET(tsp->cstat, CSTAT_SPRITE_WALL);
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
        }

        if (OverlapDraw && FAF_ConnectArea(tsp->sectnum) && tsp->owner >= 0)
        {
            ConnectCopySprite(tsp);
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
uspritetype *
get_tsprite(short SpriteNum)
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
    uspritetype * tsp;
    USERp tu;

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        SpriteNum = tsprite[tSpriteNum].owner;
        if (SpriteNum < 0) continue;    // JBF: verify this is safe
        tsp = &tsprite[tSpriteNum];
        tu = User[SpriteNum];

        if (tu)
        {
            if (tu->ID == FIREBALL_FLAMES && tu->Attach >= 0)
            {
                //uspritetype * const atsp = &sprite[tu->Attach];
                uspritetype * const atsp = get_tsprite(tu->Attach);

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

void
ResizeView(PLAYERp pp)
{
    if (MenuInputMode || InputMode || HelpInputMode || ConPanel || ConInputMode || PauseKeySet)
        return;

    if (dimensionmode == 2 || dimensionmode == 5 || dimensionmode == 6)
    {
        if (PKEY_PRESSED(KEYSC_DASH)||PKEY_PRESSED(KEYSC_GMINUS))
        {
            if ((zoom -= (zoom >> 4)) < 48) zoom = 48;
        }

        if (PKEY_PRESSED(KEYSC_EQUAL)||PKEY_PRESSED(KEYSC_GPLUS))
        {
            if ((zoom += (zoom >> 4)) > 4096) zoom = 4096;
        }

        if (KEY_PRESSED(KEYSC_ESC))
        {
            extern SWBOOL ScrollMode2D;

            KEY_PRESSED(KEYSC_ESC) = 0;
            dimensionmode = 3;
            ScrollMode2D = FALSE;
            SetRedrawScreen(pp);
        }
    }
    else
    {
        if (BUTTON(gamefunc_Shrink_Screen))      // &&
        // !BUTTONHELD(gamefunc_SizeVi
        // ewDown))
        {
            CONTROL_ClearButton(gamefunc_Shrink_Screen);
            SetBorder(pp, gs.BorderNum + 1);
            SetRedrawScreen(pp);
        }

        if (BUTTON(gamefunc_Enlarge_Screen)) // &&
        // !BUTTONHELD(gamefunc_SizeViewUp))
        {
            CONTROL_ClearButton(gamefunc_Enlarge_Screen);
            SetBorder(pp, gs.BorderNum - 1);
            SetRedrawScreen(pp);
        }
    }
}

// !JIM! 08/06
extern SWBOOL UsingMenus;

#if 0
void
ViewOutsidePlayerRecurse(PLAYERp pp, int32_t* vx, int32_t* vy, int32_t* vz, int16_t* ang, int16_t* vsectnum)
{
    int nx, ny;
    int ret;

    *vx = pp->posx;
    *vy = pp->posy;
    *vz = pp->posz;
    *vsectnum = pp->cursectnum;

    *ang = pp->pang + pp->view_outside_dang;

    nx = sintable[NORM_ANGLE(*ang + 512 + 1024)] << 11;
    ny = sintable[NORM_ANGLE(*ang + 1024)] << 11;

    ret = clipmove_old(vx, vy, vz, vsectnum, nx, ny, 64L, 4 << 8, 4 << 8, CLIPMASK_PLAYER);

    switch (TEST(ret, HIT_MASK))
    {
    case HIT_SPRITE:
    {
        short hit_sprite;
        SPRITEp sp;

        hit_sprite = NORM_SPRITE(ret);
        sp = &sprite[hit_sprite];

        // if you hit a sprite that's not a wall sprite - try again
        if (!TEST(sp->cstat, CSTAT_SPRITE_WALL))
        {
            FLIP(sp->cstat, CSTAT_SPRITE_BLOCK);
            ViewOutsidePlayerRecurse(pp, vx, vy, vz, ang, vsectnum);
            FLIP(sp->cstat, CSTAT_SPRITE_BLOCK);
        }

        break;
    }
    }

    if (TEST(sector[*vsectnum].floorstat, FLOOR_STAT_SLOPE)|TEST(sector[*vsectnum].ceilingstat, CEILING_STAT_SLOPE))
    {
        int cz, fz;

        getzsofslope(*vsectnum, *vx, *vy, &cz, &fz);

        if (*vz > fz - Z(12))
            *vz = fz - Z(12);

        if (*vz < cz + Z(12))
            *vz = cz + Z(12);

    }
}
#endif



void
BackView(int *nx, int *ny, int *nz, short *vsect, short *nang, short horiz)
{
    vec3_t n = { *nx, *ny, *nz };
    SPRITEp sp;
    hitdata_t hitinfo;
    int i, vx, vy, vz, hx, hy, hz;
    short bakcstat, daang;
    PLAYERp pp = &Player[screenpeek];
    short ang;

    ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);

    ang = *nang + pp->view_outside_dang;

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
            if (!TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                flag_backup = hsp->cstat;
                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);
                BackView(nx, ny, nz, vsect, nang, horiz);
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

    *nang = ang;
}

void
CircleCamera(int *nx, int *ny, int *nz, short *vsect, short *nang, short horiz)
{
    vec3_t n = { *nx, *ny, *nz };
    SPRITEp sp;
    hitdata_t hitinfo;
    int i, vx, vy, vz, hx, hy, hz;
    short bakcstat, daang;
    PLAYERp pp = &Player[screenpeek];
    short ang;

    ang = *nang + pp->circle_camera_ang;

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
            if (!TEST(hsp->cstat, CSTAT_SPRITE_WALL))
            {
                flag_backup = hsp->cstat;
                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

                CircleCamera(nx, ny, nz, vsect, nang, horiz);
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

    *nang = ang;
}

void PrintLocationInfo(PLAYERp pp)
{
#define Y_STEP 7
#define AVERAGEFRAMES 16
    int x = windowxy1.x+2;
    int y = windowxy1.y+2;
    static int frameval[AVERAGEFRAMES], framecnt = 0;
    int i;

    if (LocationInfo)
    {

        i = totalclock;
        if (i != frameval[framecnt])
        {
            sprintf(tempbuf, "FPS: %d", ((120 * AVERAGEFRAMES) / (i - frameval[framecnt])) + f_c);
            printext256(x, y, 1, -1, tempbuf, 1);
            frameval[framecnt] = i;
        }

        framecnt = ((framecnt + 1) & (AVERAGEFRAMES - 1));

        if (LocationInfo > 1)
        {
            y += Y_STEP;

            sprintf(buffer, "POSX:%d", pp->posx);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "POSY:%d", pp->posy);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "POSZ:%d", pp->posz);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "ANG:%d", (int32_t) pp->pang);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
        }
    }
}

SWBOOL DebugSecret = FALSE;
void SecretInfo(PLAYERp pp)
{
#define Y_STEP 7
#define AVERAGEFRAMES 16
    int x = windowxy1.x+2;
    int y = windowxy1.y+2+8;
    extern short LevelSecrets,TotalKillable;

    if (CommEnabled || numplayers > 1)
        return;

    x = x / (xdim/320.0);
    y = y / (ydim/200.0);

    if (gs.Stats)
    {
        sprintf(ds, "Kills %d/%d", Player->Kills, TotalKillable);
        DisplayMiniBarSmString(pp, x, y, PAL_XLAT_BROWN, ds);

        sprintf(ds, "Secrets %d/%d", Player->SecretsFound, LevelSecrets);
        DisplayMiniBarSmString(pp, x, y+10, PAL_XLAT_BROWN, ds);
    }
}

void PrintSpriteInfo(PLAYERp pp)
{
#define Y_STEP 7
    int x = windowxy1.x+2;
    int y = windowxy1.y+2;
    SPRITEp sp;
    USERp u;

    if (SpriteInfo && !LocationInfo)
    {
        short hit_sprite = DoPickTarget(pp->SpriteP, 32, 2);

        sp = &sprite[hit_sprite];
        u = User[hit_sprite];

        sp->hitag = 9997; // Special tag to make the actor glow red for one frame

        y += Y_STEP;

        if (hit_sprite == -1)
        {
            sprintf(buffer, "SPRITENUM: NONE TARGETED");
            printext256(x, y, 1, -1, buffer, 1);
            return;
        }
        else
            sprintf(buffer, "SPRITENUM:%d", hit_sprite);

        printext256(x, y, 1, -1, buffer, 1);
        y += Y_STEP;
        if (u)
        {
            sprintf(buffer, "ID:%d", u->ID);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "PALETTE:%d", u->spal);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "HEALTH:%d", u->Health);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "WAITTICS:%d", u->WaitTics);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "COUNTER:%d", u->Counter);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "COUNTER2:%d", u->Counter);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
        }
        if (SpriteInfo > 1)
        {
            sprintf(buffer, "POSX:%d", sp->x);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "POSY:%d", sp->y);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "POSZ:%d", sp->z);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
            sprintf(buffer, "ANG:%d", (int32_t) sp->ang);
            printext256(x, y, 1, -1, buffer, 1);
            y += Y_STEP;
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
                    memcpy(&tsprite[spritesortcnt], sp, sizeof(SPRITE));
                    tsprite[spritesortcnt++].owner = i;
                }
            }
        }
    }
}

int COVERsetgamemode(int mode, int xdim, int ydim, int bpp)
{
    extern int ScreenHeight, ScreenWidth, ScreenMode, ScreenBPP;


    ScreenHeight = ydim;
    ScreenWidth  = xdim;
    ScreenMode   = mode;
    ScreenBPP    = bpp;

    return (int)setgamemode(mode,xdim,ydim,bpp);
}

void CheatResChange(void)
{
    /*
    extern char permanentupdate;
    int i;

    // needs to be called from drawscreen - crashes otherwise

    //cycle through all vesa modes, then screen-buffer mode
    if (vidoption == 1)
        {
        for(i=0;i < validmodecnt;i++)
            {
            if ((validmodexdim[i] == xdim) && (validmodeydim[i] == ydim))
                {
                if (i == validmodecnt-1)
                    COVERsetgamemode(2,320L,200L);
                else
                    COVERsetgamemode(1,validmodexdim[i+1],validmodeydim[i+1]);
                break;
                }
            }
        }
    else
        {
        if (validmodecnt > 0)
            COVERsetgamemode(1,validmodexdim[0],validmodeydim[0]);
        }

    permanentupdate = 0;

    SetupAspectRatio();
    SetRedrawScreen(Player + myconnectindex);

    sprintf(ds,"%d x %d mode selected.", xdim, ydim);
    PutStringInfo(Player + myconnectindex, ds);
    */
    PutStringInfo(Player + myconnectindex, "JonoF: Not now");
}

#if 0
void ResChange(void)
{
    extern char permanentupdate;
    int i;

    static short sw_res[5];
    static short res_ndx=0;

    // clear pages before and after res set for good measure
    for (i = 0; i < numpages; i++)
    {
        clearview(0);
        nextpage();
    }

    // needs to be called from drawscreen - crashes otherwise

    if (res_ndx == 0)
    {
        // choose resolutions for shadow warrior
        for (i=0; i < validmodecnt; i++)
        {
            if (validmodexdim[i] + validmodeydim[i] == 320 + 200)
                sw_res[res_ndx++] = i;
            else if (validmodexdim[i] + validmodeydim[i] == 640 + 400)
                sw_res[res_ndx++] = i;
            else if (validmodexdim[i] + validmodeydim[i] == 640 + 480)
                sw_res[res_ndx++] = i;
            else if (validmodexdim[i] + validmodeydim[i] == 800 + 600)
                sw_res[res_ndx++] = i;
        }
    }

    //cycle through all sw modes, then screen-buffer mode
    if (vidoption == 1)
    {
        for (i = 0; i < res_ndx; i++)
        {
            if ((validmodexdim[sw_res[i]] == xdim) && (validmodeydim[sw_res[i]] == ydim))
            {
                if (i >= res_ndx-1)
                    COVERsetgamemode(2, 320L, 200L);
                else
                    COVERsetgamemode(1, validmodexdim[sw_res[i+1]], validmodeydim[sw_res[i+1]]);
                break;
            }
        }


        // if did not find current res then reset to 320x200
        if (i >= res_ndx)
            COVERsetgamemode(2, 320L, 200L);
    }
    else
    {
        if (validmodecnt > 0)
            COVERsetgamemode(1, validmodexdim[0], validmodeydim[0]);
    }

    permanentupdate = 0;

    for (i = 0; i < numpages; i++)
    {
        clearview(0);
        nextpage();
    }

    SetupAspectRatio();
    SetRedrawScreen(Player + myconnectindex);

    sprintf(ds,"%d x %d mode selected.", xdim, ydim);
    PutStringInfo(Player + myconnectindex, ds);
}
#endif

void ScreenCaptureKeys(void)
{
    if (ConPanel)
        return;

    // screen capture
    if (KEY_PRESSED(KEYSC_F12))
    {
        KEY_PRESSED(KEYSC_F12) = 0;
        PauseAction();
        screencapture("swcpxxxx.tga", KEY_PRESSED(KEYSC_LSHIFT) | KEY_PRESSED(KEYSC_RSHIFT));
        ResumeAction();
        PutStringInfo(Player + myconnectindex, "Screen Captured");
    }
}

void DrawCheckKeys(PLAYERp pp)
{
    extern SWBOOL ResCheat;
    extern SWBOOL PauseKeySet;

    /* JonoF: Who really needs this now?
    if (KEY_PRESSED(KEYSC_F5) && !(KEY_PRESSED(KEYSC_RSHIFT) || KEY_PRESSED(KEYSC_LSHIFT) || KEY_PRESSED(KEYSC_ALT) || KEY_PRESSED(KEYSC_RALT)) && !PauseKeySet)
        {
        KEY_PRESSED(KEYSC_F5) = 0;
        ResChange();
        }
    */

    if (ResCheat)
    {
        ResCheat = FALSE;
        CheatResChange(); // allow all modes when cheating
    }

    if (ConPanel) return;

    if (!InputMode)
        ResizeView(pp);

    ScreenCaptureKeys();
}

#if 0
void DrawMessageInput(PLAYERp pp)
{
    short w,h;
    static SWBOOL cur_show;
    short c;

    // Used to make cursor fade in and out
    c = 4-(sintable[(totalclock<<4)&2047]>>11);

    if (MessageInputMode)
    {
        MNU_MeasureString(MessageInputString, &w, &h);

        cur_show ^= 1;
        if (cur_show)
        {
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,1,ROTATE_SPRITE_SCREEN_CLIP);
            rotatesprite((TEXT_XCENTER(w)+w+7)<<16,(MESSAGE_LINE+3)<<16,64<<9,0,COINCURSOR+((totalclock>>3)%7),c,0,
                         (ROTATE_SPRITE_SCREEN_CLIP),0,0,xdim-1,ydim-1);
        }
        else
        {
            MNU_DrawString(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,1,ROTATE_SPRITE_SCREEN_CLIP);
            rotatesprite((TEXT_XCENTER(w)+w+7)<<16,(MESSAGE_LINE+3)<<16,64<<9,0,COINCURSOR+((totalclock>>3)%7),c,0,
                         (ROTATE_SPRITE_SCREEN_CLIP),0,0,xdim-1,ydim-1);
        }
    }
}
#else
void DrawMessageInput(PLAYERp pp)
{
    short w,h;
    static SWBOOL cur_show;
    short c;

    // Used to make cursor fade in and out
    c = 4-(sintable[(totalclock<<4)&2047]>>11);

    if (MessageInputMode)
    {
        MNU_MeasureSmallString(MessageInputString, &w, &h);

        cur_show ^= 1;
        if (cur_show)
        {
            minigametext(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,0,ROTATE_SPRITE_SCREEN_CLIP);
            rotatesprite((TEXT_XCENTER(w)+w+2)<<16,(MESSAGE_LINE+1)<<16,20000,0,COINCURSOR+((totalclock>>3)%7),c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
        }
        else
        {
            minigametext(TEXT_XCENTER(w), MESSAGE_LINE, MessageInputString,0,ROTATE_SPRITE_SCREEN_CLIP);
            rotatesprite((TEXT_XCENTER(w)+w+2)<<16,(MESSAGE_LINE+1)<<16,20000,0,COINCURSOR+((totalclock>>3)%7),c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
        }
    }
}
#endif

void DrawConInput(PLAYERp pp)
{
#define PANELINPUTX 30
#define PANELINPUTY 100
    short w,h;
    static SWBOOL cur_show;
    short c;

    // Used to make cursor fade in and out
    c = 4-(sintable[(totalclock<<4)&2047]>>11);

    if (ConInputMode)
    {
        MNU_MeasureSmallString(MessageInputString, &w, &h);

        cur_show ^= 1;
        if (cur_show)
        {
            MNU_DrawSmallString(PANELINPUTX, PANELINPUTY, MessageInputString,1,17);
            rotatesprite((PANELINPUTX+w+1)<<16,(PANELINPUTY)<<16,65536L,0,2992,c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
            //rotatesprite((PANELINPUTX+w+3)<<16,(PANELINPUTY)<<16,64<<8,0,COINCURSOR+((totalclock>>3)%7),c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
        }
        else
        {
            MNU_DrawSmallString(PANELINPUTX, PANELINPUTY, MessageInputString,1,17);
            rotatesprite((PANELINPUTX+w+1)<<16,(PANELINPUTY)<<16,65536L,0,2992,c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
            //rotatesprite((PANELINPUTX+w+3)<<16,(PANELINPUTY)<<16,64<<8,0,COINCURSOR+((totalclock>>3)%7),c,0,ROTATE_SPRITE_SCREEN_CLIP,0,0,xdim-1,ydim-1);
        }
    }
}

void DrawCrosshair(PLAYERp pp)
{
    extern int CrosshairX, CrosshairY;
    extern SWBOOL DemoMode,CameraTestMode;

    if (!gs.Crosshair)
        return;

    if (DemoMode || CameraTestMode)
        return;

    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
        return;

    if (dimensionmode == 6)
        return;

//  wdx = 160;
//  wdy = 100;
#if 0
    if (gs.AutoAim)
    {
        int daz;
        short hit_sprite, daang;
        static int handle=-1;

        daz = pp->posz + pp->bob_z;
        daang = 32;
        if ((hit_sprite = WeaponAutoAimHitscan(pp->SpriteP, &daz, &daang, FALSE)) != -1)
        {
            SPRITEp hp = &sprite[hit_sprite];
            USERp hu = User[hit_sprite];
            vec2_t const zero = { 0, 0 };

            // Find the delta coordinates from player to monster that is targeted
            vec2_t dxy = { hp->x - pp->posx, hp->y - pp->posy };
            int dz = ((hp->z - (SPRITE_SIZE_Z(hit_sprite)/2)) - pp->posz) >> 4;

            rotatepoint(zero, dxy, (-pp->pang)&2047, &dxy);

            if (dxy.x == 0) return;

            wdx = windowxy1.x + ((windowxy2.x-windowxy1.x)/2);
            wdy = windowxy1.y + ((windowxy2.y-windowxy1.y)/2);

            x = (dxy.y * wdx << 8) / dxy.x + (wdx << 8);
            y = (dz * wdx << 8) / dxy.x + (wdy << 8);

            y -= 100;
            y += (pp->horiz*wdx)/160;

            if (pp->CurWpn == pp->Wpn[WPN_RAIL])
            {
                if (!FX_SoundActive(handle))
                    handle = PlaySound(DIGI_RAILLOCKED, &pp->posx, &pp->posy, &pp->posz, v3df_follow|v3df_dontpan);
            }
        }
        else
        {
            // It didn't target anything.
            if (pp->CurWpn == pp->Wpn[WPN_RAIL])
            {
                if (FX_SoundActive(handle))
                    FX_StopSound(handle);
            }
            goto NORMALXHAIR;
        }

        rotatesprite(x << 8, y << 8, (1 << 16), 0,
                     2326, 10, 0,
                     ROTATE_SPRITE_VIEW_CLIP|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
    }
    else
#endif
    {
//NORMALXHAIR:
        rotatesprite(CrosshairX, CrosshairY, (1 << 16), 0,
                     2326, 10, 0,
                     //ROTATE_SPRITE_VIEW_CLIP|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
                     ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
    }

    //#define TITLE_ROT_FLAGS (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK)

}

void CameraView(PLAYERp pp, int *tx, int *ty, int *tz, short *tsectnum, short *tang, int *thoriz)
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
                    CircleCamera(tx, ty, tz, tsectnum, tang, 100);
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
                    *thoriz = 100 - (zvect/256);
                    *thoriz = max(*thoriz, PLAYER_HORIZ_MIN);
                    *thoriz = min(*thoriz, PLAYER_HORIZ_MAX);

                    //DSPRINTF(ds,"xvect %d,yvect %d,zvect %d,thoriz %d",xvect,yvect,zvect,*thoriz);
                    MONO_PRINT(ds);

                    *tang = ang;
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
    short sectnum,statnum;
    SPRITEp sp;

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
        sp = &sprite[i];
        statnum = sp->statnum;
        sectnum = sp->sectnum;
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
    SPRITEp sp,np;
    USERp u,nu;
    short New;
    int smr4,smr2;
    int x,y,z;
    short ang;
    PLAYERp pp = Player + screenpeek;

    smr4 = smoothratio + (((int) MoveSkip4) << 16);
    smr2 = smoothratio + (((int) MoveSkip2) << 16);

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
                    np = &sprite[New];

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


void FAF_DrawRooms(int x, int y, int z, short ang, int horiz, short sectnum)
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

    drawrooms(x,y,z,ang,horiz,sectnum);

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

void
drawscreen(PLAYERp pp)
{
    extern SWBOOL DemoMode,CameraTestMode;
    int tx, ty, tz,thoriz,pp_siz;
    short tang,tsectnum;
    short i,j;
    walltype *wal;
    int tiltlock;
    int bob_amt = 0;
    int quake_z, quake_x, quake_y;
    short quake_ang;
    SWBOOL PicInView(short, SWBOOL);
    extern SWBOOL FAF_DebugView;
    PLAYERp camerapp;                       // prediction player if prediction is on, else regular player
    void DoPlayerDiveMeter(PLAYERp pp);

    // last valid stuff
    static short lv_sectnum = -1;
    static int lv_x, lv_y, lv_z;

    if (HelpInputMode)
    {
        flushperms();
        // note - could put Order Info Pages at the top like this also

        rotatesprite(0,0,65536L,0,HelpPagePic[HelpPage],0,0,
                     (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK|ROTATE_SPRITE_IGNORE_START_MOST),
                     0, 0, xdim-1, ydim-1);
        nextpage();

        return;
    }

#if 0
    if (TenScreenMode)
    {
#define TEN_PIC 5109

        flushperms();
        // note - could put Order Info Pages at the top like this also
        rotatesprite(0,0,65536L,0,TEN_PIC,0,0,
                     (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK|ROTATE_SPRITE_IGNORE_START_MOST),
                     0, 0, xdim-1, ydim-1);

        nextpage();
        return;
    }
#endif

    if (getrendermode() >= 3)
        RedrawScreen = TRUE;

    DrawScreen = TRUE;
    PreDraw();
    // part of new border refresh method
    if (RedrawScreen)
    {
        RedrawCompass = TRUE;
        RedrawScreen = FALSE;
        // get rid of all PERM sprites!
        flushperms();
        // get rid of all PANF_KILL_AFTER_SHOW sprites!
        pFlushPerms(pp);
        SetBorder(pp,gs.BorderNum);
    }

    PreUpdatePanel();


    smoothratio = min(max((totalclock - ototalclock) * (65536 / synctics),0),65536);

    if (!ScreenSavePic)
    {
        dointerpolations(smoothratio);                      // Stick at beginning of drawscreen
        short_dointerpolations(smoothratio);                      // Stick at beginning of drawscreen
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
    tang = camerapp->oang + mulscale16(((camerapp->pang + 1024 - camerapp->oang) & 2047) - 1024, smoothratio);
    thoriz = camerapp->ohoriz + mulscale16(camerapp->horiz - camerapp->ohoriz, smoothratio);
    tsectnum = camerapp->cursectnum;

    //ASSERT(tsectnum >= 0 && tsectnum <= MAXSECTORS);
    // if updatesectorz is to sensitive try COVERupdatesector
    //updatesectorz(tx, ty, tz, &tsectnum);

    COVERupdatesector(tx, ty, &tsectnum);

    if (tsectnum < 0)
    {
        // if we hit an invalid sector move to the last valid position for drawing
        tsectnum = lv_sectnum;
        tx = lv_x;
        ty = lv_y;
        tz = lv_z;
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
    ASSERT(tsectnum >= 0 && tsectnum <= MAXSECTORS);

    pp->six = tx;
    pp->siy = ty;
    pp->siz = tz - pp->posz;
    pp_siz = tz;
    pp->siang = tang;

    if (pp->sop_riding || pp->sop_control)
    {
        tx = pp->posx;
        ty = pp->posy;
        tz = pp->posz;
        tang = pp->pang;
        tsectnum = pp->cursectnum;
        updatesectorz(tx, ty, tz, &tsectnum);

        pp->six = tx;
        pp->siy = ty;
        pp->siz = tz - pp->posz;
        pp->siang = tang;
    }

    QuakeViewChange(camerapp, &quake_z, &quake_x, &quake_y, &quake_ang);
    VisViewChange(camerapp, &g_visibility);
    tz = tz + quake_z;
    tx = tx + quake_x;
    ty = ty + quake_y;
    //thoriz = thoriz + quake_x;
    tang = NORM_ANGLE(tang + quake_ang);

    if (pp->sop_remote)
    {
        if (TEST_BOOL1(pp->remote_sprite))
            tang = pp->remote_sprite->ang;
        else
            tang = getangle(pp->sop_remote->xmid - tx, pp->sop_remote->ymid - ty);
    }

    //if (TEST(camerapp->Flags, PF_VIEW_FROM_OUTSIDE))
    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        BackView(&tx, &ty, &tz, &tsectnum, &tang, thoriz);
    }
    else
    {
        bob_amt = camerapp->bob_amt;

        if (DemoMode || CameraTestMode)
            CameraView(camerapp, &tx, &ty, &tz, &tsectnum, &tang, &thoriz);
    }

    if (!TEST(pp->Flags, PF_VIEW_FROM_CAMERA|PF_VIEW_FROM_OUTSIDE))
    {
        tz += bob_amt;
        tz += camerapp->bob_z;

        // recoil only when not in camera
        //thoriz = thoriz + camerapp->recoil_horizoff;
        thoriz = thoriz + pp->recoil_horizoff;
        thoriz = max(thoriz, PLAYER_HORIZ_MIN);
        thoriz = min(thoriz, PLAYER_HORIZ_MAX);
    }

    if (FAF_DebugView)
        clearview(255);

    OverlapDraw = TRUE;
    DrawOverlapRoom(tx, ty, tz, tang, thoriz, tsectnum);
    OverlapDraw = FALSE;

    if (dimensionmode != 6 && !ScreenSavePic)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tang, thoriz);
        JS_DrawMirrors(pp, tx, ty, tz, tang, thoriz);
    }

    if (!FAF_DebugView)
    {
        FAF_DrawRooms(tx, ty, tz, tang, thoriz, tsectnum);
        PicInView(FAF_MIRROR_PIC, FALSE);
    }

    analyzesprites(tx, ty, tz, FALSE);
    post_analyzesprites();
    drawmasks();

    UpdatePanel();

#define SLIME 2305
    // Only animate lava if its picnum is on screen
    // gotpic is a bit array where the tile number's bit is set
    // whenever it is drawn (ceilings, walls, sprites, etc.)
    if ((gotpic[SLIME >> 3] & (1 << (SLIME & 7))) > 0)
    {
        gotpic[SLIME >> 3] &= ~(1 << (SLIME & 7));

        if (waloff[SLIME])
            movelava((char *) waloff[SLIME]);
    }


    i = pp->cursectnum;

    show2dsector[i>>3] |= (1<<(i&7));
    wal = &wall[sector[i].wallptr];
    for (j=sector[i].wallnum; j>0; j--,wal++)
    {
        i = wal->nextsector;
        if (i < 0) continue;
        if (wal->cstat&0x0071) continue;
        if (wall[wal->nextwall].cstat&0x0071) continue;
        if (sector[i].lotag == 32767) continue;
        if (sector[i].ceilingz >= sector[i].floorz) continue;
        show2dsector[i>>3] |= (1<<(i&7));
    }

    if ((dimensionmode == 5 || dimensionmode == 6) && pp == Player+myconnectindex)
    {
        void MoveScrollMode2D(PLAYERp pp);
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
                if (TEST(sprite[j].cstat, CSTAT_SPRITE_FLOOR))
                {
                    RESET(sprite[j].cstat, CSTAT_SPRITE_FLOOR);
                    sprite[j].owner = -2;
                }
            }
        }

        if (dimensionmode == 6)
        {
            clearview(0L);
            drawmapview(tx, ty, zoom, tang);
        }

        // Draw the line map on top of texture 2d map or just stand alone
        drawoverheadmap(tx, ty, zoom, tang);
    }

    for (j = 0; j < MAXSPRITES; j++)
    {
        // Don't show sprites tagged with 257
        if (sprite[j].lotag == 257 && sprite[j].owner == -2)
            SET(sprite[j].cstat, CSTAT_SPRITE_FLOOR);
    }

    // if doing a screen save don't need to process the rest
    if (ScreenSavePic)
    {
        DrawScreen = FALSE;
        return;
    }

    PrintLocationInfo(pp);
    PrintSpriteInfo(pp);

#if SYNC_TEST
    SyncStatMessage();
#endif

    DrawCrosshair(pp);


    operatefta();           // Draw all the user quotes in the quote array

    operateconfta();        // Draw messages in the console

    DoPlayerDiveMeter(pp); // Do the underwater breathing bar

    // Boss Health Meter, if Boss present
    BossHealthMeter();

    if (ConInputMode)
    {
        DrawConInput(pp);   // Console panel input mode
    }
    else
        DrawMessageInput(pp);   // This is only used for non-multiplayer input now

    DrawCompass(pp);
    UpdateMiniBar(pp);

    if (UsingMenus)
        MNU_DrawMenu();
    else
        SecretInfo(pp);

    nextpage();

#if SYNC_TEST
    SyncStatMessage();
#endif

    // certain input is done here - probably shouldn't be
    DrawCheckKeys(pp);

    restoreinterpolations();                 // Stick at end of drawscreen
    short_restoreinterpolations();                 // Stick at end of drawscreen

    PostDraw();
    DrawScreen = FALSE;
}

void
DrawCompass(PLAYERp pp)
{
#define COMPASS_TIC     2380
#define COMPASS_TIC2    2381

#define COMPASS_NORTH   2382
#define COMPASS_NORTH2  2383

#define COMPASS_SOUTH   2384
#define COMPASS_SOUTH2  2385

#define COMPASS_EAST    2386
#define COMPASS_EAST2   2387

#define COMPASS_WEST    2388
#define COMPASS_WEST2   2389

#define COMPASS_MID_TIC    2390
#define COMPASS_MID_TIC2   2391

#define COMPASS_X   140
#define COMPASS_Y   (162-5)

#define NORM_CANG(ang) (((ang) + 32) & 31)

    short start_ang,ang;
    short x_size = tilesiz[COMPASS_NORTH].x;
    short x;
    short i;
    int flags;
    PANEL_SPRITEp psp;

    static short CompassPic[32] =
    {
        COMPASS_EAST, COMPASS_EAST2,
        COMPASS_TIC, COMPASS_TIC2,
        COMPASS_MID_TIC, COMPASS_MID_TIC2,
        COMPASS_TIC, COMPASS_TIC2,

        COMPASS_SOUTH, COMPASS_SOUTH2,
        COMPASS_TIC, COMPASS_TIC2,
        COMPASS_MID_TIC, COMPASS_MID_TIC2,
        COMPASS_TIC, COMPASS_TIC2,

        COMPASS_WEST, COMPASS_WEST2,
        COMPASS_TIC, COMPASS_TIC2,
        COMPASS_MID_TIC, COMPASS_MID_TIC2,
        COMPASS_TIC, COMPASS_TIC2,

        COMPASS_NORTH, COMPASS_NORTH2,
        COMPASS_TIC, COMPASS_TIC2,
        COMPASS_MID_TIC, COMPASS_MID_TIC2,
        COMPASS_TIC, COMPASS_TIC2,
    };

    static short CompassShade[10] =
    {
        //20, 16, 11, 6, 1, 1, 6, 11, 16, 20
        25, 19, 15, 9, 1, 1, 9, 15, 19, 25
    };

    extern SWBOOL PanelUpdateMode;

    if (!PanelUpdateMode)
        return;

    if (gs.BorderNum < BORDER_BAR || pp - Player != screenpeek)
        return;

    ang = pp->pang;

    if (pp->sop_remote)
        ang = 0;

    start_ang = (ang + 32) >> 6;

    start_ang = NORM_CANG(start_ang - 4);

    flags = ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER;
    if (RedrawCompass)
    {
        RedrawCompass = FALSE;
        SET(flags, ROTATE_SPRITE_ALL_PAGES);
    }

    for (i = 0, x = COMPASS_X; i < 10; i++)
    {
        rotatesprite(x << 16, COMPASS_Y << 16, (1 << 16), 0,
                     CompassPic[NORM_CANG(start_ang + i)], CompassShade[i], 0,
                     flags, 0, 0, xdim - 1, ydim - 1);
        x += x_size;
    }
}


void ScreenTileLock(void)
{
    walock[SAVE_SCREEN_TILE] = CACHE_LOCK_MAX;
}

void ScreenTileUnLock(void)
{
    walock[SAVE_SCREEN_TILE] = CACHE_UNLOCK_MAX;
}

int
ScreenLoadSaveSetup(PLAYERp pp)
{
    int tx, ty, tz,thoriz,pp_siz;
    short tang,tsectnum;
    short i;

    // lock and allocate memory

    ScreenTileLock();

    if (!waloff[SAVE_SCREEN_TILE])
        allocache((intptr_t*)&waloff[SAVE_SCREEN_TILE], SAVE_SCREEN_XSIZE * SAVE_SCREEN_YSIZE, &walock[SAVE_SCREEN_TILE]);

    tilesiz[SAVE_SCREEN_TILE].x = SAVE_SCREEN_XSIZE;
    tilesiz[SAVE_SCREEN_TILE].x = SAVE_SCREEN_YSIZE;

    return SAVE_SCREEN_TILE;
}

int
ScreenSaveSetup(PLAYERp pp)
{
    short i;

    ScreenLoadSaveSetup(Player + myconnectindex);

    setviewtotile(SAVE_SCREEN_TILE, SAVE_SCREEN_YSIZE, SAVE_SCREEN_XSIZE);

    ScreenSavePic = TRUE;
    drawscreen(Player + myconnectindex);
    ScreenSavePic = FALSE;

    setviewback();

    return SAVE_SCREEN_TILE;
}




