#include "compat.h"
#include "osd.h"
#include "build.h"
#include "baselayer.h"

#include "renderlayer.h"

#include "a.h"
#include "polymost.h"

// input
char inputdevices=0;
char keystatus[KEYSTATUSSIZ], keyfifo[KEYFIFOSIZ], keyasciififo[KEYFIFOSIZ];
uint8_t keyfifoplc, keyfifoend, keyasciififoplc, keyasciififoend;
char keyremap[KEYSTATUSSIZ];
int32_t keyremapinit=0;
char key_names[NUMKEYS][24];
int32_t mousex=0,mousey=0,mouseb=0;
vec2_t mouseabs;
uint8_t mousepressstate;
uint8_t moustat = 0, mousegrab = 0, mouseinwindow = 1, AppMouseGrab = 1;

int32_t mousepressstateadvance(void)
{
    if (mousepressstate == Mouse_Pressed)
    {
        mousepressstate = Mouse_Held;
        return 1;
    }
    else if (mousepressstate == Mouse_Released)
    {
        mousepressstate = Mouse_Idle;
        return 1;
    }
    else if (mousepressstate == Mouse_Held)
        return 1;

    return 0;
}

int32_t *joyaxis = NULL, joyb=0, *joyhat = NULL;
char joyisgamepad=0, joynumaxes=0, joynumbuttons=0, joynumhats=0;
int32_t joyaxespresent=0;

void(*keypresscallback)(int32_t,int32_t) = 0;
void(*mousepresscallback)(int32_t,int32_t) = 0;
void(*joypresscallback)(int32_t,int32_t) = 0;

extern int16_t brightness;

//
// set{key|mouse|joy}presscallback() -- sets a callback which gets notified when keys are pressed
//
void setkeypresscallback(void (*callback)(int32_t, int32_t)) { keypresscallback = callback; }
void setmousepresscallback(void (*callback)(int32_t, int32_t)) { mousepresscallback = callback; }
void setjoypresscallback(void (*callback)(int32_t, int32_t)) { joypresscallback = callback; }

char scantoasc[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,  0,   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', 0,   0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 0,   92,  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/', 0,   '*', 0,   32,  0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1', '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,
};

int32_t defaultres[][2]
= { { 2560, 1440 }, { 2560, 1200 }, { 2560, 1080 }, { 1920, 1440 }, { 1920, 1200 }, { 1920, 1080 }, { 1680, 1050 }, { 1600, 1200 },
    { 1600, 900 },  { 1366, 768 },  { 1280, 1024 }, { 1280, 960 },  { 1152, 864 },  { 1024, 768 },  { 1024, 600 },  { 800, 600 },
    { 640, 480 },   { 640, 400 },   { 512, 384 },   { 480, 360 },   { 400, 300 },   { 320, 240 },   { 320, 200 },   { 0, 0 } };


int32_t GetKey(int32_t key)
{
    return keystatus[keyremap[key]];
}

void SetKey(int32_t key, int32_t state)
{
    keystatus[keyremap[key]] = state;

    if (state)
    {
        keyfifo[keyfifoend] = keyremap[key];
        keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = state;
        keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1));
    }
}

//
// bgetchar, bflushchars -- character-based input functions
//
char bgetchar(void)
{
    if (keyasciififoplc == keyasciififoend)
        return 0;

    {
        char c = keyasciififo[keyasciififoplc];
        keyasciififoplc = ((keyasciififoplc+1)&(KEYFIFOSIZ-1));
        return c;
    }
}

void bflushchars(void)
{
    Bmemset(&keyasciififo,0,sizeof(keyasciififo));
    keyasciififoplc = keyasciififoend = 0;
}

const char *getkeyname(int32_t num)
{
    return ((unsigned)num >= 256) ? NULL : key_names[num];
}

void readmousexy(int32_t *x, int32_t *y)
{
    if (!moustat || !mousegrab || !appactive) { *x = *y = 0; return; }
    *x = mousex;
    *y = mousey;
    mousex = mousey = 0;
}

int32_t readmouseabsxy(vec2_t * const destination, vec2_t const * const source)
{
    int32_t xwidth;

    if (!moustat || !appactive || !mouseinwindow || (osd && osd->flags & OSD_CAPTURE))
        return 0;

    xwidth = max(scale(240<<16, xdim, ydim), 320<<16);

    destination->x = scale(source->x, xwidth, xdim) - ((xwidth>>1) - (320<<15));
    destination->y = scale(source->y, 200<<16, ydim);

    return 1;
}

void readmousebstatus(int32_t *b)
{
    if (!moustat || !appactive || !mouseinwindow || (osd && osd->flags & OSD_CAPTURE)) { *b = 0; return; }
    *b = mouseb;
}

void readjoybstatus(int32_t *b)
{
    if (!appactive) { *b = 0; return; }
    *b = joyb;
}

#if defined __linux || defined EDUKE32_BSD || defined __APPLE__
# include <sys/mman.h>
#endif

#if !defined(NOASM) && !defined(GEKKO) && !defined(__ANDROID__)
#ifdef __cplusplus
extern "C" {
#endif
    extern intptr_t dep_begin, dep_end;
#ifdef __cplusplus
}
#endif
#endif

#if !defined(NOASM) && !defined(GEKKO) && !defined(__ANDROID__)
static int32_t nx_unprotect(intptr_t beg, intptr_t end, int prot)
{
# if defined _WIN32
#  define B_PROT_RW PAGE_READWRITE
#  define B_PROT_RX PAGE_EXECUTE_READ
#  define B_PROT_RWX PAGE_EXECUTE_READWRITE

    DWORD oldprot;

    if (!VirtualProtect((LPVOID) beg, (SIZE_T)end - (SIZE_T)beg, prot, &oldprot))
    {
        initprintf("VirtualProtect() error! Crashing in 3... 2... 1...\n");
        return 1;
    }
# elif defined __linux || defined EDUKE32_BSD || defined __APPLE__
#  define B_PROT_RW (PROT_READ|PROT_WRITE)
#  define B_PROT_RX (PROT_READ|PROT_EXEC)
#  define B_PROT_RWX (PROT_READ|PROT_WRITE|PROT_EXEC)

    int32_t pagesize;
    size_t dep_begin_page;
    pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1)
    {
        initprintf("Error getting system page size\n");
        return 1;
    }
    dep_begin_page = ((size_t)beg) & ~(pagesize-1);
    if (mprotect((void *) dep_begin_page, (size_t)end - dep_begin_page, prot) < 0)
    {
        initprintf("Error making code writeable (errno=%d)\n", errno);
        return 1;
    }
# else
#  error "Don't know how to unprotect the self-modifying assembly on this platform!"
# endif

    return 0;
}
#endif


// Calculate ylookup[] and call setvlinebpl()
void calc_ylookup(int32_t bpl, int32_t lastyidx)
{
    int32_t i, j=0;
    static int32_t ylookupsiz;

    Bassert(lastyidx <= MAXYDIM);

    lastyidx++;

    if (lastyidx > ylookupsiz)
    {
        Baligned_free(ylookup);

        ylookup = (intptr_t *)Xaligned_alloc(16, lastyidx * sizeof(intptr_t));
        ylookupsiz = lastyidx;
    }

    for (i=0; i<=lastyidx-4; i+=4)
    {
        ylookup[i] = j;
        ylookup[i + 1] = j + bpl;
        ylookup[i + 2] = j + (bpl << 1);
        ylookup[i + 3] = j + (bpl * 3);
        j += (bpl << 2);
    }

    for (; i<lastyidx; i++)
    {
        ylookup[i] = j;
        j += bpl;
    }

    setvlinebpl(bpl);
}


void makeasmwriteable(void)
{
#if !defined(NOASM) && !defined(GEKKO) && !defined(__ANDROID__)
    nx_unprotect((intptr_t)&dep_begin, (intptr_t)&dep_end, B_PROT_RWX);
#endif
}

int32_t vsync=0;

#ifdef USE_OPENGL
extern int32_t nofog;

void fullscreen_tint_gl(uint8_t r, uint8_t g, uint8_t b, uint8_t f)
{
    bglMatrixMode(GL_PROJECTION);
    bglPushMatrix();
    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();

    bglDisable(GL_DEPTH_TEST);
    bglDisable(GL_ALPHA_TEST);
    bglDisable(GL_TEXTURE_2D);
    bglDisable(GL_FOG);

    bglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    bglEnable(GL_BLEND);
    bglColor4ub(r, g, b, f);

    bglBegin(GL_TRIANGLES);
    bglVertex2f(-2.5f, 1.f);
    bglVertex2f(2.5f, 1.f);
    bglVertex2f(.0f, -2.5f);
    bglEnd();

    bglPopMatrix();
    bglMatrixMode(GL_PROJECTION);
    bglPopMatrix();
}

struct glinfo_t glinfo =
{
    "Unknown",  // vendor
    "Unknown",  // renderer
    "0.0.0",    // version
    "",         // extensions

    1.0,        // max anisotropy
    0,          // brga texture format
    0,          // clamp-to-edge support
    0,          // texture compression
    0,          // non-power-of-two textures
    0,          // multisampling
    0,          // nvidia multisampling hint
    0,          // ARBfp
    0,          // depth textures
    0,          // shadow comparison
    0,          // Frame Buffer Objects
    0,          // rectangle textures
    0,          // multitexturing
    0,          // env_combine
    0,          // Vertex Buffer Objects
    0,          // VSync support
    0,          // Shader Model 4 support
    0,          // Occlusion Queries
    0,          // GLSL
    0,          // Debug Output
    0,          // Buffer storage
    0,          // GL info dumped
};
#endif

int32_t flushlogwindow = 1;

#ifdef USE_OPENGL
// Used to register the game's / editor's osdcmd_vidmode() functions here.
int32_t (*baselayer_osdcmd_vidmode_func)(osdfuncparm_t const * const parm);

static int32_t osdfunc_setrendermode(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    char *p;
    int32_t m = Bstrtol(parm->parms[0], &p, 10);

    if (m != REND_CLASSIC && m != REND_POLYMOST && m != REND_POLYMER)
        return OSDCMD_SHOWHELP;

    if ((m==REND_CLASSIC) != (bpp==8) && baselayer_osdcmd_vidmode_func)
    {
        // Mismatch between video mode and requested renderer, do auto switch.
        osdfuncparm_t parm;
        char arg[4];

        const char *ptrptr[1];
        ptrptr[0] = arg;

        Bmemset(&parm, 0, sizeof(parm));

        if (m==REND_CLASSIC)
            Bmemcpy(&arg, "8", 2);
        else
            Bmemcpy(&arg, "32", 3);

        // CAUTION: we assume that the osdcmd_vidmode function doesn't use any
        // other member!
        parm.numparms = 1;
        parm.parms = ptrptr;

        baselayer_osdcmd_vidmode_func(&parm);
    }

    setrendermode(m);

    char const *renderer;

    switch (getrendermode())
    {
    case REND_CLASSIC:
        renderer = "classic software";
        break;
    case REND_POLYMOST:
        renderer = "polygonal OpenGL";
        break;
#ifdef POLYMER
    case REND_POLYMER:
        renderer = "great justice (Polymer)";
        break;
#endif
    default:
        EDUKE32_UNREACHABLE_SECTION(break);
    }

    OSD_Printf("Rendering method changed to %s\n", renderer);

    return OSDCMD_OK;
}
#if defined(USE_OPENGL)
#ifdef DEBUGGINGAIDS
static int32_t osdcmd_hicsetpalettetint(osdfuncparm_t const * const parm)
{
    int32_t parms[8];

    if (parm->numparms < 1 || (int32_t)ARRAY_SIZE(parms) < parm->numparms) return OSDCMD_SHOWHELP;

    size_t i;
    for (i = 0; (int32_t)i < parm->numparms; ++i)
        parms[i] = Batol(parm->parms[i]);
    for (; i < ARRAY_SIZE(parms); ++i)
        parms[i] = 0;

    // order is intentional
    hicsetpalettetint(parms[0],parms[1],parms[2],parms[3],parms[5],parms[6],parms[7],parms[4]);

    return OSDCMD_OK;
}
#endif

int32_t osdcmd_glinfo(osdfuncparm_t const * const UNUSED(parm))
{
    char *s,*t,*u,i;

    UNREFERENCED_CONST_PARAMETER(parm);

    if (bpp == 8)
    {
        initprintf("glinfo: not in OpenGL mode!\n");
        return OSDCMD_OK;
    }

    initprintf("OpenGL information\n %s %s %s\n",
               glinfo.vendor, glinfo.renderer, glinfo.version);

    if (!glinfo.dumped)
        return OSDCMD_OK;

    initprintf(
               " BGRA textures:           %s\n"
               " Non-power-of-2 textures: %s\n"
               " Clamp-to-edge:           %s\n"
               " Multitexturing:          %s\n"
               " Frame Buffer Objects:    %s\n"
#ifndef EDUKE32_GLES
               " Texture compression:     %s\n"
               " Multisampling:           %s\n"
               " NVIDIA multisample hint: %s\n"
               " ARBfp fragment programs: %s\n"
               " Depth textures:          %s\n"
               " Shadow textures:         %s\n"
               " Rectangle textures:      %s\n"
               " env_combine:             %s\n"
               " Vertex Buffer Objects:   %s\n"
               " Shader Model 4:          %s\n"
               " Occlusion queries:       %s\n"
               " GLSL:                    %s\n"
               " Debug Output:            %s\n"
               " Buffer Storage:          %s\n"
#endif
               " Maximum anisotropy:      %.1f%s\n"
               " Extensions:\n",
               glinfo.bgra ? "supported": "not supported",
               glinfo.texnpot ? "supported": "not supported",
               glinfo.clamptoedge ? "supported": "not supported",
               glinfo.multitex ? "supported": "not supported",
               glinfo.fbos ? "supported": "not supported",
#ifndef EDUKE32_GLES
               glinfo.texcompr ? "supported": "not supported",
               glinfo.multisample ? "supported": "not supported",
               glinfo.nvmultisamplehint ? "supported": "not supported",
               glinfo.arbfp ? "supported": "not supported",
               glinfo.depthtex ? "supported": "not supported",
               glinfo.shadow ? "supported": "not supported",
               glinfo.rect ? "supported": "not supported",
               glinfo.envcombine ? "supported": "not supported",
               glinfo.vbos ? "supported": "not supported",
               glinfo.sm4 ? "supported": "not supported",
               glinfo.occlusionqueries ? "supported": "not supported",
               glinfo.glsl ? "supported": "not supported",
               glinfo.debugoutput ? "supported": "not supported",
               glinfo.bufferstorage ? "supported" : "not supported",
#endif
               glinfo.maxanisotropy, glinfo.maxanisotropy>1.0?"":" (no anisotropic filtering)"
              );

    s = Bstrdup(glinfo.extensions);
    if (!s) initprintf("%s", glinfo.extensions);
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
#endif

static int32_t osdcmd_cvar_set_baselayer(osdfuncparm_t const * const parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "vid_gamma") || !Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
    {
        setbrightness(GAMMA_CALC,0,0);

        return r;
    }

    return r;
}

int32_t baselayer_init(void)
{
    uint32_t i;
#ifdef _WIN32
// on Windows, don't save the "r_screenaspect" cvar because the physical screen size is
// determined at startup
# define SCREENASPECT_CVAR_TYPE (CVAR_UINT|CVAR_NOSAVE)
#else
# define SCREENASPECT_CVAR_TYPE (CVAR_UINT)
#endif
    static osdcvardata_t cvars_engine[] =
    {
        { "r_usenewaspect","enable/disable new screen aspect ratio determination code",(void *) &r_usenewaspect, CVAR_BOOL, 0, 1 },
        { "r_screenaspect","if using r_usenewaspect and in fullscreen, screen aspect ratio in the form XXYY, e.g. 1609 for 16:9",
          (void *) &r_screenxy, SCREENASPECT_CVAR_TYPE, 0, 9999 },
        { "r_novoxmips","turn off/on the use of mipmaps when rendering 8-bit voxels",(void *) &novoxmips, CVAR_BOOL, 0, 1 },
        { "r_voxels","enable/disable automatic sprite->voxel rendering",(void *) &usevoxels, CVAR_BOOL, 0, 1 },
#ifdef YAX_ENABLE
        { "r_tror_nomaskpass", "enable/disable additional pass in TROR software rendering", (void *)&r_tror_nomaskpass, CVAR_BOOL, 0, 1 },
#endif
        { "r_windowpositioning", "enable/disable window position memory", (void *) &windowpos, CVAR_BOOL, 0, 1 },
        { "vid_gamma","adjusts gamma component of gamma ramp",(void *) &vid_gamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *) &vid_contrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *) &vid_brightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
#ifdef DEBUGGINGAIDS
        { "debug1","debug counter",(void *) &debug1, CVAR_FLOAT, -100000, 100000 },
        { "debug2","debug counter",(void *) &debug2, CVAR_FLOAT, -100000, 100000 },
#endif
#ifdef DEBUG_MASK_DRAWING
        { "debug_maskdrawmode", "Show mask draw orders", (void *)&g_maskDrawMode, CVAR_BOOL, 0, 1 },
#endif
    };

    for (i=0; i<ARRAY_SIZE(cvars_engine); i++)
        OSD_RegisterCvar(&cvars_engine[i], (cvars_engine[i].flags & CVAR_FUNCPTR) ? osdcmd_cvar_set_baselayer : osdcmd_cvar_set);

#ifdef USE_OPENGL
    OSD_RegisterFunction("setrendermode","setrendermode <number>: sets the engine's rendering mode.\n"
                         "Mode numbers are:\n"
                         "   0 - Classic Build software\n"
                         "   3 - Polygonal OpenGL\n"
#ifdef POLYMER
                         "   4 - Great justice renderer (Polymer)\n"
#endif
                         ,
                         osdfunc_setrendermode);

# ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("hicsetpalettetint","hicsetpalettetint: sets palette tinting values",osdcmd_hicsetpalettetint);
# endif

    OSD_RegisterFunction("glinfo","glinfo: shows OpenGL information about the current OpenGL mode",osdcmd_glinfo);

    polymost_initosdfuncs();
#endif

    return 0;
}

void maybe_redirect_outputs(void)
{
#if !(defined __APPLE__ && defined __BIG_ENDIAN__)
    char *argp;

    // pipe standard outputs to files
    if ((argp = Bgetenv("BUILD_LOGSTDOUT")) != NULL)
        if (!Bstrcasecmp(argp, "TRUE"))
        {
            FILE *fp = freopen("stdout.txt", "w", stdout);

            if (!fp)
                fp = fopen("stdout.txt", "w");

            if (fp)
            {
                setvbuf(fp, 0, _IONBF, 0);
                *stdout = *fp;
                *stderr = *fp;
            }
        }
#endif
}
