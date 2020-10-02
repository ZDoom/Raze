#include "ns.h"
#include "wh.h"

BEGIN_WH_NS
	
int potiontilenum;
	
void potiontext(PLAYER& plr) {    
	if( plr.potion[plr.currentpotion] > 0)
		switch(plr.currentpotion) {
		case 0:
			showmessage("Health Potion", 240);
		break;
		case 1:
			showmessage("Strength Potion", 240);
		break;
		case 2:
			showmessage("Cure Poison Potion", 240);
		break;
		case 3:
			showmessage("Resist Fire Potion", 240);
		break;
		case 4:
			showmessage("Invisibility Potion", 240);
		break;
		}
}
	
void potionchange(int snum)
{
	PLAYER& plr = player[snum];
		
	int key = ((plr.pInput.bits & (15 << 16)) >> 16) - 1;
	if(key != -1 && key < 7)
	{
		if(key == 5 || key == 6)
		{
			key = ( key == 5 ? -1 : 1 );
			plr.currentpotion += key;
			if(plr.currentpotion < 0)
				plr.currentpotion = 4;
			if(plr.currentpotion >= MAXPOTIONS)
				plr.currentpotion = 0;
		} else plr.currentpotion = key;
		SND_Sound(S_BOTTLES);
		potiontext(plr);
	}
}
	
void usapotion(PLAYER& plr) {

	if( plr.currentpotion == 0  && plr.health >= plr.maxhealth )
		return;
		
	if( plr.currentpotion == 2  && plr.poisoned == 0 )
		return;
		
	if( plr.potion[plr.currentpotion] <= 0)
		return;
	else
		plr.potion[plr.currentpotion]--;
		
	switch(plr.currentpotion) {
	case 0: // health potion
		if( plr.health+25 > plr.maxhealth) {
			plr.health=0;
			SND_Sound(S_DRINK);
			addhealth(plr, plr.maxhealth);
		}
		else {   
			SND_Sound(S_DRINK);
			addhealth(plr, 25);
		}
		startblueflash(10);
	break;
	case 1: // strength
		plr.strongtime=3200;
		SND_Sound(S_DRINK);
		startredflash(10);
	break;
	case 2: // anti venom
		SND_Sound(S_DRINK);
		plr.poisoned=0;
		plr.poisontime=0;
		startwhiteflash(10);
		showmessage("poison cured", 360);
		addhealth(plr, 0);
	break;
	case 3: // fire resist
		SND_Sound(S_DRINK);
		plr.manatime=3200;
		startwhiteflash(10);
		if( lavasnd != -1 ) {
			stopsound(lavasnd);
			lavasnd = -1;
		}
	break;
	case 4: // invisi
		SND_Sound(S_DRINK);
		plr.invisibletime=3200;
		startgreenflash(10);
	break;
	}
}
	
boolean potionspace(PLAYER& plr, int vial) {
	if(plr.potion[vial] > 9) 
		return false;
	else
		return true;
}
	
void updatepotion(PLAYER& plr, int vial) {
	switch(vial) {
		case HEALTHPOTION:
			plr.potion[0]++;
		break;
		case STRENGTHPOTION:
			plr.potion[1]++;
		break;
		case ARMORPOTION:
			plr.potion[2]++;
		break;
		case FIREWALKPOTION:
			plr.potion[3]++;
		break;
		case INVISIBLEPOTION:
			plr.potion[4]++;
		break;
	}
}
	
void potionpic(PLAYER& plr, int currentpotion, int x, int y, int scale) {
	int tilenum = SFLASKBLUE;
		
	if( netgame )
		return;
	x = x + mulscale(200, scale, 16);
	y = y - mulscale(94, scale, 16);
	engine.rotatesprite(x<<16,y<<16,scale,0,SPOTIONBACKPIC,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	engine.rotatesprite((x - mulscale(4, scale, 16))<<16,(y - mulscale(7, scale, 16))<<16,scale,0,SPOTIONARROW+currentpotion,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	
	x += mulscale(4, scale, 16);
	for(int i = 0; i < MAXPOTIONS; i++) {
		if(plr.potion[i] < 0)
			plr.potion[i] = 0;
		if(plr.potion[i] > 0) {
			switch(i) {
				case 1:
					tilenum=SFLASKGREEN;
				break;
				case 2:
					tilenum=SFLASKOCHRE;
				break;
				case 3:
					tilenum=SFLASKRED;
				break;
				case 4:
					tilenum=SFLASKTAN;
				break;
			}
			potiontilenum=tilenum;

			engine.rotatesprite((x + mulscale(i*20, scale, 16))<<16,(y + mulscale(19, scale, 16))<<16,scale,0,potiontilenum,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
			char potionbuf[50];
			Bitoa(plr.potion[i],potionbuf);

			game.getFont(3).drawText(x + mulscale(7 +(i*20), scale, 16),y+mulscale(7, scale, 16), potionbuf, scale, 0, 0, TextAlign.Left, 0, false);
		}
		else 
			engine.rotatesprite((x + mulscale(i*20, scale, 16))<<16,(y + mulscale(19, scale, 16))<<16,scale,0,SFLASKBLACK,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	}
}
	
void randompotion(int i) {
	if( (engine.krand()%100) > 20)
		return;

	int j=engine.insertsprite(sprite[i].sectnum,(short)0);

	sprite[j].x=sprite[i].x;
	sprite[j].y=sprite[i].y;
	sprite[j].z=sprite[i].z-(12<<8);
	sprite[j].shade=-12;
	sprite[j].pal=0;
	sprite[j].cstat=0;
	sprite[j].cstat&=~3;
	sprite[j].xrepeat=64;
	sprite[j].yrepeat=64;
	int type = engine.krand()%4;
	sprite[j].picnum = (short) (FLASKBLUE + type);
	sprite[j].detail = (short) (FLASKBLUETYPE + type);
}
