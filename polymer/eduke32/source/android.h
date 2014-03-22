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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __android_h__
#define __android_h__

#ifdef __ANDROID__
#include "compat.h"
#include "control.h"

extern int   android_screen_width;
extern int   android_screen_height;

extern void  CONTROL_Android_ClearButton(int32_t whichbutton);
extern void  CONTROL_Android_PollDevices(ControlInfo *info);
extern void  setLastWeapon(int w);
#endif

#endif
