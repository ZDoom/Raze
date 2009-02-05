// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __baselayer_h__
#define __baselayer_h__

#include "compat.h"
#include "osd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t _buildargc;
extern const char **_buildargv;

extern char quitevent, appactive;

extern int32_t vsync;

// NOTE: these are implemented in game-land so they may be overridden in game specific ways
extern int32_t startwin_open(void);
extern int32_t startwin_close(void);
extern int32_t startwin_puts(const char *);
extern int32_t startwin_settitle(const char *);
extern int32_t startwin_idle(void *);

// video
extern int32_t xres, yres, bpp, fullscreen, bytesperline, imageSize;
extern intptr_t frameplace;
extern char offscreenrendering;

extern void (*baselayer_onvideomodechange)(int32_t);

#ifdef USE_OPENGL
extern int32_t osdcmd_glinfo(const osdfuncparm_t *parm);

struct glinfo {
    const char *vendor;
    const char *renderer;
    const char *version;
    const char *extensions;

	float maxanisotropy;
	char bgra;
	char clamptoedge;
	char texcompr;
	char texnpot;
	char multisample;
	char nvmultisamplehint;
    char arbfp;
    char depthtex;
    char shadow;
    char fbos;
    char rect;
    char multitex;
    char envcombine;
    char vbos;
    char vsync;
    char sm4;
    char dumped;
};
extern struct glinfo glinfo;
extern void setvsync(int32_t sync);
#endif

extern char inputdevices;

// keys
#define KEYFIFOSIZ 64
extern char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
extern char keyasciififo[KEYFIFOSIZ], keyasciififoplc, keyasciififoend;
extern char scantoasc[128];

// mouse
extern volatile int32_t mousex, mousey, mouseb;

// joystick
extern int32_t *joyaxis, *joyhat, joyb;
extern char joyisgamepad, joynumaxes, joynumbuttons, joynumhats;
extern int32_t joyaxespresent;

extern int32_t qsetmode;

int32_t initsystem(void);
void uninitsystem(void);

extern int32_t flushlogwindow;
void initprintf(const char *, ...);
void debugprintf(const char *,...);

int32_t handleevents(void);
extern inline void idle(void);

typedef void (*KeyPressCallback)(int32_t,int32_t);
typedef void (*MousePressCallback)(int32_t,int32_t);
typedef void (*JoyPressCallback)(int32_t,int32_t);
int32_t initinput(void);
void uninitinput(void);
void releaseallbuttons(void);
void setkeypresscallback(void (*callback)(int32_t,int32_t));
void setmousepresscallback(void (*callback)(int32_t,int32_t));
void setjoypresscallback(void (*callback)(int32_t,int32_t));
const char *getkeyname(int32_t num);
const char *getjoyname(int32_t what, int32_t num); // what: 0=axis, 1=button, 2=hat
char *strtolower(char *str, int32_t len);

char bgetchar(void);
int32_t bkbhit(void);
void bflushchars(void);

int32_t initmouse(void);
void uninitmouse(void);
void grabmouse(char a);
void readmousexy(int32_t *x, int32_t *y);
void readmousebstatus(int32_t *b);
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur);
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur);

int32_t inittimer(int32_t);
void uninittimer(void);
void sampletimer(void);
uint32_t getticks(void);
int32_t gettimerfreq(void);
void (*installusertimercallback(void (*callback)(void)))(void);

int32_t checkvideomode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced);
int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs);
void getvalidmodes(void);
void resetvideomode(void);

void begindrawing(void);
void enddrawing(void);
void showframe(int32_t);

int32_t setpalette(int32_t start, int32_t num);
//int32_t getpalette(int32_t start, int32_t num, char *dapal);
int32_t setgamma(void);
double vid_gamma, vid_contrast, vid_brightness;

#define DEFAULT_GAMMA 1.0
#define DEFAULT_CONTRAST 1.0
#define DEFAULT_BRIGHTNESS 0.0

int32_t switchrendermethod(int32_t,int32_t);    // 0 = software, 1 = opengl | bool = reinit

int32_t wm_msgbox(char *name, char *fmt, ...);
int32_t wm_ynbox(char *name, char *fmt, ...);
void wm_setapptitle(char *name);

// baselayer.c
int32_t baselayer_init();

void makeasmwriteable(void);

#ifdef __cplusplus
}
#endif

#endif // __baselayer_h__

