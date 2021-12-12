#ifndef __GL_CLIPPER
#define __GL_CLIPPER

#include "xs_Float.h"
#include "memarena.h"
#include "basics.h"
#include "vectors.h"
#include "binaryangle.h"
#include "intvec.h"

class ClipWindow
{
	FVector2 left, right; 		// left and right edge of the window in 2D
	FAngle leftang, rightang; 	// view angles of the window edges
	Plane planes[2];			// top and bottom plane of the window

	// The inside is behind the plane defined by p1 - p4, the sides are defined by p0, pn and pn+1 respectively.
	// p1 is lower left, p2 upper left, p3 upper right and p4 lower right.
	void build(const FVector3& p0, const FVector3& p1, const FVector3& p2, const FVector3& p3, const FVector3& p4, DAngle la, DAngle ra)
	{
		left = p1.XY();
		right = p4.XY();
		planes[0].Init(p0, p2, p3);	// top plane - must point inside.
		planes[1].Init(p0, p4, p1);	// bottom plane - must point inside.
	}

	bool polyInWindow(const TArrayView<FVector3>& points)
	{
		for (auto& plane : planes)
		{
			for (auto& point : points)
			{
				if (!plane.PointOnSide(point)) goto nextplane; // Too bad that C++ still has no option to continue an outer loop from here...
			}
			return false;
		nextplane:;
		}
		return true;
	}
};


class ClipNode
{
	friend class Clipper;
	
	ClipNode *prev, *next;
	int start, end;

	bool operator== (const ClipNode &other)
	{
		return other.start == start && other.end == end;
	}
};


class Clipper
{
	FMemArena nodearena;
	ClipNode * freelist = nullptr;

	ClipNode * clipnodes = nullptr;
	ClipNode * cliphead = nullptr;
	vec2_t viewpoint;
	void RemoveRange(ClipNode* cn);
	binangle visibleStart, visibleEnd;

public:
	bool IsRangeVisible(int startangle, int endangle);
	void AddClipRange(int startangle, int endangle);
	void RemoveClipRange(int startangle, int endangle);

public:

	void Clear(binangle rangestart);

	void Free(ClipNode *node)
	{
		node->next = freelist;
		freelist = node;
	}

private:
	ClipNode * GetNew()
	{
		if (freelist)
		{
			ClipNode * p = freelist;
			freelist = p->next;
			return p;
		}
		else return (ClipNode*)nodearena.Alloc(sizeof(ClipNode));
	}

	ClipNode * NewRange(int start, int end)
	{
		ClipNode * c = GetNew();

		c->start = start;
		c->end = end;
		c->next = c->prev = NULL;
		return c;
	}

public:
    
    void SetViewpoint(const vec2_t &vp)
    {
        viewpoint = vp;
    }

	void SetVisibleRange(angle_t a1, angle_t a2)
	{
		if (a2 != 0xffffffff)
		{
			visibleStart = bamang(a1 - a2);
			visibleEnd = bamang(a1 + a2);
		}
		else visibleStart = visibleEnd = bamang(0);
	}

	void RestrictVisibleRange(binangle a1, binangle a2)
	{
		if (visibleStart == visibleEnd)
		{
			visibleStart = a1;
			visibleEnd = a2;
		}
		else
		{
			if (a1.asbam() - visibleStart.asbam() < visibleEnd.asbam() - visibleStart.asbam()) visibleStart = a1;
			if (a2.asbam() - visibleStart.asbam() < visibleEnd.asbam() - visibleStart.asbam()) visibleStart = a2;
		}
	}

	void DumpClipper();
    
	binangle PointToAngle(const vec2_t& pos)
	{
		vec2_t vec = pos - viewpoint;
		return bvectangbam(vec.x, vec.y);
	}


};

#endif
