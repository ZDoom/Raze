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
   been "deprecated".

   There are two major operations going on here:

     - Converting calls to glBegin + glVertex3f + glEnd to glDrawArrays
     - Implementing display lists.

   So this code shadows all of the functions that are allowed within
   glBegin, builds up an array, and calls glDrawArrays at the end.

   Likewise, it shadows all of the functions that are allowed within
   glNewList and records those calls for later playback.


   This code only handles OpenGLES 1.x, not 2.x.


   Some things that are missing:

    - glTexGeni, meaning no spherical environment-mapping textures.

    - gluNewTess, meaning no tesselation of complex objects.

    - glMap2f mesh evaluators, meaning no Utah Teapot.

    - glPolygonMode with GL_LINE or GL_POINT, meaning no wireframe modes
      that do hidden-surface removal.

    - glSelectBuffer, meaning no mouse-hit detection on rendered objects.

    - gluNewQuadric, gluCylinder, etc: rewrite your code to use tube.c, etc.

    - Putting verts in a display list without a wrapping glBegin.
      I didn't realize that even worked!  Lockward used to do that,
      before I fixed it to not.

    - Not every function is implemented; just the ones that I needed for
      xscreensaver.  However, the trivial ones are trivial to enable
      as they come up.  Harder ones will be harder.
*/


#undef DEBUG
// #define DEBUG 1


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_JWZGLES	/* whole file */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */


#ifdef __APPLE__
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#  define USE_IPHONE
# endif
#endif

#if defined(USE_IPHONE)
# include <OpenGLES/ES1/gl.h>
# include <OpenGLES/ES1/glext.h>
#elif defined(HAVE_COCOA)
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#elif defined(HAVE_ANDROID)
# include <GLES/gl.h>
#else /* real X11 */
# ifndef  GL_GLEXT_PROTOTYPES
#  define GL_GLEXT_PROTOTYPES /* for glBindBuffer */
# endif
# include <GL/glx.h>
# include <GL/glu.h>
#endif

#include "jwzglesI.h"

#if !defined(_MSC_VER) || _MSC_FULL_VER < 180031101
# undef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

#define STRINGIFY(X) #X

#undef countof
#define countof(x) (signed)((signed)sizeof((x))/(signed)sizeof((*x)))

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "JWZGLES"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif

#undef  Assert

#ifdef HAVE_COCOA
  extern void jwxyz_abort (const char *fmt, ...) __dead2;
# define Have_Assert
# define Assert(C,S) do { if (!(C)) { jwxyz_abort ("%s",S); }} while(0)
#elif defined __ANDROID__
# define Have_Assert
# define Assert(C,S) do { \
    if (!(C)) { \
    	LOGE ( "ASSERT jwzgles: %s\n", S); \
    }} while(0)
#else
# define Assert(C,S)
#endif


typedef struct { GLfloat x, y, z; }    XYZ;
typedef struct { GLfloat x, y, z, w; } XYZW;
typedef struct { GLfloat s, t, r, q; } STRQ;
typedef struct { GLfloat r, g, b, a; } RGBA;


/* Used to record all calls to glVertex3f, glNormal3f, etc.
   while inside glBegin / glEnd so that we can convert that
   to a single call to glDrawArrays.
 */
typedef struct {
  int mode;
  int count, size;	/* size of each array */

  XYZW *verts;		/* Arrays being built */
  XYZ  *norms;
  STRQ *tex;
  RGBA *color;

  int ncount;		/* How many normals, tex coords and colors were */
  int tcount;		/* used.  We optimize based on "0, 1, or many". */
  int ccount;
  int materialistic;	/* Whether glMaterial was called inside glBegin */

  XYZ  cnorm;		/* Prevailing normal/texture/color while building */
  STRQ ctex;
  RGBA ccolor;

} vert_set;


typedef void (*list_fn_cb) (void);


/* We need this nonsense because you can't cast a double to a void*
   or vice versa.  They tend to be passed in different registers,
   and you need to know about that because it's still 1972 here.
 */
typedef union {
  const void *v; GLfloat f; GLuint i; GLshort s; GLdouble d;
} void_int;

typedef struct {		/* saved args for glDrawArrays */
  int binding, size, type, stride, bytes;
  void *data;
} draw_array;

typedef enum {			/* shorthand describing arglist signature */
  PROTO_VOID,	/* no args */
  PROTO_I,	/* 1 int arg */
  PROTO_F,	/* 1 float arg */
  PROTO_II,	/* int, int */
  PROTO_FF,	/* float, float */
  PROTO_IF,	/* int, float */
  PROTO_III,	/* int, int, int */
  PROTO_FFF,	/* float, float, float */
  PROTO_IIF,	/* int, int, float */
  PROTO_IIII,	/* int, int, int, int */
  PROTO_FFFF,	/* float, float, float, float */
  PROTO_IIV,	/* int, int[4] */
  PROTO_IFV,	/* int, float[4] */
  PROTO_IIIV,	/* int, int, int[4] */
  PROTO_IIFV,	/* int, int, float[4] */
  PROTO_FV16,	/* float[16] */
  PROTO_ARRAYS	/* glDrawArrays */
} fn_proto;

typedef struct {		/* A single element of a display list */
  const char *name;
  list_fn_cb fn;		/* saved function pointer */
  fn_proto proto;		/* arglist prototype */
  draw_array *arrays;		/* args for glDrawArrays */
  void_int argv[16];		/* args for everything else */
} list_fn;


typedef struct {	/* a display list: saved activity within glNewList */
  int id;
  int size, count;
  list_fn *fns;

  /* Named buffer that should be freed when this display list is deleted. */
  GLuint buffer;

} list;


typedef struct {	/* All display lists */
  list *lists;
  int count, size;
} list_set;


#define ISENABLED_TEXTURE_2D	(1<<0)
#define ISENABLED_TEXTURE_GEN_S	(1<<1)
#define ISENABLED_TEXTURE_GEN_T	(1<<2)
#define ISENABLED_TEXTURE_GEN_R	(1<<3)
#define ISENABLED_TEXTURE_GEN_Q	(1<<4)
#define ISENABLED_LIGHTING	(1<<5)
#define ISENABLED_BLEND		(1<<6)
#define ISENABLED_DEPTH_TEST	(1<<7)
#define ISENABLED_CULL_FACE	(1<<8)
#define ISENABLED_NORMALIZE	(1<<9)
#define ISENABLED_FOG		(1<<10)
#define ISENABLED_COLMAT	(1<<11)
#define ISENABLED_VERT_ARRAY	(1<<12)
#define ISENABLED_NORM_ARRAY	(1<<13)
#define ISENABLED_TEX_ARRAY	(1<<14)
#define ISENABLED_COLOR_ARRAY	(1<<15)
#define ISENABLED_ALPHA_TEST	(1<<16)


typedef struct {
  GLuint mode;
  GLfloat obj[4], eye[4];
} texgen_state;


typedef struct {	/* global state */

  vert_set set;		/* set being built */

  int compiling_list;	/* list id if inside glNewList; 0 means immediate */
  int replaying_list;	/* depth of call stack to glCallList */
  int compiling_verts;	/* inside glBegin */

  list_set lists;	/* saved lists */

  unsigned long enabled;	/* enabled flags, immediate mode */
  unsigned long list_enabled;	/* and for the list-in-progress */

  texgen_state s, t, r, q;

} jwzgles_state;


static jwzgles_state *state = 0;

#ifdef DEBUG
# define LOG(A)                LOGD("jwzgles: " A "\n")
# define LOG1(A,B)             LOGD("jwzgles: " A "\n",B)
# define LOG2(A,B,C)           LOGD("jwzgles: " A "\n",B,C)
# define LOG3(A,B,C,D)         LOGD("jwzgles: " A "\n",B,C,D)
# define LOG4(A,B,C,D,E)       LOGD("jwzgles: " A "\n",B,C,D,E)
# define LOG5(A,B,C,D,E,F)     LOGD("jwzgles: " A "\n",B,C,D,E,F)
# define LOG6(A,B,C,D,E,F,G)   LOGD("jwzgles: " A "\n",B,C,D,E,F,G)
# define LOG7(A,B,C,D,E,F,G,H) LOGD("jwzgles: " A "\n",B,C,D,E,F,G,H)
# define LOG8(A,B,C,D,E,F,G,H,I) \
         LOGD("jwzgles: " A "\n",B,C,D,E,F,G,H,I)
# define LOG9(A,B,C,D,E,F,G,H,I,J) \
         LOGD("jwzgles: " A "\n",B,C,D,E,F,G,H,I,J)
# define LOG10(A,B,C,D,E,F,G,H,I,J,K) \
         LOGD("jwzgles: " A "\n",B,C,D,E,F,G,H,I,J,K)
# define LOG17(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R) \
         LOGD("jwzgles: " A "\n",B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R)
# define CHECK(S) check_gl_error(S)
#else
static inline void jklmnop_donothing(void) { };
# define LOG(A)                                     jklmnop_donothing()
# define LOG1(A,B)                                  jklmnop_donothing()
# define LOG2(A,B,C)                                jklmnop_donothing()
# define LOG3(A,B,C,D)                              jklmnop_donothing()
# define LOG4(A,B,C,D,E)                            jklmnop_donothing()
# define LOG5(A,B,C,D,E,F)                          jklmnop_donothing()
# define LOG6(A,B,C,D,E,F,G)                        jklmnop_donothing()
# define LOG7(A,B,C,D,E,F,G,H)                      jklmnop_donothing()
# define LOG8(A,B,C,D,E,F,G,H,I)                    jklmnop_donothing()
# define LOG9(A,B,C,D,E,F,G,H,I,J)                  jklmnop_donothing()
# define LOG10(A,B,C,D,E,F,G,H,I,J,K)               jklmnop_donothing()
# define LOG17(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R) jklmnop_donothing()
# define CHECK(S)                                   jklmnop_donothing()
#endif

#ifdef DEBUG
static const char *
mode_desc (int mode)	/* for debugging messages */
{
  switch (mode) {
# define SS(i)(X) case GL_##X: return STRINGIFY(X);
  SS(i)(ALPHA)
  SS(i)(ALPHA_TEST)
  SS(i)(AMBIENT)
  SS(i)(AMBIENT_AND_DIFFUSE)
  SS(i)(ARRAY_BUFFER)
  SS(i)(AUTO_NORMAL)
  SS(i)(BACK)
  SS(i)(BLEND)
  SS(i)(BLEND_DST)
  SS(i)(BLEND_SRC)
  SS(i)(BLEND_SRC_ALPHA)
  SS(i)(BYTE)
  SS(i)(C3F_V3F)
  SS(i)(C4F_N3F_V3F)
  SS(i)(C4UB_V2F)
  SS(i)(C4UB_V3F)
  SS(i)(CCW)
  SS(i)(CLAMP)
  SS(i)(COLOR_ARRAY)
  SS(i)(COLOR_ARRAY_BUFFER_BINDING);
  SS(i)(COLOR_MATERIAL)
  SS(i)(COLOR_MATERIAL_FACE)
  SS(i)(COLOR_MATERIAL_PARAMETER)
  SS(i)(COMPILE)
  SS(i)(CULL_FACE)
  SS(i)(CW)
  SS(i)(DECAL)
  SS(i)(DEPTH_BUFFER_BIT)
  SS(i)(DEPTH_TEST)
  SS(i)(DIFFUSE)
  SS(i)(DOUBLEBUFFER)
  SS(i)(DST_ALPHA)
  SS(i)(DST_COLOR)
  SS(i)(DYNAMIC_DRAW)
  SS(i)(ELEMENT_ARRAY_BUFFER)
  SS(i)(EYE_LINEAR)
  SS(i)(EYE_PLANE)
  SS(i)(FEEDBACK)
  SS(i)(FILL)
  SS(i)(FLAT)
  SS(i)(FLOAT)
  SS(i)(FOG)
  SS(i)(FRONT)
  SS(i)(FRONT_AND_BACK)
  SS(i)(GREATER)
  SS(i)(INTENSITY)
  SS(i)(INVALID_ENUM)
  SS(i)(INVALID_OPERATION)
  SS(i)(INVALID_VALUE)
  SS(i)(LESS)
  SS(i)(LIGHT0)
  SS(i)(LIGHT1)
  SS(i)(LIGHT2)
  SS(i)(LIGHT3)
  SS(i)(LIGHTING)
  SS(i)(LIGHT_MODEL_AMBIENT)
  SS(i)(LIGHT_MODEL_COLOR_CONTROL)
  SS(i)(LIGHT_MODEL_LOCAL_VIEWER)
  SS(i)(LIGHT_MODEL_TWO_SIDE)
  SS(i)(LINE)
  SS(i)(LINEAR)
  SS(i)(LINEAR_MIPMAP_LINEAR)
  SS(i)(LINEAR_MIPMAP_NEAREST)
  SS(i)(LINES)
  SS(i)(LINE_LOOP)
  SS(i)(LINE_STRIP)
  SS(i)(LUMINANCE)
  SS(i)(LUMINANCE_ALPHA)
  SS(i)(MATRIX_MODE)
  SS(i)(MODELVIEW)
  SS(i)(MODULATE)
  SS(i)(N3F_V3F)
  SS(i)(NEAREST)
  SS(i)(NEAREST_MIPMAP_LINEAR)
  SS(i)(NEAREST_MIPMAP_NEAREST)
  SS(i)(NORMALIZE)
  SS(i)(NORMAL_ARRAY)
  SS(i)(NORMAL_ARRAY_BUFFER_BINDING);
  SS(i)(OBJECT_LINEAR)
  SS(i)(OBJECT_PLANE)
  SS(i)(ONE_MINUS_DST_ALPHA)
  SS(i)(ONE_MINUS_DST_COLOR)
  SS(i)(ONE_MINUS_SRC_ALPHA)
  SS(i)(ONE_MINUS_SRC_COLOR)
  SS(i)(OUT_OF_MEMORY)
  SS(i)(PACK_ALIGNMENT)
  SS(i)(POINTS)
  SS(i)(POLYGON)
  SS(i)(POLYGON_OFFSET_FILL)
  SS(i)(POLYGON_SMOOTH)
  SS(i)(POLYGON_STIPPLE)
  SS(i)(POSITION)
  SS(i)(PROJECTION)
  SS(i)(Q)
  SS(i)(QUADS)
  SS(i)(QUAD_STRIP)
  SS(i)(R)
  SS(i)(RENDER)
  SS(i)(REPEAT)
  SS(i)(RGB)
  SS(i)(RGBA)
  SS(i)(RGBA_MODE)
  SS(i)(S)
  SS(i)(SELECT)
  SS(i)(SEPARATE_SPECULAR_COLOR)
  SS(i)(SHADE_MODEL)
  SS(i)(SHININESS)
  SS(i)(SHORT)
  SS(i)(SINGLE_COLOR)
  SS(i)(SMOOTH)
  SS(i)(SPECULAR)
  SS(i)(SPHERE_MAP)
  SS(i)(SRC_ALPHA)
  SS(i)(SRC_ALPHA_SATURATE)
  SS(i)(SRC_COLOR)
  SS(i)(STACK_OVERFLOW)
  SS(i)(STACK_UNDERFLOW)
  SS(i)(STATIC_DRAW)
  SS(i)(STENCIL_BUFFER_BIT)
  SS(i)(T)
  SS(i)(T2F_C3F_V3F)
  SS(i)(T2F_C4F_N3F_V3F)
  SS(i)(T2F_C4UB_V3F)
  SS(i)(T2F_N3F_V3F)
  SS(i)(T2F_V3F)
  SS(i)(T4F_C4F_N3F_V4F)
  SS(i)(T4F_V4F)
  SS(i)(TEXTURE)
  SS(i)(TEXTURE_1D)
  SS(i)(TEXTURE_2D)
  SS(i)(TEXTURE_ALPHA_SIZE)
  SS(i)(TEXTURE_BINDING_2D)
  SS(i)(TEXTURE_BLUE_SIZE)
  SS(i)(TEXTURE_BORDER)
  SS(i)(TEXTURE_BORDER_COLOR)
  SS(i)(TEXTURE_COMPONENTS)
  SS(i)(TEXTURE_COORD_ARRAY)
  SS(i)(TEXTURE_COORD_ARRAY_BUFFER_BINDING);
  SS(i)(TEXTURE_ENV)
  SS(i)(TEXTURE_ENV_COLOR)
  SS(i)(TEXTURE_ENV_MODE)
  SS(i)(TEXTURE_GEN_MODE)
  SS(i)(TEXTURE_GEN_Q)
  SS(i)(TEXTURE_GEN_R)
  SS(i)(TEXTURE_GEN_S)
  SS(i)(TEXTURE_GEN_T)
  SS(i)(TEXTURE_GREEN_SIZE)
  SS(i)(TEXTURE_HEIGHT)
  SS(i)(TEXTURE_INTENSITY_SIZE)
  SS(i)(TEXTURE_LUMINANCE_SIZE)
  SS(i)(TEXTURE_MAG_FILTER)
  SS(i)(TEXTURE_MIN_FILTER)
  SS(i)(TEXTURE_RED_SIZE)
  SS(i)(TEXTURE_WRAP_S)
  SS(i)(TEXTURE_WRAP_T)
  SS(i)(TRIANGLES)
  SS(i)(TRIANGLE_FAN)
  SS(i)(TRIANGLE_STRIP)
  SS(i)(UNPACK_ALIGNMENT)
  SS(i)(UNPACK_ROW_LENGTH)
  SS(i)(UNSIGNED_BYTE)
  SS(i)(UNSIGNED_INT_8_8_8_8_REV)
  SS(i)(UNSIGNED_SHORT)
  SS(i)(V2F)
  SS(i)(V3F)
  SS(i)(VERTEX_ARRAY)
  SS(i)(VERTEX_ARRAY_BUFFER_BINDING);
/*SS(COLOR_BUFFER_BIT) -- same value as GL_LIGHT0 */
# undef SS
  case (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT):
    return "DEPTH_BUFFER_BIT | COLOR_BUFFER_BIT";
/* Oops, same as INVALID_ENUM.
  case (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT):
    return "DEPTH_BUFFER_BIT | STENCIL_BUFFER_BIT";
*/
  case (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT):
    return "COLOR_BUFFER_BIT | STENCIL_BUFFER_BIT";
  case (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT):
    return "DEPTH_BUFFER_BIT | COLOR_BUFFER_BIT | STENCIL_BUFFER_BIT";
  default:
    {
      static char buf[255];
      sprintf (buf, "0x%04X", mode);
      return buf;
    }
  }
}

static void
check_gl_error (const char *s)
{
  GLenum i = glGetError();
  if (i == GL_NO_ERROR) return;

  LOGE("jwzgles: GL ERROR: %s: %s\n", s, mode_desc(i));
}

#endif /* DEBUG */


static void
make_room (const char *name, void **array, int span, int *count, int *size)
{
  if (*count + 1 >= *size)
    {
      int new_size = (*count + 20) * 1.2;   /* mildly exponential */
      *array = realloc (*array, new_size * span);
      Assert (*array, "out of memory");
      /* LOG3("%s: grew %d -> %d", name, *size, new_size); */
      *size = new_size;
    }

  UNREFERENCED_PARAMETER(name);
}


void
jwzgles_reset (void)
{
  if (! state)
    state = (jwzgles_state *) calloc (1, sizeof (*state));

  if (state->lists.lists)
    {
      state->compiling_list = 0;
      if (state->lists.count)
        jwzgles_glDeleteLists (1, state->lists.count);
      free (state->lists.lists);
    }

  if (state->set.verts)   free (state->set.verts);
  if (state->set.norms)   free (state->set.norms);
  if (state->set.tex)     free (state->set.tex);
  if (state->set.color)   free (state->set.color);

  memset (state, 0, sizeof(*state));

  state->s.mode = state->t.mode = state->r.mode = state->q.mode =
    GL_EYE_LINEAR;
  state->s.obj[0] = state->s.eye[0] = 1;  /* s = 1 0 0 0 */
  state->t.obj[1] = state->t.eye[1] = 1;  /* t = 0 1 0 0 */
}


int
jwzgles_glGenLists (int n)
{
  int i;
  int ret = 0;

  Assert (!state->compiling_verts, "glGenLists not allowed inside glBegin");

  /* Ensure space in state->lists, clear the one at the end, and tick counter
     Note that lists are never really deleted, and we can never re-use elements
     of this array.  glDeleteLists zeroes out the contents of the list, but
     the list ID is still valid for use with glNewList forever.
     #### So maybe this should be a linked list instead of an array.
  */
  for (i = 0; i < n; i++)
    {
      list *L;
      int id = 0;
      make_room ("glGenLists",
                 (void **) &state->lists.lists,
                 sizeof (*state->lists.lists),
                 &state->lists.count, &state->lists.size);
      state->lists.count++;
      id = state->lists.count;
      L = &state->lists.lists[id-1];

      memset (L, 0, sizeof (*L));
      L->id = id;
      if (ret == 0) ret = id;
      LOG1("glGenLists -> %d", L->id);
    }

  /* Return the ID of the first list allocated */

  return ret;
}


void
jwzgles_glNewList (int id, int mode)
{
  list *L;
  Assert (id > 0 && id <= state->lists.count, "glNewList: bogus ID");
  Assert (mode == GL_COMPILE, "glNewList: bad mode");
  Assert (!state->compiling_verts, "glNewList not allowed inside glBegin");
  Assert (!state->compiling_list, "nested glNewList");
  Assert (state->set.count == 0, "missing glEnd");

  L = &state->lists.lists[id-1];
  Assert (L->id == id, "glNewList corrupted");

  if (L->count != 0) jwzgles_glDeleteLists (L->id, 1); /* Overwriting */
  Assert (L->count == 0, "glNewList corrupted");

  state->compiling_list = id;

  state->list_enabled = state->enabled;

  LOG1("glNewList -> %d", id);

#ifndef Have_Assert
  UNREFERENCED_PARAMETER(mode);
#endif
}


static void save_arrays (list_fn *, int);
static void restore_arrays (list_fn *, int);
static void copy_array_data (draw_array *, int, const char *);
static void optimize_arrays (void);
static void generate_texture_coords (GLuint, GLuint);


void
jwzgles_glEndList (void)
{
  Assert (state->compiling_list, "extra glEndList");
  Assert (state->set.count == 0, "missing glEnd");
  Assert (!state->compiling_verts, "glEndList not allowed inside glBegin");
  LOG1("glEndList %d", state->compiling_list);
  optimize_arrays();
  state->compiling_list = 0;
  state->list_enabled = state->enabled;
}


static void
list_push (const char * const name,
           list_fn_cb fn, fn_proto proto, void_int *av)
{
  list *L;
  list_fn *F;
  int i;

  Assert (state->compiling_list > 0, "not inside glNewList");
  Assert (state->compiling_list <= state->lists.count, "glNewList corrupted");

  L = &state->lists.lists[state->compiling_list-1];
  Assert (L, "glNewList: no list");

  make_room ("glNewLists",
             (void **) &L->fns, sizeof (*L->fns),
             &L->count, &L->size);
  memset (&L->fns[L->count], 0, sizeof (*L->fns));
  F = L->fns + L->count;

  F->name = name;
  F->fn = fn;
  F->proto = proto;
  if (proto != PROTO_VOID)
    for (i = 0; i < countof(F->argv); i++)
      F->argv[i] = av[i];

# ifdef DEBUG
  switch (proto) {
  case PROTO_VOID:
    LOG1 ("  push %-12s", name);
    break;
  case PROTO_I:
    if (fn == (list_fn_cb) &jwzgles_glBegin ||
        fn == (list_fn_cb) &jwzgles_glFrontFace ||
        fn == (list_fn_cb) &jwzgles_glEnable ||
        fn == (list_fn_cb) &jwzgles_glDisable ||
        fn == (list_fn_cb) &jwzgles_glEnableClientState ||
        fn == (list_fn_cb) &jwzgles_glDisableClientState ||
        fn == (list_fn_cb) &jwzgles_glShadeModel ||
        fn == (list_fn_cb) &jwzgles_glMatrixMode)
      LOG2 ("  push %-12s %s", name, mode_desc (av[0].i));
    else
      LOG2 ("  push %-12s %d", name, av[0].i);
    break;
  case PROTO_F:
    LOG2 ("  push %-12s %7.3f", name, av[0].f);
    break;
  case PROTO_II:
    if (fn == (list_fn_cb) &jwzgles_glBindTexture ||
        fn == (list_fn_cb) &jwzgles_glBindBuffer)
      LOG3 ("  push %-12s %s %d", name, mode_desc (av[0].i), av[1].i);
    else
      LOG3 ("  push %-12s %d %d", name, av[0].i, av[1].i);
    break;
  case PROTO_FF:
    LOG3 ("  push %-12s %7.3f %7.3f", name, av[0].f, av[1].f);
    break;
  case PROTO_IF:
    LOG3 ("  push %-12s %s %7.3f", name, mode_desc (av[0].i), av[1].f);
    break;
  case PROTO_III:
  case PROTO_ARRAYS:
    if (fn == (list_fn_cb) &jwzgles_glDrawArrays ||
        fn == (list_fn_cb) &jwzgles_glTexParameteri)
      LOG4 ("  push %-12s %s %d %d", name, mode_desc (av[0].i),
            av[1].i, av[2].i);
    else
      LOG4 ("  push %-12s %d %d %d", name, av[0].i, av[1].i, av[2].i);
    break;
  case PROTO_FFF:
    LOG4 ("  push %-12s %7.3f %7.3f %7.3f", name, av[0].f, av[1].f, av[2].f);
    break;
  case PROTO_IIF:
    LOG4 ("  push %-12s %s %s %7.3f", name,
             mode_desc(av[0].i), mode_desc(av[1].i), av[2].f);
    break;
  case PROTO_IIII:
    LOG5 ("  push %-12s %d %d %d %d", name,
          av[0].i, av[1].i, av[2].i, av[3].i);
    break;
  case PROTO_FFFF:
    LOG5 ("  push %-12s %7.3f %7.3f %7.3f %7.3f", name,
             av[0].f, av[1].f, av[2].f, av[3].f);
    break;
  case PROTO_IFV:
    LOG6 ("  push %-12s %s %3.1f %3.1f %3.1f %3.1f", name, mode_desc (av[0].i),
             av[1].f, av[2].f, av[3].f, av[4].f);
    break;
  case PROTO_IIV:
    LOG6 ("  push %-12s %s %d %d %d %d", name, mode_desc (av[0].i),
             av[1].i, av[2].i, av[3].i, av[4].i);
    break;
  case PROTO_IIFV:
    LOG7 ("  push %-12s %s %-8s %3.1f %3.1f %3.1f %3.1f", name,
          mode_desc (av[0].i), mode_desc (av[1].i),
             av[2].f, av[3].f, av[4].f, av[5].f);
    break;
  case PROTO_IIIV:
    LOG7 ("  push %-12s %s %-8s %3d %3d %3d %3d", name,
          mode_desc (av[0].i), mode_desc (av[1].i),
             av[2].i, av[3].i, av[4].i, av[5].i);
    break;
  case PROTO_FV16:
    LOG17 ("  push %-12s ["
           "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
           "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
           "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
           "%8.3f %8.3f %8.3f %8.3f ]",
           name,
           av[0].f,  av[1].f,  av[2].f,  av[3].f,
           av[4].f,  av[5].f,  av[6].f,  av[7].f,
           av[8].f,  av[9].f,  av[10].f, av[11].f,
           av[12].f, av[13].f, av[14].f, av[15].f);
    break;
  default:
    Assert (0, "bogus prototype");
    break;
  }
# endif /* DEBUG */

  if (proto == PROTO_ARRAYS) /* glDrawArrays */
    save_arrays (F, av[1].i + av[2].i);

  L->count++;
}


void
jwzgles_glBegin (int mode)
{
  Assert (!state->compiling_verts, "nested glBegin");
  state->compiling_verts++;

  /* Only these commands are allowed inside glBegin:

     glVertex		-- not allowed outside
     glColor
     glSecondaryColor
     glIndex
     glNormal
     glFogCoord
     glTexCoord
     glMultiTexCoord
     glVertexAttrib
     glEvalCoord
     glEvalPoint
     glArrayElement	-- not allowed outside
     glMaterial
     glEdgeFlag
     glCallList
     glCallLists
   */

  if (!state->replaying_list)
    LOG2 ("%sglBegin %s",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          mode_desc (mode));

  Assert (state->set.count == 0, "glBegin corrupted");
  state->set.mode   = mode;
  state->set.count  = 0;
  state->set.ncount = 0;
  state->set.tcount = 0;
  state->set.ccount = 0;
}


void
jwzgles_glDeleteLists (int id0, int range)
{
  Assert (!state->compiling_verts, "glDeleteLists not allowed inside glBegin");

  if (state->compiling_list)
    {
      void_int vv[2];
      vv[0].i = id0;
      vv[1].i = range;
      list_push ("glDeleteLists", (list_fn_cb) &jwzgles_glDeleteLists,
                 PROTO_II, vv);
    }
  else
    {
      int id;

      if (!state->replaying_list)
        LOG2 ("glDeleteLists %d %d", id0, range);

      for (id = id0 + range - 1; id >= id0; id--)
        {
          int i;
          list *L;
          if (id == 0) continue;  /* People do this stupid thing */
          if (id > state->lists.count) break;	/* this too */
          Assert (id > 0 && id <= state->lists.count,
                  "glDeleteLists: bogus ID");
          L = &state->lists.lists[id-1];
          Assert (L->id == id, "glDeleteLists corrupted");

          for (i = 0; i < L->count; i++)
            {
              list_fn *lf = &L->fns[i];
              if (lf->arrays)
                {
                  int j;
                  for (j = 0; j < 4; j++)
                    /* If there's a binding, 'data' is an index, not a ptr. */
                    if (!lf->arrays[j].binding &&
                        lf->arrays[j].data)
                      free (lf->arrays[j].data);
                  free (lf->arrays);
                }
            }
          if (L->fns)
            free (L->fns);
          if (L->buffer)
            glDeleteBuffers (1, &L->buffer);

          memset (L, 0, sizeof (*L));
          L->id = id;
        }
    }
}


extern GLboolean
jwzgles_glIsList (GLuint id)
{
  return (id > 0 && id < (unsigned)state->lists.count);
}



void
jwzgles_glNormal3fv (const GLfloat *v)
{
  if (state->compiling_list && !state->compiling_verts)
    {
      void_int vv[3];
      vv[0].f = v[0];
      vv[1].f = v[1];
      vv[2].f = v[2];
      list_push ("glNormal3f", (list_fn_cb) &jwzgles_glNormal3f,
                 PROTO_FFF, vv);
    }
  else
    {
      if (!state->replaying_list)
        LOG5 ("%s%sglNormal3f   %7.3f %7.3f %7.3f",
              (state->compiling_list || state->replaying_list ? "  " : ""),
              (state->compiling_verts ? "  rec  " : ""),
              v[0], v[1], v[2]);

      if (state->compiling_verts)	/* inside glBegin */
        {
          state->set.cnorm.x = v[0];
          state->set.cnorm.y = v[1];
          state->set.cnorm.z = v[2];
          state->set.ncount++;
          if (state->set.count > 0 && state->set.ncount == 1)  /* not first! */
            state->set.ncount++;
        }
      else				/* outside glBegin */
        {
          glNormal3f (v[0], v[1], v[2]);
          CHECK("glNormal3f");
        }
    }
}


void
jwzgles_glNormal3f (GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat v[3];
  v[0] = x;
  v[1] = y;
  v[2] = z;
  jwzgles_glNormal3fv (v);
}


void
jwzgles_glTexCoord4fv (const GLfloat *v)
{
  if (state->compiling_list && !state->compiling_verts)
    {
      void_int vv[4];
      vv[0].f = v[0];
      vv[1].f = v[1];
      vv[2].f = v[2];
      vv[3].f = v[3];
      list_push ("glTexCoord4f", (list_fn_cb) &jwzgles_glTexCoord4f,
                 PROTO_FFFF, vv);
    }
  else
    {
      if (!state->replaying_list)
        LOG6 ("%s%sglTexCoord4f %7.3f %7.3f %7.3f %7.3f",
              (state->compiling_list || state->replaying_list ? "  " : ""),
              (state->compiling_verts ? "  rec  " : ""),
              v[0], v[1], v[2], v[3]);

      Assert (state->compiling_verts, "glTexCoord4fv outside glBegin");

      if (state->compiling_verts)	/* inside glBegin */
        {
          state->set.ctex.s = v[0];
          state->set.ctex.t = v[1];
          state->set.ctex.r = v[2];
          state->set.ctex.q = v[3];
          state->set.tcount++;
          if (state->set.count > 0 && state->set.tcount == 1)  /* not first! */
            state->set.tcount++;
        }
    }
}


void
jwzgles_glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
  GLfloat v[4];
  v[0] = s;
  v[1] = t;
  v[2] = r;
  v[3] = q;
  jwzgles_glTexCoord4fv (v);
}


void
jwzgles_glTexCoord3fv (const GLfloat *v)
{
  GLfloat vv[4];
  vv[0] = v[0];
  vv[1] = v[1];
  vv[2] = v[2];
  vv[3] = 1;
  jwzgles_glTexCoord4fv (vv);
}


void
jwzgles_glTexCoord2fv (const GLfloat *v)
{
  GLfloat vv[4];
  vv[0] = v[0];
  vv[1] = v[1];
  vv[2] = 0;
  vv[3] = 1;
  jwzgles_glTexCoord4fv (vv);
}


void
jwzgles_glTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
  jwzgles_glTexCoord4f (s, t, r, 1);
}


void
jwzgles_glTexCoord2f (GLfloat s, GLfloat t)
{
  jwzgles_glTexCoord4f (s, t, 0, 1);
}

void
jwzgles_glTexCoord2i (GLint s, GLint t)
{
  jwzgles_glTexCoord4f (s, t, 0, 1);
}

void
jwzgles_glTexCoord1f (GLfloat s)
{
  jwzgles_glTexCoord4f (s, 0, 0, 1);
}


/* glColor: GLfloat */

void
jwzgles_glColor4fv (const GLfloat *v)
{
  if (state->compiling_list && !state->compiling_verts)
    {
      void_int vv[4];
      vv[0].f = v[0];
      vv[1].f = v[1];
      vv[2].f = v[2];
      vv[3].f = v[3];
      list_push ("glColor4f", (list_fn_cb) &jwzgles_glColor4f,
                 PROTO_FFFF, vv);
    }
  else
    {
      if (!state->replaying_list)
        LOG6 ("%s%sglColor4f    %7.3f %7.3f %7.3f %7.3f",
              (state->compiling_list || state->replaying_list ? "  " : ""),
              (state->compiling_verts ? "  rec  " : ""),
              v[0], v[1], v[2], v[3]);

      if (state->compiling_verts)	/* inside glBegin */
        {
          state->set.ccolor.r = v[0];
          state->set.ccolor.g = v[1];
          state->set.ccolor.b = v[2];
          state->set.ccolor.a = v[3];
          state->set.ccount++;
          if (state->set.count > 0 && state->set.ccount == 1)  /* not first! */
            state->set.ccount++;
        }
      else				/* outside glBegin */
        {
          glColor4f (v[0], v[1], v[2], v[3]);
          CHECK("glColor4");
        }
    }
}


void
jwzgles_glColor4f (GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
  GLfloat v[4];
  v[0] = r;
  v[1] = g;
  v[2] = b;
  v[3] = a;
  jwzgles_glColor4fv (v);
}

void
jwzgles_glColor3f (GLfloat r, GLfloat g, GLfloat b)
{
  jwzgles_glColor4f (r, g, b, 1);
}

void
jwzgles_glColor3fv (const GLfloat *v)
{
  jwzgles_glColor3f (v[0], v[1], v[2]);
}


/* glColor: GLdouble */

void
jwzgles_glColor4d (GLdouble r, GLdouble g, GLdouble b, GLdouble a)
{
  jwzgles_glColor4f (r, g, b, a);
}

void
jwzgles_glColor4dv (const GLdouble *v)
{
  jwzgles_glColor4d (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3d (GLdouble r, GLdouble g, GLdouble b)
{
  jwzgles_glColor4d (r, g, b, 1.0);
}

void
jwzgles_glColor3dv (const GLdouble *v)
{
  jwzgles_glColor3d (v[0], v[1], v[2]);
}


/* glColor: GLint (INT_MIN - INT_MAX) */

void
jwzgles_glColor4i (GLint r, GLint g, GLint b, GLint a)
{
  /* -0x8000000 - 0x7FFFFFFF  =>  0.0 - 1.0 */
  jwzgles_glColor4f (0.5 + (GLfloat) r / 0xFFFFFFFF,
                     0.5 + (GLfloat) g / 0xFFFFFFFF,
                     0.5 + (GLfloat) b / 0xFFFFFFFF,
                     0.5 + (GLfloat) a / 0xFFFFFFFF);
}

void
jwzgles_glColor4iv (const GLint *v)
{
  jwzgles_glColor4i (v[0], v[1], v[2], v[3]);
}


void
jwzgles_glColor3i (GLint r, GLint g, GLint b)
{
  jwzgles_glColor4i (r, g, b, 0x7FFFFFFF);
}

void
jwzgles_glColor3iv (const GLint *v)
{
  jwzgles_glColor3i (v[0], v[1], v[2]);
}


/* glColor: GLuint (0 - UINT_MAX) */

void
jwzgles_glColor4ui (GLuint r, GLuint g, GLuint b, GLuint a)
{
  /* 0 - 0xFFFFFFFF  =>  0.0 - 1.0 */
  jwzgles_glColor4f ((GLfloat) r / 0xFFFFFFFF,
                     (GLfloat) g / 0xFFFFFFFF,
                     (GLfloat) b / 0xFFFFFFFF,
                     (GLfloat) a / 0xFFFFFFFF);
}

void
jwzgles_glColor4uiv (const GLuint *v)
{
  jwzgles_glColor4ui (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3ui (GLuint r, GLuint g, GLuint b)
{
  jwzgles_glColor4ui (r, g, b, 0xFFFFFFFF);
}

void
jwzgles_glColor3uiv (const GLuint *v)
{
  jwzgles_glColor3ui (v[0], v[1], v[2]);
}


/* glColor: GLshort (SHRT_MIN - SHRT_MAX) */

void
jwzgles_glColor4s (GLshort r, GLshort g, GLshort b, GLshort a)
{
  /* -0x8000 - 0x7FFF  =>  0.0 - 1.0 */
  jwzgles_glColor4f (0.5 + (GLfloat) r / 0xFFFF,
                     0.5 + (GLfloat) g / 0xFFFF,
                     0.5 + (GLfloat) b / 0xFFFF,
                     0.5 + (GLfloat) a / 0xFFFF);
}

void
jwzgles_glColor4sv (const GLshort *v)
{
  jwzgles_glColor4s (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3s (GLshort r, GLshort g, GLshort b)
{
  jwzgles_glColor4s (r, g, b, 0x7FFF);
}

void
jwzgles_glColor3sv (const GLshort *v)
{
  jwzgles_glColor3s (v[0], v[1], v[2]);
}


/* glColor: GLushort (0 - USHRT_MAX) */

void
jwzgles_glColor4us (GLushort r, GLushort g, GLushort b, GLushort a)
{
  /* 0 - 0xFFFF  =>  0.0 - 1.0 */
  jwzgles_glColor4f ((GLfloat) r / 0xFFFF,
                     (GLfloat) g / 0xFFFF,
                     (GLfloat) b / 0xFFFF,
                     (GLfloat) a / 0xFFFF);
}

void
jwzgles_glColor4usv (const GLushort *v)
{
  jwzgles_glColor4us (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3us (GLushort r, GLushort g, GLushort b)
{
  jwzgles_glColor4us (r, g, b, 0xFFFF);
}

void
jwzgles_glColor3usv (const GLushort *v)
{
  jwzgles_glColor3us (v[0], v[1], v[2]);
}


/* glColor: GLbyte (-128 - 127) */

void
jwzgles_glColor4b (GLbyte r, GLbyte g, GLbyte b, GLbyte a)
{
  /* -128 - 127  =>  0.0 - 1.0 */
  jwzgles_glColor4f (0.5 + (GLfloat) r / 255,
                     0.5 + (GLfloat) g / 255,
                     0.5 + (GLfloat) b / 255,
                     0.5 + (GLfloat) a / 255);
}

void
jwzgles_glColor4bv (const GLbyte *v)
{
  jwzgles_glColor4b (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3b (GLbyte r, GLbyte g, GLbyte b)
{
  jwzgles_glColor4b (r, g, b, 127);
}

void
jwzgles_glColor3bv (const GLbyte *v)
{
  jwzgles_glColor3b (v[0], v[1], v[2]);
}


/* glColor: GLubyte (0 - 255) */

void
jwzgles_glColor4ub (GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
  /* 0 - 255  =>  0.0 - 1.0 */
  jwzgles_glColor4f (r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

void
jwzgles_glColor4ubv (const GLubyte *v)
{
  jwzgles_glColor4ub (v[0], v[1], v[2], v[3]);
}

void
jwzgles_glColor3ub (GLubyte r, GLubyte g, GLubyte b)
{
  jwzgles_glColor4ub (r, g, b, 255);
}

void
jwzgles_glColor3ubv (const GLubyte *v)
{
  jwzgles_glColor3ub (v[0], v[1], v[2]);
}



void
jwzgles_glMaterialfv (GLenum face, GLenum pname, const GLfloat *color)
{
  /* If this is called inside glBegin/glEnd with a front ambient color,
     then treat it the same as glColor: set the color of the upcoming
     vertex.

     Other faces or lighting types within glBegin are ignored.
   */

  if (state->compiling_verts)
    {
      if ((face == GL_FRONT ||
           face == GL_FRONT_AND_BACK) &&
          (pname == GL_AMBIENT ||
           pname == GL_DIFFUSE ||
           pname == GL_AMBIENT_AND_DIFFUSE))
        {
          jwzgles_glColor4f (color[0], color[1], color[2], color[3]);
          state->set.materialistic++;
        }
      else
        LOG2 ("  IGNORING glMaterialfv %s %s",
              mode_desc(face), mode_desc(pname));
    }
  else if (state->compiling_list)
    {
      void_int vv[6];
      vv[0].i = face;
      vv[1].i = pname;
      vv[2].f = color[0];
      vv[3].f = color[1];
      vv[4].f = color[2];
      vv[5].f = color[3];
      list_push ("glMaterialfv", (list_fn_cb) &jwzgles_glMaterialfv,
                 PROTO_IIFV, vv);
    }
  else
    {
      /* If this is called outside of glBegin/glEnd with a front
         ambient color, then the intent is presumably for that color
         to apply to the upcoming vertexes (which may be played back
         from a list that does not have vertex colors in it).  In that
         case, the only way to make the colors show up is to call
         glColor() with GL_COLOR_MATERIAL enabled.

         I'm not sure if this will have other inappropriate side effects...
       */
      if ((face == GL_FRONT ||
           face == GL_FRONT_AND_BACK) &&
          (pname == GL_AMBIENT ||
           pname == GL_DIFFUSE ||
           pname == GL_AMBIENT_AND_DIFFUSE))
        {
          jwzgles_glEnable (GL_COLOR_MATERIAL);
          jwzgles_glColor4f (color[0], color[1], color[2], color[3]);
        }

      /* OpenGLES seems to throw "invalid enum" for GL_FRONT -- but it
         goes ahead and sets the material anyway!  No error if we just
         always use GL_FRONT_AND_BACK.
       */
      if (face == GL_FRONT)
        face = GL_FRONT_AND_BACK;
      if (! state->replaying_list)
        LOG7 ("direct %-12s %s %s %7.3f %7.3f %7.3f %7.3f", "glMaterialfv",
              mode_desc(face), mode_desc(pname),
              color[0], color[1], color[2], color[3]);
      glMaterialfv (face, pname, color);  /* the real one */
      CHECK("glMaterialfv");
    }
}


void
jwzgles_glMaterialiv (GLenum face, GLenum pname, const GLint *v)
{
  GLfloat vv[4];
  vv[0] = v[0];
  vv[1] = v[1];
  vv[2] = v[2];
  vv[3] = 1;
  jwzgles_glMaterialfv (face, pname, vv);
}

void
jwzgles_glMaterialf (GLenum face, GLenum pname, const GLfloat c)
{
  GLfloat vv[4];
  vv[0] = c;
  vv[1] = c;
  vv[2] = c;
  vv[3] = 1;
  jwzgles_glMaterialfv (face, pname, vv);
}


void
jwzgles_glMateriali (GLenum face, GLenum pname, const GLuint c)
{
  jwzgles_glMaterialf (face, pname, c);
}


void
jwzgles_glColorMaterial (GLenum face, GLenum mode)
{
  Assert (!state->compiling_verts,
          "glColorMaterial not allowed inside glBegin");
#if 0
  if (state->compiling_list)
    {
      void_int vv[2];
      vv[0].i = face;
      vv[1].i = mode;
      list_push ("glColorMaterial", (list_fn_cb) &jwzgles_glColorMaterial,
                 PROTO_II, vv);
    }
  else
    {
      /* No real analog to this distinction in OpenGLES, since color
         arrays don't distinguish between "color" and "material", */
      Assert (0, "glColorMaterial: unimplemented mode");
    }
#else
  UNREFERENCED_PARAMETER(face);
  UNREFERENCED_PARAMETER(mode);
#endif
}




void
jwzgles_glVertex4fv (const GLfloat *v)
{
  vert_set *s = &state->set;
  int count = s->count;

  Assert (state->compiling_verts, "glVertex4fv not inside glBegin");

  LOG5("%s  rec  glVertex4f   %7.3f %7.3f %7.3f %7.3f",
       (state->compiling_list || state->replaying_list ? "  " : ""),
       v[0], v[1], v[2], v[3]);

  if (count >= s->size - 1)
    {
      int new_size = 20 + (s->size * 1.2);

      /* 4 arrays, different element sizes...
         We allocate all 4 arrays just in case we need them,
         but we might not end up using them all at the end.
      */

      s->verts = (XYZW *) realloc (s->verts, new_size * sizeof (*s->verts));
      Assert (s->verts, "out of memory");

      s->norms = (XYZ *) realloc (s->norms, new_size * sizeof (*s->norms));
      Assert (s->norms, "out of memory");

      s->tex = (STRQ *) realloc (s->tex, new_size * sizeof (*s->tex));
      Assert (s->tex, "out of memory");

      s->color = (RGBA *) realloc (s->color, new_size * sizeof (*s->color));
      Assert (s->color, "out of memory");

      s->size = new_size;
    }

  s->verts [count].x = v[0];
  s->verts [count].y = v[1];
  s->verts [count].z = v[2];
  s->verts [count].w = v[3];
  s->norms [count] = s->cnorm;
  s->tex   [count] = s->ctex;
  s->color [count] = s->ccolor;
  s->count++;
}


void
jwzgles_glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  GLfloat v[4];
  v[0] = x;
  v[1] = y;
  v[2] = z;
  v[3] = w;
  jwzgles_glVertex4fv (v);
}

void
jwzgles_glVertex4i (GLint x, GLint y, GLint z, GLint w)
{
  jwzgles_glVertex4f (x, y, z, w);
}

void
jwzgles_glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat v[4];
  v[0] = x;
  v[1] = y;
  v[2] = z;
  v[3] = 1;
  jwzgles_glVertex4fv (v);
}

void
jwzgles_glVertex3i (GLint x, GLint y, GLint z)
{
  jwzgles_glVertex3f (x, y, z);
}

void
jwzgles_glVertex3fv (const GLfloat *v)
{
  jwzgles_glVertex3f (v[0], v[1], v[2]);
}

void
jwzgles_glVertex3dv (const GLdouble *v)
{
  jwzgles_glVertex3f (v[0], v[1], v[2]);
}


void
jwzgles_glVertex2f (GLfloat x, GLfloat y)
{
  GLfloat v[3];
  v[0] = x;
  v[1] = y;
  v[2] = 0;
  jwzgles_glVertex3fv (v);
}

void
jwzgles_glVertex2fv (const GLfloat *v)
{
  jwzgles_glVertex2f (v[0], v[1]);
}

void
jwzgles_glVertex2i (GLint x, GLint y)
{
  jwzgles_glVertex2f (x, y);
}


void
jwzgles_glLightiv (GLenum light, GLenum pname, const GLint *params)
{
  GLfloat v[4];
  v[0] = params[0];
  v[1] = params[1];
  v[2] = params[2];
  v[3] = params[3];
  jwzgles_glLightfv (light, pname, v);
}

void
jwzgles_glLightModeliv (GLenum pname, const GLint *params)
{
  GLfloat v[4];
  v[0] = params[0];
  v[1] = params[1];
  v[2] = params[2];
  v[3] = params[3];
  jwzgles_glLightModelfv (pname, v);
}

void
jwzgles_glFogiv (GLenum pname, const GLint *params)
{
  GLfloat v[4];
  v[0] = params[0];
  v[1] = params[1];
  v[2] = params[2];
  v[3] = params[3];
  jwzgles_glFogfv (pname, v);
}

void
jwzgles_glLighti (GLenum light, GLenum pname, GLint param)
{
  jwzgles_glLightf (light, pname, param);
}

void
jwzgles_glLightModeli (GLenum pname, GLint param)
{
  jwzgles_glLightModelf (pname, param);
}

void
jwzgles_glFogi (GLenum pname, GLint param)
{
  jwzgles_glFogf (pname, param);
}


void
jwzgles_glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
  jwzgles_glRotatef (angle, x, y, z);
}


/*
void
jwzgles_glClipPlane (GLenum plane, const GLdouble *equation)
{
  Assert (state->compiling_verts, "glClipPlane not inside glBegin");
  Assert (0, "glClipPlane unimplemented");  / * no GLES equivalent... * /
}
*/


void
jwzgles_glPolygonMode (GLenum face, GLenum mode)
{
  Assert (!state->compiling_verts, "not inside glBegin");
  if (state->compiling_list)
    {
      void_int vv[2];
      vv[0].i = face;
      vv[1].i = mode;
      list_push ("glPolygonMode", (list_fn_cb) &jwzgles_glPolygonMode,
                 PROTO_II, vv);
    }
  else
    {
      /* POINT and LINE don't exist in GLES */
      Assert (mode == GL_FILL, "glPolygonMode: unimplemented mode");
    }
}


void
jwzgles_glDrawBuffer (GLenum buf)
{
  Assert (!state->compiling_verts, "not inside glBegin");
  if (state->compiling_list)
    {
      void_int vv[1];
      vv[0].i = buf;
      list_push ("glDrawBuffer", (list_fn_cb) &jwzgles_glDrawBuffer,
                 PROTO_I, vv);
    }
  else
    {
/*      Assert (buf == GL_BACK, "glDrawBuffer: back buffer only"); */
# ifndef GL_VERSION_ES_CM_1_0  /* not compiling against OpenGLES 1.x */
      if (! state->replaying_list)
        LOG1 ("direct %-12s", "glDrawBuffer");
      glDrawBuffer (buf);      /* the real one */
      CHECK("glDrawBuffer");
# endif
    }
}


/* Given an array of sets of 4 elements of arbitrary size, convert it
   to an array of sets of 6 elements instead: ABCD becomes ABC BCD.
 */
static int
cq2t (unsigned char **arrayP, int stride, int count)
{
  int count2 = count * 6 / 4;
# ifdef Have_Assert
  int size  = stride * count;
# endif
  int size2 = stride * count2;
  const unsigned char    *oarray,  *in;
  unsigned char *array2, *oarray2, *out;
  int i;

  oarray = *arrayP;
  if (!oarray || count == 0)
    return count2;

  array2 = (unsigned char *) malloc (size2);
  Assert (array2, "out of memory");
  oarray2 = array2;

  in =  oarray;
  out = oarray2;
  for (i = 0; i < count / 4; i++)
    {
      const unsigned char *a, *b, *c, *d;	/* the 4 corners */
      a = in; in += stride;
      b = in; in += stride;
      c = in; in += stride;
      d = in; in += stride;

# define PUSH(IN) do {			\
         const unsigned char *ii = IN;	\
         int j;				\
         for (j = 0; j < stride; j++) {	\
           *out++ = *ii++;		\
         }} while(0)

      PUSH (a); PUSH (b); PUSH (d);		/* the 2 triangles */
      PUSH (b); PUSH (c); PUSH (d);
# undef PUSH
    }

  Assert (in  == oarray  + size,  "convert_quads corrupted");
  Assert (out == oarray2 + size2, "convert_quads corrupted");

  free (*arrayP);
  *arrayP = oarray2;
  return count2;
}


/* Convert all coordinates in a GL_QUADS vert_set to GL_TRIANGLES.
 */
static void
convert_quads_to_triangles (vert_set *s)
{
  int count2;
  Assert (s->mode == GL_QUADS, "convert_quads bad mode");
  count2 =
   cq2t ((unsigned char **) &s->verts, sizeof(*s->verts), s->count);
   cq2t ((unsigned char **) &s->norms, sizeof(*s->norms), s->count);
   cq2t ((unsigned char **) &s->tex,   sizeof(*s->tex),   s->count);
   cq2t ((unsigned char **) &s->color, sizeof(*s->color), s->count);
  s->count = count2;
  s->size  = count2;
  s->mode = GL_TRIANGLES;
}


void
jwzgles_glEnd (void)
{
  vert_set *s = &state->set;
  int was_norm, was_tex, was_color, was_mat;
  int  is_norm,  is_tex,  is_color,  is_mat;

  Assert (state->compiling_verts == 1, "missing glBegin");
  state->compiling_verts--;

  Assert (!state->replaying_list, "how did glEnd get into a display list?");

  if (!state->replaying_list)
    {
      LOG5 ("%s  [V = %d, N = %d, T = %d, C = %d]",
            (state->compiling_list || state->replaying_list ? "  " : ""),
            s->count, s->ncount, s->tcount, s->ccount);
      LOG1 ("%sglEnd",
            (state->compiling_list || state->replaying_list ? "  " : ""));
    }

  if (s->count == 0) return;

  if (s->mode == GL_QUADS)
    convert_quads_to_triangles (s);
  else if (s->mode == GL_QUAD_STRIP)
    s->mode = GL_TRIANGLE_STRIP;	/* They do the same thing! */
  else if (s->mode == GL_POLYGON)
    s->mode = GL_TRIANGLE_FAN;		/* They do the same thing! */

  jwzgles_glColorPointer   (4,GL_FLOAT, sizeof(*s->color),s->color); /* RGBA */
  jwzgles_glNormalPointer  (  GL_FLOAT, sizeof(*s->norms),s->norms); /* XYZ  */
  jwzgles_glTexCoordPointer(4,GL_FLOAT, sizeof(*s->tex),  s->tex);   /* STRQ */
  jwzgles_glVertexPointer  (4,GL_FLOAT, sizeof(*s->verts),s->verts); /* XYZW */
  /* glVertexPointer must come after glTexCoordPointer */

  /* If there were no calls to glNormal3f inside of glBegin/glEnd,
     don't bother enabling the normals array.

     If there was exactly *one* call to glNormal3f inside of glBegin/glEnd,
     and it was before the first glVertex3f, then also don't enable the
     normals array, but do emit that call to glNormal3f before calling
     glDrawArrays.

     Likewise for texture coordinates and colors.

     Be careful to leave the arrays' enabled/disabled state the same as
     before, or a later caller might end up using one of our arrays by
     mistake.  (Remember that jwzgles_glIsEnabled() tracks the enablement
     of the list-in-progress as well as the global state.)
  */
  was_norm  = jwzgles_glIsEnabled (GL_NORMAL_ARRAY);
  was_tex   = jwzgles_glIsEnabled (GL_TEXTURE_COORD_ARRAY);
  was_color = jwzgles_glIsEnabled (GL_COLOR_ARRAY);
  was_mat   = jwzgles_glIsEnabled (GL_COLOR_MATERIAL);

  /* If we're executing glEnd in immediate mode, not from inside a display
     list (which is the only way it happens, because glEnd doesn't go into
     display lists), make sure we're not stomping on a saved buffer list:
     in immediate mode, vertexes are client-side only.
   */
  if (! state->compiling_list)
    jwzgles_glBindBuffer (GL_ARRAY_BUFFER, 0);

  if (s->ncount > 1)
    {
      is_norm = 1;
      jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    }
  else
    {
      is_norm = 0;
      if (s->ncount == 1)
        jwzgles_glNormal3f (s->cnorm.x, s->cnorm.y, s->cnorm.z);
      jwzgles_glDisableClientState (GL_NORMAL_ARRAY);
    }

  if (s->tcount > 1 ||
      ((state->compiling_list ? state->list_enabled : state->enabled)
       & (ISENABLED_TEXTURE_GEN_S | ISENABLED_TEXTURE_GEN_T |
          ISENABLED_TEXTURE_GEN_R | ISENABLED_TEXTURE_GEN_Q)))
    {
      /* Enable texture coords if any were specified; or if generation
         is on in immediate mode; or if this list turned on generation. */
      is_tex = 1;
      jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    }
  else
    {
      is_tex = 0;
      if (s->tcount == 1)
        jwzgles_glTexCoord4f (s->ctex.s, s->ctex.t, s->ctex.r, s->ctex.q);
      jwzgles_glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    }

  if (s->ccount > 1)
    {
      is_color = 1;
      jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    }
  else
    {
      is_color = 0;
      if (s->ccount == 1)
        jwzgles_glColor4f (s->ccolor.r, s->ccolor.g, s->ccolor.b, s->ccolor.a);
      jwzgles_glDisableClientState (GL_COLOR_ARRAY);
    }

  jwzgles_glEnableClientState (GL_VERTEX_ARRAY);

  /* We translated the glMaterial calls to per-vertex colors, which are
     of the glColor sort, not the glMaterial sort, so automatically
     turn on material mapping.  Maybe this is a bad idea.
   */
  if (s->materialistic && !jwzgles_glIsEnabled (GL_COLOR_MATERIAL))
    {
      is_mat = 1;
      jwzgles_glEnable (GL_COLOR_MATERIAL);
    }
  else
    is_mat = 0;

  glBindBuffer (GL_ARRAY_BUFFER, 0);    /* This comes later. */
  jwzgles_glDrawArrays (s->mode, 0, s->count);
  glBindBuffer (GL_ARRAY_BUFFER, 0);    /* Keep out of others' hands */

# define RESET(VAR,FN,ARG) do { \
         if (is_##VAR != was_##VAR) { \
            if (was_##VAR) jwzgles_glEnable##FN (ARG); \
            else jwzgles_glDisable##FN (ARG); \
         }} while(0)
  RESET (norm,  ClientState, GL_NORMAL_ARRAY);
  RESET (tex,   ClientState, GL_TEXTURE_COORD_ARRAY);
  RESET (color, ClientState, GL_COLOR_ARRAY);
  RESET (mat,   ,            GL_COLOR_MATERIAL);
# undef RESET

  s->count  = 0;
  s->ncount = 0;
  s->tcount = 0;
  s->ccount = 0;
  s->materialistic = 0;
}


/* The display list is full of calls to glDrawArrays(), plus saved arrays
   of the values we need to restore before calling it.  "Restore" means
   "ship them off to the GPU before each call".

   So instead, this function walks through the display list and
   combines all of those vertex, normal, texture and color values into
   a single VBO array; ships those values off to the GPU *once* at the
   time of glEndList; and when running the list with glCallList, the
   values are already on the GPU and don't need to be sent over again.

   The VBO persists in the GPU until the display list is deleted.
 */
static void
optimize_arrays (void)
{
  list *L = &state->lists.lists[state->compiling_list-1];
  int i, j;
  GLfloat *combo = 0;
  int combo_count = 0;
  int combo_size = 0;
  GLuint buf_name = 0;

  Assert (state->compiling_list, "not compiling a list");
  Assert (L, "no list");
  Assert (!L->buffer, "list already has a buffer");

  glGenBuffers (1, &buf_name);
  CHECK("glGenBuffers");
  if (! buf_name) return;

  L->buffer = buf_name;

  /* Go through the list and dump the contents of the various saved arrays
     into one large array.
   */
  for (i = 0; i < L->count; i++)
    {
      list_fn *F = &L->fns[i];
/*      int count; */
      if (! F->arrays)
        continue;
/*      count = F->argv[2].i;*/  /* 3rd arg to glDrawArrays */

      for (j = 0; j < 4; j++)
        {
          draw_array *A = &F->arrays[j];
          int ocount = combo_count;

          /* If some caller is using arrays that don't have floats in them,
             we just leave them as-is and ship them over at each call.
             Doubt this ever really happens.
           */
          if (A->type != GL_FLOAT)
            continue;

          if (! A->data)	/* No array. */
            continue;

          Assert (A->bytes > 0, "no bytes in draw_array");
          Assert (((unsigned long) A->data > 0xFFFF),
                  "buffer data not a pointer");

          combo_count += A->bytes / sizeof(*combo);
          make_room ("optimize_arrays",
                     (void **) &combo, sizeof(*combo),
                     &combo_count, &combo_size);
          memcpy (combo + ocount, A->data, A->bytes);
          A->binding = buf_name;
          free (A->data);
          /* 'data' is now the byte offset into the VBO. */
          A->data = (void *) (ocount * sizeof(*combo));
          /* LOG3("    loaded %lu floats to pos %d of buffer %d",
               A->bytes / sizeof(*combo), ocount, buf_name); */
        }
    }

  if (combo_count == 0)		/* Nothing to do! */
    {
      if (combo) free (combo);
      glDeleteBuffers (1, &buf_name);
      L->buffer = 0;
      return;
    }

  glBindBuffer (GL_ARRAY_BUFFER, buf_name);
  glBufferData (GL_ARRAY_BUFFER,
                combo_count * sizeof (*combo),
                combo,
                GL_STATIC_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);    /* Keep out of others' hands */

  LOG3("  loaded %d floats of list %d into VBO %d",
       combo_count, state->compiling_list, buf_name);

# ifdef DEBUG
#  if 0
  for (i = 0; i < combo_count; i++)
    {
      if (i % 4 == 0)
        fprintf (stderr, "\njwzgles:    %4d: ", i);
      fprintf (stderr, " %7.3f", combo[i]);
    }
  fprintf (stderr, "\n");
#  endif
# endif /* DEBUG */

  if (combo) free (combo);
}


void
jwzgles_glCallList (int id)
{
  if (state->compiling_list)
    {
      /* Yes, you can call lists inside of lists.
         Yes, recursion would be a mistake. */
      void_int vv[1];
      vv[0].i = id;
      list_push ("glCallList", (list_fn_cb) &jwzgles_glCallList, PROTO_I, vv);
    }
  else
    {
      list *L;
      int i;

      state->replaying_list++;

# ifdef DEBUG
      fprintf (stderr, "\n");
      LOG1 ("glCallList %d", id);
# endif

      Assert (id > 0 && id <= state->lists.count, "glCallList: bogus ID");
      L = &state->lists.lists[id-1];
      Assert (id == L->id, "glCallList corrupted");

      for (i = 0; i < L->count; i++)
        {
          list_fn *F = &L->fns[i];
          list_fn_cb fn = F->fn;
          void_int *av = F->argv;

          switch (F->proto) {
          case PROTO_VOID:
            LOG1 ("  call %-12s", F->name);
            ((void (*) (void)) fn) ();
            break;

          case PROTO_I:
            if (fn == (list_fn_cb) &jwzgles_glBegin ||
                fn == (list_fn_cb) &jwzgles_glFrontFace ||
                fn == (list_fn_cb) &jwzgles_glEnable ||
                fn == (list_fn_cb) &jwzgles_glDisable ||
                fn == (list_fn_cb) &jwzgles_glEnableClientState ||
                fn == (list_fn_cb) &jwzgles_glDisableClientState ||
                fn == (list_fn_cb) &jwzgles_glShadeModel ||
                fn == (list_fn_cb) &jwzgles_glMatrixMode)
              LOG2 ("  call %-12s %s", F->name, mode_desc (av[0].i));
            else
              LOG2 ("  call %-12s %d", F->name, av[0].i);
            ((void (*) (int)) fn) (av[0].i);
            break;

          case PROTO_F:
            LOG2 ("  call %-12s %7.3f", F->name, av[0].f);
            ((void (*) (GLfloat)) fn) (av[0].f);
            break;

          case PROTO_II:
            if (fn == (list_fn_cb) &jwzgles_glBindTexture ||
                fn == (list_fn_cb) &jwzgles_glBindBuffer)
              LOG3 ("  call %-12s %s %d", F->name,
                    mode_desc (av[0].i), av[1].i);
            else
              LOG3 ("  call %-12s %d %d", F->name, av[0].i, av[1].i);
            ((void (*) (int, int)) fn) (av[0].i, av[1].i);
            break;

          case PROTO_FF:
            LOG3 ("  call %-12s %7.3f %7.3f", F->name, av[0].f, av[1].f);
            ((void (*) (GLfloat, GLfloat)) fn) (av[0].f, av[1].f);
            break;

          case PROTO_IF:
            LOG3 ("  call %-12s %s %7.3f", F->name,
                  mode_desc (av[0].f), av[1].f);
            ((void (*) (GLint, GLfloat)) fn) (av[0].i, av[1].f);
            break;

          case PROTO_III: III:
            if (fn == (list_fn_cb) &jwzgles_glDrawArrays ||
                fn == (list_fn_cb) &jwzgles_glTexParameteri)
              LOG4 ("  call %-12s %s %d %d", F->name,
                    mode_desc (av[0].i), av[1].i, av[2].i);
            else
              LOG4 ("  call %-12s %d %d %d", F->name,
                    av[0].i, av[1].i, av[2].i);
            ((void (*) (int, int, int)) fn) (av[0].i, av[1].i, av[2].i);
            break;

          case PROTO_FFF:
            LOG4 ("  call %-12s %7.3f %7.3f %7.3f", F->name,
                  av[0].f, av[1].f, av[2].f);
            ((void (*) (GLfloat, GLfloat, GLfloat)) fn)
              (av[0].f, av[1].f, av[2].f);
            break;

          case PROTO_IIF:
            LOG4 ("  call %-12s %s %s %7.3f", F->name,
                  mode_desc (av[0].i), mode_desc (av[1].i), av[2].f);
            ((void (*) (int, int, GLfloat)) fn) (av[0].i, av[1].i, av[2].f);
            break;

          case PROTO_IIII:
            LOG5 ("  call %-12s %d %d %d %d", F->name,
                  av[0].i, av[1].i, av[2].i, av[3].i);
            ((void (*) (int, int, int, int)) fn)
              (av[0].i, av[1].i, av[2].i, av[3].i);
            break;

          case PROTO_FFFF:
            LOG5 ("  call %-12s %7.3f %7.3f %7.3f %7.3f", F->name,
                  av[0].f, av[1].f, av[2].f, av[3].f);
            ((void (*) (GLfloat, GLfloat, GLfloat, GLfloat)) fn)
              (av[0].f, av[1].f, av[2].f, av[3].f);
            break;

          case PROTO_IFV:
            {
              GLfloat v[4];
              v[0] = av[1].f;
              v[1] = av[2].f;
              v[2] = av[3].f;
              v[3] = av[4].f;
              LOG6 ("  call %-12s %s %3.1f %3.1f %3.1f %3.1f", F->name,
                    mode_desc (av[0].i),
                    av[1].f, av[2].f, av[3].f, av[4].f);
              ((void (*) (int, const GLfloat *)) fn) (av[0].i, v);
            }
            break;

          case PROTO_IIFV:
            {
              GLfloat v[4];
              v[0] = av[2].f;
              v[1] = av[3].f;
              v[2] = av[4].f;
              v[3] = av[5].f;
              LOG7 ("  call %-12s %s %-8s %3.1f %3.1f %3.1f %3.1f", F->name,
                    mode_desc (av[0].i), mode_desc (av[1].i),
                    av[2].f, av[3].f, av[4].f, av[5].f);
              ((void (*) (int, int, const GLfloat *)) fn)
                (av[0].i, av[1].i, v);
            }
            break;

          case PROTO_IIV:
            {
              int v[4];
              v[0] = av[1].i;
              v[1] = av[2].i;
              v[2] = av[3].i;
              v[3] = av[4].i;
              LOG6 ("  call %-12s %s %3d %3d %3d %3d", F->name,
                    mode_desc (av[0].i),
                    av[1].i, av[2].i, av[3].i, av[4].i);
              ((void (*) (int, const int *)) fn) (av[0].i, v);
            }
            break;

          case PROTO_IIIV:
            {
              int v[4];
              v[0] = av[2].i;
              v[1] = av[3].i;
              v[2] = av[4].i;
              v[3] = av[5].i;
              LOG7 ("  call %-12s %s %-8s %3d %3d %3d %3d", F->name,
                    mode_desc (av[0].i), mode_desc (av[1].i),
                    av[2].i, av[3].i, av[4].i, av[5].i);
              ((void (*) (int, int, const int *)) fn)
                (av[0].i, av[1].i, v);
            }
            break;

          case PROTO_ARRAYS:
            restore_arrays (F, av[1].i + av[2].i);
            goto III;
            break;

          case PROTO_FV16:
            {
              GLfloat m[16];
              int i;
              for (i = 0; i < countof(m); i++)
                m[i] = av[i].f;
              LOG17 ("  call %-12s ["
                     "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
                     "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
                     "%8.3f %8.3f %8.3f %8.3f "	"\n\t\t\t       "
                     "%8.3f %8.3f %8.3f %8.3f ]",
                     F->name,
                     m[0],  m[1],  m[2],  m[3],
                     m[4],  m[5],  m[6],  m[7],
                     m[8],  m[9],  m[10], m[11],
                     m[12], m[13], m[14], m[15]);
              ((void (*) (GLfloat *)) fn) (m);
            }
            break;

          default:
            Assert (0, "bogus prototype");
            break;
          }
        }

      LOG1 ("glCallList %d done\n", id);

      state->replaying_list--;
      Assert (state->replaying_list >= 0, "glCallList corrupted");
    }
}


/* When we save a call to glDrawArrays into a display list, we also need to
   save the prevailing copy of the arrays that it will use, and restore them
   later.
 */
static void
save_arrays (list_fn *F, int count)
{
  int i = 0;
  draw_array *A = (draw_array *) calloc (4, sizeof (*A));
  Assert (A, "out of memory");

/*  if (state->set.count > 0) */
    {
      glGetIntegerv (GL_VERTEX_ARRAY_BUFFER_BINDING, &A[i].binding);
      glGetIntegerv (GL_VERTEX_ARRAY_SIZE,    &A[i].size);
      glGetIntegerv (GL_VERTEX_ARRAY_TYPE,    &A[i].type);
      glGetIntegerv (GL_VERTEX_ARRAY_STRIDE,  &A[i].stride);
      glGetPointerv (GL_VERTEX_ARRAY_POINTER, &A[i].data);
      CHECK("glGetPointerv");
      copy_array_data (&A[i], count, "vert");
    }

  i++;
  if (state->set.ncount > 1)
    {
      A[i].size = 3;
      glGetIntegerv (GL_NORMAL_ARRAY_BUFFER_BINDING, &A[i].binding);
      glGetIntegerv (GL_NORMAL_ARRAY_TYPE,    &A[i].type);
      glGetIntegerv (GL_NORMAL_ARRAY_STRIDE,  &A[i].stride);
      glGetPointerv (GL_NORMAL_ARRAY_POINTER, &A[i].data);
      CHECK("glGetPointerv");
      copy_array_data (&A[i], count, "norm");
    }

  i++;
  if (state->set.tcount > 1)
    {
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, &A[i].binding);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_SIZE,    &A[i].size);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_TYPE,    &A[i].type);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_STRIDE,  &A[i].stride);
      glGetPointerv (GL_TEXTURE_COORD_ARRAY_POINTER, &A[i].data);
      CHECK("glGetPointerv");
      copy_array_data (&A[i], count, "tex ");
    }

  i++;
  if (state->set.ccount > 1)
    {
      glGetIntegerv (GL_COLOR_ARRAY_BUFFER_BINDING, &A[i].binding);
      glGetIntegerv (GL_COLOR_ARRAY_SIZE,    &A[i].size);
      glGetIntegerv (GL_COLOR_ARRAY_TYPE,    &A[i].type);
      glGetIntegerv (GL_COLOR_ARRAY_STRIDE,  &A[i].stride);
      glGetPointerv (GL_COLOR_ARRAY_POINTER, &A[i].data);
      CHECK("glGetPointerv");
      copy_array_data (&A[i], count, "col ");
    }

  /* Freed by glDeleteLists. */

  Assert (!F->arrays, "save_arrays corrupted");
  F->arrays = A;
}


#ifdef DEBUG

static void
dump_array_data (draw_array *A, int count,
                 const char *action, const char *name, const void *old)
{
  int bytes = count * A->stride;

  if (A->binding)
    {
      fprintf (stderr,
               "jwzgles:     %s %s %d %s %2d, %4d = %5d   bind %d @ %d\n",
               action, name,
               A->size, mode_desc(A->type), A->stride,
               count, bytes, A->binding, (int) A->data);
    }
  else
    {
      Assert (bytes == A->bytes, "array data corrupted");

      fprintf (stderr, "jwzgles:     %s %s %d %s %2d, %4d = %5d @ %lX",
               action, name,
               A->size, mode_desc(A->type), A->stride,
               count, bytes, (unsigned long) A->data);
      if (old)
        fprintf (stderr, " / %lX", (unsigned long) old);
      fprintf (stderr, "\n");
    }

  if (A->binding)
    {
      Assert (((unsigned long) A->data < 0xFFFF),
              "buffer binding should be a numeric index,"
              " but looks like a pointer");

# if 0
      /* glGetBufferSubData doesn't actually exist in OpenGLES, but this
         was helpful for debugging on real OpenGL... */
      GLfloat *d;
      int i;
      fprintf (stderr, "jwzgles: read back:\n");
      d = (GLfloat *) malloc (A->bytes);
      glGetBufferSubData (GL_ARRAY_BUFFER, (int) A->data,
                          count * A->stride, (void *) d);
      CHECK("glGetBufferSubData");
      for (i = 0; i < count * A->size; i++)
        {
          if (i % 4 == 0)
            fprintf (stderr, "\njwzgles:    %4d: ",
                     i + (int) A->data / sizeof(GLfloat));
          fprintf (stderr, " %7.3f", d[i]);
        }
      fprintf (stderr, "\n");
      free (d);
# endif
    }
# if 0
  else
    {
      unsigned char *b = (unsigned char *) A->data;
      int i;
      if ((unsigned long) A->data < 0xFFFF)
        {
          Assert (0, "buffer data not a pointer");
          return;
        }
      for (i = 0; i < count; i++)
        {
          int j;
          GLfloat *f = (GLfloat *) b;
          int s = A->size;
          if (s == 0) s = 3;  /* normals */
          fprintf (stderr, "jwzgles:    ");
          for (j = 0; j < s; j++)
            fprintf (stderr, " %7.3f", f[j]);
          fprintf (stderr, "\n");
          b += A->stride;
        }
    }
# endif
}

static void
dump_direct_array_data (int count)
{
  draw_array A = { 0, 0, 0, 0, 0, NULL};

  if (jwzgles_glIsEnabled (GL_VERTEX_ARRAY))
    {
      glGetIntegerv (GL_VERTEX_ARRAY_BUFFER_BINDING, &A.binding);
      glGetIntegerv (GL_VERTEX_ARRAY_SIZE,    &A.size);
      glGetIntegerv (GL_VERTEX_ARRAY_TYPE,    &A.type);
      glGetIntegerv (GL_VERTEX_ARRAY_STRIDE,  &A.stride);
      glGetPointerv (GL_VERTEX_ARRAY_POINTER, &A.data);
      A.bytes = count * A.stride;
      dump_array_data (&A, count, "direct", "vertex ", 0);
    }
  if (jwzgles_glIsEnabled (GL_NORMAL_ARRAY))
    {
      A.size = 0;
      glGetIntegerv (GL_NORMAL_ARRAY_BUFFER_BINDING, &A.binding);
      glGetIntegerv (GL_NORMAL_ARRAY_TYPE,    &A.type);
      glGetIntegerv (GL_NORMAL_ARRAY_STRIDE,  &A.stride);
      glGetPointerv (GL_NORMAL_ARRAY_POINTER, &A.data);
      A.bytes = count * A.stride;
      dump_array_data (&A, count, "direct", "normal ", 0);
    }
  if (jwzgles_glIsEnabled (GL_TEXTURE_COORD_ARRAY))
    {
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, &A.binding);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_SIZE,    &A.size);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_TYPE,    &A.type);
      glGetIntegerv (GL_TEXTURE_COORD_ARRAY_STRIDE,  &A.stride);
      glGetPointerv (GL_TEXTURE_COORD_ARRAY_POINTER, &A.data);
      A.bytes = count * A.stride;
      dump_array_data (&A, count, "direct", "texture", 0);
    }
  if (jwzgles_glIsEnabled (GL_COLOR_ARRAY))
    {
      glGetIntegerv (GL_COLOR_ARRAY_BUFFER_BINDING, &A.binding);
      glGetIntegerv (GL_COLOR_ARRAY_SIZE,    &A.size);
      glGetIntegerv (GL_COLOR_ARRAY_TYPE,    &A.type);
      glGetIntegerv (GL_COLOR_ARRAY_STRIDE,  &A.stride);
      glGetPointerv (GL_COLOR_ARRAY_POINTER, &A.data);
      A.bytes = count * A.stride;
      dump_array_data (&A, count, "direct", "color ", 0);
    }
}

#endif /* DEBUG */


static void
copy_array_data (draw_array *A, int count, const char *name)
{
  /* Instead of just memcopy'ing the whole array and obeying its previous
     'stride' value, we make up a more compact array.  This is because if
     the same array data is being used with multiple component types,
     e.g. with glInterleavedArrays, we don't want to copy all of the
     data multiple times.
   */
  int stride2 = 0, bytes, i, j;
  void *data2;
  const GLfloat *IF;
  GLfloat *OF;
  const unsigned char *IB;
  unsigned char *OB;

  if (((unsigned long) A->data) < 0xFFFF)
    {
      Assert (0, "buffer data not a pointer");
      return;
    }

  Assert (A->size >= 2 && A->size <= 4, "bogus array size");

  switch (A->type) {
  case GL_FLOAT:         stride2 = A->size * sizeof(GLfloat); break;
  case GL_UNSIGNED_BYTE: stride2 = A->size; break;
  default: Assert (0, "bogus array type"); break;
  }

  bytes = count * stride2;
  Assert (bytes > 0, "bogus array count or stride");
  Assert (A->data, "missing array data");
  data2 = (void *) malloc (bytes);
  Assert (data2, "out of memory");

  IB = (const unsigned char *) A->data;
  OB = (unsigned char *) data2;
  IF = (const GLfloat *) A->data;
  OF = (GLfloat *) data2;

  switch (A->type) {
  case GL_FLOAT:
    for (i = 0; i < count; i++)
      {
        for (j = 0; j < A->size; j++)
          *OF++ = IF[j];
        IF = (const GLfloat *) (((const unsigned char *) IF) + A->stride);
      }
    break;
  case GL_UNSIGNED_BYTE:
    for (i = 0; i < count; i++)
      {
        for (j = 0; j < A->size; j++)
          *OB++ = IB[j];
        IB += A->stride;
      }
    break;
  default:
    Assert (0, "bogus array type");
    break;
  }

  A->data = data2;
  A->bytes = bytes;
  A->stride = stride2;

# ifdef DEBUG
  dump_array_data (A, count, "saved", name, 0);
# else
  UNREFERENCED_PARAMETER(name);
# endif
}


static void
restore_arrays (list_fn *F, int count)
{
  int i = 0;
  draw_array *A = F->arrays;
  Assert (A, "missing array");

  for (i = 0; i < 4; i++)
    {
# ifdef DEBUG
      const char *name = 0;
# endif

      if (!A[i].size)
        continue;

      Assert ((A[i].binding || A[i].data),
              "array has neither buffer binding nor data");

      glBindBuffer (GL_ARRAY_BUFFER, A[i].binding);
      CHECK("glBindBuffer");

      switch (i) {
      case 0: glVertexPointer  (A[i].size, A[i].type, A[i].stride, A[i].data);
# ifdef DEBUG
        name = "vertex ";
# endif
        CHECK("glVertexPointer");
        break;
      case 1: glNormalPointer  (           A[i].type, A[i].stride, A[i].data);
# ifdef DEBUG
        name = "normal ";
# endif
        CHECK("glNormalPointer");
        break;
      case 2: glTexCoordPointer(A[i].size, A[i].type, A[i].stride, A[i].data);
# ifdef DEBUG
        name = "texture";
# endif
        CHECK("glTexCoordPointer");
        break;
      case 3: glColorPointer   (A[i].size, A[i].type, A[i].stride, A[i].data);
# ifdef DEBUG
        name = "color  ";
# endif
        CHECK("glColorPointer");
        break;
      default: Assert (0, "wat"); break;
      }

# ifdef DEBUG
      dump_array_data (&A[i], count, "restored", name, 0);
# endif
    }

  glBindBuffer (GL_ARRAY_BUFFER, 0);    /* Keep out of others' hands */

# ifndef DEBUG
  UNREFERENCED_PARAMETER(count);
# endif
}


void
jwzgles_glDrawArrays (GLuint mode, GLuint first, GLuint count)
{
  /* If we are auto-generating texture coordinates, do that now, after
     the vertex array was installed, but before drawing, This happens
     when recording into a list, or in direct mode.  It must happen
     before calling optimize_arrays() from glEndList().
   */
  if (! state->replaying_list &&
      ((state->compiling_list ? state->list_enabled : state->enabled)
       & (ISENABLED_TEXTURE_GEN_S | ISENABLED_TEXTURE_GEN_T |
          ISENABLED_TEXTURE_GEN_R | ISENABLED_TEXTURE_GEN_Q)))
    generate_texture_coords (first, count);

  if (state->compiling_list)
    {
      void_int vv[3];
      vv[0].i = mode;
      vv[1].i = first;
      vv[2].i = count;
      list_push ("glDrawArrays", (list_fn_cb) &jwzgles_glDrawArrays,
                 PROTO_ARRAYS, vv);
    }
  else
    {
# ifdef DEBUG
      if (! state->replaying_list) {
        LOG4("direct %-12s %d %d %d", "glDrawArrays", mode, first, count);
        dump_direct_array_data (first + count);
      }
# endif
      glDrawArrays (mode, first, count);  /* the real one */
      CHECK("glDrawArrays");
    }
}


void
jwzgles_glInterleavedArrays (GLenum format, GLsizei stride, const void *data)
{
  /* We can implement this by calling the various *Pointer functions
     with offsets into the same data, taking advantage of stride.
   */
  const unsigned char *c = (const unsigned char *) data;
# define B 1
# define F sizeof(GLfloat)

  Assert (!state->compiling_verts,
          "glInterleavedArrays not allowed inside glBegin");

  jwzgles_glEnableClientState (GL_VERTEX_ARRAY);

  if (!state->replaying_list)
    LOG4 ("%sglInterleavedArrays %s %d %lX",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          mode_desc (format), stride, (unsigned long) data);

  switch (format) {
  case GL_V2F:
    glVertexPointer (2, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    if (!state->replaying_list)
      LOG3 ("%s  -> glVertexPointer 2 FLOAT %d %lX",
            (state->compiling_list || state->replaying_list ? "  " : ""),
            stride, (unsigned long) c);
    break;
  case GL_V3F:
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    if (!state->replaying_list)
      LOG3 ("%s  -> glVertexPointer 3 FLOAT %d %lX",
            (state->compiling_list || state->replaying_list ? "  " : ""),
            stride, (unsigned long) c);
    break;
  case GL_C4UB_V2F:
    if (stride == 0)
      stride = 4*B + 2*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer (4, GL_UNSIGNED_BYTE, stride, c);
    CHECK("glColorPointer");
    c += 4*B;	/* #### might be incorrect float-aligned address */
    glVertexPointer (2, GL_FLOAT, stride, c);
    break;
  case GL_C4UB_V3F:
    if (stride == 0)
      stride = 4*B + 3*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer (4, GL_UNSIGNED_BYTE, stride, c);
    CHECK("glColorPointer");
    c += 4*B;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_C3F_V3F:
    if (stride == 0)
      stride = 3*F + 3*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer (3, GL_FLOAT, stride, c);
    CHECK("glColorPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_N3F_V3F:
    if (stride == 0)
      stride = 3*F + 3*F;
    jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    glNormalPointer (GL_FLOAT, stride, c);
    CHECK("glNormalPointer");
    if (!state->replaying_list)
      LOG3 ("%s  -> glNormalPointer   FLOAT %d %lX",
            (state->compiling_list || state->replaying_list ? "  " : ""),
            stride, (unsigned long) c);
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    if (!state->replaying_list)
      LOG3 ("%s  -> glVertexPointer 3 FLOAT %d %lX",
            (state->compiling_list || state->replaying_list ? "  " : ""),
            stride, (unsigned long) c);
    break;
  case GL_C4F_N3F_V3F:
    if (stride == 0)
      stride = 4*F + 3*F + 3*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer (4, GL_FLOAT, stride, c);
    CHECK("glColorPointer");
    c += 4*F;
    jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    glNormalPointer (GL_FLOAT, stride, c);
    CHECK("glNormalPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T2F_V3F:
    if (stride == 0)
      stride = 2*F + 3*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 2*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T4F_V4F:
    if (stride == 0)
      stride = 4*F + 4*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (4, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 4*F;
    glVertexPointer (4, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T2F_C4UB_V3F:
    if (stride == 0)
      stride = 2*F + 4*B + 3*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 2*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer  (4, GL_UNSIGNED_BYTE, stride, c);
    CHECK("glColorPointer");
    c += 4*B;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T2F_C3F_V3F:
    if (stride == 0)
      stride = 2*F + 3*F + 3*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 2*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer  (3, GL_FLOAT, stride, c);
    CHECK("glColorPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T2F_N3F_V3F:
    if (stride == 0)
      stride = 2*F + 3*F + 3*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 2*F;
    jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    glNormalPointer (GL_FLOAT, stride, c);
    CHECK("glNormalPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T2F_C4F_N3F_V3F:
    if (stride == 0)
      stride = 2*F + 4*F + 3*F + 3*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 2*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer  (3, GL_FLOAT, stride, c);
    CHECK("glColorPointer");
    c += 3*F;
    jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    glNormalPointer (GL_FLOAT, stride, c);
    CHECK("glNormalPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  case GL_T4F_C4F_N3F_V4F:
    if (stride == 0)
      stride = 4*F + 4*F + 3*F + 4*F;
    jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (4, GL_FLOAT, stride, c);
    CHECK("glTexCoordPointer");
    c += 4*F;
    jwzgles_glEnableClientState (GL_COLOR_ARRAY);
    glColorPointer  (4, GL_FLOAT, stride, c);
    CHECK("glColorPointer");
    c += 4*F;
    jwzgles_glEnableClientState (GL_NORMAL_ARRAY);
    glNormalPointer (GL_FLOAT, stride, c);
    CHECK("glNormalPointer");
    c += 3*F;
    glVertexPointer (3, GL_FLOAT, stride, c);
    CHECK("glVertexPointer");
    break;
  default:
    Assert (0, "glInterleavedArrays: bogus format");
    break;
  }

# undef B
# undef F
}



void
jwzgles_glMultMatrixf (const GLfloat *m)
{
  Assert (!state->compiling_verts,
          "glMultMatrixf not allowed inside glBegin");
  if (state->compiling_list)
    {
      void_int vv[16];
      int i;
      for (i = 0; i < countof(vv); i++)
        vv[i].f = m[i];
      list_push ("glMultMatrixf", (list_fn_cb) &jwzgles_glMultMatrixf,
                 PROTO_FV16, vv);
    }
  else
    {
      if (! state->replaying_list)
        LOG1 ("direct %-12s", "glMultMatrixf");
      glMultMatrixf (m);  /* the real one */
      CHECK("glMultMatrixf");
    }
}

void
jwzgles_glLoadMatrixf (const GLfloat * m)
{
    glLoadMatrixf(m);
}

void
jwzgles_glLoadMatrixd (const GLdouble * m)
{
    int n;
    GLfloat matrix[16];
    for (n=0;n<16;n++)
    {
        matrix[n] = (GLfloat)m[n];
    }
    jwzgles_glLoadMatrixf(matrix);
}


/*
void
jwzgles_glClearIndex(GLfloat c)
{
  / * Does GLES even do indexed color? * /
  Assert (0, "glClearIndex unimplemented");
}
*/


/*
void
jwzgles_glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
                  GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
  Assert (0, "glBitmap unimplemented");
}
*/


void
jwzgles_glPushAttrib(int flags)
{
  //Assert (0, "glPushAttrib unimplemented");
  UNREFERENCED_PARAMETER(flags);
}

void
jwzgles_glPopAttrib(void)
{
 // Assert (0, "glPopAttrib unimplemented");
}



/* These are needed for object hit detection in pinion.
   Might need to rewrite that code entirely.  Punt for now.
 */
/*
void
jwzgles_glInitNames (void)
{
/ *  Assert (0, "glInitNames unimplemented");* /
}
*/

/*
void
jwzgles_glPushName (GLuint name)
{
/ *  Assert (0, "glPushName unimplemented");* /
}

GLuint
jwzgles_glPopName (void)
{
/ *  Assert (0, "glPopName unimplemented");* /
  return 0;
}
*/

/*
GLuint
jwzgles_glRenderMode (GLuint mode)
{
/ *  Assert (0, "glRenderMode unimplemented");* /
  return 0;
}

void
jwzgles_glSelectBuffer (GLsizei size, GLuint *buf)
{
/ *  Assert (0, "glSelectBuffer unimplemented");* /
}
*/


void
jwzgles_glGenTextures (GLuint n, GLuint *ret)
{
  Assert (!state->compiling_verts,
          "glGenTextures not allowed inside glBegin");
  /* technically legal, but stupid! */
  Assert (!state->compiling_list,
          "glGenTextures not allowed inside glNewList");
  if (! state->replaying_list)
    LOG1 ("direct %-12s", "glGenTextures");
  glGenTextures (n, ret);  /* the real one */
  CHECK("glGenTextures");
}


/* return the next larger power of 2. */
static int
to_pow2 (int value)
{
  int i = 1;
  while (i < value) i <<= 1;
  return i;
}

void
jwzgles_glTexImage1D (GLenum target, GLint level,
                      GLint internalFormat,
                      GLsizei width, GLint border,
                      GLenum format, GLenum type,
                      const GLvoid *data)
{
  Assert (!state->compiling_verts, "glTexImage1D not allowed inside glBegin");
  /* technically legal, but stupid! */
  Assert (!state->compiling_list, "glTexImage1D inside glNewList");
  Assert (width  == to_pow2(width), "width must be a power of 2");

  if (target == GL_TEXTURE_1D) target = GL_TEXTURE_2D;
  jwzgles_glTexImage2D (target, level, internalFormat, width, 1,
                        border, format, type, data);
}

void
jwzgles_glTexImage2D (GLenum target,
                      GLint  	level,
                      GLint  	internalFormat,
                      GLsizei  	width,
                      GLsizei  	height,
                      GLint  	border,
                      GLenum  	format,
                      GLenum  	type,
                      const GLvoid *data)
{
  GLvoid *d2 = NULL;
  Assert (!state->compiling_verts, "glTexImage2D not allowed inside glBegin");
  Assert (!state->compiling_list,  /* technically legal, but stupid! */
          "glTexImage2D not allowed inside glNewList");

//  Assert (width  == to_pow2(width),   "width must be a power of 2");
//  Assert (height == to_pow2(height), "height must be a power of 2");

  /* OpenGLES no longer supports "4" as a synonym for "RGBA". */
  switch (internalFormat) {
  case 1: internalFormat = GL_LUMINANCE; break;
  case 2: internalFormat = GL_LUMINANCE_ALPHA; break;
  case 3: internalFormat = GL_RGB; break;
  case 4: internalFormat = GL_RGBA; break;
  }

  /* GLES does not let us omit the data pointer to create a blank texture. */
  if (! data)
    {
      data = d2 = (GLvoid *) calloc (1, width * height * sizeof(GLfloat) * 4);
      Assert (d2, "out of memory");
    }

#if 0
  // WTF is right. I see no problem with this situation.
  if (internalFormat == GL_RGB && format == GL_RGBA)
    internalFormat = GL_RGBA;  /* WTF */
#endif
  if (type == GL_UNSIGNED_INT_8_8_8_8_REV)
    type = GL_UNSIGNED_BYTE;

  if (! state->replaying_list)
    LOG10 ("direct %-12s %s %d %s %d %d %d %s %s 0x%lX", "glTexImage2D",
           mode_desc(target), level, mode_desc(internalFormat),
           width, height, border, mode_desc(format), mode_desc(type),
           (unsigned long) data);
  glTexImage2D (target, level, internalFormat, width, height, border,
                format, type, data);  /* the real one */
  CHECK("glTexImage2D");

  free (d2);
}

void
jwzgles_glTexSubImage2D (GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const GLvoid *pixels)
{
  Assert (!state->compiling_verts,
          "glTexSubImage2D not allowed inside glBegin");
  Assert (!state->compiling_list,   /* technically legal, but stupid! */
          "glTexSubImage2D not allowed inside glNewList");

  if (! state->replaying_list)
    LOG10 ("direct %-12s %s %d %d %d %d %d %s %s 0x%lX", "glTexSubImage2D",
           mode_desc(target), level, xoffset, yoffset, width, height,
           mode_desc (format), mode_desc (type), (unsigned long) pixels);
  glTexSubImage2D (target, level, xoffset, yoffset, width, height,
                   format, type, pixels);  /* the real one */
  CHECK("glTexSubImage2D");
}

void
jwzgles_glCompressedTexImage2D (GLenum target,
                                GLint  	level,
                                GLint  	internalFormat,
                                GLsizei  	width,
                                GLsizei  	height,
                                GLint  	border,
                                GLsizei imageSize,
                                const GLvoid *data)
{
  GLvoid *d2 = NULL;
  Assert (!state->compiling_verts, "glCompressedTexImage2D not allowed inside glBegin");
  Assert (!state->compiling_list,  /* technically legal, but stupid! */
          "glCompressedTexImage2D not allowed inside glNewList");

//  Assert (width  == to_pow2(width),   "width must be a power of 2");
//  Assert (height == to_pow2(height), "height must be a power of 2");

  /* GLES does not let us omit the data pointer to create a blank texture. */
  if (! data)
    {
      data = d2 = (GLvoid *) calloc (1, width * height * sizeof(GLfloat) * 4);
      Assert (d2, "out of memory");
    }

  if (! state->replaying_list)
    LOG9 ("direct %-12s %s %d %s %d %d %d %d 0x%lX", "glCompressedTexImage2D",
          mode_desc(target), level, mode_desc(internalFormat),
          width, height, border, imageSize,
          (unsigned long) data);
  glCompressedTexImage2D (target, level, internalFormat, width, height, border,
                imageSize, data);  /* the real one */
  CHECK("glCompressedTexImage2D");

  free (d2);
}

void
jwzgles_glCompressedTexSubImage2D (GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLsizei width, GLsizei height,
                                   GLenum format, GLsizei imageSize,
                                   const GLvoid *pixels)
{
  Assert (!state->compiling_verts,
          "glCompressedTexSubImage2D not allowed inside glBegin");
  Assert (!state->compiling_list,   /* technically legal, but stupid! */
          "glCompressedTexSubImage2D not allowed inside glNewList");

  if (! state->replaying_list)
    LOG10 ("direct %-12s %s %d %d %d %d %d %s %d 0x%lX", "glCompressedTexSubImage2D",
           mode_desc(target), level, xoffset, yoffset, width, height,
           mode_desc (format), imageSize, (unsigned long) pixels);
  glCompressedTexSubImage2D (target, level, xoffset, yoffset, width, height,
                   format, imageSize, pixels);  /* the real one */
  CHECK("glCompressedTexSubImage2D");
}

void
jwzgles_glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat,
                          GLint x, GLint y, GLsizei width, GLsizei height,
                          GLint border)
{
  Assert (!state->compiling_verts,
          "glCopyTexImage2D not allowed inside glBegin");
  Assert (!state->compiling_list,    /* technically legal, but stupid! */
          "glCopyTexImage2D not allowed inside glNewList");
  if (! state->replaying_list)
    LOG9 ("direct %-12s %s %d %s %d %d %d %d %d", "glCopyTexImage2D",
          mode_desc(target), level, mode_desc(internalformat),
          x, y, width, height, border);
  glCopyTexImage2D (target, level, internalformat, x, y, width, height,
                    border);  /* the real one */
  CHECK("glCopyTexImage2D");
}


/* OpenGLES doesn't have auto texture-generation at all!
   "Oh, just rewrite that code to use GPU shaders", they say.
   How fucking convenient.
 */
void
jwzgles_glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
  texgen_state *s = NULL;

  if (pname == GL_TEXTURE_GEN_MODE)
    LOG5 ("%sdirect %-12s %s %s %s",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          "glTexGenfv",
          mode_desc(coord), mode_desc(pname), mode_desc(params[0]));
  else
    LOG8 ("%sdirect %-12s %s %s %3.1f %3.1f %3.1f %3.1f",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          "glTexGenfv",
          mode_desc(coord), mode_desc(pname),
          params[0], params[1], params[2], params[3]);

  switch (coord) {
  case GL_S: s = &state->s; break;
  case GL_T: s = &state->t; break;
  case GL_R: s = &state->r; break;
  case GL_Q: s = &state->q; break;
  default: Assert (0, "glGetTexGenfv: unknown coord"); break;
  }

  switch (pname) {
  case GL_TEXTURE_GEN_MODE: s->mode = params[0]; break;
  case GL_OBJECT_PLANE:     memcpy (s->obj, params, sizeof(s->obj)); break;
  case GL_EYE_PLANE:        memcpy (s->eye, params, sizeof(s->eye)); break;
  default: Assert (0, "glTexGenfv: unknown pname"); break;
  }
}

void
jwzgles_glTexGeni (GLenum coord, GLenum pname, GLint param)
{
  GLfloat v = param;
  jwzgles_glTexGenfv (coord, pname, &v);
}

void
jwzgles_glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
  texgen_state *s = NULL;

  switch (coord) {
  case GL_S: s = &state->s; break;
  case GL_T: s = &state->t; break;
  case GL_R: s = &state->r; break;
  case GL_Q: s = &state->q; break;
  default: Assert (0, "glGetTexGenfv: unknown coord"); break;
  }

  switch (pname) {
  case GL_TEXTURE_GEN_MODE: params[0] = s->mode; break;
  case GL_OBJECT_PLANE:     memcpy (params, s->obj, sizeof(s->obj)); break;
  case GL_EYE_PLANE:        memcpy (params, s->eye, sizeof(s->eye)); break;
  default: Assert (0, "glGetTexGenfv: unknown pname"); break;
  }

  if (pname == GL_TEXTURE_GEN_MODE)
    LOG5 ("%sdirect %-12s %s %s -> %s",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          "glGetTexGenfv",
          mode_desc(coord), mode_desc(pname), mode_desc(params[0]));
  else
    LOG8 ("%sdirect %-12s %s %s -> %3.1f %3.1f %3.1f %3.1f",
          (state->compiling_list || state->replaying_list ? "  " : ""),
          "glGetTexGenfv",
          mode_desc(coord), mode_desc(pname),
          params[0], params[1], params[2], params[3]);
}


static GLfloat
dot_product (int rank, GLfloat *a, GLfloat *b)
{
  /* A dot B  =>  (A[1] * B[1]) + ... + (A[n] * B[n]) */
  GLfloat ret = 0;
  int i;
  for (i = 0; i < rank; i++)
    ret += a[i] * b[i];
  return ret;
}



/* Compute the texture coordinates of the prevailing list of verts as per
   http://www.opengl.org/wiki/Mathematics_of_glTexGen
 */
static void
generate_texture_coords (GLuint first, GLuint count)
{
  GLfloat *tex_out, *tex_array;
  GLsizei tex_stride;
  GLuint i;
  draw_array A = { 0, 0, 0, 0, 0, NULL};
  char *verts_in;

  struct { GLuint which, flag, mode; GLfloat plane[4]; } tg[4] = {
    { GL_S, ISENABLED_TEXTURE_GEN_S, 0, { 0, } },
    { GL_T, ISENABLED_TEXTURE_GEN_T, 0, { 0, } },
    { GL_R, ISENABLED_TEXTURE_GEN_R, 0, { 0, } },
    { GL_Q, ISENABLED_TEXTURE_GEN_Q, 0, { 0, }}};

  int tcoords = 0;

  /* Read the texture plane configs that were stored with glTexGen.
   */
  for (i = 0; i < countof(tg); i++)
    {
      GLfloat mode = 0;
      if (! ((state->compiling_list ? state->list_enabled : state->enabled)
             & tg[i].flag))
        continue;
      jwzgles_glGetTexGenfv (tg[i].which, GL_TEXTURE_GEN_MODE, &mode);
      jwzgles_glGetTexGenfv (tg[i].which, GL_OBJECT_PLANE, tg[i].plane);
      tg[i].mode = mode;
      tcoords++;
    }

  if (tcoords == 0) return;  /* Nothing to do! */


  /* Make the array to store our texture coords in. */

  tex_stride = tcoords * sizeof(GLfloat);
  tex_array = (GLfloat *) calloc (first + count, tex_stride);
  tex_out = tex_array;


  /* Read the prevailing vertex array, that was stored with
     glVertexPointer or glInterleavedArrays.
   */
  glGetIntegerv (GL_VERTEX_ARRAY_BUFFER_BINDING, &A.binding);
  glGetIntegerv (GL_VERTEX_ARRAY_SIZE,    &A.size);
  glGetIntegerv (GL_VERTEX_ARRAY_TYPE,    &A.type);
  glGetIntegerv (GL_VERTEX_ARRAY_STRIDE,  &A.stride);
  glGetPointerv (GL_VERTEX_ARRAY_POINTER, &A.data);
  A.bytes = count * A.stride;

  verts_in = (char *) A.data;

  /* Iterate over each vertex we're drawing.
     We just skip the ones < start, but the tex array has
     left room for zeroes there anyway.
   */
  for (i = first; i < first + count; i++)
    {
      GLfloat vert[4] = { 0, };
      int j, k;

      /* Extract this vertex into `vert' as a float, whatever its type was. */
      for (j = 0; j < A.size; j++)
        {
          switch (A.type) {
          case GL_SHORT:  vert[j] = ((GLshort *)  verts_in)[j]; break;
          case GL_INT:    vert[j] = ((GLint *)    verts_in)[j]; break;
          case GL_FLOAT:  vert[j] = ((GLfloat *)  verts_in)[j]; break;
          case GL_DOUBLE: vert[j] = ((GLdouble *) verts_in)[j]; break;
          default: Assert (0, "unknown vertex type"); break;
          }
        }

      /* Compute the texture coordinate for this vertex.
         For GL_OBJECT_LINEAR, these coordinates are static, and can go
         into the display list.  But for GL_EYE_LINEAR, GL_SPHERE_MAP and
         GL_REFLECTION_MAP, they depend on the prevailing ModelView matrix,
         and so need to be computed afresh each time glDrawArrays is called.
         Unfortunately, our verts and norms are gone by then, dumped down
         into the VBO and discarded from CPU RAM.  Bleh.
       */
      for (j = 0, k = 0; j < countof(tg); j++)
        {
          if (! ((state->compiling_list ? state->list_enabled : state->enabled)
                 & tg[j].flag))
            continue;
          switch (tg[j].mode) {
          case GL_OBJECT_LINEAR:
            tex_out[k] = dot_product (4, vert, tg[j].plane);
            break;
          default:
            Assert (0, "unimplemented texture mode");
            break;
          }
          k++;
        }

      /* fprintf (stderr, "%4d: V %-5.1f %-5.1f %-5.1f  T %-5.1f %-5.1f\n",
               i, vert[0], vert[1], vert[2], tex_out[0], tex_out[1]); */

      /* Move verts_in and tex_out forward to the next vertex by stride. */
      verts_in += A.stride;
      tex_out = (GLfloat *) (((char *) tex_out) + tex_stride);
    }

  jwzgles_glEnableClientState (GL_TEXTURE_COORD_ARRAY);
  jwzgles_glTexCoordPointer (tcoords, GL_FLOAT, tex_stride,
                             (GLvoid *) tex_array);
  free (tex_array);
}


int
jwzgles_gluBuild2DMipmaps (GLenum target,
                           GLint  	internalFormat,
                           GLsizei  	width,
                           GLsizei  	height,
                           GLenum  	format,
                           GLenum  	type,
                           const GLvoid *data)
{
  /* Not really bothering with mipmapping; only making one level.
     Note that this required a corresponding hack in glTexParameterf().
   */

  int w2 = to_pow2(width);
  int h2 = to_pow2(height);

  void *d2 = NULL;

  /* OpenGLES no longer supports "4" as a synonym for "RGBA". */
  switch (internalFormat) {
  case 1: internalFormat = GL_LUMINANCE; break;
  case 2: internalFormat = GL_LUMINANCE_ALPHA; break;
  case 3: internalFormat = GL_RGB; break;
  case 4: internalFormat = GL_RGBA; break;
  }

/*  if (w2 < h2) w2 = h2;
  if (h2 < w2) h2 = w2;*/

  if (w2 != width || h2 != height)
    {
      /* Scale up the image bits to fit the power-of-2 texture.
         We have to do this because the mipmap API assumes that
         the texture bits go to texture coordinates 1.0 x 1.0.
         This could be more efficient, but it doesn't happen often.
      */
      int istride = (format == GL_RGBA ? 4 : 3);
      int ostride = 4;
      int ibpl = istride * width;
      int obpl = ostride * w2;
      int oy;
      const unsigned char *in = (const unsigned char *) data;
      unsigned char *out = (unsigned char *) malloc(h2 * obpl);
      Assert (out, "out of memory");
      data = d2 = out;

      for (oy = 0; oy < h2; oy++)
        {
          int iy = oy * height / h2;
          const unsigned char *iline = in  + (iy * ibpl);
          unsigned char       *oline = out + (oy * obpl);
          int ox;
          for (ox = 0; ox < w2; ox++)
            {
              int ix = ox * width / w2;
              const unsigned char *i = iline + (ix * istride);
              unsigned char       *o = oline + (ox * ostride);
              *o++ = *i++;  /* R */
              *o++ = *i++;  /* G */
              *o++ = *i++;  /* B */
              *o++ = (istride == 4 ? *i : 0xFF); /* A */
            }
        }
      width  = w2;
      height = h2;
      internalFormat = GL_RGBA;
      format = GL_RGBA;
    }

  jwzgles_glTexImage2D (target, 0, internalFormat, w2, h2, 0,
                        format, type, data);
  free (d2);

  return 0;
}


void
jwzgles_glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
  jwzgles_glBegin (GL_POLYGON);
  jwzgles_glVertex2f (x1, y1);
  jwzgles_glVertex2f (x2, y1);
  jwzgles_glVertex2f (x2, y2);
  jwzgles_glVertex2f (x1, y2);
  jwzgles_glEnd ();
}

void
jwzgles_glRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
  jwzgles_glRectf (x1, y1, x2, y2);
}

/*
void
jwzgles_glClearDepth (GLfloat d)
{
  / * Not sure what to do here * /
  Assert (d == 1.0, "glClearDepth unimplemented");
}
*/


/* When in immediate mode, we store a bit into state->enabled, and also
   call the real glEnable() / glDisable().

   When recording a list, we store a bit into state->list_enabled instead,
   so that we can see what the prevailing enablement state will be when
   the list is run.

   set: 1 = set, -1 = clear, 0 = query.
*/
static int
enable_disable (GLuint bit, int set)
{
  int result = (set > 0);
  int omitp = 0;
  int csp = 0;
  unsigned long flag = 0;

  switch (bit) {
  case GL_TEXTURE_1D:     /* We implement 1D textures as 2D textures. */
  case GL_TEXTURE_2D:     flag = ISENABLED_TEXTURE_2D;		     break;
  case GL_TEXTURE_GEN_S:  flag = ISENABLED_TEXTURE_GEN_S; omitp = 1; break;
  case GL_TEXTURE_GEN_T:  flag = ISENABLED_TEXTURE_GEN_T; omitp = 1; break;
  case GL_TEXTURE_GEN_R:  flag = ISENABLED_TEXTURE_GEN_R; omitp = 1; break;
  case GL_TEXTURE_GEN_Q:  flag = ISENABLED_TEXTURE_GEN_Q; omitp = 1; break;
  case GL_LIGHTING:       flag = ISENABLED_LIGHTING;		     break;
  case GL_BLEND:          flag = ISENABLED_BLEND;		     break;
  case GL_DEPTH_TEST:     flag = ISENABLED_DEPTH_TEST;		     break;
  case GL_ALPHA_TEST:     flag = ISENABLED_ALPHA_TEST;		     break;
  case GL_CULL_FACE:      flag = ISENABLED_CULL_FACE;		     break;
  case GL_NORMALIZE:      flag = ISENABLED_NORMALIZE;		     break;
  case GL_FOG:            flag = ISENABLED_FOG;			     break;
  case GL_COLOR_MATERIAL: flag = ISENABLED_COLMAT;		     break;

  /* Maybe technically these only work with glEnableClientState,
     but we treat that as synonymous with glEnable. */
  case GL_VERTEX_ARRAY:   flag = ISENABLED_VERT_ARRAY;     csp = 1;  break;
  case GL_NORMAL_ARRAY:   flag = ISENABLED_NORM_ARRAY;     csp = 1;  break;
  case GL_COLOR_ARRAY:    flag = ISENABLED_COLOR_ARRAY;    csp = 1;  break;
  case GL_TEXTURE_COORD_ARRAY: flag = ISENABLED_TEX_ARRAY; csp = 1;  break;

  default:
    Assert (set != 0, "glIsEnabled unimplemented bit");
    break;
  }

  if (set)  /* setting or unsetting, not querying */
    {
      const char *fns[4] = { "glEnable", "glDisable",
                             "glEnableClientState", "glDisableClientState" };
      list_fn_cb fs[4] = { (list_fn_cb) &jwzgles_glEnable,
                           (list_fn_cb) &jwzgles_glDisable,
                           (list_fn_cb) &jwzgles_glEnableClientState,
                           (list_fn_cb) &jwzgles_glDisableClientState };
      const char *fn = fns[(csp ? 2 : 0) + (set < 0 ? 1 : 0)];
      list_fn_cb  f  =  fs[(csp ? 2 : 0) + (set < 0 ? 1 : 0)];

      Assert (!state->compiling_verts,
              "glEnable/glDisable not allowed inside glBegin");

      if (state->compiling_list)
        {
          void_int vv[1];
          vv[0].i = bit;
          list_push (fn, f,PROTO_I, vv);
        }

      if (! state->replaying_list &&
          ! state->compiling_list)
        LOG2 ("direct %-12s %s", fn, mode_desc(bit));

      if (csp && !state->compiling_verts)
        {
          if (set > 0)
            switch (bit) {
            case GL_NORMAL_ARRAY: state->set.ncount        += 2; break;
            case GL_TEXTURE_COORD_ARRAY: state->set.tcount += 2; break;
            case GL_COLOR_ARRAY: state->set.ccount         += 2; break;
            default: break;
            }
          else
            switch (bit) {
            case GL_NORMAL_ARRAY: state->set.ncount        = 0; break;
            case GL_TEXTURE_COORD_ARRAY: state->set.tcount = 0; break;
            case GL_COLOR_ARRAY: state->set.ccount         = 0; break;
            default: break;
            }
        }

      if (omitp || state->compiling_list)
        ;
      else if (set > 0 && csp)
        glEnableClientState (bit);	/* the real one */
      else if (set < 0 && csp)
        glDisableClientState (bit);	/* the real one */
      else if (set > 0)
        glEnable (bit);			/* the real one */
      else
        glDisable (bit);		/* the real one */

      CHECK(fn);
    }

  /* Store the bit in our state as well, or query it.
   */
  if (flag)
    {
      unsigned long *enabled = (state->compiling_list
                                ? &state->list_enabled
                                : &state->enabled);
      if (set > 0)
        *enabled |= flag;
      else if (set < 0)
        *enabled &= ~flag;
      else
        result = !!(*enabled & flag);
    }

  return result;
}


void
jwzgles_glEnable (GLuint bit)
{
  enable_disable (bit, 1);
}

void
jwzgles_glDisable (GLuint bit)
{
  enable_disable (bit, -1);
}

GLboolean
jwzgles_glIsEnabled (GLuint bit)
{
  return enable_disable (bit, 0);
}

void
jwzgles_glEnableClientState (GLuint cap)
{
  enable_disable (cap, 1);
}

void
jwzgles_glDisableClientState (GLuint cap)
{
  enable_disable (cap, -1);
}



/* The spec says that OpenGLES 1.x doesn't implement glGetFloatv.
   Were this true, it would suck, for it would mean that there was no
   way to retrieve the prevailing matrixes.  To implement this, we'd
   have to keep track of them all on the client side by combining in
   all the actions of glMultMatrixf, glRotatef, etc.

   However, Apple's iOS OpenGLES *does* provide glGetFloatv!
 */
void
jwzgles_glGetFloatv (GLenum pname, GLfloat *params)
{
  if (! state->replaying_list)
    LOG2 ("direct %-12s %s", "glGetFloatv", mode_desc(pname));
  glGetFloatv (pname, params);  /* the real one */
  CHECK("glGetFloatv");
}


/* Likewise: not supposed to be there, but it is. */
void
jwzgles_glGetPointerv (GLenum pname, GLvoid *params)
{
  if (! state->replaying_list)
    LOG2 ("direct %-12s %s", "glGetPointerv", mode_desc(pname));
  glGetPointerv (pname, (GLvoid **)params);  /* the real one */
  CHECK("glGetPointerv");
}


void jwzgles_glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    glReadPixels(x,y,width,height,format,type,pixels);
}




void jwzgles_glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    glCopyTexSubImage2D (target,level,xoffset,yoffset,x,y,width,height);
}

void jwzgles_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
    glDrawElements(mode, count, type, indices);
}


GLenum jwzgles_glGetError()
{
    return glGetError();
}
const GLubyte * jwzgles_glGetString(GLenum name)
{
    return glGetString(name);
}





/* How many cells are written into the *params array.
   We need to know this to avoid smashing the caller's stack
   if they asked for a single-value parameter.
 */
static int
glGet_ret_count (GLenum pname)
{
  switch (pname) {
/*case GL_COLOR_MATRIX: */
  case GL_MODELVIEW_MATRIX:
  case GL_PROJECTION_MATRIX:
  case GL_TEXTURE_MATRIX:
/*case GL_TRANSPOSE_COLOR_MATRIX: */
/*case GL_TRANSPOSE_MODELVIEW_MATRIX: */
/*case GL_TRANSPOSE_PROJECTION_MATRIX: */
/*case GL_TRANSPOSE_TEXTURE_MATRIX: */
    return 16;
/*case GL_ACCUM_CLEAR_VALUE: */
/*case GL_BLEND_COLOR: */
  case GL_COLOR_CLEAR_VALUE:
  case GL_COLOR_WRITEMASK:
  case GL_CURRENT_COLOR:
/*case GL_CURRENT_RASTER_COLOR: */
/*case GL_CURRENT_RASTER_POSITION: */
/*case GL_CURRENT_RASTER_SECONDARY_COLOR: */
/*case GL_CURRENT_RASTER_TEXTURE_COORDS: */
/*case GL_CURRENT_SECONDARY_COLOR: */
  case GL_CURRENT_TEXTURE_COORDS:
  case GL_FOG_COLOR:
  case GL_LIGHT_MODEL_AMBIENT:
/*case GL_MAP2_GRID_DOMAIN: */
  case GL_SCISSOR_BOX:
  case GL_VIEWPORT:
    return 4;
  case GL_CURRENT_NORMAL:
  case GL_POINT_DISTANCE_ATTENUATION:
    return 3;
  case GL_ALIASED_LINE_WIDTH_RANGE:
  case GL_ALIASED_POINT_SIZE_RANGE:
  case GL_DEPTH_RANGE:
/*case GL_LINE_WIDTH_RANGE: */
/*case GL_MAP1_GRID_DOMAIN: */
/*case GL_MAP2_GRID_SEGMENTS: */
  case GL_MAX_VIEWPORT_DIMS:
/*case GL_POINT_SIZE_RANGE: */
  case GL_POLYGON_MODE:
  case GL_SMOOTH_LINE_WIDTH_RANGE:
  case GL_SMOOTH_POINT_SIZE_RANGE:
    return 2;
  default:
    return 1;
  }
}


void
jwzgles_glGetDoublev (GLenum pname, GLdouble *params)
{
  GLfloat m[16];
  int i, j = glGet_ret_count (pname);
  jwzgles_glGetFloatv (pname, m);
  for (i = 0; i < j; i++)
    params[i] = m[i];
}


void
jwzgles_glGetIntegerv (GLenum pname, GLint *params)
{
  GLfloat m[16];
  int i, j = glGet_ret_count (pname);
  jwzgles_glGetFloatv (pname, m);
  for (i = 0; i < j; i++)
    params[i] = m[i];
}


void
jwzgles_glGetBooleanv (GLenum pname, GLboolean *params)
{
  GLfloat m[16];
  int i, j = glGet_ret_count (pname);
  jwzgles_glGetFloatv (pname, m);
  for (i = 0; i < j; i++)
    params[i] = (m[i] != 0.0);
}


const char *
jwzgles_gluErrorString (GLenum error)
{
  static char s[20];
  sprintf (s, "0x%lX", (unsigned long) error);
  return s;
}


/* These four *Pointer calls (plus glBindBuffer and glBufferData) can
   be included inside glNewList, but they actually execute immediately
   anyway, because their data is recorded in the list by the
   subsequently-recorded call to glDrawArrays.  This is a little weird.
 */
void
jwzgles_glVertexPointer (GLuint size, GLuint type, GLuint stride,
                         const GLvoid *ptr)
{
  if (! state->replaying_list)
    LOG5 ("direct %-12s %d %s %d 0x%lX", "glVertexPointer",
          size, mode_desc(type), stride, (unsigned long) ptr);
  glVertexPointer (size, type, stride, ptr);  /* the real one */
  CHECK("glVertexPointer");
}


void
jwzgles_glNormalPointer (GLuint type, GLuint stride, const GLvoid *ptr)
{
  if (! state->replaying_list)
    LOG4 ("direct %-12s %s %d 0x%lX", "glNormalPointer",
          mode_desc(type), stride, (unsigned long) ptr);
  glNormalPointer (type, stride, ptr);  /* the real one */
  CHECK("glNormalPointer");
}

void
jwzgles_glColorPointer (GLuint size, GLuint type, GLuint stride,
                        const GLvoid *ptr)
{
  if (! state->replaying_list)
    LOG5 ("direct %-12s %d %s %d 0x%lX", "glColorPointer",
          size, mode_desc(type), stride, (unsigned long) ptr);
  glColorPointer (size, type, stride, ptr);  /* the real one */
  CHECK("glColorPointer");
}

void
jwzgles_glTexCoordPointer (GLuint size, GLuint type, GLuint stride,
                           const GLvoid *ptr)
{
  if (! state->replaying_list)
    LOG5 ("direct %-12s %d %s %d 0x%lX", "glTexCoordPointer",
          size, mode_desc(type), stride, (unsigned long) ptr);
  glTexCoordPointer (size, type, stride, ptr);  /* the real one */
  CHECK("glTexCoordPointer");
}

void
jwzgles_glBindBuffer (GLuint target, GLuint buffer)
{
  if (! state->replaying_list)
    LOG3 ("direct %-12s %s %d", "glBindBuffer", mode_desc(target), buffer);
  glBindBuffer (target, buffer);  /* the real one */
  CHECK("glBindBuffer");
}

void
jwzgles_glBufferData (GLenum target, GLsizeiptr size, const void *data,
                      GLenum usage)
{
  if (! state->replaying_list)
    LOG5 ("direct %-12s %s %ld 0x%lX %s", "glBufferData",
          mode_desc(target), size, (unsigned long) data, mode_desc(usage));
  glBufferData (target, size, data, usage);  /* the real one */
  CHECK("glBufferData");
}


void
jwzgles_glTexParameterf (GLuint target, GLuint pname, GLfloat param)
{
  Assert (!state->compiling_verts,
          "glTexParameterf not allowed inside glBegin");

  /* We don't *really* implement mipmaps, so just turn this off. */
  if (param == GL_LINEAR_MIPMAP_LINEAR)   param = GL_LINEAR;
  if (param == GL_NEAREST_MIPMAP_LINEAR)  param = GL_LINEAR;
  if (param == GL_LINEAR_MIPMAP_NEAREST)  param = GL_NEAREST;
  if (param == GL_NEAREST_MIPMAP_NEAREST) param = GL_NEAREST;

  /* We implement 1D textures as 2D textures. */
  if (target == GL_TEXTURE_1D) target = GL_TEXTURE_2D;

  /* Apparently this is another invalid enum. Just ignore it. */
  if ((pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T) &&
      param == GL_CLAMP)
    return;

  if (state->compiling_list)
    {
      void_int vv[3];
      vv[0].i = target;
      vv[1].i = pname;
      vv[2].f = param;
      list_push ("glTexParameterf", (list_fn_cb) &jwzgles_glTexParameterf,
                 PROTO_IIF, vv);
    }
  else
    {
      if (! state->replaying_list)
        LOG4 ("direct %-12s %s %s %7.3f", "glTexParameterf",
              mode_desc(target), mode_desc(pname), param);
      glTexParameterf (target, pname, param);  /* the real one */
      CHECK("glTexParameterf");
    }
}

void
jwzgles_glTexParameteri (GLuint target, GLuint pname, GLuint param)
{
  jwzgles_glTexParameterf (target, pname, param);
}


void
jwzgles_glBindTexture (GLuint target, GLuint texture)
{
  Assert (!state->compiling_verts,
          "glBindTexture not allowed inside glBegin");

  /* We implement 1D textures as 2D textures. */
  if (target == GL_TEXTURE_1D) target = GL_TEXTURE_2D;

  if (state->compiling_list)
    {
      void_int vv[2];
      vv[0].i = target;
      vv[1].i = texture;
      list_push ("glBindTexture", (list_fn_cb) &jwzgles_glBindTexture,
                 PROTO_II, vv);
    }

  /* Do it immediately as well, for generate_texture_coords */
  /* else */
    {
      if (! state->replaying_list)
        LOG3 ("direct %-12s %s %d", "glBindTexture",
              mode_desc(target), texture);
      glBindTexture (target, texture);  /* the real one */
      CHECK("glBindTexture");
    }
}



/* Matrix functions, mostly cribbed from Mesa.
 */

void
jwzgles_glFrustum (GLfloat left,   GLfloat right,
                   GLfloat bottom, GLfloat top,
                   GLfloat near,   GLfloat far)
{
    GLfloat m[16];
    GLfloat x = (2 * near)        / (right-left);
    GLfloat y = (2 * near)        / (top - bottom);
    GLfloat a = (right + left)    / (right - left);
    GLfloat b = (top + bottom)    / (top - bottom);
    GLfloat c = -(far + near)     / (far - near);
    GLfloat d = -(2 * far * near) / (far - near);

    # define M(X,Y)  m[Y * 4 + X]
    M(0,0) = x; M(0,1) = 0; M(0,2) =  a; M(0,3) = 0;
    M(1,0) = 0; M(1,1) = y; M(1,2) =  b; M(1,3) = 0;
    M(2,0) = 0; M(2,1) = 0; M(2,2) =  c; M(2,3) = d;
    M(3,0) = 0; M(3,1) = 0; M(3,2) = -1; M(3,3) = 0;
    # undef M

    jwzgles_glMultMatrixf (m);
}


void
jwzgles_glOrtho (GLfloat left,   GLfloat right,
                 GLfloat bottom, GLfloat top,
                 GLfloat near,   GLfloat far)
{
  GLfloat m[16];
  GLfloat a = 2 / (right - left);
  GLfloat b = -(right + left) / (right - left);
  GLfloat c = 2 / (top - bottom);
  GLfloat d = -(top + bottom) / (top - bottom);
  GLfloat e = -2 / (far - near);
  GLfloat f = -(far + near) / (far - near);

# define M(X,Y)  m[Y * 4 + X]
  M(0,0) = a; M(0,1) = 0; M(0,2) = 0; M(0,3) = b;
  M(1,0) = 0; M(1,1) = c; M(1,2) = 0; M(1,3) = d;
  M(2,0) = 0; M(2,1) = 0; M(2,2) = e; M(2,3) = f;
  M(3,0) = 0; M(3,1) = 0; M(3,2) = 0; M(3,3) = 1;
# undef M

  jwzgles_glMultMatrixf (m);
}


void
jwzgles_gluPerspective (GLdouble fovy, GLdouble aspect,
                        GLdouble near, GLdouble far)
{
  GLfloat m[16];
  double si, co, dz;
  double rad = fovy / 2 * M_PI / 180;
  double a, b, c, d;

  dz = far - near;
  si = sin(rad);
  if (dz == 0 || si == 0 || aspect == 0)
    return;
  co = cos(rad) / si;

  a = co / aspect;
  b = co;
  c = -(far + near) / dz;
  d = -2 * near * far / dz;

# define M(X,Y)  m[Y * 4 + X]
  M(0,0) = a; M(0,1) = 0; M(0,2) = 0;  M(0,3) = 0;
  M(1,0) = 0; M(1,1) = b; M(1,2) = 0;  M(1,3) = 0;
  M(2,0) = 0; M(2,1) = 0; M(2,2) = c;  M(2,3) = d;
  M(3,0) = 0; M(3,1) = 0; M(3,2) = -1; M(3,3) = 0;
# undef M

  jwzgles_glMultMatrixf (m);
}


void
jwzgles_gluLookAt (GLfloat eyex, GLfloat eyey, GLfloat eyez,
                   GLfloat centerx, GLfloat centery, GLfloat centerz,
                   GLfloat upx, GLfloat upy, GLfloat upz)
{
  GLfloat m[16];
  GLfloat x[3], y[3], z[3];
  GLfloat mag;

  /* Make rotation matrix */

  /* Z vector */
  z[0] = eyex - centerx;
  z[1] = eyey - centery;
  z[2] = eyez - centerz;
  mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
  if (mag) {          /* mpichler, 19950515 */
    z[0] /= mag;
    z[1] /= mag;
    z[2] /= mag;
  }

  /* Y vector */
  y[0] = upx;
  y[1] = upy;
  y[2] = upz;

  /* X vector = Y cross Z */
  x[0] = y[1] * z[2] - y[2] * z[1];
  x[1] = -y[0] * z[2] + y[2] * z[0];
  x[2] = y[0] * z[1] - y[1] * z[0];

  /* Recompute Y = Z cross X */
  y[0] = z[1] * x[2] - z[2] * x[1];
  y[1] = -z[0] * x[2] + z[2] * x[0];
  y[2] = z[0] * x[1] - z[1] * x[0];

  /* mpichler, 19950515 */
  /* cross product gives area of parallelogram, which is < 1.0 for
   * non-perpendicular unit-length vectors; so normalize x, y here
   */

  mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
  if (mag) {
    x[0] /= mag;
    x[1] /= mag;
    x[2] /= mag;
  }

  mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
  if (mag) {
    y[0] /= mag;
    y[1] /= mag;
    y[2] /= mag;
  }

#define M(row,col)  m[col*4+row]
  M(0, 0) = x[0]; M(0, 1) = x[1]; M(0, 2) = x[2]; M(0, 3) = 0.0;
  M(1, 0) = y[0]; M(1, 1) = y[1]; M(1, 2) = y[2]; M(1, 3) = 0.0;
  M(2, 0) = z[0]; M(2, 1) = z[1]; M(2, 2) = z[2]; M(2, 3) = 0.0;
  M(3, 0) = 0.0;  M(3, 1) = 0.0;  M(3, 2) = 0.0;  M(3, 3) = 1.0;
#undef M

  jwzgles_glMultMatrixf(m);

  /* Translate Eye to Origin */
  jwzgles_glTranslatef(-eyex, -eyey, -eyez);
}


static void __gluMultMatrixVecd (const GLdouble matrix[16],
                                 const GLdouble in[4],
                                 GLdouble out[4])
{
  int i;

  for (i=0; i<4; i++) {
    out[i] =
      in[0] * matrix[0*4+i] +
      in[1] * matrix[1*4+i] +
      in[2] * matrix[2*4+i] +
      in[3] * matrix[3*4+i];
  }
}

GLint
jwzgles_gluProject (GLdouble objx, GLdouble objy, GLdouble objz,
                    const GLdouble modelMatrix[16],
                    const GLdouble projMatrix[16],
                    const GLint viewport[4],
                    GLdouble *winx, GLdouble *winy, GLdouble *winz)
{
  GLdouble in[4];
  GLdouble out[4];

  /* #### I suspect this is not working right.  I was seeing crazy values
     in lament.c.  Maybe there's some float-vs-double confusion going on?
   */

  in[0]=objx;
  in[1]=objy;
  in[2]=objz;
  in[3]=1.0;
  __gluMultMatrixVecd(modelMatrix, in, out);
  __gluMultMatrixVecd(projMatrix, out, in);
  if (in[3] == 0.0) return GL_FALSE;
  in[0] /= in[3];
  in[1] /= in[3];
  in[2] /= in[3];
  /* Map x, y and z to range 0-1 */
  in[0] = in[0] * 0.5 + 0.5;
  in[1] = in[1] * 0.5 + 0.5;
  in[2] = in[2] * 0.5 + 0.5;

  /* Map x,y to viewport */
  in[0] = in[0] * viewport[2] + viewport[0];
  in[1] = in[1] * viewport[3] + viewport[1];

  *winx=in[0];
  *winy=in[1];
  *winz=in[2];
  return GL_TRUE;
}


void jwzgles_glViewport (GLuint x, GLuint y, GLuint w, GLuint h)
{
# if TARGET_IPHONE_SIMULATOR
/*  fprintf (stderr, "glViewport %dx%d\n", w, h); */
# endif
  glViewport (x, y, w, h);  /* the real one */
}


/* The following functions are present in both OpenGL 1.1 and in OpenGLES 1,
   but are allowed within glNewList/glEndList, so we must wrap them to allow
   them to either be recorded in lists, or run directly.

   All this CPP obscenity is me screaming in rage at all the ways that C is
   not Lisp, as all I want to do here is DEFADVICE.
 */

#define PROTO_V   PROTO_VOID
#define TYPE_V    GLuint
#define ARGS_V    void
#define VARS_V    /* */
#define LOGS_V    "\n"
#define FILL_V    /* */

#define TYPE_I    GLuint
#define TYPE_II   TYPE_I
#define TYPE_III  TYPE_I
#define TYPE_IIII TYPE_I
#define ARGS_I    TYPE_I a
#define ARGS_II   TYPE_I a, TYPE_I b
#define ARGS_III  TYPE_I a, TYPE_I b, TYPE_I c
#define ARGS_IIII TYPE_I a, TYPE_I b, TYPE_I c, TYPE_I d
#define LOGS_I    "%s\n", mode_desc(a)
#define LOGS_II   "%s %d\n", mode_desc(a), b
#define LOGS_III  "%s %s %s\n", mode_desc(a), mode_desc(b), mode_desc(c)
#define LOGS_IIII "%d %d %d %d\n", a, b, c, d
#define VARS_I    a
#define VARS_II   a, b
#define VARS_III  a, b, c
#define VARS_IIII a, b, c, d
#define FILL_I    vv[0].i = a;
#define FILL_II   vv[0].i = a; vv[1].i = b;
#define FILL_III  vv[0].i = a; vv[1].i = b; vv[2].i = c;
#define FILL_IIII vv[0].i = a; vv[1].i = b; vv[2].i = c; vv[3].i = d;

#define TYPE_F    GLfloat
#define TYPE_FF   TYPE_F
#define TYPE_FFF  TYPE_F
#define TYPE_FFFF TYPE_F
#define ARGS_F    TYPE_F a
#define ARGS_FF   TYPE_F a, TYPE_F b
#define ARGS_FFF  TYPE_F a, TYPE_F b, TYPE_F c
#define ARGS_FFFF TYPE_F a, TYPE_F b, TYPE_F c, TYPE_F d
#define LOGS_F    "%7.3f\n", a
#define LOGS_FF   "%7.3f %7.3f\n", a, b
#define LOGS_FFF  "%7.3f %7.3f %7.3f\n", a, b, c
#define LOGS_FFFF "%7.3f %7.3f %7.3f %7.3f\n", a, b, c, d
#define VARS_F    VARS_I
#define VARS_FF   VARS_II
#define VARS_FFF  VARS_III
#define VARS_FFFF VARS_IIII
#define FILL_F    vv[0].f = a;
#define FILL_FF   vv[0].f = a; vv[1].f = b;
#define FILL_FFF  vv[0].f = a; vv[1].f = b; vv[2].f = c;
#define FILL_FFFF vv[0].f = a; vv[1].f = b; vv[2].f = c; vv[3].f = d;

#define ARGS_IF   TYPE_I a, TYPE_F b
#define VARS_IF   VARS_II
#define LOGS_IF   "%s %7.3f\n", mode_desc(a), b
#define FILL_IF   vv[0].i = a; vv[1].f = b;

#define ARGS_IIF  TYPE_I a, TYPE_I b, TYPE_F c
#define VARS_IIF  VARS_III
#define LOGS_IIF  "%s %s %7.3f\n", mode_desc(a), mode_desc(b), c
#define FILL_IIF  vv[0].i = a; vv[1].i = b; vv[2].f = c;

#define TYPE_IV   GLint
#define ARGS_IIV  TYPE_I a, const TYPE_IV *b
#define VARS_IIV  VARS_II
#define LOGS_IIV  "%s %d %d %d %d\n", mode_desc(a), b[0], b[1], b[2], b[3]
#define FILL_IIV  vv[0].i = a; \
		  vv[1].i = b[0]; vv[2].i = b[1]; \
		  vv[3].i = b[2]; vv[4].i = b[3];

#define ARGS_IFV  TYPE_I a, const TYPE_F *b
#define VARS_IFV  VARS_II
#define LOGS_IFV  "%s %7.3f %7.3f %7.3f %7.3f\n", mode_desc(a), \
		  b[0], b[1], b[2], b[3]
#define FILL_IFV  vv[0].i = a; \
		  vv[1].f = b[0]; vv[2].f = b[1]; \
		  vv[3].f = b[2]; vv[4].f = b[3];

#define ARGS_IIIV TYPE_I a, TYPE_I b, const TYPE_IV *c
#define VARS_IIIV VARS_III
#define LOGS_IIIV "%s %-8s %3d %3d %3d %3d\n", mode_desc(a), mode_desc(b), \
		  c[0], c[1], c[2], c[3]
#define FILL_IIIV vv[0].i = a; vv[1].i = b; \
		  vv[2].i = c[0]; vv[3].i = c[1]; \
		  vv[4].i = c[2]; vv[5].i = c[3];

#define ARGS_IIFV TYPE_I a, TYPE_I b, const TYPE_F *c
#define VARS_IIFV VARS_III
#define LOGS_IIFV "%s %-8s %7.3f %7.3f %7.3f %7.3f\n", \
		  mode_desc(a), mode_desc(b), \
		  c[0], c[1], c[2], c[3]
#define FILL_IIFV vv[0].i = a; vv[1].i = b; \
		  vv[2].f = c[0]; vv[3].f = c[1]; \
		  vv[4].f = c[2]; vv[5].f = c[3];

#ifdef DEBUG
# define WLOG(NAME,ARGS) \
  fprintf (stderr, "jwzgles: direct %-12s ", NAME); \
  fprintf (stderr, ARGS)
#else
# define WLOG(NAME,ARGS) /* */
#endif

#define WRAP(NAME,SIG) \
void jwzgles_##NAME (ARGS_##SIG)					\
{									\
  Assert (!state->compiling_verts,					\
          STRINGIFY(NAME) " not allowed inside glBegin");		\
  if (state->compiling_list) {						\
    void_int vv[10];							\
    FILL_##SIG								\
    list_push (STRINGIFY(NAME), (list_fn_cb) &jwzgles_##NAME,		\
	       PROTO_##SIG, vv);					\
  } else {                                                          	\
    if (! state->replaying_list) {                                   	\
      WLOG (STRINGIFY(NAME), LOGS_##SIG);		        	\
    }									\
    NAME (VARS_##SIG);							\
    CHECK(STRINGIFY(NAME));						\
  }									\
}

WRAP (glActiveTexture,	I)
WRAP (glAlphaFunc,	IF)
WRAP (glBlendFunc,	II)
WRAP (glClear,		I)
WRAP (glClearColor,	FFFF)
WRAP (glClearStencil,	I)
WRAP (glColorMask,	IIII)
WRAP (glCullFace,	I)
WRAP (glDepthFunc,	I)
WRAP (glDepthMask,	I)
WRAP (glFinish,		V)
WRAP (glFlush,		V)
WRAP (glFogf,		IF)
WRAP (glFogfv,		IFV)
WRAP (glFrontFace,	I)
WRAP (glHint,		II)
WRAP (glLightModelf,	IF)
WRAP (glLightModelfv,	IFV)
WRAP (glLightf,		IIF)
WRAP (glLightfv,	IIFV)
WRAP (glLineWidth,	F)
WRAP (glLoadIdentity,	V)
WRAP (glLogicOp,	I)
WRAP (glMatrixMode,	I)
WRAP(glPixelStorei, II)
WRAP (glPointSize,	F)
WRAP (glPolygonOffset,	FF)
WRAP (glPopMatrix,	V)
WRAP (glPushMatrix,	V)
WRAP (glRotatef,	FFFF)
WRAP (glScalef,		FFF)
WRAP (glScissor,	IIII)
WRAP (glShadeModel,	I)
WRAP (glStencilFunc,	III)
WRAP (glStencilMask,	I)
WRAP (glStencilOp,	III)
WRAP (glTexEnvf,	IIF)
WRAP (glTexEnvi,	III)
WRAP (glTranslatef,	FFF)
#undef  TYPE_IV
#define TYPE_IV GLuint
WRAP (glDeleteTextures,	IIV)


#endif /* HAVE_JWZGLES - whole file */
