#include "c_cvars.h"
#include "c_dispatch.h"
#include "v_video.h"
#include "hw_cvars.h"
#include "menu/menu.h"

//==========================================================================
//
// Texture CVARs
//
//==========================================================================
#if 0 // left as a reminder that this will have to be refactored later.
CUSTOM_CVAR(Float,gl_texture_filter_anisotropic,8.0f,CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_NOINITCALL)
{
	screen->TextureFilterChanged();
}

CCMD(gl_flush)
{
	//TexMan.FlushAll();
}

CUSTOM_CVAR(Int, gl_texture_filter, 4, CVAR_ARCHIVE|CVAR_GLOBALCONFIG|CVAR_NOINITCALL)
{
	if (self < 0 || self > 6) self=4;
	screen->TextureFilterChanged();
}
CVAR(Bool, gl_precache, false, CVAR_ARCHIVE)
#endif

