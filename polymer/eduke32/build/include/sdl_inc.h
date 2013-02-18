#ifndef __SDL_INC_H
#define __SDL_INC_H

#if defined(SDL_FRAMEWORK)
#  include <SDL/SDL.h>
#else
#  include "SDL.h"
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

#if defined(_NEED_SDLMIXER)
# if defined(SDL_FRAMEWORK)
#  if defined(_WIN32) || defined(GEKKO)
#   include <SDL/SDL_mixer.h>
#  else
#   include <SDL_mixer/SDL_mixer.h>
#  endif
# else
#  include "SDL_mixer.h"
# endif
/* starting with 1.2.1, SDL_mixer provides MIX_xxx version
   macros. the new SDL_MIXER_xxx version macros start with
   1.2.6 but they keep backwards compatibility. 1.2.0 does
   not even have any version macros, so let's reject it */
#if !defined(MIX_MAJOR_VERSION) || !defined(MIX_MINOR_VERSION) || !defined(MIX_PATCHLEVEL)
#error SDL_mixer version found is too old
#endif
#ifndef MIX_COMPILEDVERSION
#define MIX_COMPILEDVERSION	(SDL_VERSIONNUM(MIX_MAJOR_VERSION,MIX_MINOR_VERSION,MIX_PATCHLEVEL))
#endif
#if MIX_COMPILEDVERSION < MIX_REQUIREDVERSION
#error SDL_mixer version found is too old
#endif	/* end of bad version error */
#endif	/* end of SDL_mixer checks */

#endif	/* __SDL_INC_H */

