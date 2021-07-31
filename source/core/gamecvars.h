#pragma once
#include "c_cvars.h"

EXTERN_CVAR(Bool, cl_crosshair)
EXTERN_CVAR(Bool, cl_automsg)
EXTERN_CVAR(Int, cl_autoaim)
EXTERN_CVAR(Bool, cl_autorun)
EXTERN_CVAR(Bool, cl_runmode)
EXTERN_CVAR(Bool, cl_autosave)
EXTERN_CVAR(Bool, cl_autosavedeletion)
EXTERN_CVAR(Int, cl_maxautosaves)
EXTERN_CVAR(Bool, cl_obituaries)
EXTERN_CVAR(Bool, cl_idplayers)
EXTERN_CVAR(Bool, cl_viewbob)
EXTERN_CVAR(Bool, cl_weaponsway)
EXTERN_CVAR(Bool, cl_viewhbob)
EXTERN_CVAR(Bool, cl_viewvbob)
EXTERN_CVAR(Bool, cl_interpolate)
EXTERN_CVAR(Bool, cl_slopetilting)
EXTERN_CVAR(Int, cl_showweapon)
EXTERN_CVAR(Int, cl_weaponswitch)
EXTERN_CVAR(Float, crosshairscale)
EXTERN_CVAR(Bool, cl_sointerpolation)
EXTERN_CVAR(Bool, cl_syncinput)
EXTERN_CVAR(Bool, cl_swsmoothsway)
EXTERN_CVAR(Bool, cl_showmagamt)
EXTERN_CVAR(Bool, cl_nomeleeblur)
EXTERN_CVAR(Bool, cl_exhumedoldturn)
EXTERN_CVAR(Bool, cl_hudinterpolation)
EXTERN_CVAR(Bool, cl_bloodvanillarun)
EXTERN_CVAR(Bool, cl_bloodvanillabobbing)
EXTERN_CVAR(Bool, cl_bloodhudinterp)

EXTERN_CVAR(Bool, demorec_seeds_cvar)
EXTERN_CVAR(Bool, demoplay_diffs)
EXTERN_CVAR(Bool, demoplay_showsync)
EXTERN_CVAR(Bool, demorec_diffs_cvar)
EXTERN_CVAR(Bool, demorec_force_cvar)
EXTERN_CVAR(Int, demorec_difftics_cvar)

EXTERN_CVAR(Bool, snd_ambience)
EXTERN_CVAR(Bool, snd_tryformats)
EXTERN_CVAR(Bool, mus_enabled)
EXTERN_CVAR(Bool, mus_restartonload)
EXTERN_CVAR(Bool, mus_redbook)
EXTERN_CVAR(Int, snd_numchannels)
EXTERN_CVAR(Int, snd_numvoices)
EXTERN_CVAR(Int, snd_speech)
EXTERN_CVAR(Int, mus_device)

EXTERN_CVAR(Int, hud_layout)
EXTERN_CVAR(Float, hud_scalefactor)
EXTERN_CVAR(Int, hud_size)
EXTERN_CVAR(Float, hud_statscale)

EXTERN_CVAR(Int, hud_custom)
EXTERN_CVAR(Bool, hud_stats)
EXTERN_CVAR(Bool, hud_showmapname)
EXTERN_CVAR(Bool, hud_position)
EXTERN_CVAR(Bool, hud_bgstretch)
EXTERN_CVAR(Int, hud_textscale)
EXTERN_CVAR(Bool, hud_messages)

EXTERN_CVAR(Int, althud_numbertile)
EXTERN_CVAR(Int, althud_numberpal)
EXTERN_CVAR(Int, althud_shadows)
EXTERN_CVAR(Int, althud_flashing)

EXTERN_CVAR(Bool, am_textfont)
EXTERN_CVAR(Bool, am_showlabel)
EXTERN_CVAR(Bool, am_nameontop)


EXTERN_CVAR(Int, r_fov)
EXTERN_CVAR(Int, r_drawweapon)
EXTERN_CVAR(Int, r_showfps)
EXTERN_CVAR(Int, r_showfpsperiod)
EXTERN_CVAR(Float, r_ambientlight)
EXTERN_CVAR(Bool, r_shadows)
EXTERN_CVAR(Bool, r_precache)
EXTERN_CVAR(Bool, r_voxels)
EXTERN_CVAR(Int, r_upscalefactor)

EXTERN_CVAR(Float, vid_gamma)
EXTERN_CVAR(Float, vid_contrast)
EXTERN_CVAR(Float, vid_brightness)
EXTERN_CVAR(Int, gl_multisample)
EXTERN_CVAR(Int, gl_ssao)

EXTERN_CVAR(Bool, use_joystick)
EXTERN_CVAR(Bool, in_mousemode)

EXTERN_CVAR(Bool, adult_lockout)
EXTERN_CVAR(String, playername)
EXTERN_CVAR(String, rtsname)
EXTERN_CVAR(String, usermapfolder)

EXTERN_CVAR(Int, m_recstat)
EXTERN_CVAR(Int, m_coop)
EXTERN_CVAR(Int, m_ffire)
EXTERN_CVAR(Int, m_noexits)
EXTERN_CVAR(Int, playercolor)

inline const char* PlayerName(int pindex)
{
	// Todo: proper implementation of user CVARs.
	return playername;
}

inline int Autoaim(int player)
{
	// Todo: proper implementation of user CVARs.
	return cl_autoaim;
}

extern bool gNoAutoLoad;
extern int hud_statusbarrange;	// will be set by the game's configuration setup.
bool G_CheckAutorun(bool button);
inline int G_FPSLimit(void) { return 1; }
bool G_AllowAutoload();

enum EHudSize
{
	Hud_Current = -1,
	Hud_Frame50 = 0,
	Hud_Frame60,
	Hud_Frame70,
	Hud_Frame80,
	Hud_Frame90,
	Hud_Stbar,
	Hud_StbarOverlay,
	Hud_Mini,
	Hud_full,
	Hud_Nothing,
	Hud_MAX
};
