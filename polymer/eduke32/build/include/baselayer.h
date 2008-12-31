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

extern int _buildargc;
extern const char **_buildargv;

extern char quitevent, appactive;

extern int vsync;

// NOTE: these are implemented in game-land so they may be overridden in game specific ways
extern int startwin_open(void);
extern int startwin_close(void);
extern int startwin_puts(const char *);
extern int startwin_settitle(const char *);
extern int startwin_idle(void *);

// video
extern int xres, yres, bpp, fullscreen, bytesperline, imageSize;
extern intptr_t frameplace;
extern char offscreenrendering;

extern void (*baselayer_onvideomodechange)(int);

#ifdef USE_OPENGL
extern int osdcmd_glinfo(const osdfuncparm_t *parm);

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
    char dumped;
};
extern struct glinfo glinfo;
extern void setvsync(int sync);
#endif

extern char inputdevices;

// keys
#define KEYFIFOSIZ 64
extern char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
extern unsigned char keyasciififo[KEYFIFOSIZ], keyasciififoplc, keyasciififoend;
extern char scantoasc[128];

// mouse
extern volatile int mousex, mousey, mouseb;

// joystick
extern int *joyaxis, *joyhat, joyb;
extern char joyisgamepad, joynumaxes, joynumbuttons, joynumhats;
extern int joyaxespresent;

extern int qsetmode;

int initsystem(void);
void uninitsystem(void);

void initprintf(const char *, ...);
void debugprintf(const char *,...);

int handleevents(void);
extern inline void idle(void);

typedef void (*KeyPressCallback)(int,int);
typedef void (*MousePressCallback)(int,int);
typedef void (*JoyPressCallback)(int,int);
int initinput(void);
void uninitinput(void);
void releaseallbuttons(void);
void setkeypresscallback(void (*callback)(int,int));
void setmousepresscallback(void (*callback)(int,int));
void setjoypresscallback(void (*callback)(int,int));
const unsigned char *getkeyname(int num);
const unsigned char *getjoyname(int what, int num); // what: 0=axis, 1=button, 2=hat
char *strtolower(char *str, int len);

unsigned char bgetchar(void);
int bkbhit(void);
void bflushchars(void);

int initmouse(void);
void uninitmouse(void);
void grabmouse(char a);
void readmousexy(int *x, int *y);
void readmousebstatus(int *b);
void setjoydeadzone(int axis, unsigned short dead, unsigned short satur);
void getjoydeadzone(int axis, unsigned short *dead, unsigned short *satur);

int inittimer(int);
void uninittimer(void);
void sampletimer(void);
unsigned int getticks(void);
int gettimerfreq(void);
void (*installusertimercallback(void (*callback)(void)))(void);

int checkvideomode(int *x, int *y, int c, int fs, int forced);
int setvideomode(int x, int y, int c, int fs);
void getvalidmodes(void);
void resetvideomode(void);

void begindrawing(void);
void enddrawing(void);
void showframe(int);

int setpalette(int start, int num);
//int getpalette(int start, int num, char *dapal);
int setgamma(void);
double vid_gamma, vid_contrast, vid_brightness;

#define DEFAULT_GAMMA 1.0
#define DEFAULT_CONTRAST 1.0
#define DEFAULT_BRIGHTNESS 0.0

int switchrendermethod(int,int);    // 0 = software, 1 = opengl | bool = reinit

int wm_msgbox(char *name, char *fmt, ...);
int wm_ynbox(char *name, char *fmt, ...);
void wm_setapptitle(char *name);

// baselayer.c
int baselayer_init();

void makeasmwriteable(void);

#ifdef __cplusplus
}
#endif

#endif // __baselayer_h__

