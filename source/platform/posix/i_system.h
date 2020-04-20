#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include <dirent.h>
#include <ctype.h>

#if defined(__sun) || defined(__sun__) || defined(__SRV4) || defined(__srv4__)
#define __solaris__ 1
#endif

#include <thread>
#include <algorithm>
#include "tarray.h"
#include "zstring.h"

struct ticcmd_t;
struct WadStuff;

#ifndef SHARE_DIR
#define SHARE_DIR "/usr/local/share/"
#endif


// Called by DoomMain.
void I_Init (void);

// Return a seed value for the RNG.
unsigned int I_MakeRNGSeed();
void I_ShowFatalError(const char* msg);
void I_DetectOS (void);

void I_StartFrame (void);

void I_StartTic (void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd (void);

void I_Tactile (int on, int off, int total);

// Print a console string
void I_PrintStr (const char *str);

// Set the title string of the startup window
void I_SetIWADInfo ();

// Pick from multiple IWADs to use
int I_PickIWad (WadStuff *wads, int numwads, bool queryiwad, int defaultiwad);

// [RH] Checks the registry for Steam's install path, so we can scan its
// directories for IWADs if the user purchased any through Steam.
TArray<FString> I_GetSteamPath();

TArray<FString> I_GetGogPaths();

// The ini could not be saved at exit
bool I_WriteIniFailed ();

class FGameTexture;
bool I_SetCursor(FGameTexture *);

static inline char *strlwr(char *str)
{
	char *ptr = str;
	while(*ptr)
	{
		*ptr = tolower(*ptr);
		++ptr;
	}
	return str;
}

inline int I_GetNumaNodeCount() { return 1; }
inline int I_GetNumaNodeThreadCount(int numaNode) { return std::max<int>(std::thread::hardware_concurrency(), 1); }
inline void I_SetThreadNumaNode(std::thread &thread, int numaNode) { }

void I_PutInClipboard(const char* str);
FString I_GetFromClipboard (bool use_primary_selection);

#endif
