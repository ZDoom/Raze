#include "compat.h"
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
    while ((err = bglGetError()) != GL_NO_ERROR)
    {
        BuildGLError = err; // set a watchpoint/breakpoint here
    }
}

#if defined DYNAMIC_GL

#ifdef _WIN32
bwglCreateContextProcPtr bwglCreateContext;
bwglDeleteContextProcPtr bwglDeleteContext;
bwglGetProcAddressProcPtr bwglGetProcAddress;
bwglMakeCurrentProcPtr bwglMakeCurrent;

bwglSwapBuffersProcPtr bwglSwapBuffers;
bwglChoosePixelFormatProcPtr bwglChoosePixelFormat;
bwglDescribePixelFormatProcPtr bwglDescribePixelFormat;
bwglGetPixelFormatProcPtr bwglGetPixelFormat;
bwglSetPixelFormatProcPtr bwglSetPixelFormat;
#endif

bglClearColorProcPtr bglClearColor;
bglClearProcPtr bglClear;
bglColorMaskProcPtr bglColorMask;
bglAlphaFuncProcPtr bglAlphaFunc;
bglBlendFuncProcPtr bglBlendFunc;
bglCullFaceProcPtr bglCullFace;
bglFrontFaceProcPtr bglFrontFace;
bglPolygonOffsetProcPtr bglPolygonOffset;
bglPolygonModeProcPtr bglPolygonMode;
bglEnableProcPtr bglEnable;
bglDisableProcPtr bglDisable;
bglGetDoublevProcPtr bglGetDoublev;
bglGetFloatvProcPtr bglGetFloatv;
bglGetIntegervProcPtr bglGetIntegerv;
bglPushAttribProcPtr bglPushAttrib;
bglPopAttribProcPtr bglPopAttrib;
bglGetErrorProcPtr bglGetError;
bglGetStringProcPtr bglGetString;
bglHintProcPtr bglHint;
bglDrawBufferProcPtr bglDrawBuffer;
bglReadBufferProcPtr bglReadBuffer;
bglScissorProcPtr bglScissor;
bglClipPlaneProcPtr bglClipPlane;

// Depth
bglDepthFuncProcPtr bglDepthFunc;
bglDepthMaskProcPtr bglDepthMask;
//bglDepthRangeProcPtr bglDepthRange;

// Matrix
bglMatrixModeProcPtr bglMatrixMode;
bglOrthoProcPtr bglOrtho;
bglFrustumProcPtr bglFrustum;
bglViewportProcPtr bglViewport;
bglPushMatrixProcPtr bglPushMatrix;
bglPopMatrixProcPtr bglPopMatrix;
bglLoadIdentityProcPtr bglLoadIdentity;
bglLoadMatrixfProcPtr bglLoadMatrixf;
bglLoadMatrixdProcPtr bglLoadMatrixd;
bglMultMatrixfProcPtr bglMultMatrixf;
bglMultMatrixdProcPtr bglMultMatrixd;
bglRotatefProcPtr bglRotatef;
bglScalefProcPtr bglScalef;
bglTranslatefProcPtr bglTranslatef;

// Drawing
bglBeginProcPtr bglBegin;
bglEndProcPtr bglEnd;
bglVertex2fProcPtr bglVertex2f;
bglVertex2iProcPtr bglVertex2i;
bglVertex3fProcPtr bglVertex3f;
bglVertex3dProcPtr bglVertex3d;
bglVertex3fvProcPtr bglVertex3fv;
bglVertex3dvProcPtr bglVertex3dv;
bglRectiProcPtr bglRecti;
bglColor3fProcPtr bglColor3f;
bglColor4fProcPtr bglColor4f;
bglColor4ubProcPtr bglColor4ub;
bglTexCoord2dProcPtr bglTexCoord2d;
bglTexCoord2fProcPtr bglTexCoord2f;
bglTexCoord2iProcPtr bglTexCoord2i;
bglNormal3fProcPtr bglNormal3f;

// Lighting
bglShadeModelProcPtr bglShadeModel;
bglLightfvProcPtr bglLightfv;

// Raster funcs
bglReadPixelsProcPtr bglReadPixels;
bglRasterPos4iProcPtr bglRasterPos4i;
bglDrawPixelsProcPtr bglDrawPixels;
bglPixelStoreiProcPtr bglPixelStorei;

// Texture mapping
bglTexEnvfProcPtr bglTexEnvf;
bglGenTexturesProcPtr bglGenTextures;
bglDeleteTexturesProcPtr bglDeleteTextures;
bglBindTextureProcPtr bglBindTexture;
bglTexImage2DProcPtr bglTexImage2D;
bglCopyTexImage2DProcPtr bglCopyTexImage2D;
bglCopyTexSubImage2DProcPtr bglCopyTexSubImage2D;
bglTexSubImage2DProcPtr bglTexSubImage2D;
bglTexParameterfProcPtr bglTexParameterf;
bglTexParameteriProcPtr bglTexParameteri;
bglGetTexParameterivProcPtr bglGetTexParameteriv;
bglGetTexLevelParameterivProcPtr bglGetTexLevelParameteriv;
bglTexGenfvProcPtr bglTexGenfv;

// Fog
bglFogfProcPtr bglFogf;
bglFogiProcPtr bglFogi;
bglFogfvProcPtr bglFogfv;

// Display Lists
bglNewListProcPtr bglNewList;
bglEndListProcPtr bglEndList;
bglCallListProcPtr bglCallList;
bglDeleteListsProcPtr bglDeleteLists;

// Vertex Arrays
bglEnableClientStateProcPtr bglEnableClientState;
bglDisableClientStateProcPtr bglDisableClientState;
bglVertexPointerProcPtr bglVertexPointer;
bglNormalPointerProcPtr bglNormalPointer;
bglTexCoordPointerProcPtr bglTexCoordPointer;
bglDrawArraysProcPtr bglDrawArrays;
bglDrawElementsProcPtr bglDrawElements;

// Stencil Buffer
bglClearStencilProcPtr bglClearStencil;
bglStencilOpProcPtr bglStencilOp;
bglStencilFuncProcPtr bglStencilFunc;

#endif

#if defined DYNAMIC_GLEXT

bglBlendEquationProcPtr bglBlendEquation;

bglTexImage3DProcPtr bglTexImage3D;
bglCompressedTexImage2DARBProcPtr bglCompressedTexImage2DARB;
bglGetCompressedTexImageARBProcPtr bglGetCompressedTexImageARB;

// GPU Programs
bglGenProgramsARBProcPtr bglGenProgramsARB;
bglBindProgramARBProcPtr bglBindProgramARB;
bglProgramStringARBProcPtr bglProgramStringARB;
bglDeleteProgramsARBProcPtr bglDeleteProgramsARB;

// Multitexturing
bglActiveTextureARBProcPtr bglActiveTextureARB;
bglClientActiveTextureARBProcPtr bglClientActiveTextureARB;
bglMultiTexCoord2dARBProcPtr bglMultiTexCoord2dARB;
bglMultiTexCoord2fARBProcPtr bglMultiTexCoord2fARB;

// Frame Buffer Objects
bglGenFramebuffersEXTProcPtr bglGenFramebuffersEXT;
bglBindFramebufferEXTProcPtr bglBindFramebufferEXT;
bglFramebufferTexture2DEXTProcPtr bglFramebufferTexture2DEXT;
bglCheckFramebufferStatusEXTProcPtr bglCheckFramebufferStatusEXT;
bglDeleteFramebuffersEXTProcPtr bglDeleteFramebuffersEXT;

// Vertex Buffer Objects
bglGenBuffersARBProcPtr bglGenBuffersARB;
bglBindBufferARBProcPtr bglBindBufferARB;
bglDeleteBuffersARBProcPtr bglDeleteBuffersARB;
bglBufferDataARBProcPtr bglBufferDataARB;
bglBufferSubDataARBProcPtr bglBufferSubDataARB;
bglMapBufferARBProcPtr bglMapBufferARB;
bglUnmapBufferARBProcPtr bglUnmapBufferARB;

// ARB_buffer_storage
bglBufferStorageProcPtr bglBufferStorage;

// ARB_map_buffer_range
bglMapBufferRangeProcPtr bglMapBufferRange;

// Occlusion queries
bglGenQueriesARBProcPtr bglGenQueriesARB;
bglDeleteQueriesARBProcPtr bglDeleteQueriesARB;
bglIsQueryARBProcPtr bglIsQueryARB;
bglBeginQueryARBProcPtr bglBeginQueryARB;
bglEndQueryARBProcPtr bglEndQueryARB;
bglGetQueryivARBProcPtr bglGetQueryivARB;
bglGetQueryObjectivARBProcPtr bglGetQueryObjectivARB;
bglGetQueryObjectuivARBProcPtr bglGetQueryObjectuivARB;

// Shader Objects
bglDeleteObjectARBProcPtr bglDeleteObjectARB;
bglGetHandleARBProcPtr bglGetHandleARB;
bglDetachObjectARBProcPtr bglDetachObjectARB;
bglCreateShaderObjectARBProcPtr bglCreateShaderObjectARB;
bglShaderSourceARBProcPtr bglShaderSourceARB;
bglCompileShaderARBProcPtr bglCompileShaderARB;
bglCreateProgramObjectARBProcPtr bglCreateProgramObjectARB;
bglAttachObjectARBProcPtr bglAttachObjectARB;
bglLinkProgramARBProcPtr bglLinkProgramARB;
bglUseProgramObjectARBProcPtr bglUseProgramObjectARB;
bglValidateProgramARBProcPtr bglValidateProgramARB;
bglUniform1fARBProcPtr bglUniform1fARB;
bglUniform2fARBProcPtr bglUniform2fARB;
bglUniform3fARBProcPtr bglUniform3fARB;
bglUniform4fARBProcPtr bglUniform4fARB;
bglUniform1iARBProcPtr bglUniform1iARB;
bglUniform2iARBProcPtr bglUniform2iARB;
bglUniform3iARBProcPtr bglUniform3iARB;
bglUniform4iARBProcPtr bglUniform4iARB;
bglUniform1fvARBProcPtr bglUniform1fvARB;
bglUniform2fvARBProcPtr bglUniform2fvARB;
bglUniform3fvARBProcPtr bglUniform3fvARB;
bglUniform4fvARBProcPtr bglUniform4fvARB;
bglUniform1ivARBProcPtr bglUniform1ivARB;
bglUniform2ivARBProcPtr bglUniform2ivARB;
bglUniform3ivARBProcPtr bglUniform3ivARB;
bglUniform4ivARBProcPtr bglUniform4ivARB;
bglUniformMatrix2fvARBProcPtr bglUniformMatrix2fvARB;
bglUniformMatrix3fvARBProcPtr bglUniformMatrix3fvARB;
bglUniformMatrix4fvARBProcPtr bglUniformMatrix4fvARB;
bglGetObjectParameterfvARBProcPtr bglGetObjectParameterfvARB;
bglGetObjectParameterivARBProcPtr bglGetObjectParameterivARB;
bglGetInfoLogARBProcPtr bglGetInfoLogARB;
bglGetAttachedObjectsARBProcPtr bglGetAttachedObjectsARB;
bglGetUniformLocationARBProcPtr bglGetUniformLocationARB;
bglGetActiveUniformARBProcPtr bglGetActiveUniformARB;
bglGetUniformfvARBProcPtr bglGetUniformfvARB;
bglGetUniformivARBProcPtr bglGetUniformivARB;
bglGetShaderSourceARBProcPtr bglGetShaderSourceARB;

// Vertex Shaders
bglVertexAttrib1dARBProcPtr bglVertexAttrib1dARB;
bglVertexAttrib1dvARBProcPtr bglVertexAttrib1dvARB;
bglVertexAttrib1fARBProcPtr bglVertexAttrib1fARB;
bglVertexAttrib1fvARBProcPtr bglVertexAttrib1fvARB;
bglVertexAttrib1sARBProcPtr bglVertexAttrib1sARB;
bglVertexAttrib1svARBProcPtr bglVertexAttrib1svARB;
bglVertexAttrib2dARBProcPtr bglVertexAttrib2dARB;
bglVertexAttrib2dvARBProcPtr bglVertexAttrib2dvARB;
bglVertexAttrib2fARBProcPtr bglVertexAttrib2fARB;
bglVertexAttrib2fvARBProcPtr bglVertexAttrib2fvARB;
bglVertexAttrib2sARBProcPtr bglVertexAttrib2sARB;
bglVertexAttrib2svARBProcPtr bglVertexAttrib2svARB;
bglVertexAttrib3dARBProcPtr bglVertexAttrib3dARB;
bglVertexAttrib3dvARBProcPtr bglVertexAttrib3dvARB;
bglVertexAttrib3fARBProcPtr bglVertexAttrib3fARB;
bglVertexAttrib3fvARBProcPtr bglVertexAttrib3fvARB;
bglVertexAttrib3sARBProcPtr bglVertexAttrib3sARB;
bglVertexAttrib3svARBProcPtr bglVertexAttrib3svARB;
bglVertexAttrib4NbvARBProcPtr bglVertexAttrib4NbvARB;
bglVertexAttrib4NivARBProcPtr bglVertexAttrib4NivARB;
bglVertexAttrib4NsvARBProcPtr bglVertexAttrib4NsvARB;
bglVertexAttrib4NubARBProcPtr bglVertexAttrib4NubARB;
bglVertexAttrib4NubvARBProcPtr bglVertexAttrib4NubvARB;
bglVertexAttrib4NuivARBProcPtr bglVertexAttrib4NuivARB;
bglVertexAttrib4NusvARBProcPtr bglVertexAttrib4NusvARB;
bglVertexAttrib4bvARBProcPtr bglVertexAttrib4bvARB;
bglVertexAttrib4dARBProcPtr bglVertexAttrib4dARB;
bglVertexAttrib4dvARBProcPtr bglVertexAttrib4dvARB;
bglVertexAttrib4fARBProcPtr bglVertexAttrib4fARB;
bglVertexAttrib4fvARBProcPtr bglVertexAttrib4fvARB;
bglVertexAttrib4ivARBProcPtr bglVertexAttrib4ivARB;
bglVertexAttrib4sARBProcPtr bglVertexAttrib4sARB;
bglVertexAttrib4svARBProcPtr bglVertexAttrib4svARB;
bglVertexAttrib4ubvARBProcPtr bglVertexAttrib4ubvARB;
bglVertexAttrib4uivARBProcPtr bglVertexAttrib4uivARB;
bglVertexAttrib4usvARBProcPtr bglVertexAttrib4usvARB;
bglVertexAttribPointerARBProcPtr bglVertexAttribPointerARB;
bglEnableVertexAttribArrayARBProcPtr bglEnableVertexAttribArrayARB;
bglDisableVertexAttribArrayARBProcPtr bglDisableVertexAttribArrayARB;
bglGetVertexAttribdvARBProcPtr bglGetVertexAttribdvARB;
bglGetVertexAttribfvARBProcPtr bglGetVertexAttribfvARB;
bglGetVertexAttribivARBProcPtr bglGetVertexAttribivARB;
bglGetVertexAttribPointervARBProcPtr bglGetVertexAttribPointervARB;
bglBindAttribLocationARBProcPtr bglBindAttribLocationARB;
bglGetActiveAttribARBProcPtr bglGetActiveAttribARB;
bglGetAttribLocationARBProcPtr bglGetAttribLocationARB;

// Debug Output
#ifndef __APPLE__
bglDebugMessageControlARBProcPtr bglDebugMessageControlARB;
bglDebugMessageCallbackARBProcPtr bglDebugMessageCallbackARB;
#endif

#ifdef _WIN32
bwglSwapIntervalEXTProcPtr bwglSwapIntervalEXT;
bwglCreateContextAttribsARBProcPtr bwglCreateContextAttribsARB;
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
#endif

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
#define GETPROCSOFT(s)    getproc_(s,&err,0,0)
#define GETPROCEXT(s)     getproc_(s,&err,1,1)
#define GETPROCEXTSOFT(s) getproc_(s,&err,0,1)

#endif

int32_t loadgldriver(const char *driver)
{
#if defined EDUKE32_GLES
    jwzgles_reset();
#endif

#if defined DYNAMIC_GL || defined DYNAMIC_GLEXT
    int32_t err=0;

#if !defined RENDERTYPESDL && defined _WIN32
    if (hGLDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "opengl32.dll";
#elif defined EDUKE32_OSX
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
#elif defined __OpenBSD__
        driver = "libGL.so";
#else
        driver = "libGL.so.1";
#endif
    }

#if defined RENDERTYPESDL && !defined EDUKE32_IOS
    if (SDL_GL_LoadLibrary(driver))
    {
        initprintf("Failed loading \"%s\": %s\n", driver, SDL_GetError());
        return -1;
    }
#elif defined _WIN32
    hGLDLL = LoadLibrary(driver);
    if (!hGLDLL)
    {
        initprintf("Failed loading \"%s\"\n", driver);
        return -1;
    }
#endif
    gldriver = Bstrdup(driver);
#endif

#if defined DYNAMIC_GL
#ifdef _WIN32
    bwglCreateContext = (bwglCreateContextProcPtr) GETPROC("wglCreateContext");
    bwglDeleteContext = (bwglDeleteContextProcPtr) GETPROC("wglDeleteContext");
    bwglGetProcAddress = (bwglGetProcAddressProcPtr) GETPROC("wglGetProcAddress");
    bwglMakeCurrent = (bwglMakeCurrentProcPtr) GETPROC("wglMakeCurrent");

    bwglSwapBuffers = (bwglSwapBuffersProcPtr) GETPROC("wglSwapBuffers");
    bwglChoosePixelFormat = (bwglChoosePixelFormatProcPtr) GETPROC("wglChoosePixelFormat");
    bwglDescribePixelFormat = (bwglDescribePixelFormatProcPtr) GETPROC("wglDescribePixelFormat");
    bwglGetPixelFormat = (bwglGetPixelFormatProcPtr) GETPROC("wglGetPixelFormat");
    bwglSetPixelFormat = (bwglSetPixelFormatProcPtr) GETPROC("wglSetPixelFormat");
#endif

    bglClearColor = (bglClearColorProcPtr) GETPROC("glClearColor");
    bglClear = (bglClearProcPtr) GETPROC("glClear");
    bglColorMask = (bglColorMaskProcPtr) GETPROC("glColorMask");
    bglAlphaFunc = (bglAlphaFuncProcPtr) GETPROC("glAlphaFunc");
    bglBlendFunc = (bglBlendFuncProcPtr) GETPROC("glBlendFunc");
    bglCullFace = (bglCullFaceProcPtr) GETPROC("glCullFace");
    bglFrontFace = (bglFrontFaceProcPtr) GETPROC("glFrontFace");
    bglPolygonOffset = (bglPolygonOffsetProcPtr) GETPROC("glPolygonOffset");
    bglPolygonMode = (bglPolygonModeProcPtr) GETPROC("glPolygonMode");
    bglEnable = (bglEnableProcPtr) GETPROC("glEnable");
    bglDisable = (bglDisableProcPtr) GETPROC("glDisable");
    bglGetDoublev = (bglGetDoublevProcPtr) GETPROC("glGetDoublev");
    bglGetFloatv = (bglGetFloatvProcPtr) GETPROC("glGetFloatv");
    bglGetIntegerv = (bglGetIntegervProcPtr) GETPROC("glGetIntegerv");
    bglPushAttrib = (bglPushAttribProcPtr) GETPROC("glPushAttrib");
    bglPopAttrib = (bglPopAttribProcPtr) GETPROC("glPopAttrib");
    bglGetError = (bglGetErrorProcPtr) GETPROC("glGetError");
    bglGetString = (bglGetStringProcPtr) GETPROC("glGetString");
    bglHint = (bglHintProcPtr) GETPROC("glHint");
    bglDrawBuffer = (bglDrawBufferProcPtr) GETPROC("glDrawBuffer");
    bglReadBuffer = (bglReadBufferProcPtr) GETPROC("glDrawBuffer");
    bglScissor = (bglScissorProcPtr) GETPROC("glScissor");
    bglClipPlane = (bglClipPlaneProcPtr) GETPROC("glClipPlane");

    // Depth
    bglDepthFunc = (bglDepthFuncProcPtr) GETPROC("glDepthFunc");
    bglDepthMask = (bglDepthMaskProcPtr) GETPROC("glDepthMask");
//    bglDepthRange = (bglDepthRangeProcPtr) GETPROC("glDepthRange");

    // Matrix
    bglMatrixMode = (bglMatrixModeProcPtr) GETPROC("glMatrixMode");
    bglOrtho = (bglOrthoProcPtr) GETPROC("glOrtho");
    bglFrustum = (bglFrustumProcPtr) GETPROC("glFrustum");
    bglViewport = (bglViewportProcPtr) GETPROC("glViewport");
    bglPushMatrix = (bglPushMatrixProcPtr) GETPROC("glPushMatrix");
    bglPopMatrix = (bglPopMatrixProcPtr) GETPROC("glPopMatrix");
    bglLoadIdentity = (bglLoadIdentityProcPtr) GETPROC("glLoadIdentity");
    bglLoadMatrixf = (bglLoadMatrixfProcPtr) GETPROC("glLoadMatrixf");
    bglLoadMatrixd = (bglLoadMatrixdProcPtr) GETPROC("glLoadMatrixd");
    bglMultMatrixf = (bglMultMatrixfProcPtr) GETPROC("glMultMatrixf");
    bglMultMatrixd = (bglMultMatrixdProcPtr) GETPROC("glMultMatrixd");
    bglRotatef = (bglRotatefProcPtr) GETPROC("glRotatef");
    bglScalef = (bglScalefProcPtr) GETPROC("glScalef");
    bglTranslatef = (bglTranslatefProcPtr) GETPROC("glTranslatef");

    // Drawing
    bglBegin = (bglBeginProcPtr) GETPROC("glBegin");
    bglEnd = (bglEndProcPtr) GETPROC("glEnd");
    bglVertex2f = (bglVertex2fProcPtr) GETPROC("glVertex2f");
    bglVertex2i = (bglVertex2iProcPtr) GETPROC("glVertex2i");
    bglVertex3f = (bglVertex3fProcPtr) GETPROC("glVertex3f");
    bglVertex3d = (bglVertex3dProcPtr) GETPROC("glVertex3d");
    bglVertex3fv = (bglVertex3fvProcPtr) GETPROC("glVertex3fv");
    bglVertex3dv = (bglVertex3dvProcPtr) GETPROC("glVertex3dv");
    bglRecti = (bglRectiProcPtr) GETPROC("glRecti");
    bglColor3f = (bglColor3fProcPtr) GETPROC("glColor3f");
    bglColor4f = (bglColor4fProcPtr) GETPROC("glColor4f");
    bglColor4ub = (bglColor4ubProcPtr) GETPROC("glColor4ub");
    bglTexCoord2d = (bglTexCoord2dProcPtr) GETPROC("glTexCoord2d");
    bglTexCoord2f = (bglTexCoord2fProcPtr) GETPROC("glTexCoord2f");
    bglTexCoord2i = (bglTexCoord2iProcPtr) GETPROC("glTexCoord2i");
    bglNormal3f = (bglNormal3fProcPtr) GETPROC("glNormal3f");

    // Lighting
    bglShadeModel = (bglShadeModelProcPtr) GETPROC("glShadeModel");
    bglLightfv = (bglLightfvProcPtr) GETPROC("glLightfv");

    // Raster funcs
    bglReadPixels = (bglReadPixelsProcPtr) GETPROC("glReadPixels");
    bglRasterPos4i = (bglRasterPos4iProcPtr) GETPROC("glRasterPos4i");
    bglDrawPixels = (bglDrawPixelsProcPtr) GETPROC("glDrawPixels");
    bglPixelStorei = (bglPixelStoreiProcPtr) GETPROC("glPixelStorei");

    // Texture mapping
    bglTexEnvf = (bglTexEnvfProcPtr) GETPROC("glTexEnvf");
    bglGenTextures = (bglGenTexturesProcPtr) GETPROC("glGenTextures");
    bglDeleteTextures = (bglDeleteTexturesProcPtr) GETPROC("glDeleteTextures");
    bglBindTexture = (bglBindTextureProcPtr) GETPROC("glBindTexture");
    bglTexImage2D = (bglTexImage2DProcPtr) GETPROC("glTexImage2D");
    bglCopyTexImage2D = (bglCopyTexImage2DProcPtr) GETPROC("glCopyTexImage2D");
    bglCopyTexSubImage2D = (bglCopyTexSubImage2DProcPtr) GETPROC("glCopyTexSubImage2D");
    bglTexSubImage2D = (bglTexSubImage2DProcPtr) GETPROC("glTexSubImage2D");
    bglTexParameterf = (bglTexParameterfProcPtr) GETPROC("glTexParameterf");
    bglTexParameteri = (bglTexParameteriProcPtr) GETPROC("glTexParameteri");
    bglGetTexParameteriv = (bglGetTexParameterivProcPtr) GETPROC("glGetTexParameteriv");
    bglGetTexLevelParameteriv = (bglGetTexLevelParameterivProcPtr) GETPROC("glGetTexLevelParameteriv");
    bglTexGenfv = (bglTexGenfvProcPtr) GETPROC("glTexGenfv");

    // Fog
    bglFogf = (bglFogfProcPtr) GETPROC("glFogf");
    bglFogi = (bglFogiProcPtr) GETPROC("glFogi");
    bglFogfv = (bglFogfvProcPtr) GETPROC("glFogfv");

    // Display Lists
    bglNewList = (bglNewListProcPtr) GETPROC("glNewList");
    bglEndList = (bglEndListProcPtr) GETPROC("glEndList");
    bglCallList = (bglCallListProcPtr) GETPROC("glCallList");
    bglDeleteLists = (bglDeleteListsProcPtr) GETPROC("glDeleteLists");

    // Vertex Arrays
    bglEnableClientState = (bglEnableClientStateProcPtr) GETPROC("glEnableClientState");
    bglDisableClientState = (bglDisableClientStateProcPtr) GETPROC("glDisableClientState");
    bglVertexPointer = (bglVertexPointerProcPtr) GETPROC("glVertexPointer");
    bglNormalPointer = (bglNormalPointerProcPtr) GETPROC("glNormalPointer");
    bglTexCoordPointer = (bglTexCoordPointerProcPtr) GETPROC("glTexCoordPointer");
    bglDrawArrays = (bglDrawArraysProcPtr) GETPROC("glDrawArrays");
    bglDrawElements = (bglDrawElementsProcPtr) GETPROC("glDrawElements");

    // Stencil Buffer
    bglClearStencil = (bglClearStencilProcPtr) GETPROC("glClearStencil");
    bglStencilOp = (bglStencilOpProcPtr) GETPROC("glStencilOp");
    bglStencilFunc = (bglStencilFuncProcPtr) GETPROC("glStencilFunc");
#endif

    loadglextensions();
    loadglulibrary(getenv("BUILD_GLULIB"));

#if defined DYNAMIC_GL || defined DYNAMIC_GLEXT
    if (err) unloadgldriver();
    return err;
#else
    UNREFERENCED_PARAMETER(driver);
    return 0;
#endif
}

int32_t loadglextensions(void)
{
#if defined DYNAMIC_GLEXT
    int32_t err = 0;
#if !defined RENDERTYPESDL && defined _WIN32
    if (!hGLDLL) return 0;
#endif

    bglBlendEquation = (bglBlendEquationProcPtr) GETPROCEXTSOFT("glBlendEquation");

    bglTexImage3D = (bglTexImage3DProcPtr) GETPROCEXTSOFT("glTexImage3D");
    bglCompressedTexImage2DARB = (bglCompressedTexImage2DARBProcPtr) GETPROCEXTSOFT("glCompressedTexImage2DARB");
    bglGetCompressedTexImageARB = (bglGetCompressedTexImageARBProcPtr) GETPROCEXTSOFT("glGetCompressedTexImageARB");

    // GPU Programs
    bglGenProgramsARB = (bglGenProgramsARBProcPtr) GETPROCEXTSOFT("glGenProgramsARB");
    bglBindProgramARB = (bglBindProgramARBProcPtr) GETPROCEXTSOFT("glBindProgramARB");
    bglProgramStringARB = (bglProgramStringARBProcPtr) GETPROCEXTSOFT("glProgramStringARB");
    bglDeleteProgramsARB = (bglDeleteProgramsARBProcPtr) GETPROCEXTSOFT("glDeleteProgramsARB");

    // Multitexturing
    bglActiveTextureARB = (bglActiveTextureARBProcPtr) GETPROCEXTSOFT("glActiveTextureARB");
    bglClientActiveTextureARB = (bglClientActiveTextureARBProcPtr) GETPROCEXTSOFT("glClientActiveTextureARB");
    bglMultiTexCoord2dARB = (bglMultiTexCoord2dARBProcPtr) GETPROCEXTSOFT("glMultiTexCoord2dARB");
    bglMultiTexCoord2fARB = (bglMultiTexCoord2fARBProcPtr) GETPROCEXTSOFT("glMultiTexCoord2fARB");

    // Frame Buffer Objects
    bglGenFramebuffersEXT = (bglGenFramebuffersEXTProcPtr) GETPROCEXTSOFT("glGenFramebuffersEXT");
    bglBindFramebufferEXT = (bglBindFramebufferEXTProcPtr) GETPROCEXTSOFT("glBindFramebufferEXT");
    bglFramebufferTexture2DEXT = (bglFramebufferTexture2DEXTProcPtr) GETPROCEXTSOFT("glFramebufferTexture2DEXT");
    bglCheckFramebufferStatusEXT = (bglCheckFramebufferStatusEXTProcPtr) GETPROCEXTSOFT("glCheckFramebufferStatusEXT");
    bglDeleteFramebuffersEXT = (bglDeleteFramebuffersEXTProcPtr) GETPROCEXTSOFT("glDeleteFramebuffersEXT");

    // Vertex Buffer Objects
    bglGenBuffersARB = (bglGenBuffersARBProcPtr) GETPROCEXTSOFT("glGenBuffersARB");
    bglBindBufferARB = (bglBindBufferARBProcPtr) GETPROCEXTSOFT("glBindBufferARB");
    bglDeleteBuffersARB = (bglDeleteBuffersARBProcPtr) GETPROCEXTSOFT("glDeleteBuffersARB");
    bglBufferDataARB = (bglBufferDataARBProcPtr) GETPROCEXTSOFT("glBufferDataARB");
    bglBufferSubDataARB = (bglBufferSubDataARBProcPtr) GETPROCEXTSOFT("glBufferSubDataARB");
    bglMapBufferARB = (bglMapBufferARBProcPtr) GETPROCEXTSOFT("glMapBufferARB");
    bglUnmapBufferARB = (bglUnmapBufferARBProcPtr) GETPROCEXTSOFT("glUnmapBufferARB");

    // ARB_buffer_storage
    bglBufferStorage = (bglBufferStorageProcPtr)GETPROCEXTSOFT("glBufferStorage");

    // ARB_map_buffer_range
    bglMapBufferRange = (bglMapBufferRangeProcPtr)GETPROCEXTSOFT("glMapBufferRange");

    // Occlusion queries
    bglGenQueriesARB = (bglGenQueriesARBProcPtr) GETPROCEXTSOFT("glGenQueriesARB");
    bglDeleteQueriesARB = (bglDeleteQueriesARBProcPtr) GETPROCEXTSOFT("glDeleteQueriesARB");
    bglIsQueryARB = (bglIsQueryARBProcPtr) GETPROCEXTSOFT("glIsQueryARB");
    bglBeginQueryARB = (bglBeginQueryARBProcPtr) GETPROCEXTSOFT("glBeginQueryARB");
    bglEndQueryARB = (bglEndQueryARBProcPtr) GETPROCEXTSOFT("glEndQueryARB");
    bglGetQueryivARB = (bglGetQueryivARBProcPtr) GETPROCEXTSOFT("glGetQueryivARB");
    bglGetQueryObjectivARB = (bglGetQueryObjectivARBProcPtr) GETPROCEXTSOFT("glGetQueryObjectivARB");
    bglGetQueryObjectuivARB = (bglGetQueryObjectuivARBProcPtr) GETPROCEXTSOFT("glGetQueryObjectuivARB");

    // Shader Objects
    bglDeleteObjectARB = (bglDeleteObjectARBProcPtr) GETPROCEXTSOFT("glDeleteObjectARB");
    bglGetHandleARB = (bglGetHandleARBProcPtr) GETPROCEXTSOFT("glGetHandleARB");
    bglDetachObjectARB = (bglDetachObjectARBProcPtr) GETPROCEXTSOFT("glDetachObjectARB");
    bglCreateShaderObjectARB = (bglCreateShaderObjectARBProcPtr) GETPROCEXTSOFT("glCreateShaderObjectARB");
    bglShaderSourceARB = (bglShaderSourceARBProcPtr) GETPROCEXTSOFT("glShaderSourceARB");
    bglCompileShaderARB = (bglCompileShaderARBProcPtr) GETPROCEXTSOFT("glCompileShaderARB");
    bglCreateProgramObjectARB = (bglCreateProgramObjectARBProcPtr) GETPROCEXTSOFT("glCreateProgramObjectARB");
    bglAttachObjectARB = (bglAttachObjectARBProcPtr) GETPROCEXTSOFT("glAttachObjectARB");
    bglLinkProgramARB = (bglLinkProgramARBProcPtr) GETPROCEXTSOFT("glLinkProgramARB");
    bglUseProgramObjectARB = (bglUseProgramObjectARBProcPtr) GETPROCEXTSOFT("glUseProgramObjectARB");
    bglValidateProgramARB = (bglValidateProgramARBProcPtr) GETPROCEXTSOFT("glValidateProgramARB");
    bglUniform1fARB = (bglUniform1fARBProcPtr) GETPROCEXTSOFT("glUniform1fARB");
    bglUniform2fARB = (bglUniform2fARBProcPtr) GETPROCEXTSOFT("glUniform2fARB");
    bglUniform3fARB = (bglUniform3fARBProcPtr) GETPROCEXTSOFT("glUniform3fARB");
    bglUniform4fARB = (bglUniform4fARBProcPtr) GETPROCEXTSOFT("glUniform4fARB");
    bglUniform1iARB = (bglUniform1iARBProcPtr) GETPROCEXTSOFT("glUniform1iARB");
    bglUniform2iARB = (bglUniform2iARBProcPtr) GETPROCEXTSOFT("glUniform2iARB");
    bglUniform3iARB = (bglUniform3iARBProcPtr) GETPROCEXTSOFT("glUniform3iARB");
    bglUniform4iARB = (bglUniform4iARBProcPtr) GETPROCEXTSOFT("glUniform4iARB");
    bglUniform1fvARB = (bglUniform1fvARBProcPtr) GETPROCEXTSOFT("glUniform1fvARB");
    bglUniform2fvARB = (bglUniform2fvARBProcPtr) GETPROCEXTSOFT("glUniform2fvARB");
    bglUniform3fvARB = (bglUniform3fvARBProcPtr) GETPROCEXTSOFT("glUniform3fvARB");
    bglUniform4fvARB = (bglUniform4fvARBProcPtr) GETPROCEXTSOFT("glUniform4fvARB");
    bglUniform1ivARB = (bglUniform1ivARBProcPtr) GETPROCEXTSOFT("glUniform1ivARB");
    bglUniform2ivARB = (bglUniform2ivARBProcPtr) GETPROCEXTSOFT("glUniform2ivARB");
    bglUniform3ivARB = (bglUniform3ivARBProcPtr) GETPROCEXTSOFT("glUniform3ivARB");
    bglUniform4ivARB = (bglUniform4ivARBProcPtr) GETPROCEXTSOFT("glUniform4ivARB");
    bglUniformMatrix2fvARB = (bglUniformMatrix2fvARBProcPtr) GETPROCEXTSOFT("glUniformMatrix2fvARB");
    bglUniformMatrix3fvARB = (bglUniformMatrix3fvARBProcPtr) GETPROCEXTSOFT("glUniformMatrix3fvARB");
    bglUniformMatrix4fvARB = (bglUniformMatrix4fvARBProcPtr) GETPROCEXTSOFT("glUniformMatrix4fvARB");
    bglGetObjectParameterfvARB = (bglGetObjectParameterfvARBProcPtr) GETPROCEXTSOFT("glGetObjectParameterfvARB");
    bglGetObjectParameterivARB = (bglGetObjectParameterivARBProcPtr) GETPROCEXTSOFT("glGetObjectParameterivARB");
    bglGetInfoLogARB = (bglGetInfoLogARBProcPtr) GETPROCEXTSOFT("glGetInfoLogARB");
    bglGetAttachedObjectsARB = (bglGetAttachedObjectsARBProcPtr) GETPROCEXTSOFT("glGetAttachedObjectsARB");
    bglGetUniformLocationARB = (bglGetUniformLocationARBProcPtr) GETPROCEXTSOFT("glGetUniformLocationARB");
    bglGetActiveUniformARB = (bglGetActiveUniformARBProcPtr) GETPROCEXTSOFT("glGetActiveUniformARB");
    bglGetUniformfvARB = (bglGetUniformfvARBProcPtr) GETPROCEXTSOFT("glGetUniformfvARB");
    bglGetUniformivARB = (bglGetUniformivARBProcPtr) GETPROCEXTSOFT("glGetUniformivARB");
    bglGetShaderSourceARB = (bglGetShaderSourceARBProcPtr) GETPROCEXTSOFT("glGetShaderSourceARB");

    // Vertex Shaders
    bglVertexAttrib1dARB = (bglVertexAttrib1dARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1dARB");
    bglVertexAttrib1dvARB = (bglVertexAttrib1dvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1dvARB");
    bglVertexAttrib1fARB = (bglVertexAttrib1fARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1fARB");
    bglVertexAttrib1fvARB = (bglVertexAttrib1fvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1fvARB");
    bglVertexAttrib1sARB = (bglVertexAttrib1sARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1sARB");
    bglVertexAttrib1svARB = (bglVertexAttrib1svARBProcPtr) GETPROCEXTSOFT("glVertexAttrib1svARB");
    bglVertexAttrib2dARB = (bglVertexAttrib2dARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2dARB");
    bglVertexAttrib2dvARB = (bglVertexAttrib2dvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2dvARB");
    bglVertexAttrib2fARB = (bglVertexAttrib2fARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2fARB");
    bglVertexAttrib2fvARB = (bglVertexAttrib2fvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2fvARB");
    bglVertexAttrib2sARB = (bglVertexAttrib2sARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2sARB");
    bglVertexAttrib2svARB = (bglVertexAttrib2svARBProcPtr) GETPROCEXTSOFT("glVertexAttrib2svARB");
    bglVertexAttrib3dARB = (bglVertexAttrib3dARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3dARB");
    bglVertexAttrib3dvARB = (bglVertexAttrib3dvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3dvARB");
    bglVertexAttrib3fARB = (bglVertexAttrib3fARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3fARB");
    bglVertexAttrib3fvARB = (bglVertexAttrib3fvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3fvARB");
    bglVertexAttrib3sARB = (bglVertexAttrib3sARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3sARB");
    bglVertexAttrib3svARB = (bglVertexAttrib3svARBProcPtr) GETPROCEXTSOFT("glVertexAttrib3svARB");
    bglVertexAttrib4NbvARB = (bglVertexAttrib4NbvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NbvARB");
    bglVertexAttrib4NivARB = (bglVertexAttrib4NivARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NivARB");
    bglVertexAttrib4NsvARB = (bglVertexAttrib4NsvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NsvARB");
    bglVertexAttrib4NubARB = (bglVertexAttrib4NubARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NubARB");
    bglVertexAttrib4NubvARB = (bglVertexAttrib4NubvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NubvARB");
    bglVertexAttrib4NuivARB = (bglVertexAttrib4NuivARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NuivARB");
    bglVertexAttrib4NusvARB = (bglVertexAttrib4NusvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4NusvARB");
    bglVertexAttrib4bvARB = (bglVertexAttrib4bvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4bvARB");
    bglVertexAttrib4dARB = (bglVertexAttrib4dARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4dARB");
    bglVertexAttrib4dvARB = (bglVertexAttrib4dvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4dvARB");
    bglVertexAttrib4fARB = (bglVertexAttrib4fARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4fARB");
    bglVertexAttrib4fvARB = (bglVertexAttrib4fvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4fvARB");
    bglVertexAttrib4ivARB = (bglVertexAttrib4ivARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4ivARB");
    bglVertexAttrib4sARB = (bglVertexAttrib4sARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4sARB");
    bglVertexAttrib4svARB = (bglVertexAttrib4svARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4svARB");
    bglVertexAttrib4ubvARB = (bglVertexAttrib4ubvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4ubvARB");
    bglVertexAttrib4uivARB = (bglVertexAttrib4uivARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4uivARB");
    bglVertexAttrib4usvARB = (bglVertexAttrib4usvARBProcPtr) GETPROCEXTSOFT("glVertexAttrib4usvARB");
    bglVertexAttribPointerARB = (bglVertexAttribPointerARBProcPtr) GETPROCEXTSOFT("glVertexAttribPointerARB");
    bglEnableVertexAttribArrayARB = (bglEnableVertexAttribArrayARBProcPtr) GETPROCEXTSOFT("glEnableVertexAttribArrayARB");
    bglDisableVertexAttribArrayARB = (bglDisableVertexAttribArrayARBProcPtr) GETPROCEXTSOFT("glDisableVertexAttribArrayARB");
    bglGetVertexAttribdvARB = (bglGetVertexAttribdvARBProcPtr) GETPROCEXTSOFT("glGetVertexAttribdvARB");
    bglGetVertexAttribfvARB = (bglGetVertexAttribfvARBProcPtr) GETPROCEXTSOFT("glGetVertexAttribfvARB");
    bglGetVertexAttribivARB = (bglGetVertexAttribivARBProcPtr) GETPROCEXTSOFT("glGetVertexAttribivARB");
    bglGetVertexAttribPointervARB = (bglGetVertexAttribPointervARBProcPtr) GETPROCEXTSOFT("glGetVertexAttribPointervARB");
    bglBindAttribLocationARB = (bglBindAttribLocationARBProcPtr) GETPROCEXTSOFT("glBindAttribLocationARB");
    bglGetActiveAttribARB = (bglGetActiveAttribARBProcPtr) GETPROCEXTSOFT("glGetActiveAttribARB");
    bglGetAttribLocationARB = (bglGetAttribLocationARBProcPtr) GETPROCEXTSOFT("glGetAttribLocationARB");

    // Debug Output
#ifndef __APPLE__
    bglDebugMessageControlARB = (bglDebugMessageControlARBProcPtr) GETPROCEXTSOFT("glDebugMessageControlARB");
    bglDebugMessageCallbackARB = (bglDebugMessageCallbackARBProcPtr) GETPROCEXTSOFT("glDebugMessageCallbackARB");
#endif

#ifdef _WIN32
    bwglSwapIntervalEXT = (bwglSwapIntervalEXTProcPtr) GETPROCEXTSOFT("wglSwapIntervalEXT");
    bwglCreateContextAttribsARB = (bwglCreateContextAttribsARBProcPtr) GETPROCEXTSOFT("wglCreateContextAttribsARB");
#endif

    // the following ARB functions are used in POLYMER=0 builds:
    // glActiveTextureARB,
    // glDeleteBuffersARB, glGenBuffersARB, glBindBufferARB,
    // glMapBufferARB, glUnmapBufferARB, glBufferDataARB,
    // glClientActiveTextureARB,
    // glGetCompressedTexImageARB, glCompressedTexImage2DARB

    return err;
#else
    return 0;
#endif
}

int32_t unloadgldriver(void)
{
    unloadglulibrary();

#if defined DYNAMIC_GL || defined DYNAMIC_GLEXT
#if !defined RENDERTYPESDL && defined _WIN32
    if (!hGLDLL) return 0;
#endif

    DO_FREE_AND_NULL(gldriver);

#if !defined RENDERTYPESDL && defined _WIN32
    FreeLibrary(hGLDLL);
    hGLDLL = NULL;
#endif
#endif

#if defined DYNAMIC_GL
#ifdef _WIN32
    bwglCreateContext = (bwglCreateContextProcPtr) NULL;
    bwglDeleteContext = (bwglDeleteContextProcPtr) NULL;
    bwglGetProcAddress = (bwglGetProcAddressProcPtr) NULL;
    bwglMakeCurrent = (bwglMakeCurrentProcPtr) NULL;

    bwglSwapBuffers = (bwglSwapBuffersProcPtr) NULL;
    bwglChoosePixelFormat = (bwglChoosePixelFormatProcPtr) NULL;
    bwglDescribePixelFormat = (bwglDescribePixelFormatProcPtr) NULL;
    bwglGetPixelFormat = (bwglGetPixelFormatProcPtr) NULL;
    bwglSetPixelFormat = (bwglSetPixelFormatProcPtr) NULL;
#endif

    bglClearColor = (bglClearColorProcPtr) NULL;
    bglClear = (bglClearProcPtr) NULL;
    bglColorMask = (bglColorMaskProcPtr) NULL;
    bglAlphaFunc = (bglAlphaFuncProcPtr) NULL;
    bglBlendFunc = (bglBlendFuncProcPtr) NULL;
    bglCullFace = (bglCullFaceProcPtr) NULL;
    bglFrontFace = (bglFrontFaceProcPtr) NULL;
    bglPolygonOffset = (bglPolygonOffsetProcPtr) NULL;
    bglPolygonMode = (bglPolygonModeProcPtr) NULL;
    bglEnable = (bglEnableProcPtr) NULL;
    bglDisable = (bglDisableProcPtr) NULL;
    bglGetDoublev = (bglGetDoublevProcPtr) NULL;
    bglGetFloatv = (bglGetFloatvProcPtr) NULL;
    bglGetIntegerv = (bglGetIntegervProcPtr) NULL;
    bglPushAttrib = (bglPushAttribProcPtr) NULL;
    bglPopAttrib = (bglPopAttribProcPtr) NULL;
    bglGetError = (bglGetErrorProcPtr) NULL;
    bglGetString = (bglGetStringProcPtr) NULL;
    bglHint = (bglHintProcPtr) NULL;
    bglDrawBuffer = (bglDrawBufferProcPtr) NULL;
    bglReadBuffer = (bglReadBufferProcPtr) NULL;
    bglScissor = (bglScissorProcPtr) NULL;
    bglClipPlane = (bglClipPlaneProcPtr) NULL;

    // Depth
    bglDepthFunc = (bglDepthFuncProcPtr) NULL;
    bglDepthMask = (bglDepthMaskProcPtr) NULL;
//    bglDepthRange = (bglDepthRangeProcPtr) NULL;

    // Matrix
    bglMatrixMode = (bglMatrixModeProcPtr) NULL;
    bglOrtho = (bglOrthoProcPtr) NULL;
    bglFrustum = (bglFrustumProcPtr) NULL;
    bglViewport = (bglViewportProcPtr) NULL;
    bglPushMatrix = (bglPushMatrixProcPtr) NULL;
    bglPopMatrix = (bglPopMatrixProcPtr) NULL;
    bglLoadIdentity = (bglLoadIdentityProcPtr) NULL;
    bglLoadMatrixf = (bglLoadMatrixfProcPtr) NULL;
    bglLoadMatrixd = (bglLoadMatrixdProcPtr) NULL;
    bglMultMatrixf = (bglMultMatrixfProcPtr) NULL;
    bglMultMatrixd = (bglMultMatrixdProcPtr) NULL;
    bglRotatef = (bglRotatefProcPtr) NULL;
    bglScalef = (bglScalefProcPtr) NULL;
    bglTranslatef = (bglTranslatefProcPtr) NULL;

    // Drawing
    bglBegin = (bglBeginProcPtr) NULL;
    bglEnd = (bglEndProcPtr) NULL;
    bglVertex2f = (bglVertex2fProcPtr) NULL;
    bglVertex2i = (bglVertex2iProcPtr) NULL;
    bglVertex3f = (bglVertex3fProcPtr) NULL;
    bglVertex3d = (bglVertex3dProcPtr) NULL;
    bglVertex3fv = (bglVertex3fvProcPtr) NULL;
    bglVertex3dv = (bglVertex3dvProcPtr) NULL;
    bglRecti = (bglRectiProcPtr) NULL;
    bglColor3f = (bglColor3fProcPtr) NULL;
    bglColor4f = (bglColor4fProcPtr) NULL;
    bglColor4ub = (bglColor4ubProcPtr) NULL;
    bglTexCoord2d = (bglTexCoord2dProcPtr) NULL;
    bglTexCoord2f = (bglTexCoord2fProcPtr) NULL;
    bglTexCoord2i = (bglTexCoord2iProcPtr) NULL;
    bglNormal3f = (bglNormal3fProcPtr) NULL;

    // Lighting
    bglShadeModel = (bglShadeModelProcPtr) NULL;
    bglLightfv = (bglLightfvProcPtr) NULL;

    // Raster funcs
    bglReadPixels = (bglReadPixelsProcPtr) NULL;
    bglRasterPos4i = (bglRasterPos4iProcPtr) NULL;
    bglDrawPixels = (bglDrawPixelsProcPtr) NULL;
    bglPixelStorei = (bglPixelStoreiProcPtr) NULL;

    // Texture mapping
    bglTexEnvf = (bglTexEnvfProcPtr) NULL;
    bglGenTextures = (bglGenTexturesProcPtr) NULL;
    bglDeleteTextures = (bglDeleteTexturesProcPtr) NULL;
    bglBindTexture = (bglBindTextureProcPtr) NULL;
    bglTexImage2D = (bglTexImage2DProcPtr) NULL;
    bglCopyTexImage2D = (bglCopyTexImage2DProcPtr) NULL;
    bglCopyTexSubImage2D = (bglCopyTexSubImage2DProcPtr) NULL;
    bglTexSubImage2D = (bglTexSubImage2DProcPtr) NULL;
    bglTexParameterf = (bglTexParameterfProcPtr) NULL;
    bglTexParameteri = (bglTexParameteriProcPtr) NULL;
    bglGetTexParameteriv = (bglGetTexParameterivProcPtr) NULL;
    bglGetTexLevelParameteriv = (bglGetTexLevelParameterivProcPtr) NULL;
    bglTexGenfv = (bglTexGenfvProcPtr) NULL;

    // Fog
    bglFogf = (bglFogfProcPtr) NULL;
    bglFogi = (bglFogiProcPtr) NULL;
    bglFogfv = (bglFogfvProcPtr) NULL;

    // Display Lists
    bglNewList = (bglNewListProcPtr) NULL;
    bglEndList = (bglEndListProcPtr) NULL;
    bglCallList = (bglCallListProcPtr) NULL;
    bglDeleteLists = (bglDeleteListsProcPtr) NULL;

    // Vertex Arrays
    bglEnableClientState = (bglEnableClientStateProcPtr) NULL;
    bglDisableClientState = (bglDisableClientStateProcPtr) NULL;
    bglVertexPointer = (bglVertexPointerProcPtr) NULL;
    bglNormalPointer = (bglNormalPointerProcPtr) NULL;
    bglTexCoordPointer = (bglTexCoordPointerProcPtr) NULL;
    bglDrawArrays = (bglDrawArraysProcPtr) NULL;
    bglDrawElements = (bglDrawElementsProcPtr) NULL;

    // Stencil Buffer
    bglClearStencil = (bglClearStencilProcPtr) NULL;
    bglStencilOp = (bglStencilOpProcPtr) NULL;
    bglStencilFunc = (bglStencilFuncProcPtr) NULL;
#endif

#if defined DYNAMIC_GLEXT
    bglBlendEquation = (bglBlendEquationProcPtr) NULL;

    bglTexImage3D = (bglTexImage3DProcPtr) NULL;
    bglCompressedTexImage2DARB = (bglCompressedTexImage2DARBProcPtr) NULL;
    bglGetCompressedTexImageARB = (bglGetCompressedTexImageARBProcPtr) NULL;

    // GPU Programs
    bglGenProgramsARB = (bglGenProgramsARBProcPtr) NULL;
    bglBindProgramARB = (bglBindProgramARBProcPtr) NULL;
    bglProgramStringARB = (bglProgramStringARBProcPtr) NULL;
    bglDeleteProgramsARB = (bglDeleteProgramsARBProcPtr) NULL;

    // Multitexturing
    bglActiveTextureARB = (bglActiveTextureARBProcPtr) NULL;
    bglClientActiveTextureARB = (bglClientActiveTextureARBProcPtr) NULL;
    bglMultiTexCoord2dARB = (bglMultiTexCoord2dARBProcPtr) NULL;
    bglMultiTexCoord2fARB = (bglMultiTexCoord2fARBProcPtr) NULL;

    // Frame Buffer Objects
    bglGenFramebuffersEXT = (bglGenFramebuffersEXTProcPtr) NULL;
    bglBindFramebufferEXT = (bglBindFramebufferEXTProcPtr) NULL;
    bglFramebufferTexture2DEXT = (bglFramebufferTexture2DEXTProcPtr) NULL;
    bglCheckFramebufferStatusEXT = (bglCheckFramebufferStatusEXTProcPtr) NULL;
    bglDeleteFramebuffersEXT = (bglDeleteFramebuffersEXTProcPtr) NULL;

    // Vertex Buffer Objects
    bglGenBuffersARB = (bglGenBuffersARBProcPtr) NULL;
    bglBindBufferARB = (bglBindBufferARBProcPtr) NULL;
    bglDeleteBuffersARB = (bglDeleteBuffersARBProcPtr) NULL;
    bglBufferDataARB = (bglBufferDataARBProcPtr) NULL;
    bglBufferSubDataARB = (bglBufferSubDataARBProcPtr) NULL;
    bglMapBufferARB = (bglMapBufferARBProcPtr) NULL;
    bglUnmapBufferARB = (bglUnmapBufferARBProcPtr) NULL;

    // ARB_buffer_storage
    bglBufferStorage = (bglBufferStorageProcPtr) NULL;

    // ARB_map_buffer_range
    bglMapBufferRange = (bglMapBufferRangeProcPtr)NULL;

    // Occlusion queries
    bglGenQueriesARB = (bglGenQueriesARBProcPtr) NULL;
    bglDeleteQueriesARB = (bglDeleteQueriesARBProcPtr) NULL;
    bglIsQueryARB = (bglIsQueryARBProcPtr) NULL;
    bglBeginQueryARB = (bglBeginQueryARBProcPtr) NULL;
    bglEndQueryARB = (bglEndQueryARBProcPtr) NULL;
    bglGetQueryivARB = (bglGetQueryivARBProcPtr) NULL;
    bglGetQueryObjectivARB = (bglGetQueryObjectivARBProcPtr) NULL;
    bglGetQueryObjectuivARB = (bglGetQueryObjectuivARBProcPtr) NULL;

    // Shader Objects
    bglDeleteObjectARB = (bglDeleteObjectARBProcPtr) NULL;
    bglGetHandleARB = (bglGetHandleARBProcPtr) NULL;
    bglDetachObjectARB = (bglDetachObjectARBProcPtr) NULL;
    bglCreateShaderObjectARB = (bglCreateShaderObjectARBProcPtr) NULL;
    bglShaderSourceARB = (bglShaderSourceARBProcPtr) NULL;
    bglCompileShaderARB = (bglCompileShaderARBProcPtr) NULL;
    bglCreateProgramObjectARB = (bglCreateProgramObjectARBProcPtr) NULL;
    bglAttachObjectARB = (bglAttachObjectARBProcPtr) NULL;
    bglLinkProgramARB = (bglLinkProgramARBProcPtr) NULL;
    bglUseProgramObjectARB = (bglUseProgramObjectARBProcPtr) NULL;
    bglValidateProgramARB = (bglValidateProgramARBProcPtr) NULL;
    bglUniform1fARB = (bglUniform1fARBProcPtr) NULL;
    bglUniform2fARB = (bglUniform2fARBProcPtr) NULL;
    bglUniform3fARB = (bglUniform3fARBProcPtr) NULL;
    bglUniform4fARB = (bglUniform4fARBProcPtr) NULL;
    bglUniform1iARB = (bglUniform1iARBProcPtr) NULL;
    bglUniform2iARB = (bglUniform2iARBProcPtr) NULL;
    bglUniform3iARB = (bglUniform3iARBProcPtr) NULL;
    bglUniform4iARB = (bglUniform4iARBProcPtr) NULL;
    bglUniform1fvARB = (bglUniform1fvARBProcPtr) NULL;
    bglUniform2fvARB = (bglUniform2fvARBProcPtr) NULL;
    bglUniform3fvARB = (bglUniform3fvARBProcPtr) NULL;
    bglUniform4fvARB = (bglUniform4fvARBProcPtr) NULL;
    bglUniform1ivARB = (bglUniform1ivARBProcPtr) NULL;
    bglUniform2ivARB = (bglUniform2ivARBProcPtr) NULL;
    bglUniform3ivARB = (bglUniform3ivARBProcPtr) NULL;
    bglUniform4ivARB = (bglUniform4ivARBProcPtr) NULL;
    bglUniformMatrix2fvARB = (bglUniformMatrix2fvARBProcPtr) NULL;
    bglUniformMatrix3fvARB = (bglUniformMatrix3fvARBProcPtr) NULL;
    bglUniformMatrix4fvARB = (bglUniformMatrix4fvARBProcPtr) NULL;
    bglGetObjectParameterfvARB = (bglGetObjectParameterfvARBProcPtr) NULL;
    bglGetObjectParameterivARB = (bglGetObjectParameterivARBProcPtr) NULL;
    bglGetInfoLogARB = (bglGetInfoLogARBProcPtr) NULL;
    bglGetAttachedObjectsARB = (bglGetAttachedObjectsARBProcPtr) NULL;
    bglGetUniformLocationARB = (bglGetUniformLocationARBProcPtr) NULL;
    bglGetActiveUniformARB = (bglGetActiveUniformARBProcPtr) NULL;
    bglGetUniformfvARB = (bglGetUniformfvARBProcPtr) NULL;
    bglGetUniformivARB = (bglGetUniformivARBProcPtr) NULL;
    bglGetShaderSourceARB = (bglGetShaderSourceARBProcPtr) NULL;

    // Vertex Shaders
    bglVertexAttrib1dARB = (bglVertexAttrib1dARBProcPtr) NULL;
    bglVertexAttrib1dvARB = (bglVertexAttrib1dvARBProcPtr) NULL;
    bglVertexAttrib1fARB = (bglVertexAttrib1fARBProcPtr) NULL;
    bglVertexAttrib1fvARB = (bglVertexAttrib1fvARBProcPtr) NULL;
    bglVertexAttrib1sARB = (bglVertexAttrib1sARBProcPtr) NULL;
    bglVertexAttrib1svARB = (bglVertexAttrib1svARBProcPtr) NULL;
    bglVertexAttrib2dARB = (bglVertexAttrib2dARBProcPtr) NULL;
    bglVertexAttrib2dvARB = (bglVertexAttrib2dvARBProcPtr) NULL;
    bglVertexAttrib2fARB = (bglVertexAttrib2fARBProcPtr) NULL;
    bglVertexAttrib2fvARB = (bglVertexAttrib2fvARBProcPtr) NULL;
    bglVertexAttrib2sARB = (bglVertexAttrib2sARBProcPtr) NULL;
    bglVertexAttrib2svARB = (bglVertexAttrib2svARBProcPtr) NULL;
    bglVertexAttrib3dARB = (bglVertexAttrib3dARBProcPtr) NULL;
    bglVertexAttrib3dvARB = (bglVertexAttrib3dvARBProcPtr) NULL;
    bglVertexAttrib3fARB = (bglVertexAttrib3fARBProcPtr) NULL;
    bglVertexAttrib3fvARB = (bglVertexAttrib3fvARBProcPtr) NULL;
    bglVertexAttrib3sARB = (bglVertexAttrib3sARBProcPtr) NULL;
    bglVertexAttrib3svARB = (bglVertexAttrib3svARBProcPtr) NULL;
    bglVertexAttrib4NbvARB = (bglVertexAttrib4NbvARBProcPtr) NULL;
    bglVertexAttrib4NivARB = (bglVertexAttrib4NivARBProcPtr) NULL;
    bglVertexAttrib4NsvARB = (bglVertexAttrib4NsvARBProcPtr) NULL;
    bglVertexAttrib4NubARB = (bglVertexAttrib4NubARBProcPtr) NULL;
    bglVertexAttrib4NubvARB = (bglVertexAttrib4NubvARBProcPtr) NULL;
    bglVertexAttrib4NuivARB = (bglVertexAttrib4NuivARBProcPtr) NULL;
    bglVertexAttrib4NusvARB = (bglVertexAttrib4NusvARBProcPtr) NULL;
    bglVertexAttrib4bvARB = (bglVertexAttrib4bvARBProcPtr) NULL;
    bglVertexAttrib4dARB = (bglVertexAttrib4dARBProcPtr) NULL;
    bglVertexAttrib4dvARB = (bglVertexAttrib4dvARBProcPtr) NULL;
    bglVertexAttrib4fARB = (bglVertexAttrib4fARBProcPtr) NULL;
    bglVertexAttrib4fvARB = (bglVertexAttrib4fvARBProcPtr) NULL;
    bglVertexAttrib4ivARB = (bglVertexAttrib4ivARBProcPtr) NULL;
    bglVertexAttrib4sARB = (bglVertexAttrib4sARBProcPtr) NULL;
    bglVertexAttrib4svARB = (bglVertexAttrib4svARBProcPtr) NULL;
    bglVertexAttrib4ubvARB = (bglVertexAttrib4ubvARBProcPtr) NULL;
    bglVertexAttrib4uivARB = (bglVertexAttrib4uivARBProcPtr) NULL;
    bglVertexAttrib4usvARB = (bglVertexAttrib4usvARBProcPtr) NULL;
    bglVertexAttribPointerARB = (bglVertexAttribPointerARBProcPtr) NULL;
    bglEnableVertexAttribArrayARB = (bglEnableVertexAttribArrayARBProcPtr) NULL;
    bglDisableVertexAttribArrayARB = (bglDisableVertexAttribArrayARBProcPtr) NULL;
    bglGetVertexAttribdvARB = (bglGetVertexAttribdvARBProcPtr) NULL;
    bglGetVertexAttribfvARB = (bglGetVertexAttribfvARBProcPtr) NULL;
    bglGetVertexAttribivARB = (bglGetVertexAttribivARBProcPtr) NULL;
    bglGetVertexAttribPointervARB = (bglGetVertexAttribPointervARBProcPtr) NULL;
    bglBindAttribLocationARB = (bglBindAttribLocationARBProcPtr) NULL;
    bglGetActiveAttribARB = (bglGetActiveAttribARBProcPtr) NULL;
    bglGetAttribLocationARB = (bglGetAttribLocationARBProcPtr) NULL;

    // Debug Output
#ifndef __APPLE__
    bglDebugMessageControlARB = (bglDebugMessageControlARBProcPtr) NULL;
    bglDebugMessageCallbackARB = (bglDebugMessageCallbackARBProcPtr) NULL;
#endif

#ifdef _WIN32
    bwglSwapIntervalEXT = (bwglSwapIntervalEXTProcPtr) NULL;
    bwglCreateContextAttribsARB = (bwglCreateContextAttribsARBProcPtr) NULL;
#endif
#endif

    return 0;
}

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
