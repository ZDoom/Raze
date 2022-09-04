//-------------------------------------------------------------------------
/*
 Copyright (C) 2005 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFShadowWarrior

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
 */
//-------------------------------------------------------------------------

#include "ns.h"

#include "tarray.h"
#include "debugbreak.h"
BEGIN_SW_NS

#include "saveable.h"

static TArray<saveable_module*> saveablemodules;

void Saveable_Init(void)
{
    if (saveablemodules.Size() > 0) return;

#define MODULE(x) { \
        extern saveable_module saveable_ ## x; \
        saveablemodules.Push(&saveable_ ## x); \
}

    MODULE(actor)
    MODULE(ai)
    MODULE(ai) // was 'build' but that is not used anywhere anymore.
    MODULE(bunny)
    MODULE(coolg)
    MODULE(coolie)
    MODULE(eel)
    MODULE(girlninj)
    MODULE(goro)
    MODULE(hornet)
    MODULE(jweapon)
    MODULE(lava)
    MODULE(miscactr)
    MODULE(morph)
    MODULE(ninja)
    MODULE(panel)
    MODULE(player)
    MODULE(ripper)
    MODULE(ripper2)
    MODULE(rotator)
    MODULE(serp)
    MODULE(skel)
    MODULE(skull)
    MODULE(slidor)
    MODULE(spike)
    MODULE(sprite)
    MODULE(sumo)
    MODULE(track)
    MODULE(vator)
    MODULE(wallmove)
    MODULE(weapon)
    MODULE(zilla)
    MODULE(zombie)

    MODULE(sector)
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int Saveable_FindCodeSym(void *ptr, savedcodesym *sym)
{
    unsigned m,i;

    if (!ptr)
    {
        sym->name = "";
        return 0;
    }

    for (m=0; m<saveablemodules.Size(); m++)
    {
        for (i=0; i<saveablemodules[m]->numcode; i++)
        {
            if (ptr != saveablemodules[m]->code[i].base) continue;

            sym->name = saveablemodules[m]->code[i].name;
            return 0;
        }
    }

    debug_break();
    return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int Saveable_FindDataSym(void *ptr, saveddatasym *sym)
{
    unsigned m,i;

    if (!ptr)
    {
        sym->name = "";
        sym->offset = 0;
        return 0;
    }

    for (m = 0; m < saveablemodules.Size(); m++)
    {
        for (i=0; i<saveablemodules[m]->numdata; i++)
        {
            if (ptr < saveablemodules[m]->data[i].base) continue;
            if (ptr >= (void *)((intptr_t)saveablemodules[m]->data[i].base +
                                saveablemodules[m]->data[i].size)) continue;

            sym->name = saveablemodules[m]->data[i].name;
            sym->offset = unsigned((intptr_t)ptr - (intptr_t)saveablemodules[m]->data[i].base);

            return 0;
        }
    }

    debug_break();
    return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int Saveable_RestoreCodeSym(savedcodesym *sym, void **ptr)
{
    if (sym->name.IsEmpty())
    {
        *ptr = nullptr;
        return 0;
    }

    Saveable_Init();
    for (auto module : saveablemodules)
    {
        for (unsigned i = 0; i < module->numcode; i++)
        {
            if (sym->name.Compare(module->code[i].name) == 0)
            {
                *ptr = module->code[i].base;
                return 0;
            }
        }
    }
    // No way to recover. I_FatalError is the only chance to get an error message shown. :(
    I_FatalError("Unknown code reference '%s' in savegame\n", sym->name.GetChars());
    return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int Saveable_RestoreDataSym(saveddatasym *sym, void **ptr)
{
    if (sym->name.IsEmpty())
    {
        *ptr = nullptr;
        return 0;
    }

    Saveable_Init();
    for (auto module : saveablemodules)
    {
        for (unsigned i = 0; i < module->numdata; i++)
        {
            if (sym->name.Compare(module->data[i].name) == 0)
            {
                *ptr = ((uint8_t*)module->data[i].base) + sym->offset;
                return 0;
            }
        }
    }
    // No way to recover. I_FatalError is the only chance to get an error message shown. :(
    I_FatalError("Unknown data reference '%s' in savegame\n", sym->name.GetChars());
    return -1;
}

END_SW_NS
