
#include "windows_inc.h"

#include "compat.h"
#include "rawinput.h"
#include "winlayer.h"
#include "scancodes.h"
#include "build.h"

static BOOL rawinput_started = 0;
static uint8_t KeyboardState[256] = {0}; // VKeys

extern bool g_mouseEnabled, g_mouseGrabbed;
extern void keySetState(int32_t key, int32_t state);

//#define MASK_DOWN (1<<(i<<1))
//#define MASK_UP (MASK_DOWN<<1)
#ifndef GET_RAWINPUT_CODE_WPARAM
#define GET_RAWINPUT_CODE_WPARAM(wParam)    ((wParam) & 0xff)
#endif

static inline void RI_ProcessMouse(const RAWMOUSE *rmouse)
{
    int32_t i, mask;
    int8_t MWheel = 0;

    if (!g_mouseGrabbed || !appactive)
        return;

    g_mousePos.x += rmouse->lLastX;
    g_mousePos.y += rmouse->lLastY;

    if (rmouse->usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        // untested... maybe devices like wacom tablets set this flag?
        POINT pos = { xdim>>1, ydim>>1 };

        ClientToScreen((HWND)win_gethwnd(), &pos);

        g_mousePos.x -= pos.x;
        g_mousePos.y -= pos.y;
    }

    for (i = 0, mask = (1<<0); mask <= (1<<8); i++, mask<<=2)
    {
        // usButtonFlags:
        //  1<<0: left down   -> 1 / 1<<0
        //  1<<2: middle down -> 2 / 1<<1
        //  1<<4: right down  -> 3 / 1<<2
        //  1<<6: x1 down     -> 4 / 1<<3
        //                    ----------- mwheel here
        //  1<<8: x2 down     -> 7 / 1<<6

        if (mask == 1<<8)
            i = 6;

        if (rmouse->usButtonFlags & mask) // button down
        {
            if (g_mouseCallback)
                g_mouseCallback(i+1, 1);
            g_mouseBits |= 1<<i;
        }
        else if (rmouse->usButtonFlags & (mask<<1)) // button up
        {
            if (g_mouseCallback)
                g_mouseCallback(i+1, 0);
            g_mouseBits &= ~(1<<i);
        }
    }

    MWheel = (rmouse->usButtonFlags & RI_MOUSE_WHEEL) ? rmouse->usButtonData : 0;

    if (MWheel > 0)   	// wheel up
    {
        g_mouseBits |= 16;
        if (g_mouseCallback) g_mouseCallback(5, 1);
    }
    else if (MWheel < 0)  	// wheel down
    {
        g_mouseBits |= 32;
        if (g_mouseCallback) g_mouseCallback(6, 1);
    }
}

static inline void RI_ProcessKeyboard(const RAWKEYBOARD *rkbd)
{
    uint8_t key = rkbd->MakeCode, VKey = rkbd->VKey;

    // for some reason rkbd->MakeCode is wrong for these
    // even though rkbd->VKey is right...

    switch (VKey)
    {
    case VK_SHIFT:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_RSHIFT, key = sc_RightShift;
        break;
    case VK_CONTROL:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_RCONTROL, key = sc_RightControl;
        break;
    case VK_MENU:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_RMENU, key = sc_RightAlt;
        break;
    case VK_UP:
    case VK_NUMPAD8:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_UP, key = sc_UpArrow;
        else
            VKey = VK_NUMPAD8, key = sc_kpad_8;
        break;
    case VK_DOWN:
    case VK_NUMPAD2:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_DOWN, key = sc_DownArrow;
        else
            VKey = VK_NUMPAD2, key = sc_kpad_2;
        break;
    case VK_LEFT:
    case VK_NUMPAD4:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_LEFT, key = sc_LeftArrow;
        else
            VKey = VK_NUMPAD4, key = sc_kpad_4;
        break;
    case VK_RIGHT:
    case VK_NUMPAD6:
        if (rkbd->Flags & RI_KEY_E0)
            VKey = VK_RIGHT, key = sc_RightArrow;
        else
            VKey = VK_NUMPAD6, key = sc_kpad_6;
        break;
    case VK_DIVIDE:
#if 0
        if (rkbd->Flags & RI_KEY_E0)
            key = sc_Slash;
        else
#endif
        key = sc_kpad_Slash;
        break;
    case VK_INSERT:
        key = sc_Insert;
        break;
    case VK_HOME:
        key = sc_Home;
        break;
    case VK_DELETE:
        key = sc_Delete;
        break;
    case VK_END:
        key = sc_End;
        break;
    case VK_PRIOR:
        key = sc_PgUp;
        break;
    case VK_NEXT:
        key = sc_PgDn;
        break;
    case VK_RETURN:
        if (rkbd->Flags & RI_KEY_E0)
            key = sc_kpad_Enter;
        break;
    case VK_PAUSE:
        KeyboardState[VKey] = 1 - (rkbd->Flags & RI_KEY_BREAK);
        if (rkbd->Flags & RI_KEY_BREAK)
            return;

        keySetState(sc_Pause, 1);

        if (keypresscallback)
            keypresscallback(sc_Pause, 1);
    case 0xFF:
        return;
    }

    KeyboardState[VKey] = 1 - (rkbd->Flags & RI_KEY_BREAK);

    if (OSD_HandleScanCode(key, KeyboardState[VKey] > 0))
    {
        keySetState(key, KeyboardState[VKey] != 0);

        if (keypresscallback)
            keypresscallback(key, KeyboardState[VKey] != 0);
    }

    if (rkbd->Flags & RI_KEY_BREAK) return;
    if (((g_keyAsciiEnd+1)&(KEYFIFOSIZ-1)) == g_keyAsciiPos) return;
    if ((g_keyAsciiEnd - g_keyAsciiPos) > 0) return;

    {
        uint8_t buf[2];

        if (ToAscii(VKey, key, &KeyboardState[0], (LPWORD)&buf[0], 0) != 1) return;
        if ((OSD_OSDKey() < 128) && (Btolower(g_keyAsciiTable[OSD_OSDKey()]) == Btolower(buf[0]))) return;
        if (OSD_HandleChar(buf[0]) == 0) return;

        g_keyAsciiFIFO[g_keyAsciiEnd] = buf[0];
        g_keyAsciiEnd = ((g_keyAsciiEnd+1)&(KEYFIFOSIZ-1));
    }
}

// keyboard is always captured regardless of what we tell this function
BOOL RI_CaptureInput(BOOL grab, HWND target)
{
    RAWINPUTDEVICE raw[2];

    raw[0].usUsagePage = 0x01;
    raw[0].usUsage = 0x02;
    raw[0].dwFlags = grab ? (RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE) : 0;
    raw[0].hwndTarget = grab ? target : NULL;

    raw[1].usUsagePage = 0x01;
    raw[1].usUsage = 0x06;
    raw[1].dwFlags = 0;
    raw[1].hwndTarget = target;

    g_mouseGrabbed = grab;

    return (RegisterRawInputDevices(raw, 2, sizeof(raw[0])) == FALSE);
}

void RI_ProcessMessage(MSG *msg)
{
    if (GET_RAWINPUT_CODE_WPARAM(msg->wParam) == RIM_INPUT)
    {
        UINT dwSize = sizeof(RAWINPUT);
        RAWINPUT raw;

        GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER));

        if (raw.header.dwType == RIM_TYPEKEYBOARD)
            RI_ProcessKeyboard(&raw.data.keyboard);
        else if (raw.header.dwType == RIM_TYPEMOUSE)
            RI_ProcessMouse(&raw.data.mouse);
    }

    DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
}

void RI_PollDevices(BOOL loop)
{
    int32_t i;
    MSG msg;

    if (!rawinput_started)
    {
        if (RI_CaptureInput(1, (HWND)win_gethwnd()))
            return;
        rawinput_started = 1;
    }

    if (inputchecked)
    {
        if (g_mouseCallback)
        {
            if (g_mouseBits & 16)
                g_mouseCallback(5, 0);
            if (g_mouseBits & 32)
                g_mouseCallback(6, 0);
        }
        g_mouseBits &= ~(16|32);
    }

    // snapshot the whole keyboard state so we can translate key presses into ascii later
    for (i = 0; i < 256; i++)
        KeyboardState[i] = GetAsyncKeyState(i) >> 8;

    while (loop && PeekMessage(&msg, 0, WM_INPUT, WM_INPUT, PM_REMOVE | PM_QS_INPUT))
        RI_ProcessMessage(&msg);

    if (g_mouseGrabbed && appactive)
    {
        // center the cursor in the window
        POINT pt = { xdim>>1, ydim>>1 };

        ClientToScreen((HWND)win_gethwnd(), &pt);
        SetCursorPos(pt.x, pt.y);
    }
}

void mouseInit(void)
{
    if (!g_mouseEnabled)
        mouseGrabInput(g_mouseEnabled = g_mouseLockedToWindow);
}

void mouseUninit(void)
{
    if (!g_mouseEnabled) return;
    mouseGrabInput(g_mouseEnabled = 0);
}

void mouseGrabInput(bool grab)
{
    static POINT pos;
    static int32_t d = 0;

    if (!g_mouseEnabled) return;

    if (!g_mouseGrabbed || !d)
    {
        GetCursorPos(&pos);
        d++;
    }

    ShowCursor(grab == 0);
    RI_CaptureInput(grab, (HWND)win_gethwnd());
    SetCursorPos(pos.x, pos.y);
}

void mouseLockToWindow(char a)
{
    UNREFERENCED_PARAMETER(a);
}
