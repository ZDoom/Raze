// Windows DIB/DirectDraw interface layer
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __build_interface_layer__
#define __build_interface_layer__ WIN

extern int backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive
extern unsigned maxrefreshfreq;

extern int glusecds;

long win_gethwnd(void);
long win_gethinstance(void);

// resource ids
#define WIN_STARTWIN		1000
#define WIN_STARTWIN_ITEMBITMAP	100	// banner bitmap
#define WIN_STARTWIN_ITEMTEXT	101	// text header
#define WIN_STARTWIN_ITEMLIST	102	// output list box
#define WIN_STARTWINBMP		200

	// *hwnd      - receives the window handle to the startup dialog box
	// saferect[] - receives the safe area to draw controls over (l,t,w,h)
	// onclose    - called before the startup window closes
	//  returns 0 if successful, 1 if the window isn't open
int win_getstartupwin(long *hwnd, long saferect[4], void (*onclose)(void));
int win_getstartupcommand(void);

void win_allowtaskswitching(int onf);
int win_checkinstance(void);

/*
#ifdef KENBUILD
#define DISCLAIMER "IMPORTANT:\n" \
		"\tThis is a source port by Jonathon Fowler (jonof@edgenetwk.com) of the Build Engine, " \
		"editor and test game by Ken Silverman to the Windows and Linux operating systems. It is " \
		"distributed under the terms listed in BUILDLIC.TXT included with this package."
#endif
#ifdef DUKE3D
#define DISCLAIMER "IMPORTANT:\n" \
		"\tThis port of Duke Nukem 3D was done by Jonathon Fowler (jonof@edgenetwk.com) and is not " \
		"endorsed or supported by 3D Realms or Apogee. They will not provide support for it. Visit " \
		"http://jonof.edgenetwk.com/buildport/duke3d/ for details."
#endif
*/		
#include "baselayer.h"

#else
#if (__build_interface_layer__ != WIN)
#error "Already using the " __build_interface_layer__ ". Can't now use Windows."
#endif
#endif // __build_interface_layer__

