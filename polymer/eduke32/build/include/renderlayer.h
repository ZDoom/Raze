
#ifdef RENDERTYPEWIN
# include "winlayer.h"
#else
# include "sdlayer.h"
#endif

#undef STARTUP_SETUP_WINDOW
#if defined _WIN32 || (defined RENDERTYPESDL && ((defined __APPLE__ && defined OSX_STARTUPWINDOW) || defined HAVE_GTK2))
# define STARTUP_SETUP_WINDOW
#endif

#undef WM_MSGBOX_WINDOW
#if defined _WIN32 || (defined RENDERTYPESDL && ((defined __APPLE__ && defined OSX_STARTUPWINDOW) || defined HAVE_GTK2 || SDL_MAJOR_VERSION==2))
# define WM_MSGBOX_WINDOW
#endif
