// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef __baselayer_h__
#define __baselayer_h__

#include "compat.h"
#include "osd.h"

#ifdef EXTERNC
extern "C" {
#endif

#define SYSTEM_POOL_SIZE (64 * 1048576)

extern char quitevent, appactive;

extern int32_t vsync;

extern void app_crashhandler(void);

// NOTE: these are implemented in game-land so they may be overridden in game specific ways
extern int32_t startwin_open(void);
extern int32_t startwin_close(void);
extern int32_t startwin_puts(const char *);
extern int32_t startwin_settitle(const char *);
extern int32_t startwin_idle(void *);

// video
extern int32_t r_usenewaspect, newaspect_enable;
extern int32_t setaspect_new_use_dimen;
extern uint32_t r_screenxy;
extern int32_t xres, yres, bpp, fullscreen, bytesperline;
extern intptr_t frameplace;
extern char offscreenrendering;

extern void (*baselayer_onvideomodechange)(int32_t);

void calc_ylookup(int32_t bpl, int32_t lastyidx);

#ifdef USE_OPENGL
void fullscreen_tint_gl(uint8_t r, uint8_t g, uint8_t b, uint8_t f);
extern int32_t osdcmd_glinfo(const osdfuncparm_t *parm);

struct glinfo_t {
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
    char occlusionqueries;
    char glsl;
    char debugoutput;
    char dumped;
};

extern struct glinfo_t glinfo;
extern void setvsync(int32_t sync);
#endif

extern char inputdevices;

// keys
#define KEYFIFOSIZ 64
extern char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
extern char keyasciififo[KEYFIFOSIZ], keyasciififoplc, keyasciififoend;
extern char scantoasc[128], remap[256], key_names[256][24];
extern int32_t remapinit;

extern int32_t defaultres[][2];

extern void SetKey(int32_t key, int32_t state);

// mouse
extern volatile int32_t mousex, mousey, mouseb, mouseabsx, mouseabsy;
extern volatile uint8_t mousegrab, moustat;

// joystick
extern int32_t *joyaxis, *joyhat, joyb;
extern char joyisgamepad, joynumaxes, joynumbuttons, joynumhats;
extern int32_t joyaxespresent;

extern int32_t qsetmode;

static inline int32_t in3dmode(void)
{
    return (qsetmode==200);
}

int32_t initsystem(void);
void uninitsystem(void);

extern int32_t flushlogwindow;
void initprintf(const char *, ...) ATTRIBUTE((format(printf,1,2)));
void debugprintf(const char *,...) ATTRIBUTE((format(printf,1,2)));

int32_t handleevents(void);
int32_t handleevents_peekkeys(void);

extern void (*keypresscallback)(int32_t,int32_t);
extern void (*mousepresscallback)(int32_t,int32_t);
extern void (*joypresscallback)(int32_t,int32_t);

int32_t initinput(void);
void uninitinput(void);
void releaseallbuttons(void);
void setkeypresscallback(void (*callback)(int32_t,int32_t));
void setmousepresscallback(void (*callback)(int32_t,int32_t));
void setjoypresscallback(void (*callback)(int32_t,int32_t));
const char *getkeyname(int32_t num);
const char *getjoyname(int32_t what, int32_t num); // what: 0=axis, 1=button, 2=hat

char bgetchar(void);
#define bkbhit() (keyasciififoplc != keyasciififoend)

void bflushchars(void);

int32_t initmouse(void);
void uninitmouse(void);
void grabmouse(char a);
void readmousexy(int32_t *x, int32_t *y);
void readmouseabsxy(int32_t *x, int32_t *y);
void readmousebstatus(int32_t *b);
void readjoybstatus(int32_t *b);
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur);
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur);

int32_t inittimer(int32_t);
void uninittimer(void);
void sampletimer(void);
uint32_t getticks(void);
int32_t gettimerfreq(void);
uint64_t gethiticks(void);
uint64_t gethitickspersec(void);
double gethitickms(void);  // TODO: Windows
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
extern double vid_gamma, vid_contrast, vid_brightness;

#define DEFAULT_GAMMA 1.0
#define DEFAULT_CONTRAST 1.0
#define DEFAULT_BRIGHTNESS 0.0

#define GAMMA_CALC ((int32_t)(min(max((double)((vid_gamma-1.0)*10.0),0),15)))

//int32_t switchrendermethod(int32_t,int32_t);    // 0 = software, 1 = opengl | bool = reinit

int32_t wm_msgbox(char *name, char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
int32_t wm_ynbox(char *name, char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
void wm_setapptitle(char *name);

// baselayer.c
int32_t baselayer_init();

void makeasmwriteable(void);
void maybe_redirect_outputs(void);

#ifdef EXTERNC
}
#endif

#endif // __baselayer_h__

