#include "ns.h"
#include "wh.h"
#include "automap.h"
#include "mmulti.h"
#include "glbackend/glbackend.h"
#include "raze_music.h"
#include "rendering/render.h"

EXTERN_CVAR(Bool, testnewrenderer)

BEGIN_WH_NS

int curpnum;

void drawscreen(int num, double dasmoothratio, bool sceneonly)
{
	curpnum = num;
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

		if (plr.over_shoulder_on)
		{
			sprite[plr.spritenum].cstat |= CSTAT_SPRITE_TRANSLUCENT;

			cposz -= 2560;
			if (!calcChaseCamPos(&cposx, &cposy, &cposz, &sprite[plr.spritenum], &plr.sector, cang, choriz, dasmoothratio))
			{
				cposz += 2560;
				calcChaseCamPos(&cposx, &cposy, &cposz, &sprite[plr.spritenum], &plr.sector, cang, choriz, dasmoothratio);
			}
		}
		else
		{
			sprite[plr.spritenum].cstat &= ~CSTAT_SPRITE_TRANSLUCENT;
		}
	}

	// wango
    if (!testnewrenderer && (gotpic[FLOORMIRROR >> 3] & (1 << (FLOORMIRROR & 7))) != 0) {
            int dist = 0x7fffffff;
            int i = 0, j;
            for (int k = floormirrorcnt - 1; k >= 0; k--) {
            	int sect = floormirrorsector[k];
            	if((gotsector[sect >> 3] & (1 << (sect & 7))) == 0) continue;
                j = abs(wall[sector[sect].wallptr].x - plr.x);
                j += abs(wall[sector[sect].wallptr].y - plr.y);
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

	if (!testnewrenderer)
	{
		renderSetRollAngle(crotscrnang.asbuildf());
		renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), plr.sector);
		analyzesprites(plr, dasmoothratio);
		renderDrawMasks();
	}
	else
	{
		render_drawrooms(nullptr, { cposx, cposy, cposz }, plr.sector, cang, choriz, crotscrnang);
	}

	if (!sceneonly)
	{
		if (!plr.over_shoulder_on)
		{
			DrawHud(dasmoothratio);
		}

		if (automapMode != am_off)
		{
			DrawOverheadMap(cposx, cposy, cang.asbuild(), dasmoothratio);
		}
	}
}

void GameInterface::processSprites(int viewx, int viewy, int viewz, binangle viewang, double smoothRatio)
{
	PLAYER& plr = player[curpnum];
	analyzesprites(plr, (int)smoothRatio);
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
