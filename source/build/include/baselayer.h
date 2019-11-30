// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#pragma once

#ifndef baselayer_h_
#define baselayer_h_

#include "compat.h"
#include "osd.h"
#include "timer.h"
#include "c_cvars.h"
#include "inputstate.h"
#include "printf.h"
#include "zstring.h"
#include "vectors.h"


#ifdef DEBUGGINGAIDS
# define DEBUG_MASK_DRAWING
extern int32_t g_maskDrawMode;
#endif

extern char quitevent, appactive;
extern char modechange;
extern char nogl;

extern int32_t swapcomplete;

EXTERN_CVAR(Int, r_borderless);
EXTERN_CVAR(Bool, r_usenewaspect)

// video
extern int32_t newaspect_enable;
extern int32_t r_fpgrouscan;
extern int32_t setaspect_new_use_dimen;
extern uint32_t r_screenxy;
extern int32_t xres, yres, bpp, fullscreen, bytesperline, refreshfreq;
extern intptr_t frameplace;
extern char offscreenrendering;

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

#define GAMMA_CALC ((int32_t)(min(max((float)((vid_gamma - 1.0f) * 10.0f), 0.f), 15.f)))

#ifdef USE_OPENGL
extern int osdcmd_glinfo(osdcmdptr_t parm);


#endif

vec2_t CONSTEXPR const g_defaultVideoModes []
= { { 2560, 1440 }, { 2560, 1200 }, { 2560, 1080 }, { 1920, 1440 }, { 1920, 1200 }, { 1920, 1080 }, { 1680, 1050 },
    { 1600, 1200 }, { 1600, 900 },  { 1366, 768 },  { 1280, 1024 }, { 1280, 960 },  { 1280, 720 },  { 1152, 864 },
    { 1024, 768 },  { 1024, 600 },  { 800, 600 },   { 640, 480 },   { 640, 400 },   { 512, 384 },   { 480, 360 },
    { 400, 300 },   { 320, 240 },   { 320, 200 },   { 0, 0 } };

extern char inputdevices;

// keys
#define KEYFIFOSIZ 64

char CONSTEXPR const g_keyAsciiTable[128] = {
    0  ,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,  0,   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', 0,   0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 0,   92,  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/', 0,   '*', 0,   32,  0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0  ,   0,   0,   0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,
};

char CONSTEXPR const g_keyAsciiTableShift[128] = {
    0  ,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,  0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}', 0,   0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?', 0,   '*', 0,   32,  0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0  ,   0,   0,   0,   0,   0, 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,
};

// mouse

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
    int32_t  isGameController;
} controllerinput_t;

extern controllerinput_t joystick;

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

int32_t initsystem(void);
void uninitsystem(void);
void system_getcvars(void);

extern int32_t g_logFlushWindow;

int32_t handleevents(void);
int32_t handleevents_peekkeys(void);

int32_t initinput(void);
void uninitinput(void);
void joySetCallback(void (*callback)(int32_t,int32_t));
const char *joyGetName(int32_t what, int32_t num); // what: 0=axis, 1=button, 2=hat
void joyScanDevices(void);

void mouseInit(void);
void mouseUninit(void);
void mouseGrabInput(bool grab);
void mouseLockToWindow(char a);
void mouseMoveToCenter(void);

void joyReadButtons(int32_t *pResult);
void joySetDeadZone(int32_t axis, uint16_t dead, uint16_t satur);
void joyGetDeadZone(int32_t axis, uint16_t *dead, uint16_t *satur);
extern int32_t inputchecked;

void getScreen(uint8_t* imgBuf);


int32_t wm_msgbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
int32_t wm_ynbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
void wm_setapptitle(const char *name);

#include "print.h"

struct GameStats
{
	int kill, tkill;
	int secret, tsecret;
	int timesecnd;
};

struct FGameStartup
{
	int Episode;
	int Level;
	int Skill;
	int CustomLevel1;
	int CustomLevel2;
};

struct FSavegameInfo
{
	const char *savesig;
	int minsavever;
	int currentsavever;
};

struct FSaveGameNode
{
	FString SaveTitle;
	FString Filename;
	bool bOldVersion = false;
	bool bMissingWads = false;
	bool bNoDelete = false;
	bool bIsExt = false;

	bool isValid() const
	{
		return Filename.IsNotEmpty() && !bOldVersion && !bMissingWads;
	}
};


enum EMenuSounds : int;

struct GameInterface
{
	virtual ~GameInterface() {}
	virtual void faketimerhandler() {} // This is a remnant of older versions, but Blood backend has not updated yet.
	virtual int app_main() = 0;
	virtual bool validate_hud(int) = 0;
	virtual void set_hud_layout(int size) = 0;
	virtual void set_hud_scale(int size) = 0;
	virtual bool mouseInactiveConditional(bool condition) { return condition; }
	virtual FString statFPS() { return "FPS display not available"; }
	virtual GameStats getStats() { return {}; }
	virtual void DrawNativeMenuText(int fontnum, int state, int xpos, int ypos, float fontscale, const char* text, int flags) {}
	virtual void MainMenuOpened() {}
	virtual void MenuOpened() {}
	virtual void MenuClosed() {}
	virtual void MenuSound(EMenuSounds snd) {}
	virtual bool CanSave() { return true; }
	virtual void CustomMenuSelection(int menu, int item) {}
	virtual void StartGame(FGameStartup& gs) {}
	virtual FSavegameInfo GetSaveSig() { return { "", 0, 0}; }
	virtual bool DrawSpecialScreen(const DVector2 &origin, int tilenum) { return false; }
	virtual void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position) {}
	virtual void DrawMenuCaption(const DVector2& origin, const char* text) {}
	virtual bool SaveGame(FSaveGameNode*) { return false; }
	virtual bool LoadGame(FSaveGameNode*) { return false; }
};

extern GameInterface* gi;
extern double g_beforeSwapTime;


void ImGui_Begin_Frame();


#endif // baselayer_h_

