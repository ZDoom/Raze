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
extern int f_c;

extern TILE_INFO_TYPE aVoxelArray[MAXTILES];

void PreDrawStackedWater(void);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetRotation(tspriteArray& tsprites, int tSpriteNum, const DVector2& view)
{
    static const uint8_t RotTable8[] = {0, 7, 6, 5, 4, 3, 2, 1};
    static const uint8_t RotTable5[] = {0, 1, 2, 3, 4, 3, 2, 1};
    int rotation;

    tspritetype* tsp = tsprites.get(tSpriteNum);
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);

    if (!ownerActor->hasU() || ownerActor->user.RotNum == 0)
        return 0;

    // Get which of the 8 angles of the sprite to draw (0-7)
    // rotation ranges from 0-7
    DAngle angle2 = VecToAngle(tsp->pos.X - view.X, tsp->pos.Y - view.Y);
    rotation = (tsp->angle + DAngle180 + DAngle22_5 * 0.5 - angle2).Buildang() & 2047;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

/*


!AIC - At draw time this is called for actor rotation.  GetRotation() is more
complex than needs to be in part because importing of actor rotations and x-flip
directions was not standardized.

*/

int SetActorRotation(tspriteArray& tsprites, int tSpriteNum, const DVector2& viewpos)
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
    Rotation = GetRotation(tsprites, tSpriteNum, viewpos);

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

double DoShadowFindGroundPoint(tspritetype* tspr)
{
    // USES TSPRITE !!!!!
    auto ownerActor = static_cast<DSWActor*>(tspr->ownerActor);
    Collision ceilhit, florhit;
    double hiz, loz = ownerActor->user.loz;
    ESpriteFlags save_cstat, bak_cstat;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This will return invalid FAF ceiling and floor heights inside of analyzesprite
    // because the ceiling and floors get moved out of the way for drawing.

    save_cstat = tspr->cstat;
    tspr->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(tspr->pos, tspr->sectp, &hiz, &ceilhit, &loz, &florhit);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoShadows(tspriteArray& tsprites, tspritetype* tsp, double viewz)
{
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    int ground_dist = 0;
    int view_dist = 0;
    double loz;
    int xrepeat;
    int yrepeat;

    if (!ownerActor->hasU()) return;

    auto sect = tsp->sectp;
    // make sure its the correct sector
    // DoShadowFindGroundPoint calls FAFgetzrangepoint and this is sensitive
    updatesector(tsp->pos, &sect);

    if (sect == nullptr)
    {
        return;
    }

    tsp->sectp = sect;

    if ((tsp->yrepeat >> 2) > 4)
    {
		int sizey = MulScale(tileHeight(tsp->picnum), tsp->yrepeat, 6);
        yrepeat = (tsp->yrepeat >> 2) - (sizey / 64) * 2;
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
    if ((viewz - loz) > -8)
    {
        return;
    }

    tspritetype* tSpr = tsprites.newTSprite();
    *tSpr = *tsp;
    // shadow is ALWAYS draw last - status is priority
    tSpr->statnum = MAXSTATUS;
    tSpr->shade = 127;
    tSpr->cstat |= CSTAT_SPRITE_TRANSLUCENT;
    tSpr->pos.Z = loz;

    // if close to shadows z shrink it
    view_dist = int(abs(loz - viewz));
    if (view_dist < 32)
        view_dist = 256/view_dist;
    else
        view_dist = 0;

    // make shadow smaller depending on height from ground
    ground_dist = int(abs(loz - GetSpriteZOfBottom(tsp)) * (1./16));

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
        auto look = VecToAngle(tSpr->pos.XY() - Player[screenpeek].si.XY());
		tSpr->pos.XY() += look.ToVector() * 2;
    }

    // Check for voxel items and use a round generic pic if so
    //DoVoxelShadow(New);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoMotionBlur(tspriteArray& tsprites, tspritetype const * const tsp)
{
    auto ownerActor = static_cast<DSWActor*>(tsp->ownerActor);
    DVector3 npos(0, 0, 0), dpos(0, 0, 0);
    int i;
    int xrepeat, yrepeat, repeat_adj = 0;
    double z_amt_per_pixel;

    auto angle = tsp->angle + DAngle180;

    if (!ownerActor->hasU() || ownerActor->vel.X == 0)
    {
        return;
    }

    if ((tsp->extra & SPRX_PLAYER_OR_ENEMY))
    {
        z_amt_per_pixel = (- ownerActor->user.jump_speed * ACTORMOVETICS) * maptoworld / tsp->ownerActor->vel.X;
    }
    else
    {
        z_amt_per_pixel = -ownerActor->vel.Z / tsp->ownerActor->vel.X;
    }

    dpos.XY() = npos.XY() = angle.ToVector() * ownerActor->user.motion_blur_dist;
    dpos.Z = npos.Z = z_amt_per_pixel * ownerActor->user.motion_blur_dist * (1./16);

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

        tSpr->pos += dpos;
        dpos += npos;

        tSpr->xrepeat = uint8_t(xrepeat);
        tSpr->yrepeat = uint8_t(yrepeat);

        xrepeat -= repeat_adj;
        yrepeat -= repeat_adj;
    }

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WarpCopySprite(tspriteArray& tsprites)
{
    int spnum;
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
					newTSpr->pos += itActor1->spr.pos - itActor->spr.pos;
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

                    newTSpr->pos += itActor->spr.pos - itActor1->spr.pos;
                    newTSpr->sectp = itActor->sector();
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

void DoStarView(tspritetype* tsp, DSWActor* tActor, double viewz)
{
    extern STATE s_Star[], s_StarDown[];
    extern STATE s_StarStuck[], s_StarDownStuck[];
    double zdiff = viewz - tsp->pos.Z;

    if (abs(zdiff) > 24)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* CopySprite(spritetypebase const* tsp, sectortype* newsector)
{

    auto actorNew = insertActor(newsector, STAT_FAF_COPY);

	actorNew->spr.pos = tsp->pos;
    actorNew->spr.cstat = tsp->cstat;
    actorNew->spr.picnum = tsp->picnum;
    actorNew->spr.pal = tsp->pal;
    actorNew->spr.xrepeat = tsp->xrepeat;
    actorNew->spr.yrepeat = tsp->yrepeat;
    actorNew->spr.xoffset = tsp->xoffset;
    actorNew->spr.yoffset = tsp->yoffset;
    actorNew->spr.angle = tsp->angle;
    actorNew->spr.xint = tsp->xint;
    actorNew->spr.yint = tsp->yint;
    actorNew->spr.inittype = tsp->inittype;
    actorNew->spr.shade = tsp->shade;
    // this later also needs to copy the real velocity, because the sprite renderer accesses it. :(

    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    return actorNew;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DSWActor* ConnectCopySprite(spritetypebase const* tsp)
{
    sectortype* newsector;
    double testz;

    if (FAF_ConnectCeiling(tsp->sectp))
    {
        newsector = tsp->sectp;
        testz = GetSpriteZOfTop(tsp) - 10;

        if (testz < tsp->sectp->ceilingz)
            updatesectorz(DVector3(tsp->pos, testz), &newsector);

        if (newsector != nullptr && newsector != tsp->sectp)
        {
            return CopySprite(tsp, newsector);
        }
    }

    if (FAF_ConnectFloor(tsp->sectp))
    {
        newsector = tsp->sectp;
        testz = GetSpriteZOfBottom(tsp) + 10;

        if (testz > tsp->sectp->floorz)
            updatesectorz(DVector3(tsp->pos, testz), &newsector);

        if (newsector != nullptr && newsector != tsp->sectp)
        {
            return CopySprite(tsp, newsector);
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void analyzesprites(tspriteArray& tsprites, const DVector3& viewpos, double interpfrac)
{
    int tSpriteNum;
    static int ang = 0;
    PLAYER* pp = Player + screenpeek;
    int newshade=0;

    const int DART_PIC = 2526;
    const int DART_REPEAT = 16;

    ang = NORM_ANGLE(ang + 12);

    double smr4 = interpfrac + MoveSkip4;
    double smr2 = interpfrac + MoveSkip2;

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
                        tsp->pos = tActor->interpolatedpos(smr4 * 0.25);
                    }
                }

                if ((tActor->user.Flags & SPR_SKIP2))
                {
                    if (tsp->statnum <= STAT_SKIP2_INTERP_END)
                    {
                        tsp->pos = tActor->interpolatedpos(smr2 * 0.5);
                    }
                }
            }

            // workaround for mines and floor decals beneath the floor
            if (tsp->picnum == BETTY_R0 || tsp->picnum == FLOORBLOOD1)
            {
                double const florz = getflorzofslopeptrf(tActor->sector(), tActor->spr.pos);
                if (tActor->spr.pos.Z > florz)
                    tsp->pos.Z = florz;
            }

            if (r_shadows && (tActor->user.Flags & SPR_SHADOW))
            {
                DoShadows(tsprites, tsp, viewpos.Z);
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
                    tsp->angle -= DAngle90 + DAngle::fromBuild(24);
                    tsp->xrepeat = tsp->yrepeat = DART_REPEAT;
                    tsp->cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
                }
                else
                    DoStarView(tsp, tActor, viewpos.Z);
            }

            // rotation
            if (tActor->user.RotNum > 0)
                SetActorRotation(tsprites, tSpriteNum, viewpos.XY());

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
                tsp->angle -= DAngle90;
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

                    DVector3 pos;
                    if (pp->Flags & (PF_CLIMBING))
                    {
                        // move sprite forward some so he looks like he's
                        // climbing
                        pos.XY() = pp->si.XY() + tsp->angle.ToVector() * 13;
                    }
                    else
                    {
                        pos.X = pp->si.X;
                        pos.Y = pp->si.Y;
                    }

                    pos.Z = tsp->pos.Z + pp->si.Z;
					tsp->pos = pos;
                    tsp->angle = pp->siang;
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
                tsp->pos = interpolatedvalue(pp->opos, pp->pos, interpfrac);
                tsp->angle = pp->angle.interpolatedang(interpfrac);
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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

                tsp->pos.X = atsp->pos.X;
                tsp->pos.Y = atsp->pos.Y;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

std::pair<DVector3, DAngle> GameInterface::GetCoordinates()
{
    PLAYER* pp = Player + myconnectindex;
    return std::make_pair(pp->pos, pp->angle.ang);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PrintSpriteInfo(PLAYER* pp)
{
    const int Y_STEP = 7;

    //if (SpriteInfo && !LocationInfo)
    {
        auto actor = DoPickTarget(pp->actor, DAngle22_5/4, 2);

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
            Printf("POSX:%2.3f, ", actor->spr.pos.X);
            Printf("POSY:%2.3f, ", actor->spr.pos.Y);
            Printf("POSZ:%2.3f,", actor->spr.pos.Z);
            Printf("ANG:%2.0f\n", actor->spr.angle.Degrees());
        }
    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void DrawCrosshair(PLAYER* pp, const double inputfrac)
{
    ::DrawCrosshair(2326, pp->actor->user.Health, -pp->angle.look_anghalf(inputfrac), (pp->Flags & PF_VIEW_FROM_OUTSIDE) ? 5 : 0, 2, shadeToLight(10));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
                if (itActor2->spr.intangle == 0x4711)
                    continue;

                auto actorNew = ConnectCopySprite(&itActor2->spr);
                if (actorNew != nullptr)
                {
                    // spawn a user
                    actorNew->allocUser();

					actorNew->spr.intangle = 0x4711;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void drawscreen(PLAYER* pp, double interpfrac, bool sceneonly)
{
    DAngle tang, trotscrnang;
    fixedhoriz thoriz;
    sectortype* tsect;

    // prediction player if prediction is on, else regular player
    PLAYER* camerapp = (PredictionOn && CommEnabled && pp == Player+myconnectindex) ? ppp : pp;

    PreDraw();
    PreUpdatePanel(interpfrac);

    if (!sceneonly)
    {
        // Stick at beginning of drawscreen
        DoInterpolations(interpfrac);
        if (cl_sointerpolation) so_dointerpolations(interpfrac);
    }

    // Get initial player position, interpolating if required.
    DVector3 tpos = interpolatedvalue(camerapp->opos, camerapp->pos, interpfrac);
    if (SyncInput() || pp != Player+myconnectindex)
    {
        tang = camerapp->angle.interpolatedsum(interpfrac);
        thoriz = camerapp->horizon.interpolatedsum(interpfrac);
        trotscrnang = camerapp->angle.interpolatedrotscrn(interpfrac);
    }
    else
    {
        tang = pp->angle.sum();
        thoriz = pp->horizon.sum();
        trotscrnang = pp->angle.rotscrnang;
    }
    tsect = camerapp->cursector;

    updatesector(tpos, &tsect);

    if (pp->sop_riding || pp->sop_control)
    {
        if (pp->sop_control && (!cl_sointerpolation || (CommEnabled && !pp->sop_remote)))
        {
            tpos = pp->pos;
            tang = pp->angle.ang;
        }
        tsect = pp->cursector;
        updatesectorz(tpos, &tsect);
    }

    pp->si = tpos.plusZ(-pp->pos.Z);
    pp->siang = tang;

    QuakeViewChange(camerapp, tpos, tang);
    int vis = g_visibility;
    VisViewChange(camerapp, &vis);
    g_relvisibility = vis - g_visibility;

    if (pp->sop_remote)
    {
        DSWActor* ractor = pp->remoteActor;
        tang = TEST_BOOL1(ractor) ? ractor->spr.angle : (pp->sop_remote->pmid.XY() - tpos.XY()).Angle();
    }

    if (pp->Flags & (PF_VIEW_FROM_OUTSIDE))
    {
        tpos.Z -= 33;

        if (!calcChaseCamPos(tpos, pp->actor, &tsect, tang, thoriz, interpfrac))
        {
            tpos.Z += 33;
            calcChaseCamPos(tpos, pp->actor, &tsect, tang, thoriz, interpfrac);
        }
    }

    if (!(pp->Flags & (PF_VIEW_FROM_CAMERA|PF_VIEW_FROM_OUTSIDE)))
    {
        if (cl_viewbob)
        {
            tpos.Z += interpolatedvalue(pp->obob_z + pp->opbob_amt, pp->bob_z + pp->pbob_amt, interpfrac);
        }

        // recoil only when not in camera
        thoriz = q16horiz(clamp(thoriz.asq16() + interpolatedvalue(pp->recoil_ohorizoff, pp->recoil_horizoff, interpfrac), gi->playerHorizMin(), gi->playerHorizMax()));
    }

    if (automapMode != am_full)
    {
        // Cameras must be done before the main loop.
        JS_CameraParms(pp, tpos);  
    }

    if (!sceneonly)
        UpdatePanel(interpfrac);

    UpdateWallPortalState();
    render_drawrooms(pp->actor, tpos, sectnum(tsect), tang, thoriz, trotscrnang, interpfrac);
    RestorePortalState();

    if (sceneonly)
        return;

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
        DrawOverheadMap(tpos.XY(), tang, interpfrac);
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

    UpdateStatusBar();
    DrawCrosshair(pp, interpfrac);

    // Do the underwater breathing bar
    DoPlayerDiveMeter(pp);

    // Boss Health Meter, if Boss present
    BossHealthMeter();

    // Stick at end of drawscreen
    RestoreInterpolations();
    if (cl_sointerpolation) so_restoreinterpolations();

    if (paused && !M_Active())
    {
        auto str = GStrings("Game Paused");
        auto font = PickSmallFont(str);
        int w = font->StringWidth(str);
        DrawText(twod, font, CR_UNTRANSLATED, 160-w, 100, str, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
    }

    if (!CommEnabled && (pp->Flags & PF_DEAD) && ReloadPrompt)
        ReloadPrompt = false;

    PostDraw();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::GenerateSavePic()
{
    drawscreen(Player + myconnectindex, 65536, true);
    return true;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool GameInterface::DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac)
{
    static int pspr_ndx[8] = { 0,0,0,0,0,0,0,0 };
    bool sprisplayer = false;

    // Pre-caculate incoming angle vector.
    auto cangvect = cang.ToVector();

    // Draw sprites
    if (gFullMap)
    {
        for (unsigned i = 0; i < sector.Size(); i++)
        {
            SWSectIterator it(i);
            while (auto actor = it.Next())
            {
                if (actor->spr.cstat2 & CSTAT2_SPRITE_MAPPED)
                {
                    // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
                    PalEntry col = (actor->spr.cstat & CSTAT_SPRITE_BLOCK) > 0 ? GPalette.BaseColors[248] : actor == Player[screenpeek].actor ? GPalette.BaseColors[31] : GPalette.BaseColors[56];
                    auto statnum = actor->spr.statnum;
                    auto sprxy = ((statnum >= 1) && (statnum <= 8) && (statnum != 2) ? actor->interpolatedpos(interpfrac) : actor->spr.pos).XY() - cpos;

                    switch (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
                    {
                    case CSTAT_SPRITE_ALIGNMENT_FACING:  // Regular sprite
                        DrawAutomapAlignmentFacing(actor->spr, sprxy, cangvect, czoom, xydim, col);
                        break;
                    case CSTAT_SPRITE_ALIGNMENT_WALL: // Rotated sprite
                        DrawAutomapAlignmentWall(actor->spr, sprxy, cangvect, czoom, xydim, col);
                        break;
                    case CSTAT_SPRITE_ALIGNMENT_FLOOR:    // Floor sprite
                        if (automapMode == am_overlay) DrawAutomapAlignmentFloor(actor->spr, sprxy, cangvect, czoom, xydim, col);
                        break;
                    }
                }
            }
        }
    }

    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (p == screenpeek)
        {
            auto actor = Player[p].actor;
            if (actor->vel.X > 1) pspr_ndx[myconnectindex] = ((PlayClock >> 4) & 3);
            sprisplayer = true;

            if (czoom > 0.1875)
            {
                // Special case tiles
                if (actor->spr.picnum == 3123) break;

                int spnum = -1;
                if (sprisplayer)
                {
                    if (gNet.MultiGameType != MULTI_GAME_COMMBAT || actor == Player[screenpeek].actor)
                        spnum = 1196 + pspr_ndx[myconnectindex];
                }
                else spnum = actor->spr.picnum;

                if (spnum >= 0)
                {
                    const auto daang = -((!SyncInput() ? actor->spr.angle : actor->interpolatedangle(interpfrac)) - cang).Normalized360().Degrees();
                    auto vect = OutAutomapVector(mxy - cpos, cangvect, czoom, xydim);

                    // This yrepeat scale is correct.
                    double sc = czoom * actor->spr.yrepeat * (1. / 32.);

                    DrawTexture(twod, tileGetTexture(1196 + pspr_ndx[myconnectindex], true), vect.X, vect.Y, DTA_ScaleX, sc, DTA_ScaleY, sc, DTA_Rotate, daang,
                        DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, actor->spr.pal), DTA_Color, shadeToLight(actor->spr.shade),
                        DTA_Alpha, (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT) ? 0.33 : 1., TAG_DONE);
                }
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::processSprites(tspriteArray& tsprites, int viewx, int viewy, int viewz, DAngle viewang, double interpfrac)
{
    analyzesprites(tsprites, DVector3(viewx * inttoworld, viewy * inttoworld, viewz * zinttoworld), interpfrac);
    post_analyzesprites(tsprites);
}



END_SW_NS
