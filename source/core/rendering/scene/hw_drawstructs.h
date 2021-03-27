#pragma once
//==========================================================================
//
// One wall segment in the draw list
//
//==========================================================================
#include "renderstyle.h"
#include "textures.h"
#include "fcolormap.h"
#include "build.h"
#include "gamefuncs.h"
#include "render.h"

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

struct HWHorizonInfo;
struct HWSkyInfo;
class FMaterial;
struct FTexCoordInfo;
struct FSectorPortalGroup;
struct FFlatVertex;
struct FDynLightData;
class VSMatrix;
struct FSpriteModelFrame;
class FRenderState;

struct HWSectorPlane
{
	FTextureID texture;
	//secplane_t plane;
	float Texheight;
	float	Angle;
	FVector2 Offs;
	FVector2 Scale;

	void GetFromSector(sectortype* sec, int ceiling)
	{
		/*
		Offs.X = (float)sec->GetXOffset(ceiling);
		Offs.Y = (float)sec->GetYOffset(ceiling);
		Scale.X = (float)sec->GetXScale(ceiling);
		Scale.Y = (float)sec->GetYScale(ceiling);
		Angle = (float)sec->GetAngle(ceiling).Degrees;
		texture = sec->GetTexture(ceiling);
		plane = sec->GetSecPlane(ceiling);
		Texheight = (float)((ceiling == sector_t::ceiling) ? plane.fD() : -plane.fD());
		*/
	}
};

enum HWRenderStyle
{
	STYLEHW_Normal,			// default
	STYLEHW_Solid,			// drawn solid (needs special treatment for sprites)
	STYLEHW_NoAlphaTest,	// disable alpha test
};

enum WallTypes
{
	RENDERWALL_NONE,
	RENDERWALL_TOP,
	RENDERWALL_M1S,
	RENDERWALL_M2S,
	RENDERWALL_BOTTOM,
	RENDERWALL_FOGBOUNDARY,
	RENDERWALL_MIRRORSURFACE,
	RENDERWALL_M2SNF,
	RENDERWALL_COLOR,
	// Insert new types at the end!
};

enum PortalTypes
{
	PORTALTYPE_SKY,
	PORTALTYPE_HORIZON,
	PORTALTYPE_SKYBOX,
	PORTALTYPE_SECTORSTACK,
	PORTALTYPE_PLANEMIRROR,
	PORTALTYPE_MIRROR,
	PORTALTYPE_LINETOLINE,
	PORTALTYPE_LINETOSPRITE,
};

//==========================================================================
//
// One sector plane, still in fixed point
//
//==========================================================================

struct HWSeg
{
	float x1,x2;
	float y1,y2;
	float fracleft, fracright;

	FVector3 Normal() const 
	{
		// we do not use the vector math inlines here because they are not optimized for speed but accuracy in the playsim and this is called quite frequently.
		float x = y2 - y1;
		float y = x1 - x2;
		float ilength = 1.f / sqrtf(x*x + y*y);
		return FVector3(x * ilength, 0, y * ilength);
	}
};

struct texcoord
{
	float u,v;
};

struct HWDrawInfo;

class HWWall
{
public:
	static const char passflag[];

	enum
	{
		HWF_CLAMPX=1,
		HWF_CLAMPY=2,
		HWF_SKYHACK=4,
		HWF_NOSPLIT=64,
		HWF_TRANSLUCENT = 128,
		HWF_NOSLICE = 256
	};

	enum
	{
		RWF_BLANK = 0,
		RWF_TEXTURED = 1,	// actually not being used anymore because with buffers it's even less efficient not writing the texture coordinates - but leave it here
		RWF_NORENDER = 8,
	};

	enum
	{
		LOLFT,
		UPLFT,
		UPRGT,
		LORGT,
	};

	friend struct HWDrawList;
	friend class HWPortal;

	FGameTexture *texture;

	HWSeg glseg;
	float ztop[2],zbottom[2];
	texcoord tcs[4];
	float alpha;

	FRenderStyle RenderStyle;
	
	float ViewDistance;
	float visibility;
	short shade, palette;

	PalEntry fade;

	uint16_t flags;
	uint8_t type;

	int dynlightindex;

	union
	{
		// it's either one of them but never more!
		//FSectorPortal *secportal;	// sector portal (formerly skybox)
		HWSkyInfo * sky;			// for normal sky
		//HWHorizonInfo * horizon;	// for horizon information
		PortalDesc * portal;			// stacked sector portals
		int * planemirror;	// for plane mirrors
		spritetype* teleport;	// SW's teleport-views
	};

	unsigned int vertindex;
	unsigned int vertcount;

public:
	walltype* seg;
	spritetype* sprite;
	sectortype* frontsector, * backsector;
//private:

	void PutWall(HWDrawInfo *di, bool translucent);
	void PutPortal(HWDrawInfo *di, int ptype, int plane);
	void CheckTexturePosition();

	void SetupLights(HWDrawInfo *di, FDynLightData &lightdata);

	void MakeVertices(HWDrawInfo *di, bool nosplit);
	void SetLightAndFog(FRenderState& state);

	void SkyPlane(HWDrawInfo *di, sectortype *sector, int plane, bool allowmirror);
	void SkyLine(HWDrawInfo *di, sectortype *sec, walltype *line);
	void SkyNormal(HWDrawInfo* di, sectortype* fs, FVector2& v1, FVector2& v2, float fch1, float fch2, float ffh1, float ffh2);
	void SkyTop(HWDrawInfo* di, walltype* seg, sectortype* fs, sectortype* bs, FVector2& v1, FVector2& v2, float fch1, float fch2);
	void SkyBottom(HWDrawInfo* di, walltype* seg, sectortype* fs, sectortype* bs, FVector2& v1, FVector2& v2, float ffh1, float ffh2);

	bool DoHorizon(HWDrawInfo *di, walltype * seg,sectortype * fs, DVector2& v1, DVector2& v2);

	bool SetWallCoordinates(walltype* seg, float topleft, float topright, float bottomleft, float bottomright);

	void DoTexture(HWDrawInfo* di, walltype* wal, walltype* refwall, float yref, float topleft, float topright, float bottomleft, float bottomright);
	void DoOneSidedTexture(HWDrawInfo *di, walltype * seg, sectortype* frontsector, sectortype* backsector, float topleft,float topright, float bottomleft,float bottomright);
	void DoUpperTexture(HWDrawInfo* di, walltype* seg, sectortype* frontsector, sectortype* backsector, float topleft, float topright, float bottomleft, float bottomright);
	void DoLowerTexture(HWDrawInfo* di, walltype* seg, sectortype* frontsector, sectortype* backsector, float topleft, float topright, float bottomleft, float bottomright);

	void DoMidTexture(HWDrawInfo *di, walltype * seg,
					  sectortype * front, sectortype * back,
					  float fch1, float fch2, float ffh1, float ffh2,
					  float bch1, float bch2, float bfh1, float bfh2);

	int CreateVertices(FFlatVertex *&ptr, bool nosplit);

	//int CountVertices();

	void RenderWall(HWDrawInfo *di, FRenderState &state, int textured);
	void RenderFogBoundary(HWDrawInfo *di, FRenderState &state);
	void RenderMirrorSurface(HWDrawInfo *di, FRenderState &state);
	void RenderTexturedWall(HWDrawInfo *di, FRenderState &state, int rflags);
	void RenderTranslucentWall(HWDrawInfo *di, FRenderState &state);

public:
	void Process(HWDrawInfo* di, walltype* seg, sectortype* frontsector, sectortype* backsector);
	void ProcessWallSprite(HWDrawInfo* di, spritetype* spr, sectortype* frontsector);

	float PointOnSide(float x,float y)
	{
		return -((y-glseg.y1)*(glseg.x2-glseg.x1)-(x-glseg.x1)*(glseg.y2-glseg.y1));
	}

	void DrawWall(HWDrawInfo *di, FRenderState &state, bool translucent);

};

//==========================================================================
//
// One flat plane in the draw list
//
//==========================================================================

class HWFlat
{
public:
	sectortype * sec;
	spritetype* sprite; // for flat sprites.
	FGameTexture *texture;

	float z; // the z position of the flat (only valid for non-sloped planes)
	FColormap Colormap;	// light and fog

	PalEntry fade;
	int shade, palette, visibility;
	float alpha;
	FRenderStyle RenderStyle;
	int iboindex;
	//int vboheight;

	int plane;
	int vertindex, vertcount;	// this should later use a static vertex buffer, but that'd hinder the development phase, so for now vertex data gets created on the fly.
	void MakeVertices();

	int dynlightindex;

	void CreateSkyboxVertices(FFlatVertex *buffer);
	//void SetupLights(HWDrawInfo *di, FLightNode *head, FDynLightData &lightdata, int portalgroup);

	void PutFlat(HWDrawInfo* di, int whichplane);
	void ProcessSector(HWDrawInfo *di, sectortype * frontsector, int which = 7 /*SSRF_RENDERALL*/);	// cannot use constant due to circular dependencies.
	void ProcessFlatSprite(HWDrawInfo* di, spritetype* sprite, sectortype* sector);
	
	void DrawSubsectors(HWDrawInfo *di, FRenderState &state);
	void DrawFlat(HWDrawInfo* di, FRenderState& state, bool translucent);
};

//==========================================================================
//
// One sprite in the draw list
//
//==========================================================================


class HWSprite
{
public:
	int lightlevel;
	uint8_t foglevel;
	uint8_t hw_styleflags;
	bool fullbright;
	bool polyoffset;
	FColormap Colormap;
	int modeltype;
	FRenderStyle RenderStyle;
	int OverrideShader;

	int translation;
	int index;
	float depth;
	int vertexindex;

	float topclip;
	float bottomclip;

	float x,y,z;	// needed for sorting!

	float ul,ur;
	float vt,vb;
	float x1,y1,z1;
	float x2,y2,z2;
	float trans;
	int dynlightindex;

	FGameTexture *texture;
	spritetype * actor;
	//TArray<lightlist_t> *lightlist;
	DRotator Angles;


	void SplitSprite(HWDrawInfo *di, sectortype * frontsector, bool translucent);
	void PerformSpriteClipAdjustment(AActor *thing, const DVector2 &thingpos, float spriteheight);
	bool CalculateVertices(HWDrawInfo *di, FVector3 *v, DVector3 *vp);

public:

	void CreateVertices(HWDrawInfo* di) {}
	void PutSprite(HWDrawInfo *di, bool translucent);
	void Process(HWDrawInfo *di, AActor* thing,sectortype * sector, int thruportal = false);

	void DrawSprite(HWDrawInfo* di, FRenderState& state, bool translucent) {}
};



inline float Dist2(float x1,float y1,float x2,float y2)
{
	return sqrtf((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

void hw_GetDynModelLight(AActor *self, FDynLightData &modellightdata);

extern const float LARGE_VALUE;

struct FDynLightData;
struct FDynamicLight;
bool GetLight(FDynLightData& dld, int group, Plane& p, FDynamicLight* light, bool checkside);
void AddLightToList(FDynLightData &dld, int group, FDynamicLight* light, bool forceAttenuate);

inline float sectorVisibility(sectortype* sec)
{
	// Beware of wraparound madness...
	int v = sec->visibility;
	return v ? ((uint8_t)(v + 16)) / 16.f : 1.f;
}

