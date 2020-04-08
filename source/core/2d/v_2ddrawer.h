#ifndef __2DDRAWER_H
#define __2DDRAWER_H

#include "tarray.h"
#include "vectors.h"
#include "textures.h"
#include "renderstyle.h"

struct DrawParms;


struct F2DPolygons
{
	TArray<FVector4> vertices;
	TArray<int>  indices;

	unsigned AllocVertices(int num)
	{
		auto vindex = vertices.Reserve(num);
		indices.Push(num);
		return vindex;
	}

};



class F2DDrawer
{
public:

	enum EDrawType : uint8_t
	{
		DrawTypeTriangles,
		DrawTypeLines,
		DrawTypePoints,
		DrawTypeRotateSprite,
	};

	enum ETextureFlags : uint8_t
	{
		DTF_Wrap = 1,
		DTF_Scissor = 2,
        DTF_Burn = 4,
	};


	// This vertex type is hardware independent and needs conversion when put into a buffer.
	struct TwoDVertex
	{
		float x, y, z;
		float u, v;
		PalEntry color0;

		void Set(float xx, float yy, float zz)
		{
			x = xx;
			z = zz;
			y = yy;
			u = 0;
			v = 0;
			color0 = 0;
		}

		void Set(double xx, double yy, double zz, double uu, double vv, PalEntry col)
		{
			x = (float)xx;
			z = (float)zz;
			y = (float)yy;
			u = (float)uu;
			v = (float)vv;
			color0 = col;
		}

	};
	
	struct RenderCommand
	{
		EDrawType mType;
		int mVertIndex;
		int mVertCount;
		int mIndexIndex;
		int mIndexCount;

		FTexture *mTexture;
		int mRemapIndex;
		PalEntry mSpecialColormap[2];
		int mScissor[4];
		int mDesaturate;
		FRenderStyle mRenderStyle;
		PalEntry mColor1;	// Overlay color
		ETexMode mDrawMode;
		uint8_t mFlags;

		RenderCommand()
		{
			memset(this, 0, sizeof(*this));
		}

		// If these fields match, two draw commands can be batched.
		bool isCompatible(const RenderCommand &other) const
		{
			return mTexture == other.mTexture &&
				mType == other.mType &&
				mRemapIndex == other.mRemapIndex &&
				mSpecialColormap[0].d == other.mSpecialColormap[0].d &&
				mSpecialColormap[1].d == other.mSpecialColormap[1].d &&
				!memcmp(mScissor, other.mScissor, sizeof(mScissor)) &&
				mDesaturate == other.mDesaturate &&
				mRenderStyle == other.mRenderStyle &&
				mDrawMode == other.mDrawMode &&
				mFlags == other.mFlags &&
				mColor1.d == other.mColor1.d;

		}
	};

	TArray<int> mIndices;
	TArray<TwoDVertex> mVertices;
	TArray<RenderCommand> mData;
	
	int AddCommand(const RenderCommand *data);
	void AddIndices(int firstvert, int count, ...);
	void AddIndices(int firstvert, TArray<int> &v);
	bool SetStyle(FTexture *tex, DrawParms &parms, PalEntry &color0, RenderCommand &quad);
	void SetColorOverlay(PalEntry color, float alpha, PalEntry &vertexcolor, PalEntry &overlaycolor);

public:
	void AddTexture(FTexture *img, DrawParms &parms);
	void AddPoly(FTexture* img, FVector4 *vt, size_t vtcount, unsigned int *ind, size_t idxcount, int palette, int shade, int maskprops, int clipx1, int clipy1, int clipx2, int clipy2);
	void FillPolygon(int* rx1, int* ry1, int* xb1, int32_t npoints, int picnum, int palette, int shade, int props, const FVector2& xtex, const FVector2& ytex, const FVector2& otex,
		int clipx1, int clipy1, int clipx2, int clipy2);
	void AddFlatFill(int left, int top, int right, int bottom, FTexture *src, bool local_origin);

	void AddColorOnlyQuad(int left, int top, int width, int height, PalEntry color, FRenderStyle *style = nullptr);
	void ClearScreen(PalEntry color = 0xff000000);
			
	void AddLine(float x1, float y1, float x2, float y2, uint32_t color, uint8_t alpha = 255);
	void AddLine(float x1, float y1, float x2, float y2, int cx, int cy, int cx2, int cy2, uint32_t color, uint8_t alpha = 255);
	void AddThickLine(int x1, int y1, int x2, int y2, double thickness, uint32_t color, uint8_t alpha = 255);
	void AddPixel(int x1, int y1, uint32_t color);

	void rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
		int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
		int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, FTexture *pic, int basepal);

	void Clear();

	bool mIsFirstPass = true;
};

extern F2DDrawer twodgen;
extern F2DDrawer twodpsp;
extern F2DDrawer* twod;

// This is for safely substituting the 2D drawer for a block of code.
class PspTwoDSetter
{
	F2DDrawer* old;
public:
	PspTwoDSetter()
	{
		old = twod;
		twod = &twodpsp;
	}
	~PspTwoDSetter()
	{
		twod = old;
	}
	// Shadow Warrior fucked this up and draws the weapons in the same pass as the hud, meaning we have to switch this on and off depending on context.
	void set()
	{
		twod = &twodpsp;
	}
	void clear()
	{
		twod = old;
	}
};

void DrawTexture(F2DDrawer* drawer, FTexture* img, double x, double y, int tags_first, ...);
void DrawChar(F2DDrawer* drawer, FFont* font, int normalcolor, double x, double y, int character, int tag_first, ...);
void DrawText(F2DDrawer* drawer, FFont* font, int normalcolor, double x, double y, const char* string, int tag_first, ...);
void DrawText(F2DDrawer* drawer, FFont* font, int normalcolor, double x, double y, const char32_t* string, int tag_first, ...);
void DrawFrame(F2DDrawer* twod, PalEntry color, int left, int top, int width, int height, int thickness);

#endif
