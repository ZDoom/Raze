// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2006-2022 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include "basics.h"
#include "maptypes.h"
#include "memarena.h"
#include "gamefuncs.h"
#include "v_video.h"
#include "flatvertices.h"
#include "hw_vertexmap.h"
#include "hw_drawstructs.h"


EXTERN_CVAR(Bool, gl_seamless)

//==========================================================================
//
// Split left edge of wall
//
//==========================================================================

void HWWall::SplitLeftEdge(FFlatVertex *&ptr)
{
	if (seg == nullptr) return;
	vertex_t* vi = &vertices[vertexMap[wallnum(seg)]];

	if (vi->numheights)
	{
		int i = 0;

		float polyh1 = ztop[0] - zbottom[0];
		float factv1 = polyh1 ? (tcs[UPLFT].v - tcs[LOLFT].v) / polyh1 : 0;
		float factu1 = polyh1 ? (tcs[UPLFT].u - tcs[LOLFT].u) / polyh1 : 0;

		while (i<vi->numheights && vi->heightlist[i] <= zbottom[0]) i++;
		while (i<vi->numheights && vi->heightlist[i] < ztop[0])
		{
			ptr->x = glseg.x1;
			ptr->y = glseg.y1;
			ptr->z = vi->heightlist[i];
			ptr->u = factu1*(vi->heightlist[i] - ztop[0]) + tcs[UPLFT].u;
			ptr->v = factv1*(vi->heightlist[i] - ztop[0]) + tcs[UPLFT].v;
			ptr++;
			i++;
		}
	}
}

//==========================================================================
//
// Split right edge of wall
//
//==========================================================================

void HWWall::SplitRightEdge(FFlatVertex *&ptr)
{
	if (seg == nullptr) return;
	vertex_t* vi = &vertices[vertexMap[seg->point2]];

	if (vi->numheights)
	{
		int i = vi->numheights - 1;

		float polyh2 = ztop[1] - zbottom[1];
		float factv2 = polyh2 ? (tcs[UPRGT].v - tcs[LORGT].v) / polyh2 : 0;
		float factu2 = polyh2 ? (tcs[UPRGT].u - tcs[LORGT].u) / polyh2 : 0;

		while (i>0 && vi->heightlist[i] >= ztop[1]) i--;
		while (i>0 && vi->heightlist[i] > zbottom[1])
		{
			ptr->x = glseg.x2;
			ptr->y = glseg.y2;
			ptr->z = vi->heightlist[i];
			ptr->u = factu2*(vi->heightlist[i] - ztop[1]) + tcs[UPRGT].u;
			ptr->v = factv2*(vi->heightlist[i] - ztop[1]) + tcs[UPRGT].v;
			ptr++;
			i--;
		}
	}
}

//==========================================================================
//
// Create vertices for one wall
//
//==========================================================================

int HWWall::CreateVertices(FFlatVertex *&ptr, bool split)
{
	auto oo = ptr;
	ptr->Set(glseg.x1, zbottom[0], glseg.y1, tcs[LOLFT].u, tcs[LOLFT].v);
	ptr++;
	if (split && glseg.fracleft == 0) SplitLeftEdge(ptr);
	ptr->Set(glseg.x1, ztop[0], glseg.y1, tcs[UPLFT].u, tcs[UPLFT].v);
	ptr++;
	ptr->Set(glseg.x2, ztop[1], glseg.y2, tcs[UPRGT].u, tcs[UPRGT].v);
	ptr++;
	if (split && glseg.fracright == 1) SplitRightEdge(ptr);
	ptr->Set(glseg.x2, zbottom[1], glseg.y2, tcs[LORGT].u, tcs[LORGT].v);
	ptr++;
	return int(ptr - oo);
}


//==========================================================================
//
// Split left edge of wall
//
//==========================================================================

void HWWall::CountLeftEdge(unsigned &ptr)
{
	vertex_t* vi = &vertices[vertexMap[wallnum(seg)]];

	if (vi->numheights)
	{
		int i = 0;

		while (i<vi->numheights && vi->heightlist[i] <= zbottom[0]) i++;
		while (i<vi->numheights && vi->heightlist[i] < ztop[0])
		{
			ptr++;
			i++;
		}
	}
}

//==========================================================================
//
// Split right edge of wall
//
//==========================================================================

void HWWall::CountRightEdge(unsigned &ptr)
{
	vertex_t* vi = &vertices[vertexMap[seg->point2]];

	if (vi->numheights)
	{
		int i = vi->numheights - 1;

		while (i>0 && vi->heightlist[i] >= ztop[1]) i--;
		while (i>0 && vi->heightlist[i] > zbottom[1])
		{
			ptr++;
			i--;
		}
	}
}

//==========================================================================
//
// 
//
//==========================================================================

int HWWall::CountVertices()
{
	unsigned ptr = 4;
	if (seg)
	{
		if (glseg.fracleft == 0) CountLeftEdge(ptr);
		if (glseg.fracright == 1) CountRightEdge(ptr);
	}
	return (int)ptr;
}

//==========================================================================
//
// build the vertices for this wall
//
//==========================================================================

void HWWall::MakeVertices(HWDrawInfo *di, bool nosplit)
{
	if (vertcount == 0)
	{
		bool split = (gl_seamless && !nosplit && seg != nullptr);
		auto ret = screen->mVertexData->AllocVertices(split ? CountVertices() : 4);
		vertindex = ret.second;
		vertcount = CreateVertices(ret.first, split);
	}
}


