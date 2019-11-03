//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------


#ifndef __D_EVENT_H__
#define __D_EVENT_H__


#include <functional>


//
// Event handling.
//

// Input event types.
enum EGenericEvent
{
	EV_None,
	EV_KeyDown,		// data1: scan code, data2: Qwerty ASCII code
	EV_KeyUp,		// same
	EV_Mouse,		// x, y: mouse movement deltas
	EV_GUI_Event,	// subtype specifies actual event
	EV_DeviceChange,// a device has been connected or removed
};

// Event structure.
struct event_t
{
	uint8_t		type;
	uint8_t		subtype;
	int16_t 		data1;		// keys / mouse/joystick buttons
	int16_t		data2;
	int16_t		data3;
	int 		x;			// mouse/joystick x move
	int 		y;			// mouse/joystick y move
};



// Called by IO functions when input is detected.
void D_PostEvent (const event_t* ev);
void D_RemoveNextCharEvent();
void D_ProcessEvents(void);


//
// GLOBAL VARIABLES
//
#define MAXEVENTS		128

extern	event_t 		events[MAXEVENTS];


#endif
