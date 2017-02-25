#include "compat.h"

#define PI 3.141592653589793

typedef struct { float x, y; } point2d;
typedef struct { float x, y, z; } point3d;
typedef struct { float x, y, z; int32_t n; } kgln_t;
typedef struct { int32_t w, s; } vertlist_t;
typedef struct { float x, y; int32_t n, ns, nw; } wall_t;
typedef struct { float z[2]; point2d grad[2]; wall_t *wall; int32_t n; } sect_t;
static int32_t numsects;
static sect_t *sec;

    //Build1 format variables:
typedef struct { int16_t picnum, heinum; int8_t shade; uint8_t pal, xpanning, ypanning; } build7surf_t;
typedef struct
{
    int16_t wallptr, wallnum;
    int32_t z[2]; int16_t stat[2]; build7surf_t surf[2];
    uint8_t visibility, filler;
    int16_t lotag, hitag, extra;
} build7sect_t;
typedef struct
{
    int32_t x, y;
    int16_t point2, nextwall, nextsect, cstat, picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} build7wall_t;
static build7sect_t b7sec;
static build7wall_t b7wal;

static void checknextwalls (void)
{
    float x0, y0, x1, y1;
    int32_t s0, w0, w0n, s1, w1, w1n;

        //Clear all nextsect/nextwalls
    for (s0=0;s0<numsects;s0++)
        for (w0=0;w0<sec[s0].n;w0++) sec[s0].wall[w0].ns = sec[s0].wall[w0].nw = -1;

    for (s1=1;s1<numsects;s1++)
        for (w1=0;w1<sec[s1].n;w1++)
        {
            x0 = sec[s1].wall[w1].x;  y0 = sec[s1].wall[w1].y; w1n = sec[s1].wall[w1].n+w1;
            x1 = sec[s1].wall[w1n].x; y1 = sec[s1].wall[w1n].y;
            for (s0=0;s0<s1;s0++)
                for (w0=0;w0<sec[s0].n;w0++)
                    if ((sec[s0].wall[w0].x == x1) && (sec[s0].wall[w0].y == y1))
                    {
                        w0n = sec[s0].wall[w0].n+w0;
                        if ((sec[s0].wall[w0n].x == x0) && (sec[s0].wall[w0n].y == y0))
                        {
                            sec[s1].wall[w1].ns = s0;
                            sec[s1].wall[w1].nw = w0;
                            sec[s0].wall[w0].ns = s1;
                            sec[s0].wall[w0].nw = w1;
                            goto cnw_break2;
                        }
                    }
cnw_break2:;
        }
}

static void shellsrt (float *a, int32_t n)
{
    float t;
    int32_t i, j, g;

    for (g=(n>>1);g;g>>=1)
        for (i=0;i<n-g;i++)
            for (j=i;(j>=0)&&(a[j]>a[j+g]);j-=g)
                { t = a[j]; a[j] = a[j+g]; a[j+g] = t; }
}

static float getslopez (sect_t *s, int32_t i, float x, float y)
{
    wall_t *wal = s->wall;
    return (wal[0].x-x)*s->grad[i].x + (wal[0].y-y)*s->grad[i].y + s->z[i];
}

typedef struct { float x[4], y[2]; wall_t * pwal[2]; } zoid_t;
static int32_t sect2trap (wall_t *wal, int32_t n, zoid_t **retzoids, int32_t *retnzoids)
{
    zoid_t *zoids = 0;
    float f, x0, y0, x1, y1, sy0, sy1, cury, *secy = NULL, *trapx0 = NULL, *trapx1 = NULL;
    int32_t i, j, g, s, secn, ntrap, tot, zoidalloc;
    wall_t * k;
    wall_t ** pwal = NULL;

    (*retzoids) = 0; (*retnzoids) = 0; if (n < 3) return 0;

    secy = (float *)malloc(n*sizeof(secy[0])); if (!secy) goto badret;
    trapx0 = (float *)malloc(n*sizeof(trapx0[0])); if (!trapx0) goto badret;
    trapx1 = (float *)malloc(n*sizeof(trapx1[0])); if (!trapx1) goto badret;
    pwal = (wall_t **)malloc(n*sizeof(pwal[0])); if (!pwal) goto badret;

    for (i=n-1;i>=0;i--) secy[i] = wal[i].y;
    shellsrt(secy,n);
    for (i=0,secn=0,cury=-1e32f;i<n;i++) //remove dups
        if (secy[i] > cury) { secy[secn++] = cury = secy[i]; }

    zoidalloc = secn*2; //just a guess (not guaranteed to fit)
    zoids = (zoid_t *)malloc(zoidalloc*sizeof(zoid_t)); if (!zoids) goto badret;

    tot = 0;
    for (s=0;s<secn-1;s++)
    {
        sy0 = secy[s]; sy1 = secy[s+1]; ntrap = 0;
        for (i=0;i<n;i++) //FIX:optimize
        {
            x0 = wal[i].x; y0 = wal[i].y; j = wal[i].n+i;
            x1 = wal[j].x; y1 = wal[j].y;
            if (y0 > y1)
            {
                f = x0; x0 = x1; x1 = f;
                f = y0; y0 = y1; y1 = f;
            }
            if ((y0 >= sy1) || (y1 <= sy0)) continue;
            if (y0 < sy0) x0 = (sy0-wal[i].y)*(wal[j].x-wal[i].x)/(wal[j].y-wal[i].y) + wal[i].x;
            if (y1 > sy1) x1 = (sy1-wal[i].y)*(wal[j].x-wal[i].x)/(wal[j].y-wal[i].y) + wal[i].x;
            trapx0[ntrap] = x0; trapx1[ntrap] = x1; pwal[ntrap] = &wal[i]; ntrap++;
        }
        for (g=(ntrap>>1);g;g>>=1)
            for (i=0;i<ntrap-g;i++)
                for (j=i;j>=0;j-=g)
                {
                    if (trapx0[j]+trapx1[j] <= trapx0[j+g]+trapx1[j+g]) break;
                    f = trapx0[j]; trapx0[j] = trapx0[j+g]; trapx0[j+g] = f;
                    f = trapx1[j]; trapx1[j] = trapx1[j+g]; trapx1[j+g] = f;
                    k =    pwal[j];    pwal[j] =    pwal[j+g];    pwal[j+g] = k;
                }

        if (tot+ntrap > zoidalloc)
        {
            zoidalloc <<= 1; if (tot+ntrap > zoidalloc) zoidalloc = tot+ntrap;
            zoids = (zoid_t *)realloc(zoids,zoidalloc*sizeof(zoid_t)); if (!zoids) goto badret;
        }
        for (i=0;i<ntrap;i=j+1)
        {
            j = i+1; if ((trapx0[i+1] <= trapx0[i]) && (trapx1[i+1] <= trapx1[i])) continue;
            while ((j+2 < ntrap) && (trapx0[j+1] <= trapx0[j]) && (trapx1[j+1] <= trapx1[j])) j += 2;
            zoids[tot].x[0] = trapx0[i]; zoids[tot].x[1] = trapx0[j]; zoids[tot].y[0] = sy0;
            zoids[tot].x[3] = trapx1[i]; zoids[tot].x[2] = trapx1[j]; zoids[tot].y[1] = sy1;
            zoids[tot].pwal[0] = pwal[i]; zoids[tot].pwal[1] = pwal[j];
            tot++;
        }
    }
    (*retzoids) = zoids; (*retnzoids) = tot; return 1;
badret:;
    free(secy);
    free(trapx0);
    free(trapx1);
    free(pwal);
    free(zoids);
    return 0;
}

static int32_t getwalls (int32_t s, int32_t w, vertlist_t *ver, int32_t maxverts)
{
    vertlist_t tver;
    wall_t *wal, *wal2;
    float fx, fy;
    int32_t i, j, k, bs, bw, nw, vn;

    wal = sec[s].wall; bs = wal[w].ns;
    if ((unsigned)bs >= (unsigned)numsects) return 0;

    vn = 0; nw = wal[w].n+w; bw = wal[w].nw;
    do
    {
        wal2 = sec[bs].wall; i = wal2[bw].n+bw; //Make sure it's an opposite wall
        if ((wal[w].x == wal2[i].x) && (wal[nw].x == wal2[bw].x) &&
             (wal[w].y == wal2[i].y) && (wal[nw].y == wal2[bw].y))
            { if (vn < maxverts) { ver[vn].s = bs; ver[vn].w = bw; vn++; } }
        bs = wal2[bw].ns;
        bw = wal2[bw].nw;
    } while (bs != s);

        //Sort next sects by order of height in middle of wall (crap sort)
    fx = (wal[w].x+wal[nw].x)*.5f;
    fy = (wal[w].y+wal[nw].y)*.5f;
    for (k=1;k<vn;k++)
        for (j=0;j<k;j++)
            if (getslopez(&sec[ver[j].s],0,fx,fy) + getslopez(&sec[ver[j].s],1,fx,fy) >
                 getslopez(&sec[ver[k].s],0,fx,fy) + getslopez(&sec[ver[k].s],1,fx,fy))
                { tver = ver[j]; ver[j] = ver[k]; ver[k] = tver; }
    return(vn);
}

static int32_t wallclip (kgln_t *pol, kgln_t *npol)
{
    float f, dz0, dz1;

    dz0 = pol[3].z-pol[0].z; dz1 = pol[2].z-pol[1].z;
    if (dz0 > 0.0) //do not include null case for rendering
    {
        npol[0] = pol[0];
        if (dz1 > 0.0) //do not include null case for rendering
        {
            npol[1] = pol[1];
            npol[2] = pol[2];
            npol[3] = pol[3];
            npol[0].n = npol[1].n = npol[2].n = 1; npol[3].n = -3;
            return 4;
        }
        else
        {
            f = dz0/(dz0-dz1);
            npol[1].x = (pol[1].x-pol[0].x)*f + pol[0].x;
            npol[1].y = (pol[1].y-pol[0].y)*f + pol[0].y;
            npol[1].z = (pol[1].z-pol[0].z)*f + pol[0].z;
            npol[2] = pol[3];
            npol[0].n = npol[1].n = 1; npol[2].n = -2;
            return 3;
        }
    }
    if (dz1 <= 0.0) return 0; //do not include null case for rendering
    f = dz0/(dz0-dz1);
    npol[0].x = (pol[1].x-pol[0].x)*f + pol[0].x;
    npol[0].y = (pol[1].y-pol[0].y)*f + pol[0].y;
    npol[0].z = (pol[1].z-pol[0].z)*f + pol[0].z;
    npol[1] = pol[1];
    npol[2] = pol[2];
    npol[0].n = npol[1].n = 1; npol[2].n = -2;
    return 3;
}

    //    //STL binary format:
    //char filler[80];
    //uint32_t numtris;
    //for (i=0;i<numtris;i++)
    //{
    //    point3d norm, v[3]; //vertices are CCW and must be + coords
    //    int16_t filler;
    //}
static void saveasstl (char const * filnam)
{
    #define MAXVERTS 256 //WARNING:not dynamic
    vertlist_t verts[MAXVERTS];
    kgln_t pol[4], npol[4];
    point3d fp[3], fp2;
    point2d *grad;
    wall_t *wal;
    zoid_t *zoids;
    FILE *fil;
    float f, fz;
    int32_t i, j, k, n, w, s, nw, wn, vn, s0, cf0, s1, cf1, isflor, nzoids;
    int32_t numtris;
    char tbuf[80];

    fil = fopen(filnam,"wb"); if (!fil) return;
    memset(tbuf,0,80);
    fwrite(tbuf,80,1,fil); //header ignored in STL
    numtris = 0;
    fwrite(&numtris,4,1,fil); //dummy write

    for (s=0;s<numsects;s++)
    {
            //draw sector filled
        for (isflor=0;isflor<2;isflor++)
        {
            wal = sec[s].wall; fz = sec[s].z[isflor]; grad = &sec[s].grad[isflor]; n = sec[s].n;

            if (!sect2trap(wal,n,&zoids,&nzoids)) continue;

            for (i=0;i<nzoids;i++)
            {
                for (j=0,n=0;j<4;j++)
                {
                    pol[n].x = zoids[i].x[j];
                    pol[n].y = zoids[i].y[j>>1];
                    if ((!n) || (pol[n].x != pol[n-1].x) || (pol[n].y != pol[n-1].y))
                    {
                        pol[n].z = (wal[0].x-pol[n].x)*grad->x + (wal[0].y-pol[n].y)*grad->y + fz;
                        pol[n].n = 1; n++;
                    }
                }
                if (n < 3) continue;
                pol[n-1].n = 1-n;

                fp[0].x = pol[0].x; fp[0].y = pol[0].y; fp[0].z = pol[0].z;
                for (j=2;j<n;j++)
                {
                    k = j-isflor;    fp[1].x = pol[k].x; fp[1].y = pol[k].y; fp[1].z = pol[k].z;
                    k = j-1+isflor; fp[2].x = pol[k].x; fp[2].y = pol[k].y; fp[2].z = pol[k].z;
                        //fp2 = unit norm
                    fp2.x = (fp[1].y-fp[0].y)*(fp[2].z-fp[0].z) - (fp[1].z-fp[0].z)*(fp[2].y-fp[0].y);
                    fp2.y = (fp[1].z-fp[0].z)*(fp[2].x-fp[0].x) - (fp[1].x-fp[0].x)*(fp[2].z-fp[0].z);
                    fp2.z = (fp[1].x-fp[0].x)*(fp[2].y-fp[0].y) - (fp[1].y-fp[0].y)*(fp[2].x-fp[0].x);
                    f = fp2.x*fp2.x + fp2.y*fp2.y + fp2.z*fp2.z; if (f > 0.f) f = -1.f/sqrtf(f);
                    fp2.x *= f; fp2.y *= f; fp2.z *= f; fwrite(&fp2,4*3,1,fil);
                    fwrite(fp,4*3*3,1,fil); fwrite(tbuf,2,1,fil); //2 bytes of filler
                    numtris++;
                }
            }
            free(zoids);
        }

        wal = sec[s].wall; wn = sec[s].n;
        for (w=0;w<wn;w++)
        {
            nw = wal[w].n+w; vn = getwalls(s,w,verts,MAXVERTS);
            pol[0].x = wal[ w].x; pol[0].y = wal[ w].y; pol[0].n = 1;
            pol[1].x = wal[nw].x; pol[1].y = wal[nw].y; pol[1].n = 1;
            pol[2].x = wal[nw].x; pol[2].y = wal[nw].y; pol[2].n = 1;
            pol[3].x = wal[ w].x; pol[3].y = wal[ w].y; pol[3].n =-3;
            for (k=0;k<=vn;k++) //Warning: do not reverse for loop!
            {
                if (k >  0) { s0 = verts[k-1].s; cf0 = 1; } else { s0 = s; cf0 = 0; }
                if (k < vn) { s1 = verts[k  ].s; cf1 = 0; } else { s1 = s; cf1 = 1; }

                pol[0].z = getslopez(&sec[s0],cf0,pol[0].x,pol[0].y);
                pol[1].z = getslopez(&sec[s0],cf0,pol[1].x,pol[1].y);
                pol[2].z = getslopez(&sec[s1],cf1,pol[2].x,pol[2].y);
                pol[3].z = getslopez(&sec[s1],cf1,pol[3].x,pol[3].y);
                i = wallclip(pol,npol); if (!i) continue;

                fp[0].x = npol[0].x; fp[0].y = npol[0].y; fp[0].z = npol[0].z;
                for (j=2;j<i;j++)
                {
                    fp[1].x = npol[j-1].x; fp[1].y = npol[j-1].y; fp[1].z = npol[j-1].z;
                    fp[2].x = npol[j  ].x; fp[2].y = npol[j  ].y; fp[2].z = npol[j  ].z;
                        //fp2 = unit norm
                    fp2.x = (fp[1].y-fp[0].y)*(fp[2].z-fp[0].z) - (fp[1].z-fp[0].z)*(fp[2].y-fp[0].y);
                    fp2.y = (fp[1].z-fp[0].z)*(fp[2].x-fp[0].x) - (fp[1].x-fp[0].x)*(fp[2].z-fp[0].z);
                    fp2.z = (fp[1].x-fp[0].x)*(fp[2].y-fp[0].y) - (fp[1].y-fp[0].y)*(fp[2].x-fp[0].x);
                    f = fp2.x*fp2.x + fp2.y*fp2.y + fp2.z*fp2.z; if (f > 0.f) f = -1.f/sqrtf(f);
                    fp2.x *= f; fp2.y *= f; fp2.z *= f; fwrite(&fp2,4*3,1,fil);
                    fwrite(fp,4*3*3,1,fil); fwrite(tbuf,2,1,fil); //2 bytes of filler
                    numtris++;
                }
            }
        }
    }

    i = ftell(fil);
    fseek(fil,80,SEEK_SET); fwrite(&numtris,4,1,fil);
    fseek(fil,i,SEEK_SET);
    fclose(fil);
}

static int32_t loadmap (char *filnam)
{
    float f, fx, fy;
    int32_t i, j, k;
    int16_t s;
    FILE *fil;

    fil = fopen(filnam,"rb"); if (!fil) return 0;
    fread(&i,4,1,fil); if (i != 0x00000007) return 0; //not Build1 .MAP format 7
    fseek(fil,20,SEEK_SET);

    fread(&s,2,1,fil); numsects = (int32_t)s;
    sec = (sect_t *)malloc(numsects*sizeof(sect_t));
    memset(sec,0,numsects*sizeof(sect_t));
    for (i=0;i<numsects;i++)
    {
        fread(&b7sec,sizeof(b7sec),1,fil);
        sec[i].n = b7sec.wallnum;
        sec[i].wall = (wall_t *)realloc(sec[i].wall,sec[i].n*sizeof(wall_t));
        memset(sec[i].wall,0L,sec[i].n*sizeof(wall_t));
        for (j=0;j<2;j++)
        {
            sec[i].z[j] = ((float)b7sec.z[j])*(1.f/(512.f*16.f));
            sec[i].grad[j].x = sec[i].grad[j].y = 0;
            if (b7sec.stat[j]&2) //Enable slopes flag
                sec[i].grad[j].y = ((float)b7sec.surf[j].heinum)*(1.f/4096.f);
        }
    }

    fread(&s,2,1,fil); //numwalls
    for (i=k=0;i<numsects;i++)
    {
        for (j=0;j<sec[i].n;j++,k++)
        {
            fread(&b7wal,sizeof(b7wal),1,fil);
            sec[i].wall[j].x = ((float)b7wal.x)*(1.f/512.f);
            sec[i].wall[j].y = ((float)b7wal.y)*(1.f/512.f);
            sec[i].wall[j].n = b7wal.point2-k;
        }

        fx = sec[i].wall[1].y-sec[i].wall[0].y;
        fy = sec[i].wall[0].x-sec[i].wall[1].x;
        f = fx*fx + fy*fy; if (f > 0.f) f = 1.f/sqrtf(f); fx *= f; fy *= f;
        for (j=0;j<2;j++)
        {
            sec[i].grad[j].x = fx*sec[i].grad[j].y;
            sec[i].grad[j].y = fy*sec[i].grad[j].y;
        }
    }

    checknextwalls();
    fclose(fil);
    return 1;
}

int main (int argc, char **argv)
{
    if (argc != 3)
    {
        printf("map2stl [in:Build .MAP (v7)] [out:STL file]         by Ken Silverman (05/15/2009)");
        return 1;
    }
    if (!loadmap(argv[1]))
    {
        printf("error loading map\n");
        return 2;
    }
    saveasstl(argv[2]);
    return 0;
}
