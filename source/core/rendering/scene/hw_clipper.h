#ifndef __GL_CLIPPER
#define __GL_CLIPPER

#include "xs_Float.h"
#include "memarena.h"
#include "basics.h"
#include "vectors.h"
#include "binaryangle.h"
#include "intvec.h"

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
