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

#define BUTTONSET(x,value) (CONTROL_ButtonState |= ((uint64_t)value<<((uint64_t)(x))))
#define BUTTONCLEAR(x) (CONTROL_ButtonState &= ~((uint64_t)1<<((uint64_t)(x))))

extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
extern int SDL_SendKeyboardText(const char *text);

static float forwardmove, sidemove; //Joystick mode
static char sdl_text[2];

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
	}

	return 0;

}



void changeActionState(int state, int action)
{
	if (state)
	{
		//BUTTONSET(action,1);
		droidplayer.functionSticky  |= ((uint64_t)1<<((uint64_t)(action)));
		droidplayer.functionHeld    |= ((uint64_t)1<<((uint64_t)(action)));

        return;
    }

    //BUTTONCLEAR(action);
    droidplayer.functionHeld  &= ~((uint64_t) 1<<((uint64_t) (action)));
}

void PortableAction(int state, int action)
{
	LOGI("PortableAction action = %d, state = %d",action,state);

	//Special toggle for crouch, NOT when using jetpack or in water
	if (!g_player[myconnectindex].ps->jetpack_on &&
			g_player[myconnectindex].ps->on_ground &&
			(sector[g_player[myconnectindex].ps->cursectnum].lotag != ST_2_UNDERWATER))
	{

		if (action == gamefunc_Crouch)
		{
			if (state)
				droidplayer.crouchToggleState = !droidplayer.crouchToggleState;

			state = droidplayer.crouchToggleState;
		}
	}

    //Check if jumping while crouched
    if (action == gamefunc_Jump)
    {
        droidplayer.crouchToggleState = 0;
        changeActionState(0, gamefunc_Crouch);
    }

	changeActionState(state,action);
	LOGI("PortableAction state = 0x%016llX",CONTROL_ButtonState);
}

// =================== FORWARD and SIDE MOVMENT ==============

void PortableMoveFwd(float fwd)
{
	forwardmove = fclamp2(fwd, -1.f, 1.f);
}

void PortableMoveSide(float strafe)
{
	sidemove = fclamp2(strafe, -1.f, 1.f);
}

void PortableMove(float fwd, float strafe)
{
	PortableMoveFwd(fwd);
	PortableMoveSide(strafe);
}

//======================================================================

//Look up and down
int look_pitch_mode;
float look_pitch_mouse,look_pitch_abs,look_pitch_joy;
void PortableLookPitch(int mode, float pitch)
{
	//LOGI("PortableLookPitch %d %f",mode, pitch);
	look_pitch_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_pitch_mouse += pitch;
		break;
	case LOOK_MODE_ABSOLUTE:
		look_pitch_abs = pitch;
		break;
	case LOOK_MODE_JOYSTICK:
		look_pitch_joy = pitch;
		break;
	}
}

//left right
int look_yaw_mode;
float look_yaw_mouse,look_yaw_joy;
void PortableLookYaw(int mode, float yaw)
{
	look_yaw_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_yaw_mouse += yaw;
		break;
	case LOOK_MODE_JOYSTICK:
		look_yaw_joy = yaw;
		break;
	}
}



void PortableCommand(const char * cmd)
{
    OSD_Dispatch(cmd);
}

void PortableInit(int argc,const char ** argv)
{
	main(argc, argv);
}


void PortableFrame()
{
	//NOT USED for DUKE
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
        return ud.overhead_on != 0;
    case READ_KEYBOARD:
        return 0;
    case READ_RENDERER:
        return getrendermode();
    case READ_LASTWEAPON:
        return droidplayer.lastWeapon;
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
	droidplayer.lastWeapon = w;
}

void CONTROL_Android_ClearButton(int32_t whichbutton)
{
	BUTTONCLEAR(whichbutton);
	droidplayer.functionHeld  &= ~((uint64_t)1<<((uint64_t)(whichbutton)));
}

void  CONTROL_Android_PollDevices(ControlInfo *info)
{
	//LOGI("CONTROL_Android_PollDevices %f %f",forwardmove,sidemove);

	info->dz   = -forwardmove * 5000;
	info->dx   = sidemove * 200;

	switch(look_pitch_mode)
	{
	case LOOK_MODE_MOUSE:
		info->dpitch = look_pitch_mouse * 100000;
		look_pitch_mouse = 0;
		break;
	case LOOK_MODE_ABSOLUTE:
		//cl.viewangles[0] = look_pitch_abs * 80;
		break;
	case LOOK_MODE_JOYSTICK:
		info->dpitch = look_pitch_joy * 2000;
		break;
	}

	switch(look_yaw_mode)
	{
	case LOOK_MODE_MOUSE:
		info->dyaw = -look_yaw_mouse * 80000;
		look_yaw_mouse = 0;
		break;
	case LOOK_MODE_JOYSTICK:
		info->dyaw = -look_yaw_joy * 4000;
		break;
	}
	CONTROL_ButtonState = 0;
	CONTROL_ButtonState |= droidplayer.functionSticky;
	CONTROL_ButtonState |= droidplayer.functionHeld;

	droidplayer.functionSticky = 0;

	//LOGI("poll state = 0x%016llX",CONTROL_ButtonState);
}


