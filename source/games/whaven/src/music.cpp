#include "ns.h"
#include "wh.h"
#include "raze_music.h"
#include "mapinfo.h"

BEGIN_WH_NS 



enum
{
	NUMLEVELS = 17,
	SONGSPERLEVEL = 4,
	
	BASE_SONG = 0,
	songelements = 3,
	totallevels = 6,
	arrangements = 3,
	
};
static int songsperlevel;
static int oldsong;

static bool playthesong(int which, bool looped)
{
	char buffer[40];
	mysnprintf(buffer, 40, "%s/%04d", isWh2()? "F_SONGS" : "SONGS", which);
	return Mus_Play(currentLevel->labelName, buffer, looped);
}

static bool loadlevelsongs(int which) 
{
	oldsong = which;
	return true;
}

void startsong(int which) // 0, 1, 2 or 3
{
	if (oldsong < 0) return;
	int index = (oldsong * SONGSPERLEVEL) + which;
	bool result = playthesong(index, !attacktheme);
	if (result && which < 2) attacktheme = 0;
}

void startmusic(int level) 
{
	// allow music override from MAPINFO.
	if (currentLevel->music.IsNotEmpty())
	{
		Mus_Play(currentLevel->labelName, currentLevel->music, true);
		oldsong = -1;
		return;
	}
	if (!isWh2())
	{
		level %= 6;
		int index = (songsperlevel * level) + (songelements * 2) + BASE_SONG;
		playthesong(index, true);
	}
	else
	{
		if (level < 0 || level > NUMLEVELS - 1)
			level = 0;

		loadlevelsongs(level);
		startsong(0);
	}
} 

void SND_MenuMusic() 
{
	if (isWh2())
	{
		startmusic(NUMLEVELS - 1);
	}
	else
	{
		int which = (totallevels * songsperlevel) + BASE_SONG + 2;
		playthesong(which, true);
	}
}


void setupmidi() 
{
	if (isWh2())
		songsperlevel = 4;
	else
		songsperlevel = songelements * arrangements;
}


END_WH_NS
