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

#define BUTTON(x) ((CONTROL_ButtonState>> ((uint64_t)(x)) ) & 1)
#define BUTTONHELD(x) ((CONTROL_ButtonHeldState>> ((uint64_t)(x)) ) & 1)

#define BUTTONJUSTPRESSED(x) \
    ( BUTTON( x ) && !BUTTONHELD( x ) )
#define BUTTONRELEASED(x) \
    ( !BUTTON( x ) && BUTTONHELD( x ) )
#define BUTTONSTATECHANGED(x) \
    ( BUTTON( x ) != BUTTONHELD( x ) )


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


//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

extern int32_t  CONTROL_Started;
extern int32_t  CONTROL_MousePresent;
extern int32_t  CONTROL_JoyPresent;
extern int32_t  CONTROL_MouseEnabled;
extern int32_t  CONTROL_JoystickEnabled;
extern uint64_t   CONTROL_ButtonState;
extern uint64_t   CONTROL_ButtonHeldState;


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
void CONTROL_ClearButton( int32_t whichbutton );
extern float CONTROL_MouseSensitivity;
int32_t CONTROL_Startup(controltype which, int32_t ( *TimeFunction )( void ), int32_t ticspersecond);
void CONTROL_Shutdown( void );

void CONTROL_SetDoubleClickDelay(int32_t delay);
int32_t CONTROL_GetDoubleClickDelay(void);

void CONTROL_MapAnalogAxis(int whichaxis, int whichanalog, controldevice device);
void CONTROL_MapDigitalAxis(int32_t whichaxis, int32_t whichfunction, int32_t direction, controldevice device);
void CONTROL_SetAnalogAxisScale(int32_t whichaxis, int32_t axisscale, controldevice device);

//void CONTROL_PrintKeyMap(void);
//void CONTROL_PrintControlFlag(int32_t which);
//void CONTROL_PrintAxes( void );


////////// KEY/MOUSE BIND STUFF //////////

#define MAXBOUNDKEYS MAXKEYBOARDSCAN
#define MAXMOUSEBUTTONS 10

typedef struct binding {
    const char *key;  // always set to const char *
    char *cmdstr;  // alloc'd
    char repeat;
    char laststate;
} keybind;

// Direct use DEPRECATED:
extern keybind CONTROL_KeyBinds[MAXBOUNDKEYS+MAXMOUSEBUTTONS];
extern int32_t CONTROL_BindsEnabled;

void CONTROL_ClearAllBinds(void);
void CONTROL_BindKey(int i, char const * const cmd, int repeat, char const * const keyname);
void CONTROL_BindMouse(int i, char const * const cmd, int repeat, char const * const keyname);
void CONTROL_FreeKeyBind(int i);
void CONTROL_FreeMouseBind(int i);

static inline int32_t CONTROL_KeyIsBound(int32_t i)
{
    return (CONTROL_KeyBinds[i].cmdstr && CONTROL_KeyBinds[i].key);
}

void CONTROL_ProcessBinds(void);

////////////////////

#define CONTROL_NUM_FLAGS   64
extern int32_t CONTROL_OSDInput[CONTROL_NUM_FLAGS];
extern int32_t CONTROL_SmoothMouse;

#ifdef __cplusplus
}
#endif
#endif
