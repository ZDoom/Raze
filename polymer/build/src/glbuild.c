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

void (APIENTRY * bglClearColor)( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
void (APIENTRY * bglClear)( GLbitfield mask );
void (APIENTRY * bglColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
void (APIENTRY * bglAlphaFunc)( GLenum func, GLclampf ref );
void (APIENTRY * bglBlendFunc)( GLenum sfactor, GLenum dfactor );
void (APIENTRY * bglCullFace)( GLenum mode );
void (APIENTRY * bglFrontFace)( GLenum mode );
void (APIENTRY * bglPolygonOffset)( GLfloat factor, GLfloat units );
void (APIENTRY * bglPolygonMode)( GLenum face, GLenum mode );
void (APIENTRY * bglEnable)( GLenum cap );
void (APIENTRY * bglDisable)( GLenum cap );
void (APIENTRY * bglGetFloatv)( GLenum pname, GLfloat *params );
void (APIENTRY * bglGetIntegerv)( GLenum pname, GLint *params );
void (APIENTRY * bglPushAttrib)( GLbitfield mask );
void (APIENTRY * bglPopAttrib)( void );
GLenum (APIENTRY * bglGetError)( void );
const GLubyte* (APIENTRY * bglGetString)( GLenum name );
void (APIENTRY * bglHint)( GLenum target, GLenum mode );

// Depth
void (APIENTRY * bglDepthFunc)( GLenum func );
void (APIENTRY * bglDepthMask)( GLboolean flag );
void (APIENTRY * bglDepthRange)( GLclampd near_val, GLclampd far_val );

// Matrix
void (APIENTRY * bglMatrixMode)( GLenum mode );
void (APIENTRY * bglOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
void (APIENTRY * bglFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
void (APIENTRY * bglViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
void (APIENTRY * bglPushMatrix)( void );
void (APIENTRY * bglPopMatrix)( void );
void (APIENTRY * bglLoadIdentity)( void );
void (APIENTRY * bglLoadMatrixf)( const GLfloat *m );
void (APIENTRY * bglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY * bglScalef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY * bglTranslatef)(GLfloat x, GLfloat y, GLfloat z);

// Drawing
void (APIENTRY * bglBegin)( GLenum mode );
void (APIENTRY * bglEnd)( void );
void (APIENTRY * bglVertex2f)( GLfloat x, GLfloat y );
void (APIENTRY * bglVertex2i)( GLint x, GLint y );
void (APIENTRY * bglVertex3d)( GLdouble x, GLdouble y, GLdouble z );
void (APIENTRY * bglVertex3fv)( const GLfloat *v );
void (APIENTRY * bglVertex3dv)( const GLdouble *v );
void (APIENTRY * bglColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void (APIENTRY * bglColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
void (APIENTRY * bglTexCoord2d)( GLdouble s, GLdouble t );
void (APIENTRY * bglTexCoord2f)( GLfloat s, GLfloat t );

// Lighting
void (APIENTRY * bglShadeModel)( GLenum mode );

// Raster funcs
void (APIENTRY * bglReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );

// Texture mapping
void (APIENTRY * bglTexEnvf)( GLenum target, GLenum pname, GLfloat param );
void (APIENTRY * bglGenTextures)( GLsizei n, GLuint *textures );	// 1.1
void (APIENTRY * bglDeleteTextures)( GLsizei n, const GLuint *textures);	// 1.1
void (APIENTRY * bglBindTexture)( GLenum target, GLuint texture );	// 1.1
void (APIENTRY * bglTexImage2D)( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void (APIENTRY * bglTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );	// 1.1
void (APIENTRY * bglTexParameterf)( GLenum target, GLenum pname, GLfloat param );
void (APIENTRY * bglTexParameteri)( GLenum target, GLenum pname, GLint param );
void (APIENTRY * bglGetTexLevelParameteriv)( GLenum target, GLint level, GLenum pname, GLint *params );
void (APIENTRY * bglCompressedTexImage2DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
void (APIENTRY * bglGetCompressedTexImageARB)(GLenum, GLint, GLvoid *);
void (APIENTRY * bglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);

// Fog
void (APIENTRY * bglFogf)( GLenum pname, GLfloat param );
void (APIENTRY * bglFogi)( GLenum pname, GLint param );
void (APIENTRY * bglFogfv)( GLenum pname, const GLfloat *params );

// Display Lists
void (APIENTRY * bglNewList)(GLuint list, GLenum mode);
void (APIENTRY * bglEndList)(void);
void (APIENTRY * bglCallList)(GLuint list);

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

#ifdef RENDERTYPEWIN
// Windows
HGLRC (WINAPI * bwglCreateContext)(HDC);
BOOL (WINAPI * bwglDeleteContext)(HGLRC);
PROC (WINAPI * bwglGetProcAddress)(LPCSTR);
BOOL (WINAPI * bwglMakeCurrent)(HDC,HGLRC);

BOOL (WINAPI * bwglSwapBuffers)(HDC);
int (WINAPI * bwglChoosePixelFormat)(HDC,CONST PIXELFORMATDESCRIPTOR*);
int (WINAPI * bwglDescribePixelFormat)(HDC,int,UINT,LPPIXELFORMATDESCRIPTOR);
int (WINAPI * bwglGetPixelFormat)(HDC);
BOOL (WINAPI * bwglSetPixelFormat)(HDC,int,const PIXELFORMATDESCRIPTOR*);

static HANDLE hGLDLL;
#endif


char *gldriver = NULL;

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
    if (!t && fatal) {
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
    void *t;
    int err=0;

#ifdef RENDERTYPEWIN
    if (hGLDLL) return 0;
#endif

    if (!driver) {
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
    bglClear		= GETPROC("glClear");
    bglColorMask		= GETPROC("glColorMask");
    bglAlphaFunc		= GETPROC("glAlphaFunc");
    bglBlendFunc		= GETPROC("glBlendFunc");
    bglCullFace		= GETPROC("glCullFace");
    bglFrontFace		= GETPROC("glFrontFace");
    bglPolygonOffset	= GETPROC("glPolygonOffset");
    bglPolygonMode		= GETPROC("glPolygonMode");
    bglEnable		= GETPROC("glEnable");
    bglDisable		= GETPROC("glDisable");
    bglGetFloatv		= GETPROC("glGetFloatv");
    bglGetIntegerv		= GETPROC("glGetIntegerv");
    bglPushAttrib		= GETPROC("glPushAttrib");
    bglPopAttrib		= GETPROC("glPopAttrib");
    bglGetError		= GETPROC("glGetError");
    bglGetString		= GETPROC("glGetString");
    bglHint			= GETPROC("glHint");

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
    bglRotatef          = GETPROC("glRotatef");
    bglScalef          = GETPROC("glScalef");
    bglTranslatef          = GETPROC("glTranslatef");

    // Drawing
    bglBegin		= GETPROC("glBegin");
    bglEnd			= GETPROC("glEnd");
    bglVertex2f		= GETPROC("glVertex2f");
    bglVertex2i		= GETPROC("glVertex2i");
    bglVertex3d		= GETPROC("glVertex3d");
    bglVertex3fv		= GETPROC("glVertex3fv");
    bglVertex3dv		= GETPROC("glVertex3dv");
    bglColor4f		= GETPROC("glColor4f");
    bglColor4ub		= GETPROC("glColor4ub");
    bglTexCoord2d		= GETPROC("glTexCoord2d");
    bglTexCoord2f		= GETPROC("glTexCoord2f");

    // Lighting
    bglShadeModel		= GETPROC("glShadeModel");

    // Raster funcs
    bglReadPixels		= GETPROC("glReadPixels");

    // Texture mapping
    bglTexEnvf		= GETPROC("glTexEnvf");
    bglGenTextures		= GETPROC("glGenTextures");
    bglDeleteTextures	= GETPROC("glDeleteTextures");
    bglBindTexture		= GETPROC("glBindTexture");
    bglTexImage2D		= GETPROC("glTexImage2D");
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
    bglNewList      = GETPROC("glNewList");
    bglEndList      = GETPROC("glEndList");
    bglCallList      = GETPROC("glCallList");

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

    return err;
}

int unloadgldriver(void)
{
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
    bglClear		= NULL;
    bglColorMask		= NULL;
    bglAlphaFunc		= NULL;
    bglBlendFunc		= NULL;
    bglCullFace		= NULL;
    bglFrontFace		= NULL;
    bglPolygonOffset	= NULL;
    bglPolygonMode   = NULL;
    bglEnable		= NULL;
    bglDisable		= NULL;
    bglGetFloatv		= NULL;
    bglGetIntegerv		= NULL;
    bglPushAttrib		= NULL;
    bglPopAttrib		= NULL;
    bglGetError		= NULL;
    bglGetString		= NULL;
    bglHint			= NULL;

    // Depth
    bglDepthFunc		= NULL;
    bglDepthMask		= NULL;
    bglDepthRange		= NULL;

    // Matrix
    bglMatrixMode		= NULL;
    bglOrtho		= NULL;
    bglFrustum		= NULL;
    bglViewport		= NULL;
    bglPushMatrix		= NULL;
    bglPopMatrix		= NULL;
    bglLoadIdentity		= NULL;
    bglLoadMatrixf		= NULL;

    // Drawing
    bglBegin		= NULL;
    bglEnd			= NULL;
    bglVertex2f		= NULL;
    bglVertex2i		= NULL;
    bglVertex3d		= NULL;
    bglVertex3fv		= NULL;
    bglColor4f		= NULL;
    bglColor4ub		= NULL;
    bglTexCoord2d		= NULL;
    bglTexCoord2f		= NULL;

    // Lighting
    bglShadeModel		= NULL;

    // Raster funcs
    bglReadPixels		= NULL;

    // Texture mapping
    bglTexEnvf		= NULL;
    bglGenTextures		= NULL;
    bglDeleteTextures	= NULL;
    bglBindTexture		= NULL;
    bglTexImage2D		= NULL;
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
    bglNewList      = NULL;
    bglEndList      = NULL;
    bglCallList      = NULL;

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

#endif

