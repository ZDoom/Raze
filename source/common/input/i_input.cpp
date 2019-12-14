/*
** This code is partially original EDuke32 code, partially taken from ZDoom
**
** For the portions taken from ZDoom the following applies:
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <SDL.h>
#include "compat.h"

//#include "doomtype.h"
//#include "doomdef.h"
//#include "doomstat.h"
#include "m_argv.h"
//#include "v_video.h"

#include "c_buttons.h"
#include "d_event.h"
#include "d_gui.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "keydef.h"
//#include "dikeys.h"
//#include "events.h"
//#include "g_game.h"
//#include "g_levellocals.h"
#include "utf8.h"
#include "menu/menu.h"


char grabmouse_low(char a);
void WindowMoved(int x, int y);
extern SDL_Window* sdl_window;


static uint8_t keytranslation[SDL_NUM_SCANCODES];

void buildkeytranslationtable(void)
{
    memset(keytranslation, 0, sizeof(keytranslation));

#define MAP(x,y) keytranslation[x] = y
    MAP(SDL_SCANCODE_BACKSPACE, 0xe);
    MAP(SDL_SCANCODE_TAB, 0xf);
    MAP(SDL_SCANCODE_RETURN, 0x1c);
    MAP(SDL_SCANCODE_PAUSE, 0x59);	// 0x1d + 0x45 + 0x9d + 0xc5
    MAP(SDL_SCANCODE_ESCAPE, 0x1);
    MAP(SDL_SCANCODE_SPACE, 0x39);
    MAP(SDL_SCANCODE_COMMA, 0x33);
    MAP(SDL_SCANCODE_NONUSBACKSLASH, 0x56);
    MAP(SDL_SCANCODE_MINUS, 0xc);
    MAP(SDL_SCANCODE_PERIOD, 0x34);
    MAP(SDL_SCANCODE_SLASH, 0x35);
    MAP(SDL_SCANCODE_0, 0xb);
    MAP(SDL_SCANCODE_1, 0x2);
    MAP(SDL_SCANCODE_2, 0x3);
    MAP(SDL_SCANCODE_3, 0x4);
    MAP(SDL_SCANCODE_4, 0x5);
    MAP(SDL_SCANCODE_5, 0x6);
    MAP(SDL_SCANCODE_6, 0x7);
    MAP(SDL_SCANCODE_7, 0x8);
    MAP(SDL_SCANCODE_8, 0x9);
    MAP(SDL_SCANCODE_9, 0xa);
    MAP(SDL_SCANCODE_SEMICOLON, 0x27);
    MAP(SDL_SCANCODE_APOSTROPHE, 0x28);
    MAP(SDL_SCANCODE_EQUALS, 0xd);
    MAP(SDL_SCANCODE_LEFTBRACKET, 0x1a);
    MAP(SDL_SCANCODE_BACKSLASH, 0x2b);
    MAP(SDL_SCANCODE_RIGHTBRACKET, 0x1b);
    MAP(SDL_SCANCODE_A, 0x1e);
    MAP(SDL_SCANCODE_B, 0x30);
    MAP(SDL_SCANCODE_C, 0x2e);
    MAP(SDL_SCANCODE_D, 0x20);
    MAP(SDL_SCANCODE_E, 0x12);
    MAP(SDL_SCANCODE_F, 0x21);
    MAP(SDL_SCANCODE_G, 0x22);
    MAP(SDL_SCANCODE_H, 0x23);
    MAP(SDL_SCANCODE_I, 0x17);
    MAP(SDL_SCANCODE_J, 0x24);
    MAP(SDL_SCANCODE_K, 0x25);
    MAP(SDL_SCANCODE_L, 0x26);
    MAP(SDL_SCANCODE_M, 0x32);
    MAP(SDL_SCANCODE_N, 0x31);
    MAP(SDL_SCANCODE_O, 0x18);
    MAP(SDL_SCANCODE_P, 0x19);
    MAP(SDL_SCANCODE_Q, 0x10);
    MAP(SDL_SCANCODE_R, 0x13);
    MAP(SDL_SCANCODE_S, 0x1f);
    MAP(SDL_SCANCODE_T, 0x14);
    MAP(SDL_SCANCODE_U, 0x16);
    MAP(SDL_SCANCODE_V, 0x2f);
    MAP(SDL_SCANCODE_W, 0x11);
    MAP(SDL_SCANCODE_X, 0x2d);
    MAP(SDL_SCANCODE_Y, 0x15);
    MAP(SDL_SCANCODE_Z, 0x2c);
    MAP(SDL_SCANCODE_DELETE, 0xd3);
    MAP(SDL_SCANCODE_KP_0, 0x52);
    MAP(SDL_SCANCODE_KP_1, 0x4f);
    MAP(SDL_SCANCODE_KP_2, 0x50);
    MAP(SDL_SCANCODE_KP_3, 0x51);
    MAP(SDL_SCANCODE_KP_4, 0x4b);
    MAP(SDL_SCANCODE_KP_5, 0x4c);
    MAP(SDL_SCANCODE_KP_CLEAR, 0x4c);
    MAP(SDL_SCANCODE_CLEAR, 0x4c);
    MAP(SDL_SCANCODE_KP_6, 0x4d);
    MAP(SDL_SCANCODE_KP_7, 0x47);
    MAP(SDL_SCANCODE_KP_8, 0x48);
    MAP(SDL_SCANCODE_KP_9, 0x49);
    MAP(SDL_SCANCODE_KP_PERIOD, 0x53);
    MAP(SDL_SCANCODE_KP_DIVIDE, 0xb5);
    MAP(SDL_SCANCODE_KP_MULTIPLY, 0x37);
    MAP(SDL_SCANCODE_KP_MINUS, 0x4a);
    MAP(SDL_SCANCODE_KP_PLUS, 0x4e);
    MAP(SDL_SCANCODE_KP_ENTER, 0x9c);
    //MAP(SDL_SCANCODE_KP_EQUALS,	);
    MAP(SDL_SCANCODE_UP, 0xc8);
    MAP(SDL_SCANCODE_DOWN, 0xd0);
    MAP(SDL_SCANCODE_RIGHT, 0xcd);
    MAP(SDL_SCANCODE_LEFT, 0xcb);
    MAP(SDL_SCANCODE_INSERT, 0xd2);
    MAP(SDL_SCANCODE_HOME, 0xc7);
    MAP(SDL_SCANCODE_END, 0xcf);
    MAP(SDL_SCANCODE_PAGEUP, 0xc9);
    MAP(SDL_SCANCODE_PAGEDOWN, 0xd1);
    MAP(SDL_SCANCODE_F1, 0x3b);
    MAP(SDL_SCANCODE_F2, 0x3c);
    MAP(SDL_SCANCODE_F3, 0x3d);
    MAP(SDL_SCANCODE_F4, 0x3e);
    MAP(SDL_SCANCODE_F5, 0x3f);
    MAP(SDL_SCANCODE_F6, 0x40);
    MAP(SDL_SCANCODE_F7, 0x41);
    MAP(SDL_SCANCODE_F8, 0x42);
    MAP(SDL_SCANCODE_F9, 0x43);
    MAP(SDL_SCANCODE_F10, 0x44);
    MAP(SDL_SCANCODE_F11, 0x57);
    MAP(SDL_SCANCODE_F12, 0x58);
    MAP(SDL_SCANCODE_NUMLOCKCLEAR, 0x45);
    MAP(SDL_SCANCODE_CAPSLOCK, 0x3a);
    MAP(SDL_SCANCODE_SCROLLLOCK, 0x46);
    MAP(SDL_SCANCODE_RSHIFT, 0x36);
    MAP(SDL_SCANCODE_LSHIFT, 0x2a);
    MAP(SDL_SCANCODE_RCTRL, 0x9d);
    MAP(SDL_SCANCODE_LCTRL, 0x1d);
    MAP(SDL_SCANCODE_RALT, 0xb8);
    MAP(SDL_SCANCODE_LALT, 0x38);
    MAP(SDL_SCANCODE_LGUI, 0xdb);	// win l
    MAP(SDL_SCANCODE_RGUI, 0xdc);	// win r
//    MAP(SDL_SCANCODE_PRINTSCREEN,		-2);	// 0xaa + 0xb7
    MAP(SDL_SCANCODE_SYSREQ, 0x54);	// alt+printscr
//    MAP(SDL_SCANCODE_PAUSE,		0xb7);	// ctrl+pause
    MAP(SDL_SCANCODE_MENU, 0xdd);	// win menu?
    MAP(SDL_SCANCODE_GRAVE, 0x29);  // tilde
#undef MAP
}



//
// initmouse() -- init mouse input
//
void mouseInit(void)
{
    mouseGrabInput(g_mouseEnabled = g_mouseLockedToWindow);  // FIXME - SA
}

//
// uninitmouse() -- uninit mouse input
//
void mouseUninit(void)
{
    mouseGrabInput(0);
    g_mouseEnabled = 0;
}


//
// grabmouse_low() -- show/hide mouse cursor, lower level (doesn't check state).
//                    furthermore return 0 if successful.
//

char grabmouse_low(char a)
{
    /* FIXME: Maybe it's better to make sure that grabmouse_low
       is called only when a window is ready?                */
    if (sdl_window)
        SDL_SetWindowGrab(sdl_window, a ? SDL_TRUE : SDL_FALSE);
    return SDL_SetRelativeMouseMode(a ? SDL_TRUE : SDL_FALSE);
}

//
// grabmouse() -- show/hide mouse cursor
//
void mouseGrabInput(bool grab)
{
    if (appactive && g_mouseEnabled)
    {
        if ((grab != g_mouseGrabbed) && !grabmouse_low(grab))
            g_mouseGrabbed = grab;
    }
    else
        g_mouseGrabbed = grab;

    inputState.MouseSetPos(0, 0);
    SDL_ShowCursor(!grab ? SDL_ENABLE : SDL_DISABLE);
    if (grab) GUICapture &= ~1;
    else GUICapture |= 1;
}


// So this is how the engine handles text input?
// Argh. This is just gross.
int scancodetoasciihack(SDL_Event& ev)
{
    int sc = ev.key.keysym.scancode;
    SDL_Keycode keyvalue = ev.key.keysym.sym;
    int code = keytranslation[sc];
    // Modifiers that have to be held down to be effective
    // (excludes KMOD_NUM, for example).
    static const int MODIFIERS =
        KMOD_LSHIFT | KMOD_RSHIFT | KMOD_LCTRL | KMOD_RCTRL |
        KMOD_LALT | KMOD_RALT | KMOD_LGUI | KMOD_RGUI;

    // XXX: see osd.c, OSD_HandleChar(), there are more...
    if (
        (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER ||
            sc == SDL_SCANCODE_ESCAPE ||
            sc == SDL_SCANCODE_BACKSPACE ||
            sc == SDL_SCANCODE_TAB ||
            (((ev.key.keysym.mod) & MODIFIERS) == KMOD_LCTRL &&
            (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z))))
    {
        char keyvalue;
        switch (sc)
        {
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: keyvalue = '\r'; break;
        case SDL_SCANCODE_ESCAPE: keyvalue = 27; break;
        case SDL_SCANCODE_BACKSPACE: keyvalue = '\b'; break;
        case SDL_SCANCODE_TAB: keyvalue = '\t'; break;
        default: keyvalue = sc - SDL_SCANCODE_A + 1; break;  // Ctrl+A --> 1, etc.
        }
    }
    else
    {
        /*
        Necessary for Duke 3D's method of entering cheats to work without showing IMEs.
        SDL_TEXTINPUT is preferable overall, but with bitmap fonts it has no advantage.
        */
        // Note that this is not how text input is supposed to be handled!

        if ('a' <= keyvalue && keyvalue <= 'z')
        {
            if (!!(ev.key.keysym.mod & KMOD_SHIFT) ^ !!(ev.key.keysym.mod & KMOD_CAPS))
                keyvalue -= 'a' - 'A';
        }
        else if (ev.key.keysym.mod & KMOD_SHIFT)
        {
            keyvalue = g_keyAsciiTableShift[code];
        }
        else if (ev.key.keysym.mod & KMOD_NUM) // && !(ev.key.keysym.mod & KMOD_SHIFT)
        {
            switch (keyvalue)
            {
            case SDLK_KP_1: keyvalue = '1'; break;
            case SDLK_KP_2: keyvalue = '2'; break;
            case SDLK_KP_3: keyvalue = '3'; break;
            case SDLK_KP_4: keyvalue = '4'; break;
            case SDLK_KP_5: keyvalue = '5'; break;
            case SDLK_KP_6: keyvalue = '6'; break;
            case SDLK_KP_7: keyvalue = '7'; break;
            case SDLK_KP_8: keyvalue = '8'; break;
            case SDLK_KP_9: keyvalue = '9'; break;
            case SDLK_KP_0: keyvalue = '0'; break;
            case SDLK_KP_PERIOD: keyvalue = '.'; break;
            case SDLK_KP_COMMA: keyvalue = ','; break;
            }
        }

        switch (keyvalue)
        {
        case SDLK_KP_DIVIDE: keyvalue = '/'; break;
        case SDLK_KP_MULTIPLY: keyvalue = '*'; break;
        case SDLK_KP_MINUS: keyvalue = '-'; break;
        case SDLK_KP_PLUS: keyvalue = '+'; break;
        }
    }
    if (keyvalue >= 0x80) keyvalue = 0; // Sadly ASCII only...
    return keyvalue;
}

static void PostMouseMove(int x, int y)
{
    static int lastx = 0, lasty = 0;
    event_t ev = { 0,0,0,0,0,0,0 };

    ev.x = x;
    ev.y = y;
    lastx = x;
    lasty = y;
    if (ev.x | ev.y)
    {
        ev.type = EV_Mouse;
        D_PostEvent(&ev);
    }
}

CVAR(Bool, m_noprescale, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

static void MouseRead()
{
    int x, y;

#if 0
    if (NativeMouse)
    {
        return;
    }
#endif

    SDL_GetRelativeMouseState(&x, &y);
    if (!m_noprescale)
    {
        x *= 3;
        y *= 2;
    }
    if (x | y)
    {
        PostMouseMove(x, -y);
    }
}

int32_t handleevents_pollsdl(void)
{
    int32_t code, rv = 0, j;
    SDL_Event ev;
    event_t evt;

    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
        case SDL_TEXTINPUT:
        {
            j = 0;
            const uint8_t* text = (uint8_t*)ev.text.text;
            while ((j = GetCharFromString(text)))
            {
                code = ev.text.text[j];
                // Fixme: Send an EV_GUI_Event instead and properly deal with Unicode.
                if ((GUICapture & 1) && menuactive != MENU_WaitKey)
                {
                    evt = { EV_GUI_Event, EV_GUI_Char, int16_t(j), !!(SDL_GetModState() & KMOD_ALT) };
                    D_PostEvent(&evt);
                }
            }
            break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if ((GUICapture & 1) && menuactive != MENU_WaitKey)
            {
                evt = {};
                evt.type = EV_GUI_Event;
                evt.subtype = ev.type == SDL_KEYDOWN ? EV_GUI_KeyDown : EV_GUI_KeyUp;
                SDL_Keymod kmod = SDL_GetModState();
                evt.data3 = ((kmod & KMOD_SHIFT) ? GKM_SHIFT : 0) |
                    ((kmod & KMOD_CTRL) ? GKM_CTRL : 0) |
                    ((kmod & KMOD_ALT) ? GKM_ALT : 0);

                if (evt.subtype == EV_GUI_KeyDown && ev.key.repeat)
                {
                    evt.subtype = EV_GUI_KeyRepeat;
                }

                switch (ev.key.keysym.sym)
                {
                case SDLK_KP_ENTER:	evt.data1 = GK_RETURN;	break;
                case SDLK_PAGEUP:	evt.data1 = GK_PGUP;		break;
                case SDLK_PAGEDOWN:	evt.data1 = GK_PGDN;		break;
                case SDLK_END:		evt.data1 = GK_END;		break;
                case SDLK_HOME:		evt.data1 = GK_HOME;		break;
                case SDLK_LEFT:		evt.data1 = GK_LEFT;		break;
                case SDLK_RIGHT:	evt.data1 = GK_RIGHT;		break;
                case SDLK_UP:		evt.data1 = GK_UP;		break;
                case SDLK_DOWN:		evt.data1 = GK_DOWN;		break;
                case SDLK_DELETE:	evt.data1 = GK_DEL;		break;
                case SDLK_ESCAPE:	evt.data1 = GK_ESCAPE;	break;
                case SDLK_F1:		evt.data1 = GK_F1;		break;
                case SDLK_F2:		evt.data1 = GK_F2;		break;
                case SDLK_F3:		evt.data1 = GK_F3;		break;
                case SDLK_F4:		evt.data1 = GK_F4;		break;
                case SDLK_F5:		evt.data1 = GK_F5;		break;
                case SDLK_F6:		evt.data1 = GK_F6;		break;
                case SDLK_F7:		evt.data1 = GK_F7;		break;
                case SDLK_F8:		evt.data1 = GK_F8;		break;
                case SDLK_F9:		evt.data1 = GK_F9;		break;
                case SDLK_F10:		evt.data1 = GK_F10;		break;
                case SDLK_F11:		evt.data1 = GK_F11;		break;
                case SDLK_F12:		evt.data1 = GK_F12;		break;
                default:
                    if (ev.key.keysym.sym < 256)
                    {
                        evt.data1 = ev.key.keysym.sym;
                    }
                    break;
                }
                if (evt.data1 < 128)
                {
                    evt.data1 = toupper(evt.data1);
                    D_PostEvent(&evt);
                }
                }
            else
            {
                auto const& sc = ev.key.keysym.scancode;
                code = keytranslation[sc];

                // The pause key generates a release event right after
                // the pressing one. As a result, it gets unseen
                // by the game most of the time.
                if (code == 0x59 && ev.type == SDL_KEYUP)  // pause
                    break;

                int keyvalue = ev.type == SDL_KEYDOWN ? scancodetoasciihack(ev) : 0;
                event_t evt = { (uint8_t)(ev.type == SDL_KEYUP ? EV_KeyUp : EV_KeyDown), 0, (int16_t)code, (int16_t)keyvalue };
                D_PostEvent(&evt);
            }

            break;
            }

        case SDL_MOUSEWHEEL:
            if (GUICapture)
            {
                evt.type = EV_GUI_Event;

                if (ev.wheel.y == 0)
                    evt.subtype = ev.wheel.x > 0 ? EV_GUI_WheelRight : EV_GUI_WheelLeft;
                else
                    evt.subtype = ev.wheel.y > 0 ? EV_GUI_WheelUp : EV_GUI_WheelDown;

                SDL_Keymod kmod = SDL_GetModState();
                evt.data3 = ((kmod & KMOD_SHIFT) ? GKM_SHIFT : 0) |
                    ((kmod & KMOD_CTRL) ? GKM_CTRL : 0) |
                    ((kmod & KMOD_ALT) ? GKM_ALT : 0);

                D_PostEvent(&evt);
            }
            else
            {
                // This never sends keyup events. They must be delayed for this to work with game buttons.
                if (ev.wheel.y > 0)
                {
                    evt = { EV_KeyDown, 0, (int16_t)KEY_MWHEELUP };
                    D_PostEvent(&evt);
                }
                if (ev.wheel.y < 0)
                {
                    evt = { EV_KeyDown, 0, (int16_t)KEY_MWHEELDOWN };
                    D_PostEvent(&evt);
                }
                if (ev.wheel.x > 0)
                {
                    evt = { EV_KeyDown, 0, (int16_t)KEY_MWHEELRIGHT };
                    D_PostEvent(&evt);
                }
                if (ev.wheel.y < 0)
                {
                    evt = { EV_KeyDown, 0, (int16_t)KEY_MWHEELLEFT };
                    D_PostEvent(&evt);
                }
            }
            break;

        case SDL_WINDOWEVENT:
            switch (ev.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            case SDL_WINDOWEVENT_FOCUS_LOST:
                appactive = (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED);
                if (g_mouseGrabbed && g_mouseEnabled)
                    grabmouse_low(appactive);
                break;

            case SDL_WINDOWEVENT_MOVED:
            {
                WindowMoved(ev.window.data1, ev.window.data2);
                break;
            }
            case SDL_WINDOWEVENT_ENTER:
                g_mouseInsideWindow = 1;
                break;
            case SDL_WINDOWEVENT_LEAVE:
                g_mouseInsideWindow = 0;
                break;
            }

            break;

        case SDL_MOUSEMOTION:
            //case SDL_JOYBALLMOTION:
        {
            // The menus need this, even in non GUI-capture mode
            event_t event;
            event.data1 = ev.motion.x;
            event.data2 = ev.motion.y;

            //screen->ScaleCoordsFromWindow(event.data1, event.data2);

            event.type = EV_GUI_Event;
            event.subtype = EV_GUI_MouseMove;

            SDL_Keymod kmod = SDL_GetModState();
            event.data3 = ((kmod & KMOD_SHIFT) ? GKM_SHIFT : 0) |
                ((kmod & KMOD_CTRL) ? GKM_CTRL : 0) |
                ((kmod & KMOD_ALT) ? GKM_ALT : 0);

            D_PostEvent(&event);
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:

        if (!GUICapture)
        {
            evt.type = ev.type == SDL_MOUSEBUTTONDOWN ? EV_KeyDown : EV_KeyUp;

            switch (ev.button.button)
            {
            case SDL_BUTTON_LEFT:	evt.data1 = KEY_MOUSE1;		break;
            case SDL_BUTTON_MIDDLE:	evt.data1 = KEY_MOUSE3;		break;
            case SDL_BUTTON_RIGHT:	evt.data1 = KEY_MOUSE2;		break;
            case SDL_BUTTON_X1:		evt.data1 = KEY_MOUSE4;		break;
            case SDL_BUTTON_X2:		evt.data1 = KEY_MOUSE5;		break;
            case 6:		evt.data1 = KEY_MOUSE6;		break;
            case 7:		evt.data1 = KEY_MOUSE7;		break;
            case 8:		evt.data1 = KEY_MOUSE8;		break;
                //default:	printf("SDL mouse button %s %d\n", sev.type == SDL_MOUSEBUTTONDOWN ? "down" : "up", sev.button.button);	break;
            }

            if (evt.data1 != 0)
            {
                D_PostEvent(&evt);
            }
        }
        else if ((ev.button.button >= SDL_BUTTON_LEFT && ev.button.button <= SDL_BUTTON_X2))
        {
            int x, y;
            SDL_GetMouseState(&x, &y);

            evt.type = EV_GUI_Event;
            evt.data1 = x;
            evt.data2 = y;

            //screen->ScaleCoordsFromWindow(event.data1, event.data2);

            if (ev.type == SDL_MOUSEBUTTONDOWN)
            {
                switch (ev.button.button)
                {
                case SDL_BUTTON_LEFT:   evt.subtype = EV_GUI_LButtonDown;    break;
                case SDL_BUTTON_MIDDLE: evt.subtype = EV_GUI_MButtonDown;    break;
                case SDL_BUTTON_RIGHT:  evt.subtype = EV_GUI_RButtonDown;    break;
                case SDL_BUTTON_X1:     evt.subtype = EV_GUI_BackButtonDown; break;
                case SDL_BUTTON_X2:     evt.subtype = EV_GUI_FwdButtonDown;  break;
                default: assert(false); evt.subtype = EV_GUI_None;           break;
                }
            }
            else
            {
                switch (ev.button.button)
                {
                case SDL_BUTTON_LEFT:   evt.subtype = EV_GUI_LButtonUp;    break;
                case SDL_BUTTON_MIDDLE: evt.subtype = EV_GUI_MButtonUp;    break;
                case SDL_BUTTON_RIGHT:  evt.subtype = EV_GUI_RButtonUp;    break;
                case SDL_BUTTON_X1:     evt.subtype = EV_GUI_BackButtonUp; break;
                case SDL_BUTTON_X2:     evt.subtype = EV_GUI_FwdButtonUp;  break;
                default: assert(false); evt.subtype = EV_GUI_None;         break;
                }
            }

            SDL_Keymod kmod = SDL_GetModState();
            evt.data3 = ((kmod & KMOD_SHIFT) ? GKM_SHIFT : 0) |
                ((kmod & KMOD_CTRL) ? GKM_CTRL : 0) |
                ((kmod & KMOD_ALT) ? GKM_ALT : 0);

            D_PostEvent(&evt);
        }
        break;

#if 0
        case SDL_JOYAXISMOTION:
#if SDL_MAJOR_VERSION >= 2
            if (joystick.isGameController)
                break;
            fallthrough__;
        case SDL_CONTROLLERAXISMOTION:
#endif
            if (appactive && ev.jaxis.axis < joystick.numAxes)
            {
                joystick.pAxis[ev.jaxis.axis] = ev.jaxis.value;
                int32_t const scaledValue = ev.jaxis.value * 10000 / 32767;
                if ((scaledValue < joydead[ev.jaxis.axis]) &&
                    (scaledValue > -joydead[ev.jaxis.axis]))
                    joystick.pAxis[ev.jaxis.axis] = 0;
                else if (scaledValue >= joysatur[ev.jaxis.axis])
                    joystick.pAxis[ev.jaxis.axis] = 32767;
                else if (scaledValue <= -joysatur[ev.jaxis.axis])
                    joystick.pAxis[ev.jaxis.axis] = -32767;
                else
                    joystick.pAxis[ev.jaxis.axis] = joystick.pAxis[ev.jaxis.axis] * 10000 / joysatur[ev.jaxis.axis];
            }
            break;
#endif

        case SDL_JOYHATMOTION:
        {
            int32_t hatvals[16] = {
                -1,     // centre
                0,      // up 1
                9000,   // right 2
                4500,   // up+right 3
                18000,  // down 4
                -1,     // down+up!! 5
                13500,  // down+right 6
                -1,     // down+right+up!! 7
                27000,  // left 8
                27500,  // left+up 9
                -1,     // left+right!! 10
                -1,     // left+right+up!! 11
                22500,  // left+down 12
                -1,     // left+down+up!! 13
                -1,     // left+down+right!! 14
                -1,     // left+down+right+up!! 15
            };
            if (appactive && ev.jhat.hat < joystick.numHats)
                joystick.pHat[ev.jhat.hat] = hatvals[ev.jhat.value & 15];
            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            if (!GUICapture)
            {
                evt.type = ev.type == SDL_JOYBUTTONDOWN ? EV_KeyDown : EV_KeyUp;
                evt.data1 = KEY_FIRSTJOYBUTTON + ev.jbutton.button;
                if (evt.data1 != 0)
                    D_PostEvent(&evt);
            }
            break;

        case SDL_QUIT:
            throw ExitEvent(0);	// completely bypass the hackery in the games to block Alt-F4.
            return -1;

        default:
            break;
        }
    }
    MouseRead();

    return rv;
}

int32_t handleevents(void)
{
    int32_t rv;

    if (inputchecked && g_mouseEnabled)
    {
        // This is a horrible crutch
        if (inputState.mouseReadButtons() & WHEELUP_MOUSE)
        {
            event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELUP };
            D_PostEvent(&ev);
        }
        if (inputState.mouseReadButtons() & WHEELDOWN_MOUSE)
        {
            event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELDOWN };
            D_PostEvent(&ev);
        }
        if (inputState.mouseReadButtons() & WHEELLEFT_MOUSE)
        {
            event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELLEFT };
            D_PostEvent(&ev);
        }
        if (inputState.mouseReadButtons() & WHEELRIGHT_MOUSE)
        {
            event_t ev = { EV_KeyUp, 0, (int16_t)KEY_MWHEELRIGHT };
            D_PostEvent(&ev);
        }
    }

    rv = handleevents_pollsdl();

    inputchecked = 0;
    timerUpdateClock();
    //I_ProcessJoysticks();
    return rv;
}


int32_t handleevents_peekkeys(void)
{
    SDL_PumpEvents();

    return SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);
}

void I_SetMouseCapture()
{
    // Clear out any mouse movement.
    SDL_CaptureMouse(SDL_TRUE);
}

void I_ReleaseMouseCapture()
{
    SDL_CaptureMouse(SDL_FALSE);
}
