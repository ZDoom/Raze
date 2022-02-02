//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
//#include <iostream>

#include "ns.h"	// Must come before everything else!


#include "build.h"
#include "automap.h"
#include "savegamehelp.h"

#include "blood.h"

BEGIN_BLD_NS

VECTORDATA gVectorData[] = { // this is constant EXCEPT for [VECTOR_TYPE_20].maxDist. What were they thinking... 

	// Tine
	{
		kDamageBullet,
		17,
		174762,
		1152,
		10240,
		0,
		1,
		20480,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_43, FX_6, FX_NONE, 502,
		FX_43, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_7, 502,
		FX_43, FX_6, FX_7, 502,
		FX_NONE, FX_NONE, FX_NONE, 503,
		FX_43, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 503,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

	// Shell
	{
		kDamageBullet,
		4,
		65536,
		0,
		8192,
		0,
		1,
		12288,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_43, FX_6, FX_NONE, -1,
		FX_43, FX_0, FX_NONE, -1,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_43, FX_6, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

	// Bullet
	{
		kDamageBullet,
		7,
		21845,
		0,
		32768,
		0,
		1,
		12288,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_7, 510,
		FX_NONE, FX_5, FX_7, 511,
		FX_43, FX_6, FX_NONE, 512,
		FX_43, FX_0, FX_NONE, 513,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_7, 512,
		FX_43, FX_6, FX_7, 512,
		FX_NONE, FX_NONE, FX_NONE, 513,
		FX_43, FX_NONE, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,

	},

	// Tommy AP
	{
		kDamageBullet,
		20,
		65536,
		0,
		16384,
		0,
		1,
		20480,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_7, 510,
		FX_NONE, FX_5, FX_7, 511,
		FX_43, FX_6, FX_NONE, 512,
		FX_43, FX_0, FX_NONE, 513,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_7, 512,
		FX_43, FX_6, FX_7, 512,
		FX_NONE, FX_NONE, FX_NONE, 513,
		FX_43, FX_NONE, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

	// Shell AP
	{
		kDamageBullet,
		6,
		87381,
		0,
		12288,
		0,
		1,
		6144,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_43, FX_6, FX_NONE, -1,
		FX_43, FX_0, FX_NONE, -1,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_43, FX_6, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

	// Tommy regular
	{
		kDamageBullet,
		12,
		65536,
		0,
		16384,
		0,
		1,
		12288,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_43, FX_5, FX_7, 510,
		FX_NONE, FX_5, FX_7, 511,
		FX_43, FX_6, FX_NONE, 512,
		FX_43, FX_0, FX_NONE, 513,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_7, 512,
		FX_43, FX_6, FX_7, 512,
		FX_NONE, FX_NONE, FX_NONE, 513,
		FX_43, FX_NONE, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_6, FX_NONE, 513,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

	// Bat bite
	{
		kDamageBullet,
		4,
		0,
		921,
		0,
		0,
		1,
		4096,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	// Eel bite
	{
		kDamageBullet,
		12,
		0,
		1177,
		0,
		0,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	// Gill bite
	{
		kDamageBullet,
		9,
		0,
		1177,
		0,
		0,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Beast slash
	{
		kDamageExplode,
		50,
		43690,
		1024,
		8192,
		0,
		4,
		32768,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Axe
	{
		kDamageBullet,
		18,
		436906,
		1024,
		16384,
		0,
		2,
		20480,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

			// Cleaver
	{
		kDamageBullet,
		9,
		218453,
		1024,
		0,
		0,
		1,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

			// Phantasm slash
	{
		kDamageBullet,
		20,
		436906,
		1024,
		16384,
		0,
		3,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

			// Gargoyle Slash
	{
		kDamageBullet,
		16,
		218453,
		1024,
		8192,
		0,
		4,
		20480,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
	},

			// Cerberus bite
	{
		kDamageBullet,
		19,
		218453,
		614,
		8192,
		0,
		2,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Hound bite
	{
		kDamageBullet,
		10,
		218453,
		614,
		8192,
		0,
		2,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Rat bite
	{
		kDamageBullet,
		4,
		0,
		921,
		0,
		0,
		1,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Spider bite
	{
		kDamageBullet,
		8,
		0,
		614,
		0,
		0,
		1,
		24576,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

			// Unk
	{
		kDamageBullet,
		9,
		0,
		512,
		0,
		0,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_5, FX_NONE, 500,
		FX_NONE, FX_5, FX_NONE, 501,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_0, FX_NONE, 503,
		FX_NONE, FX_4, FX_NONE, -1,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_6, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 502,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	{
		(DAMAGE_TYPE)-1,
		0,
		0,
		2560,
		0,
		0,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_34, FX_35, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	// Tchernobog burn vector
	{
		kDamageBurn,
		2,
		0,
		0,
		0,
		15,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	// Vodoo 1.0 vector
	{
		kDamageSpirit,
		25,
		0,
		0,
		0,
		0,
		0,
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, -1,
	},

	// 22 kVectorGenDudePunch
	{
	kDamageFall, 
		37, 
		874762, 
		620, 
		0, 
		0, 
		0, 
		0,
		FX_NONE, FX_NONE, FX_NONE, -1,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
		FX_NONE, FX_NONE, FX_NONE, 357,
	},
};

const ITEMDATA gItemData[] = {
	{
		0,
		2552,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2553,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2554,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2555,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2556,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2557,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		2558,
		-8,
		0,
		32,
		32,
		-1,
	},
	{
		0,
		519,
		-8,
		0,
		48,
		48,
		0,
	},
	{
		0,
		822,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		2169,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		2433,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		517,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		783,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		896,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		825,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		827,
		-8,
		0,
		40,
		40,
		4,
	},
	{
		0,
		828,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		829,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		830,
		-8,
		0,
		80,
		64,
		1,
	},
	{
		0,
		831,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		863,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		760,
		-8,
		0,
		40,
		40,
		2,
	},
	{
		0,
		836,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		851,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		2428,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		839,
		-8,
		0,
		40,
		40,
		3,
	},
	{
		0,
		768,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		840,
		-8,
		0,
		48,
		48,
		-1,
	},
	{
		0,
		841,
		-8,
		0,
		48,
		48,
		-1,
	},
	{
		0,
		842,
		-8,
		0,
		48,
		48,
		-1,
	},
	{
		0,
		843,
		-8,
		0,
		48,
		48,
		-1,
	},
	{
		0,
		683,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		521,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		604,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		520,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		803,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		518,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		522,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		523,
		-8,
		0,
		40,
		40,
		-1,
	},
	{
		0,
		837,
		-8,
		0,
		80,
		64,
		-1,
	},
	{
		0,
		2628,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		2586,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		2578,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		2602,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		2594,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		753,
		-8,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		753,
		-8,
		7,
		64,
		64,
		-1,
	},
	{
		0,
		3558,
		-128,
		0,
		64,
		64,
		-1,
	},
	{
		0,
		3558,
		-128,
		7,
		64,
		64,
		-1,
	}
};

const AMMOITEMDATA gAmmoItemData[] = {
	{
		0,
		618,
		-8,
		0,
		40,
		40,
		480,
		6,
		7
	},
	{
		0,
		589,
		-8,
		0,
		48,
		48,
		1,
		5,
		6
	},
	{
		0,
		589,
		-8,
		0,
		48,
		48,
		1,
		5,
		6
	},
	{
		0,
		809,
		-8,
		0,
		48,
		48,
		5,
		5,
		6
	},
	{
		0,
		811,
		-8,
		0,
		48,
		48,
		1,
		10,
		11
	},
	{
		0,
		810,
		-8,
		0,
		48,
		48,
		1,
		11,
		12
	},
	{
		0,
		820,
		-8,
		0,
		24,
		24,
		10,
		8,
		0
	},
	{
		0,
		619,
		-8,
		0,
		48,
		48,
		4,
		2,
		0
	},
	{
		0,
		812,
		-8,
		0,
		48,
		48,
		15,
		2,
		0
	},
	{
		0,
		813,
		-8,
		0,
		48,
		48,
		15,
		3,
		0
	},
	{
		0,
		525,
		-8,
		0,
		48,
		48,
		100,
		9,
		10
	},
	{
		0,
		814,
		-8,
		0,
		48,
		48,
		15,
		255,
		0
	},
	{
		0,
		817,
		-8,
		0,
		48,
		48,
		100,
		3,
		0
	},
	{
		0,
		548,
		-8,
		0,
		24,
		24,
		32,
		7,
		0
	},
	{
		0,
		0,
		-8,
		0,
		48,
		48,
		6,
		255,
		0
	},
	{
		0,
		0,
		-8,
		0,
		48,
		48,
		6,
		255,
		0
	},
	{
		0,
		816,
		-8,
		0,
		48,
		48,
		8,
		1,
		0
	},
	{
		0,
		818,
		-8,
		0,
		48,
		48,
		8,
		255,
		0
	},
	{
		0,
		819,
		-8,
		0,
		48,
		48,
		8,
		255,
		0
	},
	{
		0,
		801,
		-8,
		0,
		48,
		48,
		6,
		4,
		0
	},
	{
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	},
};

const WEAPONITEMDATA gWeaponItemData[] = {
	{
		0,
		-1,
		0,
		0,
		0,
		0,
		0,
		-1,
		0
	},
	{
		0,
		559,
		-8,
		0,
		48,
		48,
		3,
		2,
		8
	},
	{
		0,
		558,
		-8,
		0,
		48,
		48,
		4,
		3,
		50
	},
	{
		0,
		524,
		-8,
		0,
		48,
		48,
		2,
		1,
		9
	},
	{
		0,
		525,
		-8,
		0,
		48,
		48,
		10,
		9,
		100
	},
	{
		0,
		539,
		-8,
		0,
		48,
		48,
		8,
		7,
		64
	},
	{
		0,
		526,
		-8,
		0,
		48,
		48,
		5,
		4,
		6
	},
	{
		0,
		-1,
		0,
		0,
		0,
		0,
		1,
		-1,
		0
	},
	{
		0,
		618,
		-8,
		0,
		48,
		48,
		7,
		6,
		480
	},
	{
		0,
		589,
		-8,
		0,
		48,
		48,
		6,
		5,
		1
	},
	{
		0,
		800,
		-8,
		0,
		48,
		48,
		9,
		8,
		35
	}
};

const MissileType missileInfo[] = {
	// Cleaver
	{
		2138,
		978670,
		512,
		40,
		40,
		-16,
		16,
	},
	// Regular flare
	{
		2424,
		3145728,
		0,
		32,
		32,
		-128,
		32,
	},
	// Tesla alt
	{
		3056,
		2796202,
		0,
		32,
		32,
		-128,
		32,
	},
	// Flare alt
	{
		2424,
		2446677,
		0,
		32,
		32,
		-128,
		4,
	},
	// Spray flame
	{
		0,
		1118481,
		0,
		24,
		24,
		-128,
		16,
	},
	// Fireball
	{
		0,
		1118481,
		0,
		32,
		32,
		-128,
		32,
	},
	// Tesla regular
	{
		2130,
		2796202,
		0,
		32,
		32,
		-128,
		16,
	},
	// EctoSkull
	{
		870,
		699050,
		0,
		32,
		32,
		-24,
		32,
	},
	// Hellhound flame
	{
		0,
		1118481,
		0,
		24,
		24,
		-128,
		16,
	},
	// Puke
	{
		0,
		838860,
		0,
		16,
		16,
		-16,
		16,
	},
	// Reserved
	{
		0,
		838860,
		0,
		8,
		8,
		0,
		16,
	},
	// Stone gargoyle projectile
	{
		3056,
		2097152,
		0,
		32,
		32,
		-128,
		16,
	},
	// Napalm launcher
	{
		0,
		2446677,
		0,
		30,
		30,
		-128,
		24,
	},
	// Cerberus fireball
	{
		0,
		2446677,
		0,
		30,
		30,
		-128,
		24,
	},
	// Tchernobog fireball
	{
		0,
		1398101,
		0,
		24,
		24,
		-128,
		16,
	},
	// Regular life leech
	{
		2446,
		2796202,
		0,
		32,
		32,
		-128,
		16,
	},
	// Dropped life leech (enough ammo)
	{
		3056,
		2446677,
		0,
		16,
		16,
		-128,
		16,
	},
	// Dropped life leech (no ammo)
	{
		3056,
		1747626,
		0,
		32,
		32,
		-128,
		16,
	}
};

const THINGINFO thingInfo[] = {
	//TNT Barrel
	{
		25,
		250,
		32,
		11,
		4096,
		80,
		CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_BLOCK_HITSCAN,
		907,
		0,
		0,
		0,
		0,
		256, 256, 128, 64, 0, 0, 128,
	},

	// Armed Proxy Dynamite
	{
		5,
		5,
		16,
		3,
		24576,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3444,
		-16,
		0,
		32,
		32,
		256, 256, 256, 64, 0, 0, 512,
	},
	// Armed Remote Dynamite
	{
		5,
		5,
		16,
		3,
		24576,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3457,
		-16,
		0,
		32,
		32,
		256, 256, 256, 64, 0, 0, 512,
	},
	// Vase1
	{
		1,
		20,
		32,
		3,
		32768,
		80,
		0,
		739,
		0,
		0,
		0,
		0,
		256, 0, 256, 128, 0, 0, 0,
	},
	// Vase2
	{
		1,
		150,
		32,
		3,
		32768,
		80,
		0,
		642,
		0,
		0,
		0,
		0,
		256, 256, 256, 128, 0, 0, 0,
	},
	// Crate face
	{
		10,
		0,
		0,
		0,
		0,
		0,
		0,
		462,
		0,
		0,
		0,
		0,
		0, 0, 0, 256, 0, 0, 0,
	},
	// Glass window
	{
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		266,
		0,
		0,
		0,
		0,
		256, 0, 256, 256, 0, 0, 0,
	},
	// Flourescent Light
	{
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		796,
		0,
		0,
		0,
		0,
		256, 0, 256, 256, 0, 0, 512,
	},
	// Wall Crack
	{
		50,
		0,
		0,
		0,
		0,
		0,
		0,
		1127,
		0,
		0,
		0,
		0,
		0, 0, 0, 256, 0, 0, 0,
	},
	// Wood Beam
	{
		8,
		0,
		0,
		0,
		0,
		0,
		0,
		1142,
		0,
		0,
		0,
		0,
		256, 0, 256, 128, 0, 0, 0,
	},
	// Spider's Web
	{
		4,
		0,
		0,
		0,
		0,
		0,
		0,
		1069,
		0,
		0,
		0,
		0,
		256, 256, 64, 256, 0, 0, 128,
	},
	// Metal Grate
	{
		40,
		0,
		0,
		0,
		0,
		0,
		0,
		483,
		0,
		0,
		0,
		0,
		64, 0, 128, 256, 0, 0, 0,
	},
	// Flammable Tree
	{
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		0, 256, 0, 256, 0, 0, 128,
	},
	// MachineGun Trap
	{
		1000,
		0,
		0,
		8,
		0,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		0, 0, 128, 256, 0, 0, 512,
	},
	// Falling Rock
	{
		0,
		15,
		8,
		3,
		32768,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		0, 0, 0, 0, 0, 0, 0,
	},
	// Kickable Pail
	{
		0,
		8,
		48,
		3,
		49152,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		0, 0, 0, 0, 0, 0, 0,
	},
	// Gib Object
	{
		10,
		2,
		0,
		0,
		32768,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		256, 0, 256, 256, 0, 0, 128,
	},
	// Explode Object
	{
		20,
		2,
		0,
		0,
		32768,
		0,
		0,
		-1,
		0,
		0,
		0,
		0,
		0, 0, 0, 256, 0, 0, 128,
	},
	// Armed stick Of TNT
	{
		5,
		14,
		16,
		3,
		24576,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3422,
		-32,
		0,
		32,
		32,
		64, 256, 128, 64, 0, 0, 256,
	},
	// Armed bundle Of TNT
	{
		5,
		14,
		16,
		3,
		24576,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3433,
		-32,
		0,
		32,
		32,
		64, 256, 128, 64, 0, 0, 256,
	},
	// Armed aerosol
	{
		5,
		14,
		16,
		3,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3467,
		-128,
		0,
		32,
		32,
		64, 256, 128, 64, 0, 0, 256,
	},
	// Bone (Flesh Garg.)
	{
		5,
		6,
		16,
		3,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		1462,
		0,
		0,
		32,
		32,
		0, 0, 0, 0, 0, 0, 0,
	},
	// Some alpha stuff
	{
		8,
		3,
		16,
		11,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		-1,
		0,
		0,
		0,
		0,
		256, 0, 256, 256, 0, 0, 0,
	},
	// WaterDrip 
	{
		0,
		1,
		1,
		2,
		0,
		0,
		0,
		1147,
		0,
		10,
		0,
		0,
		0, 0, 0, 0, 0, 0, 0,
	},
	// BloodDrip 
	{
		0,
		1,
		1,
		2,
		0,
		0,
		0,
		1160,
		0,
		2,
		0,
		0,
		0, 0, 0, 0, 0, 0, 0,
	},
	// Blood chucks1 
	{
		15,
		4,
		4,
		3,
		24576,
		0,
		CSTAT_SPRITE_BLOCK_ALL,
		-1,
		0,
		0,
		0,
		0,
		128, 64, 256, 256, 0, 0, 256,
	},
	// Blood chucks2
	{
		30,
		30,
		8,
		3,
		8192,
		0,
		CSTAT_SPRITE_BLOCK_ALL,
		-1,
		0,
		0,
		0,
		0,
		128, 64, 256, 256, 0, 0, 64,
	},
	// Axe Zombie Head 
	{
		60,
		5,
		32,
		3,
		40960,
		1280,
		CSTAT_SPRITE_BLOCK_ALL,
		3405,
		0,
		0,
		40,
		40,
		128, 64, 256, 256, 0, 0, 64,
	},
	// Napalm's Alt Fire explosion
	{
		80,
		30,
		32,
		3,
		57344,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3281,
		-128,
		0,
		32,
		32,
		0, 0, 0, 0, 0, 0, 0,
	},
	// Fire Pod Explosion
	{
		80,
		30,
		32,
		3,
		57344,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		2020,
		-128,
		0,
		32,
		32,
		256, 0, 256, 256, 0, 0, 0,
	},
	// Green Pod Explosion
	{
		80,
		30,
		32,
		3,
		57344,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		1860,
		-128,
		0,
		32,
		32,
		256, 0, 256, 256, 0, 0, 0,
	},
	// Life Leech
	{
		150,
		30,
		48,
		3,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_ALL,
		800,
		-128,
		0,
		48,
		48,
		64, 64, 112, 64, 0, 96, 96,
	},
	// Voodoo Head
	{
		1,
		30,
		48,
		3,
		32768,
		1600,
		0,
		2443,
		-128,
		0,
		16,
		16,
		0, 0, 0, 0, 0, 0, 0,
	},
	// 433 - kModernThingTNTProx
	{
		5,
		5,
		16,
		3,
		24576,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		3444,
		-16,
		7,
		32,
		32,
		256, 256, 256, 64, 0, 0, 512,
	},
	// 434 - kModernThingThrowableRock
	{
		5,
		6,
		16,
		3,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_HITSCAN,
		1462,
		0,
		0,
		32,
		32,
		0, 0, 0, 0, 0, 0, 0,
	},
	// 435 - kModernThingEnemyLifeLeech
	{
		150,
		30,
		48,
		3,
		32768,
		1600,
		CSTAT_SPRITE_BLOCK_ALL,
		800,
		-128,
		0,
		44,
		44,
		0, 1024, 512, 1024, 0, 64, 512,
	},
};

const EXPLOSION explodeInfo[] = {
	{
		40,
		10,
		10,
		75,
		450,
		0,
		60,
		80,
		40
	},
	{
		80,
		20,
		10,
		150,
		900,
		0,
		60,
		160,
		60
	},
	{
		120,
		40,
		15,
		225,
		1350,
		0,
		60,
		240,
		80
	},
	{
		80,
		5,
		10,
		120,
		20,
		10,
		60,
		0,
		40
	},
	{
		120,
		10,
		10,
		180,
		40,
		10,
		60,
		0,
		80
	},
	{
		160,
		15,
		10,
		240,
		60,
		10,
		60,
		0,
		120
	},
	{
		40,
		20,
		10,
		120,
		0,
		10,
		30,
		60,
		40
	},
	{
		80,
		20,
		10,
		150,
		800,
		5,
		60,
		160,
		60
	},
};

static const int16_t gPlayerGibThingComments[] = {
	734, 735, 736, 737, 738, 739, 740, 741, 3038, 3049
};

const int16_t DudeDifficulty[5] = {
	512, 384, 256, 208, 160
};

struct POSTPONE
{
	DBloodActor* sprite;
	int status;
};

TArray<POSTPONE> gPost;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool IsUnderwaterSector(sectortype* pSector)
{
	return !!pSector->hasX() && pSector->xs().Underwater;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actInitTraps()
{
	BloodStatIterator it(kStatTraps);
	while (auto act = it.Next())
	{
		if (act->spr.type == kTrapExploder)
		{
			act->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
			act->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			if (!act->hasX()) continue;
			act->xspr.waitTime = ClipLow(act->xspr.waitTime, 1);
			act->xspr.state = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actInitThings()
{
	BloodStatIterator it(kStatThing);
	while (auto act = it.Next())
	{
		if (!act->hasX()) continue;

		int nType = act->spr.type - kThingBase;
		act->xspr.health = thingInfo[nType].startHealth << 4;
#ifdef NOONE_EXTENSIONS
		// allow level designer to set custom clipdist.
		// this is especially useful for various Gib and Explode objects which have clipdist 1 for some reason predefined,
		// but what if it have voxel model...?
		if (!gModernMap)
#endif
			act->spr.clipdist = thingInfo[nType].clipdist;

		act->spr.flags = thingInfo[nType].flags;
		if (act->spr.flags & kPhysGravity) act->spr.flags |= kPhysFalling;
		act->vel.X = act->vel.Y = act->vel.Z = 0;

		switch (act->spr.type)
		{
		case kThingArmedProxBomb:
		case kTrapMachinegun:
#ifdef NOONE_EXTENSIONS
		case kModernThingTNTProx:
#endif
			act->xspr.state = 0;
			break;
		case kThingBloodChunks:
		{
			SEQINST* pInst = GetInstance(act);
			if (pInst)
			{
				auto seq = getSequence(pInst->nSeqID);
				if (!seq) break;
				seqSpawn(pInst->nSeqID, act);
			}
			break;
		}
		default:
			act->xspr.state = 1;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actInitDudes()
{
	if (gGameOptions.nMonsterSettings == 0)
	{
		gKillMgr.SetCount(0);
		BloodStatIterator it(kStatDude);
		while (auto act = it.Next())
		{
			if (act->hasX() && act->xspr.key > 0) // Drop Key
				actDropObject(act, kItemKeyBase + (act->xspr.key - 1));
			DeleteSprite(act);
		}
	}
	else
	{
		// by NoOne: WTF is this?
		///////////////
		BloodStatIterator it(kStatDude);
		while (auto act = it.Next())
		{
			if (act->spr.type < kDudeBase || act->spr.type >= kDudeMax)
				I_Error("Non-enemy sprite (%d) in the enemy sprite list.\n", act->GetIndex());
		}

		gKillMgr.CountTotalKills();
		///////////////

		for (int i = 0; i < kDudeMax - kDudeBase; i++)
			for (int j = 0; j < 7; j++)
				dudeInfo[i].damageVal[j] = MulScale(DudeDifficulty[gGameOptions.nDifficulty], dudeInfo[i].startDamage[j], 8);

		it.Reset(kStatDude);
		while (auto act = it.Next())
		{
			if (!act->hasX()) continue;

			int nType = act->spr.type - kDudeBase;
			int seqStartId = dudeInfo[nType].seqStartID;
			if (!act->IsPlayerActor())
			{
#ifdef NOONE_EXTENSIONS
				switch (act->spr.type)
				{
				case kDudeModernCustom:
				case kDudeModernCustomBurning:
					act->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
					if (act->xspr.data2 > 0 && getSequence(act->xspr.data2))
						seqStartId = act->xspr.data2; //  Custom Dude stores it's SEQ in data2

					seqStartId = genDudeSeqStartId(act); //  Custom Dude stores its SEQ in data2
					act->xspr.sysData1 = act->xspr.data3; // move sndStartId to sysData1, because data3 used by the game;
					act->xspr.data3 = 0;
					break;

				case kDudePodMother:  // FakeDude type (no seq, custom flags, clipdist and cstat)
					if (gModernMap) break;
					[[fallthrough]];
				default:
					act->spr.clipdist = dudeInfo[nType].clipdist;
					act->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
					break;
				}
#else
				act->spr.clipdist = dudeInfo[nType].clipdist;
				act->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
#endif

				act->vel.X = act->vel.Y = act->vel.Z = 0;

#ifdef NOONE_EXTENSIONS
				// add a way to set custom hp for every enemy - should work only if map just started and not loaded.
				if (!gModernMap || act->xspr.sysData2 <= 0) act->xspr.health = dudeInfo[nType].startHealth << 4;
				else act->xspr.health = ClipRange(act->xspr.sysData2 << 4, 1, 65535);
#else
				act->xspr.health = dudeInfo[nType].startHealth << 4;
#endif

			}

			if (getSequence(seqStartId)) seqSpawn(seqStartId, act);
		}
		aiInit();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actInit(TArray<DBloodActor*>& actors)
{
#ifdef NOONE_EXTENSIONS
	if (!gModernMap) nnExtResetGlobals();
	else nnExtInitModernStuff(actors);
#endif

	BloodStatIterator it(kStatItem);
	while (auto act = it.Next())
	{
		if (act->spr.type == kItemWeaponVoodooDoll)
		{
			act->spr.type = kItemAmmoVoodooDoll;
			break;
		}
	}

	actInitTraps();
	actInitThings();
	actInitDudes();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void ConcussSprite(DBloodActor* source, DBloodActor* actor, int x, int y, int z, int damage)
{
	int dx = actor->int_pos().X - x;
	int dy = actor->int_pos().Y - y;
	int dz = (actor->int_pos().Z - z) >> 4;
	int dist2 = 0x40000 + dx * dx + dy * dy + dz * dz;
	assert(dist2 > 0);
	damage = Scale(0x40000, damage, dist2);

	if (actor->spr.flags & kPhysMove)
	{
		int mass = 0;
		if (actor->IsDudeActor())
		{
			mass = getDudeInfo(actor->spr.type)->mass;
#ifdef NOONE_EXTENSIONS
			if (actor->spr.type == kDudeModernCustom || actor->spr.type == kDudeModernCustomBurning)
			{
				mass = getSpriteMassBySize(actor);
			}
#endif
		}
		else if (actor->spr.type >= kThingBase && actor->spr.type < kThingMax)
		{
			mass = thingInfo[actor->spr.type - kThingBase].mass;
		}
		else
		{
			Printf(PRINT_HIGH, "Unexpected type in ConcussSprite(): Sprite: %d  Type: %d  Stat: %d", actor->GetIndex(), (int)actor->spr.type, (int)actor->spr.statnum);
			return;
		}

		if (mass > 0)
		{
			int size = (tileWidth(actor->spr.picnum) * actor->spr.xrepeat * tileHeight(actor->spr.picnum) * actor->spr.yrepeat) >> 1;
			int t = Scale(damage, size, mass);
			actor->vel.X += MulScale(t, dx, 16);
			actor->vel.Y += MulScale(t, dy, 16);
			actor->vel.Z += MulScale(t, dz, 16);
		}
	}
	actDamageSprite(source, actor, kDamageExplode, damage);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actWallBounceVector(int* x, int* y, walltype* pWall, int a4)
{
	int wx, wy;
	GetWallNormal(pWall, &wx, &wy);
	int t = DMulScale(*x, wx, *y, wy, 16);
	int t2 = mulscale16r(t, a4 + 0x10000);
	*x -= MulScale(wx, t2, 16);
	*y -= MulScale(wy, t2, 16);
	return mulscale16r(t, 0x10000 - a4);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actFloorBounceVector(int* x, int* y, int* z, sectortype* pSector, int a5)
{
	int t = 0x10000 - a5;
	if (pSector->floorheinum == 0)
	{
		int t2 = MulScale(*z, t, 16);
		*z = -(*z - t2);
		return t2;
	}
	walltype* pWall = pSector->firstWall();
	walltype* pWall2 = pWall->point2Wall();
	int angle = getangle(pWall2->wall_int_pos().X - pWall->wall_int_pos().X, pWall2->wall_int_pos().Y - pWall->wall_int_pos().Y) + 512;
	int t2 = pSector->floorheinum << 4;
	int t3 = approxDist(-0x10000, t2);
	int t4 = DivScale(-0x10000, t3, 16);
	int t5 = DivScale(t2, t3, 16);
	int t6 = MulScale(t5, Cos(angle), 30);
	int t7 = MulScale(t5, Sin(angle), 30);
	int t8 = TMulScale(*x, t6, *y, t7, *z, t4, 16);
	int t9 = MulScale(t8, 0x10000 + a5, 16);
	*x -= MulScale(t6, t9, 16);
	*y -= MulScale(t7, t9, 16);
	*z -= MulScale(t4, t9, 16);
	return mulscale16r(t8, t);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actRadiusDamage(DBloodActor* source, int x, int y, int z, sectortype* pSector, int nDist, int baseDmg, int distDmg, DAMAGE_TYPE dmgType, int flags, int burn)
{
	auto pOwner = source->GetOwner();
	const bool newSectCheckMethod = !cl_bloodvanillaexplosions && pOwner && pOwner->IsDudeActor() && !VanillaMode(); // use new sector checking logic
	auto sectorMap = GetClosestSpriteSectors(pSector, x, y, nDist, nullptr, newSectCheckMethod);
	nDist <<= 4;
	if (flags & 2)
	{
		BloodStatIterator it(kStatDude);
		while (auto act2 = it.Next())
		{
			if (act2 != source || (flags & 1))
			{
				if (act2->hasX())
				{
					if (act2->spr.flags & 0x20) continue;
					if (!CheckSector(sectorMap, act2)) continue;
					if (!CheckProximity(act2, x, y, z, pSector, nDist)) continue;

					int dx = abs(x - act2->int_pos().X);
					int dy = abs(y - act2->int_pos().Y);
					int dz = abs(z - act2->int_pos().Z) >> 4;
					int dist = ksqrt(dx * dx + dy * dy + dz * dz);
					if (dist > nDist) continue;

					int totaldmg;
					if (dist != 0) totaldmg = baseDmg + ((nDist - dist) * distDmg) / nDist;
					else totaldmg = baseDmg + distDmg;

					actDamageSprite(source, act2, dmgType, totaldmg << 4);
					if (burn) actBurnSprite(pOwner, act2, burn);
				}
			}
		}
	}
	if (flags & 4)
	{
		BloodStatIterator it(kStatDude);
		while (auto act2 = it.Next())
		{
			if (act2->spr.flags & 0x20) continue;
			if (!CheckSector(sectorMap, act2)) continue;
			if (!CheckProximity(act2, x, y, z, pSector, nDist)) continue;

			if (act2->xspr.locked) continue;

			int dx = abs(x - act2->int_pos().X);
			int dy = abs(y - act2->int_pos().Y);
			int dist = ksqrt(dx * dx + dy * dy);
			if (dist > nDist) continue;

			int totaldmg;
			if (dist != 0) totaldmg = baseDmg + ((nDist - dist) * distDmg) / nDist;
			else totaldmg = baseDmg + distDmg;

			actDamageSprite(source, act2, dmgType, totaldmg << 4);
			if (burn) actBurnSprite(pOwner, act2, burn);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actNapalmMove(DBloodActor* actor)
{
	auto pOwner = actor->GetOwner();

	actPostSprite(actor, kStatDecoration);
	seqSpawn(9, actor);
	if (Chance(0x8000)) actor->spr.cstat |= CSTAT_SPRITE_XFLIP;

	sfxPlay3DSound(actor, 303, 24 + (actor->spr.flags & 3), 1);
	actRadiusDamage(pOwner, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), 128, 0, 60, kDamageExplode, 15, 120);

	if (actor->xspr.data4 > 1)
	{
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		int spawnparam[2];
		spawnparam[0] = actor->xspr.data4 >> 1;
		spawnparam[1] = actor->xspr.data4 - spawnparam[0];
		int ang = actor->spr.ang;
		actor->vel.X = 0;
		actor->vel.Y = 0;
		actor->vel.Z = 0;
		for (int i = 0; i < 2; i++)
		{
			int t1 = Random(0x33333) + 0x33333;
			int rndang = Random2(0x71);
			actor->spr.ang = (rndang + ang + 2048) & 2047;
			auto spawned = actFireThing(actor, 0, 0, -0x93d0, kThingNapalmBall, t1);
			spawned->SetOwner(actor->GetOwner());
			seqSpawn(61, spawned, nNapalmClient);
			spawned->xspr.data4 = spawnparam[i];
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DBloodActor* actSpawnFloor(DBloodActor* actor)
{
	auto pSector = actor->sector();
	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	updatesector(x, y, &pSector);
	int zFloor = getflorzofslopeptr(pSector, x, y);
	auto spawned = actSpawnSprite(pSector, x, y, zFloor, 3, 0);
	if (spawned) spawned->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	return spawned;
}

static DBloodActor* actDropAmmo(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	if (actor->spr.statnum < kMaxStatus && nType >= kItemAmmoBase && nType < kItemAmmoMax)
	{
		auto act2 = actSpawnFloor(actor);
		const AMMOITEMDATA* pAmmo = &gAmmoItemData[nType - kItemAmmoBase];
		act2->spr.type = nType;
		act2->spr.picnum = pAmmo->picnum;
		act2->spr.shade = pAmmo->shade;
		act2->spr.xrepeat = pAmmo->xrepeat;
		act2->spr.yrepeat = pAmmo->yrepeat;
		return act2;
	}
	return nullptr;
}

static DBloodActor* actDropWeapon(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	if (actor->spr.statnum < kMaxStatus && nType >= kItemWeaponBase && nType < kItemWeaponMax)
	{
		auto act2 = actSpawnFloor(actor);
		const WEAPONITEMDATA* pWeapon = &gWeaponItemData[nType - kItemWeaponBase];
		act2->spr.type = nType;
		act2->spr.picnum = pWeapon->picnum;
		act2->spr.shade = pWeapon->shade;
		act2->spr.xrepeat = pWeapon->xrepeat;
		act2->spr.yrepeat = pWeapon->yrepeat;
		return act2;
	}
	return nullptr;
}

static DBloodActor* actDropItem(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	if (actor->spr.statnum < kMaxStatus && nType >= kItemBase && nType < kItemMax)
	{
		auto act2 = actSpawnFloor(actor);
		const ITEMDATA* pItem = &gItemData[nType - kItemBase];
		act2->spr.type = nType;
		act2->spr.picnum = pItem->picnum;
		act2->spr.shade = pItem->shade;
		act2->spr.xrepeat = pItem->xrepeat;
		act2->spr.yrepeat = pItem->yrepeat;
		return act2;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DBloodActor* actDropKey(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	if (actor->spr.statnum < kMaxStatus && nType >= kItemKeyBase && nType < kItemKeyMax)
	{
		auto act2 = actDropItem(actor, nType);
		if (act2 && gGameOptions.nGameType == 1)
		{
			act2->addX();
			act2->xspr.respawn = 3;
			act2->hit.florhit.setNone();
			act2->hit.ceilhit.setNone();
		}
		return act2;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DBloodActor* actDropFlag(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	if (actor->spr.statnum < kMaxStatus && (nType == 147 || nType == 148))
	{
		auto act2 = actDropItem(actor, nType);
		if (act2 && gGameOptions.nGameType == 3)
		{
			evPostActor(act2, 1800, kCallbackReturnFlag);
		}
		return act2;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actDropObject(DBloodActor* actor, int nType)
{
	DBloodActor* act2 = nullptr;

	if (nType >= kItemKeyBase && nType < kItemKeyMax) act2 = actDropKey(actor, nType);
	else if (nType == kItemFlagA || nType == kItemFlagB) act2 = actDropFlag(actor, nType);
	else if (nType >= kItemBase && nType < kItemMax) act2 = actDropItem(actor, nType);
	else if (nType >= kItemAmmoBase && nType < kItemAmmoMax) act2 = actDropAmmo(actor, nType);
	else if (nType >= kItemWeaponBase && nType < kItemWeaponMax) act2 = actDropWeapon(actor, nType);

	if (act2)
	{
		int top, bottom;
		GetActorExtents(act2, &top, &bottom);
		if (bottom >= act2->int_pos().Z)
			act2->add_int_z(-(bottom - act2->int_pos().Z));
	}

	return act2;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool actHealDude(DBloodActor* actor, int add, int threshold)
{
	if (!actor) return false;
	add <<= 4;
	threshold <<= 4;
	if (actor->xspr.health < (unsigned)threshold)
	{
		if (actor->IsPlayerActor()) sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 780, actor->sector());
		actor->xspr.health = min<uint32_t>(actor->xspr.health + add, threshold);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

#ifdef NOONE_EXTENSIONS
static bool actKillModernDude(DBloodActor* actor, DAMAGE_TYPE damageType)
{
	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	removeDudeStuff(actor);
	if (actor->xspr.txID <= 0 || getNextIncarnation(actor) == nullptr)
	{
		if (pExtra->weaponType == kGenDudeWeaponKamikaze && Chance(0x4000) && damageType != kDamageSpirit && damageType != kDamageDrown)
		{
			doExplosion(actor, actor->xspr.data1 - kTrapExploder);
			if (Chance(0x9000)) damageType = kDamageExplode;
		}

		if (damageType == kDamageBurn)
		{
			if (pExtra->availDeaths[kDamageBurn] && !spriteIsUnderwater(actor))
			{
				if (pExtra->canBurn)
				{
					actor->spr.type = kDudeModernCustomBurning;
					if (actor->xspr.data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
						actor->spr.pal = 0;

					aiGenDudeNewState(actor, &genDudeBurnGoto);
					actHealDude(actor, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
					if (actor->xspr.burnTime <= 0) actor->xspr.burnTime = 1200;
					actor->dudeExtra.time = PlayClock + 360;
					return true;
				}

			}
			else
			{
				actor->xspr.burnTime = 0;
				actor->SetBurnSource(nullptr);
				damageType = kDamageFall;
			}
		}
	}
	else
	{
		actor->xspr.locked = 1; // lock while transforming

		aiSetGenIdleState(actor); // set idle state

		if (actor->xspr.key > 0) // drop keys
			actDropObject(actor, kItemKeyBase + actor->xspr.key - 1);

		if (actor->xspr.dropMsg > 0) // drop items
			actDropObject(actor, actor->xspr.dropMsg);

		actor->spr.flags &= ~kPhysMove;
		actor->vel.X = actor->vel.Y = 0;

		playGenDudeSound(actor, kGenDudeSndTransforming);
		int seqId = actor->xspr.data2 + kGenDudeSeqTransform;
		if (getSequence(seqId)) seqSpawn(seqId, actor, -1);
		else
		{
			seqKill(actor);
			DBloodActor* pEffect = gFX.fxSpawnActor((FX_ID)52, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->spr.ang);
			if (pEffect != nullptr)
			{
				pEffect->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
				pEffect->spr.pal = 6;
				pEffect->spr.xrepeat = actor->spr.xrepeat;
				pEffect->spr.yrepeat = actor->spr.yrepeat;
			}

			GIBTYPE nGibType;
			for (int i = 0; i < 3; i++)
			{
				if (Chance(0x3000)) nGibType = GIBTYPE_6;
				else if (Chance(0x2000)) nGibType = GIBTYPE_5;
				else nGibType = GIBTYPE_17;

				int top, bottom;
				GetActorExtents(actor, &top, &bottom);
				CGibPosition gibPos(actor->int_pos().X, actor->int_pos().Y, top);
				CGibVelocity gibVel(actor->vel.X >> 1, actor->vel.Y >> 1, -0xccccc);
				GibSprite(actor, nGibType, &gibPos, &gibVel);
			}
		}

		actor->xspr.sysData1 = kGenDudeTransformStatus; // in transform
		return true;
	}
	return false;
}
#endif

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool actKillDudeStage1(DBloodActor* actor, DAMAGE_TYPE damageType)
{
	switch (actor->spr.type)
	{
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
		if (actKillModernDude(actor, damageType)) return true;
		break;
#endif
	case kDudeCerberusTwoHead: // Cerberus
		seqSpawn(dudeInfo[actor->spr.type - kDudeBase].seqStartID + 1, actor, -1);
		return true;

	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
		if (damageType == kDamageBurn && actor->xspr.medium == kMediumNormal)
		{
			actor->spr.type = kDudeBurningCultist;
			aiNewState(actor, &cultistBurnGoto);
			actHealDude(actor, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
			return true;
		}
		break;

	case kDudeBeast:
		if (damageType == kDamageBurn && actor->xspr.medium == kMediumNormal)
		{
			actor->spr.type = kDudeBurningBeast;
			aiNewState(actor, &beastBurnGoto);
			actHealDude(actor, dudeInfo[53].startHealth, dudeInfo[53].startHealth);
			return true;
		}
		break;

	case kDudeInnocent:
		if (damageType == kDamageBurn && actor->xspr.medium == kMediumNormal)
		{
			actor->spr.type = kDudeBurningInnocent;
			aiNewState(actor, &innocentBurnGoto);
			actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
			return true;
		}
		break;

	case kDudeTinyCaleb:
		if (cl_bloodvanillaenemies || VanillaMode())
			break;
		if (damageType == kDamageBurn && actor->xspr.medium == kMediumNormal)
		{
			actor->spr.type = kDudeBurningTinyCaleb;
			aiNewState(actor, &tinycalebBurnGoto);
			actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
			return true;
		}
		break;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void checkAddFrag(DBloodActor* killerActor, DBloodActor* actor)
{
	if (VanillaMode())
	{
		if (killerActor->IsPlayerActor())
		{
			PLAYER* pPlayer = &gPlayer[killerActor->spr.type - kDudePlayer1];
			if (gGameOptions.nGameType == 1)
				pPlayer->fragCount++;
		}
	}
	else if (gGameOptions.nGameType == 1 && killerActor->IsPlayerActor() && actor->spr.statnum == kStatDude)
	{
		switch (actor->spr.type)
		{
		case kDudeBat:
		case kDudeRat:
		case kDudeInnocent:
		case kDudeBurningInnocent:
			break;
		default:
			PLAYER* pKillerPlayer = &gPlayer[killerActor->spr.type - kDudePlayer1];
			pKillerPlayer->fragCount++;
			break;
		}

	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void checkDropObjects(DBloodActor* actor)
{
	if (actor->xspr.key > 0) actDropObject(actor, kItemKeyBase + actor->xspr.key - 1);
	if (actor->xspr.dropMsg > 0) actDropObject(actor, actor->xspr.dropMsg);

	switch (actor->spr.type)
	{
	case kDudeCultistTommy:
	{
		int nRand = Random(100);
		if (nRand < 10) actDropObject(actor, kItemWeaponTommygun);
		else if (nRand < 50) actDropObject(actor, kItemAmmoTommygunFew);
		break;
	}
	case kDudeCultistShotgun:
	{
		int nRand = Random(100);
		if (nRand <= 10) actDropObject(actor, kItemWeaponSawedoff);
		else if (nRand <= 50) actDropObject(actor, kItemAmmoSawedoffFew);
		break;
	}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int checkDamageType(DBloodActor* actor, DAMAGE_TYPE damageType)
{
	int nSeq;

	switch (damageType)
	{
	case kDamageExplode:
		nSeq = 2;
		switch (actor->spr.type)
		{
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustom:
		case kDudeModernCustomBurning:
		{
			playGenDudeSound(actor, kGenDudeSndDeathExplode);
			GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
			if (!pExtra->availDeaths[damageType])
			{
				nSeq = 1;
				damageType = kDamageFall;
			}
			break;
		}
#endif
		case kDudeCultistTommy:
		case kDudeCultistShotgun:
		case kDudeCultistTommyProne:
		case kDudeBurningInnocent:
		case kDudeBurningCultist:
		case kDudeInnocent:
		case kDudeCultistShotgunProne:
		case kDudeCultistTesla:
		case kDudeCultistTNT:
		case kDudeCultistBeast:
		case kDudeTinyCaleb:
		case kDudeBurningTinyCaleb:
			sfxPlay3DSound(actor, 717, -1, 0);
			break;
		}
		break;
	case kDamageBurn:
		nSeq = 3;
		sfxPlay3DSound(actor, 351, -1, 0);
		break;
	case kDamageSpirit:
		switch (actor->spr.type) {
		case kDudeZombieAxeNormal:
		case kDudeZombieAxeBuried:
			nSeq = 14;
			break;
		case kDudeZombieButcher:
			nSeq = 11;
			break;
		default:
			nSeq = 1;
			break;
		}
		break;
	case kDamageFall:
		switch (actor->spr.type)
		{
		case kDudeCultistTommy:
		case kDudeCultistShotgun:
			nSeq = 1;
			break;
		default:
			nSeq = 1;
			break;
		}
		break;
	default:
		nSeq = 1;
		break;
	}
	return nSeq;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void spawnGibs(DBloodActor* actor, int type, int velz)
{
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	CGibPosition gibPos(actor->int_pos().X, actor->int_pos().Y, top);
	CGibVelocity gibVel(actor->vel.X >> 1, actor->vel.Y >> 1, velz);
	GibSprite(actor, GIBTYPE_27, &gibPos, &gibVel);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void zombieAxeNormalDeath(DBloodActor* actor, int nSeq)
{
	int nType = actor->spr.type - kDudeBase;

	sfxPlay3DSound(actor, 1107 + Random(2), -1, 0);
	if (nSeq == 2)
	{
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, nDudeToGibClient1);
		spawnGibs(actor, GIBTYPE_27, -0xccccc);
	}
	else if (nSeq == 1 && Chance(0x4000))
	{
		seqSpawn(dudeInfo[nType].seqStartID + 7, actor, nDudeToGibClient1);
		evPostActor(actor, 0, kCallbackFXZombieSpurt);
		sfxPlay3DSound(actor, 362, -1, 0);
		actor->xspr.data1 = 35;
		actor->xspr.data2 = 5;

		spawnGibs(actor, GIBTYPE_27, -0x111111);
	}
	else if (nSeq == 14)seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
	else if (nSeq == 3) seqSpawn(dudeInfo[nType].seqStartID + 13, actor, nDudeToGibClient2);
	else seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, nDudeToGibClient1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void burningCultistDeath(DBloodActor* actor, int nSeq)
{
	if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(actor, 718, -1, 0);
	else sfxPlay3DSound(actor, 1018 + Random(2), -1, 0);

	int nType = actor->spr.type - kDudeBase;
	if (Chance(0x8000))
	{
		for (int i = 0; i < 3; i++)
			GibSprite(actor, GIBTYPE_7, nullptr, nullptr);
		seqSpawn(dudeInfo[nType].seqStartID + 16 - Random(1), actor, nDudeToGibClient1);
	}
	else
		seqSpawn(dudeInfo[nType].seqStartID + 15, actor, nDudeToGibClient2);
}

#ifdef NOONE_EXTENSIONS
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void modernCustomDudeDeath(DBloodActor* actor, int nSeq, int damageType)
{
	playGenDudeSound(actor, kGenDudeSndDeathNormal);
	int dudeToGib = (actCheckRespawn(actor)) ? -1 : ((nSeq == 3) ? nDudeToGibClient2 : nDudeToGibClient1);
	if (nSeq == 3)
	{
		GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
		if (pExtra->availDeaths[kDmgBurn] == 3) seqSpawn((15 + Random(2)) + actor->xspr.data2, actor, dudeToGib);
		else if (pExtra->availDeaths[kDmgBurn] == 2) seqSpawn(16 + actor->xspr.data2, actor, dudeToGib);
		else if (pExtra->availDeaths[kDmgBurn] == 1) seqSpawn(15 + actor->xspr.data2, actor, dudeToGib);
		else if (getSequence(actor->xspr.data2 + nSeq))seqSpawn(nSeq + actor->xspr.data2, actor, dudeToGib);
		else seqSpawn(1 + actor->xspr.data2, actor, dudeToGib);

	}
	else
	{
		seqSpawn(nSeq + actor->xspr.data2, actor, dudeToGib);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void modernCustomDudeBurningDeath(DBloodActor* actor, int nSeq)
{
	playGenDudeSound(actor, kGenDudeSndDeathExplode);
	int dudeToGib = (actCheckRespawn(actor)) ? -1 : nDudeToGibClient1;

	if (Chance(0x4000)) spawnGibs(actor, GIBTYPE_27, -0xccccc);

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	int seqofs = actor->xspr.data2;
	if (pExtra->availDeaths[kDmgBurn] == 3) seqSpawn((15 + Random(2)) + seqofs, actor, dudeToGib);
	else if (pExtra->availDeaths[kDmgBurn] == 2) seqSpawn(16 + seqofs, actor, dudeToGib);
	else if (pExtra->availDeaths[kDmgBurn] == 1) seqSpawn(15 + seqofs, actor, dudeToGib);
	else seqSpawn(1 + seqofs, actor, dudeToGib);
}
#endif

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void zombieAxeBurningDeath(DBloodActor* actor, int nSeq)
{
	int nType = actor->spr.type - kDudeBase;

	if (Chance(0x8000) && nSeq == 3)
		sfxPlay3DSound(actor, 1109, -1, 0);
	else
		sfxPlay3DSound(actor, 1107 + Random(2), -1, 0);

	if (Chance(0x8000))
	{
		seqSpawn(dudeInfo[nType].seqStartID + 13, actor, nDudeToGibClient1);
		spawnGibs(actor, GIBTYPE_27, -0xccccc);
	}
	else
		seqSpawn(dudeInfo[nType].seqStartID + 13, actor, nDudeToGibClient2);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void zombieButcherDeath(DBloodActor* actor, int nSeq)
{
	int nType = actor->spr.type - kDudeBase;

	if (nSeq == 14)
	{
		sfxPlay3DSound(actor, 1206, -1, 0);
		seqSpawn(dudeInfo[nType].seqStartID + 11, actor, -1);
		return;
	}
	sfxPlay3DSound(actor, 1204 + Random(2), -1, 0);
	if (nSeq == 3)
		seqSpawn(dudeInfo[nType].seqStartID + 10, actor, -1);
	else
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void genericDeath(DBloodActor* actor, int nSeq, int sound1, int seqnum)
{
	if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(actor, sound1 + 2, -1, 0);
	else sfxPlay3DSound(actor, sound1 + Random(2), -1, 0);
	seqSpawn(seqnum, actor, -1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actKillDude(DBloodActor* killerActor, DBloodActor* actor, DAMAGE_TYPE damageType, int damage)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax&& actor->hasX());
	int nType = actor->spr.type - kDudeBase;

	if (actKillDudeStage1(actor, damageType)) return;

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (gPlayer[p].fragger == actor && gPlayer[p].deathTime > 0)
			gPlayer[p].fragger = nullptr;
	}
	if (actor->spr.type != kDudeCultistBeast)
		trTriggerSprite(actor, kCmdOff, killerActor);

	actor->spr.flags |= 7;
	checkAddFrag(killerActor, actor);
	checkDropObjects(actor);

	int nSeq = checkDamageType(actor, damageType);

	if (!getSequence(getDudeInfo(nType + kDudeBase)->seqStartID + nSeq))
	{
		seqKill(actor);
		gKillMgr.AddKill(actor);
		actPostSprite(actor, kStatFree);
		return;
	}

	auto Owner = actor->GetOwner();
	switch (actor->spr.type)
	{
	case kDudeZombieAxeNormal:
		zombieAxeNormalDeath(actor, nSeq);
		break;

	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
		sfxPlay3DSound(actor, 1018 + Random(2), -1, 0);
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, nSeq == 3 ? nDudeToGibClient2 : nDudeToGibClient1);
		break;

	case kDudeBurningCultist:
		burningCultistDeath(actor, nSeq);
		damageType = kDamageExplode;
		break;

#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
		modernCustomDudeDeath(actor, nSeq, damageType);
		genDudePostDeath(actor, damageType, damage);
		return;

	case kDudeModernCustomBurning:
		modernCustomDudeBurningDeath(actor, nSeq);
		genDudePostDeath(actor, kDamageExplode, damage);
		return;
#endif

	case kDudeBurningZombieAxe:
		zombieAxeBurningDeath(actor, nSeq);
		damageType = kDamageExplode;
		break;

	case kDudeBurningZombieButcher:
		genericDeath(actor, nSeq, 1204, dudeInfo[4].seqStartID + 10);
		break;

	case kDudeBurningInnocent:
		damageType = kDamageExplode;
		seqSpawn(dudeInfo[nType].seqStartID + 7, actor, nDudeToGibClient1);
		break;

	case kDudeZombieButcher:
		zombieButcherDeath(actor, nSeq);
		break;

	case kDudeGargoyleFlesh:
		genericDeath(actor, nSeq, 1403, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeGargoyleStone:
		genericDeath(actor, nSeq, 1453, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudePhantasm:
		genericDeath(actor, nSeq, 1603, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeHellHound:
		genericDeath(actor, nSeq, 1303, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeHand:
		genericDeath(actor, nSeq, 1903, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeSpiderBrown:
		if (Owner) Owner->dudeExtra.stats.birthCounter--;
		genericDeath(actor, nSeq, 1803, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeSpiderRed:
		if (Owner) Owner->dudeExtra.stats.birthCounter--;
		genericDeath(actor, nSeq, 1803, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeSpiderBlack:
		if (Owner) Owner->dudeExtra.stats.birthCounter--;
		genericDeath(actor, nSeq, 1803, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeSpiderMother:
		sfxPlay3DSound(actor, 1850, -1, 0);
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
		break;

	case kDudeGillBeast:
		genericDeath(actor, nSeq, 1703, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeBoneEel:
		genericDeath(actor, nSeq, 1503, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeBat:
		genericDeath(actor, nSeq, 2003, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeRat:
		genericDeath(actor, nSeq, 2103, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudePodGreen:
	case kDudeTentacleGreen:
	case kDudePodFire:
	case kDudeTentacleFire:
		if ((actor->spr.cstat & CSTAT_SPRITE_YFLIP)) actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;
		switch (actor->spr.type)
		{
		case kDudePodGreen:
			genericDeath(actor, nSeq, 2203, dudeInfo[nType].seqStartID + nSeq);
			break;
		case kDudeTentacleGreen:
			sfxPlay3DSound(actor, damage == 5 ? 2471 : 2472, -1, 0);
			seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
			break;
		case kDudePodFire:
			sfxPlay3DSound(actor, damage == 5 ? 2451 : 2452, -1, 0);
			seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
			break;
		case kDudeTentacleFire:
			sfxPlay3DSound(actor, 2501, -1, 0);
			seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
			break;
		}
		break;

	case kDudePodMother:
	case kDudeTentacleMother:
		genericDeath(actor, nSeq, 2203, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeCerberusTwoHead:
	case kDudeCerberusOneHead:
		genericDeath(actor, nSeq, 2303, dudeInfo[nType].seqStartID + nSeq);
		break;

	case kDudeTchernobog:
		sfxPlay3DSound(actor, 2380, -1, 0);
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, -1);
		break;

	case kDudeBurningTinyCaleb:
		damageType = kDamageExplode;
		seqSpawn(dudeInfo[nType].seqStartID + 11, actor, nDudeToGibClient1);
		break;

	case kDudeBeast:
		sfxPlay3DSound(actor, 9000 + Random(2), -1, 0);
		seqSpawn(dudeInfo[nType].seqStartID + nSeq, actor, nSeq == 3 ? nDudeToGibClient2 : nDudeToGibClient1);
		break;

	case kDudeBurningBeast:
		damageType = kDamageExplode;
		seqSpawn(dudeInfo[nType].seqStartID + 12, actor, nDudeToGibClient1);
		break;

	default:
		seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID + nSeq, actor, -1);
		break;
	}

	if (damageType == kDamageExplode)
	{
		DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
		for (int i = 0; i < 3; i++)
			if (pDudeInfo->nGibType[i] > -1)
				GibSprite(actor, (GIBTYPE)pDudeInfo->nGibType[i], nullptr, nullptr);
		for (int i = 0; i < 4; i++)
			fxSpawnBlood(actor, damage);
	}
	gKillMgr.AddKill(actor);
	actCheckRespawn(actor);
	actor->spr.type = kThingBloodChunks;
	actPostSprite(actor, kStatThing);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int actDamageDude(DBloodActor* source, DBloodActor* actor, int damage, DAMAGE_TYPE damageType)
{
	if (!actor->IsDudeActor())
	{
		Printf(PRINT_HIGH, "Bad Dude Failed: initial=%d type=%d %s\n", (int)actor->spr.inittype, (int)actor->spr.type, (int)(actor->spr.flags & kHitagRespawn) ? "RESPAWN" : "NORMAL");
		return damage >> 4;
		//I_Error("Bad Dude Failed: initial=%d type=%d %s\n", (int)actor->spr.inittype, (int)actor->spr.type, (int)(actor->spr.flags & 16) ? "RESPAWN" : "NORMAL");
	}

	int nType = actor->spr.type - kDudeBase;
	int nDamageFactor = getDudeInfo(nType + kDudeBase)->damageVal[damageType];
#ifdef NOONE_EXTENSIONS
	if (actor->spr.type == kDudeModernCustom)
		nDamageFactor = actor->genDudeExtra.dmgControl[damageType];
#endif

	if (!nDamageFactor) return 0;
	else if (nDamageFactor != 256) damage = MulScale(damage, nDamageFactor, 8);

	if (!actor->IsPlayerActor())
	{
		if (actor->xspr.health <= 0) return 0;
		damage = aiDamageSprite(source, actor, damageType, damage);
		if (actor->xspr.health <= 0)
			actKillDude(source, actor, ((damageType == kDamageExplode && damage < 160) ? kDamageFall : damageType), damage);
	}
	else
	{
		PLAYER* pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];
		if (actor->xspr.health > 0 || playerSeqPlaying(pPlayer, 16))
			damage = playerDamageSprite(source, pPlayer, damageType, damage);

	}
	return damage;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int actDamageThing(DBloodActor* source, DBloodActor* actor, int damage, DAMAGE_TYPE damageType, PLAYER* pSourcePlayer)
{
	assert(actor->spr.type >= kThingBase && actor->spr.type < kThingMax);
	int nType = actor->spr.type - kThingBase;
	int nDamageFactor = thingInfo[nType].dmgControl[damageType];

	if (!nDamageFactor) return 0;
	else if (nDamageFactor != 256) damage = MulScale(damage, nDamageFactor, 8);

	actor->xspr.health = ClipLow(actor->xspr.health - damage, 0);
	if (actor->xspr.health <= 0)
	{
		auto Owner = actor->GetOwner();
		switch (actor->spr.type)
		{
		case kThingDroppedLifeLeech:
#ifdef NOONE_EXTENSIONS
		case kModernThingEnemyLifeLeech:
#endif
			GibSprite(actor, GIBTYPE_14, nullptr, nullptr);
			actor->xspr.data1 = actor->xspr.data2 = actor->xspr.data3 = actor->xspr.DudeLockout = 0;
			actor->xspr.stateTimer = actor->xspr.data4 = actor->xspr.isTriggered = 0;

#ifdef NOONE_EXTENSIONS
			if (Owner && Owner->spr.type == kDudeModernCustom)
				Owner->SetSpecialOwner(); // indicates if custom dude had life leech.
#endif
			break;

		default:
			if (!(actor->spr.flags & kHitagRespawn))
				actor->SetOwner(source);
			break;
		}

		trTriggerSprite(actor, kCmdOff, source);

		switch (actor->spr.type)
		{
		case kThingObjectGib:
		case kThingObjectExplode:
		case kThingBloodBits:
		case kThingBloodChunks:
		case kThingZombieHead:
			if (damageType == 3 && pSourcePlayer && PlayClock > pSourcePlayer->laughCount && Chance(0x4000))
			{
				sfxPlay3DSound(pSourcePlayer->actor, gPlayerGibThingComments[Random(10)], 0, 2);
				pSourcePlayer->laughCount = PlayClock + 3600;
			}
			break;
		case kTrapMachinegun:
			seqSpawn(28, actor, -1);
			break;

		case kThingFluorescent:
			seqSpawn(12, actor, -1);
			GibSprite(actor, GIBTYPE_6, nullptr, nullptr);
			break;

		case kThingSpiderWeb:
			seqSpawn(15, actor, -1);
			break;

		case kThingMetalGrate:
			seqSpawn(21, actor, -1);
			GibSprite(actor, GIBTYPE_4, nullptr, nullptr);
			break;

		case kThingFlammableTree:
			switch (actor->xspr.data1)
			{
			case -1:
				GibSprite(actor, GIBTYPE_14, nullptr, nullptr);
				sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 312, actor->sector());
				actPostSprite(actor, kStatFree);
				break;

			case 0:
				seqSpawn(25, actor, nTreeToGibClient);
				sfxPlay3DSound(actor, 351, -1, 0);
				break;

			case 1:
				seqSpawn(26, actor, nTreeToGibClient);
				sfxPlay3DSound(actor, 351, -1, 0);
				break;
			}
			break;
		}
	}
	return damage;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actDamageSprite(DBloodActor* source, DBloodActor* actor, DAMAGE_TYPE damageType, int damage)
{
	if (actor->spr.flags & 32 || !actor->hasX())
		return 0;

	if ((actor->xspr.health == 0 && actor->spr.statnum != kStatDude) || actor->xspr.locked)
		return 0;

	if (source == nullptr) source = actor;

	PLAYER* pSourcePlayer = nullptr;
	if (source->IsPlayerActor()) pSourcePlayer = &gPlayer[source->spr.type - kDudePlayer1];
	if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pSourcePlayer, actor)) return 0;

	switch (actor->spr.statnum)
	{
	case kStatDude:
		damage = actDamageDude(source, actor, damage, damageType);
		break;
	case kStatThing:
		damage = actDamageThing(source, actor, damage, damageType, pSourcePlayer);
		break;
	}

	return damage >> 4;
}

//---------------------------------------------------------------------------
//
// this was condensed to the parts actually in use.
//
//---------------------------------------------------------------------------

void actHitcodeToData(int a1, HitInfo* pHitInfo, DBloodActor** pActor, walltype** ppWall)
{
	assert(pHitInfo != nullptr);
	DBloodActor* actor = nullptr;
	walltype* pWall = nullptr;
	switch (a1)
	{
	case 3:
	case 5:
		actor = pHitInfo->actor();
		break;
	case 0:
	case 4:
		pWall = pHitInfo->hitWall;
		break;
	default:
		break;
	}
	if (pActor) *pActor = actor;
	if (ppWall) *ppWall = pWall;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actImpactMissile(DBloodActor* missileActor, int hitCode)
{
	auto missileOwner = missileActor->GetOwner();

	DBloodActor* actorHit = nullptr;
	walltype* pWallHit = nullptr;

	actHitcodeToData(hitCode, &gHitInfo, &actorHit, &pWallHit);

	const THINGINFO* pThingInfo = nullptr;
	DUDEINFO* pDudeInfo = nullptr;

	if (hitCode == 3 && actorHit)
	{
		switch (actorHit->spr.statnum)
		{
		case kStatThing:
			pThingInfo = &thingInfo[actorHit->spr.type - kThingBase];
			break;
		case kStatDude:
			pDudeInfo = getDudeInfo(actorHit->spr.type);
			break;
		}
	}
	switch (missileActor->spr.type)
	{
	case kMissileLifeLeechRegular:
		if (hitCode == 3 && actorHit && (pThingInfo || pDudeInfo))
		{
			DAMAGE_TYPE rand1 = (DAMAGE_TYPE)Random(7);
			int rand2 = (7 + Random(7)) << 4;
			int nDamage = actDamageSprite(missileOwner, actorHit, rand1, rand2);

			if ((pThingInfo && pThingInfo->dmgControl[kDamageBurn] != 0) || (pDudeInfo && pDudeInfo->damageVal[kDamageBurn] != 0))
				actBurnSprite(missileActor->GetOwner(), actorHit, 360);

			// by NoOne: make Life Leech heal user, just like it was in 1.0x versions
			if (gGameOptions.weaponsV10x && !VanillaMode() && pDudeInfo != nullptr)
			{
				if (missileOwner->IsDudeActor() && missileOwner->hasX() && missileOwner->xspr.health != 0)
					actHealDude(missileOwner, nDamage >> 2, getDudeInfo(missileOwner->spr.type)->startHealth);
			}
		}

		if (missileActor->hasX())
		{
			actPostSprite(missileActor, kStatDecoration);
			if (missileActor->spr.ang == 1024) sfxPlay3DSound(missileActor, 307, -1, 0);
			missileActor->spr.type = kSpriteDecoration;
			seqSpawn(9, missileActor, -1);
		}
		else
		{
			actPostSprite(missileActor, kStatFree);
		}

		break;
	case kMissileTeslaAlt:
		teslaHit(missileActor, hitCode);
		switch (hitCode)
		{
		case 0:
		case 4:
			if (pWallHit)
			{
				auto pFX = gFX.fxSpawnActor(FX_52, missileActor->sector(), missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, 0);
				if (pFX) pFX->spr.ang = (GetWallAngle(pWallHit) + 512) & 2047;
			}
			break;
		}
		GibSprite(missileActor, GIBTYPE_24, NULL, NULL);
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissilePukeGreen:
		seqKill(missileActor);
		if (hitCode == 3 && actorHit && (pThingInfo || pDudeInfo))
		{
			int nDamage = (15 + Random(7)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileArcGargoyle:
		sfxKill3DSound(missileActor, -1, -1);
		sfxPlay3DSound(missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, 306, missileActor->sector());
		GibSprite(missileActor, GIBTYPE_6, NULL, NULL);

		if (hitCode == 3 && actorHit && (pThingInfo || pDudeInfo))
		{
			int nDamage = (25 + Random(20)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileLifeLeechAltNormal:
	case kMissileLifeLeechAltSmall:
		sfxKill3DSound(missileActor, -1, -1);
		sfxPlay3DSound(missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, 306, missileActor->sector());

		if (hitCode == 3 && actorHit && (pThingInfo || pDudeInfo))
		{
			int nDmgMul = (missileActor->spr.type == kMissileLifeLeechAltSmall) ? 6 : 3;
			int nDamage = (nDmgMul + Random(nDmgMul)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileFireball:
	case kMissileFireballNapalm:
		if (hitCode == 3 && actorHit && (pThingInfo || pDudeInfo))
		{
			if (pThingInfo && actorHit->spr.type == kThingTNTBarrel && actorHit->xspr.burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			int nDamage = (50 + Random(50)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actExplodeSprite(missileActor);
		break;

	case kMissileFlareAlt:
		sfxKill3DSound(missileActor, -1, -1);
		actExplodeSprite(missileActor);
		break;

	case kMissileFlareRegular:
		sfxKill3DSound(missileActor, -1, -1);
		if ((hitCode == 3 && actorHit) && (pThingInfo || pDudeInfo))
		{
			if ((pThingInfo && pThingInfo->dmgControl[kDamageBurn] != 0) || (pDudeInfo && pDudeInfo->damageVal[kDamageBurn] != 0))
			{
				if (pThingInfo && actorHit->spr.type == kThingTNTBarrel && actorHit->xspr.burnTime == 0)
					evPostActor(actorHit, 0, kCallbackFXFlameLick);

				actBurnSprite(missileOwner, actorHit, 480);
				actRadiusDamage(missileOwner, missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, missileActor->sector(), 16, 20, 10, kDamageBullet, 6, 480);

				// by NoOne: allow additional bullet damage for Flare Gun
				if (gGameOptions.weaponsV10x && !VanillaMode())
				{
					int nDamage = (20 + Random(10)) << 4;
					actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
				}
			}
			else
			{
				int nDamage = (20 + Random(10)) << 4;
				actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
			}

			if (surfType[actorHit->spr.picnum] == kSurfFlesh)
			{
				missileActor->spr.picnum = 2123;
				missileActor->SetTarget(actorHit);
				missileActor->xspr.TargetPos.Z = missileActor->int_pos().Z - actorHit->int_pos().Z;
				missileActor->xspr.goalAng = getangle(missileActor->int_pos().X - actorHit->int_pos().X, missileActor->int_pos().Y - actorHit->int_pos().Y) - actorHit->spr.ang;
				missileActor->xspr.state = 1;
				actPostSprite(missileActor, kStatFlare);
				missileActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				break;
			}
		}
		GibSprite(missileActor, GIBTYPE_17, NULL, NULL);
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileFlameSpray:
	case kMissileFlameHound:
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if ((actorHit->spr.statnum == kStatThing || actorHit->spr.statnum == kStatDude) && actorHit->xspr.burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			actBurnSprite(missileOwner, actorHit, (4 + gGameOptions.nDifficulty) << 2);
			actDamageSprite(missileOwner, actorHit, kDamageBurn, 8);
		}
		break;

	case kMissileFireballCerberus:
		actExplodeSprite(missileActor);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if ((actorHit->spr.statnum == kStatThing || actorHit->spr.statnum == kStatDude) && actorHit->xspr.burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			actBurnSprite(missileOwner, actorHit, (4 + gGameOptions.nDifficulty) << 2);
			actDamageSprite(missileOwner, actorHit, kDamageBurn, 8);
			int nDamage = (25 + Random(10)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actExplodeSprite(missileActor);
		break;

	case kMissileFireballTchernobog:
		actExplodeSprite(missileActor);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if ((actorHit->spr.statnum == kStatThing || actorHit->spr.statnum == kStatDude) && actorHit->xspr.burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			actBurnSprite(missileOwner, actorHit, 32);
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, 12);
			int nDamage = (25 + Random(10)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actExplodeSprite(missileActor);
		break;

	case kMissileEctoSkull:
		sfxKill3DSound(missileActor, -1, -1);
		sfxPlay3DSound(missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, 522, missileActor->sector());
		actPostSprite(missileActor, kStatDebris);
		seqSpawn(20, missileActor, -1);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if (actorHit->spr.statnum == kStatDude)
			{
				int nDamage = (25 + Random(10)) << 4;
				actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
			}
		}
		break;

	case kMissileButcherKnife:
		actPostSprite(missileActor, kStatDebris);
		missileActor->spr.cstat &= ~CSTAT_SPRITE_ALIGNMENT_WALL;
		missileActor->spr.type = kSpriteDecoration;
		seqSpawn(20, missileActor, -1);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if (actorHit->spr.statnum == kStatDude)
			{
				int nDamage = (10 + Random(10)) << 4;
				actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
				int nType = missileOwner->spr.type - kDudeBase;
				if (missileOwner->xspr.health > 0)
					actHealDude(missileOwner, 10, getDudeInfo(nType + kDudeBase)->startHealth);
			}
		}
		break;

	case kMissileTeslaRegular:
		sfxKill3DSound(missileActor, -1, -1);
		sfxPlay3DSound(missileActor->int_pos().X, missileActor->int_pos().Y, missileActor->int_pos().Z, 518, missileActor->sector());
		GibSprite(missileActor, (hitCode == 2) ? GIBTYPE_23 : GIBTYPE_22, NULL, NULL);
		evKillActor(missileActor);
		seqKill(missileActor);
		actPostSprite(missileActor, kStatFree);
		if (hitCode == 3 && actorHit)
		{
			int nDamage = (15 + Random(10)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageTesla, nDamage);
		}
		break;

	default:
		seqKill(missileActor);
		actPostSprite(missileActor, kStatFree);
		if (hitCode == 3 && actorHit)
		{
			int nDamage = (10 + Random(10)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageFall, nDamage);
		}
		break;
	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap && actorHit && actorHit->hasX() && actorHit->xspr.state != actorHit->xspr.restState && actorHit->xspr.Impact)
		trTriggerSprite(actorHit, kCmdSpriteImpact, missileActor);
#endif
	missileActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actKickObject(DBloodActor* kicker, DBloodActor* kicked)
{
	int nSpeed = ClipLow(approxDist(kicker->vel.X, kicker->vel.Y) * 2, 0xaaaaa);
	kicked->vel.X = MulScale(nSpeed, Cos(kicker->spr.ang + Random2(85)), 30);
	kicked->vel.Y = MulScale(nSpeed, Sin(kicker->spr.ang + Random2(85)), 30);
	kicked->vel.Z = MulScale(nSpeed, -0x2000, 14);
	kicked->spr.flags = 7;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actTouchFloor(DBloodActor* actor, sectortype* pSector)
{
	XSECTOR* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	bool doDamage = (pXSector && (pSector->type == kSectorDamage || pXSector->damageType > 0));
	// don't allow damage for damage sectors if they are not enabled
#ifdef NOONE_EXTENSIONS
	if (gModernMap && doDamage && pSector->type == kSectorDamage && !pXSector->state)
		doDamage = false;
#endif

	if (doDamage) {
		DAMAGE_TYPE nDamageType;
		if (pSector->type == kSectorDamage) nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType, kDamageFall, kDamageTesla);
		else nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType - 1, kDamageFall, kDamageTesla);

		int nDamage;
		if (pXSector->data) nDamage = ClipRange(pXSector->data, 0, 1000);
		else nDamage = 1000;

		actDamageSprite(actor, actor, nDamageType, Scale(4, nDamage, 120) << 4);
	}
	if (tileGetSurfType(pSector->floorpicnum) == kSurfLava)
	{
		actDamageSprite(actor, actor, kDamageBurn, 16);
		sfxPlay3DSound(actor, 352, 5, 2);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void checkCeilHit(DBloodActor* actor)
{
	auto& coll = actor->hit.ceilhit;
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSprite:
		auto actor2 = coll.actor();
		if (actor2 && actor2->hasX())
		{
			if ((actor2->spr.statnum == kStatThing || actor2->spr.statnum == kStatDude) && (actor->vel.X != 0 || actor->vel.Y != 0 || actor->vel.Z != 0))
			{
				if (actor2->spr.statnum == kStatThing)
				{
					int nType = actor2->spr.type - kThingBase;
					const THINGINFO* pThingInfo = &thingInfo[nType];
					if (pThingInfo->flags & 1) actor2->spr.flags |= 1;
					if (pThingInfo->flags & 2) actor2->spr.flags |= 4;
					// Inlined ?
					actor2->vel.X += MulScale(4, actor2->int_pos().X - actor->int_pos().X, 2);
					actor2->vel.Y += MulScale(4, actor2->int_pos().Y - actor->int_pos().Y, 2);
				}
				else
				{
					actor2->spr.flags |= 5;
					actor2->vel.X += MulScale(4, actor2->int_pos().X - actor->int_pos().X, 2);
					actor2->vel.Y += MulScale(4, actor2->int_pos().Y - actor->int_pos().Y, 2);

#ifdef NOONE_EXTENSIONS
					// add size shroom abilities
					if ((actor->IsPlayerActor() && isShrinked(actor)) || (actor2->IsPlayerActor() && isGrown(actor2))) {

						int mass1 = getDudeInfo(actor2->spr.type)->mass;
						int mass2 = getDudeInfo(actor->spr.type)->mass;
						switch (actor->spr.type)
						{
						case kDudeModernCustom:
						case kDudeModernCustomBurning:
							mass2 = getSpriteMassBySize(actor);
							break;
						}
						if (mass1 > mass2)
						{
							int dmg = abs((mass1 - mass2) * (actor2->spr.clipdist - actor->spr.clipdist));
							if (actor2->IsDudeActor())
							{
								if (dmg > 0) actDamageSprite(actor2, actor, (Chance(0x2000)) ? kDamageFall : (Chance(0x4000)) ? kDamageExplode : kDamageBullet, dmg);
								if (Chance(0x0200)) actKickObject(actor2, actor);
							}
						}
					}
#endif
					if (!actor->IsPlayerActor() || gPlayer[actor->spr.type - kDudePlayer1].godMode == 0)
					{
						switch (actor2->spr.type)
						{
						case kDudeTchernobog:
							actDamageSprite(actor2, actor, kDamageExplode, actor->xspr.health << 2);
							break;
#ifdef NOONE_EXTENSIONS
						case kDudeModernCustom:
						case kDudeModernCustomBurning:
							int dmg = 0;
							if (!actor->IsDudeActor() || (dmg = ClipLow((getSpriteMassBySize(actor2) - getSpriteMassBySize(actor)) >> 1, 0)) == 0)
								break;

							if (!actor->IsPlayerActor())
							{
								actDamageSprite(actor2, actor, kDamageFall, dmg);
								if (actor->hasX() && !actor->isActive()) aiActivateDude(actor);
							}
							else if (powerupCheck(&gPlayer[actor->spr.type - kDudePlayer1], kPwUpJumpBoots) > 0) actDamageSprite(actor2, actor, kDamageExplode, dmg);
							else actDamageSprite(actor2, actor, kDamageFall, dmg);
							break;
#endif

						}

					}
				}
			}

			if (actor2->spr.type == kTrapSawCircular && actor2->hasX())
			{
				if (!actor2->xspr.state) actDamageSprite(actor, actor, kDamageBullet, 1);
				else {
					actor2->xspr.data1 = 1;
					actor2->xspr.data2 = ClipHigh(actor2->xspr.data2 + 8, 600);
					actDamageSprite(actor, actor, kDamageBullet, 16);
				}
			}
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void checkHit(DBloodActor* actor)
{
	const auto& coll = actor->hit.hit;
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSprite:
		if (coll.actor()->hasX())
		{
			auto actor2 = coll.actor();

#ifdef NOONE_EXTENSIONS
			// add size shroom abilities
			if ((actor2->IsPlayerActor() && isShrinked(actor2)) || (actor->IsPlayerActor() && isGrown(actor)))
			{
				if (actor->vel.X != 0 && actor2->IsDudeActor())
				{
					int mass1 = getDudeInfo(actor->spr.type)->mass;
					int mass2 = getDudeInfo(actor2->spr.type)->mass;
					switch (actor2->spr.type)
					{
					case kDudeModernCustom:
					case kDudeModernCustomBurning:
						mass2 = getSpriteMassBySize(actor2);
						break;
					}
					if (mass1 > mass2)
					{
						actKickObject(actor, actor2);
						sfxPlay3DSound(actor, 357, -1, 1);
						int dmg = (mass1 - mass2) + abs(FixedToInt(actor->vel.X));
						if (dmg > 0) actDamageSprite(actor, actor2, (Chance(0x2000)) ? kDamageFall : kDamageBullet, dmg);
					}
				}
			}
#endif

			switch (actor2->spr.type)
			{
			case kThingKickablePail:
				actKickObject(actor, actor2);
				break;

			case kThingZombieHead:
				sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 357, actor->sector());
				actKickObject(actor, actor2);
				actDamageSprite(nullptr, actor2, kDamageFall, 80);
				break;

			case kDudeBurningInnocent:
			case kDudeBurningCultist:
			case kDudeBurningZombieAxe:
			case kDudeBurningZombieButcher:
				// This does not make sense
				actor->xspr.burnTime = ClipLow(actor->xspr.burnTime - 4, 0);
				actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
				break;
			}
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void checkFloorHit(DBloodActor* actor)
{
	const auto& coll = actor->hit.florhit;
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSector:
		actTouchFloor(actor, coll.hitSector);
		break;
	case kHitSprite:
		if (coll.actor()->hasX())
		{
			auto actor2 = coll.actor();

#ifdef NOONE_EXTENSIONS
			// add size shroom abilities
			if ((actor2->IsPlayerActor() && isShrinked(actor2)) || (actor->IsPlayerActor() && isGrown(actor)))
			{

				int mass1 = getDudeInfo(actor->spr.type)->mass;
				int mass2 = getDudeInfo(actor2->spr.type)->mass;
				switch (actor2->spr.type)
				{
				case kDudeModernCustom:
				case kDudeModernCustomBurning:
					mass2 = getSpriteMassBySize(actor2);
					break;
				}

				if (mass1 > mass2 && actor2->IsDudeActor())
				{
					if ((actor2->IsPlayerActor() && Chance(0x500)) || !actor2->IsPlayerActor())
						actKickObject(actor, actor2);

					int dmg = (mass1 - mass2) + actor->spr.clipdist;
					if (dmg > 0) actDamageSprite(actor, actor2, (Chance(0x2000)) ? kDamageFall : kDamageBullet, dmg);
				}
			}
#endif

			PLAYER* pPlayer = nullptr;
			if (actor->IsPlayerActor()) pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];

			switch (actor2->spr.type)
			{
			case kThingKickablePail:
				if (pPlayer)
				{
					if (pPlayer->kickPower > PlayClock) return;
					pPlayer->kickPower = PlayClock + 60;
				}
				actKickObject(actor, actor2);
				sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 357, actor->sector());
				sfxPlay3DSound(actor, 374, 0, 0);
				break;
			case kThingZombieHead:
				if (pPlayer)
				{
					if (pPlayer->kickPower > PlayClock) return;
					pPlayer->kickPower = PlayClock + 60;
				}
				actKickObject(actor, actor2);
				sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 357, actor->sector());
				actDamageSprite(nullptr, actor2, kDamageFall, 80);
				break;
			case kTrapSawCircular:
				if (!actor2->xspr.state) actDamageSprite(actor, actor, kDamageBullet, 1);
				else
				{
					actor2->xspr.data1 = 1;
					actor2->xspr.data2 = ClipHigh(actor2->xspr.data2 + 8, 600);
					actDamageSprite(actor, actor, kDamageBullet, 16);
				}
				break;
			case kDudeCultistTommy:
			case kDudeCultistShotgun:
			case kDudeZombieAxeNormal:
			case kDudeZombieButcher:
			case kDudeZombieAxeBuried:
			case kDudeGargoyleFlesh:
			case kDudeGargoyleStone:
			case kDudePhantasm:
			case kDudeHellHound:
			case kDudeHand:
			case kDudeSpiderBrown:
			case kDudeSpiderRed:
			case kDudeSpiderBlack:
			case kDudeGillBeast:
			case kDudeBat:
			case kDudeRat:
			case kDudePodGreen:
			case kDudeTentacleGreen:
			case kDudePodFire:
			case kDudeTentacleFire:
			case kDudePodMother:
			case kDudeTentacleMother:
			case kDudeCerberusTwoHead:
			case kDudeCerberusOneHead:
			case kDudeTchernobog:
			case kDudePlayer1:
			case kDudePlayer2:
			case kDudePlayer3:
			case kDudePlayer4:
			case kDudePlayer5:
			case kDudePlayer6:
			case kDudePlayer7:
			case kDudePlayer8:
#ifdef NOONE_EXTENSIONS
				if (pPlayer && !isShrinked(actor))
#else
				if (pPlayer)
#endif
					actDamageSprite(actor, actor2, kDamageBullet, 8);
				break;
			}
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void ProcessTouchObjects(DBloodActor* actor)
{
	checkCeilHit(actor);
	checkHit(actor);
	checkFloorHit(actor);

#ifdef NOONE_EXTENSIONS
	// add more trigger statements for Touch flag
	if (gModernMap && actor->IsDudeActor())
	{
		DBloodActor* actor2 = nullptr;
		for (auto* coll : { &actor->hit.hit, &actor->hit.florhit, &actor->hit.ceilhit })
		{
			if (coll->type == kHitSprite)
			{
				actor2 = coll->actor();
				break;
			}
		}

		if (actor2 && actor2->hasX())
		{
			triggerTouchSprite(actor, actor2);
		}

		// Touch walls
		const auto& coll = actor->hit.hit;
		walltype* pHWall = nullptr;
		if (coll.type == kHitWall)
		{
			pHWall = coll.hitWall;
			if (pHWall && pHWall->hasX())
			{
				triggerTouchWall(actor, pHWall);
			}
		}
	}
#endif
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actAirDrag(DBloodActor* actor, int a2)
{
	int wind_x = 0;
	int wind_y = 0;
	assert(actor->sector());
	sectortype* pSector = actor->sector();
	if (pSector->hasX())
	{
		XSECTOR* pXSector = &pSector->xs();
		if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
		{
			int wind = pXSector->windVel << 12;
			if (!pXSector->windAlways && pXSector->busy) wind = MulScale(wind, pXSector->busy, 16);
			wind_x = MulScale(wind, Cos(pXSector->windAng), 30);
			wind_y = MulScale(wind, Sin(pXSector->windAng), 30);
		}
	}
	actor->vel.X += MulScale(wind_x - actor->vel.X, a2, 16);
	actor->vel.Y += MulScale(wind_y - actor->vel.Y, a2, 16);
	actor->vel.Z -= MulScale(actor->vel.Z, a2, 16);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static Collision MoveThing(DBloodActor* actor)
{
	assert(actor->hasX());
	assert(actor->spr.type >= kThingBase && actor->spr.type < kThingMax);
	const THINGINFO* pThingInfo = &thingInfo[actor->spr.type - kThingBase];
	auto pSector = actor->sector();
	assert(pSector);
	int top, bottom;
	Collision lhit;

	lhit.setNone();
	GetActorExtents(actor, &top, &bottom);
	const int bakCompat = enginecompatibility_mode;
	if (actor->vel.X || actor->vel.Y)
	{
		auto bakCstat = actor->spr.cstat;
		actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		if ((actor->GetOwner()) && !cl_bloodvanillaexplosions && !VanillaMode())
			enginecompatibility_mode = ENGINECOMPATIBILITY_NONE; // improved clipmove accuracy
		auto pos = actor->int_pos();
		ClipMove(pos, &pSector, actor->vel.X >> 12, actor->vel.Y >> 12, actor->spr.clipdist << 2, (actor->int_pos().Z - top) / 4, (bottom - actor->int_pos().Z) / 4, CLIPMASK0, lhit);
		actor->set_int_pos(pos);
		actor->hit.hit = lhit;
		enginecompatibility_mode = bakCompat; // restore
		actor->spr.cstat = bakCstat;
		assert(pSector);
		if (actor->sector() != pSector)
		{
			assert(pSector);
			ChangeActorSect(actor, pSector);
		}

		auto& coll = actor->hit.hit;
		if (coll.type == kHitWall)
		{
			actWallBounceVector(&actor->vel.X, &actor->vel.Y, coll.hitWall, pThingInfo->elastic);
			switch (actor->spr.type)
			{
			case kThingZombieHead:
				sfxPlay3DSound(actor, 607, 0, 0);
				actDamageSprite(nullptr, actor, kDamageFall, 80);
				break;

			case kThingKickablePail:
				sfxPlay3DSound(actor, 374, 0, 0);
				break;
			}
		}
	}
	else
	{
		assert(pSector);
		FindSector(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, &pSector);
	}

	actor->add_int_z(actor->vel.Z >> 8);

	int ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist << 2, CLIPMASK0);
	GetActorExtents(actor, &top, &bottom);

	if ((actor->spr.flags & 2) && bottom < floorZ)
	{
		actor->add_int_z(455);
		actor->vel.Z += 58254;
		if (actor->spr.type == kThingZombieHead)
		{
			auto* fxActor = gFX.fxSpawnActor(FX_27, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
			if (fxActor)
			{
				int v34 = (PlayClock * 3) & 2047;
				int v30 = (PlayClock * 5) & 2047;
				int vbx = (PlayClock * 11) & 2047;
				int v2c = 0x44444;
				int v28 = 0;
				int v24 = 0;
				RotateVector(&v2c, &v28, vbx);
				RotateVector(&v2c, &v24, v30);
				RotateVector(&v28, &v24, v34);
				fxActor->vel.X = actor->vel.X + v2c;
				fxActor->vel.Y = actor->vel.Y + v28;
				fxActor->vel.Z = actor->vel.Z + v24;
			}
		}
	}
	if (CheckLink(actor)) GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist << 2, CLIPMASK0);

	GetActorExtents(actor, &top, &bottom);
	if (bottom >= floorZ)
	{
		actTouchFloor(actor, actor->sector());
		actor->hit.florhit = floorColl;
		actor->add_int_z(floorZ - bottom);

		int v20 = actor->vel.Z - actor->sector()->velFloor;
		if (v20 > 0)
		{

			actor->spr.flags |= 4;
			int vax = actFloorBounceVector(&actor->vel.X, &actor->vel.Y, (int*)&v20, actor->sector(), pThingInfo->elastic);
			int nDamage = MulScale(vax, vax, 30) - pThingInfo->dmgResist;
			if (nDamage > 0) actDamageSprite(actor, actor, kDamageFall, nDamage);

			actor->vel.Z = v20;
			if (actor->sector()->velFloor == 0 && abs(actor->vel.Z) < 0x10000)
			{
				actor->vel.Z = 0;
				actor->spr.flags &= ~4;
			}

			switch (actor->spr.type)
			{
			case kThingNapalmBall:
				if (actor->vel.Z == 0 || Chance(0xA000)) actNapalmMove(actor);
				break;

			case kThingZombieHead:
				if (abs(actor->vel.Z) > 0x80000)
				{
					sfxPlay3DSound(actor, 607, 0, 0);
					actDamageSprite(nullptr, actor, kDamageFall, 80);
				}
				break;

			case kThingKickablePail:
				if (abs(actor->vel.Z) > 0x80000)
					sfxPlay3DSound(actor, 374, 0, 0);
				break;
			}

			lhit.setSector(pSector);
		}
		else if (actor->vel.Z == 0)

			actor->spr.flags &= ~4;
	}
	else
	{
		actor->hit.florhit.setNone();

		if (actor->spr.flags & 2)
			actor->spr.flags |= 4;
	}

	if (top <= ceilZ)
	{
		actor->hit.ceilhit = ceilColl;
		actor->add_int_z(ClipLow(ceilZ - top, 0));
		if (actor->vel.Z < 0)
		{
			actor->vel.X = MulScale(actor->vel.X, 0xc000, 16);
			actor->vel.Y = MulScale(actor->vel.Y, 0xc000, 16);
			actor->vel.Z = MulScale(-actor->vel.Z, 0x4000, 16);

			switch (actor->spr.type)
			{
			case kThingZombieHead:
				if (abs(actor->vel.Z) > 0x80000)
				{
					sfxPlay3DSound(actor, 607, 0, 0);
					actDamageSprite(nullptr, actor, kDamageFall, 80);
				}
				break;

			case kThingKickablePail:
				if (abs(actor->vel.Z) > 0x80000)
					sfxPlay3DSound(actor, 374, 0, 0);
				break;
			}
		}
	}
	else actor->hit.ceilhit.setNone();

	if (bottom >= floorZ)
	{
		int nVel = approxDist(actor->vel.X, actor->vel.Y);
		int nVelClipped = ClipHigh(nVel, 0x11111);
		Collision& coll = floorColl;

		if (coll.type == kHitSprite)
		{
			auto hitActor = coll.actor();
			if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FACING)
			{
				actor->vel.X += MulScale(4, actor->int_pos().X - hitActor->int_pos().X, 2);
				actor->vel.Y += MulScale(4, actor->int_pos().Y - hitActor->int_pos().Y, 2);
				lhit = actor->hit.hit;
			}
		}
		if (nVel > 0)
		{
			int t = DivScale(nVelClipped, nVel, 16);
			actor->vel.X -= MulScale(t, actor->vel.X, 16);
			actor->vel.Y -= MulScale(t, actor->vel.Y, 16);
		}
	}
	if (actor->vel.X || actor->vel.Y)
		actor->spr.ang = getangle(actor->vel.X, actor->vel.Y);
	return lhit;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveDude(DBloodActor* actor)
{
	PLAYER* pPlayer = nullptr;
	if (actor->IsPlayerActor()) pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax))
	{
		Printf(PRINT_HIGH, "%d: actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax", actor->spr.type);
		return;
	}

	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	int bz = (bottom - actor->int_pos().Z) / 4;
	int tz = (actor->int_pos().Z - top) / 4;
	int wd = actor->spr.clipdist << 2;
	auto pSector = actor->sector();
	int nAiStateType = (actor->xspr.aiState) ? actor->xspr.aiState->stateType : -1;

	assert(pSector);

	if (actor->vel.X || actor->vel.Y)
	{
		if (pPlayer && gNoClip)
		{
			actor->add_int_pos({ actor->vel.X >> 12, actor->vel.Y >> 12, 0 });
			if (!FindSector(actor->int_pos().X, actor->int_pos().Y, &pSector))
				pSector = actor->sector();
		}
		else
		{
			auto bakCstat = actor->spr.cstat;
			actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			auto pos = actor->int_pos();
			ClipMove(pos, &pSector, actor->vel.X >> 12, actor->vel.Y >> 12, wd, tz, bz, CLIPMASK0, actor->hit.hit);
			actor->set_int_pos(pos);
			if (pSector == nullptr)
			{
				pSector = actor->sector();
				if (actor->spr.statnum == kStatDude || actor->spr.statnum == kStatThing)
					actDamageSprite(actor, actor, kDamageFall, 1000 << 4);
			}

			if (pSector->type >= kSectorPath && pSector->type <= kSectorRotate)
			{
				auto pSector2 = pSector;
				if (pushmove(actor, &pSector2, wd, tz, bz, CLIPMASK0) == -1)
					actDamageSprite(actor, actor, kDamageFall, 1000 << 4);
				if (pSector2 != nullptr)
					pSector = pSector2;
			}
			assert(pSector);
			actor->spr.cstat = bakCstat;
		}
		auto& coll = actor->hit.hit;
		switch (coll.type)
		{
		case kHitSprite:
		{
			auto hitActor = coll.actor();
			auto Owner = hitActor->GetOwner();

			if (hitActor->spr.statnum == kStatProjectile && !(hitActor->spr.flags & 32) && actor != Owner)
			{
				auto hitInfo = gHitInfo;
				gHitInfo.hitActor = actor;
				actImpactMissile(hitActor, 3);
				gHitInfo = hitInfo;
			}
#ifdef NOONE_EXTENSIONS
			if (!gModernMap && hitActor->hasX() && hitActor->xspr.Touch && !hitActor->xspr.state && !hitActor->xspr.isTriggered)
				trTriggerSprite(coll.actor(), kCmdSpriteTouch, actor);
#else
			if (hitActor->hasX() && hitActor->xspr.Touch && !hitActor->xspr.state && !hitActor->xspr.isTriggered)
				trTriggerSprite(coll.actor, kCmdSpriteTouch);
#endif

			if (pDudeInfo->lockOut && hitActor->hasX() && hitActor->xspr.Push && !hitActor->xspr.key && !hitActor->xspr.DudeLockout && !hitActor->xspr.state && !hitActor->xspr.busy && !pPlayer)
				trTriggerSprite(coll.actor(), kCmdSpritePush, actor);

			break;
		}
		case kHitWall:
		{
			walltype* pHitWall = coll.hitWall;
			XWALL* pHitXWall = nullptr;
			if (pHitWall->hasX()) pHitXWall = &pHitWall->xw();

			if (pDudeInfo->lockOut && pHitXWall && pHitXWall->triggerPush && !pHitXWall->key && !pHitXWall->dudeLockout && !pHitXWall->state && !pHitXWall->busy && !pPlayer)
				trTriggerWall(pHitWall, kCmdWallPush, actor);

			if (pHitWall->twoSided())
			{
				sectortype* pHitSector = pHitWall->nextSector();
				XSECTOR* pHitXSector = pHitSector->hasX() ? &pHitSector->xs() : nullptr;

				if (pDudeInfo->lockOut && pHitXSector && pHitXSector->Wallpush && !pHitXSector->Key && !pHitXSector->dudeLockout && !pHitXSector->state && !pHitXSector->busy && !pPlayer)
					trTriggerSector(pHitSector, kCmdSectorPush, actor);

				if (top < pHitSector->int_ceilingz() || bottom > pHitSector->int_floorz())
				{
					// ???
				}
			}
			actWallBounceVector((int*)&actor->vel.X, (int*)&actor->vel.Y, pHitWall, 0);
			break;
		}
		}
	}
	else
	{
		assert(pSector);
		FindSector(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, &pSector);
	}

	XSECTOR* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (actor->sector() != pSector)
	{
		assert(actor->sector());
		auto pOldSector = actor->sector();
		XSECTOR* pXOldSector = pOldSector->hasX() ? &pOldSector->xs() : nullptr;

		if (pXOldSector && pXOldSector->Exit && (pPlayer || !pXOldSector->dudeLockout))
			trTriggerSector(pOldSector, kCmdSectorExit, actor);
		ChangeActorSect(actor, pSector);

		if (pXSector && pXSector->Enter && (pPlayer || !pXSector->dudeLockout))
		{
			if (pSector->type == kSectorTeleport)
				pXSector->actordata = actor;
			trTriggerSector(pSector, kCmdSectorEnter, actor);
		}

		pSector = actor->sector();
	}
	int bUnderwater = 0;
	int bDepth = 0;
	if (pXSector)
	{
		if (pXSector->Underwater) bUnderwater = 1;
		if (pXSector->Depth) bDepth = 1;
	}
	DCoreActor* pUpperLink = pSector->upperLink;
	DCoreActor* pLowerLink = pSector->lowerLink;
	if (pUpperLink && (pUpperLink->spr.type == kMarkerUpWater || pUpperLink->spr.type == kMarkerUpGoo)) bDepth = 1;
	if (pLowerLink && (pLowerLink->spr.type == kMarkerLowWater || pLowerLink->spr.type == kMarkerLowGoo)) bDepth = 1;
	if (pPlayer) wd += 16;
	if (actor->vel.Z) actor->add_int_z(actor->vel.Z >> 8);

	int ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, wd, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
	GetActorExtents(actor, &top, &bottom);

	if (actor->spr.flags & 2)
	{
		int vc = 58254;
		if (bDepth)
		{
			if (bUnderwater)
			{
				int cz = getceilzofslopeptr(pSector, actor->int_pos().X, actor->int_pos().Y);
				if (cz > top)
					vc += ((bottom - cz) * -80099) / (bottom - top);
				else
					vc = 0;
			}
			else
			{
				int fz = getflorzofslopeptr(pSector, actor->int_pos().X, actor->int_pos().Y);
				if (fz < bottom)
					vc += ((bottom - fz) * -80099) / (bottom - top);
			}
		}
		else
		{
			if (bUnderwater)
				vc = 0;
			else if (bottom >= floorZ)
				vc = 0;
		}
		if (vc)
		{
			actor->add_int_z(((vc * 4) / 2) >> 8);
			actor->vel.Z += vc;
		}
	}
	if (pPlayer && actor->vel.Z > 0x155555 && !pPlayer->fallScream && actor->xspr.height > 0)
	{
		const bool playerAlive = (actor->xspr.health > 0) || VanillaMode(); // only trigger falling scream if player is alive or vanilla mode
		if (playerAlive)
		{
			pPlayer->fallScream = 1;
			sfxPlay3DSound(actor, 719, 0, 0);
		}
	}
	vec3_t const oldpos = actor->int_pos();
	int nLink = CheckLink(actor);
	if (nLink)
	{
		GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, wd, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (pPlayer)
			playerCorrectInertia(pPlayer, &oldpos);
		switch (nLink)
		{
		case kMarkerLowStack:
			if (pPlayer == gView)
				gotpic.Set(actor->sector()->floorpicnum);
			break;
		case kMarkerUpStack:
			if (pPlayer == gView)
				gotpic.Set(actor->sector()->ceilingpicnum);
			break;
		case kMarkerLowWater:
		case kMarkerLowGoo:
			actor->xspr.medium = kMediumNormal;
			if (pPlayer)
			{
				pPlayer->posture = 0;
				pPlayer->bubbleTime = 0;
				if (!pPlayer->cantJump && (pPlayer->input.actions & SB_JUMP))
				{
					actor->vel.Z = -0x6aaaa;
					pPlayer->cantJump = 1;
				}
				sfxPlay3DSound(actor, 721, -1, 0);
			}
			else
			{
				switch (actor->spr.type)
				{
				case kDudeCultistTommy:
				case kDudeCultistShotgun:
					aiNewState(actor, &cultistGoto);
					break;
				case kDudeGillBeast:
					aiNewState(actor, &gillBeastGoto);
					actor->spr.flags |= 6;
					break;
				case kDudeBoneEel:
					actKillDude(actor, actor, kDamageFall, 1000 << 4);
					break;
				}

#ifdef NOONE_EXTENSIONS
				if (actor->IsDudeActor() && actor->xspr.health > 0 && aiInPatrolState(nAiStateType))
					aiPatrolState(actor, kAiStatePatrolMoveL); // continue patrol when going from water
#endif
			}
			break;
		case kMarkerUpWater:
		case kMarkerUpGoo:
		{
			int chance = 0xa00; int medium = kMediumWater;
			if (nLink == kMarkerUpGoo) {
				medium = kMediumGoo;
				chance = 0x400;
			}

			actor->xspr.medium = medium;

			if (pPlayer)
			{
#ifdef NOONE_EXTENSIONS
				// look for palette in data2 of marker. If value <= 0, use default ones.
				if (gModernMap)
				{
					pPlayer->nWaterPal = 0;
					auto pUpper = barrier_cast<DBloodActor*>(pSector->upperLink);
					if (pUpper && pUpper->hasX()) pPlayer->nWaterPal = pUpper->xspr.data2;
				}
#endif

				pPlayer->posture = 1;
				actor->xspr.burnTime = 0;
				pPlayer->bubbleTime = abs(actor->vel.Z) >> 12;
				evPostActor(actor, 0, kCallbackPlayerBubble);
				sfxPlay3DSound(actor, 720, -1, 0);
			}
			else
			{
				switch (actor->spr.type)
				{
				case kDudeCultistTommy:
				case kDudeCultistShotgun:
					actor->xspr.burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &cultistSwimGoto);
					break;
				case kDudeBurningCultist:
				{
					const bool fixRandomCultist = !cl_bloodvanillaenemies && (actor->spr.inittype >= kDudeBase) && (actor->spr.inittype < kDudeMax) && (actor->spr.inittype != actor->spr.type) && !VanillaMode(); // fix burning cultists randomly switching types underwater
					if (Chance(chance))
						actor->spr.type = kDudeCultistTommy;
					else
						actor->spr.type = kDudeCultistShotgun;
					if (fixRandomCultist) // fix burning cultists randomly switching types underwater
						actor->spr.type = actor->spr.inittype; // restore back to spawned cultist type
					actor->xspr.burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &cultistSwimGoto);
					break;
				}
				case kDudeZombieAxeNormal:
					actor->xspr.burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &zombieAGoto);
					break;
				case kDudeZombieButcher:
					actor->xspr.burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &zombieFGoto);
					break;
				case kDudeGillBeast:
					actor->xspr.burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &gillBeastSwimGoto);

					actor->spr.flags &= ~6;
					break;
				case kDudeGargoyleFlesh:
				case kDudeHellHound:
				case kDudeSpiderBrown:
				case kDudeSpiderRed:
				case kDudeSpiderBlack:
				case kDudeBat:
				case kDudeRat:
				case kDudeBurningInnocent:
					actKillDude(actor, actor, kDamageFall, 1000 << 4);
					break;
				}

#ifdef NOONE_EXTENSIONS
				if (gModernMap) {

					if (actor->spr.type == kDudeModernCustom) {

						evPostActor(actor, 0, kCallbackEnemeyBubble);
						if (!canSwim(actor)) actKillDude(actor, actor, kDamageFall, 1000 << 4);
						break;
					}

					// continue patrol when fall into water
					if (actor->IsDudeActor() && actor->xspr.health > 0 && aiInPatrolState(nAiStateType))
						aiPatrolState(actor, kAiStatePatrolMoveW);

				}
#endif

			}
			break;
		}
		}
	}
	GetActorExtents(actor, &top, &bottom);
	if (pPlayer && bottom >= floorZ)
	{
		int floorZ2 = floorZ;
		auto floorColl2 = floorColl;
		GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist << 2, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (bottom <= floorZ && actor->int_pos().Z - floorZ2 < bz)
		{
			floorZ = floorZ2;
			floorColl = floorColl2;
		}
	}
	if (floorZ <= bottom)
	{
		actor->hit.florhit = floorColl;
		actor->add_int_z(floorZ - bottom);
		int v30 = actor->vel.Z - actor->sector()->velFloor;
		if (v30 > 0)
		{
			int vax = actFloorBounceVector((int*)&actor->vel.X, (int*)&actor->vel.Y, (int*)&v30, actor->sector(), 0);
			int nDamage = MulScale(vax, vax, 30);
			if (pPlayer)
			{
				pPlayer->fallScream = 0;

				if (nDamage > (15 << 4) && (actor->spr.flags & 4))
					playerLandingSound(pPlayer);
				if (nDamage > (30 << 4))
					sfxPlay3DSound(actor, 701, 0, 0);
			}
			nDamage -= 100 << 4;
			if (nDamage > 0)
				actDamageSprite(actor, actor, kDamageFall, nDamage);
			actor->vel.Z = v30;
			if (abs(actor->vel.Z) < 0x10000)
			{
				actor->vel.Z = actor->sector()->velFloor;
				actor->spr.flags &= ~4;
			}
			else
				actor->spr.flags |= 4;

			switch (tileGetSurfType(floorColl))
			{
			case kSurfWater:
				gFX.fxSpawnActor(FX_9, actor->sector(), actor->int_pos().X, actor->int_pos().Y, floorZ, 0);
				break;
			case kSurfLava:
			{
				auto pFX = gFX.fxSpawnActor(FX_10, actor->sector(), actor->int_pos().X, actor->int_pos().Y, floorZ, 0);
				if (pFX)
				{
					for (int i = 0; i < 7; i++)
					{
						auto pFX2 = gFX.fxSpawnActor(FX_14, pFX->sector(), pFX->int_pos().X, pFX->int_pos().Y, pFX->int_pos().Z, 0);
						if (pFX2)
						{
							pFX2->vel.X = Random2(0x6aaaa);
							pFX2->vel.Y = Random2(0x6aaaa);
							pFX2->vel.Z = -(int)Random(0xd5555);
						}
					}
				}
				break;
			}
			}
		}
		else if (actor->vel.Z == 0)

			actor->spr.flags &= ~4;
	}
	else
	{
		actor->hit.florhit.setNone();

		if (actor->spr.flags & 2)
			actor->spr.flags |= 4;
	}
	if (top <= ceilZ)
	{
		actor->hit.ceilhit = ceilColl;
		actor->add_int_z(ClipLow(ceilZ - top, 0));

		if (actor->vel.Z <= 0 && (actor->spr.flags & 4))
			actor->vel.Z = MulScale(-actor->vel.Z, 0x2000, 16);
	}
	else
		actor->hit.ceilhit.setNone();

	GetActorExtents(actor, &top, &bottom);

	actor->xspr.height = ClipLow(floorZ - bottom, 0) >> 8;
	if (actor->vel.X || actor->vel.Y)
	{
		if (floorColl.type == kHitSprite)
		{
			auto hitAct = floorColl.actor();
			if ((hitAct->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FACING)
			{
				actor->vel.X += MulScale(4, actor->int_pos().X - hitAct->int_pos().X, 2);
				actor->vel.Y += MulScale(4, actor->int_pos().Y - hitAct->int_pos().Y, 2);
				return;
			}
		}
		if (IsUnderwaterSector(actor->sector()))
			return;
		if (actor->xspr.height >= 0x100)
			return;
		int nDrag = gDudeDrag;
		if (actor->xspr.height > 0)
			nDrag -= Scale(gDudeDrag, actor->xspr.height, 0x100);
		actor->vel.X -= mulscale16r(actor->vel.X, nDrag);
		actor->vel.Y -= mulscale16r(actor->vel.Y, nDrag);

		if (approxDist(actor->vel.X, actor->vel.Y) < 0x1000)
			actor->vel.X = actor->vel.Y = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int MoveMissile(DBloodActor* actor)
{
	auto Owner = actor->GetOwner();
	int cliptype = -1;
	ESpriteFlags bakCstat = 0;
	if (Owner)
	{
		bakCstat = Owner->spr.cstat;
		if (Owner->IsDudeActor())
		{
			Owner->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		}
	}
	gHitInfo.clearObj();
	if (actor->spr.type == kMissileFlameSpray) actAirDrag(actor, 0x1000);

	if (actor->GetTarget() != nullptr && (actor->vel.X || actor->vel.Y || actor->vel.Z))
	{
		auto target = actor->GetTarget();

		if (target->spr.statnum == kStatDude && target->hasX() && target->xspr.health > 0)
		{
			int nTargetAngle = getangle(-(target->int_pos().Y - actor->int_pos().Y), target->int_pos().X - actor->int_pos().X);
			int vx = missileInfo[actor->spr.type - kMissileBase].velocity;
			int vy = 0;
			RotatePoint(&vx, &vy, (nTargetAngle + 1536) & 2047, 0, 0);
			actor->vel.X = vx;
			actor->vel.Y = vy;
			int dz = target->int_pos().Z - actor->int_pos().Z;

			int deltaz = dz / 10;
			if (target->int_pos().Z < actor->int_pos().Z) deltaz = -deltaz;
			actor->vel.Z += deltaz;
		}
	}
	int vx = actor->vel.X >> 12;
	int vy = actor->vel.Y >> 12;
	int vz = actor->vel.Z >> 8;
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	int i = 1;
	const int bakCompat = enginecompatibility_mode;
	const bool isFlameSprite = (actor->spr.type == kMissileFlameSpray || actor->spr.type == kMissileFlameHound); // do not use accurate clipmove for flame based sprites (changes damage too much)
	while (1)
	{
		vec3_t pos = actor->int_pos();
		auto pSector2 = actor->sector();
		const auto bakSpriteCstat = actor->spr.cstat;
		if (Owner && !isFlameSprite && !cl_bloodvanillaexplosions && !VanillaMode())
		{
			enginecompatibility_mode = ENGINECOMPATIBILITY_NONE; // improved clipmove accuracy
			actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL; // remove self collisions for accurate clipmove
		}
		Collision clipmoveresult;
		ClipMove(pos, &pSector2, vx, vy, actor->spr.clipdist << 2, (pos.Z - top) / 4, (bottom - pos.Z) / 4, CLIPMASK0, clipmoveresult, 1);
		enginecompatibility_mode = bakCompat; // restore
		actor->spr.cstat = bakSpriteCstat;
		auto pSector = pSector2;
		if (pSector2 == nullptr)
		{
			cliptype = -1;
			break;
		}
		if (clipmoveresult.type == kHitSprite)
		{
			gHitInfo.hitActor = clipmoveresult.actor();
			cliptype = 3;
		}
		else if (clipmoveresult.type == kHitWall)
		{
			gHitInfo.hitWall = clipmoveresult.hitWall;
			if (!gHitInfo.hitWall->twoSided()) cliptype = 0;
			else
			{
				int32_t fz, cz;
				getzsofslopeptr(clipmoveresult.hitWall->nextSector(), pos.X, pos.Y, &cz, &fz);
				if (pos.Z <= cz || pos.Z >= fz) cliptype = 0;
				else cliptype = 4;
			}
		}
		if (cliptype == 4)
		{
			walltype* pWall = clipmoveresult.hitWall;
			if (pWall->hasX())
			{
				XWALL* pXWall = &pWall->xw();
				if (pXWall->triggerVector)
				{
					trTriggerWall(pWall, kCmdWallImpact, Owner? Owner : actor);
					if (!(pWall->cstat & CSTAT_WALL_BLOCK_HITSCAN))
					{
						cliptype = -1;
						if (i-- > 0)
							continue;
						cliptype = 0;
						break;
					}
				}
			}
		}
		if (cliptype >= 0 && cliptype != 3)
		{
			int nAngle = getangle(actor->vel.X, actor->vel.Y);
			pos.X -= MulScale(Cos(nAngle), 16, 30);
			pos.Y -= MulScale(Sin(nAngle), 16, 30);
			int nVel = approxDist(actor->vel.X, actor->vel.Y);
			vz -= Scale(0x100, actor->vel.Z, nVel);
			updatesector(pos.X, pos.Y, &pSector);
			pSector2 = pSector;
		}
		int ceilZ, floorZ;
		Collision ceilColl, floorColl;
		GetZRangeAtXYZ(pos.X, pos.Y, pos.Z, pSector2, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist << 2, CLIPMASK0);
		GetActorExtents(actor, &top, &bottom);
		top += vz;
		bottom += vz;
		if (bottom >= floorZ)
		{
			actor->hit.florhit = floorColl;
			vz += floorZ - bottom;
			cliptype = 2;
		}
		if (top <= ceilZ)
		{
			actor->hit.ceilhit = ceilColl;
			vz += ClipLow(ceilZ - top, 0);
			cliptype = 1;
		}
		actor->set_int_pos( pos);
		actor->add_int_z(vz);
		updatesector(pos.X, pos.Y, &pSector);
		if (pSector != nullptr && pSector != actor->sector())
		{
			assert(pSector);
			ChangeActorSect(actor, pSector);
		}
		CheckLink(actor);
		gHitInfo.hitSector = actor->sector();
		gHitInfo.hitpos.X = actor->int_pos().X;
		gHitInfo.hitpos.Y = actor->int_pos().Y;
		gHitInfo.hitpos.Z = actor->int_pos().Z;
		break;
	}
	if (Owner) Owner->spr.cstat = bakCstat;

	return cliptype;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actExplodeSprite(DBloodActor* actor)
{
	if (!actor->hasX()) return;

	if (actor->spr.statnum == kStatExplosion) return;
	sfxKill3DSound(actor, -1, -1);
	evKillActor(actor);

	int nType = kExplosionStandard;

	switch (actor->spr.type)
	{
	case kMissileFireballNapalm:
		nType = kExplosionNapalm;
		seqSpawn(4, actor, -1);
		if (Chance(0x8000)) actor->spr.cstat |= CSTAT_SPRITE_XFLIP;
		sfxPlay3DSound(actor, 303, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kMissileFlareAlt:
		nType = kExplosionFireball;
		seqSpawn(9, actor, -1);
		if (Chance(0x8000)) actor->spr.cstat |= CSTAT_SPRITE_XFLIP;
		sfxPlay3DSound(actor, 306, 24 + (actor->GetIndex() & 3), FX_GlobalChannel);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kMissileFireballCerberus:
	case kMissileFireballTchernobog:
		nType = kExplosionFireball;
		seqSpawn(5, actor, -1);
		sfxPlay3DSound(actor, 304, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kThingArmedTNTStick:
		nType = kExplosionSmall;
		if (actor->hit.florhit.type == kHitNone) seqSpawn(4, actor, -1);
		else seqSpawn(3, actor, -1);
		sfxPlay3DSound(actor, 303, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kThingArmedProxBomb:
	case kThingArmedRemoteBomb:
	case kThingArmedTNTBundle:
#ifdef NOONE_EXTENSIONS
	case kModernThingTNTProx:
#endif
		nType = kExplosionStandard;
		if (actor->hit.florhit.type == kHitNone) seqSpawn(4, actor, -1);
		else
			seqSpawn(3, actor, -1);
		sfxPlay3DSound(actor, 304, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kThingArmedSpray:
		nType = kExplosionSpray;
		seqSpawn(5, actor, -1);
		sfxPlay3DSound(actor, 307, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kThingTNTBarrel:
	{
		auto spawned = actSpawnSprite(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 1);
		spawned->SetOwner(actor->GetOwner());
		if (actCheckRespawn(actor))
		{
			actor->xspr.state = 1;
			actor->xspr.health = thingInfo[0].startHealth << 4;
		}
		else actPostSprite(actor, kStatFree);

		nType = kExplosionLarge;
		seqSpawn(4, spawned, -1);
		actor = spawned;

		sfxPlay3DSound(actor, 305, -1, 0);
		GibSprite(actor, GIBTYPE_14, nullptr, nullptr);
		break;
	}
	case kTrapExploder:
	{
		// Defaults for exploder
		nType = 1;
		int nSnd = 304;
		int nSeq = 4;

#ifdef NOONE_EXTENSIONS
		// allow to customize hidden exploder trap
		if (gModernMap)
		{
			nType = actor->xspr.data1;  // Explosion type
			int tSeq = actor->xspr.data2; // SEQ id
			int tSnd = actor->xspr.data3; // Sound Id

			if (nType <= 1 || nType > kExplodeMax) { nType = 1; nSeq = 4; nSnd = 304; }
			else if (nType == 2) { nSeq = 4; nSnd = 305; }
			else if (nType == 3) { nSeq = 9; nSnd = 307; }
			else if (nType == 4) { nSeq = 5; nSnd = 307; }
			else if (nType <= 6) { nSeq = 4; nSnd = 303; }
			else if (nType == 7) { nSeq = 4; nSnd = 303; }
			else if (nType == 8) { nType = 0; nSeq = 3; nSnd = 303; }

			// Override previous sound and seq assigns
			if (tSeq > 0) nSeq = tSeq;
			if (tSnd > 0) nSnd = tSnd;
		}
#endif

		if (getSequence(nSeq)) seqSpawn(nSeq, actor, -1);
		sfxPlay3DSound(actor, nSnd, -1, 0);
		break;
	}
	case kThingPodFireBall:
		nType = kExplosionFireball;
		seqSpawn(9, actor, -1);
		sfxPlay3DSound(actor, 307, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		fxSpawnPodStuff(actor, 240);
		break;

	default:
		nType = kExplosionStandard;
		seqSpawn(4, actor, -1);
		if (Chance(0x8000)) actor->spr.cstat |= CSTAT_SPRITE_XFLIP;
		sfxPlay3DSound(actor, 303, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;
	}
	actor->vel.X = actor->vel.Y = actor->vel.Z = 0;
	actPostSprite(actor, kStatExplosion);
	actor->spr.xrepeat = actor->spr.yrepeat = explodeInfo[nType].repeat;

	actor->spr.flags &= ~3;
	actor->spr.type = nType;
	const EXPLOSION* pExplodeInfo = &explodeInfo[nType];
	actor->SetTarget(nullptr);
	actor->explosionhackflag = true;
	actor->xspr.data1 = pExplodeInfo->ticks;
	actor->xspr.data2 = pExplodeInfo->quakeEffect;
	actor->xspr.data3 = pExplodeInfo->flashEffect;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actActivateGibObject(DBloodActor* actor)
{
	int gib1 = ClipRange(actor->xspr.data1, 0, 31);
	int gib2 = ClipRange(actor->xspr.data2, 0, 31);
	int gib3 = ClipRange(actor->xspr.data3, 0, 31);
	int sound = actor->xspr.data4;
	int dropmsg = actor->xspr.dropMsg;

	if (gib1 > 0) GibSprite(actor, (GIBTYPE)(gib1 - 1), nullptr, nullptr);
	if (gib2 > 0) GibSprite(actor, (GIBTYPE)(gib2 - 1), nullptr, nullptr);
	if (gib3 > 0 && actor->xspr.burnTime > 0) GibSprite(actor, (GIBTYPE)(gib3 - 1), nullptr, nullptr);
	if (sound > 0) sfxPlay3DSound(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, sound, actor->sector());
	if (dropmsg > 0) actDropObject(actor, dropmsg);

	if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE) && !(actor->spr.flags & kHitagRespawn))
		actPostSprite(actor, kStatFree);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actCheckProximity()
{
	BloodStatIterator it(kStatThing);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32) continue;

		if (actor->hasX())
		{
			switch (actor->spr.type)
			{
			case kThingBloodBits:
			case kThingBloodChunks:
			case kThingZombieHead:
				if (actor->xspr.locked && PlayClock >= actor->xspr.TargetPos.X) actor->xspr.locked = 0;
				break;
			}

			if (actor->xspr.burnTime > 0)
			{
				actor->xspr.burnTime = ClipLow(actor->xspr.burnTime - 4, 0);
				actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
			}

			if (actor->xspr.Proximity)
			{
#ifdef NOONE_EXTENSIONS
				// don't process locked or 1-shot things for proximity
				if (gModernMap && (actor->xspr.locked || actor->xspr.isTriggered))
					continue;
#endif

				if (actor->spr.type == kThingDroppedLifeLeech) actor->SetTarget(nullptr);
				BloodStatIterator it1(kStatDude);
				while (auto dudeactor = it1.Next())
				{
					auto nextdude = it1.Peek();

					if (dudeactor->spr.flags & 32 || !dudeactor->hasX()) continue;

					if ((unsigned int)dudeactor->xspr.health > 0)
					{
						int proxyDist = 96;
#ifdef NOONE_EXTENSIONS
						// allow dudeLockout for proximity flag
						if (gModernMap && actor->spr.type != kThingDroppedLifeLeech && actor->xspr.DudeLockout && !dudeactor->IsPlayerActor())
							continue;

						if (actor->spr.type == kModernThingEnemyLifeLeech) proxyDist = 512;
#endif
						if (actor->spr.type == kThingDroppedLifeLeech && actor->GetTarget() == nullptr)
						{
							auto Owner = actor->GetOwner();
							if (!Owner->IsPlayerActor()) continue;

							PLAYER* pPlayer = &gPlayer[Owner->spr.type - kDudePlayer1];
							PLAYER* pPlayer2 = dudeactor->IsPlayerActor() ? &gPlayer[dudeactor->spr.type - kDudePlayer1] : nullptr;

							if (dudeactor == Owner || dudeactor->spr.type == kDudeZombieAxeBuried || dudeactor->spr.type == kDudeRat || dudeactor->spr.type == kDudeBat) continue;
							if (gGameOptions.nGameType == 3 && pPlayer2 && pPlayer->teamId == pPlayer2->teamId) continue;
							if (gGameOptions.nGameType == 1 && pPlayer2) continue;
							proxyDist = 512;
						}

						if (CheckProximity(dudeactor, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), proxyDist))
						{
							switch (actor->spr.type)
							{
							case kThingDroppedLifeLeech:
								if (!Chance(0x4000) && nextdude) continue;
								if (dudeactor->spr.cstat & CSTAT_SPRITE_BLOCK) actor->SetTarget(dudeactor);
								else continue;
								break;

#ifdef NOONE_EXTENSIONS
							case kModernThingTNTProx:
								if (!dudeactor->IsPlayerActor()) continue;
								actor->spr.pal = 0;
								break;

							case kModernThingEnemyLifeLeech:
								if (actor->GetTarget() != dudeactor) continue;
								break;
#endif

							default:
								break;
							}
							if (actor->GetOwner() == nullptr) actor->SetOwner(dudeactor);
							trTriggerSprite(actor, kCmdSpriteProximity, dudeactor);
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actCheckThings()
{
	BloodStatIterator it(kStatThing);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32) continue;
		if (!actor->hasX()) continue;

		auto pSector = actor->sector();

		XSECTOR* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;
		if (pXSector && pXSector->panVel && (pXSector->panAlways || pXSector->state || pXSector->busy))
		{
			int nType = actor->spr.type - kThingBase;
			const THINGINFO* pThingInfo = &thingInfo[nType];
			if (pThingInfo->flags & 1) actor->spr.flags |= 1;
			if (pThingInfo->flags & 2) actor->spr.flags |= 4;
		}

		if (actor->spr.flags & 3)
		{
			viewBackupSpriteLoc(actor);
			if (pXSector && pXSector->panVel)
			{
				int top, bottom;
				GetActorExtents(actor, &top, &bottom);
				if (getflorzofslopeptr(pSector, actor->int_pos().X, actor->int_pos().Y) <= bottom)
				{
					int angle = pXSector->panAngle;
					int speed = 0;
					if (pXSector->panAlways || pXSector->state || pXSector->busy)
					{
						speed = pXSector->panVel << 9;
						if (!pXSector->panAlways && pXSector->busy) speed = MulScale(speed, pXSector->busy, 16);
					}
					if (pSector->floorstat & CSTAT_SECTOR_ALIGN) angle = (angle + GetWallAngle(pSector->firstWall()) + 512) & 2047;

					actor->vel.X += MulScale(speed, Cos(angle), 30);
					actor->vel.Y += MulScale(speed, Sin(angle), 30);
				}
			}
			actAirDrag(actor, 128);

			if (((actor->GetIndex() >> 8) & 15) == (gFrameCount & 15) && (actor->spr.flags & 2))	actor->spr.flags |= 4;
			if ((actor->spr.flags & 4) || actor->vel.X || actor->vel.Y || actor->vel.Z || actor->sector()->velFloor || actor->sector()->velCeil)
			{
				Collision hit = MoveThing(actor);
				if (hit.type)
				{
					if (actor->xspr.Impact) trTriggerSprite(actor, kCmdOff, hit.type == kHitSprite? hit.safeActor() : nullptr);

					switch (actor->spr.type)
					{
					case kThingDripWater:
					case kThingDripBlood:
						MakeSplash(actor);
						break;
#ifdef NOONE_EXTENSIONS
					case kModernThingThrowableRock:
						seqSpawn(24, actor, -1);
						if (hit.type == kHitSprite)
						{
							actor->spr.xrepeat = 32;
							actor->spr.yrepeat = 32;
							actDamageSprite(actor->GetOwner(), hit.actor(), kDamageFall, actor->xspr.data1);
						}
						break;
#endif
					case kThingBone:
						seqSpawn(24, actor, -1);
						if (hit.type == kHitSprite)
						{
							actDamageSprite(actor->GetOwner(), hit.actor(), kDamageFall, 12);
						}
						break;

					case kThingPodGreenBall:
						if (hit.type == kHitSector)
						{
							actRadiusDamage(actor->GetOwner(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), 200, 1, 20, kDamageExplode, 6, 0);
							evPostActor(actor, 0, kCallbackFXPodBloodSplat);
						}
						else if (hit.type == kHitSprite)
						{
							actDamageSprite(actor->GetOwner(), hit.actor(), kDamageFall, 12);
							evPostActor(actor, 0, kCallbackFXPodBloodSplat);
						}
						break;

					case kThingPodFireBall:
						actExplodeSprite(actor);
						break;
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actCheckProjectiles()
{
	BloodStatIterator it(kStatProjectile);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32)
			continue;
		viewBackupSpriteLoc(actor);
		int hit = MoveMissile(actor);
		if (hit >= 0) actImpactMissile(actor, hit);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
static TArray<walltype*> affectedXWalls; // keep this outside the function so that it only needs to be allocated once

static void actCheckExplosion()
{
	BloodStatIterator it(kStatExplosion);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32)
			continue;

		if (!actor->hasX()) continue;

		auto Owner = actor->GetOwner();
		int nType = actor->spr.type;
		assert(nType >= 0 && nType < kExplodeMax);
		const EXPLOSION* pExplodeInfo = &explodeInfo[nType];
		int x = actor->int_pos().X;
		int y = actor->int_pos().Y;
		int z = actor->int_pos().Z;
		auto pSector = actor->sector();
		int radius = pExplodeInfo->radius;

#ifdef NOONE_EXTENSIONS
		// Allow to override explosion radius by data4 field of any sprite which have statnum 2 set in editor
		// or of Hidden Exploder.
		if (gModernMap && actor->xspr.data4 > 0)
			radius = actor->xspr.data4;
#endif

		// GetClosestSpriteSectors() has issues checking some sectors due to optimizations
		// the new flag newSectCheckMethod for GetClosestSpriteSectors() does rectify these issues, but this may cause unintended side effects for level scripted explosions
		// so only allow this new checking method for dude spawned explosions
		affectedXWalls.Clear();
		const bool newSectCheckMethod = !cl_bloodvanillaexplosions && Owner && Owner->IsDudeActor() && !VanillaMode(); // use new sector checking logic
		auto sectorMap = GetClosestSpriteSectors(pSector, x, y, radius, &affectedXWalls, newSectCheckMethod);

		for (auto pWall : affectedXWalls)
		{
			trTriggerWall(pWall, kCmdWallImpact, Owner);
		}

		BloodStatIterator it1(kStatDude);
		while (auto dudeactor = it1.Next())
		{
			if (dudeactor->spr.flags & 32) continue;

			if (CheckSector(sectorMap, dudeactor))
			{
				if (actor->xspr.data1 && CheckProximity(dudeactor, x, y, z, pSector, radius))
				{
					if (pExplodeInfo->dmg && actor->explosionhackflag)
					{
						actor->explosionhackflag = false;
						actDamageSprite(Owner, dudeactor, kDamageFall, (pExplodeInfo->dmg + Random(pExplodeInfo->dmgRng)) << 4);
					}
					if (pExplodeInfo->dmgType) ConcussSprite(actor, dudeactor, x, y, z, pExplodeInfo->dmgType);

					if (pExplodeInfo->burnTime && dudeactor->hasX())
					{
						if (!dudeactor->xspr.burnTime) evPostActor(dudeactor, 0, kCallbackFXFlameLick);
						actBurnSprite(Owner, dudeactor, pExplodeInfo->burnTime << 2);
					}
				}
			}
		}

		it1.Reset(kStatThing);
		while (auto thingactor = it1.Next())
		{
			if (thingactor->spr.flags & 32) continue;

			if (CheckSector(sectorMap, thingactor))
			{
				if (actor->xspr.data1 && CheckProximity(thingactor, x, y, z, pSector, radius) && thingactor->hasX())
				{
					if (!thingactor->xspr.locked)
					{
						if (pExplodeInfo->dmgType) ConcussSprite(Owner, thingactor, x, y, z, pExplodeInfo->dmgType);

						if (pExplodeInfo->burnTime)
						{
							if (thingactor->spr.type == kThingTNTBarrel && !thingactor->xspr.burnTime)
								evPostActor(thingactor, 0, kCallbackFXFlameLick);
							actBurnSprite(Owner, thingactor, pExplodeInfo->burnTime << 2);
						}
					}
				}
			}
		}

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			auto pos = gPlayer[p].actor->int_pos();
			int dx = (x - pos.X) >> 4;
			int dy = (y - pos.Y) >> 4;
			int dz = (z - pos.Z) >> 8;
			int nDist = dx * dx + dy * dy + dz * dz + 0x40000;
			int t = DivScale(actor->xspr.data2, nDist, 16);
			gPlayer[p].flickerEffect += t;
		}

#ifdef NOONE_EXTENSIONS
		if (actor->xspr.data1 != 0)
		{
			// add impulse for sprites from physics list
			if (gPhysSpritesCount > 0 && pExplodeInfo->dmgType != 0)
			{
				for (int i = 0; i < gPhysSpritesCount; i++)
				{
					if (gPhysSpritesList[i] == nullptr) continue;
					DBloodActor* physactor = gPhysSpritesList[i];
					if (!physactor->insector() || (physactor->spr.flags & kHitagFree) != 0) continue;

					if (!CheckSector(sectorMap, physactor) || !CheckProximity(physactor, x, y, z, pSector, radius)) continue;
					else debrisConcuss(Owner, i, x, y, z, pExplodeInfo->dmgType);
				}
			}

			// trigger sprites from impact list
			if (gImpactSpritesCount > 0) {
				for (int i = 0; i < gImpactSpritesCount; i++)
				{
					if (gImpactSpritesList[i] == nullptr) continue;

					DBloodActor* impactactor = gImpactSpritesList[i];
					if (!impactactor->hasX() || !impactactor->insector() || (impactactor->spr.flags & kHitagFree) != 0)	continue;

					if (!CheckSector(sectorMap, impactactor) || !CheckProximity(impactactor, x, y, z, pSector, radius))
						continue;

					trTriggerSprite(impactactor, kCmdSpriteImpact, Owner);
				}
			}

		}

		if (!gModernMap || !(actor->spr.flags & kModernTypeFlag1))
		{
			// if data4 > 0, do not remove explosion. This can be useful when designer wants put explosion generator in map manually via sprite statnum 2.
			actor->xspr.data1 = ClipLow(actor->xspr.data1 - 4, 0);
			actor->xspr.data2 = ClipLow(actor->xspr.data2 - 4, 0);
			actor->xspr.data3 = ClipLow(actor->xspr.data3 - 4, 0);
		}
#else
		actor->xspr.data1 = ClipLow(actor->xspr.data1 - 4, 0);
		actor->xspr.data2 = ClipLow(actor->xspr.data2 - 4, 0);
		actor->xspr.data3 = ClipLow(actor->xspr.data3 - 4, 0);
#endif

		if (actor->xspr.data1 == 0 && actor->xspr.data2 == 0 && actor->xspr.data3 == 0 && seqGetStatus(actor) < 0)
			actPostSprite(actor, kStatFree);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actCheckTraps()
{
	BloodStatIterator it(kStatTraps);
	while (auto actor = it.Next())
	{
		if ((actor->spr.flags & 32) || !actor->hasX())
			continue;

		switch (actor->spr.type) {
		case kTrapSawCircular:
			actor->xspr.data2 = ClipLow(actor->xspr.data2 - 4, 0);
			break;

		case kTrapFlame:
			if (actor->xspr.state && seqGetStatus(actor) < 0)
			{
				int x = actor->int_pos().X;
				int y = actor->int_pos().Y;
				int z = actor->int_pos().Z;
				int t = (actor->xspr.data1 << 23) / 120;
				int dx = MulScale(t, Cos(actor->spr.ang), 30);
				int dy = MulScale(t, Sin(actor->spr.ang), 30);
				for (int i = 0; i < 2; i++)
				{
					auto pFX = gFX.fxSpawnActor(FX_32, actor->sector(), x, y, z, 0);
					if (pFX)
					{
						pFX->vel.X = dx + Random2(0x8888);
						pFX->vel.Y = dy + Random2(0x8888);
						pFX->vel.Z = Random2(0x8888);
					}
					x += (dx / 2) >> 12;
					y += (dy / 2) >> 12;
				}
				dy = bsin(actor->spr.ang);
				dx = bcos(actor->spr.ang);
				gVectorData[kVectorTchernobogBurn].maxDist = actor->xspr.data1 << 9;
				actFireVector(actor, 0, 0, dx, dy, Random2(0x8888), kVectorTchernobogBurn);
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actCheckDudes()
{
	BloodStatIterator it(kStatDude);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32)
			continue;

		if (actor->hasX())
		{
			const bool fixBurnGlitch = !cl_bloodvanillaenemies && IsBurningDude(actor) && !VanillaMode(); // if enemies are burning, always apply burning damage per tick
			if ((actor->xspr.burnTime > 0) || fixBurnGlitch)
			{
				switch (actor->spr.type)
				{
				case kDudeBurningInnocent:
				case kDudeBurningCultist:
				case kDudeBurningZombieAxe:
				case kDudeBurningZombieButcher:
					actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
					break;

				default:
					actor->xspr.burnTime = ClipLow(actor->xspr.burnTime - 4, 0);
					actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
					break;
				}
			}

#ifdef NOONE_EXTENSIONS
			// handle incarnations of custom dude
			if (actor->spr.type == kDudeModernCustom && actor->xspr.txID > 0 && actor->xspr.sysData1 == kGenDudeTransformStatus)
			{
				actor->vel.X = actor->vel.Y = 0;
				if (seqGetStatus(actor) < 0) genDudeTransform(actor);
			}
#endif
			if (actor->spr.type == kDudeCerberusTwoHead)
			{
				if (actor->xspr.health <= 0 && seqGetStatus(actor) < 0)
				{
					actor->xspr.health = dudeInfo[28].startHealth << 4;
					actor->spr.type = kDudeCerberusOneHead;
					if (actor->GetTarget() != nullptr) aiSetTarget(actor, actor->GetTarget());
					aiActivateDude(actor);
				}
			}
			if (actor->xspr.Proximity && !actor->xspr.isTriggered)
			{
				BloodStatIterator it1(kStatDude);
				while (auto actor2 = it1.Next())
				{
					if (actor2->spr.flags & 32) continue;

					if (actor2->IsPlayerActor() && (unsigned int)actor2->xspr.health > 0)
					{
						if (CheckProximity(actor2, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), 128))
							trTriggerSprite(actor, kCmdSpriteProximity, actor2);
					}
				}
			}
			if (actor->IsPlayerActor())
			{
				PLAYER* pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];
				if (pPlayer->voodooTargets) voodooTarget(pPlayer);
				if (pPlayer->hand && Chance(0x8000)) actDamageSprite(actor, actor, kDamageDrown, 12);

				if (pPlayer->isUnderwater)
				{
					bool bActive = packItemActive(pPlayer, 1);

					if (bActive || pPlayer->godMode) pPlayer->underwaterTime = 1200;
					else pPlayer->underwaterTime = ClipLow(pPlayer->underwaterTime - 4, 0);

					if (pPlayer->underwaterTime < 1080 && packCheckItem(pPlayer, 1) && !bActive)
						packUseItem(pPlayer, 1);

					if (!pPlayer->underwaterTime)
					{
						pPlayer->chokeEffect += 4;
						if (Chance(pPlayer->chokeEffect))
							actDamageSprite(actor, actor, kDamageDrown, 3 << 4);
					}
					else
						pPlayer->chokeEffect = 0;

					if (actor->vel.X || actor->vel.Y)
						sfxPlay3DSound(actor, 709, 100, 2);

					pPlayer->bubbleTime = ClipLow(pPlayer->bubbleTime - 4, 0);
				}
				else if (gGameOptions.nGameType == 0)
				{
					if (pPlayer->actor->xspr.health > 0 && pPlayer->restTime >= 1200 && Chance(0x200))
					{
						pPlayer->restTime = -1;
						sfxPlay3DSound(actor, 3100 + Random(11), 0, 2);
					}
				}
			}
			ProcessTouchObjects(actor);
		}
	}

	it.Reset(kStatDude);
	while (auto actor = it.Next())
	{
		if (actor->spr.flags & 32 || !actor->hasX()) continue;

		auto pSector = actor->sector();
		viewBackupSpriteLoc(actor);
		XSECTOR* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

		if (pXSector)
		{
			int top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (getflorzofslopeptr(pSector, actor->int_pos().X, actor->int_pos().Y) <= bottom)
			{
				int angle = pXSector->panAngle;
				int speed = 0;
				if (pXSector->panAlways || pXSector->state || pXSector->busy)
				{
					speed = pXSector->panVel << 9;
					if (!pXSector->panAlways && pXSector->busy)
						speed = MulScale(speed, pXSector->busy, 16);
				}
				if (pSector->floorstat & CSTAT_SECTOR_ALIGN)
					angle = (angle + GetWallAngle(pSector->firstWall()) + 512) & 2047;
				int dx = MulScale(speed, Cos(angle), 30);
				int dy = MulScale(speed, Sin(angle), 30);
				actor->vel.X += dx;
				actor->vel.Y += dy;
			}
		}
		if (pXSector && pXSector->Underwater) actAirDrag(actor, 5376);
		else actAirDrag(actor, 128);

		if ((actor->spr.flags & 4) || actor->vel.X || actor->vel.Y || actor->vel.Z || actor->sector()->velFloor || actor->sector()->velCeil)
			MoveDude(actor);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actCheckFlares()
{
	BloodStatIterator it(kStatFlare);
	while (auto actor = it.Next())
	{
		if ((actor->spr.flags & 32) || !actor->hasX()) continue;

		auto target = actor->GetTarget();
		if (!target) continue;

		viewBackupSpriteLoc(actor);
		if (target->spr.statnum == kMaxStatus)
		{
			GibSprite(actor, GIBTYPE_17, NULL, NULL);
			actPostSprite(actor, kStatFree);
		}
		if (target->hasX() && target->xspr.health > 0)
		{
			int x = target->int_pos().X + mulscale30r(Cos(actor->xspr.goalAng + target->spr.ang), target->spr.clipdist * 2);
			int y = target->int_pos().Y + mulscale30r(Sin(actor->xspr.goalAng + target->spr.ang), target->spr.clipdist * 2);
			int z = target->int_pos().Z + actor->xspr.TargetPos.Z;
			vec3_t pos = { x, y, z };
			SetActor(actor, &pos);
			actor->vel.X = target->vel.X;
			actor->vel.Y = target->vel.Y;
			actor->vel.Z = target->vel.Z;
		}
		else
		{
			GibSprite(actor, GIBTYPE_17, NULL, NULL);
			actPostSprite(actor, kStatFree);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actProcessSprites(void)
{
#ifdef NOONE_EXTENSIONS
	if (gModernMap) nnExtProcessSuperSprites();
#endif

	actCheckProximity();
	actCheckThings();
	actCheckProjectiles();
	actCheckExplosion();
	actCheckTraps();
	actCheckDudes();
	actCheckFlares();
	aiProcessDudes();
	gFX.fxProcess();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actSpawnSprite(sectortype* pSector, int x, int y, int z, int nStat, bool setextra)
{
	DBloodActor* actor = InsertSprite(pSector, nStat);

	vec3_t pos = { x, y, z };
	SetActor(actor, &pos);
	actor->spr.type = kSpriteDecoration;
	if (setextra && !actor->hasX())
	{
		actor->addX();
		actor->hit.florhit.setNone();
		actor->hit.ceilhit.setNone();
		if (!VanillaMode()) actor->SetTarget(nullptr);
	}
	return actor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actSpawnSprite(DBloodActor* source, int nStat)
{
	DBloodActor* actor = InsertSprite(source->sector(), nStat);

	actor->set_int_pos(source->int_pos());
	actor->vel = source->vel;
	actor->spr.flags = 0;
	actor->addX();
	actor->hit.florhit.setNone();
	actor->hit.ceilhit.setNone();
	if (!VanillaMode()) actor->SetTarget(nullptr);
	return actor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actSpawnDude(DBloodActor* source, int nType, int a3, int a4)
{
	auto spawned = actSpawnSprite(source, kStatDude);
	if (!spawned) return nullptr;
	int angle = source->spr.ang;
	int nDude = nType - kDudeBase;
	int x, y, z;
	z = a4 + source->int_pos().Z;
	if (a3 < 0)
	{
		x = source->int_pos().X;
		y = source->int_pos().Y;
	}
	else
	{
		x = source->int_pos().X + mulscale30r(Cos(angle), a3);
		y = source->int_pos().Y + mulscale30r(Sin(angle), a3);
	}
	spawned->spr.type = nType;
	if (!VanillaMode())
		 spawned->spr.inittype = nType;
	spawned->spr.ang = angle;
	vec3_t pos = { x, y, z };
	SetActor(spawned, &pos);
	spawned->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_BLOOD_BIT1;
	spawned->spr.clipdist = getDudeInfo(nDude + kDudeBase)->clipdist;
	spawned->xspr.health = getDudeInfo(nDude + kDudeBase)->startHealth << 4;
	spawned->xspr.respawn = 1;
	if (getSequence(getDudeInfo(nDude + kDudeBase)->seqStartID))
		seqSpawn(getDudeInfo(nDude + kDudeBase)->seqStartID, spawned, -1);

#ifdef NOONE_EXTENSIONS
	// add a way to inherit some values of spawner type 18 by dude.
	// This way designer can count enemies via switches and do many other interesting things.
	if (gModernMap && source->spr.flags & kModernTypeFlag1)
	{
		// allow inheriting only for selected source types
		switch (source->spr.type)
		{
		case kMarkerDudeSpawn:
			//inherit pal?
			if (spawned->spr.pal <= 0) spawned->spr.pal = source->spr.pal;

			// inherit spawn sprite trigger settings, so designer can count monsters.
			spawned->xspr.txID = source->xspr.txID;
			spawned->xspr.command = source->xspr.command;
			spawned->xspr.triggerOn = source->xspr.triggerOn;
			spawned->xspr.triggerOff = source->xspr.triggerOff;

			// inherit drop items
			spawned->xspr.dropMsg = source->xspr.dropMsg;

			// inherit dude flags
			spawned->xspr.dudeDeaf = source->xspr.dudeDeaf;
			spawned->xspr.dudeGuard = source->xspr.dudeGuard;
			spawned->xspr.dudeAmbush = source->xspr.dudeAmbush;
			spawned->xspr.dudeFlag4 = source->xspr.dudeFlag4;
			spawned->xspr.unused1 = source->xspr.unused1;
			break;
		}
	}
#endif

	aiInitSprite(spawned);
	return spawned;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actSpawnThing(sectortype* pSector, int x, int y, int z, int nThingType)
{
	assert(nThingType >= kThingBase && nThingType < kThingMax);
	auto actor = actSpawnSprite(pSector, x, y, z, 4, 1);
	int nType = nThingType - kThingBase;
	actor->spr.type = nThingType;
	assert(actor->hasX());
	const THINGINFO* pThingInfo = &thingInfo[nType];
	actor->xspr.health = pThingInfo->startHealth << 4;
	actor->spr.clipdist = pThingInfo->clipdist;
	actor->spr.flags = pThingInfo->flags;
	if (actor->spr.flags & 2) actor->spr.flags |= 4;
	actor->spr.cstat |= pThingInfo->cstat;
	actor->spr.picnum = pThingInfo->picnum;
	actor->spr.shade = pThingInfo->shade;
	actor->spr.pal = pThingInfo->pal;
	if (pThingInfo->xrepeat) actor->spr.xrepeat = pThingInfo->xrepeat;
	if (pThingInfo->yrepeat) actor->spr.yrepeat = pThingInfo->yrepeat;
	actor->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;
	switch (nThingType)
	{
	case kThingVoodooHead:
		actor->xspr.data1 = 0;
		actor->xspr.data2 = 0;
		actor->xspr.data3 = 0;
		actor->xspr.data4 = 0;
		actor->xspr.state = 1;
		actor->xspr.triggerOnce = 1;
		actor->xspr.isTriggered = 0;
		break;

	case kThingDroppedLifeLeech:
#ifdef NOONE_EXTENSIONS
	case kModernThingEnemyLifeLeech:
#endif
		actor->xspr.data1 = 0;
		actor->xspr.data2 = 0;
		actor->xspr.data3 = 0;
		actor->xspr.data4 = 0;
		actor->xspr.state = 1;
		actor->xspr.triggerOnce = 0;
		actor->xspr.isTriggered = 0;
		break;

	case kThingZombieHead:
		actor->xspr.data1 = 8;
		actor->xspr.data2 = 0;
		actor->xspr.data3 = 0;
		actor->xspr.data4 = 318;
		actor->xspr.TargetPos.X = PlayClock + 180;
		actor->xspr.locked = 1;
		actor->xspr.state = 1;
		actor->xspr.triggerOnce = 0;
		actor->xspr.isTriggered = 0;
		break;

	case kThingBloodBits:
	case kThingBloodChunks:
		actor->xspr.data1 = (nThingType == kThingBloodBits) ? 19 : 8;
		actor->xspr.data2 = 0;
		actor->xspr.data3 = 0;
		actor->xspr.data4 = 319;
		actor->xspr.TargetPos.X = PlayClock + 180;
		actor->xspr.locked = 1;
		actor->xspr.state = 1;
		actor->xspr.triggerOnce = 0;
		actor->xspr.isTriggered = 0;
		break;

	case kThingArmedTNTStick:
		evPostActor(actor, 0, kCallbackFXDynPuff);
		sfxPlay3DSound(actor, 450, 0, 0);
		break;

	case kThingArmedTNTBundle:
		sfxPlay3DSound(actor, 450, 0, 0);
		evPostActor(actor, 0, kCallbackFXDynPuff);
		break;

	case kThingArmedSpray:
		evPostActor(actor, 0, kCallbackFXDynPuff);
		break;
	}
	return actor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actFireThing(DBloodActor* actor, int a2, int a3, int a4, int thingType, int a6)
{
	assert(thingType >= kThingBase && thingType < kThingMax);
	int x = actor->int_pos().X + MulScale(a2, Cos(actor->spr.ang + 512), 30);
	int y = actor->int_pos().Y + MulScale(a2, Sin(actor->spr.ang + 512), 30);
	int z = actor->int_pos().Z + a3;
	x += MulScale(actor->spr.clipdist, Cos(actor->spr.ang), 28);
	y += MulScale(actor->spr.clipdist, Sin(actor->spr.ang), 28);
	if (HitScan(actor, z, x - actor->int_pos().X, y - actor->int_pos().Y, 0, CLIPMASK0, actor->spr.clipdist) != -1)
	{
		x = gHitInfo.hitpos.X - MulScale(actor->spr.clipdist << 1, Cos(actor->spr.ang), 28);
		y = gHitInfo.hitpos.Y - MulScale(actor->spr.clipdist << 1, Sin(actor->spr.ang), 28);
	}
	auto fired = actSpawnThing(actor->sector(), x, y, z, thingType);
	fired->SetOwner(actor);
	fired->spr.ang = actor->spr.ang;
	fired->vel.X = MulScale(a6, Cos(fired->spr.ang), 30);
	fired->vel.Y = MulScale(a6, Sin(fired->spr.ang), 30);
	fired->vel.Z = MulScale(a6, a4, 14);
	fired->vel.X += actor->vel.X / 2;
	fired->vel.Y += actor->vel.Y / 2;
	fired->vel.Z += actor->vel.Z / 2;
	return fired;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actBuildMissile(DBloodActor* spawned, DBloodActor* actor)
{
	switch (spawned->spr.type)
	{
	case kMissileLifeLeechRegular:
		evPostActor(spawned, 0, kCallbackFXFlameLick);
		break;
	case kMissileTeslaAlt:
		evPostActor(spawned, 0, kCallbackFXTeslaAlt);
		break;
	case kMissilePukeGreen:
		seqSpawn(29, spawned, -1);
		break;
	case kMissileButcherKnife:
		spawned->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
		break;
	case kMissileTeslaRegular:
		sfxPlay3DSound(spawned, 251, 0, 0);
		break;
	case kMissileEctoSkull:
		seqSpawn(2, spawned, -1);
		sfxPlay3DSound(spawned, 493, 0, 0);
		break;
	case kMissileFireballNapalm:
		seqSpawn(61, spawned, nNapalmClient);
		sfxPlay3DSound(spawned, 441, 0, 0);
		break;
	case kMissileFireball:
		seqSpawn(22, spawned, nFireballClient);
		sfxPlay3DSound(spawned, 441, 0, 0);
		break;
	case kMissileFlameHound:
		seqSpawn(27, spawned, -1);
		spawned->vel.X += actor->vel.X / 2 + Random2(0x11111);
		spawned->vel.Y += actor->vel.Y / 2 + Random2(0x11111);
		spawned->vel.Z += actor->vel.Z / 2 + Random2(0x11111);
		break;
	case kMissileFireballCerberus:
		seqSpawn(61, spawned, dword_2192E0);
		sfxPlay3DSound(spawned, 441, 0, 0);
		break;
	case kMissileFireballTchernobog:
		seqSpawn(23, spawned, dword_2192D8);
		spawned->vel.X += actor->vel.X / 2 + Random2(0x11111);
		spawned->vel.Y += actor->vel.Y / 2 + Random2(0x11111);
		spawned->vel.Z += actor->vel.Z / 2 + Random2(0x11111);
		break;
	case kMissileFlameSpray:
		if (Chance(0x8000))	seqSpawn(0, spawned, -1);
		else seqSpawn(1, spawned, -1);
		spawned->vel.X += actor->vel.X / 2 + Random2(0x11111);
		spawned->vel.Y += actor->vel.Y / 2 + Random2(0x11111);
		spawned->vel.Z += actor->vel.Z / 2 + Random2(0x11111);
		break;
	case kMissileFlareAlt:
		evPostActor(spawned, 30, kCallbackFXFlareBurst);
		evPostActor(spawned, 0, kCallbackFXFlareSpark);
		sfxPlay3DSound(spawned, 422, 0, 0);
		break;
	case kMissileFlareRegular:
		evPostActor(spawned, 0, kCallbackFXFlareSpark);
		sfxPlay3DSound(spawned, 422, 0, 0);
		break;
	case kMissileLifeLeechAltSmall:
		evPostActor(spawned, 0, kCallbackFXArcSpark);
		break;
	case kMissileArcGargoyle:
		sfxPlay3DSound(spawned, 252, 0, 0);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actFireMissile(DBloodActor* actor, int a2, int a3, int a4, int a5, int a6, int nType)
{

	assert(nType >= kMissileBase && nType < kMissileMax);
	bool impact = false;
	const MissileType* pMissileInfo = &missileInfo[nType - kMissileBase];
	int x = actor->int_pos().X + MulScale(a2, Cos(actor->spr.ang + 512), 30);
	int y = actor->int_pos().Y + MulScale(a2, Sin(actor->spr.ang + 512), 30);
	int z = actor->int_pos().Z + a3;
	int clipdist = pMissileInfo->clipDist + actor->spr.clipdist;
	x += MulScale(clipdist, Cos(actor->spr.ang), 28);
	y += MulScale(clipdist, Sin(actor->spr.ang), 28);
	int hit = HitScan(actor, z, x - actor->int_pos().X, y - actor->int_pos().Y, 0, CLIPMASK0, clipdist);
	if (hit != -1)
	{
		if (hit == 3 || hit == 0)
		{
			impact = true;
			x = gHitInfo.hitpos.X - MulScale(Cos(actor->spr.ang), 16, 30);
			y = gHitInfo.hitpos.Y - MulScale(Sin(actor->spr.ang), 16, 30);
		}
		else
		{
			x = gHitInfo.hitpos.X - MulScale(pMissileInfo->clipDist << 1, Cos(actor->spr.ang), 28);
			y = gHitInfo.hitpos.Y - MulScale(pMissileInfo->clipDist << 1, Sin(actor->spr.ang), 28);
		}
	}
	auto spawned = actSpawnSprite(actor->sector(), x, y, z, 5, 1);

	spawned->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;
	spawned->spr.type = nType;
	spawned->spr.shade = pMissileInfo->shade;
	spawned->spr.pal = 0;
	spawned->spr.clipdist = pMissileInfo->clipDist;
	spawned->spr.flags = 1;
	spawned->spr.xrepeat = pMissileInfo->xrepeat;
	spawned->spr.yrepeat = pMissileInfo->yrepeat;
	spawned->spr.picnum = pMissileInfo->picnum;
	spawned->spr.ang = (actor->spr.ang + pMissileInfo->angleOfs) & 2047;
	spawned->vel.X = MulScale(pMissileInfo->velocity, a4, 14);
	spawned->vel.Y = MulScale(pMissileInfo->velocity, a5, 14);
	spawned->vel.Z = MulScale(pMissileInfo->velocity, a6, 14);
	spawned->SetOwner(actor);
	spawned->spr.cstat |= CSTAT_SPRITE_BLOCK;
	spawned->SetTarget(nullptr);
	evPostActor(spawned, 600, kCallbackRemove);

	actBuildMissile(spawned, actor);

	if (impact)
	{
		actImpactMissile(spawned, hit);
		return nullptr;
	}
	return spawned;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actGetRespawnTime(DBloodActor* actor)
{
	if (!actor->hasX()) return -1;

	if (actor->IsDudeActor() && !actor->IsPlayerActor())
	{
		if (actor->xspr.respawn == 2 || (actor->xspr.respawn != 1 && gGameOptions.nMonsterSettings == 2))
			return gGameOptions.nMonsterRespawnTime;
		return -1;
	}

	if (actor->IsWeaponActor())
	{
		if (actor->xspr.respawn == 3 || gGameOptions.nWeaponSettings == 1) return 0;
		else if (actor->xspr.respawn != 1 && gGameOptions.nWeaponSettings != 0)
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}

	if (actor->IsAmmoActor())
	{
		if (actor->xspr.respawn == 2 || (actor->xspr.respawn != 1 && gGameOptions.nWeaponSettings != 0))
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}

	if (actor->IsItemActor())
	{
		if (actor->xspr.respawn == 3 && gGameOptions.nGameType == 1) return 0;
		else if (actor->xspr.respawn == 2 || (actor->xspr.respawn != 1 && gGameOptions.nItemSettings != 0))
		{
			switch (actor->spr.type)
			{
			case kItemShadowCloak:
			case kItemTwoGuns:
			case kItemReflectShots:
				return gGameOptions.nSpecialRespawnTime;
			case kItemDeathMask:
				return gGameOptions.nSpecialRespawnTime << 1;
			default:
				return gGameOptions.nItemRespawnTime;
			}
		}
		return -1;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool actCheckRespawn(DBloodActor* actor)
{
	if (actor->hasX())
	{
		int nRespawnTime = actGetRespawnTime(actor);
		if (nRespawnTime < 0) return 0;

		actor->xspr.respawnPending = 1;
		if (actor->spr.type >= kThingBase && actor->spr.type < kThingMax)
		{
			actor->xspr.respawnPending = 3;
			if (actor->spr.type == kThingTNTBarrel) actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		}
		if (nRespawnTime > 0)
		{
			if (actor->xspr.respawnPending == 1) nRespawnTime = MulScale(nRespawnTime, 0xa000, 16);
			actor->spr.intowner = actor->spr.statnum;
			actPostSprite(actor, kStatRespawn);
			actor->spr.flags |= kHitagRespawn;

			if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax))
			{
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				actor->set_int_pos(actor->basePoint);
			}
			evPostActor(actor, nRespawnTime, kCallbackRespawn);
		}
		return 1;
	}
	return  0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool actCanSplatWall(walltype* pWall)
{
	if (pWall->cstat & (CSTAT_WALL_MOVE_MASK)) return 0;

	int nType = pWall->type;
	if (nType >= kWallBase && nType < kWallMax) return 0;

	if (pWall->twoSided())
	{
		sectortype* pSector = pWall->nextSector();
		if (pSector->type >= kSectorBase && pSector->type < kSectorMax) return 0;
	}
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actFireVector(DBloodActor* shooter, int a2, int a3, int a4, int a5, int a6, VECTOR_TYPE vectorType)
{
	assert(vectorType >= 0 && vectorType < kVectorMax);
	const VECTORDATA* pVectorData = &gVectorData[vectorType];
	int nRange = pVectorData->maxDist;
	int hit = VectorScan(shooter, a2, a3, a4, a5, a6, nRange, 1);
	if (hit == 3)
	{
		auto hitactor = gHitInfo.actor();
		assert(hitactor != nullptr);
		if (!gGameOptions.bFriendlyFire && IsTargetTeammate(shooter, hitactor)) return;
		if (hitactor->IsPlayerActor())
		{
			PLAYER* pPlayer = &gPlayer[hitactor->spr.type - kDudePlayer1];
			if (powerupCheck(pPlayer, kPwUpReflectShots))
			{
				gHitInfo.hitActor = shooter;
				gHitInfo.hitpos = shooter->int_pos();
			}
		}
	}
	int x = gHitInfo.hitpos.X - MulScale(a4, 16, 14);
	int y = gHitInfo.hitpos.Y - MulScale(a5, 16, 14);
	int z = gHitInfo.hitpos.Z - MulScale(a6, 256, 14);
	auto pSector = gHitInfo.hitSector;
	uint8_t nSurf = kSurfNone;
	if (nRange == 0 || approxDist(gHitInfo.hitpos.X - shooter->int_pos().X, gHitInfo.hitpos.Y - shooter->int_pos().Y) < nRange)
	{
		switch (hit)
		{
		case 1:
		{
			if (pSector->ceilingstat & CSTAT_SECTOR_SKY)
				nSurf = kSurfNone;
			else
				nSurf = surfType[pSector->ceilingpicnum];
			break;
		}
		case 2:
		{
			if (pSector->floorstat & CSTAT_SECTOR_SKY)
				nSurf = kSurfNone;
			else
				nSurf = surfType[pSector->floorpicnum];
			break;
		}
		case 0:
		{
			auto pWall = gHitInfo.hitWall;
			nSurf = surfType[pWall->picnum];
			if (actCanSplatWall(pWall))
			{
				int xx = gHitInfo.hitpos.X - MulScale(a4, 16, 14);
				int yy = gHitInfo.hitpos.Y - MulScale(a5, 16, 14);
				int zz = gHitInfo.hitpos.Z - MulScale(a6, 256, 14);
				int nnSurf = surfType[pWall->picnum];
				assert(nnSurf < kSurfMax);
				if (pVectorData->surfHit[nnSurf].fx1 >= 0)
				{
					auto pFX = gFX.fxSpawnActor(pVectorData->surfHit[nnSurf].fx1, pSector, xx, yy, zz, 0);
					if (pFX)
					{
						pFX->spr.ang = (GetWallAngle(pWall) + 512) & 2047;
						pFX->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
					}
				}
			}
			break;
		}
		case 4:
		{
			auto pWall = gHitInfo.hitWall;
			nSurf = surfType[pWall->overpicnum];
			if (pWall->hasX())
			{
				if (pWall->xw().triggerVector)
					trTriggerWall(pWall, kCmdWallImpact, shooter);
			}
			break;
		}
		case 3:
		{
			auto actor = gHitInfo.actor();
			nSurf = surfType[actor->spr.picnum];
			x -= MulScale(a4, 112, 14);
			y -= MulScale(a5, 112, 14);
			z -= MulScale(a6, 112 << 4, 14);
			int shift = 4;
			if (vectorType == kVectorTine && !actor->IsPlayerActor()) shift = 3;

			actDamageSprite(shooter, actor, pVectorData->dmgType, pVectorData->dmg << shift);
			if (actor->hasX() && actor->xspr.Vector) trTriggerSprite(actor, kCmdSpriteImpact, shooter);

			if (actor->spr.statnum == kStatThing)
			{
				int t = thingInfo[actor->spr.type - kThingBase].mass;
				if (t > 0 && pVectorData->impulse)
				{
					int t2 = DivScale(pVectorData->impulse, t, 8);
					actor->vel.X += MulScale(a4, t2, 16);
					actor->vel.Y += MulScale(a5, t2, 16);
					actor->vel.Z += MulScale(a6, t2, 16);
				}
				if (pVectorData->burnTime)
				{
					if (!actor->xspr.burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
					actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
				}
			}
			if (actor->spr.statnum == kStatDude && actor->hasX())
			{
				int t = getDudeInfo(actor->spr.type)->mass;

#ifdef NOONE_EXTENSIONS
				if (actor->IsDudeActor())
				{
					switch (actor->spr.type)
					{
					case kDudeModernCustom:
					case kDudeModernCustomBurning:
						t = getSpriteMassBySize(actor);
						break;
					}
				}
#endif

				if (t > 0 && pVectorData->impulse)
				{
					int t2 = DivScale(pVectorData->impulse, t, 8);
					actor->vel.X += MulScale(a4, t2, 16);
					actor->vel.Y += MulScale(a5, t2, 16);
					actor->vel.Z += MulScale(a6, t2, 16);
				}
				if (pVectorData->burnTime)
				{
					if (!actor->xspr.burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
					actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
				}
				if (Chance(pVectorData->fxChance))
				{
					int tt = gVectorData[19].maxDist;
					a4 += Random3(4000);
					a5 += Random3(4000);
					a6 += Random3(4000);
					if (HitScan(actor, gHitInfo.hitpos.Z, a4, a5, a6, CLIPMASK1, tt) == 0)
					{
						if (approxDist(gHitInfo.hitpos.X - actor->int_pos().X, gHitInfo.hitpos.Y - actor->int_pos().Y) <= tt)
						{
							auto pWall = gHitInfo.hitWall;
							auto pSector1 = gHitInfo.hitSector;
							if (actCanSplatWall(pWall))
							{
								int xx = gHitInfo.hitpos.X - MulScale(a4, 16, 14);
								int yy = gHitInfo.hitpos.Y - MulScale(a5, 16, 14);
								int zz = gHitInfo.hitpos.Z - MulScale(a6, 16 << 4, 14);
								int nnSurf = surfType[pWall->picnum];
								const VECTORDATA* pVectorData1 = &gVectorData[19];
								FX_ID t2 = pVectorData1->surfHit[nnSurf].fx2;
								FX_ID t3 = pVectorData1->surfHit[nnSurf].fx3;

								DBloodActor* pFX = nullptr;
								if (t2 > FX_NONE && (t3 == FX_NONE || Chance(0x4000))) pFX = gFX.fxSpawnActor(t2, pSector1, xx, yy, zz, 0);
								else if (t3 > FX_NONE) pFX = gFX.fxSpawnActor(t3, pSector1, xx, yy, zz, 0);
								if (pFX)
								{
									pFX->vel.Z = 0x2222;
									pFX->spr.ang = (GetWallAngle(pWall) + 512) & 2047;
									pFX->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
								}
							}
						}
					}
				}
				for (int i = 0; i < pVectorData->bloodSplats; i++)
					if (Chance(pVectorData->splatChance))
						fxSpawnBlood(actor, pVectorData->dmg << 4);
			}
#ifdef NOONE_EXTENSIONS
			// add impulse for sprites from physics list
			if (gPhysSpritesCount > 0 && pVectorData->impulse)
			{

				if (actor->hasX())
				{
					if (actor->xspr.physAttr & kPhysDebrisVector) {

						int impulse = DivScale(pVectorData->impulse, ClipLow(actor->spriteMass.mass, 10), 6);
						actor->vel.X += MulScale(a4, impulse, 16);
						actor->vel.Y += MulScale(a5, impulse, 16);
						actor->vel.Z += MulScale(a6, impulse, 16);

						if (pVectorData->burnTime != 0) {
							if (!actor->xspr.burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
							actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
						}

						if (actor->spr.type >= kThingBase && actor->spr.type < kThingMax) {
							actor->spr.statnum = kStatThing; // temporary change statnum property
							actDamageSprite(shooter, actor, pVectorData->dmgType, pVectorData->dmg << 4);
							actor->spr.statnum = kStatDecoration; // return statnum property back
						}

					}


				}


			}
#endif
			break;
		}
		}
	}
	assert(nSurf < kSurfMax);
#ifdef NOONE_EXTENSIONS

	// let the patrol enemies hear surface hit sounds!

	if (pVectorData->surfHit[nSurf].fx2 >= 0) {

		auto pFX2 = gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx2, pSector, x, y, z, 0);
		if (pFX2 && gModernMap)
			pFX2->SetOwner(shooter);
	}

	if (pVectorData->surfHit[nSurf].fx3 >= 0) {

		auto pFX3 = gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx3, pSector, x, y, z, 0);
		if (pFX3 && gModernMap)
			pFX3->SetOwner(shooter);

	}

#else
	if (pVectorData->surfHit[nSurf].fx2 >= 0)
		gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx2, pSector, x, y, z, 0);
	if (pVectorData->surfHit[nSurf].fx3 >= 0)
		gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx3, pSector, x, y, z, 0);
#endif

	if (pVectorData->surfHit[nSurf].fxSnd >= 0)
		sfxPlay3DSound(x, y, z, pVectorData->surfHit[nSurf].fxSnd, pSector);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireballSeqCallback(int, DBloodActor* actor)
{
	auto pFX = gFX.fxSpawnActor(FX_11, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X;
		pFX->vel.Y = actor->vel.Y;
		pFX->vel.Z = actor->vel.Z;
	}
}

void NapalmSeqCallback(int, DBloodActor* actor)
{
	auto pFX = gFX.fxSpawnActor(FX_12, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X;
		pFX->vel.Y = actor->vel.Y;
		pFX->vel.Z = actor->vel.Z;
	}
}

void Fx32Callback(int, DBloodActor* actor)
{
	auto pFX = gFX.fxSpawnActor(FX_32, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X;
		pFX->vel.Y = actor->vel.Y;
		pFX->vel.Z = actor->vel.Z;
	}
}

void Fx33Callback(int, DBloodActor* actor)
{
	auto pFX = gFX.fxSpawnActor(FX_33, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X;
		pFX->vel.Y = actor->vel.Y;
		pFX->vel.Z = actor->vel.Z;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void TreeToGibCallback(int, DBloodActor* actor)
{
	actor->spr.type = kThingObjectExplode;
	actor->xspr.state = 1;
	actor->xspr.data1 = 15;
	actor->xspr.data2 = 0;
	actor->xspr.data3 = 0;
	actor->xspr.health = thingInfo[17].startHealth;
	actor->xspr.data4 = 312;
	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;
}

void DudeToGibCallback1(int, DBloodActor* actor)
{
	actor->spr.type = kThingBloodChunks;
	actor->xspr.data1 = 8;
	actor->xspr.data2 = 0;
	actor->xspr.data3 = 0;
	actor->xspr.health = thingInfo[26].startHealth;
	actor->xspr.data4 = 319;
	actor->xspr.triggerOnce = 0;
	actor->xspr.isTriggered = 0;
	actor->xspr.locked = 0;
	actor->xspr.TargetPos.X = PlayClock;
	actor->xspr.state = 1;
}

void DudeToGibCallback2(int, DBloodActor* actor)
{
	actor->spr.type = kThingBloodChunks;
	actor->xspr.data1 = 3;
	actor->xspr.data2 = 0;
	actor->xspr.data3 = 0;
	actor->xspr.health = thingInfo[26].startHealth;
	actor->xspr.data4 = 319;
	actor->xspr.triggerOnce = 0;
	actor->xspr.isTriggered = 0;
	actor->xspr.locked = 0;
	actor->xspr.TargetPos.X = PlayClock;
	actor->xspr.state = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actPostSprite(DBloodActor* actor, int nStatus)
{
	assert(nStatus >= 0 && nStatus <= kStatFree);
	if (actor->spr.flags & 32)
	{
		for (auto& post : gPost)
			if (post.sprite == actor)
			{
				post.status = nStatus;
				return;
			}
	}
	else
	{
		actor->spr.flags |= 32;
		gPost.Push({ actor, nStatus });
	}
}

void actPostProcess(void)
{
	for (auto& p : gPost)
	{
		p.sprite->spr.flags &= ~32;
		int nStatus = p.status;
		if (nStatus == kStatFree)
		{
			if (p.sprite->spr.statnum != kStatFree)
			{
				evKillActor(p.sprite);
				if (p.sprite->hasX()) seqKill(p.sprite);
				DeleteSprite(p.sprite);
			}
		}
		else
			ChangeActorStat(p.sprite, nStatus);
	}
	gPost.Clear();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MakeSplash(DBloodActor* actor)
{
	actor->spr.flags &= ~2;
	actor->add_int_z(-(4 << 8));
	int nSurface = tileGetSurfType(actor->hit.florhit);
	switch (actor->spr.type)
	{
	case kThingDripWater:
		switch (nSurface)
		{
		case kSurfWater:
			seqSpawn(6, actor, -1);
			sfxPlay3DSound(actor, 356, -1, 0);
			break;
		default:
			seqSpawn(7, actor, -1);
			sfxPlay3DSound(actor, 354, -1, 0);
			break;
		}
		break;
	case kThingDripBlood:
		seqSpawn(8, actor, -1);
		sfxPlay3DSound(actor, 354, -1, 0);
		break;
	}
}

void actBurnSprite(DBloodActor* pSource, DBloodActor* pTarget, int nTime)
{
	pTarget->xspr.burnTime = ClipHigh(pTarget->xspr.burnTime + nTime, pTarget->spr.statnum == kStatDude ? 2400 : 1200);
	pTarget->SetBurnSource(pSource);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SPRITEHIT& w, SPRITEHIT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("hit", w.hit)
			("ceilhit", w.ceilhit)
			("florhit", w.florhit)
			.EndObject();
	}
	return arc;
}

void SerializeActor(FSerializer& arc)
{
	if (arc.BeginObject("actor"))
	{
		arc("maxdist20", gVectorData[kVectorTchernobogBurn].maxDist)    // The code messes around with this field so better save it.
			.EndObject();

		if (arc.isReading() && gGameOptions.nMonsterSettings != 0)
		{
			for (int i = 0; i < kDudeMax - kDudeBase; i++)
				for (int j = 0; j < 7; j++)
					dudeInfo[i].damageVal[j] = MulScale(DudeDifficulty[gGameOptions.nDifficulty], dudeInfo[i].startDamage[j], 8);
		}
	}
}

END_BLD_NS
