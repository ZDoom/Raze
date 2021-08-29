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
		384,
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
		256,
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
		256,
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
		256,
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
		256,
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
		256,
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
		256,
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
		256,
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
		257,
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
		257,
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
		257,
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
		256,
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
		256,
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
		256,
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
		257,
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
		256,
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
		256,
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
		257,
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

static const short gPlayerGibThingComments[] = {
	734, 735, 736, 737, 738, 739, 740, 741, 3038, 3049
};

const int DudeDifficulty[5] = {
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

bool IsUnderwaterSector(int nSector)
{
	int nXSector = sector[nSector].extra;
	if (nXSector > 0 && xsector[nXSector].Underwater)
		return 1;
	return 0;
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
		spritetype* pSprite = &act->s();
		if (pSprite->type == kTrapExploder)
		{
			pSprite->cstat &= ~1;
			pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
			if (!act->hasX()) continue;
			auto x = &act->x();
			x->waitTime = ClipLow(x->waitTime, 1);
			x->state = 0;
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
		spritetype* pSprite = &act->s();
		XSPRITE* pXSprite = &act->x();

		int nType = pSprite->type - kThingBase;
		pXSprite->health = thingInfo[nType].startHealth << 4;
#ifdef NOONE_EXTENSIONS
		// allow level designer to set custom clipdist.
		// this is especially useful for various Gib and Explode objects which have clipdist 1 for some reason predefined,
		// but what if it have voxel model...?
		if (!gModernMap)
#endif
			pSprite->clipdist = thingInfo[nType].clipdist;

		pSprite->flags = thingInfo[nType].flags;
		if (pSprite->flags & kPhysGravity) pSprite->flags |= kPhysFalling;
		act->xvel() = act->yvel() = act->zvel() = 0;

		switch (pSprite->type)
		{
		case kThingArmedProxBomb:
		case kTrapMachinegun:
#ifdef NOONE_EXTENSIONS
		case kModernThingTNTProx:
#endif
			pXSprite->state = 0;
			break;
		case kThingBloodChunks:
		{
			SEQINST* pInst = GetInstance(3, pSprite->extra);
			if (pInst)
			{
				auto seq = getSequence(pInst->nSeqID);
				if (!seq) break;
				seqSpawn(pInst->nSeqID, act);
			}
			break;
		}
		default:
			pXSprite->state = 1;
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
			spritetype* pSprite = &act->s();
			if (act->hasX() && act->x().key > 0) // Drop Key
				actDropObject(act, kItemKeyBase + (act->x().key - 1));
			DeleteSprite(act);
		}
	}
	else
	{
		// by NoOne: WTF is this?
		///////////////
		char unk[kDudeMax - kDudeBase];
		memset(unk, 0, sizeof(unk));
		BloodStatIterator it(kStatDude);
		while (auto act = it.Next())
		{
			spritetype* pSprite = &act->s();
			if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax)
				I_Error("Non-enemy sprite (%d) in the enemy sprite list.\n", pSprite->index);
			unk[pSprite->type - kDudeBase] = 1;
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
			spritetype* pSprite = &act->s();
			XSPRITE* pXSprite = &act->x();

			int nType = pSprite->type - kDudeBase;
			int seqStartId = dudeInfo[nType].seqStartID;
			if (!act->IsPlayerActor())
			{
#ifdef NOONE_EXTENSIONS
				switch (pSprite->type)
				{
				case kDudeModernCustom:
				case kDudeModernCustomBurning:
					pSprite->cstat |= 4096 + CSTAT_SPRITE_BLOCK_HITSCAN + CSTAT_SPRITE_BLOCK;
					seqStartId = genDudeSeqStartId(act); //  Custom Dude stores its SEQ in data2
					pXSprite->sysData1 = pXSprite->data3; // move sndStartId to sysData1, because data3 used by the game;
					pXSprite->data3 = 0;
					break;

				case kDudePodMother:  // FakeDude type (no seq, custom flags, clipdist and cstat)
					if (gModernMap) break;
					[[fallthrough]];
				default:
					pSprite->clipdist = dudeInfo[nType].clipdist;
					pSprite->cstat |= 4096 + CSTAT_SPRITE_BLOCK_HITSCAN + CSTAT_SPRITE_BLOCK;
					break;
				}
#else
				pSprite->clipdist = dudeInfo[nType].clipdist;
				pSprite->cstat |= 4096 + CSTAT_SPRITE_BLOCK_HITSCAN + CSTAT_SPRITE_BLOCK;
#endif

				act->xvel() = act->yvel() = act->zvel() = 0;

#ifdef NOONE_EXTENSIONS
				// add a way to set custom hp for every enemy - should work only if map just started and not loaded.
				if (!gModernMap || pXSprite->sysData2 <= 0) pXSprite->health = dudeInfo[nType].startHealth << 4;
				else pXSprite->health = ClipRange(pXSprite->sysData2 << 4, 1, 65535);
#else
				pXSprite->health = dudeInfo[nType].startHealth << 4;
#endif

			}

			if (getSequence(seqStartId)) seqSpawn(seqStartId, 3, pSprite->extra);
		}
		aiInit();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actInit(bool bSaveLoad)
{
#ifdef NOONE_EXTENSIONS
	if (!gModernMap) nnExtResetGlobals();
	else nnExtInitModernStuff(bSaveLoad);
#endif

	BloodStatIterator it(kStatItem);
	while (auto act = it.Next())
	{
		if (act->s().type == kItemWeaponVoodooDoll)
		{
			act->s().type = kItemAmmoVoodooDoll;
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
	auto pSprite = &actor->s();
	int dx = pSprite->x - x;
	int dy = pSprite->y - y;
	int dz = (pSprite->z - z) >> 4;
	int dist2 = 0x40000 + dx * dx + dy * dy + dz * dz;
	assert(dist2 > 0);
	damage = scale(0x40000, damage, dist2);

	if (pSprite->flags & kPhysMove)
	{
		int mass = 0;
		if (actor->IsDudeActor())
		{
			mass = getDudeInfo(pSprite->type)->mass;
#ifdef NOONE_EXTENSIONS
			if (pSprite->type == kDudeModernCustom || pSprite->type == kDudeModernCustomBurning)
			{
				mass = getSpriteMassBySize(actor);
			}
#endif
		}
		else if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
		{
			mass = thingInfo[pSprite->type - kThingBase].mass;
		}
		else
		{
			Printf(PRINT_HIGH, "Unexpected type in ConcussSprite(): Sprite: %d  Type: %d  Stat: %d", (int)pSprite->index, (int)pSprite->type, (int)pSprite->statnum);
			return;
		}

		if (mass > 0)
		{
			int size = (tileWidth(pSprite->picnum) * pSprite->xrepeat * tileHeight(pSprite->picnum) * pSprite->yrepeat) >> 1;
			int t = scale(damage, size, mass);
			int nSprite = pSprite->index;
			actor->xvel() += MulScale(t, dx, 16);
			actor->yvel() += MulScale(t, dy, 16);
			actor->zvel() += MulScale(t, dz, 16);
		}
	}
	actDamageSprite(source, actor, kDamageExplode, damage);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actWallBounceVector(int* x, int* y, int nWall, int a4)
{
	int wx, wy;
	GetWallNormal(nWall, &wx, &wy);
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

int actFloorBounceVector(int* x, int* y, int* z, int nSector, int a5)
{
	int t = 0x10000 - a5;
	if (sector[nSector].floorheinum == 0)
	{
		int t2 = MulScale(*z, t, 16);
		*z = -(*z - t2);
		return t2;
	}
	walltype* pWall = &wall[sector[nSector].wallptr];
	walltype* pWall2 = &wall[pWall->point2];
	int angle = getangle(pWall2->x - pWall->x, pWall2->y - pWall->y) + 512;
	int t2 = sector[nSector].floorheinum << 4;
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

void actRadiusDamage(DBloodActor* source, int x, int y, int z, int nSector, int nDist, int baseDmg, int distDmg, DAMAGE_TYPE dmgType, int flags, int burn)
{
	uint8_t sectmap[(kMaxSectors + 7) >> 3];
	auto pOwner = source->GetOwner();
	const bool newSectCheckMethod = !cl_bloodvanillaexplosions && pOwner && pOwner->IsDudeActor() && !VanillaMode(); // use new sector checking logic
	GetClosestSpriteSectors(nSector, x, y, nDist, sectmap, nullptr, newSectCheckMethod);
	nDist <<= 4;
	if (flags & 2)
	{
		BloodStatIterator it(kStatDude);
		while (auto act2 = it.Next())
		{
			if (act2 != source || (flags & 1))
			{
				auto pSprite2 = &act2->s();
				if (act2->hasX())
				{
					if (pSprite2->flags & 0x20) continue;
					if (!TestBitString(sectmap, pSprite2->sectnum)) continue;
					if (!CheckProximity(pSprite2, x, y, z, nSector, nDist)) continue;

					int dx = abs(x - pSprite2->x);
					int dy = abs(y - pSprite2->y);
					int dz = abs(z - pSprite2->z) >> 4;
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
			auto pSprite2 = &act2->s();

			if (pSprite2->flags & 0x20) continue;
			if (!TestBitString(sectmap, pSprite2->sectnum)) continue;
			if (!CheckProximity(pSprite2, x, y, z, nSector, nDist)) continue;

			XSPRITE* pXSprite2 = &act2->x();
			if (pXSprite2->locked) continue;

			int dx = abs(x - pSprite2->x);
			int dy = abs(y - pSprite2->y);
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
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	auto pOwner = actor->GetOwner();

	actPostSprite(actor, kStatDecoration);
	seqSpawn(9, actor);
	if (Chance(0x8000)) pSprite->cstat |= 4;

	sfxPlay3DSound(actor, 303, 24 + (pSprite->flags & 3), 1);
	actRadiusDamage(pOwner, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 128, 0, 60, kDamageExplode, 15, 120);

	if (pXSprite->data4 > 1)
	{
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		int spawnparam[2];
		spawnparam[0] = pXSprite->data4 >> 1;
		spawnparam[1] = pXSprite->data4 - spawnparam[0];
		int ang = pSprite->ang;
		actor->xvel() = 0;
		actor->yvel() = 0;
		actor->zvel() = 0;
		for (int i = 0; i < 2; i++)
		{
			int t1 = Random(0x33333) + 0x33333;
			int rndang = Random2(0x71);
			pSprite->ang = (rndang + ang + 2048) & 2047;
			auto spawned = actFireThing(actor, 0, 0, -0x93d0, kThingNapalmBall, t1);
			spawned->SetOwner(actor->GetOwner());
			seqSpawn(61, spawned, nNapalmClient);
			spawned->x().data4 = spawnparam[i];
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
	auto pSprite = &actor->s();
	int sector = pSprite->sectnum;
	int x = pSprite->x;
	int y = pSprite->y;
	updatesector(x, y, &sector);
	int zFloor = getflorzofslope(sector, x, y);
	auto spawned = actSpawnSprite(sector, x, y, zFloor, 3, 0);
	if (spawned) spawned->s().cstat &= ~257;
	return spawned;
}

static DBloodActor* actDropAmmo(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	auto pSprite = &actor->s();
	if (pSprite->statnum < kMaxStatus && nType >= kItemAmmoBase && nType < kItemAmmoMax)
	{
		auto act2 = actSpawnFloor(actor);
		const AMMOITEMDATA* pAmmo = &gAmmoItemData[nType - kItemAmmoBase];
		auto pSprite2 = &act2->s();
		pSprite2->type = nType;
		pSprite2->picnum = pAmmo->picnum;
		pSprite2->shade = pAmmo->shade;
		pSprite2->xrepeat = pAmmo->xrepeat;
		pSprite2->yrepeat = pAmmo->yrepeat;
		return act2;
	}
	return nullptr;
}

static DBloodActor* actDropWeapon(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	auto pSprite = &actor->s();
	if (pSprite->statnum < kMaxStatus && nType >= kItemWeaponBase && nType < kItemWeaponMax)
	{
		auto act2 = actSpawnFloor(actor);
		const WEAPONITEMDATA* pWeapon = &gWeaponItemData[nType - kItemWeaponBase];
		auto pSprite2 = &act2->s();
		pSprite2->type = nType;
		pSprite2->picnum = pWeapon->picnum;
		pSprite2->shade = pWeapon->shade;
		pSprite2->xrepeat = pWeapon->xrepeat;
		pSprite2->yrepeat = pWeapon->yrepeat;
		return act2;
	}
	return nullptr;
}

static DBloodActor* actDropItem(DBloodActor* actor, int nType)
{
	if (!actor) return nullptr;
	auto pSprite = &actor->s();
	if (pSprite->statnum < kMaxStatus && nType >= kItemBase && nType < kItemMax)
	{
		auto act2 = actSpawnFloor(actor);
		const ITEMDATA* pItem = &gItemData[nType - kItemBase];
		auto pSprite2 = &act2->s();
		pSprite2->type = nType;
		pSprite2->picnum = pItem->picnum;
		pSprite2->shade = pItem->shade;
		pSprite2->xrepeat = pItem->xrepeat;
		pSprite2->yrepeat = pItem->yrepeat;
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
	auto pSprite = &actor->s();
	if (pSprite->statnum < kMaxStatus && nType >= kItemKeyBase && nType < kItemKeyMax)
	{
		auto act2 = actDropItem(actor, nType);
		if (act2 && gGameOptions.nGameType == 1)
		{
			act2->addX();
			auto pSprite2 = &act2->s();
			act2->x().respawn = 3;
			act2->hit.florhit = 0;
			act2->hit.ceilhit = 0;
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
	auto pSprite = &actor->s();
	if (pSprite->statnum < kMaxStatus && (nType == 147 || nType == 148))
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
		if (bottom >= act2->s().z)
			act2->s().z -= bottom - act2->s().z;
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
	auto pXDude = &actor->x();
	add <<= 4;
	threshold <<= 4;
	if (pXDude->health < (unsigned)threshold)
	{
		spritetype* pSprite = &actor->s();
		if (actor->IsPlayerActor()) sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 780, pSprite->sectnum);
		pXDude->health = min<uint32_t>(pXDude->health + add, threshold);
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
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	removeDudeStuff(actor);
	if (pXSprite->txID <= 0 || getNextIncarnation(actor) == nullptr)
	{
		if (pExtra->weaponType == kGenDudeWeaponKamikaze && Chance(0x4000) && damageType != kDamageSpirit && damageType != kDamageDrown)
		{
			doExplosion(actor, pXSprite->data1 - kTrapExploder);
			if (Chance(0x9000)) damageType = kDamageExplode;
		}

		if (damageType == kDamageBurn)
		{
			if (pExtra->availDeaths[kDamageBurn] && !spriteIsUnderwater(actor))
			{
				if (pExtra->canBurn)
				{
					pSprite->type = kDudeModernCustomBurning;
					if (pXSprite->data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
						pSprite->pal = 0;

					aiGenDudeNewState(actor, &genDudeBurnGoto);
					actHealDude(actor, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
					if (pXSprite->burnTime <= 0) pXSprite->burnTime = 1200;
					actor->dudeExtra.time = PlayClock + 360;
					return true;
				}

			}
			else
			{
				pXSprite->burnTime = 0;
				actor->SetBurnSource(nullptr);
				damageType = kDamageFall;
			}
		}
	}
	else
	{
		pXSprite->locked = 1; // lock while transforming

		aiSetGenIdleState(actor); // set idle state

		if (pXSprite->key > 0) // drop keys
			actDropObject(actor, kItemKeyBase + pXSprite->key - 1);

		if (pXSprite->dropMsg > 0) // drop items
			actDropObject(actor, pXSprite->dropMsg);

		pSprite->flags &= ~kPhysMove;
		actor->xvel() = actor->yvel() = 0;

		playGenDudeSound(actor, kGenDudeSndTransforming);
		int seqId = pXSprite->data2 + kGenDudeSeqTransform;
		if (getSequence(seqId)) seqSpawn(seqId, actor, -1);
		else
		{
			seqKill(actor);
			DBloodActor* pEffectA = gFX.fxSpawnActor((FX_ID)52, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, pSprite->ang);
			if (pEffectA != nullptr)
			{
				auto pEffect = &pEffectA->s();
				pEffect->cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
				pEffect->pal = 6;
				pEffect->xrepeat = pSprite->xrepeat;
				pEffect->yrepeat = pSprite->yrepeat;
			}

			GIBTYPE nGibType;
			for (int i = 0; i < 3; i++)
			{
				if (Chance(0x3000)) nGibType = GIBTYPE_6;
				else if (Chance(0x2000)) nGibType = GIBTYPE_5;
				else nGibType = GIBTYPE_17;

				int top, bottom;
				GetActorExtents(actor, &top, &bottom);
				CGibPosition gibPos(pSprite->x, pSprite->y, top);
				CGibVelocity gibVel(actor->xvel() >> 1, actor->yvel() >> 1, -0xccccc);
				GibSprite(actor, nGibType, &gibPos, &gibVel);
			}
		}

		pXSprite->sysData1 = kGenDudeTransformStatus; // in transform
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
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	switch (pSprite->type)
	{
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
		if (actKillModernDude(actor, damageType)) return true;
		break;
#endif
	case kDudeCerberusTwoHead: // Cerberus
		seqSpawn(dudeInfo[pSprite->type - kDudeBase].seqStartID + 1, actor, -1);
		return true;

	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
		if (damageType == kDamageBurn && pXSprite->medium == kMediumNormal)
		{
			pSprite->type = kDudeBurningCultist;
			aiNewState(actor, &cultistBurnGoto);
			actHealDude(actor, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
			return true;
		}
		break;

	case kDudeBeast:
		if (damageType == kDamageBurn && pXSprite->medium == kMediumNormal)
		{
			pSprite->type = kDudeBurningBeast;
			aiNewState(actor, &beastBurnGoto);
			actHealDude(actor, dudeInfo[53].startHealth, dudeInfo[53].startHealth);
			return true;
		}
		break;

	case kDudeInnocent:
		if (damageType == kDamageBurn && pXSprite->medium == kMediumNormal)
		{
			pSprite->type = kDudeBurningInnocent;
			aiNewState(actor, &innocentBurnGoto);
			actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
			return true;
		}
		break;

	case kDudeTinyCaleb:
		if (cl_bloodvanillaenemies || VanillaMode())
			break;
		if (damageType == kDamageBurn && pXSprite->medium == kMediumNormal)
		{
			pSprite->type = kDudeBurningTinyCaleb;
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
	auto pKillerSprite = &killerActor->s();
	auto pSprite = &actor->s();
	if (VanillaMode())
	{
		if (killerActor->IsPlayerActor())
		{
			PLAYER* pPlayer = &gPlayer[pKillerSprite->type - kDudePlayer1];
			if (gGameOptions.nGameType == 1)
				pPlayer->fragCount++;
		}
	}
	else if (gGameOptions.nGameType == 1 && killerActor->IsPlayerActor() && pSprite->statnum == kStatDude)
	{
		switch (pSprite->type)
		{
		case kDudeBat:
		case kDudeRat:
		case kDudeInnocent:
		case kDudeBurningInnocent:
			break;
		default:
			PLAYER* pKillerPlayer = &gPlayer[pKillerSprite->type - kDudePlayer1];
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
	auto pXSprite = &actor->x();

	if (pXSprite->key > 0) actDropObject(actor, kItemKeyBase + pXSprite->key - 1);
	if (pXSprite->dropMsg > 0) actDropObject(actor, pXSprite->dropMsg);

	switch (actor->s().type)
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
	auto pSprite = &actor->s();

	switch (damageType)
	{
	case kDamageExplode:
		nSeq = 2;
		switch (pSprite->type)
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
		switch (pSprite->type) {
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
		switch (pSprite->type)
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
	CGibPosition gibPos(actor->s().x, actor->s().y, top);
	CGibVelocity gibVel(actor->xvel() >> 1, actor->yvel() >> 1, velz);
	GibSprite(actor, GIBTYPE_27, &gibPos, &gibVel);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void zombieAxeNormalDeath(DBloodActor* actor, int nSeq)
{
	auto pSprite = &actor->s();
	int nType = pSprite->type - kDudeBase;

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
		actor->x().data1 = 35;
		actor->x().data2 = 5;

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
	auto pSprite = &actor->s();
	if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(actor, 718, -1, 0);
	else sfxPlay3DSound(actor, 1018 + Random(2), -1, 0);

	int nType = pSprite->type - kDudeBase;
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
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();

	playGenDudeSound(actor, kGenDudeSndDeathNormal);
	int dudeToGib = (actCheckRespawn(actor)) ? -1 : ((nSeq == 3) ? nDudeToGibClient2 : nDudeToGibClient1);
	if (nSeq == 3)
	{
		GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
		if (pExtra->availDeaths[kDmgBurn] == 3) seqSpawn((15 + Random(2)) + pXSprite->data2, actor, dudeToGib);
		else if (pExtra->availDeaths[kDmgBurn] == 2) seqSpawn(16 + pXSprite->data2, actor, dudeToGib);
		else if (pExtra->availDeaths[kDmgBurn] == 1) seqSpawn(15 + pXSprite->data2, actor, dudeToGib);
		else if (getSequence(pXSprite->data2 + nSeq))seqSpawn(nSeq + pXSprite->data2, actor, dudeToGib);
		else seqSpawn(1 + pXSprite->data2, actor, dudeToGib);

	}
	else
	{
		seqSpawn(nSeq + pXSprite->data2, actor, dudeToGib);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void modernCustomDudeBurningDeath(DBloodActor* actor, int nSeq)
{
	auto pSprite = &actor->s();

	playGenDudeSound(actor, kGenDudeSndDeathExplode);
	int dudeToGib = (actCheckRespawn(actor)) ? -1 : nDudeToGibClient1;

	if (Chance(0x4000)) spawnGibs(actor, GIBTYPE_27, -0xccccc);

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	int seqofs = actor->x().data2;
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
	auto pSprite = &actor->s();
	int nType = pSprite->type - kDudeBase;

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
	auto pSprite = &actor->s();
	int nType = pSprite->type - kDudeBase;

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
	auto pSprite = &actor->s();
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
	spritetype* pKillerSprite = &killerActor->s();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax&& actor->hasX());
	int nType = pSprite->type - kDudeBase;
	XSPRITE* pXSprite = &actor->x();

	if (actKillDudeStage1(actor, damageType)) return;

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (gPlayer[p].fragger() == actor && gPlayer[p].deathTime > 0)
			gPlayer[p].setFragger(nullptr);
	}
	if (pSprite->type != kDudeCultistBeast)
		trTriggerSprite(actor, kCmdOff);

	pSprite->flags |= 7;
	checkAddFrag(killerActor, actor);
	checkDropObjects(actor);

	int nSeq = checkDamageType(actor, damageType);

	if (!getSequence(getDudeInfo(nType + kDudeBase)->seqStartID + nSeq))
	{
		seqKill(actor);
		gKillMgr.AddKill(pSprite);
		actPostSprite(actor, kStatFree);
		return;
	}

	auto Owner = actor->GetOwner();
	switch (pSprite->type)
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
		if ((pSprite->cstat & CSTAT_SPRITE_YFLIP)) pSprite->cstat &= ~CSTAT_SPRITE_YFLIP;
		switch (pSprite->type)
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
		DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
		for (int i = 0; i < 3; i++)
			if (pDudeInfo->nGibType[i] > -1)
				GibSprite(actor, (GIBTYPE)pDudeInfo->nGibType[i], nullptr, nullptr);
		for (int i = 0; i < 4; i++)
			fxSpawnBlood(actor, damage);
	}
	gKillMgr.AddKill(pSprite);
	actCheckRespawn(actor);
	pSprite->type = kThingBloodChunks;
	actPostSprite(actor, kStatThing);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int actDamageDude(DBloodActor* source, DBloodActor* actor, int damage, DAMAGE_TYPE damageType)
{
	auto pSprite = &actor->s();
	XSPRITE* pXSprite = &actor->x();

	if (!actor->IsDudeActor())
	{
		Printf(PRINT_HIGH, "Bad Dude Failed: initial=%d type=%d %s\n", (int)pSprite->inittype, (int)pSprite->type, (int)(pSprite->flags & kHitagRespawn) ? "RESPAWN" : "NORMAL");
		return damage >> 4;
		//I_Error("Bad Dude Failed: initial=%d type=%d %s\n", (int)pSprite->inittype, (int)pSprite->type, (int)(pSprite->flags & 16) ? "RESPAWN" : "NORMAL");
	}

	int nType = pSprite->type - kDudeBase;
	int nDamageFactor = getDudeInfo(nType + kDudeBase)->damageVal[damageType];
#ifdef NOONE_EXTENSIONS
	if (pSprite->type == kDudeModernCustom)
		nDamageFactor = actor->genDudeExtra.dmgControl[damageType];
#endif

	if (!nDamageFactor) return 0;
	else if (nDamageFactor != 256) damage = MulScale(damage, nDamageFactor, 8);

	if (!actor->IsPlayerActor())
	{
		if (pXSprite->health <= 0) return 0;
		damage = aiDamageSprite(source, actor, damageType, damage);
		if (pXSprite->health <= 0)
			actKillDude(source, actor, ((damageType == kDamageExplode && damage < 160) ? kDamageFall : damageType), damage);
	}
	else
	{
		PLAYER* pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
		if (pXSprite->health > 0 || playerSeqPlaying(pPlayer, 16))
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
	auto pSprite = &actor->s();
	XSPRITE* pXSprite = &actor->x();

	assert(pSprite->type >= kThingBase && pSprite->type < kThingMax);
	int nType = pSprite->type - kThingBase;
	int nDamageFactor = thingInfo[nType].dmgControl[damageType];

	if (!nDamageFactor) return 0;
	else if (nDamageFactor != 256) damage = MulScale(damage, nDamageFactor, 8);

	pXSprite->health = ClipLow(pXSprite->health - damage, 0);
	if (pXSprite->health <= 0)
	{
		auto Owner = actor->GetOwner();
		switch (pSprite->type)
		{
		case kThingDroppedLifeLeech:
#ifdef NOONE_EXTENSIONS
		case kModernThingEnemyLifeLeech:
#endif
			GibSprite(actor, GIBTYPE_14, nullptr, nullptr);
			pXSprite->data1 = pXSprite->data2 = pXSprite->data3 = pXSprite->DudeLockout = 0;
			pXSprite->stateTimer = pXSprite->data4 = pXSprite->isTriggered = 0;

#ifdef NOONE_EXTENSIONS
			if (Owner && Owner->s().type == kDudeModernCustom)
				Owner->SetSpecialOwner(); // indicates if custom dude had life leech.
#endif
			break;

		default:
			if (!(pSprite->flags & kHitagRespawn))
				actor->SetOwner(source);
			break;
		}

		trTriggerSprite(actor, kCmdOff);

		switch (pSprite->type)
		{
		case kThingObjectGib:
		case kThingObjectExplode:
		case kThingBloodBits:
		case kThingBloodChunks:
		case kThingZombieHead:
			if (damageType == 3 && pSourcePlayer && PlayClock > pSourcePlayer->laughCount && Chance(0x4000))
			{
				sfxPlay3DSound(pSourcePlayer->pSprite, gPlayerGibThingComments[Random(10)], 0, 2);
				pSourcePlayer->laughCount = PlayClock + 3600;
			}
			break;
		case kTrapMachinegun:
			seqSpawn(28, 3, pSprite->extra, -1);
			break;

		case kThingFluorescent:
			seqSpawn(12, 3, pSprite->extra, -1);
			GibSprite(actor, GIBTYPE_6, nullptr, nullptr);
			break;

		case kThingSpiderWeb:
			seqSpawn(15, 3, pSprite->extra, -1);
			break;

		case kThingMetalGrate:
			seqSpawn(21, 3, pSprite->extra, -1);
			GibSprite(actor, GIBTYPE_4, nullptr, nullptr);
			break;

		case kThingFlammableTree:
			switch (pXSprite->data1)
			{
			case -1:
				GibSprite(actor, GIBTYPE_14, nullptr, nullptr);
				sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 312, pSprite->sectnum);
				actPostSprite(actor, kStatFree);
				break;

			case 0:
				seqSpawn(25, actor, nTreeToGibClient);
				sfxPlay3DSound(actor, 351, -1, 0);
				break;

			case 1:
				seqSpawn(26, 3, pSprite->extra, nTreeToGibClient);
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
	auto pSprite = &actor->s();

	if (pSprite->flags & 32 || !actor->hasX())
		return 0;

	XSPRITE* pXSprite = &actor->x();
	if ((pXSprite->health == 0 && pSprite->statnum != kStatDude) || pXSprite->locked)
		return 0;

	if (source == nullptr) source = actor;

	PLAYER* pSourcePlayer = nullptr;
	if (source->IsPlayerActor()) pSourcePlayer = &gPlayer[source->s().type - kDudePlayer1];
	if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pSourcePlayer, pSprite)) return 0;

	switch (pSprite->statnum)
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

void actHitcodeToData(int a1, HITINFO* pHitInfo, DBloodActor** pActor, walltype** ppWall)
{
	assert(pHitInfo != nullptr);
	DBloodActor* actor = nullptr;
	int nWall = -1;
	walltype* pWall = nullptr;
	switch (a1)
	{
	case 3:
	case 5:
		actor = pHitInfo->hitactor;
		break;
	case 0:
	case 4:
		nWall = pHitInfo->hitwall;
		if (nWall >= 0 && nWall < kMaxWalls) pWall = &wall[nWall];
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
	auto pMissile = &missileActor->s();
	XSPRITE* pXMissile = &missileActor->x();
	auto missileOwner = missileActor->GetOwner();

	DBloodActor* actorHit = nullptr;
	walltype* pWallHit = nullptr;

	actHitcodeToData(hitCode, &gHitInfo, &actorHit, &pWallHit);
	spritetype* pSpriteHit = actorHit ? &actorHit->s() : nullptr;
	XSPRITE* pXSpriteHit = actorHit && actorHit->hasX() ? &actorHit->x() : nullptr;

	const THINGINFO* pThingInfo = nullptr;
	DUDEINFO* pDudeInfo = nullptr;

	if (hitCode == 3 && pSpriteHit)
	{
		switch (pSpriteHit->statnum)
		{
		case kStatThing:
			pThingInfo = &thingInfo[pSpriteHit->type - kThingBase];
			break;
		case kStatDude:
			pDudeInfo = getDudeInfo(pSpriteHit->type);
			break;
		}
	}
	switch (pMissile->type)
	{
	case kMissileLifeLeechRegular:
		if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
		{
			DAMAGE_TYPE rand1 = (DAMAGE_TYPE)Random(7);
			int rand2 = (7 + Random(7)) << 4;
			int nDamage = actDamageSprite(missileOwner, actorHit, rand1, rand2);

			if ((pThingInfo && pThingInfo->dmgControl[kDamageBurn] != 0) || (pDudeInfo && pDudeInfo->damageVal[kDamageBurn] != 0))
				actBurnSprite(missileActor->GetOwner(), actorHit, 360);

			// by NoOne: make Life Leech heal user, just like it was in 1.0x versions
			if (gGameOptions.weaponsV10x && !VanillaMode() && pDudeInfo != nullptr)
			{
				if (missileOwner->IsDudeActor() && missileOwner->hasX() && missileOwner->x().health != 0)
					actHealDude(missileOwner, nDamage >> 2, getDudeInfo(missileOwner->s().type)->startHealth);
			}
		}

		if (pMissile->extra > 0)
		{
			actPostSprite(missileActor, kStatDecoration);
			if (pMissile->ang == 1024) sfxPlay3DSound(pMissile, 307, -1, 0);
			pMissile->type = kSpriteDecoration;
			seqSpawn(9, missileActor, -1);
		}
		else
		{
			actPostSprite(missileActor, kStatFree);
		}

		break;
	case kMissileTeslaAlt:
		teslaHit(pMissile, hitCode);
		switch (hitCode)
		{
		case 0:
		case 4:
			if (pWallHit)
			{
				auto pFX = gFX.fxSpawnActor(FX_52, pMissile->sectnum, pMissile->x, pMissile->y, pMissile->z, 0);
				if (pFX) pFX->s().ang = (GetWallAngle(pWallHit) + 512) & 2047;
			}
			break;
		}
		GibSprite(missileActor, GIBTYPE_24, NULL, NULL);
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissilePukeGreen:
		seqKill(missileActor);
		if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
		{
			int nDamage = (15 + Random(7)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileArcGargoyle:
		sfxKill3DSound(pMissile, -1, -1);
		sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 306, pMissile->sectnum);
		GibSprite(missileActor, GIBTYPE_6, NULL, NULL);

		if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
		{
			int nDamage = (25 + Random(20)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileLifeLeechAltNormal:
	case kMissileLifeLeechAltSmall:
		sfxKill3DSound(pMissile, -1, -1);
		sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 306, pMissile->sectnum);

		if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
		{
			int nDmgMul = (pMissile->type == kMissileLifeLeechAltSmall) ? 6 : 3;
			int nDamage = (nDmgMul + Random(nDmgMul)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
		}
		actPostSprite(missileActor, kStatFree);
		break;

	case kMissileFireball:
	case kMissileFireballNapalm:
		if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
		{
			if (pThingInfo && pSpriteHit->type == kThingTNTBarrel && actorHit->x().burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			int nDamage = (50 + Random(50)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actExplodeSprite(missileActor);
		break;

	case kMissileFlareAlt:
		sfxKill3DSound(pMissile, -1, -1);
		actExplodeSprite(missileActor);
		break;

	case kMissileFlareRegular:
		sfxKill3DSound(pMissile, -1, -1);
		if ((hitCode == 3 && pSpriteHit) && (pThingInfo || pDudeInfo))
		{
			if ((pThingInfo && pThingInfo->dmgControl[kDamageBurn] != 0) || (pDudeInfo && pDudeInfo->damageVal[kDamageBurn] != 0))
			{
				if (pThingInfo && pSpriteHit->type == kThingTNTBarrel && actorHit->x().burnTime == 0)
					evPostActor(actorHit, 0, kCallbackFXFlameLick);

				actBurnSprite(missileOwner, actorHit, 480);
				actRadiusDamage(missileOwner, pMissile->x, pMissile->y, pMissile->z, pMissile->sectnum, 16, 20, 10, kDamageBullet, 6, 480);

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

			if (surfType[pSpriteHit->picnum] == kSurfFlesh)
			{
				pMissile->picnum = 2123;
				missileActor->SetTarget(actorHit);
				pXMissile->targetZ = pMissile->z - pSpriteHit->z;
				pXMissile->goalAng = getangle(pMissile->x - pSpriteHit->x, pMissile->y - pSpriteHit->y) - pSpriteHit->ang;
				pXMissile->state = 1;
				actPostSprite(missileActor, kStatFlare);
				pMissile->cstat &= ~257;
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
			if ((pSpriteHit->statnum == kStatThing || pSpriteHit->statnum == kStatDude) && pXSpriteHit->burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			actBurnSprite(missileOwner, actorHit, (4 + gGameOptions.nDifficulty) << 2);
			actDamageSprite(missileOwner, actorHit, kDamageBurn, 8);
		}
		break;

	case kMissileFireballCerberus:
		actExplodeSprite(missileActor);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if ((pSpriteHit->statnum == kStatThing || pSpriteHit->statnum == kStatDude) && pXSpriteHit->burnTime == 0)
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
			if ((pSpriteHit->statnum == kStatThing || pSpriteHit->statnum == kStatDude) && pXSpriteHit->burnTime == 0)
				evPostActor(actorHit, 0, kCallbackFXFlameLick);

			actBurnSprite(missileOwner, actorHit, 32);
			actDamageSprite(missileOwner, actorHit, kDamageSpirit, 12);
			int nDamage = (25 + Random(10)) << 4;
			actDamageSprite(missileOwner, actorHit, kDamageBullet, nDamage);
		}
		actExplodeSprite(missileActor);
		break;

	case kMissileEctoSkull:
		sfxKill3DSound(pMissile, -1, -1);
		sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 522, pMissile->sectnum);
		actPostSprite(missileActor, kStatDebris);
		seqSpawn(20, 3, pMissile->extra, -1);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if (pSpriteHit->statnum == kStatDude)
			{
				int nDamage = (25 + Random(10)) << 4;
				actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
			}
		}
		break;

	case kMissileButcherKnife:
		actPostSprite(missileActor, kStatDebris);
		pMissile->cstat &= ~16;
		pMissile->type = kSpriteDecoration;
		seqSpawn(20, 3, pMissile->extra, -1);
		if (hitCode == 3 && actorHit && actorHit->hasX())
		{
			if (pSpriteHit->statnum == kStatDude)
			{
				int nDamage = (10 + Random(10)) << 4;
				actDamageSprite(missileOwner, actorHit, kDamageSpirit, nDamage);
				int nType = missileOwner->s().type - kDudeBase;
				if (missileOwner->x().health > 0)
					actHealDude(missileOwner, 10, getDudeInfo(nType + kDudeBase)->startHealth);
			}
		}
		break;

	case kMissileTeslaRegular:
		sfxKill3DSound(pMissile, -1, -1);
		sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 518, pMissile->sectnum);
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
	if (gModernMap && pXSpriteHit && pXSpriteHit->state != pXSpriteHit->restState && pXSpriteHit->Impact)
		trTriggerSprite(actorHit, kCmdSpriteImpact);
#endif
	pMissile->cstat &= ~257;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actKickObject(DBloodActor* kicker, DBloodActor* kicked)
{
	int nSpeed = ClipLow(approxDist(kicker->xvel(), kicker->yvel()) * 2, 0xaaaaa);
	kicked->xvel() = MulScale(nSpeed, Cos(kicker->s().ang + Random2(85)), 30);
	kicked->yvel() = MulScale(nSpeed, Sin(kicker->s().ang + Random2(85)), 30);
	kicked->zvel() = MulScale(nSpeed, -0x2000, 14);
	kicked->s().flags = 7;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void actTouchFloor(DBloodActor* actor, int nSector)
{
	assert(actor != nullptr);
	assert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSector = &sector[nSector];
	XSECTOR* pXSector = nullptr;
	if (pSector->extra > 0) pXSector = &xsector[pSector->extra];

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

		actDamageSprite(actor, actor, nDamageType, scale(4, nDamage, 120) << 4);
	}
	if (tileGetSurfType(sector[nSector].floorpicnum) == kSurfLava)
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
	auto pSprite = &actor->s();
	auto pXSprite = actor->hasX() ? &actor->x() : nullptr;

	Collision coll(actor->hit.ceilhit);
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSprite:
		if (coll.actor->hasX())
		{
			auto actor2 = coll.actor;
			spritetype* pSprite2 = &actor2->s();
			XSPRITE* pXSprite2 = &actor2->x();
			if ((pSprite2->statnum == kStatThing || pSprite2->statnum == kStatDude) && (actor->xvel() != 0 || actor->yvel() != 0 || actor->zvel() != 0))
			{
				if (pSprite2->statnum == kStatThing)
				{
					int nType = pSprite2->type - kThingBase;
					const THINGINFO* pThingInfo = &thingInfo[nType];
					if (pThingInfo->flags & 1) pSprite2->flags |= 1;
					if (pThingInfo->flags & 2) pSprite2->flags |= 4;
					// Inlined ?
					actor2->xvel() += MulScale(4, pSprite2->x - pSprite->x, 2);
					actor2->yvel() += MulScale(4, pSprite2->y - pSprite->y, 2);
				}
				else
				{
					pSprite2->flags |= 5;
					actor2->xvel() += MulScale(4, pSprite2->x - pSprite->x, 2);
					actor2->yvel() += MulScale(4, pSprite2->y - pSprite->y, 2);

#ifdef NOONE_EXTENSIONS
					// add size shroom abilities
					if ((actor->IsPlayerActor() && isShrinked(actor)) || (actor2->IsPlayerActor() && isGrown(actor2))) {

						int mass1 = getDudeInfo(pSprite2->type)->mass;
						int mass2 = getDudeInfo(pSprite->type)->mass;
						switch (pSprite->type)
						{
						case kDudeModernCustom:
						case kDudeModernCustomBurning:
							mass2 = getSpriteMassBySize(actor);
							break;
						}
						if (mass1 > mass2)
						{
							int dmg = abs((mass1 - mass2) * (pSprite2->clipdist - pSprite->clipdist));
							if (actor2->IsDudeActor())
							{
								if (dmg > 0) actDamageSprite(actor2, actor, (Chance(0x2000)) ? kDamageFall : (Chance(0x4000)) ? kDamageExplode : kDamageBullet, dmg);
								if (Chance(0x0200)) actKickObject(actor2, actor);
							}
						}
					}
#endif
					if (!actor->IsPlayerActor() || gPlayer[pSprite->type - kDudePlayer1].godMode == 0)
					{
						switch (pSprite2->type)
						{
						case kDudeTchernobog:
							actDamageSprite(actor2, actor, kDamageExplode, pXSprite->health << 2);
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
								if (pXSprite && !actor->isActive()) aiActivateDude(actor);
							}
							else if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpJumpBoots) > 0) actDamageSprite(actor2, actor, kDamageExplode, dmg);
							else actDamageSprite(actor2, actor, kDamageFall, dmg);
							break;
#endif

						}

					}
				}
			}

			if (pSprite2->type == kTrapSawCircular)
			{
				if (!pXSprite2->state) actDamageSprite(actor, actor, kDamageBullet, 1);
				else {
					pXSprite2->data1 = 1;
					pXSprite2->data2 = ClipHigh(pXSprite2->data2 + 8, 600);
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
	auto pSprite = &actor->s();
	auto pXSprite = actor->hasX() ? &actor->x() : nullptr;

	Collision coll(actor->hit.hit);
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSprite:
		if (coll.actor->hasX())
		{
			auto actor2 = coll.actor;
			spritetype* pSprite2 = &actor2->s();

#ifdef NOONE_EXTENSIONS
			// add size shroom abilities
			if ((actor2->IsPlayerActor() && isShrinked(actor2)) || (actor->IsPlayerActor() && isGrown(actor)))
			{
				if (actor->xvel() != 0 && actor2->IsDudeActor())
				{
					int mass1 = getDudeInfo(pSprite->type)->mass;
					int mass2 = getDudeInfo(pSprite2->type)->mass;
					switch (pSprite2->type)
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
						int dmg = (mass1 - mass2) + abs(FixedToInt(actor->xvel()));
						if (dmg > 0) actDamageSprite(actor, actor2, (Chance(0x2000)) ? kDamageFall : kDamageBullet, dmg);
					}
				}
			}
#endif

			switch (pSprite2->type)
			{
			case kThingKickablePail:
				actKickObject(actor, actor2);
				break;

			case kThingZombieHead:
				sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
				actKickObject(actor, actor2);
				actDamageSprite(nullptr, actor2, kDamageFall, 80);
				break;

			case kDudeBurningInnocent:
			case kDudeBurningCultist:
			case kDudeBurningZombieAxe:
			case kDudeBurningZombieButcher:
				// This does not make sense
				pXSprite->burnTime = ClipLow(pXSprite->burnTime - 4, 0);
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
	auto pSprite = &actor->s();
	auto pXSprite = actor->hasX() ? &actor->x() : nullptr;

	Collision coll(actor->hit.florhit);
	switch (coll.type)
	{
	case kHitWall:
		break;
	case kHitSector:
		actTouchFloor(actor, coll.index);
		break;
	case kHitSprite:
		if (coll.actor->hasX())
		{
			auto actor2 = coll.actor;
			spritetype* pSprite2 = &actor2->s();
			XSPRITE* pXSprite2 = &actor2->x();

#ifdef NOONE_EXTENSIONS
			// add size shroom abilities
			if ((actor2->IsPlayerActor() && isShrinked(actor2)) || (actor->IsPlayerActor() && isGrown(actor)))
			{

				int mass1 = getDudeInfo(pSprite->type)->mass;
				int mass2 = getDudeInfo(pSprite2->type)->mass;
				switch (pSprite2->type)
				{
				case kDudeModernCustom:
				case kDudeModernCustomBurning:
					mass2 = getSpriteMassBySize(actor2);
					break;
				}

				if (mass1 > mass2 && actor2->IsDudeActor())
				{
					if ((IsPlayerSprite(pSprite2) && Chance(0x500)) || !IsPlayerSprite(pSprite2))
						actKickObject(actor, actor2);

					int dmg = (mass1 - mass2) + pSprite->clipdist;
					if (dmg > 0) actDamageSprite(actor, actor2, (Chance(0x2000)) ? kDamageFall : kDamageBullet, dmg);
				}
			}
#endif

			PLAYER* pPlayer = nullptr;
			if (actor->IsPlayerActor()) pPlayer = &gPlayer[pSprite->type - kDudePlayer1];

			switch (pSprite2->type)
			{
			case kThingKickablePail:
				if (pPlayer)
				{
					if (pPlayer->kickPower > PlayClock) return;
					pPlayer->kickPower = PlayClock + 60;
				}
				actKickObject(actor, actor2);
				sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
				sfxPlay3DSound(actor, 374, 0, 0);
				break;
			case kThingZombieHead:
				if (pPlayer)
				{
					if (pPlayer->kickPower > PlayClock) return;
					pPlayer->kickPower = PlayClock + 60;
				}
				actKickObject(actor, actor2);
				sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
				actDamageSprite(nullptr, actor2, kDamageFall, 80);
				break;
			case kTrapSawCircular:
				if (!pXSprite2->state) actDamageSprite(actor, actor, kDamageBullet, 1);
				else
				{
					pXSprite2->data1 = 1;
					pXSprite2->data2 = ClipHigh(pXSprite2->data2 + 8, 600);
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
		for (Collision* coll : { &actor->hit.hit, &actor->hit.florhit, &actor->hit.ceilhit})
		{
			if (coll->type == kHitSprite)
			{
				actor2 = coll->actor;
				break;
			}
		}

		if (actor2 && actor2->hasX())
		{
			XSPRITE* pXHSprite = &actor2->x();
			if (pXHSprite->Touch && !pXHSprite->isTriggered && (!pXHSprite->DudeLockout || actor->IsPlayerActor()))
				trTriggerSprite(actor2, kCmdSpriteTouch);
		}

		// Touch walls
		Collision coll = actor->hit.hit;
		int nHWall = -1;
		if (coll.type == kHitWall)
		{
			nHWall = coll.index;
			if (wallRangeIsFine(nHWall) && xwallRangeIsFine(wall[nHWall].extra))
			{
				XWALL* pXHWall = &xwall[wall[nHWall].extra];
				if (pXHWall->triggerTouch && !pXHWall->isTriggered && (!pXHWall->dudeLockout || actor->IsPlayerActor()))
					trTriggerWall(nHWall, pXHWall, kCmdWallTouch);
			}
		}

		// enough to reset SpriteHit values
		if (nHWall != -1 || actor2) actor->xvel() += 5;

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
	auto pSprite = &actor->s();

	int wind_x = 0;
	int wind_y = 0;
	int nSector = pSprite->sectnum;
	assert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSector = &sector[nSector];
	int nXSector = pSector->extra;
	if (nXSector > 0)
	{
		assert(nXSector < kMaxXSectors);
		XSECTOR* pXSector = &xsector[nXSector];
		if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
		{
			int wind = pXSector->windVel << 12;
			if (!pXSector->windAlways && pXSector->busy) wind = MulScale(wind, pXSector->busy, 16);
			wind_x = MulScale(wind, Cos(pXSector->windAng), 30);
			wind_y = MulScale(wind, Sin(pXSector->windAng), 30);
		}
	}
	actor->xvel() += MulScale(wind_x - actor->xvel(), a2, 16);
	actor->yvel() += MulScale(wind_y - actor->yvel(), a2, 16);
	actor->zvel() -= MulScale(actor->zvel(), a2, 16);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static Collision MoveThing(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	assert(actor->hasX());
	XSPRITE* pXSprite = &actor->x();
	assert(pSprite->type >= kThingBase && pSprite->type < kThingMax);
	const THINGINFO* pThingInfo = &thingInfo[pSprite->type - kThingBase];
	int nSector = pSprite->sectnum;
	assert(nSector >= 0 && nSector < kMaxSectors);
	int top, bottom;
	Collision lhit;

	lhit.setNone();
	GetActorExtents(actor, &top, &bottom);
	const int bakCompat = enginecompatibility_mode;
	if (actor->xvel() || actor->yvel())
	{
		short bakCstat = pSprite->cstat;
		pSprite->cstat &= ~257;
		if ((actor->GetOwner()) && !cl_bloodvanillaexplosions && !VanillaMode())
			enginecompatibility_mode = ENGINECOMPATIBILITY_NONE; // improved clipmove accuracy
		lhit = actor->hit.hit = ClipMove(&pSprite->pos, &nSector, actor->xvel() >> 12, actor->yvel() >> 12, pSprite->clipdist << 2, (pSprite->z - top) / 4, (bottom - pSprite->z) / 4, CLIPMASK0);
		enginecompatibility_mode = bakCompat; // restore
		pSprite->cstat = bakCstat;
		assert(nSector >= 0);
		if (pSprite->sectnum != nSector)
		{
			assert(nSector >= 0 && nSector < kMaxSectors);
			ChangeActorSect(actor, nSector);
		}

		Collision &coll = actor->hit.hit;
		if (coll.type == kHitWall)
		{
			int nHitWall = coll.index;
			actWallBounceVector(&actor->xvel(), &actor->yvel(), nHitWall, pThingInfo->elastic);
			switch (pSprite->type)
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
		assert(nSector >= 0 && nSector < kMaxSectors);
		FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
	}

	pSprite->z += actor->zvel() >> 8;

	int ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist << 2, CLIPMASK0);
	GetActorExtents(actor, &top, &bottom);

	if ((pSprite->flags & 2) && bottom < floorZ)
	{
		pSprite->z += 455;
		actor->zvel() += 58254;
		if (pSprite->type == kThingZombieHead)
		{
			auto* fxActor = gFX.fxSpawnActor(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
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
				fxActor->xvel() = actor->xvel() + v2c;
				fxActor->yvel() = actor->yvel() + v28;
				fxActor->zvel() = actor->zvel() + v24;
			}
		}
	}
	if (CheckLink(actor)) GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist << 2, CLIPMASK0);

	GetActorExtents(actor, &top, &bottom);
	if (bottom >= floorZ)
	{
		actTouchFloor(actor, pSprite->sectnum);
		actor->hit.florhit = floorColl;
		pSprite->z += floorZ - bottom;

		int v20 = actor->zvel() - velFloor[pSprite->sectnum];
		if (v20 > 0)
		{

			pSprite->flags |= 4;
			int vax = actFloorBounceVector(&actor->xvel(), &actor->yvel(), (int*)&v20, pSprite->sectnum, pThingInfo->elastic);
			int nDamage = MulScale(vax, vax, 30) - pThingInfo->dmgResist;
			if (nDamage > 0) actDamageSprite(actor, actor, kDamageFall, nDamage);

			actor->zvel() = v20;
			if (velFloor[pSprite->sectnum] == 0 && abs(actor->zvel()) < 0x10000)
			{
				actor->zvel() = 0;
				pSprite->flags &= ~4;
			}

			switch (pSprite->type)
			{
			case kThingNapalmBall:
				if (actor->zvel() == 0 || Chance(0xA000)) actNapalmMove(actor);
				break;

			case kThingZombieHead:
				if (abs(actor->zvel()) > 0x80000)
				{
					sfxPlay3DSound(actor, 607, 0, 0);
					actDamageSprite(nullptr, actor, kDamageFall, 80);
				}
				break;

			case kThingKickablePail:
				if (abs(actor->zvel()) > 0x80000)
					sfxPlay3DSound(actor, 374, 0, 0);
				break;
			}

			lhit = kHitSector | nSector;
		}
		else if (actor->zvel() == 0)

			pSprite->flags &= ~4;
	}
	else
	{
		actor->hit.florhit = 0;

		if (pSprite->flags & 2)
			pSprite->flags |= 4;
	}

	if (top <= ceilZ)
	{
		actor->hit.ceilhit = ceilColl;
		pSprite->z += ClipLow(ceilZ - top, 0);
		if (actor->zvel() < 0)
		{
			actor->xvel() = MulScale(actor->xvel(), 0xc000, 16);
			actor->yvel() = MulScale(actor->yvel(), 0xc000, 16);
			actor->zvel() = MulScale(-actor->zvel(), 0x4000, 16);

			switch (pSprite->type)
			{
			case kThingZombieHead:
				if (abs(actor->zvel()) > 0x80000)
				{
					sfxPlay3DSound(actor, 607, 0, 0);
					actDamageSprite(nullptr, actor, kDamageFall, 80);
				}
				break;

			case kThingKickablePail:
				if (abs(actor->zvel()) > 0x80000)
					sfxPlay3DSound(actor, 374, 0, 0);
				break;
			}
		}
	}
	else actor->hit.ceilhit = 0;

	if (bottom >= floorZ)
	{
		int nVel = approxDist(actor->xvel(), actor->yvel());
		int nVelClipped = ClipHigh(nVel, 0x11111);
		Collision& coll = floorColl;

		if (coll.type == kHitSprite)
		{
			auto hitActor = coll.actor;
			auto hitSpr = &hitActor->s();
			if ((hitSpr->cstat & 0x30) == 0)
			{
				actor->xvel() += MulScale(4, pSprite->x - hitSpr->x, 2);
				actor->yvel() += MulScale(4, pSprite->y - hitSpr->y, 2);
				lhit = actor->hit.hit;
			}
		}
		if (nVel > 0)
		{
			int t = DivScale(nVelClipped, nVel, 16);
			actor->xvel() -= MulScale(t, actor->xvel(), 16);
			actor->yvel() -= MulScale(t, actor->yvel(), 16);
		}
	}
	if (actor->xvel() || actor->yvel())
		pSprite->ang = getangle(actor->xvel(), actor->yvel());
	return lhit;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveDude(DBloodActor* actor)
{
	auto const pSprite = &actor->s();
	auto const pXSprite = &actor->x();
	PLAYER* pPlayer = nullptr;
	if (actor->IsPlayerActor()) pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
	{
		Printf(PRINT_HIGH, "%d: pSprite->type >= kDudeBase && pSprite->type < kDudeMax", pSprite->type);
		return;
	}

	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	int bz = (bottom - pSprite->z) / 4;
	int tz = (pSprite->z - top) / 4;
	int wd = pSprite->clipdist << 2;
	int nSector = pSprite->sectnum;
	int nAiStateType = (pXSprite->aiState) ? pXSprite->aiState->stateType : -1;

	assert(nSector >= 0 && nSector < kMaxSectors);

	if (actor->xvel() || actor->yvel())
	{
		if (pPlayer && gNoClip)
		{
			pSprite->x += actor->xvel() >> 12;
			pSprite->y += actor->yvel() >> 12;
			if (!FindSector(pSprite->x, pSprite->y, &nSector))
				nSector = pSprite->sectnum;
		}
		else
		{
			short bakCstat = pSprite->cstat;
			pSprite->cstat &= ~257;
			actor->hit.hit = ClipMove(&pSprite->pos, &nSector, actor->xvel() >> 12, actor->yvel() >> 12, wd, tz, bz, CLIPMASK0);
			if (nSector == -1)
			{
				nSector = pSprite->sectnum;
				if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
					actDamageSprite(actor, actor, kDamageFall, 1000 << 4);
			}

			if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate)
			{
				int nSector2 = nSector;
				if (pushmove(&pSprite->pos, &nSector2, wd, tz, bz, CLIPMASK0) == -1)
					actDamageSprite(actor, actor, kDamageFall, 1000 << 4);
				if (nSector2 != -1)
					nSector = nSector2;
			}
			assert(nSector >= 0);
			pSprite->cstat = bakCstat;
		}
		const Collision& coll = actor->hit.hit;
		switch (coll.type)
		{
		case kHitSprite:
		{
			spritetype* pHitSprite = &coll.actor->s();
			XSPRITE* pHitXSprite = coll.actor->hasX() ? &coll.actor->x() : nullptr;;

			auto Owner = coll.actor->GetOwner();

			if (pHitSprite->statnum == kStatProjectile && !(pHitSprite->flags & 32) && actor != Owner)
			{
				HITINFO hitInfo = gHitInfo;
				gHitInfo.hitactor = actor;
				actImpactMissile(coll.actor, 3);
				gHitInfo = hitInfo;
			}
#ifdef NOONE_EXTENSIONS
			if (!gModernMap && pHitXSprite && pHitXSprite->Touch && !pHitXSprite->state && !pHitXSprite->isTriggered)
				trTriggerSprite(coll.actor, kCmdSpriteTouch);
#else
			if (pHitXSprite && pHitXSprite->Touch && !pHitXSprite->state && !pHitXSprite->isTriggered)
				trTriggerSprite(coll.actor, kCmdSpriteTouch);
#endif

			if (pDudeInfo->lockOut && pHitXSprite && pHitXSprite->Push && !pHitXSprite->key && !pHitXSprite->DudeLockout && !pHitXSprite->state && !pHitXSprite->busy && !pPlayer)
				trTriggerSprite(coll.actor, kCmdSpritePush);

			break;
		}
		case kHitWall:
		{
			int nHitWall = coll.index;
			walltype* pHitWall = &wall[nHitWall];
			XWALL* pHitXWall = nullptr;
			if (pHitWall->extra > 0) pHitXWall = &xwall[pHitWall->extra];

			if (pDudeInfo->lockOut && pHitXWall && pHitXWall->triggerPush && !pHitXWall->key && !pHitXWall->dudeLockout && !pHitXWall->state && !pHitXWall->busy && !pPlayer)
				trTriggerWall(nHitWall, pHitXWall, kCmdWallPush);

			if (pHitWall->nextsector != -1)
			{
				sectortype* pHitSector = &sector[pHitWall->nextsector];
				XSECTOR* pHitXSector = nullptr;
				if (pHitSector->extra > 0)
					pHitXSector = &xsector[pHitSector->extra];

				if (pDudeInfo->lockOut && pHitXSector && pHitXSector->Wallpush && !pHitXSector->Key && !pHitXSector->dudeLockout && !pHitXSector->state && !pHitXSector->busy && !pPlayer)
					trTriggerSector(pHitWall->nextsector, pHitXSector, kCmdSectorPush);

				if (top < pHitSector->ceilingz || bottom > pHitSector->floorz)
				{
					// ???
				}
			}
			actWallBounceVector((int*)&actor->xvel(), (int*)&actor->yvel(), nHitWall, 0);
			break;
		}
		}
	}
	else
	{
		assert(nSector >= 0 && nSector < kMaxSectors);
		FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
	}
	if (pSprite->sectnum != nSector)
	{
		assert(nSector >= 0 && nSector < kMaxSectors);
		XSECTOR* pXSector;
		int nXSector = sector[pSprite->sectnum].extra;
		if (nXSector > 0)
			pXSector = &xsector[nXSector];
		else
			pXSector = nullptr;
		if (pXSector && pXSector->Exit && (pPlayer || !pXSector->dudeLockout))
			trTriggerSector(pSprite->sectnum, pXSector, kCmdSectorExit);
		ChangeActorSect(actor, nSector);

		nXSector = sector[nSector].extra;
		pXSector = (nXSector > 0) ? &xsector[nXSector] : nullptr;
		if (pXSector && pXSector->Enter && (pPlayer || !pXSector->dudeLockout))
		{
			if (sector[nSector].type == kSectorTeleport)
				pXSector->data = pPlayer ? pSprite->index : -1;
			trTriggerSector(nSector, pXSector, kCmdSectorEnter);
		}

		nSector = pSprite->sectnum;
	}
	int bUnderwater = 0;
	int bDepth = 0;
	if (sector[nSector].extra > 0)
	{
		XSECTOR* pXSector = &xsector[sector[nSector].extra];
		if (pXSector->Underwater) bUnderwater = 1;
		if (pXSector->Depth) bDepth = 1;
	}
	auto pUpperLink = getUpperLink(nSector);
	auto pLowerLink = getLowerLink(nSector);
	if (pUpperLink && (pUpperLink->s().type == kMarkerUpWater || pUpperLink->s().type == kMarkerUpGoo)) bDepth = 1;
	if (pLowerLink && (pLowerLink->s().type == kMarkerLowWater || pLowerLink->s().type == kMarkerLowGoo)) bDepth = 1;
	if (pPlayer) wd += 16;
	if (actor->zvel()) pSprite->z += actor->zvel() >> 8;

	int ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, wd, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
	GetActorExtents(actor, &top, &bottom);

	if (pSprite->flags & 2)
	{
		int vc = 58254;
		if (bDepth)
		{
			if (bUnderwater)
			{
				int cz = getceilzofslope(nSector, pSprite->x, pSprite->y);
				if (cz > top)
					vc += ((bottom - cz) * -80099) / (bottom - top);
				else
					vc = 0;
			}
			else
			{
				int fz = getflorzofslope(nSector, pSprite->x, pSprite->y);
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
			pSprite->z += ((vc * 4) / 2) >> 8;
			actor->zvel() += vc;
		}
	}
	if (pPlayer && actor->zvel() > 0x155555 && !pPlayer->fallScream && pXSprite->height > 0)
	{
		const bool playerAlive = (pXSprite->health > 0) || VanillaMode(); // only trigger falling scream if player is alive or vanilla mode
		if (playerAlive)
		{
			pPlayer->fallScream = 1;
			sfxPlay3DSound(actor, 719, 0, 0);
		}
	}
	vec3_t const oldpos = pSprite->pos;
	int nLink = CheckLink(actor);
	if (nLink)
	{
		GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, wd, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (pPlayer)
			playerCorrectInertia(pPlayer, &oldpos);
		switch (nLink)
		{
		case kMarkerLowStack:
			if (pPlayer == gView)
				setgotpic(sector[pSprite->sectnum].floorpicnum);
			break;
		case kMarkerUpStack:
			if (pPlayer == gView)
				setgotpic(sector[pSprite->sectnum].ceilingpicnum);
			break;
		case kMarkerLowWater:
		case kMarkerLowGoo:
			pXSprite->medium = kMediumNormal;
			if (pPlayer)
			{
				pPlayer->posture = 0;
				pPlayer->bubbleTime = 0;
				if (!pPlayer->cantJump && (pPlayer->input.actions & SB_JUMP))
				{
					actor->zvel() = -0x6aaaa;
					pPlayer->cantJump = 1;
				}
				sfxPlay3DSound(actor, 721, -1, 0);
			}
			else
			{
				switch (pSprite->type)
				{
				case kDudeCultistTommy:
				case kDudeCultistShotgun:
					aiNewState(actor, &cultistGoto);
					break;
				case kDudeGillBeast:
					aiNewState(actor, &gillBeastGoto);
					pSprite->flags |= 6;
					break;
				case kDudeBoneEel:
					actKillDude(actor, actor, kDamageFall, 1000 << 4);
					break;
				}

#ifdef NOONE_EXTENSIONS
				if (actor->IsDudeActor() && pXSprite->health > 0 && aiInPatrolState(nAiStateType))
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

			pXSprite->medium = medium;

			if (pPlayer)
			{
#ifdef NOONE_EXTENSIONS
				// look for palette in data2 of marker. If value <= 0, use default ones.
				if (gModernMap)
				{
					pPlayer->nWaterPal = 0;
					auto pUpper = getUpperLink(nSector);
					if (pUpper && pUpper->hasX()) pPlayer->nWaterPal = pUpper->x().data2;
				}
#endif

				pPlayer->posture = 1;
				pXSprite->burnTime = 0;
				pPlayer->bubbleTime = abs(actor->zvel()) >> 12;
				evPostActor(actor, 0, kCallbackPlayerBubble);
				sfxPlay3DSound(actor, 720, -1, 0);
			}
			else
			{
				switch (pSprite->type)
				{
				case kDudeCultistTommy:
				case kDudeCultistShotgun:
					pXSprite->burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &cultistSwimGoto);
					break;
				case kDudeBurningCultist:
				{
					const bool fixRandomCultist = !cl_bloodvanillaenemies && (pSprite->inittype >= kDudeBase) && (pSprite->inittype < kDudeMax) && !VanillaMode(); // fix burning cultists randomly switching types underwater
					if (Chance(chance))
						pSprite->type = kDudeCultistTommy;
					else
						pSprite->type = kDudeCultistShotgun;
					if (fixRandomCultist) // fix burning cultists randomly switching types underwater
						pSprite->type = pSprite->inittype; // restore back to spawned cultist type
					pXSprite->burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &cultistSwimGoto);
					break;
				}
				case kDudeZombieAxeNormal:
					pXSprite->burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &zombieAGoto);
					break;
				case kDudeZombieButcher:
					pXSprite->burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &zombieFGoto);
					break;
				case kDudeGillBeast:
					pXSprite->burnTime = 0;
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					sfxPlay3DSound(actor, 720, -1, 0);
					aiNewState(actor, &gillBeastSwimGoto);

					pSprite->flags &= ~6;
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

					if (pSprite->type == kDudeModernCustom) {

						evPostActor(actor, 0, kCallbackEnemeyBubble);
					if (!canSwim(actor)) actKillDude(actor, actor, kDamageFall, 1000 << 4);
						break;
					}

					// continue patrol when fall into water
					if (actor->IsDudeActor() && pXSprite->health > 0 && aiInPatrolState(nAiStateType))
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
		GetZRange(pSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist << 2, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (bottom <= floorZ && pSprite->z - floorZ2 < bz)
		{
			floorZ = floorZ2;
			floorColl = floorColl2;
		}
	}
	if (floorZ <= bottom)
	{
		actor->hit.florhit = floorColl;
		pSprite->z += floorZ - bottom;
		int v30 = actor->zvel() - velFloor[pSprite->sectnum];
		if (v30 > 0)
		{
			int vax = actFloorBounceVector((int*)&actor->xvel(), (int*)&actor->yvel(), (int*)&v30, pSprite->sectnum, 0);
			int nDamage = MulScale(vax, vax, 30);
			if (pPlayer)
			{
				pPlayer->fallScream = 0;

				if (nDamage > (15 << 4) && (pSprite->flags & 4))
					playerLandingSound(pPlayer);
				if (nDamage > (30 << 4))
					sfxPlay3DSound(actor, 701, 0, 0);
			}
			nDamage -= 100 << 4;
			if (nDamage > 0)
				actDamageSprite(actor, actor, kDamageFall, nDamage);
			actor->zvel() = v30;
			if (abs(actor->zvel()) < 0x10000)
			{
				actor->zvel() = velFloor[pSprite->sectnum];
				pSprite->flags &= ~4;
			}
			else
				pSprite->flags |= 4;

			switch (tileGetSurfType(floorColl))
			{
			case kSurfWater:
				gFX.fxSpawnActor(FX_9, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0);
				break;
			case kSurfLava:
			{
				auto pFX = gFX.fxSpawnActor(FX_10, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0);
				if (pFX)
				{
					auto pFXs = &pFX->s();
					for (int i = 0; i < 7; i++)
					{
						auto pFX2 = gFX.fxSpawnActor(FX_14, pFXs->sectnum, pFXs->x, pFXs->y, pFXs->z, 0);
						if (pFX2)
						{
							pFX2->xvel() = Random2(0x6aaaa);
							pFX2->yvel() = Random2(0x6aaaa);
							pFX2->zvel() = -(int)Random(0xd5555);
						}
					}
				}
				break;
			}
			}
		}
		else if (actor->zvel() == 0)

			pSprite->flags &= ~4;
	}
	else
	{
		actor->hit.florhit = 0;

		if (pSprite->flags & 2)
			pSprite->flags |= 4;
	}
	if (top <= ceilZ)
	{
		actor->hit.ceilhit = ceilColl;
		pSprite->z += ClipLow(ceilZ - top, 0);

		if (actor->zvel() <= 0 && (pSprite->flags & 4))
			actor->zvel() = MulScale(-actor->zvel(), 0x2000, 16);
	}
	else
		actor->hit.ceilhit = 0;

	GetActorExtents(actor, &top, &bottom);

	pXSprite->height = ClipLow(floorZ - bottom, 0) >> 8;
	if (actor->xvel() || actor->yvel())
	{
		if (floorColl.type == kHitSprite)
		{
			auto hitAct = floorColl.actor;
			if ((hitAct->s().cstat & 0x30) == 0)
			{
				actor->xvel() += MulScale(4, pSprite->x - hitAct->s().x, 2);
				actor->yvel() += MulScale(4, pSprite->y - hitAct->s().y, 2);
				return;
			}
		}
		int nXSector = sector[pSprite->sectnum].extra;
		if (nXSector > 0 && xsector[nXSector].Underwater)
			return;
		if (pXSprite->height >= 0x100)
			return;
		int nDrag = gDudeDrag;
		if (pXSprite->height > 0)
			nDrag -= scale(gDudeDrag, pXSprite->height, 0x100);
		actor->xvel() -= mulscale16r(actor->xvel(), nDrag);
		actor->yvel() -= mulscale16r(actor->yvel(), nDrag);

		if (approxDist(actor->xvel(), actor->yvel()) < 0x1000)
			actor->xvel() = actor->yvel() = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int MoveMissile(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	auto Owner = actor->GetOwner();
	int cliptype = -1;
	int bakCstat = 0;
	spritetype* pOwner = nullptr;
	if (Owner && Owner->IsDudeActor())
	{
		pOwner = &Owner->s();
		bakCstat = pOwner->cstat;
		pOwner->cstat &= ~257;
	}
	gHitInfo.clearObj();
	if (pSprite->type == kMissileFlameSpray) actAirDrag(actor, 0x1000);

	if (actor->GetTarget() != nullptr && (actor->xvel() || actor->yvel() || actor->zvel()))
	{
		auto target = actor->GetTarget();
		spritetype* pTarget = &target->s();
		XSPRITE* pXTarget = target->hasX() ? &target->x() : nullptr;

		if (pTarget->statnum == kStatDude && pXTarget && pXTarget->health > 0)
		{
			int nTargetAngle = getangle(-(pTarget->y - pSprite->y), pTarget->x - pSprite->x);
			int vx = missileInfo[pSprite->type - kMissileBase].velocity;
			int vy = 0;
			RotatePoint(&vx, &vy, (nTargetAngle + 1536) & 2047, 0, 0);
			actor->xvel() = vx;
			actor->yvel() = vy;
			int dx = pTarget->x - pSprite->x;
			int dy = pTarget->y - pSprite->y;
			int dz = pTarget->z - pSprite->z;

			int deltaz = dz / 10;
			if (pTarget->z < pSprite->z) deltaz = -deltaz;
			actor->zvel() += deltaz;
		}
	}
	int vx = actor->xvel() >> 12;
	int vy = actor->yvel() >> 12;
	int vz = actor->zvel() >> 8;
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	int i = 1;
	const int bakCompat = enginecompatibility_mode;
	const bool isFlameSprite = (pSprite->type == kMissileFlameSpray || pSprite->type == kMissileFlameHound); // do not use accurate clipmove for flame based sprites (changes damage too much)
	while (1)
	{
		vec3_t pos = pSprite->pos;
		int nSector2 = pSprite->sectnum;
		const short bakSpriteCstat = pSprite->cstat;
		if (pOwner && !isFlameSprite && !cl_bloodvanillaexplosions && !VanillaMode())
		{
			enginecompatibility_mode = ENGINECOMPATIBILITY_NONE; // improved clipmove accuracy
			pSprite->cstat &= ~257; // remove self collisions for accurate clipmove
		}
		Collision clipmoveresult = ClipMove(&pos, &nSector2, vx, vy, pSprite->clipdist << 2, (pos.z - top) / 4, (bottom - pos.z) / 4, CLIPMASK0, 1);
		enginecompatibility_mode = bakCompat; // restore
		pSprite->cstat = bakSpriteCstat;
		int nSector = nSector2;
		if (nSector2 < 0)
		{
			cliptype = -1;
			break;
		}
		if (clipmoveresult.type == kHitSprite)
		{
			gHitInfo.hitactor = clipmoveresult.actor;
			cliptype = 3;
		}
		else if (clipmoveresult.type == kHitWall)
		{
			gHitInfo.hitwall = clipmoveresult.index;
			if (wall[clipmoveresult.index].nextsector == -1) cliptype = 0;
			else
			{
				int32_t fz, cz;
				getzsofslope(wall[clipmoveresult.index].nextsector, pos.x, pos.y, &cz, &fz);
				if (pos.z <= cz || pos.z >= fz) cliptype = 0;
				else cliptype = 4;
			}
		}
		if (cliptype == 4)
		{
			walltype* pWall = &wall[gHitInfo.hitwall];
			if (pWall->extra > 0)
			{
				XWALL* pXWall = &xwall[pWall->extra];
				if (pXWall->triggerVector)
				{
					trTriggerWall(gHitInfo.hitwall, pXWall, kCmdWallImpact);
					if (!(pWall->cstat & 64))
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
			int nAngle = getangle(actor->xvel(), actor->yvel());
			pos.x -= MulScale(Cos(nAngle), 16, 30);
			pos.y -= MulScale(Sin(nAngle), 16, 30);
			int nVel = approxDist(actor->xvel(), actor->yvel());
			vz -= scale(0x100, actor->zvel(), nVel);
			updatesector(pos.x, pos.y, &nSector);
			nSector2 = nSector;
		}
		int ceilZ, floorZ;
		Collision ceilColl, floorColl;
		GetZRangeAtXYZ(pos.x, pos.y, pos.z, nSector2, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist << 2, CLIPMASK0);
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
		pSprite->pos = pos;
		pSprite->z += vz;
		updatesector(pos.x, pos.y, &nSector);
		if (nSector >= 0 && nSector != pSprite->sectnum)
		{
			assert(nSector >= 0 && nSector < kMaxSectors);
			ChangeActorSect(actor, nSector);
		}
		CheckLink(actor);
		gHitInfo.hitsect = pSprite->sectnum;
		gHitInfo.hitx = pSprite->x;
		gHitInfo.hity = pSprite->y;
		gHitInfo.hitz = pSprite->z;
		break;
	}
	if (pOwner) pOwner->cstat = bakCstat;

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
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	//auto Owner = actor->GetOwner();

	if (pSprite->statnum == kStatExplosion) return;
	sfxKill3DSound(pSprite, -1, -1);
	evKillActor(actor);

	int nType = kExplosionStandard;

	switch (pSprite->type)
	{
	case kMissileFireballNapalm:
		nType = kExplosionNapalm;
		seqSpawn(4, actor, -1);
		if (Chance(0x8000)) pSprite->cstat |= 4;
		sfxPlay3DSound(actor, 303, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;

	case kMissileFlareAlt:
		nType = kExplosionFireball;
		seqSpawn(9, actor, -1);
		if (Chance(0x8000)) pSprite->cstat |= 4;
		sfxPlay3DSound(actor, 306, 24 + (pSprite->index & 3), FX_GlobalChannel); // ouch...
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
		auto spawned = actSpawnSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0, 1);
		spawned->SetOwner(actor->GetOwner());
		if (actCheckRespawn(actor))
		{
			pXSprite->state = 1;
			pXSprite->health = thingInfo[0].startHealth << 4;
		}
		else actPostSprite(actor, kStatFree);

		nType = kExplosionLarge;
		seqSpawn(4, spawned, -1);
		actor = spawned;
		pSprite = &spawned->s();
		pXSprite = &spawned->x();

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
			nType = pXSprite->data1;  // Explosion type
			int tSeq = pXSprite->data2; // SEQ id
			int tSnd = pXSprite->data3; // Sound Id

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
		if (Chance(0x8000)) pSprite->cstat |= 4;
		sfxPlay3DSound(actor, 303, -1, 0);
		GibSprite(actor, GIBTYPE_5, nullptr, nullptr);
		break;
	}
	actor->xvel() = actor->yvel() = actor->zvel() = 0;
	actPostSprite(actor, kStatExplosion);
	pSprite->xrepeat = pSprite->yrepeat = explodeInfo[nType].repeat;

	pSprite->flags &= ~3;
	pSprite->type = nType;
	const EXPLOSION* pExplodeInfo = &explodeInfo[nType];
	actor->SetTarget(nullptr);
	actor->explosionhackflag = true;
	pXSprite->data1 = pExplodeInfo->ticks;
	pXSprite->data2 = pExplodeInfo->quakeEffect;
	pXSprite->data3 = pExplodeInfo->flashEffect;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actActivateGibObject(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();

	int gib1 = ClipRange(pXSprite->data1, 0, 31);
	int gib2 = ClipRange(pXSprite->data2, 0, 31);
	int gib3 = ClipRange(pXSprite->data3, 0, 31);
	int sound = pXSprite->data4;
	int dropmsg = pXSprite->dropMsg;

	if (gib1 > 0) GibSprite(actor, (GIBTYPE)(gib1 - 1), nullptr, nullptr);
	if (gib2 > 0) GibSprite(actor, (GIBTYPE)(gib2 - 1), nullptr, nullptr);
	if (gib3 > 0 && pXSprite->burnTime > 0) GibSprite(actor, (GIBTYPE)(gib3 - 1), nullptr, nullptr);
	if (sound > 0) sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, sound, pSprite->sectnum);
	if (dropmsg > 0) actDropObject(actor, dropmsg);

	if (!(pSprite->cstat & 32768) && !(pSprite->flags & kHitagRespawn))
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
		spritetype* pSprite = &actor->s();
		if (pSprite->flags & 32) continue;

		if (actor->hasX())
		{
			XSPRITE* pXSprite = &actor->x();
			switch (pSprite->type)
			{
			case kThingBloodBits:
			case kThingBloodChunks:
			case kThingZombieHead:
				if (pXSprite->locked && PlayClock >= pXSprite->targetX) pXSprite->locked = 0;
				break;
			}

			if (pXSprite->burnTime > 0)
			{
				pXSprite->burnTime = ClipLow(pXSprite->burnTime - 4, 0);
				actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
			}

			if (pXSprite->Proximity)
			{
#ifdef NOONE_EXTENSIONS
				// don't process locked or 1-shot things for proximity
				if (gModernMap && (pXSprite->locked || pXSprite->isTriggered))
					continue;
#endif

				if (pSprite->type == kThingDroppedLifeLeech) actor->SetTarget(nullptr);
				BloodStatIterator it1(kStatDude);
				while (auto dudeactor = it1.Next())
				{
					auto nextdude = it1.Peek();
					spritetype* pSprite2 = &dudeactor->s();

					if (pSprite2->flags & 32 || !dudeactor->hasX()) continue;

					XSPRITE* pXSprite2 = &dudeactor->x();
					if ((unsigned int)pXSprite2->health > 0)
					{
						int proxyDist = 96;
#ifdef NOONE_EXTENSIONS
						// allow dudeLockout for proximity flag
						if (gModernMap && pSprite->type != kThingDroppedLifeLeech && pXSprite->DudeLockout && !dudeactor->IsPlayerActor())
							continue;

						if (pSprite->type == kModernThingEnemyLifeLeech) proxyDist = 512;
#endif
						if (pSprite->type == kThingDroppedLifeLeech && actor->GetTarget() == nullptr)
						{
							auto Owner = actor->GetOwner();
							if (!Owner->IsPlayerActor()) continue;

							spritetype* pOwner = &Owner->s();
							PLAYER* pPlayer = &gPlayer[pOwner->type - kDudePlayer1];
							PLAYER* pPlayer2 = dudeactor->IsPlayerActor() ? &gPlayer[pSprite2->type - kDudePlayer1] : nullptr;

							if (dudeactor == Owner || pSprite2->type == kDudeZombieAxeBuried || pSprite2->type == kDudeRat || pSprite2->type == kDudeBat) continue;
							if (gGameOptions.nGameType == 3 && pPlayer2 && pPlayer->teamId == pPlayer2->teamId) continue;
							if (gGameOptions.nGameType == 1 && pPlayer2) continue;
							proxyDist = 512;
						}

						if (CheckProximity(pSprite2, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, proxyDist))
						{
							switch (pSprite->type)
							{
							case kThingDroppedLifeLeech:
								if (!Chance(0x4000) && nextdude) continue;
								if (pSprite2->cstat & CLIPMASK0) actor->SetTarget(dudeactor);
								else continue;
								break;

#ifdef NOONE_EXTENSIONS
							case kModernThingTNTProx:
								if (!dudeactor->IsPlayerActor()) continue;
								pSprite->pal = 0;
								break;

							case kModernThingEnemyLifeLeech:
								if (actor->GetTarget() != dudeactor) continue;
								break;
#endif

							default:
								break;
							}
							if (actor->GetOwner() == nullptr) actor->SetOwner(dudeactor);
							trTriggerSprite(actor, kCmdSpriteProximity);
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
		spritetype* pSprite = &actor->s();

		if (pSprite->flags & 32) continue;
		if (!actor->hasX()) continue;

		auto pXSprite = &actor->x();
		int nSector = pSprite->sectnum;

		int nXSector = sector[nSector].extra;
		XSECTOR* pXSector = NULL;
		if (nXSector > 0)
		{
			assert(nXSector > 0 && nXSector < kMaxXSectors);
			assert(xsector[nXSector].reference == nSector);
			pXSector = &xsector[nXSector];
		}
		if (pXSector && pXSector->panVel && (pXSector->panAlways || pXSector->state || pXSector->busy))
		{
			int nType = pSprite->type - kThingBase;
			const THINGINFO* pThingInfo = &thingInfo[nType];
			if (pThingInfo->flags & 1) pSprite->flags |= 1;
			if (pThingInfo->flags & 2) pSprite->flags |= 4;
		}

		if (pSprite->flags & 3)
		{
			viewBackupSpriteLoc(actor);
			if (pXSector && pXSector->panVel)
			{
				int top, bottom;
				GetActorExtents(actor, &top, &bottom);
				if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
				{
					int angle = pXSector->panAngle;
					int speed = 0;
					if (pXSector->panAlways || pXSector->state || pXSector->busy)
					{
						speed = pXSector->panVel << 9;
						if (!pXSector->panAlways && pXSector->busy) speed = MulScale(speed, pXSector->busy, 16);
					}
					if (sector[nSector].floorstat & 64) angle = (angle + GetWallAngle(sector[nSector].wallptr) + 512) & 2047;

					actor->xvel() += MulScale(speed, Cos(angle), 30);
					actor->yvel() += MulScale(speed, Sin(angle), 30);
				}
			}
			actAirDrag(actor, 128);

			if (((pSprite->index >> 8) & 15) == (gFrameCount & 15) && (pSprite->flags & 2))	pSprite->flags |= 4;
			if ((pSprite->flags & 4) || actor->xvel() || actor->yvel() || actor->zvel() || velFloor[pSprite->sectnum] || velCeil[pSprite->sectnum])
			{
				Collision hit = MoveThing(actor);
				if (hit.type)
				{
					if (pXSprite->Impact) trTriggerSprite(actor, kCmdOff);

					switch (pSprite->type)
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
							pSprite->xrepeat = 32;
							pSprite->yrepeat = 32;
							actDamageSprite(actor->GetOwner(), hit.actor, kDamageFall, pXSprite->data1);
						}
						break;
#endif
					case kThingBone:
						seqSpawn(24, actor, -1);
						if (hit.type == kHitSprite)
						{
							actDamageSprite(actor->GetOwner(), hit.actor, kDamageFall, 12);
						}
						break;

					case kThingPodGreenBall:
						if (hit.type == kHitSector)
						{
							actRadiusDamage(actor->GetOwner(), pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 200, 1, 20, kDamageExplode, 6, 0);
							evPostActor(actor, 0, kCallbackFXPodBloodSplat);
						}
						else if (hit.type == kHitSprite)
						{
							actDamageSprite(actor->GetOwner(), hit.actor, kDamageFall, 12);
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
		spritetype* pSprite = &actor->s();

		if (pSprite->flags & 32)
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

static void actCheckExplosion()
{
	BloodStatIterator it(kStatExplosion);
	while (auto actor = it.Next())
	{
		spritetype* pSprite = &actor->s();

		if (pSprite->flags & 32)
			continue;

		if (!actor->hasX()) continue;
		XSPRITE* pXSprite = &actor->x();

		auto Owner = actor->GetOwner();
		auto pOwner = Owner ? &Owner->s() : nullptr;
		int nType = pSprite->type;
		assert(nType >= 0 && nType < kExplodeMax);
		const EXPLOSION* pExplodeInfo = &explodeInfo[nType];
		int x = pSprite->x;
		int y = pSprite->y;
		int z = pSprite->z;
		int nSector = pSprite->sectnum;
		int radius = pExplodeInfo->radius;

#ifdef NOONE_EXTENSIONS
		// Allow to override explosion radius by data4 field of any sprite which have statnum 2 set in editor
		// or of Hidden Exploder.
		if (gModernMap && pXSprite->data4 > 0)
			radius = pXSprite->data4;
#endif

		uint8_t sectormap[(kMaxSectors + 7) >> 3];

		// GetClosestSpriteSectors() has issues checking some sectors due to optimizations
		// the new flag newSectCheckMethod for GetClosestSpriteSectors() does rectify these issues, but this may cause unintended side effects for level scripted explosions
		// so only allow this new checking method for dude spawned explosions
		short gAffectedXWalls[kMaxXWalls];
		const bool newSectCheckMethod = !cl_bloodvanillaexplosions && Owner && Owner->IsDudeActor() && !VanillaMode(); // use new sector checking logic
		GetClosestSpriteSectors(nSector, x, y, radius, sectormap, gAffectedXWalls, newSectCheckMethod);

		for (int i = 0; i < kMaxXWalls; i++)
		{
			int nWall = gAffectedXWalls[i];
			if (nWall == -1)
				break;
			XWALL* pXWall = &xwall[wall[nWall].extra];
			trTriggerWall(nWall, pXWall, kCmdWallImpact);
		}

		BloodStatIterator it1(kStatDude);
		while (auto dudeactor = it1.Next())
		{
			spritetype* pDude = &dudeactor->s();

			if (pDude->flags & 32) continue;

			if (TestBitString(sectormap, pDude->sectnum))
			{
				if (pXSprite->data1 && CheckProximity(pDude, x, y, z, nSector, radius))
				{
					if (pExplodeInfo->dmg && actor->explosionhackflag)
					{
						actor->explosionhackflag = false;
						actDamageSprite(Owner, dudeactor, kDamageFall, (pExplodeInfo->dmg + Random(pExplodeInfo->dmgRng)) << 4);
					}
					if (pExplodeInfo->dmgType) ConcussSprite(actor, dudeactor, x, y, z, pExplodeInfo->dmgType);

					if (pExplodeInfo->burnTime)
					{
						assert(dudeactor->hasX());
						XSPRITE* pXDude = &dudeactor->x();
						if (!pXDude->burnTime) evPostActor(dudeactor, 0, kCallbackFXFlameLick);
						actBurnSprite(Owner, dudeactor, pExplodeInfo->burnTime << 2);
					}
				}
			}
		}

		it1.Reset(kStatThing);
		while (auto thingactor = it1.Next())
		{
			spritetype* pThing = &thingactor->s();

			if (pThing->flags & 32) continue;

			if (TestBitString(sectormap, pThing->sectnum))
			{
				if (pXSprite->data1 && CheckProximity(pThing, x, y, z, nSector, radius) && thingactor->hasX())
				{
					XSPRITE* pXThing = &thingactor->x();
					if (!pXThing->locked)
					{
						if (pExplodeInfo->dmgType) ConcussSprite(Owner, thingactor, x, y, z, pExplodeInfo->dmgType);

						if (pExplodeInfo->burnTime)
						{
							if (pThing->type == kThingTNTBarrel && !pXThing->burnTime)
								evPostActor(thingactor, 0, kCallbackFXFlameLick);
							actBurnSprite(Owner, thingactor, pExplodeInfo->burnTime << 2);
						}
					}
				}
			}
		}

		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			spritetype* pSprite2 = gPlayer[p].pSprite;
			int dx = (x - pSprite2->x) >> 4;
			int dy = (y - pSprite2->y) >> 4;
			int dz = (z - pSprite2->z) >> 8;
			int nDist = dx * dx + dy * dy + dz * dz + 0x40000;
			int t = DivScale(pXSprite->data2, nDist, 16);
			gPlayer[p].flickerEffect += t;
		}

#ifdef NOONE_EXTENSIONS
		if (pXSprite->data1 != 0)
		{
			// add impulse for sprites from physics list
			if (gPhysSpritesCount > 0 && pExplodeInfo->dmgType != 0)
			{
				for (int i = 0; i < gPhysSpritesCount; i++)
				{
					if (gPhysSpritesList[i] == nullptr) continue;
					auto physactor = gPhysSpritesList[i];
					spritetype* pDebris = &physactor->s();
					if (pDebris->sectnum < 0 || (pDebris->flags & kHitagFree) != 0) continue;

					if (!TestBitString(sectormap, pDebris->sectnum) || !CheckProximity(pDebris, x, y, z, nSector, radius)) continue;
					else debrisConcuss(Owner, i, x, y, z, pExplodeInfo->dmgType);
				}
			}

			// trigger sprites from impact list
			if (gImpactSpritesCount > 0) {
				for (int i = 0; i < gImpactSpritesCount; i++)
				{
					if (gImpactSpritesList[i] == nullptr) continue;

					auto impactactor = gImpactSpritesList[i];
					if (!impactactor->hasX() || impactactor->s().sectnum < 0 || (impactactor->s().flags & kHitagFree) != 0)	continue;

					if (/*pXImpact->state == pXImpact->restState ||*/ !TestBitString(sectormap, impactactor->s().sectnum) || !CheckProximity(&impactactor->s(), x, y, z, nSector, radius))
						continue;

					trTriggerSprite(impactactor, kCmdSpriteImpact);
				}
			}

		}

		if (!gModernMap || !(pSprite->flags & kModernTypeFlag1))
		{
			// if data4 > 0, do not remove explosion. This can be useful when designer wants put explosion generator in map manually via sprite statnum 2.
			pXSprite->data1 = ClipLow(pXSprite->data1 - 4, 0);
			pXSprite->data2 = ClipLow(pXSprite->data2 - 4, 0);
			pXSprite->data3 = ClipLow(pXSprite->data3 - 4, 0);
		}
#else
		pXSprite->data1 = ClipLow(pXSprite->data1 - 4, 0);
		pXSprite->data2 = ClipLow(pXSprite->data2 - 4, 0);
		pXSprite->data3 = ClipLow(pXSprite->data3 - 4, 0);
#endif

		if (pXSprite->data1 == 0 && pXSprite->data2 == 0 && pXSprite->data3 == 0 && seqGetStatus(actor) < 0)
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
		spritetype* pSprite = &actor->s();

		if ((pSprite->flags & 32) || !actor->hasX())
			continue;

		XSPRITE* pXSprite = &actor->x();
		switch (pSprite->type) {
		case kTrapSawCircular:
			pXSprite->data2 = ClipLow(pXSprite->data2 - 4, 0);
			break;

		case kTrapFlame:
			if (pXSprite->state && seqGetStatus(actor) < 0)
			{
				int x = pSprite->x;
				int y = pSprite->y;
				int z = pSprite->z;
				int t = (pXSprite->data1 << 23) / 120;
				int dx = MulScale(t, Cos(pSprite->ang), 30);
				int dy = MulScale(t, Sin(pSprite->ang), 30);
				for (int i = 0; i < 2; i++)
				{
					auto pFX = gFX.fxSpawnActor(FX_32, pSprite->sectnum, x, y, z, 0);
					if (pFX)
					{
						pFX->xvel() = dx + Random2(0x8888);
						pFX->yvel() = dy + Random2(0x8888);
						pFX->zvel() = Random2(0x8888);
					}
					x += (dx / 2) >> 12;
					y += (dy / 2) >> 12;
				}
				dy = bsin(pSprite->ang);
				dx = bcos(pSprite->ang);
				gVectorData[kVectorTchernobogBurn].maxDist = pXSprite->data1 << 9;
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
		spritetype* pSprite = &actor->s();

		if (pSprite->flags & 32)
			continue;

		if (actor->hasX())
		{
			XSPRITE* pXSprite = &actor->x();
			const bool fixBurnGlitch = !cl_bloodvanillaenemies && IsBurningDude(actor) && !VanillaMode(); // if enemies are burning, always apply burning damage per tick
			if ((pXSprite->burnTime > 0) || fixBurnGlitch)
			{
				switch (pSprite->type)
				{
				case kDudeBurningInnocent:
				case kDudeBurningCultist:
				case kDudeBurningZombieAxe:
				case kDudeBurningZombieButcher:
					actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
					break;

				default:
					pXSprite->burnTime = ClipLow(pXSprite->burnTime - 4, 0);
					actDamageSprite(actor->GetBurnSource(), actor, kDamageBurn, 8);
					break;
				}
			}

#ifdef NOONE_EXTENSIONS
			// handle incarnations of custom dude
			if (pSprite->type == kDudeModernCustom && pXSprite->txID > 0 && pXSprite->sysData1 == kGenDudeTransformStatus)
			{
				actor->xvel() = actor->yvel() = 0;
				if (seqGetStatus(actor) < 0) genDudeTransform(actor);
			}
#endif
			if (pSprite->type == kDudeCerberusTwoHead)
			{
				if (pXSprite->health <= 0 && seqGetStatus(actor) < 0)
				{
					pXSprite->health = dudeInfo[28].startHealth << 4;
					pSprite->type = kDudeCerberusOneHead;
					if (actor->GetTarget() != nullptr) aiSetTarget(actor, actor->GetTarget());
					aiActivateDude(actor);
				}
			}
			if (pXSprite->Proximity && !pXSprite->isTriggered)
			{
				BloodStatIterator it1(kStatDude);
				while (auto actor2 = it1.Next())
				{
					spritetype* pSprite2 = &actor2->s();
					if (pSprite2->flags & 32) continue;

					XSPRITE* pXSprite2 = &actor2->x();

					if ((unsigned int)pXSprite2->health > 0 && IsPlayerSprite(pSprite2))
					{
						if (CheckProximity(pSprite2, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 128))
							trTriggerSprite(actor, kCmdSpriteProximity);
					}
				}
			}
			if (actor->IsPlayerActor())
			{
				PLAYER* pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
				if (pPlayer->voodooTargets) voodooTarget(pPlayer);
				if (pPlayer->hand && Chance(0x8000)) actDamageSprite(actor, actor, kDamageDrown, 12);

				if (pPlayer->isUnderwater)
				{
					char bActive = packItemActive(pPlayer, 1);

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

					if (actor->xvel() || actor->yvel())
						sfxPlay3DSound(actor, 709, 100, 2);

					pPlayer->bubbleTime = ClipLow(pPlayer->bubbleTime - 4, 0);
				}
				else if (gGameOptions.nGameType == 0)
				{
					if (pPlayer->pXSprite->health > 0 && pPlayer->restTime >= 1200 && Chance(0x200))
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
		spritetype* pSprite = &actor->s();
		if (pSprite->flags & 32 || !actor->hasX()) continue;

		int nSector = pSprite->sectnum;
		viewBackupSpriteLoc(actor);
		int nXSector = sector[nSector].extra;
		XSECTOR* pXSector = NULL;

		if (nXSector > 0)
		{
			assert(nXSector > 0 && nXSector < kMaxXSectors);
			assert(xsector[nXSector].reference == nSector);
			pXSector = &xsector[nXSector];
		}
		if (pXSector)
		{
			int top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
			{
				int angle = pXSector->panAngle;
				int speed = 0;
				if (pXSector->panAlways || pXSector->state || pXSector->busy)
				{
					speed = pXSector->panVel << 9;
					if (!pXSector->panAlways && pXSector->busy)
						speed = MulScale(speed, pXSector->busy, 16);
				}
				if (sector[nSector].floorstat & 64)
					angle = (angle + GetWallAngle(sector[nSector].wallptr) + 512) & 2047;
				int dx = MulScale(speed, Cos(angle), 30);
				int dy = MulScale(speed, Sin(angle), 30);
				actor->xvel() += dx;
				actor->yvel() += dy;
			}
		}
		if (pXSector && pXSector->Underwater) actAirDrag(actor, 5376);
		else actAirDrag(actor, 128);

		if ((pSprite->flags & 4) || actor->xvel() || actor->yvel() || actor->zvel() || velFloor[pSprite->sectnum] || velCeil[pSprite->sectnum])
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
		spritetype* pSprite = &actor->s();
		if ((pSprite->flags & 32) || !actor->hasX()) continue;

		XSPRITE* pXSprite = &actor->x();
		auto target = actor->GetTarget();
		if (!target) continue;

		viewBackupSpriteLoc(actor);
		spritetype* pTarget = &target->s();
		auto pXTarget = target->hasX() ? &target->x() : nullptr;
		if (pTarget->statnum == kMaxStatus)
		{
			GibSprite(actor, GIBTYPE_17, NULL, NULL);
			actPostSprite(actor, kStatFree);
		}
		if (pXTarget && pXTarget->health > 0)
		{
			int x = pTarget->x + mulscale30r(Cos(pXSprite->goalAng + pTarget->ang), pTarget->clipdist * 2);
			int y = pTarget->y + mulscale30r(Sin(pXSprite->goalAng + pTarget->ang), pTarget->clipdist * 2);
			int z = pTarget->z + pXSprite->targetZ;
			vec3_t pos = { x, y, z };
			setActorPos(actor, &pos);
			actor->xvel() = target->xvel();
			actor->yvel() = target->yvel();
			actor->zvel() = target->zvel();
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

DBloodActor* actSpawnSprite(int nSector, int x, int y, int z, int nStat, bool setextra)
{
	DBloodActor* actor;
	int nSprite = InsertSprite(nSector, nStat);
	if (nSprite >= 0)
	{
		sprite[nSprite].extra = -1;
		actor = &bloodActors[nSprite];
	}
	else
	{
		BloodStatIterator it(kStatPurge);
		actor = it.Next();
		assert(actor != nullptr);
		ChangeActorSect(actor, nSector);
		actPostSprite(actor, nStat);
	}

	vec3_t pos = { x, y, z };
	setActorPos(actor, &pos);
	spritetype* pSprite = &actor->s();
	pSprite->type = kSpriteDecoration;
	if (setextra && !actor->hasX())
	{
		actor->addX();
		actor->hit.florhit = 0;
		actor->hit.ceilhit = 0;
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
	auto pSource = &source->s();
	int nSprite = InsertSprite(pSource->sectnum, nStat);
	DBloodActor* actor;
	if (nSprite < 0)
	{
		BloodStatIterator it(kStatPurge);
		actor = it.Next();
		assert(actor);
		assert(pSource->sectnum >= 0 && pSource->sectnum < kMaxSectors);
		ChangeActorSect(actor, pSource->sectnum);
		actPostSprite(actor, nStat);
	}
	else actor = &bloodActors[nSprite];

	spritetype* pSprite = &actor->s();
	pSprite->x = pSource->x;
	pSprite->y = pSource->y;
	pSprite->z = pSource->z;
	actor->xvel() = source->xvel();
	actor->yvel() = source->yvel();
	actor->zvel() = source->zvel();
	pSprite->flags = 0;
	actor->addX();
	actor->hit.florhit = 0;
	actor->hit.ceilhit = 0;
	if (!VanillaMode()) actor->SetTarget(nullptr);
	return actor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actSpawnDude(DBloodActor* source, short nType, int a3, int a4)
{
	auto pSource = &source->s();
	XSPRITE* pXSource = &source->x();
	auto spawned = actSpawnSprite(source, kStatDude);
	if (!spawned) return NULL;
	spritetype* pSprite2 = &spawned->s();
	XSPRITE* pXSprite2 = &spawned->x();
	int angle = pSource->ang;
	int nDude = nType - kDudeBase;
	int x, y, z;
	z = a4 + pSource->z;
	if (a3 < 0)
	{
		x = pSource->x;
		y = pSource->y;
	}
	else
	{
		x = pSource->x + mulscale30r(Cos(angle), a3);
		y = pSource->y + mulscale30r(Sin(angle), a3);
	}
	pSprite2->type = nType;
	pSprite2->ang = angle;
	vec3_t pos = { x, y, z };
    setActorPos(spawned, &pos);
	pSprite2->cstat |= 0x1101;
	pSprite2->clipdist = getDudeInfo(nDude + kDudeBase)->clipdist;
	pXSprite2->health = getDudeInfo(nDude + kDudeBase)->startHealth << 4;
	pXSprite2->respawn = 1;
	if (getSequence(getDudeInfo(nDude + kDudeBase)->seqStartID))
		seqSpawn(getDudeInfo(nDude + kDudeBase)->seqStartID, spawned, -1);

#ifdef NOONE_EXTENSIONS
	// add a way to inherit some values of spawner type 18 by dude.
	// This way designer can count enemies via switches and do many other interesting things.
	if (gModernMap && pSource->flags & kModernTypeFlag1)
	{
		// allow inheriting only for selected source types
		switch (pSource->type)
		{
		case kMarkerDudeSpawn:
			//inherit pal?
			if (pSprite2->pal <= 0) pSprite2->pal = pSource->pal;

			// inherit spawn sprite trigger settings, so designer can count monsters.
			pXSprite2->txID = pXSource->txID;
			pXSprite2->command = pXSource->command;
			pXSprite2->triggerOn = pXSource->triggerOn;
			pXSprite2->triggerOff = pXSource->triggerOff;

			// inherit drop items
			pXSprite2->dropMsg = pXSource->dropMsg;

			// inherit dude flags
			pXSprite2->dudeDeaf = pXSource->dudeDeaf;
			pXSprite2->dudeGuard = pXSource->dudeGuard;
			pXSprite2->dudeAmbush = pXSource->dudeAmbush;
			pXSprite2->dudeFlag4 = pXSource->dudeFlag4;
			pXSprite2->unused1 = pXSource->unused1;
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

DBloodActor* actSpawnThing(int nSector, int x, int y, int z, int nThingType)
{
	assert(nThingType >= kThingBase && nThingType < kThingMax);
	auto actor = actSpawnSprite(nSector, x, y, z, 4, 1);
	spritetype* pSprite = &actor->s();
	int nType = nThingType - kThingBase;
	pSprite->type = nThingType;
	assert(actor->hasX());
	XSPRITE* pXThing = &actor->x();
	const THINGINFO* pThingInfo = &thingInfo[nType];
	pXThing->health = pThingInfo->startHealth << 4;
	pSprite->clipdist = pThingInfo->clipdist;
	pSprite->flags = pThingInfo->flags;
	if (pSprite->flags & 2) pSprite->flags |= 4;
	pSprite->cstat |= pThingInfo->cstat;
	pSprite->picnum = pThingInfo->picnum;
	pSprite->shade = pThingInfo->shade;
	pSprite->pal = pThingInfo->pal;
	if (pThingInfo->xrepeat) pSprite->xrepeat = pThingInfo->xrepeat;
	if (pThingInfo->yrepeat) pSprite->yrepeat = pThingInfo->yrepeat;
	pSprite->cstat2 |= CSTAT2_SPRITE_MAPPED;
	switch (nThingType)
	{
	case kThingVoodooHead:
		pXThing->data1 = 0;
		pXThing->data2 = 0;
		pXThing->data3 = 0;
		pXThing->data4 = 0;
		pXThing->state = 1;
		pXThing->triggerOnce = 1;
		pXThing->isTriggered = 0;
		break;

	case kThingDroppedLifeLeech:
#ifdef NOONE_EXTENSIONS
	case kModernThingEnemyLifeLeech:
#endif
		pXThing->data1 = 0;
		pXThing->data2 = 0;
		pXThing->data3 = 0;
		pXThing->data4 = 0;
		pXThing->state = 1;
		pXThing->triggerOnce = 0;
		pXThing->isTriggered = 0;
		break;

	case kThingZombieHead:
		pXThing->data1 = 8;
		pXThing->data2 = 0;
		pXThing->data3 = 0;
		pXThing->data4 = 318;
		pXThing->targetX = PlayClock + 180;
		pXThing->locked = 1;
		pXThing->state = 1;
		pXThing->triggerOnce = 0;
		pXThing->isTriggered = 0;
		break;

	case kThingBloodBits:
	case kThingBloodChunks:
		pXThing->data1 = (nThingType == kThingBloodBits) ? 19 : 8;
		pXThing->data2 = 0;
		pXThing->data3 = 0;
		pXThing->data4 = 318;
		pXThing->targetX = PlayClock + 180;
		pXThing->locked = 1;
		pXThing->state = 1;
		pXThing->triggerOnce = 0;
		pXThing->isTriggered = 0;
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
	auto pSprite = &actor->s();
	assert(thingType >= kThingBase && thingType < kThingMax);
	int x = pSprite->x + MulScale(a2, Cos(pSprite->ang + 512), 30);
	int y = pSprite->y + MulScale(a2, Sin(pSprite->ang + 512), 30);
	int z = pSprite->z + a3;
	x += MulScale(pSprite->clipdist, Cos(pSprite->ang), 28);
	y += MulScale(pSprite->clipdist, Sin(pSprite->ang), 28);
	if (HitScan(pSprite, z, x - pSprite->x, y - pSprite->y, 0, CLIPMASK0, pSprite->clipdist) != -1)
	{
		x = gHitInfo.hitx - MulScale(pSprite->clipdist << 1, Cos(pSprite->ang), 28);
		y = gHitInfo.hity - MulScale(pSprite->clipdist << 1, Sin(pSprite->ang), 28);
	}
	auto fired = actSpawnThing(pSprite->sectnum, x, y, z, thingType);
	spritetype* pThing = &fired->s();
	fired->SetOwner(actor);
	pThing->ang = pSprite->ang;
	fired->xvel() = MulScale(a6, Cos(pThing->ang), 30);
	fired->yvel() = MulScale(a6, Sin(pThing->ang), 30);
	fired->zvel() = MulScale(a6, a4, 14);
	fired->xvel() += actor->xvel() / 2;
	fired->yvel() += actor->yvel() / 2;
	fired->zvel() += actor->zvel() / 2;
	return fired;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actBuildMissile(DBloodActor* spawned, DBloodActor* actor)
{
	auto pMissile = &spawned->s();
	switch (pMissile->type)
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
		pMissile->cstat |= 16;
		break;
	case kMissileTeslaRegular:
		sfxPlay3DSound(pMissile, 251, 0, 0);
		break;
	case kMissileEctoSkull:
		seqSpawn(2, spawned, -1);
		sfxPlay3DSound(pMissile, 493, 0, 0);
		break;
	case kMissileFireballNapalm:
		seqSpawn(61, spawned, nNapalmClient);
		sfxPlay3DSound(pMissile, 441, 0, 0);
		break;
	case kMissileFireball:
		seqSpawn(22, spawned, nFireballClient);
		sfxPlay3DSound(pMissile, 441, 0, 0);
		break;
	case kMissileFlameHound:
		seqSpawn(27, spawned, -1);
		spawned->xvel() += actor->xvel() / 2 + Random2(0x11111);
		spawned->yvel() += actor->yvel() / 2 + Random2(0x11111);
		spawned->zvel() += actor->zvel() / 2 + Random2(0x11111);
		break;
	case kMissileFireballCerberus:
		seqSpawn(61, spawned, dword_2192E0);
		sfxPlay3DSound(pMissile, 441, 0, 0);
		break;
	case kMissileFireballTchernobog:
		seqSpawn(23, spawned, dword_2192D8);
		spawned->xvel() += actor->xvel() / 2 + Random2(0x11111);
		spawned->yvel() += actor->yvel() / 2 + Random2(0x11111);
		spawned->zvel() += actor->zvel() / 2 + Random2(0x11111);
		break;
	case kMissileFlameSpray:
		if (Chance(0x8000))	seqSpawn(0, spawned, -1);
		else seqSpawn(1, spawned, -1);
		spawned->xvel() += actor->xvel() / 2 + Random2(0x11111);
		spawned->yvel() += actor->yvel() / 2 + Random2(0x11111);
		spawned->zvel() += actor->zvel() / 2 + Random2(0x11111);
		break;
	case kMissileFlareAlt:
		evPostActor(spawned, 30, kCallbackFXFlareBurst);
		evPostActor(spawned, 0, kCallbackFXFlareSpark);
		sfxPlay3DSound(pMissile, 422, 0, 0);
		break;
	case kMissileFlareRegular:
		evPostActor(spawned, 0, kCallbackFXFlareSpark);
		sfxPlay3DSound(pMissile, 422, 0, 0);
		break;
	case kMissileLifeLeechAltSmall:
		evPostActor(spawned, 0, kCallbackFXArcSpark);
		break;
	case kMissileArcGargoyle:
		sfxPlay3DSound(pMissile, 252, 0, 0);
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
	char v4 = 0;
	auto pSprite = &actor->s();
	int nSprite = pSprite->index;
	const MissileType* pMissileInfo = &missileInfo[nType - kMissileBase];
	int x = pSprite->x + MulScale(a2, Cos(pSprite->ang + 512), 30);
	int y = pSprite->y + MulScale(a2, Sin(pSprite->ang + 512), 30);
	int z = pSprite->z + a3;
	int clipdist = pMissileInfo->clipDist + pSprite->clipdist;
	x += MulScale(clipdist, Cos(pSprite->ang), 28);
	y += MulScale(clipdist, Sin(pSprite->ang), 28);
	int hit = HitScan(pSprite, z, x - pSprite->x, y - pSprite->y, 0, CLIPMASK0, clipdist);
	if (hit != -1)
	{
		if (hit == 3 || hit == 0)
		{
			v4 = 1;
			x = gHitInfo.hitx - MulScale(Cos(pSprite->ang), 16, 30);
			y = gHitInfo.hity - MulScale(Sin(pSprite->ang), 16, 30);
		}
		else
		{
			x = gHitInfo.hitx - MulScale(pMissileInfo->clipDist << 1, Cos(pSprite->ang), 28);
			y = gHitInfo.hity - MulScale(pMissileInfo->clipDist << 1, Sin(pSprite->ang), 28);
		}
	}
	auto spawned = actSpawnSprite(pSprite->sectnum, x, y, z, 5, 1);
	spritetype* pMissile = &spawned->s();
	pMissile->cstat2 |= CSTAT2_SPRITE_MAPPED;
	pMissile->type = nType;
	pMissile->shade = pMissileInfo->shade;
	pMissile->pal = 0;
	pMissile->clipdist = pMissileInfo->clipDist;
	pMissile->flags = 1;
	pMissile->xrepeat = pMissileInfo->xrepeat;
	pMissile->yrepeat = pMissileInfo->yrepeat;
	pMissile->picnum = pMissileInfo->picnum;
	pMissile->ang = (pSprite->ang + pMissileInfo->angleOfs) & 2047;
	spawned->xvel() = MulScale(pMissileInfo->velocity, a4, 14);
	spawned->yvel() = MulScale(pMissileInfo->velocity, a5, 14);
	spawned->zvel() = MulScale(pMissileInfo->velocity, a6, 14);
	spawned->SetOwner(actor);
	pMissile->cstat |= 1;
	spawned->SetTarget(nullptr);
	evPostActor(spawned, 600, kCallbackRemove);

	actBuildMissile(spawned, actor);

	if (v4)
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
	spritetype* pSprite = &actor->s();
	if (!actor->hasX()) return -1;

	XSPRITE* pXSprite = &actor->x();

	if (actor->IsDudeActor() && !actor->IsPlayerActor())
	{
		if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nMonsterSettings == 2))
			return gGameOptions.nMonsterRespawnTime;
		return -1;
	}

	if (actor->IsWeaponActor())
	{
		if (pXSprite->respawn == 3 || gGameOptions.nWeaponSettings == 1) return 0;
		else if (pXSprite->respawn != 1 && gGameOptions.nWeaponSettings != 0)
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}

	if (actor->IsAmmoActor())
	{
		if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nWeaponSettings != 0))
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}

	if (actor->IsItemActor())
	{
		if (pXSprite->respawn == 3 && gGameOptions.nGameType == 1) return 0;
		else if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nItemSettings != 0))
		{
			switch (pSprite->type)
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
	spritetype* pSprite = &actor->s();
	if (actor->hasX())
	{
		XSPRITE* pXSprite = &actor->x();
		int nRespawnTime = actGetRespawnTime(actor);
		if (nRespawnTime < 0) return 0;

		pXSprite->respawnPending = 1;
		if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
		{
			pXSprite->respawnPending = 3;
			if (pSprite->type == kThingTNTBarrel) pSprite->cstat |= 32768;
		}
		if (nRespawnTime > 0)
		{
			if (pXSprite->respawnPending == 1) nRespawnTime = MulScale(nRespawnTime, 0xa000, 16);
			pSprite->owner = pSprite->statnum;
			actPostSprite(actor, kStatRespawn);
			pSprite->flags |= kHitagRespawn;

			if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
			{
				pSprite->cstat &= ~257;
				pSprite->x = actor->basePoint().x;
				pSprite->y = actor->basePoint().y;
				pSprite->z = actor->basePoint().z;
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

bool actCanSplatWall(int nWall)
{
	assert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall = &wall[nWall];
	if (pWall->cstat & 16384) return 0;
	if (pWall->cstat & 32768) return 0;

	int nType = GetWallType(nWall);
	if (nType >= kWallBase && nType < kWallMax) return 0;

	if (pWall->nextsector != -1)
	{
		sectortype* pSector = &sector[pWall->nextsector];
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
	auto pShooter = &shooter->s();
	assert(vectorType >= 0 && vectorType < kVectorMax);
	const VECTORDATA* pVectorData = &gVectorData[vectorType];
	int nRange = pVectorData->maxDist;
	int hit = VectorScan(pShooter, a2, a3, a4, a5, a6, nRange, 1);
	if (hit == 3)
	{
		auto hitactor = gHitInfo.hitactor;
		assert(hitactor != nullptr);
		spritetype* pSprite = &hitactor->s();
		if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pShooter, pSprite)) return;
		if (IsPlayerSprite(pSprite))
		{
			PLAYER* pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
			if (powerupCheck(pPlayer, kPwUpReflectShots))
			{
				gHitInfo.hitactor = shooter;
				gHitInfo.hitx = pShooter->x;
				gHitInfo.hity = pShooter->y;
				gHitInfo.hitz = pShooter->z;
			}
		}
	}
	int x = gHitInfo.hitx - MulScale(a4, 16, 14);
	int y = gHitInfo.hity - MulScale(a5, 16, 14);
	int z = gHitInfo.hitz - MulScale(a6, 256, 14);
	short nSector = gHitInfo.hitsect;
	uint8_t nSurf = kSurfNone;
	if (nRange == 0 || approxDist(gHitInfo.hitx - pShooter->x, gHitInfo.hity - pShooter->y) < nRange)
	{
		switch (hit)
		{
		case 1:
		{
			int nSector = gHitInfo.hitsect;
			if (sector[nSector].ceilingstat & 1)
				nSurf = kSurfNone;
			else
				nSurf = surfType[sector[nSector].ceilingpicnum];
			break;
		}
		case 2:
		{
			int nSector = gHitInfo.hitsect;
			if (sector[nSector].floorstat & 1)
				nSurf = kSurfNone;
			else
				nSurf = surfType[sector[nSector].floorpicnum];
			break;
		}
		case 0:
		{
			int nWall = gHitInfo.hitwall;
			assert(nWall >= 0 && nWall < kMaxWalls);
			nSurf = surfType[wall[nWall].picnum];
			if (actCanSplatWall(nWall))
			{
				int x = gHitInfo.hitx - MulScale(a4, 16, 14);
				int y = gHitInfo.hity - MulScale(a5, 16, 14);
				int z = gHitInfo.hitz - MulScale(a6, 256, 14);
				int nSurf = surfType[wall[nWall].picnum];
				assert(nSurf < kSurfMax);
				if (pVectorData->surfHit[nSurf].fx1 >= 0)
				{
					auto pFX = gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx1, nSector, x, y, z, 0);
					if (pFX)
					{
						pFX->s().ang = (GetWallAngle(nWall) + 512) & 2047;
						pFX->s().cstat |= 16;
					}
				}
			}
			break;
		}
		case 4:
		{
			int nWall = gHitInfo.hitwall;
			assert(nWall >= 0 && nWall < kMaxWalls);
			nSurf = surfType[wall[nWall].overpicnum];
			int nXWall = wall[nWall].extra;
			if (nXWall > 0)
			{
				XWALL* pXWall = &xwall[nXWall];
				if (pXWall->triggerVector)
					trTriggerWall(nWall, pXWall, kCmdWallImpact);
			}
			break;
		}
		case 3:
		{
			auto actor = gHitInfo.hitactor;
			spritetype* pSprite = &actor->s();
			nSurf = surfType[pSprite->picnum];
			x -= MulScale(a4, 112, 14);
			y -= MulScale(a5, 112, 14);
			z -= MulScale(a6, 112 << 4, 14);
			int shift = 4;
			if (vectorType == kVectorTine && !actor->IsPlayerActor()) shift = 3;

			actDamageSprite(shooter, actor, pVectorData->dmgType, pVectorData->dmg << shift);
			if (actor->hasX() && actor->x().Vector) trTriggerSprite(actor, kCmdSpriteImpact);

			if (pSprite->statnum == kStatThing)
			{
				int t = thingInfo[pSprite->type - kThingBase].mass;
				if (t > 0 && pVectorData->impulse)
				{
					int t2 = DivScale(pVectorData->impulse, t, 8);
					actor->xvel() += MulScale(a4, t2, 16);
					actor->yvel() += MulScale(a5, t2, 16);
					actor->zvel() += MulScale(a6, t2, 16);
				}
				if (pVectorData->burnTime)
				{
					if (!actor->x().burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
					actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
				}
			}
			if (pSprite->statnum == kStatDude && actor->hasX())
			{
				int t = getDudeInfo(pSprite->type)->mass;

#ifdef NOONE_EXTENSIONS
				if (actor->IsDudeActor())
				{
					switch (pSprite->type)
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
					actor->xvel() += MulScale(a4, t2, 16);
					actor->yvel() += MulScale(a5, t2, 16);
					actor->zvel() += MulScale(a6, t2, 16);
				}
				if (pVectorData->burnTime)
				{
					if (!actor->x().burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
					actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
				}
				if (Chance(pVectorData->fxChance))
				{
					int t = gVectorData[19].maxDist;
					a4 += Random3(4000);
					a5 += Random3(4000);
					a6 += Random3(4000);
					if (HitScan(pSprite, gHitInfo.hitz, a4, a5, a6, CLIPMASK1, t) == 0)
					{
						if (approxDist(gHitInfo.hitx - pSprite->x, gHitInfo.hity - pSprite->y) <= t)
						{
							int nWall = gHitInfo.hitwall;
							int nSector = gHitInfo.hitsect;
							if (actCanSplatWall(nWall))
							{
								int x = gHitInfo.hitx - MulScale(a4, 16, 14);
								int y = gHitInfo.hity - MulScale(a5, 16, 14);
								int z = gHitInfo.hitz - MulScale(a6, 16 << 4, 14);
								int nSurf = surfType[wall[nWall].picnum];
								const VECTORDATA* pVectorData = &gVectorData[19];
								FX_ID t2 = pVectorData->surfHit[nSurf].fx2;
								FX_ID t3 = pVectorData->surfHit[nSurf].fx3;

								DBloodActor* pFX = nullptr;
								if (t2 > FX_NONE && (t3 == FX_NONE || Chance(0x4000))) pFX = gFX.fxSpawnActor(t2, nSector, x, y, z, 0);
								else if (t3 > FX_NONE) pFX = gFX.fxSpawnActor(t3, nSector, x, y, z, 0);
								if (pFX)
								{
									pFX->zvel() = 0x2222;
									pFX->s().ang = (GetWallAngle(nWall) + 512) & 2047;
									pFX->s().cstat |= 16;
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
					XSPRITE* pXSprite = &actor->x();
					if (pXSprite->physAttr & kPhysDebrisVector) {

                        int impulse = DivScale(pVectorData->impulse, ClipLow(actor->spriteMass.mass, 10), 6);
						actor->xvel() += MulScale(a4, impulse, 16);
						actor->yvel() += MulScale(a5, impulse, 16);
						actor->zvel() += MulScale(a6, impulse, 16);

						if (pVectorData->burnTime != 0) {
                            if (!pXSprite->burnTime) evPostActor(actor, 0, kCallbackFXFlameLick);
							actBurnSprite(shooter->GetOwner(), actor, pVectorData->burnTime);
						}

						if (pSprite->type >= kThingBase && pSprite->type < kThingMax) {
							pSprite->statnum = kStatThing; // temporary change statnum property
							actDamageSprite(shooter, actor, pVectorData->dmgType, pVectorData->dmg << 4);
							pSprite->statnum = kStatDecoration; // return statnum property back
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

        auto pFX2 = gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx2, nSector, x, y, z, 0);
		if (pFX2 && gModernMap)
			pFX2->SetOwner(shooter);
	}

	if (pVectorData->surfHit[nSurf].fx3 >= 0) {

        auto pFX3 = gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx3, nSector, x, y, z, 0);
		if (pFX3 && gModernMap)
			pFX3->SetOwner(shooter);

	}

#else
	if (pVectorData->surfHit[nSurf].fx2 >= 0)
        gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx2, nSector, x, y, z, 0);
	if (pVectorData->surfHit[nSurf].fx3 >= 0)
        gFX.fxSpawnActor(pVectorData->surfHit[nSurf].fx3, nSector, x, y, z, 0);
#endif

	if (pVectorData->surfHit[nSurf].fxSnd >= 0)
		sfxPlay3DSound(x, y, z, pVectorData->surfHit[nSurf].fxSnd, nSector);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireballSeqCallback(int, DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pFX = gFX.fxSpawnActor(FX_11, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
	if (pFX)
	{
		pFX->xvel() = actor->xvel();
		pFX->yvel() = actor->yvel();
		pFX->zvel() = actor->zvel();
	}
}

void NapalmSeqCallback(int, DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pFX = gFX.fxSpawnActor(FX_12, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
	if (pFX)
	{
		pFX->xvel() = actor->xvel();
		pFX->yvel() = actor->yvel();
		pFX->zvel() = actor->zvel();
	}
}

void Fx32Callback(int, DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pFX = gFX.fxSpawnActor(FX_32, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
	if (pFX)
	{
		pFX->xvel() = actor->xvel();
		pFX->yvel() = actor->yvel();
		pFX->zvel() = actor->zvel();
	}
}

void Fx33Callback(int, DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pFX = gFX.fxSpawnActor(FX_33, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
	if (pFX)
	{
		pFX->xvel() = actor->xvel();
		pFX->yvel() = actor->yvel();
		pFX->zvel() = actor->zvel();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void TreeToGibCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	pSprite->type = kThingObjectExplode;
	pXSprite->state = 1;
	pXSprite->data1 = 15;
	pXSprite->data2 = 0;
	pXSprite->data3 = 0;
	pXSprite->health = thingInfo[17].startHealth;
	pXSprite->data4 = 312;
	pSprite->cstat |= 257;
}

void DudeToGibCallback1(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	pSprite->type = kThingBloodChunks;
	pXSprite->data1 = 8;
	pXSprite->data2 = 0;
	pXSprite->data3 = 0;
	pXSprite->health = thingInfo[26].startHealth;
	pXSprite->data4 = 319;
	pXSprite->triggerOnce = 0;
	pXSprite->isTriggered = 0;
	pXSprite->locked = 0;
	pXSprite->targetX = PlayClock;
	pXSprite->state = 1;
}

void DudeToGibCallback2(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	pSprite->type = kThingBloodChunks;
	pXSprite->data1 = 3;
	pXSprite->data2 = 0;
	pXSprite->data3 = 0;
	pXSprite->health = thingInfo[26].startHealth;
	pXSprite->data4 = 319;
	pXSprite->triggerOnce = 0;
	pXSprite->isTriggered = 0;
	pXSprite->locked = 0;
	pXSprite->targetX = PlayClock;
	pXSprite->state = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void actPostSprite(DBloodActor* actor, int nStatus)
{
	assert(nStatus >= 0 && nStatus <= kStatFree);
	auto sp = &actor->s();
	if (sp->flags & 32)
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
		sp->flags |= 32;
		gPost.Push({ actor, nStatus });
	}
}

void actPostProcess(void)
{
	for (auto& p : gPost)
	{
		p.sprite->s().flags &= ~32;
		int nStatus = p.status;
		if (nStatus == kStatFree)
		{
			if (p.sprite->s().statnum != kStatFree)
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
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	pSprite->flags &= ~2;
	int nXSprite = pSprite->extra;
	pSprite->z -= 4 << 8;
	int nSurface = tileGetSurfType(actor->hit.florhit);
	switch (pSprite->type)
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
	auto pXSprite = &pTarget->x();
	pXSprite->burnTime = ClipHigh(pXSprite->burnTime + nTime, sprite[pXSprite->reference].statnum == kStatDude ? 2400 : 1200);
	pTarget->SetBurnSource(pSource);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, Collision& w, Collision* def)
{
	int empty = 0;
	if (arc.isReading()) w = {};
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type, &empty)
			("index", w.index, &empty)
			("legacyval", w.legacyVal, &empty)
			("actor", w.actor)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, SPRITEHIT& w, SPRITEHIT* def)
{
#ifdef OLD_SAVEGAME
	int empty = 0;
	int hit = w.hit.legacyVal, ceilhit = w.ceilhit.legacyVal, florhit = w.florhit.legacyVal;
	if (arc.isReading()) w = {};
	if (arc.BeginObject(keyname))
	{
		arc("hit", hit, &empty)
			("ceilhit", ceilhit, &empty)
			("florhit", florhit, &empty)
			.EndObject();
		if (arc.isReading())
		{
			w.hit.setFromEngine(hit);
			w.florhit.setFromEngine(florhit);
			w.ceilhit.setFromEngine(ceilhit);
	}
	}
#else
	if (arc.BeginObject(keyname))
	{
        arc("hit", w.hit)
            ("ceilhit", w.ceilhit)
            ("florhit", w.florhit)
			.EndObject();
	}
#endif
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

// dumping ground for temporary wrappers.

void HITINFO::set(hitdata_t* hit)
{
	hitsect = hit->sect;
	hitwall = hit->wall;
	hitactor = hit->sprite >= 0 ? &bloodActors[hit->sprite] : nullptr;
	hitx = hit->pos.x;
	hity = hit->pos.y;
	hitz = hit->pos.z;
}


char SetSpriteState(DBloodActor* actor, int nState)
{
	return SetSpriteState(actor->GetIndex(), &actor->x(), nState);
}




END_BLD_NS
