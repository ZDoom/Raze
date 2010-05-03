#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include "rawinput.h"
#include "winlayer.h"
#include "scancodes.h"
#include "build.h"

static BOOL init_done = 0;
static uint8_t KeyboardState[256] = {0};
static uint8_t MouseState0[5] = {0};
static uint8_t MouseState1[5] = {0};
static int8_t MWheel = 0;

extern volatile uint8_t moustat, mousegrab;
extern uint32_t mousewheel[2];
extern void SetKey(int32_t key, int32_t state);

#define MASK_DOWN (1<<(i<<1))
#define MASK_UP (MASK_DOWN<<1)

#define MouseWheelFakePressTime 50

#ifndef GET_RAWINPUT_CODE_WPARAM
#define GET_RAWINPUT_CODE_WPARAM(wParam)    ((wParam) & 0xff)
#endif

void RI_ProcessMouse(const RAWMOUSE* rmouse)
{
    int32_t i, mask;

    if (!mousegrab)
        return;

    mousex += rmouse->lLastX;
    mousey += rmouse->lLastY;

    if (rmouse->usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        // untested... maybe devices like wacom tablets set this flag?
        POINT pos = { xdim>>1, ydim>>1 };

        ClientToScreen((HWND)win_gethwnd(), &pos);

        mousex -= pos.x;
        mousey -= pos.y;
    }

    for (i = 0, mask = 1; i < 4; i++)
    {
        MouseState1[i] = MouseState0[i];

        if (rmouse->usButtonFlags & mask) // button down
        {
            MouseState1[i] = 1;
            if (mousepresscallback)
                mousepresscallback(i, MouseState1[i]);
            mouseb |= 1<<i;
        }
        else if (rmouse->usButtonFlags & (mask<<1)) // button up
        {
            MouseState1[i] = 0;
            if (mousepresscallback)
                mousepresscallback(i, MouseState1[i]);
            mouseb &= ~(1<<i);
        }
        mask <<= 2;
    }

    MWheel = (rmouse->usButtonFlags & RI_MOUSE_WHEEL) ? rmouse->usButtonData : 0;

    if (MWheel > 0)   	// wheel up
    {
        if (mousewheel[0] > 0 && mousepresscallback) mousepresscallback(5,0);
        mousewheel[0] = getticks();
        mouseb |= 16;
        if (mousepresscallback) mousepresscallback(5, 1);
    }
    else if (MWheel < 0)  	// wheel down
    {
        if (mousewheel[1] > 0 && mousepresscallback) mousepresscallback(6,0);
        mousewheel[1] = getticks();
        mouseb |= 32;
        if (mousepresscallback) mousepresscallback(6, 1);
    }
}

void RI_ProcessKeyboard(const RAWKEYBOARD* rkbd)
{
    uint8_t key = rkbd->MakeCode;
    uint8_t VKey = rkbd->VKey;

    // for some reason rkbd->MakeCode is wrong for these 
    // even though rkbd->VKey is right...

    switch (VKey)
    {
    case VK_SHIFT:
        if (rkbd->Flags & RI_KEY_E0) VKey = VK_RSHIFT, key = sc_RightShift; break;
    case VK_CONTROL:
        if (rkbd->Flags & RI_KEY_E0) VKey = VK_RCONTROL, key = sc_RightControl; break;
    case VK_MENU:
        if (rkbd->Flags & RI_KEY_E0) VKey = VK_RMENU, key = sc_RightAlt; break;
    case VK_UP:
        key = sc_UpArrow; break;
    case VK_DOWN:
        key = sc_DownArrow; break;
    case VK_LEFT:
        key = sc_LeftArrow; break;
    case VK_RIGHT:
        key = sc_RightArrow; break;
    case VK_INSERT:
        key = sc_Insert; break;
    case VK_HOME:
        key = sc_Home; break;
    case VK_DELETE:
        key = sc_Delete; break;
    case VK_END:
        key = sc_End; break;
    case VK_PRIOR:
        key = sc_PgUp; break;
    case VK_NEXT:
        key = sc_PgDn; break;
    }

    KeyboardState[VKey] &= 0xfe;
    KeyboardState[VKey] |= 1 - (rkbd->Flags & RI_KEY_BREAK);

    if (OSD_HandleScanCode(key, (rkbd->Flags & RI_KEY_BREAK) == 0))
    {
        SetKey(key, (rkbd->Flags & RI_KEY_BREAK) == 0);

        if (keypresscallback)
            keypresscallback(key, (rkbd->Flags & RI_KEY_BREAK) == 0);
    }
}

// keyboard is always captured regardless of what we tell this function
int32_t RI_CaptureInput(int32_t grab, HWND target)
{
    RAWINPUTDEVICE raw[2];

    raw[0].usUsagePage = 0x01;
    raw[0].usUsage = 0x02;
    raw[0].dwFlags = grab ? (RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE) : 0;
    raw[0].hwndTarget = grab ? target : NULL;

    raw[1].usUsagePage = 0x01;
    raw[1].usUsage = 0x06;
    raw[1].dwFlags = 0;
    raw[1].hwndTarget = NULL;

    mousegrab = grab;

    return (RegisterRawInputDevices(raw, 2, sizeof(raw[0])) == FALSE);
}

uint8_t RI_MouseState(uint8_t Button)
{
    return ((MouseState0[Button-1] << 1) | MouseState1[Button-1]) & 0x03;
}

void RI_PollDevices()
{
    int32_t i;
    MSG msg;

    if (!init_done)
    {
        if (RI_CaptureInput(1, (HWND)win_gethwnd()))
            return;

        init_done = 1;
    }

    for (i = 0; i < 256; i++)
        KeyboardState[i] = (KeyboardState[i] << 1) | (1 & KeyboardState[i]);

    Bmemcpy(MouseState0, MouseState1, sizeof(MouseState0));

    MWheel = 0;

    while (PeekMessage(&msg, 0, WM_INPUT, WM_INPUT, PM_REMOVE | PM_QS_INPUT))
    {
        if (GET_RAWINPUT_CODE_WPARAM(msg.wParam) == RIM_INPUT)
        {
            UINT dwSize = sizeof(RAWINPUT);
            RAWINPUT raw;

            GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER));

            if (raw.header.dwType == RIM_TYPEKEYBOARD)
                RI_ProcessKeyboard(&raw.data.keyboard);
            else if (raw.header.dwType == RIM_TYPEMOUSE)
                RI_ProcessMouse(&raw.data.mouse);
        }

        DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    }

    if (mousegrab)
    {
        // center the cursor in the window
        POINT pt = { xdim>>1, ydim>>1 };

        ClientToScreen((HWND)win_gethwnd(), &pt);
        SetCursorPos(pt.x, pt.y);

        // do this here because we only want the wheel to signal once, but hold the state for a moment
        if (mousewheel[0] > 0 && getticks() - mousewheel[0] > MouseWheelFakePressTime)
        {
            if (mousepresscallback) mousepresscallback(5,0);
            mousewheel[0] = 0; mouseb &= ~16;
        }
        if (mousewheel[1] > 0 && getticks() - mousewheel[1] > MouseWheelFakePressTime)
        {
            if (mousepresscallback) mousepresscallback(6,0);
            mousewheel[1] = 0; mouseb &= ~32;
        }
    }
}

int32_t initmouse(void)
{
    if (moustat) return 0;
    grabmouse(moustat = 1);
    return 0;
}

void uninitmouse(void)
{
    if (!moustat) return;
    grabmouse(moustat = 0);
}

void grabmouse(char a)
{
    static POINT pos;
    static int32_t d = 0;
     
    if (!moustat) return;

    if (!mousegrab || !d)
    {
        GetCursorPos(&pos);
        d = 1;
    }

    ShowCursor(a == 0);
    RI_CaptureInput(a, (HWND)win_gethwnd());
    SetCursorPos(pos.x, pos.y);

    mousex = mousey = mouseb = 0;
}

void readmousexy(int32_t *x, int32_t *y)
{
    if (!moustat || !mousegrab) { *x = *y = 0; return; }
    *x = mousex;
    mousex = 0;
    *y = mousey;
    mousey = 0;
}

void readmousebstatus(int32_t *b)
{
    if (!moustat || !mousegrab) { *b = 0; return; }
    *b = mouseb;
}

