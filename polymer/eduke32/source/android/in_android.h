#include "function.h"




#define LOOK_MODE_MOUSE    0
#define LOOK_MODE_ABSOLUTE 1
#define LOOK_MODE_JOYSTICK 2



#ifdef __cplusplus
extern "C"
{
#endif
int PortableKeyEvent(int state, int code, int unicode);
void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);
void PortableCommand(const char * cmd);

void PortableInit(int argc,const char ** argv);
void PortableFrame();

int PortableInMenu();
int PortableShowKeyboard();
int PortableInAutomap();

unsigned int PortableGetWeapons();

int getLastWeapon();

int isPaused();


//check mode, so touch graphcics can adapt
int PortableIsSoftwareMode();


#ifdef __cplusplus
}
#endif
