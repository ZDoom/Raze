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
#include "game.h"
#include "light.h"

BEGIN_SW_NS


void SectorLightShade(DSWActor* actor, short intensity)
{
    int8_t* wall_shade;
    short base_shade;

    if (TEST_BOOL8(actor))
        intensity = -intensity;

    if (!TEST_BOOL2(actor))
    {
        if (!TEST_BOOL6(actor))
            actor->sector()->floorpal = actor->spr.pal;
        actor->sector()->floorshade = LIGHT_FloorShade(actor) + intensity;     // floor change
    }

    if (!TEST_BOOL3(actor))
    {
        if (!TEST_BOOL6(actor))
            actor->sector()->ceilingpal = actor->spr.pal;
        actor->sector()->ceilingshade = LIGHT_CeilingShade(actor) + intensity;   // ceiling change
    }

    // change wall
    if (!TEST_BOOL4(actor))
    {
        ASSERT(actor->hasU() && actor->user.WallShade.Data());
        wall_shade = actor->user.WallShade.Data();
        int wallcount = 0;

        for(auto &wal : wallsofsector(actor->sector()))
        {
            base_shade = wall_shade[wallcount];
            wal.shade = base_shade + intensity;
            if (!TEST_BOOL6(actor))
                wal.pal = actor->spr.pal;
            wallcount++;

            if ((actor->spr.extra & SPRX_BOOL5))
            {
                if (wal.twoSided())
                {
                    auto nextWall = wal.nextWall();
                    base_shade = wall_shade[wallcount];
                    nextWall->shade = base_shade + intensity;
                    if (!TEST_BOOL6(actor))
                        nextWall->pal = actor->spr.pal;
                    wallcount++;
                }
            }
        }
    }
}


void DiffuseLighting(DSWActor* actor)
{
    int i;
    short count;
    short shade;

    // diffused lighting
    count = 0;
    SWStatIterator it(STAT_LIGHTING_DIFFUSE);
    while (auto itActor = it.Next())
    {
        // make sure matchs match
        if (LIGHT_Match(itActor) != LIGHT_Match(actor))
            continue;

        shade = actor->spr.shade + ((LIGHT_DiffuseNum(itActor) + 1) * LIGHT_DiffuseMult(itActor));

        if (shade > LIGHT_MaxDark(actor))
            shade = LIGHT_MaxDark(actor);

        if (!TEST_BOOL6(itActor))
            itActor->spr.pal = actor->spr.pal;

        SectorLightShade(itActor, shade);

        count++;
    }
}

void DoLightingMatch(short match, short state)
{
    SWStatIterator it(STAT_LIGHTING);
    while (auto itActor = it.Next())
    {
        if (LIGHT_Match(itActor) != match)
            continue;

        switch (LIGHT_Type(itActor))
        {
        case LIGHT_CONSTANT:

            // initialized
            SET_BOOL9(itActor);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(itActor);

            if (state == 1)
            {
                SET_BOOL1(itActor);
                itActor->spr.shade = -LIGHT_MaxBright(itActor);
                itActor->spr.pal = itActor->user.spal; // on
                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
            }
            else
            {
                RESET_BOOL1(itActor);
                itActor->spr.shade = LIGHT_MaxDark(itActor);
                itActor->spr.pal = 0; // off
                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
            }
            break;

        case LIGHT_FLICKER:
        case LIGHT_FADE:
            // initialized
            SET_BOOL9(itActor);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(itActor);

            if (state == 1)
            {
                // allow fade or flicker
                SET_BOOL1(itActor);
            }
            else
            {
                RESET_BOOL1(itActor);
                itActor->spr.shade = LIGHT_MaxDark(itActor);
                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
            }
            break;

        case LIGHT_FADE_TO_ON_OFF:

            // initialized
            SET_BOOL9(itActor);

            // toggle
            //if (state == -1)
            //    state = !TEST_BOOL1(itActor);

            if (state == 1)
            {
                if (LIGHT_Dir(itActor) == 1)
                {
                    LIGHT_DirChange(itActor);
                }
            }
            else if (state == 0)
            {
                if (LIGHT_Dir(itActor) == 0)
                {
                    LIGHT_DirChange(itActor);
                }
            }

            // allow fade or flicker
            SET_BOOL1(itActor);
            break;

        case LIGHT_FLICKER_ON:

            // initialized
            SET_BOOL9(itActor);

            // toggle
            if (state == -1)
                state = !TEST_BOOL1(itActor);

            if (state == 1)
            {
                // allow fade or flicker
                SET_BOOL1(itActor);
            }
            else
            {
                // turn it off till next switch
                auto spal = itActor->spr.pal;
                RESET_BOOL1(itActor);
                itActor->spr.pal = 0;
                itActor->spr.shade = LIGHT_MaxDark(itActor);
                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
                itActor->spr.pal = spal;
            }
            break;
        }
    }
}

void InitLighting(void)
{
    // processed on level startup
    // puts lights in correct state
    SWStatIterator it(STAT_LIGHTING);
    while (auto actor = it.Next())
    {
        if (!TEST_BOOL9(actor))
        {
            DoLightingMatch(LIGHT_Match(actor), !!TEST_BOOL1(actor));
        }
    }
}

void DoLighting(void)
{
    SWStatIterator it(STAT_LIGHTING);
    while (auto itActor = it.Next())
    {
        // on/off test
        if (TEST_BOOL1(itActor) == 0)
            continue;

        switch (LIGHT_Type(itActor))
        {
        case LIGHT_CONSTANT:
            break;

        case LIGHT_FLICKER:

            LIGHT_Tics(itActor) += synctics;
            while (LIGHT_Tics(itActor) >= LIGHT_MaxTics(itActor))
            {
                LIGHT_Tics(itActor) -= LIGHT_MaxTics(itActor);

                if ((RANDOM_P2(128 << 8) >> 8) > 64)
                {
                    itActor->spr.shade = -LIGHT_MaxBright(itActor) + RandomRange(LIGHT_MaxBright(itActor) + LIGHT_MaxDark(itActor));
                    SectorLightShade(itActor, itActor->spr.shade);
                    DiffuseLighting(itActor);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    auto spal = itActor->spr.pal;
                    itActor->spr.pal = 0;
                    itActor->spr.shade = LIGHT_MaxDark(itActor);
                    SectorLightShade(itActor, itActor->spr.shade);
                    DiffuseLighting(itActor);
                    itActor->spr.pal = spal;
                }
            }

            break;

        case LIGHT_FADE:

            LIGHT_Tics(itActor) += synctics;

            while (LIGHT_Tics(itActor) >= LIGHT_MaxTics(itActor))
            {
                LIGHT_Tics(itActor) -= LIGHT_MaxTics(itActor);

                if (LIGHT_Dir(itActor) == 1)
                {
                    itActor->spr.shade += LIGHT_ShadeInc(itActor);
                    if (itActor->spr.shade >= LIGHT_MaxDark(itActor))
                        LIGHT_DirChange(itActor);
                }
                else
                {
                    itActor->spr.shade -= LIGHT_ShadeInc(itActor);
                    if (itActor->spr.shade <= -LIGHT_MaxBright(itActor))
                        LIGHT_DirChange(itActor);
                }

                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
            }

            break;

        case LIGHT_FADE_TO_ON_OFF:

            LIGHT_Tics(itActor) += synctics;

            while (LIGHT_Tics(itActor) >= LIGHT_MaxTics(itActor))
            {
                LIGHT_Tics(itActor) -= LIGHT_MaxTics(itActor);

                if (LIGHT_Dir(itActor) == 1)
                {
                    itActor->spr.shade += LIGHT_ShadeInc(itActor);
                    if (itActor->spr.shade >= LIGHT_MaxDark(itActor))
                    {
                        itActor->spr.pal = 0; // off
                        LIGHT_DirChange(itActor);
                        // stop it until switch is hit
                        RESET_BOOL1(itActor);
                    }
                }
                else
                {
                    itActor->spr.shade -= LIGHT_ShadeInc(itActor);
                    itActor->spr.pal = itActor->user.spal; // on
                    if (itActor->spr.shade <= -LIGHT_MaxBright(itActor))
                    {
                        LIGHT_DirChange(itActor);
                        // stop it until switch is hit
                        RESET_BOOL1(itActor);
                    }
                }

                SectorLightShade(itActor, itActor->spr.shade);
                DiffuseLighting(itActor);
            }

            break;

        case LIGHT_FLICKER_ON:

            LIGHT_Tics(itActor) += synctics;

            while (LIGHT_Tics(itActor) >= LIGHT_MaxTics(itActor))
            {
                LIGHT_Tics(itActor) -= LIGHT_MaxTics(itActor);

                if ((RANDOM_P2(128 << 8) >> 8) > 64)
                {
                    itActor->spr.shade = -LIGHT_MaxBright(itActor) + RandomRange(LIGHT_MaxBright(itActor) + LIGHT_MaxDark(itActor));
                    SectorLightShade(itActor, itActor->spr.shade);
                    DiffuseLighting(itActor);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    auto spal = itActor->spr.pal;
                    itActor->spr.pal = 0;
                    itActor->spr.shade = LIGHT_MaxDark(itActor);
                    SectorLightShade(itActor, itActor->spr.shade);
                    DiffuseLighting(itActor);
                    itActor->spr.pal = spal;
                }

                if ((RANDOM_P2(128 << 8) >> 8) < 8)
                {
                    // set to full brightness
                    itActor->spr.shade = -LIGHT_MaxBright(itActor);
                    SectorLightShade(itActor, itActor->spr.shade);
                    DiffuseLighting(itActor);
                    // turn it off until a swith happens
                    RESET_BOOL1(itActor);
                }
            }
            break;
        }
    }
}
END_SW_NS
