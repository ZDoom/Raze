// blah

#include "polymer.h"

_prsector*      prsectors[MAXSECTORS];

float           polymostprojmatrix[16];
float           polymostmodelmatrix[16];

// tesselation variables
GLUtesselator*  prtess;
int             tempverticescount;
GLdouble        tempvertice[3];

// Polymer cvars
char    pr_verbosity = 1; // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
char    pr_wireframe = 0;

int     polymer_init(void)
{
    int i;

    if (pr_verbosity >= 1) OSD_Printf("Initalizing Polymer subsystem...\n");

    i = 0;
    while (i < MAXSECTORS)
    {
        prsectors[i] = NULL;
        i++;
    }

    prtess = gluNewTess();
    if (prtess == 0)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Tesselator initialization failed.\n");
        return (0);
    }

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete.\n");
    return (1);
}

void            polymer_glinit(void)
{
    GLfloat     params[4];

    bglGetFloatv(GL_PROJECTION_MATRIX, polymostprojmatrix);
    bglGetFloatv(GL_MODELVIEW_MATRIX, polymostmodelmatrix);
    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bglViewport(0, 0, 1024, 768);

    // texturing
    bglEnable(GL_TEXTURE_2D);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    /*bglEnable(GL_TEXTURE_GEN_S);
    bglEnable(GL_TEXTURE_GEN_T);
    params[0] = GL_OBJECT_LINEAR;
    bglTexGenfv(GL_S, GL_TEXTURE_GEN_MODE, params);
    bglTexGenfv(GL_T, GL_TEXTURE_GEN_MODE, params);
    params[0] = 1.0 / 10000.0;
    params[1] = 1.0 / 10000.0;
    params[2] = 1.0 / 10000.0;
    params[3] = 1.0 / 10000.0;
    bglTexGenfv(GL_S, GL_OBJECT_PLANE, params);
    bglTexGenfv(GL_T, GL_OBJECT_PLANE, params);*/

    bglDisable(GL_FOG);
    bglEnable(GL_DEPTH_TEST);
    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bglFrustum(-1.0f, 1.0f, -0.75f, 0.75, 1.0f, 1000000.0f);
    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();
}

int             polymer_updategeometry(short sectnum)
{
    _prsector*  s;
    sectortype  *sec;
    walltype    *wal;
    int         i, j;
    long        ceilz, florz;
    long        tex, tey;
    float       secangcos, secangsin, scalecoef;
    long        ang;
    short       curstat;
    GLfloat*    curbuffer;

    s = prsectors[sectnum];
    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];

    secangcos = secangsin = 0;

    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Can't update uninitialized sector %i.\n", sectnum);
        return (-1);
    }

    if ((sec->floorstat & 64) || (sec->ceilingstat & 64))
    {
        ang = (getangle(wall[wal->point2].x - wal->x, wall[wal->point2].y - wal->y) + 512) & 2047;
        secangcos = (float)(sintable[(ang+512)&2047]) / 16383.0f;
        secangsin = (float)(sintable[ang&2047]) / 16383.0f;
    }

    i = 0;
    while (i < s->wallcount)
    {
        s->verts[(i*3)+2] = s->floorbuffer[(i*5)+2] = s->ceilbuffer[(i*5)+2] = -wal->x;
        s->verts[i*3] = s->floorbuffer[i*5] = s->ceilbuffer[i*5] = wal->y;
        getzsofslope(sectnum, wal->x, wal->y, &ceilz, &florz);
        s->verts[(i*3)+1] = 0;
        s->floorbuffer[(i*5)+1] = -florz;
        s->ceilbuffer[(i*5)+1] = -ceilz;

        j = 2;
        curstat = sec->floorstat;
        curbuffer = s->floorbuffer;

        while (j)
        {
            if (j == 1)
            {
                curstat = sec->ceilingstat;
                curbuffer = s->ceilbuffer;
            }

            tex = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangsin) + ((-wal->y - -wall[sec->wallptr].y) * secangcos) : wal->x;
            tey = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangcos) - ((-wal->y - -wall[sec->wallptr].y) * secangsin) : -wal->y;

            if (curstat & 4)
                swaplong(&tex, &tey);

            if (curstat & 16) tex = -tex;
            if (curstat & 32) tey = -tey;

            scalecoef = (curstat & 8) ? 8.0f : 16.0f;

            curbuffer[(i*5)+3] = ((float)(tex) / (scalecoef * tilesizx[sec->floorpicnum])) + ((float)(sec->floorxpanning) / 256.0f);
            curbuffer[(i*5)+4] = ((float)(tey) / (scalecoef * tilesizy[sec->floorpicnum])) + ((float)(sec->floorypanning) / 256.0f);

            j--;
        }

        i++;
        wal = &wall[sec->wallptr + i];
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated sector %i.\n", sectnum);

    return (0);
}

// This callback is called by the tesselator when it detects an intersection between contours (HELLO ROTATING SPOTLIGHT IN E1L1).
void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out)
{
    GLdouble*       ptr;

    //tempverticescount++;
    //tempvertices = realloc(tempvertices, tempverticescount * sizeof(GLdouble) * 3);
    tempvertice[0] = v[0];
    tempvertice[1] = v[1];
    tempvertice[2] = v[2];

    ptr = tempvertice;
    *out = tempvertice;

    if (pr_verbosity >= 2) OSD_Printf("PR : Created additional geometry for sector tesselation.\n");
}

// This callback is called by the tesselator whenever it raises an error.
void PR_CALLBACK    polymer_tesserror(GLenum error)
{
    if (pr_verbosity >= 1) OSD_Printf("PR : Tesselation error number %i reported : %s.\n", error, gluErrorString(errno));
}

// Passing an edgeflag callback forces the tesselator to output a triangle list
void PR_CALLBACK    polymer_tessedgeflag(GLboolean flag)
{
    flag = 0;
    return;
}

void PR_CALLBACK    polymer_tessbegin(GLenum type)
{
    if (type != GL_TRIANGLES)
    {
        OSD_Printf("PR : NOT A TRIANGLE LIST !§//... !\n");
    }
}

void PR_CALLBACK    polymer_tessend(void)
{
}
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector)
{
    _prsector*      s;

    s = (_prsector*)sector;

    if (s->curindice >= s->indicescount)
    {
        if (pr_verbosity >= 2) OSD_Printf("PR : Indice overflow, extending the indices list... !\n");
        s->indicescount++;
        s->indices = realloc(s->indices, s->indicescount * sizeof(GLushort));
    }
    s->indices[s->curindice] = (GLushort)vertex;
    s->curindice++;
}

// This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
int             polymer_buildfloor(short sectnum)
{
    _prsector*  s;
    sectortype  *sec;
    int         i;

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselating floor of sector %i...\n", sectnum);

    s = prsectors[sectnum];
    sec = &sector[sectnum];

    if (s == NULL)
        return (-1);

    if (s->indices != NULL)
        free(s->indices);

    s->indicescount = (s->wallcount - 2) * 3;
    s->indices = calloc(s->indicescount, sizeof(GLushort));
    s->curindice = 0;

    //gluTessCallback(prtess, GLU_TESS_BEGIN, polymer_tessbegin);
    //gluTessCallback(prtess, GLU_TESS_END, polymer_tessend);
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

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselated floor of sector %i.\n", sectnum);

    return (1);
}

int             polymer_initsector(short sectnum)
{
    sectortype  *sec;
    _prsector*  s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = malloc(sizeof(_prsector));
    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize sector %i : malloc failed.\n", sectnum);
        return (0);
    }

    s->invalidate = 0;
    s->wallcount = sec->wallnum;

    s->verts = calloc(s->wallcount, sizeof(GLdouble) * 3);
    s->floorbuffer = calloc(s->wallcount, sizeof(GLfloat) * 5);
    s->ceilbuffer = calloc(s->wallcount, sizeof(GLfloat) * 5);
    if ((s->verts == NULL) || (s->floorbuffer == NULL) || (s->ceilbuffer == NULL))
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize geometry of sector %i : malloc failed.\n", sectnum);
        return (0);
    }
   
    s->indices = NULL;

    prsectors[sectnum] = s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized sector %i.\n", sectnum);

    return (1);
}

void            polymer_drawsector(long daposx, long daposy, long daposz, short daang, long dahoriz, short sectnum)
{
    sectortype  *sec, *nextsec;
    walltype    *wal;
    _prsector*  s;
    float       ang;
    double      pos[3];
    int         i;
    long        zdiff;
    pthtyp*     pth;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing sector %i...\n", sectnum);

    if (prsectors[sectnum] == NULL)
    {
        polymer_initsector(sectnum);
        polymer_updategeometry(sectnum);
        polymer_buildfloor(sectnum);
    }
    else if (prsectors[sectnum]->invalidate)
    {
        if (pr_verbosity >= 2) OSD_Printf("PR : Sector %i invalidated. Tesselating...\n", sectnum);
        polymer_updategeometry(sectnum);
        polymer_buildfloor(sectnum);
        prsectors[sectnum]->invalidate = 0;
    }

    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];
    s = prsectors[sectnum];

    ang = (float)(daang) / (2048.0f / 360.0f);

    pos[0] = -daposy;
    pos[1] = daposz;
    pos[2] = daposx;

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);
    bglScalef(1.0f, 1.0f / 16.0f, 1.0f);
    bglTranslatef(pos[0], pos[1], pos[2]);

    // floor
    pth = gltexcache(sec->floorpicnum,sec->floorpal,0);
    bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), s->floorbuffer);
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &s->floorbuffer[3]);
    bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->indices);

    // ceiling
    pth = gltexcache(sec->ceilingpicnum,sec->ceilingpal,0);
    bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), s->ceilbuffer);
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &s->ceilbuffer[3]);
    bglDrawElements(GL_TRIANGLES, s->indicescount, GL_UNSIGNED_SHORT, s->indices);

    bglDisableClientState(GL_TEXTURE_COORD_ARRAY);
    bglDisableClientState(GL_VERTEX_ARRAY);

    /*// walls
    i = 0;
    while (i < sec->wallnum)
    {
        if (wal->nextsector == -1)
        { // limit of the map
            pth = gltexcache(wal->picnum,wal->pal,0);
            bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
            bglColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            bglBegin(GL_QUADS);
            bglVertex3d(s->verts[i].v[0], s->verts[i].v[1], s->verts[i].v[2]);
            bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1], s->verts[wal->point2 - sec->wallptr].v[2]);
            bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1] + (sec->floorz - sec->ceilingz), s->verts[wal->point2 - sec->wallptr].v[2]);
            bglVertex3d(s->verts[i].v[0], s->verts[i].v[1] + (sec->floorz - sec->ceilingz), s->verts[i].v[2]);
            bglEnd();
        }
        else
        {
            nextsec = &sector[wal->nextsector];
            zdiff = sec->floorz - nextsec->floorz;
            if (zdiff > 0)
            { // floor polymerization
                pth = gltexcache(wal->picnum,wal->pal,0);
                bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
                bglColor4f(0.0f, 0.0f, 1.0f, 1.0f);
                bglBegin(GL_QUADS);
                bglVertex3d(s->verts[i].v[0], s->verts[i].v[1], s->verts[i].v[2]);
                bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1], s->verts[wal->point2 - sec->wallptr].v[2]);
                bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1] + zdiff, s->verts[wal->point2 - sec->wallptr].v[2]);
                bglVertex3d(s->verts[i].v[0], s->verts[i].v[1] + zdiff, s->verts[i].v[2]);
                bglEnd();
            }
            zdiff = sec->ceilingz - nextsec->ceilingz;
            if (zdiff > 0)
            { // ceiling polymerization
                pth = gltexcache(wal->picnum,wal->pal,0);
                bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
                bglColor4f(1.0f, 0.0f, 1.0f, 1.0f);
                bglBegin(GL_QUADS);
                bglVertex3d(s->verts[i].v[0], s->verts[i].v[1] + (sec->floorz - sec->ceilingz), s->verts[i].v[2]);
                bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1] + (sec->floorz - sec->ceilingz), s->verts[wal->point2 - sec->wallptr].v[2]);
                bglVertex3d(s->verts[wal->point2 - sec->wallptr].v[0], s->verts[wal->point2 - sec->wallptr].v[1] + zdiff + (sec->floorz - sec->ceilingz), s->verts[wal->point2 - sec->wallptr].v[2]);
                bglVertex3d(s->verts[i].v[0], s->verts[i].v[1] + zdiff + (sec->floorz - sec->ceilingz), s->verts[i].v[2]);
                bglEnd();
            }
        }


        i++;
        wal = &wall[sec->wallptr + i];
    }*/

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

void    polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum)
{
    int i;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    polymer_glinit();

    i = 0;
    while (i < numsectors)
    {
        polymer_drawsector(daposx, daposy, daposz, daang, dahoriz, i);
        i++;
    }

    bglMatrixMode(GL_PROJECTION);
    bglLoadMatrixf(polymostprojmatrix);
    bglMatrixMode(GL_MODELVIEW);
    bglLoadMatrixf(polymostmodelmatrix);

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
}
