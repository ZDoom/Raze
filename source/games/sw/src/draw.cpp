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


#include "names2.h"
#include "panel.h"
#include "game.h"

#include "jsector.h"

#include "mytypes.h"
#include "gamecontrol.h"
#include "gamefuncs.h"
#include "network.h"
#include "pal.h"
#include "player.h"
#include "jtags.h"
#include "parent.h"

#include "misc.h"

#include "menus.h"
#include "interpolate.h"
#include "interpso.h"
#include "sector.h"
#include "razemenu.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_draw.h"
#include "render.h"
#include "razefont.h"
EXTERN_CVAR(Bool, testnewrenderer)

extern DCoreActor* wall_to_sprite_actors[8];

BEGIN_SW_NS

int display_mirror;
static int OverlapDraw = false;
extern bool QuitFlag, SpriteInfo;
extern bool Voxel;
bool DrawScreen;
extern int f_c;

extern ParentalStruct aVoxelArray[MAXTILES];

void PreDrawStackedWater(void);

void SW_InitMultiPsky(void)
{
    // default
    psky_t* const defaultsky = tileSetupSky(DEFAULTPSKY);
    defaultsky->lognumtiles = 1;
    defaultsky->horizfrac = 8192;
}


#if 1
void ShadeSprite(tspriteptr_t tsp)
{
    // set shade of sprite
    tsp->shade = tsp->sector()->floorshade - 25;

    if (tsp->shade > -3)
        tsp->shade = -3;

    if (tsp->shade < -30)
        tsp->shade = -30;
}
#else
#endif


int GetRotation(tspritetype* tsprite, int& spritesortcnt, int tSpriteNum, int viewx, int viewy)
{
    static const uint8_t RotTable8[] = {0, 7, 6, 5, 4, 3, 2, 1};
    static const uint8_t RotTable5[] = {0, 1, 2, 3, 4, 3, 2, 1};
    int rotation;

    tspriteptr_t tsp = &tsprite[tSpriteNum];
    USERp tu = static_cast<DSWActor*>(tsp->ownerActor)->u();
    int angle2;

    if (tu->RotNum == 0)
        return 0;

    // Get which of the 8 angles of the sprite to draw (0-7)
    // rotation ranges from 0-7
    angle2 = getangle(tsp->pos.X - viewx, tsp->pos.Y - viewy);
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

int SetActorRotation(tspritetype* tsprite, int& spritesortcnt, int tSpriteNum, int viewx, int viewy)
{
    tspriteptr_t tsp = &tsprite[tSpriteNum];
    USERp tu = static_cast<DSWActor*>(tsp->ownerActor)->u();
    int StateOffset, Rotation;

    // don't modify ANY tu vars - back them up!
    STATEp State = tu->State;
    STATEp StateStart = tu->StateStart;

    if (tu->RotNum == 0)
        return 0;

    // Get the offset into the State animation
    StateOffset = int(State - StateStart);

    // Get the rotation angle
    Rotation = GetRotation(tsprite, spritesortcnt, tSpriteNum, viewx, viewy);

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

int DoShadowFindGroundPoint(tspriteptr_t sp)
{
    // USES TSPRITE !!!!!
    USERp u = static_cast<DSWActor*>(sp->ownerActor)->u();
    SPRITEp hsp;
    Collision ceilhit, florhit;
    int hiz, loz = u->loz;
    ESpriteFlags save_cstat, bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    save_cstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(sp->pos.X, sp->pos.Y, sp->pos.Z, sp->sector(), &hiz, &ceilhit, &loz, &florhit);
    sp->cstat = save_cstat;

    switch (florhit.type)
    {
    case kHitSprite:
    {
        hsp = &florhit.actor()->s();

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

    case kHitSector:
        break;

    default:
        ASSERT(true == false);
        break;
    }

    return loz;
}

void DoShadows(tspritetype* tsprite, int& spritesortcnt, tspriteptr_t tsp, int viewz, int camang)
{
    tspriteptr_t tSpr = &tsprite[spritesortcnt];
    USERp tu = static_cast<DSWActor*>(tsp->ownerActor)->u();
    int ground_dist = 0;
    int view_dist = 0;
    int loz;
    int xrepeat;
    int yrepeat;

    auto sect = tsp->sector();
    // make sure its the correct sector
    // DoShadowFindGroundPoint calls FAFgetzrangepoint and this is sensitive
    updatesector(tsp->pos.X, tsp->pos.Y, &sect);

    if (sect == nullptr)
    {
        return;
    }

    tsp->setsector(sect);
    *tSpr = *tsp;
    // shadow is ALWAYS draw last - status is priority
    tSpr->statnum = MAXSTATUS;
    tSpr->setsector(sect);

    if ((tsp->yrepeat >> 2) > 4)
    {
        yrepeat = (tsp->yrepeat >> 2) - (SPRITEp_SIZE_Y(tsp) / 64) * 2;
        xrepeat = tSpr->xrepeat;
    }
    else
    {
        yrepeat = tSpr->yrepeat;
        xrepeat = tSpr->xrepeat;
    }

    tSpr->shade = 127;
    SET(tSpr->cstat, CSTAT_SPRITE_TRANSLUCENT);

    loz = tu->loz;
    if (tu->lowActor)
    {
        if (!TEST(tu->lowActor->spr.cstat, CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            loz = DoShadowFindGroundPoint(tsp);
        }
    }

    // need to find the ground here
    tSpr->pos.Z = loz;

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
    ground_dist = labs(loz - SPRITEp_BOS(tsp)) >> 12;

    xrepeat = max(xrepeat - ground_dist - view_dist, 4);
    yrepeat = max(yrepeat - ground_dist - view_dist, 4);
    xrepeat = min(xrepeat, 255);
    yrepeat = min(yrepeat, 255);

    tSpr->xrepeat = uint8_t(xrepeat);
    tSpr->yrepeat = uint8_t(yrepeat);

    if (tilehasmodelorvoxel(tsp->picnum,tsp->pal))
    {
        tSpr->yrepeat = 0;
        // cstat:    trans reverse
        // clipdist: tell mdsprite.cpp to use Z-buffer hacks to hide overdraw issues
        tSpr->clipdist |= TSPR_FLAGS_MDHACK;
        tSpr->cstat |= CSTAT_SPRITE_TRANS_FLIP;
    }
    else if (!testnewrenderer)
    {
        // Alter the shadow's position so that it appears behind the sprite itself.
        int look = getangle(tSpr->pos.X - Player[screenpeek].six, tSpr->pos.Y - Player[screenpeek].siy);
        tSpr->pos.X += bcos(look, -9);
        tSpr->pos.Y += bsin(look, -9);
    }
    else tSpr->time = 1;

    // Check for voxel items and use a round generic pic if so
    //DoVoxelShadow(New);

    spritesortcnt++;
}

void DoMotionBlur(tspritetype* tsprite, int& spritesortcnt, tspritetype const * const tsp)
{
    USERp tu = static_cast<DSWActor*>(tsp->ownerActor)->u();
    int nx,ny,nz = 0,dx,dy,dz;
    int i, ang;
    int xrepeat, yrepeat, repeat_adj = 0;
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
    case 128:
    case 256:
    case 512:
        nz = FixedToInt(z_amt_per_pixel * tu->motion_blur_dist);
        [[fallthrough]];
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
        tspriteptr_t tSpr = &tsprite[spritesortcnt];
        *tSpr = *tsp;
        SET(tSpr->cstat, CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_TRANS_FLIP);

        tSpr->pos.X += dx;
        tSpr->pos.Y += dy;
        dx += nx;
        dy += ny;

        tSpr->pos.Z += dz;
        dz += nz;

        tSpr->xrepeat = uint8_t(xrepeat);
        tSpr->yrepeat = uint8_t(yrepeat);

        xrepeat -= repeat_adj;
        yrepeat -= repeat_adj;

        spritesortcnt++;
    }

}

void SetVoxelSprite(SPRITEp sp, int pic)
{
    SET(sp->cstat, CSTAT_SPRITE_ALIGNMENT_SLAB);
    sp->picnum = pic;
}

void WarpCopySprite(tspritetype* tsprite, int& spritesortcnt)
{
    SPRITEp sp1, sp2, sp;
    int spnum;
    int xoff,yoff,zoff;
    int match;

    // look for the first one
    SWStatIterator it(STAT_WARP_COPY_SPRITE1);
    while (auto itActor = it.Next())
    {
        sp1 = &itActor->s();
        match = sp1->lotag;

        // look for the second one
        SWStatIterator it1(STAT_WARP_COPY_SPRITE2);
        while (auto itActor1 = it.Next())
        {
            sp = &itActor1->s();

            if (sp->lotag == match)
            {
                sp2 = sp;
                auto sect1 = sp1->sector();
                auto sect2 = sp2->sector();

                SWSectIterator it2(sect1);
                while (auto itActor2 = it.Next())
                {
                    auto spit = &itActor2->s();
                    if (spit == sp1)
                        continue;

                    if (spit->picnum == ST1)
                        continue;

                    tspriteptr_t newTSpr = renderAddTsprite(tsprite, spritesortcnt, itActor2);
                    newTSpr->statnum = 0;

                    xoff = sp1->pos.X - newTSpr->pos.X;
                    yoff = sp1->pos.Y - newTSpr->pos.Y;
                    zoff = sp1->pos.Z - newTSpr->pos.Z;

                    newTSpr->pos.X = sp2->pos.X - xoff;
                    newTSpr->pos.Y = sp2->pos.Y - yoff;
                    newTSpr->pos.Z = sp2->pos.Z - zoff;
                    newTSpr->setsector(sp2->sector());
                }

                it2.Reset(sect2);
                while (auto itActor2 = it2.Next())
                {
                    auto spit = &itActor2->s();
                    if (spit == sp2)
                        continue;

                    if (spit->picnum == ST1)
                        continue;

                    tspriteptr_t newTSpr = renderAddTsprite(tsprite, spritesortcnt, itActor2);
                    newTSpr->statnum = 0;

                    xoff = sp2->pos.X - newTSpr->pos.X;
                    yoff = sp2->pos.Y - newTSpr->pos.Y;
                    zoff = sp2->pos.Z - newTSpr->pos.Z;

                    newTSpr->pos.X = sp1->pos.X - xoff;
                    newTSpr->pos.Y = sp1->pos.Y - yoff;
                    newTSpr->pos.Z = sp1->pos.Z - zoff;
                    newTSpr->setsector(sp1->sector());
                }
            }
        }
    }
}

void DoStarView(tspriteptr_t tsp, USERp tu, int viewz)
{
    extern STATE s_Star[], s_StarDown[];
    extern STATE s_StarStuck[], s_StarDownStuck[];
    int zdiff = viewz - tsp->pos.Z;

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

template<class sprt>
DSWActor* CopySprite(sprt const* tsp, sectortype* newsector)
{
    SPRITEp sp;

    auto actorNew = insertActor(newsector, STAT_FAF_COPY);
    sp = &actorNew->s();

    sp->pos.X = tsp->pos.X;
    sp->pos.Y = tsp->pos.Y;
    sp->pos.Z = tsp->pos.Z;
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

    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    return actorNew;
}

template<class sprt>
DSWActor* ConnectCopySprite(sprt const* tsp)
{
    sectortype* newsector;
    int testz;

    if (FAF_ConnectCeiling(tsp->sector()))
    {
        newsector = tsp->sector();
        testz = SPRITEp_TOS(tsp) - Z(10);

        if (testz < tsp->sector()->ceilingz)
            updatesectorz(tsp->pos.X, tsp->pos.Y, testz, &newsector);

        if (newsector != nullptr && newsector != tsp->sector())
        {
            return CopySprite(tsp, newsector);
        }
    }

    if (FAF_ConnectFloor(tsp->sector()))
    {
        newsector = tsp->sector();
        testz = SPRITEp_BOS(tsp) + Z(10);

        if (testz > tsp->sector()->floorz)
            updatesectorz(tsp->pos.X, tsp->pos.Y, testz, &newsector);

        if (newsector != nullptr && newsector != tsp->sector())
        {
            return CopySprite(tsp, newsector);
        }
    }

    return nullptr;
}


void analyzesprites(tspritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, int camang)
{
    int tSpriteNum;
    int smr4, smr2;
    USERp tu;
    static int ang = 0;
    PLAYERp pp = Player + screenpeek;
    int newshade=0;

    const int DART_PIC = 2526;
    const int DART_REPEAT = 16;

    ang = NORM_ANGLE(ang + 12);

    smr4 = int(smoothratio) + IntToFixed(MoveSkip4);
    smr2 = int(smoothratio) + IntToFixed(MoveSkip2);

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        tspriteptr_t tsp = &tsprite[tSpriteNum];
        auto tActor = static_cast<DSWActor*>(tsp->ownerActor);
        tu = tActor->hasU()? tActor->u() : nullptr;
        auto tsectp = tsp->sector();

#if 0
        // Brighten up the sprite if set somewhere else to do so
        if (tu && tu->Vis > 0)
        {
            int tmpshade; // Having this prevent overflow

            tmpshade = tsp->shade  - tu->Vis;
            if (tmpshade < -128) tmpshade = -128;

            tsp->shade = tmpshade;
            tu->Vis -= 8;
        }
#endif

        // don't draw these
        if (tsp->statnum >= STAT_DONT_DRAW)
        {
            tsp->ownerActor = nullptr;
            continue;
        }

        if (tu)
        {
            if (tsp->statnum != STAT_DEFAULT)
            {
                if (TEST(tu->Flags, SPR_SKIP4))
                {
                    if (tsp->statnum <= STAT_SKIP4_INTERP_END)
                    {
                        tsp->pos = tsp->interpolatedvec3(smr4, 18);
                    }
                }

                if (TEST(tu->Flags, SPR_SKIP2))
                {
                    if (tsp->statnum <= STAT_SKIP2_INTERP_END)
                    {
                        tsp->pos = tsp->interpolatedvec3(smr2, 17);
                    }
                }
            }

            // workaround for mines and floor decals beneath the floor
            if (tsp->picnum == BETTY_R0 || tsp->picnum == FLOORBLOOD1)
            {
                auto sp = &tActor->s();
                int32_t const floorz = getflorzofslopeptr(sp->sector(), sp->pos.X, sp->pos.Y);
                if (sp->pos.Z > floorz)
                    tsp->pos.Z = floorz;
            }

            if (r_shadows && TEST(tu->Flags, SPR_SHADOW))
            {
                DoShadows(tsprite, spritesortcnt, tsp, viewz, camang);
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
                SetActorRotation(tsprite, spritesortcnt, tSpriteNum, viewx, viewy);

            if (tu->motion_blur_num)
            {
                DoMotionBlur(tsprite, spritesortcnt, tsp);
            }

            // set palette lookup correctly
            if (tsp->pal != tsectp->floorpal)
            {
                if (tsectp->floorpal == PALETTE_DEFAULT)
                {
                    // default pal for sprite is stored in tu->spal
                    // mostly for players and other monster types
                    tsp->pal = tu->spal;
                }
                else
                {
                    // if sector pal is something other than default
                    uint8_t pal = tsectp->floorpal;
                    bool nosectpal=false;

                    // sprite does not take on the new pal if sector flag is set
                    if (tsectp->hasU() && TEST(tsectp->flags, SECTFU_DONT_COPY_PALETTE))
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
                tActor->spr.hitag = 0;
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
            if (Player[screenpeek].Actor() == tActor)
            {
                PLAYERp pp = Player + screenpeek;
                if (display_mirror || TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE|PF_VIEW_FROM_CAMERA))
                {
                    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
                        SET(tsp->cstat, CSTAT_SPRITE_TRANSLUCENT);

                    if (TEST(pp->Flags, PF_CLIMBING))
                    {
                        // move sprite forward some so he looks like he's
                        // climbing
                        tsp->pos.X = pp->six + MOVEx(128 + 80, tsp->ang);
                        tsp->pos.Y = pp->siy + MOVEy(128 + 80, tsp->ang);
                    }
                    else
                    {
                        tsp->pos.X = pp->six;
                        tsp->pos.Y = pp->siy;
                    }

                    tsp->pos.Z = tsp->pos.Z + pp->siz;
                    tsp->ang = pp->siang;
                    //continue;
                }
                else
                {
                    // dont draw your sprite
                    tsp->ownerActor = nullptr;
                    //SET(tsp->cstat, CSTAT_SPRITE_INVISIBLE);
                }
            }
            else // Otherwise just interpolate the player sprite
            {
                PLAYERp pp = tu->PlayerP;
                int sr = 65536 - int(smoothratio);
                tsp->pos.X -= MulScale(pp->pos.X - pp->opos.X, sr, 16);
                tsp->pos.Y -= MulScale(pp->pos.Y - pp->opos.Y, sr, 16);
                tsp->pos.Z -= MulScale(pp->pos.Z - pp->opos.Z, sr, 16);
                tsp->ang -= MulScale(pp->angle.ang.asbuild() - pp->angle.oang.asbuild(), sr, 16);
            }
        }

        if (OverlapDraw && FAF_ConnectArea(tsp->sector()) && tsp->ownerActor)
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
            tsp->shade = int8_t(newshade);
        }

        if (TEST(tsectp->ceilingstat, CSTAT_SECTOR_SKY))
        {
            newshade = tsp->shade;
            newshade += tsectp->ceilingshade;
            if (newshade > 127) newshade = 127;
            if (newshade < -128) newshade = -128;
            tsp->shade = int8_t(newshade);
        }
        else
        {
            newshade = tsp->shade;
            newshade += tsectp->floorshade;
            if (newshade > 127) newshade = 127;
            if (newshade < -128) newshade = -128;
            tsp->shade = int8_t(newshade);
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
                if (Player[screenpeek].Actor() != tActor)
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

    WarpCopySprite(tsprite, spritesortcnt);

}


#if 1
tspriteptr_t get_tsprite(tspritetype* tsprite, int& spritesortcnt, DSWActor* actor)
{
    int tSpriteNum;

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        if (tsprite[tSpriteNum].ownerActor == actor)
            return &tsprite[tSpriteNum];
    }

    return nullptr;
}

void post_analyzesprites(tspritetype* tsprite, int& spritesortcnt)
{
    int tSpriteNum;
    USERp tu;

    for (tSpriteNum = spritesortcnt - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        auto actor = static_cast<DSWActor*>(tsprite[tSpriteNum].ownerActor);
        if (!actor) continue;    // JBF: verify this is safe
        tspriteptr_t tsp = &tsprite[tSpriteNum];

        if (actor->hasU())
        {
            tu = actor->u();
            if (tu->ID == FIREBALL_FLAMES && tu->attachActor != nullptr)
            {
                tspriteptr_t const atsp = get_tsprite(tsprite, spritesortcnt, tu->attachActor);

                if (!atsp)
                {
                    continue;
                }

                tsp->pos.X = atsp->pos.X;
                tsp->pos.Y = atsp->pos.Y;
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

void CircleCamera(int *nx, int *ny, int *nz, sectortype** vsect, binangle *nang, fixed_t q16horiz)
{
    SPRITEp sp;
    HitInfo hit{};
    int i, vx, vy, vz, hx, hy;
    int daang;
    PLAYERp pp = &Player[screenpeek];
    binangle ang;

    ang = *nang + buildang(pp->circle_camera_ang);

    // Calculate the vector (nx,ny,nz) to shoot backwards
    vx = -ang.bcos(-4);
    vy = -ang.bsin(-4);

    // lengthen the vector some
    vx += DIV2(vx);
    vy += DIV2(vy);

    vz = q16horiz >> 8;

    // Player sprite of current view
    sp = &pp->Actor()->s();

    auto bakcstat = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Make sure sector passed to hitscan is correct
    //updatesector(*nx, *ny, vsect);

    hitscan({ *nx, *ny, *nz }, *vsect, { vx, vy, vz }, hit, CLIPMASK_MISSILE);

    sp->cstat = bakcstat;              // Restore cstat

    hx = hit.hitpos.X - (*nx);
    hy = hit.hitpos.Y - (*ny);

    // If something is in the way, make pp->circle_camera_dist lower if necessary
    if (abs(vx) + abs(vy) > abs(hx) + abs(hy))
    {
        if (hit.hitWall)               // Push you a little bit off the wall
        {
            *vsect = hit.hitSector;

            daang = getangle(hit.hitWall->delta());

            i = vx * bsin(daang) + vy * -bcos(daang);
            if (abs(vx) > abs(vy))
                hx -= MulScale(vx, i, 28);
            else
                hy -= MulScale(vy, i, 28);
        }
        else if (hit.actor() == nullptr)        // Push you off the ceiling/floor
        {
            *vsect = hit.hitSector;

            if (abs(vx) > abs(vy))
                hx -= (vx >> 5);
            else
                hy -= (vy >> 5);
        }
        else
        {
            SPRITEp hsp = &hit.actor()->s();

            // if you hit a sprite that's not a wall sprite - try again
            if (!TEST(hsp->cstat, CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                auto flag_backup = hsp->cstat;
                RESET(hsp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

                CircleCamera(nx, ny, nz, vsect, nang, q16horiz);
                hsp->cstat = flag_backup;
                return;
            }
        }

        if (abs(vx) > abs(vy))
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

    *nang = ang;
}

FString GameInterface::GetCoordString()
{
    PLAYERp pp = Player + myconnectindex;
    FString out;
    out.AppendFormat("POSX:%d ", pp->pos.X);
    out.AppendFormat("POSY:%d ", pp->pos.Y);
    out.AppendFormat("POSZ:%d ", pp->pos.Z);
    out.AppendFormat("ANG:%d\n", pp->angle.ang.asbuild());

    return out;
}


void PrintSpriteInfo(PLAYERp pp)
{
    const int Y_STEP = 7;
    SPRITEp sp;
    USERp u;

    //if (SpriteInfo && !LocationInfo)
    {
        auto actor = DoPickTarget(pp->Actor(), 32, 2);
        sp = &actor->s();
        u = actor->u();

        sp->hitag = 9997; // Special tag to make the actor glow red for one frame

        if (actor == nullptr)
        {
            Printf("SPRITENUM: NONE TARGETED\n");
            return;
        }
        else
            Printf("SPRITENUM:%d\n", actor->GetIndex());

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
            Printf("POSX:%d, ", sp->pos.X);
            Printf("POSY:%d, ", sp->pos.Y);
            Printf("POSZ:%d,", sp->pos.Z);
            Printf("ANG:%d\n", sp->ang);
        }
    }
}


void DrawCrosshair(PLAYERp pp)
{
    extern bool CameraTestMode;

    if (!(CameraTestMode))
    {
        USERp u = pp->Actor()->u();
        ::DrawCrosshair(2326, u->Health, -pp->angle.look_anghalf(smoothratio), TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE) ? 5 : 0, 2, shadeToLight(10));
    }
}

void CameraView(PLAYERp pp, int *tx, int *ty, int *tz, sectortype** tsect, binangle *tang, fixedhoriz *thoriz)
{
    binangle ang;
    SPRITEp sp;
    bool found_camera = false;
    bool player_in_camera = false;
    bool FAFcansee_test;
    bool ang_test;

    if (pp == &Player[screenpeek])
    {
        SWStatIterator it(STAT_DEMO_CAMERA);
        while (auto actor = it.Next())
        {
            sp = &actor->s();

            ang = bvectangbam(*tx - sp->pos.X, *ty - sp->pos.Y);
            ang_test = getincangle(ang.asbuild(), sp->ang) < sp->lotag;

            FAFcansee_test =
                (FAFcansee(sp->pos.X, sp->pos.Y, sp->pos.Z, sp->sector(), *tx, *ty, *tz, pp->cursector) ||
                 FAFcansee(sp->pos.X, sp->pos.Y, sp->pos.Z, sp->sector(), *tx, *ty, *tz + SPRITEp_SIZE_Z(&pp->Actor()->s()), pp->cursector));

            player_in_camera = ang_test && FAFcansee_test;

            if (player_in_camera || pp->camera_check_time_delay > 0)
            {

                // if your not in the camera but are still looking
                // make sure that only the last camera shows you

                if (!player_in_camera && pp->camera_check_time_delay > 0)
                {
                    if (pp->last_camera_act != actor)
                        continue;
                }

                switch (sp->clipdist)
                {
                case 1:
                    pp->last_camera_act = actor;
                    CircleCamera(tx, ty, tz, tsect, tang, 0);
                    found_camera = true;
                    break;

                default:
                {
                    int xvect,yvect,zvect,zdiff;

                    pp->last_camera_act = actor;

                    xvect = ang.bcos(-3);
                    yvect = ang.bsin(-3);

                    zdiff = sp->pos.Z - *tz;
                    if (labs(sp->pos.X - *tx) > 1000)
                        zvect = Scale(xvect, zdiff, sp->pos.X - *tx);
                    else if (labs(sp->pos.Y - *ty) > 1000)
                        zvect = Scale(yvect, zdiff, sp->pos.Y - *ty);
                    else if (sp->pos.X - *tx != 0)
                        zvect = Scale(xvect, zdiff, sp->pos.X - *tx);
                    else if (sp->pos.Y - *ty != 0)
                        zvect = Scale(yvect, zdiff, sp->pos.Y - *ty);
                    else
                        zvect = 0;

                    // new horiz to player
                    *thoriz = q16horiz(clamp(-(zvect << 8), gi->playerHorizMin(), gi->playerHorizMax()));

                    //DSPRINTF(ds,"xvect %d,yvect %d,zvect %d,thoriz %d",xvect,yvect,zvect,*thoriz.asbuild());
                    MONO_PRINT(ds);

                    *tang = ang;
                    *tx = sp->pos.X;
                    *ty = sp->pos.Y;
                    *tz = sp->pos.Z;
                    *tsect = sp->sector();

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

void PreDraw(void)
{
    int i;
    PreDrawStackedWater();

    SWStatIterator it(STAT_FLOOR_SLOPE_DONT_DRAW);
    while (auto actor = it.Next())
    {
        RESET(actor->spr.sector()->floorstat, CSTAT_SECTOR_SLOPE);
    }
}

void PostDraw(void)
{
    int i;
    SWStatIterator it(STAT_FLOOR_SLOPE_DONT_DRAW);
    while (auto actor = it.Next())
    {
        SET(actor->spr.sector()->floorstat, CSTAT_SECTOR_SLOPE);
    }

    it.Reset(STAT_FAF_COPY);
    while (auto actor = it.Next())
    {
        actor->clearUser();
        actor->Destroy();
    }
}

void PreDrawStackedWater(void)
{
    SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto itActor = it.Next())
    {
        SWSectIterator it2(itActor->spr.sector());
        while (auto itActor2 = it2.Next())
        {
            if (itActor2->hasU())
            {
                auto sp = &itActor2->s();
                auto u = itActor2->u();
                if (sp->statnum == STAT_ITEM)
                    continue;

                if (sp->statnum <= STAT_DEFAULT || sp->statnum > STAT_PLAYER0 + MAX_SW_PLAYERS)
                    continue;

                // code so that a copied sprite will not make another copy
                if (u->xchange == -989898)
                    continue;

                auto actorNew = ConnectCopySprite(sp);
                if (actorNew != nullptr)
                {
                    // spawn a user
                    auto nu = actorNew->allocUser();

                    nu->xchange = -989898;

                    // copy everything reasonable from the user that
                    // analyzesprites() needs to draw the image
                    nu->State = u->State;
                    nu->Rot = u->Rot;
                    nu->StateStart = u->StateStart;
                    nu->StateEnd = u->StateEnd;
                    nu->Flags = u->Flags;
                    nu->Flags2 = u->Flags2;
                    nu->RotNum = u->RotNum;
                    nu->ID = u->ID;

                    nu->PlayerP = u->PlayerP;
                    nu->spal = u->spal;
                }
            }
        }
    }
}


short ScreenSavePic = false;

void DoPlayerDiveMeter(PLAYERp pp);

void polymost_drawscreen(PLAYERp pp, int tx, int ty, int tz, binangle tang, fixedhoriz thoriz, sectortype* tsect);


void UpdateWallPortalState()
{
    // This is too obtuse to be maintained statically, but with 8 mirrors at most easy to be kept up to date.
    for (int i = 0; i < mirrorcnt; i++)
    {
        if (mirror[i].mirrorWall == nullptr) {
            continue;
        }
         walltype* wal = mirror[i].mirrorWall;
        if (wal->picnum != MIRRORLABEL + i)
        {
            wal->portalflags = 0;
            continue;
        }
        wal->portalflags = 0;
        wal->portalnum = 0;

        if (!mirror[i].ismagic)
        {
            // a simple mirror
            wal->portalflags = PORTAL_WALL_MIRROR;
        }
        else
        {
            DSWActor* cam = mirror[i].cameraActor;
            if (cam)
            {
                auto sp = &cam->s();
                if (!TEST_BOOL1(sp))
                {
                    wal->portalflags = PORTAL_WALL_TO_SPRITE;
                    wal->portalnum = i;
                    wall_to_sprite_actors[i] = cam;
                }
            }
        }
    }

    SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();
        if (SP_TAG3(sp) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SP_TAG5(sp) = sp->sector()->ceilingpicnum;
            sp->sector()->ceilingpicnum = SP_TAG2(sp);
            SP_TAG4(sp) = sp->sector()->ceilingstat;
            //SET(sp->sector()->ceilingstat, ((int)SP_TAG7(sp))<<7);
            SET(sp->sector()->ceilingstat, ESectorFlags::FromInt(SP_TAG6(sp)));
            RESET(sp->sector()->ceilingstat, CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(sp) == 1)
        {
            SP_TAG5(sp) = sp->sector()->floorpicnum;
            sp->sector()->floorpicnum = SP_TAG2(sp);
            SP_TAG4(sp) = sp->sector()->floorstat;
            //SET(sp->sector()->floorstat, ((int)SP_TAG7(sp))<<7);
            SET(sp->sector()->floorstat, ESectorFlags::FromInt(SP_TAG6(sp)));
            RESET(sp->sector()->floorstat, CSTAT_SECTOR_SKY);
        }
    }

}

void RestorePortalState()
{
    SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();
        if (SP_TAG3(sp) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            sp->sector()->ceilingpicnum = SP_TAG5(sp);
            sp->sector()->ceilingstat = ESectorFlags::FromInt(SP_TAG4(sp));
            //RESET(sp->sector()->ceilingstat, CEILING_STAT_TYPE_MASK);
            RESET(sp->sector()->ceilingstat, CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(sp) == 1)
        {
            sp->sector()->floorpicnum = SP_TAG5(sp);
            sp->sector()->floorstat = ESectorFlags::FromInt(SP_TAG4(sp));
            //RESET(sp->sector()->floorstat, FLOOR_STAT_TYPE_MASK);
            RESET(sp->sector()->floorstat, CSTAT_SECTOR_SKY);
        }
    }
}

void drawscreen(PLAYERp pp, double smoothratio)
{
    extern bool CameraTestMode;
    int tx, ty, tz;
    binangle tang, trotscrnang;
    fixedhoriz thoriz;
    sectortype* tsect;
    short i,j;
    int bob_amt = 0;
    int quake_z, quake_x, quake_y;
    short quake_ang;
    extern bool FAF_DebugView;
    PLAYERp camerapp;                       // prediction player if prediction is on, else regular player

    int const viewingRange = viewingrange;

    DrawScreen = true;
    PreDraw();

    PreUpdatePanel(smoothratio);
    int sr = (int)smoothratio;
    pm_smoothratio = sr;

    if (!ScreenSavePic)
    {
        DoInterpolations(smoothratio / 65536.);                      // Stick at beginning of drawscreen
        if (cl_sointerpolation)
            so_dointerpolations(sr);                           // Stick at beginning of drawscreen
    }

    // TENSW: when rendering with prediction, the only thing that counts should
    // be the predicted player.
    if (PredictionOn && CommEnabled && pp == Player+myconnectindex)
        camerapp = ppp;
    else
        camerapp = pp;

    tx = interpolatedvalue(camerapp->opos.X, camerapp->pos.X, sr);
    ty = interpolatedvalue(camerapp->opos.Y, camerapp->pos.Y, sr);
    tz = interpolatedvalue(camerapp->opos.Z, camerapp->pos.Z, sr);

    // Interpolate the player's angle while on a sector object, just like VoidSW.
    // This isn't needed for the turret as it was fixable, but moving sector objects are problematic.
    if (SyncInput() || pp != Player+myconnectindex)
    {
        tang = camerapp->angle.interpolatedsum(smoothratio);
        thoriz = camerapp->horizon.interpolatedsum(smoothratio);
        trotscrnang = camerapp->angle.interpolatedrotscrn(smoothratio);
    }
    else
    {
        tang = pp->angle.sum();
        thoriz = pp->horizon.sum();
        trotscrnang = pp->angle.rotscrnang;
    }
    tsect = camerapp->cursector;

    updatesector(tx, ty, &tsect);

    if (pp->sop_riding || pp->sop_control)
    {
        if (pp->sop_control &&
            (!cl_sointerpolation || (CommEnabled && !pp->sop_remote)))
        {
            tx = pp->pos.X;
            ty = pp->pos.Y;
            tz = pp->pos.Z;
            tang = pp->angle.ang;
        }
        tsect = pp->cursector;
        updatesectorz(tx, ty, tz, &tsect);
    }

    pp->six = tx;
    pp->siy = ty;
    pp->siz = tz - pp->pos.Z;
    pp->siang = tang.asbuild();

    QuakeViewChange(camerapp, &quake_z, &quake_x, &quake_y, &quake_ang);
    VisViewChange(camerapp, &g_visibility);
    tz = tz + quake_z;
    tx = tx + quake_x;
    ty = ty + quake_y;
    //thoriz += buildhoriz(quake_x);
    tang += buildang(quake_ang);

    if (pp->sop_remote)
    {
        auto rsp = &pp->remoteActor->s();
        if (TEST_BOOL1(rsp))
            tang = buildang(rsp->ang);
        else
            tang = bvectangbam(pp->sop_remote->xmid - tx, pp->sop_remote->ymid - ty);
    }

    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        tz -= 8448;
        
        if (!calcChaseCamPos(&tx, &ty, &tz, &pp->Actor()->s(), &tsect, tang, thoriz, smoothratio))
        {
            tz += 8448;
            calcChaseCamPos(&tx, &ty, &tz, &pp->Actor()->s(), &tsect, tang, thoriz, smoothratio);
        }
    }
    else
    {
        bob_amt = camerapp->bob_amt;

        if (CameraTestMode)
        {
            CameraView(camerapp, &tx, &ty, &tz, &tsect, &tang, &thoriz);
        }
    }

    if (!TEST(pp->Flags, PF_VIEW_FROM_CAMERA|PF_VIEW_FROM_OUTSIDE))
    {
        if (cl_viewbob)
        {
            tz += bob_amt;
            tz += interpolatedvalue(pp->obob_z, pp->bob_z, smoothratio);
        }

        // recoil only when not in camera
        thoriz = q16horiz(clamp(thoriz.asq16() + interpolatedvalue(pp->recoil_ohorizoff, pp->recoil_horizoff, smoothratio), gi->playerHorizMin(), gi->playerHorizMax()));
    }

    if (automapMode != am_full)// && !ScreenSavePic)
    {
        // Cameras must be done before the main loop.
        if (!testnewrenderer) JS_DrawCameras(pp, tx, ty, tz, smoothratio);
        else JS_CameraParms(pp, tx, ty, tz);  
    }

    if (!testnewrenderer)
    {
        renderSetRollAngle((float)trotscrnang.asbuildf());
        polymost_drawscreen(pp, tx, ty, tz, tang, thoriz, tsect);
    }
    else
    {
        UpdateWallPortalState();
        render_drawrooms(pp->Actor(), { tx, ty, tz }, sectnum(tsect), tang, thoriz, trotscrnang, smoothratio);
        RestorePortalState();
    }


    if (!ScreenSavePic) UpdatePanel(smoothratio);

    // if doing a screen save don't need to process the rest
    if (ScreenSavePic)
    {
        DrawScreen = false;
        return;
    }


    MarkSectorSeen(pp->cursector);

    if ((automapMode != am_off) && pp == Player+myconnectindex)
    {
        SWSpriteIterator it;
        while (auto actor = it.Next())
        {
            auto sp = &actor->s();
            // Don't show sprites tagged with 257
            if (sp->lotag == 257)
            {
                if (TEST(sp->cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR))
                {
                    RESET(sp->cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR);
                    sp->owner = -2;
                }
            }
        }
        DrawOverheadMap(tx, ty, tang.asbuild(), smoothratio);
    }

    SWSpriteIterator it;
    while (auto actor = it.Next())
    {
        auto sp = &actor->s();
        // Don't show sprites tagged with 257
        if (sp->lotag == 257 && sp->owner == -2)
        {
            SET(sp->cstat, CSTAT_SPRITE_ALIGNMENT_FLOOR);
            sp->owner = -1;
        }
    }

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

    RestoreInterpolations();                 // Stick at end of drawscreen
    if (cl_sointerpolation)
        so_restoreinterpolations();                       // Stick at end of drawscreen

    if (paused && !M_Active())
    {
        auto str = GStrings("Game Paused");
        auto font = PickSmallFont(str);
        int w = font->StringWidth(str);
        DrawText(twod, font, CR_UNTRANSLATED, 160-w, 100, str, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
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




bool GameInterface::DrawAutomapPlayer(int mx, int my, int cposx, int cposy, int czoom, int cang, double const smoothratio)
{
    int i, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect;
    walltype* wal, * wal2;
    spritetype* spr;
    short p;
    static int pspr_ndx[8] = { 0,0,0,0,0,0,0,0 };
    bool sprisplayer = false;
    short txt_x, txt_y;

    xvect = -bsin(cang) * czoom;
    yvect = -bcos(cang) * czoom;


    // Draw sprites
    auto peekActor = Player[screenpeek].Actor();
    for (unsigned i = 0; i < sector.Size(); i++)
    {
        SWSectIterator it(i);
        while (auto actor = it.Next())
        {
            spr = &actor->s();
            for (p = connecthead; p >= 0; p = connectpoint2[p])
            {
                if (Player[p].Actor() == actor)
                {
                    if (spr->xvel > 16)
                        pspr_ndx[myconnectindex] = ((PlayClock >> 4) & 3);
                    sprisplayer = true;

                    goto SHOWSPRITE;
                }
            }
            if (gFullMap || (spr->cstat2 & CSTAT2_SPRITE_MAPPED))
            {
            SHOWSPRITE:

                PalEntry col = GPalette.BaseColors[56]; // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
                if ((spr->cstat & CSTAT_SPRITE_BLOCK) > 0)
                    col = GPalette.BaseColors[248];
                if (actor == peekActor)
                    col = GPalette.BaseColors[31];

                sprx = spr->pos.X;
                spry = spr->pos.Y;

                k = spr->statnum;
                if ((k >= 1) && (k <= 8) && (k != 2))   // Interpolate moving
                {
                    sprx = spr->interpolatedx(smoothratio);
                    spry = spr->interpolatedy(smoothratio);
                }

                switch (spr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
                {
                case 0:  // Regular sprite
                    if (Player[p].Actor() == actor)
                    {
                        ox = mx - cposx;
                        oy = my - cposy;
                        x1 = DMulScale(ox, xvect, -oy, yvect, 16);
                        y1 = DMulScale(oy, xvect, ox, yvect, 16);
                        int xx = xdim / 2. + x1 / 4096.;
                        int yy = ydim / 2. + y1 / 4096.;

                        if (czoom > 192)
                        {
                            daang = ((!SyncInput() ? spr->ang : spr->interpolatedang(smoothratio)) - cang) & 2047;

                            // Special case tiles
                            if (spr->picnum == 3123) break;

                            int spnum = -1;
                            if (sprisplayer)
                            {
                                if (gNet.MultiGameType != MULTI_GAME_COMMBAT || actor == Player[screenpeek].Actor())
                                    spnum = 1196 + pspr_ndx[myconnectindex];
                            }
                            else spnum = spr->picnum;

                            double sc = czoom * (spr->yrepeat) / 32768.;
                            if (spnum >= 0)
                            {
                                DrawTexture(twod, tileGetTexture(1196 + pspr_ndx[myconnectindex], true), xx, yy, DTA_ScaleX, sc, DTA_ScaleY, sc, DTA_Rotate, daang * -BAngToDegree,
                                    DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, spr->pal), DTA_Color, shadeToLight(spr->shade),
                                    DTA_Alpha, (spr->cstat & CSTAT_SPRITE_TRANSLUCENT) ? 0.33 : 1., TAG_DONE);
                            }
                        }
                    }
                    break;
                case 16: // Rotated sprite
                    x1 = sprx;
                    y1 = spry;
                    tilenum = spr->picnum;
                    xoff = (int)tileLeftOffset(tilenum) + (int)spr->xoffset;
                    if ((spr->cstat & CSTAT_SPRITE_XFLIP) > 0)
                        xoff = -xoff;
                    k = spr->ang;
                    l = spr->xrepeat;
                    dax = bsin(k) * l;
                    day = -bcos(k) * l;
                    l = tileWidth(tilenum);
                    k = (l >> 1) + xoff;
                    x1 -= MulScale(dax, k, 16);
                    x2 = x1 + MulScale(dax, l, 16);
                    y1 -= MulScale(day, k, 16);
                    y2 = y1 + MulScale(day, l, 16);

                    ox = x1 - cposx;
                    oy = y1 - cposy;
                    x1 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                    y1 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

                    ox = x2 - cposx;
                    oy = y2 - cposy;
                    x2 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                    y2 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

                    drawlinergb(x1 + (xdim << 11), y1 + (ydim << 11),
                        x2 + (xdim << 11), y2 + (ydim << 11), col);

                    break;
                case 32:    // Floor sprite
                    if (automapMode == am_overlay)
                    {
                        tilenum = spr->picnum;
                        xoff = (int)tileLeftOffset(tilenum) + (int)spr->xoffset;
                        yoff = (int)tileTopOffset(tilenum) + (int)spr->yoffset;
                        if ((spr->cstat & CSTAT_SPRITE_XFLIP) > 0)
                            xoff = -xoff;
                        if ((spr->cstat & CSTAT_SPRITE_YFLIP) > 0)
                            yoff = -yoff;

                        k = spr->ang;
                        cosang = bcos(k);
                        sinang = bsin(k);
                        xspan = tileWidth(tilenum);
                        xrepeat = spr->xrepeat;
                        yspan = tileHeight(tilenum);
                        yrepeat = spr->yrepeat;

                        dax = ((xspan >> 1) + xoff) * xrepeat;
                        day = ((yspan >> 1) + yoff) * yrepeat;
                        x1 = sprx + MulScale(sinang, dax, 16) + MulScale(cosang, day, 16);
                        y1 = spry + MulScale(sinang, day, 16) - MulScale(cosang, dax, 16);
                        l = xspan * xrepeat;
                        x2 = x1 - MulScale(sinang, l, 16);
                        y2 = y1 + MulScale(cosang, l, 16);
                        l = yspan * yrepeat;
                        k = -MulScale(cosang, l, 16);
                        x3 = x2 + k;
                        x4 = x1 + k;
                        k = -MulScale(sinang, l, 16);
                        y3 = y2 + k;
                        y4 = y1 + k;

                        ox = x1 - cposx;
                        oy = y1 - cposy;
                        x1 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                        y1 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

                        ox = x2 - cposx;
                        oy = y2 - cposy;
                        x2 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                        y2 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

                        ox = x3 - cposx;
                        oy = y3 - cposy;
                        x3 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                        y3 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

                        ox = x4 - cposx;
                        oy = y4 - cposy;
                        x4 = MulScale(ox, xvect, 16) - MulScale(oy, yvect, 16);
                        y4 = MulScale(oy, xvect, 16) + MulScale(ox, yvect, 16);

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
    }
    return true;
}

void GameInterface::processSprites(tspritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio)
{
    analyzesprites(tsprite, spritesortcnt, viewx, viewy, viewz, viewang.asbuild());
    post_analyzesprites(tsprite, spritesortcnt);
}



END_SW_NS
