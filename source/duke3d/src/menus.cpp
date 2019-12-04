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

void Menu_Init(void)
{
    int32_t i, j, k;

    if (FURY)
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





#endif

END_DUKE_NS
