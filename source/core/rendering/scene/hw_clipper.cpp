/*
*
** gl_clipper.cpp
**
** Handles visibility checks.
** Loosely based on the JDoom clipper.
**
**---------------------------------------------------------------------------
** Copyright 2003 Tim Stump
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "hw_clipper.h"
#include "basics.h"
#include "build.h"
#include "printf.h"

unsigned Clipper::starttime;

Clipper::Clipper()
{
	starttime++;
}

//-----------------------------------------------------------------------------
//
// RemoveRange
//
//-----------------------------------------------------------------------------

void Clipper::RemoveRange(ClipNode * range)
{
	if (range == cliphead)
	{
		cliphead = cliphead->next;
	}
	else
	{
		if (range->prev) range->prev->next = range->next;
		if (range->next) range->next->prev = range->prev;
	}
	
	Free(range);
}

//-----------------------------------------------------------------------------
//
// Clear
//
//-----------------------------------------------------------------------------

void Clipper::Clear()
{
	ClipNode *node = cliphead;
	ClipNode *temp;
	
	blocked = false;
	while (node != nullptr)
	{
		temp = node;
		node = node->next;
		Free(temp);
	}
	node = silhouette;

	while (node != nullptr)
	{
		temp = node;
		node = node->next;
		Free(temp);
	}
	
	cliphead = nullptr;
	silhouette = nullptr;
	starttime++;
}

//-----------------------------------------------------------------------------
//
// SetSilhouette
//
//-----------------------------------------------------------------------------

void Clipper::SetSilhouette()
{
	ClipNode *node = cliphead;
	ClipNode *last = nullptr;

	while (node != nullptr)
	{
		ClipNode *snode = NewRange(node->start, node->end);
		if (silhouette == nullptr) silhouette = snode;
		snode->prev = last;
		if (last != nullptr) last->next = snode;
		last = snode;
		node = node->next;
	}
}

//-----------------------------------------------------------------------------
//
// IsRangeVisible
//
//-----------------------------------------------------------------------------

bool Clipper::IsRangeVisible(binangle startAngle, binangle endAngle)
{
	ClipNode *ci;
	ci = cliphead;
	
	if (endAngle.asbam()==0 && ci && ci->start==0) return false;
	
	while (ci != nullptr && ci->start < endAngle.asbam())
	{
		if (startAngle.asbam() >= ci->start && endAngle.asbam() <= ci->end)
		{
			return false;
		}
		ci = ci->next;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
//
// AddClipRange
//
//-----------------------------------------------------------------------------

void Clipper::AddClipRange(binangle start_, binangle end_)
{
	auto start = start_.asbam(), end = end_.asbam();
	ClipNode *node, *temp, *prevNode;

	if (cliphead)
	{
		//check to see if range contains any old ranges
		node = cliphead;
		while (node != nullptr && node->start < end)
		{
			if (node->start >= start && node->end <= end)
			{
				temp = node;
				node = node->next;
				RemoveRange(temp);
			}
			else if (node->start<=start && node->end>=end)
			{
				return;
			}
			else
			{
				node = node->next;
			}
		}
		
		//check to see if range overlaps a range (or possibly 2)
		node = cliphead;
		while (node != nullptr && node->start <= end)
		{
			if (node->end >= start)
			{
				// we found the first overlapping node
				if (node->start > start)
				{
					// the new range overlaps with this node's start point
					node->start = start;
				}

				if (node->end < end) 
				{
					node->end = end;
				}

				ClipNode *node2 = node->next;
				while (node2 && node2->start <= node->end)
				{
					if (node2->end > node->end) node->end = node2->end;
					ClipNode *delnode = node2;
					node2 = node2->next;
					RemoveRange(delnode);
				}
				return;
			}
			node = node->next;		
		}
		
		//just add range
		node = cliphead;
		prevNode = nullptr;
		temp = NewRange(start, end);
		
		while (node != nullptr && node->start < end)
		{
			prevNode = node;
			node = node->next;
		}
		
		temp->next = node;
		if (node == nullptr)
		{
			temp->prev = prevNode;
			if (prevNode) prevNode->next = temp;
			if (!cliphead) cliphead = temp;
		}
		else
		{
			if (node == cliphead)
			{
				cliphead->prev = temp;
				cliphead = temp;
			}
			else
			{
				temp->prev = prevNode;
				prevNode->next = temp;
				node->prev = temp;
			}
		}
	}
	else
	{
		temp = NewRange(start, end);
		cliphead = temp;
		return;
	}
}


//-----------------------------------------------------------------------------
//
// RemoveClipRange
//
//-----------------------------------------------------------------------------

void Clipper::RemoveClipRange(binangle start_, binangle end_)
{
	auto start = start_.asbam(), end = end_.asbam();
	ClipNode *node;

	if (silhouette)
	{
		node = silhouette;
		while (node != nullptr && node->end <= start)
		{
			node = node->next;
		}
		if (node != nullptr && node->start <= start)
		{
			if (node->end >= end) return;
			start = node->end;
			node = node->next;
		}
		while (node != nullptr && node->start < end)
		{
			DoRemoveClipRange(start, node->start);
			start = node->end;
			node = node->next;
		}
		if (start >= end) return;
	}
	DoRemoveClipRange(start, end);
}
	
//-----------------------------------------------------------------------------
//
// RemoveClipRange worker function
//
//-----------------------------------------------------------------------------

void Clipper::DoRemoveClipRange(angle_t start, angle_t end)
{
	ClipNode *node, *temp;

	if (cliphead)
	{
		//check to see if range contains any old ranges
		node = cliphead;
		while (node != nullptr && node->start < end)
		{
			if (node->start >= start && node->end <= end)
			{
				temp = node;
				node = node->next;
				RemoveRange(temp);
			}
			else
			{
				node = node->next;
			}
		}
		
		//check to see if range overlaps a range (or possibly 2)
		node = cliphead;
		while (node != nullptr)
		{
			if (node->start >= start && node->start <= end)
			{
				node->start = end;
				break;
			}
			else if (node->end >= start && node->end <= end)
			{
				node->end=start;
			}
			else if (node->start < start && node->end > end)
			{
				temp = NewRange(end, node->end);
				node->end=start;
				temp->next=node->next;
				temp->prev=node;
				node->next=temp;
				if (temp->next) temp->next->prev=temp;
				break;
			}
			node = node->next;
		}
	}
}


//-----------------------------------------------------------------------------
//
// ! Returns the pseudoangle between the line p1 to (infinity, p1.y) and the 
// line from p1 to p2. The pseudoangle has the property that the ordering of 
// points by true angle around p1 and ordering of points by pseudoangle are the 
// same.
//
// For clipping exact angles are not needed. Only the ordering matters.
// This is about as fast as the fixed point R_PointToAngle2 but without
// the precision issues associated with that function.
//
//-----------------------------------------------------------------------------

binangle Clipper::PointToAngle(const vec2_t& pos)
{
	vec2_t vec = pos - viewpoint;
#if 0

	if (vec.x == 0 && vec.y == 0)
	{
		return 0;
	}
	else
	{
		double result = vec.y / double(abs(vec.x) + fabs(vec.y));
		if (vec.x < 0)
		{
			result = 2. - result;
		}
		return bamang(xs_Fix<30>::ToFix(result));
	}
#else
	return q16ang(gethiq16angle(vec.x, vec.y));
#endif
}

void Clipper::DumpClipper()
{
	for (auto node = cliphead; node; node = node->next)
	{
		Printf("Range from %f to %f\n", bamang(node->start).asdeg(), bamang(node->end).asdeg());
	}
}
