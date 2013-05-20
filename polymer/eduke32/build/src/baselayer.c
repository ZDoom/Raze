#include "compat.h"
#include "osd.h"
#include "build.h"
#include "baselayer.h"

#include "renderlayer.h"

#include "a.h"
#include "polymost.h"

// input
char inputdevices=0;
char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
char keyasciififo[KEYFIFOSIZ], keyasciififoplc, keyasciififoend;
char remap[256];
int32_t remapinit=0;
char key_names[256][24];
volatile int32_t mousex=0,mousey=0,mouseb=0,mouseabsx=0,mouseabsy=0;
volatile uint8_t moustat = 0, mousegrab = 0;
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

int32_t defaultres[][2] =
{
    {1920, 1440}, {1920, 1200}, {1920, 1080}, {1680, 1050}, {1600, 1200}, {1600, 900}, {1366, 768}, {1280, 1024}, 
    {1280, 960}, {1152, 864}, {1024, 768}, {1024, 600}, {800, 600}, {640, 480}, {640, 400}, 
    {512, 384}, {480, 360}, {400, 300}, {320, 240}, {320, 200}, {0, 0}
};


void SetKey(int32_t key, int32_t state)
{
    keystatus[remap[key]] = state;

    if (state)
    {
        keyfifo[keyfifoend] = remap[key];
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

void readmouseabsxy(int32_t *x, int32_t *y)
{
    if (!moustat || !mousegrab || !appactive)
    {
        // no mouse, centre it
        *x = xdim >> 1;
        *y = ydim >> 1;
    }
    else
    {
        *x = mouseabsx;
        *y = mouseabsy;
    }
}

void readmousebstatus(int32_t *b)
{
    if (!moustat || !mousegrab || !appactive) { *b = 0; return; }
    *b = mouseb;
}

void readjoybstatus(int32_t *b)
{
    if (!appactive) { *b = 0; return; }
    *b = joyb;
}

// Calculate ylookup[] and call setvlinebpl()
void calc_ylookup(int32_t bpl, int32_t lastyidx)
{
    int32_t i, j=0;

    Bassert(lastyidx <= MAXYDIM);

    for (i=0; i<=lastyidx; i++)
    {
        ylookup[i] = j;
        j += bpl;
    }

    setvlinebpl(bpl);
}

#ifdef USE_OPENGL
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

    bglEnable(GL_BLEND);
    bglColor4ub(r, g, b, f);

    bglBegin(GL_TRIANGLES);
    bglVertex2f(-2.5f, 1.f);
    bglVertex2f(2.5f, 1.f);
    bglVertex2f(.0f, -2.5f);
    bglEnd();

    bglDisable(GL_BLEND);

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
    0,          // GL info dumped
};
#endif

int32_t flushlogwindow = 1;

static void onvideomodechange(int32_t newmode) { UNREFERENCED_PARAMETER(newmode); }
void (*baselayer_onvideomodechange)(int32_t) = onvideomodechange;

#ifdef USE_OPENGL
static int32_t osdfunc_setrendermode(const osdfuncparm_t *parm)
{
    int32_t m;
    char *p;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    m = Bstrtol(parm->parms[0], &p, 10);

    if (m < REND_CLASSIC || m > REND_POLYMER) return OSDCMD_SHOWHELP;

    setrendermode(m);

    switch(getrendermode())
    {
    case REND_CLASSIC:
        p = "classic software";
        break;
    case REND_POLYMOST:
        p = "polygonal OpenGL";
        break;
    case REND_POLYMER:
        p = "great justice (Polymer)";
        break;
    }

    OSD_Printf("Rendering method changed to %s\n", p);

    return OSDCMD_OK;
}
#if defined(USE_OPENGL)
#ifdef DEBUGGINGAIDS
static int32_t osdcmd_hicsetpalettetint(const osdfuncparm_t *parm)
{
    int32_t pal, cols[3], eff;

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

int32_t osdcmd_glinfo(const osdfuncparm_t *parm)
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
               " Renderer: %s\n",
               glinfo.version,
               glinfo.vendor,
               glinfo.renderer);

    if (!glinfo.dumped)
        return OSDCMD_OK;

    initprintf(" Maximum anisotropy:      %.1f%s\n"
               " BGRA textures:           %s\n"
               " Non-power-of-2 textures: %s\n"
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
               " Shader Model 4:          %s\n"
               " Occlusion queries:       %s\n"
               " GLSL:                    %s\n"
               " Debug Output:            %s\n"
               " Extensions:\n",
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
               glinfo.vbos ? "supported": "not supported",
               glinfo.sm4 ? "supported": "not supported",
               glinfo.occlusionqueries ? "supported": "not supported",
               glinfo.glsl ? "supported": "not supported",
               glinfo.debugoutput ? "supported": "not supported"
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

static int32_t osdcmd_cvar_set_baselayer(const osdfuncparm_t *parm)
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
    cvar_t cvars_engine[] =
    {
        { "r_usenewaspect","enable/disable new screen aspect ratio determination code",(void *) &r_usenewaspect, CVAR_BOOL, 0, 1 },
        { "r_screenaspect","if using r_usenewaspect and in fullscreen, screen aspect ratio in the form XXYY, e.g. 1609 for 16:9",
          (void *) &r_screenxy, SCREENASPECT_CVAR_TYPE, 0, 9999 },
        { "r_novoxmips","turn off/on the use of mipmaps when rendering 8-bit voxels",(void *) &novoxmips, CVAR_BOOL, 0, 1 },
        { "r_voxels","enable/disable automatic sprite->voxel rendering",(void *) &usevoxels, CVAR_BOOL, 0, 1 },
#ifdef YAX_ENABLE
        { "r_tror_nomaskpass", "enable/disable additional pass in TROR software rendering", (void *)&r_tror_nomaskpass, CVAR_BOOL, 0, 1 },
#endif
        { "vid_gamma","adjusts gamma component of gamma ramp",(void *) &vid_gamma, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","adjusts contrast component of gamma ramp",(void *) &vid_contrast, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","adjusts brightness component of gamma ramp",(void *) &vid_brightness, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
#ifdef DEBUGGINGAIDS
        { "debug1","debug counter",(void *) &debug1, CVAR_FLOAT, -100000, 100000 },
        { "debug2","debug counter",(void *) &debug2, CVAR_FLOAT, -100000, 100000 },
#endif
    };

    for (i=0; i<sizeof(cvars_engine)/sizeof(cvars_engine[0]); i++)
    {
        if (OSD_RegisterCvar(&cvars_engine[i]))
            continue;

        OSD_RegisterFunction(cvars_engine[i].name, cvars_engine[i].desc,
                             (cvars_engine[i].type & CVAR_FUNCPTR) ? osdcmd_cvar_set_baselayer : osdcmd_cvar_set);
    }

#ifdef USE_OPENGL
    OSD_RegisterFunction("setrendermode","setrendermode <number>: sets the engine's rendering mode.\n"
                         "Mode numbers are:\n"
                         "   0 - Classic Build software\n"
#ifdef USE_OPENGL
                         "   3 - Polygonal OpenGL\n"
                         "   4 - Great justice renderer (Polymer)\n"
#endif
                         ,
                         osdfunc_setrendermode);
#ifdef USE_OPENGL
# ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("hicsetpalettetint","hicsetpalettetint: sets palette tinting values",osdcmd_hicsetpalettetint);
# endif
    OSD_RegisterFunction("glinfo","glinfo: shows OpenGL information about the current OpenGL mode",osdcmd_glinfo);
#endif
    polymost_initosdfuncs();
#endif

    return 0;
}


#if defined _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#elif defined __linux || defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__ || defined __APPLE__
# include <sys/mman.h>
#endif

#if !defined(NOASM) && !defined(GEKKO) && !defined(__ANDROID__)
#ifdef __cplusplus
extern "C" {
#endif
extern int32_t dep_begin, dep_end;
#ifdef __cplusplus
};
#endif
#endif

void makeasmwriteable(void)
{
#if !defined(NOASM) && !defined(GEKKO) && !defined(__ANDROID__)
//    extern int32_t dep_begin, dep_end;
# if defined _WIN32
    DWORD oldprot;
    if (!VirtualProtect((LPVOID)&dep_begin, (SIZE_T)&dep_end - (SIZE_T)&dep_begin, PAGE_EXECUTE_READWRITE, &oldprot))
    {
        initprintf("Error making code writeable\n");
        return;
    }
# elif defined __linux || defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__ || defined __APPLE__
    int32_t pagesize;
    size_t dep_begin_page;
    pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1)
    {
        initprintf("Error getting system page size\n");
        return;
    }
    dep_begin_page = ((size_t)&dep_begin) & ~(pagesize-1);
    if (mprotect((void *)dep_begin_page, (size_t)&dep_end - dep_begin_page, PROT_READ|PROT_WRITE) < 0)
    {
        initprintf("Error making code writeable (errno=%d)\n", errno);
        return;
    }
# else
#  error "Don't know how to unprotect the self-modifying assembly on this platform!"
# endif
#endif
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
