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
	auto u = actor->hasU()? actor->u() : nullptr;
    auto sp = &actor->s();
    int8_t* wall_shade;
    short base_shade;

    if (TEST_BOOL8(sp))
        intensity = -intensity;

    if (!TEST_BOOL2(sp))
    {
        if (!TEST_BOOL6(sp))
            sp->sector()->floorpal = sp->pal;
        sp->sector()->floorshade = LIGHT_FloorShade(actor) + intensity;     // floor change
    }

    if (!TEST_BOOL3(sp))
    {
        if (!TEST_BOOL6(sp))
            sp->sector()->ceilingpal = sp->pal;
        sp->sector()->ceilingshade = LIGHT_CeilingShade(actor) + intensity;   // ceiling change
    }

    // change wall
    if (!TEST_BOOL4(sp))
    {
        ASSERT(u && u->WallShade.Data());
        wall_shade = u->WallShade.Data();
        int wallcount = 0;

        for(auto &wal : wallsofsector(sp->sector()))
        {
            base_shade = wall_shade[wallcount];
            wal.shade = base_shade + intensity;
            if (!TEST_BOOL6(sp))
                wal.pal = sp->pal;
            wallcount++;

            if (TEST(sp->extra, SPRX_BOOL5))
            {
                if (wal.twoSided())
                {
                    auto nextWall = wal.nextWall();
                    base_shade = wall_shade[wallcount];
                    nextWall->shade = base_shade + intensity;
                    if (!TEST_BOOL6(sp))
                        nextWall->pal = sp->pal;
                    wallcount++;
                }
            }
        }
    }
}


void DiffuseLighting(DSWActor* actor)
{
    auto sp = &actor->s();
    int i;
    short count;
    short shade;
    SPRITEp dsp;

    // diffused lighting
    count = 0;
    SWStatIterator it(STAT_LIGHTING_DIFFUSE);
    while (auto itActor = it.Next())
    {
        // make sure matchs match
        if (LIGHT_Match(itActor) != LIGHT_Match(actor))
            continue;

        shade = sp->shade + ((LIGHT_DiffuseNum(itActor) + 1) * LIGHT_DiffuseMult(itActor));

        if (shade > LIGHT_MaxDark(actor))
            shade = LIGHT_MaxDark(actor);

        if (!TEST_BOOL6(itActor))
            itActor->spr.pal = sp->pal;

        SectorLightShade(itActor, shade);

        count++;
    }
}

void DoLightingMatch(short match, short state)
{
    SWStatIterator it(STAT_LIGHTING);
    while (auto itActor = it.Next())
    {
        auto u = itActor->u();

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

            if (state == ON)
            {
                SET_BOOL1(itActor);
                itActor->spr.shade = -LIGHT_MaxBright(itActor);
                itActor->spr.pal = u->spal; // on
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

            if (state == ON)
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

            if (state == ON)
            {
                if (LIGHT_Dir(itActor) == 1)
                {
                    LIGHT_DirChange(itActor);
                }
            }
            else if (state == OFF)
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

            if (state == ON)
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
    SPRITEp sp;

    SWStatIterator it(STAT_LIGHTING);
    while (auto itActor = it.Next())
    {
        auto u = itActor->u();
        sp = &itActor->s();

        // on/off test
        if (TEST_BOOL1(sp) == OFF)
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
                    sp->shade = -LIGHT_MaxBright(itActor) + RandomRange(LIGHT_MaxBright(itActor) + LIGHT_MaxDark(itActor));
                    SectorLightShade(itActor, sp->shade);
                    DiffuseLighting(itActor);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    auto spal = sp->pal;
                    sp->pal = 0;
                    sp->shade = LIGHT_MaxDark(itActor);
                    SectorLightShade(itActor, sp->shade);
                    DiffuseLighting(itActor);
                    sp->pal = spal;
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
                    sp->shade += LIGHT_ShadeInc(itActor);
                    if (sp->shade >= LIGHT_MaxDark(itActor))
                        LIGHT_DirChange(itActor);
                }
                else
                {
                    sp->shade -= LIGHT_ShadeInc(itActor);
                    if (sp->shade <= -LIGHT_MaxBright(itActor))
                        LIGHT_DirChange(itActor);
                }

                SectorLightShade(itActor, sp->shade);
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
                    sp->shade += LIGHT_ShadeInc(itActor);
                    if (sp->shade >= LIGHT_MaxDark(itActor))
                    {
                        sp->pal = 0; // off
                        LIGHT_DirChange(itActor);
                        // stop it until switch is hit
                        RESET_BOOL1(itActor);
                    }
                }
                else
                {
                    sp->shade -= LIGHT_ShadeInc(itActor);
                    sp->pal = u->spal; // on
                    if (sp->shade <= -LIGHT_MaxBright(itActor))
                    {
                        LIGHT_DirChange(itActor);
                        // stop it until switch is hit
                        RESET_BOOL1(itActor);
                    }
                }

                SectorLightShade(itActor, sp->shade);
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
                    sp->shade = -LIGHT_MaxBright(itActor) + RandomRange(LIGHT_MaxBright(itActor) + LIGHT_MaxDark(itActor));
                    SectorLightShade(itActor, sp->shade);
                    DiffuseLighting(itActor);
                }
                else
                {
                    // turn off lighting - even colored lighting
                    auto spal = sp->pal;
                    sp->pal = 0;
                    sp->shade = LIGHT_MaxDark(itActor);
                    SectorLightShade(itActor, sp->shade);
                    DiffuseLighting(itActor);
                    sp->pal = spal;
                }

                if ((RANDOM_P2(128 << 8) >> 8) < 8)
                {
                    // set to full brightness
                    sp->shade = -LIGHT_MaxBright(itActor);
                    SectorLightShade(itActor, sp->shade);
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
