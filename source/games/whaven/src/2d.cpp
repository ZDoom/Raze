#include "ns.h"
#include "wh.h"
#include "screenjob.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "v_draw.h"
#include "v_font.h"
#include "mapinfo.h"

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



static const char* ratings[] = { "poor", "average", "good", "perfect" };

/*
		inited = false;
		if (init("stairs.smk"))
			inited = true;
*/
class DStatisticsScreen : public DScreenJob
{

	boolean inited = false;
	int bonus, rating;

public:
	DStatisticsScreen(PLAYER& plr)
	{
		if (kills > killcnt)
			kills = killcnt;
		int killp = (kills * 100) / (killcnt + 1);
		if (treasuresfound > treasurescnt)
			treasuresfound = treasurescnt;
		int treap = (treasuresfound * 100) / (treasurescnt + 1);
		rating = (killp + treap) / 2;
		if (rating >= 95) {
			rating = 3;
		}
		else if (rating >= 70)
			rating = 2;
		else if (rating >= 40)
			rating = 1;
		else rating = 0;
		bonus = rating * 500;
		plr.score += bonus;
	}

	void drawText(int x, int y, const char* text)
	{
		DrawText(twod, SmallFont, CR_UNTRANSLATED, x, y, text, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
	}

	int Frame(uint64_t nsclock, bool skiprequest) override
	{
		if (nsclock == 0) SND_Sound(S_CHAINDOOR1);
		DrawTexture(twod, tileGetTexture(VMAINBLANK), 0, 0, DTA_Fullscreen, FSMode_ScaleToFit43, TAG_DONE);

		drawText(10, 13, currentLevel->DisplayName());
		drawText(10, 31, GStrings("Level conquered"));

		drawText(10, 64, GStrings("Enemies killed"));
		drawText(160 + 48 + 14, 64, FStringf("%d %s %d", kills, GStrings("TXT_OF"), killcnt));

		drawText(10, 64 + 18, GStrings("Treasures found"));
		drawText(160 + 48 + 14, 64 + 18, FStringf("%d %s %d", treasuresfound, GStrings("TXT_OF"), treasurescnt));

		drawText(10, 64 + 2 * 18, GStrings("Experience gained"));
		drawText(160 + 48 + 14, 64 + 2 * 18, FStringf("%s", (expgained + bonus)));

		drawText(10, 64 + 3 * 18, GStrings("Rating"));
		drawText(160 + 48 + 14, 64 + 3 * 18, FStringf("%d", GStrings(ratings[rating])));

		drawText(10, 64 + 4 * 18, GStrings("TXT_Bonus"));
		drawText(160 + 48 + 14, 64 + 4 * 18, FStringf("%d", bonus));

		return skiprequest ? -1 : 1;
	}

};


void showStatisticsScreen(CompletionFunc completion)
{
	JobDesc job = { Create<DStatisticsScreen>(player[pyrn]), nullptr };
	RunScreenJob(&job, 1, completion, true, false);
}

void startWh2Ending(CompletionFunc completion)
{
	JobDesc jobs[3];
	jobs[0] = { PlayVideo("smk/ending1.smk", nullptr) };
	jobs[1] = { PlayVideo("smk/ending2.smk", nullptr) };
	jobs[2] = { PlayVideo("smk/ending3.smk", nullptr) };
	RunScreenJob(jobs, 3, completion, true, false);
}

void showVictoryScreen(CompletionFunc completion)
{
	JobDesc jobs[3];
	jobs[0] = { Create<DImageScreen>(VICTORYA, DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff), []() { SND_Sound(S_DROPFLAG); } };
	jobs[1] = { Create<DImageScreen>(VICTORYB, DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff), []() { SND_Sound(S_WISP2); } };
	jobs[2] = { Create<DImageScreen>(VICTORYC, DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff) };
	RunScreenJob(jobs, 3, completion, true, false);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DWHLoadScreen : public DScreenJob
{
	MapRecord* rec;

public:
	DWHLoadScreen(MapRecord* maprec) : DScreenJob(0), rec(maprec) {}

	void drawText(int x, int y, const char* text)
	{
		DrawText(twod, SmallFont, CR_UNTRANSLATED, x - SmallFont->StringWidth(text)/2, y, text, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(MAINMENU), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

		drawText(160, 100, GStrings("TXT_LOADING"));
		drawText(160, 114, GStrings("TXTB_PLSWAIT"));
		return 0;
	}
};

void loadscreen(MapRecord* rec, CompletionFunc func)
{
	JobDesc job = { Create<DWHLoadScreen>(rec) };
	RunScreenJob(&job, 1, func);
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
