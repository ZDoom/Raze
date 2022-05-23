/*
** savegame.cpp
**
** common savegame utilities for all front ends.
** 
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "savegamehelp.h"
#include "gstrings.h"
#include "i_specialpaths.h"
#include "cmdlib.h"
#include "filesystem.h"
#include "statistics.h"
#include "secrets.h"
#include "quotemgr.h"
#include "mapinfo.h"
#include "v_video.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "serializer.h"
#include "version.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "gamestruct.h"
#include "automap.h"
#include "statusbar.h"
#include "gamestate.h"
#include "razemenu.h"
#include "interpolate.h"
#include "gamefuncs.h"
#include "render.h"
#include "hw_sections.h"
#include "sectorgeometry.h"
#include "d_net.h"
#include "ns.h"
#include "serialize_obj.h"
#include "games/blood/src/mapstructs.h"
#include <zlib.h>


void WriteSavePic(FileWriter* file, int width, int height);
bool WriteZip(const char* filename, TArray<FString>& filenames, TArray<FCompressedBuffer>& content);
extern FString savename;
extern FString BackupSaveGame;
int SaveVersion;

void SerializeMap(FSerializer &arc);

CVAR(String, cl_savedir, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

BEGIN_BLD_NS
FSerializer& Serialize(FSerializer& arc, const char* keyname, XWALL& w, XWALL* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, XSECTOR& w, XSECTOR* def);
END_BLD_NS

//=============================================================================
//
//
//
//=============================================================================

static void SerializeGlobals(FSerializer& arc)
{
	if (arc.BeginObject("globals"))
	{
		arc("crouch_toggle", crouch_toggle)
		.EndObject();
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void SerializeSession(FSerializer& arc)
{
	arc.ReadObjects(false);
	SerializeMap(arc);
	SerializeStatistics(arc);
	SECRET_Serialize(arc);
	Mus_Serialize(arc);
	quoteMgr.Serialize(arc);
	S_SerializeSounds(arc);
	SerializeAutomap(arc);
	SerializeHud(arc);
	SerializeGlobals(arc);
	gi->SerializeGameState(arc);
}

//=============================================================================
//
//
//
//=============================================================================

bool ReadSavegame(const char* name)
{
	auto savereader = FResourceFile::OpenResourceFile(name, true, true);

	if (savereader != nullptr)
	{
		auto lump = savereader->FindLump("info.json");
		if (!lump)
		{
			delete savereader;
			return false;
		}
		auto file = lump->NewReader();
		if (G_ValidateSavegame(file, nullptr, false) <= 0)
		{
			delete savereader;
			return false;
		}
		file.Close();

		FResourceLump* info = savereader->FindLump("session.json");
		if (info == nullptr)
		{
			delete savereader;
			return false;
		}

		void* data = info->Lock();
		FSerializer arc;
		if (!arc.OpenReader((const char*)data, info->LumpSize))
		{
			info->Unlock();
			delete savereader;
			return false;
		}

		// Load the savegame.
		loadMapBackup(currentLevel->fileName);
		SerializeSession(arc);
		g_nextskill = gi->GetCurrentSkill();
		arc.Close();
		info->Unlock();
		delete savereader;
		ResetStatusBar();
		return true;
	}
	return false;
}

CVAR(Bool, save_formatted, false, 0)	// should be set to false once the conversion is done

//=============================================================================
//
// Creates the savegame and writes all cross-game content.
//
//=============================================================================

bool WriteSavegame(const char* filename, const char *name)
{
	BufferWriter savepic;
	FSerializer savegameinfo;		// this is for displayable info about the savegame.
	FSerializer savegamesession;	// saved game session settings.

	char buf[100];
	mysnprintf(buf, countof(buf), GAMENAME " %s", GetVersionString());
	auto savesig = gi->GetSaveSig();
	auto gs = gi->getStats();
	FStringf timeStr("%02d:%02d", gs.timesecnd / 60, gs.timesecnd % 60);
	auto lev = currentLevel;

	savegameinfo.OpenWriter(true);
	savegameinfo.AddString("Software", buf)
		("Save Version", savesig.currentsavever)
		.AddString("Engine", savesig.savesig)
		.AddString("Game Resource", fileSystem.GetResourceFileName(1))
		.AddString("Map Name", lev->DisplayName())
		.AddString("Creation Time", myasctime())
		.AddString("Title", name)
		.AddString("Map File", lev->fileName)
		.AddString("Map Label", lev->labelName)
		.AddString("Map Time", timeStr);

	const char *fn = currentLevel->fileName;
	if (*fn == '/') fn++;
	if (strncmp(fn, "file://", 7) != 0) // this only has meaning for non-usermaps
	{
		auto fileno = fileSystem.FindFile(fn);
		auto mapfile = fileSystem.GetFileContainer(fileno);
		auto mapcname = fileSystem.GetResourceFileName(mapfile);
		if (mapcname) savegameinfo.AddString("Map Resource", mapcname);
		else
		{
			return false; // this should never happen. Saving on a map that isn't present is impossible.
		}
	}



	// Save the game state
	savegamesession.OpenWriter(save_formatted);
	SerializeSession(savegamesession);

	WriteSavePic(&savepic, 240, 180);
	mysnprintf(buf, countof(buf), GAMENAME " %s", GetVersionString());
	// put some basic info into the PNG so that this isn't lost when the image gets extracted.
	M_AppendPNGText(&savepic, "Software", buf);
	M_AppendPNGText(&savepic, "Title", name);
	M_AppendPNGText(&savepic, "Current Map", lev->labelName);
	M_FinishPNG(&savepic);

	auto picdata = savepic.GetBuffer();
	FCompressedBuffer bufpng = { picdata->Size(), picdata->Size(), METHOD_STORED, 0, static_cast<unsigned int>(crc32(0, &(*picdata)[0], picdata->Size())), (char*)&(*picdata)[0] };

	TArray<FCompressedBuffer> savegame_content;
	TArray<FString> savegame_filenames;

	savegame_content.Push(bufpng);
	savegame_filenames.Push("savepic.png");
	savegame_content.Push(savegameinfo.GetCompressedOutput());
	savegame_filenames.Push("info.json");
	savegame_content.Push(savegamesession.GetCompressedOutput());
	savegame_filenames.Push("session.json");

	if (WriteZip(filename, savegame_filenames, savegame_content))
	{
		// Check whether the file is ok by trying to open it.
		FResourceFile* test = FResourceFile::OpenResourceFile(filename, true);
		if (test != nullptr)
		{
			delete test;
	return true;
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSingleFile (const char *name, bool &printRequires, bool printwarn)
{
	if (name == NULL)
	{
		return true;
	}
	if (strncmp(name, "file://", 7) == 0)
	{
		return FileExists(name + 7);	// User maps  must be present to be validated.
	}
	if (fileSystem.CheckIfResourceFileLoaded(name) < 0)
	{
		if (printwarn)
		{
			if (!printRequires)
			{
				Printf ("%s:\n%s", GStrings("TXT_SAVEGAMENEEDS"), name);
			}
			else
			{
				Printf (", %s", name);
			}
		}
		printRequires = true;
		return false;
	}
	return true;
}

//=============================================================================
//
// Return false if not all the needed wads have been loaded.
//
//=============================================================================

static bool G_CheckSaveGameWads (const char *gamegrp, const char *mapgrp, bool printwarn)
{
	bool printRequires = false;
	CheckSingleFile (gamegrp, printRequires, printwarn);
	CheckSingleFile (mapgrp, printRequires, printwarn);

	if (printRequires)
	{
		if (printwarn)
		{
			Printf ("\n");
		}
		return false;
	}

	return true;
}

//=============================================================================
//
// Checks if the savegame is valid. Gets a reader to the included info.json
// Returns 1 if valid, 0 if invalid and -1 if old and -2 if content missing
//
//=============================================================================

int G_ValidateSavegame(FileReader &fr, FString *savetitle, bool formenu)
{
	auto data = fr.Read();
	FSerializer arc;
	if (!arc.OpenReader((const char*)data.Data(), data.Size()))
	{
		return -2;
	}

	int savever;
	FString engine, gamegrp, mapgrp, title, filename, label;

	arc("Save Version", savever)
		("Engine", engine)
		("Game Resource", gamegrp)
		("Map Resource", mapgrp)
		("Title", title)
		("Map Label", label)
		("Map File", filename);

	auto savesig = gi->GetSaveSig();

	if (savetitle) *savetitle = title;
	if (engine.Compare(savesig.savesig) != 0 || savever > savesig.currentsavever)
	{
		// different engine or newer version:
		// not our business. Leave it alone.
		return 0;
	}
	SaveVersion = savesig.currentsavever;

	MapRecord *curLevel = FindMapByName(label);

	// If the map does not exist, check if it's a user map.
	if (!curLevel)
	{
		curLevel = AllocateMap();
		if (!formenu)
		{
			curLevel->name = "";
			curLevel->SetFileName(filename);
		}
	}
	if (!curLevel) return 0;
	if (!formenu) currentLevel = curLevel;


	if (savever < savesig.minsavever)
	{
		// old, incompatible savegame. List as not usable.
		return -1;
	}
	else
	{
		auto ggfn = ExtractFileBase(fileSystem.GetResourceFileName(1), true);
		if (gamegrp.CompareNoCase(ggfn) == 0)
		{
			return G_CheckSaveGameWads(gamegrp, mapgrp, false) ? 1 : -2;
		}
		else
		{
			// different game. Skip this.
			return 0;
		}
	}
	return 0;
}

//=============================================================================
//
//
//
//=============================================================================

FString G_BuildSaveName (const char *prefix)
{
	FString name;
	bool usefilter;

	if (const char *const dir = Args->CheckValue("-savedir"))
	{
		name = dir;
		usefilter = false;
	}
	else
	{
		name = **cl_savedir ? cl_savedir : M_GetSavegamesPath();
		usefilter = true;
	}

	const size_t len = name.Len();
	if (len > 0)
	{
		name.Substitute("\\", "/");
		if (name[len - 1] != '/')
			name << '/';
	}

	if (usefilter)
		name << LumpFilter << '/';

	CreatePath(name);

	name << prefix;
	if (!strchr(prefix, '.')) name << SAVEGAME_EXT; // only add an extension if the prefix doesn't have one already.
	name = NicePath(name);
	name.Substitute("\\", "/");
	return name;
}

#include "build.h"

#define V(x) x
static spritetype zsp;
static spriteext_t zspx;

FSerializer &Serialize(FSerializer &arc, const char *key, spritetype &c, spritetype *def)
{
	def = &zsp; // always delta against 0
	if (arc.BeginObject(key))
	{
		arc("x", c.pos.X, def->pos.X)
			("y", c.pos.Y, def->pos.Y)
			("z", c.pos.Z, def->pos.Z)
			("cstat", c.cstat, def->cstat)
			("picnum", c.picnum, def->picnum)
			("shade", c.shade, def->shade)
			("pal", c.pal, def->pal)
			("clipdist", c.clipdist, def->clipdist)
			("blend", c.blend, def->blend)
			("xrepeat", c.xrepeat, def->xrepeat)
			("yrepeat", c.yrepeat, def->yrepeat)
			("xoffset", c.xoffset, def->xoffset)
			("yoffset", c.yoffset, def->yoffset)
			("statnum", c.statnum)
			("sectnum", c.sectp)
			("ang", c.ang, def->ang)
			("owner", c.intowner, def->intowner)
			("xvel", c.xvel, def->xvel)
			("yvel", c.yvel, def->yvel)
			("zvel", c.zvel, def->zvel)
			("lotag", c.lotag, def->lotag)
			("hitag", c.hitag, def->hitag)
			("extra", c.extra, def->extra)
			("detail", c.detail, def->detail)
			("cstat2", c.cstat2, def->cstat2)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, spriteext_t& c, spriteext_t* def)
{
	if (arc.isWriting() && c.mdanimtims)
	{
		c.mdanimtims -= mdtims;
		if (c.mdanimtims == 0) c.mdanimtims++;
	}

	def = &zspx; // always delta against 0
	if (arc.BeginObject(key))
	{
		arc("mdanimtims", c.mdanimtims, def->mdanimtims)
			("mdanimcur", c.mdanimcur, def->mdanimcur)
			("angoff", c.angoff, def->angoff)
			("pitch", c.pitch, def->pitch)
			("roll", c.roll, def->roll)
			("pivot_offset", c.pivot_offset, def->pivot_offset)
			("position_offset", c.position_offset, def->position_offset)
			("flags", c.renderflags, def->renderflags)
			("alpha", c.alpha, def->alpha)
			.EndObject();
	}

	if (c.mdanimtims) c.mdanimtims += mdtims;
	return arc;
}

FSerializer &Serialize(FSerializer &arc, const char *key, sectortype &c, sectortype *def)
{
	if (arc.BeginObject(key))
	{
		arc("firstentry", c.firstEntry)
			("lastentry", c.lastEntry)
			("wallptr", c.wallptr, def->wallptr)
			("wallnum", c.wallnum, def->wallnum)
#ifndef SECTOR_HACKJOB // can't save these in test mode...
			("ceilingz", c.ceilingz, def->ceilingz)
			("floorz", c.floorz, def->floorz)
#endif
			("ceilingstat", c.ceilingstat, def->ceilingstat)
			("floorstat", c.floorstat, def->floorstat)
			("ceilingpicnum", c.ceilingpicnum, def->ceilingpicnum)
			("ceilingheinum", c.ceilingheinum, def->ceilingheinum)
			("ceilingshade", c.ceilingshade, def->ceilingshade)
			("ceilingpal", c.ceilingpal, def->ceilingpal)
			("ceilingxpanning", c.ceilingxpan_, def->ceilingxpan_)
			("ceilingypanning", c.ceilingypan_, def->ceilingypan_)
			("floorpicnum", c.floorpicnum, def->floorpicnum)
			("floorheinum", c.floorheinum, def->floorheinum)
			("floorshade", c.floorshade, def->floorshade)
			("floorpal", c.floorpal, def->floorpal)
			("floorxpanning", c.floorxpan_, def->floorxpan_)
			("floorypanning", c.floorypan_, def->floorypan_)
			("visibility", c.visibility, def->visibility)
			("fogpal", c.fogpal, def->fogpal)
			("lotag", c.lotag, def->lotag)
			("hitag", c.hitag, def->hitag)
			("extra", c.extra, def->extra)
			("portalflags", c.portalflags, def->portalflags)
			("portalnum", c.portalnum, def->portalnum);

		// Save the extensions only when playing their respective games.
		if (isDukeLike())
		{
			arc("keyinfo", c.keyinfo, def->keyinfo)
				("shadedsector", c.shadedsector, def->shadedsector)
				("hitagactor", c.hitagactor, def->hitagactor);

		}
		else if (isBlood())
		{
			arc("upperlink", c.upperLink, def->upperLink)
				("lowerlink", c.lowerLink, def->lowerLink)
				("basefloor", c.baseFloor, def->baseFloor)
				("baseCeil", c.baseCeil, def->baseCeil)
				("velfloor", c.velFloor, def->velFloor)
				("velCeil", c.velCeil, def->velCeil)
				("slopwwallofs", c.slopewallofs, def->slopewallofs);

			if (arc.isWriting())
			{
				if (c.hasX())
				{
					BLD_NS::Serialize(arc, "xsector", *c._xs, nullptr);
				}
			}
			else
			{
				if (arc.HasObject("xsector"))
				{
					c.allocX();
					BLD_NS::Serialize(arc, "xsector", *c._xs, nullptr);
				}
			}
		}
		else if (isExhumed())
		{
			arc("SoundSect", c.pSoundSect, def->pSoundSect)
				("Depth", c.Depth, def->Depth)
				("Above", c.pAbove, def->pAbove)
				("Below", c.pBelow, def->pBelow)
				("Sound", c.Sound, def->Sound)
				("Flag", c.Flag, def->Flag)
				("Damage", c.Damage, def->Damage)
				("Speed", c.Speed, def->Speed);

		}
		else if (isSWALL())
		{
			arc("flags", c.flags, def->flags)
				("depth_fixed", c.depth_fixed, def->depth_fixed)
				("stag", c.stag, def->stag)
				("ang", c.ang, def->ang)
				("height", c.height, def->height)
				("speed", c.speed, def->speed)
				("damage", c.damage, def->damage)
				("number", c.number, def->number)
				("u_defined", c.u_defined, def->u_defined)
				("flags2", c.flags2, def->flags2);
		}

		arc.EndObject();
	}
	return arc;
}

FSerializer &Serialize(FSerializer &arc, const char *key, walltype &c, walltype *def)
{
	if (arc.BeginObject(key))
	{
		arc("x", c.pos.X, def->pos.X)
			("y", c.pos.Y, def->pos.Y)
			("point2", c.point2, def->point2)
			("nextwall", c.nextwall, def->nextwall)
			("nextsector", c.nextsector, def->nextsector)
			("cstat", c.cstat, def->cstat)
			("picnum", c.picnum, def->picnum)
			("overpicnum", c.overpicnum, def->overpicnum)
			("shade", c.shade, def->shade)
			("pal", c.pal, def->pal)
			("xrepeat", c.xrepeat, def->xrepeat)
			("yrepeat", c.yrepeat, def->yrepeat)
			("xpanning", c.xpan_, def->xpan_)
			("ypanning", c.ypan_, def->ypan_)
			("lotag", c.lotag, def->lotag)
			("hitag", c.hitag, def->hitag)
			("extra", c.extra, def->extra)
			("portalflags", c.portalflags, def->portalflags)
			("portalnum", c.portalnum, def->portalnum);

		// Save the blood-specific extensions only when playing Blood
		if (isBlood())
		{
			arc("wallbase", c.baseWall, def->baseWall);
			if (arc.isWriting())
			{
				if (c.hasX())
				{
					BLD_NS::Serialize(arc, "xwall", *c._xw, nullptr);
				}
			}
			else
			{
				if (arc.HasObject("xwall"))
				{
					c.allocX();
					BLD_NS::Serialize(arc, "xwall", *c._xw, nullptr);
				}
			}
		}

		arc.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* key, ActorStatList& c, ActorStatList* def)
{
	if (arc.BeginObject(key))
	{
		arc("firstentry", c.firstEntry)
			("lastentry", c.lastEntry)
			.EndObject();
	}
	return arc;
}

void DCoreActor::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc("link_stat", link_stat)
		("link_sector", link_sector)
		("prevstat", prevStat)
		("nextstat", nextStat)
		("prevsect", prevSect)
		("nextsect", nextSect)
		("sprite", spr)
		("time", time)
		("spritesetindex", spritesetindex)
		("spriteext", sprext);

	if (arc.isReading()) spsmooth = {};
}


void SerializeMap(FSerializer& arc)
{
	if (arc.BeginObject("engine"))
	{
		arc.Array("statlist", statList, MAXSTATUS)
			("sectors", sector, sectorbackup)
			("walls", wall, wallbackup)

			("tailspritefree", tailspritefree)
			("myconnectindex", myconnectindex)
			("connecthead", connecthead)
			.Array("connectpoint2", connectpoint2, countof(connectpoint2))
			("randomseed", randomseed)
			("numshades", numshades)	// is this really needed?
			("visibility", g_visibility)
			("relvisibility", g_relvisibility)
			("numsprites", Numsprites)
			("gamesetinput", gamesetinput)
			("allportals", allPortals);

		SerializeInterpolations(arc);

		if (arc.BeginArray("picanm")) // write this in the most compact form available.
		{
			for (int i = 0; i < MAXTILES; i++)
			{
				arc(nullptr, picanm[i].sf)
					(nullptr, picanm[i].extra);
			}
			arc.EndArray();
		}

		arc.EndObject();
	}

	if (arc.isReading())
	{
		setWallSectors();
		hw_CreateSections();
		sectionGeometry.SetSize(sections.Size());
	}
}

//=============================================================================
//
//
//
//=============================================================================

CVAR(Bool, saveloadconfirmation, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

CVAR(Int, autosavenum, 0, CVAR_NOSET | CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
static int nextautosave = -1;
CVAR(Int, disableautosave, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CUSTOM_CVAR(Int, autosavecount, 4, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 1)
		self = 1;
}

CVAR(Int, quicksavenum, 0, CVAR_NOSET | CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
static int nextquicksave = -1;
 CUSTOM_CVAR(Int, quicksavecount, 4, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 1)
		self = 1;
}

 void DoLoadGame(const char* name)
 {
	 gi->FreeLevelData();
	 if (ReadSavegame(name))
	 {
		 gameaction = ga_level;
	 }
	 else
	 {
		 I_Error("%s: Failed to open savegame", name);
	 }
 }


 void G_LoadGame(const char* name, bool hidecon)
 {
	 if (name != NULL)
	 {
		 savename = name;
		 gameaction = !hidecon ? ga_loadgame : ga_loadgamehidecon;
	 }
 }

 void G_DoLoadGame()
 {
	 if (gameaction == ga_loadgamehidecon && gamestate == GS_FULLCONSOLE)
	 {
		 // does this even do anything anymore?
		 gamestate = GS_HIDECONSOLE;
	 }

	 inputState.ClearAllInput();
	 DoLoadGame(savename);
	 BackupSaveGame = savename;
 }

 extern bool sendsave;
 extern FString	savedescription;
 extern FString	savegamefile;

 void G_SaveGame(const char* filename, const char* description)
 {
	 if (sendsave || gameaction == ga_savegame)
	 {
		 Printf("%s\n", GStrings("TXT_SAVEPENDING"));
	 }
	 else if (gamestate != GS_LEVEL)
	 {
		 Printf("%s\n", GStrings("TXT_NOTINLEVEL"));
	 }
	 else if (!gi->CanSave())
	 {
		 Printf("%s\n", GStrings("TXT_SPPLAYERDEAD"));
	 }
	 else
	 {
		 savegamefile = filename;
		 savedescription = description;
		 sendsave = true;
	 }
 }

 //---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

 void startSaveGame(int player, uint8_t** stream, bool skip)
 {
	 auto s = ReadString(stream);
	 savegamefile = s;
	 delete[] s;
	 s = ReadString(stream);
	 savedescription = s;
	 if (!skip && gi->CanSave())
	 {
		 if (player != consoleplayer)
		 {
			 // Paths sent over the network will be valid for the system that sent
			 // the save command. For other systems, the path needs to be changed.
			 savegamefile = G_BuildSaveName(ExtractFileBase(savegamefile, true));
		 }
		 gameaction = ga_savegame;
	 }
 }

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

 void G_DoSaveGame(bool ok4q, bool forceq, const char* fn, const char* desc)
 {
	 if (WriteSavegame(fn, desc))
	 {
		 savegameManager.NotifyNewSave(fn, desc, ok4q, forceq);
		 Printf(PRINT_NOTIFY, "%s\n", GStrings("GGSAVED"));
		 BackupSaveGame = fn;
	 }
 }

 //---------------------------------------------------------------------------
 //
 //
 //
 //---------------------------------------------------------------------------

 void M_Autosave()
{
	if (disableautosave) return;
	if (!gi->CanSave()) return;
	FString description;
	FString file;
	// Keep a rotating sets of autosaves
	UCVarValue num;
	const char* readableTime;
	int count = autosavecount != 0 ? autosavecount : 1;

	if (nextautosave == -1)
	{
		nextautosave = (autosavenum + 1) % count;
	}

	num.Int = nextautosave;
	autosavenum.ForceSet(num, CVAR_Int);

	auto Filename = G_BuildSaveName(FStringf("auto%04d", nextautosave));
	readableTime = myasctime();
	FStringf SaveTitle("Autosave %s", readableTime);
	nextautosave = (nextautosave + 1) % count;
	G_DoSaveGame(false, false, Filename, SaveTitle);
}

CCMD(autosave)
{
	gameaction = ga_autosave;
}

CCMD(rotatingquicksave)
{
	if (!gi->CanSave()) return;
	FString description;
	FString file;
	// Keep a rotating sets of quicksaves
	UCVarValue num;
	const char* readableTime;
	int count = quicksavecount != 0 ? quicksavecount : 1;

	if (nextquicksave == -1)
	{
		nextquicksave = (quicksavenum + 1) % count;
	}

	num.Int = nextquicksave;
	quicksavenum.ForceSet(num, CVAR_Int);

	FSaveGameNode sg;
	auto Filename = G_BuildSaveName(FStringf("quick%04d", nextquicksave));
	readableTime = myasctime();
	FStringf SaveTitle("Quicksave %s", readableTime);
	nextquicksave = (nextquicksave + 1) % count;
	G_SaveGame(Filename, SaveTitle);
}


//==========================================================================
//
// CCMD load
//
// Load a saved game.
//
//==========================================================================

UNSAFE_CCMD(load)
{
	if (argv.argc() != 2)
	{
		Printf("usage: load <filename>\n");
		return;
	}
	if (netgame)
	{
		Printf("cannot load during a network game\n");
		return;
	}
	FString fname = G_BuildSaveName(argv[1]);
	G_LoadGame(fname);
}

//==========================================================================
//
// CCMD save
//
// Save the current game.
//
//==========================================================================

UNSAFE_CCMD(save)
{
	if (argv.argc() < 2 || argv.argc() > 3)
	{
		Printf("usage: save <filename> [description]\n");
		return;
	}
	FString fname = G_BuildSaveName(argv[1]);
	G_SaveGame(fname, argv.argc() > 2 ? argv[2] : argv[1]);
}


