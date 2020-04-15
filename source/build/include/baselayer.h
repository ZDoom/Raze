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

extern int32_t swapcomplete;

EXTERN_CVAR(Bool, r_usenewaspect)

// video
extern int32_t newaspect_enable;
extern int32_t r_fpgrouscan;
extern int32_t setaspect_new_use_dimen;
extern int32_t xres, yres, bpp;
extern double refreshfreq;

int32_t videoCheckMode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced);
int32_t videoSetMode(int32_t x, int32_t y, int32_t c, int32_t fs);
void    videoGetModes(void);
void    videoShowFrame(int32_t);
int32_t videoUpdatePalette(int32_t start, int32_t num);

extern int32_t qsetmode;

#define in3dmode() (qsetmode==200)

extern int32_t g_logFlushWindow;

void mouseGrabInput(bool grab);

void getScreen(uint8_t* imgBuf);


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
	virtual const char* Name() { return "$"; }
	virtual ~GameInterface() {}
	virtual bool GenerateSavePic() { return false; }
	virtual void faketimerhandler() {} // This is a remnant of older versions, but Blood backend has not updated yet.
	virtual int app_main() = 0;
	virtual void UpdateScreenSize() {}
	virtual void FreeGameData() {}
	virtual bool validate_hud(int) { return true; }
	virtual void set_hud_layout(int size) = 0;
	virtual void set_hud_scale(int size) {}
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
	virtual bool CleanupForLoad() { return true; }
	virtual void DoPrintMessage(int prio, const char*) {}
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
	virtual int GetStringTile(int font, const char* t, int f) { return -1; }

};

extern GameInterface* gi;
extern double g_beforeSwapTime;


void ImGui_Begin_Frame();


#endif // baselayer_h_

