
#ifndef BGLBUILD_H_INCLUDED_
#define BGLBUILD_H_INCLUDED_

#ifdef USE_OPENGL

#if !defined GEKKO
# define USE_GLEXT
#endif

#if defined EDUKE32_OSX
# include <OpenGL/glu.h>
#else
# include <GL/glu.h>
#endif

#endif //USE_OPENGL


#endif
