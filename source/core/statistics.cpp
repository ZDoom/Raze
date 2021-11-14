/*
**
** statistics.cpp
** Save game statistics to a file
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
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

#include <stdio.h>
#include <time.h>

#include "strnatcmp.h"
#include "c_dispatch.h"
#include "m_png.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "stats.h"
#include "c_cvars.h"
#include "sc_man.h"
#include "serializer.h"
#include "gstrings.h"
#include "version.h"
#include "engineerrors.h"
#include "gamestruct.h"
#include "printf.h"

CVAR(Int, savestatistics, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(String, statfile, GAMENAMELOWERCASE "stat.txt", CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

//==========================================================================
//
// Global statistics data
//
//==========================================================================

// This struct is used to track statistics data in game
struct OneLevel
{
	int totalkills = 0, killcount = 0;
	int totalsecrets = 0, secretcount = 0;
	int leveltime = 0;
	FString Levelname;
};

// Current game's statistics
static TArray<OneLevel> LevelData;
static FString StartEpisode;
static int StartSkill;
static FString LevelName;

// The statistics for one level
struct FLevelStatistics
{
	char info[60];
	short skill;
	short playerclass;
	char name[24];
	int timeneeded;
};

// Statistics for one episode playthrough
struct FSessionStatistics : public FLevelStatistics
{
	TArray<FLevelStatistics> levelstats;
};

// Collected statistics for one episode
struct FStatistics
{
	TArray<FSessionStatistics> stats;
	FString epi_name;
	FString epi_header;
};

// All statistics ever collected
static TArray<FStatistics> EpisodeStatistics;

//==========================================================================
//
// Initializes statistics data from external file
//
//==========================================================================

static void ParseStatistics(const char *fn, TArray<FStatistics> &statlist)
{
	statlist.Clear();
	try
	{
		FScanner sc;
		if (!sc.OpenFile(fn)) return;

		while (sc.GetString())
		{
			FStatistics &ep_entry = statlist[statlist.Reserve(1)];

			ep_entry.epi_header = sc.String;
			sc.MustGetString();
			ep_entry.epi_name = sc.String;

			sc.MustGetStringName("{");
			while (!sc.CheckString("}"))
			{
				FSessionStatistics &session = ep_entry.stats[ep_entry.stats.Reserve(1)];

				sc.MustGetString();
				sc.MustGetString();
				strncpy(session.name, sc.String, 24);
				sc.MustGetString();
				strncpy(session.info, sc.String, 60);

				int h,m,s;
				sc.MustGetString();
				sscanf(sc.String, "%d:%d:%d", &h, &m, &s);
				session.timeneeded= ((((h*60)+m)*60)+s);

				sc.MustGetNumber();
				session.skill=sc.Number;
				if (sc.CheckString("{"))
				{
					while (!sc.CheckString("}"))
					{
						FLevelStatistics &lstats = session.levelstats[session.levelstats.Reserve(1)];

						sc.MustGetString();
						strncpy(lstats.name, sc.String, 24);
						sc.MustGetString();
						strncpy(lstats.info, sc.String, 60);

						int h,m,s;
						sc.MustGetString();
						sscanf(sc.String, "%d:%d:%d", &h, &m, &s);
						lstats.timeneeded= ((((h*60)+m)*60)+s);

						lstats.skill = 0;
					}
				}
			}
		}
	}
	catch(CRecoverableError &)
	{
	}
}


// ====================================================================
//
// Reads the statistics file
//
// ====================================================================

void InitStatistics()
{
	ParseStatistics(statfile, EpisodeStatistics);
}

// ====================================================================
//
// Saves the statistics file
// Sorting helpers.
//
// ====================================================================

int compare_episode_names(const void *a, const void *b)
{
	FStatistics *A = (FStatistics*)a;
	FStatistics *B = (FStatistics*)b;

	return strnatcasecmp(A->epi_header, B->epi_header);
}

int compare_level_names(const void *a, const void *b)
{
	FLevelStatistics *A = (FLevelStatistics*)a;
	FLevelStatistics *B = (FLevelStatistics*)b;

	return strnatcasecmp(A->name, B->name);
}

int compare_dates(const void *a, const void *b)
{
	FLevelStatistics *A = (FLevelStatistics*)a;
	FLevelStatistics *B = (FLevelStatistics*)b;
	char *p;

	int aday = strtol(A->name, &p, 10);
	int amonth = strtol(p+1, &p, 10);
	int ayear = strtol(p+1, &p, 10);
	int av = aday + 100 * amonth + 2000*ayear;

	int bday = strtol(B->name, &p, 10);
	int bmonth = strtol(p+1, &p, 10);
	int byear = strtol(p+1, &p, 10);
	int bv = bday + 100 * bmonth + 2000*byear;

	return av-bv;
}


// ====================================================================
//
// Main save routine
//
// ====================================================================

inline int hours(int v) { return v / (60*60); }
inline int minutes(int v) { return (v % (60*60)) / (60); }
inline int seconds(int v) { return (v % (60)); }

static void SaveStatistics(const char *fn, TArray<FStatistics> &statlist)
{
	unsigned int j;

	FileWriter *fw = FileWriter::Open(fn);
	if (fw == nullptr) return;

	qsort(&statlist[0], statlist.Size(), sizeof(statlist[0]), compare_episode_names);
	for(unsigned i=0;i<statlist.Size ();i++)
	{
		FStatistics &ep_stats = statlist[i];

		qsort(&ep_stats.stats[0], ep_stats.stats.Size(), sizeof(ep_stats.stats[0]), compare_dates);

		fw->Printf("%s \"%s\"\n{\n", ep_stats.epi_header.GetChars(), ep_stats.epi_name.GetChars());
		for(j=0;j<ep_stats.stats.Size();j++)
		{
			FSessionStatistics *sst = &ep_stats.stats[j];
			if (sst->info[0]>0)
			{
				fw->Printf("\t%2i. %10s \"%-33s\" %02d:%02d:%02d %i\n", j+1, sst->name, sst->info, 
					hours(sst->timeneeded),	minutes(sst->timeneeded), seconds(sst->timeneeded),	sst->skill);

				TArray<FLevelStatistics> &ls = sst->levelstats;
				if (ls.Size() > 0)
				{
					fw->Printf("\t{\n");

					// Only makes sense if level names follow a strict format. This is noz the case here.
					//qsort(&ls[0], ls.Size(), sizeof(ls[0]), compare_level_names);

					for(unsigned k=0;k<ls.Size ();k++)
					{
						fw->Printf("\t\t%-8s \"%-33s\" %02d:%02d:%02d\n", ls[k].name, ls[k].info, 
							hours(ls[k].timeneeded), minutes(ls[k].timeneeded), seconds(ls[k].timeneeded));
					}
					fw->Printf("\t}\n");
				}
			}
		}
		fw->Printf("}\n\n");
	}
	delete fw;
}


// ====================================================================
//
// Gets list for current episode
//
// ====================================================================
static FStatistics *GetStatisticsList(TArray<FStatistics> &statlist, const char *section, const char *fullname)
{
	for(unsigned int i=0;i<statlist.Size();i++)
	{
		if (!stricmp(section, statlist[i].epi_header)) 
		{
			return &statlist[i];
		}
	}
	FStatistics * stats = &statlist[statlist.Reserve(1)];
	stats->epi_header = section;
	stats->epi_name = fullname;
	return stats;
}

// ====================================================================
//
// Adds a statistics entry
//
// ====================================================================
static FSessionStatistics *StatisticsEntry(FStatistics *stats, const char *text, int playtime)
{
	FSessionStatistics s;
	time_t clock;
	struct tm *lt;

	time (&clock);
	lt = localtime (&clock);

	if (lt != NULL)
		mysnprintf(s.name, countof(s.name), "%02d.%02d.%04d",lt->tm_mday, lt->tm_mon+1, lt->tm_year+1900);
	else
		strcpy(s.name,"00.00.0000");

	s.skill=StartSkill;
	strcpy(s.info, text);
	s.timeneeded=playtime;

	stats->stats.Push(s);
	return &stats->stats[stats->stats.Size()-1];
}

// ====================================================================
//
// Adds a statistics entry
//
// ====================================================================
static void LevelStatEntry(FSessionStatistics *es, const char *level, const char *text, int playtime)
{
	FLevelStatistics s;
	time_t clock;
	struct tm *lt;

	time (&clock);
	lt = localtime (&clock);

	strcpy(s.name, level);
	strcpy(s.info, text);
	s.timeneeded=playtime;
	es->levelstats.Push(s);
}



//==========================================================================
//
// STAT_StartNewGame: called when a new game starts. Sets the current episode
//
//==========================================================================

void STAT_StartNewGame(const char *episode, int skill)
{
	StartEpisode = GStrings.localize(episode);
	StartSkill = skill;
	LevelData.Clear();
	LevelName = "";
}

void STAT_NewLevel(const char* mapname)
{
	if (strncmp(mapname, "file://", 7) == 0)
	{
		STAT_StartNewGame("", 0);	// reset and deactivate for user maps
	}
	else
	{
		LevelName = mapname[0] == '/' ? mapname + 1 : mapname;
	}
}

//==========================================================================
//
// Store the current level's statistics
//
//==========================================================================

static void StoreLevelStats()
{
	unsigned int i;

	for(i=0;i<LevelData.Size();i++)
	{
		if (!LevelData[i].Levelname.CompareNoCase(LevelName)) break;
	}
	if (i==LevelData.Size())
	{
		LevelData.Reserve(1);
		LevelData[i].Levelname = LevelName; // should never happen
	}
	auto stat = gi->getStats();
	LevelData[i].totalkills = stat.tkill;
	LevelData[i].killcount = stat.kill;
	LevelData[i].totalsecrets = stat.tsecret;
	LevelData[i].secretcount = stat.secret;
	LevelData[i].leveltime = stat.timesecnd;
}

//==========================================================================
//
// STAT_ChangeLevel: called when the level changes or the current statistics are
// requested
//
//==========================================================================

void STAT_Update(bool endofgame)
{
	if (*StartEpisode == 0 || *LevelName == 0) return;
	const char* fn = "?";
	// record the current level's stats.
	StoreLevelStats();

	if (savestatistics == 1 && endofgame)
	{
		auto lump = fileSystem.FindFile(LevelName);
		if (lump >= 0)
		{
			int file = fileSystem.GetFileContainer(lump);
			fn = fileSystem.GetResourceFileName(file);
		}

		FString section = ExtractFileBase(fn) + "." + ExtractFileBase(LevelData[0].Levelname);
		section.ToUpper();
		FStatistics* sl = GetStatisticsList(EpisodeStatistics, section, StartEpisode);

		int statvals[] = { 0,0,0,0, 0 };
		FString infostring;
		int validlevels = LevelData.Size();
		for (unsigned i = 0; i < LevelData.Size(); i++)
		{
			statvals[0] += LevelData[i].killcount;
			statvals[1] += LevelData[i].totalkills;
			statvals[2] += LevelData[i].secretcount;
			statvals[3] += LevelData[i].totalsecrets;
			statvals[4] += LevelData[i].leveltime;
		}

		infostring.Format("%4d/%4d, %3d/%3d, %2d", statvals[0], statvals[1], statvals[2], statvals[3], validlevels);
		FSessionStatistics* es = StatisticsEntry(sl, infostring, statvals[4]);

		for (unsigned i = 0; i < LevelData.Size(); i++)
		{
			FString lsection = ExtractFileBase(LevelData[i].Levelname);
			lsection.ToUpper();
			infostring.Format("%4d/%4d, %3d/%3d", LevelData[i].killcount, LevelData[i].totalkills, LevelData[i].secretcount, LevelData[i].totalsecrets);
			LevelStatEntry(es, lsection, infostring, LevelData[i].leveltime);
		}
		SaveStatistics(statfile, EpisodeStatistics);
		LevelData.Clear();
		StartEpisode = LevelName = "";
	}
}

void STAT_Cancel()
{
	LevelData.Clear();
	StartEpisode = LevelName = "";
}

//==========================================================================
//
// saves statistics info to savegames
//
//==========================================================================

FSerializer& Serialize(FSerializer& arc, const char* key, OneLevel& l, OneLevel* def)
{
	if (arc.BeginObject(key))
	{
		arc("totalkills", l.totalkills)
			("killcount", l.killcount)
			("totalsecrets", l.totalsecrets)
			("secretcount", l.secretcount)
			("leveltime", l.leveltime)
			("levelname", l.Levelname)
			.EndObject();
	}
	return arc;
}

void SerializeStatistics(FSerializer &arc)
{
	if (arc.BeginObject("statistics"))
	{
		arc("levelname", LevelName)
			("episode", StartEpisode)
			("skill", StartSkill)
			("levels", LevelData)
			.EndObject();
	}
}


//==========================================================================
//
// show statistics
//
//==========================================================================

FString GetStatString()
{
	FString compose;
	for(unsigned i = 0; i < LevelData.Size(); i++)
	{
		OneLevel *l = &LevelData[i];
		compose.AppendFormat("Level %s - Kills: %d/%d - Secrets: %d/%d - Time: %d:%02d\n", 
			l->Levelname.GetChars(), l->killcount, l->totalkills, l->secretcount, l->totalsecrets,
			l->leveltime/(60), (l->leveltime)%60);
	}
	return compose;
}

CCMD(printstats)
{
	if (*StartEpisode == 0 || *LevelName == 0) return;
	StoreLevelStats();	// Refresh the current level's results.
	Printf("%s", GetStatString().GetChars());
}

ADD_STAT(statistics)
{
	if (*StartEpisode == 0 || *LevelName == 0) return "";
	StoreLevelStats();	// Refresh the current level's results.
	return GetStatString();
}

