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
		if (cliphead) cliphead->prev = nullptr;
	}
	else
	{
		if (range->prev) range->prev->next = range->next;
		if (range->next) range->next->prev = range->prev;
	}
	
	Free(range);
}

void Clipper::InsertRange(ClipNode* prev, ClipNode* node)
{
	if (prev)
	{
		node->next = prev->next;
		prev->next = node;
	}
	else
	{
		node->next = cliphead;
		cliphead = node;
	}
	node->prev = prev;
	if (node->next) node->next->prev = node;
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
		if (startAngle >= ci->start && endAngle <= ci->end && ci->topclip <= ci->bottomclip)
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
	if (cliphead)
	{
		auto node = cliphead;
		while (node != nullptr && node->start < end)
		{
			// check to see if range contains any old ranges.
			// These can be removed, regardless whether they are a window or fully closed.
			if (node->start >= start && node->end <= end)
			{
				auto temp = node;
				node = node->next;
				RemoveRange(temp);
			}
			// check if the new range lies fully within an existing range.
			else if (node->start <= start && node->end >= end)
			{
				// existing range is closed, we're done.
				if (node->topclip <= node->bottomclip)
				{
					return;
				}
				// this splits up a window node so we got to insert two new nodes
				else
				{
					int nodeend = node->end;
					node->end = start;

					auto mynode = NewRange(start, end, 0, 0);
					InsertRange(node, mynode);

					if (end != nodeend)
					{
						auto afternode = NewRange(end, nodeend, node->topclip, node->bottomclip);
						InsertRange(mynode, afternode);
					}

					// We can only delete the old node after being done with its data.
					if (node->end == node->start) RemoveRange(node);

					return;
				}
			}
			else
			{
				node = node->next;
			}
		}

		// at this point we know that overlaps can only be at one side because all full overlaps have been resolved already.
		// so what follows can at most intersect two other nodes - one at the left and one at the right
		node = cliphead;
		while (node != nullptr && node->start <= end)
		{
			// node overlaps at the start.
			if (node->start < start && node->end >= start)
			{
				if (node->topclip <= node->bottomclip)
				{
					node->end = end;
					auto next = node->next;
					if (next && next->start <= end)
					{
						// check if the following range overlaps. We know already that it will go past the end of the newly added range
						// (otherwise the first loop above would have taken care of it) so we can skip any checks for the full inclusion case.
						if (next->topclip <= next->bottomclip)
						{
							// next range is closed, so merge the two.
							node->end = next->end;
							RemoveRange(next);
						}
						else
						{
							next->start = end;
						}
					}
					// we're done and do not need to add a new node.
					return;
				}
				else
				{
					// if the old node is a window, restrict its size to the left of the new range and continue checking
					node->end = start;
				}
			}
			// range overlaps at the end.
			else if (node->start >= start && node->start <= end)
			{
				// node is closed - we can just merge this and exit
				if (node->topclip <= node->bottomclip)
				{
					node->start = start;
					return;
				}
				// node is a window - so restrict its size and insert the new node in front of it,
				else
				{
					node->start = end;

					auto mynode = NewRange(start, end, 0, 0);
					InsertRange(node->prev, mynode);
					return;
				}
			}
			node = node->next;
		}

		//found no intersections - just add range
		node = cliphead;
		ClipNode* prevNode = nullptr;
		// advance to the place where this can be inserted.		
		while (node != nullptr && node->start < end)
		{
			prevNode = node;
			node = node->next;
		}
		auto mynode = NewRange(start, end, 0, 0);
		InsertRange(prevNode, mynode);
	}
	else
	{
		cliphead = NewRange(start, end, 0, 0);
	}
}


//-----------------------------------------------------------------------------
//
// AddWindowRange
//
//-----------------------------------------------------------------------------

void Clipper::AddWindowRange(int start, int end, float topclip, float bottomclip)
{
	if (cliphead)
	{
		auto node = cliphead;
		while (node != nullptr && node->start < end)
		{
			// check to see if range contains any old ranges.

			// existing range is part of new one.
			if (node->start >= start && node->end <= end)
			{
				// if old range is a window, make some adjustments.
				if (topclip <= node->topclip && bottomclip >= node->bottomclip)
				{
					// if the new window is more narrow both on top and bottom, we can remove the old range.
					auto temp = node;
					node = node->next;
					RemoveRange(temp);
					continue;
				}

				// in all other cases we must adjust the node, and recursively process both sub-ranges.
				if (topclip < node->topclip) node->topclip = topclip;
				if (bottomclip > node->bottomclip) node->bottomclip = bottomclip;
				int nodestart = node->start, nodeend = node->end;
				// At this point it is just easier to recursively add the sub-ranges because we'd have to run the full program on both anyway,
				if (start < nodestart) AddWindowRange(start, nodestart, topclip, bottomclip);
				if (end > nodeend) AddWindowRange(nodeend, end, topclip, bottomclip);
				return;
			}
			// check if the new range lies fully within an existing range.
			else if (node->start <= start && node->end >= end)
			{
				// existing range is closed or a more narrow window on both sides, we're done.
				if (node->topclip <= node->bottomclip || (topclip >= node->topclip && bottomclip <= node->bottomclip))
				{
					return;
				}
				// this splits up a window node so we got to insert two new nodes
				else
				{
					// adapt the window for the intersection.
					if (topclip > node->topclip) topclip = node->topclip;
					if (bottomclip < node->bottomclip) bottomclip = node->bottomclip;

					int nodeend = node->end;
					node->end = start;

					auto mynode = NewRange(start, end, topclip, bottomclip);
					InsertRange(node, mynode);

					if (end != nodeend)
					{
						auto afternode = NewRange(end, nodeend, node->topclip, node->bottomclip);
						InsertRange(mynode, afternode);
					}

					// We can only delete the old node after being done with its data.
					if (node->end == node->start) RemoveRange(node);

					return;
				}
			}
			else
			{
				node = node->next;
			}
		}
		
		// at this point we know that overlaps can only be at one side because all full overlaps have been resolved already.
		// so what follows can at most intersect two other nodes - one at the left and one at the right
		node = cliphead;
		while (node != nullptr && node->start <= end)
		{
			// node overlaps at the start.
			if (node->start < start && node->end >= start)
			{
				struct temprange
				{
					int start, end;
					float top, bottom;
				};
				temprange ranges[2];
				memset(ranges, -1, sizeof(ranges));
				// only split if this changes the old range, otherwise just truncate
				if (node->topclip > node->bottomclip && (topclip < node->topclip || bottomclip > node->bottomclip))
				{
					ranges[0].start = start;
					ranges[0].end = node->end;
					ranges[0].top = min(topclip, node->topclip);
					ranges[0].bottom = max(bottomclip, node->bottomclip);
					node->end = start;
				}
				start = node->end;

				// check if the following range overlaps. We know already that it will go past the end of the newly added range
				// (otherwise the first loop above would have taken care of it) so we can skip any checks for the full inclusion case.
				auto next = node->next;
				if (next && next->start <= end)
				{
					// only split if this changes the old range, otherwise just truncate
					if (next->topclip > next->bottomclip && (topclip < next->topclip || bottomclip > next->bottomclip))
					{
						ranges[1].start = next->start;
						ranges[1].end = end;
						ranges[1].top = min(topclip, next->topclip);
						ranges[1].bottom = max(bottomclip, next->bottomclip);
						next->start = end;
					}
					end = next->start;
				}
				auto after = node;
				ClipNode* insert;
				if (ranges[0].end != -1)
				{
					insert = NewRange(ranges[0].start, ranges[0].end, ranges[0].top, ranges[0].bottom);
					InsertRange(after, insert);
					after = insert;
				}
				insert = NewRange(start, end, topclip, bottomclip);
				InsertRange(after, insert);
				after = insert;
				if (ranges[1].end != -1)
				{
					insert = NewRange(ranges[1].start, ranges[1].end, ranges[1].top, ranges[1].bottom);
					InsertRange(after, insert);
				}
				return;
			}
			// range overlaps at the end.
			else if (node->start >= start && node->start <= end)
			{
				auto prev = node->prev;
				if (node->topclip > node->bottomclip && (topclip < node->topclip || bottomclip > node->bottomclip))
				{
					auto inode = NewRange(start, node->end, min(topclip, node->topclip), max(bottomclip, node->bottomclip));
					node->end = start;
					InsertRange(prev, inode);
				}
				start = node->end;
				auto mynode = NewRange(start, end, topclip, bottomclip);
				InsertRange(prev, mynode);
				return;
			}
			node = node->next;		
		}
		
		//found no intersections - just add range
		node = cliphead;
		ClipNode* prevNode = nullptr;
		// advance to the place where this can be inserted.		
		while (node != nullptr && node->start < end)
		{
			prevNode = node;
			node = node->next;
		}
		auto mynode = NewRange(start, end, topclip, bottomclip);
		InsertRange(prevNode, mynode);
	}
	else
	{
		cliphead = NewRange(start, end, topclip, bottomclip);
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
				temp = NewRange(end, node->end, node->topclip, node->bottomclip);
				InsertRange(node, temp);
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
		Printf("Range from %2.3f to %2.3f (top = %2.3f, bottom = %2.3f)\n", bamang(node->start).asdeg(), bamang(node->end).asdeg(), node->topclip, node->bottomclip);
	}
}
