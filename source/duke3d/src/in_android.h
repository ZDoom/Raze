#include "function.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MENU_UP                  0x200
#define MENU_DOWN                0x201
#define MENU_LEFT                0x202
#define MENU_RIGHT               0x203
#define MENU_SELECT              0x204
#define MENU_BACK                0x205

#define KEY_QUICK_CMD    0x1005
#define KEY_SHOW_KBRD    0x1008
#define KEY_SHOW_INVEN    0x1009
#define KEY_QUICK_SAVE    0x100A
#define KEY_QUICK_LOAD    0x100B

#define KEY_QUICK_KEY1    0x1011
#define KEY_QUICK_KEY2    0x1012
#define KEY_QUICK_KEY3    0x1013
#define KEY_QUICK_KEY4    0x1014

// #define BUTTONSET(x,value) (CONTROL_ButtonState |= ((uint64_t)value<<((uint64_t)(x))))
// #define BUTTONCLEAR(x) (CONTROL_ButtonState &= ~((uint64_t)1<<((uint64_t)(x))))

#define PRECISIONSHOOTFACTOR        0.3f

// where do these numbers come from?
#define ANDROIDMOVEFACTOR           6400
#define ANDROIDLOOKFACTOR          1600

#define ANDROIDPITCHFACTORJOYSTICK          2000
#define ANDROIDYAWFACTORJOYSTICK            4000

typedef enum {
    R_TOUCH_MODE,
    R_PLAYER_GOTWEAPON,
    R_UD_OVERHEAD_ON,
    R_UD_SCROLLMODE,
    R_PLAYER_LASTWEAPON,
    R_GAME_PAUSED,
    R_PLAYER_DEAD_FLAG,
    R_PLAYER_INV_AMOUNT,
    R_SOMETHINGONPLAYER
} portableread_t;


typedef enum {
    TOUCH_SCREEN_BLANK, //Nothing on screen (not used)
    TOUCH_SCREEN_BLANK_TAP, //One button filling screen with no graphic, tap to send Enter key
    TOUCH_SCREEN_YES_NO, //Yes/No buttons on screen, sends Enter or Esc
    TOUCH_SCREEN_MENU, //Normal menu
    TOUCH_SCREEN_MENU_NOBACK, // menu without back button
    TOUCH_SCREEN_GAME, //Normal game screen
    TOUCH_SCREEN_AUTOMAP, //When auto map is up (not used yet)
    TOUCH_SCREEN_CONSOLE //When Console is up
} touchscreemode_t;


typedef struct
{
    int32_t crouchToggleState;
    int32_t lastWeapon;
    int32_t toggleCrouch;
    int32_t quickSelectWeapon;

    uint64_t functionSticky; //To let at least one tick
    uint64_t functionHeld;

    int32_t left_double_action;
    int32_t right_double_action;

    int32_t invertLook, hideStick;

    double pitch, yaw;
    double pitch_joystick, yaw_joystick;
    float forwardmove, sidemove;

    // set by configuration UI
    float strafe_sens, forward_sens;
    float pitch_sens, yaw_sens;

    float gameControlsAlpha;
} droidinput_t;

typedef struct
{
    int32_t audio_sample_rate;
    int32_t audio_buffer_size;
    uint16_t screen_width, screen_height;
} droidsysinfo_t;

extern droidinput_t droidinput;
extern droidsysinfo_t droidinfo;

void AndroidTimer(int tics);
int AndroidKeyEvent(int state, int code, int unicode);
int AndroidRead(portableread_t r);

void AndroidAction(int state, int action);

void AndroidMouseMenu(float x,float y);
void AndroidMouseMenuButton(int state,int button);

void AndroidMove(float fwd, float strafe);
void AndroidLook(float yaw, float pitch);
void AndroidLookJoystick(float yaw, float pitch);
void AndroidOSD(const char * cmd);

void AndroidAutomapControl(float zoom,float dx,float dy);

void AndroidShowKeyboard(int onf);

void AndroidToggleButtonEditor(void);
#ifdef __cplusplus
}
#endif
