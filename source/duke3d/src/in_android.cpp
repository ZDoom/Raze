//-------------------------------------------------------------------------
/*
Copyright (C) 2015 EDuke32 developers and contributors
Copyright (C) 2015 Voidpoint, LLC

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "sdl_inc.h"
#include "baselayer.h"
#include "keys.h"
#include "duke3d.h"
#include "common_game.h"
#include "osd.h"
#include "player.h"
#include "game.h"
#include "build.h"
#include "anim.h"
#include "player.h"

#include "keyboard.h"
#include "control.h"
#include "_control.h"

#include "menus.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
extern int SDL_SendKeyboardText(const char *text);
extern int SDL_SendMouseMotion(SDL_Window * window, Uint32 mouseID, int relative, int x, int y);
extern int SDL_SendMouseButton(SDL_Window * window, Uint32 mouseID, Uint8 state, Uint8 button);

#ifdef __cplusplus
}
#endif

#include "in_android.h"
#include <android/log.h>

#if defined __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"DUKE", __VA_ARGS__))

static char sdl_text[2];

droidinput_t droidinput;

void AndroidTimer(int tics) { G_InitTimer(tics);  }

int AndroidKeyEvent(int state, int code,int unicode)
{
    LOGI("AndroidKeyEvent %d %d %d",state,(SDL_Scancode)code,unicode);
    SDL_SendKeyboardKey(state ? SDL_PRESSED : SDL_RELEASED, (SDL_Scancode)code);
    SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);

    // if (code == 42)
    //    unicode = 42;

    if (state)
    {
        //if (unicode < 128)
        {
            sdl_text[0] = unicode;
            sdl_text[1] = 0;

            int posted = SDL_SendKeyboardText((const char*)sdl_text);
            LOGI("posted = %d",posted);
        }

        if (state == 2)
            AndroidKeyEvent(0, code, unicode);
    }

    return 0;
}

void AndroidMouseMenu(float x,float y)
{
    SDL_SendMouseMotion(NULL,0,0,x,y);
}

void AndroidMouseMenuButton(int state,int button)
{
    SDL_SendMouseButton(NULL, SDL_TOUCH_MOUSEID, state?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_LEFT);
}

void changeActionState(int state, int action)
{
    if (state)
    {
        //BUTTONSET(action,1);
        droidinput.functionSticky  |= ((uint64_t)1<<((uint64_t)(action)));
        droidinput.functionHeld    |= ((uint64_t)1<<((uint64_t)(action)));
        return;
    }

    //BUTTONCLEAR(action);
    droidinput.functionHeld  &= ~((uint64_t) 1<<((uint64_t) (action)));
}

void AndroidAction(int state, int action)
{
    LOGI("AndroidAction action = %d, state = %d", action, state);

    if (action >= MENU_UP && action <= MENU_BACK)
    {
        int const sdl_code [] = { SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
        AndroidKeyEvent(state, sdl_code[action-MENU_UP], 0);
        return;
    }
    else
    {
        //if (AndroidRead(READ_SCREEN_MODE) != TOUCH_SCREEN_GAME) //If not in game don't do any of this
        ///     return;

        //Special toggle for crouch, NOT when using jetpack or in water
        if (droidinput.toggleCrouch)
        {
            int lotag = sector[g_player[myconnectindex].ps->cursectnum].lotag;

            if (droidinput.crouchToggleState && (lotag == ST_2_UNDERWATER || lotag == ST_1_ABOVE_WATER))
            {
                droidinput.crouchToggleState = 0;
                if (action == gamefunc_Crouch)
                    state = 0;
                else AndroidAction(0, gamefunc_Crouch);
            }

            if (action == gamefunc_Crouch)
            {
                if (!g_player[myconnectindex].ps->jetpack_on && g_player[myconnectindex].ps->on_ground &&
                    lotag != ST_2_UNDERWATER && lotag != ST_1_ABOVE_WATER)
                {
                    if (state)
                        droidinput.crouchToggleState = !droidinput.crouchToggleState;

                    state = droidinput.crouchToggleState;
                }
            }

        }

        //Check if jumping while crouched
        if (action == gamefunc_Jump)
        {
            if (droidinput.crouchToggleState)
            {
                droidinput.crouchToggleState = 0;
                changeActionState(0, gamefunc_Crouch);
            }
            else
                changeActionState(state, action);
        }
        else
            changeActionState(state, action);

        if (state == 2)
            AndroidAction(0, action);

        // LOGI("AndroidAction state = 0x%016llX", CONTROL_ButtonState);
    }
}

int const deadRegion = 0.3;

float analogCalibrate(float v)
{
    float rv = 0;

    if (v > deadRegion) rv = (v - deadRegion) * (v - deadRegion);
    else if (-v > deadRegion) rv = -(-v - deadRegion) * (-v - deadRegion);

    return rv;
}

//Need these NAN check as not cumulative.
void AndroidMove(float fwd, float strafe)
{
    if (!ud.auto_run)
    {
        fwd *= 0.5f;
        strafe *= 0.5f;
    }

    if (!isnan(fwd))
        droidinput.forwardmove = fclamp2(analogCalibrate(fwd), -1.f, 1.f);

    if (!isnan(strafe))
        droidinput.sidemove    = fclamp2(analogCalibrate(strafe),    -1.f, 1.f);
}

void AndroidLook(float yaw, float pitch)
{
    droidinput.pitch += pitch;
    droidinput.yaw   += yaw;
}

void AndroidLookJoystick(float yaw, float pitch)
{
    if (!isnan(pitch))
        droidinput.pitch_joystick = analogCalibrate(pitch);

    if (!isnan(yaw))
        droidinput.yaw_joystick   = analogCalibrate(yaw);
}

void AndroidOSD(const char * cmd)
{
    OSD_Dispatch(cmd);
}

int consoleShown = 0;
void AndroidSetConsoleShown(int onf)
{
    consoleShown = onf;
}

extern int inExtraScreens; //In game.c
int32_t AndroidRead(portableread_t r)
{
    int32_t rv;

    switch (r)
    {
    case R_TOUCH_MODE:
        if (g_animPtr || inExtraScreens)
            rv = TOUCH_SCREEN_BLANK_TAP;
        else if (consoleShown)
            rv = TOUCH_SCREEN_CONSOLE;
        else if ((g_player[myconnectindex].ps->gm & MODE_MENU) == MODE_MENU && g_currentMenu != MENU_MAIN)
            rv = (m_currentMenu->type == Verify && totalclock > (m_animation.length + m_animation.start)) ? TOUCH_SCREEN_YES_NO : TOUCH_SCREEN_MENU;
        else if ((g_player[myconnectindex].ps->gm & MODE_MENU) == MODE_MENU && g_currentMenu == MENU_MAIN)
            rv = TOUCH_SCREEN_MENU_NOBACK;
/*
        else if (ud.overhead_on == 2)
            rv = TOUCH_SCREEN_AUTOMAP;
*/
        else if ((g_player[myconnectindex].ps->gm & MODE_GAME))
            if (AndroidRead(R_PLAYER_DEAD_FLAG))
                rv = TOUCH_SCREEN_BLANK_TAP;
            else
                rv = TOUCH_SCREEN_GAME;
        else
            rv = TOUCH_SCREEN_BLANK_TAP;
         break;
    case R_PLAYER_GOTWEAPON:
        rv = g_player[myconnectindex].ps->gotweapon; break;
    case R_UD_OVERHEAD_ON:
        rv = 0; break;//ud.overhead_on != 0;  break;// ud.overhead_on ranges from 0-2
    case R_UD_SCROLLMODE:
        rv = ud.scrollmode; break;
    case R_PLAYER_LASTWEAPON:
        rv = droidinput.lastWeapon;
        if ((unsigned)rv < MAX_WEAPONS && !g_player[myconnectindex].ps->ammo_amount[rv])
            rv = -1;
        break;
    case R_GAME_PAUSED:
        rv = ud.pause_on != 0; break;
    case R_PLAYER_DEAD_FLAG:
        rv = g_player[myconnectindex].ps->dead_flag; break;
    case R_PLAYER_INV_AMOUNT:
        rv = 0;
        for (bssize_t i = 0; i < GET_MAX; i++)
        {
            if (g_player[myconnectindex].ps->inv_amount[i])
                rv += (1 << i);
        }
        break;
    case R_SOMETHINGONPLAYER:
        rv = g_player[myconnectindex].ps->somethingonplayer != -1;
        break;
    default:
        rv = 0; break;
    }

    return rv;
}

static float map_zoom,map_dx,map_dy = 0;

void AndroidAutomapControl(float zoom,float dx,float dy)
{
    map_zoom += zoom;
    map_dx += dx;
    map_dy += dy;
}

///This stuff is called from the game/engine

extern void  CONTROL_Android_ScrollMap(int32_t *angle,int32_t *x, int32_t *y, uint16_t *zoom )
{

    *x += ((int)(map_dx * -30000)*sintable[(512+2048-*angle)&2047])>>14;
    *y += ((int)(map_dy * -30000)*sintable[(512+1024-512-*angle)&2047])>>14;

//    *zoom += map_zoom * 2000;
    //*angle = 0;
    map_dx = map_dy = map_zoom = 0;
}

void CONTROL_Android_SetLastWeapon(int w)
{
    droidinput.lastWeapon = w;
}

void CONTROL_Android_ClearButton(int32_t whichbutton)
{
    droidinput.functionHeld  &= ~((uint64_t)1<<((uint64_t)(whichbutton)));
}

void CONTROL_Android_PollDevices(ControlInfo *info)
{
    //LOGI("CONTROL_Android_PollDevices %f %f",forwardmove,sidemove);
    //LOGI("CONTROL_Android_PollDevices %f %f",droidinput.pitch,droidinput.yaw);

    info->dz = (int32_t)nearbyintf(-droidinput.forwardmove * ANDROIDMOVEFACTOR);
    info->dx = (int32_t)nearbyintf(droidinput.sidemove * ANDROIDMOVEFACTOR) >> 5;
    info->dpitch =
    (int32_t)nearbyintf(droidinput.pitch * ANDROIDLOOKFACTOR + droidinput.pitch_joystick * ANDROIDPITCHFACTORJOYSTICK);
    info->dyaw =
    (int32_t)nearbyintf(-droidinput.yaw * ANDROIDLOOKFACTOR - droidinput.yaw_joystick * ANDROIDYAWFACTORJOYSTICK);

    droidinput.pitch = droidinput.yaw = 0.f;
    CONTROL_ButtonState = droidinput.functionSticky | droidinput.functionHeld;
    droidinput.functionSticky = 0;

    //LOGI("poll state = 0x%016llX",CONTROL_ButtonState);
}

#if defined __GNUC__
# pragma GCC diagnostic pop
#endif
