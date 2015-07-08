/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
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
#ifndef linklist_h_
#define linklist_h_
#ifdef __cplusplus
extern "C" {
#endif


#define NewNode(type) ((type *)Bmalloc(sizeof(type)))


#define LL_New(rootnode, type, next, prev)                                                                             \
    {                                                                                                                  \
        (rootnode) = NewNode(type);                                                                                    \
        (rootnode)->prev = (rootnode);                                                                                 \
        (rootnode)->next = (rootnode);                                                                                 \
    }


#define LL_Add(rootnode, newnode, next, prev)                                                                          \
    {                                                                                                                  \
        (newnode)->next = (rootnode);                                                                                  \
        (newnode)->prev = (rootnode)->prev;                                                                            \
        (rootnode)->prev->next = (newnode);                                                                            \
        (rootnode)->prev = (newnode);                                                                                  \
    }

#define LL_TransferList(oldroot, newroot, next, prev)                                                                  \
    {                                                                                                                  \
        if ((oldroot)->prev != (oldroot))                                                                              \
        {                                                                                                              \
            (oldroot)->prev->next = (newroot);                                                                         \
            (oldroot)->next->prev = (newroot)->prev;                                                                   \
            (newroot)->prev->next = (oldroot)->next;                                                                   \
            (newroot)->prev = (oldroot)->prev;                                                                         \
            (oldroot)->next = (oldroot);                                                                               \
            (oldroot)->prev = (oldroot);                                                                               \
        }                                                                                                              \
    }

#define LL_ReverseList(root, type, next, prev)                                                                         \
    {                                                                                                                  \
        type *newend = (root)->next, *trav, *tprev;                                                                    \
        for (trav = (root)->prev; trav != newend; trav = tprev)                                                        \
        {                                                                                                              \
            tprev = trav->prev;                                                                                        \
            LL_Move(trav, newend, next, prev);                                                                         \
        }                                                                                                              \
    }


#define LL_Remove(node, next, prev)                                                                                    \
    {                                                                                                                  \
        (node)->prev->next = (node)->next;                                                                             \
        (node)->next->prev = (node)->prev;                                                                             \
        (node)->next = (node);                                                                                         \
        (node)->prev = (node);                                                                                         \
    }


#define LL_SortedInsertion(rootnode, insertnode, next, prev, type, sortparm)                                           \
    {                                                                                                                  \
        type *hoya = (rootnode)->next;                                                                                 \
        while ((hoya != (rootnode)) && ((insertnode)->sortparm > hoya->sortparm))                                      \
        {                                                                                                              \
            hoya = hoya->next;                                                                                         \
        }                                                                                                              \
        LL_Add(hoya, (insertnode), next, prev);                                                                        \
    }

#define LL_Move(node, newroot, next, prev)                                                                             \
    {                                                                                                                  \
        LL_Remove((node), next, prev);                                                                                 \
        LL_Add((newroot), (node), next, prev);                                                                         \
    }

#define LL_Empty(list, next, prev) (((list)->next == (list)) && ((list)->prev == (list)))

#define LL_Free(list) Bfree(list)
#define LL_Reset(list, next, prev) (list)->next = (list)->prev = (list)


#ifdef __cplusplus
}
#endif
#endif
