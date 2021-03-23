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
	angle_t start, end;

	bool operator== (const ClipNode &other)
	{
		return other.start == start && other.end == end;
	}
};


class Clipper
{
	static unsigned starttime;
	FMemArena nodearena;
	ClipNode * freelist = nullptr;

	ClipNode * clipnodes = nullptr;
	ClipNode * cliphead = nullptr;
	ClipNode * silhouette = nullptr;	// will be preserved even when RemoveClipRange is called
	vec2_t viewpoint;
	bool blocked = false;

	bool IsRangeVisible(binangle startangle, binangle endangle);
	void RemoveRange(ClipNode * cn);
	void AddClipRange(binangle startangle, binangle endangle);
	void RemoveClipRange(binangle startangle, binangle endangle);

public:

	Clipper();

	void Clear();
	static binangle AngleToPseudo(binangle ang);

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

	ClipNode * NewRange(angle_t start, angle_t end)
	{
		ClipNode * c = GetNew();

		c->start = start;
		c->end = end;
		c->next = c->prev = NULL;
		return c;
	}

	void DoRemoveClipRange(angle_t start, angle_t end);

public:
    
    void SetViewpoint(const vec2_t &vp)
    {
        viewpoint = vp;
    }

	void SetSilhouette();

	bool SafeCheckRange(binangle startAngle, binangle endAngle)
	{
		if(startAngle.asbam() > endAngle.asbam())
		{
			return (IsRangeVisible(startAngle, bamang(ANGLE_MAX)) || IsRangeVisible(bamang(0), endAngle));
		}
		
		return IsRangeVisible(startAngle, endAngle);
	}

	void SafeAddClipRange(binangle startangle, binangle endangle)
	{
		if(startangle.asbam() > endangle.asbam())
		{
			// The range has to added in two parts.
			AddClipRange(startangle, bamang(ANGLE_MAX));
			AddClipRange(bamang(0), endangle);
		}
		else
		{
			// Add the range as usual.
			AddClipRange(startangle, endangle);
		}
	}
    
    void SafeAddClipRange(const vec2_t& v1, const vec2_t& v2)
    {
        binangle a2 = PointToAngle(v1);
        binangle a1 = PointToAngle(v2);
        SafeAddClipRange(a1,a2);
    }

	void SafeRemoveClipRange(binangle startangle, binangle endangle)
	{
		if(startangle.asbam() > endangle.asbam())
		{
			// The range has to added in two parts.
			RemoveClipRange(startangle, bamang(ANGLE_MAX));
			RemoveClipRange(bamang(0), endangle);
		}
		else
		{
			// Add the range as usual.
			RemoveClipRange(startangle, endangle);
		}
	}

	void SetBlocked(bool on)
	{
		blocked = on;
	}

	bool IsBlocked() const
	{
		return blocked;
	}

	void DumpClipper();
    
    binangle PointToAngle(const vec2_t& point);

};

#endif
