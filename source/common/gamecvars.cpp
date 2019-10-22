#include "c_cvars.h"
#include "common.h"
#include "fx_man.h"
#include "baselayer.h"
#include "gameconfigfile.h"

/* Notes
 
 RedNukem has this for the toggle autorun command. Todo: Check what this is supposed to accomplish. The implementation makes no sense at all.
 (!RRRA || (!g_player[myconnectindex].ps->on_motorcycle && !g_player[myconnectindex].ps->on_boat)))
 
 
 */

FGameConfigFile* GameConfig;
static FString GameName;

void G_LoadConfig(const char *game)
{
	GameConfig = new FGameConfigFile();
	GameConfig->DoGlobalSetup();
	GameConfig->DoGameSetup(game);
	FBaseCVar::EnableCallbacks();
	GameName = game;
}

void G_SaveConfig()
{
	GameConfig->ArchiveGameData(GameName);
	GameConfig->WriteConfigFile();
	delete GameConfig;
	GameConfig = nullptr;
}

CVARD(Bool, cl_crosshair, true, CVAR_ARCHIVE, "enable/disable crosshair");
CVARD(Bool, cl_automsg, false, CVAR_ARCHIVE, "enable/disable automatically sending messages to all players") // Not implemented for Blood
CVARD(Bool, cl_autorun, true, CVAR_ARCHIVE, "enable/disable autorun")
CVARD(Bool, cl_runmode, true, CVAR_ARCHIVE, "enable/disable modernized run key operation")
CVARD(Bool, cl_autosave, true, CVAR_ARCHIVE, "enable/disable autosaves") // Not implemented for Blood (but looks like the other games never check it either.)
CVARD(Bool, cl_autosavedeletion, true, CVAR_ARCHIVE, "enable/disable automatic deletion of autosaves") // Not implemented for Blood
CVARD(Int, cl_maxautosaves, 8, CVAR_ARCHIVE, "number of autosaves to keep before deleting the oldest") // Not implemented for Blood
CVARD(Int, cl_cheatmask, ~0, CVAR_ARCHIVE, "configure what cheats show in the cheats menu")
CVARD(Bool, cl_obituaries, true, CVAR_ARCHIVE, "enable/disable multiplayer death messages") // Not implemented for Blood
CVARD(Bool, cl_democams, true, CVAR_ARCHIVE, "enable/disable demo playback cameras") // Not implemented for Blood
CVARD(Bool, cl_idplayers, true, CVAR_ARCHIVE, "enable/disable name display when aiming at opponents") // Not implemented for Blood
CVARD(Bool, cl_showcoords, false, 0, "show your position in the game world") // This is a debug oprion in its current form, not implemented in Blood
CVARD(Bool, cl_weaponsway, true, CVAR_ARCHIVE, "enable/disable player weapon swaying") // Not implemented for Blood

// Todo: Consolidate these to be consistent across games?
CVARD(Bool, cl_viewbob, true, CVAR_ARCHIVE, "enable/disable player head bobbing") // Not implemented for Blood
CVARD(Bool, cl_viewhbob, true, CVAR_ARCHIVE, "enable/disable view horizontal bobbing") // Only implemented in Blood
CVARD(Bool, cl_viewvbob, true, CVAR_ARCHIVE, "enable/disable view vertical bobbing") // Only implemented in Blood

CVARD(Bool, cl_interpolate, true, CVAR_ARCHIVE, "enable/disable view interpolation") // only implemented in Blood
CVARD(Bool, cl_slopetilting, false, CVAR_ARCHIVE, "enable/disable slope tilting") // only implemented in Blood
CVARD(Bool, cl_showweapon, true, CVAR_ARCHIVE, "enable/disable show weapons") // only implemented in Blood
CUSTOM_CVARD(Int, cl_crosshairscale, 50, CVAR_ARCHIVE, "changes the size of the crosshair")
{
	if (self < 1) self = 1;
	else if (self > 100) self = 100;
}


CUSTOM_CVARD(Int, cl_autoaim, 1, CVAR_ARCHIVE|CVAR_USERINFO, "enable/disable weapon autoaim")
{
	if (self < 0 || self > (playing_blood? 2 : 3)) self = 1;	// The Shadow Warrior backend only has a bool for this.
	//UpdatePlayerFromMenu(); todo: networking (only operational in EDuke32 frontend anyway.)
};

CUSTOM_CVARD(Int, cl_weaponswitch, 3, CVAR_ARCHIVE|CVAR_USERINFO, "enable/disable auto weapon switching")

{
	if (self < 0) self = 0;
	if (self > 3 && playing_blood) self = 3;
	if (self > 7) self = 7;
	//UpdatePlayerFromMenu(); todo: networking (only operational in EDuke32 frontend anyway.)
}


CUSTOM_CVARD(Int, cl_autovote, 0, CVAR_ARCHIVE, "enable/disable automatic voting")
{
	if (self < 0 || self > 2) self = 0;
}

bool G_CheckAutorun(bool button)
{
	if (cl_runmode) return button || cl_autorun;
	else return button ^ !!cl_autorun;
}

// Demos

CVARD_NAMED(Bool, demorec_diffcompress, demorec_diffcompress_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "Compression for diffs")
CVARD_NAMED(Bool, demorec_synccompress, demorec_synccompress_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "Compression for input")
CVARD_NAMED(Bool, demorec_seeds, demorec_seeds_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable recording of random seed for later sync checking")
CVARD_NAMED(Bool, demorec_diffs, demorec_diffs_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable diff recording in demos")
CVARD_NAMED(Bool, demorec_force, demorec_force_cvar, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable forced demo recording")
CVARD_NAMED(Int, demorec_difftics, demorec_difftics_cvar, 60, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "sets game tic interval after which a diff is recorded")
CVARD(Bool, demoplay_diffs, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable application of diffs in demo playback")
CVARD(Bool, demoplay_showsync, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable display of sync status")

// Sound

CVARD(Bool, snd_ambience, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables ambient sounds") // Not implemented for Blood
CVARD(Bool, snd_enabled, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables sound effects")
CVARD(Bool, snd_tryformats, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables automatic discovery of replacement sounds and music in .flac and .ogg formats")
CVARD(Bool, snd_doppler, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable 3d sound")

CVARD(Bool, mus_enabled, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables music")
CVARD(Bool, mus_restartonload, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "restart the music when loading a saved game with the same map or not") // only implemented for Blood - todo: generalize
CVARD(Bool, mus_redbook, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables redbook audio (Blood only!)") // only Blood has assets for this.

CUSTOM_CVARD(Bool, snd_reversestereo, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_NOINITCALL, "reverses the stereo channels")
{
	FX_SetReverseStereo(self);
}

CUSTOM_CVARD(Int, snd_fxvolume, 255, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "controls volume for sound effects")
{
	if (self < 0) self = 0;
	if (self > 255) self = 255;
}

CUSTOM_CVARD(Int, snd_mixrate, 44100, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "sound mixing rate")
{
	if (self < 11025) self = 11025;
	else if (self > 48000) self = 48000;
}

CUSTOM_CVARD(Int, snd_numchannels, 2, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "the number of sound channels")
{
	if (self < 1) self = 1;
	else if (self > 2) self = 2;
}

CUSTOM_CVARD(Int, snd_numvoices, 64, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "the number of concurrent sounds")
{
	if (self < 8) self = 8;
	else if (self > 128) self = 128;
}

CUSTOM_CVARD(Int, snd_speech, 5, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables player speech")
{
	if (self < 0) self = 0;
	else if (self > 5) self = 5;
}

CUSTOM_CVARD(Int, mus_volume, 255, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "controls music volume")
{
	if (self < 0) self = 0;
	if (self > 255) self = 255;
}

// HUD

// NBlood had this differently with an inverted scale of 0-7 with 0 having no HUD, For consistency all frontends now use the same scale, with 0 being the smallest and 11 being the largest.
CUSTOM_CVARD(Int, r_size, 9, CVAR_ARCHIVE | CVAR_NOINITCALL, "Defines the HUD layout")
{
	if (self < 0) self = 0;
	else if (self > 11) self = 11;
	else
	{
		if (gi->validate_hud(self))
			gi->set_hud_layout(self);
		else
			OSD_Printf("Hud size %d not available\n");
	}
}

CUSTOM_CVARD(Int, hud_scale, 100, CVAR_ARCHIVE | CVAR_NOINITCALL, "changes the hud scale")//, (void*)&ud.statusbarscale, CVAR_INT | CVAR_FUNCPTR, 36, 100 },
{
	if (self < 36) self = 36;
	else if (self > 100) self = 100;
	else gi->set_hud_scale(r_size);
}

// This is to allow flattening the overly complicated HUD configuration to one single value and keep the complexity safely inside the HUD code.
bool G_ChangeHudLayout(int direction)
{
	if (direction < 0 && r_size > 0)
	{
		int layout = r_size - 1;
		while (!gi->validate_hud(layout) && layout >= 0) layout--;
		if (layout >= 0)
		{
			r_size = layout;
			return true;
		}
	}
	else if (r_size < 11)
	{
		int layout = r_size + 1;
		while (!gi->validate_hud(layout) && layout <= 11) layout++;
		if (layout <= 11)
		{
			r_size = layout;
			return true;
		}
	}
	return false;
}

int hud_statusbarrange;	// will be set by the game's configuration setup.
CUSTOM_CVARD(Int, hud_custom, 0, CVAR_ARCHIVE|CVAR_NOINITCALL, "change the custom hud") // this has no backing implementation, it seems to be solely for scripted HUDs.
{
	if (self < 0) self = 0;
	else if (self >= hud_statusbarrange) self = hud_statusbarrange - 1;
}

CVARD(Bool, hud_stats, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable level statistics display")
CVARD(Bool, hud_showmapname, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable map name display on load")
CUSTOM_CVARD(Int, r_fov, 90, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "change the field of view") //, (void*)r_fov.Value, CVAR_INT, 60, 140
{
	if (self < 60) self = 60;
	else if (self < 140) self = 140;
}

#if 0


// DN3D
    static osdcvardata_t cvars_game[] =
    {

        { "in_joystick","enables input from the joystick if it is present",(void *)&ud.setup.usejoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","enables input from the mouse if it is present",(void *)&ud.setup.usemouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "in_aimmode", "0:toggle, 1:hold to aim", (void *)&ud.mouseaiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&ud.config.MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "amount of mouse movement to filter out", (void *)&ud.config.MouseDeadZone, CVAR_INT, 0, 512 },
        { "in_mouseflip", "invert vertical mouse movement", (void *)&ud.mouseflip, CVAR_BOOL, 0, 1 },
        { "in_mousemode", "toggles vertical mouse view", (void *)&g_myAimMode, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "enable/disable mouse input smoothing", (void *)&ud.config.SmoothInput, CVAR_BOOL, 0, 1 },

		{ "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },
        { "r_drawweapon", "enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        { "r_showfps", "show the frame rate counter", (void *)&ud.showfps, CVAR_INT, 0, 3 },
        { "r_showfpsperiod", "time in seconds before averaging min and max stats for r_showfps 2+", (void *)&ud.frameperiod, CVAR_INT, 0, 5 },
        { "r_shadows", "enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

        { "wchoice","sets weapon autoselection order", (void *)ud.wchoice, CVAR_STRING|CVAR_FUNCPTR, 0, MAX_WEAPONS },
    };

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (auto & cv : cvars_game)
    {
        switch (cv.flags & (CVAR_FUNCPTR|CVAR_MULTI))
        {
            case CVAR_FUNCPTR:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_game); break;
            case CVAR_MULTI:
            case CVAR_FUNCPTR|CVAR_MULTI:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_multi); break;
            default:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set); break;
        }
    }

// RR

    static osdcvardata_t cvars_game[] =
    {

        { "in_joystick","enables input from the joystick if it is present",(void *)&ud.setup.usejoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","enables input from the mouse if it is present",(void *)&ud.setup.usemouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "in_aimmode", "0:toggle, 1:hold to aim", (void *)&ud.mouseaiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&ud.config.MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "amount of mouse movement to filter out", (void *)&ud.config.MouseDeadZone, CVAR_INT, 0, 512 },
        { "in_mouseflip", "invert vertical mouse movement", (void *)&ud.mouseflip, CVAR_BOOL, 0, 1 },
        { "in_mousemode", "toggles vertical mouse view", (void *)&g_myAimMode, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "enable/disable mouse input smoothing", (void *)&ud.config.SmoothInput, CVAR_BOOL, 0, 1 },

        { "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },
        { "r_drawweapon", "enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        { "r_showfps", "show the frame rate counter", (void *)&ud.showfps, CVAR_INT, 0, 3 },
        { "r_showfpsperiod", "time in seconds before averaging min and max stats for r_showfps 2+", (void *)&ud.frameperiod, CVAR_INT, 0, 5 },
        { "r_shadows", "enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

		{ "wchoice","sets weapon autoselection order", (void *)ud.wchoice, CVAR_STRING|CVAR_FUNCPTR, 0, MAX_WEAPONS },
    };

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (auto & cv : cvars_game)
    {
        switch (cv.flags & (CVAR_FUNCPTR|CVAR_MULTI))
        {
            case CVAR_FUNCPTR:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_game); break;
            case CVAR_MULTI:
            case CVAR_FUNCPTR|CVAR_MULTI:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_multi); break;
            default:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set); break;
        }
    }

// Blood

int32_t registerosdcommands(void)
{
    char buffer[256];
    static osdcvardata_t cvars_game[] =
    {
        { "horizcenter", "enable/disable centered horizon line", (void *)&gCenterHoriz, CVAR_BOOL, 0, 1 },
        { "in_joystick","enables input from the joystick if it is present",(void *)&gSetup.usejoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","enables input from the mouse if it is present",(void *)&gSetup.usemouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "in_aimmode", "0:toggle, 1:hold to aim", (void *)&gMouseAiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "amount of mouse movement to filter out", (void *)&MouseDeadZone, CVAR_INT, 0, 512 },
        { "in_mouseflip", "invert vertical mouse movement", (void *)&gMouseAimingFlipped, CVAR_BOOL, 0, 1 },
        { "in_mousemode", "toggles vertical mouse view", (void *)&gMouseAim, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "enable/disable mouse input smoothing", (void *)&SmoothInput, CVAR_BOOL, 0, 1 },
//
//        { "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },
//        { "r_drawweapon", "enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        { "r_showfps", "show the frame rate counter", (void *)&gShowFps, CVAR_INT, 0, 3 },
        { "r_showfpsperiod", "time in seconds before averaging min and max stats for r_showfps 2+", (void *)&gFramePeriod, CVAR_INT, 0, 5 },
//        { "r_shadows", "enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
//        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
},
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&useprecache, CVAR_BOOL, 0, 1 },
//
        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },
//
//        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },
//
//        { "wchoice","sets weapon autoselection order", (void *)ud.wchoice, CVAR_STRING|CVAR_FUNCPTR, 0, MAX_WEAPONS },
    };
//
//    osdcmd_cheatsinfo_stat.cheatnum = -1;
//
    for (auto & cv : cvars_game)
    {
        switch (cv.flags & (CVAR_FUNCPTR|CVAR_MULTI))
        {
            case CVAR_FUNCPTR:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_game); break;
            case CVAR_MULTI:
            case CVAR_FUNCPTR|CVAR_MULTI:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set_multi); break;
            default:
                OSD_RegisterCvar(&cv, osdcmd_cvar_set); break;
        }
    }

// These I don't care about.
//{ "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&ud.detail, CVAR_INT|CVAR_FUNCPTR, 1, 16 },
//{ "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&gUpscaleFactor, CVAR_INT|CVAR_FUNCPTR, 1, 16

// These have to wait until the HUD code is cleaned up (no idea which may survive and which won't.)
/*
	{ "hud_position", "aligns the status bar to the bottom/top", (void*)&ud.hudontop, CVAR_BOOL, 0, 1 },
	{ "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void*)&ud.bgstretch, CVAR_BOOL, 0, 1 },
	{ "hud_messagetime", "length of time to display multiplayer chat messages", (void*)&ud.msgdisptime, CVAR_INT, 0, 3600 },
	{ "hud_numbertile", "first tile in alt hud number set", (void*)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES - 10 },
	{ "hud_numberpal", "pal for alt hud numbers", (void*)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS - 1 },
	{ "hud_shadows", "enable/disable althud shadows", (void*)&althud_shadows, CVAR_BOOL, 0, 1 },
	{ "hud_flashing", "enable/disable althud flashing", (void*)&althud_flashing, CVAR_BOOL, 0, 1 },
	{ "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void*)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
	{ "hud_textscale", "sets multiplayer chat message size", (void*)&ud.textscale, CVAR_INT, 100, 400 },
	{ "hud_weaponscale","changes the weapon scale", (void*)&ud.weaponscale, CVAR_INT, 10, 100 },

	{ "hud_messages", "enable/disable showing messages", (void*)&gMessageState, CVAR_BOOL, 0, 1 },
	{ "hud_powerupduration", "enable/disable displaying the remaining seconds for power-ups", (void*)&gPowerupDuration, CVAR_BOOL, 0, 1 },

	// Currently unavailable due to dependency on an obsolete OpenGL feature
	{ "deliriumblur", "enable/disable delirium blur effect(polymost)", (void *)&gDeliriumBlur, CVAR_BOOL, 0, 1 },

	// This needs some serious internal cleanup first, the implementation is all over the place and prone to whacking the user setting.
	{ "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

*/
#endif
