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

int32_t videoCheckMode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced);
int32_t videoSetMode(int32_t x, int32_t y, int32_t c, int32_t fs);
void    videoGetModes(void);
void    videoResetMode(void);
void    videoEndDrawing(void);
void    videoShowFrame(int32_t);
int32_t videoUpdatePalette(int32_t start, int32_t num);
int32_t videoSetGamma(void);
int32_t videoSetVsync(int32_t newSync);

//#define DEBUG_FRAME_LOCKING
#if !defined DEBUG_FRAME_LOCKING
void videoBeginDrawing(void);
#else
void begindrawing_real(void);
# define BEGINDRAWING_SIZE 256
extern uint32_t begindrawing_line[BEGINDRAWING_SIZE];
extern const char *begindrawing_file[BEGINDRAWING_SIZE];
extern int32_t lockcount;
# define videoBeginDrawing() do {                     \
    if (lockcount < BEGINDRAWING_SIZE) {         \
        begindrawing_line[lockcount] = __LINE__; \
        begindrawing_file[lockcount] = __FILE__; \
    }                                            \
    begindrawing_real();                         \
} while(0)
#endif

extern float g_videoGamma, g_videoContrast, g_videoBrightness;

#define DEFAULT_GAMMA 1.0f
#define DEFAULT_CONTRAST 1.0f
#define DEFAULT_BRIGHTNESS 0.0f

#define GAMMA_CALC ((int32_t)(min(max((float)((g_videoGamma - 1.0f) * 10.0f), 0.f), 15.f)))

#ifdef USE_OPENGL
extern int32_t (*baselayer_osdcmd_vidmode_func)(osdfuncparm_t const * const parm);
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
    char sync;
    char dumped;
};

extern struct glinfo_t glinfo;
#endif
extern vec2_t const g_defaultVideoModes[];

extern char inputdevices;

// keys
#define NUMKEYS 256
#define KEYFIFOSIZ 64
extern char const g_keyAsciiTable[128];

extern char    keystatus[NUMKEYS];
extern char    g_keyFIFO[KEYFIFOSIZ];
extern char    g_keyAsciiFIFO[KEYFIFOSIZ];
extern uint8_t g_keyAsciiPos;
extern uint8_t g_keyAsciiEnd;
extern uint8_t g_keyFIFOend;
extern char    g_keyRemapTable[NUMKEYS];
extern char    g_keyNameTable[NUMKEYS][24];

extern int32_t keyGetState(int32_t key);
extern void keySetState(int32_t key, int32_t state);

// mouse
extern vec2_t  g_mousePos;
extern vec2_t  g_mouseAbs;
extern int32_t g_mouseBits;
extern uint8_t g_mouseClickState;
extern bool    g_mouseGrabbed;
extern bool    g_mouseEnabled;
extern bool    g_mouseInsideWindow;
extern bool    g_mouseLockedToWindow;

enum
{
    MOUSE_IDLE = 0,
    MOUSE_PRESSED,
    MOUSE_HELD,
    MOUSE_RELEASED,
};
extern int32_t mouseAdvanceClickState(void);

// joystick

typedef struct
{
    int32_t *pAxis;
    int32_t *pHat;
    void (*pCallback)(int32_t, int32_t);
    int32_t  bits;
    int32_t  numAxes;
    int32_t  numButtons;
    int32_t  numHats;
} controllerinput_t;

extern controllerinput_t joystick;

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

int32_t initsystem(void);
void uninitsystem(void);
void system_getcvars(void);

extern int32_t g_logFlushWindow;
void initputs(const char *);
#define buildputs initputs
void initprintf(const char *, ...) ATTRIBUTE((format(printf,1,2)));
#define buildprintf initprintf
void debugprintf(const char *,...) ATTRIBUTE((format(printf,1,2)));

int32_t handleevents(void);
int32_t handleevents_peekkeys(void);

extern void (*keypresscallback)(int32_t,int32_t);
extern void (*g_mouseCallback)(int32_t,int32_t);

int32_t initinput(void);
void uninitinput(void);
void keySetCallback(void (*callback)(int32_t,int32_t));
void mouseSetCallback(void (*callback)(int32_t,int32_t));
void joySetCallback(void (*callback)(int32_t,int32_t));
const char *keyGetName(int32_t num);
const char *joyGetName(int32_t what, int32_t num); // what: 0=axis, 1=button, 2=hat

char keyGetChar(void);
#define keyBufferWaiting() (g_keyAsciiPos != g_keyAsciiEnd)

static FORCE_INLINE int keyBufferFull(void)
{
    return ((g_keyAsciiEnd+1)&(KEYFIFOSIZ-1)) == g_keyAsciiPos;
}

static FORCE_INLINE void keyBufferInsert(char code)
{
    g_keyAsciiFIFO[g_keyAsciiEnd] = code;
    g_keyAsciiEnd = ((g_keyAsciiEnd+1)&(KEYFIFOSIZ-1));
}

void keyFlushChars(void);

int32_t mouseInit(void);
void mouseUninit(void);
int32_t mouseReadAbs(vec2_t *const destination, vec2_t const *const source);
void mouseGrabInput(bool grab);
void mouseLockToWindow(char a);
void mouseReadButtons(int32_t *b);
void mouseReadPos(int32_t *x, int32_t *y);

void joyReadButtons(int32_t *b);
void joySetDeadZone(int32_t axis, uint16_t dead, uint16_t satur);
void joyGetDeadZone(int32_t axis, uint16_t *dead, uint16_t *satur);
extern int32_t inputchecked;

int32_t  timerInit(int32_t);
void     timerUninit(void);
void     timerUpdate(void);
int32_t  timerGetFreq(void);
uint64_t timerGetTicksU64(void);
uint64_t timerGetFreqU64(void);
double   timerGetHiTicks(void);
void (*timerSetCallback(void (*callback)(void)))(void);

#if defined RENDERTYPESDL && !defined LUNATIC
static FORCE_INLINE uint32_t timerGetTicks(void) { return (uint32_t)SDL_GetTicks(); }
#else
uint32_t timerGetTicks(void);
#endif

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

