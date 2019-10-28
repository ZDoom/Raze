#include "zstring.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "resourcefile.h"
#include "sc_man.h"
#include "i_specialpaths.h"
#include "inputstate.h"
#include "c_cvars.h"
#include "../../glbackend/glbackend.h"


// Currently there is no global state for the current game. This is a temporary workaround because the video init code needs to do a few things based on the active game.

FString currentGame;	

namespace Duke
{
	extern GameInterface Interface;
}
namespace Redneck
{
	extern GameInterface Interface;
}
namespace Blood
{
	extern GameInterface Interface;
}
namespace ShadowWarrior
{
	extern GameInterface Interface;
}

GameInterface *CheckFrontend()
{
	FILE* f = fopen("blood.rff", "rb");
	if (f)
	{
		currentGame = "Blood";
		fclose(f);
		return &Blood::Interface;
	}
	else
	{
		f = fopen("redneck.grp", "rb");
		if (f)
		{
			currentGame = "Redneck";
			fseek(f, 0, SEEK_END);
			auto pos = ftell(f);
			// Quick hack to distinguish these two. This won't survive until production but for testing it's sufficient.
			if (pos > 190'000'000) currentGame = "RedneckRides";
			fclose(f);
			return &Redneck::Interface;
		}
		else
		{
			f = fopen("sw.grp", "rb");
			if (f)
			{
				currentGame = "ShadowWarrior";
				fclose(f);
				return &ShadowWarrior::Interface;
			}
			f = fopen("fury.grp", "rb");
			if (f)
			{
				currentGame = "IonFury";
				fclose(f);
				return &Duke::Interface;
			}
			f = fopen("nam.grp", "rb");
			if (f)
			{
				currentGame = "Nam";
				fclose(f);
				return &Duke::Interface;
			}
			f = fopen("ww2gi.grp", "rb");
			if (f)
			{
				currentGame = "WW2GI";
				fclose(f);
				return &Duke::Interface;
			}
			else
			{
				currentGame = "Duke";
			}
			return &Duke::Interface;
		}
	}
}

void ChooseGame()
{
	auto dir = Args->CheckValue("-game");
	if (dir && !chdir(dir))
	{
		gi = CheckFrontend();
		return;
	}

	TArray<FString> paths;
	std::vector<std::wstring> wgames;
	TArray<TASKDIALOG_BUTTON> buttons;
	char* token;
	
	FileReader fr;
	if (fr.OpenFile("./games.list"))
	{
		auto filedata = fr.ReadPadded(1);

		auto script = scriptfile_fromstring((char*)filedata.Data());
		int id = 1000;
		while (!scriptfile_eof(script))
		{
			scriptfile_getstring(script, &token);
			if (scriptfile_eof(script))
			{
				break;
			}
			FString game = token;
			scriptfile_getstring(script, &token);
			paths.Push(token);
			FStringf display("%s\n%s", game.GetChars(), token);
			wgames.push_back(display.WideString());
			buttons.Push({ id++, wgames.back().c_str() });
		}
	}
	if (paths.Size() == 0)
	{
		exit(1);
	}

	int nResult = 0;

	TASKDIALOGCONFIG stTaskConfig;
	ZeroMemory(&stTaskConfig, sizeof(stTaskConfig));

	stTaskConfig.cbSize = sizeof(TASKDIALOGCONFIG);
	stTaskConfig.hwndParent = NULL;
	stTaskConfig.hInstance = NULL;

	stTaskConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION| TDF_USE_COMMAND_LINKS;

	if (!gi)
	{
		// Open a popup to select the game.
		// The entire startup code just doesn't work right if this isn't checked as the very first thing.
		stTaskConfig.pszWindowTitle = L"Demolition";
		stTaskConfig.pszMainInstruction = L"Choose your game";
		stTaskConfig.pszContent = L"";
		stTaskConfig.cButtons = buttons.Size();

		stTaskConfig.pButtons = buttons.Data();
		stTaskConfig.nDefaultButton = 1000;

		if (SUCCEEDED(TaskDialogIndirect(&stTaskConfig, &nResult, NULL, NULL)))
		{
			if (nResult >= 1000 && nResult < 1000 +(int)buttons.Size())
			{
				nResult -= 1000;
				chdir(paths[nResult]);
				gi = CheckFrontend();
			}
		}
		if (gi == nullptr) exit(1);
	}
}





std::unique_ptr<FResourceFile> engine_res;

// The resourge manager in cache1d is far too broken to add some arbitrary file without some adjustment.
// For now, keep this file here, until the resource management can be redone in a more workable fashion.
extern FString progdir;

void InitBaseRes()
{
	if (!engine_res)
	{
		// If we get here for the first time, load the engine-internal data.
		FString baseres = progdir + "demolition.pk3";
		engine_res.reset(FResourceFile::OpenResourceFile(baseres, true, true));
		if (!engine_res)
		{
			I_Error("Engine resources (%s) not found", baseres.GetChars());
		}
	}
}

FileReader openFromBaseResource(const char* fn)
{
	InitBaseRes();
	auto lump = engine_res->FindLump(fn);
	if (lump) return lump->NewReader();
	// Also look in game filtered directories.
	FStringf filtername("filter/game-%s/%s", currentGame.GetChars(), fn);
	lump = engine_res->FindLump(filtername);
	if (lump) return lump->NewReader();
	return FileReader(nullptr);

}



int GameMain()
{
	try
	{
		// Write to the DOCUMENTS directory, not the game directory
		
		FString logpath = M_GetDocumentsPath() + "demolition.log";
		OSD_SetLogFile(logpath);
		CONFIG_ReadCombatMacros();

		// Startup dialog must be presented here so that everything can be set up before reading the keybinds.
		G_LoadConfig(currentGame);
		CONFIG_Init();
		r = gi->app_main(buildargc, (const char**)buildargv);
	}
	catch (const std::runtime_error & err)
	{
		wm_msgbox("Error", "%s", err.what());
		return 3;
	}
	catch (const ExitEvent & exit)
	{
		// Just let the rest of the function execute.
		r = exit.Reason();
	}
}