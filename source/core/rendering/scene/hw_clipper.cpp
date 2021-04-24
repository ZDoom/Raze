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

void Clipper::Clear(binangle rangestart)
{
	ClipNode *node = cliphead;
	ClipNode *temp;
	
	while (node != nullptr)
	{
		temp = node;
		node = node->next;
		Free(temp);
	}
	
	cliphead = nullptr;

	if (visibleStart.asbam() != 0 || visibleEnd.asbam() != 0)
	{
		int vstart = int(visibleStart.asbam() - rangestart.asbam());
		if (vstart > 1) AddClipRange(0, vstart - 1);

		int vend = int(visibleEnd.asbam() - rangestart.asbam());
		if (vend > 0 && vend < INT_MAX - 1) AddClipRange(vend + 1, INT_MAX);
	}


}

//-----------------------------------------------------------------------------
//
// IsRangeVisible
//
//-----------------------------------------------------------------------------

bool Clipper::IsRangeVisible(int startAngle, int endAngle)
{
	ClipNode *ci;
	ci = cliphead;
	
	if (endAngle == 0 && ci && ci->start==0) return false;
	
	while (ci != nullptr && ci->start < endAngle)
	{
		if (startAngle >= ci->start && endAngle <= ci->end)
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

void Clipper::AddClipRange(int start, int end)
{
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

void Clipper::RemoveClipRange(int start, int end)
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
//-----------------------------------------------------------------------------

void Clipper::DumpClipper()
{
	for (auto node = cliphead; node; node = node->next)
	{
		Printf("Range from %f to %f\n", bamang(node->start).asdeg(), bamang(node->end).asdeg());
	}
}
