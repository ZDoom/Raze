// blah
#ifdef POLYMOST
#include "polymer.h"

// CVARS
int             pr_scissorstest = 1;
int             pr_fov = 426;           // appears to be the classic setting.
int             pr_showportals = 0;
int             pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int             pr_wireframe = 0;

// DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];

GLfloat         skybox[16];

// CONTROL
float           pos[3];

float           *frustumstack;
int             frustumstacksize = 0;

char            *frustumsizes;
GLint           *projectedportals;
int             maxfrustumcount = 0;

int             frustumdepth = 0;
int             frustumstackposition = 0;

int             *curportal;

//float           frustumnorms[5];

GLfloat         *clippedportalpoints = NULL;
float           *distances = NULL;
int             maxclippedportalpointcount = 0;
int             clippedportalpointcount;

GLfloat         triangle[9];

GLdouble        modelviewmatrix[16];
GLdouble        projectionmatrix[16];
GLint           viewport[4];

int             updatesectors = 1;

GLUtesselator*  prtess;
int             tempverticescount;
GLdouble        tempvertice[3];

short           cursky;

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

    prtess = bgluNewTess();
    if (prtess == 0)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Tesselator initialization failed.\n");
        return (0);
    }

    if (frustumstacksize == 0)
    {
        frustumstacksize = 20; // 5 planes, 4 components
        frustumstack = malloc(frustumstacksize * sizeof(float));
        if (!frustumstack)
        {
            if (pr_verbosity >= 1) OSD_Printf("PR : Cannot allocate initial frustum stack : malloc failed.\n");
            frustumstacksize = 0;
            return (0);
        }
    }

    if (maxfrustumcount == 0)
    {
        maxfrustumcount = 1;
        frustumsizes = malloc (maxfrustumcount * sizeof(char));
        projectedportals = malloc(maxfrustumcount * 4 * sizeof(GLint));
        if (!frustumsizes || !projectedportals)
        {
            if (pr_verbosity >= 1) OSD_Printf("PR : Cannot allocate initial frustum size record : malloc failed.\n");
            maxfrustumcount = 0;
            return (0);
        }
    }

    if (maxclippedportalpointcount == 0)
    {
        maxclippedportalpointcount = 4;
        clippedportalpoints = calloc(maxclippedportalpointcount, sizeof(GLfloat) * 3);
        distances = calloc(maxclippedportalpointcount, sizeof(float));
        if (!clippedportalpoints || !distances)
        {
            if (pr_verbosity >= 1) OSD_Printf("PR : Cannot allocate initial clipping memory : malloc failed.\n");
            maxclippedportalpointcount = 0;
            return (0);
        }
    }

    polymer_loadboard();

    polymer_initskybox();

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete.\n");
    return (1);
}

void                polymer_glinit(void)
{
    float           a;

    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClearStencil(0);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    bglViewport(0, 0, xdim, ydim);

    bglGetIntegerv(GL_VIEWPORT, viewport);

    // texturing
    bglEnable(GL_TEXTURE_2D);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    bglEnable(GL_DEPTH_TEST);
    bglDepthFunc(GL_LEQUAL);

    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bgluPerspective((float)(pr_fov) / (2048.0f / 360.0f), (float)xdim / (float)ydim, 0.001f, 1000000.0f);

    // get the new projection matrix
    bglGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglDisable(GL_FOG);

    bglFogi(GL_FOG_MODE, GL_EXP2);
    //glFogfv(GL_FOG_COLOR, fogColor);
    bglEnable(GL_FOG);

    a = (1 - ((float)(visibility) / 512.0f)) / 10.0f;
    bglFogf(GL_FOG_DENSITY, 0.1f - a);
    bglFogf(GL_FOG_START, 0.0f);
    bglFogf(GL_FOG_END, 1000000.0f);

    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);

    if (pr_scissorstest)
        bglEnable(GL_SCISSOR_TEST);
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

    polymer_getsky();

    if (pr_verbosity >= 1) OSD_Printf("PR : Board loaded.\n");
}
void                polymer_drawrooms(int daposx, int daposy, int daposz, short daang, int dahoriz, short dacursectnum, int root)
{
    int             i, j;
    float           ang, horizang, tiltang;
    _point2d        ref;
    sectortype      *sec;
    walltype        *wal;
    short           drawnsectors, fov;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    ang = (float)(daang) / (2048.0f / 360.0f);
    horizang = (float)(100 - dahoriz) / (512.0f / 180.0f);
    tiltang = (gtang * 90.0f);
    fov = (pr_fov * (float)xdim / (float)ydim * 1) / 2;

    pos[0] = daposy;
    pos[1] = -daposz;
    pos[2] = -daposx;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglRotatef(tiltang, 0.0f, 0.0f, -1.0f);
    bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

    bglDisable(GL_DEPTH_TEST);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    polymer_drawartsky(cursky);
    bglEnable(GL_DEPTH_TEST);

    bglScalef(1.0f / 1000.0f, 1.0f / 16000.0f, 1.0f / 1000.0f);
    bglTranslatef(-pos[0], -pos[1], -pos[2]);

    // get the new modelview
    bglGetDoublev(GL_MODELVIEW_MATRIX, modelviewmatrix);

    polymer_extractfrustum(modelviewmatrix, projectionmatrix);
    if (pr_scissorstest)
    {
        memcpy(projectedportals, viewport, sizeof(GLint) * 4);
        bglScissor(projectedportals[0],
                   viewport[3] - projectedportals[3],
                   projectedportals[2] - projectedportals[0],
                   projectedportals[3] - projectedportals[1]);
    }
    
    frustumsizes[0] = 5;
    frustumdepth = 0;
    frustumstackposition = 0;

    // game tic
    if (updatesectors)
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

    // external view (editor)
    if (dacursectnum == -1)
    {
        i = 0;
        while (i < numsectors)
        {
            polymer_drawsector(i);
            i++;
        }

        i = 0;
        while (i < numwalls)
        {
            polymer_drawwall(i);
            i++;
        }
        return;
    }

    // unflag all sectors
    i = 0;
    while (i < numsectors)
    {
        prsectors[i]->drawingstate = 0;
        i++;
    }
    i = 0;
    while (i < numwalls)
    {
        prwalls[i]->drawn = 0;
        i++;
    }

    // stupid waste of performance - the position doesn't match the sector number when running from a sector to another
    updatesector(daposx, daposy, &dacursectnum);

    // GO
    polymer_drawroom(dacursectnum);

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
}

void                polymer_rotatesprite(int sx, int sy, int z, short a, short picnum, signed char dashade, char dapalnum, char dastat, int cx1, int cy1, int cx2, int cy2)
{
}

void                polymer_drawmaskwall(int damaskwallcnt)
{
    OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);
}

void                polymer_drawsprite(int snum)
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
    int             ceilz, florz;
    int             tex, tey;
    float           secangcos, secangsin, scalecoef;
    int             ang;
    short           curstat, curpicnum;
    char            curxpanning, curypanning;
    GLfloat*        curbuffer;
    GLuint          *curglpic, *curfbglpic;
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
        if ((-wal->x != s->verts[(i*3)+2]))
        {
            s->verts[(i*3)+2] = s->floorbuffer[(i*5)+2] = s->ceilbuffer[(i*5)+2] = -wal->x;
            s->controlstate |= 2;
        }
        if ((wal->y != s->verts[i*3]))
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
        curfbglpic = &s->floorfbglpic;

        while (j > 0)
        {
            if (j == 1)
            {
                curbuffer = s->ceilcolor;
                curstat = sec->ceilingshade;
                curxpanning = sec->ceilingpal;
                curpicnum = sec->ceilingpicnum;
                curglpic = &s->ceilglpic;
                curfbglpic = &s->ceilfbglpic;
            }

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,sectnum);

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            curbuffer[0] = curbuffer[1] = curbuffer[2] = ((float)(numpalookups-min(max(curstat,0),numpalookups)))/((float)numpalookups);
            curbuffer[3] = 1.0f;

            pth = gltexcache(curpicnum,curxpanning,0);

            if (pth && (pth->flags & 2) && (pth->palnum != curxpanning))
            {
                curbuffer[0] *= (float)hictinting[curxpanning].r / 255.0;
                curbuffer[1] *= (float)hictinting[curxpanning].g / 255.0;
                curbuffer[2] *= (float)hictinting[curxpanning].b / 255.0;
            }

            *curglpic = (pth) ? pth->glpic : 0;

            if (pth && (pth->flags & 16))
                *curfbglpic = pth->ofb->glpic;
            else
                *curfbglpic = 0;

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
{
    // This callback is called by the tesselator when it detects an intersection between contours (HELLO ROTATING SPOTLIGHT IN E1L1).
    GLdouble*       ptr;

    tempvertice[0] = v[0];
    tempvertice[1] = v[1];
    tempvertice[2] = v[2];

    ptr = tempvertice;
    *out = tempvertice;

    if (pr_verbosity >= 2) OSD_Printf("PR : Created additional geometry for sector tesselation.\n");
}


void PR_CALLBACK    polymer_tesserror(GLenum error)
{
    // This callback is called by the tesselator whenever it raises an error.
    if (pr_verbosity >= 1) OSD_Printf("PR : Tesselation error number %i reported : %s.\n", error, bgluErrorString(errno));
}


void PR_CALLBACK    polymer_tessedgeflag(GLenum error)
{
    // Passing an edgeflag callback forces the tesselator to output a triangle list
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
{
    // This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
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

    bgluTessCallback(prtess, GLU_TESS_VERTEX_DATA, polymer_tessvertex);
    bgluTessCallback(prtess, GLU_TESS_EDGE_FLAG, polymer_tessedgeflag);
    //bgluTessCallback(prtess, GLU_TESS_COMBINE, polymer_tesscombine);
    bgluTessCallback(prtess, GLU_TESS_ERROR, polymer_tesserror);

    bgluTessProperty(prtess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);

    bgluTessBeginPolygon(prtess, s);
    bgluTessBeginContour(prtess);

    i = 0;
    while (i < sec->wallnum)
    {
        bgluTessVertex(prtess, s->verts + (3 * i), (void *)i);
        if ((i != (sec->wallnum - 1)) && ((sec->wallptr + i) > wall[sec->wallptr + i].point2))
        {
            bgluTessEndContour(prtess);
            bgluTessBeginContour(prtess);
        }
        i++;
    }
    bgluTessEndContour(prtess);
    bgluTessEndPolygon(prtess);

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
    int             zdiff;

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

        if (s->floorfbglpic)
        {
            bglBindTexture(GL_TEXTURE_2D, s->floorfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->floorindices);
        }
    }

    // ceiling
    if (!(sec->ceilingstat & 1))
    {
        bglBindTexture(GL_TEXTURE_2D, s->ceilglpic);
        bglColor4f(s->ceilcolor[0], s->ceilcolor[1], s->ceilcolor[2], s->ceilcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), s->ceilbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &s->ceilbuffer[3]);
        bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->ceilindices);

        if (s->ceilfbglpic)
        {
            bglBindTexture(GL_TEXTURE_2D, s->ceilfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->floorindices);
        }
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
    char            curxpanning, curypanning;
    walltype        *wal;
    sectortype      *sec, *nsec;
    _prwall         *w;
    _prsector       *s, *ns;
    pthtyp*         pth;
    int             xref, yref, xdif, ydif;
    float           ypancoef, dist;
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
        xref = 1;
    else
        xref = 0;

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

        if (pth && (pth->flags & 16))
            w->wallfbglpic = pth->ofb->glpic;
        else
            w->wallfbglpic = 0;

        if (pth && (pth->flags & 2) && (pth->palnum != wal->pal))
        {
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
            if ((i == 0) || (i == 3))
                dist = xref;
            else
                dist = (xref == 0);

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
            {
                curpicnum = wall[nwallnum].picnum;
                curxpanning = wall[nwallnum].xpanning;
                curypanning = wall[nwallnum].ypanning;
            }
            else
            {
                curpicnum = wal->picnum;
                curxpanning = wal->xpanning;
                curypanning = wal->ypanning;
            }

            if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,wallnum+16384);

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            pth = gltexcache(curpicnum, wal->pal, 0);
            w->wallglpic = pth ? pth->glpic : 0;

            if (pth && (pth->flags & 16))
                w->wallfbglpic = pth->ofb->glpic;
            else
                w->wallfbglpic = 0;

            if (pth && (pth->flags & 2) && (pth->palnum != wal->pal))
            {
                w->wallcolor[0] *= (float)hictinting[wal->pal].r / 255.0;
                w->wallcolor[1] *= (float)hictinting[wal->pal].g / 255.0;
                w->wallcolor[2] *= (float)hictinting[wal->pal].b / 255.0;
            }

            if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                yref = sec->ceilingz;
            else
                yref = nsec->floorz;

            if (curypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                if (ypancoef < tilesizy[curpicnum])
                    ypancoef *= 2;
                ypancoef *= (float)(curypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                if ((i == 0) || (i == 3))
                    dist = xref;
                else
                    dist = (xref == 0);

                w->wallbuffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + curxpanning) / (float)(tilesizx[curpicnum]);
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

            if (pth && (pth->flags & 16))
                w->overfbglpic = pth->ofb->glpic;
            else
                w->overfbglpic = 0;

            memcpy(w->overcolor, w->wallcolor, sizeof(GLfloat) * 4);

            if (pth && (pth->flags & 2) && (pth->palnum != wal->pal))
            {
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
                if ((i == 0) || (i == 3))
                    dist = xref;
                else
                    dist = (xref == 0);

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

        if (w->wallfbglpic)
        {
            bglBindTexture(GL_TEXTURE_2D, w->wallfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawArrays(GL_QUADS, 0, 4);
        }
    }
    if (w->underover & 2)
    {
        bglBindTexture(GL_TEXTURE_2D, w->overglpic);
        bglColor4f(w->overcolor[0], w->overcolor[1], w->overcolor[2], w->overcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->overbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->overbuffer[3]);
        bglDrawArrays(GL_QUADS, 0, 4);

        if (w->overfbglpic)
        {
            bglBindTexture(GL_TEXTURE_2D, w->overfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawArrays(GL_QUADS, 0, 4);
        }
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing wall %i...\n", wallnum);
}

// HSR
void                polymer_portaltofrustum(GLfloat* portal, int portalpointcount, GLfloat* pos, GLfloat* frustum)
{
    int             i;

    memcpy(&triangle[3], pos, sizeof(GLfloat) * 3);
    i = 0;
    while (i < portalpointcount)
    {
        memcpy(&triangle[0], &portal[i * 3], sizeof(GLfloat) * 3);
        if (i != (portalpointcount - 1))
            memcpy(&triangle[6], &portal[(i+1) * 3], sizeof(GLfloat) * 3);
        else
            memcpy(&triangle[6], &portal[0], sizeof(GLfloat) * 3);
        polymer_triangletoplane(triangle, &frustum[i*4]);
        i++;
    }

    //near
    //memcpy(&triangle[3], &portal[(i-2) * 3], sizeof(GLfloat) * 3);
    //polymer_triangletoplane(triangle, &frustum[i*4]);
}

void                polymer_triangletoplane(GLfloat* triangle, GLfloat* plane)
{
    GLfloat         vec1[3], vec2[3];

    vec1[0] = triangle[3] - triangle[0];
    vec1[1] = triangle[4] - triangle[1];
    vec1[2] = triangle[5] - triangle[2];

    vec2[0] = triangle[6] - triangle[0];
    vec2[1] = triangle[7] - triangle[1];
    vec2[2] = triangle[8] - triangle[2];

    polymer_crossproduct(vec1, vec2, plane);

    plane[3] = -(plane[0] * triangle[0] + plane[1] * triangle[1] + plane[2] * triangle[2]);
}

void                polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLfloat* out)
{
    out[0] = in_a[1] * in_b[2] - in_a[2] * in_b[1];
    out[1] = in_a[2] * in_b[0] - in_a[0] * in_b[2];
    out[2] = in_a[0] * in_b[1] - in_a[1] * in_b[0];
}

void                polymer_extractfrustum(GLdouble* modelview, GLdouble* projection)
{
    GLdouble        matrix[16];
    int             i;

    bglMatrixMode(GL_TEXTURE);
    bglLoadMatrixd(projection);
    bglMultMatrixd(modelview);
    bglGetDoublev(GL_TEXTURE_MATRIX, matrix);
    bglLoadIdentity();

    i = 0;
    while (i < 4)
    {
        frustumstack[i] = matrix[(4 * i) + 3] + matrix[4 * i];               // left
        frustumstack[i + 4] = matrix[(4 * i) + 3] - matrix[4 * i];           // right
        frustumstack[i + 8] = matrix[(4 * i) + 3] - matrix[(4 * i) + 1];     // top
        frustumstack[i + 12] = matrix[(4 * i) + 3] + matrix[(4 * i) + 1];    // bottom
        frustumstack[i + 16] = matrix[(4 * i) + 3] + matrix[(4 * i) + 2];    // near
        i++;
    }
    i = 0;
    /*while (i < 5)
    {
        // frustum plane norms
        frustumnorms[i] = sqrt((frustum[(i * 4) + 0] * frustum[(i * 4) + 0]) +
                               (frustum[(i * 4) + 1] * frustum[(i * 4) + 1]) +
                               (frustum[(i * 4) + 2] * frustum[(i * 4) + 2]));
        i++;
    }*/

    if (pr_verbosity >= 3) OSD_Printf("PR : Frustum extracted.\n");
}

void                polymer_drawroom(short sectnum)
{
    int             i, j, secwallcount, curwallcount, walclips;
    sectortype      *sec;
    walltype        *wal;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing room %d.\n", sectnum);

    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];

    secwallcount = sec->wallnum;

    // first draw the sector
    polymer_drawsector(sectnum);
    prsectors[sectnum]->drawingstate = 1;

    i = 0;
    while (i < secwallcount)
    {
        walclips = 0;
        curwallcount = 1;

        if ((prwalls[sec->wallptr + i]->drawn == 0) &&
            (wallvisible(sec->wallptr + i)) &&
            (walclips |= polymer_portalinfrustum(sec->wallptr + i)))
        {
            if ((wal->nextsector != -1) && (prsectors[wal->nextsector]->drawingstate == 0))
            {
                // check for contigous walls with the same nextsector
                if ((i == 0) &&
                    (wal->nextsector == wall[sec->wallptr + secwallcount - 1].nextsector) &&
                    (prwalls[sec->wallptr + secwallcount - 1]->drawn == 0) &&
                    (wallvisible(sec->wallptr + secwallcount - 1)) &&
                    (walclips |= polymer_portalinfrustum(sec->wallptr + secwallcount - 1)))
                {
                    j = secwallcount - 2;
                    while ((j > 1) &&
                           (wall[sec->wallptr + j].nextsector == wal->nextsector) &&
                           (prwalls[sec->wallptr + j]->drawn == 0) &&
                           (wallvisible(sec->wallptr + j)) &&
                           (walclips |= polymer_portalinfrustum(sec->wallptr + j)))
                        j--;
                    secwallcount = i = j + 1;
                    curwallcount += sec->wallnum - secwallcount;
                }
                j = i + 1;
                while ((wall[sec->wallptr + j].nextsector == wal->nextsector) &&
                       (prwalls[sec->wallptr + j]->drawn == 0) &&
                       (wallvisible(sec->wallptr + j)) &&
                       (walclips |= polymer_portalinfrustum(sec->wallptr + j)))
                    j++;
                curwallcount += j - i - 1;
            }

            j = 0;
            while (j < curwallcount)
            {
                prwalls[sec->wallptr + ((i + j) % sec->wallnum)]->drawn = 1;
                j++;
            }

            if ((wal->nextsector != -1) && (prsectors[wal->nextsector]->drawingstate == 0))
            {
                clippedportalpointcount = 4 + ((curwallcount - 1) * 2);

                // generate the portal from all the walls
                if (curwallcount == 1)
                    memcpy(clippedportalpoints, prwalls[sec->wallptr + i]->portal, sizeof(float) * 4 * 3);
                else
                {
                    if (clippedportalpointcount > maxclippedportalpointcount)
                    {
                        clippedportalpoints = realloc(clippedportalpoints, sizeof(GLfloat) * 3 * clippedportalpointcount);
                        distances = realloc(distances, sizeof(float) * clippedportalpointcount);
                        maxclippedportalpointcount = clippedportalpointcount;
                    }

                    j = 0;
                    while (j < curwallcount)
                    {
                        memcpy(&clippedportalpoints[j * 3],
                               &prwalls[sec->wallptr + ((i + j) % sec->wallnum)]->portal[0],
                               sizeof(GLfloat) * 3);
                        memcpy(&clippedportalpoints[(clippedportalpointcount - j - 1) * 3],
                               &prwalls[sec->wallptr + ((i + j) % sec->wallnum)]->portal[9],
                               sizeof(GLfloat) * 3);
                        j++;
                    }
                    memcpy(&clippedportalpoints[j * 3],
                           &prwalls[sec->wallptr + ((i + j - 1) % sec->wallnum)]->portal[3],
                           sizeof(GLfloat) * 3);
                    memcpy(&clippedportalpoints[(clippedportalpointcount - j - 1) * 3],
                           &prwalls[sec->wallptr + ((i + j - 1) % sec->wallnum)]->portal[6],
                           sizeof(GLfloat) * 3);
                }

                if (walclips > 1) // the wall intersected at least one plane and needs to be clipped
                    clippedportalpointcount = polymer_cliptofrustum(sec->wallptr + i);

                if (pr_showportals)
                {
                    bglDisable(GL_FOG);
                    bglDisable(GL_TEXTURE_2D);
                    bglColor4f(0.0f, 1.0f, 0.0f, 1.0f);
                    bglBegin(GL_LINE_LOOP);
                    j = 0;
                    while (j < clippedportalpointcount)
                    {
                        bglVertex3fv(&clippedportalpoints[j*3]);
                        j++;
                    }
                    bglEnd();
                    bglEnable(GL_FOG);
                    bglEnable(GL_TEXTURE_2D);
                }

                if (frustumstacksize <= (frustumstackposition + (frustumsizes[frustumdepth] + clippedportalpointcount + 1) * 4))
                {
                    frustumstacksize += clippedportalpointcount * 4;
                    frustumstack = realloc(frustumstack, sizeof(GLfloat) * frustumstacksize);
                }
                if (maxfrustumcount == (frustumdepth + 1))
                {
                    maxfrustumcount++;
                    frustumsizes = realloc(frustumsizes, sizeof(char) * maxfrustumcount);
                    projectedportals = realloc(projectedportals, maxfrustumcount * 4 * sizeof(GLint));
                }
                // push a new frustum on the stack
                frustumstackposition += frustumsizes[frustumdepth] * 4;
                frustumdepth++;
                frustumsizes[frustumdepth] = clippedportalpointcount;

                // calculate the new frustum data
                polymer_portaltofrustum(clippedportalpoints, clippedportalpointcount, pos, &frustumstack[frustumstackposition]);

                if (pr_scissorstest)
                {
                    // project the scissor
                    curportal = &projectedportals[frustumdepth * 4];
                    polymer_getportal(clippedportalpoints, clippedportalpointcount, curportal);
                    bglScissor(curportal[0] - 1, viewport[3] - curportal[3] - 1, curportal[2] - curportal[0] + 2, curportal[3] - curportal[1] + 2);

                    // draw the new scissor
                    if (pr_showportals)
                        polymer_drawportal(curportal);
                }

                // recurse
                polymer_drawroom(wal->nextsector);

                if (pr_verbosity >= 3) OSD_Printf("PR : Popping...\n");
                frustumdepth--;
                frustumstackposition -= frustumsizes[frustumdepth] * 4;

                if (pr_scissorstest)
                {
                    // pop the scissor
                    curportal = &projectedportals[frustumdepth * 4];
                    bglScissor(curportal[0] - 1, viewport[3] - curportal[3] - 1, curportal[2] - curportal[0] + 2, curportal[3] - curportal[1] + 2);
                }
            }

            j = 0;
            while (j < curwallcount)
            {
                polymer_drawwall(sec->wallptr + ((i + j) % sec->wallnum));
                prwalls[sec->wallptr + ((i + j) % sec->wallnum)]->drawn = 0;
                j++;
            }
        }
        i += curwallcount;
        if (i > sec->wallnum)
            i = i % sec->wallnum;

        wal = &wall[sec->wallptr + i];
    }

    prsectors[sectnum]->drawingstate = 0;
}

int                 polymer_portalinfrustum(short wallnum)
{
    int             i, j, k, result;
    float           sqdist, *frustum;
    _prwall         *w;

    frustum = &frustumstack[frustumstackposition];
    w = prwalls[wallnum];

    i = result = 0;
    while (i < frustumsizes[frustumdepth])
    {
        j = k = 0;
        while (j < 4)
        {
            sqdist = frustum[(i * 4) + 0] * w->portal[(j * 3) + 0] +
                     frustum[(i * 4) + 1] * w->portal[(j * 3) + 1] +
                     frustum[(i * 4) + 2] * w->portal[(j * 3) + 2] +
                     frustum[(i * 4) + 3];
            if (sqdist < 0)
                k++;
            j++;
        }
        if (k == 4)
            return (0); // OUT !
        if (k != 0)
            result |= 2<<i;
        i++;
    }

    result |= 1;

    return (result);
}

float               polymer_pointdistancetoplane(GLfloat* point, GLfloat* plane)
{
    float           result, t;

    result = plane[0] * point[0] +
             plane[1] * point[1] +
             plane[2] * point[2] +
             plane[3];

    return (result);
}

float               polymer_pointdistancetopoint(GLfloat* point1, GLfloat* point2)
{
    return ((point2[0] - point1[0]) * (point2[0] - point1[0]) +
            (point2[1] - point1[1]) * (point2[1] - point1[1]) +
            (point2[2] - point1[2]) * (point2[2] - point1[2]));
}

void                polymer_lineplaneintersection(GLfloat *point1, GLfloat *point2, float dist1, float dist2, GLfloat *output)
{
    GLfloat         result[3];
    float           s;

    s = dist1 / (dist1 - dist2);

    result[0] = point1[0] + (s * (point2[0] - point1[0]));
    result[1] = point1[1] + (s * (point2[1] - point1[1]));
    result[2] = point1[2] + (s * (point2[2] - point1[2]));

    memcpy(output, result, sizeof(GLfloat) * 3);
}

int                 polymer_cliptofrustum(short wallnum)
{
    // sutherland-hofnman polygon clipping algorithm against all planes of the frustum
    GLfloat         intersect[3], *frustum;
    int             i, j, k, l, m, result, exitpoint;

    exitpoint = 0; // annoying "warning: 'exitpoint' may be used uninialized in this function"

    frustum = &frustumstack[frustumstackposition];
    result = clippedportalpointcount; // 4 points to start with
    if (pr_verbosity >= 3) OSD_Printf("PR : Clipping wall %d...\n", wallnum);
    //memcpy(clippedportalpoints, prwalls[wallnum]->portal, sizeof(GLfloat) * 3 * 4);

    i = 0;
    while (i < frustumsizes[frustumdepth])
    {
        if ((frustumdepth == 0) && (i == frustumsizes[frustumdepth] - 1))
        {
            // don't near-clip with the viewing frustum
            i++;
            continue;
        }
        // frustum planes
        j = k = 0;
        m = -1;
        while (j < result)
        {
            distances[j] = polymer_pointdistancetoplane(&clippedportalpoints[j * 3], &frustum[i * 4]);
            if (distances[j] < 0)
                k = 1; // at least one is outside
            if ((distances[j] > 0) && (m < 0))
                m = j; // first point inside
            j++;
        }

        if ((k) && (m != -1))
        {
            // divide and conquer while we may
            j = m;
            while ((j != m) || (k))
            {
                if (k)
                {
                    k = 0;
                    if (pr_verbosity >= 3) OSD_Printf("PR : Clipping against frustum plane %d starting with point %d...\n", i, m);
                }

                l = j + 1; // L is next point
                if (l == result)
                    l = 0;

                if ((distances[j] >= 0) && (distances[l] < 0))
                {
                    // case 1 : line exits the plane -> compute intersection
                    polymer_lineplaneintersection(&clippedportalpoints[j * 3], &clippedportalpoints[l * 3], distances[j], distances[l], intersect);
                    exitpoint = l;
                    if (pr_verbosity >= 3) OSD_Printf("PR : %d: EXIT\n", j);
                }
                else if ((distances[j] < 0) && (distances[l] < 0))
                {
                    // case 2 : line is totally outside the plane
                    if (j != exitpoint)
                    {
                        // if we didn't just exit we need to delete this point forever
                        result--;
                        if (j != result)
                        {
                            memmove(&clippedportalpoints[j * 3], &clippedportalpoints[l * 3], (result - j) * sizeof(GLfloat) * 3);
                            memmove(&distances[j], &distances[l], (result - j) * sizeof(float));
                            if (m >= l)
                            {
                                m--;
                            }
                            l--;
                        }
                        if (l == result)
                            l = 0;
                    }
                    else
                        memcpy(&clippedportalpoints[j * 3], intersect, sizeof(GLfloat) * 3); // replace point by intersection from previous entry
                    if (pr_verbosity >= 3) OSD_Printf("PR : %d: IN\n", j);
                }
                else if ((distances[j] < 0) && (distances[l] >= 0))
                {
                    // case 3 : we're going back into the plane -> replace current point with intersection
                    if (j == exitpoint)
                    {
                        // if we just exited a point is created
                        if (result == maxclippedportalpointcount)
                        {
                            clippedportalpoints = realloc(clippedportalpoints, sizeof(GLfloat) * 3 * (maxclippedportalpointcount + 1));
                            distances = realloc(distances, sizeof(float) * (maxclippedportalpointcount + 1));
                            maxclippedportalpointcount++;
                        }
                        if ((result - 1) != j)
                        {
                            memmove(&clippedportalpoints[(l + 1) * 3], &clippedportalpoints[l * 3], (result - l) * sizeof(GLfloat) * 3);
                            memmove(&distances[l + 1], &distances[l], (result - l) * sizeof(float));
                            if (m >= l)
                                m++;
                        }
                        result++;
                        polymer_lineplaneintersection(&clippedportalpoints[j * 3], &clippedportalpoints[l * 3], distances[j], distances[l], &clippedportalpoints[(j + 1) * 3]);
                        memcpy(&clippedportalpoints[j * 3], intersect, sizeof(GLfloat) * 3); // replace point by intersection from previous entry
                        if ((l) && (l != m))
                            l++; // if not at the end of the list, skip the point we just created
                    }
                    else
                        polymer_lineplaneintersection(&clippedportalpoints[j * 3], &clippedportalpoints[l * 3], distances[j], distances[l], &clippedportalpoints[j * 3]);
                    if (pr_verbosity >= 3) OSD_Printf("PR : %d: ENTER\n", j);
                }
                else
                    if (pr_verbosity >= 3) OSD_Printf("PR : %d: OUT\n", j);

                j = l; // L
            }
        }

        if (pr_verbosity >= 3) OSD_Printf("PR : Plane %d finished, result : %d.\n", i, result);

        i++;
    }

    // pruning duplicates
    i = 0;
    while (i < result)
    {
        if (i == (result - 1))
            intersect[0] = polymer_pointdistancetopoint(&clippedportalpoints[i*3], &clippedportalpoints[0]);
        else
            intersect[0] = polymer_pointdistancetopoint(&clippedportalpoints[i*3], &clippedportalpoints[(i+1)*3]);
        if (intersect[0] < 1)
        {
            if (i == (result - 1))
                result--;
            else
            {
                result--;
                memmove(&clippedportalpoints[i*3], &clippedportalpoints[(i+1)*3], (result - i) * sizeof(GLfloat) * 3);
                continue;
            }
        }
        i++;
    }

    return (result);
}

void                polymer_getportal(GLfloat* portalpoints, int portalpointcount, GLint* output)
{
    GLdouble        result[3];
    int             i;

    bgluProject(portalpoints[0], portalpoints[1], portalpoints[2], modelviewmatrix, projectionmatrix, viewport, &(result[0]), &(result[1]), &(result[2]));

    result[1] = viewport[3] - result[1];

    output[0] = (GLint)result[0];
    output[1] = (GLint)result[1];
    output[2] = (GLint)result[0];
    output[3] = (GLint)result[1];

    i = 1;
    while (i < portalpointcount)
    {
        bgluProject(portalpoints[(i * 3)], portalpoints[(i * 3) + 1], portalpoints[(i * 3) + 2], modelviewmatrix, projectionmatrix, viewport, &(result[0]), &(result[1]), &(result[2]));

        result[1] = viewport[3] - result[1];

        if (((GLint)result[0]) < output[0])
            output[0] = (GLint)result[0];
        if (((GLint)result[0]) > output[2])
            output[2] = (GLint)result[0];
        if (((GLint)result[1]) < output[1])
            output[1] = (GLint)result[1];
        if (((GLint)result[1]) > output[3])
            output[3] = (GLint)result[1];

        i++;
    }
}

void                polymer_drawportal(int *portal)
{
    bglMatrixMode(GL_PROJECTION);
    bglPushMatrix();
    bglLoadIdentity();
    bglOrtho(0, xdim, ydim, 0, 0, 1);
    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();

    bglColor4f(1.0f, (1.0f - 0.1 * (frustumdepth-1)), (1.0f - 0.1 * (frustumdepth-1)), 1.0f);
    bglDisable(GL_TEXTURE_2D);

    bglBegin(GL_LINE_LOOP);
    bglVertex3f(portal[0], portal[1], 0.0f);
    bglVertex3f(portal[0], portal[3], 0.0f);
    bglVertex3f(portal[2], portal[3], 0.0f);
    bglVertex3f(portal[2], portal[1], 0.0f);
    bglEnd();

    bglEnable(GL_TEXTURE_2D);

    bglPopMatrix();
    bglMatrixMode(GL_PROJECTION);
    bglPopMatrix();
    bglMatrixMode(GL_MODELVIEW);
}

// SKIES
void                polymer_initskybox(void)
{
    GLfloat         halfsqrt2 = 0.70710678f;

    skybox[0] = -1.0f;          skybox[1] = 0.0f;           // 0
    skybox[2] = -halfsqrt2;     skybox[3] = halfsqrt2;      // 1
    skybox[4] = 0.0f;           skybox[5] = 1.0f;           // 2
    skybox[6] = halfsqrt2;      skybox[7] = halfsqrt2;      // 3
    skybox[8] = 1.0f;           skybox[9] = 0.0f;           // 4
    skybox[10] = halfsqrt2;     skybox[11] = -halfsqrt2;    // 5
    skybox[12] = 0.0f;          skybox[13] = -1.0f;         // 6
    skybox[14] = -halfsqrt2;    skybox[15] = -halfsqrt2;    // 7

    /*skybox[0] = -1.0f;          skybox[1] = 0.0f;           // 0
    skybox[2] = -1.0f;          skybox[3] = 1.0;      // 1
    skybox[4] = 0.0f;           skybox[5] = 1.0f;           // 2
    skybox[6] = 1.0f;           skybox[7] = 1.0f;      // 3
    skybox[8] = 1.0f;           skybox[9] = 0.0f;           // 4
    skybox[10] = 1.0;           skybox[11] = -1.0;    // 5
    skybox[12] = 0.0f;          skybox[13] = -1.0f;         // 6
    skybox[14] = -1.0;          skybox[15] = -1.0;    // 7*/
}

void                polymer_getsky(void)
{
    int             i;

    i = 0;
    while (i < numsectors)
    {
        if (sector[i].ceilingstat & 1)
        {
            cursky = sector[i].ceilingpicnum;
            return;
        }
        i++;
    }
}

void                polymer_drawskyquad(int p1, int p2, GLfloat height)
{
    bglBegin(GL_QUADS);
    bglTexCoord2f(0.0f, 0.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p1 * 2) + 1], height, skybox[p1 * 2]);
    bglVertex3f(skybox[(p1 * 2) + 1], height, skybox[p1 * 2]);
    bglTexCoord2f(0.0f, 1.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p1 * 2) + 1], -height, skybox[p1 * 2]);
    bglVertex3f(skybox[(p1 * 2) + 1], -height, skybox[p1 * 2]);
    bglTexCoord2f(1.0f, 1.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p2 * 2) + 1], -height, skybox[p2 * 2]);
    bglVertex3f(skybox[(p2 * 2) + 1], -height, skybox[p2 * 2]);
    bglTexCoord2f(1.0f, 0.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p2 * 2) + 1], height, skybox[p2 * 2]);
    bglVertex3f(skybox[(p2 * 2) + 1], height, skybox[p2 * 2]);
    bglEnd();
}

void                polymer_drawartsky(short tilenum)
{
    pthtyp*         pth;
    GLuint          glpics[5];
    int             i, j;
    GLfloat         height = 2.45f / 2.0f;

    i = 0;
    while (i < 5)
    {
        if (!waloff[tilenum + i])
            loadtile(tilenum + i);
        pth = gltexcache(tilenum + i, 0, 0);
        glpics[i] = pth ? pth->glpic : 0;
        i++;
    }

    i = 0;
    j = (1<<pskybits);
    while (i < j)
    {
        bglBindTexture(GL_TEXTURE_2D, glpics[pskyoff[i]]);
        polymer_drawskyquad(i, (i + 1) & (j - 1), height);
        i++;
    }
}
#endif
