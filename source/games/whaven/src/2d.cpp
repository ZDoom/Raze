#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

void showStatisticsScreen()
{
#if 0
	gStatisticsScreen.show(plr, new Runnable(){
		@Override
		public void run() {
			mapon++;
			playsound_loc(S_CHAINDOOR1, plr.x, plr.y);
			playertorch = 0;
			playsound_loc(S_WARP, plr.x, plr.y);
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

END_WH_NS
