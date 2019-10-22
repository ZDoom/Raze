#include "compat.h"
#include "osd.h"
#include "build.h"
#include "baselayer.h"

#include "renderlayer.h"

#include "a.h"
#include "polymost.h"
#include "cache1d.h"
#include "../../glbackend/glbackend.h"

// video
#ifdef _WIN32
extern "C"
{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
    __declspec(dllexport) DWORD NvOptimusEnablement                = 0x00000001;
}
#endif // _WIN32

int32_t g_borderless=2;

// input
char    inputdevices = 0;

char    keystatus[NUMKEYS];
char    g_keyFIFO[KEYFIFOSIZ];
char    g_keyAsciiFIFO[KEYFIFOSIZ];
uint8_t g_keyFIFOpos;
uint8_t g_keyFIFOend;
uint8_t g_keyAsciiPos;
uint8_t g_keyAsciiEnd;
char    g_keyRemapTable[NUMKEYS];
char    g_keyNameTable[NUMKEYS][24];

void (*keypresscallback)(int32_t, int32_t);

void keySetCallback(void (*callback)(int32_t, int32_t)) { keypresscallback = callback; }

int32_t keyGetState(int32_t key) { return keystatus[g_keyRemapTable[key]]; }

void keySetState(int32_t key, int32_t state)
{
    keystatus[g_keyRemapTable[key]] = state;

    if (state)
    {
        g_keyFIFO[g_keyFIFOend] = g_keyRemapTable[key];
        g_keyFIFO[(g_keyFIFOend+1)&(KEYFIFOSIZ-1)] = state;
        g_keyFIFOend = ((g_keyFIFOend+2)&(KEYFIFOSIZ-1));
    }
}

char keyGetScan(void)
{
    if (g_keyFIFOpos == g_keyFIFOend)
        return 0;

    char const c    = g_keyFIFO[g_keyFIFOpos];
    g_keyFIFOpos = ((g_keyFIFOpos + 2) & (KEYFIFOSIZ - 1));

    return c;
}

void keyFlushScans(void)
{
    Bmemset(&g_keyFIFO,0,sizeof(g_keyFIFO));
    g_keyFIFOpos = g_keyFIFOend = 0;
}

//
// character-based input functions
//
char keyGetChar(void)
{
    if (g_keyAsciiPos == g_keyAsciiEnd)
        return 0;

    char const c    = g_keyAsciiFIFO[g_keyAsciiPos];
    g_keyAsciiPos = ((g_keyAsciiPos + 1) & (KEYFIFOSIZ - 1));

    return c;
}

void keyFlushChars(void)
{
    Bmemset(&g_keyAsciiFIFO,0,sizeof(g_keyAsciiFIFO));
    g_keyAsciiPos = g_keyAsciiEnd = 0;
}

const char *keyGetName(int32_t num) { return ((unsigned)num >= NUMKEYS) ? NULL : g_keyNameTable[num]; }

vec2_t  g_mousePos;
vec2_t  g_mouseAbs;
int32_t g_mouseBits;
uint8_t g_mouseClickState;

bool g_mouseEnabled;
bool g_mouseGrabbed;
bool g_mouseInsideWindow   = 1;
bool g_mouseLockedToWindow = 1;

void (*g_mouseCallback)(int32_t, int32_t);
void mouseSetCallback(void(*callback)(int32_t, int32_t)) { g_mouseCallback = callback; }

int32_t mouseAdvanceClickState(void)
{
    switch (g_mouseClickState)
    {
        case MOUSE_PRESSED: g_mouseClickState  = MOUSE_HELD; return 1;
        case MOUSE_RELEASED: g_mouseClickState = MOUSE_IDLE; return 1;
        case MOUSE_HELD: return 1;
    }
    return 0;
}

void mouseReadPos(int32_t *x, int32_t *y)
{
    if (!g_mouseEnabled || !g_mouseGrabbed || !appactive)
    {
        *x = *y = 0;
        return;
    }

    *x = g_mousePos.x;
    *y = g_mousePos.y;
    g_mousePos.x = g_mousePos.y = 0;
}

int32_t mouseReadAbs(vec2_t * const pResult, vec2_t const * const pInput)
{
    if (!g_mouseEnabled || !appactive || !g_mouseInsideWindow || (osd && osd->flags & OSD_CAPTURE))
        return 0;

    int32_t const xwidth = max(scale(240<<16, xdim, ydim), 320<<16);

    pResult->x = scale(pInput->x, xwidth, xres) - ((xwidth>>1) - (320<<15));
    pResult->y = scale(pInput->y, 200<<16, yres);

    pResult->y = divscale16(pResult->y - (200<<15), rotatesprite_yxaspect) + (200<<15) - rotatesprite_y_offset;

    return 1;
}

int32_t mouseReadButtons(void)
{
    return (!g_mouseEnabled || !appactive || !g_mouseInsideWindow || (osd && osd->flags & OSD_CAPTURE)) ? 0 : g_mouseBits;
}

controllerinput_t joystick;

void joySetCallback(void (*callback)(int32_t, int32_t)) { joystick.pCallback = callback; }
void joyReadButtons(int32_t *pResult) { *pResult = appactive ? joystick.bits : 0; }

#if defined __linux || defined EDUKE32_BSD || defined __APPLE__
# include <sys/mman.h>
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
        Xaligned_free(ylookup);

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
}

int32_t vsync=0;
int32_t g_logFlushWindow = 1;

#ifdef USE_OPENGL
struct glinfo_t glinfo =
{
    "Unknown",  // vendor
    "Unknown",  // renderer
    "0.0.0",    // version
    "",         // extensions

    1.0,        // max anisotropy
};

// Used to register the game's / editor's osdcmd_vidmode() functions here.
int32_t (*baselayer_osdcmd_vidmode_func)(osdcmdptr_t parm);

static int osdfunc_setrendermode(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    int32_t m = Bstrtol(parm->parms[0], NULL, 10);

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

    videoSetRenderMode(m);

    char const *renderer = "other";

    switch (videoGetRenderMode())
    {
    case REND_CLASSIC:
#ifdef NOASM
        renderer = "classic software (C)";
#else
        renderer = "classic software (ASM)";
#endif
        break;
    case REND_POLYMOST:
        renderer = "polygonal OpenGL";
        break;
#ifdef POLYMER
    case REND_POLYMER:
        renderer = "great justice (Polymer)";
        break;
#endif
    }

    OSD_Printf("Rendering method changed to %s\n", renderer);

    return OSDCMD_OK;
}

#ifdef DEBUGGINGAIDS
static int osdcmd_hicsetpalettetint(osdcmdptr_t parm)
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

int osdcmd_glinfo(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    initprintf("OpenGL information\n %s %s %s\n",
               GLInterface.glinfo.vendor, GLInterface.glinfo.renderer, GLInterface.glinfo.version);

    return OSDCMD_OK;
}
#endif

static int osdcmd_cvar_set_baselayer(osdcmdptr_t parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "vid_gamma") || !Bstrcasecmp(parm->name, "vid_brightness") || !Bstrcasecmp(parm->name, "vid_contrast"))
    {
        videoSetPalette(GAMMA_CALC,0,0);

        return r;
    }

    return r;
}

int32_t baselayer_init(void)
{
#ifdef _WIN32
// on Windows, don't save the "r_screenaspect" cvar because the physical screen size is
// determined at startup
# define SCREENASPECT_CVAR_TYPE (CVAR_UINT|CVAR_NOSAVE)
#else
# define SCREENASPECT_CVAR_TYPE (CVAR_UINT)
#endif
    static osdcvardata_t cvars_engine[] =
    {
        { "lz4compressionlevel","adjust LZ4 compression level used for savegames",(void *) &lz4CompressionLevel, CVAR_INT, 1, 32 },
        { "r_borderless", "borderless windowed mode: 0: never  1: always  2: if resolution matches desktop", (void *) &r_borderless, CVAR_INT|CVAR_RESTARTVID, 0, 2 },
        { "r_displayindex","index of output display",(void *)&r_displayindex, CVAR_INT|CVAR_RESTARTVID, 0, 10 },
        { "r_usenewaspect","enable/disable new screen aspect ratio determination code",(void *) &r_usenewaspect, CVAR_BOOL, 0, 1 },
        { "r_screenaspect","if using r_usenewaspect and in fullscreen, screen aspect ratio in the form XXYY, e.g. 1609 for 16:9",
          (void *) &r_screenxy, SCREENASPECT_CVAR_TYPE, 0, 9999 },
        { "r_fpgrouscan","use floating-point numbers for slope rendering",(void *) &r_fpgrouscan, CVAR_BOOL, 0, 1 },
        { "r_novoxmips","turn off/on the use of mipmaps when rendering 8-bit voxels",(void *) &novoxmips, CVAR_BOOL, 0, 1 },
        { "r_voxels","enable/disable automatic sprite->voxel rendering",(void *) &usevoxels, CVAR_BOOL, 0, 1 },
#ifdef YAX_ENABLE
        { "r_tror_nomaskpass", "enable/disable additional pass in TROR software rendering", (void *)&r_tror_nomaskpass, CVAR_BOOL, 0, 1 },
#endif
        { "r_windowpositioning", "enable/disable window position memory", (void *) &windowpos, CVAR_BOOL, 0, 1 },
        { "vid_gamma","adjusts gamma component of gamma ramp",(void *) &g_videoGamma, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *) &g_videoContrast, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *) &g_videoBrightness, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
    };

    for (auto & i : cvars_engine)
        OSD_RegisterCvar(&i, (i.flags & CVAR_FUNCPTR) ? osdcmd_cvar_set_baselayer : osdcmd_cvar_set);

#ifdef USE_OPENGL
    OSD_RegisterFunction("setrendermode","setrendermode <number>: sets the engine's rendering mode.\n"
                         "Mode numbers are:\n"
                         "   0 - Classic Build software\n"
                         "   3 - Polygonal OpenGL\n"
                         ,
                         osdfunc_setrendermode);

# ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("hicsetpalettetint","hicsetpalettetint: sets palette tinting values",osdcmd_hicsetpalettetint);
# endif

    OSD_RegisterFunction("glinfo","glinfo: shows OpenGL information about the current OpenGL mode",osdcmd_glinfo);

    polymost_initosdfuncs();
#endif

    for (native_t i = 0; i < NUMKEYS; i++) g_keyRemapTable[i] = i;

    return 0;
}

