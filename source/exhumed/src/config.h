
#ifndef __config_h__
#define __config_h__

#define kMaxGameFunctions	40
#define kMaxGameFuncLen     64

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXJOYBUTTONS 32
#define MAXJOYBUTTONSANDHATS (MAXJOYBUTTONS+4)

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXMOUSEAXES 2
#define MAXMOUSEDIGITAL (MAXMOUSEAXES*2)

// KEEPINSYNC mact/include/_control.h, build/src/sdlayer.cpp
#define MAXJOYAXES 9
#define MAXJOYDIGITAL (MAXJOYAXES*2)

// default mouse scale
#define DEFAULTMOUSEANALOGUESCALE           65536

// default joystick settings
#define DEFAULTJOYSTICKANALOGUESCALE        65536
#define DEFAULTJOYSTICKANALOGUEDEAD         2000
#define DEFAULTJOYSTICKANALOGUESATURATE     9500

extern const char gamefunctions[kMaxGameFunctions][kMaxGameFuncLen];
extern const char keydefaults[kMaxGameFunctions*2][kMaxGameFuncLen];
extern const char oldkeydefaults[kMaxGameFunctions*2][kMaxGameFuncLen];

enum {
	gamefunc_Move_Forward,  // 0
	gamefunc_Move_Backward, // 1
	gamefunc_Turn_Left,
	gamefunc_Turn_Right,
	gamefunc_Strafe,
	gamefunc_Strafe_Left,
	gamefunc_Strafe_Right,
	gamefunc_Run, // 7
	gamefunc_Jump, // 8
	gamefunc_Crouch, // 9
	gamefunc_Fire, // 10
	gamefunc_Open, // 11
	gamefunc_Look_Up, // 12
	gamefunc_Look_Down,
	gamefunc_Look_Straight,
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_SendMessage,
	gamefunc_Weapon_1,
	gamefunc_Weapon_2,
	gamefunc_Weapon_3,
	gamefunc_Weapon_4,
	gamefunc_Weapon_5,
	gamefunc_Weapon_6,
	gamefunc_Weapon_7,
	gamefunc_Mouseview,
	gamefunc_Pause,
	gamefunc_Map,
	gamefunc_Zoom_In,
	gamefunc_Zoom_Out,
	gamefunc_Gamma_Correction,
	gamefunc_Escape,
	gamefunc_Shrink_Screen,
	gamefunc_Enlarge_Screen,
	gamefunc_Inventory,
	gamefunc_Inventory_Left,
	gamefunc_Inventory_Right,
	gamefunc_Mouse_Sensitivity_Up,
	gamefunc_Mouse_Sensitivity_Down,
    gamefunc_Show_Console,
};

typedef struct {
    int32_t usejoystick;
    int32_t usemouse;
    int32_t fullscreen;
    int32_t xdim;
    int32_t ydim;
    int32_t bpp;
    int32_t forcesetup;
    int32_t noautoload;
} ud_setup_t;

void SetupInput();

void LoadConfig();
int CONFIG_ReadSetup();

extern int lMouseSens;

extern ud_setup_t gSetup;
extern int32_t scripthandle;
extern int32_t setupread;
extern int32_t useprecache;
extern int32_t MouseDeadZone, MouseBias;
extern int32_t SmoothInput;

// JBF 20031211: Store the input settings because
// (currently) mact can't regurgitate them
extern int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
extern int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
extern int32_t MouseAnalogueAxes[MAXMOUSEAXES];
extern int32_t MouseAnalogueScale[MAXMOUSEAXES];
extern int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
extern int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
extern int32_t JoystickAnalogueAxes[MAXJOYAXES];
extern int32_t JoystickAnalogueScale[MAXJOYAXES];
extern int32_t JoystickAnalogueInvert[MAXJOYAXES];
extern int32_t JoystickAnalogueDead[MAXJOYAXES];
extern int32_t JoystickAnalogueSaturate[MAXJOYAXES];
extern uint8_t KeyboardKeys[kMaxGameFunctions][2];

extern int32_t MAXCACHE1DSIZE;

void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2);
int32_t CONFIG_FunctionNameToNum(const char* func);

#endif
