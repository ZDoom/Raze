
#ifndef BGLBUILD_H_INCLUDED_
#define BGLBUILD_H_INCLUDED_

#ifdef USE_OPENGL

#if !defined GEKKO && !defined EDUKE32_GLES
# define DYNAMIC_GL
# define DYNAMIC_GLU
# define DYNAMIC_GLEXT
# define USE_GLEXT
#endif

#if defined EDUKE32_OSX
# include <OpenGL/glu.h>
#else
# include <GL/glu.h>
#endif
#if defined EDUKE32_GLES
# include "jwzgles.h"
#endif

# ifdef _WIN32
#  define PR_CALLBACK __stdcall
# else
#  define PR_CALLBACK
# endif
// custom error checking

extern GLenum BuildGLError;
extern void BuildGLErrorCheck(void);


//////// dynamic/static API wrapping ////////

#if !defined RENDERTYPESDL && defined _WIN32 && defined DYNAMIC_GL
typedef HGLRC (WINAPI * bwglCreateContextProcPtr)(HDC);
extern bwglCreateContextProcPtr bwglCreateContext;
#define wglCreateContext bwglCreateContext
typedef BOOL (WINAPI * bwglDeleteContextProcPtr)(HGLRC);
extern bwglDeleteContextProcPtr bwglDeleteContext;
#define wglDeleteContext bwglDeleteContext
typedef PROC (WINAPI * bwglGetProcAddressProcPtr)(LPCSTR);
extern bwglGetProcAddressProcPtr bwglGetProcAddress;
#define wglGetProcAddress bwglGetProcAddress
typedef BOOL (WINAPI * bwglMakeCurrentProcPtr)(HDC,HGLRC);
extern bwglMakeCurrentProcPtr bwglMakeCurrent;
#define wglMakeCurrent bwglMakeCurrent

typedef int32_t (WINAPI * bwglChoosePixelFormatProcPtr)(HDC,CONST PIXELFORMATDESCRIPTOR*);
extern bwglChoosePixelFormatProcPtr bwglChoosePixelFormat;
#define wglChoosePixelFormat bwglChoosePixelFormat
typedef int32_t (WINAPI * bwglDescribePixelFormatProcPtr)(HDC,int32_t,UINT,LPPIXELFORMATDESCRIPTOR);
extern bwglDescribePixelFormatProcPtr bwglDescribePixelFormat;
#define wglDescribePixelFormat bwglDescribePixelFormat
typedef int32_t (WINAPI * bwglGetPixelFormatProcPtr)(HDC);
extern bwglGetPixelFormatProcPtr bwglGetPixelFormat;
#define wglGetPixelFormat bwglGetPixelFormat
typedef BOOL (WINAPI * bwglSetPixelFormatProcPtr)(HDC,int32_t,const PIXELFORMATDESCRIPTOR*);
extern bwglSetPixelFormatProcPtr bwglSetPixelFormat;
#define wglSetPixelFormat bwglSetPixelFormat
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
# define glGenTextures(numtexs, texnamear) texdbg_bglGenTextures(numtexs, texnamear, __FILE__)
# define glDeleteTextures(numtexs, texnamear) texdbg_bglDeleteTextures(numtexs, texnamear, __FILE__)
#endif

#endif //USE_OPENGL

#if !defined RENDERTYPESDL && defined _WIN32 && defined DYNAMIC_GL
extern char *gldriver;

int32_t loadwgl(const char *driver);
int32_t unloadwgl(void);
#endif

#ifdef POLYMER
int32_t loadglulibrary(const char *driver);
int32_t unloadglulibrary(void);
#endif

#endif
