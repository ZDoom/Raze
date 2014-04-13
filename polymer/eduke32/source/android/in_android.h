#include "function.h"
#include "compat.h"

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

#define BUTTONSET(x,value) (CONTROL_ButtonState |= ((uint64_t)value<<((uint64_t)(x))))
#define BUTTONCLEAR(x) (CONTROL_ButtonState &= ~((uint64_t)1<<((uint64_t)(x))))

#define PRECISIONSHOOTFACTOR        0.3f

// where do these numbers come from?
#define ANDROIDFORWARDMOVEFACTOR    5000
#define ANDROIDSIDEMOVEFACTOR       200
#define ANDROIDPITCHFACTOR          100000
#define ANDROIDYAWFACTOR            80000

#define MINCONTROLALPHA             0.25f

typedef enum {
    READ_MENU,
    READ_WEAPONS,
    READ_AUTOMAP,
    READ_MAPFOLLOWMODE,
    READ_RENDERER,
    READ_LASTWEAPON,
    READ_PAUSED
} portableread_t;

typedef struct
{
    int32_t crouchToggleState;
    int32_t lastWeapon;

    uint64_t functionSticky; //To let at least one tick
    uint64_t functionHeld;

    double pitch, yaw;
    float forwardmove, sidemove;
} droidinput_t;

typedef struct  
{
    int32_t audio_sample_rate;
    int32_t audio_buffer_size;
    uint16_t screen_width, screen_height;
} droidsysinfo_t;

extern droidsysinfo_t droidinfo;

int PortableKeyEvent(int state, int code, int unicode);
int PortableRead(portableread_t r);

void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableLook(double yaw, double pitch);
void PortableCommand(const char * cmd);

void PortableInit(int argc, const char ** argv);

#ifdef __cplusplus
}
#endif
