#include "compat.h"
#include "osd.h"
#include "build.h"
#include "engineinfo.h"
#include "baselayer.h"

#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif

#ifdef USE_OPENGL
struct glinfo glinfo = {
	"Unknown",	// vendor
	"Unknown",	// renderer
	"0.0.0",	// version
	"",		// extensions
	
	1.0,		// max anisotropy
	0,		// brga texture format
	0,		// clamp-to-edge support
	0,		// texture compression
	0,		// non-power-of-two textures
	0,		// multisampling
	0,		// nvidia multisampling hint
};
#endif

static int osdfunc_dumpbuildinfo(const osdfuncparm_t *parm)
{
	OSD_Printf(
		"Build engine compilation:\n"
		"  CFLAGS: %s\n"
		"  LIBS: %s\n"
		"  Host: %s\n"
		"  Compiler: %s\n"
		"  Built: %s\n",
		_engine_cflags,
		_engine_libs,
		_engine_uname,
		_engine_compiler,
		_engine_date);

	return OSDCMD_OK;
}

static void onvideomodechange(int newmode) { }
void (*baselayer_onvideomodechange)(int) = onvideomodechange;

static int osdfunc_setrendermode(const osdfuncparm_t *parm)
{
	int m;
	char *p;

	char *modestrs[] = {
		"classic software", "polygonal flat-shaded software",
		"polygonal textured software", "polygonal OpenGL"
	};

	if (parm->numparms != 1) return OSDCMD_SHOWHELP;
	m = Bstrtol(parm->parms[0], &p, 10);

	if (m < 0 || m > 3) return OSDCMD_SHOWHELP;

	setrendermode(m);
	OSD_Printf("Rendering method changed to %s\n", modestrs[ getrendermode() ] );

	return OSDCMD_OK;
}

#if defined(POLYMOST) && defined(USE_OPENGL)
#ifdef DEBUGGINGAIDS
static int osdcmd_hicsetpalettetint(const osdfuncparm_t *parm)
{
	long pal, cols[3], eff;
	char *p;
	
	if (parm->numparms != 5) return OSDCMD_SHOWHELP;
	
	pal = Batol(parm->parms[0]);
	cols[0] = Batol(parm->parms[1]);
	cols[1] = Batol(parm->parms[2]);
	cols[2] = Batol(parm->parms[3]);
	eff = Batol(parm->parms[4]);
	
	hicsetpalettetint(pal,cols[0],cols[1],cols[2],eff);
	
	return OSDCMD_OK;
}
#endif

static int osdcmd_glinfo(const osdfuncparm_t *parm)
{
	char *s,*t,*u,i;
	
	if (bpp == 8) {
		OSD_Printf("glinfo: Not in OpenGL mode.\n");
		return OSDCMD_OK;
	}
	
	OSD_Printf("OpenGL Information:\n"
	           " Version:  %s\n"
		   " Vendor:   %s\n"
		   " Renderer: %s\n"
		   " Maximum anisotropy:      %.1f%s\n"
		   " BGRA textures:           %s\n"
		   " Non-x^2 textures:        %s\n"
		   " Texure compression:      %s\n"
		   " Clamp-to-edge:           %s\n"
		   " Multisampling:           %s\n"
		   " Nvidia multisample hint: %s\n"
		   " Extensions:\n",
		   	glinfo.version,
			glinfo.vendor,
			glinfo.renderer,
			glinfo.maxanisotropy, glinfo.maxanisotropy>1.0?"":" (no anisotropic filtering)",
			glinfo.bgra ? "supported": "not supported",
			glinfo.texnpot ? "supported": "not supported",
			glinfo.texcompr ? "supported": "not supported",
			glinfo.clamptoedge ? "supported": "not supported",
			glinfo.multisample ? "supported": "not supported",
			glinfo.nvmultisamplehint ? "supported": "not supported"
		);

	s = Bstrdup(glinfo.extensions);
	if (!s) OSD_Printf(glinfo.extensions);
	else {
		i = 0; t = u = s;
		while (*t) {
			if (*t == ' ') {
				if (i&1) {
					*t = 0;
					OSD_Printf("   %s\n",u);
					u = t+1;
				}
				i++;
			}
			t++;
		}
		if (i&1) OSD_Printf("   %s\n",u);
		Bfree(s);
	}
	
	return OSDCMD_OK;
}
#endif

static int osdcmd_vars(const osdfuncparm_t *parm)
{
	int showval = (parm->numparms < 1);
	
	if (!Bstrcasecmp(parm->name, "screencaptureformat")) {
		const char *fmts[2][2] = { {"TGA", "PCX"}, {"0", "1"} };
		if (showval) { OSD_Printf("captureformat is %s\n", fmts[captureformat]); }
		else {
			int i,j;
			for (j=0; j<2; j++)
				for (i=0; i<2; i++)
					if (!Bstrcasecmp(parm->parms[0], fmts[j][i])) break;
			if (j == 2) return OSDCMD_SHOWHELP;
			captureformat = i;
		}
		return OSDCMD_OK;
	}
#ifdef SUPERBUILD
	else if (!Bstrcasecmp(parm->name, "novoxmips")) {
		if (showval) { OSD_Printf("novoxmips is %d\n", novoxmips); }
		else { novoxmips = (atoi(parm->parms[0]) != 0); }
	}
	else if (!Bstrcasecmp(parm->name, "usevoxels")) {
		if (showval) { OSD_Printf("usevoxels is %d\n", usevoxels); }
		else { usevoxels = (atoi(parm->parms[0]) != 0); }
	}
#endif
	return OSDCMD_SHOWHELP;
}

int baselayer_init(void)
{
#ifdef POLYMOST
	OSD_RegisterFunction("setrendermode","setrendermode <number>: sets the engine's rendering mode.\n"
			"Mode numbers are:\n"
			"   0 - Classic Build software\n"
			"   1 - Polygonal flat-shaded software\n"
			"   2 - Polygonal textured software\n"
#ifdef USE_OPENGL
			"   3 - Polygonal OpenGL\n"
#endif
			,
			osdfunc_setrendermode);
#endif
	OSD_RegisterFunction("dumpbuildinfo","dumpbuildinfo: outputs engine compilation information",osdfunc_dumpbuildinfo);
	OSD_RegisterFunction("screencaptureformat","screencaptureformat: sets the output format for screenshots (TGA or PCX)",osdcmd_vars);
#ifdef SUPERBUILD
	OSD_RegisterFunction("novoxmips","novoxmips: turn off/on the use of mipmaps when rendering 8-bit voxels",osdcmd_vars);
	OSD_RegisterFunction("usevoxels","usevoxels: enable/disable automatic sprite->voxel rendering",osdcmd_vars);
#endif
#if defined(POLYMOST) && defined(USE_OPENGL)
#ifdef DEBUGGINGAIDS
	OSD_RegisterFunction("hicsetpalettetint","hicsetpalettetint: sets palette tinting values",osdcmd_hicsetpalettetint);
#endif
	OSD_RegisterFunction("glinfo","glinfo: shows OpenGL information about the current OpenGL mode",osdcmd_glinfo);
#endif
	
	return 0;
}

