#ifndef __GL_CLIPPER
#define __GL_CLIPPER

#include "xs_Float.h"
#include "memarena.h"
#include "basics.h"
#include "vectors.h"
#include "intvec.h"

class ClipNode
{
	friend class Clipper;

	ClipNode *prev, *next;
	int start, end;
	float topclip, bottomclip;
};


class Clipper
{
	FMemArena nodearena;
	ClipNode * freelist = nullptr;

	ClipNode * cliphead = nullptr;
	angle_t visibleStart, visibleEnd;

public:
	bool IsRangeVisible(int startangle, int endangle);
	void AddClipRange(int startangle, int endangle);
	void AddWindowRange(int startangle, int endangle, float topclip, float bottomclip, float viewz);
	void RemoveClipRange(int startangle, int endangle);

public:

	void Clear(angle_t rangestart);

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

	ClipNode * NewRange(int start, int end, float top, float bottom)
	{
		ClipNode * c = GetNew();

		c->start = start;
		c->end = end;
		c->topclip = top;
		c->bottomclip = bottom;
		c->next = c->prev = NULL;
		return c;
	}

	void RemoveRange(ClipNode* cn, bool free = true);
	bool InsertRange(ClipNode* prev, ClipNode* node);
	bool InsertBlockingRange(ClipNode* prev, ClipNode* node, int start, int end);
	void SplitRange(ClipNode* node, int start, int end, float topclip, float bottomclip);
	void ValidateList();

public:

	void SetVisibleRange(angle_t a1, angle_t a2)
	{
		if (a2 != 0xffffffff)
		{
			visibleStart = a1 - a2;
			visibleEnd = a1 + a2;
		}
		else visibleStart = visibleEnd = 0;
	}

	void RestrictVisibleRange(angle_t a1, angle_t a2)
	{
		if (visibleStart == visibleEnd)
		{
			visibleStart = a1;
			visibleEnd = a2;
		}
		else
		{
			if (a1 - visibleStart < visibleEnd - visibleStart) visibleStart = a1;
			if (a2 - visibleStart < visibleEnd - visibleStart) visibleStart = a2;
		}
	}

	void DumpClipper();
};

#endif
