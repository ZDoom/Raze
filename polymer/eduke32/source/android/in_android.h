#include "function.h"
#include "compat.h"

enum {
    LOOK_MODE_MOUSE = 0,
    LOOK_MODE_ABSOLUTE,
    LOOK_MODE_JOYSTICK
};

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum {
        READ_MENU,
        READ_WEAPONS,
        READ_AUTOMAP,
        READ_KEYBOARD,
        READ_RENDERER,
        READ_LASTWEAPON,
        READ_PAUSED
    } portableread_t;

    int32_t PortableRead(portableread_t r);

    typedef struct
    {
        int32_t crouchToggleState;
        int32_t lastWeapon;
        uint64_t functionSticky; //To let at least one tick
        uint64_t functionHeld;
    } androidplayer_t;

    extern androidplayer_t droidplayer;

    int PortableKeyEvent(int state, int code, int unicode);
    void PortableAction(int state, int action);

    void PortableMove(float fwd, float strafe);
    void PortableMoveFwd(float fwd);
    void PortableMoveSide(float strafe);
    void PortableLookPitch(int mode, float pitch);
    void PortableLookYaw(int mode, float pitch);
    void PortableCommand(const char * cmd);

    void PortableInit(int argc, const char ** argv);
    void PortableFrame();

#ifdef __cplusplus
}
#endif
