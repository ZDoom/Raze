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

/********************************************************************/

typedef
    struct List
{
    struct List *Next;
    struct List *Prev;
} LISTHEAD, *LIST;

#define FIRST(list)        (list->Next)
#define LAST(list)         (list->Prev)


#define INITLIST(list)          ( ((LIST) list)->Prev = ((LIST) list)->Next = (LIST) list)


#define INSERT(list, nodep)  ( ((LIST) nodep)->Prev = (LIST) list,         \
                               ((LIST) nodep)->Next = ((LIST) list)->Next, \
                               ((LIST)  list)->Next = (LIST) nodep,        \
                               ((LIST) nodep)->Next->Prev = (LIST) nodep)

#define INSERT_TAIL(list, nodep)  ( ((LIST) nodep)->Next = (LIST) list,  \
                                    ((LIST) nodep)->Prev = ((LIST) list)->Prev, \
                                    ((LIST)  list)->Prev = (LIST) nodep,        \
                                    ((LIST) nodep)->Prev->Next = (LIST) nodep)

#define REMOVE(nodep)        ( ((LIST) nodep)->Prev->Next = ((LIST) nodep)->Next, \
                               ((LIST) nodep)->Next->Prev = ((LIST) nodep)->Prev)


#define TRAVERSE(l, o, n)    ASSERT(((LIST)l)->Next && ((LIST)l)->Prev); for (o = (decltype(o))(((LIST)l)->Next);      \
                                                                              n = o->Next, (LIST) o != (LIST) l; \
                                                                              o = n)

#define EMPTY(list)          (((LIST) list)->Next == (LIST) list)

#endif


