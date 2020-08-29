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
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "rts.h"
#include "stats.h"
#include "raze_music.h"
#include "c_dispatch.h"
#include "gstrings.h"
#include "quotemgr.h"
#include "gamestruct.h"
#include "statusbar.h"

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
CVARD(Bool, cl_obituaries, true, CVAR_ARCHIVE, "enable/disable multiplayer death messages") // Not implemented 
CVARD(Bool, cl_idplayers, true, CVAR_ARCHIVE, "enable/disable name display when aiming at opponents")
CVARD(Bool, cl_weaponsway, true, CVAR_ARCHIVE, "enable/disable player weapon swaying")

// Todo: Consolidate these to be consistent across games?
CVARD(Bool, cl_viewbob, true, CVAR_ARCHIVE, "enable/disable player head bobbing")
CVARD(Bool, cl_viewhbob, true, CVAR_ARCHIVE, "enable/disable view horizontal bobbing") // Only implemented in Blood
CVARD(Bool, cl_viewvbob, true, CVAR_ARCHIVE, "enable/disable view vertical bobbing") // Only implemented in Blood

CVARD(Bool, cl_interpolate, true, CVAR_ARCHIVE, "enable/disable view interpolation") // only implemented in Blood
CVARD(Bool, cl_slopetilting, false, CVAR_ARCHIVE, "enable/disable slope tilting") // only implemented in Blood
CVARD(Int, cl_showweapon, 1, CVAR_ARCHIVE, "enable/disable show weapons") // only implemented in Blood
CVARD(Bool, cl_sointerpolation, true, CVAR_ARCHIVE, "enable/disable sector object interpolation") // only implemented in SW
CVARD(Bool, cl_syncinput, false, CVAR_ARCHIVE, "enable/disable synchronized input with game's ticrate") // only implemented in Duke
CVARD(Bool, cl_swaltnukeinit, false, CVAR_ARCHIVE, "enable/disable SW alternative nuke initialisation") // only implemented in SW
CVARD(Bool, cl_dukefixrpgrecoil, false, CVAR_ARCHIVE, "soften recoil of Duke 3D's RPG")
CVARD(Bool, cl_smoothsway, false, CVAR_ARCHIVE, "move SW weapon left and right smoothly while bobbing")
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

// Sound

CUSTOM_CVARD(Bool, snd_ambience, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL, "enables/disables ambient sounds") // Not implemented for Blood
{
	gi->SetAmbience(self);
}
CVARD(Bool, snd_enabled, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables sound effects")
CVARD(Bool, snd_tryformats, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables automatic discovery of replacement sounds and music in .flac and .ogg formats")

CVARD(Bool, mus_restartonload, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "restart the music when loading a saved game with the same map or not")
CVARD(Bool, mus_redbook, true, CVAR_ARCHIVE, "enables/disables redbook audio") 

CUSTOM_CVARD(Int, snd_speech, 1, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enables/disables player speech")
{
	if (self < 0) self = 0;
	else if (self > 1) self = 1;
}


// HUD

CUSTOM_CVARD(Int, hud_size, Hud_Stbar, CVAR_ARCHIVE, "Defines the HUD size and style")
{
	if (self < 0) self = 0;
	else if (self > Hud_Nothing) self = Hud_Nothing;
	else setViewport(self);
}

CUSTOM_CVARD(Float, hud_scale, 1, CVAR_ARCHIVE, "changes the hud scale")
{
	if (self < 0.36f) self = 0.36f;
	else if (self > 1) self = 1;
	else setViewport(hud_size);
}

// Note: The shift detection here should be part of the key event data, but that requires a lot more work. Ideally use a ShiftBinds mapping. For control through bound keys this should be fine, bunt not for use from the console.
CCMD(sizeup)
{
	if (!inputState.ShiftPressed())
	{
		if (hud_size < Hud_Nothing)
		{
			hud_size = hud_size + 1;
			gi->PlayHudSound();
		}
	}
	else
	{
		hud_scale = hud_scale + 0.04;
	}
}

CCMD(sizedown)
{
	if (!inputState.ShiftPressed())
	{
		if (hud_size > 0)
		{
			hud_size = hud_size - 1;
			gi->PlayHudSound();
		}
	}
	else
	{
		hud_scale = hud_scale - 0.04;
	}
}



CUSTOM_CVARD(Float, hud_statscale, 0.5, CVAR_ARCHIVE, "change the scale of the stats display")
{
	if (self < 0.36f) self = 0.36f;
	else if (self > 1) self = 1;
}


CVARD(Bool, hud_stats, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable level statistics display")
CVARD(Bool, hud_showmapname, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable map name display on load")
CVARD(Bool, hud_position, false, CVAR_ARCHIVE, "aligns the status bar to the bottom/top")
CVARD(Bool, hud_bgstretch, false, CVAR_ARCHIVE, "enable/disable background image stretching in wide resolutions")
CVARD(Bool, hud_messages, 1, CVAR_ARCHIVE, "enable/disable showing messages")

// This cannot be done with the 'toggle' CCMD because it needs to control itself when to output the message
CCMD (togglemessages)
{
	if (hud_messages)
	{
		Printf(PRINT_MEDIUM | PRINT_NOTIFY, "%s\n", quoteMgr.GetQuote(24));
		hud_messages = false;
	}
	else
	{
		hud_messages = true;
		Printf(PRINT_MEDIUM | PRINT_NOTIFY, "%s\n", quoteMgr.GetQuote(23));
	}
}

CVARD_NAMED(Int, hud_flashing, althud_flashing, true, CVAR_ARCHIVE, "enable/disable althud flashing")
CUSTOM_CVARD(Int, r_fov, 90, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "change the field of view")
{
	if (self < 60) self = 60;
	else if (self > 140) self = 140;
}

CVARD(Bool, r_horizcenter, false, CVAR_ARCHIVE, "enable/disable centered horizon line") // only present in Blood.

CVARD(Bool, in_mousemode, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "toggles vertical mouse view")

CVAR(Bool, silentmouseaimtoggle, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

CCMD(togglemouseaim)
{
	in_mousemode = !in_mousemode;
	if (!silentmouseaimtoggle)
	{
		Printf(PRINT_MEDIUM|PRINT_NOTIFY, "%s\n", in_mousemode? GStrings("TXT_MOUSEAIMON") : GStrings("TXT_MOUSEAIMOFF"));
	}
}

CVARD(Bool, in_mouseflip, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "invert vertical mouse movement")

CUSTOM_CVARD(Int, in_mousebias, 0, CVAR_GLOBALCONFIG|CVAR_ARCHIVE, "emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time")
{
	if (self < 0) self = 0;
	else if (self > 32) self = 32;
}

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

CUSTOM_CVARD(Float, r_ambientlight, 1.0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "sets the global map light level")
{
	if (self < 0.1f) self = 0.1f;
	else if (self > 10.f) self = 10.f;
}

CVARD(Bool, r_shadows, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable sprite and model shadows")

CVARD(Bool, r_precache, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG, "enable/disable the pre-level caching routine")

CVARD(Bool, r_voxels, true, CVAR_ARCHIVE, "enable/disable automatic sprite->voxel rendering")


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
	else ;// gi->PlayerColorChanged(); // this part is game specific
}

// Will only become useful if the obituary system gets overhauled and for localization
CUSTOM_CVAR(Int, playergender, 0, CVAR_USERINFO|CVAR_ARCHIVE)
{
	if (self < 0 || self > 3) self = 0;
}


CVAR(Bool, am_textfont, false, CVAR_ARCHIVE)
CVAR(Bool, am_showlabel, false, CVAR_ARCHIVE)
CVAR(Bool, am_nameontop, false, CVAR_ARCHIVE)


CVAR(Int, m_coop, 0, CVAR_NOSET)

#if 0

/*


	// Currently unavailable due to dependency on an obsolete OpenGL feature
	{ "deliriumblur", "enable/disable delirium blur effect(polymost)", (void *)&gDeliriumBlur, CVAR_BOOL, 0, 1 },

	// This requires a different approach, because it got used like a CCMD, not a CVAR.
	{ "skill","changes the game skill setting", (void *)&ud.m_player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOSAVE/*|CVAR_NOMULTI*/, 0, 5 },


#endif
