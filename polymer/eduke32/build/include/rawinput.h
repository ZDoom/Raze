#ifndef rawinput_h__
#define rawinput_h__

#include "compat.h"

void RI_PollDevices();

uint8_t RI_MouseState( uint8_t Button );

int8_t RI_WheelState();

int32_t RI_CaptureInput(int32_t grab, HWND target);

#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

// mouse states for RI_MouseState

#define BUTTON_PRESSED    0x01
#define BUTTON_RELEASED   0x02
#define BUTTON_HELD       0x03

#endif // rawinput_h__

