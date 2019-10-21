#include "c_cvars.h"
#include "common.h"

CVARD(Bool, cl_crosshair, true, CVAR_ARCHIVE, "enable/disable crosshair");
CVARD(Bool, cl_automsg, false, CVAR_ARCHIVE, "enable/disable automatically sending messages to all players")

CUSTOM_CVARD(Int, cl_autoaim, 1, CVAR_ARCHIVE, "enable/disable weapon autoaim")
{
	if (self < 0 || self > (playing_blood? 2 : 3)) self = 1;	// The Shadow Warrior backend only has a bool for this.
	//UpdatePlayerFromMenu(); todo: networking (only operational in EDuke32 frontend anyway.)
};



#if 0
        { "cl_autorun", "enable/disable autorun", (void *)&ud.auto_run, CVAR_BOOL, 0, 1 },
        { "cl_autorun", "enable/disable autorun", (void *)&gAutoRun, CVAR_BOOL, 0, 1 },
        { "cl_runmode", "enable/disable modernized run key operation", (void *)&gRunKeyMode, CVAR_BOOL, 0, 1 },
        { "cl_runmode", "enable/disable modernized run key operation", (void *)&ud.runkey_mode, CVAR_BOOL, 0, 1 },


// DN3D
    static osdcvardata_t cvars_game[] =
    {


        { "cl_autorun", "enable/disable autorun", (void *)&ud.auto_run, CVAR_BOOL, 0, 1 },

        { "cl_autosave", "enable/disable autosaves", (void *) &ud.autosave, CVAR_BOOL, 0, 1 },
        { "cl_autosavedeletion", "enable/disable automatic deletion of autosaves", (void *) &ud.autosavedeletion, CVAR_BOOL, 0, 1 },
        { "cl_maxautosaves", "number of autosaves to keep before deleting the oldest", (void *) &ud.maxautosaves, CVAR_INT, 1, 100 },

        { "cl_autovote", "enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },

        { "cl_cheatmask", "configure what cheats show in the cheats menu", (void *)&cl_cheatmask, CVAR_UINT, 0, ~0 },

        { "cl_obituaries", "enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
        { "cl_democams", "enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },

        { "cl_idplayers", "enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },

        { "cl_runmode", "enable/disable modernized run key operation", (void *)&ud.runkey_mode, CVAR_BOOL, 0, 1 },

        { "cl_showcoords", "show your position in the game world", (void *)&ud.coords, CVAR_INT, 0,
#ifdef USE_OPENGL
          2
#else
          1
#endif
        },

        { "cl_viewbob", "enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },

        { "cl_weaponsway", "enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "demorec_diffs","enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_force","enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
        {
            "demorec_difftics","sets game tic interval after which a diff is recorded",
            (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*REALGAMETICSPERSEC
        },
        { "demorec_diffcompress","Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_synccompress","Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_seeds","enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
        { "demoplay_diffs","enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
        { "demoplay_showsync","enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },

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

#ifdef EDUKE32_TOUCH_DEVICES
        { "hud_hidestick", "hide the touch input stick", (void *)&droidinput.hideStick, CVAR_BOOL, 0, 1 },
#endif

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

        { "mus_enabled", "enables/disables music", (void *)&ud.config.MusicToggle, CVAR_BOOL, 0, 1 },
        { "mus_volume", "controls music volume", (void *)&ud.config.MusicVolume, CVAR_INT, 0, 255 },

        { "osdhightile", "enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },
        { "osdscale", "adjust console text size", (void *)&osdscale, CVAR_FLOAT|CVAR_FUNCPTR, 1, 4 },

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

        { "snd_ambience", "enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        { "snd_enabled", "enables/disables sound effects", (void *)&ud.config.SoundToggle, CVAR_BOOL, 0, 1 },
        { "snd_fxvolume", "controls volume for sound effects", (void *)&ud.config.FXVolume, CVAR_INT, 0, 255 },
        { "snd_mixrate", "sound mixing rate", (void *)&ud.config.MixRate, CVAR_INT, 0, 48000 },
        { "snd_numchannels", "the number of sound channels", (void *)&ud.config.NumChannels, CVAR_INT, 0, 2 },
        { "snd_numvoices", "the number of concurrent sounds", (void *)&ud.config.NumVoices, CVAR_INT, 1, 128 },
        { "snd_reversestereo", "reverses the stereo channels", (void *)&ud.config.ReverseStereo, CVAR_BOOL, 0, 1 },
        { "snd_speech", "enables/disables player speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },
        { "snd_tryformats", "enables/disables automatic discovery of replacement sounds and music in .flac and .ogg formats", (void *)&g_maybeUpgradeSoundFormats, CVAR_BOOL, 0, 1 },
        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

#ifdef EDUKE32_TOUCH_DEVICES
        { "touch_sens_move_x","touch input sensitivity for moving forward/back", (void *)&droidinput.forward_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_move_y","touch input sensitivity for strafing", (void *)&droidinput.strafe_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_x", "touch input sensitivity for turning left/right", (void *) &droidinput.yaw_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_y", "touch input sensitivity for looking up/down", (void *) &droidinput.pitch_sens, CVAR_FLOAT, 1, 9 },
        { "touch_invert", "invert look up/down touch input", (void *) &droidinput.invertLook, CVAR_BOOL, 0, 1 },
#endif

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

        { "cl_autosave", "enable/disable autosaves", (void *) &ud.autosave, CVAR_BOOL, 0, 1 },
        { "cl_autosavedeletion", "enable/disable automatic deletion of autosaves", (void *) &ud.autosavedeletion, CVAR_BOOL, 0, 1 },
        { "cl_maxautosaves", "number of autosaves to keep before deleting the oldest", (void *) &ud.maxautosaves, CVAR_INT, 1, 100 },

        { "cl_autovote", "enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },

        { "cl_cheatmask", "configure what cheats show in the cheats menu", (void *)&cl_cheatmask, CVAR_UINT, 0, ~0 },

        { "cl_obituaries", "enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
        { "cl_democams", "enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },

        { "cl_idplayers", "enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },


        { "cl_showcoords", "show your position in the game world", (void *)&ud.coords, CVAR_INT, 0,
#ifdef USE_OPENGL
          2
#else
          1
#endif
        },

        { "cl_viewbob", "enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },

        { "cl_weaponsway", "enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 7 },

        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },

        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "demorec_diffs","enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_force","enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
        {
            "demorec_difftics","sets game tic interval after which a diff is recorded",
            (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*REALGAMETICSPERSEC
        },
        { "demorec_diffcompress","Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_synccompress","Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_seeds","enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
        { "demoplay_diffs","enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
        { "demoplay_showsync","enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },

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

#ifdef EDUKE32_TOUCH_DEVICES
        { "hud_hidestick", "hide the touch input stick", (void *)&droidinput.hideStick, CVAR_BOOL, 0, 1 },
#endif

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

        { "mus_enabled", "enables/disables music", (void *)&ud.config.MusicToggle, CVAR_BOOL, 0, 1 },
        { "mus_volume", "controls music volume", (void *)&ud.config.MusicVolume, CVAR_INT, 0, 255 },

        { "osdhightile", "enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },
        { "osdscale", "adjust console text size", (void *)&osdscale, CVAR_FLOAT|CVAR_FUNCPTR, 1, 4 },

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

        { "snd_ambience", "enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        { "snd_enabled", "enables/disables sound effects", (void *)&ud.config.SoundToggle, CVAR_BOOL, 0, 1 },
        { "snd_fxvolume", "controls volume for sound effects", (void *)&ud.config.FXVolume, CVAR_INT, 0, 255 },
        { "snd_mixrate", "sound mixing rate", (void *)&ud.config.MixRate, CVAR_INT, 0, 48000 },
        { "snd_numchannels", "the number of sound channels", (void *)&ud.config.NumChannels, CVAR_INT, 0, 2 },
        { "snd_numvoices", "the number of concurrent sounds", (void *)&ud.config.NumVoices, CVAR_INT, 1, 128 },
        { "snd_reversestereo", "reverses the stereo channels", (void *)&ud.config.ReverseStereo, CVAR_BOOL, 0, 1 },
        { "snd_speech", "enables/disables player speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },

        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

#ifdef EDUKE32_TOUCH_DEVICES
        { "touch_sens_move_x","touch input sensitivity for moving forward/back", (void *)&droidinput.forward_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_move_y","touch input sensitivity for strafing", (void *)&droidinput.strafe_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_x", "touch input sensitivity for turning left/right", (void *) &droidinput.yaw_sens, CVAR_FLOAT, 1, 9 },
        { "touch_sens_look_y", "touch input sensitivity for looking up/down", (void *) &droidinput.pitch_sens, CVAR_FLOAT, 1, 9 },
        { "touch_invert", "invert look up/down touch input", (void *) &droidinput.invertLook, CVAR_INT, 0, 1 },
#endif

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

//
//        { "cl_autosave", "enable/disable autosaves", (void *) &ud.autosave, CVAR_BOOL, 0, 1 },
//        { "cl_autosavedeletion", "enable/disable automatic deletion of autosaves", (void *) &ud.autosavedeletion, CVAR_BOOL, 0, 1 },
//        { "cl_maxautosaves", "number of autosaves to keep before deleting the oldest", (void *) &ud.maxautosaves, CVAR_INT, 1, 100 },
//
//        { "cl_autovote", "enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },
//
//        { "cl_cheatmask", "configure what cheats show in the cheats menu", (void *)&cl_cheatmask, CVAR_UINT, 0, ~0 },
//
//        { "cl_obituaries", "enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
//        { "cl_democams", "enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },
//
//        { "cl_idplayers", "enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },
//

        { "cl_interpolate", "enable/disable view interpolation", (void *)&gViewInterpolate, CVAR_BOOL, 0, 1 },
        { "cl_viewhbob", "enable/disable view horizontal bobbing", (void *)&gViewHBobbing, CVAR_BOOL, 0, 1 },
        { "cl_viewvbob", "enable/disable view vertical bobbing", (void *)&gViewVBobbing, CVAR_BOOL, 0, 1 },
        { "cl_slopetilting", "enable/disable slope tilting", (void *)&gSlopeTilting, CVAR_BOOL, 0, 1 },
        { "cl_showweapon", "enable/disable show weapons", (void *)&gShowWeapon, CVAR_BOOL, 0, 1 },

//
//        { "cl_showcoords", "show your position in the game world", (void *)&ud.coords, CVAR_INT, 0,
//#ifdef USE_OPENGL
//          2
//#else
//          1
//#endif
//        },
//
//        { "cl_viewbob", "enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },
//
//        { "cl_weaponsway", "enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        { "cl_weaponswitch", "enable/disable auto weapon switching", (void *)&gWeaponSwitch, CVAR_INT|CVAR_MULTI, 0, 3 },
//
//        { "color", "changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, MAXPALOOKUPS-1 },
//
//        { "crosshairscale","changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },
//
//        { "demorec_diffs","enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
//        { "demorec_force","enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
//        {
//            "demorec_difftics","sets game tic interval after which a diff is recorded",
//            (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*REALGAMETICSPERSEC
//        },
//        { "demorec_diffcompress","Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_INT, 0, 1 },
//        { "demorec_synccompress","Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_INT, 0, 1 },
//        { "demorec_seeds","enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
//        { "demoplay_diffs","enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
//        { "demoplay_showsync","enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },
//
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
//
//#ifdef EDUKE32_TOUCH_DEVICES
//        { "hud_hidestick", "hide the touch input stick", (void *)&droidinput.hideStick, CVAR_BOOL, 0, 1 },
//#endif
//
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
        { "mus_enabled", "enables/disables music", (void *)&MusicToggle, CVAR_BOOL, 0, 1 },
        { "mus_restartonload", "restart the music when loading a saved game with the same map or not", (void *)&MusicRestartsOnLoadToggle, CVAR_BOOL, 0, 1 },
        { "mus_volume", "controls music volume", (void *)&MusicVolume, CVAR_INT, 0, 255 },
        { "mus_device", "music device", (void *)&MusicDevice, CVAR_INT, 0, 1 },
        { "mus_redbook", "enables/disables redbook audio", (void *)&CDAudioToggle, CVAR_BOOL, 0, 1 },
//
//        { "osdhightile", "enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },
//        { "osdscale", "adjust console text size", (void *)&osdscale, CVAR_FLOAT|CVAR_FUNCPTR, 1, 4 },
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
//        { "snd_ambience", "enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        { "snd_enabled", "enables/disables sound effects", (void *)&SoundToggle, CVAR_BOOL, 0, 1 },
        { "snd_fxvolume", "controls volume for sound effects", (void *)&FXVolume, CVAR_INT, 0, 255 },
        { "snd_mixrate", "sound mixing rate", (void *)&MixRate, CVAR_INT, 0, 48000 },
        { "snd_numchannels", "the number of sound channels", (void *)&NumChannels, CVAR_INT, 0, 2 },
        { "snd_numvoices", "the number of concurrent sounds", (void *)&NumVoices, CVAR_INT, 1, 128 },
        { "snd_reversestereo", "reverses the stereo channels", (void *)&ReverseStereo, CVAR_BOOL, 0, 1 },
        { "snd_doppler", "enable/disable 3d sound", (void *)&gDoppler, CVAR_BOOL, 0, 1 },
//        { "snd_speech", "enables/disables player speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },
//
//        { "team","change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },
//
//#ifdef EDUKE32_TOUCH_DEVICES
//        { "touch_sens_move_x","touch input sensitivity for moving forward/back", (void *)&droidinput.forward_sens, CVAR_FLOAT, 1, 9 },
//        { "touch_sens_move_y","touch input sensitivity for strafing", (void *)&droidinput.strafe_sens, CVAR_FLOAT, 1, 9 },
//        { "touch_sens_look_x", "touch input sensitivity for turning left/right", (void *) &droidinput.yaw_sens, CVAR_FLOAT, 1, 9 },
//        { "touch_sens_look_y", "touch input sensitivity for looking up/down", (void *) &droidinput.pitch_sens, CVAR_FLOAT, 1, 9 },
//        { "touch_invert", "invert look up/down touch input", (void *) &droidinput.invertLook, CVAR_INT, 0, 1 },
//#endif
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
