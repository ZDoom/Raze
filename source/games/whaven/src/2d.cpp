#include "ns.h"
#include "wh.h"
#include "screenjob.h"
#include "raze_music.h"
#include "raze_sound.h"

BEGIN_WH_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void IntroMovie(const CompletionFunc& completion)
{
	Mus_Stop();
	FX_StopAllSounds();

	if (userConfig.nologo)
	{
		completion(false);
		return;
	}
	else
	{
		JobDesc job = { PlayVideo(g_gameType & GAMEFLAG_WH2? "smk/intro.smk" : "intro.smk"), nullptr };
		RunScreenJob(&job, 1, completion, true, true);
	}
}



void showStatisticsScreen()
{
#if 0
	gStatisticsScreen.show(plr, new Runnable(){
		@Override
		public void run() {
			mapon++;
			spritesound(S_CHAINDOOR1, &sprite[plr.spritenum]);
			playertorch = 0;
			spritesound(S_WARP, &sprite[plr.spritenum]);
			loadnewlevel(mapon);
		}
		});
#endif
}

void startWh2Ending()
{
#if 0
					if (gCutsceneScreen.init("ending1.smk"))
					if (gCutsceneScreen.init("ending2.smk"))
					if (gCutsceneScreen.init("ending3.smk"))
					game.changeScreen(gMenuScreen);
#endif
}

void showVictoryScreen()
{
	//game.changeScreen(gVictoryScreen);
}

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

			fancyfont((266<<1)+(i*20),394,SPOTIONFONT-26,potionbuf,0);
			//game.getFont(3).drawText(x + mulscale(7 +(i*20), scale, 16),y+mulscale(7, scale, 16), potionbuf, scale, 0, 0, TextAlign.Left, 0, false);
		}
		else 
			engine.rotatesprite((x + mulscale(i*20, scale, 16))<<16,(y + mulscale(19, scale, 16))<<16,scale,0,SFLASKBLACK,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	}
#endif
}
#endif	

END_WH_NS
