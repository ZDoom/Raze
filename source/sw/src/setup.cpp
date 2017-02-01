//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
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

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "build.h"

#include "keys.h"
#include "game.h"

#include "mytypes.h"
#include "fx_man.h"
#include "music.h"
#include "scriplib.h"
#include "file_lib.h"
#include "gamedefs.h"
#include "keyboard.h"

#include "control.h"
#include "config.h"
#include "sounds.h"
#include "function.h"

#include "rts.h"

/*
===================

=
= SetupGameButtons
=
===================
*/

void SetupGameButtons(void)
{
    // processes array from function.h - char * gamefunctions[];

    short gamefunc;

    for (gamefunc = 0; gamefunc < NUMGAMEFUNCTIONS; gamefunc++)
    {
        CONTROL_DefineFlag(gamefunc,FALSE);
    }
}

void CenterCenter(void)
{
    printf("\nCenter the joystick and press a button\n");
}

void UpperLeft(void)
{
    printf("Move joystick to upper-left corner and press a button\n");
}

void LowerRight(void)
{
    printf("Move joystick to lower-right corner and press a button\n");
}

void CenterThrottle(void)
{
    printf("Center the throttle control and press a button\n");
}

void CenterRudder(void)
{
    printf("Center the rudder control and press a button\n");
}

/*
===================
=
= GetTime
=
===================
*/

static int32_t timert;

int32_t GetTime(void)
{
    return totalclock;
    //return timert++;
}

void InitSetup(void)
{
    int i;
    //RegisterShutdownFunction( ShutDown );

    //StartWindows();
    //initkeys();
    //CONFIG_GetSetupFilename();
    //InitializeKeyDefList();
    //CONFIG_ReadSetup();
    if (CONTROL_Startup(1, &GetTime, /*120*/ 140)) exit(1);
    SetupGameButtons();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (UseJoystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled = (UseMouse && CONTROL_MousePresent);

    /*{
    int i;
    CONTROL_PrintKeyMap();
    for(i=0;i<NUMGAMEFUNCTIONS;i++) CONTROL_PrintControlFlag(i);
    CONTROL_PrintAxes();
    }*/

    RTS_Init(RTSName);
}

#if 0
void TermSetup(void)
{
    //FreeKeyDefList();
}
#endif

// BELOW IS FROM A TEST SETUP BY MARK DOC
//******************************************************************************
//******************************************************************************
//******************************************************************************
//******************************************************************************

#if 0
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "sndcards.h"
#include "fx_man.h"
#include "music.h"
#include "scriplib.h"
#include "file_lib.h"
#include "gamedefs.h"
#include "keyboard.h"

#include "control.h"
#include "config.h"
#include "sounds.h"
#include "function.h"
#include "rts.h"
#include "timer.h"

int32_t timerhandle=0;
volatile int32_t timer;
/*
===================
=
= Shutdown
=
===================
*/

void ShutDown(void)
{
    KB_Shutdown();
    TIME_RemoveTimer(timerhandle);
    SoundShutdown();
    MusicShutdown();
    CONFIG_WriteSetup();
}

/*
===================

=
= SetupGameButtons
=
===================
*/

void SetupGameButtons(void)
{
    CONTROL_DefineFlag(gamefunc_Move_Forward,      FALSE);
    CONTROL_DefineFlag(gamefunc_Move_Backward,     FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Left,         FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Right,        FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe,            FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Left,       FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Right,      FALSE);
    CONTROL_DefineFlag(gamefunc_TurnAround,        FALSE);
    CONTROL_DefineFlag(gamefunc_Jump,              FALSE);
    CONTROL_DefineFlag(gamefunc_Crouch,            FALSE);
    CONTROL_DefineFlag(gamefunc_Fire,              FALSE);
    CONTROL_DefineFlag(gamefunc_Open,              FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Up,           FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Down,         FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Up,            FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Down,          FALSE);
    CONTROL_DefineFlag(gamefunc_Run,               FALSE);
    CONTROL_DefineFlag(gamefunc_SendMessage,       FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_1,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_2,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_3,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_4,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_5,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_6,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_7,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_8,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_9,          FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_10,         FALSE);
    CONTROL_DefineFlag(gamefunc_Map,               FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Left,         FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Right,        FALSE);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen,     FALSE);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen,    FALSE);
    CONTROL_DefineFlag(gamefunc_AutoRun,           FALSE);
    CONTROL_DefineFlag(gamefunc_Center_View,       FALSE);
    CONTROL_DefineFlag(gamefunc_Holster_Weapon,    FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Left,    FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Right,   FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory,         FALSE);

}

/*
===================
=
= GetTime
=
===================
*/

int32_t GetTime(void)
{
    return timer;
}

/*
===================
=
= CenterCenter
=
===================
*/

void CenterCenter(void)
{
    printf("Center the joystick and press a button\n");
}

/*
===================
=
= UpperLeft
=
===================
*/

void UpperLeft(void)
{
    printf("Move joystick to upper-left corner and press a button\n");
}

/*
===================
=
= LowerRight
=
===================
*/

void LowerRight(void)
{
    printf("Move joystick to lower-right corner and press a button\n");
}

/*
===================
=
= CenterThrottle
=
===================
*/

void CenterThrottle(void)
{
    printf("Center the throttle control and press a button\n");
}

/*
===================
=
= CenterRudder
=
===================
*/

void CenterRudder(void)
{
    printf("Center the rudder control and press a button\n");
}

void main()
{
    char *song;
    char *voc;
    volatile int32_t lasttime;

    RegisterShutdownFunction(ShutDown);
    KB_Startup();
    timerhandle = TIME_AddTimer(40, &timer);
    //CONFIG_GetSetupFilename();
    CONFIG_ReadSetup();
    CONTROL_Startup(1, &GetTime, 1500);
    SetupGameButtons();

    MusicStartup();
    SoundStartup();
    RTS_Init(RTSName);

    // load in some test data

    LoadFile("test.mid",&song);
    LoadFile("test.voc",&voc);

    // start playing a song

    MUSIC_PlaySong(song, MUSIC_LoopSong);


    lasttime = timer;
    while (1)
    {
        int32_t i;
        ControlInfo info;

        while (lasttime==timer)
        {
            ServiceEvents();
        }
        lasttime = timer;
//      printf("timer=%ld\n",timer);
        CONTROL_GetInput(&info);

        if (
            info.dx!=0 ||
            info.dy!=0 ||
            info.dz!=0 ||
            info.dpitch!=0 ||
            info.dyaw!=0 ||
            info.droll!=0
            )
            printf("x=%6ld y=%6ld z=%6ld yaw=%6ld pitch=%6ld roll=%6ld\n",
                   info.dx,info.dy,info.dz,info.dyaw,info.dpitch,info.droll);
        // Get Keyboard input and set appropiate game function states
        for (i=0; i<NUMGAMEFUNCTIONS; i++)
        {
            if (BUTTON(i) && !BUTTONHELD(i))
            {
                printf("%s\n",gamefunctions[i]);
            }
        }
        for (i=0; i<10; i++)
        {
            if (KB_KeyPressed(sc_F1+i))
            {
                uint8_t *ptr;
                KB_ClearKeyDown(sc_F1+i);
                ptr = (uint8_t *)RTS_GetSound(i);
                FX_PlayVOC(ptr, 0, 255, 255, 255, 255, 0);
            }
        }
        // Check to see if fire is being pressed so we can play a sound
        if (BUTTON(gamefunc_Fire) && !BUTTONHELD(gamefunc_Fire))
        {
            FX_PlayVOC(voc, 0, 255, 255, 255, 255, 0);
        }

        // Check to see if we want to exit
        if (KB_KeyPressed(sc_Escape))
        {
            break;
        }

    }

    ShutDown();
}
#endif
