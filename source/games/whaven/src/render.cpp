#include "ns.h"
#include "wh.h"
#include "automap.h"
#include "mmulti.h"
#include "glbackend/glbackend.h"
#include "raze_music.h"

BEGIN_WH_NS


void drawscreen(int num, double dasmoothratio, bool sceneonly)
{

	PLAYER& plr = player[num];

	int cposx = plr.x;
	int cposy = plr.y;
	int cposz = plr.z;
	binangle cang = plr.angle.sum();
	fixedhoriz choriz = plr.horizon.horiz + plr.horizon.interpolatedoff(dasmoothratio);
	lookangle crotscrnang = plr.angle.rotscrnang;

	if (!paused)
	{
		auto& prevloc = gPrevPlayerLoc[num];

		cposx = prevloc.x + MulScale(cposx - prevloc.x, dasmoothratio, 16);
		cposy = prevloc.y + MulScale(cposy - prevloc.y, dasmoothratio, 16);
		cposz = prevloc.z + MulScale(cposz - prevloc.z, dasmoothratio, 16);

		if (cl_syncinput)
		{
			cang = plr.angle.interpolatedsum(dasmoothratio);
			choriz = plr.horizon.interpolatedsum(dasmoothratio);
			crotscrnang = plr.angle.interpolatedrotscrn(dasmoothratio);
		}
	}

	// wango
    if ((gotpic[FLOORMIRROR >> 3] & (1 << (FLOORMIRROR & 7))) != 0) {
            int dist = 0x7fffffff;
            int i = 0, j;
            for (int k = floormirrorcnt - 1; k >= 0; k--) {
            	int sect = floormirrorsector[k];
            	if((gotsector[sect >> 3] & (1 << (sect & 7))) == 0) continue;
                j = klabs(wall[sector[sect].wallptr].x - plr.x);
                j += klabs(wall[sector[sect].wallptr].y - plr.y);
                if (j < dist) {
                    dist = j; i = k;
                }
            }

			// Todo: render this with 30% light only.
			inpreparemirror = true;
			renderSetRollAngle(1024);
			renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), floormirrorsector[i]);
			analyzesprites(plr, dasmoothratio);
			renderDrawMasks();
			renderSetRollAngle(0);
			inpreparemirror = false;
   
            gotpic[FLOORMIRROR >> 3] &= ~(1 << (FLOORMIRROR & 7));  
    }

	int ceilz, floorz;
	getzsofslope(plr.sector, cposx, cposy, &ceilz, &floorz);
	int lz = 4 << 8;
	if (cposz < ceilz + lz)
		cposz = ceilz + lz;
	if (cposz > floorz - lz)
		cposz = floorz - lz;

	// do screen rotation.
	renderSetRollAngle(crotscrnang.asbam() / (double)(BAMUNIT));

	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), plr.sector);
	analyzesprites(plr, dasmoothratio);
	renderDrawMasks();
	if (!sceneonly)
	{
		DrawHud(dasmoothratio);
		if (automapMode != am_off)
		{
			DrawOverheadMap(cposx, cposy, cang.asbuild());
		}
	}
}

void GameInterface::Render()
{
	double const smoothRatio = playrunning() ? I_GetTimeFrac() * MaxSmoothRatio : MaxSmoothRatio;

	drawscreen(pyrn, smoothRatio, false); 
	if (!paused && isWh2() && attacktheme && !Mus_IsPlaying())
	{
		startsong(krand() % 2);
    	attacktheme = 0;
	}
}

bool GameInterface::GenerateSavePic()
{
	drawscreen(pyrn, FRACUNIT, true);
	return true;
}


END_WH_NS
