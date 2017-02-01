/* xscreensaver, Copyright (c) 2012-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* A compatibility shim to allow OpenGL 1.3 source code to work in an
   OpenGLES environment, where almost every OpenGL 1.3 function has
   been "deprecated".  See jwzgles.c for details.
 */

#ifndef __JWZGLES_I_H__
#define __JWZGLES_I_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GL_VERSION_ES_CM_1_0  /* compiling against OpenGLES 1.x */

/* These OpenGL 1.3 constants are not present in OpenGLES 1.
   Fortunately, it looks like they didn't re-use any of the numbers,
   so we can just keep using the OpenGL 1.3 values.  I'm actually
   kind of shocked that the GLES folks passed up that opportunity
   for further clusterfuckery.
 */
# define GLdouble double

# define GL_ACCUM_BUFFER_BIT			0x00000200
# undef GL_ALL_ATTRIB_BITS
# define GL_ALL_ATTRIB_BITS			0x000FFFFF
# define GL_AUTO_NORMAL				0x0D80
# define GL_BLEND_SRC_ALPHA			0x80CB
# define GL_C3F_V3F				0x2A24
# define GL_C4F_N3F_V3F				0x2A26
# define GL_C4UB_V2F				0x2A22
# define GL_C4UB_V3F				0x2A23
# define GL_CLAMP				0x2900
# define GL_COLOR_BUFFER_BIT			0x00004000
# define GL_COLOR_MATERIAL_FACE			0x0B55
# define GL_COLOR_MATERIAL_PARAMETER		0x0B56
# define GL_COMPILE				0x1300
# define GL_CURRENT_BIT				0x00000001
# define GL_DEPTH_BUFFER_BIT			0x00000100
# define GL_DOUBLEBUFFER			0x0C32
# define GL_ENABLE_BIT				0x00002000
# define GL_EVAL_BIT				0x00010000
# define GL_EYE_LINEAR				0x2400
# define GL_EYE_PLANE				0x2502
# define GL_FEEDBACK				0x1C01
# define GL_FILL				0x1B02
# define GL_FOG_BIT				0x00000080
# define GL_HINT_BIT				0x00008000
# define GL_INTENSITY				0x8049
# define GL_LIGHTING_BIT			0x00000040
# define GL_LIGHT_MODEL_COLOR_CONTROL		0x81F8
# define GL_LIGHT_MODEL_LOCAL_VIEWER		0x0B51
# define GL_LINE				0x1B01
# define GL_LINE_BIT				0x00000004
# define GL_LIST_BIT				0x00020000
# define GL_N3F_V3F				0x2A25
# define GL_OBJECT_LINEAR			0x2401
# define GL_OBJECT_PLANE			0x2501
# define GL_PIXEL_MODE_BIT			0x00000020
# define GL_POINT_BIT				0x00000002
# define GL_POLYGON				0x0009
# define GL_POLYGON_BIT				0x00000008
# define GL_POLYGON_MODE			0x0B40
# define GL_POLYGON_SMOOTH			0x0B41
# define GL_POLYGON_STIPPLE			0x0B42
# define GL_POLYGON_STIPPLE_BIT			0x00000010
# define GL_Q					0x2003
# define GL_QUADS				0x0007
# define GL_QUAD_STRIP				0x0008
# define GL_R					0x2002
# define GL_RENDER				0x1C00
# define GL_RGBA_MODE				0x0C31
# define GL_S					0x2000
# define GL_SCISSOR_BIT				0x00080000
# define GL_SELECT				0x1C02
# define GL_SEPARATE_SPECULAR_COLOR		0x81FA
# define GL_SINGLE_COLOR			0x81F9
# define GL_SPHERE_MAP				0x2402
# define GL_STENCIL_BUFFER_BIT			0x00000400
# define GL_T					0x2001
# define GL_T2F_C3F_V3F				0x2A2A
# define GL_T2F_C4F_N3F_V3F			0x2A2C
# define GL_T2F_C4UB_V3F			0x2A29
# define GL_T2F_N3F_V3F				0x2A2B
# define GL_T2F_V3F				0x2A27
# define GL_T4F_C4F_N3F_V4F			0x2A2D
# define GL_T4F_V4F				0x2A28
# define GL_TEXTURE_1D				0x0DE0
# define GL_TEXTURE_ALPHA_SIZE			0x805F
# define GL_TEXTURE_BIT				0x00040000
# define GL_TEXTURE_BLUE_SIZE			0x805E
# define GL_TEXTURE_BORDER			0x1005
# define GL_TEXTURE_BORDER_COLOR		0x1004
# define GL_TEXTURE_COMPONENTS			0x1003
# define GL_TEXTURE_GEN_MODE			0x2500
# define GL_TEXTURE_GEN_Q			0x0C63
# define GL_TEXTURE_GEN_R			0x0C62
# define GL_TEXTURE_GEN_S			0x0C60
# define GL_TEXTURE_GEN_T			0x0C61
# define GL_TEXTURE_GREEN_SIZE			0x805D
# define GL_TEXTURE_HEIGHT			0x1001
# define GL_TEXTURE_INTENSITY_SIZE		0x8061
# define GL_TEXTURE_LUMINANCE_SIZE		0x8060
# define GL_TEXTURE_RED_SIZE			0x805C
# define GL_TEXTURE_WIDTH			0x1000
# define GL_TRANSFORM_BIT			0x00001000
# define GL_UNPACK_ROW_LENGTH			0x0CF2
# define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
# define GL_V2F					0x2A20
# define GL_V3F					0x2A21
# define GL_VIEWPORT_BIT			0x00000800
# define GL_INT					0x1404
# define GL_DOUBLE				0x140A

#endif


extern void jwzgles_reset (void);


/* Prototypes for the things re-implemented in jwzgles.c 
 */

extern int  jwzgles_glGenLists (int n);
extern void jwzgles_glNewList (int id, int mode);
extern void jwzgles_glEndList (void);
extern void jwzgles_glDeleteLists (int list, int range);
extern void jwzgles_glBegin (int mode);
extern void jwzgles_glNormal3fv (const GLfloat *);
extern void jwzgles_glNormal3f (GLfloat x, GLfloat y, GLfloat z);
extern void jwzgles_glTexCoord1f (GLfloat s);
extern void jwzgles_glTexCoord2fv (const GLfloat *);
extern void jwzgles_glTexCoord2f (GLfloat s, GLfloat t);
extern void jwzgles_glTexCoord2i (GLint s, GLint t);
extern void jwzgles_glTexCoord3fv (const GLfloat *);
extern void jwzgles_glTexCoord3f (GLfloat s, GLfloat t, GLfloat r);
extern void jwzgles_glTexCoord4fv (const GLfloat *);
extern void jwzgles_glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void jwzgles_glVertex2f (GLfloat x, GLfloat y);
extern void jwzgles_glVertex2fv (const GLfloat *);
extern void jwzgles_glVertex2i (GLint x, GLint y);
extern void jwzgles_glVertex3f (GLfloat x, GLfloat y, GLfloat z);
extern void jwzgles_glVertex3dv (const GLdouble *);
extern void jwzgles_glVertex3fv (const GLfloat *);
extern void jwzgles_glVertex3i (GLint x, GLint y, GLint z);
extern void jwzgles_glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void jwzgles_glVertex4fv (const GLfloat *);
extern void jwzgles_glVertex4i (GLint x, GLint y, GLint z, GLint w);
extern void jwzgles_glEnd (void);
extern void jwzgles_glCallList (int id);
extern void jwzgles_glClearIndex(GLfloat c);
extern void jwzgles_glBitmap (GLsizei, GLsizei, GLfloat, GLfloat, GLfloat,
                              GLfloat, const GLubyte *);
extern void jwzgles_glPushAttrib(int);
extern void jwzgles_glPopAttrib(void);


/* These functions are present in both OpenGL 1.3 and in OpenGLES 1,
   but are allowed within glNewList/glEndList, so we must wrap them
   to allow them to be recorded.
 */
extern void jwzgles_glActiveTexture (GLuint);
extern void jwzgles_glBindTexture (GLuint, GLuint);
extern void jwzgles_glBlendFunc (GLuint, GLuint);
extern void jwzgles_glClear (GLuint);
extern void jwzgles_glClearColor (GLclampf, GLclampf, GLclampf, GLclampf);
extern void jwzgles_glClearStencil (GLuint);
extern void jwzgles_glColorMask (GLuint, GLuint, GLuint, GLuint);
extern void jwzgles_glCullFace (GLuint);
extern void jwzgles_glDepthFunc (GLuint);
extern void jwzgles_glDepthMask (GLuint);
extern void jwzgles_glDisable (GLuint);
extern void jwzgles_glDrawArrays (GLuint, GLuint, GLuint);
extern GLboolean jwzgles_glIsEnabled (GLuint);
extern void jwzgles_glEnable (GLuint);
extern void jwzgles_glFrontFace (GLuint);
extern void jwzgles_glHint (GLuint, GLuint);
extern void jwzgles_glLineWidth (GLfloat);
extern void jwzgles_glLoadIdentity (void);
extern void jwzgles_glLogicOp (GLuint);
extern void jwzgles_glMatrixMode (GLuint);
extern void jwzgles_glMultMatrixf (const GLfloat *);
extern void jwzgles_glLoadMatrixf (const GLfloat *);
extern void jwzgles_glPointSize (GLfloat);
extern void jwzgles_glPolygonOffset (GLfloat, GLfloat);
extern void jwzgles_glPopMatrix (void);
extern void jwzgles_glPushMatrix (void);
extern void jwzgles_glScissor (GLuint, GLuint, GLuint, GLuint);
extern void jwzgles_glShadeModel (GLuint);
extern void jwzgles_glStencilFunc (GLuint, GLuint, GLuint);
extern void jwzgles_glStencilMask (GLuint);
extern void jwzgles_glStencilOp (GLuint, GLuint, GLuint);
extern void jwzgles_glViewport (GLuint, GLuint, GLuint, GLuint);
extern void jwzgles_glTranslatef (GLfloat, GLfloat, GLfloat);
extern void jwzgles_glRotatef (GLfloat, GLfloat, GLfloat, GLfloat);
extern void jwzgles_glRotated (GLdouble, GLdouble x, GLdouble y, GLdouble z);
extern void jwzgles_glScalef (GLfloat, GLfloat, GLfloat);
extern void jwzgles_glColor3f (GLfloat, GLfloat, GLfloat);
extern void jwzgles_glColor4f (GLfloat, GLfloat, GLfloat, GLfloat);
extern void jwzgles_glColor3fv (const GLfloat *);
extern void jwzgles_glColor4fv (const GLfloat *);
extern void jwzgles_glColor3s (GLshort, GLshort, GLshort);
extern void jwzgles_glColor4s (GLshort, GLshort, GLshort, GLshort);
extern void jwzgles_glColor3sv (const GLshort *);
extern void jwzgles_glColor4sv (const GLshort *);
extern void jwzgles_glColor3us (GLushort, GLushort, GLushort);
extern void jwzgles_glColor4us (GLushort, GLushort, GLushort, GLushort);
extern void jwzgles_glColor3usv (const GLushort *);
extern void jwzgles_glColor4usv (const GLushort *);
extern void jwzgles_glColor3d (GLdouble, GLdouble, GLdouble);
extern void jwzgles_glColor4d (GLdouble, GLdouble, GLdouble, GLdouble);
extern void jwzgles_glColor3dv (const GLdouble *);
extern void jwzgles_glColor4dv (const GLdouble *);
extern void jwzgles_glColor4i (GLint, GLint, GLint, GLint);
extern void jwzgles_glColor3i (GLint, GLint, GLint);
extern void jwzgles_glColor3iv (const GLint *);
extern void jwzgles_glColor4iv (const GLint *);
extern void jwzgles_glColor4ui (GLuint, GLuint, GLuint, GLuint);
extern void jwzgles_glColor3ui (GLuint, GLuint, GLuint);
extern void jwzgles_glColor3uiv (const GLuint *);
extern void jwzgles_glColor4uiv (const GLuint *);
extern void jwzgles_glColor4b (GLbyte, GLbyte, GLbyte, GLbyte);
extern void jwzgles_glColor3b (GLbyte, GLbyte, GLbyte);
extern void jwzgles_glColor4bv (const GLbyte *);
extern void jwzgles_glColor3bv (const GLbyte *);
extern void jwzgles_glColor4ub (GLubyte, GLubyte, GLubyte, GLubyte);
extern void jwzgles_glColor3ub (GLubyte, GLubyte, GLubyte);
extern void jwzgles_glColor4ubv (const GLubyte *);
extern void jwzgles_glColor3ubv (const GLubyte *);
extern void jwzgles_glMaterialf (GLuint, GLuint, GLfloat);
extern void jwzgles_glMateriali (GLuint, GLuint, GLuint);
extern void jwzgles_glMaterialfv (GLuint, GLuint, const GLfloat *);
extern void jwzgles_glMaterialiv (GLuint, GLuint, const GLint *);
extern void jwzgles_glFinish (void);
extern void jwzgles_glFlush (void);
extern void jwzgles_glPixelStorei (GLuint, GLuint);
extern void jwzgles_glEnableClientState (GLuint);
extern void jwzgles_glDisableClientState (GLuint);

extern void jwzgles_glInitNames (void);
extern void jwzgles_glPushName (GLuint);
extern GLuint jwzgles_glPopName (void);
extern GLuint jwzgles_glRenderMode (GLuint);
extern void jwzgles_glSelectBuffer (GLsizei, GLuint *);
extern void jwzgles_glLightf (GLenum, GLenum, GLfloat);
extern void jwzgles_glLighti (GLenum, GLenum, GLint);
extern void jwzgles_glLightfv (GLenum, GLenum, const GLfloat *);
extern void jwzgles_glLightiv (GLenum, GLenum, const GLint *);
extern void jwzgles_glLightModelf (GLenum, GLfloat);
extern void jwzgles_glLightModeli (GLenum, GLint);
extern void jwzgles_glLightModelfv (GLenum, const GLfloat *);
extern void jwzgles_glLightModeliv (GLenum, const GLint *);
extern void jwzgles_glGenTextures (GLuint, GLuint *);
extern void jwzgles_glFrustum (GLfloat, GLfloat, GLfloat, GLfloat,
                               GLfloat, GLfloat);
extern void jwzgles_glOrtho (GLfloat, GLfloat, GLfloat, GLfloat, 
                             GLfloat, GLfloat);
extern void jwzgles_glTexImage1D (GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLsizei width, GLint border,
                                  GLenum format, GLenum type,
                                  const GLvoid *pixels);
extern void jwzgles_glTexImage2D (GLenum target,
                                  GLint  	level,
                                  GLint  	internalFormat,
                                  GLsizei  	width,
                                  GLsizei  	height,
                                  GLint  	border,
                                  GLenum  	format,
                                  GLenum  	type,
                                  const GLvoid *data);
extern void jwzgles_glTexSubImage2D (GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format, GLenum type,
                                     const GLvoid *pixels);
extern void jwzgles_glCompressedTexImage2D (GLenum target,
                                            GLint  	level,
                                            GLint  	internalFormat,
                                            GLsizei  	width,
                                            GLsizei  	height,
                                            GLint  	border,
                                            GLsizei imageSize,
                                            const GLvoid *data);
extern void jwzgles_glCompressedTexSubImage2D (GLenum target, GLint level,
                                               GLint xoffset, GLint yoffset,
                                               GLsizei width, GLsizei height,
                                               GLenum format, GLsizei imageSize,
                                               const GLvoid *pixels);
extern void jwzgles_glCopyTexImage2D (GLenum target, GLint level, 
                                      GLenum internalformat,
                                      GLint x, GLint y, 
                                      GLsizei width, GLsizei height, 
                                      GLint border);
extern void jwzgles_glInterleavedArrays (GLenum, GLsizei, const GLvoid *);
extern void jwzgles_glTexEnvf (GLuint, GLuint, GLfloat);
extern void jwzgles_glTexEnvi (GLuint, GLuint, GLuint);
extern void jwzgles_glTexParameterf (GLuint, GLuint, GLfloat);
extern void jwzgles_glTexParameteri (GLuint, GLuint, GLuint);
extern void jwzgles_glTexGeni (GLenum, GLenum, GLint);
extern void jwzgles_glTexGenfv (GLenum, GLenum, const GLfloat *);
extern void jwzgles_glGetTexGenfv (GLenum, GLenum, GLfloat *);
extern void jwzgles_glRectf (GLfloat, GLfloat, GLfloat, GLfloat);
extern void jwzgles_glRecti (GLint, GLint, GLint, GLint);
extern void jwzgles_glLightModelfv (GLenum, const GLfloat *);
extern void jwzgles_glClearDepth (GLfloat);
extern GLboolean jwzgles_glIsList (GLuint);
extern void jwzgles_glColorMaterial (GLenum, GLenum);
extern void jwzgles_glPolygonMode (GLenum, GLenum);
extern void jwzgles_glFogf (GLenum, GLfloat);
extern void jwzgles_glFogi (GLenum, GLint);
extern void jwzgles_glFogfv (GLenum, const GLfloat *);
extern void jwzgles_glFogiv (GLenum, const GLint *);
extern void jwzgles_glAlphaFunc (GLenum, GLfloat);
extern void jwzgles_glClipPlane (GLenum, const GLdouble *);
extern void jwzgles_glDrawBuffer (GLenum);
extern void jwzgles_glDeleteTextures (GLuint, const GLuint *);

extern void jwzgles_gluPerspective (GLdouble fovy, GLdouble aspect, 
                                    GLdouble near, GLdouble far);
extern void jwzgles_gluLookAt (GLfloat eyex, GLfloat eyey, GLfloat eyez,
                               GLfloat centerx, GLfloat centery, 
                               GLfloat centerz,
                               GLfloat upx, GLfloat upy, GLfloat upz);
extern GLint jwzgles_gluProject (GLdouble objx, GLdouble objy, GLdouble objz, 
                                 const GLdouble modelMatrix[16], 
                                 const GLdouble projMatrix[16],
                                 const GLint viewport[4],
                                 GLdouble *winx, GLdouble *winy, 
                                 GLdouble *winz);
extern int jwzgles_gluBuild2DMipmaps (GLenum target,
                                      GLint internalFormat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLenum format,
                                      GLenum type,
                                      const GLvoid *data);
extern void jwzgles_glGetFloatv (GLenum pname, GLfloat *params);
extern void jwzgles_glGetPointerv (GLenum pname, GLvoid *params);
extern void jwzgles_glGetDoublev (GLenum pname, GLdouble *params);
extern void jwzgles_glGetIntegerv (GLenum pname, GLint *params);
extern void jwzgles_glGetBooleanv (GLenum pname, GLboolean *params);
extern void jwzgles_glVertexPointer (GLuint, GLuint, GLuint, const void *);
extern void jwzgles_glNormalPointer (GLenum, GLuint, const void *);
extern void jwzgles_glColorPointer (GLuint, GLuint, GLuint, const void *);
extern void jwzgles_glTexCoordPointer (GLuint, GLuint, GLuint, const void *);
extern void jwzgles_glBindBuffer (GLuint, GLuint);
extern void jwzgles_glBufferData (GLenum, GLsizeiptr, const void *, GLenum);
extern const char *jwzgles_gluErrorString (GLenum error);

extern GLenum jwzgles_glGetError();
extern const GLubyte * jwzgles_glGetString(GLenum name);


// typedef float GLclampd;

extern void jwzgles_glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void jwzgles_glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void jwzgles_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
    
#ifdef __cplusplus
}
#endif

#endif /* __JWZGLES_I_H__ */
