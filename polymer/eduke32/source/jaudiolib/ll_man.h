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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
/**********************************************************************
   module: LL_MAN.H

   author: James R. Dose
   date:   February 4, 1994

   Public header for LL_MAN.C.  Linked list management routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __LL_MAN_H
#define __LL_MAN_H

#include "compat.h"

enum LL_Errors
   {
   LL_Warning = -2,
   LL_Error   = -1,
   LL_Ok      = 0
   };

typedef struct list
   {
   void *start;
   void *end;
   } list;

void LL_AddNode( char *node, char **head, char **tail, int32_t next, int32_t prev );
void LL_RemoveNode( char *node, char **head, char **tail, int32_t next, int32_t prev );

#define LL_AddToHead( type, listhead, node )         \
    LL_AddNode( ( char * )( node ),                  \
                ( char ** )&( ( listhead )->start ), \
                ( char ** )&( ( listhead )->end ),   \
                ( int32_t )&( ( type * ) 0 )->next,      \
                ( int32_t )&( ( type * ) 0 )->prev )

#define LL_AddToTail( type, listhead, node )         \
    LL_AddNode( ( char * )( node ),                  \
                ( char ** )&( ( listhead )->end ),   \
                ( char ** )&( ( listhead )->start ), \
                ( int32_t )&( ( type * ) 0 )->prev,      \
                ( int32_t )&( ( type * ) 0 )->next )

#define LL_Remove( type, listhead, node )               \
    LL_RemoveNode( ( char * )( node ),                  \
                   ( char ** )&( ( listhead )->start ), \
                   ( char ** )&( ( listhead )->end ),   \
                   ( int32_t )&( ( type * ) 0 )->next,      \
                   ( int32_t )&( ( type * ) 0 )->prev )

#define LL_NextNode( node )     ( ( node )->next )
#define LL_PreviousNode( node ) ( ( node )->prev )

#endif
