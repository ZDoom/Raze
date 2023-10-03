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

#ifndef LISTS_H

#define LISTS_H

BEGIN_SW_NS

/********************************************************************/

inline void INITLIST(DPanelSprite* list)
{
    list->Prev = list->Next = list;
}

inline void INSERT(DPanelSprite* list, DPanelSprite* nodep)
{
    GC::WriteBarrier(list);
    GC::WriteBarrier(list->Next);
    GC::WriteBarrier(nodep);

    nodep->Prev = list;
    nodep->Next = list->Next;
    list->Next = nodep;
    nodep->Next->Prev = nodep;
}

inline void REMOVE(DPanelSprite* nodep)
{
    GC::WriteBarrier(nodep->Next);
    GC::WriteBarrier(nodep->Prev);
    nodep->Prev->Next = nodep->Next;
    nodep->Next->Prev = nodep->Prev;
}

inline bool EMPTY(DPanelSprite* list)
{
    return list->Next == list;
}

END_SW_NS

#endif


