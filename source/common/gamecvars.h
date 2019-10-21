#include "c_cvars.h"

EXTERN_CVAR(Bool, cl_crosshair)
EXTERN_CVAR(Bool, cl_automsg)
EXTERN_CVAR(Int, cl_autoaim)
EXTERN_CVAR(Bool, cl_autorun)
EXTERN_CVAR(Bool, cl_runmode)

CUSTOM_CVARD(Int, cl_autoaim, 1, CVAR_ARCHIVE, "enable/disable weapon autoaim")
{
	if (self < 0 || self > (playing_blood? 2 : 3)) self = 1;	// Note: The Shadow Warrior backend only has a bool for this.
	//UpdatePlayerFromMenu(); todo: networking (only operational in EDuke32 frontend anyway.)
};


bool G_CheckAutorun(bool button);
