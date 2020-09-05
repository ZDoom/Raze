#pragma once

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
enum gamestate_t : int
{
	GS_LEVEL,
	GS_INTRO,
	GS_INTERMISSION,
	GS_FINALE,
	GS_MENUSCREEN,
	GS_FULLCONSOLE,		// [RH]	Fullscreen console
	GS_HIDECONSOLE,		// [RH] The menu just did something that should hide fs console
	GS_STARTUP,			// [RH] Console is fullscreen, and game is just starting
	GS_TITLELEVEL,		// [RH] A combination of GS_LEVEL and GS_MENUSCREEN

	GS_FORCEWIPE = -1,
	GS_FORCEWIPEFADE = -2,
	GS_FORCEWIPEBURN = -3,
	GS_FORCEWIPEMELT = -4
};

enum gameaction_t : int
{
	ga_nothing,
	ga_level,				// Switch to play mode without any initialization
	ga_intro,
	ga_intermission,

	ga_startup,				// go back to intro after uninitializing the game state
	ga_mainmenu,			// go back to main menu after uninitializing the game state
	ga_mainmenunostopsound,	// Same but doesn't stop playing sounds.
	ga_creditsmenu,			// go to the credits menu after uninitializing the game state
	ga_newgame,				// start a new game
	ga_recordgame,			// start a new demo recording (later)
	ga_loadgame,			// load a savegame and resume play.
	ga_loadgameplaydemo,	// load a savegame and play a demo.
	ga_autoloadgame,		// load last autosave and resume play.
	ga_savegame,			// save the game
	ga_autosave,			// autosave the game (for triggering a save from within the game.)
	ga_completed,			// Level was exited.
	ga_nextlevel			// Actually start the next level.
};
extern	gamestate_t 	gamestate;
extern gameaction_t		gameaction;
