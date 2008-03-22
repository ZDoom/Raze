#include "compat.h"
#include "glbuild.h"
#include "baselayer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined USE_OPENGL

#ifdef RENDERTYPESDL
# ifdef __APPLE__
#  include <SDL/SDL.h>
# else
#include "SDL.h"
#endif
#endif

void (APIENTRY * bglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY * bglClear)(GLbitfield mask);
void (APIENTRY * bglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void (APIENTRY * bglAlphaFunc)(GLenum func, GLclampf ref);
void (APIENTRY * bglBlendFunc)(GLenum sfactor, GLenum dfactor);
void (APIENTRY * bglCullFace)(GLenum mode);
void (APIENTRY * bglFrontFace)(GLenum mode);
void (APIENTRY * bglPolygonOffset)(GLfloat factor, GLfloat units);
void (APIENTRY * bglPolygonMode)(GLenum face, GLenum mode);
void (APIENTRY * bglEnable)(GLenum cap);
void (APIENTRY * bglDisable)(GLenum cap);
void (APIENTRY * bglGetDoublev)(GLenum pname, GLdouble *params);
void (APIENTRY * bglGetFloatv)(GLenum pname, GLfloat *params);
void (APIENTRY * bglGetIntegerv)(GLenum pname, GLint *params);
void (APIENTRY * bglPushAttrib)(GLbitfield mask);
void (APIENTRY * bglPopAttrib)(void);
GLenum(APIENTRY * bglGetError)(void);
const GLubyte*(APIENTRY * bglGetString)(GLenum name);
void (APIENTRY * bglHint)(GLenum target, GLenum mode);
void (APIENTRY * bglDrawBuffer)(GLenum mode);
void (APIENTRY * bglReadBuffer)(GLenum mode);
void (APIENTRY * bglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);

// Depth
void (APIENTRY * bglDepthFunc)(GLenum func);
void (APIENTRY * bglDepthMask)(GLboolean flag);
void (APIENTRY * bglDepthRange)(GLclampd near_val, GLclampd far_val);

// Matrix
void (APIENTRY * bglMatrixMode)(GLenum mode);
void (APIENTRY * bglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (APIENTRY * bglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (APIENTRY * bglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY * bglPushMatrix)(void);
void (APIENTRY * bglPopMatrix)(void);
void (APIENTRY * bglLoadIdentity)(void);
void (APIENTRY * bglLoadMatrixf)(const GLfloat *m);
void (APIENTRY * bglLoadMatrixd)(const GLdouble *m);
void (APIENTRY * bglMultMatrixf)(const GLfloat *m);
void (APIENTRY * bglMultMatrixd)(const GLdouble *m);
void (APIENTRY * bglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY * bglScalef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY * bglTranslatef)(GLfloat x, GLfloat y, GLfloat z);

// Drawing
void (APIENTRY * bglBegin)(GLenum mode);
void (APIENTRY * bglEnd)(void);
void (APIENTRY * bglVertex2f)(GLfloat x, GLfloat y);
void (APIENTRY * bglVertex2i)(GLint x, GLint y);
void (APIENTRY * bglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY * bglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY * bglVertex3fv)(const GLfloat *v);
void (APIENTRY * bglVertex3dv)(const GLdouble *v);
void (APIENTRY * bglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (APIENTRY * bglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void (APIENTRY * bglTexCoord2d)(GLdouble s, GLdouble t);
void (APIENTRY * bglTexCoord2f)(GLfloat s, GLfloat t);

// Lighting
void (APIENTRY * bglShadeModel)(GLenum mode);

// Raster funcs
void (APIENTRY * bglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);

// Texture mapping
void (APIENTRY * bglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
void (APIENTRY * bglGenTextures)(GLsizei n, GLuint *textures);	// 1.1
void (APIENTRY * bglDeleteTextures)(GLsizei n, const GLuint *textures);	// 1.1
void (APIENTRY * bglBindTexture)(GLenum target, GLuint texture);	// 1.1
void (APIENTRY * bglTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY * bglCopyTexImage2D)(GLenum	target, GLint level, GLenum	internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void (APIENTRY * bglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY * bglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);	// 1.1
void (APIENTRY * bglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
void (APIENTRY * bglTexParameteri)(GLenum target, GLenum pname, GLint param);
void (APIENTRY * bglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
void (APIENTRY * bglCompressedTexImage2DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
void (APIENTRY * bglGetCompressedTexImageARB)(GLenum, GLint, GLvoid *);
void (APIENTRY * bglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);

// Fog
void (APIENTRY * bglFogf)(GLenum pname, GLfloat param);
void (APIENTRY * bglFogi)(GLenum pname, GLint param);
void (APIENTRY * bglFogfv)(GLenum pname, const GLfloat *params);

// Display Lists
void (APIENTRY * bglNewList)(GLuint list, GLenum mode);
void (APIENTRY * bglEndList)(void);
void (APIENTRY * bglCallList)(GLuint list);
void (APIENTRY * bglDeleteLists)(GLuint list, GLsizei range);

// Vertex Arrays
void (APIENTRY * bglEnableClientState)(GLenum cap);
void (APIENTRY * bglDisableClientState)(GLenum cap);
void (APIENTRY * bglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void (APIENTRY * bglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void (APIENTRY * bglDrawArrays)(GLenum mode, GLint first, GLsizei count);
void (APIENTRY * bglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

// Stencil Buffer
void (APIENTRY * bglClearStencil)(GLint s);
void (APIENTRY * bglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
void (APIENTRY * bglStencilFunc)(GLenum func, GLint ref, GLuint mask);

// GPU Programs
void (APIENTRY * bglGenProgramsARB)(GLsizei, GLuint *);
void (APIENTRY * bglBindProgramARB)(GLenum, GLuint);
void (APIENTRY * bglProgramStringARB)(GLenum, GLenum, GLsizei, const GLvoid *);
void (APIENTRY * bglDeleteProgramsARB)(GLsizei n, const GLuint *programs);

// Multitexturing
void (APIENTRY * bglActiveTextureARB)(GLenum texture);
void (APIENTRY * bglClientActiveTextureARB)(GLenum texture);
void (APIENTRY * bglMultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t);
void (APIENTRY * bglMultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);

// Frame Buffer Objects
void (APIENTRY * bglGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
void (APIENTRY * bglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
void (APIENTRY * bglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLenum(APIENTRY * bglCheckFramebufferStatusEXT)(GLenum target);
void (APIENTRY * bglDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);

// Vertex Buffer Objects
void (APIENTRY * bglGenBuffersARB)(GLsizei n, GLuint * buffers);
void (APIENTRY * bglBindBufferARB)(GLenum target, GLuint buffer);
void (APIENTRY * bglDeleteBuffersARB)(GLsizei n, const GLuint * buffers);
void (APIENTRY * bglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage);
void* (APIENTRY * bglMapBufferARB)(GLenum target, GLenum access);
GLboolean(APIENTRY * bglUnmapBufferARB)(GLenum target);

// GLU
void (APIENTRY * bgluTessBeginContour)(GLUtesselator* tess);
void (APIENTRY * bgluTessBeginPolygon)(GLUtesselator* tess, GLvoid* data);
void (APIENTRY * bgluTessCallback)(GLUtesselator* tess, GLenum which, void (PR_CALLBACK CallBackFunc)());
void (APIENTRY * bgluTessEndContour)(GLUtesselator* tess);
void (APIENTRY * bgluTessEndPolygon)(GLUtesselator* tess);
void (APIENTRY * bgluTessNormal)(GLUtesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
void (APIENTRY * bgluTessProperty)(GLUtesselator* tess, GLenum which, GLdouble data);
void (APIENTRY * bgluTessVertex)(GLUtesselator* tess, GLdouble *location, GLvoid* data);
GLUtesselator*(APIENTRY * bgluNewTess)(void);

void (APIENTRY * bgluPerspective)(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

const GLubyte * (APIENTRY * bgluErrorString)(GLenum error);

GLint(APIENTRY * bgluProject)(GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint	*view, GLdouble* winX, GLdouble* winY, GLdouble* winZ);

#ifdef RENDERTYPEWIN
// Windows
HGLRC(WINAPI * bwglCreateContext)(HDC);
BOOL (WINAPI * bwglDeleteContext)(HGLRC);
PROC(WINAPI * bwglGetProcAddress)(LPCSTR);
BOOL (WINAPI * bwglMakeCurrent)(HDC,HGLRC);

BOOL (WINAPI * bwglSwapBuffers)(HDC);
int (WINAPI * bwglChoosePixelFormat)(HDC,CONST PIXELFORMATDESCRIPTOR*);
int (WINAPI * bwglDescribePixelFormat)(HDC,int,UINT,LPPIXELFORMATDESCRIPTOR);
int (WINAPI * bwglGetPixelFormat)(HDC);
BOOL (WINAPI * bwglSetPixelFormat)(HDC,int,const PIXELFORMATDESCRIPTOR*);

static HANDLE hGLDLL, hGLUDLL;
#else
#include <dlfcn.h>

static void *gluhandle = NULL;
#endif

char *gldriver = NULL, *glulibrary = NULL;

static void * getproc_(const char *s, int *err, int fatal, int extension)
{
    void *t;
#if defined RENDERTYPESDL
    t = (void*)SDL_GL_GetProcAddress(s);
#elif defined _WIN32
    if (extension) t = (void*)bwglGetProcAddress(s);
    else t = (void*)GetProcAddress(hGLDLL,s);
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

int loadgldriver(const char *driver)
{
    int err=0;

#ifdef RENDERTYPEWIN
    if (hGLDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "OPENGL32.DLL";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
#else
        driver = "libGL.so";
#endif
    }

    initprintf("Loading %s\n",driver);

#if defined RENDERTYPESDL
    if (SDL_GL_LoadLibrary(driver)) return -1;
#elif defined _WIN32
    hGLDLL = LoadLibrary(driver);
    if (!hGLDLL) return -1;
#endif
    gldriver = strdup(driver);

#ifdef RENDERTYPEWIN
    bwglCreateContext	= GETPROC("wglCreateContext");
    bwglDeleteContext	= GETPROC("wglDeleteContext");
    bwglGetProcAddress	= GETPROC("wglGetProcAddress");
    bwglMakeCurrent		= GETPROC("wglMakeCurrent");

    bwglSwapBuffers		= GETPROC("wglSwapBuffers");
    bwglChoosePixelFormat	= GETPROC("wglChoosePixelFormat");
    bwglDescribePixelFormat	= GETPROC("wglDescribePixelFormat");
    bwglGetPixelFormat	= GETPROC("wglGetPixelFormat");
    bwglSetPixelFormat	= GETPROC("wglSetPixelFormat");
#endif

    bglClearColor		= GETPROC("glClearColor");
    bglClear		    = GETPROC("glClear");
    bglColorMask		= GETPROC("glColorMask");
    bglAlphaFunc		= GETPROC("glAlphaFunc");
    bglBlendFunc		= GETPROC("glBlendFunc");
    bglCullFace		    = GETPROC("glCullFace");
    bglFrontFace		= GETPROC("glFrontFace");
    bglPolygonOffset	= GETPROC("glPolygonOffset");
    bglPolygonMode		= GETPROC("glPolygonMode");
    bglEnable		    = GETPROC("glEnable");
    bglDisable		    = GETPROC("glDisable");
    bglGetDoublev		= GETPROC("glGetDoublev");
    bglGetFloatv		= GETPROC("glGetFloatv");
    bglGetIntegerv		= GETPROC("glGetIntegerv");
    bglPushAttrib		= GETPROC("glPushAttrib");
    bglPopAttrib		= GETPROC("glPopAttrib");
    bglGetError		    = GETPROC("glGetError");
    bglGetString		= GETPROC("glGetString");
    bglHint			    = GETPROC("glHint");
    bglDrawBuffer       = GETPROC("glDrawBuffer");
    bglReadBuffer       = GETPROC("glDrawBuffer");
    bglScissor          = GETPROC("glScissor");

    // Depth
    bglDepthFunc		= GETPROC("glDepthFunc");
    bglDepthMask		= GETPROC("glDepthMask");
    bglDepthRange		= GETPROC("glDepthRange");

    // Matrix
    bglMatrixMode		= GETPROC("glMatrixMode");
    bglOrtho		= GETPROC("glOrtho");
    bglFrustum		= GETPROC("glFrustum");
    bglViewport		= GETPROC("glViewport");
    bglPushMatrix		= GETPROC("glPushMatrix");
    bglPopMatrix		= GETPROC("glPopMatrix");
    bglLoadIdentity		= GETPROC("glLoadIdentity");
    bglLoadMatrixf		= GETPROC("glLoadMatrixf");
    bglLoadMatrixd		= GETPROC("glLoadMatrixd");
    bglMultMatrixf		= GETPROC("glMultMatrixf");
    bglMultMatrixd		= GETPROC("glMultMatrixd");
    bglRotatef          = GETPROC("glRotatef");
    bglScalef          = GETPROC("glScalef");
    bglTranslatef          = GETPROC("glTranslatef");

    // Drawing
    bglBegin		=       GETPROC("glBegin");
    bglEnd			=       GETPROC("glEnd");
    bglVertex2f		=       GETPROC("glVertex2f");
    bglVertex2i		=       GETPROC("glVertex2i");
    bglVertex3f		=       GETPROC("glVertex3f");
    bglVertex3d		=       GETPROC("glVertex3d");
    bglVertex3fv		=   GETPROC("glVertex3fv");
    bglVertex3dv		=   GETPROC("glVertex3dv");
    bglColor4f		=       GETPROC("glColor4f");
    bglColor4ub		=       GETPROC("glColor4ub");
    bglTexCoord2d		=   GETPROC("glTexCoord2d");
    bglTexCoord2f		=   GETPROC("glTexCoord2f");

    // Lighting
    bglShadeModel		= GETPROC("glShadeModel");

    // Raster funcs
    bglReadPixels		= GETPROC("glReadPixels");

    // Texture mapping
    bglTexEnvf		=       GETPROC("glTexEnvf");
    bglGenTextures		= GETPROC("glGenTextures");
    bglDeleteTextures	= GETPROC("glDeleteTextures");
    bglBindTexture		= GETPROC("glBindTexture");
    bglTexImage2D		= GETPROC("glTexImage2D");
    bglCopyTexImage2D	= GETPROC("glCopyTexImage2D");
    bglCopyTexSubImage2D= GETPROC("glCopyTexSubImage2D");
    bglTexSubImage2D	= GETPROC("glTexSubImage2D");
    bglTexParameterf	= GETPROC("glTexParameterf");
    bglTexParameteri	= GETPROC("glTexParameteri");
    bglGetTexLevelParameteriv = GETPROC("glGetTexLevelParameteriv");
    bglTexGenfv         = GETPROC("glTexGenfv");

    // Fog
    bglFogf			= GETPROC("glFogf");
    bglFogi			= GETPROC("glFogi");
    bglFogfv		= GETPROC("glFogfv");

    // Display Lists
    bglNewList      =       GETPROC("glNewList");
    bglEndList      =       GETPROC("glEndList");
    bglCallList      =      GETPROC("glCallList");
    bglDeleteLists      =   GETPROC("glDeleteLists");

    // Vertex Arrays
    bglEnableClientState    = GETPROC("glEnableClientState");
    bglDisableClientState   = GETPROC("glDisableClientState");
    bglVertexPointer        = GETPROC("glVertexPointer");
    bglTexCoordPointer      = GETPROC("glTexCoordPointer");
    bglDrawArrays           = GETPROC("glDrawArrays");
    bglDrawElements         = GETPROC("glDrawElements");

    // Stencil Buffer
    bglClearStencil = GETPROC("glClearStencil");
    bglStencilOp    = GETPROC("glStencilOp");
    bglStencilFunc  = GETPROC("glStencilFunc");

    loadglextensions();
    loadglulibrary(getenv("BUILD_GLULIB"));

    if (err) unloadgldriver();
    return err;
}

int loadglextensions(void)
{
    int err = 0;
#ifdef RENDERTYPEWIN
    if (!hGLDLL) return 0;
#endif

    bglCompressedTexImage2DARB  = GETPROCEXTSOFT("glCompressedTexImage2DARB");
    bglGetCompressedTexImageARB = GETPROCEXTSOFT("glGetCompressedTexImageARB");

    // GPU Programs
    bglGenProgramsARB   = GETPROCEXTSOFT("glGenProgramsARB");
    bglBindProgramARB   = GETPROCEXTSOFT("glBindProgramARB");
    bglProgramStringARB = GETPROCEXTSOFT("glProgramStringARB");
    bglDeleteProgramsARB= GETPROCEXTSOFT("glDeleteProgramsARB");

    // Multitexturing
    bglActiveTextureARB         = GETPROCEXTSOFT("glActiveTextureARB");
    bglClientActiveTextureARB   = GETPROCEXTSOFT("glClientActiveTextureARB");
    bglMultiTexCoord2dARB       = GETPROCEXTSOFT("glMultiTexCoord2dARB");
    bglMultiTexCoord2fARB       = GETPROCEXTSOFT("glMultiTexCoord2fARB");

    // Frame Buffer Objects
    bglGenFramebuffersEXT =         GETPROCEXTSOFT("glGenFramebuffersEXT");
    bglBindFramebufferEXT =         GETPROCEXTSOFT("glBindFramebufferEXT");
    bglFramebufferTexture2DEXT =    GETPROCEXTSOFT("glFramebufferTexture2DEXT");
    bglCheckFramebufferStatusEXT =  GETPROCEXTSOFT("glCheckFramebufferStatusEXT");
    bglDeleteFramebuffersEXT =      GETPROCEXTSOFT("glDeleteFramebuffersEXT");

    // Vertex Buffer Objects
    bglGenBuffersARB    = GETPROCEXTSOFT("glGenBuffersARB");
    bglBindBufferARB    = GETPROCEXTSOFT("glBindBufferARB");
    bglDeleteBuffersARB = GETPROCEXTSOFT("glDeleteBuffersARB");
    bglBufferDataARB    = GETPROCEXTSOFT("glBufferDataARB");
    bglMapBufferARB     = GETPROCEXTSOFT("glMapBufferARB");
    bglUnmapBufferARB   = GETPROCEXTSOFT("glUnmapBufferARB");

    return err;
}

int unloadgldriver(void)
{
    unloadglulibrary();

#ifdef RENDERTYPEWIN
    if (!hGLDLL) return 0;
#endif

    free(gldriver);
    gldriver = NULL;

#ifdef RENDERTYPEWIN
    FreeLibrary(hGLDLL);
    hGLDLL = NULL;
#endif

    bglClearColor		= NULL;
    bglClear		    = NULL;
    bglColorMask		= NULL;
    bglAlphaFunc		= NULL;
    bglBlendFunc		= NULL;
    bglCullFace		    = NULL;
    bglFrontFace		= NULL;
    bglPolygonOffset	= NULL;
    bglPolygonMode      = NULL;
    bglEnable		    = NULL;
    bglDisable		    = NULL;
    bglGetDoublev		= NULL;
    bglGetFloatv		= NULL;
    bglGetIntegerv		= NULL;
    bglPushAttrib		= NULL;
    bglPopAttrib		= NULL;
    bglGetError		    = NULL;
    bglGetString		= NULL;
    bglHint			    = NULL;
    bglDrawBuffer       = NULL;
    bglReadBuffer       = NULL;
    bglScissor          = NULL;

    // Depth
    bglDepthFunc		= NULL;
    bglDepthMask		= NULL;
    bglDepthRange		= NULL;

    // Matrix
    bglMatrixMode		= NULL;
    bglOrtho		    = NULL;
    bglFrustum		    = NULL;
    bglViewport		    = NULL;
    bglPushMatrix		= NULL;
    bglPopMatrix		= NULL;
    bglLoadIdentity		= NULL;
    bglLoadMatrixf		= NULL;
    bglLoadMatrixd		= NULL;
    bglMultMatrixf      = NULL;
    bglMultMatrixd      = NULL;
    bglRotatef          = NULL;
    bglScalef           = NULL;
    bglTranslatef       = NULL;

    // Drawing
    bglBegin		= NULL;
    bglEnd			= NULL;
    bglVertex2f		= NULL;
    bglVertex2i		= NULL;
    bglVertex3f		= NULL;
    bglVertex3d		= NULL;
    bglVertex3fv    = NULL;
    bglColor4f		= NULL;
    bglColor4ub		= NULL;
    bglTexCoord2d	= NULL;
    bglTexCoord2f	= NULL;

    // Lighting
    bglShadeModel		= NULL;

    // Raster funcs
    bglReadPixels		= NULL;

    // Texture mapping
    bglTexEnvf		    = NULL;
    bglGenTextures		= NULL;
    bglDeleteTextures	= NULL;
    bglBindTexture		= NULL;
    bglTexImage2D		= NULL;
    bglCopyTexImage2D	= NULL;
    bglCopyTexSubImage2D= NULL;
    bglTexSubImage2D	= NULL;
    bglTexParameterf	= NULL;
    bglTexParameteri	= NULL;
    bglGetTexLevelParameteriv   = NULL;
    bglCompressedTexImage2DARB  = NULL;
    bglGetCompressedTexImageARB = NULL;

    // Fog
    bglFogf			= NULL;
    bglFogi			= NULL;
    bglFogfv		= NULL;

    // Display Lists
    bglNewList          = NULL;
    bglEndList          = NULL;
    bglCallList         = NULL;
    bglDeleteLists       = NULL;

    // Vertex Arrays
    bglEnableClientState    = NULL;
    bglDisableClientState   = NULL;
    bglVertexPointer        = NULL;
    bglTexCoordPointer      = NULL;
    bglDrawElements         = NULL;

    // Stencil Buffer
    bglClearStencil = NULL;
    bglStencilOp    = NULL;
    bglStencilFunc  = NULL;

    // GPU Programs
    bglGenProgramsARB   = NULL;
    bglBindProgramARB   = NULL;
    bglProgramStringARB = NULL;
    bglDeleteProgramsARB= NULL;

    // Multitexturing
    bglActiveTextureARB         = NULL;
    bglClientActiveTextureARB   = NULL;
    bglMultiTexCoord2dARB       = NULL;
    bglMultiTexCoord2fARB       = NULL;

    // Frame Buffer Objects
    bglGenFramebuffersEXT = NULL;
    bglBindFramebufferEXT = NULL;
    bglFramebufferTexture2DEXT = NULL;
    bglCheckFramebufferStatusEXT = NULL;
    bglDeleteFramebuffersEXT = NULL;

    // Vertex Buffer Objects
    bglGenBuffersARB    = NULL;
    bglBindBufferARB    = NULL;
    bglDeleteBuffersARB = NULL;
    bglBufferDataARB    = NULL;
    bglMapBufferARB     = NULL;
    bglUnmapBufferARB   = NULL;

#ifdef RENDERTYPEWIN
    bwglCreateContext	= NULL;
    bwglDeleteContext	= NULL;
    bwglGetProcAddress	= NULL;
    bwglMakeCurrent		= NULL;

    bwglSwapBuffers		= NULL;
    bwglChoosePixelFormat	= NULL;
    bwglDescribePixelFormat	= NULL;
    bwglGetPixelFormat	= NULL;
    bwglSetPixelFormat	= NULL;
#endif

    return 0;
}

static void * glugetproc_(const char *s, int *err, int fatal)
{
    void *t;
#if defined _WIN32
    t = (void*)GetProcAddress(hGLUDLL,s);
#else
    t = (void*)dlsym(gluhandle,s);
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

int loadglulibrary(const char *driver)
{
    int err=0;

#ifdef RENDERTYPEWIN
    if (hGLUDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "GLU32.DLL";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL"; // FIXME: like I know anything about Apple.  Hah.
#else
        driver = "libGLU.so";
#endif
    }

    initprintf("Loading %s\n",driver);

#if defined _WIN32
    hGLUDLL = LoadLibrary(driver);
    if (!hGLUDLL) return -1;
#else
    gluhandle = dlopen(driver, RTLD_NOW|RTLD_GLOBAL);
    if (!gluhandle) return -1;
#endif
    glulibrary = strdup(driver);

    bgluTessBeginContour = GLUGETPROC("gluTessBeginContour");
    bgluTessBeginPolygon = GLUGETPROC("gluTessBeginPolygon");
    bgluTessCallback     = GLUGETPROC("gluTessCallback");
    bgluTessEndContour = GLUGETPROC("gluTessEndContour");
    bgluTessEndPolygon = GLUGETPROC("gluTessEndPolygon");
    bgluTessNormal = GLUGETPROC("gluTessNormal");
    bgluTessProperty = GLUGETPROC("gluTessProperty");
    bgluTessVertex = GLUGETPROC("gluTessVertex");
    bgluNewTess = GLUGETPROC("gluNewTess");

    bgluPerspective = GLUGETPROC("gluPerspective");

    bgluErrorString = GLUGETPROC("gluErrorString");

    bgluProject = GLUGETPROC("gluProject");

    if (err) unloadglulibrary();
    return err;
}

int unloadglulibrary(void)
{
#ifdef RENDERTYPEWIN
    if (!hGLUDLL) return 0;
#endif

    free(glulibrary);
    glulibrary = NULL;

#ifdef RENDERTYPEWIN
    FreeLibrary(hGLUDLL);
    hGLUDLL = NULL;
#else
    if (gluhandle) dlclose(gluhandle);
    gluhandle = NULL;
#endif

    bgluTessBeginContour    = NULL;
    bgluTessBeginPolygon    = NULL;
    bgluTessCallback        = NULL;
    bgluTessEndContour      = NULL;
    bgluTessEndPolygon      = NULL;
    bgluTessNormal          = NULL;
    bgluTessProperty        = NULL;
    bgluTessVertex          = NULL;
    bgluNewTess             = NULL;

    bgluPerspective         = NULL;

    bgluErrorString         = NULL;

    bgluProject             = NULL;

    return 0;
}
#endif

