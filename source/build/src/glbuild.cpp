#include "compat.h"
#include "glad/glad.h"
#include "glbuild.h"
#include "baselayer.h"

#if defined USE_OPENGL

#ifdef RENDERTYPESDL
# include "sdlayer.h"
#endif

GLenum BuildGLError;
void BuildGLErrorCheck(void)
{
    volatile GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        BuildGLError = err; // set a watchpoint/breakpoint here
    }
}

#if defined DYNAMIC_GL

#if !defined RENDERTYPESDL && defined _WIN32
bwglCreateContextProcPtr bwglCreateContext;
bwglDeleteContextProcPtr bwglDeleteContext;
bwglGetProcAddressProcPtr bwglGetProcAddress;
bwglMakeCurrentProcPtr bwglMakeCurrent;

bwglChoosePixelFormatProcPtr bwglChoosePixelFormat;
bwglDescribePixelFormatProcPtr bwglDescribePixelFormat;
bwglGetPixelFormatProcPtr bwglGetPixelFormat;
bwglSetPixelFormatProcPtr bwglSetPixelFormat;
#endif

#endif

#if defined DYNAMIC_GLU

// GLU
bgluTessBeginContourProcPtr bgluTessBeginContour;
bgluTessBeginPolygonProcPtr bgluTessBeginPolygon;
bgluTessCallbackProcPtr bgluTessCallback;
bgluTessEndContourProcPtr bgluTessEndContour;
bgluTessEndPolygonProcPtr bgluTessEndPolygon;
bgluTessNormalProcPtr bgluTessNormal;
bgluTessPropertyProcPtr bgluTessProperty;
bgluTessVertexProcPtr bgluTessVertex;
bgluNewTessProcPtr bgluNewTess;
bgluDeleteTessProcPtr bgluDeleteTess;

bgluPerspectiveProcPtr bgluPerspective;

bgluErrorStringProcPtr bgluErrorString;

bgluProjectProcPtr bgluProject;
bgluUnProjectProcPtr bgluUnProject;

#endif

#if defined DYNAMIC_GL || defined DYNAMIC_GLEXT || defined DYNAMIC_GLU
# if !defined _WIN32
#  include <dlfcn.h>
# endif
#endif

#if defined DYNAMIC_GL || defined DYNAMIC_GLEXT

#if !defined RENDERTYPESDL && defined _WIN32
static HMODULE hGLDLL;

char *gldriver = NULL;

static void *getproc_(const char *s, int32_t *err, int32_t fatal, int32_t extension)
{
    void *t;
#if defined RENDERTYPESDL
    UNREFERENCED_PARAMETER(extension);
    t = (void *)SDL_GL_GetProcAddress(s);
#elif defined _WIN32
    if (extension) t = (void *)bwglGetProcAddress(s);
    else t = (void *)GetProcAddress(hGLDLL,s);
#else
#error Need a dynamic loader for this platform...
#endif
    if (!t && fatal)
    {
        initprintf("Failed to find %s in %s\n", s, gldriver);
        *err = 1;
    }
    return t;
}
#define GETPROC(s)        getproc_(s,&err,1,0)

int32_t loadwgl(const char *driver)
{
    int32_t err=0;

    if (hGLDLL) return 0;

    if (!driver)
    {
        driver = "opengl32.dll";
    }

    hGLDLL = LoadLibrary(driver);
    if (!hGLDLL)
    {
        initprintf("Failed loading \"%s\"\n", driver);
        return -1;
    }

    gldriver = Bstrdup(driver);

    bwglCreateContext = (bwglCreateContextProcPtr) GETPROC("wglCreateContext");
    bwglDeleteContext = (bwglDeleteContextProcPtr) GETPROC("wglDeleteContext");
    bwglGetProcAddress = (bwglGetProcAddressProcPtr) GETPROC("wglGetProcAddress");
    bwglMakeCurrent = (bwglMakeCurrentProcPtr) GETPROC("wglMakeCurrent");

    bwglChoosePixelFormat = (bwglChoosePixelFormatProcPtr) GETPROC("wglChoosePixelFormat");
    bwglDescribePixelFormat = (bwglDescribePixelFormatProcPtr) GETPROC("wglDescribePixelFormat");
    bwglGetPixelFormat = (bwglGetPixelFormatProcPtr) GETPROC("wglGetPixelFormat");
    bwglSetPixelFormat = (bwglSetPixelFormatProcPtr) GETPROC("wglSetPixelFormat");

    if (err) unloadwgl();
    return err;
}
int32_t unloadwgl(void)
{
    if (!hGLDLL) return 0;

    DO_FREE_AND_NULL(gldriver);

    FreeLibrary(hGLDLL);
    hGLDLL = NULL;

    bwglCreateContext = (bwglCreateContextProcPtr) NULL;
    bwglDeleteContext = (bwglDeleteContextProcPtr) NULL;
    bwglGetProcAddress = (bwglGetProcAddressProcPtr) NULL;
    bwglMakeCurrent = (bwglMakeCurrentProcPtr) NULL;

    bwglChoosePixelFormat = (bwglChoosePixelFormatProcPtr) NULL;
    bwglDescribePixelFormat = (bwglDescribePixelFormatProcPtr) NULL;
    bwglGetPixelFormat = (bwglGetPixelFormatProcPtr) NULL;
    bwglSetPixelFormat = (bwglSetPixelFormatProcPtr) NULL;
    return 0;
}
#endif

#endif

#if defined DYNAMIC_GLU
#if defined _WIN32
static HMODULE hGLUDLL;
#else
static void *gluhandle = NULL;
#endif

char *glulibrary = NULL;

static void *glugetproc_(const char *s, int32_t *err, int32_t fatal)
{
    void *t;
#if defined _WIN32
    t = (void *)GetProcAddress(hGLUDLL,s);
#else
    t = (void *)dlsym(gluhandle,s);
#endif
    if (!t && fatal)
    {
        initprintf("Failed to find %s in %s\n", s, glulibrary);
        *err = 1;
    }
    return t;
}
#define GLUGETPROC(s)        glugetproc_(s,&err,1)
#define GLUGETPROCSOFT(s)    glugetproc_(s,&err,0)
#endif

int32_t loadglulibrary(const char *driver)
{
#if defined DYNAMIC_GLU
    int32_t err=0;

#if defined _WIN32
    if (hGLUDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "glu32.dll";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL"; // FIXME: like I know anything about Apple.  Hah.
#elif defined __OpenBSD__
        driver = "libGLU.so";
#else
        driver = "libGLU.so.1";
#endif
    }

#if defined _WIN32
    hGLUDLL = LoadLibrary(driver);
    if (!hGLUDLL)
#else
    gluhandle = dlopen(driver, RTLD_NOW|RTLD_GLOBAL);
    if (!gluhandle)
#endif
    {
        initprintf("Failed loading \"%s\"\n",driver);
        return -1;
    }

    glulibrary = Bstrdup(driver);

    bgluTessBeginContour = (bgluTessBeginContourProcPtr) GLUGETPROC("gluTessBeginContour");
    bgluTessBeginPolygon = (bgluTessBeginPolygonProcPtr) GLUGETPROC("gluTessBeginPolygon");
    bgluTessCallback = (bgluTessCallbackProcPtr) GLUGETPROC("gluTessCallback");
    bgluTessEndContour = (bgluTessEndContourProcPtr) GLUGETPROC("gluTessEndContour");
    bgluTessEndPolygon = (bgluTessEndPolygonProcPtr) GLUGETPROC("gluTessEndPolygon");
    bgluTessNormal = (bgluTessNormalProcPtr) GLUGETPROC("gluTessNormal");
    bgluTessProperty = (bgluTessPropertyProcPtr) GLUGETPROC("gluTessProperty");
    bgluTessVertex = (bgluTessVertexProcPtr) GLUGETPROC("gluTessVertex");
    bgluNewTess = (bgluNewTessProcPtr) GLUGETPROC("gluNewTess");
    bgluDeleteTess = (bgluDeleteTessProcPtr) GLUGETPROC("gluDeleteTess");

    bgluPerspective = (bgluPerspectiveProcPtr) GLUGETPROC("gluPerspective");

    bgluErrorString = (bgluErrorStringProcPtr) GLUGETPROC("gluErrorString");

    bgluProject = (bgluProjectProcPtr) GLUGETPROC("gluProject");
    bgluUnProject = (bgluUnProjectProcPtr) GLUGETPROC("gluUnProject");

    if (err) unloadglulibrary();
    return err;
#else
    UNREFERENCED_PARAMETER(driver);
    return 0;
#endif
}

int32_t unloadglulibrary(void)
{
#if defined DYNAMIC_GLU
#if defined _WIN32
    if (!hGLUDLL) return 0;
#endif

    DO_FREE_AND_NULL(glulibrary);

#if defined _WIN32
    FreeLibrary(hGLUDLL);
    hGLUDLL = NULL;
#else
    if (gluhandle) dlclose(gluhandle);
    gluhandle = NULL;
#endif

    bgluTessBeginContour = (bgluTessBeginContourProcPtr) NULL;
    bgluTessBeginPolygon = (bgluTessBeginPolygonProcPtr) NULL;
    bgluTessCallback = (bgluTessCallbackProcPtr) NULL;
    bgluTessEndContour = (bgluTessEndContourProcPtr) NULL;
    bgluTessEndPolygon = (bgluTessEndPolygonProcPtr) NULL;
    bgluTessNormal = (bgluTessNormalProcPtr) NULL;
    bgluTessProperty = (bgluTessPropertyProcPtr) NULL;
    bgluTessVertex = (bgluTessVertexProcPtr) NULL;
    bgluNewTess = (bgluNewTessProcPtr) NULL;
    bgluDeleteTess = (bgluDeleteTessProcPtr) NULL;

    bgluPerspective = (bgluPerspectiveProcPtr) NULL;

    bgluErrorString = (bgluErrorStringProcPtr) NULL;

    bgluProject = (bgluProjectProcPtr) NULL;
    bgluUnProject = (bgluUnProjectProcPtr) NULL;
#endif

    return 0;
}


//////// glGenTextures/glDeleteTextures debugging ////////
# if defined DEBUGGINGAIDS && defined DEBUG_TEXTURE_NAMES
static uint8_t *texnameused;  // bitmap
static uint32_t *texnamefromwhere;  // hash of __FILE__
static uint32_t texnameallocsize;

// djb3 algorithm
static inline uint32_t texdbg_getcode(const char *s)
{
    uint32_t h = 5381;
    int32_t ch;

    while ((ch = *s++) != '\0')
        h = ((h << 5) + h) ^ ch;

    return h;
}

static void texdbg_realloc(uint32_t maxtexname)
{
    uint32_t newsize = texnameallocsize ? texnameallocsize : 64;

    if (maxtexname < texnameallocsize)
        return;

    while (maxtexname >= newsize)
        newsize <<= 1;
//    initprintf("texdebug: new size %u\n", newsize);

    texnameused = Xrealloc(texnameused, newsize>>3);
    texnamefromwhere = Xrealloc(texnamefromwhere, newsize*sizeof(uint32_t));

    Bmemset(texnameused + (texnameallocsize>>3), 0, (newsize-texnameallocsize)>>3);
    Bmemset(texnamefromwhere + texnameallocsize, 0, (newsize-texnameallocsize)*sizeof(uint32_t));

    texnameallocsize = newsize;
}

#undef bglGenTextures
void texdbg_bglGenTextures(GLsizei n, GLuint *textures, const char *srcfn)
{
    int32_t i;
    uint32_t hash = srcfn ? texdbg_getcode(srcfn) : 0;

    for (i=0; i<n; i++)
        if (textures[i] < texnameallocsize && (texnameused[textures[i]>>3]&(1<<(textures[i]&7))))
            initprintf("texdebug %x Gen: overwriting used tex name %u from %x\n", hash, textures[i], texnamefromwhere[textures[i]]);

    bglGenTextures(n, textures);

    {
        GLuint maxtexname = 0;

        for (i=0; i<n; i++)
            maxtexname = max(maxtexname, textures[i]);

        texdbg_realloc(maxtexname);

        for (i=0; i<n; i++)
        {
            texnameused[textures[i]>>3] |= (1<<(textures[i]&7));
            texnamefromwhere[textures[i]] = hash;
        }
    }
}

#undef bglDeleteTextures
void texdbg_bglDeleteTextures(GLsizei n, const GLuint *textures, const char *srcfn)
{
    int32_t i;
    uint32_t hash = srcfn ? texdbg_getcode(srcfn) : 0;

    for (i=0; i<n; i++)
        if (textures[i] < texnameallocsize)
        {
            if ((texnameused[textures[i]>>3]&(1<<(textures[i]&7)))==0)
                initprintf("texdebug %x Del: deleting unused tex name %u\n", hash, textures[i]);
            else if ((texnameused[textures[i]>>3]&(1<<(textures[i]&7))) &&
                         texnamefromwhere[textures[i]] != hash)
                initprintf("texdebug %x Del: deleting foreign tex name %u from %x\n", hash,
                           textures[i], texnamefromwhere[textures[i]]);
        }

    bglDeleteTextures(n, textures);

    if (texnameallocsize)
        for (i=0; i<n; i++)
        {
            texnameused[textures[i]>>3] &= ~(1<<(textures[i]&7));
            texnamefromwhere[textures[i]] = 0;
        }
}
# endif  // defined DEBUGGINGAIDS

#endif
