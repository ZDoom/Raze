//------------------------------------- MD2/MD3 LIBRARY BEGINS -------------------------------------

#ifdef USE_OPENGL

#include "build.h"
#include "mdsprite.h"
#include "coreactor.h"

#include "palette.h"
#include "textures.h"
#include "bitmap.h"
#include "v_video.h"
#include "flatvertices.h"
#include "texturemanager.h"
#include "hw_renderstate.h"
#include "printf.h"
#include "hw_voxels.h"

#ifdef _MSC_VER
// just make it shut up. Most of this file will go down the drain anyway soon.
#pragma warning(disable:4244) 
#pragma warning(disable:4267) 
#endif

static int32_t curextra=MAXTILES;

#define MIN_CACHETIME_PRINT 10

static int32_t addtileP(int32_t ,int32_t tile,int32_t pallet)
{
    // tile >= 0 && tile < MAXTILES

    if (curextra==MAXTILES+EXTRATILES-1)
    {
        Printf("warning: max EXTRATILES reached\n");
        return curextra;
    }

    if (tile2model[tile].modelid==-1)
    {
        tile2model[tile].pal=pallet;
        return tile;
    }

    if (tile2model[tile].pal==pallet)
        return tile;

    while (tile2model[tile].nexttile!=-1)
    {
        tile=tile2model[tile].nexttile;
        if (tile2model[tile].pal==pallet)
            return tile;
    }

    tile2model[tile].nexttile=curextra;
    tile2model[curextra].pal=pallet;

    return curextra++;
}

int32_t Ptile2tile(int32_t tile,int32_t palette)
{
    int t = tile;
    while ((tile = tile2model[tile].nexttile) != -1)
        if (tile2model[tile].pal == palette)
        {
            t = tile;
            break;
        }
    return t;
}

#define MODELALLOCGROUP 256
static int32_t nummodelsalloced = 0;

static int32_t maxmodelverts = 0, allocmodelverts = 0;
static int32_t maxmodeltris = 0, allocmodeltris = 0;
static FVector3 *vertlist = NULL; //temp array to store interpolated vertices for drawing

#ifdef USE_GLEXT
static int32_t allocvbos = 0, curvbo = 0;
static GLuint *vertvbos = NULL;
static GLuint *indexvbos = NULL;
#endif

#ifdef POLYMER
static int32_t *tribuf = NULL;
static int32_t tribufverts = 0;
#endif

static mdmodel_t *mdload(const char *);
static void mdfree(mdmodel_t *);
static int32_t globalnoeffect=0;

void freeallmodels()
{
    int32_t i;

    if (models)
    {
        for (i=0; i<nextmodelid; i++) mdfree(models[i]);
        M_Free(models);
        models = nullptr;
        nummodelsalloced = 0;
        nextmodelid = 0;
    }

    memset(tile2model,-1,sizeof(tile2model));

    curextra=MAXTILES;

    if (vertlist)
    {
        M_Free(vertlist);
        vertlist = nullptr;
        allocmodelverts = maxmodelverts = 0;
        allocmodeltris = maxmodeltris = 0;
    }
}

void mdinit()
{
    freeallmodels();
    mdinited = 1;
}

int32_t md_loadmodel(const char *fn)
{
    mdmodel_t *vm, **ml;

    if (!mdinited) mdinit();

    if (nextmodelid >= nummodelsalloced)
    {
        ml = (mdmodel_t **)M_Realloc(models,(nummodelsalloced+MODELALLOCGROUP)*sizeof(void *));
        models = ml; nummodelsalloced += MODELALLOCGROUP;
    }

    vm = mdload(fn); if (!vm) return -1;
    models[nextmodelid++] = vm;
    return nextmodelid-1;
}

int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, float yoffset, int32_t flags)
{
    mdmodel_t *m;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    m = models[modelid];
    m->bscale = scale;
    m->shadeoff = shadeoff;
    m->zadd = zadd;
    m->yoffset = yoffset;
    m->flags = flags;

    return 0;
}

static int32_t framename2index(mdmodel_t *vm, const char *nam)
{
    int32_t i = 0;

    switch (vm->mdnum)
    {
    case 2:
    {
        md2model_t *m = (md2model_t *)vm;
        md2frame_t *fr;
        for (i=0; i<m->numframes; i++)
        {
            fr = (md2frame_t *)&m->frames[i*m->framebytes];
            if (!strcmp(fr->name, nam)) break;
        }
    }
    break;
    case 3:
    {
        md3model_t *m = (md3model_t *)vm;
        for (i=0; i<m->numframes; i++)
            if (!strcmp(m->head.frames[i].nam,nam)) break;
    }
    break;
    }
    return i;
}

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume, int32_t skinnum, float smoothduration, int32_t pal)
{
    md2model_t *m;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if ((uint32_t)tilenume >= (uint32_t)MAXTILES) return -2;
    if (!framename) return -3;

    tilenume=addtileP(modelid,tilenume,pal);
    m = (md2model_t *)models[modelid];
    if (m->mdnum == 1)
    {
        tile2model[tilenume].modelid = modelid;
        tile2model[tilenume].framenum = tile2model[tilenume].skinnum = 0;
        return 0;
    }

    i = framename2index((mdmodel_t *)m,framename);
    if (i == m->numframes) return -3;   // frame name invalid

    tile2model[tilenume].modelid = modelid;
    tile2model[tilenume].framenum = i;
    tile2model[tilenume].skinnum = skinnum;
    tile2model[tilenume].smoothduration = xs_CRoundToInt((float)UINT16_MAX * smoothduration);

    return i;
}

int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend, int32_t fpssc, int32_t flags)
{
    md2model_t *m;
    mdanim_t ma, *map;
    int32_t i;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;

    memset(&ma, 0, sizeof(ma));
    m = (md2model_t *)models[modelid];
    if (m->mdnum < 2) return 0;

    //find index of start frame
    i = framename2index((mdmodel_t *)m,framestart);
    if (i == m->numframes) return -2;
    ma.startframe = i;

    //find index of finish frame which must trail start frame
    i = framename2index((mdmodel_t *)m,frameend);
    if (i == m->numframes) return -3;
    ma.endframe = i;

    ma.fpssc = fpssc;
    ma.flags = flags;

    map = (mdanim_t *)M_Malloc(sizeof(mdanim_t));

    memcpy(map, &ma, sizeof(ma));

    map->next = m->animations;
    m->animations = map;

    return 0;
}

int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum, int32_t surfnum, float param, float specpower, float specfactor, int32_t flags)
{
    mdskinmap_t *sk, *skl;
    md2model_t *m;

    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if (!skinfn) return -2;
    if ((unsigned)palnum >= (unsigned)MAXPALOOKUPS) return -3;

    m = (md2model_t *)models[modelid];
    if (m->mdnum < 2) return 0;
    if (m->mdnum == 2) surfnum = 0;

    skl = NULL;
    for (sk = m->skinmap; sk; skl = sk, sk = sk->next)
        if (sk->palette == (uint8_t)palnum && skinnum == sk->skinnum && surfnum == sk->surfnum)
            break;
    if (!sk)
    {
        sk = (mdskinmap_t *)M_Calloc(1,sizeof(mdskinmap_t));

        if (!skl) m->skinmap = sk;
        else skl->next = sk;
    }

    sk->palette = (uint8_t)palnum;
    sk->flags = (uint8_t)flags;
    sk->skinnum = skinnum;
    sk->surfnum = surfnum;
    sk->param = param;
    sk->specpower = specpower;
    sk->specfactor = specfactor;
	sk->texture = TexMan.CheckForTexture(skinfn, ETextureType::Any);
	if (!sk->texture.isValid())
	{
		Printf("Unable to load %s as model skin\n", skinfn);
	}

    return 0;
}

int32_t md_definehud(int32_t modelid, int32_t tilex, FVector3 add, int32_t angadd, int32_t flags, int32_t fov)
{
    if (!mdinited) mdinit();

    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;
    if ((uint32_t)tilex >= (uint32_t)MAXTILES) return -2;

    hudtyp * const hud = &tile2model[tilex].hudmem[(flags>>2)&1];

    hud->add = add;
    hud->angadd = ((int16_t)angadd)|2048;
    hud->flags = (int16_t)flags;
    hud->fov = (int16_t)fov;

    return 0;
}

int32_t md_undefinetile(int32_t tile)
{
    if (!mdinited) return 0;
    if ((unsigned)tile >= (unsigned)MAXTILES) return -1;

    tile2model[tile].modelid = -1;
    tile2model[tile].nexttile = -1;
    return 0;
}

/* this function is problematic, it leaves NULL holes in model[]
 * (which runs from 0 to nextmodelid-1) */
int32_t md_undefinemodel(int32_t modelid)
{
    int32_t i;
    if (!mdinited) return 0;
    if ((uint32_t)modelid >= (uint32_t)nextmodelid) return -1;

    for (i=MAXTILES+EXTRATILES-1; i>=0; i--)
        if (tile2model[i].modelid == modelid)
        {
            tile2model[i].modelid = -1;
        }

    if (models)
    {
        mdfree(models[modelid]);
        models[modelid] = NULL;
    }

    return 0;
}


//Note: even though it says md2model, it works for both md2model&md3model
FGameTexture *mdloadskin(idmodel_t *m, int32_t number, int32_t pal, int32_t surf, bool *exact)
{
    int32_t i;
    mdskinmap_t *sk, *skzero = NULL;

    if (m->mdnum == 2)
        surf = 0;

    if ((unsigned)pal >= (unsigned)MAXPALOOKUPS)
        return 0;

    i = -1;
    for (sk = m->skinmap; sk; sk = sk->next)
    {
        if (sk->palette == pal && sk->skinnum == number && sk->surfnum == surf)
        {
			if (exact) *exact = true;
            //Printf("Using exact match skin (pal=%d,skinnum=%d,surfnum=%d) %s\n",pal,number,surf,skinfile);
            return TexMan.GetGameTexture(sk->texture);
        }
        //If no match, give highest priority to number, then pal.. (Parkar's request, 02/27/2005)
        else if ((sk->palette ==   0) && (sk->skinnum == number) && (sk->surfnum == surf) && (i < 5)) { i = 5; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 4)) { i = 4; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum ==      0) && (sk->surfnum == surf) && (i < 3)) { i = 3; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum == number) && (i < 2)) { i = 2; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum ==      0) && (i < 1)) { i = 1; skzero = sk; }
        else if ((sk->palette ==   0) && (sk->skinnum ==      0) && (i < 0)) { i = 0; skzero = sk; }
    }

	// Special palettes do not get replacements
	if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
		return 0;

	if (skzero)
	{
		//Printf("Using def skin 0,0 as fallback, pal=%d\n", pal);
		if (exact) *exact = false;
        return TexMan.GetGameTexture(skzero->texture);
	}
	else
		return nullptr;
}


//--------------------------------------- MD2 LIBRARY BEGINS ---------------------------------------
static md2model_t *md2load(FileReader & fil, const char *filnam)
{
    md2model_t *m;
    md3model_t *m3;
    md3surf_t *s;
    md2frame_t *f;
    md2head_t head;
    char st[BMAX_PATH];
    int32_t i, j, k;

    int32_t ournumskins, ournumglcmds;

    m = (md2model_t *)M_Calloc(1,sizeof(md2model_t));
    m->mdnum = 2; m->scale = .01f;

    fil.Read((char *)&head,sizeof(md2head_t));
#if WORDS_BIGENDIAN
    head.id = LittleLong(head.id);                 head.vers = LittleLong(head.vers);
    head.skinxsiz = LittleLong(head.skinxsiz);     head.skinysiz = LittleLong(head.skinysiz);
    head.framebytes = LittleLong(head.framebytes); head.numskins = LittleLong(head.numskins);
    head.numverts = LittleLong(head.numverts);     head.numuv = LittleLong(head.numuv);
    head.numtris = LittleLong(head.numtris);       head.numglcmds = LittleLong(head.numglcmds);
    head.numframes = LittleLong(head.numframes);   head.ofsskins = LittleLong(head.ofsskins);
    head.ofsuv = LittleLong(head.ofsuv);           head.ofstris = LittleLong(head.ofstris);
    head.ofsframes = LittleLong(head.ofsframes);   head.ofsglcmds = LittleLong(head.ofsglcmds);
    head.ofseof = LittleLong(head.ofseof);
#endif

    if ((head.id != IDP2_MAGIC) || (head.vers != 8)) { M_Free(m); return 0; } //"IDP2"

    ournumskins = head.numskins ? head.numskins : 1;
    ournumglcmds = head.numglcmds ? head.numglcmds : 1;

    m->numskins = head.numskins;
    m->numframes = head.numframes;
    m->numverts = head.numverts;
    m->numglcmds = head.numglcmds;
    m->framebytes = head.framebytes;

    m->frames = (char *)M_Malloc(m->numframes*m->framebytes);
    m->glcmds = (int32_t *)M_Malloc(ournumglcmds*sizeof(int32_t));
    m->tris = (md2tri_t *)M_Malloc(head.numtris*sizeof(md2tri_t));
    m->uv = (md2uv_t *)M_Malloc(head.numuv*sizeof(md2uv_t));

    fil.Seek(head.ofsframes,FileReader::SeekSet);
    if (fil.Read((char *)m->frames,m->numframes*m->framebytes) != m->numframes*m->framebytes)
        { M_Free(m->uv); M_Free(m->tris); M_Free(m->glcmds); M_Free(m->frames); M_Free(m); return 0; }

    if (m->numglcmds > 0)
    {
        fil.Seek(head.ofsglcmds,FileReader::SeekSet);
        if (fil.Read((char *)m->glcmds,m->numglcmds*sizeof(int32_t)) != (int32_t)(m->numglcmds*sizeof(int32_t)))
            { M_Free(m->uv); M_Free(m->tris); M_Free(m->glcmds); M_Free(m->frames); M_Free(m); return 0; }
    }

    fil.Seek(head.ofstris,FileReader::SeekSet);
    if (fil.Read((char *)m->tris,head.numtris*sizeof(md2tri_t)) != (int32_t)(head.numtris*sizeof(md2tri_t)))
        { M_Free(m->uv); M_Free(m->tris); M_Free(m->glcmds); M_Free(m->frames); M_Free(m); return 0; }

    fil.Seek(head.ofsuv,FileReader::SeekSet);
    if (fil.Read((char *)m->uv,head.numuv*sizeof(md2uv_t)) != (int32_t)(head.numuv*sizeof(md2uv_t)))
        { M_Free(m->uv); M_Free(m->tris); M_Free(m->glcmds); M_Free(m->frames); M_Free(m); return 0; }

#if WORDS_BIGENDIAN
    {
        char *f = (char *)m->frames;
        int32_t *l,j;
        md2frame_t *fr;

        for (i = m->numframes-1; i>=0; i--)
        {
            fr = (md2frame_t *)f;
            l = (int32_t *)&fr->mul;
            for (j=5; j>=0; j--) l[j] = LittleLong(l[j]);
            f += m->framebytes;
        }

        for (i = m->numglcmds-1; i>=0; i--)
        {
            m->glcmds[i] = LittleLong(m->glcmds[i]);
        }
        for (i = head.numtris-1; i>=0; i--)
        {
            m->tris[i].v[0] = LittleShort(m->tris[i].v[0]);
            m->tris[i].v[1] = LittleShort(m->tris[i].v[1]);
            m->tris[i].v[2] = LittleShort(m->tris[i].v[2]);
            m->tris[i].u[0] = LittleShort(m->tris[i].u[0]);
            m->tris[i].u[1] = LittleShort(m->tris[i].u[1]);
            m->tris[i].u[2] = LittleShort(m->tris[i].u[2]);
        }
        for (i = head.numuv-1; i>=0; i--)
        {
            m->uv[i].u = LittleShort(m->uv[i].u);
            m->uv[i].v = LittleShort(m->uv[i].v);
        }
    }
#endif

    strcpy(st,filnam);
    for (i=strlen(st)-1; i>0; i--)
        if ((st[i] == '/') || (st[i] == '\\')) { i++; break; }
    if (i<0) i=0;
    st[i] = 0;
    m->basepath = (char *)M_Malloc(i+1);
    strcpy(m->basepath, st);

    m->skinfn = (char *)M_Malloc(ournumskins*64);
    if (m->numskins > 0)
    {
        fil.Seek(head.ofsskins,FileReader::SeekSet);
        if (fil.Read(m->skinfn,64*m->numskins) != 64*m->numskins)
            { M_Free(m->glcmds); M_Free(m->frames); M_Free(m); return 0; }
    }

    maxmodelverts = max(maxmodelverts, m->numverts);
    maxmodeltris = max(maxmodeltris, head.numtris);

    //return m;

    // the MD2 is now loaded internally - let's begin the MD3 conversion process
    //Printf("Beginning md3 conversion.\n");
    m3 = (md3model_t *)M_Calloc(1, sizeof(md3model_t));
	m3->mdnum = 3; m3->texture = nullptr; m3->scale = m->scale;
    m3->head.id = IDP3_MAGIC; m3->head.vers = 15;

    m3->head.flags = 0;

    m3->head.numframes = m->numframes;
    m3->head.numtags = 0; m3->head.numsurfs = 1;
    m3->head.numskins = 0;

    m3->numskins = m3->head.numskins;
    m3->numframes = m3->head.numframes;

    m3->head.frames = (md3frame_t *)M_Calloc(m3->head.numframes, sizeof(md3frame_t));
    m3->muladdframes = (FVector3 *)M_Calloc(m->numframes * 2, sizeof(FVector3));

    f = (md2frame_t *)(m->frames);

    // frames converting
    i = 0;
    while (i < m->numframes)
    {
        f = (md2frame_t *)&m->frames[i*m->framebytes];
        strcpy(m3->head.frames[i].nam, f->name);
        //Printf("Copied frame %s.\n", m3->head.frames[i].nam);
        m3->muladdframes[i*2] = f->mul;
        m3->muladdframes[i*2+1] = f->add;
        i++;
    }

    m3->head.tags = NULL;

    m3->head.surfs = (md3surf_t *)M_Calloc(1, sizeof(md3surf_t));
    s = m3->head.surfs;

    // model converting
    s->id = IDP3_MAGIC; s->flags = 0;
    s->numframes = m->numframes; s->numshaders = 0;
    s->numtris = head.numtris;
    s->numverts = head.numtris * 3; // oh man talk about memory effectiveness :((((
    // MD2 is actually more accurate than MD3 in term of uv-mapping, because each triangle has a triangle counterpart on the UV-map.
    // In MD3, each vertex unique UV coordinates, meaning that you have to duplicate vertices if you need non-seamless UV-mapping.

    maxmodelverts = max(maxmodelverts, s->numverts);

    strcpy(s->nam, "Dummy surface from MD2");

    s->shaders = NULL;

    s->tris = (md3tri_t *)M_Calloc(head.numtris, sizeof(md3tri_t));
    s->uv = (md3uv_t *)M_Calloc(s->numverts, sizeof(md3uv_t));
    s->xyzn = (md3xyzn_t *)M_Calloc(s->numverts * m->numframes, sizeof(md3xyzn_t));

    //memoryusage += (s->numverts * m->numframes * sizeof(md3xyzn_t));
    //Printf("Current model geometry memory usage : %i.\n", memoryusage);

    //Printf("Number of frames : %i\n", m->numframes);
    //Printf("Number of triangles : %i\n", head.numtris);
    //Printf("Number of vertices : %i\n", s->numverts);

    // triangle converting
    i = 0;
    while (i < head.numtris)
    {
        j = 0;
        //Printf("Triangle : %i\n", i);
        while (j < 3)
        {
            // triangle vertex indexes
            s->tris[i].i[j] = i*3 + j;

            // uv coords
            s->uv[i*3+j].u = (float)(m->uv[m->tris[i].u[j]].u) / (float)(head.skinxsiz);
            s->uv[i*3+j].v = (float)(m->uv[m->tris[i].u[j]].v) / (float)(head.skinysiz);

            // vertices for each frame
            k = 0;
            while (k < m->numframes)
            {
                f = (md2frame_t *)&m->frames[k*m->framebytes];
                s->xyzn[(k*s->numverts) + (i*3) + j].x = (int16_t) (((f->verts[m->tris[i].v[j]].v[0] * f->mul.X) + f->add.X) * 64.f);
                s->xyzn[(k*s->numverts) + (i*3) + j].y = (int16_t) (((f->verts[m->tris[i].v[j]].v[1] * f->mul.Y) + f->add.Y) * 64.f);
                s->xyzn[(k*s->numverts) + (i*3) + j].z = (int16_t) (((f->verts[m->tris[i].v[j]].v[2] * f->mul.Z) + f->add.Z) * 64.f);

                k++;
            }
            j++;
        }
        //Printf("End triangle.\n");
        i++;
    }
    //Printf("Finished md3 conversion.\n");

    {
        mdskinmap_t *sk;

        sk = (mdskinmap_t *)M_Calloc(1,sizeof(mdskinmap_t));
        sk->palette = 0;
        sk->skinnum = 0;
        sk->surfnum = 0;

        if (m->numskins > 0)
        {
			FStringf fn("%s%s", m->basepath, m->skinfn);
			sk->texture = TexMan.CheckForTexture(fn, ETextureType::Any);
			if (!sk->texture.isValid())
			{
				Printf("Unable to load %s as model skin\n", m->skinfn);
			}
        }
        m3->skinmap = sk;
    }

    m3->indexes = (uint16_t *)M_Malloc(sizeof(uint16_t) * s->numtris);
    m3->vindexes = (uint16_t *)M_Malloc(sizeof(uint16_t) * s->numtris * 3);
    m3->maxdepths = (float *)M_Malloc(sizeof(float) * s->numtris);

    // die MD2 ! DIE !
    M_Free(m->skinfn); M_Free(m->basepath); M_Free(m->uv); M_Free(m->tris); M_Free(m->glcmds); M_Free(m->frames); M_Free(m);

    return ((md2model_t *)m3);
}
//---------------------------------------- MD2 LIBRARY ENDS ----------------------------------------

// DICHOTOMIC RECURSIVE SORTING - USED BY MD3DRAW
static int32_t partition(uint16_t *indexes, float *depths, int32_t f, int32_t l)
{
    int32_t up = f, down = l;
    float piv = depths[f];
    uint16_t piv2 = indexes[f];
    do
    {
        while ((up < l) && (depths[up] <= piv))
            up++;
        while ((depths[down] > piv) && (down > f))
            down--;
        if (up < down)
        {
            std::swap(depths[up], depths[down]);
            std::swap(indexes[up], indexes[down]);
        }
    }
    while (down > up);
    depths[f] = depths[down];
    depths[down] = piv;
    indexes[f] = indexes[down];
    indexes[down] = piv2;
    return down;
}

static inline void quicksort(uint16_t *indexes, float *depths, int32_t first, int32_t last)
{
    int32_t pivIndex;
    if (first >= last) return;
    pivIndex = partition(indexes, depths, first, last);
    if (first < (pivIndex-1)) quicksort(indexes, depths, first, (pivIndex-1));
    if ((pivIndex+1) >= last) return;
    quicksort(indexes, depths, (pivIndex+1), last);
}
// END OF QUICKSORT LIB

//--------------------------------------- MD3 LIBRARY BEGINS ---------------------------------------

static md3model_t *md3load(FileReader & fil)
{
    int32_t i, surfi, ofsurf, offs[4], leng[4];
    int32_t maxtrispersurf;
    md3model_t *m;
    md3surf_t *s;

    m = (md3model_t *)M_Calloc(1,sizeof(md3model_t));
    m->mdnum = 3; m->texture = nullptr; m->scale = .01f;

    m->muladdframes = NULL;

    fil.Read(&m->head,SIZEOF_MD3HEAD_T);

#if WORDS_BIGENDIAN
    m->head.id = LittleLong(m->head.id);             m->head.vers = LittleLong(m->head.vers);
    m->head.flags = LittleLong(m->head.flags);       m->head.numframes = LittleLong(m->head.numframes);
    m->head.numtags = LittleLong(m->head.numtags);   m->head.numsurfs = LittleLong(m->head.numsurfs);
    m->head.numskins = LittleLong(m->head.numskins); m->head.ofsframes = LittleLong(m->head.ofsframes);
    m->head.ofstags = LittleLong(m->head.ofstags); m->head.ofssurfs = LittleLong(m->head.ofssurfs);
    m->head.eof = LittleLong(m->head.eof);
#endif

    if ((m->head.id != IDP3_MAGIC) && (m->head.vers != 15)) { M_Free(m); return 0; } //"IDP3"

    m->numskins = m->head.numskins; //<- dead code?
    m->numframes = m->head.numframes;

    ofsurf = m->head.ofssurfs;

    fil.Seek(m->head.ofsframes,FileReader::SeekSet); i = m->head.numframes*sizeof(md3frame_t);
    m->head.frames = (md3frame_t *)M_Malloc(i);
    fil.Read(m->head.frames,i);

    if (m->head.numtags == 0) m->head.tags = NULL;
    else
    {
        fil.Seek(m->head.ofstags,FileReader::SeekSet); i = m->head.numtags*sizeof(md3tag_t);
        m->head.tags = (md3tag_t *)M_Malloc(i);
        fil.Read(m->head.tags,i);
    }

    fil.Seek(m->head.ofssurfs,FileReader::SeekSet);
    m->head.surfs = (md3surf_t *)M_Calloc(m->head.numsurfs, sizeof(md3surf_t));
    // NOTE: We assume that NULL is represented by all-zeros.
    // surfs[0].geometry is for POLYMER_MD_PROCESS_CHECK (else: crashes).
    // surfs[i].geometry is for FREE_SURFS_GEOMETRY.
    assert(m->head.surfs[0].geometry == NULL);

#if WORDS_BIGENDIAN
    {
        int32_t j, *l;

        for (i = m->head.numframes-1; i>=0; i--)
        {
            l = (int32_t *)&m->head.frames[i].min;
            for (j=3+3+3+1-1; j>=0; j--) l[j] = LittleLong(l[j]);
        }

        for (i = m->head.numtags-1; i>=0; i--)
        {
            l = (int32_t *)&m->head.tags[i].p;
            for (j=3+3+3+3-1; j>=0; j--) l[j] = LittleLong(l[j]);
        }
    }
#endif

    maxtrispersurf = 0;

    for (surfi=0; surfi<m->head.numsurfs; surfi++)
    {
        s = &m->head.surfs[surfi];
        fil.Seek(ofsurf,FileReader::SeekSet); fil.Read(s,SIZEOF_MD3SURF_T);

#if WORDS_BIGENDIAN
        {
            int32_t j, *l;
            s->id = LittleLong(s->id);
            l =	(int32_t *)&s->flags;
            for	(j=1+1+1+1+1+1+1+1+1+1-1; j>=0; j--) l[j] = LittleLong(l[j]);
        }
#endif

        offs[0] = ofsurf+s->ofstris;
        offs[1] = ofsurf+s->ofsshaders;
        offs[2] = ofsurf+s->ofsuv;
        offs[3] = ofsurf+s->ofsxyzn;

        leng[0] = s->numtris*sizeof(md3tri_t);
        leng[1] = s->numshaders*sizeof(md3shader_t);
        leng[2] = s->numverts*sizeof(md3uv_t);
        leng[3] = s->numframes*s->numverts*sizeof(md3xyzn_t);

        //memoryusage += (s->numverts * s->numframes * sizeof(md3xyzn_t));
        //Printf("Current model geometry memory usage : %i.\n", memoryusage);

        s->tris = (md3tri_t *)M_Malloc((leng[0] + leng[1]) + (leng[2] + leng[3]));

        s->shaders = (md3shader_t *)(((intptr_t)s->tris)+leng[0]);
        s->uv      = (md3uv_t *)(((intptr_t)s->shaders)+leng[1]);
        s->xyzn    = (md3xyzn_t *)(((intptr_t)s->uv)+leng[2]);

        fil.Seek(offs[0],FileReader::SeekSet); fil.Read(s->tris   ,leng[0]);
        fil.Seek(offs[1],FileReader::SeekSet); fil.Read(s->shaders,leng[1]);
        fil.Seek(offs[2],FileReader::SeekSet); fil.Read(s->uv     ,leng[2]);
        fil.Seek(offs[3],FileReader::SeekSet); fil.Read(s->xyzn   ,leng[3]);

#if WORDS_BIGENDIAN
        {
            int32_t j, *l;

            for (i=s->numtris-1; i>=0; i--)
            {
                for (j=2; j>=0; j--) s->tris[i].i[j] = LittleLong(s->tris[i].i[j]);
            }
            for (i=s->numshaders-1; i>=0; i--)
            {
                s->shaders[i].i = LittleLong(s->shaders[i].i);
            }
            for (i=s->numverts-1; i>=0; i--)
            {
                l = (int32_t *)&s->uv[i].u;
                l[0] = LittleLong(l[0]);
                l[1] = LittleLong(l[1]);
            }
            for (i=s->numframes*s->numverts-1; i>=0; i--)
            {
                s->xyzn[i].x = (int16_t)LittleShort((uint16_t)s->xyzn[i].x);
                s->xyzn[i].y = (int16_t)LittleShort((uint16_t)s->xyzn[i].y);
                s->xyzn[i].z = (int16_t)LittleShort((uint16_t)s->xyzn[i].z);
            }
        }
#endif
        maxmodelverts = max(maxmodelverts, s->numverts);
        maxmodeltris = max(maxmodeltris, s->numtris);
        maxtrispersurf = max(maxtrispersurf, s->numtris);
        ofsurf += s->ofsend;
    }

    m->indexes = (uint16_t *)M_Malloc(sizeof(uint16_t) * maxtrispersurf);
    m->vindexes = (uint16_t *)M_Malloc(sizeof(uint16_t) * maxtrispersurf * 3);
    m->maxdepths = (float *)M_Malloc(sizeof(float) * maxtrispersurf);

    return m;
}

static void      md3postload_common(md3model_t *m)
{
    int         framei, surfi, verti;
    md3frame_t  *frame;
    md3xyzn_t   *frameverts;
    float       dist, vec1[3];

    // apparently we can't trust loaded models bounding box/sphere information,
    // so let's compute it ourselves

    framei = 0;

    while (framei < m->head.numframes)
    {
        frame = &m->head.frames[framei];

        memset(&frame->min, 0, sizeof(FVector3));
        memset(&frame->max, 0, sizeof(FVector3));

        frame->r        = 0.0f;

        surfi = 0;
        while (surfi < m->head.numsurfs)
        {
            frameverts = &m->head.surfs[surfi].xyzn[framei * m->head.surfs[surfi].numverts];

            verti = 0;
            while (verti < m->head.surfs[surfi].numverts)
            {
                if (!verti && !surfi)
                {
                    md3xyzn_t const & framevert = frameverts[0];

                    frame->min.X    = framevert.x;
                    frame->min.Y    = framevert.y;
                    frame->min.Z    = framevert.z;

                    frame->max      = frame->min;
                }
                else
                {
                    md3xyzn_t const & framevert = frameverts[verti];

                    if (frame->min.X > framevert.x)
                        frame->min.X = framevert.x;
                    if (frame->max.X < framevert.x)
                        frame->max.X = framevert.x;

                    if (frame->min.Y > framevert.y)
                        frame->min.Y = framevert.y;
                    if (frame->max.Y < framevert.y)
                        frame->max.Y = framevert.y;

                    if (frame->min.Z > framevert.z)
                        frame->min.Z = framevert.z;
                    if (frame->max.Z < framevert.z)
                        frame->max.Z = framevert.z;
                }

                ++verti;
            }

            ++surfi;
        }

        frame->cen.X = (frame->min.X + frame->max.X) * .5f;
        frame->cen.Y = (frame->min.Y + frame->max.Y) * .5f;
        frame->cen.Z = (frame->min.Z + frame->max.Z) * .5f;

        surfi = 0;
        while (surfi < m->head.numsurfs)
        {
            md3surf_t const & surf = m->head.surfs[surfi];

            frameverts = &surf.xyzn[framei * surf.numverts];

            verti = 0;
            while (verti < surf.numverts)
            {
                md3xyzn_t const & framevert = frameverts[verti];

                vec1[0] = framevert.x - frame->cen.X;
                vec1[1] = framevert.y - frame->cen.Y;
                vec1[2] = framevert.z - frame->cen.Z;

                dist = vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2];

                if (dist > frame->r)
                    frame->r = dist;

                ++verti;
            }

            ++surfi;
        }

        frame->r = sqrtf(frame->r);

        ++framei;
    }
}



static void md3free(md3model_t *m)
{
    mdanim_t *anim, *nanim = NULL;
    mdskinmap_t *sk, *nsk = NULL;

    if (!m) return;

    for (anim=m->animations; anim; anim=nanim)
    {
        nanim = anim->next;
        M_Free(anim);
    }
    for (sk=m->skinmap; sk; sk=nsk)
    {
        nsk = sk->next;
        M_Free(sk);
    }

    if (m->head.surfs)
    {
        for (int surfi=m->head.numsurfs-1; surfi>=0; surfi--)
        {
            md3surf_t *s = &m->head.surfs[surfi];
            M_Free(s->tris);
            M_Free(s->geometry);  // FREE_SURFS_GEOMETRY
        }
        M_Free(m->head.surfs);
    }
    M_Free(m->head.tags);
    M_Free(m->head.frames);

    M_Free(m->muladdframes);

    M_Free(m->indexes);
    M_Free(m->vindexes);
    M_Free(m->maxdepths);

    M_Free(m);
}

//---------------------------------------- MD3 LIBRARY ENDS ----------------------------------------
//--------------------------------------- MD LIBRARY BEGINS  ---------------------------------------

static mdmodel_t *mdload(const char *filnam)
{
    mdmodel_t *vm;
    int32_t i;

    //vm = (mdmodel_t *)voxload(filnam);
    //if (vm) return vm;

    auto fil = fileSystem.OpenFileReader(filnam);

    if (!fil.isOpen())
        return NULL;

    fil.Read(&i,4);
    fil.Seek(0,FileReader::SeekSet);

    switch (LittleLong(i))
    {
    case IDP2_MAGIC:
//        Printf("Warning: model \"%s\" is version IDP2; wanted version IDP3\n",filnam);
        vm = (mdmodel_t *)md2load(fil,filnam);
        break; //IDP2
    case IDP3_MAGIC:
        vm = (mdmodel_t *)md3load(fil);
        break; //IDP3
    default:
        vm = NULL;
        break;
    }

    if (vm)
    {
        md3model_t *vm3 = (md3model_t *)vm;

        // smuggle the file name into the model struct.
        // head.nam is unused as far as I can tell
        strncpy(vm3->head.nam, filnam, sizeof(vm3->head.nam));
		vm3->head.nam[sizeof(vm3->head.nam)-1] = 0;
        md3postload_common(vm3);

    }

    return vm;
}


void voxfree(voxmodel_t* m)
{
    if (m) delete m;
}

static void mdfree(mdmodel_t *vm)
{
    if (vm->mdnum == 1) { voxfree((voxmodel_t *)vm); return; }
    if (vm->mdnum == 2 || vm->mdnum == 3) { md3free((md3model_t *)vm); return; }
}

void updateModelInterpolation()
{
	// sigh...
	omdtims = mdtims;
	mdtims = I_msTime();

    TSpriteIterator<DCoreActor> it;
    while (auto actor = it.Next())
    {
        if ((mdpause && actor->sprext.mdanimtims) || (actor->sprext.renderflags & SPREXT_NOMDANIM))
            actor->sprext.mdanimtims += mdtims - omdtims;
    }
}
#endif

//---------------------------------------- MD LIBRARY ENDS  ----------------------------------------
