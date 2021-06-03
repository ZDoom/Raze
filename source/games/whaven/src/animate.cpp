#include "ns.h"
#include "wh.h"
#include "interpolate.h"

BEGIN_WH_NS


ANIMATION gAnimationData[MAXANIMATES];
int gAnimationCount;

int getanimationgoal(sectortype& object, int type)
{
	int j = -1;
	for (int i = gAnimationCount - 1; i >= 0; i--)
		if (&object == &sector[gAnimationData[i].id] && type == gAnimationData[i].type)
		{
			j = i; break;
		}
	return(j);
}

static int getanimationgoal(int object, int type)
{
	int j = -1;
	for (int i = gAnimationCount - 1; i >= 0; i--)
		if (object == gAnimationData[i].id && type == gAnimationData[i].type)
		{
			j = i; break;
		}
	return(j);
}

int setanimation(int index, int thegoal, int thevel, int theacc, int type)
{
	if (gAnimationCount >= MAXANIMATES) return -1;

	int j = getanimationgoal(index, type);
	if (j == -1) j = gAnimationCount;

	ANIMATION& gAnm = gAnimationData[j];
	gAnm.id = (short)index;
	gAnm.goal = thegoal;
	gAnm.vel = thevel;
	gAnm.acc = theacc;
	gAnm.type = (byte)type;

	switch (gAnm.type)
	{
	case WALLX:
		StartInterpolation(gAnm.id, Interp_Wall_X);
		break;
	case WALLY:
		StartInterpolation(gAnm.id, Interp_Wall_Y);
		break;
	case FLOORZ:
		StartInterpolation(gAnm.id, Interp_Sect_Floorz);
		break;
	case CEILZ:
		StartInterpolation(gAnm.id, Interp_Sect_Ceilingz);
		break;
	}

	if (j == gAnimationCount) gAnimationCount++;

	return j;
}
	
void doanimations()
{
	int j = 0;
	for (int i = gAnimationCount - 1; i >= 0; i--)
	{
		ANIMATION& gAnm = gAnimationData[i];
		switch (gAnm.type)
		{
		case WALLX:
			j = wall[gAnm.id].x;
			if (j < gAnm.goal)
				wall[gAnm.id].x = min(j + gAnm.vel * TICSPERFRAME, gAnm.goal);
			else
				wall[gAnm.id].x = max(j - gAnm.vel * TICSPERFRAME, gAnm.goal);
			break;
		case WALLY:
			j = wall[gAnm.id].y;
			if (j < gAnm.goal)
				wall[gAnm.id].y = min(j + gAnm.vel * TICSPERFRAME, gAnm.goal);
			else
				wall[gAnm.id].y = max(j - gAnm.vel * TICSPERFRAME, gAnm.goal);
			break;
		case FLOORZ:
			j = sector[gAnm.id].floorz;
			if (j < gAnm.goal)
				sector[gAnm.id].floorz = min(j + gAnm.vel * TICSPERFRAME, gAnm.goal);
			else
				sector[gAnm.id].floorz = max(j - gAnm.vel * TICSPERFRAME, gAnm.goal);
			break;
		case CEILZ:
			j = sector[gAnm.id].ceilingz;
			if (j < gAnm.goal)
				sector[gAnm.id].ceilingz = min(j + gAnm.vel * TICSPERFRAME, gAnm.goal);
			else
				sector[gAnm.id].ceilingz = max(j - gAnm.vel * TICSPERFRAME, gAnm.goal);
			break;
		}
		gAnm.vel += gAnm.acc;

		if (j == gAnm.goal)
		{
			switch (gAnm.type)
			{
			case WALLX:
				StopInterpolation(gAnm.id, Interp_Wall_X);
				break;
			case WALLY:
				StopInterpolation(gAnm.id, Interp_Wall_Y);
				break;
			case FLOORZ:
				StopInterpolation(gAnm.id, Interp_Sect_Floorz);
				break;
			case CEILZ:
				StopInterpolation(gAnm.id, Interp_Sect_Ceilingz);
				break;
			}

			gAnimationCount--;
			if (i != gAnimationCount)
				gAnm = gAnimationData[gAnimationCount];
		}
	}
}

END_WH_NS
