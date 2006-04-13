#ifdef USE_OPENGL

#ifdef RENDERTYPEWIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(__APPLE__)
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif

// get this header from http://oss.sgi.com/projects/ogl-sample/registry/
// if you are missing it
//#include <GL/glext.h>
#if defined(__APPLE__)
# include <OpenGL/glext.h>
#else
# include "glext.h"
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#error You should get an updated copy of glext.h from http://oss.sgi.com/projects/ogl-sample/registry/
#endif

#ifdef DYNAMIC_OPENGL

#ifndef APIENTRY
# define APIENTRY
#endif

extern void (APIENTRY * bglClearColor)( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
extern void (APIENTRY * bglClear)( GLbitfield mask );
extern void (APIENTRY * bglColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
extern void (APIENTRY * bglAlphaFunc)( GLenum func, GLclampf ref );
extern void (APIENTRY * bglBlendFunc)( GLenum sfactor, GLenum dfactor );
extern void (APIENTRY * bglCullFace)( GLenum mode );
extern void (APIENTRY * bglFrontFace)( GLenum mode );
extern void (APIENTRY * bglPolygonOffset)( GLfloat factor, GLfloat units );
extern void (APIENTRY * bglPolygonMode)( GLenum face, GLenum mode );
extern void (APIENTRY * bglEnable)( GLenum cap );
extern void (APIENTRY * bglDisable)( GLenum cap );
extern void (APIENTRY * bglGetFloatv)( GLenum pname, GLfloat *params );
extern void (APIENTRY * bglGetIntegerv)( GLenum pname, GLint *params );
extern void (APIENTRY * bglPushAttrib)( GLbitfield mask );
extern void (APIENTRY * bglPopAttrib)( void );
extern GLenum (APIENTRY * bglGetError)( void );
extern const GLubyte* (APIENTRY * bglGetString)( GLenum name );
extern void (APIENTRY * bglHint)( GLenum target, GLenum mode );

// Depth
extern void (APIENTRY * bglDepthFunc)( GLenum func );
extern void (APIENTRY * bglDepthMask)( GLboolean flag );
extern void (APIENTRY * bglDepthRange)( GLclampd near_val, GLclampd far_val );

// Matrix
extern void (APIENTRY * bglMatrixMode)( GLenum mode );
extern void (APIENTRY * bglOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
extern void (APIENTRY * bglFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
extern void (APIENTRY * bglViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
extern void (APIENTRY * bglPushMatrix)( void );
extern void (APIENTRY * bglPopMatrix)( void );
extern void (APIENTRY * bglLoadIdentity)( void );
extern void (APIENTRY * bglLoadMatrixf)( const GLfloat *m );

// Drawing
extern void (APIENTRY * bglBegin)( GLenum mode );
extern void (APIENTRY * bglEnd)( void );
extern void (APIENTRY * bglVertex2f)( GLfloat x, GLfloat y );
extern void (APIENTRY * bglVertex2i)( GLint x, GLint y );
extern void (APIENTRY * bglVertex3d)( GLdouble x, GLdouble y, GLdouble z );
extern void (APIENTRY * bglVertex3fv)( const GLfloat *v );
extern void (APIENTRY * bglColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
extern void (APIENTRY * bglColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
extern void (APIENTRY * bglTexCoord2d)( GLdouble s, GLdouble t );
extern void (APIENTRY * bglTexCoord2f)( GLfloat s, GLfloat t );

// Lighting
extern void (APIENTRY * bglShadeModel)( GLenum mode );

// Raster funcs
extern void (APIENTRY * bglReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );

// Texture mapping
extern void (APIENTRY * bglTexEnvf)( GLenum target, GLenum pname, GLfloat param );
extern void (APIENTRY * bglGenTextures)( GLsizei n, GLuint *textures );	// 1.1
extern void (APIENTRY * bglDeleteTextures)( GLsizei n, const GLuint *textures);	// 1.1
extern void (APIENTRY * bglBindTexture)( GLenum target, GLuint texture );	// 1.1
extern void (APIENTRY * bglTexImage2D)( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
extern void (APIENTRY * bglTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );	// 1.1
extern void (APIENTRY * bglTexParameterf)( GLenum target, GLenum pname, GLfloat param );
extern void (APIENTRY * bglTexParameteri)( GLenum target, GLenum pname, GLint param );
extern void (APIENTRY * bglGetTexLevelParameteriv)( GLenum target, GLint level, GLenum pname, GLint *params );
extern void (APIENTRY * bglCompressedTexImage2DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
extern void (APIENTRY * bglGetCompressedTexImageARB)(GLenum, GLint, GLvoid *);

// Fog
extern void (APIENTRY * bglFogf)( GLenum pname, GLfloat param );
extern void (APIENTRY * bglFogi)( GLenum pname, GLint param );
extern void (APIENTRY * bglFogfv)( GLenum pname, const GLfloat *params );
			
#ifdef RENDERTYPEWIN
// Windows
extern HGLRC (WINAPI * bwglCreateContext)(HDC);
extern BOOL (WINAPI * bwglDeleteContext)(HGLRC);
extern PROC (WINAPI * bwglGetProcAddress)(LPCSTR);
extern BOOL (WINAPI * bwglMakeCurrent)(HDC,HGLRC);

extern BOOL (WINAPI * bwglSwapBuffers)(HDC);
extern int (WINAPI * bwglChoosePixelFormat)(HDC,CONST PIXELFORMATDESCRIPTOR*);
extern int (WINAPI * bwglDescribePixelFormat)(HDC,int,UINT,LPPIXELFORMATDESCRIPTOR);
extern int (WINAPI * bwglGetPixelFormat)(HDC);
extern BOOL (WINAPI * bwglSetPixelFormat)(HDC,int,const PIXELFORMATDESCRIPTOR*);
#endif

#else	// DYNAMIC_OPENGL

#define bglClearColor		glClearColor
#define bglClear		glClear
#define bglColorMask		glColorMask
#define bglAlphaFunc		glAlphaFunc
#define bglBlendFunc		glBlendFunc
#define bglCullFace		glCullFace
#define bglFrontFace		glFrontFace
#define bglPolygonOffset	glPolygonOffset
#define bglPolygonMode    glPolygonMode
#define bglEnable		glEnable
#define bglDisable		glDisable
#define bglGetFloatv		glGetFloatv
#define bglGetIntegerv		glGetIntegerv
#define bglPushAttrib		glPushAttrib
#define bglPopAttrib		glPopAttrib
#define bglGetError		glGetError
#define bglGetString		glGetString
#define bglHint			glHint

// Depth
#define bglDepthFunc		glDepthFunc
#define bglDepthMask		glDepthMask
#define bglDepthRange		glDepthRange

// Matrix
#define bglMatrixMode		glMatrixMode
#define bglOrtho		glOrtho
#define bglFrustum		glFrustum
#define bglViewport		glViewport
#define bglPushMatrix		glPushMatrix
#define bglPopMatrix		glPopMatrix
#define bglLoadIdentity		glLoadIdentity
#define bglLoadMatrixf		glLoadMatrixf

// Drawing
#define bglBegin		glBegin
#define bglEnd			glEnd
#define bglVertex2f		glVertex2f
#define bglVertex2i		glVertex2i
#define bglVertex3d		glVertex3d
#define bglVertex3fv		glVertex3fv
#define bglColor4f		glColor4f
#define bglColor4ub		glColor4ub
#define bglTexCoord2d		glTexCoord2d
#define bglTexCoord2f		glTexCoord2f

// Lighting
#define bglShadeModel		glShadeModel

// Raster funcs
#define bglReadPixels		glReadPixels

// Texture mapping
#define bglTexEnvf		glTexEnvf
#define bglGenTextures		glGenTextures
#define bglDeleteTextures	glDeleteTextures
#define bglBindTexture		glBindTexture
#define bglTexImage2D		glTexImage2D
#define bglTexSubImage2D	glTexSubImage2D
#define bglTexParameterf	glTexParameterf
#define bglTexParameteri	glTexParameteri
#define bglGetTexLevelParameteriv glGetTexLevelParameteriv
#define bglCompressedTexImage2DARB glCompressedTexImage2DARB
#define bglGetCompressedTexImageARB glGetCompressedTexImageARB

// Fog
#define bglFogf			glFogf
#define bglFogi			glFogi
#define bglFogfv		glFogfv
			
#ifdef RENDERTYPEWIN
#define bwglCreateContext	wglCreateContext
#define bwglDeleteContext	wglDeleteContext
#define bwglGetProcAddress	wglGetProcAddress
#define bwglMakeCurrent		wglMakeCurrent

#define bwglSwapBuffers		SwapBuffers
#define bwglChoosePixelFormat	ChoosePixelFormat
#define bwglDescribePixelFormat	DescribePixelFormat
#define bwglGetPixelFormat	GetPixelFormat
#define bwglSetPixelFormat	SetPixelFormat
#endif

#endif

#endif //USE_OPENGL

extern char *gldriver;

int loadgldriver(const char *driver);
int loadglextensions(void);
int unloadgldriver(void);

