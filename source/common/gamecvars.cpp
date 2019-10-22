#include "c_cvars.h"
#include "common.h"
#include "fx_man.h"

/* Notes
 
 RedNukem has this for the toggle autorun command. Todo: Check what this is supposed to accomplish. The implementation makes no sense at all.
 (!RRRA || (!g_player[myconnectindex].ps->on_motorcycle && !g_player[myconnectindex].ps->on_boat)))
 
 
 */

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
CVARD(Bool, cl_viewbob, true, CVAR_ARCHIVE, "enable/disable player head bobbing") // Not implemented for Blood
CVARD(Bool, cl_weaponsway, true, CVAR_ARCHIVE, "enable/disable player weapon swaying") // Not implemented for Blood

CUSTOM_CVARD(Int, cl_autoaim, 1, CVAR_ARCHIVE, "enable/disable weapon autoaim")
{
	if (self < 0 || self > (playing_blood? 2 : 3)) self = 1;	// The Shadow Warrior backend only has a bool for this.
	//UpdatePlayerFromMenu(); todo: networking (only operational in EDuke32 frontend anyway.)
};

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
	if (self < 8) self = 0;
	else if (self > 5) self = 5;
}

CUSTOM_CVARD(Int, mus_volume, 255, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "controls music volume")
{
	if (self < 0) self = 0;
	if (self > 255) self = 255;
}

#if 0


// DN3D
    static osdcvardata_t cvars_game[] =
    {
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "fov", "change the field of view", (void *)&ud.fov, CVAR_INT, 60, 140 },

        { "hud_althud", "enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_BOOL, 0, 1 },
        { "hud_custom", "change the custom hud", (void *)&ud.statusbarcustom, CVAR_INT, 0, ud.statusbarrange },
        { "hud_position", "aligns the status bar to the bottom/top", (void *)&ud.hudontop, CVAR_BOOL, 0, 1 },
        { "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        { "hud_messagetime", "length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
        { "hud_numbertile", "first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES-10 },
        { "hud_numberpal", "pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "hud_shadows", "enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
        { "hud_flashing", "enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
        { "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
        { "hud_scale","changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 50, 100 },
        { "hud_showmapname", "enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        { "hud_stats", "enable/disable level statistics display", (void *)&ud.levelstats, CVAR_BOOL, 0, 1 },
        { "hud_textscale", "sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
        { "hud_weaponscale","changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },
        { "hud_statusbarmode", "change overlay mode of status bar", (void *)&ud.statusbarmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

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
        { "r_size", "change size of viewable area", (void *)&ud.screen_size, CVAR_INT|CVAR_FUNCPTR, 0, 64 },
        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&ud.detail, CVAR_INT|CVAR_FUNCPTR, 1, 16 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

        { "vid_gamma","adjusts gamma component of gamma ramp",(void *)&g_videoGamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *)&g_videoContrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *)&g_videoBrightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
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
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "fov", "change the field of view", (void *)&ud.fov, CVAR_INT|CVAR_FUNCPTR, 75, 120 },

        { "hud_althud", "enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_BOOL, 0, 1 },
        { "hud_custom", "change the custom hud", (void *)&ud.statusbarcustom, CVAR_INT, 0, ud.statusbarrange },
        { "hud_position", "aligns the status bar to the bottom/top", (void *)&ud.hudontop, CVAR_BOOL, 0, 1 },
        { "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        { "hud_messagetime", "length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
        { "hud_numbertile", "first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES-10 },
        { "hud_numberpal", "pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "hud_shadows", "enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
        { "hud_flashing", "enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
        { "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
        { "hud_scale","changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 36, 100 },
        { "hud_showmapname", "enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        { "hud_stats", "enable/disable level statistics display", (void *)&ud.levelstats, CVAR_BOOL, 0, 1 },
        { "hud_textscale", "sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
        { "hud_weaponscale","changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },
        { "hud_statusbarmode", "change overlay mode of status bar", (void *)&ud.statusbarmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

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
        { "r_size", "change size of viewable area", (void *)&ud.screen_size, CVAR_INT|CVAR_FUNCPTR, 0, 64 },
        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&ud.detail, CVAR_INT|CVAR_FUNCPTR, 1, 16 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

        { "vid_gamma","adjusts gamma component of gamma ramp",(void *)&g_videoGamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *)&g_videoContrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *)&g_videoBrightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
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
        { "cl_interpolate", "enable/disable view interpolation", (void *)&gViewInterpolate, CVAR_BOOL, 0, 1 },
        { "cl_viewhbob", "enable/disable view horizontal bobbing", (void *)&gViewHBobbing, CVAR_BOOL, 0, 1 },
        { "cl_viewvbob", "enable/disable view vertical bobbing", (void *)&gViewVBobbing, CVAR_BOOL, 0, 1 },
        { "cl_slopetilting", "enable/disable slope tilting", (void *)&gSlopeTilting, CVAR_BOOL, 0, 1 },
        { "cl_showweapon", "enable/disable show weapons", (void *)&gShowWeapon, CVAR_BOOL, 0, 1 },

        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&gWeaponSwitch, CVAR_INT|CVAR_MULTI, 0, 3 },
//
//        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },
//
//        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },
//        { "hud_althud", "enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_BOOL, 0, 1 },
//        { "hud_custom", "change the custom hud", (void *)&ud.statusbarcustom, CVAR_INT, 0, ud.statusbarrange },
//        { "hud_position", "aligns the status bar to the bottom/top", (void *)&ud.hudontop, CVAR_BOOL, 0, 1 },
//        { "hud_bgstretch", "enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        { "hud_messages", "enable/disable showing messages", (void *)&gMessageState, CVAR_BOOL, 0, 1 },
//        { "hud_messagetime", "length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
//        { "hud_numbertile", "first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXUSERTILES-10 },
//        { "hud_numberpal", "pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
//        { "hud_shadows", "enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
//        { "hud_flashing", "enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
//        { "hud_glowingquotes", "enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
//        { "hud_scale","changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 36, 100 },
//        { "hud_showmapname", "enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        { "hud_stats", "enable/disable level statistics display", (void *)&gLevelStats, CVAR_BOOL, 0, 1 },
        { "hud_powerupduration", "enable/disable displaying the remaining seconds for power-ups", (void *)&gPowerupDuration, CVAR_BOOL, 0, 1 },
        { "hud_showmaptitle", "enable/disable displaying the map title at the beginning of the maps", (void*)& gShowMapTitle, CVAR_BOOL, 0, 1 },
//        { "hud_textscale", "sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
//        { "hud_weaponscale","changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },
//        { "hud_statusbarmode", "change overlay mode of status bar", (void *)&ud.statusbarmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "horizcenter", "enable/disable centered horizon line", (void *)&gCenterHoriz, CVAR_BOOL, 0, 1 },
        { "deliriumblur", "enable/disable delirium blur effect(polymost)", (void *)&gDeliriumBlur, CVAR_BOOL, 0, 1 },
        { "fov", "change the field of view", (void *)&gFov, CVAR_INT|CVAR_FUNCPTR, 75, 120 },
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
        { "r_size", "change size of viewable area", (void *)&gViewSize, CVAR_INT|CVAR_FUNCPTR, 0, 7 },
//        { "r_rotatespritenowidescreen", "pass bit 1024 to all CON rotatesprite calls", (void *)&g_rotatespriteNoWidescreen, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_upscalefactor", "increase performance by rendering at upscalefactor less than the screen resolution and upscale to the full resolution in the software renderer", (void *)&gUpscaleFactor, CVAR_INT|CVAR_FUNCPTR, 1, 16 },
        { "r_precache", "enable/disable the pre-level caching routine", (void *)&useprecache, CVAR_BOOL, 0, 1 },
//
        { "r_ambientlight", "sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },
        { "r_maxfpsoffset", "menu-controlled offset for r_maxfps",(void *)&r_maxfpsoffset, CVAR_INT|CVAR_FUNCPTR, -10, 10 },

        { "sensitivity","changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },
//
//        { "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },
//

        { "vid_gamma","adjusts gamma component of gamma ramp",(void *)&g_videoGamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *)&g_videoContrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *)&g_videoBrightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
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

#endif
