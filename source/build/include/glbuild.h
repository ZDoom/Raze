
#ifndef BGLBUILD_H_INCLUDED_
#define BGLBUILD_H_INCLUDED_

#ifdef USE_OPENGL

#if !defined GEKKO && !defined EDUKE32_GLES
# define DYNAMIC_GL
# define DYNAMIC_GLU
# define DYNAMIC_GLEXT
# define USE_GLEXT
#endif

// include this before glext does
#ifdef _WIN32
# include "windows_inc.h"
#endif

// Tell gl.h to not include glext.h, we'll include our own copy in a minute
#define GL_GLEXT_LEGACY

#if defined EDUKE32_OSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#elif defined EDUKE32_IOS
# include <OpenGLES/ES1/gl.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#if !defined DYNAMIC_GLEXT
# define GL_GLEXT_PROTOTYPES
#endif

// get this header from http://oss.sgi.com/projects/ogl-sample/registry/
// if you are missing it
//#include <GL/glext.h>
#if defined EDUKE32_OSX
# include <OpenGL/glext.h>
# define GL_MAP_WRITE_BIT                  0x0002
# define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
# define GL_MAP_PERSISTENT_BIT             0x0040
# define GL_MAP_COHERENT_BIT               0x0080
# define GL_DYNAMIC_STORAGE_BIT            0x0100
# define GL_DEBUG_TYPE_ERROR_ARB           0x824C
#elif defined EDUKE32_IOS
typedef double GLdouble;
# include <OpenGLES/ES1/glext.h>
typedef GLintptr GLintptrARB;
typedef GLsizeiptr GLsizeiptrARB;
typedef GLchar GLcharARB;
typedef void* GLhandleARB;
typedef unsigned int GLenum;
# define GL_TEXTURE0_ARB GL_TEXTURE0
# define GL_RGB_SCALE_ARB GL_RGB_SCALE
# define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
# define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
# define GL_WRITE_ONLY_ARB                 0x88B9
# define GL_STREAM_DRAW_ARB                0x88E0
# define GL_STATIC_DRAW_ARB                0x88E4

# define GL_RGBA4                          0x8056
# define GL_RGB5_A1                        0x8057
# define GL_RGB565                         0x8D62
# define GL_COMPRESSED_RGB8_ETC2           0x9274
# define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
# define GL_COMPRESSED_RGBA8_ETC2_EAC      0x9278
#else
# include "glext.h"
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#error You should get an updated copy of glext.h from http://oss.sgi.com/projects/ogl-sample/registry/
#endif

#if defined EDUKE32_GLES
# include "jwzgles.h"
#endif

#ifndef APIENTRY
# define APIENTRY
#endif

# ifdef _WIN32
#  define PR_CALLBACK __stdcall
# else
#  define PR_CALLBACK
# endif

// those defines are somehow missing from glext.h
#define GL_FRAMEBUFFER_EXT                      0x8D40
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT             0x8CD5

#define GL_TEXTURE_RECTANGLE                    0x84F5

#ifndef GL_ETC1_RGB8_OES
# define GL_ETC1_RGB8_OES 0x8D64
#endif

// custom error checking

extern GLenum BuildGLError;
extern void BuildGLErrorCheck(void);


//////// dynamic/static API wrapping ////////

#if defined DYNAMIC_GL

#ifdef _WIN32
typedef HGLRC (WINAPI * bwglCreateContextProcPtr)(HDC);
extern bwglCreateContextProcPtr bwglCreateContext;
typedef BOOL (WINAPI * bwglDeleteContextProcPtr)(HGLRC);
extern bwglDeleteContextProcPtr bwglDeleteContext;
typedef PROC (WINAPI * bwglGetProcAddressProcPtr)(LPCSTR);
extern bwglGetProcAddressProcPtr bwglGetProcAddress;
typedef BOOL (WINAPI * bwglMakeCurrentProcPtr)(HDC,HGLRC);
extern bwglMakeCurrentProcPtr bwglMakeCurrent;

typedef BOOL (WINAPI * bwglSwapBuffersProcPtr)(HDC);
extern bwglSwapBuffersProcPtr bwglSwapBuffers;
typedef int32_t (WINAPI * bwglChoosePixelFormatProcPtr)(HDC,CONST PIXELFORMATDESCRIPTOR*);
extern bwglChoosePixelFormatProcPtr bwglChoosePixelFormat;
typedef int32_t (WINAPI * bwglDescribePixelFormatProcPtr)(HDC,int32_t,UINT,LPPIXELFORMATDESCRIPTOR);
extern bwglDescribePixelFormatProcPtr bwglDescribePixelFormat;
typedef int32_t (WINAPI * bwglGetPixelFormatProcPtr)(HDC);
extern bwglGetPixelFormatProcPtr bwglGetPixelFormat;
typedef BOOL (WINAPI * bwglSetPixelFormatProcPtr)(HDC,int32_t,const PIXELFORMATDESCRIPTOR*);
extern bwglSetPixelFormatProcPtr bwglSetPixelFormat;
#endif

typedef void (APIENTRY * bglClearColorProcPtr)( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
extern bglClearColorProcPtr bglClearColor;
typedef void (APIENTRY * bglClearProcPtr)( GLbitfield mask );
extern bglClearProcPtr bglClear;
typedef void (APIENTRY * bglColorMaskProcPtr)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
extern bglColorMaskProcPtr bglColorMask;
typedef void (APIENTRY * bglAlphaFuncProcPtr)( GLenum func, GLclampf ref );
extern bglAlphaFuncProcPtr bglAlphaFunc;
typedef void (APIENTRY * bglBlendFuncProcPtr)( GLenum sfactor, GLenum dfactor );
extern bglBlendFuncProcPtr bglBlendFunc;
typedef void (APIENTRY * bglCullFaceProcPtr)( GLenum mode );
extern bglCullFaceProcPtr bglCullFace;
typedef void (APIENTRY * bglFrontFaceProcPtr)( GLenum mode );
extern bglFrontFaceProcPtr bglFrontFace;
typedef void (APIENTRY * bglPolygonOffsetProcPtr)( GLfloat factor, GLfloat units );
extern bglPolygonOffsetProcPtr bglPolygonOffset;
typedef void (APIENTRY * bglPolygonModeProcPtr)( GLenum face, GLenum mode );
extern bglPolygonModeProcPtr bglPolygonMode;
typedef void (APIENTRY * bglEnableProcPtr)( GLenum cap );
extern bglEnableProcPtr bglEnable;
typedef void (APIENTRY * bglDisableProcPtr)( GLenum cap );
extern bglDisableProcPtr bglDisable;
typedef void (APIENTRY * bglGetDoublevProcPtr)( GLenum pname, GLdouble *params );
extern bglGetDoublevProcPtr bglGetDoublev;
typedef void (APIENTRY * bglGetFloatvProcPtr)( GLenum pname, GLfloat *params );
extern bglGetFloatvProcPtr bglGetFloatv;
typedef void (APIENTRY * bglGetIntegervProcPtr)( GLenum pname, GLint *params );
extern bglGetIntegervProcPtr bglGetIntegerv;
typedef void (APIENTRY * bglPushAttribProcPtr)( GLbitfield mask );
extern bglPushAttribProcPtr bglPushAttrib;
typedef void (APIENTRY * bglPopAttribProcPtr)( void );
extern bglPopAttribProcPtr bglPopAttrib;
typedef GLenum (APIENTRY * bglGetErrorProcPtr)( void );
extern bglGetErrorProcPtr bglGetError;
typedef const GLubyte* (APIENTRY * bglGetStringProcPtr)( GLenum name );
extern bglGetStringProcPtr bglGetString;
typedef void (APIENTRY * bglHintProcPtr)( GLenum target, GLenum mode );
extern bglHintProcPtr bglHint;
typedef void (APIENTRY * bglDrawBufferProcPtr)(GLenum mode);
extern bglDrawBufferProcPtr bglDrawBuffer;
typedef void (APIENTRY * bglReadBufferProcPtr)(GLenum mode);
extern bglReadBufferProcPtr bglReadBuffer;
typedef void (APIENTRY * bglScissorProcPtr)(GLint x, GLint y, GLsizei width, GLsizei height);
extern bglScissorProcPtr bglScissor;
typedef void (APIENTRY * bglClipPlaneProcPtr)(GLenum plane, const GLdouble *equation);
extern bglClipPlaneProcPtr bglClipPlane;

// Depth
typedef void (APIENTRY * bglDepthFuncProcPtr)( GLenum func );
extern bglDepthFuncProcPtr bglDepthFunc;
typedef void (APIENTRY * bglDepthMaskProcPtr)( GLboolean flag );
extern bglDepthMaskProcPtr bglDepthMask;
//typedef void (APIENTRY * bglDepthRangeProcPtr)( GLclampd near_val, GLclampd far_val );
//extern bglDepthRangeProcPtr bglDepthRange;

// Matrix
typedef void (APIENTRY * bglMatrixModeProcPtr)( GLenum mode );
extern bglMatrixModeProcPtr bglMatrixMode;
typedef void (APIENTRY * bglOrthoProcPtr)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
extern bglOrthoProcPtr bglOrtho;
typedef void (APIENTRY * bglFrustumProcPtr)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
extern bglFrustumProcPtr bglFrustum;
typedef void (APIENTRY * bglViewportProcPtr)( GLint x, GLint y, GLsizei width, GLsizei height );
extern bglViewportProcPtr bglViewport;
typedef void (APIENTRY * bglPushMatrixProcPtr)( void );
extern bglPushMatrixProcPtr bglPushMatrix;
typedef void (APIENTRY * bglPopMatrixProcPtr)( void );
extern bglPopMatrixProcPtr bglPopMatrix;
typedef void (APIENTRY * bglLoadIdentityProcPtr)( void );
extern bglLoadIdentityProcPtr bglLoadIdentity;
typedef void (APIENTRY * bglLoadMatrixfProcPtr)( const GLfloat *m );
extern bglLoadMatrixfProcPtr bglLoadMatrixf;
typedef void (APIENTRY * bglLoadMatrixdProcPtr)( const GLdouble *m );
extern bglLoadMatrixdProcPtr bglLoadMatrixd;
typedef void (APIENTRY * bglMultMatrixfProcPtr)( const GLfloat *m );
extern bglMultMatrixfProcPtr bglMultMatrixf;
typedef void (APIENTRY * bglMultMatrixdProcPtr)( const GLdouble *m );
extern bglMultMatrixdProcPtr bglMultMatrixd;
typedef void (APIENTRY * bglRotatefProcPtr)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern bglRotatefProcPtr bglRotatef;
typedef void (APIENTRY * bglScalefProcPtr)(GLfloat x, GLfloat y, GLfloat z);
extern bglScalefProcPtr bglScalef;
typedef void (APIENTRY * bglTranslatefProcPtr)(GLfloat x, GLfloat y, GLfloat z);
extern bglTranslatefProcPtr bglTranslatef;

// Drawing
typedef void (APIENTRY * bglBeginProcPtr)( GLenum mode );
extern bglBeginProcPtr bglBegin;
typedef void (APIENTRY * bglEndProcPtr)( void );
extern bglEndProcPtr bglEnd;
typedef void (APIENTRY * bglVertex2fProcPtr)( GLfloat x, GLfloat y );
extern bglVertex2fProcPtr bglVertex2f;
typedef void (APIENTRY * bglVertex2iProcPtr)( GLint x, GLint y );
extern bglVertex2iProcPtr bglVertex2i;
typedef void (APIENTRY * bglVertex3fProcPtr)( GLfloat x, GLfloat y, GLfloat z );
extern bglVertex3fProcPtr bglVertex3f;
typedef void (APIENTRY * bglVertex3dProcPtr)( GLdouble x, GLdouble y, GLdouble z );
extern bglVertex3dProcPtr bglVertex3d;
typedef void (APIENTRY * bglVertex3fvProcPtr)( const GLfloat *v );
extern bglVertex3fvProcPtr bglVertex3fv;
typedef void (APIENTRY * bglVertex3dvProcPtr)( const GLdouble *v );
extern bglVertex3dvProcPtr bglVertex3dv;
typedef void (APIENTRY * bglRectiProcPtr)( GLint   x1, GLint   y1, GLint   x2, GLint   y2 );
extern bglRectiProcPtr bglRecti;
typedef void (APIENTRY * bglColor3fProcPtr)(GLfloat red, GLfloat green, GLfloat blue);
extern bglColor3fProcPtr bglColor3f;
typedef void (APIENTRY * bglColor4fProcPtr)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
extern bglColor4fProcPtr bglColor4f;
typedef void (APIENTRY * bglColor4ubProcPtr)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
extern bglColor4ubProcPtr bglColor4ub;
typedef void (APIENTRY * bglTexCoord2dProcPtr)( GLdouble s, GLdouble t );
extern bglTexCoord2dProcPtr bglTexCoord2d;
typedef void (APIENTRY * bglTexCoord2fProcPtr)( GLfloat s, GLfloat t );
extern bglTexCoord2fProcPtr bglTexCoord2f;
typedef void (APIENTRY * bglTexCoord2iProcPtr)( GLint s, GLint t );
extern bglTexCoord2iProcPtr bglTexCoord2i;
typedef void (APIENTRY * bglNormal3fProcPtr)( GLfloat x, GLfloat y, GLfloat z );
extern bglNormal3fProcPtr bglNormal3f;

// Lighting
typedef void (APIENTRY * bglShadeModelProcPtr)( GLenum mode );
extern bglShadeModelProcPtr bglShadeModel;
typedef void (APIENTRY * bglLightfvProcPtr)( GLenum light, GLenum pname, const GLfloat * params );
extern bglLightfvProcPtr bglLightfv;

// Raster funcs
typedef void (APIENTRY * bglReadPixelsProcPtr)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
extern bglReadPixelsProcPtr bglReadPixels;
typedef void (APIENTRY * bglRasterPos4iProcPtr)( GLint x, GLint y, GLint z, GLint w );
extern bglRasterPos4iProcPtr bglRasterPos4i;
typedef void (APIENTRY * bglDrawPixelsProcPtr)( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
extern bglDrawPixelsProcPtr bglDrawPixels;
typedef void (APIENTRY * bglPixelStoreiProcPtr)( GLenum pname, GLint param );
extern bglPixelStoreiProcPtr bglPixelStorei;

// Texture mapping
typedef void (APIENTRY * bglTexEnvfProcPtr)( GLenum target, GLenum pname, GLfloat param );
extern bglTexEnvfProcPtr bglTexEnvf;
typedef void (APIENTRY * bglGenTexturesProcPtr)( GLsizei n, GLuint *textures );	// 1.1
extern bglGenTexturesProcPtr bglGenTextures;
typedef void (APIENTRY * bglDeleteTexturesProcPtr)( GLsizei n, const GLuint *textures);	// 1.1
extern bglDeleteTexturesProcPtr bglDeleteTextures;
typedef void (APIENTRY * bglBindTextureProcPtr)( GLenum target, GLuint texture );	// 1.1
extern bglBindTextureProcPtr bglBindTexture;
typedef void (APIENTRY * bglTexImage2DProcPtr)( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
extern bglTexImage2DProcPtr bglTexImage2D;
typedef void (APIENTRY * bglCopyTexImage2DProcPtr)( GLenum	target, GLint level, GLenum	internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
extern bglCopyTexImage2DProcPtr bglCopyTexImage2D;
typedef void (APIENTRY * bglCopyTexSubImage2DProcPtr)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern bglCopyTexSubImage2DProcPtr bglCopyTexSubImage2D;
typedef void (APIENTRY * bglTexSubImage2DProcPtr)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );	// 1.1
extern bglTexSubImage2DProcPtr bglTexSubImage2D;
typedef void (APIENTRY * bglTexParameterfProcPtr)( GLenum target, GLenum pname, GLfloat param );
extern bglTexParameterfProcPtr bglTexParameterf;
typedef void (APIENTRY * bglTexParameteriProcPtr)( GLenum target, GLenum pname, GLint param );
extern bglTexParameteriProcPtr bglTexParameteri;
typedef void (APIENTRY * bglGetTexParameterivProcPtr)( GLenum target, GLenum pname, GLint *params );
extern bglGetTexParameterivProcPtr bglGetTexParameteriv;
typedef void (APIENTRY * bglGetTexLevelParameterivProcPtr)( GLenum target, GLint level, GLenum pname, GLint *params );
extern bglGetTexLevelParameterivProcPtr bglGetTexLevelParameteriv;
typedef void (APIENTRY * bglTexGenfvProcPtr)(GLenum coord, GLenum pname, const GLfloat *params);
extern bglTexGenfvProcPtr bglTexGenfv;

// Fog
typedef void (APIENTRY * bglFogfProcPtr)( GLenum pname, GLfloat param );
extern bglFogfProcPtr bglFogf;
typedef void (APIENTRY * bglFogiProcPtr)( GLenum pname, GLint param );
extern bglFogiProcPtr bglFogi;
typedef void (APIENTRY * bglFogfvProcPtr)( GLenum pname, const GLfloat *params );
extern bglFogfvProcPtr bglFogfv;

// Display Lists
typedef void (APIENTRY * bglNewListProcPtr)(GLuint list, GLenum mode);
extern bglNewListProcPtr bglNewList;
typedef void (APIENTRY * bglEndListProcPtr)(void);
extern bglEndListProcPtr bglEndList;
typedef void (APIENTRY * bglCallListProcPtr)(GLuint list);
extern bglCallListProcPtr bglCallList;
typedef void (APIENTRY * bglDeleteListsProcPtr)(GLuint list, GLsizei range);
extern bglDeleteListsProcPtr bglDeleteLists;

// Vertex Arrays
typedef void (APIENTRY * bglEnableClientStateProcPtr)(GLenum cap);
extern bglEnableClientStateProcPtr bglEnableClientState;
typedef void (APIENTRY * bglDisableClientStateProcPtr)(GLenum cap);
extern bglDisableClientStateProcPtr bglDisableClientState;
typedef void (APIENTRY * bglVertexPointerProcPtr)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern bglVertexPointerProcPtr bglVertexPointer;
typedef void (APIENTRY * bglNormalPointerProcPtr)(GLenum type, GLsizei stride, const GLvoid *pointer);
extern bglNormalPointerProcPtr bglNormalPointer;
typedef void (APIENTRY * bglTexCoordPointerProcPtr)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern bglTexCoordPointerProcPtr bglTexCoordPointer;
typedef void (APIENTRY * bglDrawArraysProcPtr)(GLenum mode, GLint first, GLsizei count);
extern bglDrawArraysProcPtr bglDrawArrays;
typedef void (APIENTRY * bglDrawElementsProcPtr)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern bglDrawElementsProcPtr bglDrawElements;

// Stencil Buffer
typedef void (APIENTRY * bglClearStencilProcPtr)(GLint s);
extern bglClearStencilProcPtr bglClearStencil;
typedef void (APIENTRY * bglStencilOpProcPtr)(GLenum fail, GLenum zfail, GLenum zpass);
extern bglStencilOpProcPtr bglStencilOp;
typedef void (APIENTRY * bglStencilFuncProcPtr)(GLenum func, GLint ref, GLuint mask);
extern bglStencilFuncProcPtr bglStencilFunc;

#else

#define bglClearColor glClearColor
#define bglClear glClear
#define bglColorMask glColorMask
#define bglAlphaFunc glAlphaFunc
#define bglBlendFunc glBlendFunc
#define bglCullFace glCullFace
#define bglFrontFace glFrontFace
#define bglPolygonOffset glPolygonOffset
#define bglPolygonMode glPolygonMode
#define bglEnable glEnable
#define bglDisable glDisable
#define bglGetDoublev glGetDoublev
#define bglGetFloatv glGetFloatv
#define bglGetIntegerv glGetIntegerv
#define bglPushAttrib glPushAttrib
#define bglPopAttrib glPopAttrib
#define bglGetError glGetError
#define bglGetString glGetString
#define bglHint glHint
#define bglDrawBuffer glDrawBuffer
#define bglReadBuffer glReadBuffer
#define bglScissor glScissor
#define bglClipPlane glClipPlane

// Depth
#define bglDepthFunc glDepthFunc
#define bglDepthMask glDepthMask
//#define bglDepthRange glDepthRange

// Matrix
#define bglMatrixMode glMatrixMode
#define bglOrtho glOrtho
#define bglFrustum glFrustum
#define bglViewport glViewport
#define bglPushMatrix glPushMatrix
#define bglPopMatrix glPopMatrix
#define bglLoadIdentity glLoadIdentity
#define bglLoadMatrixf glLoadMatrixf
#define bglLoadMatrixd glLoadMatrixd
#define bglMultMatrixf glMultMatrixf
#define bglMultMatrixd glMultMatrixd
#define bglRotatef glRotatef
#define bglScalef glScalef
#define bglTranslatef glTranslatef

// Drawing
#define bglBegin glBegin
#define bglEnd glEnd
#define bglVertex2f glVertex2f
#define bglVertex2i glVertex2i
#define bglVertex3f glVertex3f
#define bglVertex3d glVertex3d
#define bglVertex3fv glVertex3fv
#define bglVertex3dv glVertex3dv
#define bglRecti glRecti
#define bglColor3f glColor3f
#define bglColor4f glColor4f
#define bglColor4ub glColor4ub
#define bglTexCoord2d glTexCoord2d
#define bglTexCoord2f glTexCoord2f
#define bglTexCoord2i glTexCoord2i
#define bglNormal3f glNormal3f

// Lighting
#define bglShadeModel glShadeModel
#define bglLightfv glLightfv

// Raster funcs
#define bglReadPixels glReadPixels
#define bglRasterPos4i glRasterPos4i
#define bglDrawPixels glDrawPixels
#define bglPixelStorei glPixelStorei

// Texture mapping
#define bglTexEnvf glTexEnvf
#define bglGenTextures glGenTextures
#define bglDeleteTextures glDeleteTextures
#define bglBindTexture glBindTexture
#define bglTexImage2D glTexImage2D
#define bglCopyTexImage2D glCopyTexImage2D
#define bglCopyTexSubImage2D glCopyTexSubImage2D;
#define bglTexSubImage2D glTexSubImage2D
#define bglTexParameterf glTexParameterf
#define bglTexParameteri glTexParameteri
#define bglGetTexParameteriv glGetTexParameteriv
#define bglGetTexLevelParameteriv glGetTexLevelParameteriv

// Fog
#define bglFogf glFogf
#define bglFogi glFogi
#define bglFogfv glFogfv

// Display Lists
#define bglNewList glNewList
#define bglEndList glEndList
#define bglCallList glCallList
#define bglDeleteLists glDeleteLists

// Vertex Arrays
#define bglEnableClientState glEnableClientState
#define bglDisableClientState glDisableClientState
#define bglVertexPointer glVertexPointer
#define bglNormalPointer glNormalPointer
#define bglTexCoordPointer glTexCoordPointer
#define bglDrawElements glDrawElements

// Stencil Buffer
#define bglClearStencil glClearStencil
#define bglStencilOp glStencilOp
#define bglStencilFunc glStencilFunc

#endif

#if defined DYNAMIC_GLEXT

typedef void (APIENTRY * bglBlendEquationProcPtr)( GLenum mode );
extern bglBlendEquationProcPtr bglBlendEquation;

typedef void (APIENTRY * bglTexImage3DProcPtr)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern bglTexImage3DProcPtr bglTexImage3D;
typedef void (APIENTRY * bglCompressedTexImage2DARBProcPtr)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
extern bglCompressedTexImage2DARBProcPtr bglCompressedTexImage2DARB;
typedef void (APIENTRY * bglGetCompressedTexImageARBProcPtr)(GLenum, GLint, GLvoid *);
extern bglGetCompressedTexImageARBProcPtr bglGetCompressedTexImageARB;

// GPU Programs
typedef void (APIENTRY * bglGenProgramsARBProcPtr)(GLsizei, GLuint *);
extern bglGenProgramsARBProcPtr bglGenProgramsARB;
typedef void (APIENTRY * bglBindProgramARBProcPtr)(GLenum, GLuint);
extern bglBindProgramARBProcPtr bglBindProgramARB;
typedef void (APIENTRY * bglProgramStringARBProcPtr)(GLenum, GLenum, GLsizei, const GLvoid *);
extern bglProgramStringARBProcPtr bglProgramStringARB;
typedef void (APIENTRY * bglDeleteProgramsARBProcPtr)(GLsizei n, const GLuint *programs);
extern bglDeleteProgramsARBProcPtr bglDeleteProgramsARB;

// Multitexturing
typedef void (APIENTRY * bglActiveTextureARBProcPtr)(GLenum texture);
extern bglActiveTextureARBProcPtr bglActiveTextureARB;
typedef void (APIENTRY * bglClientActiveTextureARBProcPtr)(GLenum texture);
extern bglClientActiveTextureARBProcPtr bglClientActiveTextureARB;
typedef void (APIENTRY * bglMultiTexCoord2dARBProcPtr)(GLenum target, GLdouble s, GLdouble t );
extern bglMultiTexCoord2dARBProcPtr bglMultiTexCoord2dARB;
typedef void (APIENTRY * bglMultiTexCoord2fARBProcPtr)(GLenum target, GLfloat s, GLfloat t );
extern bglMultiTexCoord2fARBProcPtr bglMultiTexCoord2fARB;

// Frame Buffer Objects
typedef void (APIENTRY * bglGenFramebuffersEXTProcPtr)(GLsizei n, GLuint *framebuffers);
extern bglGenFramebuffersEXTProcPtr bglGenFramebuffersEXT;
typedef void (APIENTRY * bglBindFramebufferEXTProcPtr)(GLenum target, GLuint framebuffer);
extern bglBindFramebufferEXTProcPtr bglBindFramebufferEXT;
typedef void (APIENTRY * bglFramebufferTexture2DEXTProcPtr)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern bglFramebufferTexture2DEXTProcPtr bglFramebufferTexture2DEXT;
typedef GLenum (APIENTRY * bglCheckFramebufferStatusEXTProcPtr)(GLenum target);
extern bglCheckFramebufferStatusEXTProcPtr bglCheckFramebufferStatusEXT;
typedef void (APIENTRY * bglDeleteFramebuffersEXTProcPtr)(GLsizei n, const GLuint *framebuffers);
extern bglDeleteFramebuffersEXTProcPtr bglDeleteFramebuffersEXT;

// Vertex Buffer Objects
typedef void        (APIENTRY * bglGenBuffersARBProcPtr)(GLsizei n, GLuint * buffers);
extern bglGenBuffersARBProcPtr bglGenBuffersARB;
typedef void        (APIENTRY * bglBindBufferARBProcPtr)(GLenum target, GLuint buffer);
extern bglBindBufferARBProcPtr bglBindBufferARB;
typedef void        (APIENTRY * bglDeleteBuffersARBProcPtr)(GLsizei n, const GLuint * buffers);
extern bglDeleteBuffersARBProcPtr bglDeleteBuffersARB;
typedef void        (APIENTRY * bglBufferDataARBProcPtr)(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage);
extern bglBufferDataARBProcPtr bglBufferDataARB;
typedef void        (APIENTRY * bglBufferSubDataARBProcPtr)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data);
extern bglBufferSubDataARBProcPtr bglBufferSubDataARB;
typedef void*       (APIENTRY * bglMapBufferARBProcPtr)(GLenum target, GLenum access);
extern bglMapBufferARBProcPtr bglMapBufferARB;
typedef GLboolean   (APIENTRY * bglUnmapBufferARBProcPtr)(GLenum target);
extern bglUnmapBufferARBProcPtr bglUnmapBufferARB;

// ARB_buffer_storage
typedef void        (APIENTRY * bglBufferStorageProcPtr)(GLenum target, GLintptrARB size, const GLvoid * data, GLbitfield flags);
extern bglBufferStorageProcPtr bglBufferStorage;

// ARB_map_buffer_range
typedef void*       (APIENTRY * bglMapBufferRangeProcPtr) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern bglMapBufferRangeProcPtr bglMapBufferRange;

// Occlusion queries
typedef void (APIENTRY * bglGenQueriesARBProcPtr)(GLsizei n, GLuint *ids);
extern bglGenQueriesARBProcPtr bglGenQueriesARB;
typedef void (APIENTRY * bglDeleteQueriesARBProcPtr)(GLsizei n, const GLuint *ids);
extern bglDeleteQueriesARBProcPtr bglDeleteQueriesARB;
typedef GLboolean (APIENTRY * bglIsQueryARBProcPtr)(GLuint id);
extern bglIsQueryARBProcPtr bglIsQueryARB;
typedef void (APIENTRY * bglBeginQueryARBProcPtr)(GLenum target, GLuint id);
extern bglBeginQueryARBProcPtr bglBeginQueryARB;
typedef void (APIENTRY * bglEndQueryARBProcPtr)(GLenum target);
extern bglEndQueryARBProcPtr bglEndQueryARB;
typedef void (APIENTRY * bglGetQueryivARBProcPtr)(GLenum target, GLenum pname, GLint *params);
extern bglGetQueryivARBProcPtr bglGetQueryivARB;
typedef void (APIENTRY * bglGetQueryObjectivARBProcPtr)(GLuint id, GLenum pname, GLint *params);
extern bglGetQueryObjectivARBProcPtr bglGetQueryObjectivARB;
typedef void (APIENTRY * bglGetQueryObjectuivARBProcPtr)(GLuint id, GLenum pname, GLuint *params);
extern bglGetQueryObjectuivARBProcPtr bglGetQueryObjectuivARB;

// Shader Objects
typedef void (APIENTRY * bglDeleteObjectARBProcPtr)(GLhandleARB);
extern bglDeleteObjectARBProcPtr bglDeleteObjectARB;
typedef GLhandleARB (APIENTRY * bglGetHandleARBProcPtr)(GLenum);
extern bglGetHandleARBProcPtr bglGetHandleARB;
typedef void (APIENTRY * bglDetachObjectARBProcPtr)(GLhandleARB, GLhandleARB);
extern bglDetachObjectARBProcPtr bglDetachObjectARB;
typedef GLhandleARB (APIENTRY * bglCreateShaderObjectARBProcPtr)(GLenum);
extern bglCreateShaderObjectARBProcPtr bglCreateShaderObjectARB;
typedef void (APIENTRY * bglShaderSourceARBProcPtr)(GLhandleARB, GLsizei, const GLcharARB* *, const GLint *);
extern bglShaderSourceARBProcPtr bglShaderSourceARB;
typedef void (APIENTRY * bglCompileShaderARBProcPtr)(GLhandleARB);
extern bglCompileShaderARBProcPtr bglCompileShaderARB;
typedef GLhandleARB (APIENTRY * bglCreateProgramObjectARBProcPtr)(void);
extern bglCreateProgramObjectARBProcPtr bglCreateProgramObjectARB;
typedef void (APIENTRY * bglAttachObjectARBProcPtr)(GLhandleARB, GLhandleARB);
extern bglAttachObjectARBProcPtr bglAttachObjectARB;
typedef void (APIENTRY * bglLinkProgramARBProcPtr)(GLhandleARB);
extern bglLinkProgramARBProcPtr bglLinkProgramARB;
typedef void (APIENTRY * bglUseProgramObjectARBProcPtr)(GLhandleARB);
extern bglUseProgramObjectARBProcPtr bglUseProgramObjectARB;
typedef void (APIENTRY * bglValidateProgramARBProcPtr)(GLhandleARB);
extern bglValidateProgramARBProcPtr bglValidateProgramARB;
typedef void (APIENTRY * bglUniform1fARBProcPtr)(GLint, GLfloat);
extern bglUniform1fARBProcPtr bglUniform1fARB;
typedef void (APIENTRY * bglUniform2fARBProcPtr)(GLint, GLfloat, GLfloat);
extern bglUniform2fARBProcPtr bglUniform2fARB;
typedef void (APIENTRY * bglUniform3fARBProcPtr)(GLint, GLfloat, GLfloat, GLfloat);
extern bglUniform3fARBProcPtr bglUniform3fARB;
typedef void (APIENTRY * bglUniform4fARBProcPtr)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
extern bglUniform4fARBProcPtr bglUniform4fARB;
typedef void (APIENTRY * bglUniform1iARBProcPtr)(GLint, GLint);
extern bglUniform1iARBProcPtr bglUniform1iARB;
typedef void (APIENTRY * bglUniform2iARBProcPtr)(GLint, GLint, GLint);
extern bglUniform2iARBProcPtr bglUniform2iARB;
typedef void (APIENTRY * bglUniform3iARBProcPtr)(GLint, GLint, GLint, GLint);
extern bglUniform3iARBProcPtr bglUniform3iARB;
typedef void (APIENTRY * bglUniform4iARBProcPtr)(GLint, GLint, GLint, GLint, GLint);
extern bglUniform4iARBProcPtr bglUniform4iARB;
typedef void (APIENTRY * bglUniform1fvARBProcPtr)(GLint, GLsizei, const GLfloat *);
extern bglUniform1fvARBProcPtr bglUniform1fvARB;
typedef void (APIENTRY * bglUniform2fvARBProcPtr)(GLint, GLsizei, const GLfloat *);
extern bglUniform2fvARBProcPtr bglUniform2fvARB;
typedef void (APIENTRY * bglUniform3fvARBProcPtr)(GLint, GLsizei, const GLfloat *);
extern bglUniform3fvARBProcPtr bglUniform3fvARB;
typedef void (APIENTRY * bglUniform4fvARBProcPtr)(GLint, GLsizei, const GLfloat *);
extern bglUniform4fvARBProcPtr bglUniform4fvARB;
typedef void (APIENTRY * bglUniform1ivARBProcPtr)(GLint, GLsizei, const GLint *);
extern bglUniform1ivARBProcPtr bglUniform1ivARB;
typedef void (APIENTRY * bglUniform2ivARBProcPtr)(GLint, GLsizei, const GLint *);
extern bglUniform2ivARBProcPtr bglUniform2ivARB;
typedef void (APIENTRY * bglUniform3ivARBProcPtr)(GLint, GLsizei, const GLint *);
extern bglUniform3ivARBProcPtr bglUniform3ivARB;
typedef void (APIENTRY * bglUniform4ivARBProcPtr)(GLint, GLsizei, const GLint *);
extern bglUniform4ivARBProcPtr bglUniform4ivARB;
typedef void (APIENTRY * bglUniformMatrix2fvARBProcPtr)(GLint, GLsizei, GLboolean, const GLfloat *);
extern bglUniformMatrix2fvARBProcPtr bglUniformMatrix2fvARB;
typedef void (APIENTRY * bglUniformMatrix3fvARBProcPtr)(GLint, GLsizei, GLboolean, const GLfloat *);
extern bglUniformMatrix3fvARBProcPtr bglUniformMatrix3fvARB;
typedef void (APIENTRY * bglUniformMatrix4fvARBProcPtr)(GLint, GLsizei, GLboolean, const GLfloat *);
extern bglUniformMatrix4fvARBProcPtr bglUniformMatrix4fvARB;
typedef void (APIENTRY * bglGetObjectParameterfvARBProcPtr)(GLhandleARB, GLenum, GLfloat *);
extern bglGetObjectParameterfvARBProcPtr bglGetObjectParameterfvARB;
typedef void (APIENTRY * bglGetObjectParameterivARBProcPtr)(GLhandleARB, GLenum, GLint *);
extern bglGetObjectParameterivARBProcPtr bglGetObjectParameterivARB;
typedef void (APIENTRY * bglGetInfoLogARBProcPtr)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
extern bglGetInfoLogARBProcPtr bglGetInfoLogARB;
typedef void (APIENTRY * bglGetAttachedObjectsARBProcPtr)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
extern bglGetAttachedObjectsARBProcPtr bglGetAttachedObjectsARB;
typedef GLint (APIENTRY * bglGetUniformLocationARBProcPtr)(GLhandleARB, const GLcharARB *);
extern bglGetUniformLocationARBProcPtr bglGetUniformLocationARB;
typedef void (APIENTRY * bglGetActiveUniformARBProcPtr)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
extern bglGetActiveUniformARBProcPtr bglGetActiveUniformARB;
typedef void (APIENTRY * bglGetUniformfvARBProcPtr)(GLhandleARB, GLint, GLfloat *);
extern bglGetUniformfvARBProcPtr bglGetUniformfvARB;
typedef void (APIENTRY * bglGetUniformivARBProcPtr)(GLhandleARB, GLint, GLint *);
extern bglGetUniformivARBProcPtr bglGetUniformivARB;
typedef void (APIENTRY * bglGetShaderSourceARBProcPtr)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
extern bglGetShaderSourceARBProcPtr bglGetShaderSourceARB;

// Vertex Shaders
typedef void (APIENTRY * bglVertexAttrib1dARBProcPtr)(GLuint, GLdouble);
extern bglVertexAttrib1dARBProcPtr bglVertexAttrib1dARB;
typedef void (APIENTRY * bglVertexAttrib1dvARBProcPtr)(GLuint, const GLdouble *);
extern bglVertexAttrib1dvARBProcPtr bglVertexAttrib1dvARB;
typedef void (APIENTRY * bglVertexAttrib1fARBProcPtr)(GLuint, GLfloat);
extern bglVertexAttrib1fARBProcPtr bglVertexAttrib1fARB;
typedef void (APIENTRY * bglVertexAttrib1fvARBProcPtr)(GLuint, const GLfloat *);
extern bglVertexAttrib1fvARBProcPtr bglVertexAttrib1fvARB;
typedef void (APIENTRY * bglVertexAttrib1sARBProcPtr)(GLuint, GLshort);
extern bglVertexAttrib1sARBProcPtr bglVertexAttrib1sARB;
typedef void (APIENTRY * bglVertexAttrib1svARBProcPtr)(GLuint, const GLshort *);
extern bglVertexAttrib1svARBProcPtr bglVertexAttrib1svARB;
typedef void (APIENTRY * bglVertexAttrib2dARBProcPtr)(GLuint, GLdouble, GLdouble);
extern bglVertexAttrib2dARBProcPtr bglVertexAttrib2dARB;
typedef void (APIENTRY * bglVertexAttrib2dvARBProcPtr)(GLuint, const GLdouble *);
extern bglVertexAttrib2dvARBProcPtr bglVertexAttrib2dvARB;
typedef void (APIENTRY * bglVertexAttrib2fARBProcPtr)(GLuint, GLfloat, GLfloat);
extern bglVertexAttrib2fARBProcPtr bglVertexAttrib2fARB;
typedef void (APIENTRY * bglVertexAttrib2fvARBProcPtr)(GLuint, const GLfloat *);
extern bglVertexAttrib2fvARBProcPtr bglVertexAttrib2fvARB;
typedef void (APIENTRY * bglVertexAttrib2sARBProcPtr)(GLuint, GLshort, GLshort);
extern bglVertexAttrib2sARBProcPtr bglVertexAttrib2sARB;
typedef void (APIENTRY * bglVertexAttrib2svARBProcPtr)(GLuint, const GLshort *);
extern bglVertexAttrib2svARBProcPtr bglVertexAttrib2svARB;
typedef void (APIENTRY * bglVertexAttrib3dARBProcPtr)(GLuint, GLdouble, GLdouble, GLdouble);
extern bglVertexAttrib3dARBProcPtr bglVertexAttrib3dARB;
typedef void (APIENTRY * bglVertexAttrib3dvARBProcPtr)(GLuint, const GLdouble *);
extern bglVertexAttrib3dvARBProcPtr bglVertexAttrib3dvARB;
typedef void (APIENTRY * bglVertexAttrib3fARBProcPtr)(GLuint, GLfloat, GLfloat, GLfloat);
extern bglVertexAttrib3fARBProcPtr bglVertexAttrib3fARB;
typedef void (APIENTRY * bglVertexAttrib3fvARBProcPtr)(GLuint, const GLfloat *);
extern bglVertexAttrib3fvARBProcPtr bglVertexAttrib3fvARB;
typedef void (APIENTRY * bglVertexAttrib3sARBProcPtr)(GLuint, GLshort, GLshort, GLshort);
extern bglVertexAttrib3sARBProcPtr bglVertexAttrib3sARB;
typedef void (APIENTRY * bglVertexAttrib3svARBProcPtr)(GLuint, const GLshort *);
extern bglVertexAttrib3svARBProcPtr bglVertexAttrib3svARB;
typedef void (APIENTRY * bglVertexAttrib4NbvARBProcPtr)(GLuint, const GLbyte *);
extern bglVertexAttrib4NbvARBProcPtr bglVertexAttrib4NbvARB;
typedef void (APIENTRY * bglVertexAttrib4NivARBProcPtr)(GLuint, const GLint *);
extern bglVertexAttrib4NivARBProcPtr bglVertexAttrib4NivARB;
typedef void (APIENTRY * bglVertexAttrib4NsvARBProcPtr)(GLuint, const GLshort *);
extern bglVertexAttrib4NsvARBProcPtr bglVertexAttrib4NsvARB;
typedef void (APIENTRY * bglVertexAttrib4NubARBProcPtr)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
extern bglVertexAttrib4NubARBProcPtr bglVertexAttrib4NubARB;
typedef void (APIENTRY * bglVertexAttrib4NubvARBProcPtr)(GLuint, const GLubyte *);
extern bglVertexAttrib4NubvARBProcPtr bglVertexAttrib4NubvARB;
typedef void (APIENTRY * bglVertexAttrib4NuivARBProcPtr)(GLuint, const GLuint *);
extern bglVertexAttrib4NuivARBProcPtr bglVertexAttrib4NuivARB;
typedef void (APIENTRY * bglVertexAttrib4NusvARBProcPtr)(GLuint, const GLushort *);
extern bglVertexAttrib4NusvARBProcPtr bglVertexAttrib4NusvARB;
typedef void (APIENTRY * bglVertexAttrib4bvARBProcPtr)(GLuint, const GLbyte *);
extern bglVertexAttrib4bvARBProcPtr bglVertexAttrib4bvARB;
typedef void (APIENTRY * bglVertexAttrib4dARBProcPtr)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
extern bglVertexAttrib4dARBProcPtr bglVertexAttrib4dARB;
typedef void (APIENTRY * bglVertexAttrib4dvARBProcPtr)(GLuint, const GLdouble *);
extern bglVertexAttrib4dvARBProcPtr bglVertexAttrib4dvARB;
typedef void (APIENTRY * bglVertexAttrib4fARBProcPtr)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
extern bglVertexAttrib4fARBProcPtr bglVertexAttrib4fARB;
typedef void (APIENTRY * bglVertexAttrib4fvARBProcPtr)(GLuint, const GLfloat *);
extern bglVertexAttrib4fvARBProcPtr bglVertexAttrib4fvARB;
typedef void (APIENTRY * bglVertexAttrib4ivARBProcPtr)(GLuint, const GLint *);
extern bglVertexAttrib4ivARBProcPtr bglVertexAttrib4ivARB;
typedef void (APIENTRY * bglVertexAttrib4sARBProcPtr)(GLuint, GLshort, GLshort, GLshort, GLshort);
extern bglVertexAttrib4sARBProcPtr bglVertexAttrib4sARB;
typedef void (APIENTRY * bglVertexAttrib4svARBProcPtr)(GLuint, const GLshort *);
extern bglVertexAttrib4svARBProcPtr bglVertexAttrib4svARB;
typedef void (APIENTRY * bglVertexAttrib4ubvARBProcPtr)(GLuint, const GLubyte *);
extern bglVertexAttrib4ubvARBProcPtr bglVertexAttrib4ubvARB;
typedef void (APIENTRY * bglVertexAttrib4uivARBProcPtr)(GLuint, const GLuint *);
extern bglVertexAttrib4uivARBProcPtr bglVertexAttrib4uivARB;
typedef void (APIENTRY * bglVertexAttrib4usvARBProcPtr)(GLuint, const GLushort *);
extern bglVertexAttrib4usvARBProcPtr bglVertexAttrib4usvARB;
typedef void (APIENTRY * bglVertexAttribPointerARBProcPtr)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
extern bglVertexAttribPointerARBProcPtr bglVertexAttribPointerARB;
typedef void (APIENTRY * bglEnableVertexAttribArrayARBProcPtr)(GLuint);
extern bglEnableVertexAttribArrayARBProcPtr bglEnableVertexAttribArrayARB;
typedef void (APIENTRY * bglDisableVertexAttribArrayARBProcPtr)(GLuint);
extern bglDisableVertexAttribArrayARBProcPtr bglDisableVertexAttribArrayARB;
typedef void (APIENTRY * bglGetVertexAttribdvARBProcPtr)(GLuint, GLenum, GLdouble *);
extern bglGetVertexAttribdvARBProcPtr bglGetVertexAttribdvARB;
typedef void (APIENTRY * bglGetVertexAttribfvARBProcPtr)(GLuint, GLenum, GLfloat *);
extern bglGetVertexAttribfvARBProcPtr bglGetVertexAttribfvARB;
typedef void (APIENTRY * bglGetVertexAttribivARBProcPtr)(GLuint, GLenum, GLint *);
extern bglGetVertexAttribivARBProcPtr bglGetVertexAttribivARB;
typedef void (APIENTRY * bglGetVertexAttribPointervARBProcPtr)(GLuint, GLenum, GLvoid* *);
extern bglGetVertexAttribPointervARBProcPtr bglGetVertexAttribPointervARB;
typedef void (APIENTRY * bglBindAttribLocationARBProcPtr)(GLhandleARB, GLuint, const GLcharARB *);
extern bglBindAttribLocationARBProcPtr bglBindAttribLocationARB;
typedef void (APIENTRY * bglGetActiveAttribARBProcPtr)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
extern bglGetActiveAttribARBProcPtr bglGetActiveAttribARB;
typedef GLint (APIENTRY * bglGetAttribLocationARBProcPtr)(GLhandleARB, const GLcharARB *);
extern bglGetAttribLocationARBProcPtr bglGetAttribLocationARB;

// Debug Output
#ifndef __APPLE__
typedef void (APIENTRY * bglDebugMessageControlARBProcPtr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
extern bglDebugMessageControlARBProcPtr bglDebugMessageControlARB;
typedef void (APIENTRY * bglDebugMessageCallbackARBProcPtr)(GLDEBUGPROCARB callback, const GLvoid *userParam);
extern bglDebugMessageCallbackARBProcPtr bglDebugMessageCallbackARB;
#endif

#ifdef _WIN32
typedef BOOL (WINAPI * bwglSwapIntervalEXTProcPtr)(int32_t);
extern bwglSwapIntervalEXTProcPtr bwglSwapIntervalEXT;
typedef HGLRC (WINAPI * bwglCreateContextAttribsARBProcPtr)(HDC hDC, HGLRC hShareContext, const int *attribList);
extern bwglCreateContextAttribsARBProcPtr bwglCreateContextAttribsARB;
#endif

#else

#define bglBlendEquation glBlendEquation

#define bglTexImage3D glTexImage3D
#define bglCompressedTexImage2DARB glCompressedTexImage2DARB
#define bglGetCompressedTexImageARB glGetCompressedTexImageARB

// GPU Programs
#define bglGenProgramsARB glGenProgramsARB
#define bglBindProgramARB glBindProgramARB
#define bglProgramStringARB glProgramStringARB
#define bglDeleteProgramsARB glDeleteProgramsARB;

// Multitexturing
#define bglActiveTextureARB glActiveTextureARB
#define bglClientActiveTextureARB glClientActiveTextureARB
#define bglMultiTexCoord2dARB glMultiTexCoord2dARB
#define bglMultiTexCoord2fARB glMultiTexCoord2fARB

// Frame Buffer Objects
#define bglGenFramebuffersEXT glGenFramebuffersEXT
#define bglBindFramebufferEXT glBindFramebufferEXT
#define bglFramebufferTexture2DEXT glFramebufferTexture2DEXT
#define bglCheckFramebufferStatusEXT glCheckFramebufferStatusEXT
#define bglDeleteFramebuffersEXT glDeleteFramebuffersEXT

// Vertex Buffer Objects
#define bglGenBuffersARB glGenBuffersARB
#define bglBindBufferARB glBindBufferARB
#define bglDeleteBuffersARB glDeleteBuffersARB
#define bglBufferDataARB glBufferDataARB
#define bglBufferSubDataARB glBufferSubDataARB
#define bglMapBufferARB glMapBufferARB
#define bglUnmapBufferARB glUnmapBufferARB

// Occlusion queries
#define bglGenQueriesARB glGenQueriesARB
#define bglDeleteQueriesARB glDeleteQueriesARB
#define bglIsQueryARB glIsQueryARB
#define bglBeginQueryARB glBeginQueryARB
#define bglEndQueryARB glEndQueryARB
#define bglGetQueryivARB glGetQueryivARB
#define bglGetQueryObjectivARB glGetQueryObjectivARB
#define bglGetQueryObjectuivARB glGetQueryObjectuivARB

// Shader Objects
#define bglDeleteObjectARB glDeleteObjectARB
#define bglGetHandleARB glGetHandleARB
#define bglDetachObjectARB glDetachObjectARB
#define bglCreateShaderObjectARB glCreateShaderObjectARB
#define bglShaderSourceARB glShaderSourceARB
#define bglCompileShaderARB glCompileShaderARB
#define bglCreateProgramObjectARB glCreateProgramObjectARB
#define bglAttachObjectARB glAttachObjectARB
#define bglLinkProgramARB glLinkProgramARB
#define bglUseProgramObjectARB glUseProgramObjectARB
#define bglValidateProgramARB glValidateProgramARB
#define bglUniform1fARB glUniform1fARB
#define bglUniform2fARB glUniform2fARB
#define bglUniform3fARB glUniform3fARB
#define bglUniform4fARB glUniform4fARB
#define bglUniform1iARB glUniform1iARB
#define bglUniform2iARB glUniform2iARB
#define bglUniform3iARB glUniform3iARB
#define bglUniform4iARB glUniform4iARB
#define bglUniform1fvARB glUniform1fvARB
#define bglUniform2fvARB glUniform2fvARB
#define bglUniform3fvARB glUniform3fvARB
#define bglUniform4fvARB glUniform4fvARB
#define bglUniform1ivARB glUniform1ivARB
#define bglUniform2ivARB glUniform2ivARB
#define bglUniform3ivARB glUniform3ivARB
#define bglUniform4ivARB glUniform4ivARB
#define bglUniformMatrix2fvARB glUniformMatrix2fvARB
#define bglUniformMatrix3fvARB glUniformMatrix3fvARB
#define bglUniformMatrix4fvARB glUniformMatrix4fvARB
#define bglGetObjectParameterfvARB glGetObjectParameterfvARB
#define bglGetObjectParameterivARB glGetObjectParameterivARB
#define bglGetInfoLogARB glGetInfoLogARB
#define bglGetAttachedObjectsARB glGetAttachedObjectsARB
#define bglGetUniformLocationARB glGetUniformLocationARB
#define bglGetActiveUniformARB glGetActiveUniformARB
#define bglGetUniformfvARB glGetUniformfvARB
#define bglGetUniformivARB glGetUniformivARB
#define bglGetShaderSourceARB glGetShaderSourceARB

// Vertex Shaders
#define bglVertexAttrib1dARB glVertexAttrib1dARB
#define bglVertexAttrib1dvARB glVertexAttrib1dvARB
#define bglVertexAttrib1fARB glVertexAttrib1fARB
#define bglVertexAttrib1fvARB glVertexAttrib1fvARB
#define bglVertexAttrib1sARB glVertexAttrib1sARB
#define bglVertexAttrib1svARB glVertexAttrib1svARB
#define bglVertexAttrib2dARB glVertexAttrib2dARB
#define bglVertexAttrib2dvARB glVertexAttrib2dvARB
#define bglVertexAttrib2fARB glVertexAttrib2fARB
#define bglVertexAttrib2fvARB glVertexAttrib2fvARB
#define bglVertexAttrib2sARB glVertexAttrib2sARB
#define bglVertexAttrib2svARB glVertexAttrib2svARB
#define bglVertexAttrib3dARB glVertexAttrib3dARB
#define bglVertexAttrib3dvARB glVertexAttrib3dvARB
#define bglVertexAttrib3fARB glVertexAttrib3fARB
#define bglVertexAttrib3fvARB glVertexAttrib3fvARB
#define bglVertexAttrib3sARB glVertexAttrib3sARB
#define bglVertexAttrib3svARB glVertexAttrib3svARB
#define bglVertexAttrib4NbvARB glVertexAttrib4NbvARB
#define bglVertexAttrib4NivARB glVertexAttrib4NivARB
#define bglVertexAttrib4NsvARB glVertexAttrib4NsvARB
#define bglVertexAttrib4NubARB glVertexAttrib4NubARB
#define bglVertexAttrib4NubvARB glVertexAttrib4NubvARB
#define bglVertexAttrib4NuivARB glVertexAttrib4NuivARB
#define bglVertexAttrib4NusvARB glVertexAttrib4NusvARB
#define bglVertexAttrib4bvARB glVertexAttrib4bvARB
#define bglVertexAttrib4dARB glVertexAttrib4dARB
#define bglVertexAttrib4dvARB glVertexAttrib4dvARB
#define bglVertexAttrib4fARB glVertexAttrib4fARB
#define bglVertexAttrib4fvARB glVertexAttrib4fvARB
#define bglVertexAttrib4ivARB glVertexAttrib4ivARB
#define bglVertexAttrib4sARB glVertexAttrib4sARB
#define bglVertexAttrib4svARB glVertexAttrib4svARB
#define bglVertexAttrib4ubvARB glVertexAttrib4ubvARB
#define bglVertexAttrib4uivARB glVertexAttrib4uivARB
#define bglVertexAttrib4usvARB glVertexAttrib4usvARB
#define bglVertexAttribPointerARB glVertexAttribPointerARB
#define bglEnableVertexAttribArrayARB glEnableVertexAttribArrayARB
#define bglDisableVertexAttribArrayARB glDisableVertexAttribArrayARB
#define bglGetVertexAttribdvARB glGetVertexAttribdvARB
#define bglGetVertexAttribfvARB glGetVertexAttribfvARB
#define bglGetVertexAttribivARB glGetVertexAttribivARB
#define bglGetVertexAttribPointervARB glGetVertexAttribPointervARB
#define bglBindAttribLocationARB glBindAttribLocationARB
#define bglGetActiveAttribARB glGetActiveAttribARB
#define bglGetAttribLocationARB glGetAttribLocationARB

#endif

#if defined DYNAMIC_GLU

// GLU
#if defined __clang__ && defined __APPLE__
// XXX: OS X 10.9 deprecated GLUtesselator.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

typedef void             (APIENTRY * bgluTessBeginContourProcPtr)(GLUtesselator* tess);
extern bgluTessBeginContourProcPtr bgluTessBeginContour;
typedef void             (APIENTRY * bgluTessBeginPolygonProcPtr)(GLUtesselator* tess, GLvoid* data);
extern bgluTessBeginPolygonProcPtr bgluTessBeginPolygon;
typedef void             (APIENTRY * bgluTessCallbackProcPtr)(GLUtesselator* tess, GLenum which, void (PR_CALLBACK CallBackFuncProcPtr)());
extern bgluTessCallbackProcPtr bgluTessCallback;
typedef void             (APIENTRY * bgluTessEndContourProcPtr)(GLUtesselator* tess);
extern bgluTessEndContourProcPtr bgluTessEndContour;
typedef void             (APIENTRY * bgluTessEndPolygonProcPtr)(GLUtesselator* tess);
extern bgluTessEndPolygonProcPtr bgluTessEndPolygon;
typedef void             (APIENTRY * bgluTessNormalProcPtr)(GLUtesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
extern bgluTessNormalProcPtr bgluTessNormal;
typedef void             (APIENTRY * bgluTessPropertyProcPtr)(GLUtesselator* tess, GLenum which, GLdouble data);
extern bgluTessPropertyProcPtr bgluTessProperty;
typedef void             (APIENTRY * bgluTessVertexProcPtr)(GLUtesselator* tess, GLdouble *location, GLvoid* data);
extern bgluTessVertexProcPtr bgluTessVertex;
typedef GLUtesselator*   (APIENTRY * bgluNewTessProcPtr)(void);
extern bgluNewTessProcPtr bgluNewTess;
typedef void             (APIENTRY * bgluDeleteTessProcPtr)(GLUtesselator* tess);
extern bgluDeleteTessProcPtr bgluDeleteTess;

#if defined __clang__ && defined __APPLE__
#pragma clang diagnostic pop
#endif

typedef void             (APIENTRY * bgluPerspectiveProcPtr)(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern bgluPerspectiveProcPtr bgluPerspective;

typedef const GLubyte *  (APIENTRY * bgluErrorStringProcPtr)(GLenum error);
extern bgluErrorStringProcPtr bgluErrorString;

typedef GLint            (APIENTRY * bgluProjectProcPtr)(GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint	*view, GLdouble* winX, GLdouble* winY, GLdouble* winZ);
extern bgluProjectProcPtr bgluProject;
typedef GLint            (APIENTRY * bgluUnProjectProcPtr)(GLdouble winX, GLdouble winY, GLdouble winZ, const GLdouble * model, const GLdouble * proj, const GLint * view, GLdouble* objX, GLdouble* objY, GLdouble* objZ);
extern bgluUnProjectProcPtr bgluUnProject;

#else

#define bgluTessBeginContour gluTessBeginContour
#define bgluTessBeginPolygon gluTessBeginPolygon
#define bgluTessCallback gluTessCallback
#define bgluTessEndContour gluTessEndContour
#define bgluTessEndPolygon gluTessEndPolygon
#define bgluTessNormal gluTessNormal
#define bgluTessProperty gluTessProperty
#define bgluTessVertex gluTessVertex
#define bgluNewTess gluNewTess
#define bgluDeleteTess gluDeleteTess

#define bgluPerspective gluPerspective

#define bgluErrorString gluErrorString

#define bgluProject gluProject
#define bgluUnProject gluUnProject

#endif


//////// glGenTextures/glDeleteTextures debugging ////////
void texdbg_bglGenTextures(GLsizei n, GLuint *textures, const char *srcfn);
void texdbg_bglDeleteTextures(GLsizei n, const GLuint *textures, const char *srcfn);

//#define DEBUG_TEXTURE_NAMES

#if defined DEBUGGINGAIDS && defined DEBUG_TEXTURE_NAMES
# undef bglGenTextures
# undef bglDeleteTextures
# define bglGenTextures(numtexs, texnamear) texdbg_bglGenTextures(numtexs, texnamear, __FILE__)
# define bglDeleteTextures(numtexs, texnamear) texdbg_bglDeleteTextures(numtexs, texnamear, __FILE__)
#endif

#endif //USE_OPENGL

#ifdef _WIN32
extern char *gldriver;
#endif

int32_t loadgldriver(const char *driver);
int32_t loadglextensions(void);
int32_t unloadgldriver(void);

int32_t loadglulibrary(const char *driver);
int32_t unloadglulibrary(void);

#endif
