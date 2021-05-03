#include "ns.h"
#include "wh.h"
#include "screenjob.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "v_draw.h"
#include "v_font.h"
#include "mapinfo.h"

BEGIN_WH_NS



#if 0
void orbpic(PLAYER& plr, int currentorb) {
	if (plr.orbammo[currentorb] < 0)
		plr.orbammo[currentorb] = 0;

#pragma message("fix orbpic")
#if 0
	itoa(plr->orbammo[currentorb],tempbuf,10);

	int y = 382;// was 389 originally.
	if (currentorb == 2)
		y = 381;
	if (currentorb == 3)
		y = 383;
	if (currentorb == 6)
		y = 383;
	if (currentorb == 7)
		y = 380;

	int spellbookpage = sspellbookanim[currentorb][8].daweaponframe;
	overwritesprite(121 << 1, y, spellbookpage, 0, 0, 0);
	fancyfont(126<<1,439,SSCOREFONT-26,tempbuf,0);
#endif
}


void potionpic(PLAYER& plr, int currentpotion, int x, int y, int scale) {
	int tilenum = SFLASKBLUE;
		
	if( netgame )
		return;
#pragma message("fix potionpic")
#if 0
	x = x + MulScale(200, scale, 16);
	y = y - MulScale(94, scale, 16);
	engine.rotatesprite(x<<16,y<<16,scale,0,SPOTIONBACKPIC,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	engine.rotatesprite((x - MulScale(4, scale, 16))<<16,(y - MulScale(7, scale, 16))<<16,scale,0,SPOTIONARROW+currentpotion,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	
	x += MulScale(4, scale, 16);
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

			engine.rotatesprite((x + MulScale(i*20, scale, 16))<<16,(y + MulScale(19, scale, 16))<<16,scale,0,potiontilenum,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
			char potionbuf[50];
			Bitoa(plr.potion[i],potionbuf);

			fancyfont((266<<1)+(i*20),394,SPOTIONFONT-26,potionbuf,0);
			//game.getFont(3).drawText(x + MulScale(7 +(i*20), scale, 16),y+MulScale(7, scale, 16), potionbuf, scale, 0, 0, TextAlign.Left, 0, false);
		}
		else 
			engine.rotatesprite((x + MulScale(i*20, scale, 16))<<16,(y + MulScale(19, scale, 16))<<16,scale,0,SFLASKBLACK,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	}
#endif
}
#endif	

END_WH_NS
