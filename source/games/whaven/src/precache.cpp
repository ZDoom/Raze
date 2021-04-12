#include "ns.h"
#include "wh.h"
#include "precache.h"

BEGIN_WH_NS

void addTile(int num)
{
	markTileForPrecache(num, 0);
}

void cachespritenum(int i)
{
	int maxc = 1;
	if (sprite[i].picnum == RAT || sprite[i].picnum == GUARDIAN)
		maxc = 15;
	if (sprite[i].picnum == HANGMAN)
		maxc = 40;

	if (sprite[i].picnum == GRONHAL || sprite[i].picnum == GRONMU || sprite[i].picnum == GRONSW)
		maxc = 19;

	switch (sprite[i].picnum)
	{
	case GOBLINSTAND:
	case GOBLIN:
		maxc = 21;
		break;
	case KOBOLD:
		maxc = 24;
		break;
	case DEVILSTAND:
	case DEVIL:
		maxc = 25;
		break;
	case DRAGON:
		maxc = 11;
		break;

	case SPIDER:
		maxc = 39;
		break;
	case MINOTAUR:
		maxc = 35;
		break;
	case FATWITCH:
		maxc = 19;
		break;
	case SKULLY:
		maxc = 20;
		break;
	case JUDYSIT:
	case JUDY:
		maxc = 18;
		break;
	}
	for (int j = sprite[i].picnum; j < (sprite[i].picnum + maxc); j++)
		addTile(j);
}

void precacheTiles()
{
	for (int i = 0; i < numsectors; i++) {
		addTile(sector[i].floorpicnum);
		addTile(sector[i].ceilingpicnum);
	}
	for (int i = 0; i < numwalls; i++) {
		addTile(wall[i].picnum);
		if (wall[i].overpicnum >= 0) {
			addTile(wall[i].overpicnum);
		}
	}
	for (int i = 0; i < MAXSPRITES; i++) {
		if (sprite[i].statnum < MAXSTATUS) 
			cachespritenum(i);
	}
				
	addTile(BAT);
	addTile(GOBLINATTACK);
	addTile(MINOTAURATTACK);
	addTile(KOBOLDATTACK);
	addTile(FREDATTACK);
	addTile(DEVILATTACK);
	
	for(int i = KNIFEREADY; i <= BIGAXEDRAW10; i++) //hud weapons
		addTile(i);
	for(int i = THEFONT; i < CRYSTALSTAFF; i++) //small font
		addTile(i);
				
	addTile(SSTATUSBAR);
	for(int i = 0; i < 8; i++)
		addTile(sspellbookanim[i][0].daweaponframe);
				
	addTile(ANNIHILATE);
	addTile(HELMET);
	addTile(SSCOREBACKPIC);
	addTile(SHEALTHBACK);
				
	for(int i = 0; i < 4; i++) {
		addTile(SKEYBLANK+i);
		addTile(SCARY+i);
	}
				
	addTile(SPOTIONBACKPIC);
	for(int i = 0; i < MAXPOTIONS; i++) {
		addTile(SPOTIONARROW+i);
		addTile(SFLASKBLUE+i);
	}
	addTile(SFLASKBLACK);
	for(int i = 0; i < 5; i++)
		addTile(spikeanimtics[i].daweaponframe);

	precacheMarkedTiles();
}

END_WH_NS
