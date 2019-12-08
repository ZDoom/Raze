#pragma once
#include <functional>

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
	int16_t 	data1;		// keys / mouse/joystick buttons
	int16_t		data2;
	int16_t		data3;
	int 		x;			// mouse/joystick x move
	int 		y;			// mouse/joystick y move
};



// Called by IO functions when input is detected.
void D_PostEvent (const event_t* ev);
void D_RemoveNextCharEvent();
void D_ProcessEvents(void);

enum
{
	NUM_EVENTS = 128
};

extern	event_t 		events[NUM_EVENTS];

