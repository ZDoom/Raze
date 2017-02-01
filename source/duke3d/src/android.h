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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef android_h_
#define android_h_

#ifdef __ANDROID__
#include "compat.h"
#include "control.h"
#include "in_android.h"

//extern int   android_sample_rate;

//extern int   android_audio_buffer_size;


extern void  CONTROL_Android_ClearButton(int32_t whichbutton);
extern void  CONTROL_Android_PollDevices(ControlInfo *info);
extern void  CONTROL_Android_SetLastWeapon(int w);

extern void  CONTROL_Android_ScrollMap(int32_t *angle,int32_t *x, int32_t *y, uint16_t *zoom );

#endif

#endif
