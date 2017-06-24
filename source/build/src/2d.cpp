#include "build.h"
#include "editor.h"

int32_t editorgridextent = 131072;

////////// editor side view //////////
int32_t m32_sideview = 0;
int32_t m32_sideelev = 256;  // elevation in BUILD degrees, 0..512
int16_t m32_sideang = 200;  // azimuth, 0..2047

int32_t m32_sidecos, m32_sidesin;
int32_t m32_swcnt;
int32_t m32_wallscreenxy[MAXWALLS][2];
int16_t m32_wallsprite[MAXWALLS+MAXSPRITES];
static int32_t m32_sidedist[MAXWALLS+MAXSPRITES];
static vec3_t m32_viewplane;

static void drawpixel_safe(void *s, char a)
{
#if defined __GNUC__
    if (__builtin_expect((intptr_t) s >= frameplace && (intptr_t) s < frameplace+bytesperline*ydim, 1))
#else
    if ((intptr_t) s >= frameplace && (intptr_t) s < frameplace+bytesperline*ydim)
#endif
        drawpixel(s, a);
#ifdef DEBUGGINGAIDS
    else
    {
        const char c = editorcolors[15];

        drawpixel((intptr_t *) frameplace, c);
        drawpixel((intptr_t *) frameplace+1, c);
        drawpixel((intptr_t *) frameplace+2, c);
        drawpixel((intptr_t *) frameplace+bytesperline, c);
        drawpixel((intptr_t *) frameplace+bytesperline+1, c);
        drawpixel((intptr_t *) frameplace+bytesperline+2, c);
        drawpixel((intptr_t *) frameplace+2*bytesperline, c);
        drawpixel((intptr_t *) frameplace+2*bytesperline+1, c);
        drawpixel((intptr_t *) frameplace+2*bytesperline+2, c);
    }
#endif
}



//
// plotpixel
//
void plotpixel(int32_t x, int32_t y, char col)
{
    // XXX: if we ever want the editor to work under GL ES, find a replacement for the raster functions
#if defined USE_OPENGL && !defined EDUKE32_GLES
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        palette_t p = getpal(col);

        bglRasterPos4i(x, y, 0, 1);
        bglDrawPixels(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &p);
        bglRasterPos4i(0, 0, 0, 1);
        return;
    }
#endif

    begindrawing(); //{{{
    drawpixel_safe((void *) (ylookup[y]+x+frameplace), col);
    enddrawing();   //}}}
}

void plotlines2d(const int32_t *xx, const int32_t *yy, int32_t numpoints, int col)
{
    int32_t i;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode())
    {
        palette_t p = getpal(col);

        bglBegin(GL_LINE_STRIP);

        bglColor4ub(p.r, p.g, p.b, 1);

        for (i=0; i<numpoints; i++)
            bglVertex2i(xx[i], yy[i]);

        bglEnd();
        return;
    }
#endif
    {
        int32_t odrawlinepat = drawlinepat;
        drawlinepat = 0xffffffff;

        begindrawing();
        for (i=0; i<numpoints-1; i++)
            drawline16(xx[i], yy[i], xx[i+1], yy[i+1], col);
        enddrawing();

        drawlinepat = odrawlinepat;
    }
}


//
// getpixel
//
char getpixel(int32_t x, int32_t y)
{
    char r;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && in3dmode()) return 0;
#endif

    begindrawing(); //{{{
    r = readpixel((void *) (ylookup[y]+x+frameplace));
    enddrawing();   //}}}
    return r;
}


//
// drawline256
//
#ifdef USE_OPENGL
static void drawlinegl(int32_t x1, int32_t y1, int32_t x2, int32_t y2, palette_t p)
{
    //        setpolymost2dview();	// JBF 20040205: more efficient setup

    bglViewport(0, 0, xres, yres);
    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bglOrtho(0, xres, yres, 0, -1, 1);
    if (getrendermode() == REND_POLYMER)
    {
        bglMatrixMode(GL_MODELVIEW);
        bglLoadIdentity();
    }

    gloy1 = -1;
    bglDisable(GL_ALPHA_TEST);
    bglDisable(GL_DEPTH_TEST);
    bglDisable(GL_TEXTURE_2D);
    bglEnable(GL_BLEND);	// When using line antialiasing, this is needed

    bglBegin(GL_LINES);
    bglColor4ub(p.r, p.g, p.b, 255);
    bglVertex2f((float) x1 * (1.f/4096.f), (float) y1 * (1.f/4096.f));
    bglVertex2f((float) x2 * (1.f/4096.f), (float) y2 * (1.f/4096.f));

    bglEnd();
}
#endif

static void drawlinepixels(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    int32_t dx, dy, i, j, inc, plc, daend;
    intptr_t p;

    dx = x2-x1; dy = y2-y1;
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

    if (klabs(dx) >= klabs(dy))
    {
        if (dx == 0) return;
        if (dx < 0)
        {
            i = x1; x1 = x2; x2 = i;
            i = y1; y1 = y2; y2 = i;
        }

        inc = divscale12(dy, dx);
        plc = y1+mulscale12((2047-x1)&4095, inc);
        i = ((x1+2048)>>12); daend = ((x2+2048)>>12);

        begindrawing(); //{{{
        for (; i<daend; i++)
        {
            j = (plc>>12);
            if ((j >= startumost[i]) && (j < startdmost[i]))
                drawpixel_safe((void *) (frameplace+ylookup[j]+i), col);
            plc += inc;
        }
        enddrawing();   //}}}
    }
    else
    {
        if (dy < 0)
        {
            i = x1; x1 = x2; x2 = i;
            i = y1; y1 = y2; y2 = i;
        }

        inc = divscale12(dx, dy);
        plc = x1+mulscale12((2047-y1)&4095, inc);
        i = ((y1+2048)>>12); daend = ((y2+2048)>>12);

        begindrawing(); //{{{
        p = ylookup[i]+frameplace;
        for (; i<daend; i++)
        {
            j = (plc>>12);
            if ((i >= startumost[j]) && (i < startdmost[j]))
                drawpixel_safe((void *) (j+p), col);
            plc += inc; p += ylookup[1];
        }
        enddrawing();   //}}}
    }
}

void drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, palette_t p)
{
#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        drawlinegl(x1, y1, x2, y2, p);
        return;
    }
#endif

    char const col = palookup[0][p.f];
    drawlinepixels(x1, y1, x2, y2, col);
}

void drawline256(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    col = palookup[0][col];

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        palette_t p = getpal(col);
        p.f = col;
        drawlinegl(x1, y1, x2, y2, p);
        return;
    }
#endif

    drawlinepixels(x1, y1, x2, y2, col);
}


//static void attach_here() {}

//
// drawline16
//
// JBF: Had to add extra tests to make sure x-coordinates weren't winding up -'ve
//   after clipping or crashes would ensue
uint32_t drawlinepat = 0xffffffff;

int32_t drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int col)
{
    //int32_t odx,ody;
    //int32_t ox1=x1,oy1=y1, ox2=x2,oy2=y2;

    vec2_t d = { x2-x1, y2-y1 };

    //odx=dx;
    //ody=dy;

    if (d.x < 0)
    {
        swaplong(&x1, &x2);
        swaplong(&y1, &y2);
    }

    if (x1 >= xres || x2 < 0)
        return 0;

    if (x1 < 0)
    {
        if (d.y) y1 += scale(0-x1, d.y, d.x);
        x1 = 0;
    }

    if (x2 >= xres)
    {
        if (d.y) y2 += scale(xres-1-x2, d.y, d.x);
        x2 = xres-1;
    }

    if ((d.x < 0 && d.y >= 0) || (d.y < 0 && d.x >= 0))
    {
        swaplong(&x1, &x2);
        swaplong(&y1, &y2);
    }

    if (y1 >= ydim16 || y2 < 0)
        return 0;

    if (y1 < 0)
    {
        if (d.x)
            x1 = clamp(x1 + scale(0 - y1, d.x, d.y), 0, xres - 1);
        y1 = 0;
    }

    if (y2 >= ydim16)
    {
        if (d.x)
            x2 = clamp(x2 + scale(ydim16-1-y2, d.x, d.y), 0, xres-1);
        y2 = ydim16-1;
    }

    if (d.y < 0)
    {
        swaplong(&x1, &x2);
        swaplong(&y1, &y2);
    }

    //if (ox1||ox2||oy1||oy2)
    //    if (x1<0||x1>=xres || y2<0||y2>=yres)
    //        attach_here();

    d.x = klabs(x2-x1)+1;
    d.y = klabs(y2-y1)+1;

    if ((d.x >= d.y && x2 < x1) || (d.x < d.y && y2 < y1))
    {
        swaplong(&x1, &x2);
        swaplong(&y1, &y2);
    }

    int pinc, inc = 1;

    begindrawing(); //{{{

    intptr_t p = (y1*bytesperline)+x1+frameplace;

    if (d.x >= d.y)
    {
        pinc = (y2 > y1) ? bytesperline : -bytesperline;
    }
    else
    {
        pinc = (x2 > x1) ? 1 : -1;
        swaplong(&d.x, &d.y);
        inc = bytesperline;
    }

    int const trans = (col < 0);

    if (trans)
    {
        col = -col;

        if (drawlinepat == 0xffffffff)
        {
            for (bssize_t i=d.x, df=0; i>0; i--)
            {
                drawtranspixel((char *) p, col);
                df += d.y;
                if (df >= d.x) { df -= d.x; p += pinc; }
                p += inc;
            }
        }
        else
        {
            uint32_t patc = UINT_MAX;

            for (bssize_t i=d.x, df=0; i>0; i--)
            {
                if (drawlinepat & pow2long[(++patc)&31])
                    drawtranspixel((char *) p, col);
                df += d.y;
                if (df >= d.x) { df -= d.x; p += pinc; }
                p += inc;
            }
        }

        enddrawing();   //}}}

        return 1;
    }

    if (inc == 1 && d.y == 1 && drawlinepat == 0xffffffff)
        clearbufbyte((void *) p, d.x, ((int32_t) col<<24)|((int32_t) col<<16)|((int32_t) col<<8)|col);
    else if (drawlinepat == 0xffffffff)
    {
        for (bssize_t i=d.x, df=0; i>0; i--)
        {
            drawpixel((char *) p, col);
            df += d.y;
            if (df >= d.x) { df -= d.x; p += pinc; }
            p += inc;
        }
    }
    else
    {
        uint32_t patc = UINT_MAX;

        for (bssize_t i=d.x, df=0; i>0; i--)
        {
            if (drawlinepat & pow2long[(++patc)&31])
                drawpixel((char *) p, col);
            df += d.y;
            if (df >= d.x) { df -= d.x; p += pinc; }
            p += inc;
        }
    }

    enddrawing();   //}}}

    return 1;
}

static FORCE_INLINE void drawline16mid(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    drawline16(halfxdim16+x1, midydim16+y1, halfxdim16+x2, midydim16+y2, col);
}

// eccen: eccentricity of the ellipse,
//   16384: circle
//  <16384: shrink in y
//  >16384: grow in y
void drawcircle16(int32_t x1, int32_t y1, int32_t r, int32_t eccen, char col)
{
    if (eccen != 16384)
    {
        // JonoF's rough approximation of a circle
        int32_t l, spx, spy, lpx, lpy, px, py;

        spx = lpx = x1 + mulscale14(r, sintable[0]);
        spy = lpy = y1 + mulscale14(eccen, mulscale14(r, sintable[512]));

        for (l=64; l<2048; l+=64)
        {
            px = x1 + mulscale14(r, sintable[l]);
            py = y1 + mulscale14(eccen, mulscale14(r, sintable[(l+512)&2047]));

            drawline16(lpx, lpy, px, py, col);

            lpx = px;
            lpy = py;
        }

        drawline16(lpx, lpy, spx, spy, col);

        return;
    }

    if (r < 0) r = -r;
    if (x1+r < 0 || y1+r < 0 || x1-r >= xres || y1-r >= ydim16) return;

    uint32_t const uxres = xres, uydim16 = ydim16;

    /*
    *      d
    *    6 | 7
    *   \  |  /
    *  5  \|/  8
    * c----+----a
    *  4  /|\  1
    *   /  |  \
    *    3 | 2
    *      b
    */

    begindrawing();
    intptr_t const p = (y1*bytesperline)+x1+frameplace;

    uint32_t patc = UINT_MAX;

    if (drawlinepat == 0xffffffff || drawlinepat & pow2long[(++patc)&31])
    {
        if ((uint32_t) y1 < uydim16 && (uint32_t) (x1+r) < uxres)
            drawpixel((char *) (p+r), col);    // a
        if ((uint32_t) x1 < uxres   && (uint32_t) (y1+r) < uydim16)
            drawpixel((char *) (p+(r*bytesperline)), col);    // b
        if ((uint32_t) y1 < uydim16 && (uint32_t) (x1-r) < uxres)
            drawpixel((char *) (p-r), col);    // c
        if ((uint32_t) x1 < uxres   && (uint32_t) (y1-r) < uydim16)
            drawpixel((char *) (p-(r*bytesperline)), col);    // d
    }

    int32_t xp = 0, yp = r;
    int32_t d = 1 - r, de = 2, dse = 5 - (r << 1);

    if (drawlinepat != 0xffffffff)
    {
        do
        {
            if (d < 0)
            {
                d += de;
                dse += 2;
            }
            else
            {
                d += dse;
                dse += 4;
                yp--;
            }

            xp++;
            de += 2;

            int32_t const ypbpl = yp*bytesperline;
            int32_t const xpbpl = xp*bytesperline;

            if (drawlinepat & pow2long[(++patc) & 31])
            {
                if ((uint32_t) (x1 + yp) < uxres && (uint32_t) (y1 + xp) < uydim16)
                    drawpixel_safe((char *) (p + yp + xpbpl), col);  // 1
                if ((uint32_t) (x1 + xp) < uxres && (uint32_t) (y1 + yp) < uydim16)
                    drawpixel_safe((char *) (p + xp + ypbpl), col);  // 2
                if ((uint32_t) (x1 - xp) < uxres && (uint32_t) (y1 + yp) < uydim16)
                    drawpixel_safe((char *) (p - xp + ypbpl), col);  // 3
                if ((uint32_t) (x1 - yp) < uxres && (uint32_t) (y1 + xp) < uydim16)
                    drawpixel_safe((char *) (p - yp + xpbpl), col);  // 4
                if ((uint32_t) (x1 - yp) < uxres && (uint32_t) (y1 - xp) < uydim16)
                    drawpixel_safe((char *) (p - yp - xpbpl), col);  // 5
                if ((uint32_t) (x1 - xp) < uxres && (uint32_t) (y1 - yp) < uydim16)
                    drawpixel_safe((char *) (p - xp - ypbpl), col);  // 6
                if ((uint32_t) (x1 + xp) < uxres && (uint32_t) (y1 - yp) < uydim16)
                    drawpixel_safe((char *) (p + xp - ypbpl), col);  // 7
                if ((uint32_t) (x1 + yp) < uxres && (uint32_t) (y1 - xp) < uydim16)
                    drawpixel_safe((char *) (p + yp - xpbpl), col);  // 8
            }
        } while (yp > xp);

        enddrawing();
        return;
    }

    do
    {
        if (d < 0)
        {
            d += de;
            dse += 2;
        }
        else
        {
            d += dse;
            dse += 4;
            yp--;
        }

        xp++;
        de += 2;

        int32_t const ypbpl = yp*bytesperline;
        int32_t const xpbpl = xp*bytesperline;

        if ((uint32_t) (x1 + yp) < uxres && (uint32_t) (y1 + xp) < uydim16)
            drawpixel_safe((char *) (p + yp + xpbpl), col);  // 1
        if ((uint32_t) (x1 + xp) < uxres && (uint32_t) (y1 + yp) < uydim16)
            drawpixel_safe((char *) (p + xp + ypbpl), col);  // 2
        if ((uint32_t) (x1 - xp) < uxres && (uint32_t) (y1 + yp) < uydim16)
            drawpixel_safe((char *) (p - xp + ypbpl), col);  // 3
        if ((uint32_t) (x1 - yp) < uxres && (uint32_t) (y1 + xp) < uydim16)
            drawpixel_safe((char *) (p - yp + xpbpl), col);  // 4
        if ((uint32_t) (x1 - yp) < uxres && (uint32_t) (y1 - xp) < uydim16)
            drawpixel_safe((char *) (p - yp - xpbpl), col);  // 5
        if ((uint32_t) (x1 - xp) < uxres && (uint32_t) (y1 - yp) < uydim16)
            drawpixel_safe((char *) (p - xp - ypbpl), col);  // 6
        if ((uint32_t) (x1 + xp) < uxres && (uint32_t) (y1 - yp) < uydim16)
            drawpixel_safe((char *) (p + xp - ypbpl), col);  // 7
        if ((uint32_t) (x1 + yp) < uxres && (uint32_t) (y1 - xp) < uydim16)
            drawpixel_safe((char *) (p + yp - xpbpl), col);  // 8
    } while (yp > xp);

    enddrawing();
}

//
// clear2dscreen
//
void clear2dscreen(void)
{
    int32_t const clearsz = (ydim16 <= yres - STATUS2DSIZ2) ? yres - STATUS2DSIZ2 : yres;
    begindrawing();  //{{{
    Bmemset((char *) frameplace, 0, bytesperline*clearsz);
    enddrawing();   //}}}
}


////////// editor side view //////////

int32_t scalescreeny(int32_t sy) { return (m32_sideview) ? mulscale14(sy, m32_sidesin) : sy; }

// return screen coordinates for BUILD coords x and y (relative to current position)
void screencoords(int32_t *xres, int32_t *yres, int32_t x, int32_t y, int32_t zoome)
{
    vec2_t coord = { x, y };

    if (m32_sideview)
        rotatepoint(zerovec, coord, m32_sideang, &coord);

    *xres = mulscale14(coord.x, zoome);
    *yres = scalescreeny(mulscale14(coord.y, zoome));
}

#if 0
void invscreencoords(int32_t *dx, int32_t *dy, int32_t sx, int32_t sy, int32_t zoome)
{
    if (m32_sidesin==0 || zoome==0) { *dx=0; *dy=0; return; }

    sy = divscale14(divscale14(sy, m32_sidesin), zoome);
    sx = divscale14(sx, zoome);

    rotatepoint(0, 0, sx, sy, -m32_sideang, dx, dy);
}
#endif

// invscreencoords with sx==0 and sy==getscreenvdisp(dz, zoom)
int32_t getinvdisplacement(int32_t *dx, int32_t *dy, int32_t dz)
{
    if (m32_sidesin==0)
        return 1;

    dz = (((int64_t) dz * (int64_t) m32_sidecos)/(int64_t) m32_sidesin)>>4;

    vec2_t v[2] = { { 0, dz },{ *dx, *dy } };

    rotatepoint(zerovec, v[0], -m32_sideang, &v[1]);

    *dx = v[1].x;
    *dy = v[1].y;

    return 0;
}

// return vertical screen coordinate displacement for BUILD z coord
int32_t getscreenvdisp(int32_t bz, int32_t zoome)
{
    return mulscale32(bz, zoome*m32_sidecos);
}

void setup_sideview_sincos(void)
{
    if (m32_sideview)
    {
        m32_viewplane.x = 0;
        m32_viewplane.y = -512;

        m32_sidesin = sintable[m32_sideelev&2047];
        m32_sidecos = sintable[(m32_sideelev+512)&2047];

        rotatepoint(zerovec, *(vec2_t *) &m32_viewplane, -m32_sideang, (vec2_t *) &m32_viewplane);
        m32_viewplane.x = mulscale14(m32_viewplane.x, m32_sidecos);
        m32_viewplane.y = mulscale14(m32_viewplane.y, m32_sidecos);
        m32_viewplane.z = m32_sidesin>>5;
    }
}

static void sideview_getdist(int16_t sw, int16_t sect)
{
    vec3_t *p;
    vec3_t v;

    if (sw<MAXWALLS)
    {
        v.x = (wall[sw].x + wall[wall[sw].point2].x)>>1;
        v.y = (wall[sw].y + wall[wall[sw].point2].y)>>1;
        v.z = getflorzofslope(sect, v.x, v.y);
        p = &v;
    }
    else
        p = (vec3_t *) &sprite[sw-MAXWALLS];

    m32_sidedist[sw] = p->x*m32_viewplane.x + p->y*m32_viewplane.y + (p->z>>4)*m32_viewplane.z;
}

static int sideview_cmppoints(const void *sw1, const void *sw2)
{
    int32_t dist1 = m32_sidedist[B_UNBUF16(sw1)];
    int32_t dist2 = m32_sidedist[B_UNBUF16(sw2)];

    if (dist2>dist1)
        return 1;
    else if (dist1>dist2)
        return -1;

    //    if (*sw1<MAXWALLS && *sw2<MAXWALLS)
    //        return (wall[*sw2].nextwall>=0) - (wall[*sw1].nextwall>=0);

    return 0;
}

//
// draw2dgrid
//
void draw2dgrid(int32_t posxe, int32_t posye, int32_t posze, int16_t cursectnum, int16_t ange, int32_t zoome, int16_t gride)
{
    int64_t i, xp1, yp1, xp2=0, yp2, tempy;

    UNREFERENCED_PARAMETER(ange);

    if (gride <= 0)
        return;

    begindrawing();	//{{{

    if (m32_sideview)
    {
        int32_t sx1, sy1, sx2, sy2, dx=0, dy=0;
        int32_t xinc=0, yinc=2048>>gride, yofs;

        //        yofs = getscreenvdisp((yinc-posze)&((yinc<<4)-1), zoome);
        if (cursectnum<0 || cursectnum>=numsectors)
            yofs = getscreenvdisp(-posze, zoome);
        else
            yofs = getscreenvdisp(getflorzofslope(cursectnum, posxe, posye)-posze, zoome);

        while (scalescreeny(mulscale14(yinc, zoome))==0 && gride>2)
        {
            gride--;
            yinc = 2048>>gride;
        }

        xp2 = xp1 = ((posxe + (1024 >> gride)) & -(((int64_t)(1)) << (11 - gride)));
        yp2 = yp1 = ((posye + (1024 >> gride)) & -(((int64_t)(1)) << (11 - gride)));

        do
        {
            if (xinc==0)
            {
                screencoords(&sx1, &sy1, -editorgridextent-posxe, yp2-posye, zoome);
                if (yp2 == yp1)
                {
                    screencoords(&sx2, &sy2, editorgridextent-posxe, yp2-posye, zoome);
                    dx = sx2-sx1;
                    dy = sy2-sy1;
                }
                yp2 += yinc;
            }
            else  // if (yinc==0)
            {
                screencoords(&sx1, &sy1, xp2-posxe, -editorgridextent-posye, zoome);
                if (xp2 == xp1)
                {
                    screencoords(&sx2, &sy2, xp2-posxe, editorgridextent-posye, zoome);
                    dx = sx2-sx1;
                    dy = sy2-sy1;
                }
                xp2 += xinc;
            }

            i = drawline16(halfxdim16+sx1, midydim16+sy1+yofs, halfxdim16+sx1+dx, midydim16+sy1+dy+yofs, editorcolors[25]);
            if (i==0 || (xp2<-editorgridextent || xp2>editorgridextent ||
                yp2<-editorgridextent || yp2>editorgridextent))
            {
                xp2 = xp1;
                yp2 = yp1;

                i = 1;

                if (yinc>0)
                    yinc *= -1;
                else if (yinc<0)
                {
                    xinc = -yinc;
                    yinc = 0;
                }
                else if (xinc>0)
                    xinc *= -1;
                else // if (xinc<0)
                    i = 0;
            }
        } while (i);
    }
    else
    {
        // vertical lines
        yp1 = midydim16-mulscale14(posye+editorgridextent, zoome);
        if (yp1 < 0) yp1 = 0;

        yp2 = midydim16-mulscale14(posye-editorgridextent, zoome);
        if (yp2 >= ydim16) yp2 = ydim16-1;

        if ((yp1 < ydim16) && (yp2 >= 0) && (yp2 >= yp1))
        {
            xp1 = halfxdim16-mulscale14(posxe+editorgridextent, zoome);

            for (i=-editorgridextent; i<=editorgridextent; i+=(2048>>gride))
            {
                xp2 = xp1;
                xp1 = halfxdim16-mulscale14(posxe-i, zoome);

                if (xp1 >= xdim)
                    break;

                if (xp1 >= 0)
                {
                    if (xp1 != xp2)
                        drawline16(xp1, yp1, xp1, yp2, editorcolors[25]);
                }
            }
            if (i >= editorgridextent && xp1 < xdim)
                xp2 = xp1;
            if (xp2 >= 0 && xp2 < xdim)
                drawline16(xp2, yp1, xp2, yp2, editorcolors[25]);
        }

        // horizontal lines
        xp1 = mulscale14(posxe+editorgridextent, zoome);
        xp2 = mulscale14(posxe-editorgridextent, zoome);
        tempy = 0x80000000l;

        for (i=-editorgridextent; i<=editorgridextent; i+=(2048>>gride))
        {
            yp1 = ((posye-i)*zoome)>>14;
            if (yp1 != tempy)
            {
                if ((yp1 > midydim16-ydim16) && (yp1 <= midydim16))
                {
                    drawline16mid(-xp1, -yp1, -xp2, -yp1, editorcolors[25]);
                    tempy = yp1;
                }
            }
        }
    }

    enddrawing();   //}}}
}


static void drawscreen_drawwall(int32_t i, int32_t posxe, int32_t posye, int32_t posze, int32_t zoome, int32_t grayp)
{
    const walltype *const wal = &wall[i];

    int32_t j = wal->nextwall;
#if 0
    if (editstatus == 0)
    {
        if ((show2dwall[i>>3]&pow2char[i&7]) == 0) return;
        if ((j >= 0) && (i > j))
            if ((show2dwall[j>>3]&pow2char[j&7]) > 0) return;
    }
    else
#endif
    {
        if (!m32_sideview && !(grayp&2) && (j >= 0) && (i > j)) return;
    }

    char col;

    if (grayp&1)
        col = editorcolors[8];
    else if (j < 0)
        col = (i == linehighlight) ? editorcolors[15] - M32_THROB : editorcolors[15];
    else
    {
        if ((unsigned) wal->nextwall < MAXWALLS && ((wal->cstat^wall[j].cstat)&1))
            col = editorcolors[2];
        else if ((wal->cstat&1) != 0)
            col = editorcolors[5];
        else col = editorcolors[4];

        if (i == linehighlight || (linehighlight >= 0 && i == wall[linehighlight].nextwall))
            col += M32_THROB>>2;
    }

    int const p2 = wal->point2;

    int32_t x1, y1, x2, y2;
    screencoords(&x1, &y1, wal->x-posxe, wal->y-posye, zoome);
    screencoords(&x2, &y2, wall[p2].x-posxe, wall[p2].y-posye, zoome);

    int64_t const dx = wal->x-wall[p2].x;
    int64_t const dy = wal->y-wall[p2].y;
    int64_t const dist = dx*dx + dy*dy;

    int const bothSidesHighlighted = ((show2dwall[i>>3]&pow2char[i&7]) && (show2dwall[p2>>3]&pow2char[p2&7]));

    if (dist > INT32_MAX)
    {
        col=editorcolors[9];
        if (i == linehighlight || (linehighlight >= 0 && i == wall[linehighlight].nextwall))
            col -= M32_THROB>>3;
    }
    else if ((showfirstwall && searchsector>=0 && (sector[searchsector].wallptr == i ||
        sector[searchsector].wallptr == wal->nextwall)) ||
        bothSidesHighlighted)
    {
        col = editorcolors[14];
        if (i == linehighlight || (linehighlight >= 0 && i == wall[linehighlight].nextwall) || bothSidesHighlighted)
            col -= M32_THROB>>1;
    }
    else if (circlewall >= 0 && (i == circlewall || wal->nextwall == circlewall))
        col = editorcolors[14];

    int32_t fz=0, fzn=0;

    if (m32_sideview)
    {
        // draw vertical line to neighboring wall
        int32_t const sect = sectorofwall(i);

        fz = getflorzofslope(sect, wal->x, wal->y);
        int32_t fz2 = getflorzofslope(sect, wall[p2].x, wall[p2].y);

        int32_t const dz = getscreenvdisp(fz-posze, zoome);
        int32_t const dz2 = getscreenvdisp(fz2-posze, zoome);

        y1 += dz;
        y2 += dz2;

        if (wal->nextwall>=0)
        {
            fzn = getflorzofslope(wal->nextsector, wal->x, wal->y);
            //            if (i < wall[j].point2)
            drawline16mid(x1, y1, x1, y1+getscreenvdisp(fzn-fz, zoome), col);
        }
#ifdef YAX_ENABLE
        {
            int16_t const nw = yax_getnextwall(i, YAX_CEILING);

            if (nw >= 0)
            {
                int32_t const odrawlinepat = drawlinepat;
                fz2 = getflorzofslope(sectorofwall(nw), wall[nw].x, wall[nw].y);
                drawlinepat = 0x11111111;
                drawline16mid(x1, y1, x1, y1+getscreenvdisp(fz2-fz, zoome), col);
                drawlinepat = odrawlinepat;
            }
        }
#endif

        m32_wallscreenxy[i][0] = halfxdim16+x1;
        m32_wallscreenxy[i][1] = midydim16+y1;
    }

    if (wal->cstat&64)  // if hitscan bit set
    {
        int32_t const one=(klabs(x2-x1) >= klabs(y2-y1)), no=!one;

        drawline16mid(x1+no, y1+one, x2+no, y2+one, col);
        drawline16mid(x1-no, y1-one, x2-no, y2-one, col);
    }

    drawline16mid(x1, y1, x2, y2, col);

    // Draw height indicators at center of walls if requested and if not in
    // side-view mode.
    // XXX: This does not take sloping into account.
    if (showheightindicators && !m32_sideview)
    {
        int32_t dax, day;
        int32_t const k = getangle(x1-x2, y1-y2);

        screencoords(&dax, &day,
            ((wal->x+wall[wal->point2].x)>>1)-posxe,
            ((wal->y+wall[wal->point2].y)>>1)-posye, zoome);

        if (wal->nextsector >= 0)
        {
            int32_t const z1 = sector[sectorofwall(i)].floorz;
            int32_t const z2 = sector[wal->nextsector].floorz;

            if (z1 != z2 || showheightindicators == 2)
            {
                // Red walls. Show them on equal-height walls ONLY with setting 2.
                int32_t const bb = (z2 < z1);
                int32_t const dx = mulscale11(sintable[(k+1024 + 1024*bb)&2047], min(4096, zoome)) / 2560;
                int32_t const dy = scalescreeny(mulscale11(sintable[(k+512 + 1024*bb)&2047], min(4096, zoome)) / 2560);

                drawline16mid(dax, day, dax+dx, day+dy, col);
            }
        }
        else if (showheightindicators == 2)
        {
            // Show them on white walls ONLY with setting 2.
            int32_t const dx = mulscale11(sintable[(k+2048)&2047], min(4096, zoome)) / 2560;
            int32_t const dy = scalescreeny(mulscale11(sintable[(k+1536)&2047], min(4096, zoome)) / 2560);

            drawline16mid(dax, day, dax+dx, day+dy, col);
        }
    }

    if ((zoome >= 256 && editstatus == 1) || show2dwall[i>>3]&pow2char[i&7])
        if ((halfxdim16+x1 >= 2) && (halfxdim16+x1 <= xdim-3) &&
            (midydim16+y1 >= 2) && (midydim16+y1 <= ydim16-3))
        {
            int32_t pointsize = 2;

            col = editorcolors[15];

            if (i == pointhighlight || ((unsigned) pointhighlight < MAXWALLS &&
                (wal->x == wall[pointhighlight].x) && (wal->y == wall[pointhighlight].y)))
            {
                col = editorcolors[15] - (M32_THROB>>1);

                if (totalclock & 16)
                    pointsize++;
            }

            if (show2dwall[i>>3]&pow2char[i&7])
                col = editorcolors[14] - (M32_THROB>>1);

            if (m32_sideview)
            {
                if (wal->nextwall >= 0)
                {
                    if (fz < fzn)
                        col = editorcolors[7];
                    else if (fz == fzn)
                        col = editorcolors[4];
                }
            }

            drawcircle16(halfxdim16+x1, midydim16+y1, pointsize, 16384, col);
        }
}

int32_t getspritecol(int32_t spr)
{
    int const picnum = sprite[spr].picnum;
    int pal = sprite[spr].pal;
    int const tilecol = tilecols[picnum];

    if (palookup[pal] == NULL || (tilecol && palookup[pal][tilecol] == 0))
        pal = 0;

    if (tilecol) return palookup[pal][tilecol];

    if (!waloff[picnum]) loadtile(picnum);
    if (!waloff[picnum]) return editorcolors[3];

    // Calculate 2D mode tile color.

    uint32_t cols[256];

    Bmemset(cols, 0, sizeof(cols));

    const uint8_t *const texbuf = (const uint8_t *) waloff[picnum];

    for (bssize_t i = 0; i < tilesiz[picnum].x * tilesiz[picnum].y; i++)
        cols[texbuf[i]]++;

    unsigned col = 0, cnt = 0;

    for (bssize_t i = 0; i < 240; i++)
        if (cols[i] > cnt)
            col = i, cnt = cols[i];

    while (col < 240 && curpalette[col+1].r > curpalette[col].r)
        col++;

    tilecols[picnum] = col - 4;

    return palookup[pal][tilecols[picnum]];
}

static void drawscreen_drawsprite(int32_t j, int32_t posxe, int32_t posye, int32_t posze, int32_t zoome)
{
    int32_t x1, y1, x2, y2;
    int col;

    const spritetype *const spr = &sprite[j];
    int16_t const blocking = (spr->cstat&1), hitblocking = (spr->cstat&256);
    int16_t const flooraligned = (spr->cstat&32), wallaligned = (spr->cstat&16);

    int16_t const angofs = m32_sideview ? m32_sideang : 0;
    uint8_t const spritecol = spritecol2d[spr->picnum][blocking];

    // KEEPINSYNC build.c: drawspritelabel()
    if (spr->sectnum<0)
        col = editorcolors[4];  // red
    else
        col = spritecol ? editorcolors[spritecol] : blocking ? editorcolors[5] : getspritecol(j);

    if (editstatus == 1)
    {
        if (pointhighlight >= 16384 &&
            (j+16384 == pointhighlight ||
            (!m32_sideview && (spr->x == sprite[pointhighlight-16384].x &&
                spr->y == sprite[pointhighlight-16384].y))))
        {
            if (spritecol >= 8 && spritecol <= 15)
                col -= M32_THROB>>1;
            else col += M32_THROB>>2;
        }
        else // if (highlightcnt > 0)
        {
            if (show2dsprite[j>>3]&pow2char[j&7])
                col = editorcolors[14] - (M32_THROB>>1);
        }
    }

    screencoords(&x1, &y1, spr->x-posxe, spr->y-posye, zoome);
    //   tempint = ((midydim16+y1)*bytesperline)+(halfxdim16+x1)+frameplace;

    if (m32_sideview)
        y1 += getscreenvdisp(spr->z-posze, zoome);

    int f = mulscale12(128, zoome);

    if ((halfxdim16+x1 >= -f) && (halfxdim16+x1 < xdim+f) &&
        (midydim16+y1 >= -f) && (midydim16+y1 < ydim16+f))
    {
        if (zoome > 512 && spr->clipdist > 32)
            drawcircle16(halfxdim16+x1, midydim16+y1, mulscale14(spr->clipdist<<2, zoome), 16384, col);

        drawcircle16(halfxdim16+x1, midydim16+y1, 4, 16384, col);

        x2 = mulscale11(sintable[(spr->ang+angofs+2560)&2047], zoome) / 768;
        y2 = mulscale11(sintable[(spr->ang+angofs+2048)&2047], zoome) / 768;
        y2 = scalescreeny(y2);

        drawline16mid(x1, y1, x1+x2, y1+y2, col);

        if (hitblocking)
        {
            drawline16mid(x1, y1+1, x1+x2, y1+y2+1, col);
            drawline16mid(x1, y1-1, x1+x2, y1+y2-1, col);
            drawline16mid(x1-1, y1, x1+x2-1, y1+y2, col);
            drawline16mid(x1+1, y1, x1+x2+1, y1+y2, col);
        }

        if (flooraligned)
        {
            int32_t fx = mulscale10(mulscale6(tilesiz[spr->picnum].x, spr->xrepeat), zoome) >> 1;
            int32_t fy = mulscale10(mulscale6(tilesiz[spr->picnum].y, spr->yrepeat), zoome) >> 1;
            int32_t co[4][2], ii, in;
            int32_t sinang = sintable[(spr->ang+angofs+1536)&2047];
            int32_t cosang = sintable[(spr->ang+angofs+1024)&2047];
            int32_t r, s;

            co[0][0] = co[3][0] = -fx;
            co[0][1] = co[1][1] = -fy;
            co[1][0] = co[2][0] = fx;
            co[2][1] = co[3][1] = fy;

            for (ii=3; ii>=0; ii--)
            {
                r = mulscale14(cosang, co[ii][0]) - mulscale14(sinang, co[ii][1]);
                s = mulscale14(sinang, co[ii][0]) + mulscale14(cosang, co[ii][1]);
                s = scalescreeny(s);
                co[ii][0] = r;
                co[ii][1] = s;
            }
            drawlinepat = 0xcfcfcfcf;
            for (ii=3; ii>=0; ii--)
            {
                in = (ii+1)&3;
                drawline16mid(x1+co[ii][0], y1-co[ii][1], x1+co[in][0], y1-co[in][1], col);
                if (hitblocking)
                {
                    drawline16mid(x1+co[ii][0], y1-co[ii][1]+1, x1+co[in][0], y1-co[in][1]+1, col);
                    drawline16mid(x1+co[ii][0], y1-co[ii][1]-1, x1+co[in][0], y1-co[in][1]-1, col);
                    drawline16mid(x1+co[ii][0]+1, y1-co[ii][1], x1+co[in][0]+1, y1-co[in][1], col);
                    drawline16mid(x1+co[ii][0]-1, y1-co[ii][1], x1+co[in][0]-1, y1-co[in][1], col);
                }
                drawline16mid(x1, y1, x1 + co[in][0], y1 - co[in][1], col);
            }
            drawlinepat = 0xffffffff;
        }
        else if (wallaligned)
        {
            int32_t fx = mulscale6(tilesiz[spr->picnum].x, spr->xrepeat);
            int32_t one=(((spr->ang+angofs+256)&512) == 0), no=!one;

            x2 = mulscale11(sintable[(spr->ang+angofs+2560)&2047], zoome) / 6144;
            y2 = mulscale11(sintable[(spr->ang+angofs+2048)&2047], zoome) / 6144;
            y2 = scalescreeny(y2);

            drawline16mid(x1, y1, x1+x2, y1+y2, col);
            if (!(spr->cstat&64))  // not 1-sided
            {
                drawline16mid(x1, y1, x1-x2, y1-y2, col);
                if (hitblocking)
                {
                    drawline16mid(x1-no, y1-one, x1-x2-no, y1-y2-one, col);
                    drawline16mid(x1+no, y1+one, x1-x2+no, y1-y2+one, col);
                }
            }

            if (hitblocking)
            {
                drawline16mid(x1-no, y1-one, x1+x2-no, y1+y2-one, col);
                drawline16mid(x1+no, y1+one, x1+x2+no, y1+y2+one, col);
            }


            x2 = mulscale13(sintable[(spr->ang+angofs+1024)&2047], zoome) * fx / 4096;
            y2 = mulscale13(sintable[(spr->ang+angofs+512)&2047], zoome) * fx / 4096;
            y2 = scalescreeny(y2);

            drawline16mid(x1, y1, x1-x2, y1-y2, col);
            drawline16mid(x1, y1, x1+x2, y1+y2, col);

            if (hitblocking)
            {
                drawline16mid(x1+1, y1, x1+x2+1, y1+y2, col);
                drawline16mid(x1-1, y1, x1-x2-1, y1-y2, col);
                drawline16mid(x1-1, y1, x1+x2-1, y1+y2, col);
                drawline16mid(x1+1, y1, x1-x2+1, y1-y2, col);

                drawline16mid(x1, y1-1, x1+x2, y1+y2-1, col);
                drawline16mid(x1, y1+1, x1-x2, y1-y2+1, col);
                drawline16mid(x1, y1+1, x1+x2, y1+y2+1, col);
                drawline16mid(x1, y1-1, x1-x2, y1-y2-1, col);
            }
        }
    }
}

//
// draw2dscreen
//

static int8_t tempbuf[(MAXWALLS+7)>>3];

void draw2dscreen(const vec3_t *pos, int16_t cursectnum, int16_t ange, int32_t zoome, int16_t gride)
{
    int32_t i, x1, y1;
    int16_t angofs = m32_sideview ? m32_sideang : 0;

    int32_t posxe=pos->x, posye=pos->y, posze=pos->z;
    uint8_t *graybitmap = (uint8_t *) tempbuf;
    int32_t alwaysshowgray = get_alwaysshowgray();

    if (in3dmode()) return;

    setup_sideview_sincos();

    begindrawing(); //{{{


    if (editstatus == 0)
    {
        //        faketimerhandler();
        clear2dscreen();

        //        faketimerhandler();
        draw2dgrid(posxe, posye, posze, cursectnum, ange, zoome, gride);
    }

    faketimerhandler();

    m32_swcnt = 0;

    if (numgraysects==0)
        Bmemset(graybitmap, 0, (numwalls+7)>>3);
    else
    {
        for (i=0; i<numwalls; i++)
        {
            int32_t j = wall[i].nextwall;
            if ((graywallbitmap[i>>3]&(1<<(i&7))) && (j < 0 || (graywallbitmap[j>>3]&(1<<(j&7)))))
                graybitmap[i>>3] |= (1<<(i&7));
            else
                graybitmap[i>>3] &= ~(1<<(i&7));
        }
    }

    if (!m32_sideview)
    {
#ifndef YAX_ENABLE
        for (i=numwalls-1; i>=0; i--)
            drawscreen_drawwall(i, posxe, posye, posze, zoome, 0);
#else
        if (alwaysshowgray)
            for (i=numwalls-1; i>=0; i--)
                if (graybitmap[i>>3]&(1<<(i&7)))
                    drawscreen_drawwall(i, posxe, posye, posze, zoome, 1+2);

        for (i=numwalls-1; i>=0; i--)
            if ((graybitmap[i>>3]&(1<<(i&7)))==0)
                drawscreen_drawwall(i, posxe, posye, posze, zoome, 2);
#endif
    }
    else
    {
        int32_t j = 0;

        for (i=0; i<numsectors; i++)
            for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
            {
                m32_wallsprite[m32_swcnt++] = j;
                sideview_getdist(j, i);
            }

        // j = sector[numsectors-1].wallptr + sector[numsectors-1].wallnum
        for (; j < numwalls; j++)  // new walls ...
        {
            m32_wallsprite[m32_swcnt++] = j;
            sideview_getdist(j, 0);
        }
    }

    faketimerhandler();

    if (zoome >= 256 || highlightcnt>0)
        for (bssize_t j=0; j<MAXSPRITES; j++)
            if (sprite[j].statnum<MAXSTATUS)
            {
                // if sprite is highlighted, always draw it
                if ((show2dsprite[j>>3]&pow2char[j&7])==0)
                {
                    if (!m32_sideview && sprite[j].sectnum >= 0)
                        YAX_SKIPSECTOR(sprite[j].sectnum);

                    if (zoome<256)
                        continue;
                }

                if (!m32_sideview)
                    drawscreen_drawsprite(j, posxe, posye, posze, zoome);
                else
                {
                    m32_wallsprite[m32_swcnt++] = MAXWALLS+j;
                    sideview_getdist(MAXWALLS+j, -1);
                }
            }

    faketimerhandler();

    if (m32_sideview)
    {
        qsort(m32_wallsprite, m32_swcnt, sizeof(int16_t), &sideview_cmppoints);

        for (i=0; i<m32_swcnt; i++)  // shouldn't it go the other way around?
        {
            int32_t j = m32_wallsprite[i];
            if (j<MAXWALLS)
            {
                if (alwaysshowgray || !(graybitmap[j>>3]&(1<<(j&7))))
                    drawscreen_drawwall(j, posxe, posye, posze, zoome, !!(graybitmap[j>>3]&(1<<(j&7))));
            }
            else
            {
                if (!alwaysshowgray && sprite[j-MAXWALLS].sectnum>=0)
                    YAX_SKIPSECTOR(sprite[j-MAXWALLS].sectnum);

                drawscreen_drawsprite(j-MAXWALLS, posxe, posye, posze, zoome);
            }
        }

        faketimerhandler();
    }

#if 0
    {
        int32_t xx, yy, xx2, yy2;
        screencoords(&xx, &yy, -posxe, -posye, zoome);
        screencoords(&xx2, &yy2, (m32_viewplane.x)-posxe, (m32_viewplane.y)-posye, zoome);
        if (m32_sideview)
            yy2 += getscreenvdisp((m32_viewplane.z<<4)-posze, zoome);

        drawcircle16(halfxdim16+xx, midydim16+yy, 2, 16384, editorcolors[4]); //red
        drawcircle16(halfxdim16+xx2, midydim16+yy2, 2, 16384, editorcolors[14]); //yellow
        drawline16mid(xx, yy, xx2, yy2, editorcolors[15]);
    }
#endif

    x1 = mulscale11(sintable[(ange+angofs+2560)&2047], zoome) / 768; //Draw white arrow
    y1 = mulscale11(sintable[(ange+angofs+2048)&2047], zoome) / 768;

    i = scalescreeny(x1);
    int32_t j = scalescreeny(y1);

    drawline16mid(x1, j, -x1, -j, editorcolors[15]);
    drawline16mid(x1, j, +y1, -i, editorcolors[15]);
    drawline16mid(x1, j, -y1, +i, editorcolors[15]);


    enddrawing();   //}}}
}

//
// setpolymost2dview
//  Sets OpenGL for 2D drawing
//
void setpolymost2dview(void)
{
#ifdef USE_OPENGL
    if (getrendermode() < REND_POLYMOST) return;

    bglViewport(0, 0, xres, yres);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bglOrtho(0, xres, yres, 0, -1, 1);

    if (getrendermode() == REND_POLYMER)
    {
        bglMatrixMode(GL_MODELVIEW);
        bglLoadIdentity();
    }

    gloy1 = -1;

    bglDisable(GL_DEPTH_TEST);
    bglDisable(GL_TEXTURE_2D);
    bglDisable(GL_BLEND);
#endif
}
