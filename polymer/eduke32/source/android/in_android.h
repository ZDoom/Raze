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

#define BUTTONSET(x,value) (CONTROL_ButtonState |= ((uint64_t)value<<((uint64_t)(x))))
#define BUTTONCLEAR(x) (CONTROL_ButtonState &= ~((uint64_t)1<<((uint64_t)(x))))

#define PRECISIONSHOOTFACTOR    0.3f

typedef enum {
    READ_MENU,
    READ_WEAPONS,
    READ_AUTOMAP,
    READ_KEYBOARD,
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
