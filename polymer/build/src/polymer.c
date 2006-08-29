// blah

#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "osd.h"

typedef struct  s_prvertex {
    float       x;
    float       y;
    float       z;
    short       wallnum;
}               _prvertex;

typedef struct  s_prtriangle {
    int         index[3];
}               _prtriangle;

typedef struct      s_prsector {
    _prvertex*      v;
    _prtriangle*    tris;
}               _prsector;

_prsector*      prsectors[MAXSECTORS];

void    polymer_init(void)
{
    int i;

    OSD_Printf("Initalizing Polymer subsystem.\n");

    i = 0;
    while (i < MAXSECTORS)
    {
        prsectors[i] = NULL;
        i++;
    }
}

void    polymer_glinit(void)
{
    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bglViewport(0, 0, 1024, 768);
    bglDisable(GL_TEXTURE_2D);
    bglDisable(GL_FOG);
    bglEnable(GL_DEPTH_TEST);
    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bglFrustum(-1.0f, 1.0f, -0.75f, 0.75, 1.0f, 10000.0f);
    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();
}

void            polymer_buildsector(short sectnum)
{
    _prsector*  s;
    sectortype  *sec, *nextsec;
    walltype    *wal, *wal2;
    int         i;

    OSD_Printf("Polymer: building sector %i\n", sectnum);

    s = malloc(sizeof(_prsector));
    memset(s, 0, sizeof(_prsector));

    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];

    s->v = malloc(sizeof(_prvertex) * sec->wallnum);

    i = 0;
    while (i < sec->wallnum)
    {
        s->v[i].wallnum = sec->wallptr + i;
        s->v[i].z = -wal->x;
        s->v[i].x = wal->y;
        s->v[i].y = -sec->floorz;

        i++;
        wal = &wall[sec->wallptr + i];
    }

    prsectors[sectnum] = s;
}

void            polymer_drawsector(long daposx, long daposy, long daposz, short daang, long dahoriz, short sectnum)
{
    sectortype  *sec;
    int         i;
    float       ang;
    _prvertex   v, pos;

    OSD_Printf("Polymer: drawing sector %i with angle %i\n", sectnum, daang);

    //if (prsectors[sectnum] == NULL)
        polymer_buildsector(sectnum);

    ang = (float)(daang) / (2048.0f / 360.0f);

    sec = &sector[sectnum];

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

    bglBegin(GL_LINE_LOOP);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    pos.x = daposy;
    pos.y = -daposz;
    pos.z = -daposx;
    i = 0;
    while (i < sec->wallnum)
    {
        v.x = (float)(prsectors[sectnum]->v[i].x - pos.x) / 1;
        v.y = (float)(prsectors[sectnum]->v[i].y - pos.y) / (15.66 * 1);
        v.z = (float)(prsectors[sectnum]->v[i].z - pos.z) / 1;
        //OSD_Printf("Polymer: drawing poly %f, %f, %f\n", v.x, v.y, v.z);
        bglVertex3d(v.x, v.y, v.z);
        i++;
    }
    bglEnd();
    bglBegin(GL_LINE_LOOP);
    i = 0;
    while (i < sec->wallnum)
    {
        v.x = (float)(prsectors[sectnum]->v[i].x - pos.x) / 1;
        v.y = (float)(prsectors[sectnum]->v[i].y - pos.y - (sec->ceilingz - sec->floorz)) / (15.66 * 1);
        v.z = (float)(prsectors[sectnum]->v[i].z - pos.z) / 1;
        //OSD_Printf("Polymer: drawing poly %f, %f, %f\n", v.x, v.y, v.z);
        bglVertex3d(v.x, v.y, v.z);
        i++;
    }
    bglEnd();
}

void    polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum)
{
    int i;
    OSD_Printf("Polymer: drawing rooms\n");

    polymer_glinit();
    i = 0;
    while (i < numsectors)
    {
        polymer_drawsector(daposx, daposy, daposz, daang, dahoriz, i);
        i++;
    }
}
