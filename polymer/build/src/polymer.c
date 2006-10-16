// blah

#include "polymer.h"

// CVARS
int             pr_cliplanes = 1;
int             pr_fov = 426;           // appears to be the classic setting.
int             pr_frustumculling = 1;
int             pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int             pr_wireframe = 0;

// DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];

// CONTROL
float           frustum[16]; // left right top bottom

_cliplane       *cliplanes;
int             cliplanecount, maxcliplanecount;

int             updatesectors = 1;

GLUtesselator*  prtess;
int             tempverticescount;
GLdouble        tempvertice[3];

// EXTERNAL FUNCTIONS
int                 polymer_init(void)
{
    int             i;

    if (pr_verbosity >= 1) OSD_Printf("Initalizing Polymer subsystem...\n");

    i = 0;
    while (i < MAXSECTORS)
    {
        prsectors[i] = NULL;
        i++;
    }

    i = 0;
    while (i < MAXWALLS)
    {
        prwalls[i] = NULL;
        i++;
    }

    cliplanes = NULL;
    cliplanecount = maxcliplanecount = 0;

    prtess = gluNewTess();
    if (prtess == 0)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Tesselator initialization failed.\n");
        return (0);
    }

    polymer_loadboard();

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete.\n");
    return (1);
}

void                polymer_glinit(void)
{
    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClearStencil(0);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    bglViewport(0, 0, xdim, ydim);

    // texturing
    bglEnable(GL_TEXTURE_2D);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    bglDisable(GL_FOG);
    bglEnable(GL_DEPTH_TEST);
    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    gluPerspective((float)(pr_fov) / (2048.0f / 360.0f), (float)xdim / (float)ydim, 1.0f, 1000000.0f);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);
}

void                polymer_loadboard(void)
{
    int             i;

    i = 0;
    while (i < numsectors)
    {
        polymer_initsector(i);
        polymer_updatesector(i);
        i++;
    }

    i = 0;
    while (i < numwalls)
    {
        polymer_initwall(i);
        polymer_updatewall(i);
        i++;
    }

    if (pr_verbosity >= 1) OSD_Printf("PR : Board loaded.\n");
}
void                polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum, int root)
{
    int             i, j;
    float           ang, horizang, tiltang;
    double          pos[3];
    _point2d        ref;
    sectortype      *sec;
    walltype        *wal;
    short           drawnsectors, fov;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    ang = (float)(daang) / (2048.0f / 360.0f);
    horizang = (float)(100 - dahoriz) / (512.0f / 180.0f);
    tiltang = (gtang * 90.0f);
    fov = (pr_fov * (float)xdim / (float)ydim * 1) / 2;

    pos[0] = -daposy;
    pos[1] = daposz;
    pos[2] = daposx;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglRotatef(tiltang, 0.0f, 0.0f, -1.0f);
    bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

    bglScalef(1.0f, 1.0f / 16.0f, 1.0f);
    bglTranslatef(pos[0], pos[1], pos[2]);

    if (pr_frustumculling)
        polymer_extractfrustum();

    cliplanecount = 0;

    if ((updatesectors) || 1)
    {
        i = 0;
        while (i < numsectors)
        {
            polymer_updatesector(i);
            i++;
        }

        i = 0;
        while (i < numwalls)
        {
            polymer_updatewall(i);
            i++;
        }
        updatesectors = 0;
    }
        
    i = 0;
    while (i < numsectors)
    {
        prsectors[i]->drawingstate = 0;
        i++;
    }

    prsectors[dacursectnum]->drawingstate = 2; // SEED OF LIFE

    drawnsectors = 1;
    while (drawnsectors > 0)
    {
        drawnsectors = 0;

        i = 0;
        while (i < numsectors)
        {
            if (prsectors[i] && prsectors[i]->drawingstate == 2)
            {
                polymer_drawsector(i);

                sec = &sector[i];
                wal = &wall[sec->wallptr];

                j = 0;
                while (j < sec->wallnum)
                {
                    if (((pr_frustumculling == 0) || polymer_portalinfrustum(sec->wallptr + j)) &&
                        ((pr_cliplanes == 0) || polymer_wallincliplanes(sec->wallptr + j)))
                    {
                        polymer_drawwall(sec->wallptr + j);
                        if ((wal->nextsector != -1) && (prsectors[wal->nextsector]) && (prsectors[wal->nextsector]->drawingstate == 0))
                            prsectors[wal->nextsector]->drawingstate = 1;
                        if (wal->nextsector == -1 && pr_cliplanes)
                        {   // add a 2D cliplane for map limits
                            polymer_addcliplane(equation(wal->x, wal->y, wall[wal->point2].x, wall[wal->point2].y),
                                                equation(daposx, daposy, wal->x, wal->y),
                                                equation(daposx, daposy, wall[wal->point2].x, wall[wal->point2].y),
                                                (float)(daposx + wal->x + wall[wal->point2].x) / 3.0f,
                                                (float)(daposy + wal->y + wall[wal->point2].y) / 3.0f);
                        }
                    }

                    j++;
                    wal = &wall[sec->wallptr + j];
                }

                prsectors[i]->drawingstate = 3;
                drawnsectors++;
            }
            i++;
        }

        i = 0;
        while (i < numsectors)
        {
            if (prsectors[i] && prsectors[i]->drawingstate == 1)
                prsectors[i]->drawingstate = 2;
            i++;
        }
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
}

void                polymer_rotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2)
{
}

void                polymer_drawmaskwall(long damaskwallcnt)
{
    OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);
}

void                polymer_drawsprite(long snum)
{
    OSD_Printf("PR : Sprite %i...\n", snum);
}

// SECTORS
int                 polymer_initsector(short sectnum)
{
    sectortype      *sec;
    _prsector*      s;
    int             i;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = malloc(sizeof(_prsector));
    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize sector %i : malloc failed.\n", sectnum);
        return (0);
    }

    s->verts = calloc(sec->wallnum, sizeof(GLdouble) * 3);
    s->floorbuffer = calloc(sec->wallnum, sizeof(GLfloat) * 5);
    s->ceilbuffer = calloc(sec->wallnum, sizeof(GLfloat) * 5);
    if ((s->verts == NULL) || (s->floorbuffer == NULL) || (s->ceilbuffer == NULL))
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize geometry of sector %i : malloc failed.\n", sectnum);
        return (0);
    }
   
    s->floorindices = s->ceilindices = NULL;

    s->controlstate = s->drawingstate = 0;

    prsectors[sectnum] = s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized sector %i.\n", sectnum);

    return (1);
}

int                 polymer_updatesector(short sectnum)
{
    _prsector*      s;
    sectortype      *sec;
    walltype        *wal;
    int             i, j;
    long            ceilz, florz;
    long            tex, tey;
    float           secangcos, secangsin, scalecoef;
    long            ang;
    short           curstat, curpicnum;
    char            curxpanning, curypanning;
    GLfloat*        curbuffer;
    GLuint*         curglpic;
    pthtyp*         pth;

    s = prsectors[sectnum];
    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];

    secangcos = secangsin = 0;

    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Can't update uninitialized sector %i.\n", sectnum);
        return (-1);
    }

    s->controlstate = 0;

    if ((sec->floorstat & 64) || (sec->ceilingstat & 64))
    {
        ang = (getangle(wall[wal->point2].x - wal->x, wall[wal->point2].y - wal->y) + 512) & 2047;
        secangcos = (float)(sintable[(ang+512)&2047]) / 16383.0f;
        secangsin = (float)(sintable[ang&2047]) / 16383.0f;
    }

    // geometry
    i = 0;
    while (i < sec->wallnum)
    {
        if ((-wal->x != s->verts[(i*3)+2]) || 0)
        {
            s->verts[(i*3)+2] = s->floorbuffer[(i*5)+2] = s->ceilbuffer[(i*5)+2] = -wal->x;
            s->controlstate |= 2;
        }
        if ((wal->y != s->verts[i*3]) || 0)
        {
            s->verts[i*3] = s->floorbuffer[i*5] = s->ceilbuffer[i*5] = wal->y;
            s->controlstate |= 2;
        }
        getzsofslope(sectnum, wal->x, wal->y, &ceilz, &florz);
        s->verts[(i*3)+1] = 0;
        s->floorbuffer[(i*5)+1] = -florz;
        s->ceilbuffer[(i*5)+1] = -ceilz;

        j = 2;
        curstat = sec->floorstat;
        curbuffer = s->floorbuffer;
        curpicnum = sec->floorpicnum;
        curxpanning = sec->floorxpanning;
        curypanning = sec->floorypanning;

        while (j)
        {
            if (j == 1)
            {
                curstat = sec->ceilingstat;
                curbuffer = s->ceilbuffer;
                curpicnum = sec->ceilingpicnum;
                curxpanning = sec->ceilingxpanning;
                curypanning = sec->ceilingypanning;
            }

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,sectnum);

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            tex = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangsin) + ((-wal->y - -wall[sec->wallptr].y) * secangcos) : wal->x;
            tey = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangcos) - ((wall[sec->wallptr].y - wal->y) * secangsin) : -wal->y;

            if (curstat & 4)
                swaplong(&tex, &tey);

            if (curstat & 16) tex = -tex;
            if (curstat & 32) tey = -tey;

            scalecoef = (curstat & 8) ? 8.0f : 16.0f;

            curbuffer[(i*5)+3] = ((float)(tex) / (scalecoef * tilesizx[curpicnum])) + ((float)(curxpanning) / 256.0f);
            curbuffer[(i*5)+4] = ((float)(tey) / (scalecoef * tilesizy[curpicnum])) + ((float)(curypanning) / 256.0f);

            j--;
        }

        //attributes
        j = 2;
        curbuffer = s->floorcolor;
        curstat = sec->floorshade;
        curxpanning = sec->floorpal;
        curpicnum = sec->floorpicnum;
        curglpic = &s->floorglpic;

        while (j > 0)
        {
            if (j == 1)
            {
                curbuffer = s->ceilcolor;
                curstat = sec->ceilingshade;
                curxpanning = sec->ceilingpal;
                curpicnum = sec->ceilingpicnum;
                curglpic = &s->ceilglpic;
            }

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,sectnum);
        
            if (!waloff[curpicnum])
                loadtile(curpicnum);

            curbuffer[0] = curbuffer[1] = curbuffer[2] = ((float)(numpalookups-min(max(curstat,0),numpalookups)))/((float)numpalookups);
            curbuffer[3] = 1.0f;

            pth = gltexcache(curpicnum,curxpanning,0);

            if (pth && (pth->flags & 2) && (pth->palnum != curxpanning)) {
                curbuffer[0] *= (float)hictinting[curxpanning].r / 255.0;
                curbuffer[1] *= (float)hictinting[curxpanning].g / 255.0;
                curbuffer[2] *= (float)hictinting[curxpanning].b / 255.0;
            }

            *curglpic = (pth) ? pth->glpic : 0;

            j--;
        }

        i++;
        wal = &wall[sec->wallptr + i];
    }

    if (s->controlstate & 2)
    {
        polymer_buildfloor(sectnum);
        s->controlstate ^= 2;
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated sector %i.\n", sectnum);

    return (0);
}


void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out)
{   // This callback is called by the tesselator when it detects an intersection between contours (HELLO ROTATING SPOTLIGHT IN E1L1).
    GLdouble*       ptr;

    tempvertice[0] = v[0];
    tempvertice[1] = v[1];
    tempvertice[2] = v[2];

    ptr = tempvertice;
    *out = tempvertice;

    if (pr_verbosity >= 2) OSD_Printf("PR : Created additional geometry for sector tesselation.\n");
}


void PR_CALLBACK    polymer_tesserror(GLenum error)
{   // This callback is called by the tesselator whenever it raises an error.
    if (pr_verbosity >= 1) OSD_Printf("PR : Tesselation error number %i reported : %s.\n", error, gluErrorString(errno));
}


void PR_CALLBACK    polymer_tessedgeflag(GLenum error)
{   // Passing an edgeflag callback forces the tesselator to output a triangle list
    return;
}

void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector)
{
    _prsector*      s;

    s = (_prsector*)sector;

    if (s->curindice >= s->indicescount)
    {
        if (pr_verbosity >= 2) OSD_Printf("PR : Indice overflow, extending the indices list... !\n");
        s->indicescount++;
        s->floorindices = realloc(s->floorindices, s->indicescount * sizeof(GLushort));
        s->ceilindices = realloc(s->ceilindices, s->indicescount * sizeof(GLushort));
    }
    s->ceilindices[s->curindice] = (int)vertex;
    s->curindice++;
}
int                 polymer_buildfloor(short sectnum)
{   // This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
    _prsector*      s;
    sectortype      *sec;
    int             i;

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselating floor of sector %i...\n", sectnum);

    s = prsectors[sectnum];
    sec = &sector[sectnum];

    if (s == NULL)
        return (-1);

    if (s->floorindices == NULL)
    {
        s->indicescount = (sec->wallnum - 2) * 3;
        s->floorindices = calloc(s->indicescount, sizeof(GLushort));
        s->ceilindices = calloc(s->indicescount, sizeof(GLushort));
    }

    s->curindice = 0;

    gluTessCallback(prtess, GLU_TESS_VERTEX_DATA, polymer_tessvertex);
    gluTessCallback(prtess, GLU_TESS_EDGE_FLAG, polymer_tessedgeflag);
    //gluTessCallback(prtess, GLU_TESS_COMBINE, polymer_tesscombine);
    gluTessCallback(prtess, GLU_TESS_ERROR, polymer_tesserror);

    gluTessProperty(prtess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);

    gluTessBeginPolygon(prtess, s);
    gluTessBeginContour(prtess);

    i = 0;
    while (i < sec->wallnum)
    {
        gluTessVertex(prtess, s->verts + (3 * i), (void *)i);
        if ((i != (sec->wallnum - 1)) && ((sec->wallptr + i) > wall[sec->wallptr + i].point2))
        {
            gluTessEndContour(prtess);
            gluTessBeginContour(prtess);
        }
        i++;
    }
    gluTessEndContour(prtess);
    gluTessEndPolygon(prtess);

    i = 0;
    while (i < s->indicescount)
    {
        s->floorindices[s->indicescount - i - 1] = s->ceilindices[i];

        i++;
    }

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselated floor of sector %i.\n", sectnum);

    return (1);
}

void                polymer_drawsector(short sectnum)
{
    sectortype      *sec, *nextsec;
    walltype        *wal;
    _prsector*      s;
    int             i;
    long            zdiff;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];
    s = prsectors[sectnum];

    if (prsectors[sectnum] == NULL)
    {
        polymer_initsector(sectnum);
        polymer_updatesector(sectnum);
    }

    // floor
    if (!(sec->floorstat & 1))
    {
        bglBindTexture(GL_TEXTURE_2D, s->floorglpic);
        bglColor4f(s->floorcolor[0], s->floorcolor[1], s->floorcolor[2], s->floorcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), s->floorbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &s->floorbuffer[3]);
        bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->floorindices);
    }

    // ceiling
    if (!(sec->ceilingstat & 1))
    {
        bglBindTexture(GL_TEXTURE_2D, s->ceilglpic);
        bglColor4f(s->ceilcolor[0], s->ceilcolor[1], s->ceilcolor[2], s->ceilcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), s->ceilbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &s->ceilbuffer[3]);
        bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->ceilindices);
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

// WALLS
int                 polymer_initwall(short wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing wall %i...\n", wallnum);

    w = malloc(sizeof(_prwall));
    if (w == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize wall %i : malloc failed.\n", wallnum);
        return (0);
    }

    w->invalidate = w->underover = 0;
    w->wallbuffer = w->overbuffer = w->portal = NULL;

    prwalls[wallnum] = w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized wall %i.\n", wallnum);

    return (1);
}

void                polymer_updatewall(short wallnum)
{
    short           nwallnum, nnwallnum, curpicnum;
    walltype        *wal;
    sectortype      *sec, *nsec;
    _prwall         *w;
    _prsector       *s, *ns;
    pthtyp*         pth;
    long            xref[2], yref, xdif, ydif, dist;
    float           ypancoef;
    int             i;

    wal = &wall[wallnum];
    sec = &sector[sectorofwall(wallnum)];
    w = prwalls[wallnum];
    s = prsectors[sectorofwall(wallnum)];

    if (w->wallbuffer == NULL)
        w->wallbuffer = calloc(4, sizeof(GLfloat) * 5);

    w->underover = 0;

    w->wallcolor[0] = w->wallcolor[1] = w->wallcolor[2] = ((float)(numpalookups-min(max(wal->shade,0),numpalookups)))/((float)numpalookups);
    w->wallcolor[3] = 1.0f;

    if (wal->cstat & 8)
    {
        xref[0] = wall[wal->point2].x;
        xref[1] = wall[wal->point2].y;
    }
    else
    {
        xref[0] = wal->x;
        xref[1] = wal->y;
    }

    if (wal->nextsector == -1)
    {
        memcpy(w->wallbuffer, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[5], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[10], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[15], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

        curpicnum = wal->picnum;

        if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,wallnum+16384);

        if (!waloff[curpicnum])
            loadtile(curpicnum);

        pth = gltexcache(curpicnum, wal->pal, 0);
        w->wallglpic = pth ? pth->glpic : 0;

        if (pth && (pth->flags & 2) && (pth->palnum != wal->pal)) {
            w->wallcolor[0] *= (float)hictinting[wal->pal].r / 255.0;
            w->wallcolor[1] *= (float)hictinting[wal->pal].g / 255.0;
            w->wallcolor[2] *= (float)hictinting[wal->pal].b / 255.0;
        }

        if (wal->cstat & 4)
            yref = sec->floorz;            
        else
            yref = sec->ceilingz;

        if (wal->ypanning)
        {
            ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
            if (ypancoef < tilesizy[curpicnum])
                ypancoef *= 2;
            ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
        }
        else
            ypancoef = 0;

        i = 0;
        while (i < 4)
        {
            xdif = xref[0] + w->wallbuffer[(i * 5) + 2];
            ydif = xref[1] - w->wallbuffer[(i * 5)];
            dist = ((xdif * xdif) + (ydif * ydif)) != 0;

            w->wallbuffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
            w->wallbuffer[(i * 5) + 4] = (-(float)(yref + w->wallbuffer[(i * 5) + 1]) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

            if (wal->cstat & 256) w->wallbuffer[(i * 5) + 4] = -w->wallbuffer[(i * 5) + 4];

            i++;
        }

        w->underover |= 1;
    }
    else
    {
        nwallnum = wal->nextwall;
        nnwallnum = wall[nwallnum].point2;
        nsec = &sector[wal->nextsector];
        ns = prsectors[wal->nextsector];

        if (((s->floorbuffer[((wallnum - sec->wallptr) * 5) + 1] != ns->floorbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
           (s->floorbuffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->floorbuffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
           (s->floorbuffer[((wallnum - sec->wallptr) * 5) + 1] <= ns->floorbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]))
        {
            memcpy(w->wallbuffer, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[5], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[10], &ns->floorbuffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[15], &ns->floorbuffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);

            if (wal->cstat & 2)
                curpicnum = wall[nwallnum].picnum;
            else
                curpicnum = wal->picnum;

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,wallnum+16384);

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            pth = gltexcache(curpicnum, wal->pal, 0);
            w->wallglpic = pth ? pth->glpic : 0;

            if (pth && (pth->flags & 2) && (pth->palnum != wal->pal)) {
                w->wallcolor[0] *= (float)hictinting[wal->pal].r / 255.0;
                w->wallcolor[1] *= (float)hictinting[wal->pal].g / 255.0;
                w->wallcolor[2] *= (float)hictinting[wal->pal].b / 255.0;
            }

            if (wal->cstat & 4)
                yref = sec->ceilingz;            
            else
                yref = nsec->floorz;

            if (wal->ypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                if (ypancoef < tilesizy[curpicnum])
                    ypancoef *= 2;
                ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                xdif = xref[0] + w->wallbuffer[(i * 5) + 2];
                ydif = xref[1] - w->wallbuffer[(i * 5)];
                dist = ((xdif * xdif) + (ydif * ydif)) != 0;

                w->wallbuffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
                w->wallbuffer[(i * 5) + 4] = (-(float)(yref + w->wallbuffer[(i * 5) + 1]) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if (wal->cstat & 256) w->wallbuffer[(i * 5) + 4] = -w->wallbuffer[(i * 5) + 4];

                i++;
            }

            if (!((sec->floorstat & 1) && (nsec->floorstat & 1)))
                w->underover |= 1;
        }

        if (((s->ceilbuffer[((wallnum - sec->wallptr) * 5) + 1] != ns->ceilbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
           (s->ceilbuffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->ceilbuffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
           (s->ceilbuffer[((wallnum - sec->wallptr) * 5) + 1] >= ns->ceilbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]))
        {
            if (w->overbuffer == NULL)
                w->overbuffer = calloc(4, sizeof(GLfloat) * 5);

            memcpy(w->overbuffer, &ns->ceilbuffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[5], &ns->ceilbuffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[10], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[15], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

            if ((wal->cstat & 16) || (wal->overpicnum == 0))
                curpicnum = wal->picnum;
            else
                curpicnum = wal->picnum;

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,wallnum+16384);

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            pth = gltexcache(curpicnum, wal->pal, 0);
            w->overglpic = pth ? pth->glpic : 0;

            memcpy(w->overcolor, w->wallcolor, sizeof(GLfloat) * 4);

            if (pth && (pth->flags & 2) && (pth->palnum != wal->pal)) {
                w->overcolor[0] *= (float)hictinting[wal->pal].r / 255.0;
                w->overcolor[1] *= (float)hictinting[wal->pal].g / 255.0;
                w->overcolor[2] *= (float)hictinting[wal->pal].b / 255.0;
            }

            if (wal->cstat & 4)
                yref = sec->ceilingz;            
            else
                yref = nsec->ceilingz;

            if (wal->ypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                if (ypancoef < tilesizy[curpicnum])
                    ypancoef *= 2;
                ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                xdif = xref[0] + w->overbuffer[(i * 5) + 2];
                ydif = xref[1] - w->overbuffer[(i * 5)];
                dist = ((xdif * xdif) + (ydif * ydif)) != 0;

                w->overbuffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
                w->overbuffer[(i * 5) + 4] = (-(float)(yref + w->overbuffer[(i * 5) + 1]) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if (wal->cstat & 256) w->overbuffer[(i * 5) + 4] = -w->overbuffer[(i * 5) + 4];

                i++;
            }

            if (!((sec->ceilingstat & 1) && (nsec->ceilingstat & 1)))
                w->underover |= 2;
        }
    }

    if (w->portal == NULL)
        w->portal = calloc(4, sizeof(GLfloat) * 3);

    memcpy(w->portal, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->portal[3], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->portal[6], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->portal[9], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated wall %i.\n", wallnum);
}

void                polymer_drawwall(short wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing wall %i...\n", wallnum);

    w = prwalls[wallnum];

    if (w->underover & 1)
    {
        bglBindTexture(GL_TEXTURE_2D, w->wallglpic);
        bglColor4f(w->wallcolor[0], w->wallcolor[1], w->wallcolor[2], w->wallcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->wallbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->wallbuffer[3]);
        bglDrawArrays(GL_QUADS, 0, 4);
    }
    if (w->underover & 2)
    {
        bglBindTexture(GL_TEXTURE_2D, w->overglpic);
        bglColor4f(w->overcolor[0], w->overcolor[1], w->overcolor[2], w->overcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->overbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->overbuffer[3]);
        bglDrawArrays(GL_QUADS, 0, 4);
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing wall %i...\n", wallnum);
}

// HSR
void                polymer_extractfrustum(void)
{
    GLfloat         matrix[16];
    int             i;

    bglMatrixMode(GL_TEXTURE);
    bglGetFloatv(GL_PROJECTION_MATRIX, matrix);
    bglLoadMatrixf(matrix);
    bglGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    bglMultMatrixf(matrix);
    bglGetFloatv(GL_TEXTURE_MATRIX, matrix);
    bglLoadIdentity();

    i = 0;
    while (i < 4)
    {
        frustum[i] = matrix[(4 * i) + 3] + matrix[4 * i];               // left
        frustum[i + 4] = matrix[(4 * i) + 3] - matrix[4 * i];           // right
        frustum[i + 8] = matrix[(4 * i) + 3] - matrix[(4 * i) + 1];     // top
        frustum[i + 12] = matrix[(4 * i) + 3] + matrix[(4 * i) + 1];    // bottom
        i++;
    }
}

int                 polymer_portalinfrustum(short wallnum)
{
    int             i, j, k;
    float           sqdist;
    _prwall         *w;

    w = prwalls[wallnum];

    i = 0;
    while (i < 4)
    {
        j = k = 0;
        while (j < 4)
        {
            sqdist = frustum[(i * 4) + 0] * w->portal[(j * 3) + 0] +
                     frustum[(i * 4) + 1] * w->portal[(j * 3) + 1] + 
                     frustum[(i * 4) + 2] * w->portal[(j * 3) + 2] + 
                     frustum[(i * 4) + 3];
            if (sqdist  < 0)
                k++;
            j++;
        }
        if (k == 4)
            return (0); // OUT !
        i++;
    }

    return (1);
}

void                polymer_addcliplane(_equation clip, _equation left, _equation right, float refx, float refy)
{
    if (cliplanecount == maxcliplanecount)
        cliplanes = realloc(cliplanes, sizeof(_cliplane) * ++maxcliplanecount);

    cliplanes[cliplanecount].clip = clip;
    cliplanes[cliplanecount].left = left;
    cliplanes[cliplanecount].right = right;
    cliplanes[cliplanecount].ref.x = refx;
    cliplanes[cliplanecount].ref.y = refy;

    cliplanecount++;
}

int                 polymer_wallincliplanes(short wallnum)
{
    walltype        *wal;
    _point2d        p1, p2;
    int             i, j;
    
    wal = &wall[wallnum];

    p1.x = wal->x;
    p1.y = wal->y;

    p2.x = wall[wal->point2].x;
    p2.y = wall[wal->point2].y;

    i = 0;
    while (i < cliplanecount)
    {
        j = 0;

        if ((sameside(&cliplanes[i].clip, &cliplanes[i].ref, &p1) == 0) && (sameside(&cliplanes[i].clip, &cliplanes[i].ref, &p2) == 0))
            j++;

        if ((sameside(&cliplanes[i].left, &cliplanes[i].ref, &p1) == 1) && (sameside(&cliplanes[i].left, &cliplanes[i].ref, &p2) == 1))
            j++;

        if ((sameside(&cliplanes[i].right, &cliplanes[i].ref, &p1) == 1) && (sameside(&cliplanes[i].right, &cliplanes[i].ref, &p2) == 1))
            j++;

        if (j == 3)
            return (0);

        i++;
    }

    return (1);
}
