// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#pragma once

#ifndef baselayer_h_
#define baselayer_h_

#include "compat.h"
#include "osd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int app_main(int argc, char const * const * argv);
extern const char* AppProperName;
extern const char* AppTechnicalName;

#ifdef DEBUGGINGAIDS
# define DEBUG_MASK_DRAWING
extern int32_t g_maskDrawMode;
#endif

extern char quitevent, appactive;
extern char modechange;

extern int32_t vsync;

extern void app_crashhandler(void);

// NOTE: these are implemented in game-land so they may be overridden in game specific ways
extern int32_t startwin_open(void);
extern int32_t startwin_close(void);
extern int32_t startwin_puts(const char *);
extern int32_t startwin_settitle(const char *);
extern int32_t startwin_idle(void *);
extern int32_t startwin_run(void);

// video
extern int32_t r_usenewaspect, newaspect_enable;
extern int32_t setaspect_new_use_dimen;
extern uint32_t r_screenxy;
extern int32_t xres, yres, bpp, fullscreen, bytesperline;
extern intptr_t frameplace;
extern char offscreenrendering;
extern int32_t nofog;

void calc_ylookup(int32_t bpl, int32_t lastyidx);

#ifdef USE_OPENGL
extern int32_t (*baselayer_osdcmd_vidmode_func)(osdfuncparm_t const * const parm);

void fullscreen_tint_gl(uint8_t r, uint8_t g, uint8_t b, uint8_t f);
extern int32_t osdcmd_glinfo(osdfuncparm_t const * const parm);

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
    char bufferstorage;
    char dumped;
};

extern struct glinfo_t glinfo;
#endif
extern int32_t setvsync(int32_t newSync);
extern char inputdevices;

// keys
#define NUMKEYS 256
#define KEYSTATUSSIZ 256
#define KEYFIFOSIZ 64
extern char keystatus[KEYSTATUSSIZ], keyfifo[KEYFIFOSIZ], keyasciififo[KEYFIFOSIZ];
extern uint8_t keyasciififoplc, keyasciififoend, keyfifoplc, keyfifoend;
extern char scantoasc[128], keyremap[KEYSTATUSSIZ], key_names[NUMKEYS][24];
extern int32_t keyremapinit;

extern int32_t defaultres[][2];

extern int32_t GetKey(int32_t key);
extern void SetKey(int32_t key, int32_t state);

// mouse
extern int32_t mousex, mousey, mouseb;
extern vec2_t mouseabs;
extern uint8_t mousepressstate;
extern uint8_t mousegrab, moustat, mouseinwindow, AppMouseGrab;
enum
{
    Mouse_Idle = 0,
    Mouse_Pressed = 1,
    Mouse_Held = 2,
    Mouse_Released = 3,
};
extern int32_t mousepressstateadvance(void);

// joystick
extern int32_t *joyaxis, *joyhat, joyb;
extern char joyisgamepad, joynumaxes, joynumbuttons, joynumhats;
extern int32_t joyaxespresent;

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

int32_t initsystem(void);
void uninitsystem(void);
void system_getcvars(void);

extern int32_t flushlogwindow;
void initputs(const char *);
#define buildputs initputs
void initprintf(const char *, ...) ATTRIBUTE((format(printf,1,2)));
#define buildprintf initprintf
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

static FORCE_INLINE int keyascfifo_isfull(void)
{
    return ((keyasciififoend+1)&(KEYFIFOSIZ-1)) == keyasciififoplc;
}

static FORCE_INLINE void keyascfifo_insert(char code)
{
    keyasciififo[keyasciififoend] = code;
    keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
}

void bflushchars(void);

int32_t initmouse(void);
void uninitmouse(void);
void grabmouse(char a);
void AppGrabMouse(char a);
void readmousexy(int32_t *x, int32_t *y);
int32_t readmouseabsxy(vec2_t * const destination, vec2_t const * const source);
void readmousebstatus(int32_t *b);
void readjoybstatus(int32_t *b);
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur);
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur);
extern int32_t inputchecked;

int32_t inittimer(int32_t);
void uninittimer(void);
void sampletimer(void);
#if defined RENDERTYPESDL && !defined LUNATIC
static FORCE_INLINE uint32_t getticks(void) { return (uint32_t)SDL_GetTicks(); }
#else
uint32_t getticks(void);
#endif
int32_t gettimerfreq(void);
uint64_t getu64ticks(void);
uint64_t getu64tickspersec(void);
double gethiticks(void);
void (*installusertimercallback(void (*callback)(void)))(void);

int32_t checkvideomode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced);
int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs);
void getvalidmodes(void);
void resetvideomode(void);

//#define DEBUG_FRAME_LOCKING

void enddrawing(void);
void showframe(int32_t);
#if !defined DEBUG_FRAME_LOCKING
void begindrawing(void);
#else
void begindrawing_real(void);
# define BEGINDRAWING_SIZE 256
extern uint32_t begindrawing_line[BEGINDRAWING_SIZE];
extern const char *begindrawing_file[BEGINDRAWING_SIZE];
extern int32_t lockcount;
# define begindrawing() do {                     \
    if (lockcount < BEGINDRAWING_SIZE) {         \
        begindrawing_line[lockcount] = __LINE__; \
        begindrawing_file[lockcount] = __FILE__; \
    }                                            \
    begindrawing_real();                         \
} while(0)
#endif

int32_t setpalette(int32_t start, int32_t num);
//int32_t getpalette(int32_t start, int32_t num, char *dapal);
int32_t setgamma(void);
extern float vid_gamma, vid_contrast, vid_brightness;

#define DEFAULT_GAMMA 1.0f
#define DEFAULT_CONTRAST 1.0f
#define DEFAULT_BRIGHTNESS 0.0f

#define GAMMA_CALC ((int32_t)(min(max((float)((vid_gamma-1.0f)*10.0f),0.f),15.f)))

//int32_t switchrendermethod(int32_t,int32_t);    // 0 = software, 1 = opengl | bool = reinit

int32_t wm_msgbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
int32_t wm_ynbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
void wm_setapptitle(const char *name);

// baselayer.c
int32_t baselayer_init();

void makeasmwriteable(void);
void maybe_redirect_outputs(void);

#ifdef __cplusplus
}
#endif

#include "print.h"

#endif // baselayer_h_

