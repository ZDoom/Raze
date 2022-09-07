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

extern DCoreActor* wall_to_sprite_actors[8];

BEGIN_SW_NS

int display_mirror;
static int OverlapDraw = false;
extern bool QuitFlag, SpriteInfo;
extern bool Voxel;
bool DrawScreen;
extern int f_c;

extern TILE_INFO_TYPE aVoxelArray[MAXTILES];

void PreDrawStackedWater(void);

#if 1
void ShadeSprite(tspritetype* tsp)
{
    // set shade of sprite
    tsp->shade = tsp->sectp->floorshade - 25;

    if (tsp->shade > -3)
        tsp->shade = -3;

    if (tsp->shade < -30)
        tsp->shade = -30;
}
#else
#endif


int GetRotation(tspriteArray& tsprites, int tSpriteNum, int viewx, int viewy)
{
    static const uint8_t RotTable8[] = {0, 7, 6, 5, 4, 3, 2, 1};
    static const uint8_t RotTable5[] = {0, 1, 2, 3, 4, 3, 2, 1};
    int rotation;

    tspritetype* tsp = tsprites.get(tSpriteNum);
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    int angle2;

    if (!ownerActor->hasU() || ownerActor->user.RotNum == 0)
        return 0;

    // Get which of the 8 angles of the sprite to draw (0-7)
    // rotation ranges from 0-7
    angle2 = getangle(tsp->int_pos().X - viewx, tsp->int_pos().Y - viewy);
    rotation = ((tsp->ang + 3072 + 128 - angle2) & 2047);
    rotation = (rotation >> 8) & 7;

    if (ownerActor->user.RotNum == 5)
    {
        if ((ownerActor->user.Flags & SPR_XFLIP_TOGGLE))
        {
            if (rotation <= 4)
            {
                // leave rotation alone
                tsp->cstat &= ~(CSTAT_SPRITE_XFLIP);
            }
            else
            {
                rotation = (8 - rotation);
                tsp->cstat |= (CSTAT_SPRITE_XFLIP);    // clear x-flipping bit
            }
        }
        else
        {
            if (rotation > 3 || rotation == 0)
            {
                // leave rotation alone
                tsp->cstat &= ~(CSTAT_SPRITE_XFLIP);  // clear x-flipping bit
            }
            else
            {
                rotation = (8 - rotation);
                tsp->cstat |= (CSTAT_SPRITE_XFLIP);    // set
            }
        }

        // Special case bunk
        int ID = ownerActor->user.ID;
        if (ID == TOILETGIRL_R0 || ID == WASHGIRL_R0 || ID == TRASHCAN ||
            ID == CARGIRL_R0 || ID == MECHANICGIRL_R0 || ID == PRUNEGIRL_R0 ||
            ID == SAILORGIRL_R0)
            tsp->cstat &= ~(CSTAT_SPRITE_XFLIP);  // clear x-flipping bit

        return RotTable5[rotation];
    }

    return RotTable8[rotation];

}

/*


!AIC - At draw time this is called for actor rotation.  GetRotation() is more
complex than needs to be in part because importing of actor rotations and x-flip
directions was not standardized.

*/

int SetActorRotation(tspriteArray& tsprites, int tSpriteNum, int viewx, int viewy)
{
    tspritetype* tsp = tsprites.get(tSpriteNum);
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    int StateOffset, Rotation;

    if (!ownerActor->hasU()) return 0;
    // don't modify ANY tu vars - back them up!
    STATE* State = ownerActor->user.State;
    STATE* StateStart = ownerActor->user.StateStart;

    if (ownerActor->user.RotNum == 0)
        return 0;

    // Get the offset into the State animation
    StateOffset = int(State - StateStart);

    // Get the rotation angle
    Rotation = GetRotation(tsprites, tSpriteNum, viewx, viewy);

    ASSERT(Rotation < 5);

    // Reset the State animation start based on the Rotation
    StateStart = ownerActor->user.Rot[Rotation];

    // Set the sprites state
    State = StateStart + StateOffset;

    // set the picnum here - may be redundant, but we just changed states and
    // thats a big deal
    tsp->picnum = State->Pic;

    return 0;
}

int DoShadowFindGroundPoint(tspritetype* tspr)
{
    // USES TSPRITE !!!!!
    auto ownerActor = static_cast<DSWActor*>(tspr->ownerActor);
    Collision ceilhit, florhit;
    int hiz, loz = ownerActor->user.loz;
    ESpriteFlags save_cstat, bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    save_cstat = tspr->cstat;
    tspr->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(tspr->int_pos().X, tspr->int_pos().Y, tspr->int_pos().Z, tspr->sectp, &hiz, &ceilhit, &loz, &florhit);
    tspr->cstat = save_cstat;

    switch (florhit.type)
    {
    case kHitSprite:
    {
        auto hitactor = florhit.actor();

        if ((hitactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            // found a sprite floor
            return loz;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            bak_cstat = hitactor->spr.cstat;
            hitactor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            loz = DoShadowFindGroundPoint(tspr);
            hitactor->spr.cstat = bak_cstat;
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

void DoShadows(tspriteArray& tsprites, tspritetype* tsp, int viewz, int camang)
{
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    int ground_dist = 0;
    int view_dist = 0;
    int loz;
    int xrepeat;
    int yrepeat;

    if (!ownerActor->hasU()) return;

    auto sect = tsp->sectp;
    // make sure its the correct sector
    // DoShadowFindGroundPoint calls FAFgetzrangepoint and this is sensitive
    updatesector(tsp->int_pos().X, tsp->int_pos().Y, &sect);

    if (sect == nullptr)
    {
        return;
    }

    tsp->sectp = sect;

    if ((tsp->yrepeat >> 2) > 4)
    {
        yrepeat = (tsp->yrepeat >> 2) - (GetSpriteSizeY(tsp) / 64) * 2;
        xrepeat = tsp->xrepeat;
    }
    else
    {
        yrepeat = tsp->yrepeat;
        xrepeat = tsp->xrepeat;
    }

    loz = ownerActor->user.loz;
    if (ownerActor->user.lowActor)
    {
        if (!(ownerActor->user.lowActor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ALIGNMENT_FLOOR)))
        {
            loz = DoShadowFindGroundPoint(tsp);
        }
    }

    // need to find the ground here

    // if below or close to sprites z don't bother to draw it
    if ((viewz - loz) > -Z(8))
    {
        return;
    }

    tspritetype* tSpr = tsprites.newTSprite();
    *tSpr = *tsp;
    // shadow is ALWAYS draw last - status is priority
    tSpr->statnum = MAXSTATUS;
    tSpr->shade = 127;
    tSpr->cstat |= CSTAT_SPRITE_TRANSLUCENT;
    tSpr->set_int_z(loz);

    // if close to shadows z shrink it
    view_dist = labs(loz - viewz) >> 8;
    if (view_dist < 32)
        view_dist = 256/view_dist;
    else
        view_dist = 0;

    // make shadow smaller depending on height from ground
    ground_dist = labs(loz - GetSpriteZOfBottom(tsp)) >> 12;

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
    else
    {
        // Alter the shadow's position so that it appears behind the sprite itself.
        int look = getangle(tSpr->int_pos().X - Player[screenpeek].si.X, tSpr->int_pos().Y - Player[screenpeek].si.Y);
        tSpr->add_int_x(bcos(look, -9));
        tSpr->add_int_y(bsin(look, -9));
    }

    // Check for voxel items and use a round generic pic if so
    //DoVoxelShadow(New);
}

void DoMotionBlur(tspriteArray& tsprites, tspritetype const * const tsp)
{
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    int nx,ny,nz = 0,dx,dy,dz;
    int i, ang;
    int xrepeat, yrepeat, repeat_adj = 0;
    int z_amt_per_pixel;

    ang = NORM_ANGLE(tsp->ang + 1024);

    if (!ownerActor->hasU() || tsp->xvel == 0)
    {
        return;
    }

    if ((tsp->extra & SPRX_PLAYER_OR_ENEMY))
    {
        z_amt_per_pixel = IntToFixed((int)-ownerActor->user.jump_speed * ACTORMOVETICS)/tsp->xvel;
    }
    else
    {
        z_amt_per_pixel = IntToFixed((int)-tsp->zvel)/tsp->xvel;
    }

    switch (ownerActor->user.motion_blur_dist)
    {
    case 64:
    case 128:
    case 256:
    case 512:
        nz = FixedToInt(z_amt_per_pixel * ownerActor->user.motion_blur_dist);
        [[fallthrough]];
    default:
        dx = nx = MOVEx(ownerActor->user.motion_blur_dist, ang);
        dy = ny = MOVEy(ownerActor->user.motion_blur_dist, ang);
        break;
    }

    dz = nz;

    xrepeat = tsp->xrepeat;
    yrepeat = tsp->yrepeat;

    switch ((ownerActor->user.Flags2 & SPR2_BLUR_TAPER))
    {
    case 0:
        repeat_adj = 0;
        break;
    case SPR2_BLUR_TAPER_SLOW:
        repeat_adj = xrepeat / (ownerActor->user.motion_blur_num*2);
        break;
    case SPR2_BLUR_TAPER_FAST:
        repeat_adj = xrepeat / ownerActor->user.motion_blur_num;
        break;
    }

    for (i = 0; i < ownerActor->user.motion_blur_num; i++)
    {
        tspritetype* tSpr = tsprites.newTSprite();
        *tSpr = *tsp;
        tSpr->cstat |= CSTAT_SPRITE_TRANSLUCENT|CSTAT_SPRITE_TRANS_FLIP;

        tSpr->add_int_x(dx);
        tSpr->add_int_y(dy);
        dx += nx;
        dy += ny;

        tSpr->add_int_z(dz);
        dz += nz;

        tSpr->xrepeat = uint8_t(xrepeat);
        tSpr->yrepeat = uint8_t(yrepeat);

        xrepeat -= repeat_adj;
        yrepeat -= repeat_adj;
    }

}

void WarpCopySprite(tspriteArray& tsprites)
{
    int spnum;
    int xoff,yoff,zoff;
    int match;

    // look for the first one
    SWStatIterator it(STAT_WARP_COPY_SPRITE1);
    while (auto itActor = it.Next())
    {
        match = itActor->spr.lotag;

        // look for the second one
        SWStatIterator it1(STAT_WARP_COPY_SPRITE2);
        while (auto itActor1 = it.Next())
        {
            if (itActor1->spr.lotag == match)
            {
                auto sect1 = itActor->sector();
                auto sect2 = itActor1->sector();

                SWSectIterator it2(sect1);
                while (auto itActor2 = it.Next())
                {
                    if (itActor2 == itActor)
                        continue;

                    if (itActor2->spr.picnum == ST1)
                        continue;

                    tspritetype* newTSpr = renderAddTsprite(tsprites, itActor2);
                    newTSpr->statnum = 0;

                    xoff = itActor->int_pos().X - newTSpr->int_pos().X;
                    yoff = itActor->int_pos().Y - newTSpr->int_pos().Y;
                    zoff = itActor->int_pos().Z - newTSpr->int_pos().Z;

                    newTSpr->set_int_pos({
                        itActor1->int_pos().X - xoff,
                        itActor1->int_pos().Y - yoff,
                        itActor1->int_pos().Z - zoff });
                    newTSpr->sectp = itActor1->sector();
                }

                it2.Reset(sect2);
                while (auto itActor2 = it2.Next())
                {
                    if (itActor2 == itActor1)
                        continue;

                    if (itActor2->spr.picnum == ST1)
                        continue;

                    tspritetype* newTSpr = renderAddTsprite(tsprites, itActor2);
                    newTSpr->statnum = 0;

                    auto off = itActor1->int_pos() - newTSpr->int_pos();
                    newTSpr->set_int_pos(itActor->int_pos() - off);
                    newTSpr->sectp = itActor->sector();
                }
            }
        }
    }
}

void DoStarView(tspritetype* tsp, DSWActor* tActor, int viewz)
{
    extern STATE s_Star[], s_StarDown[];
    extern STATE s_StarStuck[], s_StarDownStuck[];
    int zdiff = viewz - tsp->int_pos().Z;

    if (labs(zdiff) > Z(24))
    {
        if (tActor->user.StateStart == s_StarStuck)
            tsp->picnum = s_StarDownStuck[tActor->user.State - s_StarStuck].Pic;
        else
            tsp->picnum = s_StarDown[tActor->user.State - s_Star].Pic;

        if (zdiff > 0)
            tsp->cstat |= (CSTAT_SPRITE_YFLIP);
    }
    else
    {
        if (zdiff > 0)
            tsp->cstat |= (CSTAT_SPRITE_YFLIP);
    }
}

template<class sprt>
DSWActor* CopySprite(sprt const* tsp, sectortype* newsector)
{

    auto actorNew = insertActor(newsector, STAT_FAF_COPY);

    actorNew->set_int_pos(tsp->int_pos());
    actorNew->spr.cstat = tsp->cstat;
    actorNew->spr.picnum = tsp->picnum;
    actorNew->spr.pal = tsp->pal;
    actorNew->spr.xrepeat = tsp->xrepeat;
    actorNew->spr.yrepeat = tsp->yrepeat;
    actorNew->spr.xoffset = tsp->xoffset;
    actorNew->spr.yoffset = tsp->yoffset;
    actorNew->spr.ang = tsp->ang;
    actorNew->spr.xvel = tsp->xvel;
    actorNew->spr.yvel = tsp->yvel;
    actorNew->spr.zvel = tsp->zvel;
    actorNew->spr.shade = tsp->shade;

    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    return actorNew;
}

DSWActor* ConnectCopySprite(spritetypebase const* tsp)
{
    sectortype* newsector;
    int testz;

    if (FAF_ConnectCeiling(tsp->sectp))
    {
        newsector = tsp->sectp;
        testz = GetSpriteZOfTop(tsp) - Z(10);

        if (testz < tsp->sectp->int_ceilingz())
            updatesectorz(tsp->int_pos().X, tsp->int_pos().Y, testz, &newsector);

        if (newsector != nullptr && newsector != tsp->sectp)
        {
            return CopySprite(tsp, newsector);
        }
    }

    if (FAF_ConnectFloor(tsp->sectp))
    {
        newsector = tsp->sectp;
        testz = GetSpriteZOfBottom(tsp) + Z(10);

        if (testz > tsp->sectp->int_floorz())
            updatesectorz(tsp->int_pos().X, tsp->int_pos().Y, testz, &newsector);

        if (newsector != nullptr && newsector != tsp->sectp)
        {
            return CopySprite(tsp, newsector);
        }
    }

    return nullptr;
}


void analyzesprites(tspriteArray& tsprites, int viewx, int viewy, int viewz, int camang)
{
    int tSpriteNum;
    int smr4, smr2;
    static int ang = 0;
    PLAYER* pp = Player + screenpeek;
    int newshade=0;

    const int DART_PIC = 2526;
    const int DART_REPEAT = 16;

    ang = NORM_ANGLE(ang + 12);

    smr4 = int(smoothratio) + IntToFixed(MoveSkip4);
    smr2 = int(smoothratio) + IntToFixed(MoveSkip2);

    for (tSpriteNum = (int)tsprites.Size() - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        tspritetype* tsp = tsprites.get(tSpriteNum);
        auto tActor = static_cast<DSWActor*>(tsp->ownerActor);
        auto tsectp = tsp->sectp;

#if 0
        // Brighten up the sprite if set somewhere else to do so
        if (tu && tActor->user.Vis > 0)
        {
            int tmpshade; // Having this prevent overflow

            tmpshade = tsp->shade  - tActor->user.Vis;
            if (tmpshade < -128) tmpshade = -128;

            tsp->shade = tmpshade;
            tActor->user.Vis -= 8;
        }
#endif

        // don't draw these
        if (tsp->statnum >= STAT_DONT_DRAW)
        {
            tsp->ownerActor = nullptr;
            continue;
        }

        if (tActor->hasU())
        {
            if (tsp->statnum != STAT_DEFAULT)
            {
                if ((tActor->user.Flags & SPR_SKIP4))
                {
                    if (tsp->statnum <= STAT_SKIP4_INTERP_END)
                    {
                        tsp->set_int_pos(tActor->interpolatedvec3(smr4, 18));
                    }
                }

                if ((tActor->user.Flags & SPR_SKIP2))
                {
                    if (tsp->statnum <= STAT_SKIP2_INTERP_END)
                    {
                        tsp->set_int_pos(tActor->interpolatedvec3(smr2, 17));
                    }
                }
            }

            // workaround for mines and floor decals beneath the floor
            if (tsp->picnum == BETTY_R0 || tsp->picnum == FLOORBLOOD1)
            {
                int32_t const florz = getflorzofslopeptr(tActor->sector(), tActor->int_pos().X, tActor->int_pos().Y);
                if (tActor->int_pos().Z > florz)
                    tsp->set_int_z(florz);
            }

            if (r_shadows && (tActor->user.Flags & SPR_SHADOW))
            {
                DoShadows(tsprites, tsp, viewz, camang);
            }

            //#define UK_VERSION 1

            //#define DART_REPEAT 6
            //#define DART_PIC 2233
            if (sw_darts)
                if (tActor->user.ID == 1793 || tsp->picnum == 1793)
                {
                    tsp->picnum = 2519;
                    tsp->xrepeat = 27;
                    tsp->yrepeat = 29;
                }

            if (tActor->user.ID == STAR1)
            {
                if (sw_darts)
                {

                    tsp->picnum = DART_PIC;
                    tsp->ang = NORM_ANGLE(tsp->ang - 512 - 24);
                    tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                    tsp->cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
                }
                else
                    DoStarView(tsp, tActor, viewz);
            }

            // rotation
            if (tActor->user.RotNum > 0)
                SetActorRotation(tsprites, tSpriteNum, viewx, viewy);

            if (tActor->user.motion_blur_num)
            {
                DoMotionBlur(tsprites, tsp);
            }

            // set palette lookup correctly
            if (tsp->pal != tsectp->floorpal)
            {
                if (tsectp->floorpal == PALETTE_DEFAULT)
                {
                    // default pal for sprite is stored in tActor->user.spal
                    // mostly for players and other monster types
                    tsp->pal = tActor->user.spal;
                }
                else
                {
                    // if sector pal is something other than default
                    uint8_t pal = tsectp->floorpal;
                    bool nosectpal=false;

                    // sprite does not take on the new pal if sector flag is set
                    if (tsectp->hasU() && (tsectp->flags & SECTFU_DONT_COPY_PALETTE))
                    {
                        pal = PALETTE_DEFAULT;
                        nosectpal = true;
                    }

                    //if(tActor->user.spal == PALETTE_DEFAULT)
                    if (tsp->hitag != SECTFU_DONT_COPY_PALETTE && tsp->hitag != LUMINOUS
                        && !nosectpal
                        && pal != PALETTE_FOG && pal != PALETTE_DIVE &&
                        pal != PALETTE_DIVE_LAVA)
                        tsp->pal = pal;
                    else
                        tsp->pal = tActor->user.spal;

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
                tsp->cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
            }

        // Call my sprite handler
        // Does autosizing and voxel handling
        JAnalyzeSprites(tsp);

        // only do this of you are a player sprite
        //if (tsp->statnum >= STAT_PLAYER0 && tsp->statnum < STAT_PLAYER0 + MAX_SW_PLAYERS)
        if (tActor->hasU() && tActor->user.PlayerP)
        {
            // Shadow spell
            if (!(tsp->cstat & CSTAT_SPRITE_TRANSLUCENT))
                ShadeSprite(tsp);

            // sw if its your playersprite
            if (Player[screenpeek].actor == tActor)
            {
                pp = Player + screenpeek;
                if (display_mirror || (pp->Flags & (PF_VIEW_FROM_OUTSIDE|PF_VIEW_FROM_CAMERA)))
                {
                    if (pp->Flags & (PF_VIEW_FROM_OUTSIDE))
                        tsp->cstat |= (CSTAT_SPRITE_TRANSLUCENT);

                    vec3_t pos;
                    if (pp->Flags & (PF_CLIMBING))
                    {
                        // move sprite forward some so he looks like he's
                        // climbing
                        pos.X = pp->si.X + MOVEx(128 + 80, tsp->ang);
                        pos.Y = pp->si.Y + MOVEy(128 + 80, tsp->ang);
                    }
                    else
                    {
                        pos.X = pp->si.X;
                        pos.Y = pp->si.Y;
                    }

                    pos.Z = tsp->int_pos().Z + pp->si.Z;
                    tsp->set_int_pos(pos);
                    tsp->ang = pp->siang;
                    //continue;
                }
                else
                {
                    // dont draw your sprite
                    tsp->ownerActor = nullptr;
                    //tsp->cstat |= (CSTAT_SPRITE_INVISIBLE);
                }
            }
            else // Otherwise just interpolate the player sprite
            {
                pp = tActor->user.PlayerP;
                int sr = 65536 - int(smoothratio);
                tsp->add_int_x(-MulScale(pp->pos.X - pp->opos.X, sr, 16));
                tsp->add_int_y(-MulScale(pp->pos.Y - pp->opos.Y, sr, 16));
                tsp->add_int_z(-MulScale(pp->pos.Z - pp->opos.Z, sr, 16));
                tsp->ang -= MulScale(pp->angle.ang.asbuild() - pp->angle.oang.asbuild(), sr, 16);
            }
        }

        if (OverlapDraw && FAF_ConnectArea(tsp->sectp) && tsp->ownerActor)
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

        if ((tsectp->ceilingstat & CSTAT_SECTOR_SKY))
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

        if (pp->NightVision && (tsp->extra & SPRX_PLAYER_OR_ENEMY))
        {
            if (tActor->hasU() && tActor->user.ID == TRASHCAN) continue; // Don't light up trashcan

            tsp->pal = PALETTE_ILLUMINATE;  // Make sprites REALLY bright green.
            tsp->shade = -128;
        }

        if (tActor->hasU() && tActor->user.PlayerP)
        {
            if ((tActor->user.Flags2 & SPR2_VIS_SHADING))
            {
                if (Player[screenpeek].actor != tActor)
                {
                    if (!(tActor->user.PlayerP->Flags & PF_VIEW_FROM_OUTSIDE))
                    {
                        tsp->cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
                    }
                }

                tsp->shade = 12 - StdRandomRange(30);
            }
        }
    }

    WarpCopySprite(tsprites);

}


#if 1
tspritetype* get_tsprite(tspriteArray& tsprites, DSWActor* actor)
{
    int tSpriteNum;

    for (tSpriteNum = (int)tsprites.Size() - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        if (tsprites.get(tSpriteNum)->ownerActor == actor)
            return tsprites.get(tSpriteNum);
    }

    return nullptr;
}

void post_analyzesprites(tspriteArray& tsprites)
{
    int tSpriteNum;

    for (tSpriteNum = (int)tsprites.Size() - 1; tSpriteNum >= 0; tSpriteNum--)
    {
        auto actor = static_cast<DSWActor*>(tsprites.get(tSpriteNum)->ownerActor);
        if (!actor) continue;    // JBF: verify this is safe
        tspritetype* tsp = tsprites.get(tSpriteNum);

        if (actor->hasU())
        {
            if (actor->user.ID == FIREBALL_FLAMES && actor->user.attachActor != nullptr)
            {
                tspritetype* const atsp = get_tsprite(tsprites, actor->user.attachActor);

                if (!atsp)
                {
                    continue;
                }

                tsp->set_int_x(atsp->int_pos().X);
                tsp->set_int_y(atsp->int_pos().Y);
                // statnum is priority - draw this ALWAYS first at 0
                // statnum is priority - draw this ALWAYS last at MAXSTATUS
                if ((atsp->extra & SPRX_BURNABLE))
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
    HitInfo hit{};
    int i, vx, vy, vz, hx, hy;
    int daang;
    PLAYER* pp = &Player[screenpeek];
    binangle ang;

    ang = *nang + buildang(pp->circle_camera_ang);

    // Calculate the vector (nx,ny,nz) to shoot backwards
    vx = -ang.bcos(-4);
    vy = -ang.bsin(-4);

    // lengthen the vector some
    vx += vx >> 1;
    vy += vy >> 1;

    vz = q16horiz >> 8;

    // Player sprite of current view
    DSWActor* actor = pp->actor;

    auto bakcstat = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Make sure sector passed to hitscan is correct
    //updatesector(*nx, *ny, vsect);

    hitscan({ *nx, *ny, *nz }, *vsect, { vx, vy, vz }, hit, CLIPMASK_MISSILE);

    actor->spr.cstat = bakcstat;              // Restore cstat

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
            auto hitactor = hit.actor();

            // if you hit a sprite that's not a wall sprite - try again
            if (!(hitactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                auto flag_backup = hitactor->spr.cstat;
                hitactor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

                CircleCamera(nx, ny, nz, vsect, nang, q16horiz);
                hitactor->spr.cstat = flag_backup;
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
    PLAYER* pp = Player + myconnectindex;
    FString out;
    out.AppendFormat("POSX:%d ", pp->pos.X);
    out.AppendFormat("POSY:%d ", pp->pos.Y);
    out.AppendFormat("POSZ:%d ", pp->pos.Z);
    out.AppendFormat("ANG:%d\n", pp->angle.ang.asbuild());

    return out;
}


void PrintSpriteInfo(PLAYER* pp)
{
    const int Y_STEP = 7;

    //if (SpriteInfo && !LocationInfo)
    {
        auto actor = DoPickTarget(pp->actor, 32, 2);

        actor->spr.hitag = 9997; // Special tag to make the actor glow red for one frame

        if (actor == nullptr)
        {
            Printf("SPRITENUM: NONE TARGETED\n");
            return;
        }
        else
            Printf("SPRITENUM:%d\n", actor->GetIndex());

        if (actor->hasU())
        {
            Printf("ID:%d, ", actor->user.ID);
            Printf("PALETTE:%d, ", actor->user.spal);
            Printf("HEALTH:%d, ", actor->user.Health);
            Printf("WAITTICS:%d, ", actor->user.WaitTics);
            Printf("COUNTER:%d, ", actor->user.Counter);
            Printf("COUNTER2:%d\n", actor->user.Counter);
        }

        {
            Printf("POSX:%d, ", actor->int_pos().X);
            Printf("POSY:%d, ", actor->int_pos().Y);
            Printf("POSZ:%d,", actor->int_pos().Z);
            Printf("ANG:%d\n", actor->spr.ang);
        }
    }
}


void DrawCrosshair(PLAYER* pp)
{
    if (!(CameraTestMode))
    {
        ::DrawCrosshair(2326, pp->actor->user.Health, -pp->angle.look_anghalf(smoothratio), (pp->Flags & PF_VIEW_FROM_OUTSIDE) ? 5 : 0, 2, shadeToLight(10));
    }
}

void CameraView(PLAYER* pp, int *tx, int *ty, int *tz, sectortype** tsect, binangle *tang, fixedhoriz *thoriz)
{
    binangle ang;
    bool found_camera = false;
    bool player_in_camera = false;
    bool FAFcansee_test;
    bool ang_test;

    if (pp == &Player[screenpeek])
    {
        SWStatIterator it(STAT_DEMO_CAMERA);
        while (auto actor = it.Next())
        {
            ang = bvectangbam(*tx - actor->int_pos().X, *ty - actor->int_pos().Y);
            ang_test = getincangle(ang.asbuild(), actor->spr.ang) < actor->spr.lotag;

            FAFcansee_test =
                (FAFcansee(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), *tx, *ty, *tz, pp->cursector) ||
                 FAFcansee(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), *tx, *ty, *tz + ActorSizeZ(pp->actor), pp->cursector));

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

                switch (actor->spr.clipdist)
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

                    zdiff = actor->int_pos().Z - *tz;
                    if (labs(actor->int_pos().X - *tx) > 1000)
                        zvect = Scale(xvect, zdiff, actor->int_pos().X - *tx);
                    else if (labs(actor->int_pos().Y - *ty) > 1000)
                        zvect = Scale(yvect, zdiff, actor->int_pos().Y - *ty);
                    else if (actor->int_pos().X - *tx != 0)
                        zvect = Scale(xvect, zdiff, actor->int_pos().X - *tx);
                    else if (actor->int_pos().Y - *ty != 0)
                        zvect = Scale(yvect, zdiff, actor->int_pos().Y - *ty);
                    else
                        zvect = 0;

                    // new horiz to player
                    *thoriz = q16horiz(clamp(-(zvect << 8), gi->playerHorizMin(), gi->playerHorizMax()));

                    *tang = ang;
                    *tx = actor->int_pos().X;
                    *ty = actor->int_pos().Y;
                    *tz = actor->int_pos().Z;
                    *tsect = actor->sector();

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
        pp->Flags |= (PF_VIEW_FROM_CAMERA);

        ASSERT(found_camera);
    }
    else
    // if you !player_in_camera you still might have a camera
    // for a split second
    {
        if (found_camera)
        {
            pp->Flags |= (PF_VIEW_FROM_CAMERA);
        }
        else
        {
            pp->circle_camera_ang = 0;
            pp->circle_camera_dist = CIRCLE_CAMERA_DIST_MIN;
            pp->Flags &= ~(PF_VIEW_FROM_CAMERA);
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
        actor->sector()->floorstat &= ~(CSTAT_SECTOR_SLOPE);
    }
}

void PostDraw(void)
{
    int i;
    SWStatIterator it(STAT_FLOOR_SLOPE_DONT_DRAW);
    while (auto actor = it.Next())
    {
        actor->sector()->floorstat |= (CSTAT_SECTOR_SLOPE);
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
        SWSectIterator it2(itActor->sector());
        while (auto itActor2 = it2.Next())
        {
            if (itActor2->hasU())
            {
                if (itActor2->spr.statnum == STAT_ITEM)
                    continue;

                if (itActor2->spr.statnum <= STAT_DEFAULT || itActor2->spr.statnum > STAT_PLAYER0 + MAX_SW_PLAYERS)
                    continue;

                // code so that a copied sprite will not make another copy
                if (itActor2->user.change.X == -989898)
                    continue;

                auto actorNew = ConnectCopySprite(&itActor2->spr);
                if (actorNew != nullptr)
                {
                    // spawn a user
                    actorNew->allocUser();

                    actorNew->user.change.X = -989898;

                    // copy everything reasonable from the user that
                    // analyzesprites() needs to draw the image
                    actorNew->user.State = itActor2->user.State;
                    actorNew->user.Rot = itActor2->user.Rot;
                    actorNew->user.StateStart = itActor2->user.StateStart;
                    actorNew->user.StateEnd = itActor2->user.StateEnd;
                    actorNew->user.Flags = itActor2->user.Flags;
                    actorNew->user.Flags2 = itActor2->user.Flags2;
                    actorNew->user.RotNum = itActor2->user.RotNum;
                    actorNew->user.ID = itActor2->user.ID;

                    actorNew->user.PlayerP = itActor2->user.PlayerP;
                    actorNew->user.spal = itActor2->user.spal;
                }
            }
        }
    }
}


void DoPlayerDiveMeter(PLAYER* pp);

void polymost_drawscreen(PLAYER* pp, int tx, int ty, int tz, binangle tang, fixedhoriz thoriz, sectortype* tsect);


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
                if (!TEST_BOOL1(cam))
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
        if (SP_TAG3(actor) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SP_TAG5(actor) = actor->sector()->ceilingpicnum;
            actor->sector()->ceilingpicnum = SP_TAG2(actor);
            SP_TAG4(actor) = actor->sector()->ceilingstat;
            actor->sector()->ceilingstat |= (ESectorFlags::FromInt(SP_TAG6(actor)));
            actor->sector()->ceilingstat &= ~(CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(actor) == 1)
        {
            SP_TAG5(actor) = actor->sector()->floorpicnum;
            actor->sector()->floorpicnum = SP_TAG2(actor);
            SP_TAG4(actor) = actor->sector()->floorstat;
            actor->sector()->floorstat |= (ESectorFlags::FromInt(SP_TAG6(actor)));
            actor->sector()->floorstat &= ~(CSTAT_SECTOR_SKY);
        }
    }

}

void RestorePortalState()
{
    SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto actor = it.Next())
    {
        if (SP_TAG3(actor) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            actor->sector()->ceilingpicnum = SP_TAG5(actor);
            actor->sector()->ceilingstat = ESectorFlags::FromInt(SP_TAG4(actor));
            actor->sector()->ceilingstat &= ~(CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(actor) == 1)
        {
            actor->sector()->floorpicnum = SP_TAG5(actor);
            actor->sector()->floorstat = ESectorFlags::FromInt(SP_TAG4(actor));
            actor->sector()->floorstat &= ~(CSTAT_SECTOR_SKY);
        }
    }
}

void drawscreen(PLAYER* pp, double smoothratio, bool sceneonly)
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
    PLAYER* camerapp;                       // prediction player if prediction is on, else regular player

    DrawScreen = true;
    PreDraw();

    PreUpdatePanel(smoothratio);
    int sr = (int)smoothratio;

    if (!sceneonly)
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

    pp->si.X = tx;
    pp->si.Y = ty;
    pp->si.Z = tz - pp->pos.Z;
    pp->siang = tang.asbuild();

    QuakeViewChange(camerapp, &quake_z, &quake_x, &quake_y, &quake_ang);
    int vis = g_visibility;
    VisViewChange(camerapp, &vis);
    g_relvisibility = vis - g_visibility;
    tz = tz + quake_z;
    tx = tx + quake_x;
    ty = ty + quake_y;
    //thoriz += buildhoriz(quake_x);
    tang += buildang(quake_ang);

    if (pp->sop_remote)
    {
        DSWActor* ractor = pp->remoteActor;
        if (TEST_BOOL1(ractor))
            tang = buildang(ractor->spr.ang);
        else
            tang = bvectangbam(pp->sop_remote->pmid.X - tx, pp->sop_remote->pmid.Y - ty);
    }

    if (pp->Flags & (PF_VIEW_FROM_OUTSIDE))
    {
        tz -= 8448;

        if (!calcChaseCamPos(&tx, &ty, &tz, pp->actor, &tsect, tang, thoriz, smoothratio))
        {
            tz += 8448;
            calcChaseCamPos(&tx, &ty, &tz, pp->actor, &tsect, tang, thoriz, smoothratio);
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

    if (!(pp->Flags & (PF_VIEW_FROM_CAMERA|PF_VIEW_FROM_OUTSIDE)))
    {
        if (cl_viewbob)
        {
            tz += bob_amt;
            tz += interpolatedvalue(pp->obob_z, pp->bob_z, smoothratio);
        }

        // recoil only when not in camera
        thoriz = q16horiz(clamp(thoriz.asq16() + interpolatedvalue(pp->recoil_ohorizoff, pp->recoil_horizoff, smoothratio), gi->playerHorizMin(), gi->playerHorizMax()));
    }

    if (automapMode != am_full)
    {
        // Cameras must be done before the main loop.
        JS_CameraParms(pp, tx, ty, tz);  
    }

    if (!sceneonly) UpdatePanel(smoothratio);
    UpdateWallPortalState();
    render_drawrooms(pp->actor, { tx, ty, tz }, sectnum(tsect), tang, thoriz, trotscrnang, smoothratio);
    RestorePortalState();

    if (sceneonly)
    {
        DrawScreen = false;
        return;
    }

    // if doing a screen save don't need to process the rest


    MarkSectorSeen(pp->cursector);

    if ((automapMode != am_off) && pp == Player+myconnectindex)
    {
        SWSpriteIterator it;
        while (auto actor = it.Next())
        {
            // Don't show sprites tagged with 257
            if (actor->spr.lotag == 257)
            {
                if (actor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_FLOOR))
                {
                    actor->spr.cstat &= ~(CSTAT_SPRITE_ALIGNMENT_FLOOR);
                    actor->spr.intowner = -2;
                }
            }
        }
        DrawOverheadMap(tx, ty, tang.asbuild(), smoothratio);
    }

    SWSpriteIterator it;
    while (auto actor = it.Next())
    {
        // Don't show sprites tagged with 257
        if (actor->spr.lotag == 257 && actor->spr.intowner == -2)
        {
            actor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_FLOOR);
            actor->spr.intowner = -1;
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

    if (!CommEnabled && (pp->Flags & PF_DEAD))
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
    drawscreen(Player + myconnectindex, 65536, true);
    return true;
}




bool GameInterface::DrawAutomapPlayer(int mx, int my, int cposx, int cposy, int czoom, int cang, double const smoothratio)
{
    int k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect;
    walltype* wal, * wal2;
    short p;
    static int pspr_ndx[8] = { 0,0,0,0,0,0,0,0 };
    bool sprisplayer = false;
    short txt_x, txt_y;

    xvect = -bsin(cang) * czoom;
    yvect = -bcos(cang) * czoom;

    int xdim = twod->GetWidth() << 11;
    int ydim = twod->GetHeight() << 11;

    // Draw sprites
    auto peekActor = Player[screenpeek].actor;
    for (unsigned i = 0; i < sector.Size(); i++)
    {
        SWSectIterator it(i);
        while (auto actor = it.Next())
        {
            for (p = connecthead; p >= 0; p = connectpoint2[p])
            {
                if (Player[p].actor == actor)
                {
                    if (actor->spr.xvel > 16)
                        pspr_ndx[myconnectindex] = ((PlayClock >> 4) & 3);
                    sprisplayer = true;

                    goto SHOWSPRITE;
                }
            }
            if (gFullMap || (actor->spr.cstat2 & CSTAT2_SPRITE_MAPPED))
            {
            SHOWSPRITE:

                PalEntry col = GPalette.BaseColors[56]; // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
                if ((actor->spr.cstat & CSTAT_SPRITE_BLOCK) > 0)
                    col = GPalette.BaseColors[248];
                if (actor == peekActor)
                    col = GPalette.BaseColors[31];

                sprx = actor->int_pos().X;
                spry = actor->int_pos().Y;

                k = actor->spr.statnum;
                if ((k >= 1) && (k <= 8) && (k != 2))   // Interpolate moving
                {
                    sprx = actor->interpolatedx(smoothratio);
                    spry = actor->interpolatedy(smoothratio);
                }

                switch (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
                {
                case 0:  // Regular sprite
                    if (Player[p].actor == actor)
                    {
                        ox = mx - cposx;
                        oy = my - cposy;
                        x1 = DMulScale(ox, xvect, -oy, yvect, 16);
                        y1 = DMulScale(oy, xvect, ox, yvect, 16);
                        int xx = twod->GetWidth() / 2. + x1 / 4096.;
                        int yy = twod->GetHeight() / 2. + y1 / 4096.;

                        if (czoom > 192)
                        {
                            daang = ((!SyncInput() ? actor->spr.ang : actor->interpolatedang(smoothratio)) - cang) & 2047;

                            // Special case tiles
                            if (actor->spr.picnum == 3123) break;

                            int spnum = -1;
                            if (sprisplayer)
                            {
                                if (gNet.MultiGameType != MULTI_GAME_COMMBAT || actor == Player[screenpeek].actor)
                                    spnum = 1196 + pspr_ndx[myconnectindex];
                            }
                            else spnum = actor->spr.picnum;

                            double sc = czoom * (actor->spr.yrepeat) / 32768.;
                            if (spnum >= 0)
                            {
                                DrawTexture(twod, tileGetTexture(1196 + pspr_ndx[myconnectindex], true), xx, yy, DTA_ScaleX, sc, DTA_ScaleY, sc, DTA_Rotate, daang * -BAngToDegree,
                                    DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, actor->spr.pal), DTA_Color, shadeToLight(actor->spr.shade),
                                    DTA_Alpha, (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT) ? 0.33 : 1., TAG_DONE);
                            }
                        }
                    }
                    break;
                case 16: // Rotated sprite
                    x1 = sprx;
                    y1 = spry;
                    tilenum = actor->spr.picnum;
                    xoff = (int)tileLeftOffset(tilenum) + (int)actor->spr.xoffset;
                    if ((actor->spr.cstat & CSTAT_SPRITE_XFLIP) > 0)
                        xoff = -xoff;
                    k = actor->spr.ang;
                    l = actor->spr.xrepeat;
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

                    drawlinergb(x1 + xdim, y1 + ydim, x2 + xdim, y2 + ydim, col);

                    break;
                case 32:    // Floor sprite
                    if (automapMode == am_overlay)
                    {
                        tilenum = actor->spr.picnum;
                        xoff = (int)tileLeftOffset(tilenum) + (int)actor->spr.xoffset;
                        yoff = (int)tileTopOffset(tilenum) + (int)actor->spr.yoffset;
                        if ((actor->spr.cstat & CSTAT_SPRITE_XFLIP) > 0)
                            xoff = -xoff;
                        if ((actor->spr.cstat & CSTAT_SPRITE_YFLIP) > 0)
                            yoff = -yoff;

                        k = actor->spr.ang;
                        cosang = bcos(k);
                        sinang = bsin(k);
                        xspan = tileWidth(tilenum);
                        xrepeat = actor->spr.xrepeat;
                        yspan = tileHeight(tilenum);
                        yrepeat = actor->spr.yrepeat;

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

                        drawlinergb(x1 + xdim, y1 + ydim,
                            x2 + xdim, y2 + ydim, col);

                        drawlinergb(x2 + xdim, y2 + ydim,
                            x3 + xdim, y3 + ydim, col);

                        drawlinergb(x3 + xdim, y3 + ydim,
                            x4 + xdim, y4 + ydim, col);

                        drawlinergb(x4 + xdim, y4 + ydim,
                            x1 + xdim, y1 + ydim, col);

                    }
                    break;
                }
            }
        }
    }
    return true;
}

void GameInterface::processSprites(tspriteArray& tsprites, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio)
{
    analyzesprites(tsprites, viewx, viewy, viewz, viewang.asbuild());
    post_analyzesprites(tsprites);
}



END_SW_NS
