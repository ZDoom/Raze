// blah
#ifdef POLYMOST

#define POLYMER_C
#include "polymer.h"

// CVARS
int             pr_occlusionculling = 1;
int             pr_fov = 426;           // appears to be the classic setting.
int             pr_showportals = 0;
int             pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int             pr_wireframe = 0;

int             glerror;

// DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];

GLfloat         skybox[16];

GLfloat         vertsprite[4 * 5] = {
    -0.5f, 0.0f, 0.0f,
     0.0f, 1.0f,
     0.5f, 0.0f, 0.0f,
     1.0f, 1.0f,
     0.5f, 1.0f, 0.0f,
     1.0f, 0.0f,
    -0.5f, 1.0f, 0.0f,
     0.0f, 0.0f,
};

GLfloat         horizsprite[4 * 5] = {
    -0.5f, 0.0f, -0.03125f,
     0.0f, 1.0f,
     0.5f, 0.0f, -0.03125f,
     1.0f, 1.0f,
     0.5f, 0.0f, 0.03125f,
     1.0f, 0.0f,
    -0.5f, 0.0f, 0.03125f,
     0.0f, 0.0f,
};

// CONTROL
float           pos[3], spos[3];

float           frustum[5 * 4];

int             front;
int             back;
int             firstback;
short           sectorqueue[MAXSECTORS];
short           querydelay[MAXSECTORS];

GLdouble        modelviewmatrix[16];
GLdouble        spritemodelview[16];
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
    int             i, j;

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

    polymer_loadboard();

    polymer_initskybox();

    // init the face sprite modelview to identity
    i = 0;
    while (i < 4)
    {
        j = 0;
        while (j < 4)
        {
            if (i == j)
                spritemodelview[(i * 4) + j] = 1.0;
            else
                spritemodelview[(i * 4) + j] = 0.0;
            j++;
        }
        i++;
    }

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
    bgluPerspective((float)(pr_fov) / (2048.0f / 360.0f), (float)xdim / (float)ydim, 0.01f, 100.0f);

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

void                polymer_drawrooms(int daposx, int daposy, int daposz, short daang, int dahoriz, short dacursectnum)
{
    short           cursectnum;
    int             i, j;
    float           ang, horizang, tiltang;
    sectortype      *sec, *nextsec;
    walltype        *wal, *nextwal;
    spritetype      *spr;
    GLint           result;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    ang = (float)(daang) / (2048.0f / 360.0f);
    horizang = (float)(100 - dahoriz) / (512.0f / 180.0f);
    tiltang = (gtang * 90.0f);

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

    cursectnum = dacursectnum;
    updatesector(daposx, daposy, &cursectnum);

    if ((cursectnum >= 0) && (cursectnum < numsectors))
        dacursectnum = cursectnum;

    // external view (editor)
    if ((dacursectnum < 0) || (dacursectnum >= numsectors))
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
        prsectors[i]->controlstate = 0;
        prsectors[i]->wallsproffset = 0.0f;
        prsectors[i]->floorsproffset = 0.0f;
        i++;
    }
    i = 0;
    while (i < numwalls)
    {
        prwalls[i]->controlstate = 0;
        i++;
    }

    // GO
    front = 0;
    back = 0;

    polymer_pokesector(dacursectnum);
    polymer_drawsector(dacursectnum);
    prsectors[dacursectnum]->drawingstate = 1;

    sec = &sector[dacursectnum];
    wal = &wall[sec->wallptr];

    // scan sprites
    for (j = headspritesect[dacursectnum];j >=0;j = nextspritesect[j])
    {
        spr = &sprite[j];
        if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
               (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
               (spritesortcnt < MAXSPRITESONSCREEN))
        {
            copybufbyte(spr,&tsprite[spritesortcnt],sizeof(spritetype));
            tsprite[spritesortcnt++].owner = j;
        }
    }

    i = 0;
    while (i < sec->wallnum)
    {
        if ((wallvisible(sec->wallptr + i)) &&
            (polymer_portalinfrustum(sec->wallptr + i)))
        {
            polymer_drawwall(sec->wallptr + i);
            // mask
            if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = sec->wallptr + i;
            //thewall[maskwall[maskwallcnt++]] = sec->wallptr + i;

            if ((wal->nextsector != -1) &&
                (prsectors[wal->nextsector]->drawingstate == 0))
            {
                sectorqueue[back++] = wal->nextsector;
                prsectors[wal->nextsector]->drawingstate = 1;
            }
        }
        i++;
        wal = &wall[sec->wallptr + i];
    }

    firstback = back;

    while (front != back)
    {
        if ((front >= firstback) && (pr_occlusionculling) && (!querydelay[sectorqueue[front] + 1]))
        {
            bglGetQueryObjectivARB(sectorqueue[front] + 1,
                                   GL_QUERY_RESULT_ARB,
                                   &result);
            if (!result)
            {
                front++;
                continue;
            }
            else
                querydelay[sectorqueue[front] + 1] = pr_occlusionculling-1;
        }
        else if ((front >= firstback) && (pr_occlusionculling) && (querydelay[sectorqueue[front] + 1]))
            querydelay[sectorqueue[front] + 1]--;

        polymer_pokesector(sectorqueue[front]);
        polymer_drawsector(sectorqueue[front]);

        // scan sectors
        sec = &sector[sectorqueue[front]];
        wal = &wall[sec->wallptr];

        i = 0;
        while (i < sec->wallnum)
        {
            if ((wallvisible(sec->wallptr + i)) &&
                (polymer_portalinfrustum(sec->wallptr + i)))
            {
                polymer_drawwall(sec->wallptr + i);
                 // mask
                if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = sec->wallptr + i;
                //thewall[maskwall[maskwallcnt++]] = sec->wallptr + i;

                if ((wal->nextsector != -1) &&
                    (prsectors[wal->nextsector]->drawingstate == 0))
                {
                    polymer_pokesector(wal->nextsector);
                    sectorqueue[back++] = wal->nextsector;
                    prsectors[wal->nextsector]->drawingstate = 1;

                    if (pr_occlusionculling && !querydelay[wal->nextsector + 1])
                    {
                        nextsec = &sector[wal->nextsector];
                        nextwal = &wall[nextsec->wallptr];

                        bglDisable(GL_TEXTURE_2D);
                        bglDisable(GL_FOG);
                        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                        bglDepthMask(GL_FALSE);

                        bglBeginQueryARB(GL_SAMPLES_PASSED_ARB, wal->nextsector + 1);

                        j = 0;
                        while (j < nextsec->wallnum)
                        {
                            if ((nextwal->nextwall == (sec->wallptr + i)) ||
                               ((nextwal->nextwall != -1) &&
                                (wallvisible(nextwal->nextwall)) &&
                                (polymer_portalinfrustum(nextwal->nextwall))))
                            {
                                bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), prwalls[nextwal->nextwall]->portal);
                                bglDrawArrays(GL_QUADS, 0, 4);
                            }

                            j++;
                            nextwal = &wall[nextsec->wallptr + j];
                        }
                        bglEndQueryARB(GL_SAMPLES_PASSED_ARB);

                        bglDepthMask(GL_TRUE);
                        bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                        bglEnable(GL_FOG);
                        bglEnable(GL_TEXTURE_2D);
                    }
                }
            }
            i++;
            wal = &wall[sec->wallptr + i];
        }

        // scan sprites
        for (j = headspritesect[sectorqueue[front]];j >=0;j = nextspritesect[j])
        {
            spr = &sprite[j];
            if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                   (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                   (spritesortcnt < MAXSPRITESONSCREEN))
            {
                copybufbyte(spr,&tsprite[spritesortcnt],sizeof(spritetype));
                tsprite[spritesortcnt++].owner = j;
            }
        }

        front++;
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
}

void                polymer_drawmasks(void)
{
    bglEnable(GL_ALPHA_TEST);
    bglEnable(GL_BLEND);
    bglEnable(GL_POLYGON_OFFSET_FILL);

    while (spritesortcnt)
    {
        polymer_drawsprite(--spritesortcnt);
    }

    bglDisable(GL_POLYGON_OFFSET_FILL);
    bglDisable(GL_BLEND);
    bglDisable(GL_ALPHA_TEST);
}

void                polymer_rotatesprite(int sx, int sy, int z, short a, short picnum, signed char dashade, char dapalnum, char dastat, int cx1, int cy1, int cx2, int cy2)
{
    UNREFERENCED_PARAMETER(sx);
    UNREFERENCED_PARAMETER(sy);
    UNREFERENCED_PARAMETER(z);
    UNREFERENCED_PARAMETER(a);
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(dashade);
    UNREFERENCED_PARAMETER(dapalnum);
    UNREFERENCED_PARAMETER(dastat);
    UNREFERENCED_PARAMETER(cx1);
    UNREFERENCED_PARAMETER(cy1);
    UNREFERENCED_PARAMETER(cx2);
    UNREFERENCED_PARAMETER(cy2);
}

void                polymer_drawmaskwall(int damaskwallcnt)
{
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);

    w = prwalls[maskwall[damaskwallcnt]];

    bglBindTexture(GL_TEXTURE_2D, w->maskglpic);

    bglColor4f(w->maskcolor[0], w->maskcolor[1], w->maskcolor[2], w->maskcolor[3]);

    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->portal);
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->portal[3]);
    bglDrawArrays(GL_QUADS, 0, 4);
}

void                polymer_drawsprite(int snum)
{
    int             curpicnum, glpic, xsize, ysize;
    spritetype      *tspr;
    pthtyp*         pth;
    float           color[4], xratio, ang, *curspritedata;

    if (pr_verbosity >= 3) OSD_Printf("PR : Sprite %i...\n", snum);

    tspr = tspriteptr[snum];

    curpicnum = tspr->picnum;
    if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,tspr->owner+32768);

    if (!waloff[curpicnum])
        loadtile(curpicnum);

    pth = gltexcache(curpicnum, tspr->pal, 0);

    color[0] = color[1] = color[2] = ((float)(numpalookups-min(max(tspr->shade,0),numpalookups)))/((float)numpalookups);

    if (pth && (pth->flags & 2) && (pth->palnum != tspr->pal))
    {
        color[0] *= (float)hictinting[tspr->pal].r / 255.0;
        color[1] *= (float)hictinting[tspr->pal].g / 255.0;
        color[2] *= (float)hictinting[tspr->pal].b / 255.0;
    }

    if (tspr->cstat & 2)
    {
        if (tspr->cstat & 512)
            color[3] = 0.33f;
        else
            color[3] = 0.66f;
    }
    else
        color[3] = 1.0f;

    glpic = (pth) ? pth->glpic : 0;

    if (((tspr->cstat>>4) & 3) == 0)
    {
        xratio = (float)(tspr->xrepeat) / 160.0f;
        xsize = tilesizx[curpicnum] * 32 * xratio;
    }
    else
        xsize = tspr->xrepeat * tilesizx[curpicnum] / 4;

    ysize = tspr->yrepeat * tilesizy[curpicnum] * 4;

    spos[0] = tspr->y;
    spos[1] = -tspr->z;
    spos[2] = -tspr->x;

    curspritedata = vertsprite;

    switch ((tspr->cstat>>4) & 3)
    {
        case 0:
            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();

            spritemodelview[12] =   modelviewmatrix[0] * spos[0] +
                                    modelviewmatrix[4] * spos[1] +
                                    modelviewmatrix[8] * spos[2] +
                                    modelviewmatrix[12];
            spritemodelview[13] =   modelviewmatrix[1] * spos[0] +
                                    modelviewmatrix[5] * spos[1] +
                                    modelviewmatrix[9] * spos[2] +
                                    modelviewmatrix[13];
            spritemodelview[14] =   modelviewmatrix[2]  * spos[0] +
                                    modelviewmatrix[6]  * spos[1] +
                                    modelviewmatrix[10] * spos[2] +
                                    modelviewmatrix[14];

            bglLoadMatrixd(spritemodelview);
            bglRotatef((gtang * 90.0f), 0.0f, 0.0f, -1.0f);
            bglScalef((float)(xsize) / 1000.0f, (float)(ysize) / 16000.0f, 1.0f / 1000.0f);

            bglPolygonOffset(0.0f, 0.0f);
            break;
        case 1:
            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();

            ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

            bglTranslatef(spos[0], spos[1], spos[2]);
            bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
            bglScalef((float)(xsize), (float)(ysize), 1.0f);

            prsectors[tspr->sectnum]->wallsproffset += 0.5f;
            bglPolygonOffset(-prsectors[tspr->sectnum]->wallsproffset,
                             -prsectors[tspr->sectnum]->wallsproffset);
            break;
        case 2:
            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();

            ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

            bglTranslatef(spos[0], spos[1], spos[2]);
            bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
            bglScalef((float)(xsize), 1.0f, (float)(ysize));

            curspritedata = horizsprite;

            prsectors[tspr->sectnum]->floorsproffset += 0.5f;
            bglPolygonOffset(-prsectors[tspr->sectnum]->floorsproffset,
                             -prsectors[tspr->sectnum]->floorsproffset);
            break;
    }

    bglMatrixMode(GL_TEXTURE);
    bglLoadIdentity();

    if ((tspr->cstat & 4) || (((tspr->cstat>>4) & 3) == 2))
        bglScalef(-1.0f, 1.0f, 1.0f);

    if (tspr->cstat & 8)
        bglScalef(1.0f, -1.0f, 1.0f);

    bglBindTexture(GL_TEXTURE_2D, glpic);
    bglColor4f(color[0], color[1], color[2], color[3]);

    if (tspr->cstat & 64)
        bglEnable(GL_CULL_FACE);

    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), curspritedata);
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &curspritedata[3]);
    bglDrawArrays(GL_QUADS, 0, 4);

    if (tspr->cstat & 64)
        bglDisable(GL_CULL_FACE);

    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);
    bglPopMatrix();
}

// SECTORS
static int          polymer_initsector(short sectnum)
{
    sectortype      *sec;
    _prsector*      s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = calloc(1, sizeof(_prsector));
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

    s->controlstate = 2; // let updatesector know that everything needs to go

    prsectors[sectnum] = s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized sector %i.\n", sectnum);

    return (1);
}

static int          polymer_updatesector(short sectnum)
{
    _prsector*      s;
    sectortype      *sec;
    walltype        *wal;
    int             i, j;
    int             ceilz, florz;
    int             tex, tey;
    float           secangcos, secangsin, scalecoef;
    int             ang, needfloor, wallinvalidate;
    short           curstat, curpicnum, floorpicnum, ceilingpicnum;
    char            curxpanning, curypanning;
    GLfloat*        curbuffer;
    GLuint          *curglpic, *curfbglpic;
    pthtyp*         pth;

    s = prsectors[sectnum];
    sec = &sector[sectnum];

    secangcos = secangsin = 2;

    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Can't update uninitialized sector %i.\n", sectnum);
        return (-1);
    }

    needfloor = wallinvalidate = 0;

    // geometry
    wal = &wall[sec->wallptr];
    i = 0;
    while (i < sec->wallnum)
    {
        if ((-wal->x != s->verts[(i*3)+2]))
        {
            s->verts[(i*3)+2] = s->floorbuffer[(i*5)+2] = s->ceilbuffer[(i*5)+2] = -wal->x;
            needfloor = wallinvalidate = 1;
        }
        if ((wal->y != s->verts[i*3]))
        {
            s->verts[i*3] = s->floorbuffer[i*5] = s->ceilbuffer[i*5] = wal->y;
            needfloor = wallinvalidate = 1;
        }

        i++;
        wal = &wall[sec->wallptr + i];
    }

    if ((s->controlstate == 2) ||
        (sec->floorz != s->floorz) ||
        (sec->ceilingz != s->ceilingz) ||
        (sec->floorheinum != s->floorheinum) ||
        (sec->ceilingheinum != s->ceilingheinum))
    {
        wallinvalidate = 1;

        wal = &wall[sec->wallptr];
        i = 0;
        while (i < sec->wallnum)
        {
            getzsofslope(sectnum, wal->x, wal->y, &ceilz, &florz);
            s->floorbuffer[(i*5)+1] = -florz;
            s->ceilbuffer[(i*5)+1] = -ceilz;

            i++;
            wal = &wall[sec->wallptr + i];
        }

        s->floorz = sec->floorz;
        s->ceilingz = sec->ceilingz;
        s->floorheinum = sec->floorheinum;
        s->ceilingheinum = sec->ceilingheinum;
    }

    floorpicnum = sec->floorpicnum;
    if (picanm[floorpicnum]&192) floorpicnum += animateoffs(floorpicnum,sectnum);
    ceilingpicnum = sec->ceilingpicnum;
    if (picanm[ceilingpicnum]&192) ceilingpicnum += animateoffs(ceilingpicnum,sectnum);

    if ((s->controlstate != 2) && (!needfloor) &&
        (sec->floorstat == s->floorstat) &&
        (sec->ceilingstat == s->ceilingstat) &&
        (floorpicnum == s->floorpicnum) &&
        (ceilingpicnum == s->ceilingpicnum) &&
        (sec->floorxpanning == s->floorxpanning) &&
        (sec->ceilingxpanning == s->ceilingxpanning) &&
        (sec->floorypanning == s->floorypanning) &&
        (sec->ceilingypanning == s->ceilingypanning))
        goto attributes;

    wal = &wall[sec->wallptr];
    i = 0;
    while (i < sec->wallnum)
    {
        j = 2;
        curstat = sec->floorstat;
        curbuffer = s->floorbuffer;
        curpicnum = floorpicnum;
        curxpanning = sec->floorxpanning;
        curypanning = sec->floorypanning;

        while (j)
        {
            if (j == 1)
            {
                curstat = sec->ceilingstat;
                curbuffer = s->ceilbuffer;
                curpicnum = ceilingpicnum;
                curxpanning = sec->ceilingxpanning;
                curypanning = sec->ceilingypanning;
            }

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            if (((sec->floorstat & 64) || (sec->ceilingstat & 64)) &&
                ((secangcos == 2) && (secangsin == 2)))
            {
                ang = (getangle(wall[wal->point2].x - wal->x, wall[wal->point2].y - wal->y) + 512) & 2047;
                secangcos = (float)(sintable[(ang+512)&2047]) / 16383.0f;
                secangsin = (float)(sintable[ang&2047]) / 16383.0f;
            }


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
        i++;
        wal = &wall[sec->wallptr + i];
    }

    s->floorstat = sec->floorstat;
    s->ceilingstat = sec->ceilingstat;
    s->floorxpanning = sec->floorxpanning;
    s->ceilingxpanning = sec->ceilingxpanning;
    s->floorypanning = sec->floorypanning;
    s->ceilingypanning = sec->ceilingypanning;

attributes:

    if ((s->controlstate != 2) &&
        (sec->floorshade == s->floorshade) &&
        (sec->floorpal == s->floorpal) &&
        (floorpicnum == s->floorpicnum) &&
        (ceilingpicnum == s->ceilingpicnum))
        goto finish;

    i = 0;
    while (i < sec->wallnum)
    {
        //attributes
        j = 2;
        curbuffer = s->floorcolor;
        curstat = sec->floorshade;
        curxpanning = sec->floorpal;
        curpicnum = floorpicnum;
        curglpic = &s->floorglpic;
        curfbglpic = &s->floorfbglpic;

        while (j > 0)
        {
            if (j == 1)
            {
                curbuffer = s->ceilcolor;
                curstat = sec->ceilingshade;
                curxpanning = sec->ceilingpal;
                curpicnum = ceilingpicnum;
                curglpic = &s->ceilglpic;
                curfbglpic = &s->ceilfbglpic;
            }

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
    }

    s->floorshade = sec->floorshade;
    s->floorpal = sec->floorpal;
    s->floorpicnum = floorpicnum;
    s->ceilingpicnum = ceilingpicnum;

finish:

    if (needfloor)
        polymer_buildfloor(sectnum);

    if (wallinvalidate)
        s->invalidid++;

    s->controlstate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated sector %i.\n", sectnum);

    return (0);
}

void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out)
{
    // This callback is called by the tesselator when it detects an intersection between contours (HELLO ROTATING SPOTLIGHT IN E1L1).
    GLdouble*       ptr;

    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(weight);

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
    UNREFERENCED_PARAMETER(error);
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

static int          polymer_buildfloor(short sectnum)
{
    // This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
    _prsector*      s;
    sectortype      *sec;
    int             i;

    if (pr_verbosity >= 0) OSD_Printf("PR : Tesselating floor of sector %i...\n", sectnum);

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

static void         polymer_drawsector(short sectnum)
{
    sectortype      *sec;
    walltype        *wal;
    _prsector*      s;

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
    if (sec->floorstat & 1)
        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

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

    if (sec->floorstat & 1)
        bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


    // ceiling
    if (sec->ceilingstat & 1)
        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

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

    if (sec->ceilingstat & 1)
        bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

// WALLS
static int          polymer_initwall(short wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing wall %i...\n", wallnum);

    w = calloc(1, sizeof(_prwall));
    if (w == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize wall %i : malloc failed.\n", wallnum);
        return (0);
    }

    if (w->portal == NULL)
        w->portal = calloc(4, sizeof(GLfloat) * 5);
    if (w->bigportal == NULL)
        w->bigportal = calloc(4, sizeof(GLfloat) * 3);

    w->controlstate = 2;

    prwalls[wallnum] = w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized wall %i.\n", wallnum);

    return (1);
}

static void         polymer_updatewall(short wallnum)
{
    short           nwallnum, nnwallnum, curpicnum, wallpicnum, walloverpicnum, nwallpicnum;
    char            curxpanning, curypanning, underwall, overwall;
    walltype        *wal;
    sectortype      *sec, *nsec;
    _prwall         *w;
    _prsector       *s, *ns;
    pthtyp*         pth;
    int             xref, yref;
    float           ypancoef, dist;
    int             i;
    unsigned int    invalid;

    // yes, this function is messy and unefficient
    // it also works, bitches
    wal = &wall[wallnum];
    nwallnum = wal->nextwall;
    sec = &sector[sectorofwall(wallnum)];
    w = prwalls[wallnum];
    s = prsectors[sectorofwall(wallnum)];
    invalid = s->invalidid;
    if (nwallnum != -1)
    {
        ns = prsectors[wal->nextsector];
        invalid += ns->invalidid;
        nsec = &sector[wal->nextsector];
    }
    else
        ns = NULL;

    if (w->wallbuffer == NULL)
        w->wallbuffer = calloc(4, sizeof(GLfloat) * 5);

    wallpicnum = wal->picnum;
    if (picanm[wallpicnum]&192) wallpicnum += animateoffs(wallpicnum,wallnum+16384);
    walloverpicnum = wal->overpicnum;
    if (picanm[walloverpicnum]&192) walloverpicnum += animateoffs(walloverpicnum,wallnum+16384);
    if (nwallnum != -1)
    {
        nwallpicnum = wall[nwallnum].picnum;
        if (picanm[nwallpicnum]&192) nwallpicnum += animateoffs(nwallpicnum,wallnum+16384);
    }

    if ((w->controlstate != 2) &&
        (w->invalidid == invalid) &&
        (wal->cstat == w->cstat) &&
        (wallpicnum == w->picnum) &&
        (wal->pal == w->pal) &&
        (wal->xpanning == w->xpanning) &&
        (wal->ypanning == w->ypanning) &&
        (wal->xrepeat == w->xrepeat) &&
        (wal->yrepeat == w->yrepeat) &&
        (walloverpicnum == w->overpicnum) &&
        (wal->shade == w->shade) &&
        ((nwallnum == -1) ||
        ((nwallpicnum == w->nwallpicnum) &&
         (wall[nwallnum].xpanning == w->nwallxpanning) &&
         (wall[nwallnum].ypanning == w->nwallypanning) &&
         (wall[nwallnum].cstat == w->nwallcstat))))
    {
        w->controlstate = 1;
        return; // screw you guys I'm going home
    }
    else
    {
        w->invalidid = invalid;
        w->cstat = wal->cstat;
        w->picnum = wallpicnum;
        w->pal = wal->pal;
        w->xpanning = wal->xpanning;
        w->ypanning = wal->ypanning;
        w->xrepeat = wal->xrepeat;
        w->yrepeat = wal->yrepeat;
        w->overpicnum = walloverpicnum;
        w->shade = wal->shade;
        if (nwallnum != -1)
        {
            w->nwallpicnum = nwallpicnum;
            w->nwallxpanning = wall[nwallnum].xpanning;
            w->nwallypanning = wall[nwallnum].ypanning;
            w->nwallcstat = wall[nwallnum].cstat;
        }
    }

    w->underover = underwall = overwall = 0;

    w->wallcolor[0] = w->wallcolor[1] = w->wallcolor[2] = ((float)(numpalookups-min(max(wal->shade,0),numpalookups)))/((float)numpalookups);
    w->wallcolor[3] = 1.0f;

    if (wal->cstat & 8)
        xref = 1;
    else
        xref = 0;

    if ((wal->nextsector == -1) || (wal->cstat & 32))
    {
        memcpy(w->wallbuffer, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[5], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[10], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wallbuffer[15], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

        curpicnum = wallpicnum;

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

        if ((wal->cstat & 32) && (wal->nextsector != -1))
        {
            if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                yref = sec->ceilingz;
            else
                yref = nsec->floorz;
        }

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
        nnwallnum = wall[nwallnum].point2;

        if (((s->floorbuffer[((wallnum - sec->wallptr) * 5) + 1] != ns->floorbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
             (s->floorbuffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->floorbuffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
            ((s->floorbuffer[((wallnum - sec->wallptr) * 5) + 1] <= ns->floorbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
             (s->floorbuffer[((wal->point2 - sec->wallptr) * 5) + 1] <= ns->floorbuffer[((nwallnum - nsec->wallptr) * 5) + 1])))
            underwall = 1;

        if ((underwall) || (wal->cstat & 16))
        {
            memcpy(w->wallbuffer, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[5], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[10], &ns->floorbuffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wallbuffer[15], &ns->floorbuffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);

            if (wal->cstat & 2)
            {
                curpicnum = nwallpicnum;
                curxpanning = wall[nwallnum].xpanning;
                curypanning = wall[nwallnum].ypanning;
            }
            else
            {
                curpicnum = wallpicnum;
                curxpanning = wal->xpanning;
                curypanning = wal->ypanning;
            }

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

            if (underwall)
            {
                w->underover |= 1;
                if ((sec->floorstat & 1) && (nsec->floorstat & 1))
                    w->underover |= 4;
            }
            memcpy(w->portal, &w->wallbuffer[15], sizeof(GLfloat) * 5);
            memcpy(&w->portal[5], &w->wallbuffer[10], sizeof(GLfloat) * 5);
        }
        else
        {
            memcpy(w->portal, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            memcpy(&w->portal[5], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }

        if (((s->ceilbuffer[((wallnum - sec->wallptr) * 5) + 1] != ns->ceilbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
             (s->ceilbuffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->ceilbuffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
            ((s->ceilbuffer[((wallnum - sec->wallptr) * 5) + 1] >= ns->ceilbuffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
             (s->ceilbuffer[((wal->point2 - sec->wallptr) * 5) + 1] >= ns->ceilbuffer[((nwallnum - nsec->wallptr) * 5) + 1])))
            overwall = 1;

        if ((overwall) || (wal->cstat & 16))
        {
            if (w->overbuffer == NULL)
                w->overbuffer = calloc(4, sizeof(GLfloat) * 5);

            memcpy(w->overbuffer, &ns->ceilbuffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[5], &ns->ceilbuffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[10], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->overbuffer[15], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

            if ((wal->cstat & 16) || (wal->overpicnum == 0))
                curpicnum = wallpicnum;
            else
                curpicnum = wallpicnum;

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            if (wal->cstat & 16)
            {
                // mask glpic and color
                if (!waloff[wal->overpicnum])
                    loadtile(wal->overpicnum);

                pth = gltexcache(wal->overpicnum, wal->pal, 0);
                w->maskglpic = pth ? pth->glpic : 0;

                memcpy(w->maskcolor, w->wallcolor, sizeof(GLfloat) * 3);

                if (pth && (pth->flags & 2) && (pth->palnum != wal->pal))
                {
                    w->maskcolor[0] *= (float)hictinting[wal->pal].r / 255.0;
                    w->maskcolor[1] *= (float)hictinting[wal->pal].g / 255.0;
                    w->maskcolor[2] *= (float)hictinting[wal->pal].b / 255.0;
                }
                if (wal->cstat & 128)
                {
                    if (wal->cstat & 512)
                        w->maskcolor[3] = 0.33f;
                    else
                        w->maskcolor[3] = 0.66f;
                }
                else
                    w->maskcolor[3] = 1.0f;
            }

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

            if (overwall)
            {
                w->underover |= 2;
                if ((sec->ceilingstat & 1) && (nsec->ceilingstat & 1))
                    w->underover |= 8;
            }
            memcpy(&w->portal[10], &w->overbuffer[5], sizeof(GLfloat) * 5);
            memcpy(&w->portal[15], &w->overbuffer[0], sizeof(GLfloat) * 5);

            if (wal->cstat & 16)
            {
                // mask wall pass
                if (wal->cstat & 4)
                    yref = min(sec->floorz, nsec->floorz);
                else
                    yref = max(sec->ceilingz, nsec->ceilingz);

                curpicnum = wal->overpicnum;

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

                    w->portal[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
                    w->portal[(i * 5) + 4] = (-(float)(yref + w->portal[(i * 5) + 1]) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                    if (wal->cstat & 256) w->portal[(i * 5) + 4] = -w->portal[(i * 5) + 4];

                    i++;
                }
            }
        }
        else
        {
            memcpy(&w->portal[10], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            memcpy(&w->portal[15], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }
    }

    if ((wal->nextsector == -1) || (wal->cstat & 32))
        memcpy(w->portal, w->wallbuffer, sizeof(GLfloat) * 4 * 5);

    memcpy(w->bigportal, &s->floorbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[3], &s->floorbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[6], &s->ceilbuffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[9], &s->ceilbuffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

    w->controlstate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated wall %i.\n", wallnum);
}

static void         polymer_drawwall(short wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing wall %i...\n", wallnum);

    w = prwalls[wallnum];

    if (w->underover & 1)
    {
        if (w->underover & 4)
            bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        bglBindTexture(GL_TEXTURE_2D, w->wallglpic);
        bglColor4f(w->wallcolor[0], w->wallcolor[1], w->wallcolor[2], w->wallcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->wallbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->wallbuffer[3]);
        bglDrawArrays(GL_QUADS, 0, 4);

        if ((w->wallfbglpic) && !(w->underover & 4))
        {
            bglBindTexture(GL_TEXTURE_2D, w->wallfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawArrays(GL_QUADS, 0, 4);
        }

        if (w->underover & 4)
            bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    if (w->underover & 2)
    {
        if (w->underover & 8)
            bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        bglBindTexture(GL_TEXTURE_2D, w->overglpic);
        bglColor4f(w->overcolor[0], w->overcolor[1], w->overcolor[2], w->overcolor[3]);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), w->overbuffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &w->overbuffer[3]);
        bglDrawArrays(GL_QUADS, 0, 4);

        if ((w->overfbglpic) && !(w->underover & 4))
        {
            bglBindTexture(GL_TEXTURE_2D, w->overfbglpic);
            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            bglDrawArrays(GL_QUADS, 0, 4);
        }

        if (w->underover & 8)
            bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing wall %i...\n", wallnum);
}

// HSR
static void         polymer_pokesector(short sectnum)
{
    sectortype      *sec;
    _prsector       *s;
    walltype        *wal;
    int             i;

    sec = &sector[sectnum];
    s = prsectors[sectnum];
    wal = &wall[sec->wallptr];

    if (!s->controlstate)
        polymer_updatesector(sectnum);

    i = 0;
    while (i < sec->wallnum)
    {
        if ((wal->nextsector != -1) && (!prsectors[wal->nextsector]->controlstate))
            polymer_updatesector(wal->nextsector);
        if (!prwalls[sec->wallptr + i]->controlstate)
            polymer_updatewall(sec->wallptr + i);

        i++;
        wal = &wall[sec->wallptr + i];
    }
}

static void         polymer_extractfrustum(GLdouble* modelview, GLdouble* projection)
{
    GLdouble        matrix[16];
    int             i;

    bglMatrixMode(GL_TEXTURE);
    bglLoadMatrixd(projection);
    bglMultMatrixd(modelview);
    bglGetDoublev(GL_TEXTURE_MATRIX, matrix);
    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);

    i = 0;
    while (i < 4)
    {
        frustum[i] = matrix[(4 * i) + 3] + matrix[4 * i];               // left
        frustum[i + 4] = matrix[(4 * i) + 3] - matrix[4 * i];           // right
        frustum[i + 8] = matrix[(4 * i) + 3] - matrix[(4 * i) + 1];     // top
        frustum[i + 12] = matrix[(4 * i) + 3] + matrix[(4 * i) + 1];    // bottom
        frustum[i + 16] = matrix[(4 * i) + 3] + matrix[(4 * i) + 2];    // near
        i++;
    }
    i = 0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Frustum extracted.\n");
}

static int          polymer_portalinfrustum(short wallnum)
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
            sqdist = frustum[(i * 4) + 0] * w->bigportal[(j * 3) + 0] +
                     frustum[(i * 4) + 1] * w->bigportal[(j * 3) + 1] +
                     frustum[(i * 4) + 2] * w->bigportal[(j * 3) + 2] +
                     frustum[(i * 4) + 3];
            if (sqdist < 0)
                k++;
            j++;
        }
        if (k == 4)
            return (0); // OUT !
        i++;
    }

    return (1);
}

// SKIES
static void         polymer_initskybox(void)
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

static void         polymer_getsky(void)
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

static void         polymer_drawskyquad(int p1, int p2, GLfloat height)
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

static void         polymer_drawartsky(short tilenum)
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
