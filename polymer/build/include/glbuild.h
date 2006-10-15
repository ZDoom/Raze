#ifdef USE_OPENGL

#ifdef RENDERTYPEWIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(__APPLE__)
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
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
extern void (APIENTRY * bglMultMatrixf)( const GLfloat *m );
extern void (APIENTRY * bglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY * bglScalef)(GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY * bglTranslatef)(GLfloat x, GLfloat y, GLfloat z);

// Drawing
extern void (APIENTRY * bglBegin)( GLenum mode );
extern void (APIENTRY * bglEnd)( void );
extern void (APIENTRY * bglVertex2f)( GLfloat x, GLfloat y );
extern void (APIENTRY * bglVertex2i)( GLint x, GLint y );
extern void (APIENTRY * bglVertex3d)( GLdouble x, GLdouble y, GLdouble z );
extern void (APIENTRY * bglVertex3fv)( const GLfloat *v );
extern void (APIENTRY * bglVertex3dv)( const GLdouble *v );
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
extern void (APIENTRY * bglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);

// Fog
extern void (APIENTRY * bglFogf)( GLenum pname, GLfloat param );
extern void (APIENTRY * bglFogi)( GLenum pname, GLint param );
extern void (APIENTRY * bglFogfv)( GLenum pname, const GLfloat *params );

// Display Lists
extern void (APIENTRY * bglNewList)(GLuint list, GLenum mode);
extern void (APIENTRY * bglEndList)(void);
extern void (APIENTRY * bglCallList)(GLuint list);

// Vertex Arrays
extern void (APIENTRY * bglEnableClientState)(GLenum cap);
extern void (APIENTRY * bglDisableClientState)(GLenum cap);
extern void (APIENTRY * bglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY * bglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY * bglDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRY * bglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

// Stencil Buffer
extern void (APIENTRY * bglClearStencil)(GLint s);
extern void (APIENTRY * bglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
extern void (APIENTRY * bglStencilFunc)(GLenum func, GLint ref, GLuint mask);

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

#endif //USE_OPENGL

extern char *gldriver;

int loadgldriver(const char *driver);
int loadglextensions(void);
int unloadgldriver(void);

