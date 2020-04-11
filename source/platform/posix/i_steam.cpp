/*
** i_steam.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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
**
*/

#include <sys/stat.h>

#include "sc_man.h"
#include "cmdlib.h"
#include "i_specialpaths.h"
#include "engineerrors.h"

static void PSR_FindEndBlock(FScanner &sc)
{
	int depth = 1;
	do
	{
		if(sc.CheckToken('}'))
			--depth;
		else if(sc.CheckToken('{'))
			++depth;
		else
			sc.MustGetAnyToken();
	}
	while(depth);
}
static void PSR_SkipBlock(FScanner &sc)
{
	sc.MustGetToken('{');
	PSR_FindEndBlock(sc);
}
static bool PSR_FindAndEnterBlock(FScanner &sc, const char* keyword)
{
	// Finds a block with a given keyword and then enter it (opening brace)
	// Should be closed with PSR_FindEndBlock
	while(sc.GetToken())
	{
		if(sc.TokenType == '}')
		{
			sc.UnGet();
			return false;
		}

		sc.TokenMustBe(TK_StringConst);
		if(!sc.Compare(keyword))
		{
			if(!sc.CheckToken(TK_StringConst))
				PSR_SkipBlock(sc);
		}
		else
		{
			sc.MustGetToken('{');
			return true;
		}
	}
	return false;
}
static TArray<FString> PSR_ReadBaseInstalls(FScanner &sc)
{
	TArray<FString> result;

	// Get a list of possible install directories.
	while(sc.GetToken())
	{
		if(sc.TokenType == '}')
			break;

		sc.TokenMustBe(TK_StringConst);
		FString key(sc.String);
		if(key.Left(18).CompareNoCase("BaseInstallFolder_") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			result.Push(FString(sc.String) + "/steamapps/common");
		}
		else
		{
			if(sc.CheckToken('{'))
				PSR_FindEndBlock(sc);
			else
				sc.MustGetToken(TK_StringConst);
		}
	}

	return result;
}
static TArray<FString> ParseSteamRegistry(const char* path)
{
	TArray<FString> dirs;

	// Read registry data
	FScanner sc;
	sc.OpenFile(path);
	//if (sc.Sc)
	{
		sc.SetCMode(true);

		// Find the SteamApps listing
		if (PSR_FindAndEnterBlock(sc, "InstallConfigStore"))
		{
			if (PSR_FindAndEnterBlock(sc, "Software"))
			{
				if (PSR_FindAndEnterBlock(sc, "Valve"))
				{
					if (PSR_FindAndEnterBlock(sc, "Steam"))
					{
						dirs = PSR_ReadBaseInstalls(sc);
					}
					PSR_FindEndBlock(sc);
				}
				PSR_FindEndBlock(sc);
			}
			PSR_FindEndBlock(sc);
		}
	}
	return dirs;
}


const char *AppInfo[] =
{
	"Duke Nukem 3D/gameroot",
	"Duke Nukem 3D/gameroot/addons/dc",
	"Duke Nukem 3D/gameroot/addons/nw",
	"Duke Nukem 3D/gameroot/addons/vacation",
	"World War II GI/WW2GI",
	"Shadow Warrior Classic/gameroot",
	"Shadow Warrior Classic/gameroot/addons",
	"Shadow Warrior Original/gameroot",
	"Ion Fury",
	"Blood",
	"One Unit Whole Blood",
	"Nam/NAM",
	"Redneck Rampage/Redneck",
	"Redneck Rampage Rides Again/AGAIN",
	"Redneck Deer Huntin'/HUNTIN"
#ifdef __APPLE__
	"Duke Nukem 3D/Duke Nukem 3D.app/drive_c/Program Files/Duke Nukem 3D",
	"Nam/Nam.app/Contents/Resources/Nam.boxer/C.harddisk/NAM",
	"Shadow Warrior DOS/Shadow Warrior.app/Contents/Resources/sw",
	"Redneck Rampage/Redneck Rampage.app/Contents/Resources/Redneck Rampage.boxer/C Redneck Rampage.harddisk",
	// macOS version of Redneck Rampage Rides Again is completely broken on Steam
	"Redneck Deer Huntin'/Redneck Deer Huntin.app/Contents/Resources/Redneck Deer Huntin.boxer/C.harddisk/INTRPLAY/HUNTIN"
#endif
};

TArray<FString> I_GetSteamPath()
{
	TArray<FString> result;
	TArray<FString> SteamInstallFolders;

	// Linux and OS X actually allow the user to install to any location, so
	// we need to figure out on an app-by-app basis where the game is installed.
	// To do so, we read the virtual registry.
#ifdef __APPLE__
	const FString appSupportPath = M_GetMacAppSupportPath();
	FString regPath = appSupportPath + "/Steam/config/config.vdf";
	try
	{
		SteamInstallFolders = ParseSteamRegistry(regPath);
	}
	catch(class CRecoverableError& error)
	{
		// If we can't parse for some reason just pretend we can't find anything.
		return result;
	}

	SteamInstallFolders.Push(appSupportPath + "/Steam/SteamApps/common");
#else
	char* home = getenv("HOME");
	if(home != NULL && *home != '\0')
	{
		FString regPath;
		regPath.Format("%s/.steam/config/config.vdf", home);
		// [BL] The config seems to have moved from the more modern .local to
		// .steam at some point. Not sure if it's just my setup so I guess we
		// can fall back on it?
		if(!FileExists(regPath))
			regPath.Format("%s/.local/share/Steam/config/config.vdf", home);

		try
		{
			SteamInstallFolders = ParseSteamRegistry(regPath);
		}
		catch(class CRecoverableError &error)
		{
			// If we can't parse for some reason just pretend we can't find anything.
			return result;
		}

		regPath.Format("%s/.local/share/Steam/SteamApps/common", home);
		SteamInstallFolders.Push(regPath);
	}
#endif

	for(unsigned int i = 0;i < SteamInstallFolders.Size();++i)
	{
		for(unsigned int app = 0;app < countof(AppInfo);++app)
		{
			struct stat st;
			FString candidate(SteamInstallFolders[i] + "/" + AppInfo[app]);
			if(DirExists(candidate))
				result.Push(candidate);
		}
	}

	return result;
}

