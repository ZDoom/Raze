
#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "mdsprite.h"


static md2head_t head;


static void quit(int32_t status)
{
    exit(status);
}

static md2model_t *md2load(int *fd, const char *filename, int32_t ronly)
{
    md2model_t *m;
    int fil;

    fil = Bopen(filename, ronly?BO_RDONLY:BO_RDWR);
    if (fil<0)
    {
        Bfprintf(stderr, "Couldn't open `%s': %s\n", filename, strerror(errno));
        quit(2);
    }

    m = (md2model_t *)Bcalloc(1,sizeof(md2model_t)); if (!m) quit(1);
    m->mdnum = 2; m->scale = .01f;

    Bread(fil,(char *)&head,sizeof(md2head_t));

    head.id = B_LITTLE32(head.id);                 head.vers = B_LITTLE32(head.vers);
    head.skinxsiz = B_LITTLE32(head.skinxsiz);     head.skinysiz = B_LITTLE32(head.skinysiz);
    head.framebytes = B_LITTLE32(head.framebytes); head.numskins = B_LITTLE32(head.numskins);
    head.numverts = B_LITTLE32(head.numverts);     head.numuv = B_LITTLE32(head.numuv);
    head.numtris = B_LITTLE32(head.numtris);       head.numglcmds = B_LITTLE32(head.numglcmds);
    head.numframes = B_LITTLE32(head.numframes);   head.ofsskins = B_LITTLE32(head.ofsskins);
    head.ofsuv = B_LITTLE32(head.ofsuv);           head.ofstris = B_LITTLE32(head.ofstris);
    head.ofsframes = B_LITTLE32(head.ofsframes);   head.ofsglcmds = B_LITTLE32(head.ofsglcmds);
    head.ofseof = B_LITTLE32(head.ofseof);

    if ((head.id != 0x32504449) || (head.vers != 8))
    {
        Bfprintf(stderr, "File `%s' is not an md2 file.\n", filename);
        quit(3);
    } //"IDP2"

    m->numskins = head.numskins;
    m->numframes = head.numframes;
    m->numverts = head.numverts;
    m->numglcmds = head.numglcmds;
    m->framebytes = head.framebytes;

    m->frames = (char *)Bmalloc(m->numframes*m->framebytes); if (!m->frames) quit(1);
    m->tris = (md2tri_t *)Bmalloc(head.numtris*sizeof(md2tri_t)); if (!m->tris) quit(1);
    m->uv = (md2uv_t *)Bmalloc(head.numuv*sizeof(md2uv_t)); if (!m->uv) quit(1);

    Blseek(fil,head.ofsframes,SEEK_SET);
    if (Bread(fil,(char *)m->frames,m->numframes*m->framebytes) != m->numframes*m->framebytes)
        quit(1);

    Blseek(fil,head.ofstris,SEEK_SET);
    if (Bread(fil,(char *)m->tris,head.numtris*sizeof(md2tri_t)) != (int32_t)(head.numtris*sizeof(md2tri_t)))
        quit(1);

    Blseek(fil,head.ofsuv,SEEK_SET);
    if (Bread(fil,(char *)m->uv,head.numuv*sizeof(md2uv_t)) != (int32_t)(head.numuv*sizeof(md2uv_t)))
        quit(1);

#if B_BIG_ENDIAN != 0
    {
        char *f = (char *)m->frames;
        int32_t *l,i,j;
        md2frame_t *fr;

        for (i = m->numframes-1; i>=0; i--)
        {
            fr = (md2frame_t *)f;
            l = (int32_t *)&fr->mul;
            for (j=5; j>=0; j--) l[j] = B_LITTLE32(l[j]);
            f += m->framebytes;
        }

        for (i = head.numtris-1; i>=0; i--)
        {
            m->tris[i].v[0] = B_LITTLE16(m->tris[i].v[0]);
            m->tris[i].v[1] = B_LITTLE16(m->tris[i].v[1]);
            m->tris[i].v[2] = B_LITTLE16(m->tris[i].v[2]);
            m->tris[i].u[0] = B_LITTLE16(m->tris[i].u[0]);
            m->tris[i].u[1] = B_LITTLE16(m->tris[i].u[1]);
            m->tris[i].u[2] = B_LITTLE16(m->tris[i].u[2]);
        }
        for (i = head.numuv-1; i>=0; i--)
        {
            m->uv[i].u = B_LITTLE16(m->uv[i].u);
            m->uv[i].v = B_LITTLE16(m->uv[i].v);
        }
    }
#endif

    *fd = fil;
    return(m);
}


static void usage_and_quit()
{
    Bfprintf(stderr,
            "Usage:\n"
            "   md2tool <modelfile>.md2:  display info about model\n"
            "   md2tool -minmax <minx>,<miny>,<minz>:<maxx>,<maxy>,<maxz> <modelfile>.md2:\n"
            "      modify `scale' and `translate' fields of MD2 (in-place) to produce given bounds\n"
        );
    quit(1);
}

int main(int argc, char **argv)
{
    char *fn=NULL, *cp;
    int32_t fd=-1, i, j;

    int32_t doinfo=1;

    // md2 mul[x,y,z], add[x,y,z]
    float mx,my,mz, ax,ay,az;

    // desired model bounds
    float dminx=0,dminy=0,dminz=0, dmaxx=1,dmaxy=1,dmaxz=1;

    // md2 uint8-coordinate bounds
    uint8_t maxv[3]={0,0,0};
    uint8_t minv[3]={255,255,255};

    md2frame_t *fr;
    uint8_t *vp;

    md2model_t *m;

    if (argc<=1)
        usage_and_quit();

    for (i=1; i<argc; i++)
    {
        cp = argv[i];

        if (cp[0]=='-')
        {
            if (!strcmp(cp, "-minmax"))
            {
                doinfo=0;
                if (i+1 >= argc)
                    usage_and_quit();
                if (Bsscanf(argv[i+1], "%f,%f,%f:%f,%f,%f", &dminx,&dminy,&dminz, &dmaxx,&dmaxy,&dmaxz)!=6)
                    usage_and_quit();
                i++;
            }
            else
            {
                Bfprintf(stderr, "unrecognized option `%s'\n", cp);
                quit(2);
            }
        }
        else
            fn = cp;
    }

    if (!fn)
        usage_and_quit();

    m = md2load(&fd, fn, doinfo);

    fr = (md2frame_t *)m->frames;
    mx=fr->mul.x; my=fr->mul.y; mz=fr->mul.z;
    ax=fr->add.x; ay=fr->add.y; az=fr->add.z;

    for (i=0, vp=fr->verts->v; i<m->numverts; i++, vp+=sizeof(md2vert_t))
    {
        for (j=0; j<3; j++)
        {
            maxv[j] = max(maxv[j], vp[j]);
            minv[j] = min(minv[j], vp[j]);
        }
    }

    if (doinfo)
    {
        Bprintf("------ %s ------\n", fn);
        Bprintf("numframes: %d\n", m->numframes);
        Bprintf("numverts: %d\n", m->numverts);
        Bprintf("numtris: %d\n", head.numtris);
        Bprintf("\n");
        Bprintf("ofsframes: %x\n", head.ofsframes);
        Bprintf("framebytes: %d\n", head.framebytes);
//        Bprintf("framebytes: %d, calculated=%d\n", head.framebytes, sizeof(md2frame_t)+(m->numverts-1)*sizeof(md2vert_t));
        Bprintf("\n");

        Bprintf("mul=%f %f %f\n", mx, my, mz);
        Bprintf("add=%f %f %f\n", ax, ay, az);

        Bprintf("min xyz (s+t) = %f %f %f\n", minv[0]*mx+ax, minv[1]*my+ay, minv[2]*mz+az);
        Bprintf("max xyz (s+t) = %f %f %f\n", maxv[0]*mx+ax, maxv[1]*my+ay, maxv[2]*mz+az);

        Bprintf("\n");
    }
    else
    {
        if (maxv[0]-minv[0]>0) mx = (dmaxx-dminx)/(maxv[0]-minv[0]); else mx=0;
        if (maxv[1]-minv[1]>0) my = (dmaxy-dminy)/(maxv[1]-minv[1]); else my=0;
        if (maxv[2]-minv[2]>0) mz = (dmaxz-dminz)/(maxv[2]-minv[2]); else mz=0;

        if (mx==0||my==0||mz==0)
        {
            Bfprintf(stderr, "max[x,y,z]-min[x,y,z] must each be grater 0!\n");
            quit(2);
        }

        ax = dmaxx-maxv[0]*mx;
        ay = dmaxy-maxv[1]*my;
        az = dmaxz-maxv[2]*mz;

#define ISNAN(x) ((x)!=(x))
#define ISINF(x) ((x!=0)&&(x/2==x))

        if (ISNAN(mx)||ISNAN(my)||ISNAN(mz)||ISNAN(ax)||ISNAN(ay)||ISNAN(az)||
            ISINF(mx)||ISINF(my)||ISINF(mz)||ISINF(ax)||ISINF(ay)||ISINF(az))
        {
            Bfprintf(stderr, "Calculation resulted in NaN or Inf.\n");
            quit(2);
        }

        Blseek(fd,head.ofsframes,SEEK_SET);
        if (Bwrite(fd, &mx, sizeof(mx))!=sizeof(mx)) { perror("write"); quit(3); }
        if (Bwrite(fd, &my, sizeof(my))!=sizeof(my)) { perror("write"); quit(3); }
        if (Bwrite(fd, &mz, sizeof(mz))!=sizeof(mz)) { perror("write"); quit(3); }
        if (Bwrite(fd, &ax, sizeof(ax))!=sizeof(ax)) { perror("write"); quit(3); }
        if (Bwrite(fd, &ay, sizeof(ay))!=sizeof(ay)) { perror("write"); quit(3); }
        if (Bwrite(fd, &az, sizeof(az))!=sizeof(az)) { perror("write"); quit(3); }
        Bclose(fd);

        Bprintf("wrote scale and translate of `%s'.\n", fn);
    }

    return 0;
}
