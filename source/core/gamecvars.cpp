/*
** gamecvars.cpp
**
** most of the game CVARs from the frontend consolidated to only have one instance
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/ 

#include "c_cvars.h"
#include "common.h"
#include "baselayer.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "rts.h"
#include "stats.h"
#include "raze_music.h"
#include "c_dispatch.h"
#include "gstrings.h"
#include "quotemgr.h"

#define CVAR_FRONTEND_BLOOD 0
#define CVAR_FRONTEND_DUKELIKE 0

/* Notes
 
 RedNukem has this for the toggle autorun command. Todo: Check what this is supposed to accomplish. The implementation makes no sense at all.
 (!RRRA || (!g_player[myconnectindex].ps->on_motorcycle && !g_player[myconnectindex].ps->on_boat)))
 
 
 */

CVARD(Bool, cl_crosshair, true, CVAR_ARCHIVE, "enable/disable crosshair");
CVARD(Bool, cl_automsg, false, CVAR_ARCHIVE, "enable/disable automatically sending messages to all players") // Not implemented for Blood
CVARD(Bool, cl_autorun, true, CVAR_ARCHIVE, "enable/disable autorun")

CVARD(Bool, cl_runmode, true, CVAR_ARCHIVE, "enable/disable modernized run key operation")

bool G_CheckAutorun(bool button)
{
	if (cl_runmode) return button || cl_autorun;
	else return button ^ !!cl_autorun;
}


CVARD(Bool, cl_autosave, true, CVAR_ARCHIVE, "enable/disable autosaves") // Not implemented for Blood (but looks like the other games never check it either.)
CVARD(Bool, cl_autosavedeletion, true, CVAR_ARCHIVE, "enable/disable automatic deletion of autosaves") // Not implemented for Blood
CVARD(Int, cl_maxautosaves, 8, CVAR_ARCHIVE, "number of autosaves to keep before deleting the oldest") // Not implemented for Blood
CVARD(Int, cl_cheatmask, ~0, CVAR_ARCHIVE, "configure what cheats show in the cheats menu")
CVARD(Bool, cl_obituaries, true, CVAR_ARCHIVE, "enable/disable multiplayer death messages") // Not implemented for Blood
CVARD(Bool, cl_democams, true, CVAR_ARCHIVE, "enable/disable demo playback cameras") // Not implemented for Blood
CVARD(Bool, cl_idplayers, true, CVAR_ARCHIVE, "enable/disable name display when aiming at opponents")
CVARD(Bool, cl_weaponsway, true, CVAR_ARCHIVE, "enable/disable player weapon swaying") // Not implemented for Blood

// Todo: Consolidate these to be consistent across games?
CVARD(Bool, cl_viewbob, true, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "enable/disable player head bobbing") // Not implemented for Blood
CVARD(Bool, cl_viewhbob, true, CVAR_ARCHIVE|CVAR_FRONTEND_BLOOD, "enable/disable view horizontal bobbing") // Only implemented in Blood
CVARD(Bool, cl_viewvbob, true, CVAR_ARCHIVE|CVAR_FRONTEND_BLOOD, "enable/disable view vertical bobbing") // Only implemented in Blood

CVARD(Bool, cl_interpolate, true, CVAR_ARCHIVE, "enable/disable view interpolation") // only implemented in Blood
CVARD(Bool, cl_slopetilting, false, CVAR_ARCHIVE, "enable/disable slope tilting") // only implemented in Blood
CVARD(Int, cl_showweapon, 1, CVAR_ARCHIVE, "enable/disable show weapons") // only implemented in Blood
CUSTOM_CVARD(Int, cl_crosshairscale, 50, CVAR_ARCHIVE, "changes the size of the crosshair")
{
	if (self < 1) self = 1;
	else if (self > 100) self = 100;
}


CUSTOM_CVARD(Int, cl_autoaim, 1, CVAR_ARCHIVE|CVAR_USERINFO, "enable/disable weapon autoaim")
{
	if (self < 0 || self > ((g_gameType & GAMEFLAG_BLOOD)? 2 : 3)) self = 1;	// The Shadow Warrior backend only has a bool for this.
};

CUSTOM_CVARD(Int, cl_weaponswitch, 3, CVAR_ARCHIVE|CVAR_USERINFO, "enable/disable auto weapon switching")
{
	if (self < 0) self = 0;
	if (self > 1 && (g_gameType & GAMEFLAG_SW)) self = 1;
	if (self > 3 && (g_gameType & GAMEFLAG_BLOOD)) self = 3;
	if (self > 7) self = 7;
}


CUSTOM_CVARD(Int, cl_autovote, 0, CVAR_ARCHIVE, "enable/disable automatic voting")
{
	if (self < 0 || self > 2) self = 0;
}


// Demos

CVAR(Bool, demo_playloop, true, CVAR_ARCHIVE)	// only active in Blood, because none of the other games can play demos right now.
CVARD_NAMED(Bool, demorec_seeds, demorec_seeds_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable recording of random seed for later sync checking")
CVARD_NAMED(Bool, demorec_diffs, demorec_diffs_cvar, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable diff recording in demos")
CVARD_NAMED(Bool, demorec_force, demorec_force_cvar, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable forced demo recording")
CVARD_NAMED(Int, demorec_difftics, demorec_difftics_cvar, 60, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "sets game tic interval after which a diff is recorded")
CVARD(Bool, demoplay_diffs, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable application of diffs in demo playback")
CVARD(Bool, demoplay_showsync, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable display of sync status")

// Sound

CUSTOM_CVARD(Bool, snd_ambience, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL, "enables/disables ambient sounds") // Not implemented for Blood
{
	gi->SetAmbience(self);
}
CVARD(Bool, snd_enabled, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables sound effects")
CVARD(Bool, snd_tryformats, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables automatic discovery of replacement sounds and music in .flac and .ogg formats")
CVARD(Bool, snd_doppler, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable 3d sound")

CVARD(Bool, mus_restartonload, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "restart the music when loading a saved game with the same map or not") // only implemented for Blood - todo: generalize
CVARD(Bool, mus_redbook, true, CVAR_ARCHIVE, "enables/disables redbook audio") 

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

CUSTOM_CVARD(Int, snd_speech, 1, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables player speech")
{
	if (self < 0) self = 0;
	else if (self > 1) self = 1;
}


// HUD

// This was particularly messy. EDuke and Rednukem had no consistent setting for this but a complex combination of 4 CVARs and lots of mod flags controlling the HUD layout
// NBlood had this differently with an inverted scale of 0-7 with 0 having no HUD.
// For consistency all frontends now use the same scale, with 0 being the smallest and 11 being the largest, which get converted to the internal settings by the set_hud_layout callback.

int hud_size_max = 11;	// 11 is for Duke Nukem and its offspring games.

CUSTOM_CVARD(Int, hud_size, 9, CVAR_ARCHIVE | CVAR_NOINITCALL, "Defines the HUD size and style")
{
	if (self < 0) self = 0;
	else if (self > hud_size_max) self = hud_size_max;
	else
	{
		if (gi->validate_hud(self))
			gi->set_hud_layout(self);
		else
			Printf("Hud size %d not available\n", *self);
	}
}

CUSTOM_CVARD(Int, hud_scale, 100, CVAR_ARCHIVE | CVAR_NOINITCALL, "changes the hud scale")
{
	if (self < 35) self = 35;
	else if (self > 100) self = 100;
	else gi->set_hud_scale(self);
}

// This is to allow flattening the overly complicated HUD configuration to one single value and keep the complexity safely inside the HUD code.
bool G_ChangeHudLayout(int direction)
{
	if (direction < 0 && hud_size > 0)
	{
		int layout = hud_size - 1;
		while (!gi->validate_hud(layout) && layout >= 0) layout--;
		if (layout >= 0 && layout < hud_size && gi->validate_hud(layout))
		{
			hud_size = layout;
			return true;
		}
	}
	else if (direction > 0 && hud_size < 11)
	{
		int layout = hud_size + 1;
		while (!gi->validate_hud(layout) && layout <= 11) layout++;
		if (layout <= 11 && layout > hud_size && gi->validate_hud(layout))
		{
			hud_size = layout;
			return true;
		}
	}
	return false;
}

int hud_statusbarrange;	// will be set by the game's configuration setup.
CUSTOM_CVARD(Int, hud_custom, 0, CVAR_ARCHIVE|CVAR_NOINITCALL, "change the custom hud") // this has no backing implementation, it seems to be solely for scripted HUDs.
{
	if (self < 0) self = 0;
	else if (self > 0 && self >= hud_statusbarrange) self = hud_statusbarrange - 1;
}

CVARD(Bool, hud_stats, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable level statistics display")
CVARD(Bool, hud_showmapname, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable map name display on load")
CVARD(Bool, hud_position, false, CVAR_ARCHIVE, "aligns the status bar to the bottom/top")
CVARD(Bool, hud_bgstretch, false, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "enable/disable background image stretching in wide resolutions")
CVARD(Int, hud_messagetime, 120, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "length of time to display multiplayer chat messages")
CUSTOM_CVARD(Int, hud_messages, 1, CVAR_ARCHIVE, "enable/disable showing messages")
{
	if (self < 0 || self > 2) self = 1;
}

// This cannot be done with the 'toggle' CCMD because it needs to control itself when to output the message
CCMD (togglemessages)
{
	if (hud_messages)
	{
		gi->PrintMessage(PRINT_MEDIUM, "%s\n", quoteMgr.GetQuote(24));
		hud_messages = false;
	}
	else
	{
		hud_messages = true;
		gi->PrintMessage(PRINT_MEDIUM, "%s\n", quoteMgr.GetQuote(23));
	}
}



//{
	//Blood::gGameMessageMgr.SetState(self); // this is for terminaing an active message. Cannot be done like this because CVARs are global.
//}

CVARD_NAMED(Int, hud_numbertile, althud_numbertile, 2930, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "first tile in alt hud number set")
CVARD_NAMED(Int, hud_numberpal, althud_numberpal, 0, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "pal for alt hud numbers")
CVARD_NAMED(Int, hud_shadows, althud_shadows, true, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "enable/disable althud shadows")
CVARD_NAMED(Int, hud_flashing, althud_flashing, true, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "enable/disable althud flashing")
CVARD(Bool, hud_glowingquotes, true, CVAR_ARCHIVE, "enable/disable \"glowing\" quote text")

CUSTOM_CVARD(Int, hud_textscale, 200, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "sets multiplayer chat message size")
{
	if (self < 100) self = 100;
	else if (self > 400) self = 400;
}

CUSTOM_CVARD(Int, hud_weaponscale, 100, CVAR_ARCHIVE|CVAR_FRONTEND_DUKELIKE, "changes the weapon scale")
{
	if (self < 30) self = 30;
	else if (self > 100) self = 100;
}

CUSTOM_CVARD(Int, r_fov, 90, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "change the field of view")
{
	if (self < 60) self = 60;
	else if (self > 140) self = 140;
}

CVARD(Bool, r_horizcenter, false, CVAR_ARCHIVE|CVAR_FRONTEND_BLOOD, "enable/disable centered horizon line") // only present in Blood, maybe add to others?

CVARD(Bool, in_mousemode, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "toggles vertical mouse view")

CVAR(Bool, silentmouseaimtoggle, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

CCMD(togglemouseaim)
{
	in_mousemode = !in_mousemode;
	if (!silentmouseaimtoggle)
	{
		gi->DoPrintMessage(PRINT_MEDIUM, in_mousemode? GStrings("TXT_MOUSEAIMON") : GStrings("TXT_MOUSEAIMOFF"));
	}
}

CVARD(Bool, in_mouseflip, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "invert vertical mouse movement")

CUSTOM_CVARD(Int, in_mousebias, 0, CVAR_GLOBALCONFIG|CVAR_ARCHIVE, "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time")
{
	if (self < 0) self = 0;
	else if (self > 32) self = 32;
}

CVARD(Bool, in_mousesmoothing, false, CVAR_GLOBALCONFIG|CVAR_ARCHIVE, "enable/disable mouse input smoothing")

CUSTOM_CVARD(Float, in_mousesensitivity, 1, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "changes the mouse sensitivity")
{
	if (self < 0) self = 0;
	else if (self > 6) self = 6;
}

CUSTOM_CVARD(Float, in_mousescalex, 1, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "changes the mouse sensitivity")
{
	if (self < -4) self = 4;
	else if (self > 4) self = 4;
}

CUSTOM_CVARD(Float, in_mousescaley, 1, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "changes the mouse sensitivity")
{
	if (self < -4) self = 4;
	else if (self > 4) self = 4;
}


CUSTOM_CVARD(Int, r_drawweapon, 1, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable weapon drawing")
{
	if (self < 0 || self > 2) self = 1;
}

ADD_STAT(fps)
{
	return gi->statFPS();
}

ADD_STAT(coord)
{
	return gi->GetCoordString();
}

CUSTOM_CVARD(Int, r_showfps, 0, 0, "show the frame rate counter")
{
	if (self < 0 || self > 3) self = 1;
	FStat::EnableStat("fps", self != 0);
}

CUSTOM_CVARD(Int, r_showfpsperiod, 0, 0, "time in seconds before averaging min and max stats for r_showfps 2+")
{
	if (self < 0 || self > 5) self = 1;
}

float r_ambientlightrecip;

CUSTOM_CVARD(Float, r_ambientlight, 1.0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "sets the global map light level")
{
	if (self < 0.1f) self = 0.1f;
	else if (self > 10.f) self = 10.f;
	else r_ambientlightrecip = 1.f / self;
}

CVARD(Bool, r_shadows, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable sprite and model shadows")

// Gross hack stuff. Only settable from the command line
CVARD(Bool, r_rotatespritenowidescreen, false, CVAR_NOSET, "pass bit 1024 to all CON rotatesprite calls")
CVARD(Bool, r_precache, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable the pre-level caching routine")

CUSTOM_CVARD(Int, r_maxfps, 200, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "limit the frame rate")
{
	if (self < 0) self = 0;
	else if (self > 0 && self < 30) self = 30;
	else if (self > 1000) self = 1000;
}

int G_FPSLimit(void)
{
    if (r_maxfps <= 0)
        return 1;
	
	auto frameDelay = timerGetFreqU64()/(double)r_maxfps;

    static double   nextPageDelay;
    static uint64_t lastFrameTicks;

    nextPageDelay = clamp(nextPageDelay, 0.0, frameDelay);

    uint64_t const frameTicks   = timerGetTicksU64();
    uint64_t const elapsedTime  = frameTicks - lastFrameTicks;
    double const   dElapsedTime = elapsedTime;

    if (dElapsedTime >= nextPageDelay)
    {
        if (dElapsedTime <= nextPageDelay+frameDelay)
            nextPageDelay += frameDelay-dElapsedTime;

        lastFrameTicks = frameTicks;

        return 1;
    }

    return 0;
}

CUSTOM_CVARD(String, wchoice, "3457860291", CVAR_ARCHIVE | CVAR_NOINITCALL | CVAR_FRONTEND_DUKELIKE, "sets weapon autoselection order")
{
	char dest[11];
	char const* c = self;
	if (*c)
	{
		int j = 0;

		while (*c && j < 10)
		{
			dest[j] = *c - '0';
			c++;
			j++;
		}

		while (j < 10)
		{
			if (j == 9)
				dest[9] = 1;
			else
				dest[j] = 2;

			j++;
		}
		// if (!gi->SetWeaponChoice(dest)) Printf("Weapon ordering not supported\n");
	}
	else
	{
		Printf("Using default weapon orders.\n");
		self = "3457860291";
	}
}


CVARD(Bool, r_voxels, true, CVAR_ARCHIVE, "enable/disable automatic sprite->voxel rendering")


//==========================================================================
//
// Global setup that formerly wasn't CVARs but merely global stuff saved in the config.
//
//==========================================================================

CVAR(Bool, displaysetup, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
bool gNoAutoLoad;	// for overrides from the def files
CVAR(Bool, noautoload, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

bool G_AllowAutoload()
{
	if (noautoload || gNoAutoLoad || Args->CheckParm("-noautoload")) return false;
	return true;
}


// color code format is as follows:
// ^## sets a color, where ## is the palette number
// ^S# sets a shade, range is 0-7 equiv to shades 0-14
// ^O resets formatting to defaults

static const char* OSD_StripColors(char* outBuf, const char* inBuf)
{
	const char* ptr = outBuf;

	while (*inBuf)
	{
		if (*inBuf == '^')
		{
			if (isdigit(*(inBuf + 1)))
			{
				inBuf += 2 + !!isdigit(*(inBuf + 2));
				continue;
			}
			else if ((toupper(*(inBuf + 1)) == 'O'))
			{
				inBuf += 2;
				continue;
			}
			else if ((toupper(*(inBuf + 1)) == 'S') && isdigit(*(inBuf + 2)))
			{
				inBuf += 3;
				continue;
			}
		}
		*(outBuf++) = *(inBuf++);
	}

	*outBuf = '\0';
	return ptr;
}

CVAR(Bool, adult_lockout, false, CVAR_ARCHIVE)
CUSTOM_CVAR(String, playername, "Player", CVAR_ARCHIVE | CVAR_USERINFO)
{
	TArray<char> buffer(strlen(self)+1, 1);
	OSD_StripColors(buffer.Data(), self);
	if (buffer.Size() < strlen(self))
	{
		self = buffer.Data();
	}
	//Net_SendClientInfo();	This is in the client code. Todo.
}


CUSTOM_CVAR(String, rtsname, "", CVAR_ARCHIVE | CVAR_USERINFO)
{
	RTS_Init(self);
}

CVAR(String, usermapfolder, "", CVAR_ARCHIVE);

CUSTOM_CVAR(Int, playercolor, 0, CVAR_ARCHIVE|CVAR_USERINFO)
{
	if (self < 0 || self > 10) self = 0;
	else ;// gi->UpdatePlayerColor(); // this part is game specific
}

CUSTOM_CVAR(Int, playerteam, 0, CVAR_USERINFO) // this one is transient and won't be saved.
{
	if (self < 0 || self > 3) self = 0;
	else ;// gi->UpdatePlayerTeam(); // this part is game specific
}

// Will only become useful if the obituary system gets overhauled and for localization
CUSTOM_CVAR(Int, playergender, 0, CVAR_USERINFO|CVAR_ARCHIVE)
{
	if (self < 0 || self > 3) self = 0;
}


// Internal settings for demo recording and the multiplayer menu. These won't get saved and only are CVARs so that the menu code can use them.
CVAR(Int, m_recstat, false, CVAR_NOSET)
CVAR(Int, m_coop, 0, CVAR_NOSET)
CVAR(Int, m_ffire, 1, CVAR_NOSET)
CVAR(Int, m_monsters, 1, CVAR_NOSET)
CVAR(Int, m_marker, 1, CVAR_NOSET)
CVAR(Int, m_level_number, 0, CVAR_NOSET)
CVAR(Int, m_episode_number, 0, CVAR_NOSET)
CVAR(Int, m_noexits, 0, CVAR_NOSET)
CVAR(String, m_server, "localhost", CVAR_NOSET)
CVAR(String, m_netport, "19014", CVAR_NOSET)

#if 0

/*


	// Currently unavailable due to dependency on an obsolete OpenGL feature
	{ "deliriumblur", "enable/disable delirium blur effect(polymost)", (void *)&gDeliriumBlur, CVAR_BOOL, 0, 1 },

	// This one gets changed at run time by the game code, so making it persistent does not work

	// This option is not really useful anymore
	{ "r_camrefreshdelay", "minimum delay between security camera sprite updates, 120 = 1 second", (void *)&ud.camera_time, CVAR_INT, 1, 240 },

	// This requires a different approach, because it got used like a CCMD, not a CVAR.
	{ "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },

	// just as a reminder:
	/*
	else if (!Bstrcasecmp(parm->name, "vid_gamma"))
	{
	}
	else if (!Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
	{
	}
	*/


#endif
