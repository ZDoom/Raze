#pragma once

#include <atomic>
#include <functional>
#include "build.h"
#include "vectors.h"
#include "hw_viewpointuniforms.h"
#include "v_video.h"
#include "hw_drawlist.h"
#include "hw_bunchdrawer.h"
//#include "r_viewpoint.h"

EXTERN_CVAR(Int, hw_lightmode)

enum EDrawMode
{
	DM_MAINVIEW,
	DM_OFFSCREEN,
	DM_PORTAL,
	DM_SKYPORTAL
};

struct FSectorPortalGroup;
struct FFlatVertex;
class HWWall;
class HWFlat;
class HWSprite;
class IShadowMap;
struct FDynLightData;
class Clipper;
class HWPortal;
class FFlatVertexBuffer;
class IRenderQueue;
class HWScenePortalBase;
class FRenderState;

struct FRenderViewpoint
{
	DCoreActor* CameraActor;
	DVector3 Pos;
	FRotator HWAngles;
	FAngle FieldOfView;
	angle_t RotAngle;
	int* SectNums;
	int SectCount;
	double TicFrac;
	double TanCos, TanSin;	// needed for calculating a sprite's screen depth.
	DVector2 ViewVector;		// direction the camera is facing. 
};
//==========================================================================
//
// these are used to link faked planes due to missing textures to a sector
//
//==========================================================================

enum SectorRenderFlags
{
	// This is used to merge several subsectors into a single draw item
	SSRF_RENDERFLOOR = 1,
	SSRF_RENDERCEILING = 2,
	SSRF_RENDERALL = 7,
	SSRF_PROCESSED = 8,
	SSRF_SEEN = 16,
};

enum EPortalClip
{
	PClip_InFront,
	PClip_Inside,
	PClip_Behind,
};

enum DrawListType
{
	GLDL_PLAINWALLS,
	GLDL_PLAINFLATS,
	GLDL_MASKEDWALLS,
	GLDL_MASKEDWALLSS,	// arbitrary wall sprites, not attached to walls
	GLDL_MASKEDWALLSD,	// arbitrary wall sprites, attached to walls.
	GLDL_MASKEDWALLSV,	// vertical wall sprites
	GLDL_MASKEDWALLSH,  // horizontal wall sprites. These two lists merely exist for easier sorting.
	GLDL_MASKEDFLATS,
	GLDL_MASKEDSLOPEFLATS,
	GLDL_MODELS,

	GLDL_TRANSLUCENT,
	GLDL_TRANSLUCENTBORDER,

	GLDL_TYPES,
};


struct HWDrawInfo
{
	struct wallseg
	{
		float x1, y1, z1, x2, y2, z2;
	};

	HWDrawList drawlists[GLDL_TYPES];
	int vpIndex;
	//ELightMode lightmode;

	HWDrawInfo * outer = nullptr;
	int FullbrightFlags;
	std::atomic<int> spriteindex;
	HWPortal *mClipPortal;
	HWPortal *mCurrentPortal;
	//FRotator mAngles;
	BunchDrawer mDrawer;
	Clipper *mClipper;
	FRenderViewpoint Viewpoint;
	HWViewpointUniforms VPUniforms;	// per-viewpoint uniform state
	TArray<HWPortal *> Portals;
	tspritetype tsprite[MAXSPRITESONSCREEN];
	int spritesortcnt;
	TArray<FFlatVertex> SlopeSpriteVertices;	// we need to cache these in system memory in case of translucency splits.

	// This is needed by the BSP traverser.
	bool multithread;
	bool ingeo;
	FVector2 geoofs;

	int rellight;
	float visibility;

private:
	bool inview;
	sectortype *currentsector;

	void WorkerThread();

	void UnclipSubsector(sectortype *sub);

	void AddLine(walltype *seg, bool portalclip);
	void AddLines(sectortype* sector);
	void AddSpecialPortalLines(sectortype * sector, walltype* line);
	public:
	//void RenderThings(sectortype * sub, sectortype * sector);
	//void RenderParticles(sectortype *sub, sectortype *front);
	void SetColor(FRenderState &state, int sectorlightlevel, int rellight, bool fullbright, const FColormap &cm, float alpha, bool weapon = false);
	int CalcLightLevel(int lightlevel, int rellight, bool weapon, int blendfactor);
	PalEntry CalcLightColor(int light, PalEntry pe, int blendfactor);
	void SetShaderLight(FRenderState& state, float level, float olight);
	void SetFog(FRenderState& state, int lightlevel, float visibility, bool fullbright, const FColormap* cmap, bool isadditive);

	float GetFogDensity(int lightlevel, PalEntry fogcolor, int sectorfogdensity, int blendfactor);

public:

	void SetCameraPos(const DVector3 &pos)
	{
		VPUniforms.mCameraPos = { (float)pos.X, (float)pos.Z, (float)pos.Y, 0.f };
	}

	void SetClipHeight(float h, float d)
	{
		VPUniforms.mClipHeight = h;
		VPUniforms.mClipHeightDirection = d;
		VPUniforms.mClipLine.X = -1000001.f;
	}

	void SetClipLine(walltype *line)
	{
		//VPUniforms.mClipLine = { (float)line->v1->fX(), (float)line->v1->fY(), (float)line->Delta().X, (float)line->Delta().Y };
		VPUniforms.mClipHeight = 0;
	}

	HWPortal * FindPortal(const void * src);

	static HWDrawInfo *StartDrawInfo(HWDrawInfo *parent, FRenderViewpoint &parentvp, HWViewpointUniforms *uniforms);
	void StartScene(FRenderViewpoint &parentvp, HWViewpointUniforms *uniforms);
	void ClearBuffers();
	HWDrawInfo *EndDrawInfo();

	void DrawScene(int drawmode, bool portal);
	void CreateScene(bool portal);
	void DispatchSprites();
	void RenderScene(FRenderState &state);
	void RenderTranslucent(FRenderState &state);
	void RenderPortal(HWPortal *p, FRenderState &state, bool usestencil);
	void EndDrawScene(FRenderState &state);
	void Set3DViewport(FRenderState &state);
	void ProcessScene(bool toscreen);
	void SetVisibility();

	//void GetDynSpriteLight(AActor *self, float x, float y, float z, FLightNode *node, int portalgroup, float *out);
	//void GetDynSpriteLight(AActor *thing, particle_t *particle, float *out);

	void SetViewMatrix(const FRotator &angles, float vx, float vy, float vz, bool mirror, bool planemirror);
	void SetupView(FRenderState &state, float vx, float vy, float vz, bool mirror, bool planemirror);
	angle_t FrustumAngle();

	void DrawPlayerSprites(bool hudModelStep, FRenderState &state);

	//void AddSubsectorToPortal(FSectorPortalGroup *portal, sectortype *sub);

	void AddWall(HWWall *w);
	void AddMirrorSurface(HWWall *w);
	void AddFlat(HWFlat *flat);
	void AddSprite(HWSprite *sprite, bool translucent);


	bool isBuildSoftwareLighting() const
	{
		return hw_lightmode == 0 || hw_int_useindexedcolortextures;
	}

	bool isDarkLightMode() const
	{
		return hw_lightmode == 2;
	}
};

void CleanSWDrawer();

inline int hw_ClampLight(int lightlevel)
{
	return clamp(lightlevel, 0, 255);
}


