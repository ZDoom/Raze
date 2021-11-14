//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "serializer.h"
#include "names.h"
#include "build.h"
#include "gamevar.h"
#include "mapinfo.h"

// This currently only works for WW2GI.
#include "names_d.h"

BEGIN_DUKE_NS


MATTGAMEVAR aGameVars[MAXGAMEVARS];
int iGameVarCount;

extern int errorcount, warningcount, line_count;

//intptr_t *actorLoadEventScrptr[MAXTILES];
intptr_t apScriptGameEvent[MAXGAMEEVENTS];

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeGameVars(FSerializer &arc)
{
	if (arc.BeginObject("gamevars"))
	{
		// Only save the ones which hold their own data, i.e. skip pointer variables.
		for (auto& gv : aGameVars)
		{
			if (!(gv.dwFlags & (GAMEVAR_FLAG_PLONG|GAMEVAR_FLAG_PFUNC)))
			{
				if (arc.BeginObject(gv.szLabel))
				{
					arc("value", gv.lValue);
					if (gv.dwFlags & (GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_PERACTOR))
					{
						arc("array", gv.plArray);
					}
					arc.EndObject();
				}
			}
		}
		arc.EndObject();
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int AddGameVar(const char* pszLabel, intptr_t lValue, unsigned dwFlags)
{

	int i;
	int j;

	if (dwFlags & (GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_PFUNC))
		dwFlags |= GAMEVAR_FLAG_SYSTEM;	// force system if PLONG

	for (i = 0; i < iGameVarCount; i++)
	{
		if (strcmp(pszLabel, aGameVars[i].szLabel) == 0)
		{
			// found it...
			if ((aGameVars[i].dwFlags & GAMEVAR_FLAG_DEFAULT)
				|| (aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM)
				)
			{
				// it's OK to replace
				break;
			}
			else return -1;
		}
	}
	if (i < MAXGAMEVARS)
	{
		// Set values
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM && !(dwFlags & (GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_PFUNC)))
		{
			// if existing is system, they only get to change default value....
			aGameVars[i].lValue = (int)lValue;
			if (!(dwFlags & GAMEVAR_FLAG_NODEFAULT))
			{
				aGameVars[i].defaultValue = (int)lValue;
			}
		}
		else
		{
			strcpy(aGameVars[i].szLabel, pszLabel);
			aGameVars[i].dwFlags = dwFlags;
			if (dwFlags & (GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_PFUNC))
			{
				aGameVars[i].plValue = (int*)lValue;
			}
			else
			{
				aGameVars[i].lValue = (int)lValue;
			}
			if (!(dwFlags & GAMEVAR_FLAG_NODEFAULT))
			{
				aGameVars[i].defaultValue = (int)lValue;
			}
		}

		if (i == iGameVarCount)
		{
			// we're adding a new one.
			iGameVarCount++;
		}
		if (!(aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM))
		{
			// only free if not system
			aGameVars[i].plArray.Reset();
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
		{
			aGameVars[i].plArray.Resize(MAXPLAYERS);
			for (j = 0; j < MAXPLAYERS; j++)
			{
				aGameVars[i].plArray[j] = (int)lValue;
			}
		}
		else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
		{
			aGameVars[i].plArray.Resize(MAXSPRITES);
			for (j = 0; j < MAXSPRITES; j++)
			{
				aGameVars[i].plArray[j] = (int)lValue;
			}
		}
		return 1;
	}
	else
	{
		// no room to add...
		return -2;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetGameID(const char *szGameLabel)
{
	int i;
	for(i=0;i<iGameVarCount;i++)
	{
		if( strcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
		{
			return i;
		}
	}
	return -1;	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetDefID(const char *szGameLabel)
{
	int i;
	for(i=0;i<iGameVarCount;i++)
	{
		if( strcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
		{
			return i;
		}
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ClearGameVars(void)
{
	// only call this function ONCE (at game init)...
	int i;

	for (i = 0; i < MAXGAMEVARS; i++)
	{
		aGameVars[i].plValue = nullptr;
		aGameVars[i].szLabel[0] = 0;
		aGameVars[i].dwFlags = 0;
	}
	iGameVarCount=0;
	return;
}
//---------------------------------------------------------------------------
//
//  I think the best way to describe the original code here is 
//  "utterly broken by design" ...
//  I hope this version is saner, there's really no need to tear down and
//  rebuild the complete set of game vars just to reset them to the defaults...
//
//---------------------------------------------------------------------------

void ResetGameVars(void)
{
	int i;

	for(i=0;i<iGameVarCount;i++)
	{
		if (!(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_PFUNC)))
		{
			if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_PERACTOR))
			{
				for (auto& v : aGameVars[i].plArray)
				{
					v = aGameVars[i].defaultValue;
				}
			}
			else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
			{
				aGameVars[i].lValue = aGameVars[i].defaultValue;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetGameVarID(int id, DDukeActor* sActor, int sPlayer)
{
	if(id<0 || id >= iGameVarCount)
	{
		Printf("GetGameVarID: Invalid Game ID %d\n", id);
		return -1;
	}
	if (id == g_iThisActorID)
	{
		return sActor->GetSpriteIndex();
	}
	if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER )
	{
		// for the current player
		if(sPlayer >=0 && sPlayer < MAXPLAYERS)
		{
			return aGameVars[id].plArray[sPlayer];
		}
		else
		{
			return aGameVars[id].lValue;
		}
	}
	else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR )
	{
		// for the current actor
		if(sActor != nullptr)
		{
			return aGameVars[id].plArray[sActor->GetSpriteIndex()];
		}
		else
		{
			return aGameVars[id].lValue;
		}
	}
	else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG )
	{
		if( !aGameVars[id].plValue)
		{
			Printf("GetGameVarID NULL PlValues for PLONG Var=%s\n",aGameVars[id].szLabel);
		}

		return	*aGameVars[id].plValue;
	}
	else if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PFUNC)
	{
		if (!aGameVars[id].plValue)
		{
			Printf("GetGameVarID NULL PlValues for PFUNC Var=%s\n", aGameVars[id].szLabel);
		}

		return	aGameVars[id].getter();
	}
	else
	{
		return aGameVars[id].lValue;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetGameVarID(int id, int lValue, DDukeActor* sActor, int sPlayer)
{
	if(id<0 || id >= iGameVarCount)
	{
		Printf("Invalid Game ID %d\n", id);
		return;
	}
	if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER )
	{
		// for the current player
		if (sPlayer >= 0) aGameVars[id].plArray[sPlayer] = lValue;
		else for (auto& i : aGameVars[id].plArray) i = lValue; // -1 sets all players - was undefined OOB access in WW2GI.
	}
	else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR )
	{
		// for the current actor
		if (sActor != nullptr) aGameVars[id].plArray[sActor->GetSpriteIndex()]=lValue;
		else for (auto& i : aGameVars[id].plArray) i = lValue; // -1 sets all actors - was undefined OOB access in WW2GI.
	}
	else if( aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG )
	{
		// set the value at pointer
		*aGameVars[id].plValue=lValue;
	}
	else if( !(aGameVars[id].dwFlags & GAMEVAR_FLAG_PFUNC) )
	{
		aGameVars[id].lValue=lValue;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GetGameVar(const char *szGameLabel, int lDefault, DDukeActor* sActor, int sPlayer)
{
	for (int i = 0; i < iGameVarCount; i++)
	{
		if (strcmp(szGameLabel, aGameVars[i].szLabel) == 0)
		{
			return GetGameVarID(i, sActor, sPlayer);
		}
	}
	return lDefault;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int *GetGameValuePtr(char *szGameLabel)
{
	int i;
	for(i=0;i<iGameVarCount;i++)
	{
		if( strcmp(szGameLabel, aGameVars[i].szLabel) == 0 )
		{
			if(aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR | GAMEVAR_FLAG_PERPLAYER))
			{
				if(aGameVars[i].plArray.Size() == 0)
				{
					Printf("INTERNAL ERROR: NULL array !!!\n");
				}
				return aGameVars[i].plArray.Data();
			}
			return &(aGameVars[i].lValue);
		}
	}
	return NULL;
	
}

//---------------------------------------------------------------------------
//
//  Event stuff
//
//---------------------------------------------------------------------------

void ClearGameEvents()
{
	int i;
	for (i=0;i<MAXGAMEEVENTS;i++)
	{
		apScriptGameEvent[i]=(intptr_t)0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool IsGameEvent(int i)
{
	if (i<0) return 0;
	if (i>=MAXGAMEEVENTS) return 0;
	return (apScriptGameEvent[i] != 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int *aplWeaponClip[MAX_WEAPONS];		// number of items in clip
int *aplWeaponReload[MAX_WEAPONS];		// delay to reload (include fire)
int *aplWeaponFireDelay[MAX_WEAPONS];	// delay to fire
int *aplWeaponHoldDelay[MAX_WEAPONS];	// delay after release fire button to fire (0 for none)
int *aplWeaponTotalTime[MAX_WEAPONS];	// The total time the weapon is cycling before next fire.
int *aplWeaponFlags[MAX_WEAPONS];		// Flags for weapon
int *aplWeaponShoots[MAX_WEAPONS];		// what the weapon shoots
int *aplWeaponSpawnTime[MAX_WEAPONS];	// the frame at which to spawn an item
int *aplWeaponSpawn[MAX_WEAPONS];		// the item to spawn
int *aplWeaponShotsPerBurst[MAX_WEAPONS];	// number of shots per 'burst' (one ammo per 'burst'
int *aplWeaponWorksLike[MAX_WEAPONS];	// What original the weapon works like
int *aplWeaponInitialSound[MAX_WEAPONS];	// Sound made when initialy firing. zero for no sound
int *aplWeaponFireSound[MAX_WEAPONS];	// Sound made when firing (each time for automatic)
int *aplWeaponSound2Time[MAX_WEAPONS];	// Alternate sound time
int *aplWeaponSound2Sound[MAX_WEAPONS];	// Alternate sound sound ID

int g_iReturnVarID = -1;	// var ID of "RETURN"
int g_iWeaponVarID = -1;	// var ID of "WEAPON"
int g_iWorksLikeVarID = -1;	// var ID of "WORKSLIKE"
int g_iZRangeVarID = -1;	// var ID of "ZRANGE"
int g_iAngRangeVarID = -1;	// var ID of "ANGRANGE"
int g_iAimAngleVarID = -1;	// var ID of "AUTOAIMANGLE"
int g_iAtWithVarID = -1;	// var ID of "AtWith"
int g_iLoTagID = -1;			// var ID of "LOTAG"
int g_iHiTagID = -1;			// ver ID of "HITAG"
int g_iTextureID = -1;		// var ID of "TEXTURE"
int g_iThisActorID = -1;		// var ID of "THISACTOR"

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitGameVarPointers(void)
{
	int i;
	char aszBuf[64];
	// called from game Init AND when level is loaded...

	for(i=0;i<12/*MAX_WEAPONS*/;i++)	// Setup only exists for the original 12 weapons.
	{
		sprintf(aszBuf,"WEAPON%d_CLIP",i);
		aplWeaponClip[i]=GetGameValuePtr(aszBuf);
		if(!aplWeaponClip[i])
		{
			I_FatalError("ERROR: NULL Weapon\n");
		}
		sprintf(aszBuf,"WEAPON%d_RELOAD",i);
		aplWeaponReload[i]=GetGameValuePtr(aszBuf);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
		aplWeaponFireDelay[i]=GetGameValuePtr(aszBuf);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
		aplWeaponTotalTime[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
		aplWeaponHoldDelay[i]=GetGameValuePtr(aszBuf);

		sprintf(aszBuf,"WEAPON%d_FLAGS",i);
		aplWeaponFlags[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",i);
		aplWeaponShoots[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
		aplWeaponSpawnTime[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",i);
		aplWeaponSpawn[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
		aplWeaponShotsPerBurst[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
		aplWeaponWorksLike[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
		aplWeaponInitialSound[i]=GetGameValuePtr(aszBuf);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
		aplWeaponFireSound[i]=GetGameValuePtr(aszBuf);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
		aplWeaponSound2Time[i]=GetGameValuePtr(aszBuf);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
		aplWeaponSound2Sound[i]=GetGameValuePtr(aszBuf);
	
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

// These are deliberately not stored in accessible variables anymore. Use is deprecated.
int getmap() { return currentLevel->levelNumber; }
int getvol() { return currentLevel->cluster; }

void AddSystemVars()
{
	// only call ONCE
	char aszBuf[64];

/////////////////////////////		
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",KNEE_WEAPON);
		AddGameVar(aszBuf, KNEE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",KNEE_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",KNEE_WEAPON);
		AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",KNEE_WEAPON);
		AddGameVar(aszBuf, 14, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",KNEE_WEAPON);
		AddGameVar(aszBuf, 14, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",KNEE_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_NOVISIBLE | WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_RANDOMRESTART, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",KNEE_WEAPON);
		AddGameVar(aszBuf, KNEE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",KNEE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",PISTOL_WEAPON);
		AddGameVar(aszBuf, PISTOL_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",PISTOL_WEAPON);
		AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_RELOAD",PISTOL_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIREDELAY",PISTOL_WEAPON);
		AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",PISTOL_WEAPON);
		AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",PISTOL_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",PISTOL_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_HOLSTER_CLEARS_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",PISTOL_WEAPON);
		AddGameVar(aszBuf, SHOTSPARK1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",PISTOL_WEAPON);
		AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",PISTOL_WEAPON);
		AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",PISTOL_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",PISTOL_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",PISTOL_WEAPON);
		AddGameVar(aszBuf, PISTOL_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",PISTOL_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",PISTOL_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, SHOTGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 13, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 31, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_CHECKATRELOAD, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, SHOTGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 24, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, SHOTGUNSHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, SHOTGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, 15, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHOTGUN_WEAPON);
		AddGameVar(aszBuf, SHOTGUN_COCK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, CHAINGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_FIREEVERYTHIRD | WEAPON_FLAG_AMMOPERSHOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_SHOOTS",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, CHAINGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, CHAINGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",CHAINGUN_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",RPG_WEAPON);
		AddGameVar(aszBuf, RPG_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",RPG_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",RPG_WEAPON);
		AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",RPG_WEAPON);
		AddGameVar(aszBuf, 20, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",RPG_WEAPON);
		AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",RPG_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, HANDBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 6, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 19, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_THROWIT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, HEAVYHBOMB, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHRINKER_WEAPON);
		AddGameVar(aszBuf, SHRINKER_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_TOTALTIME",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",SHRINKER_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",SHRINKER_WEAPON);
		AddGameVar(aszBuf, SHRINKER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHRINKER_WEAPON);
		AddGameVar(aszBuf, SHRINKER_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHRINKER_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, DEVISTATOR_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_FIREEVERYOTHER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",DEVISTATOR_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, TRIPBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 16, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_STANDSTILL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, HANDHOLDINGLASER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",TRIPBOMB_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",FREEZE_WEAPON);
		AddGameVar(aszBuf, FREEZE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",FREEZE_WEAPON);
		AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",FREEZE_WEAPON);
		AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",FREEZE_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_FIREEVERYOTHER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",FREEZE_WEAPON);
		AddGameVar(aszBuf, FREEZEBLAST, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",FREEZE_WEAPON);
		AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",FREEZE_WEAPON);
		AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",FREEZE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
/////////////////////////////
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, HANDREMOTE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_BOMB_TRIGGER | WEAPON_FLAG_NOVISIBLE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDREMOTE_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

///////////////////////////////////////////////////////		
		sprintf(aszBuf,"WEAPON%d_WORKSLIKE",GROW_WEAPON);
		AddGameVar(aszBuf, GROW_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_CLIP",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_RELOAD",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FIREDELAY",GROW_WEAPON);
		AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_TOTALTIME",GROW_WEAPON);
		AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);


		sprintf(aszBuf,"WEAPON%d_HOLDDELAY",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_FLAGS",GROW_WEAPON);
		AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOOTS",GROW_WEAPON);
		AddGameVar(aszBuf, GROWSPARK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWNTIME",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SPAWN",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_INITIALSOUND",GROW_WEAPON);
		AddGameVar(aszBuf, EXPANDERSHOOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
		
		sprintf(aszBuf,"WEAPON%d_FIRESOUND",GROW_WEAPON);
		AddGameVar(aszBuf, EXPANDERSHOOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2TIME",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

		sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",GROW_WEAPON);
		AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

	AddGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

	AddGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

	AddGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

	
	AddGameVar("WEAPON", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("WORKSLIKE", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("RETURN", 0, GAMEVAR_FLAG_SYSTEM);
	AddGameVar("ZRANGE", 0, GAMEVAR_FLAG_SYSTEM);
	AddGameVar("ANGRANGE", 0, GAMEVAR_FLAG_SYSTEM);
	AddGameVar("AUTOAIMANGLE", 0, GAMEVAR_FLAG_SYSTEM);
	AddGameVar("LOTAG", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("HITAG", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("TEXTURE", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("THISACTOR", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
	AddGameVar("ATWITH", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);

	AddGameVar("RESPAWN_MONSTERS", (intptr_t)&ud.respawn_monsters,GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("RESPAWN_ITEMS",(intptr_t)&ud.respawn_items, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("RESPAWN_INVENTORY",(intptr_t)&ud.respawn_inventory, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("MONSTERS_OFF",(intptr_t)&ud.monsters_off, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("MARKER",(intptr_t)&ud.marker, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("FFIRE",(intptr_t)&ud.ffire, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("LEVEL", (intptr_t)getmap, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PFUNC | GAMEVAR_FLAG_READONLY);
	AddGameVar("VOLUME",(intptr_t)getvol, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PFUNC | GAMEVAR_FLAG_READONLY);

	AddGameVar("COOP",(intptr_t)&ud.coop, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
	AddGameVar("MULTIMODE",(intptr_t)&ud.multimode, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);

}

void ResetSystemDefaults(void)
{
	// call many times...
	
	int i,j;
	char aszBuf[64];

	for(j=0;j<MAXPLAYERS;j++)
	{
		for(i=0;i<12/*MAX_WEAPONS*/;i++)
		{
			sprintf(aszBuf,"WEAPON%d_CLIP",i);
			aplWeaponClip[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_RELOAD",i);
			aplWeaponReload[i][j]=GetGameVar(aszBuf,0, nullptr, j);

			sprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
			aplWeaponFireDelay[i][j]=GetGameVar(aszBuf,0, nullptr, j);

			sprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
			aplWeaponTotalTime[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
			aplWeaponHoldDelay[i][j]=GetGameVar(aszBuf,0, nullptr, j);

			sprintf(aszBuf,"WEAPON%d_FLAGS",i);
			aplWeaponFlags[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_SHOOTS",i);
			aplWeaponShoots[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
			aplWeaponSpawnTime[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_SPAWN",i);
			aplWeaponSpawn[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
			aplWeaponShotsPerBurst[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
			aplWeaponWorksLike[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
			aplWeaponInitialSound[i][j]=GetGameVar(aszBuf,0, nullptr, j);
		
			sprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
			aplWeaponFireSound[i][j]=GetGameVar(aszBuf,0, nullptr, j);

			sprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
			aplWeaponSound2Time[i][j]=GetGameVar(aszBuf,0, nullptr, j);

			sprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
			aplWeaponSound2Sound[i][j]=GetGameVar(aszBuf,0, nullptr, j);
	
		}
	}

	g_iReturnVarID=GetGameID("RETURN");
	g_iWeaponVarID=GetGameID("WEAPON");
	g_iWorksLikeVarID=GetGameID("WORKSLIKE");
	g_iZRangeVarID=GetGameID("ZRANGE");
	g_iAngRangeVarID=GetGameID("ANGRANGE");
	g_iAimAngleVarID=GetGameID("AUTOAIMANGLE");
	g_iAtWithVarID = GetGameID("ATWITH");
	g_iLoTagID = GetGameID("LOTAG");
	g_iHiTagID = GetGameID("HITAG");
	g_iTextureID = GetGameID("TEXTURE");
	g_iThisActorID = GetGameID("THISACTOR");
}
	

END_DUKE_NS
