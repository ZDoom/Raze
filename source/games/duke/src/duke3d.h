//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef duke3d_h_
#define duke3d_h_

// JBF
#include "baselayer.h"
#include "build.h"

#include "compat.h"

#include "pragmas.h"

#include "polymost.h"
#include "gamecvars.h"
#include "menu/menu.h"

BEGIN_DUKE_NS

extern int32_t g_fakeMultiMode;

#define VOLUMEALL           (g_Shareware == 0)
#define PLUTOPAK            (true)//g_scriptVersion >= 14)
#define VOLUMEONE           (g_Shareware == 1)

// increase by 3, because atomic GRP adds 1, and Shareware adds 2
// Non-Lua build
# define BYTEVERSION_EDUKE32      336

//#define BYTEVERSION_13      27
//#define BYTEVERSION_14      116
//#define BYTEVERSION_15      117
#define BYTEVERSION         (BYTEVERSION_EDUKE32+(PLUTOPAK?1:(VOLUMEONE<<1)))

#define NUMPAGES            1

#define RECSYNCBUFSIZ       2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ         256

#define MAXLEVELS           64
#define MAXGAMETYPES        16

enum {
    MUS_FIRST_SPECIAL = MAXVOLUMES*MAXLEVELS,

    MUS_INTRO = MUS_FIRST_SPECIAL,
    MUS_BRIEFING = MUS_FIRST_SPECIAL + 1,
    MUS_LOADING = MUS_FIRST_SPECIAL + 2,
};

////////// TIMING CONSTANTS //////////
// The number of 'totalclock' increments per second:
#define TICRATE             120
// The number of game state updates per second:
#define REALGAMETICSPERSEC  30
// The number of 'totalclock' increments per game state update:
// NOTE: calling a game state update a 'frame' is really weird.
// (This used to be TICRATE/GAMETICSPERSEC, which was 120/26 = 4.615~ truncated
// to 4 by integer division.)
#define TICSPERFRAME        (TICRATE/REALGAMETICSPERSEC)
// Used as a constant to satisfy all of the calculations written with ticrate =
// 26 in mind:
#define GAMETICSPERSEC      26


#define PACKBUF_SIZE        32768

#define TILE_VIEWSCR        (MAXTILES-5)
EDUKE32_STATIC_ASSERT(7 <= MAXTILES-MAXUSERTILES);

// sprites with these statnums should be considered for fixing
#define ROTFIXSPR_STATNUMP(k) ((k)==STAT_DEFAULT || (k)==STAT_STANDABLE || (k)==STAT_FX || \
                            (k)==STAT_FALLER || (k)==STAT_LIGHT)
#define ROTFIXSPR_MAGIC 0x18190000

// JBF 20040604: sync is a function on some platforms
#define sync                dsync

// Uncomment the following to remove calls to a.nasm functions with the GL renderers
// so that debugging with valgrind --smc-check=none is possible:
//#define DEBUG_VALGRIND_NO_SMC

END_DUKE_NS

#include "zz_actors.h"
#include "common_game.h"
#include "gamecontrol.h"
#include "game.h"
#include "gamedef.h"
#include "gameexec.h"
#include "gamevar.h"
#include "global.h"
#include "inv.h"
#include "macros.h"
#include "namesdyn.h"
#include "net.h"
#include "player.h"
#include "quotes.h"
#include "rts.h"
#include "zz_text.h"
#include "sector.h"
#include "sounds.h"
#include "soundefs.h"

BEGIN_DUKE_NS

extern FFont* IndexFont;
extern FFont* DigiFont;

// Order is that of EDuke32 by necessity because it exposes the key binds to scripting  by index instead of by name.
enum GameFunction_t
{
	gamefunc_Move_Forward,
	gamefunc_Move_Backward,
	gamefunc_Turn_Left,
	gamefunc_Turn_Right,
	gamefunc_Strafe,
	gamefunc_Fire,
	gamefunc_Open,
	gamefunc_Run,
	gamefunc_Alt_Fire,	// Duke3D, Blood
	gamefunc_Jump,
	gamefunc_Crouch,
	gamefunc_Look_Up,
	gamefunc_Look_Down,
	gamefunc_Look_Left,
	gamefunc_Look_Right,
	gamefunc_Strafe_Left,
	gamefunc_Strafe_Right,
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_Weapon_1, // CCMD
	gamefunc_Weapon_2, // CCMD
	gamefunc_Weapon_3, // CCMD
	gamefunc_Weapon_4, // CCMD
	gamefunc_Weapon_5, // CCMD
	gamefunc_Weapon_6, // CCMD
	gamefunc_Weapon_7, // CCMD
	gamefunc_Weapon_8, // CCMD
	gamefunc_Weapon_9, // CCMD
	gamefunc_Weapon_10, // CCMD
	gamefunc_Inventory, // CCMD
	gamefunc_Inventory_Left, // CCMD
	gamefunc_Inventory_Right, // CCMD
	gamefunc_Holo_Duke, // CCMD			// Duke3D, RR
	gamefunc_Jetpack, // CCMD
	gamefunc_NightVision, // CCMD
	gamefunc_MedKit, // CCMD
	gamefunc_TurnAround,
	gamefunc_SendMessage,
	gamefunc_Map, // CCMD
	gamefunc_Shrink_Screen, // CCMD
	gamefunc_Enlarge_Screen, // CCMD
	gamefunc_Center_View, // CCMD
	gamefunc_Holster_Weapon, // CCMD
	gamefunc_Show_Opponents_Weapon, // CCMD
	gamefunc_Map_Follow_Mode, // CCMD
	gamefunc_See_Coop_View, // CCMD
	gamefunc_Mouse_Aiming, // CCMD
	gamefunc_Toggle_Crosshair, // CCMD
	gamefunc_Steroids, // CCMD
	gamefunc_Quick_Kick, // CCMD
	gamefunc_Next_Weapon, // CCMD
	gamefunc_Previous_Weapon, // CCMD
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Last_Weapon, // CCMD
	gamefunc_Alt_Weapon,
	gamefunc_Third_Person_View, // CCMD
	gamefunc_Show_DukeMatch_Scores, // CCMD
	gamefunc_Toggle_Crouch,	// This is the last one used by EDuke32.
	NUM_ACTIONS
};

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Duke"; }
	int app_main() override;
	void UpdateScreenSize() override;
	void FreeGameData() override;
	bool GenerateSavePic() override;
	bool validate_hud(int) override;
	void set_hud_layout(int size) override;
	void set_hud_scale(int size) override;
	FString statFPS() override;
	GameStats getStats() override;
	void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) override;
	void MenuOpened() override;
	void MenuSound(EMenuSounds snd) override;
	void MenuClosed() override;
	bool CanSave() override;
	void StartGame(FNewGameStartup& gs) override;
	FSavegameInfo GetSaveSig() override;
	void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg) override;
	double SmallFontScale() override { return isRR() ? 0.5 : 1.; }
	void DrawMenuCaption(const DVector2& origin, const char* text) override;
	bool SaveGame(FSaveGameNode*) override;
	bool LoadGame(FSaveGameNode*) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	int GetStringTile(int font, const char* t, int f) override;
};

END_DUKE_NS

#endif
