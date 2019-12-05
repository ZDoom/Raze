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

#include "ns.h"	// Must come before everything else!

#include "cheats.h"
#include "compat.h"
#include "demo.h"
#include "duke3d.h"
#include "input.h"
#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "superfasthash.h"
#include "gamecvars.h"
#include "gamecontrol.h"
#include "c_bind.h"
#include "../../glbackend/glbackend.h"

bool ShowOptionMenu();

namespace ImGui
{
	void ShowDemoWindow(bool*);
}

BEGIN_DUKE_NS

#if 0

// common positions
#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100

#define USERMAPENTRYLENGTH 25


/*
MenuEntry_t is passed in arrays of pointers so that the callback function
that is called when an entry is modified or activated can test equality of the current
entry pointer directly against the known ones, instead of relying on an ID number.

That way, individual menu entries can be ifdef'd out painlessly.
*/


// Zhe menu code lacks flexibility, it can either be hardwired to ints or to CVARs.
// Since CVARs are more important it means that these need to be implemented as CVARs even though they are just temporaries.
// By giving them no name we ensure that they remain outside the CVAR system.
CVAR_UNAMED(Int, newresolution)
CVAR_UNAMED(Int, newrendermode)
CVAR_UNAMED(Int, newfullscreen)
CVAR_UNAMED(Int, newvsync)
CVAR_UNAMED(Int, newborderless)

enum resflags_t {
    RES_FS  = 0x1,
    RES_WIN = 0x2,
};

#define MAXRESOLUTIONSTRINGLENGTH 19

typedef struct resolution_t {
    int32_t xdim, ydim;
    int32_t flags;
    int32_t bppmax;
    char name[MAXRESOLUTIONSTRINGLENGTH];
} resolution_t;

resolution_t resolution[MAXVALIDMODES];

static char const *MEOSN_VIDEOSETUP_RESOLUTION[MAXVALIDMODES];
static MenuOptionSet_t MEOS_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTIONSETDYN( MEOSN_VIDEOSETUP_RESOLUTION, NULL, 0, 0x0 );
static MenuOption_t MEO_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RESOLUTION, &newresolution );
static MenuEntry_t ME_VIDEOSETUP_RESOLUTION = MAKE_MENUENTRY( "Resolution:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RESOLUTION, Option );

#ifdef USE_OPENGL
static char const *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "OpenGL", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER, 0x2 );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( "Renderer:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_RENDERER, Option );
#endif

static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_YesNo, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( "Windowed:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_FULLSCREEN, Option );

static char const *MEOSN_VIDEOSETUP_BORDERLESS [] = { "No", "Yes", "Auto", };
static int32_t MEOSV_VIDEOSETUP_BORDERLESS [] = { 0, 1, 2, };
static MenuOptionSet_t MEOS_VIDEOSETUP_BORDERLESS = MAKE_MENUOPTIONSET(MEOSN_VIDEOSETUP_BORDERLESS, MEOSV_VIDEOSETUP_BORDERLESS, 0x2);
static MenuOption_t MEO_VIDEOSETUP_BORDERLESS = MAKE_MENUOPTION(&MF_Redfont, &MEOS_VIDEOSETUP_BORDERLESS, &newborderless);
static MenuEntry_t ME_VIDEOSETUP_BORDERLESS = MAKE_MENUENTRY("Borderless:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_BORDERLESS, Option);

static char const *MEOSN_VIDEOSETUP_VSYNC [] = { "Adaptive", "Off", "On", };
static int32_t MEOSV_VIDEOSETUP_VSYNC [] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_VIDEOSETUP_VSYNC = MAKE_MENUOPTIONSET(MEOSN_VIDEOSETUP_VSYNC, MEOSV_VIDEOSETUP_VSYNC, 0x2);
static MenuOption_t MEO_VIDEOSETUP_VSYNC = MAKE_MENUOPTION(&MF_Redfont, &MEOS_VIDEOSETUP_VSYNC, &newvsync);
static MenuEntry_t ME_VIDEOSETUP_VSYNC = MAKE_MENUENTRY("VSync:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_VIDEOSETUP_VSYNC, Option);



//static char const *MEOSN_VIDEOSETUP_FRAMELIMIT [] = { "None", "30 fps", "60 fps", "75 fps", "100 fps", "120 fps", "144 fps", "165 fps", "240 fps" };

static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );





#ifdef USE_OPENGL
static MenuLink_t MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST = { MENU_POLYMOST, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST = MAKE_MENUENTRY( "Polymost setup", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_ADVANCED_GL_POLYMOST, Link );
#endif

#ifndef EDUKE32_ANDROID_MENU
static MenuLink_t MEO_DISPLAYSETUP_VIDEOSETUP = { MENU_VIDEOSETUP, MA_Advance, };
static MenuEntry_t ME_DISPLAYSETUP_VIDEOSETUP = MAKE_MENUENTRY( "Video mode", &MF_Redfont, &MEF_BigOptionsRt, &MEO_DISPLAYSETUP_VIDEOSETUP, Link );
#endif


};

static MenuEntry_t *MEL_OPTIONS[] = {
#ifndef EDUKE32_SIMPLE_MENU
    &ME_OPTIONS_GAMESETUP,
#endif
    &ME_OPTIONS_DISPLAYSETUP,
    &ME_OPTIONS_SOUNDSETUP,
#ifndef EDUKE32_ANDROID_MENU
#ifndef EDUKE32_SIMPLE_MENU
    &ME_OPTIONS_PLAYERSETUP,
#endif
    &ME_OPTIONS_CONTROLS,
#else
    &ME_OPTIONS_TOUCHSETUP,
#endif
#ifdef EDUKE32_SIMPLE_MENU
    &ME_GAMESETUP_SAVESETUP,
    &ME_OPTIONS_CHEATS
#endif
};

static MenuEntry_t *MEL_CONTROLS[] = {
#ifndef EDUKE32_ANDROID_MENU
    &ME_OPTIONS_KEYBOARDSETUP,
    &ME_OPTIONS_MOUSESETUP,
    &ME_OPTIONS_JOYSTICKSETUP,
#else
    &ME_OPTIONS_TOUCHSETUP,
#endif
    &ME_Space6_Redfont,
    &ME_GAMESETUP_AIM_AUTO,
    &ME_GAMESETUP_ALWAYS_RUN,
    &ME_GAMESETUP_WEAPSWITCH_PICKUP,
#ifdef EDUKE32_ANDROID_MENU
    &ME_GAMESETUP_QUICKSWITCH,
    &ME_GAMESETUP_CROUCHLOCK,
#endif
};

static MenuEntry_t *MEL_CHEATS[ARRAY_SIZE(ME_CheatCodes)+1] = {
    &ME_ENTERCHEAT,
};

static MenuEntry_t *MEL_VIDEOSETUP[] = {
    &ME_VIDEOSETUP_RESOLUTION,
#ifdef USE_OPENGL
    &ME_VIDEOSETUP_RENDERER,
#endif
    &ME_VIDEOSETUP_FULLSCREEN,
    &ME_VIDEOSETUP_BORDERLESS,
    &ME_VIDEOSETUP_VSYNC,
    &ME_Space4_Redfont,
    &ME_VIDEOSETUP_APPLY,
};
static MenuEntry_t *MEL_DISPLAYSETUP[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
#ifndef EDUKE32_ANDROID_MENU
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_VOXELS,
    &ME_DISPLAYSETUP_FOV,
#endif
};

#ifdef USE_OPENGL
static MenuEntry_t *MEL_DISPLAYSETUP_GL[] = {
    &ME_DISPLAYSETUP_SCREENSETUP,
    &ME_DISPLAYSETUP_COLORCORR,
    &ME_DISPLAYSETUP_VIDEOSETUP,
    &ME_DISPLAYSETUP_ASPECTRATIO,
    &ME_DISPLAYSETUP_VOXELS,
    &ME_DISPLAYSETUP_FOV,
    &ME_DISPLAYSETUP_TEXFILTER,
    &ME_DISPLAYSETUP_ANISOTROPY,
    &ME_DISPLAYSETUP_ADVANCED_GL_POLYMOST,
#endif
};




#ifdef USE_OPENGL
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_hightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( "True color textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_HIGHTILE, Option );


static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &r_precache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( "Pre-load map textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_PRECACHE, Option );
# ifndef EDUKE32_GLES
static char const *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compr.", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( MEOSN_RENDERERSETUP_TEXCACHE, NULL, 0x2 );
# endif
# ifdef USE_GLEXT
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_detailmapping );
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( "Detail textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_DETAILTEX, Option );
static MenuOption_t MEO_RENDERERSETUP_GLOWTEX = MAKE_MENUOPTION(&MF_Bluefont, &MEOS_NoYes, &hw_glowmapping);
static MenuEntry_t ME_RENDERERSETUP_GLOWTEX = MAKE_MENUENTRY("Glow textures:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_GLOWTEX, Option);
# endif
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &hw_models );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( "3D models:", &MF_Bluefont, &MEF_SmallOptions, &MEO_RENDERERSETUP_MODELS, Option );
#endif


#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP_POLYMOST[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_PRECACHE,
# ifdef USE_GLEXT
    &ME_RENDERERSETUP_DETAILTEX,
    &ME_RENDERERSETUP_GLOWTEX,
# endif
    &ME_Space4_Bluefont,
    &ME_RENDERERSETUP_MODELS,
};

#endif

#ifdef EDUKE32_ANDROID_MENU
#define MINVIS 1.f
#else
#define MINVIS 0.125f
#endif
#ifndef EDUKE32_SIMPLE_MENU
static MenuRangeFloat_t MEO_COLCORR_AMBIENT = MAKE_MENURANGE( &r_ambientlight, &MF_Bluefont, MINVIS, 4.f, 0.f, 32, 1 );
static MenuEntry_t ME_COLCORR_AMBIENT = MAKE_MENUENTRY( "Visibility:", &MF_Redfont, &MEF_ColorCorrect, &MEO_COLCORR_AMBIENT, RangeFloat );
#endif
static MenuEntry_t *MEL_COLCORR[] = {
    &ME_COLCORR_GAMMA,
#ifndef EDUKE32_ANDROID_MENU
    &ME_COLCORR_CONTRAST,
    &ME_COLCORR_BRIGHTNESS,
#endif
#ifndef EDUKE32_SIMPLE_MENU
    &ME_COLCORR_AMBIENT,
#endif
    &ME_Space8_Redfont,
    &ME_COLCORR_RESET,
};

static MenuEntry_t *MEL_SCREENSETUP[] = {
#ifdef EDUKE32_ANDROID_MENU
    &ME_SCREENSETUP_STATUSBARONTOP,
#endif
    &ME_SCREENSETUP_SCREENSIZE,
    &ME_SCREENSETUP_SBARSIZE,

    &ME_SCREENSETUP_CROSSHAIR,
    &ME_SCREENSETUP_CROSSHAIRSIZE,

    &ME_SCREENSETUP_LEVELSTATS,
    &ME_SCREENSETUP_TEXTSIZE,

    &ME_SCREENSETUP_SHOWPICKUPMESSAGES,
};

CVAR_UNAMED(Int, soundrate)
CVAR_UNAMED(Int, soundvoices)
CVAR_UNAMED(Int, musicdevice)
static MenuOption_t MEO_SOUND = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &snd_enabled );
static MenuEntry_t ME_SOUND = MAKE_MENUENTRY( "Sound:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND, Option );

static MenuOption_t MEO_SOUND_MUSIC = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &mus_enabled );
static MenuEntry_t ME_SOUND_MUSIC = MAKE_MENUENTRY( "Music:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_MUSIC, Option );

static char const s_Volume[] = "Volume:";

static MenuRangeInt32_t MEO_SOUND_VOLUME_FX = MAKE_MENURANGE( &snd_fxvolume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_FX = MAKE_MENUENTRY( s_Volume, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SOUND_VOLUME_FX, RangeInt32 );

static MenuRangeInt32_t MEO_SOUND_VOLUME_MUSIC = MAKE_MENURANGE( &mus_volume, &MF_Redfont, 0, 255, 0, 33, 2 );
static MenuEntry_t ME_SOUND_VOLUME_MUSIC = MAKE_MENUENTRY( s_Volume, &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SOUND_VOLUME_MUSIC, RangeInt32 );

#ifndef EDUKE32_STANDALONE
static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_NoYes, NULL);
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY( "Duke talk:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option );
#else
static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION(&MF_Redfont, &MEOS_YesNo, NULL);
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY("Silent protagonist:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_DUKETALK, Option);
#endif

static char const *MEOSN_SOUND_SAMPLINGRATE[] = { "22050Hz", "44100Hz", "48000Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE, 0x3 );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( "Sample rate:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_SAMPLINGRATE, Option );

#ifndef EDUKE32_SIMPLE_MENU
static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &soundvoices, &MF_Redfont, 16, 128, 0, 8, 1 );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( "Voices:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_NUMVOICES, RangeInt32 );
#endif

static char const *MEOSN_SOUND_MIDIDRIVER[] = {
    "OPL3",
#ifdef _WIN32
    "Windows",
#endif
};
static int32_t MEOSV_SOUND_MIDIDRIVER[] = {
    ASS_OPL3,
#ifdef _WIN32
    ASS_WinMM,
#endif
};

static MenuOptionSet_t MEOS_SOUND_MIDIDRIVER = MAKE_MENUOPTIONSET( MEOSN_SOUND_MIDIDRIVER, MEOSV_SOUND_MIDIDRIVER, 0x2 );
static MenuOption_t MEO_SOUND_MIDIDRIVER = MAKE_MENUOPTION( &MF_Redfont, &MEOS_SOUND_MIDIDRIVER, &musicdevice );
static MenuEntry_t ME_SOUND_MIDIDRIVER = MAKE_MENUENTRY( "MIDI driver:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SOUND_MIDIDRIVER, Option );

static MenuEntry_t ME_SOUND_RESTART = MAKE_MENUENTRY( "Apply Changes", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_NULL, Link );

#ifndef EDUKE32_SIMPLE_MENU
static MenuLink_t MEO_ADVSOUND = { MENU_ADVSOUND, MA_Advance, };
static MenuEntry_t ME_SOUND_ADVSOUND = MAKE_MENUENTRY( "Advanced", &MF_Redfont, &MEF_BigOptionsRt, &MEO_ADVSOUND, Link );
#endif

static MenuEntry_t *MEL_SOUND[] = {
    &ME_SOUND,
    &ME_SOUND_VOLUME_FX,
    &ME_SOUND_MUSIC,
    &ME_SOUND_VOLUME_MUSIC,
    &ME_SOUND_DUKETALK,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_SOUND_ADVSOUND,
#endif
};

static MenuEntry_t *MEL_ADVSOUND[] = {
    &ME_SOUND_SAMPLINGRATE,
    &ME_Space2_Redfont,
#ifndef EDUKE32_SIMPLE_MENU
    &ME_SOUND_NUMVOICES,
    &ME_Space2_Redfont,
#endif
    &ME_SOUND_MIDIDRIVER,
    &ME_SOUND_RESTART,
};


static MenuOption_t MEO_SAVESETUP_AUTOSAVE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OffOn, &cl_autosave );
static MenuEntry_t ME_SAVESETUP_AUTOSAVE = MAKE_MENUENTRY( "Checkpoints:", &MF_Redfont, &MEF_BigOptionsRt, &MEO_SAVESETUP_AUTOSAVE, Option );

static MenuOption_t MEO_SAVESETUP_AUTOSAVEDELETION = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &cl_autosavedeletion );
static MenuEntry_t ME_SAVESETUP_AUTOSAVEDELETION = MAKE_MENUENTRY( "Auto-Delete:", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SAVESETUP_AUTOSAVEDELETION, Option );
static MenuRangeInt32_t MEO_SAVESETUP_MAXAUTOSAVES = MAKE_MENURANGE( &cl_maxautosaves, &MF_Redfont, 1, 10, 0, 10, 1 );
static MenuEntry_t ME_SAVESETUP_MAXAUTOSAVES = MAKE_MENUENTRY( "Limit:", &MF_Redfont, &MEF_BigOptions_Apply, &MEO_SAVESETUP_MAXAUTOSAVES, RangeInt32 );

static MenuEntry_t ME_SAVESETUP_CLEANUP = MAKE_MENUENTRY( "Clean Up Saves", &MF_Redfont, &MEF_BigOptionsRt, &MEO_NULL, Link );

static MenuEntry_t *MEL_SAVESETUP[] = {
    &ME_SAVESETUP_AUTOSAVE,
    &ME_SAVESETUP_AUTOSAVEDELETION,
    &ME_SAVESETUP_MAXAUTOSAVES,
    &ME_SAVESETUP_CLEANUP,
};


#define NoTitle NULL

#define MAKE_MENUMENU(Title, Format, Entries) { Title, Format, Entries, ARRAY_SIZE(Entries), 0, 0, 0 }
#define MAKE_MENUMENU_CUSTOMSIZE(Title, Format, Entries) { Title, Format, Entries, 0, 0, 0, 0 }

#ifndef EDUKE32_SIMPLE_MENU
static MenuMenu_t M_GAMESETUP = MAKE_MENUMENU( "Game Setup", &MMF_BigOptions, MEL_GAMESETUP );
#endif
static MenuMenu_t M_OPTIONS = MAKE_MENUMENU( s_Options, &MMF_Top_Options, MEL_OPTIONS );
static MenuMenu_t M_VIDEOSETUP = MAKE_MENUMENU( "Video Mode", &MMF_BigOptions, MEL_VIDEOSETUP );
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( "Configure Controls", &MMF_Top_Options, MEL_KEYBOARDSETUP );
static MenuMenu_t M_CONTROLS = MAKE_MENUMENU( "Control Setup", &MMF_BigOptions, MEL_CONTROLS );
static MenuMenu_t M_CHEATS = MAKE_MENUMENU( "Cheats", &MMF_SmallOptions, MEL_CHEATS );
static MenuMenu_t M_MOUSESETUP = MAKE_MENUMENU( "Mouse Setup", &MMF_BigOptions, MEL_MOUSESETUP );
static MenuMenu_t M_JOYSTICKSETUP = MAKE_MENUMENU( "Gamepad Setup", &MMF_BigOptions, MEL_JOYSTICKSETUP );
static MenuMenu_t M_JOYSTICKAXES = MAKE_MENUMENU( "Gamepad Axes", &MMF_BigSliders, MEL_JOYSTICKAXES );
static MenuMenu_t M_KEYBOARDKEYS = MAKE_MENUMENU( "Key Configuration", &MMF_KeyboardSetupFuncs, MEL_KEYBOARDSETUPFUNCS );
static MenuMenu_t M_MOUSEADVANCED = MAKE_MENUMENU( "Advanced Mouse", &MMF_BigSliders, MEL_MOUSEADVANCED );
static MenuMenu_t M_JOYSTICKAXIS = MAKE_MENUMENU( NULL, &MMF_BigSliders, MEL_JOYSTICKAXIS );
#ifdef USE_OPENGL
static MenuMenu_t M_RENDERERSETUP_POLYMOST = MAKE_MENUMENU( "Polymost Setup", &MMF_SmallOptions, MEL_RENDERERSETUP_POLYMOST );
#endif
static MenuMenu_t M_COLCORR = MAKE_MENUMENU( "Color Correction", &MMF_ColorCorrect, MEL_COLCORR );
static MenuMenu_t M_SCREENSETUP = MAKE_MENUMENU( "HUD Setup", &MMF_BigOptions, MEL_SCREENSETUP );
static MenuMenu_t M_DISPLAYSETUP = MAKE_MENUMENU( "Display Setup", &MMF_BigOptions, MEL_DISPLAYSETUP );
static MenuMenu_t M_LOAD = MAKE_MENUMENU_CUSTOMSIZE( s_LoadGame, &MMF_LoadSave, MEL_LOAD );
static MenuMenu_t M_SAVE = MAKE_MENUMENU_CUSTOMSIZE( s_SaveGame, &MMF_LoadSave, MEL_SAVE );
static MenuMenu_t M_SOUND = MAKE_MENUMENU( "Sound Setup", &MMF_BigOptions, MEL_SOUND );
static MenuMenu_t M_ADVSOUND = MAKE_MENUMENU( "Advanced Sound", &MMF_BigOptions, MEL_ADVSOUND );
static MenuMenu_t M_SAVESETUP = MAKE_MENUMENU( "Save Setup", &MMF_BigOptions, MEL_SAVESETUP );
static MenuMenu_t M_NETWORK = MAKE_MENUMENU( "Network Game", &MMF_Top_Joystick_Network, MEL_NETWORK );
static MenuMenu_t M_PLAYER = MAKE_MENUMENU( "Player Setup", &MMF_SmallOptions, MEL_PLAYER );
static MenuMenu_t M_MACROS = MAKE_MENUMENU( "Multiplayer Macros", &MMF_Macros, MEL_MACROS );
static MenuMenu_t M_NETHOST = MAKE_MENUMENU( "Host Network Game", &MMF_SmallOptionsNarrow, MEL_NETHOST );
static MenuMenu_t M_NETOPTIONS = MAKE_MENUMENU( "Net Game Options", &MMF_NetSetup, MEL_NETOPTIONS );
static MenuMenu_t M_NETJOIN = MAKE_MENUMENU( "Join Network Game", &MMF_SmallOptionsNarrow, MEL_NETJOIN );

static MenuPanel_t M_CREDITS4 = { "About " APPNAME, MENU_CREDITS3, MA_Return, MENU_CREDITS5, MA_Advance, };
static MenuPanel_t M_CREDITS5 = { "About " APPNAME, MENU_CREDITS4, MA_Return, MENU_CREDITS, MA_Advance, };

#define CURSOR_CENTER_2LINE { MENU_MARGIN_CENTER<<16, 120<<16, }
#define CURSOR_CENTER_3LINE { MENU_MARGIN_CENTER<<16, 129<<16, }
#define CURSOR_BOTTOMRIGHT { 304<<16, 186<<16, }

static MenuVerify_t M_SAVECLEANVERIFY = { CURSOR_CENTER_3LINE, MENU_SAVESETUP, MA_None, };
static MenuVerify_t M_QUIT = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_QUITTOTITLE = { CURSOR_CENTER_2LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_LOADVERIFY = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };
static MenuVerify_t M_LOADDELVERIFY = { CURSOR_CENTER_3LINE, MENU_LOAD, MA_None, };
static MenuVerify_t M_NEWVERIFY = { CURSOR_CENTER_2LINE, MENU_EPISODE, MA_Advance, };
static MenuVerify_t M_SAVEVERIFY = { CURSOR_CENTER_2LINE, MENU_SAVE, MA_None, };
static MenuVerify_t M_SAVEDELVERIFY = { CURSOR_CENTER_3LINE, MENU_SAVE, MA_None, };
static MenuVerify_t M_RESETPLAYER = { CURSOR_CENTER_3LINE, MENU_CLOSE, MA_None, };

static MenuVerify_t M_COLCORRRESETVERIFY = { CURSOR_CENTER_2LINE, MENU_COLCORR, MA_None, };
static MenuVerify_t M_KEYSRESETVERIFY = { CURSOR_CENTER_2LINE, MENU_KEYBOARDSETUP, MA_None, };
static MenuVerify_t M_KEYSCLASSICVERIFY = { CURSOR_CENTER_2LINE, MENU_KEYBOARDSETUP, MA_None, };
static MenuVerify_t M_JOYSTANDARDVERIFY = { CURSOR_CENTER_2LINE, MENU_JOYSTICKSETUP, MA_None, };
static MenuVerify_t M_JOYPROVERIFY = { CURSOR_CENTER_2LINE, MENU_JOYSTICKSETUP, MA_None, };
static MenuVerify_t M_JOYCLEARVERIFY = { CURSOR_CENTER_2LINE, MENU_JOYSTICKSETUP, MA_None, };

static MenuMessage_t M_NETWAITMASTER = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_NETWAITVOTES = { CURSOR_BOTTOMRIGHT, MENU_NULL, MA_None, };
static MenuMessage_t M_BUYDUKE = { CURSOR_BOTTOMRIGHT, MENU_EPISODE, MA_Return, };

static MenuTextForm_t M_ADULTPASSWORD = { NULL, "Enter Password:", MAXPWLOCKOUT, MTF_Password };
static MenuTextForm_t M_CHEATENTRY = { NULL, "Enter Cheat Code:", MAXCHEATLEN, 0 };
static MenuTextForm_t M_CHEAT_WARP = { NULL, "Enter Warp #:", 3, 0 };
static MenuTextForm_t M_CHEAT_SKILL = { NULL, "Enter Skill #:", 1, 0 };


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void Menu_Init(void)
{
    int32_t i, j, k;

    if (FURY)
    {
        MMF_Top_Skill.pos.x = (320<<15);
        ME_SKILL_TEMPLATE.format = &MEF_LeftMenu;
    }


    ++k;
    MEOS_NETOPTIONS_GAMETYPE.numOptions = k;
    if (NAM_WW2GI)
        ME_NETOPTIONS_MONSTERS.name = "Enemies";

    // prepare cheats
    for (i = 0; i < NUMCHEATFUNCS; ++i)
        MEL_CHEATS[i+1] = &ME_CheatCodes[i];

 

    // prepare shareware
    if (VOLUMEONE)
    {
        // blue out episodes beyond the first
        for (i = 1; i < g_volumeCnt; ++i)
        {
            if (MEL_EPISODE[i])
            {
                ME_EPISODE[i].entry = &MEO_EPISODE_SHAREWARE;
                ME_EPISODE[i].flags |= MEF_LookDisabled;
            }
        }
        M_EPISODE.numEntries = g_volumeCnt; // remove User Map (and spacer)
        MEOS_NETOPTIONS_EPISODE.numOptions = 1;
        MenuEntry_DisableOnCondition(&ME_NETOPTIONS_EPISODE, 1);
    }

    // prepare pre-Atomic
    if (!VOLUMEALL || !PLUTOPAK)
    {
        // prepare credits
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = s_Credits;
    }

    MenuEntry_HideOnCondition(&ME_MAIN_HELP, G_GetLogoFlags() & LOGO_NOHELP);
#ifndef EDUKE32_SIMPLE_MENU
    MenuEntry_HideOnCondition(&ME_MAIN_CREDITS, G_GetLogoFlags() & LOGO_NOCREDITS);
#endif
}



static void Menu_Pre(MenuID_t cm)
{
    int32_t i;
    auto ps = g_player[myconnectindex].ps;

    switch (cm)
    {

    case MENU_GAMESETUP:
        MEO_GAMESETUP_DEMOREC.options = (ps->gm&MODE_GAME) ? &MEOS_DemoRec : &MEOS_OffOn;
        MenuEntry_DisableOnCondition(&ME_GAMESETUP_DEMOREC, (ps->gm&MODE_GAME) && m_recstat != 1);
        break;

    case MENU_DISPLAYSETUP:
        MenuEntry_HideOnCondition(&ME_DISPLAYSETUP_VOXELS, !g_haveVoxels);
#ifdef USE_OPENGL
        if (videoGetRenderMode() == REND_CLASSIC)
            MenuMenu_ChangeEntryList(M_DISPLAYSETUP, MEL_DISPLAYSETUP);
        else
            MenuMenu_ChangeEntryList(M_DISPLAYSETUP, MEL_DISPLAYSETUP_GL);

        MEO_SCREENSETUP_SCREENSIZE.steps = !(ud.statusbarflags & STATUSBAR_NONONE) +
                                           !(ud.statusbarflags & STATUSBAR_NOMODERN) +
                                           !(ud.statusbarflags & STATUSBAR_NOMINI) +
                                           !(ud.statusbarflags & STATUSBAR_NOOVERLAY) +
                                           !(ud.statusbarflags & STATUSBAR_NOFULL) +
                                           !(ud.statusbarflags & STATUSBAR_NOSHRINK) * 14;
        MEO_SCREENSETUP_SCREENSIZE.max = MEO_SCREENSETUP_SCREENSIZE.steps - 1;
        if (MEO_SCREENSETUP_SCREENSIZE.steps <= 2 && !(ud.statusbarflags & STATUSBAR_NONONE))
        {
            ME_SCREENSETUP_SCREENSIZE.entry = &MEO_SCREENSETUP_SCREENSIZE_TWO;
            ME_SCREENSETUP_SCREENSIZE.type = Option;
        }
        else
        {
            ME_SCREENSETUP_SCREENSIZE.entry = &MEO_SCREENSETUP_SCREENSIZE;
            ME_SCREENSETUP_SCREENSIZE.type = RangeInt32;
        }
        MenuEntry_HideOnCondition(&ME_SCREENSETUP_SCREENSIZE, (MEO_SCREENSETUP_SCREENSIZE.steps < 2));

        break;

    case MENU_POLYMER:
    case MENU_POLYMOST:
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_PRECACHE, !hw_hightile);
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_DETAILTEX, !hw_hightile);
        MenuEntry_DisableOnCondition(&ME_RENDERERSETUP_GLOWTEX, !hw_hightile);
#endif
        break;

    case MENU_VIDEOSETUP:
    {
        Bmemset(resolution, 0, sizeof(resolution));
        MEOS_VIDEOSETUP_RESOLUTION.numOptions = 0;

        // prepare video setup
        for (int i = 0; i < validmodecnt; ++i)
        {
            int j;

            for (j = 0; j < MEOS_VIDEOSETUP_RESOLUTION.numOptions; ++j)
            {
                if (validmode[i].xdim == resolution[j].xdim && validmode[i].ydim == resolution[j].ydim)
                {
                    resolution[j].flags |= validmode[i].fs ? RES_FS : RES_WIN;
                    Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", resolution[j].xdim, resolution[j].ydim, (resolution[j].flags & RES_FS) ? "" : "Win");
                    MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
                    if (validmode[i].bpp > resolution[j].bppmax)
                        resolution[j].bppmax = validmode[i].bpp;
                    break;
                }
            }

            if (j == MEOS_VIDEOSETUP_RESOLUTION.numOptions) // no match found
            {
                resolution[j].xdim = validmode[i].xdim;
                resolution[j].ydim = validmode[i].ydim;
                resolution[j].bppmax = validmode[i].bpp;
                resolution[j].flags = validmode[i].fs ? RES_FS : RES_WIN;
                Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", resolution[j].xdim, resolution[j].ydim, (resolution[j].flags & RES_FS) ? "" : "Win");
                MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
                ++MEOS_VIDEOSETUP_RESOLUTION.numOptions;
            }
        }

        const int32_t nr = newresolution;

        // don't allow setting fullscreen mode if it's not supported by the resolution
        MenuEntry_DisableOnCondition(&ME_VIDEOSETUP_FULLSCREEN, !(resolution[nr].flags & RES_FS));

        MenuEntry_DisableOnCondition(&ME_VIDEOSETUP_APPLY,
             (xres == resolution[nr].xdim && yres == resolution[nr].ydim &&
              videoGetRenderMode() == newrendermode && fullscreen == newfullscreen
              && vid_vsync == newvsync && r_borderless == newborderless
             )
             || (newrendermode != REND_CLASSIC && resolution[nr].bppmax <= 8));
        MenuEntry_DisableOnCondition(&ME_VIDEOSETUP_BORDERLESS, newfullscreen);
        break;
    }

    case MENU_SOUND:
    case MENU_SOUND_INGAME:
    case MENU_ADVSOUND:
        MenuEntry_DisableOnCondition(&ME_SOUND_VOLUME_FX, !snd_enabled);
        MenuEntry_DisableOnCondition(&ME_SOUND_VOLUME_MUSIC, !mus_enabled);
        MenuEntry_DisableOnCondition(&ME_SOUND_DUKETALK, !snd_enabled);
        MenuEntry_DisableOnCondition(&ME_SOUND_SAMPLINGRATE, !snd_enabled && !mus_enabled);
#ifndef EDUKE32_SIMPLE_MENU
        MenuEntry_DisableOnCondition(&ME_SOUND_NUMVOICES, !snd_enabled);
#endif
        MenuEntry_DisableOnCondition(&ME_SOUND_RESTART, soundrate == snd_mixrate &&
                                                        soundvoices == snd_numvoices);
        break;

    case MENU_SAVESETUP:
        MenuEntry_DisableOnCondition(&ME_SAVESETUP_MAXAUTOSAVES, !cl_autosavedeletion);
        break;

    case MENU_JOYSTICKSETUP:
        MenuEntry_DisableOnCondition(&ME_JOYSTICK_EDITBUTTONS, !CONTROL_JoyPresent || (joystick.numButtons == 0 && joystick.numHats == 0));
        MenuEntry_DisableOnCondition(&ME_JOYSTICK_EDITAXES, !CONTROL_JoyPresent || joystick.numAxes == 0);
        MenuEntry_DisableOnCondition(&ME_JOYSTICK_DEFAULTS_STANDARD, !joystick.isGameController);
        MenuEntry_DisableOnCondition(&ME_JOYSTICK_DEFAULTS_PRO, !joystick.isGameController);
        break;

    case MENU_NETOPTIONS:
        if (MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption] == MAXVOLUMES)
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_USERMAP;
        else
        {
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_LEVEL;
            MEO_NETOPTIONS_LEVEL.options = &MEOS_NETOPTIONS_LEVEL[MEOSV_NetEpisodes[MEO_NETOPTIONS_EPISODE.currentOption]];
        }
        if (!(g_gametypeFlags[m_coop] & GAMETYPE_MARKEROPTION))
        {
            ME_NETOPTIONS_MARKERS.type = Dummy;
            ME_NETOPTIONS_MARKERS.flags |= MEF_Disabled;
        }
        else
        {
            ME_NETOPTIONS_MARKERS.type = Option;
            ME_NETOPTIONS_MARKERS.flags &= ~MEF_Disabled;
        }
        MEL_NETOPTIONS[5] = (g_gametypeFlags[m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)) ? &ME_NETOPTIONS_FRFIRE : &ME_NETOPTIONS_MAPEXITS;
        break;

    case MENU_OPTIONS:
        MenuEntry_DisableOnCondition(&ME_OPTIONS_PLAYERSETUP, ud.recstat == 1);
        break;
    }

    default:
        break;
    }
}


static void Menu_DrawVerifyPrompt(int32_t x, int32_t y, const char * text, int numlines = 1)
{
    mgametextcenter(x, y + (90<<16), text);
#ifndef EDUKE32_ANDROID_MENU
    char const * inputs = CONTROL_LastSeenInput == LastSeenInput::Joystick
        ? "Press (A) to accept, (B) to return."
        : "(Y/N)";
    mgametextcenter(x, y + (90<<16) + MF_Bluefont.get_yline() * numlines, inputs);
#endif
}

static void Menu_PreDraw(MenuID_t cm, MenuEntry_t *entry, const vec2_t origin)
{
    int32_t i, j, l = 0;

    switch (cm)
    {

    case MENU_PLAYER:
        rotatesprite_fs(origin.x + (260<<16), origin.y + ((24+(tilesiz[APLAYER].y>>1))<<16), 49152L,0,1441-((((4-((int32_t) totalclock>>4)))&3)*5),0,entry == &ME_PLAYER_TEAM ? G_GetTeamPalette(playerteam) : playercolor,10);
        break;

    case MENU_MACROS:
        mgametextcenter(origin.x, origin.y + (144<<16), "Activate in-game with Shift-F#");
        break;

    case MENU_COLCORR:
    case MENU_COLCORR_INGAME:
        // center panel
        rotatesprite_fs(origin.x + (120<<16), origin.y + (32<<16), 16384, 0, 3290, 0, 0, 2|8|16);
        rotatesprite_fs(origin.x + (160<<16) - (tilesiz[BOTTOMSTATUSBAR].x<<13), origin.y + (82<<16) - (tilesiz[BOTTOMSTATUSBAR].y<<14), 16384, 0, BOTTOMSTATUSBAR, 0, 0, 2|8|16);

        // left panel
        rotatesprite_fs(origin.x + (40<<16), origin.y + (32<<16), 16384, 0, BONUSSCREEN, 0, 0, 2|8|16);

        // right panel
        rotatesprite_fs(origin.x + (200<<16), origin.y + (32<<16), 16384, 0, LOADSCREEN, 0, 0, 2|8|16);
        break;

    case MENU_SAVECLEANVERIFY:
        videoFadeToBlack(1);

        if (g_oldSaveCnt)
        {
            Bsprintf(tempbuf, "Delete %d obsolete saves?\nThis action cannot be undone.", g_oldSaveCnt);
            Menu_DrawVerifyPrompt(origin.x, origin.y, tempbuf, 2);
        }
        else
            mgametextcenter(origin.x, origin.y + (90<<16), "No obsolete saves found!");

        break;

    case MENU_LOADVERIFY:
    {
        videoFadeToBlack(1);
        menusave_t & msv = g_menusaves[M_LOAD.currentEntry];
        if (msv.isOldVer && msv.brief.isExt)
        {
            Bsprintf(tempbuf, "Resume game from sequence point:\n\"%s\"", msv.brief.name);
            Menu_DrawVerifyPrompt(origin.x, origin.y, tempbuf, 2);
        }
        else
        {
            Bsprintf(tempbuf, "Load game:\n\"%s\"", msv.brief.name);
            Menu_DrawVerifyPrompt(origin.x, origin.y, tempbuf, 2);
        }
        break;
    }

    case MENU_SAVEVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Overwrite previous saved game?");
        break;

    case MENU_LOADDELVERIFY:
    case MENU_SAVEDELVERIFY:
    {
        videoFadeToBlack(1);
        menusave_t & msv = cm == MENU_LOADDELVERIFY ? g_menusaves[M_LOAD.currentEntry] : g_menusaves[M_SAVE.currentEntry-1];
        Bsprintf(tempbuf, "Delete saved game:\n\"%s\"?", msv.brief.name);
        Menu_DrawVerifyPrompt(origin.x, origin.y, tempbuf, 2);
        break;
    }

    case MENU_NEWVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Abort this game?");
        break;

    case MENU_COLCORRRESETVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Reset color correction to defaults?");
        break;
    case MENU_KEYSRESETVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Reset keys to defaults?");
        break;
    case MENU_KEYSCLASSICVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Reset keys to classic defaults?");
        break;
    case MENU_JOYSTANDARDVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Reset gamepad to standard layout?");
        break;
    case MENU_JOYPROVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Reset gamepad to pro layout?");
        break;
    case MENU_JOYCLEARVERIFY:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Clear all gamepad settings?");
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "Are you sure you want to quit?");
        break;

    case MENU_QUITTOTITLE:
        videoFadeToBlack(1);
        Menu_DrawVerifyPrompt(origin.x, origin.y, "End game and return to title screen?");
        break;

    case MENU_NETWAITMASTER:
        G_DrawFrags();
        mgametextcenter(origin.x, origin.y + (50<<16), "Waiting for master\n"
                                                       "to select level");
        break;

    case MENU_NETWAITVOTES:
        G_DrawFrags();
        mgametextcenter(origin.x, origin.y + (90<<16), "Waiting for votes");
        break;

#ifndef EDUKE32_STANDALONE
    case MENU_BUYDUKE:
        mgametextcenter(origin.x, origin.y + (33<<16), "You are playing the shareware\n"
                                                       "version of Duke Nukem 3D.  While\n"
                                                       "this version is really cool, you\n"
                                                       "are missing over 75% of the total\n"
                                                       "game, along with other great extras\n"
                                                       "which you'll get when you order\n"
                                                       "the complete version and get\n"
                                                       "the final three episodes.");

        mgametextcenter(origin.x, origin.y + ((148+16)<<16), "Press any key or button...");
        break;
#endif
            }
        }
        break;
    case MENU_CREDITS4:   // JBF 20031220
    {
#define MENU_YOFFSET 40
#define MENU_INCREMENT(x) (oy += ((x) << 16))  // maybe this should have been MENU_EXCREMENT instead

        int32_t oy = origin.y;

        mgametextcenter(origin.x, MENU_INCREMENT(MENU_YOFFSET), "Developers");
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(11), "Richard \"TerminX\" Gobeille", 8);
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(7), "Evan \"Hendricks266\" Ramos", 8);
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(7), "Alex \"pogokeen\" Dawson", 8);

        mgametextcenter(origin.x, MENU_INCREMENT(11), "Retired developers");
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(11), "Pierre-Loup \"Plagman\" Griffais", 8);
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(7), "Philipp \"Helixhorned\" Kutin", 8);

        mgametextcenter(origin.x, MENU_INCREMENT(11), "Special thanks to");
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(11), "Jonathon \"JonoF\" Fowler", 8);

        mgametextcenter(origin.x, MENU_INCREMENT(11), "Uses BUILD Engine technology by");
        creditsminitext(origin.x + (160 << 16), MENU_INCREMENT(11), "Ken \"Awesoken\" Silverman", 8);

#undef MENU_INCREMENT
#undef MENU_YOFFSET
    }
        break;

    case MENU_CREDITS5:
        l = 7;

        mgametextcenter(origin.x, origin.y + ((38-l)<<16), "License and Other Contributors");
        {
            static const char *header[] =
            {
                "This program is distributed under the terms of the",
                "GNU General Public License version 2 as published by the",
                "Free Software Foundation. See gpl-2.0.txt for details.",
                "BUILD engine technology available under license. See buildlic.txt.",
                nullptr,
                "The EDuke32 team thanks the following people for their contributions:",
                nullptr,
            };
            static const char *body[] =
            {
                "Alexey Skrybykin",  // Nuke.YKT - Polymost fixes
                "Bioman",            // GTK work, APT repository and package upkeep
                "Brandon Bergren",   // "Bdragon" - tiles.cfg
                "Charlie Honig",     // "CONAN" - showview command
                "Dan Gaskill",       // "DeeperThought" - testing
                "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                "Ed Coolidge",       // Mapster32 improvements
                "Emile Belanger",    // original Android work
                "Fox",               // various patches
                "Hunter_rus",        // tons of stuff
                "James Bentler",     // Mapster32 improvements
                "Jasper Foreman",    // netcode contributions
                "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                "Jeff Hart",         // website graphics
                "Jonathan Strander", // "Mblackwell" - testing and feature speccing
                "Jordon Moss",       // "Striker" - various patches, OldMP work
                "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                "Lachlan McDonald",  // official EDuke32 icon
                "LSDNinja",          // OS X help and testing
                "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                "Matt Saettler",     // original DOS EDuke/WW2GI enhancements
                "NY00123",           // Linux / SDL usability patches
                "Ozkan Sezer",       // SDL/GTK version checking improvements
                "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                "Peter Veenstra",    // "Qbix" - port to 64-bit
                "Robin Green",       // CON array support
                "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                "Stephen Anthony",   // early 64-bit porting work
                "tueidj",            // Wii port
            };
            EDUKE32_STATIC_ASSERT(ARRAY_SIZE(body) % 3 == 0);
            static const char *footer[] =
            {
                nullptr,
                "Visit eduke32.com for news and updates",
            };

            static constexpr int header_numlines = ARRAY_SIZE(header);
            static constexpr int body_numlines   = ARRAY_SIZE(body);
            static constexpr int footer_numlines = ARRAY_SIZE(footer);

            static constexpr int CCOLUMNS = 3;
            static constexpr int CCOLXBUF = 20;

            int c;
            i = 0;
            for (c = 0; c < header_numlines; c++)
                if (header[c])
                    creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(c*7)-l)<<16), header[c], 8);
            i += c;
            for (c = 0; c < body_numlines; c++)
                if (body[c])
                    creditsminitext(origin.x + ((CCOLXBUF+((320-CCOLXBUF*2)/(CCOLUMNS*2)) +((320-CCOLXBUF*2)/CCOLUMNS)*(c/(body_numlines/CCOLUMNS)))<<16), origin.y + ((17+10+10+8+4+((c%(body_numlines/CCOLUMNS))*7)+(i*7)-l)<<16), body[c], 8);
            i += c/CCOLUMNS;
            for (c = 0; c < footer_numlines; c++)
                if (footer[c])
                    creditsminitext(origin.x + (160<<16), origin.y + ((17+10+10+8+4+(c*7)+(i*7)-l)<<16), footer[c], 8);
        }

        break;

    default:
        break;
    }
}


/*
Functions where a "newValue" or similar is passed are run *before* the linked variable is actually changed.
That way you can compare the new and old values and potentially block the change.
*/
static void Menu_EntryLinkActivate(MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
        break;
    }

    case MENU_JOYSTICKAXES:
        M_JOYSTICKAXIS.title = joyGetName(0, M_JOYSTICKAXES.currentEntry);
#if 0
        MEO_JOYSTICKAXIS_ANALOG.data = &JoystickAnalogueAxes[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_SCALE.variable = &JoystickAnalogueScale[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_INVERT.data = &JoystickAnalogueInvert[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_DEAD.variable = &JoystickAnalogueDead[M_JOYSTICKAXES.currentEntry];
        MEO_JOYSTICKAXIS_SATU.variable = &JoystickAnalogueSaturate[M_JOYSTICKAXES.currentEntry];
#endif
        break;

    case MENU_CHEATS:
    {
        const int32_t cheatFuncID = M_CHEATS.currentEntry - 1;
        switch (cheatFuncID)
        {
            case -1:
            case CHEATFUNC_WARP:
            case CHEATFUNC_SKILL:
                break;
            default:
                Menu_DoCheat(CheatFunctionIDs[cheatFuncID]);
                break;
        }
        break;
    }

    default:
        break;
    }

    if (entry == &ME_VIDEOSETUP_APPLY)
    {
        resolution_t p = { xres, yres, fullscreen, bpp, 0 };
        int32_t prend = videoGetRenderMode();
        int32_t pvsync = vid_vsync;
        int pborderless = r_borderless;

        resolution_t n = { resolution[newresolution].xdim, resolution[newresolution].ydim,
                           (resolution[newresolution].flags & RES_FS) ? newfullscreen : 0,
                           (newrendermode == REND_CLASSIC) ? 8 : resolution[newresolution].bppmax, 0 };

        if (r_borderless != newborderless)
            videoResetMode();

        r_borderless = newborderless;

        if (videoSetGameMode(n.flags, n.xdim, n.ydim, n.bppmax, upscalefactor) < 0)
        {
            r_borderless = pborderless;

            if (videoSetGameMode(p.flags, p.xdim, p.ydim, p.bppmax, upscalefactor) < 0)
            {
                videoSetRenderMode(prend);
                G_GameExit("Failed restoring old video mode.");
            }
            else
            {
                onvideomodechange(p.bppmax > 8);
                vid_vsync = videoSetVsync(pvsync);
            }
        }
        else
        {
            videoSetRenderMode(newrendermode);
            vid_vsync = videoSetVsync(newvsync);
            onvideomodechange(n.bppmax > 8);
        }

        g_restorePalette = -1;
        G_UpdateScreenArea();
        ScreenMode = fullscreen;
        ScreenWidth = xres;
        ScreenHeight = yres;
        ScreenBPP = bpp;
    }
    else if (entry == &ME_SOUND_RESTART)
    {
        snd_mixrate = soundrate;
        snd_numvoices = soundvoices;
        mus_device = musicdevice;

        S_SoundShutdown();

        S_SoundStartup();

        FX_StopAllSounds();
        S_ClearSoundLocks();
    }
    else if (entry == &ME_SAVESETUP_CLEANUP)
    {
        Menu_Change(MENU_SAVECLEANVERIFY);
    }
    else if (entry == &ME_NETHOST_LAUNCH)
    {
        // master does whatever it wants
        if (g_netServer)
        {
            Net_FillNewGame(&pendingnewgame, 1);
            Net_StartNewGame();
            Net_SendNewGame(1, NULL);
        }
        else if (voting == -1)
        {
            Net_SendMapVoteInitiate();
            Menu_Change(MENU_NETWAITVOTES);
        }
    }
}

static int32_t Menu_EntryOptionModify(MenuEntry_t *entry, int32_t newOption)
{
    int32_t x;
    auto ps = g_player[myconnectindex].ps;

    if (entry == &ME_GAMESETUP_DEMOREC)
    {
        if ((ps->gm&MODE_GAME))
            G_CloseDemoWrite();
    }
    else if (entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
    {
        ud.weaponswitch = ud.weaponswitch & ~(1|4);
        switch (newOption)
        {
        case 2:
            ud.weaponswitch = ud.weaponswitch | 4;
            fallthrough__;
        case 1:
            ud.weaponswitch = ud.weaponswitch | 1;
            break;
        default:
            break;
        }
    }
    else if (entry == &ME_SOUND)
    {
        if (newOption == 0)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }
    }
    else if (entry == &ME_SOUND_MUSIC)
    {
        mus_enabled = newOption;

        if (newOption == 0)
            S_PauseMusic(true);
        else
        {
            S_PauseMusic(false);
        }
    }
    else if (entry == &ME_SOUND_DUKETALK)
        snd_speech = (snd_speech&~1) | newOption;
    else if (entry == &ME_JOYSTICK_ENABLE)
    {
        if (newOption)
            CONTROL_ScanForControllers();
        CONTROL_JoystickEnabled = (newOption && CONTROL_JoyPresent);
    }
    else if (entry == &ME_JOYSTICKAXIS_ANALOG)
        CONTROL_MapAnalogAxis(M_JOYSTICKAXES.currentEntry, newOption, controldevice_joystick);
    else if (entry == &ME_JOYSTICKAXIS_INVERT)
        CONTROL_SetAnalogAxisInvert(M_JOYSTICKAXES.currentEntry, newOption, controldevice_joystick);
    else if (entry == &ME_NETOPTIONS_EPISODE)
    {
        if (newOption < g_volumeCnt)
            ud.m_volume_number = newOption;
    }
    else if (entry == &ME_NETOPTIONS_MONSTERS)
    {
        ud.m_monsters_off = (newOption == g_skillCnt);
        if (newOption < g_skillCnt)
            ud.m_player_skill = newOption;
    }
    else if (entry == &ME_ADULTMODE)
    {
        if (newOption)
        {
            for (x=0; x<g_animWallCnt; x++)
                switch (DYNAMICTILEMAP(wall[animwall[x].wallnum].picnum))
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }
        else
        {
            adult_lockout = 0;
        }
    }


    return 0;
}


/*
This is polled when the menu code is populating the screen but for some reason doesn't have the data.
*/
static int32_t Menu_EntryOptionSource(MenuEntry_t *entry, int32_t currentValue)
{
    if (entry == &ME_GAMESETUP_WEAPSWITCH_PICKUP)
        return (cl_weaponswitch & 1) ? ((cl_weaponswitch & 4) ? 2 : 1) : 0;
    else if (entry == &ME_SOUND_DUKETALK)
        return snd_speech & 1;
    else if (entry == &ME_NETOPTIONS_MONSTERS)
        return (ud.m_monsters_off ? g_skillCnt : ud.m_player_skill);

    return currentValue;
}

void Menu_Close(uint8_t playerID)
{
    auto & gm = g_player[playerID].ps->gm;
    if (gm & MODE_GAME)
    {
        if (gm & MODE_MENU)
            I_ClearAllInput();

        // The following lines are here so that you cannot close the menu when no game is running.
        gm &= ~MODE_MENU;
        mouseLockToWindow(1);

        if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
        {
            ready2send = 1;
            totalclock = ototalclock;
            CAMERACLOCK = (int32_t) totalclock;
            CAMERADIST = 65536;
            m_animation.start = 0;
            m_animation.length = 0;

            // Reset next-viewscreen-redraw counter.
            // XXX: are there any other cases like that in need of handling?
            if (g_curViewscreen >= 0)
                actor[g_curViewscreen].t_data[0] = (int32_t) totalclock;
        }

        G_UpdateScreenArea();
        S_PauseSounds(false);
    }
}


#endif

END_DUKE_NS
