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
#include "automap.h"
#include "pragmas.h"


#include "names2.h"
#include "panel.h"
#include "game.h"

#include "jsector.h"

#include "mytypes.h"
#include "gamecontrol.h"
#include "network.h"
#include "pal.h"
#include "player.h"
#include "jtags.h"
#include "parent.h"

#include "misc.h"

#include "menus.h"
#include "interp.h"
#include "interpso.h"
#include "sector.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "glbackend/glbackend.h"

BEGIN_SW_NS

static int OverlapDraw = false;
extern bool QuitFlag, SpriteInfo;
extern bool Voxel;
extern char buffer[];
bool DrawScreen;
extern short f_c;

extern ParentalStruct aVoxelArray[MAXTILES];

int ConnectCopySprite(uspritetype const * tsp);
void PreDrawStackedWater(void);

void SW_InitMultiPsky(void)
{
    // default
    psky_t* const defaultsky = tileSetupSky(DEFAULTPSKY);
    defaultsky->lognumtiles = 1;
    defaultsky->horizfrac = 8192;
}


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
        ASSERT(true == false);
        break;
    }

    return loz;
}

void
DoShadows(tspriteptr_t tsp, int viewz, bool mirror)
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
        z_amt_per_pixel = IntToFixed((int)-tu->jump_speed * ACTORMOVETICS)/tsp->xvel;
    }
    else
    {
        z_amt_per_pixel = IntToFixed((int)-tsp->zvel)/tsp->xvel;
    }

    switch (tu->motion_blur_dist)
    {
    case 64:
        dx = nx = MOVEx(64, ang);
        dy = ny = MOVEy(64, ang);
        nz = FixedToInt(z_amt_per_pixel * 64);
        break;
    case 128:
        dx = nx = MOVEx(128, ang);
        dy = ny = MOVEy(128, ang);
        nz = FixedToInt(z_amt_per_pixel * 128);
        break;
    case 256:
        dx = nx = MOVEx(256, ang);
        dy = ny = MOVEy(256, ang);
        nz = FixedToInt(z_amt_per_pixel * 256);
        break;
    case 512:
        dx = nx = MOVEx(512, ang);
        dy = ny = MOVEy(512, ang);
        nz = FixedToInt(z_amt_per_pixel * 512);
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
analyzesprites(int viewx, int viewy, int viewz, bool mirror)
{
    int tSpriteNum;
    short SpriteNum;
    int smr4, smr2;
    USERp tu;
    static int ang = 0;
    PLAYERp pp = Player + screenpeek;
    short newshade=0;

    const int DART_PIC = 2526;
    const int DART_REPEAT = 16;

    ang = NORM_ANGLE(ang + 12);

    smr4 = smoothratio + IntToFixed(MoveSkip4);
    smr2 = smoothratio + IntToFixed(MoveSkip2);

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        SpriteNum = tsprite[tSpriteNum].owner;
        tspriteptr_t tsp = &tsprite[tSpriteNum];
        tu = User[SpriteNum];

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
        if (adult_lockout)
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
                    bool nosectpal=false;

                    // sprite does not take on the new pal if sector flag is set
                    if (sectu && TEST(sectu->flags, SECTFU_DONT_COPY_PALETTE))
                    {
                        pal = PALETTE_DEFAULT;
                        nosectpal = true;
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
            else // Otherwise just interpolate the player sprite
            {
                PLAYERp pp = tu->PlayerP;
                tsp->x -= mulscale16(pp->posx - pp->oposx, 65536-smoothratio);
                tsp->y -= mulscale16(pp->posy - pp->oposy, 65536-smoothratio);
                tsp->z -= mulscale16(pp->posz - pp->oposz, 65536-smoothratio);
                tsp->ang -= FixedToInt(mulscale16(pp->q16ang - pp->oq16ang, 65536-smoothratio));
            }
        }

        if (OverlapDraw && FAF_ConnectArea(tsp->sectnum) && tsp->owner >= 0)
        {
            static_assert(sizeof(uspritetype) == sizeof(tspritetype)); // see TSPRITE_SIZE
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


void
BackView(int *nx, int *ny, int *nz, short *vsect, fixed_t *nq16ang, short horiz)
{
    vec3_t n = { *nx, *ny, *nz };
    SPRITEp sp;
    hitdata_t hitinfo;
    int i, vx, vy, vz, hx, hy;
    short bakcstat, daang;
    PLAYERp pp = &Player[screenpeek];
    short ang;

    ASSERT(*vsect >= 0 && *vsect < MAXSECTORS);

    ang = FixedToInt(*nq16ang) + pp->view_outside_dang;

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
            i = IntToFixed(hx) / vx;
        else
            i = IntToFixed(hy) / vy;

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

    *nq16ang = IntToFixed(ang);
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

    ang = FixedToInt(*nq16ang) + pp->circle_camera_ang;

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
            i = IntToFixed(hx) / vx;
        else
            i = IntToFixed(hy) / vy;

        if (i < pp->circle_camera_dist)
            pp->circle_camera_dist = i;
    }

    // Actually move you!  (Camerdist is 65536 if nothing is in the way)
    *nx = (*nx) + FixedToInt(vx * pp->circle_camera_dist);
    *ny = (*ny) + FixedToInt(vy * pp->circle_camera_dist);
    *nz = (*nz) + FixedToInt(vz * pp->circle_camera_dist);

    // Slowly increase pp->circle_camera_dist until it reaches 65536
    // Synctics is a timer variable so it increases the same rate
    // on all speed computers
    pp->circle_camera_dist = min(pp->circle_camera_dist + (3 << 8), 65536);
    //pp->circle_camera_dist = min(pp->circle_camera_dist + (synctics << 10), 65536);

    // Make sure vsect is correct
    updatesectorz(*nx, *ny, *nz, vsect);

    *nq16ang = IntToFixed(ang);
}

FString GameInterface::GetCoordString()
{
    PLAYERp pp = Player + myconnectindex;
    FString out;
    out.AppendFormat("POSX:%d ", pp->posx);
    out.AppendFormat("POSY:%d ", pp->posy);
    out.AppendFormat("POSZ:%d ", pp->posz);
    out.AppendFormat("ANG:%d\n", FixedToInt(pp->q16ang));

    return out;
}


void PrintSpriteInfo(PLAYERp pp)
{
    const int Y_STEP = 7;
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

void DrawCrosshair(PLAYERp pp)
{
    extern bool CameraTestMode;

    if (!(CameraTestMode) && !TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        USERp u = User[pp->PlayerSprite];
        ::DrawCrosshair(2326, u->Health, 0, 0, 2, shadeToLight(10));
    }
}

void CameraView(PLAYERp pp, int *tx, int *ty, int *tz, short *tsectnum, fixed_t *tq16ang, fixed_t *tq16horiz)
{
    int i,nexti;
    short ang;
    SPRITEp sp;
    bool found_camera = false;
    bool player_in_camera = false;
    bool FAFcansee_test;
    bool ang_test;

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
                    found_camera = true;
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
                    *tq16horiz = IntToFixed(100 - (zvect/256));
                    *tq16horiz = max(*tq16horiz, IntToFixed(PLAYER_HORIZ_MIN));
                    *tq16horiz = min(*tq16horiz, IntToFixed(PLAYER_HORIZ_MAX));

                    //DSPRINTF(ds,"xvect %d,yvect %d,zvect %d,tq16horiz %d",xvect,yvect,zvect,*tq16horiz);
                    MONO_PRINT(ds);

                    *tq16ang = IntToFixed(ang);
                    *tx = sp->x;
                    *ty = sp->y;
                    *tz = sp->z;
                    *tsectnum = sp->sectnum;

                    found_camera = true;
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


void FAF_DrawRooms(int x, int y, int z, fixed_t q16ang, fixed_t q16horiz, short sectnum)
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

short ScreenSavePic = false;

bool PicInView(short, bool);
void DoPlayerDiveMeter(PLAYERp pp);

void
drawscreen(PLAYERp pp, double smoothratio)
{
    extern bool CameraTestMode;
    int tx, ty, tz;
    fixed_t tq16horiz, tq16ang;
    short tsectnum;
    short i,j;
    int bob_amt = 0;
    int quake_z, quake_x, quake_y;
    short quake_ang;
    extern bool FAF_DebugView;
    PLAYERp camerapp;                       // prediction player if prediction is on, else regular player

    // last valid stuff
    static short lv_sectnum = -1;
    static int lv_x, lv_y, lv_z;

    int const viewingRange = viewingrange;

    DrawScreen = true;
    PreDraw();

    PreUpdatePanel(smoothratio);

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

    tx = camerapp->oposx + xs_CRoundToInt(fmulscale16(camerapp->posx - camerapp->oposx, smoothratio));
    ty = camerapp->oposy + xs_CRoundToInt(fmulscale16(camerapp->posy - camerapp->oposy, smoothratio));
    tz = camerapp->oposz + xs_CRoundToInt(fmulscale16(camerapp->posz - camerapp->oposz, smoothratio));

    // Interpolate the player's angle while on a sector object, just like VoidSW.
    // This isn't needed for the turret as it was fixable, but moving sector objects are problematic.
    if (cl_syncinput || pp != Player+myconnectindex || (!cl_syncinput && pp->sop && !TEST(pp->Flags2, PF2_INPUT_CAN_TURN_TURRET)))
    {
        tq16ang = camerapp->oq16ang + xs_CRoundToInt(fmulscale16(NORM_Q16ANGLE(camerapp->q16ang + IntToFixed(1024) - camerapp->oq16ang) - IntToFixed(1024), smoothratio));
        tq16horiz = camerapp->oq16horiz + xs_CRoundToInt(fmulscale16(camerapp->q16horiz - camerapp->oq16horiz, smoothratio));
    }
    else
    {
        tq16ang = pp->q16ang;
        tq16horiz = pp->q16horiz;
    }
    tsectnum = camerapp->cursectnum;

    COVERupdatesector(tx, ty, &tsectnum);

    if (tsectnum >= 0)
    {
        // last valid stuff
        lv_sectnum = tsectnum;
        lv_x = tx;
        lv_y = ty;
        lv_z = tz;
    }

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
    pp->siang = FixedToInt(tq16ang);

    QuakeViewChange(camerapp, &quake_z, &quake_x, &quake_y, &quake_ang);
    VisViewChange(camerapp, &g_visibility);
    tz = tz + quake_z;
    tx = tx + quake_x;
    ty = ty + quake_y;
    //tq16horiz = tq16horiz + IntToFixed(quake_x);
    tq16ang = NORM_Q16ANGLE(tq16ang + IntToFixed(quake_ang));

    if (pp->sop_remote)
    {
        if (TEST_BOOL1(pp->remote_sprite))
            tq16ang = IntToFixed(pp->remote_sprite->ang);
        else
            tq16ang = gethiq16angle(pp->sop_remote->xmid - tx, pp->sop_remote->ymid - ty);
    }

    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        BackView(&tx, &ty, &tz, &tsectnum, &tq16ang, FixedToInt(tq16horiz));
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
        if (cl_viewbob)
        {
            tz += bob_amt;
            tz += pp->obob_z + xs_CRoundToInt(fmulscale16(pp->bob_z - pp->obob_z, smoothratio));
        }

        // recoil only when not in camera
        tq16horiz = tq16horiz + pp->recoil_horizoff;
        tq16horiz = max(tq16horiz, IntToFixed(PLAYER_HORIZ_MIN));
        tq16horiz = min(tq16horiz, IntToFixed(PLAYER_HORIZ_MAX));
    }

    if (automapMode != am_full)// && !ScreenSavePic)
    {
        // Cameras must be done before the main loop.
        JS_DrawCameras(pp, tx, ty, tz);
    }


    videoSetCorrectedAspect();
    renderSetAspect(xs_CRoundToInt(double(viewingrange)* tan(r_fov* (PI / 360.))), yxaspect);
    OverlapDraw = true;
    DrawOverlapRoom(tx, ty, tz, tq16ang, tq16horiz, tsectnum);
    OverlapDraw = false;

    if (automapMode != am_full)// && !ScreenSavePic)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tq16ang, tq16horiz);
        JS_DrawMirrors(pp, tx, ty, tz, tq16ang, tq16horiz);
    }

    // TODO: This call is redundant if the tiled overhead map is shown, but the
    // HUD elements should be properly outputted with hardware rendering first.
    if (!FAF_DebugView)
        FAF_DrawRooms(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

    analyzesprites(tx, ty, tz, false);
    post_analyzesprites();
    renderDrawMasks();


    renderSetAspect(viewingRange, divscale16(ydim * 8, xdim * 5));
    if (!ScreenSavePic) UpdatePanel(smoothratio);

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

    // if doing a screen save don't need to process the rest
    if (ScreenSavePic)
    {
        DrawScreen = false;
        return;
    }


    MarkSectorSeen(pp->cursectnum);

    if ((automapMode != am_off) && pp == Player+myconnectindex)
    {
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
        DrawOverheadMap(tx, ty, FixedToInt(tq16ang));
    }

    for (j = 0; j < MAXSPRITES; j++)
    {
        // Don't show sprites tagged with 257
        if (sprite[j].lotag == 257 && sprite[j].owner == -2)
            SET(sprite[j].cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR);
    }


    //PrintLocationInfo(pp);
    //PrintSpriteInfo(pp);

#if SYNC_TEST
    SyncStatMessage();
#endif

    UpdateStatusBar();
    DrawCrosshair(pp);
    DoPlayerDiveMeter(pp); // Do the underwater breathing bar

    // Boss Health Meter, if Boss present
    BossHealthMeter();

#if SYNC_TEST
    SyncStatMessage();
#endif

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
            ReloadPrompt = false;
        }
    }

    PostDraw();
    DrawScreen = false;
}

bool GameInterface::GenerateSavePic()
{
    ScreenSavePic = true;
    drawscreen(Player + myconnectindex, 65536);
    ScreenSavePic = false;
    return true;
}




bool GameInterface::DrawAutomapPlayer(int cposx, int cposy, int czoom, int cang)
{
    int i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect, xvect2, yvect2;
    walltype* wal, * wal2;
    spritetype* spr;
    short p;
    static int pspr_ndx[8] = { 0,0,0,0,0,0,0,0 };
    bool sprisplayer = false;
    short txt_x, txt_y;

    xvect = sintable[(2048 - cang) & 2047] * czoom;
    yvect = sintable[(1536 - cang) & 2047] * czoom;
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);


    // Draw sprites
    k = Player[screenpeek].PlayerSprite;
    for (i = 0; i < numsectors; i++)
        for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
        {
            for (p = connecthead; p >= 0; p = connectpoint2[p])
            {
                if (Player[p].PlayerSprite == j)
                {
                    if (sprite[Player[p].PlayerSprite].xvel > 16)
                        pspr_ndx[myconnectindex] = ((PlayClock >> 4) & 3);
                    sprisplayer = true;

                    goto SHOWSPRITE;
                }
            }
            if (gFullMap || show2dsprite[j])
            {
            SHOWSPRITE:
                spr = &sprite[j];

                PalEntry col = GPalette.BaseColors[56]; // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
                if ((spr->cstat & 1) > 0)
                    col = GPalette.BaseColors[248];
                if (j == k)
                    col = GPalette.BaseColors[31];

                sprx = spr->x;
                spry = spr->y;

                k = spr->statnum;
                if ((k >= 1) && (k <= 8) && (k != 2))   // Interpolate moving
                {
                    sprx = sprite[j].x;
                    spry = sprite[j].y;
                }

                switch (spr->cstat & 48)
                {
                case 0:  // Regular sprite
                    if (Player[p].PlayerSprite == j)
                    {
                        ox = sprx - cposx;
                        oy = spry - cposy;
                        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        if (((gotsector[i >> 3] & (1 << (i & 7))) > 0) && (czoom > 192))
                        {
                            daang = (spr->ang - cang) & 2047;

                            // Special case tiles
                            if (spr->picnum == 3123) break;

                            int spnum = -1;
                            if (sprisplayer)
                            {
                                if (gNet.MultiGameType != MULTI_GAME_COMMBAT || j == Player[screenpeek].PlayerSprite)
                                    spnum = 1196 + pspr_ndx[myconnectindex];
                            }
                            else spnum = spr->picnum;

                            double xd = ((x1 << 4) + (xdim << 15)) / 65536.;
                            double yd = ((y1 << 4) + (ydim << 15)) / 65536.;
                            double sc = mulscale16(czoom * (spr->yrepeat), yxaspect) / 32768.;
                            if (spnum >= 0)
                            {
                                DrawTexture(twod, tileGetTexture(1196 + pspr_ndx[myconnectindex], true), xd, yd, DTA_ScaleX, sc, DTA_ScaleY, sc, DTA_Rotate, daang * (-360. / 2048),
                                    DTA_CenterOffsetRel, true, DTA_TranslationIndex, TRANSLATION(Translation_Remap, spr->pal), DTA_Color, shadeToLight(spr->shade),
                                    DTA_Alpha, (spr->cstat & 2) ? 0.33 : 1., TAG_DONE);
                            }
                        }
                    }
                    break;
                case 16: // Rotated sprite
                    x1 = sprx;
                    y1 = spry;
                    tilenum = spr->picnum;
                    xoff = (int)tileLeftOffset(tilenum) + (int)spr->xoffset;
                    if ((spr->cstat & 4) > 0)
                        xoff = -xoff;
                    k = spr->ang;
                    l = spr->xrepeat;
                    dax = sintable[k & 2047] * l;
                    day = sintable[(k + 1536) & 2047] * l;
                    l = tilesiz[tilenum].x;
                    k = (l >> 1) + xoff;
                    x1 -= mulscale16(dax, k);
                    x2 = x1 + mulscale16(dax, l);
                    y1 -= mulscale16(day, k);
                    y2 = y1 + mulscale16(day, l);

                    ox = x1 - cposx;
                    oy = y1 - cposy;
                    x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                    y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                    ox = x2 - cposx;
                    oy = y2 - cposy;
                    x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                    y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                    drawlinergb(x1 + (xdim << 11), y1 + (ydim << 11),
                        x2 + (xdim << 11), y2 + (ydim << 11), col);

                    break;
                case 32:    // Floor sprite
                    if (automapMode == am_overlay)
                    {
                        tilenum = spr->picnum;
                        xoff = (int)tileLeftOffset(tilenum) + (int)spr->xoffset;
                        yoff = (int)tileTopOffset(tilenum) + (int)spr->yoffset;
                        if ((spr->cstat & 4) > 0)
                            xoff = -xoff;
                        if ((spr->cstat & 8) > 0)
                            yoff = -yoff;

                        k = spr->ang;
                        cosang = sintable[(k + 512) & 2047];
                        sinang = sintable[k];
                        xspan = tilesiz[tilenum].x;
                        xrepeat = spr->xrepeat;
                        yspan = tilesiz[tilenum].y;
                        yrepeat = spr->yrepeat;

                        dax = ((xspan >> 1) + xoff) * xrepeat;
                        day = ((yspan >> 1) + yoff) * yrepeat;
                        x1 = sprx + mulscale16(sinang, dax) + mulscale16(cosang, day);
                        y1 = spry + mulscale16(sinang, day) - mulscale16(cosang, dax);
                        l = xspan * xrepeat;
                        x2 = x1 - mulscale16(sinang, l);
                        y2 = y1 + mulscale16(cosang, l);
                        l = yspan * yrepeat;
                        k = -mulscale16(cosang, l);
                        x3 = x2 + k;
                        x4 = x1 + k;
                        k = -mulscale16(sinang, l);
                        y3 = y2 + k;
                        y4 = y1 + k;

                        ox = x1 - cposx;
                        oy = y1 - cposy;
                        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x2 - cposx;
                        oy = y2 - cposy;
                        x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x3 - cposx;
                        oy = y3 - cposy;
                        x3 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y3 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x4 - cposx;
                        oy = y4 - cposy;
                        x4 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y4 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        drawlinergb(x1 + (xdim << 11), y1 + (ydim << 11),
                            x2 + (xdim << 11), y2 + (ydim << 11), col);

                        drawlinergb(x2 + (xdim << 11), y2 + (ydim << 11),
                            x3 + (xdim << 11), y3 + (ydim << 11), col);

                        drawlinergb(x3 + (xdim << 11), y3 + (ydim << 11),
                            x4 + (xdim << 11), y4 + (ydim << 11), col);

                        drawlinergb(x4 + (xdim << 11), y4 + (ydim << 11),
                            x1 + (xdim << 11), y1 + (ydim << 11), col);

                    }
                    break;
                }
            }
        }
        return true;
}



END_SW_NS
