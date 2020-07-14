/**************************************************************************************************
"POLYMOST" code originally written by Ken Silverman
Ken Silverman's official web site: http://www.advsys.net/ken

**************************************************************************************************/


#include "build.h"
#include "common.h"
#include "engine_priv.h"
#include "mdsprite.h"
#include "polymost.h"
#include "files.h"
#include "buildtiles.h"
#include "bitmap.h"
#include "../../glbackend/glbackend.h"
#include "c_cvars.h"
#include "gamecvars.h"
#include "v_video.h"
#include "flatvertices.h"
#include "palettecontainer.h"
#include "texturemanager.h"
#include "hw_renderstate.h"

CVAR(Bool, hw_detailmapping, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, hw_glowmapping, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVARD(Bool, hw_animsmoothing, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable model animation smoothing")
CVARD(Bool, hw_hightile, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable hightile texture rendering")
CVARD(Bool, hw_models, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable model rendering")
CVARD(Bool, hw_parallaxskypanning, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable parallaxed floor/ceiling panning when drawing a parallaxing sky")
CVARD(Bool, hw_shadeinterpolate, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable shade interpolation")
CVARD(Float, hw_shadescale, 1.0f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "multiplier for shading")
bool hw_int_useindexedcolortextures;
CUSTOM_CVARD(Bool, hw_useindexedcolortextures, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable indexed color texture rendering")
{
    hw_int_useindexedcolortextures = self;
}


//{ "r_yshearing", "enable/disable y-shearing", (void*)&r_yshearing, CVAR_BOOL, 0, 1 }, disabled because not fully functional 

// For testing - will be removed later.
CVAR(Int, skytile, 0, 0)

typedef struct { float x, cy[2], fy[2]; int32_t tag; int16_t n, p, ctag, ftag; } vsptyp;
#define VSPMAX 2048 //<- careful!
static vsptyp vsp[VSPMAX];
static int32_t gtag, viewportNodeCount;
static float xbl, xbr, xbt, xbb;
static int32_t domost_rejectcount;

static float dxb1[MAXWALLSB], dxb2[MAXWALLSB];

//POGOTODO: the SCISDIST could be set to 0 now to allow close objects to render properly,
//          but there's a nasty rendering bug that needs to be dug into when setting SCISDIST lower than 1
#define SCISDIST 1.f  //close plane clipping distance

#define SOFTROTMAT 0

static int32_t r_pogoDebug = 0;

static float gviewxrange;
static float ghoriz, ghoriz2;
static float ghorizcorrect;
double gxyaspect;
float gyxscale, ghalfx, grhalfxdown10, grhalfxdown10x, ghalfy;
float gcosang, gsinang, gcosang2, gsinang2;
float gtang = 0.f;

static float gchang = 0, gshang = 0, gctang = 0, gstang = 0;
static float gvrcorrection = 1.f;

static vec3d_t xtex, ytex, otex, xtex2, ytex2, otex2;

static float fsearchx, fsearchy, fsearchz;
static int psectnum, pwallnum, pbottomwall, pisbottomwall, psearchstat;

static int32_t drawpoly_srepeat = 0, drawpoly_trepeat = 0;
#define MAX_DRAWPOLY_VERTS 8

static int32_t lastglpolygonmode = 0; //FUK

static int32_t r_yshearing = 0;

// used for fogcalc
static float fogresult, fogresult2;

static char ptempbuf[MAXWALLSB<<1];

// polymost ART sky control
static int32_t r_parallaxskyclamping = 1;

#define MIN_CACHETIME_PRINT 10



#define Bfabsf fabsf

static int32_t drawingskybox = 0;
static int32_t hicprecaching = 0;

static hitdata_t polymost_hitdata;

void polymost_outputGLDebugMessage(uint8_t severity, const char* format, ...)
{
}

float sectorVisibility(int sectnum)
{
    // Beware of wraparound madness...
    int v = sector[sectnum].visibility;
    return v? ((uint8_t)(v + 16)) / 16.f : 1.f;
}

//--------------------------------------------------------------------------------------------------

//Use this for both initialization and uninitialization of OpenGL.

//in-place multiply m0=m0*m1
static float* multiplyMatrix4f(float m0[4*4], const float m1[4*4])
{
    float mR[4*4];

#define multMatrix4RowCol(r, c) mR[r*4+c] = m0[r*4]*m1[c] + m0[r*4+1]*m1[c+4] + m0[r*4+2]*m1[c+8] + m0[r*4+3]*m1[c+12]

    multMatrix4RowCol(0, 0);
    multMatrix4RowCol(0, 1);
    multMatrix4RowCol(0, 2);
    multMatrix4RowCol(0, 3);

    multMatrix4RowCol(1, 0);
    multMatrix4RowCol(1, 1);
    multMatrix4RowCol(1, 2);
    multMatrix4RowCol(1, 3);

    multMatrix4RowCol(2, 0);
    multMatrix4RowCol(2, 1);
    multMatrix4RowCol(2, 2);
    multMatrix4RowCol(2, 3);

    multMatrix4RowCol(3, 0);
    multMatrix4RowCol(3, 1);
    multMatrix4RowCol(3, 2);
    multMatrix4RowCol(3, 3);

    Bmemcpy(m0, mR, sizeof(float)*4*4);

    return m0;

#undef multMatrix4RowCol
}


void polymost_glreset()
{
    //Reset if this is -1 (meaning 1st texture call ever), or > 0 (textures in memory)
    gcosang = gcosang2 = 16384.f/262144.f;
    gsinang = gsinang2 = 0.f;
}

FileReader GetBaseResource(const char* fn);

static void resizeglcheck(void)
{
    const int32_t ourxdimen = (windowxy2.x-windowxy1.x+1);
    float ratio = 1;
    const int32_t fovcorrect = (int32_t)(ourxdimen*ratio - ourxdimen);

    ratio = 1.f/ratio;

	GLInterface.SetViewport(windowxy1.x-(fovcorrect/2), ydim-(windowxy2.y+1),
                ourxdimen+fovcorrect, windowxy2.y-windowxy1.y+1);

    float m[4][4]{};

    float const nearclip = 4.0f / (gxyaspect * gyxscale);
    float const farclip = 65536.f;

    m[0][0] = 1.f;
    m[1][1] = fxdimen / (fydimen * ratio);
    m[2][0] = 2.f * ghoriz2 * gstang / fxdimen;
    m[2][1] = 2.f * (ghoriz2 * gctang + ghorizcorrect) / fydimen;
    m[2][2] = (farclip + nearclip) / (farclip - nearclip);
    m[2][3] = 1.f;
    m[3][2] = -(2.f * farclip * nearclip) / (farclip - nearclip);
	renderSetProjectionMatrix(&m[0][0]);
}

//(dpx,dpy) specifies an n-sided polygon. The polygon must be a convex clockwise loop.
//    n must be <= 8 (assume clipping can double number of vertices)
//method: 0:solid, 1:masked(255 is transparent), 2:transluscent #1, 3:transluscent #2
//    +4 means it's a sprite, so wraparound isn't needed

// drawpoly's hack globals
static int32_t pow2xsplit = 0, skyclamphack = 0, skyzbufferhack = 0, flatskyrender = 0;
static float drawpoly_alpha = 0.f;
static uint8_t drawpoly_blend = 0;

int32_t polymost_maskWallHasTranslucency(uwalltype const * const wall)
{
    if (wall->cstat & CSTAT_WALL_TRANSLUCENT)
        return true;

	auto tex = tileGetTexture(wall->picnum);
	auto si = TileFiles.FindReplacement(wall->picnum, wall->pal);
	if (si && hw_hightile) tex = si->faces[0];
	if (tex->GetTexelWidth() == 0 || tex->GetTexelHeight() == 0) return false;
	return tex && tex->GetTranslucency();
}

int32_t polymost_spriteHasTranslucency(tspritetype const * const tspr)
{
    if ((tspr->cstat & CSTAT_SPRITE_TRANSLUCENT) || (tspr->clipdist & TSPR_FLAGS_DRAW_LAST) || 
        ((unsigned)tspr->owner < MAXSPRITES && spriteext[tspr->owner].alpha))
        return true;

	auto tex = tileGetTexture(tspr->picnum);
	auto si = TileFiles.FindReplacement(tspr->picnum, tspr->shade, 0);
	if (si && hw_hightile) tex = si->faces[0];
    if (tex->GetTexelWidth() == 0 || tex->GetTexelHeight() == 0) return false;
    return tex && tex->GetTranslucency();
}

int32_t polymost_spriteIsModelOrVoxel(tspritetype const * const tspr)
{
    if ((unsigned)tspr->owner < MAXSPRITES && spriteext[tspr->owner].flags&SPREXT_NOTMD)
        return false;


    if (hw_models && tile2model[Ptile2tile(tspr->picnum, tspr->pal)].modelid >= 0 &&
        tile2model[Ptile2tile(tspr->picnum, tspr->pal)].framenum >= 0)
        return true;

    if (r_voxels && (tspr->cstat & CSTAT_SPRITE_ALIGNMENT) != CSTAT_SPRITE_ALIGNMENT_SLAB && tiletovox[tspr->picnum] >= 0 && voxmodels[tiletovox[tspr->picnum]])
        return true;

    if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT) == CSTAT_SPRITE_ALIGNMENT_SLAB && voxmodels[tspr->picnum])
        return true;

    return false;
}

static void polymost_updaterotmat(void)
{
    //Up/down rotation
    float matrix[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, gchang, -gshang*gvrcorrection, 0.f,
        0.f, gshang/gvrcorrection, gchang, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    // Tilt rotation
    float tiltmatrix[16] = {
        gctang, -gstang, 0.f, 0.f,
        gstang, gctang, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    multiplyMatrix4f(matrix, tiltmatrix);
    renderSetViewMatrix(matrix);
    renderSetVisibility(mulscale16(g_visibility, mulscale16(xdimenscale, viewingrangerecip)) * fviewingrange * (1.f / (65536.f * 65536.f)) / r_ambientlight);
}

static void polymost_flatskyrender(vec2f_t const* const dpxy, int32_t const n, int32_t method, const vec2_16_t& tilesiz);

// Hack for Duke's camera until I can find out why this behaves erratically.
int skiptile = -1;

static void polymost_drawpoly(vec2f_t const * const dpxy, int32_t const n, int32_t method, const vec2_16_t &tilesize)
{
    if (method == DAMETH_BACKFACECULL || globalpicnum == skiptile ||
        (uint32_t)globalpicnum >= MAXTILES)
        return;

    const int32_t method_ = method;

    if (n == 3)
    {
        if ((dpxy[0].x-dpxy[1].x) * (dpxy[2].y-dpxy[1].y) >=
            (dpxy[2].x-dpxy[1].x) * (dpxy[0].y-dpxy[1].y)) return; //for triangle
    }
    else if (n > 3)
    {
        float f = 0; //f is area of polygon / 2

        for (bssize_t i=n-2, j=n-1,k=0; k<n; i=j,j=k,k++)
            f += (dpxy[i].x-dpxy[k].x)*dpxy[j].y;

        if (f <= 0) return;
    }

    static int32_t skyzbufferhack_pass = 0;
    
    if (flatskyrender && skyzbufferhack_pass == 0)
    {
        polymost_flatskyrender(dpxy, n, method|DAMETH_SKY, tilesize);
        return;
    }

    if (!lookups.checkTable(globalpal))
        globalpal = 0;

    //Load texture (globalpicnum)
    setgotpic(globalpicnum);
	vec2_t tsiz = { tilesize.x, tilesize.y };

    Bassert(n <= MAX_DRAWPOLY_VERTS);

    int j = 0;
    float px[8], py[8], dd[8], uu[8], vv[8];

    for (bssize_t i=0; i<n; ++i)
    {
        px[j] = dpxy[i].x;
        py[j] = dpxy[i].y;

        dd[j] = (dpxy[i].x * xtex.d + dpxy[i].y * ytex.d + otex.d);
        if (dd[j] <= 0.f) // invalid polygon
            return;
        uu[j] = (dpxy[i].x * xtex.u + dpxy[i].y * ytex.u + otex.u);
        vv[j] = (dpxy[i].x * xtex.v + dpxy[i].y * ytex.v + otex.v);
        j++;
    }

    while ((j >= 3) && (px[j-1] == px[0]) && (py[j-1] == py[0])) j--;

    if (j < 3)
        return;

    int const npoints = j;

    float usub = 0;
    float vsub = 0;
#if 0
    if (skyclamphack)
    {
        drawpoly_srepeat = false;
        drawpoly_trepeat = false;
        method = DAMETH_CLAMPED;

        vec2f_t const scale = { 1.f / tsiz.x, 1.f / tsiz.y };

#if 0
        usub = FLT_MAX;
        vsub = FLT_MAX;
        for (int i = 0; i < npoints; i++)
        {
            float const r = 1.f / dd[i];
            float u = floor(uu[i] * r * scale.x);
            float v = floor(vv[i] * r * scale.y);
            if (u < usub) usub = u;
            if (v < vsub) vsub = v;
        }
#endif

        for (int i = 0; i < npoints; i++)
        {
            float const r = 1.f / dd[i];
            float u = uu[i] * r * scale.x - usub;
            float v = vv[i] * r * scale.y - vsub;
            if (u < -FLT_EPSILON || u > 1 + FLT_EPSILON) drawpoly_srepeat = true;
            if (v < -FLT_EPSILON || v > 1 + FLT_EPSILON) drawpoly_trepeat = true;
        }
    }
#endif

    polymost_outputGLDebugMessage(3, "polymost_drawpoly(dpxy:%p, n:%d, method_:%X), method: %X", dpxy, n, method_, method);

	// This only takes effect for textures with their default set to SamplerClampXY.
	int sampleroverride = CLAMP_NONE;
    if (method & DAMETH_CLAMPED)
    {
        if (drawpoly_srepeat) sampleroverride |= CLAMP_Y;
        if (drawpoly_trepeat) sampleroverride |= CLAMP_X;
    }


    int palid = TRANSLATION(Translation_Remap + curbasepal, globalpal);
	bool success = GLInterface.SetTexture(globalpicnum, tileGetTexture(globalpicnum), palid, sampleroverride);
	if (!success)
	{
		tsiz.x = tsiz.y = 1;
		GLInterface.SetColorMask(false); //Hack to update Z-buffer for invalid mirror textures
	}

	GLInterface.SetShade(globalshade, numshades);

    if ((method & DAMETH_WALL) != 0)
    {
        int32_t size = tilesize.y;
        int32_t size2;
        for (size2 = 1; size2 < size; size2 += size2) {}
        if (size == size2)
			GLInterface.SetNpotEmulation(0.f, 0.f); 
        else
        {
            float xOffset = 1.f / tilesize.x;
			GLInterface.SetNpotEmulation((1.f*size2) / size, xOffset);
        }
    }
    else
    {
		GLInterface.SetNpotEmulation(0.f, 0.f);
    }

    vec2_t tsiz2 = tsiz;


    if (method & DAMETH_MASKPROPS)
    {
        SetRenderStyleFromBlend((method & DAMETH_MASKPROPS) > DAMETH_MASK, drawpoly_blend, (method & DAMETH_MASKPROPS) == DAMETH_TRANS2);
    }

    if (!(method & (DAMETH_WALL | DAMETH_CLAMPED | DAMETH_MASK)))
        GLInterface.SetTextureMode(TM_OPAQUE);

    float pc[4];

    // The shade rgb from the tint is ignored here.
    pc[0] = (float)globalr * (1.f / 255.f);
    pc[1] = (float)globalg * (1.f / 255.f);
    pc[2] = (float)globalb * (1.f / 255.f);
  	pc[3] = GetAlphaFromBlend(method & DAMETH_MASKPROPS, drawpoly_blend) * (1.f - drawpoly_alpha);

    if (skyzbufferhack_pass)
        pc[3] = 0.01f;

    GLInterface.SetColor(pc[0], pc[1], pc[2], pc[3]);

    vec2f_t const scale = { 1.f / tsiz2.x, 1.f / tsiz2.y };
	auto data = screen->mVertexData->AllocVertices(npoints);
	auto vt = data.first;
    for (bssize_t i = 0; i < npoints; ++i, vt++)
    {
        float const r = 1.f / dd[i];

        if (tileGetTexture(globalpicnum)->GetTexture()->isHardwareCanvas())
        {
            //update texcoords, canvas textures are upside down!
            vt->SetTexCoord(
                uu[i] * r * scale.x - usub,
                1.f - (vv[i] * r * scale.y - vsub));
        }
        else
        {
            //update texcoords
            vt->SetTexCoord(
                uu[i] * r * scale.x - usub,
                vv[i] * r * scale.y - vsub);
        }

        //update verts
		vt->SetVertex(
			(px[i] - ghalfx) * r * grhalfxdown10x * 1024.f,
			(ghalfy - py[i]) * r * grhalfxdown10 * 1024.f,
			r);

    }
	GLInterface.Draw(DT_TriangleFan, data.second, npoints);

    GLInterface.SetTinting(-1, 0xffffff, 0xffffff);
	GLInterface.SetNpotEmulation(0.f, 0.f);
    GLInterface.SetTextureMode(TM_NORMAL);

	if (skyzbufferhack && skyzbufferhack_pass == 0)
    {
        vec3d_t const bxtex = xtex, bytex = ytex, botex = otex;
        xtex = xtex2, ytex = ytex2, otex = otex2;
		GLInterface.SetColorMask(false);
        GLInterface.Draw(DT_TriangleFan, data.second, npoints);
        GLInterface.SetColorMask(true);
		xtex = bxtex, ytex = bytex, otex = botex;
    }

	if (!success)
		GLInterface.SetColorMask(true);
}


static inline void vsp_finalize_init(int32_t const vcnt)
{
    for (bssize_t i=0; i<vcnt; ++i)
    {
        vsp[i].cy[1] = vsp[i+1].cy[0]; vsp[i].ctag = i;
        vsp[i].fy[1] = vsp[i+1].fy[0]; vsp[i].ftag = i;
        vsp[i].n = i+1; vsp[i].p = i-1;
//        vsp[i].tag = -1;
    }
    vsp[vcnt-1].n = 0; vsp[0].p = vcnt-1;

    //VSPMAX-1 is dummy empty node
    for (bssize_t i=vcnt; i<VSPMAX; i++) { vsp[i].n = i+1; vsp[i].p = i-1; }
    vsp[VSPMAX-1].n = vcnt; vsp[vcnt].p = VSPMAX-1;
}

#define COMBINE_STRIPS

#ifdef COMBINE_STRIPS
static inline void vsdel(int const i)
{
    //Delete i
    int const pi = vsp[i].p;
    int const ni = vsp[i].n;

    vsp[ni].p = pi;
    vsp[pi].n = ni;

    //Add i to empty list
    vsp[i].n = vsp[VSPMAX-1].n;
    vsp[i].p = VSPMAX-1;
    vsp[vsp[VSPMAX-1].n].p = i;
    vsp[VSPMAX-1].n = i;
}

static inline void vsmerge(int const i, int const ni)
{
    vsp[i].cy[1] = vsp[ni].cy[1];
    vsp[i].fy[1] = vsp[ni].fy[1];
    vsdel(ni);
}

#endif

static inline int32_t vsinsaft(int const i)
{
    //i = next element from empty list
    int32_t const r = vsp[VSPMAX-1].n;
    vsp[vsp[r].n].p = VSPMAX-1;
    vsp[VSPMAX-1].n = vsp[r].n;

    vsp[r] = vsp[i]; //copy i to r

    //insert r after i
    vsp[r].p = i; vsp[r].n = vsp[i].n;
    vsp[vsp[i].n].p = r; vsp[i].n = r;

    return r;
}


static int32_t domostpolymethod = DAMETH_NOMASK;

#define DOMOST_OFFSET .01f

static void polymost_clipmost(vec2f_t *dpxy, int &n, float x0, float x1, float y0top, float y0bot, float y1top, float y1bot)
{
    if (y0bot < y0top || y1bot < y1top)
        return;

    //Clip to (x0,y0top)-(x1,y1top)

    vec2f_t dp2[8];

    float t0, t1;
    int n2 = 0;
    t1 = -((dpxy[0].x - x0) * (y1top - y0top) - (dpxy[0].y - y0top) * (x1 - x0));

    for (bssize_t i=0; i<n; i++)
    {
        int j = i + 1;

        if (j >= n)
            j = 0;

        t0 = t1;
        t1 = -((dpxy[j].x - x0) * (y1top - y0top) - (dpxy[j].y - y0top) * (x1 - x0));

        if (t0 >= 0)
            dp2[n2++] = dpxy[i];

        if ((t0 >= 0) != (t1 >= 0) && (t0 <= 0) != (t1 <= 0))
        {
            float const r = t0 / (t0 - t1);
            dp2[n2] = { (dpxy[j].x - dpxy[i].x) * r + dpxy[i].x,
                        (dpxy[j].y - dpxy[i].y) * r + dpxy[i].y };
            n2++;
        }
    }

    if (n2 < 3)
    {
        n = 0;
        return;
    }

    //Clip to (x1,y1bot)-(x0,y0bot)
    t1 = -((dp2[0].x - x1) * (y0bot - y1bot) - (dp2[0].y - y1bot) * (x0 - x1));
    n = 0;

    for (bssize_t i = 0, j = 1; i < n2; j = ++i + 1)
    {
        if (j >= n2)
            j = 0;

        t0 = t1;
        t1 = -((dp2[j].x - x1) * (y0bot - y1bot) - (dp2[j].y - y1bot) * (x0 - x1));

        if (t0 >= 0)
            dpxy[n++] = dp2[i];

        if ((t0 >= 0) != (t1 >= 0) && (t0 <= 0) != (t1 <= 0))
        {
            float const r = t0 / (t0 - t1);
            dpxy[n] = { (dp2[j].x - dp2[i].x) * r + dp2[i].x,
                        (dp2[j].y - dp2[i].y) * r + dp2[i].y };
            n++;
        }
    }

    if (n < 3)
    {
        n = 0;
        return;
    }
}

static void polymost_domost(float x0, float y0, float x1, float y1, float y0top = 0.f, float y0bot = -1.f, float y1top = 0.f, float y1bot = -1.f)
{
    int const dir = (x0 < x1);

    polymost_outputGLDebugMessage(3, "polymost_domost(x0:%f, y0:%f, x1:%f, y1:%f, y0top:%f, y0bot:%f, y1top:%f, y1bot:%f)",
                                  x0, y0, x1, y1, y0top, y0bot, y1top, y1bot);

    y0top -= DOMOST_OFFSET;
    y1top -= DOMOST_OFFSET;
    y0bot += DOMOST_OFFSET;
    y1bot += DOMOST_OFFSET;

    if (dir) //clip dmost (floor)
    {
        y0 -= DOMOST_OFFSET;
        y1 -= DOMOST_OFFSET;
    }
    else //clip umost (ceiling)
    {
        if (x0 == x1) return;
        swapfloat(&x0, &x1);
        swapfloat(&y0, &y1);
        swapfloat(&y0top, &y1top);
        swapfloat(&y0bot, &y1bot);
        y0 += DOMOST_OFFSET;
        y1 += DOMOST_OFFSET; //necessary?
    }

    // Test if span is outside screen bounds
    if (x1 < xbl || x0 > xbr)
    {
        domost_rejectcount++;
        return;
    }

    vec2f_t dm0 = { x0 - DOMOST_OFFSET, y0 };
    vec2f_t dm1 = { x1 + DOMOST_OFFSET, y1 };

    float const slop = (dm1.y - dm0.y) / (dm1.x - dm0.x);

    if (dm0.x < xbl)
    {
        dm0.y += slop*(xbl-dm0.x);
        dm0.x = xbl;
    }

    if (dm1.x > xbr)
    {
        dm1.y += slop*(xbr-dm1.x);
        dm1.x = xbr;
    }

    drawpoly_alpha = 0.f;
    drawpoly_blend = 0;

    vec2f_t n0, n1;
    float spx[4];
    int32_t  spt[4];
    int firstnode = vsp[0].n;

    for (bssize_t newi, i=vsp[0].n; i; i=newi)
    {
        newi = vsp[i].n; n0.x = vsp[i].x; n1.x = vsp[newi].x;

        if (dm0.x >= n1.x)
        {
            firstnode = i;
            continue;
        }
        
        if (n0.x >= dm1.x)
            break;

        if (vsp[i].ctag <= 0) continue;

        float const dx = n1.x-n0.x;
        float const cy[2] = { vsp[i].cy[0], vsp[i].fy[0] },
                    cv[2] = { vsp[i].cy[1]-cy[0], vsp[i].fy[1]-cy[1] };

        int scnt = 0;

        //Test if left edge requires split (dm0.x,dm0.y) (nx0,cy(0)),<dx,cv(0)>
        if ((dm0.x > n0.x) && (dm0.x < n1.x))
        {
            float const t = (dm0.x-n0.x)*cv[dir] - (dm0.y-cy[dir])*dx;
            if (((!dir) && (t < 0.f)) || ((dir) && (t > 0.f)))
                { spx[scnt] = dm0.x; spt[scnt] = -1; scnt++; }
        }

        //Test for intersection on umost (0) and dmost (1)

        float const d[2] = { ((dm0.y - dm1.y) * dx) - ((dm0.x - dm1.x) * cv[0]),
                             ((dm0.y - dm1.y) * dx) - ((dm0.x - dm1.x) * cv[1]) };

        float const n[2] = { ((dm0.y - cy[0]) * dx) - ((dm0.x - n0.x) * cv[0]),
                             ((dm0.y - cy[1]) * dx) - ((dm0.x - n0.x) * cv[1]) };

        float const fnx[2] = { dm0.x + ((n[0] / d[0]) * (dm1.x - dm0.x)),
                               dm0.x + ((n[1] / d[1]) * (dm1.x - dm0.x)) };

        if ((Bfabsf(d[0]) > Bfabsf(n[0])) && (d[0] * n[0] >= 0.f) && (fnx[0] > n0.x) && (fnx[0] < n1.x))
            spx[scnt] = fnx[0], spt[scnt++] = 0;

        if ((Bfabsf(d[1]) > Bfabsf(n[1])) && (d[1] * n[1] >= 0.f) && (fnx[1] > n0.x) && (fnx[1] < n1.x))
            spx[scnt] = fnx[1], spt[scnt++] = 1;

        //Nice hack to avoid full sort later :)
        if ((scnt >= 2) && (spx[scnt-1] < spx[scnt-2]))
        {
            swapfloat(&spx[scnt-1], &spx[scnt-2]);
            swaplong(&spt[scnt-1], &spt[scnt-2]);
        }

        //Test if right edge requires split
        if ((dm1.x > n0.x) && (dm1.x < n1.x))
        {
            float const t = (dm1.x-n0.x)*cv[dir] - (dm1.y-cy[dir])*dx;
            if (((!dir) && (t < 0.f)) || ((dir) && (t > 0.f)))
                { spx[scnt] = dm1.x; spt[scnt] = -1; scnt++; }
        }

        vsp[i].tag = vsp[newi].tag = -1;

        float const rdx = 1.f/dx;

        for (bssize_t z=0, vcnt=0; z<=scnt; z++,i=vcnt)
        {
            float t;

            if (z == scnt)
                goto skip;

            t = (spx[z]-n0.x)*rdx;
            vcnt = vsinsaft(i);
            vsp[i].cy[1] = t*cv[0] + cy[0];
            vsp[i].fy[1] = t*cv[1] + cy[1];
            vsp[vcnt].x = spx[z];
            vsp[vcnt].cy[0] = vsp[i].cy[1];
            vsp[vcnt].fy[0] = vsp[i].fy[1];
            vsp[vcnt].tag = spt[z];

skip: ;
            int32_t const ni = vsp[i].n; if (!ni) continue; //this 'if' fixes many bugs!
            float const dx0 = vsp[i].x; if (dm0.x > dx0) continue;
            float const dx1 = vsp[ni].x; if (dm1.x < dx1) continue;
            n0.y = (dx0-dm0.x)*slop + dm0.y;
            n1.y = (dx1-dm0.x)*slop + dm0.y;

            //      dx0           dx1
            //       ~             ~
            //----------------------------
            //     t0+=0         t1+=0
            //   vsp[i].cy[0]  vsp[i].cy[1]
            //============================
            //     t0+=1         t1+=3
            //============================
            //   vsp[i].fy[0]    vsp[i].fy[1]
            //     t0+=2         t1+=6
            //
            //     ny0 ?         ny1 ?

            int k = 4;

            if ((vsp[i].tag == 0) || (n0.y <= vsp[i].cy[0]+DOMOST_OFFSET)) k--;
            if ((vsp[i].tag == 1) || (n0.y >= vsp[i].fy[0]-DOMOST_OFFSET)) k++;
            if ((vsp[ni].tag == 0) || (n1.y <= vsp[i].cy[1]+DOMOST_OFFSET)) k -= 3;
            if ((vsp[ni].tag == 1) || (n1.y >= vsp[i].fy[1]-DOMOST_OFFSET)) k += 3;

            if (!dir)
            {
                switch (k)
                {
                    case 4:
                    case 5:
                    case 7:
                    {
                        vec2f_t dpxy[8] = {
                            { dx0, vsp[i].cy[0] }, { dx1, vsp[i].cy[1] }, { dx1, n1.y }, { dx0, n0.y }
                        };

                        int n = 4;
                        polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                            polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                        vsp[i].cy[0] = n0.y;
                        vsp[i].cy[1] = n1.y;
                        vsp[i].ctag = gtag;
                    }
                    break;
                    case 1:
                    case 2:
                    {
                        vec2f_t dpxy[8] = { { dx0, vsp[i].cy[0] }, { dx1, vsp[i].cy[1] }, { dx0, n0.y } };

                        int n = 3;
                        polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                            polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                        vsp[i].cy[0] = n0.y;
                        vsp[i].ctag = gtag;
                    }
                    break;
                    case 3:
                    case 6:
                    {
                        vec2f_t dpxy[8] = { { dx0, vsp[i].cy[0] }, { dx1, vsp[i].cy[1] }, { dx1, n1.y } };

                        int n = 3;
                        polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                            polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                        vsp[i].cy[1] = n1.y;
                        vsp[i].ctag = gtag;
                    }
                    break;
                    case 8:
                    {
                        vec2f_t dpxy[8] = {
                            { dx0, vsp[i].cy[0] }, { dx1, vsp[i].cy[1] }, { dx1, vsp[i].fy[1] }, { dx0, vsp[i].fy[0] }
                        };

                        int n = 4;
                        polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                            polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                        vsp[i].ctag = vsp[i].ftag = -1;
                    }
                    default: break;
                }
            }
            else
            {
                switch (k)
                {
                case 4:
                case 3:
                case 1:
                {
                    vec2f_t dpxy[8] = {
                        { dx0, n0.y }, { dx1, n1.y }, { dx1, vsp[i].fy[1] }, { dx0, vsp[i].fy[0] }
                    };

                    int n = 4;
                    polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                        polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                    vsp[i].fy[0] = n0.y;
                    vsp[i].fy[1] = n1.y;
                    vsp[i].ftag = gtag;
                }
                    break;
                case 7:
                case 6:
                {
                    vec2f_t dpxy[8] = { { dx0, n0.y }, { dx1, vsp[i].fy[1] }, { dx0, vsp[i].fy[0] } };

                    int n = 3;
                    polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                        polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                    vsp[i].fy[0] = n0.y;
                    vsp[i].ftag = gtag;
                }
                    break;
                case 5:
                case 2:
                {
                    vec2f_t dpxy[8] = { { dx0, vsp[i].fy[0] }, { dx1, n1.y }, { dx1, vsp[i].fy[1] } };

                    int n = 3;
                    polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                        polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                    vsp[i].fy[1] = n1.y;
                    vsp[i].ftag = gtag;
                }
                    break;
                case 0:
                {
                    vec2f_t dpxy[8] = { { dx0, vsp[i].cy[0] }, { dx1, vsp[i].cy[1] }, { dx1, vsp[i].fy[1] }, { dx0, vsp[i].fy[0] } };

                    int n = 4;
                    polymost_clipmost(dpxy, n, x0, x1, y0top, y0bot, y1top, y1bot);
                        polymost_drawpoly(dpxy, n, domostpolymethod, tilesiz[globalpicnum]);

                    vsp[i].ctag = vsp[i].ftag = -1;
                }
                default:
                    break;
                }
            }
        }
    }

    gtag++;

    //Combine neighboring vertical strips with matching collinear top&bottom edges
    //This prevents x-splits from propagating through the entire scan
#ifdef COMBINE_STRIPS
    int i = firstnode;

    do
    {
        if (vsp[i].x >= dm1.x)
            break;

        if ((vsp[i].cy[0]+DOMOST_OFFSET*2 >= vsp[i].fy[0]) && (vsp[i].cy[1]+DOMOST_OFFSET*2 >= vsp[i].fy[1]))
            vsp[i].ctag = vsp[i].ftag = -1;

        int const ni = vsp[i].n;

        //POGO: specially treat the viewport nodes so that we will never end up in a situation where we accidentally access the sentinel node
        if (ni >= viewportNodeCount)
        {
            if ((vsp[i].ctag == vsp[ni].ctag) && (vsp[i].ftag == vsp[ni].ftag))
            {
                vsmerge(i, ni);
                continue;
            }
            if (vsp[ni].x - vsp[i].x < DOMOST_OFFSET)
            {
                vsp[i].x = vsp[ni].x;
                vsp[i].cy[0] = vsp[ni].cy[0];
                vsp[i].fy[0] = vsp[ni].fy[0];
                vsp[i].ctag = vsp[ni].ctag;
                vsp[i].ftag = vsp[ni].ftag;
                vsmerge(i, ni);
                continue;
            }
        }
        i = ni;
    }
    while (i);
#endif
}


// variables that are set to ceiling- or floor-members, depending
// on which one is processed right now
static int32_t global_cf_z;
static float global_cf_xpanning, global_cf_ypanning, global_cf_heinum;
static int32_t global_cf_shade, global_cf_pal, global_cf_fogpal;
static float (*global_getzofslope_func)(usectorptr_t, float, float);

static void polymost_internal_nonparallaxed(vec2f_t n0, vec2f_t n1, float ryp0, float ryp1, float x0, float x1,
                                            float y0, float y1, int32_t sectnum)
{
    int const have_floor = sectnum & MAXSECTORS;
    sectnum &= ~MAXSECTORS;
    auto const sec = (usectorptr_t)&sector[sectnum];

    // comments from floor code:
            //(singlobalang/-16384*(sx-ghalfx) + 0*(sy-ghoriz) + (cosviewingrangeglobalang/16384)*ghalfx)*d + globalposx    = u*16
            //(cosglobalang/ 16384*(sx-ghalfx) + 0*(sy-ghoriz) + (sinviewingrangeglobalang/16384)*ghalfx)*d + globalposy    = v*16
            //(                  0*(sx-ghalfx) + 1*(sy-ghoriz) + (                             0)*ghalfx)*d + globalposz/16 = (sec->floorz/16)

    float ft[4] = { fglobalposx, fglobalposy, fcosglobalang, fsinglobalang };

    polymost_outputGLDebugMessage(3, "polymost_internal_nonparallaxed(n0:{x:%f, y:%f}, n1:{x:%f, y:%f}, ryp0:%f, ryp1:%f, x0:%f, x1:%f, y0:%f, y1:%f, sectnum:%d)",
                                  n0.x, n0.y, n1.x, n1.y, ryp0, ryp1, x0, x1, y0, y1, sectnum);

    if (globalorientation & 64)
    {
        //relative alignment
        vec2_t const xy = { wall[wall[sec->wallptr].point2].x - wall[sec->wallptr].x,
                            wall[wall[sec->wallptr].point2].y - wall[sec->wallptr].y };
        float r;

        if (globalorientation & 2)
        {
            int i = krecipasm(nsqrtasm(uhypsq(xy.x,xy.y)));
            r = i * (1.f/1073741824.f);
        }
        else
        {
            int i = nsqrtasm(uhypsq(xy.x,xy.y)); if (i == 0) i = 1024; else i = tabledivide32(1048576, i);
            r = i * (1.f/1048576.f);
        }

        vec2f_t const fxy = { xy.x*r, xy.y*r };

        ft[0] = ((float)(globalposx - wall[sec->wallptr].x)) * fxy.x + ((float)(globalposy - wall[sec->wallptr].y)) * fxy.y;
        ft[1] = ((float)(globalposy - wall[sec->wallptr].y)) * fxy.x - ((float)(globalposx - wall[sec->wallptr].x)) * fxy.y;
        ft[2] = fcosglobalang * fxy.x + fsinglobalang * fxy.y;
        ft[3] = fsinglobalang * fxy.x - fcosglobalang * fxy.y;

        globalorientation ^= (!(globalorientation & 4)) ? 32 : 16;
    }

    xtex.d = 0;
    ytex.d = gxyaspect;

    if (!(globalorientation&2) && global_cf_z-globalposz)  // PK 2012: don't allow div by zero
            ytex.d /= (double)(global_cf_z-globalposz);

    otex.d = -ghoriz * ytex.d;

    if (globalorientation & 8)
    {
        ft[0] *=  (1.f / 8.f);
        ft[1] *= -(1.f / 8.f);
        ft[2] *=  (1.f / 2097152.f);
        ft[3] *=  (1.f / 2097152.f);
    }
    else
    {
        ft[0] *=  (1.f / 16.f);
        ft[1] *= -(1.f / 16.f);
        ft[2] *=  (1.f / 4194304.f);
        ft[3] *=  (1.f / 4194304.f);
    }

    xtex.u = ft[3] * -(1.f / 65536.f) * (double)viewingrange;
    xtex.v = ft[2] * -(1.f / 65536.f) * (double)viewingrange;
    ytex.u = ft[0] * ytex.d;
    ytex.v = ft[1] * ytex.d;
    otex.u = ft[0] * otex.d;
    otex.v = ft[1] * otex.d;
    otex.u += (ft[2] - xtex.u) * ghalfx;
    otex.v -= (ft[3] + xtex.v) * ghalfx;

    //Texture flipping
    if (globalorientation&4)
    {
        swapdouble(&xtex.u, &xtex.v);
        swapdouble(&ytex.u, &ytex.v);
        swapdouble(&otex.u, &otex.v);
    }

    if (globalorientation&16) { xtex.u = -xtex.u; ytex.u = -ytex.u; otex.u = -otex.u; }
    if (globalorientation&32) { xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; }

    //Texture panning
    vec2f_t fxy = { global_cf_xpanning * ((float)(1 << widthBits(globalpicnum))) * (1.0f / 256.f),
                    global_cf_ypanning * ((float)(1 << heightBits(globalpicnum))) * (1.0f / 256.f) };

    if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
    {
        float r = global_cf_heinum * (1.0f / 4096.f);
        r = polymost_invsqrt_approximation(r * r + 1);

        if (!(globalorientation & 4))
            fxy.y *= r;
        else
            fxy.x *= r;
    }
    ytex.u += ytex.d*fxy.x; otex.u += otex.d*fxy.x;
    ytex.v += ytex.d*fxy.y; otex.v += otex.d*fxy.y;

    if (globalorientation&2) //slopes
    {
        //Pick some point guaranteed to be not collinear to the 1st two points
        vec2f_t dxy = { n1.y - n0.y, n0.x - n1.x };

        float const dxyr = polymost_invsqrt_approximation(dxy.x * dxy.x + dxy.y * dxy.y);

        dxy.x *= dxyr * 4096.f;
        dxy.y *= dxyr * 4096.f;

        vec2f_t const oxy = { n0.x + dxy.x, n0.y + dxy.y };

        float const ox2 = (oxy.y - fglobalposy) * gcosang - (oxy.x - fglobalposx) * gsinang;
        float oy2 = 1.f / ((oxy.x - fglobalposx) * gcosang2 + (oxy.y - fglobalposy) * gsinang2);

        double const px[3] = { x0, x1, (double)ghalfx * ox2 * oy2 + ghalfx };

        oy2 *= gyxscale;

        double py[3] = { ryp0 + (double)ghoriz, ryp1 + (double)ghoriz, oy2 + (double)ghoriz };

        vec3d_t const duv[3] = {
            { (px[0] * xtex.d + py[0] * ytex.d + otex.d),
              (px[0] * xtex.u + py[0] * ytex.u + otex.u),
              (px[0] * xtex.v + py[0] * ytex.v + otex.v)
            },
            { (px[1] * xtex.d + py[1] * ytex.d + otex.d),
              (px[1] * xtex.u + py[1] * ytex.u + otex.u),
              (px[1] * xtex.v + py[1] * ytex.v + otex.v)
            },
            { (px[2] * xtex.d + py[2] * ytex.d + otex.d),
              (px[2] * xtex.u + py[2] * ytex.u + otex.u),
              (px[2] * xtex.v + py[2] * ytex.v + otex.v)
            }
        };

        py[0] = y0;
        py[1] = y1;
        py[2] = double(global_getzofslope_func((usectorptr_t)&sector[sectnum], oxy.x, oxy.y) - globalposz) * oy2 + ghoriz;

        vec3f_t oxyz[2] = { { (float)(py[1] - py[2]), (float)(py[2] - py[0]), (float)(py[0] - py[1]) },
                            { (float)(px[2] - px[1]), (float)(px[0] - px[2]), (float)(px[1] - px[0]) } };

        float const r = 1.f / (oxyz[0].x * px[0] + oxyz[0].y * px[1] + oxyz[0].z * px[2]);

        xtex.d = (oxyz[0].x * duv[0].d + oxyz[0].y * duv[1].d + oxyz[0].z * duv[2].d) * r;
        xtex.u = (oxyz[0].x * duv[0].u + oxyz[0].y * duv[1].u + oxyz[0].z * duv[2].u) * r;
        xtex.v = (oxyz[0].x * duv[0].v + oxyz[0].y * duv[1].v + oxyz[0].z * duv[2].v) * r;

        ytex.d = (oxyz[1].x * duv[0].d + oxyz[1].y * duv[1].d + oxyz[1].z * duv[2].d) * r;
        ytex.u = (oxyz[1].x * duv[0].u + oxyz[1].y * duv[1].u + oxyz[1].z * duv[2].u) * r;
        ytex.v = (oxyz[1].x * duv[0].v + oxyz[1].y * duv[1].v + oxyz[1].z * duv[2].v) * r;

        otex.d = duv[0].d - px[0] * xtex.d - py[0] * ytex.d;
        otex.u = duv[0].u - px[0] * xtex.u - py[0] * ytex.u;
        otex.v = duv[0].v - px[0] * xtex.v - py[0] * ytex.v;

        if (globalorientation&64) //Hack for relative alignment on slopes
        {
            float r = global_cf_heinum * (1.0f / 4096.f);
            r = Bsqrtf(r*r+1);
            if (!(globalorientation&4)) { xtex.v *= r; ytex.v *= r; otex.v *= r; }
            else { xtex.u *= r; ytex.u *= r; otex.u *= r; }
        }
    }

    domostpolymethod = (globalorientation>>7) & DAMETH_MASKPROPS;

    pow2xsplit = 0;
    drawpoly_alpha = 0.f;
    drawpoly_blend = 0;

    if (have_floor)
    {
        if (globalposz > getflorzofslope(sectnum, globalposx, globalposy))
            domostpolymethod = DAMETH_BACKFACECULL; //Back-face culling

        if (domostpolymethod & DAMETH_MASKPROPS)
            GLInterface.EnableBlend(true);

        polymost_domost(x0, y0, x1, y1); //flor
    }
    else
    {
        if (globalposz < getceilzofslope(sectnum, globalposx, globalposy))
            domostpolymethod = DAMETH_BACKFACECULL; //Back-face culling

        if (domostpolymethod & DAMETH_MASKPROPS)
            GLInterface.EnableBlend(true);

        polymost_domost(x1, y1, x0, y0); //ceil
    }

    if (domostpolymethod & DAMETH_MASKPROPS)
        GLInterface.EnableBlend(false);

    domostpolymethod = DAMETH_NOMASK;
}

static void calc_ypanning(int32_t refposz, float ryp0, float ryp1,
                          float x0, float x1, uint8_t ypan, uint8_t yrepeat,
                          int32_t dopancor, const vec2_16_t &tilesize)
{
    float const t0 = ((float)(refposz-globalposz))*ryp0 + ghoriz;
    float const t1 = ((float)(refposz-globalposz))*ryp1 + ghoriz;
    float t = (float(xtex.d*x0 + otex.d) * (float)yrepeat) / ((x1-x0) * ryp0 * 2048.f);
    int i = 1<< heightBits(globalpicnum);
    if (i < tilesize.y) i <<= 1;


    float const fy = (float)(ypan * i) * (1.f / 256.f);

    xtex.v = double(t0 - t1) * t;
    ytex.v = double(x1 - x0) * t;
    otex.v = -xtex.v * x0 - ytex.v * t0 + fy * otex.d;
    xtex.v += fy * xtex.d;
    ytex.v += fy * ytex.d;
}

static inline int32_t testvisiblemost(float const x0, float const x1)
{
    for (bssize_t i=vsp[0].n, newi; i; i=newi)
    {
        newi = vsp[i].n;
        if ((x0 < vsp[newi].x) && (vsp[i].x < x1) && (vsp[i].ctag >= 0))
            return 1;
    }
    return 0;
}

static inline int polymost_getclosestpointonwall(vec2_t const * const pos, int32_t dawall, vec2_t * const n)
{
    vec2_t const w = { wall[dawall].x, wall[dawall].y };
    vec2_t const d = { POINT2(dawall).x - w.x, POINT2(dawall).y - w.y };
    int64_t i = d.x * ((int64_t)pos->x - w.x) + d.y * ((int64_t)pos->y - w.y);

    if (i < 0)
        return 1;

    int64_t const j = (int64_t)d.x * d.x + (int64_t)d.y * d.y;

    if (i > j)
        return 1;

    i = tabledivide64((i << 15), j) << 15;

    n->x = w.x + ((d.x * i) >> 30);
    n->y = w.y + ((d.y * i) >> 30);

    return 0;
}

static float fgetceilzofslope(usectorptr_t sec, float dax, float day)
{
    if (!(sec->ceilingstat&2))
        return float(sec->ceilingz);

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->ceilingz;

    float const j = (d.x*(day-w.y)-d.y*(dax-w.x))*(1.f/8.f);
    return float(sec->ceilingz) + (sec->ceilingheinum*j)/i;
}

static float fgetflorzofslope(usectorptr_t sec, float dax, float day)
{
    if (!(sec->floorstat&2))
        return float(sec->floorz);

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const w = *(vec2_t const *)wal;
    vec2_t const d = { wal2->x - w.x, wal2->y - w.y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return sec->floorz;

    float const j = (d.x*(day-w.y)-d.y*(dax-w.x))*(1.f/8.f);
    return float(sec->floorz) + (sec->floorheinum*j)/i;
}

static void fgetzsofslope(usectorptr_t sec, float dax, float day, float* ceilz, float *florz)
{
    *ceilz = float(sec->ceilingz); *florz = float(sec->floorz);

    if (((sec->ceilingstat|sec->floorstat)&2) != 2)
        return;

    auto const wal  = (uwallptr_t)&wall[sec->wallptr];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];

    vec2_t const d = { wal2->x - wal->x, wal2->y - wal->y };

    int const i = nsqrtasm(uhypsq(d.x,d.y))<<5;
    if (i == 0) return;
    
    float const j = (d.x*(day-wal->y)-d.y*(dax-wal->x))*(1.f/8.f);
    if (sec->ceilingstat&2)
        *ceilz += (sec->ceilingheinum*j)/i;
    if (sec->floorstat&2)
        *florz += (sec->floorheinum*j)/i;
}

static void polymost_flatskyrender(vec2f_t const* const dpxy, int32_t const n, int32_t method, const vec2_16_t &tilesiz)
{
    flatskyrender = 0;
    vec2f_t xys[8];

    auto f = GLInterface.useMapFog;
    GLInterface.useMapFog = false;
    // Transform polygon to sky coordinates
    for (int i = 0; i < n; i++)
    {
        vec3f_t const o = { dpxy[i].x-ghalfx, dpxy[i].y-ghalfy, ghalfx / gvrcorrection };

        //Up/down rotation
        vec3d_t v = { o.x, o.y * gchang - o.z * gshang, o.z * gchang + o.y * gshang };
        float const r = (ghalfx / gvrcorrection) / v.z;
        xys[i].x = v.x * r + ghalfx;
        xys[i].y = v.y * r + ghalfy;
    }
    
    float const fglobalang = fix16_to_float(qglobalang);
    int32_t dapyscale, dapskybits, dapyoffs, daptileyscale;
    int8_t const * dapskyoff = getpsky(globalpicnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);

    ghoriz = (qglobalhoriz*(1.f/65536.f)-float(ydimen>>1))*dapyscale*(1.f/65536.f)+float(ydimen>>1)+ghorizcorrect;

    float const dd = fxdimen*.0000001f; //Adjust sky depth based on screen size!
    float vv[2];
    float t = (float)((1<<(widthBits(globalpicnum)))<<dapskybits);
    vv[1] = dd*((float)xdimscale*fviewingrange) * (1.f/(daptileyscale*65536.f));
    vv[0] = dd*((float)((tilesiz.y>>1)+dapyoffs)) - vv[1]*ghoriz;
    int ti = (1<<(heightBits(globalpicnum))); if (ti != tilesiz.y) ti += ti;
    vec3f_t o;

    skyclamphack = 0;

    xtex.d = xtex.v = 0;
    ytex.d = ytex.u = 0;
    otex.d = dd;
    xtex.u = otex.d * (t * double(((uint64_t)xdimscale * yxaspect) * viewingrange)) *
                        (1.0 / (16384.0 * 65536.0 * 65536.0 * 5.0 * 1024.0));
    ytex.v = vv[1];
    otex.v = hw_parallaxskypanning ? vv[0] + dd*(float)global_cf_ypanning*(float)ti*(1.f/256.f) : vv[0];

    float x0 = xys[0].x, x1 = xys[0].x;

    for (bssize_t i=n-1; i>=1; i--)
    {
        if (xys[i].x < x0) x0 = xys[i].x;
        if (xys[i].x > x1) x1 = xys[i].x;
    }

    int const npot = (1<<(widthBits(globalpicnum))) != tileWidth(globalpicnum);
    int const xpanning = (hw_parallaxskypanning?global_cf_xpanning:0);

    int picnumbak = globalpicnum;
    ti = globalpicnum;
    o.y = fviewingrange/(ghalfx*256.f); o.z = 1.f/o.y;

    int y = ((int32_t)(((x0-ghalfx)*o.y)+fglobalang)>>(11-dapskybits));
    float fx = x0;

	skyclamphack = true;	// Hack to make Blood's skies show properly.
    do
    {
        globalpicnum = dapskyoff[y&((1<<dapskybits)-1)]+ti;
		if (skytile > 0) 
			globalpicnum = skytile;
        if (npot)
        {
            fx = ((float)((y<<(11-dapskybits))-fglobalang))*o.z+ghalfx;
            int tang = (y<<(11-dapskybits))&2047;
            otex.u = otex.d*(t*((float)(tang)) * (1.f/2048.f) + xpanning) - xtex.u*fx;
        }
        else
            otex.u = otex.d*(t*((float)(fglobalang-(y<<(11-dapskybits)))) * (1.f/2048.f) + xpanning) - xtex.u*ghalfx;
        y++;
        o.x = fx; fx = ((float)((y<<(11-dapskybits))-fglobalang))*o.z+ghalfx;

        if (fx > x1) { fx = x1; ti = -1; }

        vec3d_t otexbak = otex, xtexbak = xtex, ytexbak = ytex;

        // Transform texture mapping factors
        vec2f_t fxy[3] = { { ghalfx * (1.f - 0.25f), ghalfy * (1.f - 0.25f) },
                          { ghalfx, ghalfy * (1.f + 0.25f) },
                          { ghalfx * (1.f + 0.25f), ghalfy * (1.f - 0.25f) } };

        vec3d_t duv[3] = {
            { (fxy[0].x * xtex.d + fxy[0].y * ytex.d + otex.d),
              (fxy[0].x * xtex.u + fxy[0].y * ytex.u + otex.u),
              (fxy[0].x * xtex.v + fxy[0].y * ytex.v + otex.v)
            },
            { (fxy[1].x * xtex.d + fxy[1].y * ytex.d + otex.d),
              (fxy[1].x * xtex.u + fxy[1].y * ytex.u + otex.u),
              (fxy[1].x * xtex.v + fxy[1].y * ytex.v + otex.v)
            },
            { (fxy[2].x * xtex.d + fxy[2].y * ytex.d + otex.d),
              (fxy[2].x * xtex.u + fxy[2].y * ytex.u + otex.u),
              (fxy[2].x * xtex.v + fxy[2].y * ytex.v + otex.v)
            }
        };
        vec2f_t fxyt[3];
        vec3d_t duvt[3];

        for (int i = 0; i < 3; i++)
        {
            vec2f_t const o = { fxy[i].x-ghalfx, fxy[i].y-ghalfy };
            vec3f_t const o2 = { o.x, o.y, ghalfx / gvrcorrection };

            //Up/down rotation (backwards)
            vec3d_t v = { o2.x, o2.y * gchang + o2.z * gshang, o2.z * gchang - o2.y * gshang };
            float const r = (ghalfx / gvrcorrection) / v.z;
            fxyt[i].x = v.x * r + ghalfx;
            fxyt[i].y = v.y * r + ghalfy;
            duvt[i].d = duv[i].d*r;
            duvt[i].u = duv[i].u*r;
            duvt[i].v = duv[i].v*r;
        }

        vec3f_t oxyz[2] = { { (float)(fxyt[1].y - fxyt[2].y), (float)(fxyt[2].y - fxyt[0].y), (float)(fxyt[0].y - fxyt[1].y) },
                            { (float)(fxyt[2].x - fxyt[1].x), (float)(fxyt[0].x - fxyt[2].x), (float)(fxyt[1].x - fxyt[0].x) } };

        float const rr = 1.f / (oxyz[0].x * fxyt[0].x + oxyz[0].y * fxyt[1].x + oxyz[0].z * fxyt[2].x);

        xtex.d = (oxyz[0].x * duvt[0].d + oxyz[0].y * duvt[1].d + oxyz[0].z * duvt[2].d) * rr;
        xtex.u = (oxyz[0].x * duvt[0].u + oxyz[0].y * duvt[1].u + oxyz[0].z * duvt[2].u) * rr;
        xtex.v = (oxyz[0].x * duvt[0].v + oxyz[0].y * duvt[1].v + oxyz[0].z * duvt[2].v) * rr;

        ytex.d = (oxyz[1].x * duvt[0].d + oxyz[1].y * duvt[1].d + oxyz[1].z * duvt[2].d) * rr;
        ytex.u = (oxyz[1].x * duvt[0].u + oxyz[1].y * duvt[1].u + oxyz[1].z * duvt[2].u) * rr;
        ytex.v = (oxyz[1].x * duvt[0].v + oxyz[1].y * duvt[1].v + oxyz[1].z * duvt[2].v) * rr;

        otex.d = duvt[0].d - fxyt[0].x * xtex.d - fxyt[0].y * ytex.d;
        otex.u = duvt[0].u - fxyt[0].x * xtex.u - fxyt[0].y * ytex.u;
        otex.v = duvt[0].v - fxyt[0].x * xtex.v - fxyt[0].y * ytex.v;

        vec2f_t cxy[8];
        vec2f_t cxy2[8];
        int n2 = 0, n3 = 0;

        // Clip to o.x
        for (bssize_t i=0; i<n; i++)
        {
            int const j = i < n-1 ? i + 1 : 0;

            if (xys[i].x >= o.x)
                cxy[n2++] = xys[i];

            if ((xys[i].x >= o.x) != (xys[j].x >= o.x))
            {
                float const r = (o.x - xys[i].x) / (xys[j].x - xys[i].x);
                cxy[n2++] = { o.x, (xys[j].y - xys[i].y) * r + xys[i].y };
            }
        }

        // Clip to fx
        for (bssize_t i=0; i<n2; i++)
        {
            int const j = i < n2-1 ? i + 1 : 0;

            if (cxy[i].x <= fx)
                cxy2[n3++] = cxy[i];

            if ((cxy[i].x <= fx) != (cxy[j].x <= fx))
            {
                float const r = (fx - cxy[i].x) / (cxy[j].x - cxy[i].x);
                cxy2[n3++] = { fx, (cxy[j].y - cxy[i].y) * r + cxy[i].y };
            }
        }

        // Transform back to polymost coordinates
        for (int i = 0; i < n3; i++)
        {
            vec3f_t const o = { cxy2[i].x-ghalfx, cxy2[i].y-ghalfy, ghalfx / gvrcorrection };

            //Up/down rotation
            vec3d_t v = { o.x, o.y * gchang + o.z * gshang, o.z * gchang - o.y * gshang };
            float const r = (ghalfx / gvrcorrection) / v.z;
            cxy[i].x = v.x * r + ghalfx;
            cxy[i].y = v.y * r + ghalfy;
        }

        polymost_drawpoly(cxy, n3, method|DAMETH_WALL, tilesiz);

        otex = otexbak, xtex = xtexbak, ytex = ytexbak;
    }
    while (ti >= 0);
    skyclamphack = false;

    globalpicnum = picnumbak;

    flatskyrender = 1;
    GLInterface.useMapFog = f;
}

static void polymost_drawalls(int32_t const bunch)
{
    drawpoly_alpha = 0.f;
    drawpoly_blend = 0;

    int32_t const sectnum = thesector[bunchfirst[bunch]];
    auto const sec = (usectorptr_t)&sector[sectnum];
    float const fglobalang = fix16_to_float(qglobalang);

    polymost_outputGLDebugMessage(3, "polymost_drawalls(bunch:%d)", bunch);

    //DRAW WALLS SECTION!
    for (bssize_t z=bunchfirst[bunch]; z>=0; z=bunchp2[z])
    {
        int32_t const wallnum = thewall[z];

        auto const wal = (uwallptr_t)&wall[wallnum];
        auto const wal2 = (uwallptr_t)&wall[wal->point2];
        int32_t const nextsectnum = wal->nextsector;
        auto const nextsec = nextsectnum>=0 ? (usectorptr_t)&sector[nextsectnum] : NULL;

        //Offset&Rotate 3D coordinates to screen 3D space
        vec2f_t walpos = { (float)(wal->x-globalposx), (float)(wal->y-globalposy) };

        vec2f_t p0 = { walpos.y * gcosang - walpos.x * gsinang, walpos.x * gcosang2 + walpos.y * gsinang2 };
        vec2f_t const op0 = p0;

        walpos = { (float)(wal2->x - globalposx),
                   (float)(wal2->y - globalposy) };

        vec2f_t p1 = { walpos.y * gcosang - walpos.x * gsinang, walpos.x * gcosang2 + walpos.y * gsinang2 };

        //Clip to close parallel-screen plane

        vec2f_t n0, n1;
        float t0, t1;

        if (p0.y < SCISDIST)
        {
            if (p1.y < SCISDIST) continue;
            t0 = (SCISDIST-p0.y)/(p1.y-p0.y);
            p0 = { (p1.x-p0.x)*t0+p0.x, SCISDIST };
            n0 = { (wal2->x-wal->x)*t0+wal->x,
                   (wal2->y-wal->y)*t0+wal->y };
        }
        else
        {
            t0 = 0.f;
            n0 = { (float)wal->x, (float)wal->y };
        }

        if (p1.y < SCISDIST)
        {
            t1 = (SCISDIST-op0.y)/(p1.y-op0.y);
            p1 = { (p1.x-op0.x)*t1+op0.x, SCISDIST };
            n1 = { (wal2->x-wal->x)*t1+wal->x,
                   (wal2->y-wal->y)*t1+wal->y };
        }
        else
        {
            t1 = 1.f;
            n1 = { (float)wal2->x, (float)wal2->y };
        }

        float ryp0 = 1.f/p0.y, ryp1 = 1.f/p1.y;

        //Generate screen coordinates for front side of wall
        float const x0 = ghalfx*p0.x*ryp0 + ghalfx, x1 = ghalfx*p1.x*ryp1 + ghalfx;

        if (x1 <= x0) continue;

        ryp0 *= gyxscale; ryp1 *= gyxscale;

        float cz, fz;

        fgetzsofslope((usectorptr_t)&sector[sectnum],n0.x,n0.y,&cz,&fz);
        float const cy0 = (cz-globalposz)*ryp0 + ghoriz, fy0 = (fz-globalposz)*ryp0 + ghoriz;

        fgetzsofslope((usectorptr_t)&sector[sectnum],n1.x,n1.y,&cz,&fz);
        float const cy1 = (cz-globalposz)*ryp1 + ghoriz, fy1 = (fz-globalposz)*ryp1 + ghoriz;

        xtex2.d = (ryp0 - ryp1)*gxyaspect / (x0 - x1);
        ytex2.d = 0;
        otex2.d = ryp0 * gxyaspect - xtex2.d*x0;

        xtex2.u = ytex2.u = otex2.u = 0;
        xtex2.v = ytex2.v = otex2.v = 0;

        // Floor

        globalpicnum = sec->floorpicnum;
        globalshade = sec->floorshade;
        globalpal = sec->floorpal;
        globalorientation = sec->floorstat;

		GLInterface.SetVisibility(sectorVisibility(sectnum));

        tileUpdatePicnum(&globalpicnum, sectnum);

        int32_t dapyscale, dapskybits, dapyoffs, daptileyscale;
        int8_t const * dapskyoff = getpsky(globalpicnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);

        global_cf_fogpal = sec->fogpal;
        global_cf_shade = sec->floorshade, global_cf_pal = sec->floorpal; global_cf_z = sec->floorz;  // REFACT
        global_cf_xpanning = sec->floorxpanning; global_cf_ypanning = sec->floorypanning, global_cf_heinum = sec->floorheinum;
        global_getzofslope_func = &fgetflorzofslope;

        if (globalpicnum >= r_rortexture && globalpicnum < r_rortexture + r_rortexturerange && r_rorphase == 0)
        {
            xtex.d = (ryp0-ryp1)*gxyaspect / (x0-x1);
            ytex.d = 0;
            otex.d = ryp0*gxyaspect - xtex.d*x0;
        
            xtex.u = ytex.u = otex.u = 0;
            xtex.v = ytex.v = otex.v = 0;
            polymost_domost(x0, fy0, x1, fy1);
        }
        else if (!(globalorientation&1))
        {
            int32_t fz = getflorzofslope(sectnum, globalposx, globalposy);
            if (globalposz <= fz)
                polymost_internal_nonparallaxed(n0, n1, ryp0, ryp1, x0, x1, fy0, fy1, sectnum | MAXSECTORS);
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].floorstat&1)))
        {
            //Use clamping for tiled sky textures
            //(don't wrap around edges if the sky use multiple panels)
            for (bssize_t i=(1<<dapskybits)-1; i>0; i--)
                if (dapskyoff[i] != dapskyoff[i-1])
                    { skyclamphack = r_parallaxskyclamping; break; }

            skyzbufferhack = 1;

            //if (!hw_hightile || !hicfindskybox(globalpicnum, globalpal))
            {
                float const ghorizbak = ghoriz;
				pow2xsplit = 0;
                skyclamphack = 0;
                flatskyrender = 1;
                //globalshade += globvis2*xdimscale*fviewingrange*(1.f / (64.f * 65536.f * 256.f * 1024.f));
				GLInterface.SetVisibility(0.f);
                polymost_domost(x0,fy0,x1,fy1);
                flatskyrender = 0;
                ghoriz = ghorizbak;
            }
#if 0
            else  //NOTE: code copied from ceiling code... lots of duplicated stuff :/
            {
                //Skybox code for parallax floor!
                float sky_t0, sky_t1; // _nx0, _ny0, _nx1, _ny1;
                float sky_ryp0, sky_ryp1, sky_x0, sky_x1, sky_cy0, sky_fy0, sky_cy1, sky_fy1, sky_ox0, sky_ox1;
                static vec2f_t const skywal[4] = { { -512, -512 }, { 512, -512 }, { 512, 512 }, { -512, 512 } };

                pow2xsplit = 0;
                skyclamphack = 1;

                for (bssize_t i=0; i<4; i++)
                {
                    walpos = skywal[i&3];
                    vec2f_t skyp0 = { walpos.y * gcosang - walpos.x * gsinang,
                                      walpos.x * gcosang2 + walpos.y * gsinang2 };

                    walpos = skywal[(i + 1) & 3];
                    vec2f_t skyp1 = { walpos.y * gcosang - walpos.x * gsinang,
                                      walpos.x * gcosang2 + walpos.y * gsinang2 };

                    vec2f_t const oskyp0 = skyp0;

                    //Clip to close parallel-screen plane
                    if (skyp0.y < SCISDIST)
                    {
                        if (skyp1.y < SCISDIST) continue;
                        sky_t0 = (SCISDIST - skyp0.y) / (skyp1.y - skyp0.y);
                        skyp0  = { (skyp1.x - skyp0.x) * sky_t0 + skyp0.x, SCISDIST };
                    }
                    else { sky_t0 = 0.f; }

                    if (skyp1.y < SCISDIST)
                    {
                        sky_t1  = (SCISDIST - oskyp0.y) / (skyp1.y - oskyp0.y);
                        skyp1 = { (skyp1.x - oskyp0.x) * sky_t1 + oskyp0.x, SCISDIST };
                    }
                    else { sky_t1 = 1.f; }

                    sky_ryp0 = 1.f/skyp0.y; sky_ryp1 = 1.f/skyp1.y;

                    //Generate screen coordinates for front side of wall
                    sky_x0 = ghalfx*skyp0.x*sky_ryp0 + ghalfx;
                    sky_x1 = ghalfx*skyp1.x*sky_ryp1 + ghalfx;
                    if ((sky_x1 <= sky_x0) || (sky_x0 >= x1) || (x0 >= sky_x1)) continue;

                    sky_ryp0 *= gyxscale; sky_ryp1 *= gyxscale;

                    sky_cy0 = -8192.f*sky_ryp0 + ghoriz;
                    sky_fy0 =  8192.f*sky_ryp0 + ghoriz;
                    sky_cy1 = -8192.f*sky_ryp1 + ghoriz;
                    sky_fy1 =  8192.f*sky_ryp1 + ghoriz;

                    sky_ox0 = sky_x0; sky_ox1 = sky_x1;

                    //Make sure: x0<=_x0<_x1<=x1
                    float nfy[2] = { fy0, fy1 };

                    if (sky_x0 < x0)
                    {
                        float const t = (x0-sky_x0)/(sky_x1-sky_x0);
                        sky_cy0 += (sky_cy1-sky_cy0)*t;
                        sky_fy0 += (sky_fy1-sky_fy0)*t;
                        sky_x0 = x0;
                    }
                    else if (sky_x0 > x0) nfy[0] += (sky_x0-x0)*(fy1-fy0)/(x1-x0);

                    if (sky_x1 > x1)
                    {
                        float const t = (x1-sky_x1)/(sky_x1-sky_x0);
                        sky_cy1 += (sky_cy1-sky_cy0)*t;
                        sky_fy1 += (sky_fy1-sky_fy0)*t;
                        sky_x1 = x1;
                    }
                    else if (sky_x1 < x1) nfy[1] += (sky_x1-x1)*(fy1-fy0)/(x1-x0);

                    //   (skybox floor)
                    //(_x0,_fy0)-(_x1,_fy1)
                    //   (skybox wall)
                    //(_x0,_cy0)-(_x1,_cy1)
                    //   (skybox ceiling)
                    //(_x0,nfy0)-(_x1,nfy1)

                    //floor of skybox
                    drawingskybox = 6; //floor/6th texture/index 5 of skybox
                    float const ft[4] = { 512 / 16, 512 / -16, fcosglobalang * (1.f / 2147483648.f),
                                          fsinglobalang * (1.f / 2147483648.f) };

                    xtex.d = 0;
                    ytex.d = gxyaspect*(1.0/4194304.0);
                    otex.d = -ghoriz*ytex.d;
                    xtex.u = ft[3]*fviewingrange*(-1.0/65536.0);
                    xtex.v = ft[2]*fviewingrange*(-1.0/65536.0);
                    ytex.u = ft[0]*ytex.d; ytex.v = ft[1]*ytex.d;
                    otex.u = ft[0]*otex.d; otex.v = ft[1]*otex.d;
                    otex.u += (ft[2]-xtex.u)*ghalfx;
                    otex.v -= (ft[3]+xtex.v)*ghalfx;
                    xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; //y-flip skybox floor

                    if ((sky_fy0 > nfy[0]) && (sky_fy1 > nfy[1]))
                        polymost_domost(sky_x0,sky_fy0,sky_x1,sky_fy1);
                    else if ((sky_fy0 > nfy[0]) != (sky_fy1 > nfy[1]))
                    {
                        //(ox,oy) is intersection of: (_x0,_fy0)-(_x1,_fy1)
                        //                            (_x0,nfy0)-(_x1,nfy1)
                        float const t = (sky_fy0-nfy[0])/(nfy[1]-nfy[0]-sky_fy1+sky_fy0);
                        vec2f_t const o = { sky_x0 + (sky_x1-sky_x0)*t, sky_fy0 + (sky_fy1-sky_fy0)*t };
                        if (nfy[0] > sky_fy0)
                        {
                            polymost_domost(sky_x0,nfy[0],o.x,o.y);
                            polymost_domost(o.x,o.y,sky_x1,sky_fy1);
                        }
                        else
                        {
                            polymost_domost(sky_x0,sky_fy0,o.x,o.y);
                            polymost_domost(o.x,o.y,sky_x1,nfy[1]);
                        }
                    }
                    else
                        polymost_domost(sky_x0,nfy[0],sky_x1,nfy[1]);

                    //wall of skybox
                    drawingskybox = i+1; //i+1th texture/index i of skybox
                    xtex.d = (sky_ryp0-sky_ryp1)*gxyaspect*(1.0/512.0) / (sky_ox0-sky_ox1);
                    ytex.d = 0;
                    otex.d = sky_ryp0*gxyaspect*(1.0/512.0) - xtex.d*sky_ox0;
                    xtex.u = (sky_t0*sky_ryp0 - sky_t1*sky_ryp1)*gxyaspect*(64.0/512.0) / (sky_ox0-sky_ox1);
                    otex.u = sky_t0*sky_ryp0*gxyaspect*(64.0/512.0) - xtex.u*sky_ox0;
                    ytex.u = 0;
                    sky_t0 = -8192.f*sky_ryp0 + ghoriz;
                    sky_t1 = -8192.f*sky_ryp1 + ghoriz;
                    float const t = ((xtex.d*sky_ox0 + otex.d)*8.f) / ((sky_ox1-sky_ox0) * sky_ryp0 * 2048.f);
                    xtex.v = (sky_t0-sky_t1)*t;
                    ytex.v = (sky_ox1-sky_ox0)*t;
                    otex.v = -xtex.v*sky_ox0 - ytex.v*sky_t0;

                    if ((sky_cy0 > nfy[0]) && (sky_cy1 > nfy[1]))
                        polymost_domost(sky_x0,sky_cy0,sky_x1,sky_cy1);
                    else if ((sky_cy0 > nfy[0]) != (sky_cy1 > nfy[1]))
                    {
                        //(ox,oy) is intersection of: (_x0,_fy0)-(_x1,_fy1)
                        //                            (_x0,nfy0)-(_x1,nfy1)
                        float const t = (sky_cy0-nfy[0])/(nfy[1]-nfy[0]-sky_cy1+sky_cy0);
                        vec2f_t const o = { sky_x0 + (sky_x1 - sky_x0) * t, sky_cy0 + (sky_cy1 - sky_cy0) * t };
                        if (nfy[0] > sky_cy0)
                        {
                            polymost_domost(sky_x0,nfy[0],o.x,o.y);
                            polymost_domost(o.x,o.y,sky_x1,sky_cy1);
                        }
                        else
                        {
                            polymost_domost(sky_x0,sky_cy0,o.x,o.y);
                            polymost_domost(o.x,o.y,sky_x1,nfy[1]);
                        }
                    }
                    else
                        polymost_domost(sky_x0,nfy[0],sky_x1,nfy[1]);
                }

                //Ceiling of skybox
                drawingskybox = 5; //ceiling/5th texture/index 4 of skybox
                float const ft[4] = { 512 / 16, -512 / -16, fcosglobalang * (1.f / 2147483648.f),
                                      fsinglobalang * (1.f / 2147483648.f) };

                xtex.d = 0;
                ytex.d = gxyaspect*(-1.0/4194304.0);
                otex.d = -ghoriz*ytex.d;
                xtex.u = ft[3]*fviewingrange*(-1.0/65536.0);
                xtex.v = ft[2]*fviewingrange*(-1.0/65536.0);
                ytex.u = ft[0]*ytex.d; ytex.v = ft[1]*ytex.d;
                otex.u = ft[0]*otex.d; otex.v = ft[1]*otex.d;
                otex.u += (ft[2]-xtex.u)*ghalfx;
                otex.v -= (ft[3]+xtex.v)*ghalfx;

                polymost_domost(x0,fy0,x1,fy1);

                skyclamphack = 0;
                drawingskybox = 0;
            }
#endif

            skyclamphack = 0;
            skyzbufferhack = 0;
        }

        // Ceiling

        globalpicnum = sec->ceilingpicnum;
        globalshade = sec->ceilingshade;
        globalpal = sec->ceilingpal;
        globalorientation = sec->ceilingstat;
        GLInterface.SetVisibility(sectorVisibility(sectnum));

        tileUpdatePicnum(&globalpicnum, sectnum);


        dapskyoff = getpsky(globalpicnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);

        global_cf_fogpal = sec->fogpal;
        global_cf_shade = sec->ceilingshade, global_cf_pal = sec->ceilingpal; global_cf_z = sec->ceilingz;  // REFACT
        global_cf_xpanning = sec->ceilingxpanning; global_cf_ypanning = sec->ceilingypanning, global_cf_heinum = sec->ceilingheinum;
        global_getzofslope_func = &fgetceilzofslope;
        
        if (globalpicnum >= r_rortexture && globalpicnum < r_rortexture + r_rortexturerange && r_rorphase == 0)
        {
            xtex.d = (ryp0-ryp1)*gxyaspect / (x0-x1);
            ytex.d = 0;
            otex.d = ryp0*gxyaspect - xtex.d*x0;
        
            xtex.u = ytex.u = otex.u = 0;
            xtex.v = ytex.v = otex.v = 0;
            polymost_domost(x1, cy1, x0, cy0);
        }
        else if (!(globalorientation&1))
        {
            int32_t cz = getceilzofslope(sectnum, globalposx, globalposy);
            if (globalposz >= cz)
                polymost_internal_nonparallaxed(n0, n1, ryp0, ryp1, x0, x1, cy0, cy1, sectnum);
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].ceilingstat&1)))
        {
            //Use clamping for tiled sky textures
            //(don't wrap around edges if the sky use multiple panels)
            for (bssize_t i=(1<<dapskybits)-1; i>0; i--)
                if (dapskyoff[i] != dapskyoff[i-1])
                    { skyclamphack = r_parallaxskyclamping; break; }

            skyzbufferhack = 1;

			//if (!hw_hightile || !hicfindskybox(globalpicnum, globalpal))
			{
				float const ghorizbak = ghoriz;
				pow2xsplit = 0;
				skyclamphack = 0;
				flatskyrender = 1;
				//globalshade += globvis2 * xdimscale * fviewingrange * (1.f / (64.f * 65536.f * 256.f * 1024.f));
				GLInterface.SetVisibility(0.f);
				polymost_domost(x1, cy1, x0, cy0);
				flatskyrender = 0;
                ghoriz = ghorizbak;
			}
#if 0
            else
            {
                //Skybox code for parallax ceiling!
                float sky_t0, sky_t1; // _nx0, _ny0, _nx1, _ny1;
                float sky_ryp0, sky_ryp1, sky_x0, sky_x1, sky_cy0, sky_fy0, sky_cy1, sky_fy1, sky_ox0, sky_ox1;
                static vec2f_t const skywal[4] = { { -512, -512 }, { 512, -512 }, { 512, 512 }, { -512, 512 } };

                pow2xsplit = 0;
                skyclamphack = 1;

                for (bssize_t i=0; i<4; i++)
                {
                    walpos = skywal[i&3];
                    vec2f_t skyp0 = { walpos.y * gcosang - walpos.x * gsinang,
                                      walpos.x * gcosang2 + walpos.y * gsinang2 };

                    walpos = skywal[(i + 1) & 3];
                    vec2f_t skyp1 = { walpos.y * gcosang - walpos.x * gsinang,
                                      walpos.x * gcosang2 + walpos.y * gsinang2 };

                    vec2f_t const oskyp0 = skyp0;

                    //Clip to close parallel-screen plane
                    if (skyp0.y < SCISDIST)
                    {
                        if (skyp1.y < SCISDIST) continue;
                        sky_t0 = (SCISDIST - skyp0.y) / (skyp1.y - skyp0.y);
                        skyp0  = { (skyp1.x - skyp0.x) * sky_t0 + skyp0.x, SCISDIST };
                    }
                    else { sky_t0 = 0.f; }

                    if (skyp1.y < SCISDIST)
                    {
                        sky_t1 = (SCISDIST - oskyp0.y) / (skyp1.y - oskyp0.y);
                        skyp1  = { (skyp1.x - oskyp0.x) * sky_t1 + oskyp0.x, SCISDIST };
                    }
                    else { sky_t1 = 1.f; }

                    sky_ryp0 = 1.f/skyp0.y; sky_ryp1 = 1.f/skyp1.y;

                    //Generate screen coordinates for front side of wall
                    sky_x0 = ghalfx*skyp0.x*sky_ryp0 + ghalfx;
                    sky_x1 = ghalfx*skyp1.x*sky_ryp1 + ghalfx;
                    if ((sky_x1 <= sky_x0) || (sky_x0 >= x1) || (x0 >= sky_x1)) continue;

                    sky_ryp0 *= gyxscale; sky_ryp1 *= gyxscale;

                    sky_cy0 = -8192.f*sky_ryp0 + ghoriz;
                    sky_fy0 =  8192.f*sky_ryp0 + ghoriz;
                    sky_cy1 = -8192.f*sky_ryp1 + ghoriz;
                    sky_fy1 =  8192.f*sky_ryp1 + ghoriz;

                    sky_ox0 = sky_x0; sky_ox1 = sky_x1;

                    //Make sure: x0<=_x0<_x1<=x1
                    float ncy[2] = { cy0, cy1 };

                    if (sky_x0 < x0)
                    {
                        float const t = (x0-sky_x0)/(sky_x1-sky_x0);
                        sky_cy0 += (sky_cy1-sky_cy0)*t;
                        sky_fy0 += (sky_fy1-sky_fy0)*t;
                        sky_x0 = x0;
                    }
                    else if (sky_x0 > x0) ncy[0] += (sky_x0-x0)*(cy1-cy0)/(x1-x0);

                    if (sky_x1 > x1)
                    {
                        float const t = (x1-sky_x1)/(sky_x1-sky_x0);
                        sky_cy1 += (sky_cy1-sky_cy0)*t;
                        sky_fy1 += (sky_fy1-sky_fy0)*t;
                        sky_x1 = x1;
                    }
                    else if (sky_x1 < x1) ncy[1] += (sky_x1-x1)*(cy1-cy0)/(x1-x0);

                    //   (skybox ceiling)
                    //(_x0,_cy0)-(_x1,_cy1)
                    //   (skybox wall)
                    //(_x0,_fy0)-(_x1,_fy1)
                    //   (skybox floor)
                    //(_x0,ncy0)-(_x1,ncy1)

                    //ceiling of skybox
                    drawingskybox = 5; //ceiling/5th texture/index 4 of skybox
                    float const ft[4] = { 512 / 16, -512 / -16, fcosglobalang * (1.f / 2147483648.f),
                                          fsinglobalang * (1.f / 2147483648.f) };

                    xtex.d = 0;
                    ytex.d = gxyaspect*(-1.0/4194304.0);
                    otex.d = -ghoriz*ytex.d;
                    xtex.u = ft[3]*fviewingrange*(-1.0/65536.0);
                    xtex.v = ft[2]*fviewingrange*(-1.0/65536.0);
                    ytex.u = ft[0]*ytex.d; ytex.v = ft[1]*ytex.d;
                    otex.u = ft[0]*otex.d; otex.v = ft[1]*otex.d;
                    otex.u += (ft[2]-xtex.u)*ghalfx;
                    otex.v -= (ft[3]+xtex.v)*ghalfx;


                    if ((sky_cy0 < ncy[0]) && (sky_cy1 < ncy[1]))
                        polymost_domost(sky_x1,sky_cy1,sky_x0,sky_cy0);
                    else if ((sky_cy0 < ncy[0]) != (sky_cy1 < ncy[1]))
                    {
                        //(ox,oy) is intersection of: (_x0,_cy0)-(_x1,_cy1)
                        //                            (_x0,ncy0)-(_x1,ncy1)
                        float const t = (sky_cy0-ncy[0])/(ncy[1]-ncy[0]-sky_cy1+sky_cy0);
                        vec2f_t const o = { sky_x0 + (sky_x1-sky_x0)*t, sky_cy0 + (sky_cy1-sky_cy0)*t };
                        if (ncy[0] < sky_cy0)
                        {
                            polymost_domost(o.x,o.y,sky_x0,ncy[0]);
                            polymost_domost(sky_x1,sky_cy1,o.x,o.y);
                        }
                        else
                        {
                            polymost_domost(o.x,o.y,sky_x0,sky_cy0);
                            polymost_domost(sky_x1,ncy[1],o.x,o.y);
                        }
                    }
                    else
                        polymost_domost(sky_x1,ncy[1],sky_x0,ncy[0]);

                    //wall of skybox
                    drawingskybox = i+1; //i+1th texture/index i of skybox
                    xtex.d = (sky_ryp0-sky_ryp1)*gxyaspect*(1.0/512.0) / (sky_ox0-sky_ox1);
                    ytex.d = 0;
                    otex.d = sky_ryp0*gxyaspect*(1.0/512.0) - xtex.d*sky_ox0;
                    xtex.u = (sky_t0*sky_ryp0 - sky_t1*sky_ryp1)*gxyaspect*(64.0/512.0) / (sky_ox0-sky_ox1);
                    otex.u = sky_t0*sky_ryp0*gxyaspect*(64.0/512.0) - xtex.u*sky_ox0;
                    ytex.u = 0;
                    sky_t0 = -8192.f*sky_ryp0 + ghoriz;
                    sky_t1 = -8192.f*sky_ryp1 + ghoriz;
                    float const t = ((xtex.d*sky_ox0 + otex.d)*8.f) / ((sky_ox1-sky_ox0) * sky_ryp0 * 2048.f);
                    xtex.v = (sky_t0-sky_t1)*t;
                    ytex.v = (sky_ox1-sky_ox0)*t;
                    otex.v = -xtex.v*sky_ox0 - ytex.v*sky_t0;

                    if ((sky_fy0 < ncy[0]) && (sky_fy1 < ncy[1]))
                        polymost_domost(sky_x1,sky_fy1,sky_x0,sky_fy0);
                    else if ((sky_fy0 < ncy[0]) != (sky_fy1 < ncy[1]))
                    {
                        //(ox,oy) is intersection of: (_x0,_fy0)-(_x1,_fy1)
                        //                            (_x0,ncy0)-(_x1,ncy1)
                        float const t = (sky_fy0-ncy[0])/(ncy[1]-ncy[0]-sky_fy1+sky_fy0);
                        vec2f_t const o = { sky_x0 + (sky_x1 - sky_x0) * t, sky_fy0 + (sky_fy1 - sky_fy0) * t };
                        if (ncy[0] < sky_fy0)
                        {
                            polymost_domost(o.x,o.y,sky_x0,ncy[0]);
                            polymost_domost(sky_x1,sky_fy1,o.x,o.y);
                        }
                        else
                        {
                            polymost_domost(o.x,o.y,sky_x0,sky_fy0);
                            polymost_domost(sky_x1,ncy[1],o.x,o.y);
                        }
                    }
                    else
                        polymost_domost(sky_x1,ncy[1],sky_x0,ncy[0]);
                }

                //Floor of skybox
                drawingskybox = 6; //floor/6th texture/index 5 of skybox
                float const ft[4] = { 512 / 16, 512 / -16, fcosglobalang * (1.f / 2147483648.f),
                                      fsinglobalang * (1.f / 2147483648.f) };

                xtex.d = 0;
                ytex.d = gxyaspect*(1.0/4194304.0);
                otex.d = -ghoriz*ytex.d;
                xtex.u = ft[3]*fviewingrange*(-1.0/65536.0);
                xtex.v = ft[2]*fviewingrange*(-1.0/65536.0);
                ytex.u = ft[0]*ytex.d; ytex.v = ft[1]*ytex.d;
                otex.u = ft[0]*otex.d; otex.v = ft[1]*otex.d;
                otex.u += (ft[2]-xtex.u)*ghalfx;
                otex.v -= (ft[3]+xtex.v)*ghalfx;
                xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; //y-flip skybox floor
                polymost_domost(x1,cy1,x0,cy0);

                skyclamphack = 0;
                drawingskybox = 0;
            }
#endif

            skyclamphack = 0;
            skyzbufferhack = 0;
        }

        // Wall

        xtex.d = (ryp0-ryp1)*gxyaspect / (x0-x1);
        ytex.d = 0;
        otex.d = ryp0*gxyaspect - xtex.d*x0;

        xtex.u = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
        otex.u = t0*ryp0*gxyaspect*wal->xrepeat*8.0 - xtex.u*x0;
        otex.u += (float)wal->xpanning*otex.d;
        xtex.u += (float)wal->xpanning*xtex.d;
        ytex.u = 0;

        float const ogux = xtex.u, oguy = ytex.u, oguo = otex.u;

        Bassert(domostpolymethod == DAMETH_NOMASK);
        domostpolymethod = DAMETH_WALL;

        if (nextsectnum >= 0)
        {
            fgetzsofslope((usectorptr_t)&sector[nextsectnum],n0.x,n0.y,&cz,&fz);
            float const ocy0 = (cz-globalposz)*ryp0 + ghoriz;
            float const ofy0 = (fz-globalposz)*ryp0 + ghoriz;
            fgetzsofslope((usectorptr_t)&sector[nextsectnum],n1.x,n1.y,&cz,&fz);
            float const ocy1 = (cz-globalposz)*ryp1 + ghoriz;
            float const ofy1 = (fz-globalposz)*ryp1 + ghoriz;

            if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = z;

            if (((cy0 < ocy0) || (cy1 < ocy1)) && (!((sec->ceilingstat&sector[nextsectnum].ceilingstat)&1)))
            {
                globalpicnum = wal->picnum; globalshade = wal->shade; globalpal = (int32_t)((uint8_t)wal->pal);
                GLInterface.SetVisibility(sectorVisibility(sectnum));
                globalorientation = wal->cstat;
                tileUpdatePicnum(&globalpicnum, wallnum+16384);

                int i = (!(wal->cstat&4)) ? sector[nextsectnum].ceilingz : sec->ceilingz;

                // over
                calc_ypanning(i, ryp0, ryp1, x0, x1, wal->ypanning, wal->yrepeat, wal->cstat&4, tilesiz[globalpicnum]);

                if (wal->cstat&8) //xflip
                {
                    float const t = (float)(wal->xrepeat*8 + wal->xpanning*2);
                    xtex.u = xtex.d*t - xtex.u;
                    ytex.u = ytex.d*t - ytex.u;
                    otex.u = otex.d*t - otex.u;
                }
                if (wal->cstat&256) { xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; } //yflip

                pow2xsplit = 1;
                polymost_domost(x1,ocy1,x0,ocy0,cy1,ocy1,cy0,ocy0);
                if (wal->cstat&8) { xtex.u = ogux; ytex.u = oguy; otex.u = oguo; }
            }
            if (((ofy0 < fy0) || (ofy1 < fy1)) && (!((sec->floorstat&sector[nextsectnum].floorstat)&1)))
            {
                uwallptr_t nwal;

                if (!(wal->cstat&2)) nwal = wal;
                else
                {
                    nwal = (uwallptr_t)&wall[wal->nextwall];
                    otex.u += (float)(nwal->xpanning - wal->xpanning) * otex.d;
                    xtex.u += (float)(nwal->xpanning - wal->xpanning) * xtex.d;
                    ytex.u += (float)(nwal->xpanning - wal->xpanning) * ytex.d;
                }
                globalpicnum = nwal->picnum; globalshade = nwal->shade; globalpal = (int32_t)((uint8_t)nwal->pal);
                GLInterface.SetVisibility(sectorVisibility(sectnum));
                globalorientation = nwal->cstat;
                tileUpdatePicnum(&globalpicnum, wallnum+16384);

                int i = (!(nwal->cstat&4)) ? sector[nextsectnum].floorz : sec->ceilingz;

                // under
                calc_ypanning(i, ryp0, ryp1, x0, x1, nwal->ypanning, wal->yrepeat, !(nwal->cstat&4), tilesiz[globalpicnum]);

                if (wal->cstat&8) //xflip
                {
                    float const t = (float)(wal->xrepeat*8 + nwal->xpanning*2);
                    xtex.u = xtex.d*t - xtex.u;
                    ytex.u = ytex.d*t - ytex.u;
                    otex.u = otex.d*t - otex.u;
                }
                if (nwal->cstat&256) { xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; } //yflip

                pow2xsplit = 1;
                polymost_domost(x0,ofy0,x1,ofy1,ofy0,fy0,ofy1,fy1);
                if (wal->cstat&(2+8)) { otex.u = oguo; xtex.u = ogux; ytex.u = oguy; }
            }
        }

        if ((nextsectnum < 0) || (wal->cstat&32))   //White/1-way wall
        {
            do
            {
                const int maskingOneWay = (nextsectnum >= 0 && (wal->cstat&32));

                if (maskingOneWay)
                {
                    vec2_t n, pos = { globalposx, globalposy };
                    if (!polymost_getclosestpointonwall(&pos, wallnum, &n) && klabs(pos.x - n.x) + klabs(pos.y - n.y) <= 128)
                        break;
                }

                globalpicnum = (nextsectnum < 0) ? wal->picnum : wal->overpicnum;

                globalshade = wal->shade;
                globalpal = wal->pal;
                GLInterface.SetVisibility(sectorVisibility(sectnum));
                globalorientation = wal->cstat;
                tileUpdatePicnum(&globalpicnum, wallnum+16384);

                int i;
                int const nwcs4 = !(wal->cstat & 4);

                if (nextsectnum >= 0) { i = nwcs4 ? nextsec->ceilingz : sec->ceilingz; }
                else { i = nwcs4 ? sec->ceilingz : sec->floorz; }

                // white / 1-way
                calc_ypanning(i, ryp0, ryp1, x0, x1, wal->ypanning, wal->yrepeat, nwcs4 && !maskingOneWay, tilesiz[globalpicnum]);

                if (wal->cstat&8) //xflip
                {
                    float const t = (float) (wal->xrepeat*8 + wal->xpanning*2);
                    xtex.u = xtex.d*t - xtex.u;
                    ytex.u = ytex.d*t - ytex.u;
                    otex.u = otex.d*t - otex.u;
                }
                if (wal->cstat&256) { xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; } //yflip

                pow2xsplit = 1;

                    polymost_domost(x0, cy0, x1, cy1, cy0, fy0, cy1, fy1);
            } while (0);
        }

        domostpolymethod = DAMETH_NOMASK;

        if (nextsectnum >= 0)
            if ((!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])) && testvisiblemost(x0,x1))
                polymost_scansector(nextsectnum);
    }
}

static int32_t polymost_bunchfront(const int32_t b1, const int32_t b2)
{
    int b1f = bunchfirst[b1];
    const double x2b2 = dxb2[bunchlast[b2]];
    const double x1b1 = dxb1[b1f];

    if (nexttowardf(x1b1, x2b2) >= x2b2)
        return -1;

    int b2f = bunchfirst[b2];
    const double x1b2 = dxb1[b2f];

    if (nexttowardf(x1b2, dxb2[bunchlast[b1]]) >= dxb2[bunchlast[b1]])
        return -1;

    if (nexttowardf(x1b1, x1b2) > x1b2)
    {
        while (nexttowardf(dxb2[b2f], x1b1) <= x1b1) b2f=bunchp2[b2f];
        return wallfront(b1f, b2f);
    }

    while (nexttowardf(dxb2[b1f], x1b2) <= x1b2) b1f=bunchp2[b1f];
    return wallfront(b1f, b2f);
}

void polymost_scansector(int32_t sectnum)
{
    if (sectnum < 0) return;

    if (automapping)
        show2dsector.Set(sectnum);

    sectorborder[0] = sectnum;
    int sectorbordercnt = 1;
    do
    {
        sectnum = sectorborder[--sectorbordercnt];

        for (bssize_t z=headspritesect[sectnum]; z>=0; z=nextspritesect[z])
        {
            auto const spr = (uspriteptr_t)&sprite[z];

            if ((spr->cstat & 0x8000 && !showinvisibility) || spr->xrepeat == 0 || spr->yrepeat == 0)
                continue;

            vec2_t const s = { spr->x-globalposx, spr->y-globalposy };

            if ((spr->cstat&48) ||
                (hw_models && tile2model[spr->picnum].modelid>=0) ||
                ((s.x * gcosang) + (s.y * gsinang) > 0))
            {
                if ((spr->cstat&(64+48))!=(64+16) ||
                    (r_voxels && tiletovox[spr->picnum] >= 0 && voxmodels[tiletovox[spr->picnum]]) ||
                    dmulscale6(sintable[(spr->ang+512)&2047],-s.x, sintable[spr->ang&2047],-s.y) > 0)
                    if (renderAddTsprite(z, sectnum))
                        break;
            }
        }

        gotsector[sectnum>>3] |= pow2char[sectnum&7];

        int const bunchfrst = numbunches;
        int const onumscans = numscans;
        int const startwall = sector[sectnum].wallptr;
        int const endwall   = sector[sectnum].wallnum + startwall;

        int scanfirst = numscans;

        vec2d_t p2 = { 0, 0 };

        uwallptr_t wal;
        int z;

        for (z=startwall,wal=(uwallptr_t)&wall[z]; z<endwall; z++,wal++)
        {
            auto const wal2 = (uwallptr_t)&wall[wal->point2];

            vec2d_t const fp1 = { double(wal->x - globalposx), double(wal->y - globalposy) };
            vec2d_t const fp2 = { double(wal2->x - globalposx), double(wal2->y - globalposy) };

            int const nextsectnum = wal->nextsector; //Scan close sectors

            if (nextsectnum >= 0 && !(wal->cstat&32) && sectorbordercnt < ARRAY_SSIZE(sectorborder))
            if ((gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
            {
                double const d = fp1.x*fp2.y - fp2.x*fp1.y;
                vec2d_t const p1 = { fp2.x-fp1.x, fp2.y-fp1.y };

                // this said (SCISDIST*SCISDIST*260.f), but SCISDIST is 1 and the significance of 260 isn't obvious to me
                // is 260 fudged to solve a problem, and does the problem still apply to our version of the renderer?
                if (d*d < (p1.x*p1.x + p1.y*p1.y) * 256.f)
                {
                    sectorborder[sectorbordercnt++] = nextsectnum;
                    gotsector[nextsectnum>>3] |= pow2char[nextsectnum&7];
                }
            }

            vec2d_t p1;

            if ((z == startwall) || (wall[z-1].point2 != z))
            {
                p1 = { (((fp1.y * fcosglobalang) - (fp1.x * fsinglobalang)) * (1.0/64.0)),
                       (((fp1.x * cosviewingrangeglobalang) + (fp1.y * sinviewingrangeglobalang)) * (1.0/64.0)) };
            }
            else { p1 = p2; }

            p2 = { (((fp2.y * fcosglobalang) - (fp2.x * fsinglobalang)) * (1.0/64.0)),
                   (((fp2.x * cosviewingrangeglobalang) + (fp2.y * sinviewingrangeglobalang)) * (1.0/64.0)) };

            if (numscans >= MAXWALLSB-1)
            {
                Printf("!!numscans\n");
                return;
            }

            //if wall is facing you...
            if ((p1.y >= SCISDIST || p2.y >= SCISDIST) && (nexttoward(p1.x*p2.y, p2.x*p1.y) < p2.x*p1.y))
            {
                dxb1[numscans] = (p1.y >= SCISDIST) ? float(p1.x*ghalfx/p1.y + ghalfx) : -1e32f;
                dxb2[numscans] = (p2.y >= SCISDIST) ? float(p2.x*ghalfx/p2.y + ghalfx) : 1e32f;

                if (dxb1[numscans] < xbl)
                    dxb1[numscans] = xbl;
                else if (dxb1[numscans] > xbr)
                    dxb1[numscans] = xbr;
                if (dxb2[numscans] < xbl)
                    dxb2[numscans] = xbl;
                else if (dxb2[numscans] > xbr)
                    dxb2[numscans] = xbr;

                if (nexttowardf(dxb1[numscans], dxb2[numscans]) < dxb2[numscans])
                {
                    thesector[numscans] = sectnum;
                    thewall[numscans] = z;
                    bunchp2[numscans] = numscans + 1;
                    numscans++;
                }
            }

            if ((wall[z].point2 < z) && (scanfirst < numscans))
            {
                bunchp2[numscans-1] = scanfirst;
                scanfirst = numscans;
            }
        }

        for (bssize_t z=onumscans; z<numscans; z++)
        {
            if ((wall[thewall[z]].point2 != thewall[bunchp2[z]]) || (dxb2[z] > nexttowardf(dxb1[bunchp2[z]], dxb2[z])))
            {
                bunchfirst[numbunches++] = bunchp2[z];
                bunchp2[z] = -1;
            }
        }

        for (bssize_t z=bunchfrst; z<numbunches; z++)
        {
            int zz;
            for (zz=bunchfirst[z]; bunchp2[zz]>=0; zz=bunchp2[zz]) { }
            bunchlast[z] = zz;
        }
    }
    while (sectorbordercnt > 0);
}

/*Init viewport boundary (must be 4 point convex loop):
//      (px[0],py[0]).----.(px[1],py[1])
//                  /      \
//                /          \
// (px[3],py[3]).--------------.(px[2],py[2])
*/

static void polymost_initmosts(const float * px, const float * py, int const n)
{
    if (n < 3) return;

    int32_t imin = (px[1] < px[0]);

    for (bssize_t i=n-1; i>=2; i--)
        if (px[i] < px[imin]) imin = i;

    int32_t vcnt = 1; //0 is dummy solid node

    vsp[vcnt].x = px[imin];
    vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[imin];
    vcnt++;

    int i = imin+1, j = imin-1;
    if (i >= n) i = 0;
    if (j < 0) j = n-1;

    do
    {
        if (px[i] < px[j])
        {
            if (px[i] <= vsp[vcnt-1].x) vcnt--;
            vsp[vcnt].x = px[i];
            vsp[vcnt].cy[0] = py[i];
            int k = j+1; if (k >= n) k = 0;
            //(px[k],py[k])
            //(px[i],?)
            //(px[j],py[j])
            vsp[vcnt].fy[0] = (px[i]-px[k])*(py[j]-py[k])/(px[j]-px[k]) + py[k];
            vcnt++;
            i++; if (i >= n) i = 0;
        }
        else if (px[j] < px[i])
        {
            if (px[j] <= vsp[vcnt-1].x) vcnt--;
            vsp[vcnt].x = px[j];
            vsp[vcnt].fy[0] = py[j];
            int k = i-1; if (k < 0) k = n-1;
            //(px[k],py[k])
            //(px[j],?)
            //(px[i],py[i])
            vsp[vcnt].cy[0] = (px[j]-px[k])*(py[i]-py[k])/(px[i]-px[k]) + py[k];
            vcnt++;
            j--; if (j < 0) j = n-1;
        }
        else
        {
            if (px[i] <= vsp[vcnt-1].x) vcnt--;
            vsp[vcnt].x = px[i];
            vsp[vcnt].cy[0] = py[i];
            vsp[vcnt].fy[0] = py[j];
            vcnt++;
            i++; if (i >= n) i = 0; if (i == j) break;
            j--; if (j < 0) j = n-1;
        }
    } while (i != j);

    if (px[i] > vsp[vcnt-1].x)
    {
        vsp[vcnt].x = px[i];
        vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[i];
        vcnt++;
    }

    domost_rejectcount = 0;

    vsp_finalize_init(vcnt);

    xbl = px[0];
    xbr = px[0];
    xbt = py[0];
    xbb = py[0];

    for (bssize_t i=n-1; i>=1; i--)
    {
        if (xbl > px[i]) xbl = px[i];
        if (xbr < px[i]) xbr = px[i];
        if (xbt > py[i]) xbt = py[i];
        if (xbb < py[i]) xbb = py[i];
    }

    gtag = vcnt;
    viewportNodeCount = vcnt;
}

void polymost_drawrooms()
{
    if (videoGetRenderMode() == REND_CLASSIC) return;

    polymost_outputGLDebugMessage(3, "polymost_drawrooms()");

	GLInterface.ClearDepth();
    GLInterface.EnableBlend(false);
    GLInterface.EnableAlphaTest(false);
    GLInterface.EnableDepthTest(true);
	GLInterface.SetDepthFunc(DF_LEqual);

    gvrcorrection = viewingrange*(1.f/65536.f);
    //if (glprojectionhacks == 2)
    {
        // calculates the extend of the zenith glitch
        float verticalfovtan = (fviewingrange * (windowxy2.y-windowxy1.y) * 5.f) / ((float)yxaspect * (windowxy2.x-windowxy1.x) * 4.f);
        float verticalfov = atanf(verticalfovtan) * (2.f / fPI);
        static constexpr float const maxhorizangle = 0.6361136f; // horiz of 199 in degrees
        float zenglitch = verticalfov + maxhorizangle - 0.95f; // less than 1 because the zenith glitch extends a bit
        if (zenglitch > 0.f)
            gvrcorrection /= (zenglitch * 2.5f) + 1.f;
    }

    //Polymost supports true look up/down :) Here, we convert horizon to angle.
    //gchang&gshang are cos&sin of this angle (respectively)
    gyxscale = ((float)xdimenscale)*(1.0f/131072.f);
    gxyaspect = ((double)xyaspect*fviewingrange)*(5.0/(65536.0*262144.0));
    gviewxrange = fviewingrange * fxdimen * (1.f/(32768.f*1024.f));
    gcosang = fcosglobalang*(1.0f/262144.f);
    gsinang = fsinglobalang*(1.0f/262144.f);
    gcosang2 = gcosang * (fviewingrange * (1.0f/65536.f));
    gsinang2 = gsinang * (fviewingrange * (1.0f/65536.f));
    ghalfx = (float)(xdimen>>1);
    ghalfy = (float)(ydimen>>1);
    grhalfxdown10 = 1.f/(ghalfx*1024.f);
    ghoriz = fix16_to_float(qglobalhoriz);
    ghorizcorrect = fix16_to_float((100-polymostcenterhoriz)*divscale16(xdimenscale, viewingrange));

    GLInterface.SetShadeInterpolate(hw_shadeinterpolate);

    //global cos/sin height angle
    if (r_yshearing)
    {
        gshang  = 0.f;
        gchang  = 1.f;
        ghoriz2 = (float)(ydimen >> 1) - (ghoriz + ghorizcorrect);
    }
    else
    {
        float r = (float)(ydimen >> 1) - (ghoriz + ghorizcorrect);
        gshang  = r / Bsqrtf(r * r + ghalfx * ghalfx / (gvrcorrection * gvrcorrection));
        gchang  = Bsqrtf(1.f - gshang * gshang);
        ghoriz2 = 0.f;
    }

    ghoriz = (float)(ydimen>>1);

    resizeglcheck();
    float const ratio = 1.f;

    //global cos/sin tilt angle
    gctang = cosf(gtang);
    gstang = sinf(gtang);

    if (Bfabsf(gstang) < .001f)  // This avoids nasty precision bugs in domost()
    {
        gstang = 0.f;
        gctang = (gctang > 0.f) ? 1.f : -1.f;
    }

    if (inpreparemirror)
        gstang = -gstang;

    //Generate viewport trapezoid (for handling screen up/down)
    vec3f_t p[4] = {  { 0-1,                                        0-1+ghorizcorrect,                                  0 },
                      { (float)(windowxy2.x + 1 - windowxy1.x + 2), 0-1+ghorizcorrect,                                  0 },
                      { (float)(windowxy2.x + 1 - windowxy1.x + 2), (float)(windowxy2.y + 1 - windowxy1.y + 2)+ghorizcorrect, 0 },
                      { 0-1,                                        (float)(windowxy2.y + 1 - windowxy1.y + 2)+ghorizcorrect, 0 } };

    for (auto & v : p)
    {
        //Tilt rotation (backwards)
        vec2f_t const o = { (v.x-ghalfx)*ratio, (v.y-ghoriz)*ratio };
        vec3f_t const o2 = { o.x*gctang + o.y*gstang, o.y*gctang - o.x*gstang + ghoriz2, ghalfx / gvrcorrection };

        //Up/down rotation (backwards)
        v = { o2.x, o2.y * gchang + o2.z * gshang, o2.z * gchang - o2.y * gshang };
    }

    if (inpreparemirror)
        gstang = -gstang;
    polymost_updaterotmat();

    //Clip to SCISDIST plane
    int n = 0;

    vec3f_t p2[6];

    for (bssize_t i=0; i<4; i++)
    {
        int const j = i < 3 ? i + 1 : 0;

        if (p[i].z >= SCISDIST)
            p2[n++] = p[i];

        if ((p[i].z >= SCISDIST) != (p[j].z >= SCISDIST))
        {
            float const r = (SCISDIST - p[i].z) / (p[j].z - p[i].z);
            p2[n++] = { (p[j].x - p[i].x) * r + p[i].x, (p[j].y - p[i].y) * r + p[i].y, SCISDIST };
        }
    }

    if (n < 3) 
	{
		GLInterface.SetDepthFunc(DF_LEqual);
		return; 
	}

    float sx[6], sy[6];

    for (bssize_t i = 0; i < n; i++)
    {
        float const r = (ghalfx / gvrcorrection) / p2[i].z;
        sx[i] = p2[i].x * r + ghalfx;
        sy[i] = p2[i].y * r + ghoriz;
    }

    polymost_initmosts(sx, sy, n);

    numscans = numbunches = 0;

    // MASKWALL_BAD_ACCESS
    // Fixes access of stale maskwall[maskwallcnt] (a "scan" index, in BUILD lingo):
    maskwallcnt = 0;

    // NOTE: globalcursectnum has been already adjusted in ADJUST_GLOBALCURSECTNUM.
    Bassert((unsigned)globalcursectnum < MAXSECTORS);
    polymost_scansector(globalcursectnum);

    grhalfxdown10x = grhalfxdown10;

    renderBeginScene();

    if (inpreparemirror)
    {
        // see engine.c: INPREPAREMIRROR_NO_BUNCHES
        if (numbunches > 0)
        {
            grhalfxdown10x = -grhalfxdown10;
            polymost_drawalls(0);
            numbunches--;
            bunchfirst[0] = bunchfirst[numbunches];
            bunchlast[0] = bunchlast[numbunches];
        } else
        {
            inpreparemirror = 0;
        }
    }


    while (numbunches > 0)
    {
        Bmemset(ptempbuf,0,numbunches+3); ptempbuf[0] = 1;

        int32_t closest = 0;              //Almost works, but not quite :(

        for (bssize_t i=1; i<numbunches; ++i)
        {
            int const bnch = polymost_bunchfront(i,closest); if (bnch < 0) continue;
            ptempbuf[i] = 1;
            if (!bnch) { ptempbuf[closest] = 1; closest = i; }
        }
        for (bssize_t i=0; i<numbunches; ++i) //Double-check
        {
            if (ptempbuf[i]) continue;
            int const bnch = polymost_bunchfront(i,closest); if (bnch < 0) continue;
            ptempbuf[i] = 1;
            if (!bnch) { ptempbuf[closest] = 1; closest = i; i = 0; }
        }

        polymost_drawalls(closest);

        if (automapping)
        {
            for (int z=bunchfirst[closest]; z>=0; z=bunchp2[z])
                show2dwall[thewall[z]>>3] |= pow2char[thewall[z]&7];
        }

        numbunches--;
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }
    renderFinishScene();

	GLInterface.SetDepthFunc(DF_LEqual);
}

static void polymost_drawmaskwallinternal(int32_t wallIndex)
{
    auto const wal = (uwallptr_t)&wall[wallIndex];
    auto const wal2 = (uwallptr_t)&wall[wal->point2];
    int32_t const sectnum = wall[wal->nextwall].nextsector;
    auto const sec = (usectorptr_t)&sector[sectnum];

//    if (wal->nextsector < 0) return;
    // Without MASKWALL_BAD_ACCESS fix:
    // wal->nextsector is -1, WGR2 SVN Lochwood Hollow (Til' Death L1)  (or trueror1.map)

    auto const nsec = (usectorptr_t)&sector[wal->nextsector];

    polymost_outputGLDebugMessage(3, "polymost_drawmaskwallinternal(wallIndex:%d)", wallIndex);

    globalpicnum = wal->overpicnum;
    if ((uint32_t)globalpicnum >= MAXTILES)
        globalpicnum = 0;

    globalorientation = (int32_t)wal->cstat;
    tileUpdatePicnum(&globalpicnum, (int16_t)wallIndex+16384);

    GLInterface.SetVisibility(sectorVisibility(sectnum));

    globalshade = (int32_t)wal->shade;
    globalpal = (int32_t)((uint8_t)wal->pal);

    vec2f_t s0 = { (float)(wal->x-globalposx), (float)(wal->y-globalposy) };
    vec2f_t p0 = { s0.y*gcosang - s0.x*gsinang, s0.x*gcosang2 + s0.y*gsinang2 };

    vec2f_t s1 = { (float)(wal2->x-globalposx), (float)(wal2->y-globalposy) };
    vec2f_t p1 = { s1.y*gcosang - s1.x*gsinang, s1.x*gcosang2 + s1.y*gsinang2 };

    if ((p0.y < SCISDIST) && (p1.y < SCISDIST)) return;

    //Clip to close parallel-screen plane
    vec2f_t const op0 = p0;

    float t0 = 0.f;

    if (p0.y < SCISDIST)
    {
        t0 = (SCISDIST - p0.y) / (p1.y - p0.y);
        p0 = { (p1.x - p0.x) * t0 + p0.x, SCISDIST };
    }

    float t1 = 1.f;

    if (p1.y < SCISDIST)
    {
        t1 = (SCISDIST - op0.y) / (p1.y - op0.y);
        p1 = { (p1.x - op0.x) * t1 + op0.x, SCISDIST };
    }

    int32_t m0 = (int32_t)((wal2->x - wal->x) * t0 + wal->x);
    int32_t m1 = (int32_t)((wal2->y - wal->y) * t0 + wal->y);
    int32_t cz[4], fz[4];
    getzsofslope(sectnum, m0, m1, &cz[0], &fz[0]);
    getzsofslope(wal->nextsector, m0, m1, &cz[1], &fz[1]);
    m0 = (int32_t)((wal2->x - wal->x) * t1 + wal->x);
    m1 = (int32_t)((wal2->y - wal->y) * t1 + wal->y);
    getzsofslope(sectnum, m0, m1, &cz[2], &fz[2]);
    getzsofslope(wal->nextsector, m0, m1, &cz[3], &fz[3]);

    float ryp0 = 1.f/p0.y;
    float ryp1 = 1.f/p1.y;

    //Generate screen coordinates for front side of wall
    float const x0 = ghalfx*p0.x*ryp0 + ghalfx;
    float const x1 = ghalfx*p1.x*ryp1 + ghalfx;
    if (x1 <= x0) return;

    ryp0 *= gyxscale; ryp1 *= gyxscale;

    xtex.d = (ryp0-ryp1)*gxyaspect / (x0-x1);
    ytex.d = 0;
    otex.d = ryp0*gxyaspect - xtex.d*x0;

    //gux*x0 + guo = t0*wal->xrepeat*8*yp0
    //gux*x1 + guo = t1*wal->xrepeat*8*yp1
    xtex.u = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
    otex.u = t0*ryp0*gxyaspect*(float)wal->xrepeat*8.f - xtex.u*x0;
    otex.u += (float)wal->xpanning*otex.d;
    xtex.u += (float)wal->xpanning*xtex.d;
    ytex.u = 0;

    // mask
    calc_ypanning((!(wal->cstat & 4)) ? max(nsec->ceilingz, sec->ceilingz) : min(nsec->floorz, sec->floorz), ryp0, ryp1,
                  x0, x1, wal->ypanning, wal->yrepeat, 0, tilesiz[globalpicnum]);

    if (wal->cstat&8) //xflip
    {
        float const t = (float)(wal->xrepeat*8 + wal->xpanning*2);
        xtex.u = xtex.d*t - xtex.u;
        ytex.u = ytex.d*t - ytex.u;
        otex.u = otex.d*t - otex.u;
    }
    if (wal->cstat&256) { xtex.v = -xtex.v; ytex.v = -ytex.v; otex.v = -otex.v; } //yflip

    int method = DAMETH_MASK | DAMETH_WALL;

    if (wal->cstat & 128)
        method = DAMETH_WALL | (((wal->cstat & 512)) ? DAMETH_TRANS2 : DAMETH_TRANS1);

#ifdef NEW_MAP_FORMAT
    uint8_t const blend = wal->blend;
#else
    uint8_t const blend = wallext[wallIndex].blend;
#endif
    SetRenderStyleFromBlend(!!(wal->cstat & 128), blend, !!(wal->cstat & 512));

    drawpoly_alpha = 0.f;
    drawpoly_blend = blend;

    float const csy[4] = { ((float)(cz[0] - globalposz)) * ryp0 + ghoriz,
                           ((float)(cz[1] - globalposz)) * ryp0 + ghoriz,
                           ((float)(cz[2] - globalposz)) * ryp1 + ghoriz,
                           ((float)(cz[3] - globalposz)) * ryp1 + ghoriz };

    float const fsy[4] = { ((float)(fz[0] - globalposz)) * ryp0 + ghoriz,
                           ((float)(fz[1] - globalposz)) * ryp0 + ghoriz,
                           ((float)(fz[2] - globalposz)) * ryp1 + ghoriz,
                           ((float)(fz[3] - globalposz)) * ryp1 + ghoriz };

    //Clip 2 quadrilaterals
    //               /csy3
    //             /   |
    // csy0------/----csy2
    //   |     /xxxxxxx|
    //   |   /xxxxxxxxx|
    // csy1/xxxxxxxxxxx|
    //   |xxxxxxxxxxx/fsy3
    //   |xxxxxxxxx/   |
    //   |xxxxxxx/     |
    // fsy0----/------fsy2
    //   |   /
    // fsy1/

    vec2f_t dpxy[16] = { { x0, csy[1] }, { x1, csy[3] }, { x1, fsy[3] }, { x0, fsy[1] } };

    //Clip to (x0,csy[0])-(x1,csy[2])

    vec2f_t dp2[8];

    int n2 = 0;
    t1 = -((dpxy[0].x - x0) * (csy[2] - csy[0]) - (dpxy[0].y - csy[0]) * (x1 - x0));

    for (bssize_t i=0; i<4; i++)
    {
        int j = i + 1;

        if (j >= 4)
            j = 0;

        t0 = t1;
        t1 = -((dpxy[j].x - x0) * (csy[2] - csy[0]) - (dpxy[j].y - csy[0]) * (x1 - x0));

        if (t0 >= 0)
            dp2[n2++] = dpxy[i];

        if ((t0 >= 0) != (t1 >= 0) && (t0 <= 0) != (t1 <= 0))
        {
            float const r = t0 / (t0 - t1);
            dp2[n2++] = { (dpxy[j].x - dpxy[i].x) * r + dpxy[i].x, (dpxy[j].y - dpxy[i].y) * r + dpxy[i].y };
        }
    }

    if (n2 < 3)
        return;

    //Clip to (x1,fsy[2])-(x0,fsy[0])
    t1 = -((dp2[0].x - x1) * (fsy[0] - fsy[2]) - (dp2[0].y - fsy[2]) * (x0 - x1));
    int n = 0;

    for (bssize_t i = 0, j = 1; i < n2; j = ++i + 1)
    {
        if (j >= n2)
            j = 0;

        t0 = t1;
        t1 = -((dp2[j].x - x1) * (fsy[0] - fsy[2]) - (dp2[j].y - fsy[2]) * (x0 - x1));

        if (t0 >= 0)
            dpxy[n++] = dp2[i];

        if ((t0 >= 0) != (t1 >= 0) && (t0 <= 0) != (t1 <= 0))
        {
            float const r = t0 / (t0 - t1);
            dpxy[n++] = { (dp2[j].x - dp2[i].x) * r + dp2[i].x, (dp2[j].y - dp2[i].y) * r + dp2[i].y };
        }
    }

    if (n < 3)
        return;

    pow2xsplit = 0;
    skyclamphack = 0;

    polymost_drawpoly(dpxy, n, method, tilesiz[globalpicnum]);
}

void polymost_drawmaskwall(int32_t damaskwallcnt)
{
    int const z = maskwall[damaskwallcnt];
    polymost_drawmaskwallinternal(thewall[z]);
}

void polymost_prepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang, fix16_t dahoriz, int16_t mirrorWall)
{
    polymost_outputGLDebugMessage(3, "polymost_prepareMirror(%u)", mirrorWall);

    //POGO: prepare necessary globals for drawing, as we intend to call this outside of drawrooms
    gvrcorrection = viewingrange*(1.f/65536.f);
    //if (glprojectionhacks == 2)
    {
        // calculates the extend of the zenith glitch
        float verticalfovtan = (fviewingrange * (windowxy2.y-windowxy1.y) * 5.f) / ((float)yxaspect * (windowxy2.x-windowxy1.x) * 4.f);
        float verticalfov = atanf(verticalfovtan) * (2.f / fPI);
        static constexpr float const maxhorizangle = 0.6361136f; // horiz of 199 in degrees
        float zenglitch = verticalfov + maxhorizangle - 0.95f; // less than 1 because the zenith glitch extends a bit
        if (zenglitch > 0.f)
            gvrcorrection /= (zenglitch * 2.5f) + 1.f;
    }

    set_globalpos(dax, day, daz);
    set_globalang(daang);
    globalhoriz = mulscale16(fix16_to_int(dahoriz)-100,divscale16(xdimenscale,viewingrange))+(ydimen>>1);
    qglobalhoriz = mulscale16(dahoriz-F16(100), divscale16(xdimenscale, viewingrange))+fix16_from_int(ydimen>>1);
    gyxscale = ((float)xdimenscale)*(1.0f/131072.f);
    gxyaspect = ((double)xyaspect*fviewingrange)*(5.0/(65536.0*262144.0));
    gviewxrange = fviewingrange * fxdimen * (1.f/(32768.f*1024.f));
    gcosang = fcosglobalang*(1.0f/262144.f);
    gsinang = fsinglobalang*(1.0f/262144.f);
    gcosang2 = gcosang * (fviewingrange * (1.0f/65536.f));
    gsinang2 = gsinang * (fviewingrange * (1.0f/65536.f));
    ghalfx = (float)(xdimen>>1);
    ghalfy = (float)(ydimen>>1);
    grhalfxdown10 = 1.f/(ghalfx*1024.f);
    ghoriz = fix16_to_float(qglobalhoriz);
    ghorizcorrect = fix16_to_float((100-polymostcenterhoriz)*divscale16(xdimenscale, viewingrange));
    resizeglcheck();
    if (r_yshearing)
    {
        gshang  = 0.f;
        gchang  = 1.f;
        ghoriz2 = (float)(ydimen >> 1) - (ghoriz+ghorizcorrect);
    }
    else
    {
        float r = (float)(ydimen >> 1) - (ghoriz+ghorizcorrect);
        gshang  = r / Bsqrtf(r * r + ghalfx * ghalfx / (gvrcorrection * gvrcorrection));
        gchang  = Bsqrtf(1.f - gshang * gshang);
        ghoriz2 = 0.f;
    }
    ghoriz = (float)(ydimen>>1);
    gctang = cosf(gtang);
    gstang = sinf(gtang);
    if (Bfabsf(gstang) < .001f)
    {
        gstang = 0.f;
        gctang = (gctang > 0.f) ? 1.f : -1.f;
    }
    polymost_updaterotmat();
    grhalfxdown10x = grhalfxdown10;

    renderBeginScene();
    //POGO: write the mirror region to the stencil buffer to allow showing mirrors & skyboxes at the same time
	GLInterface.EnableStencilWrite(1);
    GLInterface.EnableAlphaTest(false);
    GLInterface.EnableDepthTest(false);
    polymost_drawmaskwallinternal(mirrorWall);
    GLInterface.EnableAlphaTest(true);
    GLInterface.EnableDepthTest(true);
    renderFinishScene();

    //POGO: render only to the mirror region
	GLInterface.EnableStencilTest(1);
}

void polymost_completeMirror()
{
    polymost_outputGLDebugMessage(3, "polymost_completeMirror()");
	GLInterface.DisableStencil();
}

typedef struct
{
    int16_t wall;
    int8_t wdist;
    int8_t filler;
} wallspriteinfo_t;

static wallspriteinfo_t wsprinfo[MAXSPRITES];

void Polymost_prepare_loadboard(void)
{
    Bmemset(wsprinfo, 0, sizeof(wsprinfo));
}

void polymost_deletesprite(int num)
{
    wsprinfo[num].wall = -1;

}


static inline int32_t polymost_findwall(tspritetype const * const tspr, vec2_t const * const tsiz, int32_t * rd)
{
    int32_t dist = 4, closest = -1;
    auto const sect = (usectortype  * )&sector[tspr->sectnum];
    vec2_t n;

    for (bssize_t i=sect->wallptr; i<sect->wallptr + sect->wallnum; i++)
    {
        if ((wall[i].nextsector == -1 || ((sector[wall[i].nextsector].ceilingz > (tspr->z - ((tsiz->y * tspr->yrepeat) << 2))) ||
             sector[wall[i].nextsector].floorz < tspr->z)) && !polymost_getclosestpointonwall((const vec2_t *) tspr, i, &n))
        {
            int const dst = klabs(tspr->x - n.x) + klabs(tspr->y - n.y);

            if (dst <= dist)
            {
                dist = dst;
                closest = i;
            }
        }
    }

    *rd = dist;

    return closest;
}

static int32_t polymost_lintersect(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            int32_t x3, int32_t y3, int32_t x4, int32_t y4)
{
    // p1 to p2 is a line segment
    int32_t const x21 = x2 - x1, x34 = x3 - x4;
    int32_t const y21 = y2 - y1, y34 = y3 - y4;
    int32_t const bot = x21 * y34 - y21 * x34;

    if (!bot)
        return 0;

    int32_t const x31 = x3 - x1, y31 = y3 - y1;
    int32_t const topt = x31 * y34 - y31 * x34;

    int rv = 1;

    if (bot > 0)
    {
        if ((unsigned)topt >= (unsigned)bot)
            rv = 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu >= (unsigned)bot)
            rv = 0;
    }
    else
    {
        if ((unsigned)topt <= (unsigned)bot)
            rv = 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu <= (unsigned)bot)
            rv = 0;
    }

    return rv;
}

#define TSPR_OFFSET_FACTOR .0002f
#define TSPR_OFFSET(tspr) (TSPR_OFFSET_FACTOR + ((tspr->owner != -1 ? tspr->owner & 63 : 0) * TSPR_OFFSET_FACTOR))

#define TSPR_OFFSET_FACTOR2 .000008f
#define TSPR_OFFSET2(tspr) ((TSPR_OFFSET_FACTOR + ((tspr->owner != -1 ? tspr->owner & 63 : 1) * TSPR_OFFSET_FACTOR)) * (float)sepdist(globalposx - tspr->x, globalposy - tspr->y, globalposz - tspr->z) * 0.025f)


void polymost_drawsprite(int32_t snum)
{
    auto const tspr = tspriteptr[snum];

    if (EDUKE32_PREDICT_FALSE(bad_tspr(tspr)))
        return;

    usectorptr_t sec;

    int32_t spritenum = tspr->owner;

    polymost_outputGLDebugMessage(3, "polymost_drawsprite(snum:%d)", snum);

    if ((tspr->cstat&48) != 48)
        tileUpdatePicnum(&tspr->picnum, spritenum + 32768);

    globalpicnum = tspr->picnum;
    globalshade = tspr->shade;
    globalpal = tspr->pal;
    globalorientation = tspr->cstat;

    GLInterface.SetVisibility(sectorVisibility(tspr->sectnum));

    vec2_t off = { 0, 0 };

    if ((globalorientation & 48) != 48)  // only non-voxel sprites should do this
    {
        int const flag = hw_hightile && h_xsize[globalpicnum];
        off = { (int32_t)tspr->xoffset + (flag ? h_xoffs[globalpicnum] : tileLeftOffset(globalpicnum)),
                (int32_t)tspr->yoffset + (flag ? h_yoffs[globalpicnum] : tileTopOffset(globalpicnum)) };
    }

    int32_t method = DAMETH_MASK | DAMETH_CLAMPED;

    if (tspr->cstat & 2)
        method = DAMETH_CLAMPED | ((tspr->cstat & 512) ? DAMETH_TRANS2 : DAMETH_TRANS1);

    SetRenderStyleFromBlend(!!(tspr->cstat & 2), tspr->blend, !!(tspr->cstat & 512));

    drawpoly_alpha = spriteext[spritenum].alpha;
    drawpoly_blend = tspr->blend;

    sec = (usectorptr_t)&sector[tspr->sectnum];

    while (!(spriteext[spritenum].flags & SPREXT_NOTMD))
    {
        if (hw_models && tile2model[Ptile2tile(tspr->picnum, tspr->pal)].modelid >= 0 &&
            tile2model[Ptile2tile(tspr->picnum, tspr->pal)].framenum >= 0)
        {
            if (polymost_mddraw(tspr)) return;
            break;  // else, render as flat sprite
        }

        if (r_voxels)
        {
            if ((tspr->cstat & 48) != 48 && tiletovox[tspr->picnum] >= 0 && voxmodels[tiletovox[tspr->picnum]])
            {
                if (polymost_voxdraw(voxmodels[tiletovox[tspr->picnum]], tspr)) return;
                break;  // else, render as flat sprite
            }

            if ((tspr->cstat & 48) == 48 && tspr->picnum < MAXVOXELS && voxmodels[tspr->picnum])
            {
                polymost_voxdraw(voxmodels[tspr->picnum], tspr);
                return;
            }
        }


        break;
    }

    vec3_t pos = tspr->pos;

    if (spriteext[spritenum].flags & SPREXT_AWAY1)
    {
        pos.x += (sintable[(tspr->ang + 512) & 2047] >> 13);
        pos.y += (sintable[(tspr->ang) & 2047] >> 13);
    }
    else if (spriteext[spritenum].flags & SPREXT_AWAY2)
    {
        pos.x -= (sintable[(tspr->ang + 512) & 2047] >> 13);
        pos.y -= (sintable[(tspr->ang) & 2047] >> 13);
    }

    vec2_16_t const oldsiz = tilesiz[globalpicnum];
    vec2_t tsiz = { oldsiz.x, oldsiz.y };

    if (hw_hightile && h_xsize[globalpicnum])
        tsiz = { h_xsize[globalpicnum], h_ysize[globalpicnum] };

    if (tsiz.x <= 0 || tsiz.y <= 0)
        return;

    vec2f_t const ftsiz = { (float) tsiz.x, (float) tsiz.y };

    switch ((globalorientation >> 4) & 3)
    {
        case 0:  // Face sprite
        {
            // Project 3D to 2D
            if (globalorientation & 4)
                off.x = -off.x;
            // NOTE: yoff not negated not for y flipping, unlike wall and floor
            // aligned sprites.

            int const ang = (getangle(tspr->x - globalposx, tspr->y - globalposy) + 1024) & 2047;

            float foffs = TSPR_OFFSET(tspr);
            float foffs2 = TSPR_OFFSET(tspr);
			if (fabs(foffs2) < fabs(foffs)) foffs = foffs2;

            vec2f_t const offs = { (float)(sintable[(ang + 512) & 2047] >> 6)* foffs,  (float) (sintable[(ang) & 2047] >> 6) * foffs };

            vec2f_t s0 = { (float)(tspr->x - globalposx) + offs.x,
                           (float)(tspr->y - globalposy) + offs.y};

            vec2f_t p0 = { s0.y * gcosang - s0.x * gsinang, s0.x * gcosang2 + s0.y * gsinang2 };

            if (p0.y <= SCISDIST)
                goto _drawsprite_return;

            float const ryp0 = 1.f / p0.y;
            s0 = { ghalfx * p0.x * ryp0 + ghalfx, ((float)(pos.z - globalposz)) * gyxscale * ryp0 + ghoriz };

            float const f = ryp0 * fxdimen * (1.0f / 160.f);

            vec2f_t ff = { ((float)tspr->xrepeat) * f,
                           ((float)tspr->yrepeat) * f * ((float)yxaspect * (1.0f / 65536.f)) };

            if (tsiz.x & 1)
                s0.x += ff.x * 0.5f;
            if (globalorientation & 128 && tsiz.y & 1)
                s0.y += ff.y * 0.5f;

            s0.x -= ff.x * (float) off.x;
            s0.y -= ff.y * (float) off.y;

            ff.x *= ftsiz.x;
            ff.y *= ftsiz.y;

            vec2f_t pxy[4];

            pxy[0].x = pxy[3].x = s0.x - ff.x * 0.5f;
            pxy[1].x = pxy[2].x = s0.x + ff.x * 0.5f;
            if (!(globalorientation & 128))
            {
                pxy[0].y = pxy[1].y = s0.y - ff.y;
                pxy[2].y = pxy[3].y = s0.y;
            }
            else
            {
                pxy[0].y = pxy[1].y = s0.y - ff.y * 0.5f;
                pxy[2].y = pxy[3].y = s0.y + ff.y * 0.5f;
            }

            xtex.d = ytex.d = ytex.u = xtex.v = 0;
            otex.d = ryp0 * gviewxrange;

            if (!(globalorientation & 4))
            {
                xtex.u = ftsiz.x * otex.d / (pxy[1].x - pxy[0].x + .002f);
                otex.u = -xtex.u * (pxy[0].x - .001f);
            }
            else
            {
                xtex.u = ftsiz.x * otex.d / (pxy[0].x - pxy[1].x - .002f);
                otex.u = -xtex.u * (pxy[1].x + .001f);
            }

            if (!(globalorientation & 8))
            {
                ytex.v = ftsiz.y * otex.d / (pxy[3].y - pxy[0].y + .002f);
                otex.v = -ytex.v * (pxy[0].y - .001f);
            }
            else
            {
                ytex.v = ftsiz.y * otex.d / (pxy[0].y - pxy[3].y - .002f);
                otex.v = -ytex.v * (pxy[3].y + .001f);
            }

            // sprite panning
            if (spriteext[spritenum].xpanning)
            {
                ytex.u -= ytex.d * ((float) (spriteext[spritenum].xpanning) * (1.0f / 255.f)) * ftsiz.x;
                otex.u -= otex.d * ((float) (spriteext[spritenum].xpanning) * (1.0f / 255.f)) * ftsiz.x;
                drawpoly_srepeat = 1;
            }

            if (spriteext[spritenum].ypanning)
            {
                ytex.v -= ytex.d * ((float) (spriteext[spritenum].ypanning) * (1.0f / 255.f)) * ftsiz.y;
                otex.v -= otex.d * ((float) (spriteext[spritenum].ypanning) * (1.0f / 255.f)) * ftsiz.y;
                drawpoly_trepeat = 1;
            }

            // Clip sprites to ceilings/floors when no parallaxing and not sloped
            if (!(sector[tspr->sectnum].ceilingstat & 3))
            {
                s0.y = ((float) (sector[tspr->sectnum].ceilingz - globalposz)) * gyxscale * ryp0 + ghoriz;
                if (pxy[0].y < s0.y)
                    pxy[0].y = pxy[1].y = s0.y;
            }

            if (!(sector[tspr->sectnum].floorstat & 3))
            {
                s0.y = ((float) (sector[tspr->sectnum].floorz - globalposz)) * gyxscale * ryp0 + ghoriz;
                if (pxy[2].y > s0.y)
                    pxy[2].y = pxy[3].y = s0.y;
            }

            vec2_16_t tempsiz = { (int16_t)tsiz.x, (int16_t)tsiz.y };
            pow2xsplit = 0;
            polymost_drawpoly(pxy, 4, method, tempsiz);

            drawpoly_srepeat = 0;
            drawpoly_trepeat = 0;
        }
        break;

        case 1:  // Wall sprite
        {
            // Project 3D to 2D
            if (globalorientation & 4)
                off.x = -off.x;

            if (globalorientation & 8)
                off.y = -off.y;

            vec2f_t const extent = { (float)tspr->xrepeat * (float)sintable[(tspr->ang) & 2047] * (1.0f / 65536.f),
                                     (float)tspr->xrepeat * (float)sintable[(tspr->ang + 1536) & 2047] * (1.0f / 65536.f) };

            float f = (float)(tsiz.x >> 1) + (float)off.x;

            vec2f_t const vf = { extent.x * f, extent.y * f };

            vec2f_t vec0 = { (float)(pos.x - globalposx) - vf.x,
                             (float)(pos.y - globalposy) - vf.y };

            int32_t const s = tspr->owner;
            int32_t walldist = 1;
            int32_t w = (s == -1) ? -1 : wsprinfo[s].wall;

            // find the wall most likely to be what the sprite is supposed to be ornamented against
            // this is really slow, so cache the result. Also assume that this association never changes once it is set up
            if (s == -1 || !wsprinfo[s].wall)
            {
                w = polymost_findwall(tspr, &tsiz, &walldist);

                if (s != -1)
                {
                    wallspriteinfo_t *ws = &wsprinfo[s];
                    ws->wall = w;

                    if (w != -1)
                    {
                        ws->wdist = walldist;
                    }
                }
            }
            else if (s != -1)
                walldist = wsprinfo[s].wdist;

            // detect if the sprite is either on the wall line or the wall line and sprite intersect
            if (w != -1)
            {
                vec2_t v = { /*Blrintf(vf.x)*/(int)vf.x, /*Blrintf(vf.y)*/(int)vf.y };

                if (walldist <= 2 || ((pos.x - v.x) + (pos.x + v.x)) == (wall[w].x + POINT2(w).x) ||
                    ((pos.y - v.y) + (pos.y + v.y)) == (wall[w].y + POINT2(w).y) ||
                    polymost_lintersect(pos.x - v.x, pos.y - v.y, pos.x + v.x, pos.y + v.y, wall[w].x, wall[w].y,
                                        POINT2(w).x, POINT2(w).y))
                {
                    int32_t const ang = getangle(wall[w].x - POINT2(w).x, wall[w].y - POINT2(w).y);
                    float const foffs = TSPR_OFFSET(tspr);
                    vec2f_t const offs = { (float)(sintable[(ang + 1024) & 2047] >> 6) * foffs,
                                     (float)(sintable[(ang + 512) & 2047] >> 6) * foffs};

                    vec0.x -= offs.x;
                    vec0.y -= offs.y;
                }
            }

            vec2f_t p0 = { vec0.y * gcosang - vec0.x * gsinang,
                           vec0.x * gcosang2 + vec0.y * gsinang2 };

            vec2f_t const pp = { extent.x * ftsiz.x + vec0.x,
                                 extent.y * ftsiz.x + vec0.y };

            vec2f_t p1 = { pp.y * gcosang - pp.x * gsinang,
                           pp.x * gcosang2 + pp.y * gsinang2 };

            if ((p0.y <= SCISDIST) && (p1.y <= SCISDIST))
                goto _drawsprite_return;

            // Clip to close parallel-screen plane
            vec2f_t const op0 = p0;

            float t0 = 0.f, t1 = 1.f;

            if (p0.y < SCISDIST)
            {
                t0 = (SCISDIST - p0.y) / (p1.y - p0.y);
                p0 = { (p1.x - p0.x) * t0 + p0.x, SCISDIST };
            }

            if (p1.y < SCISDIST)
            {
                t1 = (SCISDIST - op0.y) / (p1.y - op0.y);
                p1 = { (p1.x - op0.x) * t1 + op0.x, SCISDIST };
            }

            f = 1.f / p0.y;
            const float ryp0 = f * gyxscale;
            float sx0 = ghalfx * p0.x * f + ghalfx;

            f = 1.f / p1.y;
            const float ryp1 = f * gyxscale;
            float sx1 = ghalfx * p1.x * f + ghalfx;

            pos.z -= ((off.y * tspr->yrepeat) << 2);

            if (globalorientation & 128)
            {
                pos.z += ((tsiz.y * tspr->yrepeat) << 1);

                if (tsiz.y & 1)
                    pos.z += (tspr->yrepeat << 1);  // Odd yspans
            }

            xtex.d = (ryp0 - ryp1) * gxyaspect / (sx0 - sx1);
            ytex.d = 0;
            otex.d = ryp0 * gxyaspect - xtex.d * sx0;

            if (globalorientation & 4)
            {
                t0 = 1.f - t0;
                t1 = 1.f - t1;
            }

            // sprite panning
            if (spriteext[spritenum].xpanning)
            {
                float const xpan = ((float)(spriteext[spritenum].xpanning) * (1.0f / 255.f));
                t0 -= xpan;
                t1 -= xpan;
                drawpoly_srepeat = 1;
            }

            xtex.u = (t0 * ryp0 - t1 * ryp1) * gxyaspect * ftsiz.x / (sx0 - sx1);
            ytex.u = 0;
            otex.u = t0 * ryp0 * gxyaspect * ftsiz.x - xtex.u * sx0;

            f = ((float) tspr->yrepeat) * ftsiz.y * 4;

            float sc0 = ((float) (pos.z - globalposz - f)) * ryp0 + ghoriz;
            float sc1 = ((float) (pos.z - globalposz - f)) * ryp1 + ghoriz;
            float sf0 = ((float) (pos.z - globalposz)) * ryp0 + ghoriz;
            float sf1 = ((float) (pos.z - globalposz)) * ryp1 + ghoriz;

            // gvx*sx0 + gvy*sc0 + gvo = 0
            // gvx*sx1 + gvy*sc1 + gvo = 0
            // gvx*sx0 + gvy*sf0 + gvo = tsizy*(gdx*sx0 + gdo)
            f = ftsiz.y * (xtex.d * sx0 + otex.d) / ((sx0 - sx1) * (sc0 - sf0));

            if (!(globalorientation & 8))
            {
                xtex.v = (sc0 - sc1) * f;
                ytex.v = (sx1 - sx0) * f;
                otex.v = -xtex.v * sx0 - ytex.v * sc0;
            }
            else
            {
                xtex.v = (sf1 - sf0) * f;
                ytex.v = (sx0 - sx1) * f;
                otex.v = -xtex.v * sx0 - ytex.v * sf0;
            }

            // sprite panning
            if (spriteext[spritenum].ypanning)
            {
                float const ypan = ((float)(spriteext[spritenum].ypanning) * (1.0f / 255.f)) * ftsiz.y;
                xtex.v -= xtex.d * ypan;
                ytex.v -= ytex.d * ypan;
                otex.v -= otex.d * ypan;
                drawpoly_trepeat = 1;
            }

            // Clip sprites to ceilings/floors when no parallaxing
            if (!(sector[tspr->sectnum].ceilingstat & 1))
            {
                if (sector[tspr->sectnum].ceilingz > pos.z - (float)((tspr->yrepeat * tsiz.y) << 2))
                {
                    sc0 = (float)(sector[tspr->sectnum].ceilingz - globalposz) * ryp0 + ghoriz;
                    sc1 = (float)(sector[tspr->sectnum].ceilingz - globalposz) * ryp1 + ghoriz;
                }
            }
            if (!(sector[tspr->sectnum].floorstat & 1))
            {
                if (sector[tspr->sectnum].floorz < pos.z)
                {
                    sf0 = (float)(sector[tspr->sectnum].floorz - globalposz) * ryp0 + ghoriz;
                    sf1 = (float)(sector[tspr->sectnum].floorz - globalposz) * ryp1 + ghoriz;
                }
            }

            if (sx0 > sx1)
            {
                if (globalorientation & 64)
                    goto _drawsprite_return;  // 1-sided sprite

                swapfloat(&sx0, &sx1);
                swapfloat(&sc0, &sc1);
                swapfloat(&sf0, &sf1);
            }

            vec2f_t const pxy[4] = { { sx0, sc0 }, { sx1, sc1 }, { sx1, sf1 }, { sx0, sf0 } };

			vec2_16_t tempsiz = { (int16_t)tsiz.x, (int16_t)tsiz.y };
			pow2xsplit = 0;
            polymost_drawpoly(pxy, 4, method, tempsiz);

            drawpoly_srepeat = 0;
            drawpoly_trepeat = 0;
        }
        break;

        case 2:  // Floor sprite
            GLInterface.SetVisibility(sectorVisibility(tspr->sectnum) * (4.f/5.f)); // No idea why this uses a different visibility setting...

            if ((globalorientation & 64) != 0 && (globalposz > pos.z) == (!(globalorientation & 8)))
                goto _drawsprite_return;
            else
            {
                if ((globalorientation & 4) > 0)
                    off.x = -off.x;
                if ((globalorientation & 8) > 0)
                    off.y = -off.y;

                vec2f_t const p0 = { (float)(((tsiz.x + 1) >> 1) - off.x) * tspr->xrepeat,
                                     (float)(((tsiz.y + 1) >> 1) - off.y) * tspr->yrepeat },
                              p1 = { (float)((tsiz.x >> 1) + off.x) * tspr->xrepeat,
                                     (float)((tsiz.y >> 1) + off.y) * tspr->yrepeat };

                float const c = sintable[(tspr->ang + 512) & 2047] * (1.0f / 65536.f);
                float const s = sintable[tspr->ang & 2047] * (1.0f / 65536.f);

                vec2f_t pxy[6];

                // Project 3D to 2D
                for (bssize_t j = 0; j < 4; j++)
                {
                    vec2f_t s0 = { (float)(tspr->x - globalposx), (float)(tspr->y - globalposy) };

                    if ((j + 0) & 2)
                    {
                        s0.y -= s * p0.y;
                        s0.x -= c * p0.y;
                    }
                    else
                    {
                        s0.y += s * p1.y;
                        s0.x += c * p1.y;
                    }
                    if ((j + 1) & 2)
                    {
                        s0.x -= s * p0.x;
                        s0.y += c * p0.x;
                    }
                    else
                    {
                        s0.x += s * p1.x;
                        s0.y -= c * p1.x;
                    }

                    pxy[j] = { s0.y * gcosang - s0.x * gsinang, s0.x * gcosang2 + s0.y * gsinang2 };
                }

                if (pos.z < globalposz)  // if floor sprite is above you, reverse order of points
                {
                    EDUKE32_STATIC_ASSERT(sizeof(uint64_t) == sizeof(vec2f_t));

                    swap64bit(&pxy[0], &pxy[1]);
                    swap64bit(&pxy[2], &pxy[3]);
                }

                // Clip to SCISDIST plane
                int32_t npoints = 0;
                vec2f_t p2[6];

                for (bssize_t i = 0, j = 1; i < 4; j = ((++i + 1) & 3))
                {
                    if (pxy[i].y >= SCISDIST)
                        p2[npoints++] = pxy[i];

                    if ((pxy[i].y >= SCISDIST) != (pxy[j].y >= SCISDIST))
                    {
                        float const f = (SCISDIST - pxy[i].y) / (pxy[j].y - pxy[i].y);
                        vec2f_t const t = { (pxy[j].x - pxy[i].x) * f + pxy[i].x,
                                            (pxy[j].y - pxy[i].y) * f + pxy[i].y };
                        p2[npoints++] = t;
                    }
                }

                if (npoints < 3)
                    goto _drawsprite_return;

                // Project rotated 3D points to screen

                int fadjust = 0;

                // unfortunately, offsetting by only 1 isn't enough on most Android devices
                if (pos.z == sec->ceilingz || pos.z == sec->ceilingz + 1)
                    pos.z = sec->ceilingz + 2, fadjust = (tspr->owner & 31);

                if (pos.z == sec->floorz || pos.z == sec->floorz - 1)
                    pos.z = sec->floorz - 2, fadjust = -((tspr->owner & 31));

                float f = (float)(pos.z - globalposz + fadjust) * gyxscale;

                for (bssize_t j = 0; j < npoints; j++)
                {
                    float const ryp0 = 1.f / p2[j].y;
                    pxy[j] = { ghalfx * p2[j].x * ryp0 + ghalfx, f * ryp0 + ghoriz };
                }

                // gd? Copied from floor rendering code

                xtex.d = 0;
                ytex.d = gxyaspect / (double)(pos.z - globalposz + fadjust);
                otex.d = -ghoriz * ytex.d;

                // copied&modified from relative alignment
                vec2f_t const vv = { (float)tspr->x + s * p1.x + c * p1.y, (float)tspr->y + s * p1.y - c * p1.x };
                vec2f_t ff = { -(p0.x + p1.x) * s, (p0.x + p1.x) * c };

                f = polymost_invsqrt_approximation(ff.x * ff.x + ff.y * ff.y);

                ff.x *= f;
                ff.y *= f;

                float const ft[4] = { ((float)(globalposy - vv.y)) * ff.y + ((float)(globalposx - vv.x)) * ff.x,
                                      ((float)(globalposx - vv.x)) * ff.y - ((float)(globalposy - vv.y)) * ff.x,
                                      fsinglobalang * ff.y + fcosglobalang * ff.x,
                                      fsinglobalang * ff.x - fcosglobalang * ff.y };

                f = fviewingrange * -(1.f / (65536.f * 262144.f));
                xtex.u = (float)ft[3] * f;
                xtex.v = (float)ft[2] * f;
                ytex.u = ft[0] * ytex.d;
                ytex.v = ft[1] * ytex.d;
                otex.u = ft[0] * otex.d;
                otex.v = ft[1] * otex.d;
                otex.u += (ft[2] * (1.0f / 262144.f) - xtex.u) * ghalfx;
                otex.v -= (ft[3] * (1.0f / 262144.f) + xtex.v) * ghalfx;

                f = 4.f / (float)tspr->xrepeat;
                xtex.u *= f;
                ytex.u *= f;
                otex.u *= f;

                f = -4.f / (float)tspr->yrepeat;
                xtex.v *= f;
                ytex.v *= f;
                otex.v *= f;

                if (globalorientation & 4)
                {
                    xtex.u = ftsiz.x * xtex.d - xtex.u;
                    ytex.u = ftsiz.x * ytex.d - ytex.u;
                    otex.u = ftsiz.x * otex.d - otex.u;
                }

                // sprite panning
                if (spriteext[spritenum].xpanning)
                {
                    float const f = ((float)(spriteext[spritenum].xpanning) * (1.0f / 255.f)) * ftsiz.x;
                    ytex.u -= ytex.d * f;
                    otex.u -= otex.d * f;
                    drawpoly_srepeat = 1;
                }

                if (spriteext[spritenum].ypanning)
                {
                    float const f = ((float)(spriteext[spritenum].ypanning) * (1.0f / 255.f)) * ftsiz.y;
                    ytex.v -= ytex.d * f;
                    otex.v -= otex.d * f;
                    drawpoly_trepeat = 1;
                }

				vec2_16_t tempsiz = { (int16_t)tsiz.x, (int16_t)tsiz.y };
				pow2xsplit = 0;

                polymost_drawpoly(pxy, npoints, method, tempsiz);

                drawpoly_srepeat = 0;
                drawpoly_trepeat = 0;
            }

            break;

        case 3:  // Voxel sprite
            break;
    }

    if (automapping == 1 && (unsigned)spritenum < MAXSPRITES)
        show2dsprite[spritenum>>3] |= pow2char[spritenum&7];

_drawsprite_return:
    ;
}

EDUKE32_STATIC_ASSERT((int)RS_YFLIP == (int)HUDFLAG_FLIPPED);

void polymost_initosdfuncs(void)
{
}

void polymost_precache(int32_t dapicnum, int32_t dapalnum, int32_t datype)
{
    // dapicnum and dapalnum are like you'd expect
    // datype is 0 for a wall/floor/ceiling and 1 for a sprite
    //    basically this just means walls are repeating
    //    while sprites are clamped

    if (videoGetRenderMode() < REND_POLYMOST) return;
   if ((dapalnum < (MAXPALOOKUPS - RESERVEDPALS)) && (!lookups.checkTable(dapalnum))) return;//dapalnum = 0;

    //Printf("precached %d %d type %d\n", dapicnum, dapalnum, datype);
    hicprecaching = 1;
    int palid = TRANSLATION(Translation_Remap + curbasepal, dapalnum);
    GLInterface.SetTexture(dapicnum, tileGetTexture(dapicnum), palid, CLAMP_NONE);
    hicprecaching = 0;

    if (datype == 0 || !hw_models) return;

    int const mid = md_tilehasmodel(dapicnum, dapalnum);

    if (mid < 0 || models[mid]->mdnum < 2) return;

    int const surfaces = (models[mid]->mdnum == 3) ? ((md3model_t *)models[mid])->head.numsurfs : 0;

    for (int i = 0; i <= surfaces; i++)
	{
        auto tex = mdloadskin((md2model_t *)models[mid], 0, dapalnum, i, nullptr);
        int palid = TRANSLATION(Translation_Remap + curbasepal, dapalnum);
        if (tex) GLInterface.SetTexture(-1, tex, palid, CLAMP_NONE);
	}
}

void PrecacheHardwareTextures(int nTile)
{
	// PRECACHE
	// This really *really* needs improvement on the game side - the entire precaching logic has no clue about the different needs of a hardware renderer.
	polymost_precache(nTile, 0, 1);
}

extern char* voxfilenames[MAXVOXELS];
void (*PolymostProcessVoxels_Callback)(void) = NULL;
static void PolymostProcessVoxels(void)
{
    if (PolymostProcessVoxels_Callback)
        PolymostProcessVoxels_Callback();

    if (g_haveVoxels != 1)
        return;

    g_haveVoxels = 2;

    Printf("Generating voxel models for Polymost. This may take a while...\n");
    //videoNextPage();

    for (bssize_t i = 0; i < MAXVOXELS; i++)
    {
        if (voxfilenames[i])
        {
            voxmodels[i] = voxload(voxfilenames[i]);
            voxmodels[i]->scale = voxscale[i] * (1.f / 65536.f);
            DO_FREE_AND_NULL(voxfilenames[i]);
        }
    }
}

void Polymost_Startup()
{
    PolymostProcessVoxels();
}
