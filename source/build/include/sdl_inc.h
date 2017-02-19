#ifndef SDL_INC_H_
#define SDL_INC_H_

// include this before SDL does
#ifdef _WIN32
# include "windows_inc.h"
#endif

// Workaround for i686-MinGW-w64.
#if defined __MINGW64_VERSION_MAJOR && !defined __MINGW64__
# define __MINGW64_VERSION_MAJOR_BACKUP __MINGW64_VERSION_MAJOR
# undef __MINGW64_VERSION_MAJOR
#endif

#if defined SDL_USEFOLDER
# if SDL_TARGET == 2
#  include <SDL2/SDL.h>
#  include <SDL2/SDL_syswm.h>
# else
#  include <SDL/SDL.h>
#  include <SDL/SDL_syswm.h>
# endif
#else
# include "SDL.h"
# if !defined __APPLE__
#  include "SDL_syswm.h"
# endif
#endif

#if defined __MINGW64_VERSION_MAJOR_BACKUP && !defined __MINGW64__
# define __MINGW64_VERSION_MAJOR __MINGW64_VERSION_MAJOR_BACKUP
# undef __MINGW64_VERSION_MAJOR_BACKUP
#endif

/* =================================================================
Minimum required SDL versions:
=================================================================== */

#define SDL_MIN_X	1
#define SDL_MIN_Y	2
#define SDL_MIN_Z	10

#define SDL_REQUIREDVERSION	(SDL_VERSIONNUM(SDL_MIN_X,SDL_MIN_Y,SDL_MIN_Z))

#define SDL_MIXER_MIN_X	1
#define SDL_MIXER_MIN_Y	2
#define SDL_MIXER_MIN_Z	7
#define MIX_REQUIREDVERSION	(SDL_VERSIONNUM(SDL_MIXER_MIN_X,SDL_MIXER_MIN_Y,SDL_MIXER_MIN_Z))

#if !(SDL_VERSION_ATLEAST(SDL_MIN_X,SDL_MIN_Y,SDL_MIN_Z))
#error SDL version found is too old
#endif

#if defined _NEED_SDLMIXER

# if defined SDL_USEFOLDER
#  if SDL_TARGET == 2
#   include <SDL2/SDL_mixer.h>
#  else
#   include <SDL/SDL_mixer.h>
#  endif
# else
#  include "SDL_mixer.h"
# endif

/* starting with 1.2.1, SDL_mixer provides MIX_xxx version
   macros. the new SDL_MIXER_xxx version macros start with
   1.2.6 but they keep backwards compatibility. 1.2.0 does
   not even have any version macros, so let's reject it */
# if !defined(MIX_MAJOR_VERSION) || !defined(MIX_MINOR_VERSION) || !defined(MIX_PATCHLEVEL)
#  error SDL_mixer version found is too old
# endif

# ifndef MIX_COMPILEDVERSION
#  define MIX_COMPILEDVERSION	(SDL_VERSIONNUM(MIX_MAJOR_VERSION,MIX_MINOR_VERSION,MIX_PATCHLEVEL))
# endif
# if MIX_COMPILEDVERSION < MIX_REQUIREDVERSION
#  error SDL_mixer version found is too old
# endif

# if SDL_MAJOR_VERSION != MIX_MAJOR_VERSION
#  error Major version of SDL_mixer headers does not match SDL headers
# endif

#endif

#endif	/* SDL_INC_H_ */
