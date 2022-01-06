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
//
//
//-----------------------------------------------------------------------------

void Clipper::ValidateList()
{
#ifdef _DEBUG
	// Make sure we catch ordering issues right away in debug mode.
	auto check = cliphead;
	while (check)
	{
		assert(!check->next || check->end <= check->next->start);
		check = check->next;
	}
#endif
}

//-----------------------------------------------------------------------------
//
// RemoveRange
//
//-----------------------------------------------------------------------------

void Clipper::RemoveRange(ClipNode * range, bool free)
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

	if (free) Free(range);
	ValidateList();
}

//-----------------------------------------------------------------------------
//
// InsertRange
//
//-----------------------------------------------------------------------------

bool Clipper::InsertRange(ClipNode* prev, ClipNode* node)
{
	if (node->start == node->end)
	{
		Free(node);
		return false;
	}
	assert(node->start <= node->end);
	assert(!prev || prev->end <= node->start);
	assert(!prev || !prev->next || prev->next->start >= node->end);

	if (node->topclip <= node->bottomclip)
	{
		if (prev)
		{
			if (prev->topclip <= prev->bottomclip && prev->end >= node->start)
			{
				prev->end = node->end;
				Free(node);
				if (prev->next && prev->end >= prev->next->start && prev->next->topclip <= prev->next->bottomclip)
				{
					prev->end = prev->next->end;
					RemoveRange(prev->next);
					return true;
				}
				ValidateList();
				return false;
			}
			else if (prev->next && node->end >= prev->next->start && prev->next->topclip <= prev->next->bottomclip)
			{
				prev->next->start = node->start;
				Free(node);
				ValidateList();
				return false;
			}
		}
	}

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

	ValidateList();
	return false;
}

//-----------------------------------------------------------------------------
//
// SplitRange
//
//-----------------------------------------------------------------------------

void Clipper::SplitRange(ClipNode* node, int start, int end, float topclip, float bottomclip)
{
	assert(start < end);
	int clones = 0;
	if (end < node->end)
	{
		int nodeend = node->end;
		node->end = end;
		auto endnode = NewRange(end, nodeend, node->topclip, node->bottomclip);
		InsertRange(node, endnode);
	}
	if (start > node->start)
	{
		node->end = start;
		auto startnode = NewRange(start, end, topclip, bottomclip);
		InsertRange(node, startnode);
	}
	else
	{
		// remove and reinsert to do proper checks of the clipping window.
		auto prev = node->prev;
		RemoveRange(node, false);
		node->topclip = topclip;
		node->bottomclip = bottomclip;
		InsertRange(prev, node);
	}
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
		if (vstart > 1) AddClipRange(0, vstart);

		int vend = int(visibleEnd.asbam() - rangestart.asbam());
		if (vend > 0 && vend < INT_MAX) AddClipRange(vend, INT_MAX);
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
				// if the existing range is closed, we're done.
				// Other split up the old window to insert the new range in the middle.
				if (node->topclip > node->bottomclip)
				{
					SplitRange(node, start, end, 0, 0);
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
					ValidateList();
					return;
				}
				else
				{
					// if the old node is a window, restrict its size to the left of the new range and continue checking
					node->end = start;
					ValidateList();
				}
			}
			// range overlaps at the end.
			else if (node->start >= start && node->start <= end)
			{
				// node is closed - we can just merge this and exit
				if (node->topclip <= node->bottomclip)
				{
					node->start = start;
					ValidateList();
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

void Clipper::AddWindowRange(int start, int end, float topclip, float bottomclip, float viewz)
{
	auto mergeClip = [](ClipNode* node, float& topclip, float& bottomclip, float viewz)
	{
		// If the node is already closed, return a closed range.
		if (node->topclip <= node->bottomclip)
		{
			topclip = bottomclip = 0;
			return;
		}
		float mintopclip = min(node->topclip, topclip);
		float maxbotclip = max(node->bottomclip, bottomclip);

		if (mintopclip > max(viewz, maxbotclip)) topclip = FLT_MAX;
		else topclip = mintopclip;

		if (maxbotclip < min(viewz, mintopclip)) bottomclip = -FLT_MAX;
		else bottomclip = maxbotclip;
	};

	ClipNode* prevNode = nullptr;

	if (cliphead)
	{
		auto node = cliphead;
		while (node != nullptr && node->start < end)
		{
			// check to see if range contains any old ranges.

			//-----------------------------------------------------------------------------
			//
			// existing range is part of new one.
			//
			//-----------------------------------------------------------------------------

			if (node->start >= start && node->end <= end)
			{
				int nodestart = node->start, nodeend = node->end;
				if (node->topclip > node->bottomclip) // we only need to make adjustments to the old node if it is not closed.
				{
					float mtopclip = topclip, mbottomclip = bottomclip;
					mergeClip(node, mtopclip, mbottomclip, viewz);
					// if the new window is closed, we must remove the old range and insert a closed one, so that it gets merged with its neighbours.
					if (mtopclip <= mbottomclip)
					{
						auto prev = node->prev;
						RemoveRange(node);
						auto mynode = NewRange(nodestart, nodeend, 0, 0);
						InsertRange(prev, mynode);
					}
					else
					{
						// in all other cases we must adjust the node's top and bottom
						node->topclip = mtopclip;
						node->bottomclip = mbottomclip;
					}
				}
				// At this point it is just easier to recursively add the sub-ranges because we'd have to run the full program on both anyway,
				// We must ensure the the new ranges' length are > 0.
				if (start < nodestart) AddWindowRange(start, nodestart, topclip, bottomclip, viewz);
				if (end > nodeend) AddWindowRange(nodeend, end, topclip, bottomclip, viewz);
				return;
			}

			//-----------------------------------------------------------------------------
			//
			// check if the new range lies fully within an existing range.
			//
			//-----------------------------------------------------------------------------

			else if (node->start <= start && node->end >= end)
			{
				// Shortcut if existing range is closed. In this case there's nothing to do.
				if (node->topclip <= node->bottomclip)
				{
					return;
				}

				float mtopclip = topclip, mbottomclip = bottomclip;
				mergeClip(node, mtopclip, mbottomclip, viewz);

				// existing range is a more narrow window on both sides, we're done.
				if (mtopclip > mbottomclip && mtopclip >= node->topclip && mbottomclip <= node->bottomclip)
				{
					return;
				}
				else
				{
					// adapt the window for the intersection.
					SplitRange(node, start, end, mtopclip, mbottomclip);
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
			auto next = node->next;	// get this before making any edits.

			//-----------------------------------------------------------------------------
			//
			// node overlaps the start of the new range
			//
			//-----------------------------------------------------------------------------

			if (node->start < start && node->end > start)
			{
				// if the old range is closed, just shorten the new one and continue.
				if (node->topclip < node->bottomclip)
				{
					start = node->end;
					ValidateList();
					if (start >= end) return; // may have been crushed to a single point.
				}
				else
				{
					float mtopclip = topclip, mbottomclip = bottomclip;
					mergeClip(node, mtopclip, mbottomclip, viewz);

					// if the old range is more narrow than the new one, just shorten the new one and continue.
					if (mtopclip > mbottomclip && mtopclip >= node->topclip && mbottomclip <= node->bottomclip)
					{
						start = node->end;
						ValidateList();
						if (start >= end) return; // may have been crushed to a single point.
					}

					// the unaltered new range is more narrow than the old one - just shorten the old one and go on.
					else if (topclip <= bottomclip || (topclip < node->topclip && bottomclip > node->bottomclip))
					{
						node->end = start;
						ValidateList();
						assert(node->end > node->start); // ensured by initial condition.
					}

					// if the intersection needs to take properties of both old and new we need to make a split.
					else
					{
						int nodeend = node->end;
						SplitRange(node, start, nodeend, mtopclip, mbottomclip);
						start = nodeend;
						if (start >= end) return; // may have been crushed to a single point.
						if (mtopclip <= mbottomclip) next = cliphead; // list may have become out of sync.
					}
				}
			}

			//-----------------------------------------------------------------------------
			//
			// nosw overlaps at the end of the new range.
			//
			//-----------------------------------------------------------------------------

			else if (node->start >= start && node->start < end)
			{
				// if the old range is closed, just shorten the new one and continue.
				if (node->topclip < node->bottomclip)
				{
					end = node->start;
					ValidateList();
					if (start >= end) return; // may have been crushed to a single point.
				}
				else
				{
					float mtopclip = topclip, mbottomclip = bottomclip;
					mergeClip(node, mtopclip, mbottomclip, viewz);

					// if the old range is more narrow than the new one, just shorten the new one and continue.
					if (mtopclip > mbottomclip && mtopclip >= node->topclip && mbottomclip <= node->bottomclip)
					{
						end = node->start;
						ValidateList();
						if (start >= end) return; // may have been crushed to a single point.
					}

					// the unaltered new range is more narrow than the old one - just shorten the old one and go on.
					else if (topclip <= bottomclip || (topclip < node->topclip && bottomclip > node->bottomclip))
					{
						node->start = end;
						ValidateList();
						assert(node->end > node->start);
					}

					// if the intersection needs to take properties of both old and new we need to make a split.
					else
					{
						int nodestart = node->start;
						SplitRange(node, nodestart, end, mtopclip, mbottomclip);
						end = nodestart;
						if (start >= end) return; // may have been crushed to a single point.
						if (mtopclip <= mbottomclip) next = cliphead; // list may have become out of sync.
					}
				}
			}
			node = next;
		}

		// cliphead *can* be null here if a sole existing older range got removed because this one covers it entirely.
		if (cliphead)
		{
			// we get here if a new range needs to be inserted.
			node = cliphead;
			// advance to the place where this can be inserted.		
			while (node != nullptr && node->start < end)
			{
				prevNode = node;
				node = node->next;
			}
			assert(!prevNode || prevNode->end <= start);
			assert(!prevNode || !prevNode->next || prevNode->next->start >= end);
			assert(prevNode || (cliphead && (!cliphead->next || cliphead->next->start >= end)));
		}
	}

	if (topclip > viewz) topclip = FLT_MAX;
	if (bottomclip < viewz) bottomclip = -FLT_MAX;
	// only insert a new node if it restricts the window.
	if (topclip != FLT_MAX || bottomclip != -FLT_MAX)
	{
		auto mynode = NewRange(start, end, topclip, bottomclip);
		InsertRange(prevNode, mynode);
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
				node->end = start;
				InsertRange(node, temp);
				break;
			}
			node = node->next;
		}
	}
	ValidateList();
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
