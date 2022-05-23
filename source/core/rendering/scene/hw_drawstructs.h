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
#include "matrix.h"
#include "gamecontrol.h"
#include "hw_renderstate.h"
#include "hw_cvars.h"
#include "coreactor.h"

#ifdef _MSC_VER
#pragma warning(disable:4244) // this gets a bit annoying in the renderer...
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
struct voxmodel_t;
struct Section;

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
	};

	enum
	{
		RWF_BLANK = 0,
		RWF_TEXTURED = 1,	// actually not being used anymore because with buffers it's even less efficient not writing the texture coordinates - but leave it here
		RWF_TRANS = 2,
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
	walltype* walldist;
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
		const int * planemirror;	// for plane mirrors
	};

	unsigned int vertindex;
	unsigned int vertcount;

public:
	walltype* seg;
	tspritetype* Sprite;
	sectortype* frontsector, * backsector;
//private:

	void PutWall(HWDrawInfo *di, bool translucent);
	void PutPortal(HWDrawInfo *di, int ptype, int plane);
	void CheckTexturePosition();

	void SetupLights(HWDrawInfo *di, FDynLightData &lightdata);

	void MakeVertices(HWDrawInfo *di, bool nosplit);

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

	int CountVertices();
	void SplitLeftEdge(FFlatVertex*& ptr);
	void SplitRightEdge(FFlatVertex*& ptr);
	void CountLeftEdge(unsigned& ptr);
	void CountRightEdge(unsigned& ptr);

	void RenderWall(HWDrawInfo *di, FRenderState &state, int textured);
	void RenderFogBoundary(HWDrawInfo *di, FRenderState &state);
	void RenderMirrorSurface(HWDrawInfo *di, FRenderState &state);
	void RenderTexturedWall(HWDrawInfo *di, FRenderState &state, int rflags);
	void RenderTranslucentWall(HWDrawInfo *di, FRenderState &state);
	int CheckWallSprite(tspritetype* spr, tspritetype* last);

public:
	void Process(HWDrawInfo* di, walltype* seg, sectortype* frontsector, sectortype* backsector);
	void ProcessWallSprite(HWDrawInfo* di, tspritetype* spr, sectortype* frontsector);

	float PointOnSide(float x,float y)
	{
		return -((y-glseg.y1)*(glseg.x2-glseg.x1)-(x-glseg.x1)*(glseg.y2-glseg.y1));
	}

	void DrawWall(HWDrawInfo *di, FRenderState &state, bool translucent);

};

//==========================================================================
//
// Common fields needed by the sprite sorter.
//
//==========================================================================

class HWFlatOrSprite
{
public:
	float depth;
	tspritetype* Sprite; // for flat sprites.
};

//==========================================================================
//
// One flat plane in the draw list
//
//==========================================================================

class HWFlat : public HWFlatOrSprite
{
public:
	sectortype* sec;
	FGameTexture* texture;

	int section;
	float z; // the z position of the flat (only valid for non-sloped planes)

	PalEntry fade;
	int shade, palette;
	float visibility;
	float alpha;
	FRenderStyle RenderStyle;
	int iboindex;
	bool stack;
	uint8_t plane;
	short slopecount;
	FVector2 geoofs;
	FVector3 normal;
	//int vboheight;

	int slopeindex;
	int vertindex, vertcount;	// this should later use a static vertex buffer, but that'd hinder the development phase, so for now vertex data gets created on the fly.
	void MakeVertices(HWDrawInfo* di);

	int dynlightindex;

	void CreateSkyboxVertices(FFlatVertex *buffer);
	//void SetupLights(HWDrawInfo *di, FLightNode *head, FDynLightData &lightdata, int portalgroup);

	void PutFlat(HWDrawInfo* di, int whichplane);
	void ProcessSector(HWDrawInfo *di, sectortype * frontsector, int sectionnum, int which = 7 /*SSRF_RENDERALL*/);	// cannot use constant due to circular dependencies.
	void ProcessFlatSprite(HWDrawInfo* di, tspritetype* sprite, sectortype* sector);

	void DrawSubsectors(HWDrawInfo *di, FRenderState &state);
	void DrawFlat(HWDrawInfo* di, FRenderState& state, bool translucent);
};

//==========================================================================
//
// One sprite in the draw list
//
//==========================================================================


class HWSprite : public HWFlatOrSprite
{
public:

	PalEntry fade;
	int shade, palette;
	float visibility;
	float alpha;
	FRenderStyle RenderStyle;
	int modelframe; // : sprite, 1: model, -1:voxel
	voxmodel_t* voxel;

	int index;
	int vertexindex;

	float x,y,z;	// needed for sorting!

	union
	{
		struct
		{
			float ul, ur;
			float vt, vb;
			float x1, y1, z1;
			float x2, y2, z2;
		};
		VSMatrix rotmat;
	};
	int dynlightindex;
	float alphaThreshold;

	FGameTexture *texture;
	DRotator Angles;


	void CalculateVertices(HWDrawInfo *di, FVector3 *v, DVector3 *vp);

public:

	void CreateVertices(HWDrawInfo* di);
	void PutSprite(HWDrawInfo *di, bool translucent);
	void Process(HWDrawInfo *di, tspritetype* thing,sectortype * sector, int thruportal = false);
	bool ProcessVoxel(HWDrawInfo* di, voxmodel_t* voxel, tspritetype* tspr, sectortype* sector, bool rotate);

	void DrawSprite(HWDrawInfo* di, FRenderState& state, bool translucent);
};



inline float Dist2(float x1,float y1,float x2,float y2)
{
	return sqrtf((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

//void hw_GetDynModelLight(AActor *self, FDynLightData &modellightdata);

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

inline const float hw_density = 0.35f;

int checkTranslucentReplacement(FTextureID picnum, int pal);

inline bool maskWallHasTranslucency(const walltype* wall)
{
	return (wall->cstat & CSTAT_WALL_TRANSLUCENT) || checkTranslucentReplacement(tileGetTexture(wall->picnum)->GetID(), wall->pal);
}

inline bool spriteHasTranslucency(const tspritetype* tspr)
{
	if ((tspr->cstat & CSTAT_SPRITE_TRANSLUCENT) || //(tspr->clipdist & TSPR_FLAGS_DRAW_LAST) ||
		(tspr->ownerActor->sprext.alpha))
		return true;

	return checkTranslucentReplacement(tileGetTexture(tspr->picnum)->GetID(), tspr->pal);
}

inline void SetSpriteTranslucency(const tspritetype* sprite, float& alpha, FRenderStyle& RenderStyle)
{
	bool trans = (sprite->cstat & CSTAT_SPRITE_TRANSLUCENT);
	if (trans)
	{
		RenderStyle = GetRenderStyle(0, !!(sprite->cstat & CSTAT_SPRITE_TRANS_FLIP));
		alpha = GetAlphaFromBlend((sprite->cstat & CSTAT_SPRITE_TRANS_FLIP) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
	}
	else
	{
		RenderStyle = LegacyRenderStyles[STYLE_Translucent];
		alpha = 1.f;
	}
	alpha *= 1.f - sprite->ownerActor->sprext.alpha;
}

extern PalEntry GlobalMapFog;
extern float GlobalFogDensity;

void SetLightAndFog(HWDrawInfo* di, FRenderState& state, PalEntry fade, int palette, int shade, float visibility, float alpha);
