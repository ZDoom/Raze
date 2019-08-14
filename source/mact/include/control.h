//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

//***************************************************************************
//
// Public header for CONTROL.C.
//
//***************************************************************************

#pragma once

#ifndef control_public_h_
#define control_public_h_
#ifdef __cplusplus
extern "C" {
#endif


//***************************************************************************
//
// DEFINES
//
//***************************************************************************

#define MAXGAMEBUTTONS      64

#define BUTTON(x) ((CONTROL_ButtonState >> ((uint64_t)(x))) & 1)
#define BUTTONHELD(x) ((CONTROL_ButtonHeldState >> ((uint64_t)(x))) & 1)

#define BUTTONJUSTPRESSED(x) (BUTTON(x) && !BUTTONHELD(x))
#define BUTTONRELEASED(x) (!BUTTON(x) && BUTTONHELD(x))
#define BUTTONSTATECHANGED(x) (BUTTON(x) != BUTTONHELD(x))


//***************************************************************************
//
// TYPEDEFS
//
//***************************************************************************
typedef enum
   {
   axis_up,
   axis_down,
   axis_left,
   axis_right
   } axisdirection;

typedef enum
   {
   analog_turning=0,
   analog_strafing=1,
   analog_lookingupanddown=2,
   analog_elevation=3,
   analog_rolling=4,
   analog_moving=5,
   analog_maxtype
   } analogcontrol;

typedef enum
   {
   dir_North,
   dir_NorthEast,
   dir_East,
   dir_SouthEast,
   dir_South,
   dir_SouthWest,
   dir_West,
   dir_NorthWest,
   dir_None
   } direction;

typedef struct
   {
   int32_t   button0;
   int32_t   button1;
   direction dir;
   } UserInput;

typedef struct
   {
   int32_t     dx;
   int32_t     dy;
   int32_t     dz;
   int32_t     dyaw;
   int32_t     dpitch;
   int32_t     droll;
   int32_t     mousex;
   int32_t     mousey;
   } ControlInfo;

typedef enum
   {
   controltype_keyboard,
   controltype_keyboardandmouse,
   controltype_keyboardandjoystick
   } controltype;

typedef enum
   {
   controldevice_keyboard,
   controldevice_mouse,
   controldevice_joystick
   } controldevice;

enum GameControllerButton : int
{
    GAMECONTROLLER_BUTTON_INVALID = -1,
    GAMECONTROLLER_BUTTON_A,
    GAMECONTROLLER_BUTTON_B,
    GAMECONTROLLER_BUTTON_X,
    GAMECONTROLLER_BUTTON_Y,
    GAMECONTROLLER_BUTTON_BACK,
    GAMECONTROLLER_BUTTON_GUIDE,
    GAMECONTROLLER_BUTTON_START,
    GAMECONTROLLER_BUTTON_LEFTSTICK,
    GAMECONTROLLER_BUTTON_RIGHTSTICK,
    GAMECONTROLLER_BUTTON_LEFTSHOULDER,
    GAMECONTROLLER_BUTTON_RIGHTSHOULDER,
    GAMECONTROLLER_BUTTON_DPAD_UP,
    GAMECONTROLLER_BUTTON_DPAD_DOWN,
    GAMECONTROLLER_BUTTON_DPAD_LEFT,
    GAMECONTROLLER_BUTTON_DPAD_RIGHT,
    GAMECONTROLLER_BUTTON_MAX
};

enum GameControllerAxis : int
{
    GAMECONTROLLER_AXIS_INVALID = -1,
    GAMECONTROLLER_AXIS_LEFTX,
    GAMECONTROLLER_AXIS_LEFTY,
    GAMECONTROLLER_AXIS_RIGHTX,
    GAMECONTROLLER_AXIS_RIGHTY,
    GAMECONTROLLER_AXIS_TRIGGERLEFT,
    GAMECONTROLLER_AXIS_TRIGGERRIGHT,
    GAMECONTROLLER_AXIS_MAX
};

enum class LastSeenInput : unsigned char
{
    Keyboard,
    Joystick,
};

//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

extern bool CONTROL_Started;
extern bool CONTROL_MousePresent;
extern bool CONTROL_JoyPresent;
extern bool CONTROL_MouseEnabled;
extern bool CONTROL_JoystickEnabled;

extern uint64_t CONTROL_ButtonState;
extern uint64_t CONTROL_ButtonHeldState;

extern LastSeenInput CONTROL_LastSeenInput;


//***************************************************************************
//
// PROTOTYPES
//
//***************************************************************************

//void CONTROL_MapKey( int32_t which, kb_scancode key1, kb_scancode key2 );
void CONTROL_MapButton(int whichfunction, int whichbutton, int doubleclicked, controldevice device);
void CONTROL_DefineFlag( int which, int toggle );
int CONTROL_FlagActive( int which );
void CONTROL_ClearAssignments( void );
// void CONTROL_GetFunctionInput( void );
void CONTROL_GetInput( ControlInfo *info );
void CONTROL_ClearButton( int whichbutton );
void CONTROL_ClearAllButtons( void );
extern float CONTROL_MouseSensitivity;
bool CONTROL_Startup(controltype which, int32_t ( *TimeFunction )( void ), int32_t ticspersecond);
void CONTROL_Shutdown( void );

void CONTROL_MapAnalogAxis(int whichaxis, int whichanalog, controldevice device);
void CONTROL_MapDigitalAxis(int32_t whichaxis, int32_t whichfunction, int32_t direction, controldevice device);
void CONTROL_SetAnalogAxisScale(int32_t whichaxis, int32_t axisscale, controldevice device);
void CONTROL_SetAnalogAxisInvert(int32_t whichaxis, int32_t invert, controldevice device);

void CONTROL_ScanForControllers(void);

int32_t CONTROL_GetGameControllerDigitalAxisPos(int32_t axis);
int32_t CONTROL_GetGameControllerDigitalAxisNeg(int32_t axis);
void CONTROL_ClearGameControllerDigitalAxisPos(int32_t axis);
void CONTROL_ClearGameControllerDigitalAxisNeg(int32_t axis);

//void CONTROL_PrintKeyMap(void);
//void CONTROL_PrintControlFlag(int32_t which);
//void CONTROL_PrintAxes( void );


////////// KEY/MOUSE BIND STUFF //////////

#define MAXBOUNDKEYS MAXKEYBOARDSCAN
#define MAXMOUSEBUTTONS 10

typedef struct
{
    const char *key;
    char *cmdstr;
    char repeat;
    char laststate;
}
consolekeybind_t;

// Direct use DEPRECATED:
extern consolekeybind_t CONTROL_KeyBinds[MAXBOUNDKEYS+MAXMOUSEBUTTONS];
extern bool CONTROL_BindsEnabled;

void CONTROL_ClearAllBinds(void);
void CONTROL_BindKey(int i, char const * cmd, int repeat, char const * keyname);
void CONTROL_BindMouse(int i, char const * cmd, int repeat, char const * keyname);
void CONTROL_FreeKeyBind(int i);
void CONTROL_FreeMouseBind(int i);

static inline int CONTROL_KeyIsBound(int const key)
{
    auto &bind = CONTROL_KeyBinds[key];
    return bind.cmdstr && bind.key;
}

void CONTROL_ProcessBinds(void);

#define CONTROL_GetUserInput(...)
#define CONTROL_ClearUserInput(...)

////////////////////

#define CONTROL_NUM_FLAGS   64
extern int32_t CONTROL_ButtonFlags[CONTROL_NUM_FLAGS];
extern bool CONTROL_SmoothMouse;

#ifdef __cplusplus
}
#endif
#endif
