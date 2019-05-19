//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

// this is all the crap for accessing the game's structs through the CON VM
// I got a 3-4 fps gain by inlining these...

#include "compat.h"
#include "m32script.h"
#include "m32def.h"
#ifdef POLYMER
# include "prlights.h"
#endif

// how: bitfield: 1=set? 2=vars? 4=use spriteext[].tspr? (otherwise use tsprite[])
#define ACCESS_SET 1
#define ACCESS_USEVARS 2


/// This file is #included into other files, so don't define variables here!


static int32_t __fastcall VM_AccessWall(int32_t how, int32_t lVar1, int32_t labelNum, int32_t lVar2)
{
    int32_t lValue;
    int32_t i = (how&ACCESS_USEVARS) ? Gv_GetVar(lVar1) : lVar1;

    if (!m32_script_expertmode && (i<0 || i >= numwalls))
        goto badwall;

    if (how&ACCESS_SET)
    {
        if (!m32_script_expertmode && (WallLabels[labelNum].flags & 1))
            goto readonly;

        lValue = (how&ACCESS_USEVARS) ? Gv_GetVar(lVar2) : lVar2;

        asksave = 1;

        if (!m32_script_expertmode && (WallLabels[labelNum].min != 0 || WallLabels[labelNum].max != 0))
        {
            if (lValue < WallLabels[labelNum].min)
                lValue = WallLabels[labelNum].min;
            if (lValue > WallLabels[labelNum].max)
                lValue = WallLabels[labelNum].max;
        }

        switch (labelNum)
        {
        case WALL_X: wall[i].x=lValue; break;
        case WALL_Y: wall[i].y=lValue; break;
        case WALL_POINT2: wall[i].point2=lValue; break;
        case WALL_NEXTWALL: wall[i].nextwall=lValue; break;
        case WALL_NEXTSECTOR: wall[i].nextsector=lValue; break;
        case WALL_CSTAT:
#ifdef YAX_ENABLE__COMPAT
            if (!m32_script_expertmode)
                SET_PROTECT_BITS(wall[i].cstat, lValue, YAX_NEXTWALLBITS);
            else
#endif
                wall[i].cstat = lValue;
            break;
        case WALL_PICNUM: wall[i].picnum=lValue; break;
        case WALL_OVERPICNUM: wall[i].overpicnum=lValue; break;
        case WALL_SHADE: wall[i].shade=lValue; break;
        case WALL_PAL: wall[i].pal=lValue; break;
        case WALL_XREPEAT: wall[i].xrepeat=lValue; break;
        case WALL_YREPEAT: wall[i].yrepeat=lValue; break;
        case WALL_XPANNING: wall[i].xpanning=lValue; break;
        case WALL_YPANNING: wall[i].ypanning=lValue; break;
        case WALL_LOTAG:
#ifdef YAX_ENABLE__COMPAT
            if (!m32_script_expertmode && numyaxbunches>0 && yax_getnextwall(i,YAX_CEILING)>=0)
                goto yax_readonly;
#endif
            wall[i].lotag=lValue;
            break;
        case WALL_HITAG: wall[i].hitag=lValue; break;
        case WALL_EXTRA:
#ifdef YAX_ENABLE__COMPAT
            if (!m32_script_expertmode && numyaxbunches>0 && yax_getnextwall(i,YAX_FLOOR)>=0)
                goto yax_readonly;
#endif
            wall[i].extra=lValue;
            break;
        default:
            return -1;
        }

        return 0;
    }
    else
    {
        switch (labelNum)
        {
        case WALL_X: lValue=wall[i].x; break;
        case WALL_Y: lValue=wall[i].y; break;
        case WALL_POINT2: lValue=wall[i].point2; break;
        case WALL_NEXTWALL: lValue=wall[i].nextwall; break;
        case WALL_NEXTSECTOR: lValue=wall[i].nextsector; break;
        case WALL_CSTAT: lValue=wall[i].cstat; break;
        case WALL_PICNUM: lValue=wall[i].picnum; break;
        case WALL_OVERPICNUM: lValue=wall[i].overpicnum; break;
        case WALL_SHADE: lValue=wall[i].shade; break;
        case WALL_PAL: lValue=wall[i].pal; break;
        case WALL_XREPEAT: lValue=wall[i].xrepeat; break;
        case WALL_YREPEAT: lValue=wall[i].yrepeat; break;
        case WALL_XPANNING: lValue=wall[i].xpanning; break;
        case WALL_YPANNING: lValue=wall[i].ypanning; break;
        case WALL_LOTAG: lValue=wall[i].lotag; break;
        case WALL_HITAG: lValue=wall[i].hitag; break;
        case WALL_EXTRA: lValue=wall[i].extra; break;
        default:
            return -1;
        }

        if (how&ACCESS_USEVARS)
            Gv_SetVar(lVar2, lValue);

        return lValue;
    }

badwall:
    M32_ERROR("Invalid wall %d", i);
    return -1;
readonly:
    M32_ERROR("Wall structure member `%s' is read-only.", WallLabels[labelNum].name);
    return -1;
#ifdef YAX_ENABLE__COMPAT
yax_readonly:
    M32_ERROR("Wall structure member `%s' is read-only because it is used for TROR",
              WallLabels[labelNum].name);
    return -1;
#endif
}

// how: bitfield: 1=set? 2=vars?
static int32_t __fastcall VM_AccessSector(int32_t how, int32_t lVar1, int32_t labelNum, int32_t lVar2)
{
    int32_t lValue;
    int32_t i = (how&ACCESS_USEVARS) ? sprite[vm.spriteNum].sectnum : lVar1;

    if ((how&ACCESS_USEVARS) && lVar1 != M32_THISACTOR_VAR_ID)
        i = Gv_GetVar(lVar1);

    if (!m32_script_expertmode && (i<0 || i >= numsectors))
        goto badsector;

    if (how&ACCESS_SET)
    {
        if (!m32_script_expertmode && (SectorLabels[labelNum].flags & 1))
            goto readonly;

        lValue = (how&ACCESS_USEVARS) ? Gv_GetVar(lVar2) : lVar2;

        asksave = 1;

        if (!m32_script_expertmode && (SectorLabels[labelNum].min != 0 || SectorLabels[labelNum].max != 0))
        {
            if (lValue < SectorLabels[labelNum].min)
                lValue = SectorLabels[labelNum].min;
            if (lValue > SectorLabels[labelNum].max)
                lValue = SectorLabels[labelNum].max;
        }

        switch (labelNum)
        {
        case SECTOR_WALLPTR: sector[i].wallptr=lValue; break;
        case SECTOR_WALLNUM: sector[i].wallnum=lValue; break;
        case SECTOR_CEILINGZ: sector[i].ceilingz=lValue; break;
        case SECTOR_FLOORZ: sector[i].floorz=lValue; break;
        case SECTOR_CEILINGSTAT:
#ifdef YAX_ENABLE__COMPAT
            if (!m32_script_expertmode)
                SET_PROTECT_BITS(sector[i].ceilingstat, lValue, YAX_BIT);
            else
#endif
                sector[i].ceilingstat = lValue;
            break;
        case SECTOR_FLOORSTAT:
#ifdef YAX_ENABLE__COMPAT
            if (!m32_script_expertmode)
                SET_PROTECT_BITS(sector[i].floorstat, lValue, YAX_BIT);
            else
#endif
                sector[i].floorstat = lValue;
            break;
        case SECTOR_CEILINGPICNUM: sector[i].ceilingpicnum=lValue; break;
        case SECTOR_CEILINGSLOPE:
            setslope(i, 0, lValue);
            break;
        case SECTOR_CEILINGSHADE: sector[i].ceilingshade=lValue; break;
        case SECTOR_CEILINGPAL: sector[i].ceilingpal=lValue; break;
        case SECTOR_CEILINGXPANNING: sector[i].ceilingxpanning=lValue; break;
        case SECTOR_CEILINGYPANNING: sector[i].ceilingypanning=lValue; break;
        case SECTOR_FLOORPICNUM: sector[i].floorpicnum=lValue; break;
        case SECTOR_FLOORSLOPE:
            setslope(i, 1, lValue);
            break;
        case SECTOR_FLOORSHADE: sector[i].floorshade=lValue; break;
        case SECTOR_FLOORPAL: sector[i].floorpal=lValue; break;
        case SECTOR_FLOORXPANNING: sector[i].floorxpanning=lValue; break;
        case SECTOR_FLOORYPANNING: sector[i].floorypanning=lValue; break;
        case SECTOR_VISIBILITY: sector[i].visibility=lValue; break;
        case SECTOR_FOGPAL: sector[i].fogpal=lValue; break;
        case SECTOR_LOTAG: sector[i].lotag=lValue; break;
        case SECTOR_HITAG: sector[i].hitag=lValue; break;
        case SECTOR_EXTRA: sector[i].extra=lValue; break;
        default:
            return -1;
        }

        return 0;
    }
    else
    {
        switch (labelNum)
        {
        case SECTOR_WALLPTR: lValue=sector[i].wallptr; break;
        case SECTOR_WALLNUM: lValue=sector[i].wallnum; break;
        case SECTOR_CEILINGZ: lValue=sector[i].ceilingz; break;
        case SECTOR_FLOORZ: lValue=sector[i].floorz; break;
        case SECTOR_CEILINGSTAT: lValue=sector[i].ceilingstat; break;
        case SECTOR_FLOORSTAT: lValue=sector[i].floorstat; break;
        case SECTOR_CEILINGPICNUM: lValue=sector[i].ceilingpicnum; break;
        case SECTOR_CEILINGSLOPE: lValue=sector[i].ceilingheinum; break;
        case SECTOR_CEILINGSHADE: lValue=sector[i].ceilingshade; break;
        case SECTOR_CEILINGPAL: lValue=sector[i].ceilingpal; break;
        case SECTOR_CEILINGXPANNING: lValue=sector[i].ceilingxpanning; break;
        case SECTOR_CEILINGYPANNING: lValue=sector[i].ceilingypanning; break;
        case SECTOR_FLOORPICNUM: lValue=sector[i].floorpicnum; break;
        case SECTOR_FLOORSLOPE: lValue=sector[i].floorheinum; break;
        case SECTOR_FLOORSHADE: lValue=sector[i].floorshade; break;
        case SECTOR_FLOORPAL: lValue=sector[i].floorpal; break;
        case SECTOR_FLOORXPANNING: lValue=sector[i].floorxpanning; break;
        case SECTOR_FLOORYPANNING: lValue=sector[i].floorypanning; break;
        case SECTOR_VISIBILITY: lValue=sector[i].visibility; break;
        case SECTOR_FOGPAL: lValue=sector[i].fogpal; break;
        case SECTOR_LOTAG: lValue=sector[i].lotag; break;
        case SECTOR_HITAG: lValue=sector[i].hitag; break;
        case SECTOR_EXTRA: lValue=sector[i].extra; break;
        default:
            return -1;
        }

        if (how&ACCESS_USEVARS)
            Gv_SetVar(lVar2, lValue);

        return lValue;
    }

badsector:
    M32_ERROR("Invalid sector %d", i);
    return -1;
readonly:
    M32_ERROR("Sector structure member `%s' is read-only.", SectorLabels[labelNum].name);
    return -1;
}

// how: bitfield: 1=set? 2=vars?
static int32_t __fastcall VM_AccessSprite(int32_t how, int32_t lVar1, int32_t labelNum, int32_t lVar2)
{
    int32_t lValue;
    int32_t i = (how&ACCESS_USEVARS) ? vm.spriteNum : lVar1;

    if ((how&ACCESS_USEVARS) && lVar1 != M32_THISACTOR_VAR_ID)
        i = Gv_GetVar(lVar1);

    if ((unsigned)i >= MAXSPRITES)
        goto badsprite;

    if (how&ACCESS_SET)
    {
        if (!m32_script_expertmode && (SpriteLabels[labelNum].flags & 1))
            goto readonly;

        lValue = (how&ACCESS_USEVARS) ? Gv_GetVar(lVar2) : lVar2;

        asksave = 1;

        if (!m32_script_expertmode && (SpriteLabels[labelNum].min != 0 || SpriteLabels[labelNum].max != 0))
        {
            if (lValue < SpriteLabels[labelNum].min)
                lValue = SpriteLabels[labelNum].min;
            if (lValue > SpriteLabels[labelNum].max)
                lValue = SpriteLabels[labelNum].max;
        }

        switch (labelNum)
        {
        case SPRITE_X: sprite[i].x=lValue; break;
        case SPRITE_Y: sprite[i].y=lValue; break;
        case SPRITE_Z: sprite[i].z=lValue; break;
        case SPRITE_CSTAT: sprite[i].cstat = lValue; break;
        case SPRITE_PICNUM: sprite[i].picnum=lValue; break;
        case SPRITE_SHADE: sprite[i].shade=lValue; break;
        case SPRITE_PAL: sprite[i].pal=lValue; break;
        case SPRITE_CLIPDIST: sprite[i].clipdist=lValue; break;
        case SPRITE_BLEND: sprite[i].blend=lValue; break;
        case SPRITE_XREPEAT: sprite[i].xrepeat=lValue; break;
        case SPRITE_YREPEAT: sprite[i].yrepeat=lValue; break;
        case SPRITE_XOFFSET: sprite[i].xoffset=lValue; break;
        case SPRITE_YOFFSET: sprite[i].yoffset=lValue; break;
        case SPRITE_SECTNUM: changespritesect(i,lValue); break;
        case SPRITE_STATNUM: changespritestat(i,lValue); break;
        case SPRITE_ANG:
            sprite[i].ang = lValue&2047;
            break;
        case SPRITE_OWNER: sprite[i].owner=lValue; break;
        case SPRITE_XVEL: sprite[i].xvel=lValue; break;
        case SPRITE_YVEL: sprite[i].yvel=lValue; break;
        case SPRITE_ZVEL: sprite[i].zvel=lValue; break;
        case SPRITE_LOTAG: sprite[i].lotag=lValue; break;
        case SPRITE_HITAG: sprite[i].hitag=lValue; break;
        case SPRITE_EXTRA: sprite[i].extra=lValue; break;
        default:
            return -1;
        }

        return 0;
    }
    else
    {
        switch (labelNum)
        {
        case SPRITE_X: lValue=sprite[i].x; break;
        case SPRITE_Y: lValue=sprite[i].y; break;
        case SPRITE_Z: lValue=sprite[i].z; break;
        case SPRITE_CSTAT: lValue=sprite[i].cstat; break;
        case SPRITE_PICNUM: lValue=sprite[i].picnum; break;
        case SPRITE_SHADE: lValue=sprite[i].shade; break;
        case SPRITE_PAL: lValue=sprite[i].pal; break;
        case SPRITE_CLIPDIST: lValue=sprite[i].clipdist; break;
        case SPRITE_BLEND: lValue=sprite[i].blend; break;
        case SPRITE_XREPEAT: lValue=sprite[i].xrepeat; break;
        case SPRITE_YREPEAT: lValue=sprite[i].yrepeat; break;
        case SPRITE_XOFFSET: lValue=sprite[i].xoffset; break;
        case SPRITE_YOFFSET: lValue=sprite[i].yoffset; break;
        case SPRITE_SECTNUM: lValue=sprite[i].sectnum; break;
        case SPRITE_STATNUM: lValue=sprite[i].statnum; break;
        case SPRITE_ANG: lValue=sprite[i].ang; break;
        case SPRITE_OWNER: lValue=sprite[i].owner; break;
        case SPRITE_XVEL: lValue=sprite[i].xvel; break;
        case SPRITE_YVEL: lValue=sprite[i].yvel; break;
        case SPRITE_ZVEL: lValue=sprite[i].zvel; break;
        case SPRITE_LOTAG: lValue=sprite[i].lotag; break;
        case SPRITE_HITAG: lValue=sprite[i].hitag; break;
        case SPRITE_EXTRA: lValue=sprite[i].extra; break;
        default:
            return -1;
        }

        if (how&ACCESS_USEVARS)
            Gv_SetVar(lVar2, lValue);

        return lValue;
    }
badsprite:
    M32_ERROR("tried to set %s on invalid target sprite (%d)", SpriteLabels[labelNum].name, i);
    return -1;
readonly:
    M32_ERROR("sprite structure member `%s' is read-only.", SpriteLabels[labelNum].name);
    return -1;
}

// how: bitfield: 1=set? 2=vars? 4=use spriteext[].tspr? (otherwise use tsprite[])
static int32_t __fastcall VM_AccessTsprite(int32_t how, int32_t lVar1, int32_t labelNum, int32_t lVar2)
{
    int32_t lightp = (labelNum >= LIGHT_X);
    int32_t i = (how&ACCESS_USEVARS) ? vm.spriteNum : lVar1;
    uspritetype *datspr = NULL;
    const memberlabel_t *dalabel = lightp ? &LightLabels[labelNum-LIGHT_X] : &SpriteLabels[labelNum];

    if ((how&ACCESS_USEVARS) && lVar1 != M32_THISACTOR_VAR_ID)
        i = Gv_GetVar(lVar1);

    if (!lightp)
    {
        if (i<0 || i>=spritesortcnt)
            goto badsprite;
        datspr = &tsprite[i];
    }
    else
    {
        // access Polymer light
#ifndef POLYMER
        M32_ERROR("Polymer not compiled in, accessing lights is invalid.");
        return -1;
#else
        if ((how&ACCESS_USEVARS) && lVar1 == M32_THISACTOR_VAR_ID)
        {
            if ((unsigned)i >= MAXSPRITES)
                goto badsprite;
            M32_ERROR("Polymer light access via current sprite not implemented.");
            return -1;
        }
        else
        {
// check whether videoGetRenderMode() == REND_POLYMER ?
            if ((unsigned)i >= PR_MAXLIGHTS)
            {
                M32_ERROR("invalid light index (%d)", i);
                return -1;
            }

            if (labelNum != LIGHT_ACTIVE && !prlights[i].flags.active)
            {
                M32_ERROR("light with index %d is inactive!", i);
                return -1;
            }
        }
#endif
    }

    if (how&ACCESS_SET)
    {
        int32_t lValue, damin, damax;

        if (!m32_script_expertmode && (dalabel->flags & 1))
            goto readonly;

        lValue = (how&ACCESS_USEVARS) ? Gv_GetVar(lVar2) : lVar2;

        damin = dalabel->min;
        damax = dalabel->max;

        if (!m32_script_expertmode && (damin|damax))
        {
            if (lValue < damin)
                lValue = damin;
            if (lValue > damax)
                lValue = damax;
        }

        switch (labelNum)
        {
        case SPRITE_X: datspr->x=lValue; break;
        case SPRITE_Y: datspr->y=lValue; break;
        case SPRITE_Z: datspr->z=lValue; break;
        case SPRITE_CSTAT: datspr->cstat = lValue; break;
        case SPRITE_PICNUM: datspr->picnum=lValue; break;
        case SPRITE_SHADE: datspr->shade=lValue; break;
        case SPRITE_PAL: datspr->pal=lValue; break;
        case SPRITE_CLIPDIST: datspr->clipdist=lValue; break;
        case SPRITE_BLEND: datspr->blend=lValue; break;
        case SPRITE_XREPEAT: datspr->xrepeat=lValue; break;
        case SPRITE_YREPEAT: datspr->yrepeat=lValue; break;
        case SPRITE_XOFFSET: datspr->xoffset=lValue; break;
        case SPRITE_YOFFSET: datspr->yoffset=lValue; break;
        case SPRITE_SECTNUM: datspr->sectnum=lValue; break;
        case SPRITE_STATNUM: datspr->statnum=lValue; break;
        case SPRITE_ANG:
            datspr->ang = lValue&2047;
            break;
        case SPRITE_OWNER: datspr->owner=lValue; break;
        case SPRITE_XVEL: datspr->xvel=lValue; break;
        case SPRITE_YVEL: datspr->yvel=lValue; break;
        case SPRITE_ZVEL: datspr->zvel=lValue; break;
        case SPRITE_LOTAG: datspr->lotag=lValue; break;
        case SPRITE_HITAG: datspr->hitag=lValue; break;
        case SPRITE_EXTRA: datspr->extra=lValue; break;
#ifdef POLYMER
        // lights
        case LIGHT_X: prlights[i].x = lValue; break;
        case LIGHT_Y: prlights[i].y = lValue; break;
        case LIGHT_Z: prlights[i].z = lValue; break;
        case LIGHT_HORIZ: prlights[i].horiz = lValue; break;
        case LIGHT_RANGE: prlights[i].range = lValue; break;
        case LIGHT_ANGLE: prlights[i].angle = lValue; break;
        case LIGHT_FADERADIUS: prlights[i].faderadius = lValue; break;
        case LIGHT_RADIUS: prlights[i].radius = lValue; break;
        case LIGHT_SECTOR: prlights[i].sector = lValue; break;
        case LIGHT_R: prlights[i].color[0] = lValue; break;
        case LIGHT_G: prlights[i].color[1] = lValue; break;
        case LIGHT_B: prlights[i].color[2] = lValue; break;
        case LIGHT_PRIORITY: prlights[i].priority = lValue; break;
        case LIGHT_TILENUM: prlights[i].tilenum = lValue; break;
        case LIGHT_MINSHADE: prlights[i].minshade = lValue; break;
        case LIGHT_MAXSHADE: prlights[i].maxshade = lValue; break;
#endif
        default:
            return -1;
        }

        return 0;
    }
    else
    {
        int32_t lValue;

        switch (labelNum)
        {
        case SPRITE_X: lValue=datspr->x; break;
        case SPRITE_Y: lValue=datspr->y; break;
        case SPRITE_Z: lValue=datspr->z; break;
        case SPRITE_CSTAT: lValue=datspr->cstat; break;
        case SPRITE_PICNUM: lValue=datspr->picnum; break;
        case SPRITE_SHADE: lValue=datspr->shade; break;
        case SPRITE_PAL: lValue=datspr->pal; break;
        case SPRITE_CLIPDIST: lValue=datspr->clipdist; break;
        case SPRITE_BLEND: lValue=datspr->blend; break;
        case SPRITE_XREPEAT: lValue=datspr->xrepeat; break;
        case SPRITE_YREPEAT: lValue=datspr->yrepeat; break;
        case SPRITE_XOFFSET: lValue=datspr->xoffset; break;
        case SPRITE_YOFFSET: lValue=datspr->yoffset; break;
        case SPRITE_SECTNUM: lValue=datspr->sectnum; break;
        case SPRITE_STATNUM: lValue=datspr->statnum; break;
        case SPRITE_ANG: lValue=datspr->ang; break;
        case SPRITE_OWNER: lValue=datspr->owner; break;
        case SPRITE_XVEL: lValue=datspr->xvel; break;
        case SPRITE_YVEL: lValue=datspr->yvel; break;
        case SPRITE_ZVEL: lValue=datspr->zvel; break;
        case SPRITE_LOTAG: lValue=datspr->lotag; break;
        case SPRITE_HITAG: lValue=datspr->hitag; break;
        case SPRITE_EXTRA: lValue=datspr->extra; break;
#ifdef POLYMER
        // lights
        case LIGHT_X: lValue = prlights[i].x; break;
        case LIGHT_Y: lValue = prlights[i].y; break;
        case LIGHT_Z: lValue = prlights[i].z; break;
        case LIGHT_HORIZ: lValue = prlights[i].horiz; break;
        case LIGHT_RANGE: lValue = prlights[i].range; break;
        case LIGHT_ANGLE: lValue = prlights[i].angle; break;
        case LIGHT_FADERADIUS: lValue = prlights[i].faderadius; break;
        case LIGHT_RADIUS: lValue = prlights[i].radius; break;
        case LIGHT_SECTOR: lValue = prlights[i].sector; break;
        case LIGHT_R: lValue = prlights[i].color[0]; break;
        case LIGHT_G: lValue = prlights[i].color[1]; break;
        case LIGHT_B: lValue = prlights[i].color[2]; break;
        case LIGHT_PRIORITY: lValue = prlights[i].priority; break;
        case LIGHT_TILENUM: lValue = prlights[i].tilenum; break;
        case LIGHT_MINSHADE: lValue = prlights[i].minshade; break;
        case LIGHT_MAXSHADE: lValue = prlights[i].maxshade; break;
        case LIGHT_ACTIVE: lValue = !!prlights[i].flags.active; break;
#endif
        default:
            return -1;
        }

        if (how&ACCESS_USEVARS)
            Gv_SetVar(lVar2, lValue);

        return lValue;
    }

badsprite:
    M32_ERROR("invalid target sprite (%d)", i);
    return -1;
readonly:
    M32_ERROR("structure member `%s' is read-only.", dalabel->name);
    return -1;
}

#undef ACCESS_SET
#undef ACCESS_USEVARS
