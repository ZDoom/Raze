#include "sdl_inc.h"
#include "baselayer.h"
#include "keys.h"
#include "duke3d.h"
#include "common_game.h"
#include "osd.h"
#include "player.h"

#include "jmact/keyboard.h"
#include "jmact/control.h"

#include "../src/video/android/SDL_androidkeyboard.h"

#include "in_android.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"DUKE", __VA_ARGS__))


extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
extern int SDL_SendKeyboardText(const char *text);

char sdl_text[2];
int PortableKeyEvent(int state, int code,int unicode){

	LOGI("PortableKeyEvent %d %d %d",state,code,unicode);

	/*
	if (state)
		 Android_OnKeyDown(code);
	else
		 Android_OnKeyUp(code);

	return;
	 */

	if (state)
		SDL_SendKeyboardKey(SDL_PRESSED, code);
	else
		SDL_SendKeyboardKey(SDL_RELEASED, code);

	SDL_EventState(SDL_TEXTINPUT,SDL_ENABLE);

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


int crouchToggleState=0;

#define BUTTONSET(x,value) (CONTROL_ButtonState |= ((uint64_t)value<<((uint64_t)(x))))
#define BUTTONCLEAR(x) (CONTROL_ButtonState &= ~((uint64_t)1<<((uint64_t)(x))))

uint64_t functionSticky = 0; //To let at least one tick
uint64_t functionHeld = 0;

void changeActionState(int state, int action)
{
	if (state)
	{
		//BUTTONSET(action,1);
		functionSticky  |= ((uint64_t)1<<((uint64_t)(action)));
		functionHeld    |= ((uint64_t)1<<((uint64_t)(action)));
	}
	else
	{
		//BUTTONCLEAR(action);
		functionHeld  &= ~((uint64_t)1<<((uint64_t)(action)));
	}
}

void PortableAction(int state, int action)
{
	LOGI("PortableAction action = %d, state = %d",action,state);

	//Special toggle for crouch, NOT when using jetpack or in water
	if (!g_player[myconnectindex].ps->jetpack_on &&
			g_player[myconnectindex].ps->on_ground &&
			(sector[g_player[myconnectindex].ps->cursectnum].lotag != 2))// This means underwater!
	{

		if (action == gamefunc_Crouch)
		{
			if (state)
			{
				crouchToggleState =!crouchToggleState;
			}
			state = crouchToggleState;
		}
	}

	//Check if jumping while crouched
	if (action == gamefunc_Jump)
	{
		if (crouchToggleState)
		{
			crouchToggleState = 0;
			changeActionState(0,gamefunc_Crouch);
		}
	}

	changeActionState(state,action);
	LOGI("PortableAction state = 0x%016llX",CONTROL_ButtonState);
}

// =================== FORWARD and SIDE MOVMENT ==============

float forwardmove, sidemove; //Joystick mode

void PortableMoveFwd(float fwd)
{
	if (fwd > 1)
		fwd = 1;
	else if (fwd < -1)
		fwd = -1;

	forwardmove = fwd;
}

void PortableMoveSide(float strafe)
{
	if (strafe > 1)
		strafe = 1;
	else if (strafe < -1)
		strafe = -1;

	sidemove = strafe;
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



void PortableCommand(const char * cmd){}

extern int32_t main(int32_t argc, char *argv[]);
void PortableInit(int argc,const char ** argv){

	main(argc,argv);
}


void PortableFrame(){
	//NOT USED for DUKE
}

int PortableInMenu(){
	return  ( (g_player[myconnectindex].ps->gm & MODE_MENU) || !(g_player[myconnectindex].ps->gm & MODE_GAME))?1:0;
}

unsigned int PortableGetWeapons()
{
	return g_player[myconnectindex].ps->gotweapon;
}
int PortableInAutomap()
{
	return 0;
}

int PortableShowKeyboard(){

	return 0;
}

int PortableIsSoftwareMode()
{
	if (getrendermode() >= REND_POLYMOST)
		return 0;
	else
		return 1;

}


static int lastWeapon = -1;
int getLastWeapon(){
	return lastWeapon;
}

extern user_defs ud;

int isPaused()
{
	return ud.pause_on;
}

///This stuff is called from the game/engine

void setLastWeapon(int w)
{
	LOGI("setLastWeapon %d",w);
	lastWeapon = w;
}


void CONTROL_Android_ClearButton(int32_t whichbutton)
{
	BUTTONCLEAR(whichbutton);
	functionHeld  &= ~((uint64_t)1<<((uint64_t)(whichbutton)));
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
	CONTROL_ButtonState |= functionSticky;
	CONTROL_ButtonState |= functionHeld;

	functionSticky = 0;

	//LOGI("poll state = 0x%016llX",CONTROL_ButtonState);
}


