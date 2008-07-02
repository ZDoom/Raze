#include "compat.h"
#include "osd.h"
#include "build.h"
#include "baselayer.h"

#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif

char scantoasc[128] =
{
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
    'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

#ifdef USE_OPENGL
struct glinfo glinfo =
{
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
    0,       // ARBfp
    0,       // depth textures
    0,       // shadow comparison
    0,       // Frame Buffer Objects
    0,       // rectangle textures
    0,       // multitexturing
    0,       // env_combine
    0,       // Vertex Buffer Objects
    0,       // GL info dumped
};
#endif

static void onvideomodechange(int newmode) { UNREFERENCED_PARAMETER(newmode); }
void (*baselayer_onvideomodechange)(int) = onvideomodechange;

static int osdfunc_setrendermode(const osdfuncparm_t *parm)
{
    int m;
    char *p;

    char *modestrs[] =
    {
        "classic software", "polygonal flat-shaded software",
        "polygonal textured software", "polygonal OpenGL", "great justice"
    };

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    m = Bstrtol(parm->parms[0], &p, 10);

    if (m < 0 || m > 4) return OSDCMD_SHOWHELP;

    setrendermode(m);
    OSD_Printf("Rendering method changed to %s\n", modestrs[ getrendermode()]);

    return OSDCMD_OK;
}

#if defined(POLYMOST) && defined(USE_OPENGL)
#ifdef DEBUGGINGAIDS
static int osdcmd_hicsetpalettetint(const osdfuncparm_t *parm)
{
    int pal, cols[3], eff;

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

int osdcmd_glinfo(const osdfuncparm_t *parm)
{
    char *s,*t,*u,i;

    UNREFERENCED_PARAMETER(parm);

    if (bpp == 8)
    {
        initprintf("glinfo: Not in OpenGL mode.\n");
        return OSDCMD_OK;
    }

    initprintf("OpenGL Information:\n"
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
               " ARBfp fragment programs: %s\n"
               " Depth textures:          %s\n"
               " Shadow textures:         %s\n"
               " Frame Buffer Objects:    %s\n"
               " Rectangle textures:      %s\n"
               " Multitexturing:          %s\n"
               " env_combine:             %s\n"
               " Vertex Buffer Objects:   %s\n"
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
               glinfo.nvmultisamplehint ? "supported": "not supported",
               glinfo.arbfp ? "supported": "not supported",
               glinfo.depthtex ? "supported": "not supported",
               glinfo.shadow ? "supported": "not supported",
               glinfo.fbos ? "supported": "not supported",
               glinfo.rect ? "supported": "not supported",
               glinfo.multitex ? "supported": "not supported",
               glinfo.envcombine ? "supported": "not supported",
               glinfo.vbos ? "supported": "not supported"
              );

    s = Bstrdup(glinfo.extensions);
    if (!s) initprintf(glinfo.extensions);
    else
    {
        i = 0; t = u = s;
        while (*t)
        {
            if (*t == ' ')
            {
                if (i&1)
                {
                    *t = 0;
                    initprintf("   %s\n",u);
                    u = t+1;
                }
                i++;
            }
            t++;
        }
        if (i&1) initprintf("   %s\n",u);
        Bfree(s);
    }

    return OSDCMD_OK;
}
#endif

static int osdcmd_vars(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);

    if (!Bstrcasecmp(parm->name, "screencaptureformat"))
    {
        const char *fmts[] = {"TGA", "PCX"};
        if (showval) { OSD_Printf("captureformat is %s\n", fmts[captureformat]); }
        else
        {
            int j;
            for (j=0; j<2; j++)
                if (!Bstrcasecmp(parm->parms[0], fmts[j])) break;
            if (j == 2) return OSDCMD_SHOWHELP;
            captureformat = j;
        }
        return OSDCMD_OK;
    }
#ifdef SUPERBUILD
    else if (!Bstrcasecmp(parm->name, "novoxmips"))
    {
        if (showval) { OSD_Printf("novoxmips is %d\n", novoxmips); }
        else { novoxmips = (atoi(parm->parms[0]) != 0); }
    }
    else if (!Bstrcasecmp(parm->name, "usevoxels"))
    {
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
//                         "   4 - great justice renderer\n"
#endif
                         ,
                         osdfunc_setrendermode);
#endif
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

