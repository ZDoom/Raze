
#ifndef __config_h__
#define __config_h__

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
extern int32_t MAXCACHE1DSIZE;

void CONFIG_MapKey(int which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2);

#endif
