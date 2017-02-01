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

#include "names2.h"
#include "game.h"
#include "light.h"

/* #define LIGHT_Match(sp)         (SP_TAG2((sp)))                      */
/* #define LIGHT_Type(sp)          (SP_TAG3((sp)))                      */
/* #define LIGHT_MaxTics(sp)       (SP_TAG4((sp)))                      */
/* #define LIGHT_MaxBright(sp)     (SP_TAG5((sp)))                      */
/* #define LIGHT_MaxDark(sp)       (SP_TAG6((sp)))                      */
/* #define LIGHT_ShadeInc(sp)      (SP_TAG7((sp)))                      */
/*                                                                      */
/* #define LIGHT_Dir(sp)           (!!(TEST((sp)->extra, SPRX_BOOL10))) */
/* #define LIGHT_DirChange(sp)     (FLIP((sp)->extra, SPRX_BOOL10))     */
/*                                                                      */
/* #define LIGHT_Shade(sp)         ((sp)->shade)                        */
/* #define LIGHT_FloorShade(sp)    ((sp)->xoffset)                      */
/* #define LIGHT_CeilingShade(sp)  ((sp)->yoffset)                      */
/* #define LIGHT_Tics(sp)          ((sp)->z)                            */
/*                                                                      */
/* #define LIGHT_DiffuseNum(sp) (SP_TAG3((sp)))                         */
/* #define LIGHT_DiffuseMult(sp) (SP_TAG4((sp)))                        */

void SectorLightShade(SPRITEp sp, short intensity)
{
    short w, startwall, endwall;
    void *void_ptr;
    int8_t* wall_shade;
    short base_shade;
    short wallcount;

    if (TEST_BOOL8(sp))
        intensity = -intensity;

    if (!TEST_BOOL2(sp))
    {
        if (!TEST_BOOL6(sp))
            sector[sp->sectnum].floorpal = sp->pal;
        sector[sp->sectnum].floorshade = LIGHT_FloorShade(sp) + intensity;     // floor change
    }

    if (!TEST_BOOL3(sp))
    {
        if (!TEST_BOOL6(sp))
            sector[sp->sectnum].ceilingpal = sp->pal;
        sector[sp->sectnum].ceilingshade = LIGHT_CeilingShade(sp) + intensity;   // ceiling change
    }

    // change wall
    if (!TEST_BOOL4(sp))
    {
        ASSERT(User[sp - sprite] && User[sp - sprite]->WallShade);
        wall_shade = User[sp - sprite]->WallShade;

        startwall = sector[sp->sectnum].wallptr;
        endwall = startwall + sector[sp->sectnum].wallnum - 1;

        for (w = startwall, wallcount = 0; w <= endwall; w++)
        {
            base_shade = wall_shade[wallcount];
            wall[w].shade = base_shade + intensity;
            if (!TEST_BOOL6(sp))
                wall[w].pal = sp->pal;
            wallcount++;

            if (TEST(sp->extra, SPRX_BOOL5) && wall[w].nextwall >= 0)
            {
                base_shade = wall_shade[wallcount];
                wall[wall[w].nextwall].shade = base_shade + intensity;
                if (!TEST_BOOL6(sp))
                    wall[wall[w].nextwall].pal = sp->pal;
                wallcount++;
            }
        }
    }
}


void DiffuseLighting(SPRITEp sp)
{
    short i, nexti;
    short count;
    short shade;
    SPRITEp dsp;

    // diffused lighting
    count = 0;
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_LIGHTING_DIFFUSE],i,nexti)
    {
        dsp = &sprite[i];

        // make sure matchs match
        if (LIGHT_Match(dsp) != LIGHT_Match(sp))
            continue;

        shade = sp->shade + ((LIGHT_DiffuseNum(dsp) + 1) * LIGHT_DiffuseMult(dsp));

        if (shade > LIGHT_MaxDark(sp))
            shade = LIGHT_MaxDark(sp);

        if (!TEST_BOOL6(dsp))
            dsp->pal = sp->pal;

        SectorLightShade(dsp, shade);

        count++;
    }
}

void DoLightingMatch(short match, short state)
{
    short i,nexti;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_LIGHTING],i,nexti)
    {
        sp = &sprite[i];

        if (LIGHT_Match(sp) != match)
            continue;

        switch (LIGHT_Type(sp))
        {
        case LIGHT_CONSTANT:

            // initialized
            SET_BOOL9(sp);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(sp);

            if (state == ON)
            {
                SET_BOOL1(sp);
                sp->shade = -LIGHT_MaxBright(sp);
                sp->pal = User[sp-sprite]->spal; // on
                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
            }
            else
            {
                RESET_BOOL1(sp);
                sp->shade = LIGHT_MaxDark(sp);
                sp->pal = 0; // off
                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
            }
            break;

        case LIGHT_FLICKER:
        case LIGHT_FADE:
            // initialized
            SET_BOOL9(sp);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(sp);

            if (state == ON)
            {
                // allow fade or flicker
                SET_BOOL1(sp);
            }
            else
            {
                RESET_BOOL1(sp);
                sp->shade = LIGHT_MaxDark(sp);
                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
            }
            break;

        case LIGHT_FADE_TO_ON_OFF:

            // initialized
            SET_BOOL9(sp);

            // toggle
            //if (state == -1)
            //    state = !TEST_BOOL1(sp);

            if (state == ON)
            {
                if (LIGHT_Dir(sp) == 1)
                {
                    LIGHT_DirChange(sp);
                }
            }
            else if (state == OFF)
            {
                if (LIGHT_Dir(sp) == 0)
                {
                    LIGHT_DirChange(sp);
                }
            }

            // allow fade or flicker
            SET_BOOL1(sp);
            break;

        case LIGHT_FLICKER_ON:

            // initialized
            SET_BOOL9(sp);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(sp);

            if (state == ON)
            {
                // allow fade or flicker
                SET_BOOL1(sp);
            }
            else
            {
                // turn it off till next switch
                short spal = sp->pal;
                RESET_BOOL1(sp);
                sp->pal = 0;
                sp->shade = LIGHT_MaxDark(sp);
                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
                sp->pal = spal;
            }
            break;
        }
    }
}

void InitLighting(void)
{
    short i,nexti;
    SPRITEp sp;


    // processed on level startup
    // puts lights in correct state
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_LIGHTING],i,nexti)
    {
        sp = &sprite[i];

        if (!TEST_BOOL9(sp))
        {
            DoLightingMatch(LIGHT_Match(sp), !!TEST_BOOL1(sp));
        }
    }
}

void DoLighting(void)
{
    short i,nexti;
    SPRITEp sp;
    short count;


    TRAVERSE_SPRITE_STAT(headspritestat[STAT_LIGHTING],i,nexti)
    {
        sp = &sprite[i];

        // on/off test
        if (TEST_BOOL1(sp) == OFF)
            continue;

        switch (LIGHT_Type(sp))
        {
        case LIGHT_CONSTANT:
            break;

        case LIGHT_FLICKER:

            LIGHT_Tics(sp) += synctics;
            while (LIGHT_Tics(sp) >= LIGHT_MaxTics(sp))
            {
                LIGHT_Tics(sp) -= LIGHT_MaxTics(sp);

                if ((RANDOM_P2(128 << 8) >> 8) > 64)
                {
                    sp->shade = -LIGHT_MaxBright(sp) + RANDOM_RANGE(LIGHT_MaxBright(sp) + LIGHT_MaxDark(sp));
                    SectorLightShade(sp, sp->shade);
                    DiffuseLighting(sp);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    short spal = sp->pal;
                    sp->pal = 0;
                    sp->shade = LIGHT_MaxDark(sp);
                    SectorLightShade(sp, sp->shade);
                    DiffuseLighting(sp);
                    sp->pal = spal;
                }
            }

            break;

        case LIGHT_FADE:

            LIGHT_Tics(sp) += synctics;

            while (LIGHT_Tics(sp) >= LIGHT_MaxTics(sp))
            {
                LIGHT_Tics(sp) -= LIGHT_MaxTics(sp);

                if (LIGHT_Dir(sp) == 1)
                {
                    sp->shade += LIGHT_ShadeInc(sp);
                    if (sp->shade >= LIGHT_MaxDark(sp))
                        LIGHT_DirChange(sp);
                }
                else
                {
                    sp->shade -= LIGHT_ShadeInc(sp);
                    if (sp->shade <= -LIGHT_MaxBright(sp))
                        LIGHT_DirChange(sp);
                }

                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
            }

            break;

        case LIGHT_FADE_TO_ON_OFF:

            LIGHT_Tics(sp) += synctics;

            while (LIGHT_Tics(sp) >= LIGHT_MaxTics(sp))
            {
                LIGHT_Tics(sp) -= LIGHT_MaxTics(sp);

                if (LIGHT_Dir(sp) == 1)
                {
                    sp->shade += LIGHT_ShadeInc(sp);
                    if (sp->shade >= LIGHT_MaxDark(sp))
                    {
                        sp->pal = 0; // off
                        LIGHT_DirChange(sp);
                        // stop it until switch is hit
                        RESET_BOOL1(sp);
                    }
                }
                else
                {
                    sp->shade -= LIGHT_ShadeInc(sp);
                    sp->pal = User[sp-sprite]->spal; // on
                    if (sp->shade <= -LIGHT_MaxBright(sp))
                    {
                        LIGHT_DirChange(sp);
                        // stop it until switch is hit
                        RESET_BOOL1(sp);
                    }
                }

                SectorLightShade(sp, sp->shade);
                DiffuseLighting(sp);
            }

            break;

        case LIGHT_FLICKER_ON:

            LIGHT_Tics(sp) += synctics;

            while (LIGHT_Tics(sp) >= LIGHT_MaxTics(sp))
            {
                LIGHT_Tics(sp) -= LIGHT_MaxTics(sp);

                if ((RANDOM_P2(128 << 8) >> 8) > 64)
                {
                    sp->shade = -LIGHT_MaxBright(sp) + RANDOM_RANGE(LIGHT_MaxBright(sp) + LIGHT_MaxDark(sp));
                    SectorLightShade(sp, sp->shade);
                    DiffuseLighting(sp);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    short spal = sp->pal;
                    sp->pal = 0;
                    sp->shade = LIGHT_MaxDark(sp);
                    SectorLightShade(sp, sp->shade);
                    DiffuseLighting(sp);
                    sp->pal = spal;
                }

                if ((RANDOM_P2(128 << 8) >> 8) < 8)
                {
                    // set to full brightness
                    sp->shade = -LIGHT_MaxBright(sp);
                    SectorLightShade(sp, sp->shade);
                    DiffuseLighting(sp);
                    // turn it off until a swith happens
                    RESET_BOOL1(sp);
                }
            }
            break;
        }
    }
}
