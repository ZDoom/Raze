#include "sdl_inc.h"
#include "baselayer.h"
#include "keys.h"
#include "duke3d.h"
#include "common_game.h"
#include "osd.h"
#include "player.h"
#include "game.h"
#include "build.h"

#include "jmact/keyboard.h"
#include "jmact/control.h"

#include "../src/video/android/SDL_androidkeyboard.h" // FIXME: include header locally if necessary

#include "in_android.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"DUKE", __VA_ARGS__))

extern int32_t main(int32_t argc, char *argv []);

extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
extern int SDL_SendKeyboardText(const char *text);

static char sdl_text[2];

droidinput_t droidinput;

int PortableKeyEvent(int state, int code,int unicode)
{
    LOGI("PortableKeyEvent %d %d %d",state,code,unicode);

    SDL_SendKeyboardKey(state ? SDL_PRESSED : SDL_RELEASED, code);
    SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);

    if (code == 42)
        unicode = 42;

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
            PortableKeyEvent(0, code, unicode);
    }

    return 0;
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

#ifdef GP_LIC
#define GP_LIC_INC 4
#include "s-setup/gp_lic_include.h"
#endif

void PortableAction(int state, int action)
{
    LOGI("PortableAction action = %d, state = %d", action, state);

    if (action >= MENU_UP && action <= MENU_BACK)
    {
        if (PortableRead(READ_MENU))
        {
            int sdl_code [] = { SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                    SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
            PortableKeyEvent(state, sdl_code[action-MENU_UP], 0);
            return;
        }
    }
    else
    {
        if (PortableRead(READ_MENU)) //If in the menu, dont do any game actions
            return;

#ifdef GP_LIC
#define GP_LIC_INC 5
#include "s-setup/gp_lic_include.h"
#endif

        //Special toggle for crouch, NOT when using jetpack or in water
        if (!g_player[myconnectindex].ps->jetpack_on &&
                g_player[myconnectindex].ps->on_ground &&
                (sector[g_player[myconnectindex].ps->cursectnum].lotag != ST_2_UNDERWATER))
        {
            if (toggleCrouch)
            {
                if (action == gamefunc_Crouch)
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
            droidinput.crouchToggleState = 0;
            changeActionState(0, gamefunc_Crouch);
        }

        changeActionState(state, action);

        if (state == 2)
            PortableAction(0,action);

        // LOGI("PortableAction state = 0x%016llX", CONTROL_ButtonState);
    }
}

//Need these NAN check as not cumulative.
void PortableMove(float fwd, float strafe)
{
    if (!isnan(fwd))
        droidinput.forwardmove = fclamp2(fwd, -1.f, 1.f);

    if (!isnan(strafe))
        droidinput.sidemove    = fclamp2(strafe,    -1.f, 1.f);
}

void PortableLook(double yaw, double pitch)
{
    // LOGI("PortableLookPitch %f %f",yaw, pitch);
    droidinput.pitch += pitch;
    droidinput.yaw   += yaw;
}

void PortableLookJoystick(double yaw, double pitch)
{
    if (!isnan(pitch))
        droidinput.pitch_joystick = pitch;

    if (!isnan(yaw))
        droidinput.yaw_joystick   = yaw;
}

void PortableCommand(const char * cmd)
{
    OSD_Dispatch(cmd);
}

void PortableInit(int argc, const char ** argv)
{
    droidinput.left_double_action = -1;
    droidinput.right_double_action = -1;

    main(argc, argv);
}

int32_t PortableRead(portableread_t r)
{
    switch (r)
    {
    case READ_MENU:
        return (g_player[myconnectindex].ps->gm & MODE_MENU) == MODE_MENU || (g_player[myconnectindex].ps->gm & MODE_GAME) != MODE_GAME;
    case READ_WEAPONS:
        return g_player[myconnectindex].ps->gotweapon;
    case READ_AUTOMAP:
        return ud.overhead_on != 0; // ud.overhead_on ranges from 0-2
    case READ_MAPFOLLOWMODE:
        return ud.scrollmode;
    case READ_RENDERER:
        return getrendermode();
    case READ_LASTWEAPON:
        return droidinput.lastWeapon;
    case READ_PAUSED:
        return ud.pause_on != 0;
    default:
        return 0;
    }
}

///This stuff is called from the game/engine

void CONTROL_Android_SetLastWeapon(int w)
{
    LOGI("setLastWeapon %d",w);
    droidinput.lastWeapon = w;
}

void CONTROL_Android_ClearButton(int32_t whichbutton)
{
    BUTTONCLEAR(whichbutton);
    droidinput.functionHeld  &= ~((uint64_t)1<<((uint64_t)(whichbutton)));
}

void CONTROL_Android_PollDevices(ControlInfo *info)
{
    //LOGI("CONTROL_Android_PollDevices %f %f",forwardmove,sidemove);
    //LOGI("CONTROL_Android_PollDevices %f %f",droidinput.pitch,droidinput.yaw);

    info->dz     = (int32_t)nearbyintf(-droidinput.forwardmove * ANDROIDFORWARDMOVEFACTOR);
    info->dx     = (int32_t)nearbyintf(droidinput.sidemove * ANDROIDSIDEMOVEFACTOR);
    info->dpitch = (int32_t)nearbyint(droidinput.pitch * ANDROIDPITCHFACTOR +
            droidinput.pitch_joystick * ANDROIDPITCHFACTORJOYSTICK);
    info->dyaw   = (int32_t)nearbyint(-droidinput.yaw * ANDROIDYAWFACTOR -
            droidinput.yaw_joystick * ANDROIDYAWFACTORJOYSTICK);

    //droidinput.forwardmove = droidinput.sidemove = 0.f;
    droidinput.pitch = droidinput.yaw = 0.f;

    CONTROL_ButtonState = 0;
    CONTROL_ButtonState |= droidinput.functionSticky;
    CONTROL_ButtonState |= droidinput.functionHeld;

    droidinput.functionSticky = 0;

    //LOGI("poll state = 0x%016llX",CONTROL_ButtonState);
}


