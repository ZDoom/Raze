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

#include "compat.h"
#include "saveable.h"

#define maxModules 35

static saveable_module *saveablemodules[maxModules];
static unsigned nummodules = 0;

void Saveable_Init(void)
{
    if (nummodules > 0) return;

#define MODULE(x) { \
        extern saveable_module saveable_ ## x; \
        saveablemodules[nummodules++] = &saveable_ ## x; \
}

    MODULE(actor)
    MODULE(ai)
    MODULE(build)
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
    MODULE(quake)
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

int Saveable_FindCodeSym(void *ptr, savedcodesym *sym)
{
    unsigned m,i;

    if (!ptr)
    {
        sym->module = 0;    // module 0 is the "null module" for null pointers
        sym->index  = 0;
        return 0;
    }

    for (m=0; m<nummodules; m++)
    {
        for (i=0; i<saveablemodules[m]->numcode; i++)
        {
            if (ptr != saveablemodules[m]->code[i]) continue;

            sym->module = 1+m;
            sym->index  = i;

            return 0;
        }
    }

    return -1;
}

int Saveable_FindDataSym(void *ptr, saveddatasym *sym)
{
    unsigned m,i;

    if (!ptr)
    {
        sym->module = 0;
        sym->index  = 0;
        sym->offset = 0;
        return 0;
    }

    for (m=0; m<nummodules; m++)
    {
        for (i=0; i<saveablemodules[m]->numdata; i++)
        {
            if (ptr < saveablemodules[m]->data[i].base) continue;
            if (ptr >= (void *)((intptr_t)saveablemodules[m]->data[i].base +
                                saveablemodules[m]->data[i].size)) continue;

            sym->module = 1+m;
            sym->index  = i;
            sym->offset = (intptr_t)ptr - (intptr_t)saveablemodules[m]->data[i].base;

            return 0;
        }
    }
    return -1;
}

int Saveable_RestoreCodeSym(savedcodesym *sym, void **ptr)
{
    if (sym->module == 0)
    {
        *ptr = NULL;
        return 0;
    }

    if (sym->module > nummodules) return -1;
    if (sym->index  >= saveablemodules[sym->module-1]->numcode) return -1;

    *ptr = saveablemodules[sym->module-1]->code[sym->index];

    return 0;
}

int Saveable_RestoreDataSym(saveddatasym *sym, void **ptr)
{
    if (sym->module == 0)
    {
        *ptr = NULL;
        return 0;
    }

    if (sym->module > nummodules) return -1;
    if (sym->index  >= saveablemodules[sym->module-1]->numdata) return -1;
    if (sym->offset >= saveablemodules[sym->module-1]->data[sym->index].size) return -1;

    *ptr = (void *)((intptr_t)saveablemodules[sym->module-1]->data[sym->index].base + sym->offset);

    return 0;
}
