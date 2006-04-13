//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is NOT part of Duke Nukem 3D version 1.5 - Atomic Edition
However, it is either an older version of a file that is, or is
some test code written during the development of Duke Nukem 3D.
This file is provided purely for educational interest.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

//****************************************************************************
//
// Private header for CONTROL.C
//
//****************************************************************************

#ifndef _control_private
#define _control_private
#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
//
// DEFINES
//
//****************************************************************************

#define BUTTON0_SCAN_1   sc_Space
#define BUTTON0_SCAN_2   sc_Enter
#define BUTTON0_SCAN_3   sc_kpad_Enter
#define BUTTON1_SCAN     sc_Escape

#define AXISUNDEFINED   0x7f
#define BUTTONUNDEFINED 0x7f
#define KEYUNDEFINED    0x7f

#define SENSITIVE        0x400
//#define MINSENSITIVE     0x30

#define THRESHOLD        0x200
#define MINTHRESHOLD     0x80

#define USERINPUTDELAY   200

#define ResetMouse       0
#define GetMouseButtons  3
#define GetMouseDelta    11

#define MouseInt         0x33
#define JoyMax           0xa00
#define MaxJoyValue      5000
#define MINIMUMMOUSESENSITIVITY 0x1000
#define DEFAULTMOUSESENSITIVITY 0x7000+MINIMUMMOUSESENSITIVITY

#define CONTROL_NUM_FLAGS   64
#define INSTANT_ONOFF       0
#define TOGGLE_ONOFF        1

#define MAXCONTROLVALUE  0x7fff

// Maximum number of buttons for any controller
#define MAXBUTTONS 32

// Number of Mouse buttons

#define MAXMOUSEBUTTONS 6

// Number of Mouse Axes

#define MAXMOUSEAXES 2

// Number of JOY buttons

#define MAXJOYBUTTONS 32

// Number of JOY axes

#define MAXJOYAXES 6

// Number of GamePad axes

#define MAXGAMEPADAXES 2

// Number of axes

#define MAXAXES 6

// NORMAL axis scale

#define NORMALAXISSCALE (65536)

#define BUTTONSET(x,value) \
    (\
    ((x)>31) ?\
    (CONTROL_ButtonState2 |= (value<<((x)-32)))  :\
    (CONTROL_ButtonState1 |= (value<<(x)))\
    )

#define BUTTONCLEAR(x) \
    (\
    ((x)>31) ?\
    (CONTROL_ButtonState2 &= (~(1<<((x)-32)))) :\
    (CONTROL_ButtonState1 &= (~(1<<(x))))\
    )

#define BUTTONHELDSET(x,value)\
    (\
    ((x)>31) ?\
    (CONTROL_ButtonHeldState2 |= value<<((x)-32)) :\
    (CONTROL_ButtonHeldState1 |= value<<(x))\
    )

#define LIMITCONTROL(x)\
    {\
    if ((*x)>MAXCONTROLVALUE) \
       {\
       (*x) = MAXCONTROLVALUE;\
       }\
    if ((*x)<-MAXCONTROLVALUE) \
       {\
       (*x) = -MAXCONTROLVALUE;\
       }\
    }
#define SGN(x) \
    ( ( (x) > 0 ) ? 1 : ( (x) < 0 ) ? -1 : 0 )

//****************************************************************************
//
// TYPEDEFS
//
//****************************************************************************

typedef enum
   {
   motion_Left  = -1,
   motion_Up    = -1,
   motion_None  = 0,
   motion_Right = 1,
   motion_Down  = 1
   } motion;


typedef struct
   {
   int32 joyMinX;
   int32 joyMinY;
   int32 threshMinX;
   int32 threshMinY;
   int32 threshMaxX;
   int32 threshMaxY;
   int32 joyMaxX;
   int32 joyMaxY;
   int32 joyMultXL;
   int32 joyMultYL;
   int32 joyMultXH;
   int32 joyMultYH;
   } JoystickDef;

//   int32 ThrottleMin;
//   int32 RudderMin;
//   int32 ThrottlethreshMin;
//   int32 RudderthreshMin;
//   int32 ThrottlethreshMax;
//   int32 RudderthreshMax;
//   int32 ThrottleMax;
//   int32 RudderMax;
//   int32 ThrottleMultL;
//   int32 RudderMultL;
//   int32 ThrottleMultH;
//   int32 RudderMultH;

/*
typedef struct
   {
   byte active     : 1;
   byte used       : 1;
   byte toggle     : 1;
   byte buttonheld : 1;
   byte cleared    : 1;
   } controlflags;
typedef struct
   {
   volatile byte active     : 1;
   volatile byte used       : 1;
   volatile byte toggle     : 1;
   volatile byte buttonheld : 1;
   volatile byte cleared    : 1;
   } controlflags;
*/
typedef struct
   {
   byte active     ;
   byte used       ;
   byte toggle     ;
   byte buttonheld ;
   int32 cleared    ;
   } controlflags;

typedef struct
   {
   kb_scancode key1;
   kb_scancode key2;
   } controlkeymaptype;

typedef struct
   {
   byte singleclicked;
   byte doubleclicked;
   word extra;
   } controlbuttontype;

typedef struct
   {
   byte analogmap;
   byte minmap;
   byte maxmap;
   byte extra;
   } controlaxismaptype;

typedef struct
   {
   int32 analog;
   int32 digital;
   } controlaxistype;


//***************************************************************************
//
// PROTOTYPES
//
//***************************************************************************

void    CONTROL_GetMouseDelta( void );
byte    CONTROL_GetMouseButtons( void );
boolean CONTROL_StartMouse( void );
void    CONTROL_GetJoyAbs( void );
void    CONTROL_GetJoyDelta( void );
void    CONTROL_SetJoyScale( void );
boolean CONTROL_StartJoy( int32 joy );
void    CONTROL_ShutJoy( int32 joy );
void    CONTROL_SetFlag( int32 which, boolean active );
void    CONTROL_ButtonFunctionState( boolean * state );
boolean CONTROL_KeyboardFunctionPressed( int32 whichfunction );
boolean CONTROL_CheckRange( int32 which );
int32   CONTROL_GetTime( void );
void    CONTROL_AxisFunctionState( boolean * state );
void    CONTROL_GetJoyMovement( ControlInfo * info );

#ifdef __cplusplus
};
#endif
#endif
