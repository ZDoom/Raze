#include "compat.h"
#include "glbuild.h"
#include "baselayer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined USE_OPENGL

#ifdef RENDERTYPESDL
#include "sdl_inc.h"
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
void (APIENTRY * bglClipPlane)(GLenum plane, const GLdouble *equation);

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
void (APIENTRY * bglTexCoord2i)(GLint s, GLint t);
void (APIENTRY * bglNormal3f)(GLfloat x, GLfloat y, GLfloat z);

// Lighting
void (APIENTRY * bglShadeModel)(GLenum mode);
void (APIENTRY * bglLightfv)(GLenum light, GLenum pname, const GLfloat * params);

// Raster funcs
void (APIENTRY * bglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void (APIENTRY * bglRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
void (APIENTRY * bglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

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
void (APIENTRY * bglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
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
void (APIENTRY * bglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data);
void*(APIENTRY * bglMapBufferARB)(GLenum target, GLenum access);
GLboolean(APIENTRY * bglUnmapBufferARB)(GLenum target);

// Occlusion queries
void (APIENTRY * bglGenQueriesARB)(GLsizei n, GLuint *ids);
void (APIENTRY * bglDeleteQueriesARB)(GLsizei n, const GLuint *ids);
GLboolean(APIENTRY * bglIsQueryARB)(GLuint id);
void (APIENTRY * bglBeginQueryARB)(GLenum target, GLuint id);
void (APIENTRY * bglEndQueryARB)(GLenum target);
void (APIENTRY * bglGetQueryivARB)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY * bglGetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
void (APIENTRY * bglGetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);

// Shader Objects
void (APIENTRY * bglDeleteObjectARB)(GLhandleARB);
GLhandleARB(APIENTRY * bglGetHandleARB)(GLenum);
void (APIENTRY * bglDetachObjectARB)(GLhandleARB, GLhandleARB);
GLhandleARB(APIENTRY * bglCreateShaderObjectARB)(GLenum);
void (APIENTRY * bglShaderSourceARB)(GLhandleARB, GLsizei, const GLcharARB* *, const GLint *);
void (APIENTRY * bglCompileShaderARB)(GLhandleARB);
GLhandleARB(APIENTRY * bglCreateProgramObjectARB)(void);
void (APIENTRY * bglAttachObjectARB)(GLhandleARB, GLhandleARB);
void (APIENTRY * bglLinkProgramARB)(GLhandleARB);
void (APIENTRY * bglUseProgramObjectARB)(GLhandleARB);
void (APIENTRY * bglValidateProgramARB)(GLhandleARB);
void (APIENTRY * bglUniform1fARB)(GLint, GLfloat);
void (APIENTRY * bglUniform2fARB)(GLint, GLfloat, GLfloat);
void (APIENTRY * bglUniform3fARB)(GLint, GLfloat, GLfloat, GLfloat);
void (APIENTRY * bglUniform4fARB)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
void (APIENTRY * bglUniform1iARB)(GLint, GLint);
void (APIENTRY * bglUniform2iARB)(GLint, GLint, GLint);
void (APIENTRY * bglUniform3iARB)(GLint, GLint, GLint, GLint);
void (APIENTRY * bglUniform4iARB)(GLint, GLint, GLint, GLint, GLint);
void (APIENTRY * bglUniform1fvARB)(GLint, GLsizei, const GLfloat *);
void (APIENTRY * bglUniform2fvARB)(GLint, GLsizei, const GLfloat *);
void (APIENTRY * bglUniform3fvARB)(GLint, GLsizei, const GLfloat *);
void (APIENTRY * bglUniform4fvARB)(GLint, GLsizei, const GLfloat *);
void (APIENTRY * bglUniform1ivARB)(GLint, GLsizei, const GLint *);
void (APIENTRY * bglUniform2ivARB)(GLint, GLsizei, const GLint *);
void (APIENTRY * bglUniform3ivARB)(GLint, GLsizei, const GLint *);
void (APIENTRY * bglUniform4ivARB)(GLint, GLsizei, const GLint *);
void (APIENTRY * bglUniformMatrix2fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
void (APIENTRY * bglUniformMatrix3fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
void (APIENTRY * bglUniformMatrix4fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
void (APIENTRY * bglGetObjectParameterfvARB)(GLhandleARB, GLenum, GLfloat *);
void (APIENTRY * bglGetObjectParameterivARB)(GLhandleARB, GLenum, GLint *);
void (APIENTRY * bglGetInfoLogARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
void (APIENTRY * bglGetAttachedObjectsARB)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
GLint(APIENTRY * bglGetUniformLocationARB)(GLhandleARB, const GLcharARB *);
void (APIENTRY * bglGetActiveUniformARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
void (APIENTRY * bglGetUniformfvARB)(GLhandleARB, GLint, GLfloat *);
void (APIENTRY * bglGetUniformivARB)(GLhandleARB, GLint, GLint *);
void (APIENTRY * bglGetShaderSourceARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);

// Vertex Shaders
void (APIENTRY * bglVertexAttrib1dARB)(GLuint, GLdouble);
void (APIENTRY * bglVertexAttrib1dvARB)(GLuint, const GLdouble *);
void (APIENTRY * bglVertexAttrib1fARB)(GLuint, GLfloat);
void (APIENTRY * bglVertexAttrib1fvARB)(GLuint, const GLfloat *);
void (APIENTRY * bglVertexAttrib1sARB)(GLuint, GLshort);
void (APIENTRY * bglVertexAttrib1svARB)(GLuint, const GLshort *);
void (APIENTRY * bglVertexAttrib2dARB)(GLuint, GLdouble, GLdouble);
void (APIENTRY * bglVertexAttrib2dvARB)(GLuint, const GLdouble *);
void (APIENTRY * bglVertexAttrib2fARB)(GLuint, GLfloat, GLfloat);
void (APIENTRY * bglVertexAttrib2fvARB)(GLuint, const GLfloat *);
void (APIENTRY * bglVertexAttrib2sARB)(GLuint, GLshort, GLshort);
void (APIENTRY * bglVertexAttrib2svARB)(GLuint, const GLshort *);
void (APIENTRY * bglVertexAttrib3dARB)(GLuint, GLdouble, GLdouble, GLdouble);
void (APIENTRY * bglVertexAttrib3dvARB)(GLuint, const GLdouble *);
void (APIENTRY * bglVertexAttrib3fARB)(GLuint, GLfloat, GLfloat, GLfloat);
void (APIENTRY * bglVertexAttrib3fvARB)(GLuint, const GLfloat *);
void (APIENTRY * bglVertexAttrib3sARB)(GLuint, GLshort, GLshort, GLshort);
void (APIENTRY * bglVertexAttrib3svARB)(GLuint, const GLshort *);
void (APIENTRY * bglVertexAttrib4NbvARB)(GLuint, const GLbyte *);
void (APIENTRY * bglVertexAttrib4NivARB)(GLuint, const GLint *);
void (APIENTRY * bglVertexAttrib4NsvARB)(GLuint, const GLshort *);
void (APIENTRY * bglVertexAttrib4NubARB)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
void (APIENTRY * bglVertexAttrib4NubvARB)(GLuint, const GLubyte *);
void (APIENTRY * bglVertexAttrib4NuivARB)(GLuint, const GLuint *);
void (APIENTRY * bglVertexAttrib4NusvARB)(GLuint, const GLushort *);
void (APIENTRY * bglVertexAttrib4bvARB)(GLuint, const GLbyte *);
void (APIENTRY * bglVertexAttrib4dARB)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
void (APIENTRY * bglVertexAttrib4dvARB)(GLuint, const GLdouble *);
void (APIENTRY * bglVertexAttrib4fARB)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
void (APIENTRY * bglVertexAttrib4fvARB)(GLuint, const GLfloat *);
void (APIENTRY * bglVertexAttrib4ivARB)(GLuint, const GLint *);
void (APIENTRY * bglVertexAttrib4sARB)(GLuint, GLshort, GLshort, GLshort, GLshort);
void (APIENTRY * bglVertexAttrib4svARB)(GLuint, const GLshort *);
void (APIENTRY * bglVertexAttrib4ubvARB)(GLuint, const GLubyte *);
void (APIENTRY * bglVertexAttrib4uivARB)(GLuint, const GLuint *);
void (APIENTRY * bglVertexAttrib4usvARB)(GLuint, const GLushort *);
void (APIENTRY * bglVertexAttribPointerARB)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
void (APIENTRY * bglEnableVertexAttribArrayARB)(GLuint);
void (APIENTRY * bglDisableVertexAttribArrayARB)(GLuint);
void (APIENTRY * bglGetVertexAttribdvARB)(GLuint, GLenum, GLdouble *);
void (APIENTRY * bglGetVertexAttribfvARB)(GLuint, GLenum, GLfloat *);
void (APIENTRY * bglGetVertexAttribivARB)(GLuint, GLenum, GLint *);
void (APIENTRY * bglGetVertexAttribPointervARB)(GLuint, GLenum, GLvoid* *);
void (APIENTRY * bglBindAttribLocationARB)(GLhandleARB, GLuint, const GLcharARB *);
void (APIENTRY * bglGetActiveAttribARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
GLint(APIENTRY * bglGetAttribLocationARB)(GLhandleARB, const GLcharARB *);

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

const GLubyte *(APIENTRY * bgluErrorString)(GLenum error);

GLint(APIENTRY * bgluProject)(GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint	*view, GLdouble* winX, GLdouble* winY, GLdouble* winZ);
GLint (APIENTRY * bgluUnProject)(GLdouble winX, GLdouble winY, GLdouble winZ, const GLdouble * model, const GLdouble * proj, const GLint * view, GLdouble* objX, GLdouble* objY, GLdouble* objZ);


#ifdef RENDERTYPEWIN
// Windows
HGLRC(WINAPI * bwglCreateContext)(HDC);
BOOL (WINAPI * bwglDeleteContext)(HGLRC);
PROC(WINAPI * bwglGetProcAddress)(LPCSTR);
BOOL (WINAPI * bwglMakeCurrent)(HDC,HGLRC);

BOOL (WINAPI * bwglSwapBuffers)(HDC);
int32_t (WINAPI * bwglChoosePixelFormat)(HDC,CONST PIXELFORMATDESCRIPTOR*);
int32_t (WINAPI * bwglDescribePixelFormat)(HDC,int32_t,UINT,LPPIXELFORMATDESCRIPTOR);
int32_t (WINAPI * bwglGetPixelFormat)(HDC);
BOOL (WINAPI * bwglSetPixelFormat)(HDC,int32_t,const PIXELFORMATDESCRIPTOR*);
BOOL (WINAPI * bwglSwapIntervalEXT)(int32_t);

static HANDLE hGLDLL, hGLUDLL;
#else
#include <dlfcn.h>

static void *gluhandle = NULL;
#endif

char *gldriver = NULL, *glulibrary = NULL;

static void * getproc_(const char *s, int32_t *err, int32_t fatal, int32_t extension)
{
    void *t;
#if defined RENDERTYPESDL
    UNREFERENCED_PARAMETER(extension);
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

int32_t loadgldriver(const char *driver)
{
    int32_t err=0;

#ifdef RENDERTYPEWIN
    if (hGLDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "opengl32.dll";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
#else
        driver = "libGL.so.1";
#endif
    }

    initprintf("Loading %s\n",driver);

#if defined RENDERTYPESDL
    if (SDL_GL_LoadLibrary(driver)) return -1;
#elif defined _WIN32
    hGLDLL = LoadLibrary(driver);
    if (!hGLDLL) return -1;
#endif
    gldriver = Bstrdup(driver);

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
    bglClipPlane        = GETPROC("glClipPlane");

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
    bglTexCoord2i		=   GETPROC("glTexCoord2i");
    bglNormal3f		=       GETPROC("glNormal3f");

    // Lighting
    bglShadeModel		= GETPROC("glShadeModel");
    bglLightfv  		= GETPROC("glLightfv");

    // Raster funcs
    bglReadPixels		= GETPROC("glReadPixels");
    bglRasterPos4i		= GETPROC("glRasterPos4i");
    bglDrawPixels		= GETPROC("glDrawPixels");

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
    bglNormalPointer        = GETPROC("glNormalPointer");
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

int32_t loadglextensions(void)
{
    int32_t err = 0;
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
    bglBufferSubDataARB = GETPROCEXTSOFT("glBufferSubDataARB");
    bglMapBufferARB     = GETPROCEXTSOFT("glMapBufferARB");
    bglUnmapBufferARB   = GETPROCEXTSOFT("glUnmapBufferARB");

    // Occlusion queries
    bglGenQueriesARB        = GETPROCEXTSOFT("glGenQueriesARB");
    bglDeleteQueriesARB     = GETPROCEXTSOFT("glDeleteQueriesARB");
    bglIsQueryARB           = GETPROCEXTSOFT("glIsQueryARB");
    bglBeginQueryARB        = GETPROCEXTSOFT("glBeginQueryARB");
    bglEndQueryARB          = GETPROCEXTSOFT("glEndQueryARB");
    bglGetQueryivARB        = GETPROCEXTSOFT("glGetQueryivARB");
    bglGetQueryObjectivARB  = GETPROCEXTSOFT("glGetQueryObjectivARB");
    bglGetQueryObjectuivARB = GETPROCEXTSOFT("glGetQueryObjectuivARB");

    // Shader Objects
    bglDeleteObjectARB          = GETPROCEXTSOFT("glDeleteObjectARB");
    bglGetHandleARB             = GETPROCEXTSOFT("glGetHandleARB");
    bglDetachObjectARB          = GETPROCEXTSOFT("glDetachObjectARB");
    bglCreateShaderObjectARB    = GETPROCEXTSOFT("glCreateShaderObjectARB");
    bglShaderSourceARB          = GETPROCEXTSOFT("glShaderSourceARB");
    bglCompileShaderARB         = GETPROCEXTSOFT("glCompileShaderARB");
    bglCreateProgramObjectARB   = GETPROCEXTSOFT("glCreateProgramObjectARB");
    bglAttachObjectARB          = GETPROCEXTSOFT("glAttachObjectARB");
    bglLinkProgramARB           = GETPROCEXTSOFT("glLinkProgramARB");
    bglUseProgramObjectARB      = GETPROCEXTSOFT("glUseProgramObjectARB");
    bglValidateProgramARB       = GETPROCEXTSOFT("glValidateProgramARB");
    bglUniform1fARB             = GETPROCEXTSOFT("glUniform1fARB");
    bglUniform2fARB             = GETPROCEXTSOFT("glUniform2fARB");
    bglUniform3fARB             = GETPROCEXTSOFT("glUniform3fARB");
    bglUniform4fARB             = GETPROCEXTSOFT("glUniform4fARB");
    bglUniform1iARB             = GETPROCEXTSOFT("glUniform1iARB");
    bglUniform2iARB             = GETPROCEXTSOFT("glUniform2iARB");
    bglUniform3iARB             = GETPROCEXTSOFT("glUniform3iARB");
    bglUniform4iARB             = GETPROCEXTSOFT("glUniform4iARB");
    bglUniform1fvARB            = GETPROCEXTSOFT("glUniform1fvARB");
    bglUniform2fvARB            = GETPROCEXTSOFT("glUniform2fvARB");
    bglUniform3fvARB            = GETPROCEXTSOFT("glUniform3fvARB");
    bglUniform4fvARB            = GETPROCEXTSOFT("glUniform4fvARB");
    bglUniform1ivARB            = GETPROCEXTSOFT("glUniform1ivARB");
    bglUniform2ivARB            = GETPROCEXTSOFT("glUniform2ivARB");
    bglUniform3ivARB            = GETPROCEXTSOFT("glUniform3ivARB");
    bglUniform4ivARB            = GETPROCEXTSOFT("glUniform4ivARB");
    bglUniformMatrix2fvARB      = GETPROCEXTSOFT("glUniformMatrix2fvARB");
    bglUniformMatrix3fvARB      = GETPROCEXTSOFT("glUniformMatrix3fvARB");
    bglUniformMatrix4fvARB      = GETPROCEXTSOFT("glUniformMatrix4fvARB");
    bglGetObjectParameterfvARB  = GETPROCEXTSOFT("glGetObjectParameterfvARB");
    bglGetObjectParameterivARB  = GETPROCEXTSOFT("glGetObjectParameterivARB");
    bglGetInfoLogARB            = GETPROCEXTSOFT("glGetInfoLogARB");
    bglGetAttachedObjectsARB    = GETPROCEXTSOFT("glGetAttachedObjectsARB");
    bglGetUniformLocationARB    = GETPROCEXTSOFT("glGetUniformLocationARB");
    bglGetActiveUniformARB      = GETPROCEXTSOFT("glGetActiveUniformARB");
    bglGetUniformfvARB          = GETPROCEXTSOFT("glGetUniformfvARB");
    bglGetUniformivARB          = GETPROCEXTSOFT("glGetUniformivARB");
    bglGetShaderSourceARB       = GETPROCEXTSOFT("glGetShaderSourceARB");

    // Vertex Shaders
    bglVertexAttrib1dARB = GETPROCEXTSOFT("glVertexAttrib1dARB");
    bglVertexAttrib1dvARB = GETPROCEXTSOFT("glVertexAttrib1dvARB");
    bglVertexAttrib1fARB = GETPROCEXTSOFT("glVertexAttrib1fARB");
    bglVertexAttrib1fvARB = GETPROCEXTSOFT("glVertexAttrib1fvARB");
    bglVertexAttrib1sARB = GETPROCEXTSOFT("glVertexAttrib1sARB");
    bglVertexAttrib1svARB = GETPROCEXTSOFT("glVertexAttrib1svARB");
    bglVertexAttrib2dARB = GETPROCEXTSOFT("glVertexAttrib2dARB");
    bglVertexAttrib2dvARB = GETPROCEXTSOFT("glVertexAttrib2dvARB");
    bglVertexAttrib2fARB = GETPROCEXTSOFT("glVertexAttrib2fARB");
    bglVertexAttrib2fvARB = GETPROCEXTSOFT("glVertexAttrib2fvARB");
    bglVertexAttrib2sARB = GETPROCEXTSOFT("glVertexAttrib2sARB");
    bglVertexAttrib2svARB = GETPROCEXTSOFT("glVertexAttrib2svARB");
    bglVertexAttrib3dARB = GETPROCEXTSOFT("glVertexAttrib3dARB");
    bglVertexAttrib3dvARB = GETPROCEXTSOFT("glVertexAttrib3dvARB");
    bglVertexAttrib3fARB = GETPROCEXTSOFT("glVertexAttrib3fARB");
    bglVertexAttrib3fvARB = GETPROCEXTSOFT("glVertexAttrib3fvARB");
    bglVertexAttrib3sARB = GETPROCEXTSOFT("glVertexAttrib3sARB");
    bglVertexAttrib3svARB = GETPROCEXTSOFT("glVertexAttrib3svARB");
    bglVertexAttrib4NbvARB = GETPROCEXTSOFT("glVertexAttrib4NbvARB");
    bglVertexAttrib4NivARB = GETPROCEXTSOFT("glVertexAttrib4NivARB");
    bglVertexAttrib4NsvARB = GETPROCEXTSOFT("glVertexAttrib4NsvARB");
    bglVertexAttrib4NubARB = GETPROCEXTSOFT("glVertexAttrib4NubARB");
    bglVertexAttrib4NubvARB = GETPROCEXTSOFT("glVertexAttrib4NubvARB");
    bglVertexAttrib4NuivARB = GETPROCEXTSOFT("glVertexAttrib4NuivARB");
    bglVertexAttrib4NusvARB = GETPROCEXTSOFT("glVertexAttrib4NusvARB");
    bglVertexAttrib4bvARB = GETPROCEXTSOFT("glVertexAttrib4bvARB");
    bglVertexAttrib4dARB = GETPROCEXTSOFT("glVertexAttrib4dARB");
    bglVertexAttrib4dvARB = GETPROCEXTSOFT("glVertexAttrib4dvARB");
    bglVertexAttrib4fARB = GETPROCEXTSOFT("glVertexAttrib4fARB");
    bglVertexAttrib4fvARB = GETPROCEXTSOFT("glVertexAttrib4fvARB");
    bglVertexAttrib4ivARB = GETPROCEXTSOFT("glVertexAttrib4ivARB");
    bglVertexAttrib4sARB = GETPROCEXTSOFT("glVertexAttrib4sARB");
    bglVertexAttrib4svARB = GETPROCEXTSOFT("glVertexAttrib4svARB");
    bglVertexAttrib4ubvARB = GETPROCEXTSOFT("glVertexAttrib4ubvARB");
    bglVertexAttrib4uivARB = GETPROCEXTSOFT("glVertexAttrib4uivARB");
    bglVertexAttrib4usvARB = GETPROCEXTSOFT("glVertexAttrib4usvARB");
    bglVertexAttribPointerARB = GETPROCEXTSOFT("glVertexAttribPointerARB");
    bglEnableVertexAttribArrayARB = GETPROCEXTSOFT("glEnableVertexAttribArrayARB");
    bglDisableVertexAttribArrayARB = GETPROCEXTSOFT("glDisableVertexAttribArrayARB");
    bglGetVertexAttribdvARB = GETPROCEXTSOFT("glGetVertexAttribdvARB");
    bglGetVertexAttribfvARB = GETPROCEXTSOFT("glGetVertexAttribfvARB");
    bglGetVertexAttribivARB = GETPROCEXTSOFT("glGetVertexAttribivARB");
    bglGetVertexAttribPointervARB = GETPROCEXTSOFT("glGetVertexAttribPointervARB");
    bglBindAttribLocationARB = GETPROCEXTSOFT("glBindAttribLocationARB");
    bglGetActiveAttribARB = GETPROCEXTSOFT("glGetActiveAttribARB");
    bglGetAttribLocationARB = GETPROCEXTSOFT("glGetAttribLocationARB");

#ifdef RENDERTYPEWIN
    bwglSwapIntervalEXT	    = GETPROCEXTSOFT("wglSwapIntervalEXT");
#endif
    return err;
}

int32_t unloadgldriver(void)
{
    unloadglulibrary();

#ifdef RENDERTYPEWIN
    if (!hGLDLL) return 0;
#endif

    Bfree(gldriver);
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
    bglClipPlane        = NULL;

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
    bglTexCoord2i	= NULL;
    bglNormal3f		= NULL;

    // Lighting
    bglShadeModel		= NULL;
    bglLightfv			= NULL;

    // Raster funcs
    bglReadPixels		= NULL;
    bglRasterPos4i		= NULL;
    bglDrawPixels		= NULL;

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
    bglNormalPointer        = NULL;
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
    bglBufferSubDataARB = NULL;
    bglMapBufferARB     = NULL;
    bglUnmapBufferARB   = NULL;

    // Occlusion queries
    bglGenQueriesARB        = NULL;
    bglDeleteQueriesARB     = NULL;
    bglIsQueryARB           = NULL;
    bglBeginQueryARB        = NULL;
    bglEndQueryARB          = NULL;
    bglGetQueryivARB        = NULL;
    bglGetQueryObjectivARB  = NULL;
    bglGetQueryObjectuivARB = NULL;

    // Shader Objects
    bglDeleteObjectARB          = NULL;
    bglGetHandleARB             = NULL;
    bglDetachObjectARB          = NULL;
    bglCreateShaderObjectARB    = NULL;
    bglShaderSourceARB          = NULL;
    bglCompileShaderARB         = NULL;
    bglCreateProgramObjectARB   = NULL;
    bglAttachObjectARB          = NULL;
    bglLinkProgramARB           = NULL;
    bglUseProgramObjectARB      = NULL;
    bglValidateProgramARB       = NULL;
    bglUniform1fARB             = NULL;
    bglUniform2fARB             = NULL;
    bglUniform3fARB             = NULL;
    bglUniform4fARB             = NULL;
    bglUniform1iARB             = NULL;
    bglUniform2iARB             = NULL;
    bglUniform3iARB             = NULL;
    bglUniform4iARB             = NULL;
    bglUniform1fvARB            = NULL;
    bglUniform2fvARB            = NULL;
    bglUniform3fvARB            = NULL;
    bglUniform4fvARB            = NULL;
    bglUniform1ivARB            = NULL;
    bglUniform2ivARB            = NULL;
    bglUniform3ivARB            = NULL;
    bglUniform4ivARB            = NULL;
    bglUniformMatrix2fvARB      = NULL;
    bglUniformMatrix3fvARB      = NULL;
    bglUniformMatrix4fvARB      = NULL;
    bglGetObjectParameterfvARB  = NULL;
    bglGetObjectParameterivARB  = NULL;
    bglGetInfoLogARB            = NULL;
    bglGetAttachedObjectsARB    = NULL;
    bglGetUniformLocationARB    = NULL;
    bglGetActiveUniformARB      = NULL;
    bglGetUniformfvARB          = NULL;
    bglGetUniformivARB          = NULL;
    bglGetShaderSourceARB       = NULL;

    // Vertex Shaders
    bglVertexAttrib1dARB = NULL;
    bglVertexAttrib1dvARB = NULL;
    bglVertexAttrib1fARB = NULL;
    bglVertexAttrib1fvARB = NULL;
    bglVertexAttrib1sARB = NULL;
    bglVertexAttrib1svARB = NULL;
    bglVertexAttrib2dARB = NULL;
    bglVertexAttrib2dvARB = NULL;
    bglVertexAttrib2fARB = NULL;
    bglVertexAttrib2fvARB = NULL;
    bglVertexAttrib2sARB = NULL;
    bglVertexAttrib2svARB = NULL;
    bglVertexAttrib3dARB = NULL;
    bglVertexAttrib3dvARB = NULL;
    bglVertexAttrib3fARB = NULL;
    bglVertexAttrib3fvARB = NULL;
    bglVertexAttrib3sARB = NULL;
    bglVertexAttrib3svARB = NULL;
    bglVertexAttrib4NbvARB = NULL;
    bglVertexAttrib4NivARB = NULL;
    bglVertexAttrib4NsvARB = NULL;
    bglVertexAttrib4NubARB = NULL;
    bglVertexAttrib4NubvARB = NULL;
    bglVertexAttrib4NuivARB = NULL;
    bglVertexAttrib4NusvARB = NULL;
    bglVertexAttrib4bvARB = NULL;
    bglVertexAttrib4dARB = NULL;
    bglVertexAttrib4dvARB = NULL;
    bglVertexAttrib4fARB = NULL;
    bglVertexAttrib4fvARB = NULL;
    bglVertexAttrib4ivARB = NULL;
    bglVertexAttrib4sARB = NULL;
    bglVertexAttrib4svARB = NULL;
    bglVertexAttrib4ubvARB = NULL;
    bglVertexAttrib4uivARB = NULL;
    bglVertexAttrib4usvARB = NULL;
    bglVertexAttribPointerARB = NULL;
    bglEnableVertexAttribArrayARB = NULL;
    bglDisableVertexAttribArrayARB = NULL;
    bglGetVertexAttribdvARB = NULL;
    bglGetVertexAttribfvARB = NULL;
    bglGetVertexAttribivARB = NULL;
    bglGetVertexAttribPointervARB = NULL;
    bglBindAttribLocationARB = NULL;
    bglGetActiveAttribARB = NULL;
    bglGetAttribLocationARB = NULL;

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
    bwglSwapIntervalEXT	= NULL;
#endif

    return 0;
}

static void * glugetproc_(const char *s, int32_t *err, int32_t fatal)
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

int32_t loadglulibrary(const char *driver)
{
    int32_t err=0;

#ifdef RENDERTYPEWIN
    if (hGLUDLL) return 0;
#endif

    if (!driver)
    {
#ifdef _WIN32
        driver = "glu32.dll";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL"; // FIXME: like I know anything about Apple.  Hah.
#else
        driver = "libGLU.so.1";
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
    glulibrary = Bstrdup(driver);

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
    bgluUnProject = GLUGETPROC("gluUnProject");

    if (err) unloadglulibrary();
    return err;
}

int32_t unloadglulibrary(void)
{
#ifdef RENDERTYPEWIN
    if (!hGLUDLL) return 0;
#endif

    Bfree(glulibrary);
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
    bgluUnProject             = NULL;

    return 0;
}
#endif

