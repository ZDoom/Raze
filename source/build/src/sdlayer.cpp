// SDL interface layer for the Build Engine
// Use SDL 1.2 or 2.0 from http://www.libsdl.org

#ifdef _WIN32
#include <Windows.h>
#include <CommCtrl.h>
#endif
#include <signal.h>
#include <string>
#include <stdexcept>
# include "gl_load.h"

#include "a.h"
#include "build.h"

#include "common.h"
#include "compat.h"
#include "engine_priv.h"
#include "osd.h"
#include "palette.h"
#include "baselayer.h"
#include "m_argv.h"
#include "mmulti.h"
#include "scriptfile.h"
#include "zstring.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "resourcefile.h"
#include "sc_man.h"
#include "i_specialpaths.h"
#include "inputstate.h"
#include "c_cvars.h"
#include "i_time.h"
#include "c_dispatch.h"
#include "d_gui.h"
#include "menu.h"
#include "utf8.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
# include "glsurface.h"



double g_beforeSwapTime;
GameInterface* gi;

int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS];

void ImGui_Init_Backend()
{
	//ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_context);
}

void ImGui_Begin_Frame()
{
	//ImGui_ImplOpenGL3_NewFrame();
	//ImGui_ImplSDL2_NewFrame(sdl_window);
	//ImGui::NewFrame();
}

int32_t xres=-1, yres=-1, bpp=0, bytesperline, refreshfreq=-1;
intptr_t frameplace=0;
int32_t lockcount=0;
char modechange=1;
char offscreenrendering=0;

#define MAX_ERRORTEXT 4096

//==========================================================================
//
// I_Error
//
// Throw an error that will send us to the console if we are far enough
// along in the startup process.
//
//==========================================================================

void I_Error(const char *error, ...)
{
	va_list argptr;
	char errortext[MAX_ERRORTEXT];

	va_start(argptr, error);
	vsnprintf(errortext, MAX_ERRORTEXT, error, argptr);
	va_end(argptr);
	#ifdef _WIN32
	OutputDebugStringA(errortext);
	#endif

	throw std::runtime_error(errortext);
}

void I_FatalError(const char* error, ...)
{
    va_list argptr;
    char errortext[MAX_ERRORTEXT];

    va_start(argptr, error);
    vsnprintf(errortext, MAX_ERRORTEXT, error, argptr);
    va_end(argptr);
#ifdef _WIN32
    OutputDebugStringA(errortext);
#endif

    throw std::runtime_error(errortext);
}


// Calculate ylookup[] and call setvlinebpl()
void calc_ylookup(int32_t bpl, int32_t lastyidx)
{
    int32_t i, j = 0;
    static int32_t ylookupsiz;

    Bassert(lastyidx <= MAXYDIM);

    lastyidx++;

    if (lastyidx > ylookupsiz)
    {
        ylookup.Resize(lastyidx);
        ylookupsiz = lastyidx;
    }

    for (i = 0; i <= lastyidx - 4; i += 4)
    {
        ylookup[i] = j;
        ylookup[i + 1] = j + bpl;
        ylookup[i + 2] = j + (bpl << 1);
        ylookup[i + 3] = j + (bpl * 3);
        j += (bpl << 2);
    }

    for (; i < lastyidx; i++)
    {
        ylookup[i] = j;
        j += bpl;
    }

    setvlinebpl(bpl);
}

//
// begindrawing() -- locks the framebuffer for drawing
//

void videoBeginDrawing(void)
{
    if (bpp > 8)
    {
        if (offscreenrendering) return;
        frameplace = 0;
        bytesperline = 0;
        modechange = 0;
        return;
    }

    // lock the frame
    if (lockcount++ > 0)
        return;

    static intptr_t backupFrameplace = 0;

    if (inpreparemirror)
    {
        //POGO: if we are offscreenrendering and we need to render a mirror
        //      or we are rendering a mirror and we start offscreenrendering,
        //      backup our offscreen target so we can restore it later
        //      (but only allow one level deep,
        //       i.e. no viewscreen showing a camera showing a mirror that reflects the same viewscreen and recursing)
        if (offscreenrendering)
        {
            if (!backupFrameplace)
                backupFrameplace = frameplace;
            else if (frameplace != (intptr_t)mirrorBuffer.Data() &&
                     frameplace != backupFrameplace)
                return;
        }

        frameplace = (intptr_t)mirrorBuffer.Data();

        if (offscreenrendering)
            return;
    }
    else if (offscreenrendering)
    {
        if (backupFrameplace)
        {
            frameplace = backupFrameplace;
            backupFrameplace = 0;
        }
        return;
    }
    else
        frameplace = (intptr_t)glsurface_getBuffer();

    if (modechange)
    {
        bytesperline = xdim;
        calc_ylookup(bytesperline, ydim);
        modechange=0;
    }
}


//
// enddrawing() -- unlocks the framebuffer
//
void videoEndDrawing(void)
{
    if (bpp > 8)
    {
        if (!offscreenrendering) frameplace = 0;
        return;
    }

    if (!frameplace) return;
    if (lockcount > 1) { lockcount--; return; }
    if (!offscreenrendering) frameplace = 0;
    if (lockcount == 0) return;
    lockcount = 0;
}

//
//
// ---------------------------------------
//
// Miscellany
//
// ---------------------------------------
//
//

auto vsnprintfptr = vsnprintf;	// This is an inline in Visual Studio but we need an address for it to satisfy the MinGW compiled libraries.

//
// debugprintf() -- sends a debug string to the debugger
//
void debugprintf(const char* f, ...)
{
	va_list va;
    va_start(va, f);

#ifdef _WIN32
    if (!IsDebuggerPresent()) return;

    char buf[1024];
    vsnprintf(buf, 1024, f, va);
    va_end(va);
    OutputDebugStringA(buf);
#else
    vprintf(f, va);
#endif
}

