// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "build.h"
#include "editor.h"
#include "../../glbackend/glbackend.h"



//
// drawline256
//
#ifdef USE_OPENGL
static void drawlinegl(int32_t x1, int32_t y1, int32_t x2, int32_t y2, palette_t p)
{
    //        setpolymost2dview();	// JBF 20040205: more efficient setup

    int const dx = x2-x1;
    int const dy = y2-y1;

    if (dx >= 0)
    {
        if ((x1 >= wx2) || (x2 < wx1)) return;
        if (x1 < wx1) y1 += scale(wx1-x1, dy, dx), x1 = wx1;
        if (x2 > wx2) y2 += scale(wx2-x2, dy, dx), x2 = wx2;
    }
    else
    {
        if ((x2 >= wx2) || (x1 < wx1)) return;
        if (x2 < wx1) y2 += scale(wx1-x2, dy, dx), x2 = wx1;
        if (x1 > wx2) y1 += scale(wx2-x1, dy, dx), x1 = wx2;
    }
    if (dy >= 0)
    {
        if ((y1 >= wy2) || (y2 < wy1)) return;
        if (y1 < wy1) x1 += scale(wy1-y1, dx, dy), y1 = wy1;
        if (y2 > wy2) x2 += scale(wy2-y2, dx, dy), y2 = wy2;
    }
    else
    {
        if ((y2 >= wy2) || (y1 < wy1)) return;
        if (y2 < wy1) x2 += scale(wy1-y2, dx, dy), y2 = wy1;
        if (y1 > wy2) x1 += scale(wy2-y1, dx, dy), y1 = wy2;
    }

    glViewport(0, 0, xdim, ydim);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, xdim, ydim, 0, -1, 1);

    gloy1 = -1;
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);	// When using line antialiasing, this is needed

    polymost_useColorOnly(true);
    glColor4ub(p.r, p.g, p.b, 255);

	auto data = GLInterface.AllocVertices(2);
	data.second[0].Set((float) x1 * (1.f/4096.f), (float) y1 * (1.f/4096.f));
	data.second[1].Set((float) x2 * (1.f/4096.f), (float) y2 * (1.f/4096.f));
	GLInterface.Draw(DT_LINES, data.first, 2);

	polymost_useColorOnly(false);
}
#endif


void drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, palette_t p)
{
#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        drawlinegl(x1, y1, x2, y2, p);
        return;
    }
#endif

    char const col = palookup[0][p.f];
    drawlinepixels(x1, y1, x2, y2, col);
}

void renderDrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    col = palookup[0][col];

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        palette_t p = paletteGetColor(col);
        p.f = col;
        drawlinegl(x1, y1, x2, y2, p);
        return;
    }
#endif

    drawlinepixels(x1, y1, x2, y2, col);
}



//
// setpolymost2dview
//  Sets OpenGL for 2D drawing
//
void polymostSet2dView(void)
{
#ifdef USE_OPENGL
    if (videoGetRenderMode() < REND_POLYMOST) return;

    glViewport(0, 0, xdim, ydim);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, xdim, ydim, 0, -1, 1);

    gloy1 = -1;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
#endif
}
