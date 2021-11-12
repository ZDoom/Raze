#pragma once

#include "tarray.h"
#include "hw_drawinfo.h"
#include "hw_drawstructs.h"
#include "hw_renderstate.h"
#include "hw_material.h"
#include "render.h"

class FSkyBox;

enum
{
	plane_floor,
	plane_ceiling
};



struct HWSkyInfo
{
	float x_offset;
	float y_offset;
	float y_scale;
	int shade;
	bool cloudy;
	FGameTexture * texture;
	PalEntry fadecolor;

	bool operator==(const HWSkyInfo & inf)
	{
		return !memcmp(this, &inf, sizeof(*this));
	}
	bool operator!=(const HWSkyInfo & inf)
	{
		return !!memcmp(this, &inf, sizeof(*this));
	}
};

struct HWHorizonInfo
{
	HWSectorPlane plane;
	int lightlevel;
	FColormap colormap;
	PalEntry specialcolor;
};

struct BoundingRect
{
	double left, top, right, bottom;

	BoundingRect() = default;
	BoundingRect(bool)
	{
		setEmpty();
	}

	void setEmpty()
	{
		left = top = FLT_MAX;
		bottom = right = -FLT_MAX;
	}

	bool contains(const BoundingRect& other) const
	{
		return left <= other.left && top <= other.top && right >= other.right && bottom >= other.bottom;
	}

	bool contains(double x, double y) const
	{
		return left <= x && top <= y && right >= x && bottom >= y;
	}

	template <class T>
	bool contains(const T& vec) const
	{
		return left <= vec.X && top <= vec.Y && right >= vec.X && bottom >= vec.Y;
	}

	bool intersects(const BoundingRect& other) const
	{
		return !(other.left > right ||
			other.right < left ||
			other.top > bottom ||
			other.bottom < top);
	}

	void Union(const BoundingRect& other)
	{
		if (other.left < left) left = other.left;
		if (other.right > right) right = other.right;
		if (other.top < top) top = other.top;
		if (other.bottom > bottom) bottom = other.bottom;
	}

	double distanceTo(const BoundingRect& other) const
	{
		if (intersects(other)) return 0;
		return max(min(fabs(left - other.right), fabs(right - other.left)),
			min(fabs(top - other.bottom), fabs(bottom - other.top)));
	}

	void addVertex(double x, double y)
	{
		if (x < left) left = x;
		if (x > right) right = x;
		if (y < top) top = y;
		if (y > bottom) bottom = y;
	}

	bool operator == (const BoundingRect& other) const
	{
		return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
	}

};

struct secplane_t
{
	double a, b, c, d, ic;
};

struct FPortalSceneState;

class HWPortal
{
	friend struct FPortalSceneState;

	enum
	{
		STP_Stencil,
		STP_DepthClear,
		STP_DepthRestore,
		STP_AllInOne
	};

	TArray<unsigned int> mPrimIndices;
	unsigned int mTopCap = ~0u, mBottomCap = ~0u;

	void DrawPortalStencil(FRenderState &state, int pass);

public:
	FPortalSceneState * mState;
	TArray<HWWall> lines;
	BoundingRect boundingBox;
	int planesused = 0;

    HWPortal(FPortalSceneState *s, bool local = false) : mState(s), boundingBox(false)
    {
    }
    virtual ~HWPortal() {}
    virtual int ClipSeg(walltype *seg, const DVector3 &viewpos) { return PClip_Inside; }
    virtual int ClipSector(sectortype *sub) { return PClip_Inside; }
    virtual int ClipPoint(const DVector2 &pos) { return PClip_Inside; }
    virtual walltype *ClipLine() { return nullptr; }
	virtual void * GetSource() const = 0;	// GetSource MUST be implemented!
	virtual const char *GetName() = 0;
	virtual bool AllowSSAO() { return true; }
	virtual bool IsSky() { return false; }
	virtual bool NeedCap() { return true; }
	virtual bool NeedDepthBuffer() { return true; }
	virtual void DrawContents(HWDrawInfo *di, FRenderState &state) = 0;
	virtual void RenderAttached(HWDrawInfo *di) {}
	virtual int GetType() { return -1; }
	void SetupStencil(HWDrawInfo *di, FRenderState &state, bool usestencil);
	void RemoveStencil(HWDrawInfo *di, FRenderState &state, bool usestencil);

	void AddLine(HWWall * l)
	{
		lines.Push(*l);
		boundingBox.addVertex(l->glseg.x1, l->glseg.y1);
		boundingBox.addVertex(l->glseg.x2, l->glseg.y2);
	}


};


struct FPortalSceneState
{
	int MirrorFlag = 0;
	int PlaneMirrorFlag = 0;
	int renderdepth = 0;

	int PlaneMirrorMode = 0;
	bool inskybox = 0;
	bool insectorportal = false;

	UniqueList<HWSkyInfo> UniqueSkies;
	UniqueList<HWHorizonInfo> UniqueHorizons;
	UniqueList<secplane_t> UniquePlaneMirrors;

	int skyboxrecursion = 0;

	void BeginScene()
	{
		UniqueSkies.Clear();
		UniqueHorizons.Clear();
		UniquePlaneMirrors.Clear();
	}

	bool isMirrored() const
	{
		return !!((MirrorFlag ^ PlaneMirrorFlag) & 1);
	}

	void StartFrame();
	bool RenderFirstSkyPortal(int recursion, HWDrawInfo *outer_di, FRenderState &state);
	void EndFrame(HWDrawInfo *outer_di, FRenderState &state);
	void RenderPortal(HWPortal *p, FRenderState &state, bool usestencil, HWDrawInfo *outer_di);
};

extern FPortalSceneState portalState;

    
class HWScenePortalBase : public HWPortal
{
protected:
    HWScenePortalBase(FPortalSceneState *state) : HWPortal(state, false)
    {
        
    }
public:
	void ClearClipper(HWDrawInfo *di, Clipper *clipper);
	virtual bool NeedDepthBuffer() { return true; }
	virtual void DrawContents(HWDrawInfo* di, FRenderState& state);
	virtual bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) = 0;
	virtual void Shutdown(HWDrawInfo *di, FRenderState &rstate) {}
};

struct HWLinePortal : public HWScenePortalBase
{
protected:
	// this must be the same as at the start of line_t, so that we can pass in this structure directly to P_ClipLineToPortal.
	walltype* line;
	/*
	vec2_t	*v1, *v2;	// vertices, from v1 to v2
	DVector2	delta;		// precalculated v2 - v1 for side checking

	angle_t		angv1, angv2;	// for quick comparisons with a line or subsector
	*/

	HWLinePortal(FPortalSceneState *state, walltype *line) : HWScenePortalBase(state)
	{
		this->line = line;
		//v1 = &line->pos;
		//v2 = &wall[line->point2].pos;
		//CalcDelta();
	}

	void CalcDelta()
	{
		//delta = v2->fPos() - v1->fPos();
	}

	int ClipSeg(walltype *seg, const DVector3 &viewpos) override;
	int ClipSector(sectortype *sub) override;
	int ClipPoint(const DVector2 &pos);
	bool NeedCap() override { return false; }
};

struct HWMirrorPortal : public HWLinePortal
{
protected:
	bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) override;
	void Shutdown(HWDrawInfo *di, FRenderState &rstate) override;
	void * GetSource() const override { return line; }
	const char *GetName() override;
	virtual int GetType() { return PORTAL_WALL_MIRROR; }

public:

	HWMirrorPortal(FPortalSceneState *state, walltype * line)
		: HWLinePortal(state, line)
	{
	}
};


struct HWLineToLinePortal : public HWLinePortal
{
	walltype* origin;
protected:
	bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) override;
	virtual void * GetSource() const override { return origin; }
	virtual const char *GetName() override;
	virtual walltype *ClipLine() override { return line; }
	virtual int GetType() { return PORTAL_WALL_VIEW; }

public:

	HWLineToLinePortal(FPortalSceneState *state, walltype* from, walltype *to)
		: HWLinePortal(state, to), origin(from)
	{
	}
};

struct HWLineToSpritePortal : public HWLinePortal
{
	walltype* origin;
	spritetype* camera;
protected:
	bool Setup(HWDrawInfo* di, FRenderState& rstate, Clipper* clipper) override;
	virtual void* GetSource() const override { return origin; }
	virtual const char* GetName() override;
	virtual walltype* ClipLine() override { return line; }
	virtual int GetType() { return PORTAL_WALL_TO_SPRITE; }

public:

	HWLineToSpritePortal(FPortalSceneState* state, walltype* from, spritetype* to)
		: HWLinePortal(state, &wall[numwalls]), origin(from), camera(to)
	{
		// todo: set up two fake walls at the end of the walls array to be used for backside clipping.
		// Not really needed for vanilla support but maybe later for feature enhancement.
	}
};


#if 0
struct HWSkyboxPortal : public HWScenePortalBase
{
	bool oldclamp;
	int old_pm;
	spritetype * portal;

protected:
	bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) override;
	void Shutdown(HWDrawInfo *di, FRenderState &rstate) override;
	virtual void * GetSource() const { return portal; }
	virtual bool IsSky() { return true; }
	virtual const char *GetName();
	virtual bool AllowSSAO() override;

public:


	HWSkyboxPortal(FPortalSceneState *state, spritetype * pt) : HWScenePortalBase(state)
	{
		portal = pt;
	}

};
#endif


struct HWSectorStackPortal : public HWScenePortalBase
{
	TArray<sectortype *> sectors;
	int type = -1;
protected:
	bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) override;
	void Shutdown(HWDrawInfo *di, FRenderState &rstate) override;
	virtual void * GetSource() const { return origin; }
	virtual const char *GetName();
	virtual int GetType() { return PORTAL_SECTOR_CEILING; }
	PortalDesc *origin;

public:

	HWSectorStackPortal(FPortalSceneState *state, PortalDesc *pt) : HWScenePortalBase(state)
	{
		origin = pt;
	}
	//void SetupCoverage(HWDrawInfo *di);

};

#if 0
struct HWPlaneMirrorPortal : public HWScenePortalBase
{
	int old_pm;
protected:
	bool Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper) override;
	void Shutdown(HWDrawInfo *di, FRenderState &rstate) override;
	virtual void * GetSource() const { return origin; }
	virtual const char *GetName();
	secplane_t * origin;

public:

	HWPlaneMirrorPortal(FPortalSceneState *state, secplane_t * pt) : HWScenePortalBase(state)
	{
		origin = pt;
	}

};
#endif

#if 0
struct HWHorizonPortal : public HWPortal
{
	HWHorizonInfo * origin;
	unsigned int voffset;
	unsigned int vcount;
	friend struct HWEEHorizonPortal;

protected:
	virtual void DrawContents(HWDrawInfo *di, FRenderState &state);
	virtual void * GetSource() const { return origin; }
	virtual bool NeedDepthBuffer() { return false; }
	virtual bool NeedCap() { return false; }
	virtual const char *GetName();

public:
	
	HWHorizonPortal(FPortalSceneState *state, HWHorizonInfo * pt, FRenderViewpoint &vp, bool local = false);
};
#endif

struct HWSkyPortal : public HWPortal
{
	HWSkyInfo * origin;
	FSkyVertexBuffer *vertexBuffer;
	friend struct HWEEHorizonPortal;

protected:
	virtual void DrawContents(HWDrawInfo *di, FRenderState &state);
	virtual void * GetSource() const { return origin; }
	virtual bool IsSky() { return true; }
	virtual bool NeedDepthBuffer() { return false; }
	virtual const char *GetName();

public:


	HWSkyPortal(FSkyVertexBuffer *vertexbuffer, FPortalSceneState *state, HWSkyInfo *  pt, bool local = false)
		: HWPortal(state, local)
	{
		origin = pt;
		vertexBuffer = vertexbuffer;
	}

};
