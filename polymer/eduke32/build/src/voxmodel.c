//--------------------------------------- VOX LIBRARY BEGINS ---------------------------------------

#ifdef USE_OPENGL

#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "pragmas.h"
#include "baselayer.h"
#include "engine_priv.h"
#include "hightile.h"
#include "polymost.h"
#include "texcache.h"
#include "mdsprite.h"
#include "cache1d.h"
#include "kplib.h"

#include <math.h>

//For loading/conversion only
static vec3_t voxsiz;
static int32_t yzsiz, *vbit = 0; //vbit: 1 bit per voxel: 0=air,1=solid
static vec3f_t voxpiv;
static int32_t *vcolhashead = 0, vcolhashsizm1;
typedef struct { int32_t p, c, n; } voxcol_t;
static voxcol_t *vcol = 0; int32_t vnum = 0, vmax = 0;
typedef struct { int16_t x, y; } spoint2d;
static spoint2d *shp;
static int32_t *shcntmal, *shcnt = 0, shcntp;
static int32_t mytexo5, *zbit, gmaxx, gmaxy, garea, pow2m1[33];
static voxmodel_t *gvox;

//pitch must equal xsiz*4
uint32_t gloadtex(int32_t *picbuf, int32_t xsiz, int32_t ysiz, int32_t is8bit, int32_t dapal)
{
    uint32_t rtexid;
    int32_t i;

    const char *const cptr = &britable[gammabrightness ? 0 : curbrightness][0];

    // Correct for GL's RGB order; also apply gamma here:
    const coltype *const pic = (const coltype *) picbuf;
    coltype *pic2 = (coltype *) Xmalloc(xsiz*ysiz*sizeof(coltype));

    if (!is8bit)
    {
        for (i=xsiz*ysiz-1; i>=0; i--)
        {
            pic2[i].b = cptr[pic[i].r];
            pic2[i].g = cptr[pic[i].g];
            pic2[i].r = cptr[pic[i].b];
            pic2[i].a = 255;
        }
    }
    else
    {
        if (palookup[dapal] == NULL)
            dapal = 0;

        for (i=xsiz*ysiz-1; i>=0; i--)
        {
            const int32_t ii = palookup[dapal][pic[i].a] * 3;

            pic2[i].b = cptr[palette[ii+2]*4];
            pic2[i].g = cptr[palette[ii+1]*4];
            pic2[i].r = cptr[palette[ii+0]*4];
            pic2[i].a = 255;
        }
    }

    bglGenTextures(1, (GLuint *) &rtexid);
    bglBindTexture(GL_TEXTURE_2D, rtexid);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    bglTexImage2D(GL_TEXTURE_2D, 0, 4, xsiz, ysiz, 0, GL_RGBA, GL_UNSIGNED_BYTE, (char *) pic2);

    Bfree(pic2);

    return rtexid;
}

static int32_t getvox(int32_t x, int32_t y, int32_t z)
{
    z += x*yzsiz + y*voxsiz.z;
    for (x=vcolhashead[(z*214013)&vcolhashsizm1]; x>=0; x=vcol[x].n)
        if (vcol[x].p == z) return(vcol[x].c);
    return(0x808080);
}

static void putvox(int32_t x, int32_t y, int32_t z, int32_t col)
{
    if (vnum >= vmax) { vmax = max(vmax<<1, 4096); vcol = (voxcol_t *) Xrealloc(vcol, vmax*sizeof(voxcol_t)); }

    z += x*yzsiz + y*voxsiz.z;
    vcol[vnum].p = z; z = ((z*214013)&vcolhashsizm1);
    vcol[vnum].c = col;
    vcol[vnum].n = vcolhashead[z]; vcolhashead[z] = vnum++;
}

//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 0's
#if 0
static void setzrange0(int32_t *lptr, int32_t z0, int32_t z1)
{
    int32_t z, ze;
    if (!((z0^z1)&~31)) { lptr[z0>>5] &= ((~(-1<<SHIFTMOD32(z0)))|(-1<<SHIFTMOD32(z1))); return; }
    z = (z0>>5); ze = (z1>>5);
    lptr[z] &=~(-1<<SHIFTMOD32(z0)); for (z++; z<ze; z++) lptr[z] = 0;
    lptr[z] &= (-1<<SHIFTMOD32(z1));
}
#endif
//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 1's
static void setzrange1(int32_t *lptr, int32_t z0, int32_t z1)
{
    int32_t z, ze;
    if (!((z0^z1)&~31)) { lptr[z0>>5] |= ((~(-1<<SHIFTMOD32(z1)))&(-1<<SHIFTMOD32(z0))); return; }
    z = (z0>>5); ze = (z1>>5);
    lptr[z] |= (-1<<SHIFTMOD32(z0)); for (z++; z<ze; z++) lptr[z] = -1;
    lptr[z] |=~(-1<<SHIFTMOD32(z1));
}

static int32_t isrectfree(int32_t x0, int32_t y0, int32_t dx, int32_t dy)
{
#if 0
    int32_t i, j, x;
    i = y0*gvox->mytexx + x0;
    for (dy=0; dy; dy--, i+=gvox->mytexx)
        for (x=0; x<dx; x++) { j = i+x; if (zbit[j>>5]&(1<<SHIFTMOD32(j))) return(0); }
#else
    int32_t i, c, m, m1, x;

    i = y0*mytexo5 + (x0>>5); dx += x0-1; c = (dx>>5) - (x0>>5);
    m = ~pow2m1[x0&31]; m1 = pow2m1[(dx&31)+1];
    if (!c) { for (m&=m1; dy; dy--, i+=mytexo5) if (zbit[i]&m) return(0); }
    else
    {
        for (; dy; dy--, i+=mytexo5)
        {
            if (zbit[i]&m) return(0);
            for (x=1; x<c; x++) if (zbit[i+x]) return(0);
            if (zbit[i+x]&m1) return(0);
        }
    }
#endif
    return(1);
}

static void setrect(int32_t x0, int32_t y0, int32_t dx, int32_t dy)
{
#if 0
    int32_t i, j, y;
    i = y0*gvox->mytexx + x0;
    for (y=0; y<dy; y++, i+=gvox->mytexx)
        for (x=0; x<dx; x++) { j = i+x; zbit[j>>5] |= (1<<SHIFTMOD32(j)); }
#else
    int32_t i, c, m, m1, x;

    i = y0*mytexo5 + (x0>>5); dx += x0-1; c = (dx>>5) - (x0>>5);
    m = ~pow2m1[x0&31]; m1 = pow2m1[(dx&31)+1];
    if (!c) { for (m&=m1; dy; dy--, i+=mytexo5) zbit[i] |= m; }
    else
    {
        for (; dy; dy--, i+=mytexo5)
        {
            zbit[i] |= m;
            for (x=1; x<c; x++) zbit[i+x] = -1;
            zbit[i+x] |= m1;
        }
    }
#endif
}

static void cntquad(int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t face)
{
    int32_t x, y, z;

    UNREFERENCED_PARAMETER(x1);
    UNREFERENCED_PARAMETER(y1);
    UNREFERENCED_PARAMETER(z1);
    UNREFERENCED_PARAMETER(face);

    x = labs(x2-x0); y = labs(y2-y0); z = labs(z2-z0);
    if (!x) x = z; else if (!y) y = z;
    if (x < y) { z = x; x = y; y = z; }
    shcnt[y*shcntp+x]++;
    if (x > gmaxx) gmaxx = x;
    if (y > gmaxy) gmaxy = y;
    garea += (x+(VOXBORDWIDTH<<1))*(y+(VOXBORDWIDTH<<1));
    gvox->qcnt++;
}

static void addquad(int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t face)
{
    int32_t i, j, x, y, z, xx, yy, nx = 0, ny = 0, nz = 0, *lptr;
    voxrect_t *qptr;

    x = labs(x2-x0); y = labs(y2-y0); z = labs(z2-z0);
    if (!x) { x = y; y = z; i = 0; }
    else if (!y) { y = z; i = 1; }
    else i = 2;
    if (x < y) { z = x; x = y; y = z; i += 3; }
    z = shcnt[y*shcntp+x]++;
    lptr = &gvox->mytex[(shp[z].y+VOXBORDWIDTH)*gvox->mytexx+(shp[z].x+VOXBORDWIDTH)];
    switch (face)
    {
    case 0:
        ny = y1; x2 = x0; x0 = x1; x1 = x2; break;
    case 1:
        ny = y0; y0++; y1++; y2++; break;
    case 2:
        nz = z1; y0 = y2; y2 = y1; y1 = y0; z0++; z1++; z2++; break;
    case 3:
        nz = z0; break;
    case 4:
        nx = x1; y2 = y0; y0 = y1; y1 = y2; x0++; x1++; x2++; break;
    case 5:
        nx = x0; break;
    }
    for (yy=0; yy<y; yy++, lptr+=gvox->mytexx)
        for (xx=0; xx<x; xx++)
        {
        switch (face)
        {
        case 0:
            if (i < 3) { nx = x1+x-1-xx; nz = z1+yy; } //back
            else { nx = x1+y-1-yy; nz = z1+xx; }
            break;
        case 1:
            if (i < 3) { nx = x0+xx;     nz = z0+yy; } //front
            else { nx = x0+yy;     nz = z0+xx; }
            break;
        case 2:
            if (i < 3) { nx = x1-x+xx;   ny = y1-1-yy; } //bot
            else { nx = x1-1-yy;   ny = y1-1-xx; }
            break;
        case 3:
            if (i < 3) { nx = x0+xx;     ny = y0+yy; } //top
            else { nx = x0+yy;     ny = y0+xx; }
            break;
        case 4:
            if (i < 3) { ny = y1+x-1-xx; nz = z1+yy; } //right
            else { ny = y1+y-1-yy; nz = z1+xx; }
            break;
        case 5:
            if (i < 3) { ny = y0+xx;     nz = z0+yy; } //left
            else { ny = y0+yy;     nz = z0+xx; }
            break;
        }
        lptr[xx] = getvox(nx, ny, nz);
        }

    //Extend borders horizontally
    for (yy=VOXBORDWIDTH; yy<y+VOXBORDWIDTH; yy++)
        for (xx=0; xx<VOXBORDWIDTH; xx++)
        {
        lptr = &gvox->mytex[(shp[z].y+yy)*gvox->mytexx+shp[z].x];
        lptr[xx] = lptr[VOXBORDWIDTH]; lptr[xx+x+VOXBORDWIDTH] = lptr[x-1+VOXBORDWIDTH];
        }
    //Extend borders vertically
    for (yy=0; yy<VOXBORDWIDTH; yy++)
    {
        Bmemcpy(&gvox->mytex[(shp[z].y+yy)*gvox->mytexx+shp[z].x],
            &gvox->mytex[(shp[z].y+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
            (x+(VOXBORDWIDTH<<1))<<2);
        Bmemcpy(&gvox->mytex[(shp[z].y+y+yy+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
            &gvox->mytex[(shp[z].y+y-1+VOXBORDWIDTH)*gvox->mytexx+shp[z].x],
            (x+(VOXBORDWIDTH<<1))<<2);
    }

    qptr = &gvox->quad[gvox->qcnt];
    qptr->v[0].x = x0; qptr->v[0].y = y0; qptr->v[0].z = z0;
    qptr->v[1].x = x1; qptr->v[1].y = y1; qptr->v[1].z = z1;
    qptr->v[2].x = x2; qptr->v[2].y = y2; qptr->v[2].z = z2;
    for (j=0; j<3; j++) { qptr->v[j].u = shp[z].x+VOXBORDWIDTH; qptr->v[j].v = shp[z].y+VOXBORDWIDTH; }
    if (i < 3) qptr->v[1].u += x; else qptr->v[1].v += y;
    qptr->v[2].u += x; qptr->v[2].v += y;

    qptr->v[3].u = qptr->v[0].u - qptr->v[1].u + qptr->v[2].u;
    qptr->v[3].v = qptr->v[0].v - qptr->v[1].v + qptr->v[2].v;
    qptr->v[3].x = qptr->v[0].x - qptr->v[1].x + qptr->v[2].x;
    qptr->v[3].y = qptr->v[0].y - qptr->v[1].y + qptr->v[2].y;
    qptr->v[3].z = qptr->v[0].z - qptr->v[1].z + qptr->v[2].z;
    if (gvox->qfacind[face] < 0) gvox->qfacind[face] = gvox->qcnt;
    gvox->qcnt++;

}

static inline int32_t isolid(int32_t x, int32_t y, int32_t z)
{
    if ((uint32_t) x >= (uint32_t) voxsiz.x) return(0);
    if ((uint32_t) y >= (uint32_t) voxsiz.y) return(0);
    if ((uint32_t) z >= (uint32_t) voxsiz.z) return(0);
    z += x*yzsiz + y*voxsiz.z; return(vbit[z>>5]&(1<<SHIFTMOD32(z)));
}

static voxmodel_t *vox2poly()
{
    int32_t i, j, x, y, z, v, ov, oz = 0, cnt, sc, x0, y0, dx, dy, *bx0, *by0;
    void(*daquad)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);

    gvox = (voxmodel_t *) Xmalloc(sizeof(voxmodel_t));
    memset(gvox, 0, sizeof(voxmodel_t));

    //x is largest dimension, y is 2nd largest dimension
    x = voxsiz.x; y = voxsiz.y; z = voxsiz.z;
    if ((x < y) && (x < z)) x = z; else if (y < z) y = z;
    if (x < y) { z = x; x = y; y = z; }
    shcntp = x; i = x*y*sizeof(int32_t);
    shcntmal = (int32_t *) Xmalloc(i);
    memset(shcntmal, 0, i); shcnt = &shcntmal[-shcntp-1];
    gmaxx = gmaxy = garea = 0;

    if (pow2m1[32] != -1) { for (i=0; i<32; i++) pow2m1[i] = (1u<<i)-1; pow2m1[32] = -1; }
    for (i=0; i<7; i++) gvox->qfacind[i] = -1;

    i = ((max(voxsiz.y, voxsiz.z)+1)<<2);
    bx0 = (int32_t *) Xmalloc(i<<1);
    by0 = (int32_t *) (((intptr_t) bx0)+i);

    for (cnt=0; cnt<2; cnt++)
    {
        if (!cnt) daquad = cntquad;
        else daquad = addquad;
        gvox->qcnt = 0;

        memset(by0, -1, (max(voxsiz.y, voxsiz.z)+1)<<2); v = 0;

        for (i=-1; i<=1; i+=2)
            for (y=0; y<voxsiz.y; y++)
                for (x=0; x<=voxsiz.x; x++)
                    for (z=0; z<=voxsiz.z; z++)
                    {
            ov = v; v = (isolid(x, y, z) && (!isolid(x, y+i, z)));
            if ((by0[z] >= 0) && ((by0[z] != oz) || (v >= ov)))
            {
                daquad(bx0[z], y, by0[z], x, y, by0[z], x, y, z, i>=0); by0[z] = -1;
            }
            if (v > ov) oz = z; else if ((v < ov) && (by0[z] != oz)) { bx0[z] = x; by0[z] = oz; }
                    }

        for (i=-1; i<=1; i+=2)
            for (z=0; z<voxsiz.z; z++)
                for (x=0; x<=voxsiz.x; x++)
                    for (y=0; y<=voxsiz.y; y++)
                    {
            ov = v; v = (isolid(x, y, z) && (!isolid(x, y, z-i)));
            if ((by0[y] >= 0) && ((by0[y] != oz) || (v >= ov)))
            {
                daquad(bx0[y], by0[y], z, x, by0[y], z, x, y, z, (i>=0)+2); by0[y] = -1;
            }
            if (v > ov) oz = y; else if ((v < ov) && (by0[y] != oz)) { bx0[y] = x; by0[y] = oz; }
                    }

        for (i=-1; i<=1; i+=2)
            for (x=0; x<voxsiz.x; x++)
                for (y=0; y<=voxsiz.y; y++)
                    for (z=0; z<=voxsiz.z; z++)
                    {
            ov = v; v = (isolid(x, y, z) && (!isolid(x-i, y, z)));
            if ((by0[z] >= 0) && ((by0[z] != oz) || (v >= ov)))
            {
                daquad(x, bx0[z], by0[z], x, y, by0[z], x, y, z, (i>=0)+4); by0[z] = -1;
            }
            if (v > ov) oz = z; else if ((v < ov) && (by0[z] != oz)) { bx0[z] = y; by0[z] = oz; }
                    }

        if (!cnt)
        {
            shp = (spoint2d *) Xmalloc(gvox->qcnt*sizeof(spoint2d));

            sc = 0;
            for (y=gmaxy; y; y--)
                for (x=gmaxx; x>=y; x--)
                {
                i = shcnt[y*shcntp+x]; shcnt[y*shcntp+x] = sc; //shcnt changes from counter to head index
                for (; i>0; i--) { shp[sc].x = x; shp[sc].y = y; sc++; }
                }

            for (gvox->mytexx=32; gvox->mytexx<(gmaxx+(VOXBORDWIDTH<<1)); gvox->mytexx<<=1);
            for (gvox->mytexy=32; gvox->mytexy<(gmaxy+(VOXBORDWIDTH<<1)); gvox->mytexy<<=1);
            while (gvox->mytexx*gvox->mytexy*8 < garea*9) //This should be sufficient to fit most skins...
            {
            skindidntfit:
                ;
                if (gvox->mytexx <= gvox->mytexy) gvox->mytexx <<= 1; else gvox->mytexy <<= 1;
            }
            mytexo5 = (gvox->mytexx>>5);

            i = (((gvox->mytexx*gvox->mytexy+31)>>5)<<2);
            zbit = (int32_t *) Xmalloc(i);
            memset(zbit, 0, i);

            v = gvox->mytexx*gvox->mytexy;
            for (z=0; z<sc; z++)
            {
                dx = shp[z].x+(VOXBORDWIDTH<<1); dy = shp[z].y+(VOXBORDWIDTH<<1); i = v;
                do
                {
#if (VOXUSECHAR != 0)
                    x0 = (((rand()&32767)*(min(gvox->mytexx, 255)-dx))>>15);
                    y0 = (((rand()&32767)*(min(gvox->mytexy, 255)-dy))>>15);
#else
                    x0 = (((rand()&32767)*(gvox->mytexx+1-dx))>>15);
                    y0 = (((rand()&32767)*(gvox->mytexy+1-dy))>>15);
#endif
                    i--;
                    if (i < 0) //Time-out! Very slow if this happens... but at least it still works :P
                    {
                        Bfree(zbit);

                        //Re-generate shp[].x/y (box sizes) from shcnt (now head indices) for next pass :/
                        j = 0;
                        for (y=gmaxy; y; y--)
                            for (x=gmaxx; x>=y; x--)
                            {
                            i = shcnt[y*shcntp+x];
                            for (; j<i; j++) { shp[j].x = x0; shp[j].y = y0; }
                            x0 = x; y0 = y;
                            }
                        for (; j<sc; j++) { shp[j].x = x0; shp[j].y = y0; }

                        goto skindidntfit;
                    }
                } while (!isrectfree(x0, y0, dx, dy));
                while ((y0) && (isrectfree(x0, y0-1, dx, 1))) y0--;
                while ((x0) && (isrectfree(x0-1, y0, 1, dy))) x0--;
                setrect(x0, y0, dx, dy);
                shp[z].x = x0; shp[z].y = y0; //Overwrite size with top-left location
            }

            gvox->quad = (voxrect_t *) Xmalloc(gvox->qcnt*sizeof(voxrect_t));
            gvox->mytex = (int32_t *) Xmalloc(gvox->mytexx*gvox->mytexy*sizeof(int32_t));
        }
    }
    Bfree(shp); Bfree(zbit); Bfree(bx0);
    return(gvox);
}

static int32_t loadvox(const char *filnam)
{
    int32_t i, j, k, x, y, z, pal[256], fil;
    char c[3], *tbuf;

    fil = kopen4load(filnam, 0); if (fil < 0) return(-1);
    kread(fil, &voxsiz, sizeof(vec3_t));
#if B_BIG_ENDIAN != 0
    voxsiz.x = B_LITTLE32(voxsiz.x);
    voxsiz.y = B_LITTLE32(voxsiz.y);
    voxsiz.z = B_LITTLE32(voxsiz.z);
#endif
    voxpiv.x = (float) voxsiz.x * .5f;
    voxpiv.y = (float) voxsiz.y * .5f;
    voxpiv.z = (float) voxsiz.z * .5f;

    klseek(fil, -768, SEEK_END);
    for (i=0; i<256; i++)
    {
        kread(fil, c, 3); pal[i] = (((int32_t) c[0])<<18)+(((int32_t) c[1])<<10)+(((int32_t) c[2])<<2)+(i<<24);
    }
    pal[255] = -1;

    vcolhashsizm1 = 8192-1;
    vcolhashead = (int32_t *) Xmalloc((vcolhashsizm1+1)*sizeof(int32_t));
    memset(vcolhashead, -1, (vcolhashsizm1+1)*sizeof(int32_t));

    yzsiz = voxsiz.y*voxsiz.z; i = ((voxsiz.x*yzsiz+31)>>3)+1;
    vbit = (int32_t *) Xmalloc(i);
    memset(vbit, 0, i);

    tbuf = (char *) Xmalloc(voxsiz.z*sizeof(uint8_t));

    klseek(fil, 12, SEEK_SET);
    for (x=0; x<voxsiz.x; x++)
        for (y=0, j=x*yzsiz; y<voxsiz.y; y++, j+=voxsiz.z)
        {
        kread(fil, tbuf, voxsiz.z);
        for (z=voxsiz.z-1; z>=0; z--)
        {
            if (tbuf[z] != 255) { i = j+z; vbit[i>>5] |= (1<<SHIFTMOD32(i)); }
        }
        }

    klseek(fil, 12, SEEK_SET);
    for (x=0; x<voxsiz.x; x++)
        for (y=0, j=x*yzsiz; y<voxsiz.y; y++, j+=voxsiz.z)
        {
        kread(fil, tbuf, voxsiz.z);
        for (z=0; z<voxsiz.z; z++)
        {
            if (tbuf[z] == 255) continue;
            if ((!x) || (!y) || (!z) || (x == voxsiz.x-1) || (y == voxsiz.y-1) || (z == voxsiz.z-1))
            {
                putvox(x, y, z, pal[tbuf[z]]); continue;
            }
            k = j+z;
            if ((!(vbit[(k-yzsiz)>>5]&(1<<SHIFTMOD32(k-yzsiz)))) ||
                (!(vbit[(k+yzsiz)>>5]&(1<<SHIFTMOD32(k+yzsiz)))) ||
                (!(vbit[(k- voxsiz.z)>>5]&(1<<SHIFTMOD32(k- voxsiz.z)))) ||
                (!(vbit[(k+ voxsiz.z)>>5]&(1<<SHIFTMOD32(k+ voxsiz.z)))) ||
                (!(vbit[(k-    1)>>5]&(1<<SHIFTMOD32(k-    1)))) ||
                (!(vbit[(k+    1)>>5]&(1<<SHIFTMOD32(k+    1)))))
            {
                putvox(x, y, z, pal[tbuf[z]]); continue;
            }
        }
        }

    Bfree(tbuf); kclose(fil); return(0);
}

static int32_t loadkvx(const char *filnam)
{
    int32_t i, j, k, x, y, z, pal[256], z0, z1, mip1leng, ysizp1, fil;
    uint16_t *xyoffs;
    char c[3], *tbuf, *cptr;

    fil = kopen4load(filnam, 0); if (fil < 0) return(-1);
    kread(fil, &mip1leng, 4); mip1leng = B_LITTLE32(mip1leng);
    kread(fil, &voxsiz, sizeof(vec3_t));
#if B_BIG_ENDIAN != 0
    voxsiz.x = B_LITTLE32(voxsiz.x);
    voxsiz.y = B_LITTLE32(voxsiz.y);
    voxsiz.z = B_LITTLE32(voxsiz.z);
#endif
    kread(fil, &i, 4); voxpiv.x = (float) B_LITTLE32(i)*(1.f/256.f);
    kread(fil, &i, 4); voxpiv.y = (float) B_LITTLE32(i)*(1.f/256.f);
    kread(fil, &i, 4); voxpiv.z = (float) B_LITTLE32(i)*(1.f/256.f);
    klseek(fil, (voxsiz.x+1)<<2, SEEK_CUR);

    ysizp1 = voxsiz.y+1;
    i = voxsiz.x*ysizp1*sizeof(int16_t);
    xyoffs = (uint16_t *) Xmalloc(i);
    kread(fil, xyoffs, i); for (i=i/sizeof(int16_t)-1; i>=0; i--) xyoffs[i] = B_LITTLE16(xyoffs[i]);

    klseek(fil, -768, SEEK_END);
    for (i=0; i<256; i++)
    {
        kread(fil, c, 3);
//#if B_BIG_ENDIAN != 0
        pal[i] = B_LITTLE32((((int32_t) c[0])<<18)+(((int32_t) c[1])<<10)+(((int32_t) c[2])<<2)+(i<<24));
//#endif
    }

    yzsiz = voxsiz.y*voxsiz.z; i = ((voxsiz.x*yzsiz+31)>>3)+1;
    vbit = (int32_t *) Xmalloc(i);
    memset(vbit, 0, i);

    for (vcolhashsizm1=4096; vcolhashsizm1<(mip1leng>>1); vcolhashsizm1<<=1)
    {
        /* do nothing */
    }
    vcolhashsizm1--; //approx to numvoxs!
    vcolhashead = (int32_t *) Xmalloc((vcolhashsizm1+1)*sizeof(int32_t));
    memset(vcolhashead, -1, (vcolhashsizm1+1)*sizeof(int32_t));

    klseek(fil, 28+((voxsiz.x+1)<<2)+((ysizp1*voxsiz.x)<<1), SEEK_SET);

    i = kfilelength(fil)-ktell(fil);
    tbuf = (char *) Xmalloc(i);
    kread(fil, tbuf, i); kclose(fil);

    cptr = tbuf;
    for (x=0; x<voxsiz.x; x++) //Set surface voxels to 1 else 0
        for (y=0, j=x*yzsiz; y<voxsiz.y; y++, j+=voxsiz.z)
        {
        i = xyoffs[x*ysizp1+y+1] - xyoffs[x*ysizp1+y]; if (!i) continue;
        z1 = 0;
        while (i)
        {
            z0 = (int32_t) cptr[0]; k = (int32_t) cptr[1]; cptr += 3;
            if (!(cptr[-1]&16)) setzrange1(vbit, j+z1, j+z0);
            i -= k+3; z1 = z0+k;
            setzrange1(vbit, j+z0, j+z1);  // PK: oob in AMC TC dev if vbit alloc'd w/o +1
            for (z=z0; z<z1; z++) putvox(x, y, z, pal[*cptr++]);
        }
        }

    Bfree(tbuf); Bfree(xyoffs); return(0);
}

static int32_t loadkv6(const char *filnam)
{
    int32_t i, j, x, y, numvoxs, z0, z1, fil;
    uint16_t *ylen;
    char c[8];

    fil = kopen4load((char *) filnam, 0); if (fil < 0) return(-1);
    kread(fil, &i, 4); if (B_LITTLE32(i) != 0x6c78764b) { kclose(fil); return(-1); } //Kvxl
    kread(fil, &voxsiz, sizeof(vec3_t));
#if B_BIG_ENDIAN != 0
    voxsiz.x = B_LITTLE32(voxsiz.x);
    voxsiz.y = B_LITTLE32(voxsiz.y);
    voxsiz.z = B_LITTLE32(voxsiz.z);
#endif
    kread(fil, &i, 4);       voxpiv.x = (float) (B_LITTLE32(i));
    kread(fil, &i, 4);       voxpiv.y = (float) (B_LITTLE32(i));
    kread(fil, &i, 4);       voxpiv.z = (float) (B_LITTLE32(i));
    kread(fil, &numvoxs, 4); numvoxs = B_LITTLE32(numvoxs);

    ylen = (uint16_t *) Xmalloc(voxsiz.x*voxsiz.y*sizeof(int16_t));

    klseek(fil, 32+(numvoxs<<3)+(voxsiz.x<<2), SEEK_SET);
    kread(fil, ylen, voxsiz.x*voxsiz.y*sizeof(int16_t)); for (i=voxsiz.x*voxsiz.y-1; i>=0; i--) ylen[i] = B_LITTLE16(ylen[i]);
    klseek(fil, 32, SEEK_SET);

    yzsiz = voxsiz.y*voxsiz.z; i = ((voxsiz.x*yzsiz+31)>>3)+1;
    vbit = (int32_t *) Xmalloc(i);
    memset(vbit, 0, i);

    for (vcolhashsizm1=4096; vcolhashsizm1<numvoxs; vcolhashsizm1<<=1)
    {
        /* do nothing */
    }
    vcolhashsizm1--;
    vcolhashead = (int32_t *) Xmalloc((vcolhashsizm1+1)*sizeof(int32_t));
    memset(vcolhashead, -1, (vcolhashsizm1+1)*sizeof(int32_t));

    for (x=0; x<voxsiz.x; x++)
        for (y=0, j=x*yzsiz; y<voxsiz.y; y++, j+=voxsiz.z)
        {
        z1 = voxsiz.z;
        for (i=ylen[x*voxsiz.y+y]; i>0; i--)
        {
            kread(fil, c, 8); //b,g,r,a,z_lo,z_hi,vis,dir
            z0 = B_LITTLE16(*(uint16_t *) &c[4]);
            if (!(c[6]&16)) setzrange1(vbit, j+z1, j+z0);
            vbit[(j+z0)>>5] |= (1<<SHIFTMOD32(j+z0));
            putvox(x, y, z0, B_LITTLE32(*(int32_t *) &c[0])&0xffffff);
            z1 = z0+1;
        }
        }
    Bfree(ylen); kclose(fil); return(0);
}

void voxfree(voxmodel_t *m)
{
    if (!m) return;
    DO_FREE_AND_NULL(m->mytex);
    DO_FREE_AND_NULL(m->quad);
    DO_FREE_AND_NULL(m->texid);
    Bfree(m);
}

voxmodel_t *voxload(const char *filnam)
{
    int32_t i, is8bit, ret;
    voxmodel_t *vm;

    i = strlen(filnam)-4; if (i < 0) return NULL;
    if (!Bstrcasecmp(&filnam[i], ".vox")) { ret = loadvox(filnam); is8bit = 1; }
    else if (!Bstrcasecmp(&filnam[i], ".kvx")) { ret = loadkvx(filnam); is8bit = 1; }
    else if (!Bstrcasecmp(&filnam[i], ".kv6")) { ret = loadkv6(filnam); is8bit = 0; }
    //else if (!Bstrcasecmp(&filnam[i],".vxl")) { ret = loadvxl(filnam); is8bit = 0; }
    else return NULL;
    if (ret >= 0) vm = vox2poly(); else vm = 0;
    if (vm)
    {
        vm->mdnum = 1; //VOXel model id
        vm->scale = vm->bscale = 1.f;
        vm->siz.x = voxsiz.x; vm->siz.y = voxsiz.y; vm->siz.z = voxsiz.z;
        vm->piv.x = voxpiv.x; vm->piv.y = voxpiv.y; vm->piv.z = voxpiv.z;
        vm->is8bit = is8bit;

        vm->texid = (uint32_t *) Xcalloc(MAXPALOOKUPS, sizeof(uint32_t));
    }
    DO_FREE_AND_NULL(shcntmal);
    DO_FREE_AND_NULL(vbit);
    DO_FREE_AND_NULL(vcol);
    vnum = vmax = 0;
    DO_FREE_AND_NULL(vcolhashead);

    return vm;
}

//Draw voxel model as perfect cubes
int32_t polymost_voxdraw(voxmodel_t *m, const spritetype *tspr)
{
    vec3f_t m0, a0;
    int32_t i, j, fi, xx, yy, zz;
    float ru, rv, phack[2]; //, clut[6] = {1.02,1.02,0.94,1.06,0.98,0.98};
    float f, g, k0, mat[16], omat[16], pc[4];
    vert_t *vptr;

    if ((intptr_t) m == (intptr_t) (-1)) // hackhackhack
        return 0;
    if ((tspr->cstat&48)==32) return 0;

    //updateanimation((md2model *)m,tspr);

    m0.x = m->scale;
    m0.y = m->scale;
    m0.z = m->scale;
    a0.x = a0.y = 0; a0.z = ((globalorientation&8) ? -m->zadd : m->zadd)*m->scale;

    //if (globalorientation&8) //y-flipping
    //{
    //   m0.z = -m0.z; a0.z = -a0.z;
    //      //Add height of 1st frame (use same frame to prevent animation bounce)
    //   a0.z += m->zsiz*m->scale;
    //}
    //if (globalorientation&4) { m0.y = -m0.y; a0.y = -a0.y; } //x-flipping

    k0 = m->bscale / 64.f;
    f = (float) tspr->xrepeat * (256.f/320.f) * k0;
    if ((sprite[tspr->owner].cstat&48)==16)
        f *= 1.25f;

    m0.x *= f; a0.x *= f; f = -f;
    m0.y *= f; a0.y *= f;
    f = (float) tspr->yrepeat * k0;
    m0.z *= f; a0.z *= f;

    k0 = (float) tspr->z;
    if (globalorientation&128) k0 += (float) ((tilesiz[tspr->picnum].y*tspr->yrepeat)<<1);

    f = (65536.f*512.f)/((float) xdimen*viewingrange);
    g = 32.f/((float) xdimen*gxyaspect);
    m0.y *= f; a0.y = (((float) (tspr->x-globalposx))* (1.f/1024.f) + a0.y)*f;
    m0.x *=-f; a0.x = (((float) (tspr->y-globalposy))* -(1.f/1024.f) + a0.x)*-f;
    m0.z *= g; a0.z = (((float) (k0     -globalposz))* -(1.f/16384.f) + a0.z)*g;

    md3_vox_calcmat_common(tspr, &a0, f, mat);

    //Mirrors
    if (grhalfxdown10x < 0) { mat[0] = -mat[0]; mat[4] = -mat[4]; mat[8] = -mat[8]; mat[12] = -mat[12]; }

    if (tspr->cstat&CSTAT_SPRITE_MDHACK)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0, 0.9999);
    }
    bglPushAttrib(GL_POLYGON_BIT);
    if ((grhalfxdown10x >= 0) /*^ ((globalorientation&8) != 0) ^ ((globalorientation&4) != 0)*/) bglFrontFace(GL_CW); else bglFrontFace(GL_CCW);
    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);

    bglEnable(GL_TEXTURE_2D);

    pc[0] = pc[1] = pc[2] = ((float) (numshades-min(max((globalshade * shadescale)+m->shadeoff, 0), numshades)))/((float) numshades);
    hictinting_apply(pc, globalpal);

    if (tspr->cstat&2) { if (!(tspr->cstat&512)) pc[3] = 0.66f; else pc[3] = 0.33f; }
    else pc[3] = 1.0f;
    pc[3] *= 1.0f - spriteext[tspr->owner].alpha;
    if ((tspr->cstat&2) || spriteext[tspr->owner].alpha > 0.f || pc[3] < 1.0f) bglEnable(GL_BLEND); //else bglDisable(GL_BLEND);
    //------------

    //transform to Build coords
    Bmemcpy(omat, mat, sizeof(omat));
    f = 1.f/64.f;
    g = m0.x*f; mat[0] *= g; mat[1] *= g; mat[2] *= g;
    g = m0.y*f; mat[4] = omat[8]*g; mat[5] = omat[9]*g; mat[6] = omat[10]*g;
    g =-m0.z*f; mat[8] = omat[4]*g; mat[9] = omat[5]*g; mat[10] = omat[6]*g;
    mat[12] -= (m->piv.x*mat[0] + m->piv.y*mat[4] + (m->piv.z+m->siz.z*.5f)*mat[8]);
    mat[13] -= (m->piv.x*mat[1] + m->piv.y*mat[5] + (m->piv.z+m->siz.z*.5f)*mat[9]);
    mat[14] -= (m->piv.x*mat[2] + m->piv.y*mat[6] + (m->piv.z+m->siz.z*.5f)*mat[10]);
    bglMatrixMode(GL_MODELVIEW); //Let OpenGL (and perhaps hardware :) handle the matrix rotation
    mat[3] = mat[7] = mat[11] = 0.f; mat[15] = 1.f;

    bglLoadMatrixf(mat);

    ru = 1.f/((float) m->mytexx);
    rv = 1.f/((float) m->mytexy);
#if (VOXBORDWIDTH == 0)
    uhack[0] = ru*.125; uhack[1] = -uhack[0];
    vhack[0] = rv*.125; vhack[1] = -vhack[0];
#endif
    phack[0] = 0; phack[1] = 1.f/256.f;

    if (!m->texid[globalpal]) m->texid[globalpal] = gloadtex(m->mytex, m->mytexx, m->mytexy, m->is8bit, globalpal);
    else bglBindTexture(GL_TEXTURE_2D, m->texid[globalpal]);
    bglBegin(GL_QUADS);
    for (i=0, fi=0; i<m->qcnt; i++)
    {
        if (i == m->qfacind[fi]) { f = 1 /*clut[fi++]*/; bglColor4f(pc[0]*f, pc[1]*f, pc[2]*f, pc[3]*f); }
        vptr = &m->quad[i].v[0];

        xx = vptr[0].x+vptr[2].x;
        yy = vptr[0].y+vptr[2].y;
        zz = vptr[0].z+vptr[2].z;

        for (j=0; j<4; j++)
        {
            vec3f_t fp;
#if (VOXBORDWIDTH == 0)
            bglTexCoord2f(((float) vptr[j].u)*ru+uhack[vptr[j].u!=vptr[0].u],
                ((float) vptr[j].v)*rv+vhack[vptr[j].v!=vptr[0].v]);
#else
            bglTexCoord2f(((float) vptr[j].u)*ru, ((float) vptr[j].v)*rv);
#endif
            fp.x = ((float) vptr[j].x) - phack[xx>vptr[j].x*2] + phack[xx<vptr[j].x*2];
            fp.y = ((float) vptr[j].y) - phack[yy>vptr[j].y*2] + phack[yy<vptr[j].y*2];
            fp.z = ((float) vptr[j].z) - phack[zz>vptr[j].z*2] + phack[zz<vptr[j].z*2];
            bglVertex3fv((float *) &fp);
        }
    }
    bglEnd();

    //------------
    bglDisable(GL_CULL_FACE);
    bglPopAttrib();
    if (tspr->cstat&CSTAT_SPRITE_MDHACK)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
        bglDepthRange(0.0, 0.99999);
    }
    bglLoadIdentity();
    return 1;
}
#endif

//---------------------------------------- VOX LIBRARY ENDS ----------------------------------------
