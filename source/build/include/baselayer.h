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

extern char appactive;
extern char modechange;
extern char nogl;

extern int32_t swapcomplete;

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

void videoBeginDrawing(void);


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

// mouse

// joystick

typedef struct
{
    int32_t *pAxis;
    int32_t *pHat;
    void (*pCallback)(int32_t, int32_t);
    int32_t  bits;
    int32_t  numAxes;
    int32_t  numBalls;
    int32_t  numButtons;
    int32_t  numHats;
    int32_t  isGameController;
} controllerinput_t;

extern controllerinput_t joystick;

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

extern int32_t g_logFlushWindow;

void I_StartTic();

inline int32_t handleevents(void)
{
	timerUpdateClock();
	I_StartTic();
	return 0;
}

void mouseGrabInput(bool grab);

extern int32_t inputchecked;

void getScreen(uint8_t* imgBuf);


int32_t wm_msgbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));
int32_t wm_ynbox(const char *name, const char *fmt, ...) ATTRIBUTE((format(printf,2,3)));

#include "print.h"

struct GameStats
{
	int kill, tkill;
	int secret, tsecret;
	int timesecnd;
	int frags;
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
	virtual FString statFPS() { return "FPS display not available"; }
	virtual GameStats getStats() { return {}; }
	virtual void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) {}
	virtual void MainMenuOpened() {}
	virtual void MenuOpened() {}
	virtual void MenuClosed() {}
	virtual void MenuSound(EMenuSounds snd) {}
	virtual bool CanSave() { return true; }
	virtual void CustomMenuSelection(int menu, int item) {}
	virtual void StartGame(FGameStartup& gs) {}
	virtual FSavegameInfo GetSaveSig() { return { "", 0, 0}; }
	virtual bool DrawSpecialScreen(const DVector2 &origin, int tilenum) { return false; }
	virtual void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool withbg = true) {}
	virtual void DrawMenuCaption(const DVector2& origin, const char* text) {}
	virtual bool SaveGame(FSaveGameNode*) { return false; }
	virtual bool LoadGame(FSaveGameNode*) { return false; }
	virtual void DoPrintMessage(int prio, const char*) = 0;
	void PrintMessage(int prio, const char*fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		FString f;
		f.VFormat(fmt, ap);
		DoPrintMessage(prio, f);
	}
	virtual void DrawPlayerSprite(const DVector2& origin, bool onteam) {}
	virtual void QuitToTitle() {}
	virtual void SetAmbience(bool on) {}
	virtual FString GetCoordString() { return "'stat coord' not implemented"; }

};

extern GameInterface* gi;
extern double g_beforeSwapTime;


void ImGui_Begin_Frame();


#endif // baselayer_h_

