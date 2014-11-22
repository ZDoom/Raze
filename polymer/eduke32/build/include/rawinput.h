#ifndef rawinput_h_
#define rawinput_h_

#include "compat.h"

void RI_PollDevices(BOOL loop);
void RI_ProcessMessage(MSG *msg);
BOOL RI_CaptureInput(BOOL grab, HWND target);

#ifndef VK_LBUTTON
#define VK_LBUTTON        0x01
#endif
#ifndef VK_RBUTTON
#define VK_RBUTTON        0x02
#endif
#ifndef VK_CANCEL
#define VK_CANCEL         0x03
#endif
#ifndef VK_MBUTTON
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */
#endif

#endif // rawinput_h_

