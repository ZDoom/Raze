#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

int d_soundplayed = 0;
int delaycnt;
Delayitem delayitem[MAXSECTORS];

short ironbarsector[16];
short ironbarscnt;
int ironbarsgoal1[16], ironbarsgoal2[16];
short ironbarsdone[16], ironbarsanim[16];
int ironbarsgoal[16];

short warpsectorlist[64], warpsectorcnt;
short xpanningsectorlist[16], xpanningsectorcnt;
short ypanningwalllist[128], ypanningwallcnt;
short floorpanninglist[64], floorpanningcnt;
SwingDoor swingdoor[MAXSWINGDOORS];
short swingcnt;

short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
int dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];


void operatesprite(PLAYER& plr, short s) {
	if (sprite[s].picnum == SPAWNFIREBALL)
		newstatus(s, DEVILFIRE);
	if (sprite[s].picnum == SPAWNJAVLIN)
		trowajavlin(s);

	switch (sprite[s].picnum) {
	case STONEGONZOCHM:
	case STONEGONZOGSH:
	case STONEGRONDOVAL:
	case STONEGONZOBSW:
		sprite[s].lotag *= 120;
		changespritestat((short) s, STONETOFLESH);
		break;
	case GONZOHMJUMP:
	case GONZOSHJUMP:
		newstatus((short) s, AMBUSH);
		break;
	case STAINGLASS1:
	case STAINGLASS2:
	case STAINGLASS3:
	case STAINSKULL:
	case STAINHEAD:
	case STAINSNAKE:
	case STAINCIRCLE:
	case STAINQ:
	case STAINSCENE:
		switch (sprite[s].lotag) {
		case 2:
			playsound_loc(S_GLASSBREAK1 + (rand() % 3), sprite[s].x, sprite[s].y);
			for (int j = 0; j < 20; j++) {
				shards(s, 2);
			}
			deletesprite((short) s);
			break;
		}
		break;
	}

	if ((sprite[s].lotag == 1800 || sprite[s].lotag == 1810 || sprite[s].lotag == 1820)
			&& sprite[s].sectnum == plr.sector) {
		for (short j = 0; j < MAXSPRITES; j++) {
			if (sprite[s].sectnum == sprite[j].sectnum && (sprite[j].lotag >= 1800 && sprite[j].lotag <= 1899))
				newstatus(j, LIFTDN);
		}
	}
	if ((sprite[s].lotag == 1801 || sprite[s].lotag == 1811 || sprite[s].lotag == 1821)
			&& sprite[s].sectnum == plr.sector) {
		for (short j = 0; j < MAXSPRITES; j++) {
			if (sprite[s].sectnum == sprite[j].sectnum && (sprite[j].lotag >= 1800 && sprite[j].lotag <= 1899))
				newstatus(j, LIFTUP);
		}
	}
}


void operatesector(PLAYER& plr, int s) {
	int botz, dax2, day2, goalz, i, j, size, topz;
	int daz;
	int doorantic, doorkey, doortype;

	int temp1, temp2, temp3;
	short k;

	int keysok = 0;
	int datag = sector[s].lotag;
	int startwall = sector[s].wallptr;
	int endwall = startwall + sector[s].wallnum - 1;
	int centx = 0, centy = 0;
	int opwallfind[2] = {};

	for (i = startwall; i <= endwall; i++) {
		centx += wall[i].x;
		centy += wall[i].y;
	}
	centx /= (endwall - startwall + 1);
	centy /= (endwall - startwall + 1);

	switch (datag) {
	case 61:
	case 131:
		// check for proper key
		if (plr.treasure[TBRASSKEY] == 0) {
			keysok = 0;
			showmessage("BRASS KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 62:
	case 132:
		// check for proper key
		if (plr.treasure[TBLACKKEY] == 0) {
			keysok = 0;
			showmessage("Black KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 63:
	case 133:
		// check for proper key
		if (plr.treasure[TGLASSKEY] == 0) {
			keysok = 0;
			showmessage("glass KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 64:
	case 134:
		if (plr.treasure[TIVORYKEY] == 0) {
			keysok = 0;
			showmessage("ivory KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 71:
		// check for proper key
		if (plr.treasure[TBRASSKEY] == 0) {
			keysok = 0;
			showmessage("BRASS KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 72:
		// check for proper key
		if (plr.treasure[TBLACKKEY] == 0) {
			keysok = 0;
			showmessage("black KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 73:
		// check for proper key
		if (plr.treasure[TGLASSKEY] == 0) {
			keysok = 0;
			showmessage("glass KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	case 74:
		if (plr.treasure[TIVORYKEY] == 0) {
			keysok = 0;
			showmessage("Ivory KEY NEEDED", 360);
		} else
			keysok = 1;
		break;
	}

	switch (datag) {
	case DOORBOX:
		opwallfind[0] = -1;
		opwallfind[1] = -1;
		for (i = startwall; i <= endwall; i++) {
			if (wall[i].lotag == 6) {
				if (opwallfind[0] == -1)
					opwallfind[0] = i;
				else
					opwallfind[1] = i;
			}
		}

		for (j = 0; j < 2; j++) {
			if(opwallfind[j] == -1)
				break;
				
			if ((((wall[opwallfind[j]].x + wall[wall[opwallfind[j]].point2].x) >> 1) == centx)
					&& (((wall[opwallfind[j]].y + wall[wall[opwallfind[j]].point2].y) >> 1) == centy)) {
				i = opwallfind[j] - 1;
				if (i < startwall)
					i = endwall;
				dax2 = wall[i].x - wall[opwallfind[j]].x;
				day2 = wall[i].y - wall[opwallfind[j]].y;
				if (dax2 != 0) {
					dax2 = wall[wall[wall[wall[opwallfind[j]].point2].point2].point2].x;
					dax2 -= wall[wall[wall[opwallfind[j]].point2].point2].x;
					setanimation(opwallfind[j], wall[opwallfind[j]].x + dax2, 4, 0, WALLX);
					setanimation(i, wall[i].x + dax2, 4, 0, WALLX);
					setanimation(wall[opwallfind[j]].point2, wall[wall[opwallfind[j]].point2].x + dax2, 4, 0,
							WALLX);
					setanimation(wall[wall[opwallfind[j]].point2].point2,
							wall[wall[wall[opwallfind[j]].point2].point2].x + dax2, 4, 0, WALLX);
				} else if (day2 != 0) {
					day2 = wall[wall[wall[wall[opwallfind[j]].point2].point2].point2].y;
					day2 -= wall[wall[wall[opwallfind[j]].point2].point2].y;
					setanimation(opwallfind[j], wall[opwallfind[j]].y + day2, 4, 0, WALLY);
					setanimation(i, wall[i].y + day2, 4, 0, WALLY);
					setanimation(wall[opwallfind[j]].point2, wall[wall[opwallfind[j]].point2].y + day2, 4, 0,
							WALLY);
					setanimation(wall[wall[opwallfind[j]].point2].point2,
							wall[wall[wall[opwallfind[j]].point2].point2].y + day2, 4, 0, WALLY);
				}
			} else {
				i = opwallfind[j] - 1;
				if (i < startwall)
					i = endwall;
				dax2 = wall[i].x - wall[opwallfind[j]].x;
				day2 = wall[i].y - wall[opwallfind[j]].y;
				if (dax2 != 0) {
					setanimation(opwallfind[j], centx, 4, 0, WALLX);
					setanimation(i, centx + dax2, 4, 0, WALLX);
					setanimation(wall[opwallfind[j]].point2, centx, 4, 0, WALLX);
					setanimation(wall[wall[opwallfind[j]].point2].point2, centx + dax2, 4, 0, WALLX);
				} else if (day2 != 0) {
					setanimation(opwallfind[j], centy, 4, 0, WALLY);
					setanimation(i, centy + day2, 4, 0, WALLY);
					setanimation(wall[opwallfind[j]].point2, centy, 4, 0, WALLY);
					setanimation(wall[wall[opwallfind[j]].point2].point2, centy + day2, 4, 0, WALLY);
				}
			}
		}

		SND_Sound(S_DOOR2);

		break;
	case DOORUPTAG: // a door that opens up
		i = getanimationgoal(sector[s], 2);
		if (i >= 0) {
			goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
			if (gAnimationData[i].goal == goalz) {
				gAnimationData[i].goal = sector[s].floorz;
			} else {
				gAnimationData[i].goal = goalz;
			}
		} else {
			if (sector[s].ceilingz == sector[s].floorz) {
				goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;

			} else {
				goalz = sector[s].floorz;

			}
			setanimation(s, goalz, DOORSPEED, 0, CEILZ);
		}
		SND_Sound(S_DOOR2);
		break;

	case DOORDOWNTAG: // a door that opens down
		i = getanimationgoal(sector[s], 1);
		if (i >= 0) {
			goalz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
			if (gAnimationData[i].goal == goalz) {
				gAnimationData[i].goal = sector[s].ceilingz;
			} else {
				gAnimationData[i].goal = goalz;
			}
		} else {
			if (sector[s].ceilingz == sector[s].floorz) {
				goalz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
			} else {
				goalz = sector[s].ceilingz;
			}
			setanimation(s, goalz, DOORSPEED, 0, FLOORZ);
		}
		SND_Sound(S_DOOR1);
		break;

	case PLATFORMELEVTAG:
		i = getanimationgoal(sector[s], 1);
		goalz = sector[plr.sector].floorz;
		if (i >= 0) {
			gAnimationData[i].goal = goalz;
		} else {
			setanimation(s, goalz, ELEVSPEED, 0, FLOORZ);
		}
		break;
	case BOXELEVTAG:
		i = getanimationgoal(sector[s], 1);
		j = getanimationgoal(sector[s], 2);
		size = sector[s].ceilingz - sector[s].floorz;
		goalz = sector[plr.sector].floorz;
		if (i >= 0) {

			gAnimationData[i].goal = goalz;
		} else {
			setanimation(s, goalz, ELEVSPEED, 0, FLOORZ);
		}
		goalz = goalz + size;
		if (j >= 0) {
			gAnimationData[j].goal = goalz;
		} else {
			setanimation(s, goalz, ELEVSPEED, 0, CEILZ);
		}
		break;
	case DOORSPLITHOR:
		i = getanimationgoal(sector[s], 1);
		j = getanimationgoal(sector[s], 2);
		if (i >= 0) {
			botz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
			if (gAnimationData[i].goal == botz) {
				gAnimationData[i].goal = (sector[s].ceilingz + sector[s].floorz) >> 1;
			} else {
				gAnimationData[i].goal = botz;
			}
		} else {
			if (sector[s].ceilingz == sector[s].floorz) {
				botz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;

			} else {
				botz = (sector[s].ceilingz + sector[s].floorz) >> 1;

			}
			setanimation(s, botz, ELEVSPEED, 0, FLOORZ);
		}
		if (j >= 0) {
			topz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
			if (gAnimationData[j].goal == topz) {
				gAnimationData[j].goal = (sector[s].ceilingz + sector[s].floorz) >> 1;
			} else {
				gAnimationData[j].goal = topz;
			}
		} else {
			if (sector[s].ceilingz == sector[s].floorz) {
				topz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
			} else {
				topz = (sector[s].ceilingz + sector[s].floorz) >> 1;
			}
			setanimation(s, topz, ELEVSPEED, 0, CEILZ);
		}
		SND_Sound(S_DOOR1 + (krand() % 3));
		break;
	case DOORSPLITVER:

		opwallfind[0] = -1;
		opwallfind[1] = -1;
		for (i = startwall; i <= endwall; i++)
			if ((wall[i].x == centx) || (wall[i].y == centy)) {
				if (opwallfind[0] == -1)
					opwallfind[0] = i;
				else
					opwallfind[1] = i;
			}

		for (j = 0; j < 2; j++) {
			if ((wall[opwallfind[j]].x == centx) && (wall[opwallfind[j]].y == centy)) {
				i = opwallfind[j] - 1;
				if (i < startwall)
					i = endwall;
				dax2 = ((wall[i].x + wall[wall[opwallfind[j]].point2].x) >> 1) - wall[opwallfind[j]].x;
				day2 = ((wall[i].y + wall[wall[opwallfind[j]].point2].y) >> 1) - wall[opwallfind[j]].y;
				if (dax2 != 0) {
					dax2 = wall[wall[wall[opwallfind[j]].point2].point2].x;
					dax2 -= wall[wall[opwallfind[j]].point2].x;
					setanimation(opwallfind[j], wall[opwallfind[j]].x + dax2, 4, 0, WALLX);
					setanimation(i, wall[i].x + dax2, 4, 0, WALLX);
					setanimation(wall[opwallfind[j]].point2, wall[wall[opwallfind[j]].point2].x + dax2, 4, 0,
							WALLX);
				} else if (day2 != 0) {
					day2 = wall[wall[wall[opwallfind[j]].point2].point2].y;
					day2 -= wall[wall[opwallfind[j]].point2].y;
					setanimation(opwallfind[j], wall[opwallfind[j]].y + day2, 4, 0, WALLY);
					setanimation(i, wall[i].y + day2, 4, 0, WALLY);
					setanimation(wall[opwallfind[j]].point2, wall[wall[opwallfind[j]].point2].y + day2, 4, 0,
							WALLY);
				}
			} else {
				i = opwallfind[j] - 1;
				if (i < startwall)
					i = endwall;
				dax2 = ((wall[i].x + wall[wall[opwallfind[j]].point2].x) >> 1) - wall[opwallfind[j]].x;
				day2 = ((wall[i].y + wall[wall[opwallfind[j]].point2].y) >> 1) - wall[opwallfind[j]].y;
				if (dax2 != 0) {
					setanimation(opwallfind[j], centx, 4, 0, WALLX);
					setanimation(i, centx + dax2, 4, 0, WALLX);
					setanimation(wall[opwallfind[j]].point2, centx + dax2, 4, 0, WALLX);
				} else if (day2 != 0) {
					setanimation(opwallfind[j], centy, 4, 0, WALLY);
					setanimation(i, centy + day2, 4, 0, WALLY);
					setanimation(wall[opwallfind[j]].point2, centy + day2, 4, 0, WALLY);
				}
			}
		}
		break;

	case 131:
		if (keysok == 0)
			break;
		else {
			playsound(S_CREAKDOOR2, 0, 0, 0);
			d_soundplayed = 1;
		}

	case 132:
		if (keysok == 0)
			break;
		else {
			playsound(S_CREAKDOOR2, 0, 0, 0);
			d_soundplayed = 1;
		}

	case 133:
		if (keysok == 0)
			break;
		else {
			playsound(S_CREAKDOOR2, 0, 0, 0);
			d_soundplayed = 1;
		}

	case 134:
		if (keysok == 0)
			break;
		else {
			playsound(S_CREAKDOOR2, 0, 0, 0);
			d_soundplayed = 1;
		}

	case DOORSWINGTAG:
		if (isWh2() && d_soundplayed == 0)
			playsound(S_SWINGDOOR, 0, 0, 0);
		else d_soundplayed = 1;

		for (i = 0; i < swingcnt; i++) {
			if (swingdoor[i].sector == s) {
				if (swingdoor[i].anginc == 0) {
					if (swingdoor[i].ang == swingdoor[i].angclosed) {
						swingdoor[i].anginc = swingdoor[i].angopendir;
					} else {
						swingdoor[i].anginc = -swingdoor[i].angopendir;
					}
				} else {
					swingdoor[i].anginc = -swingdoor[i].anginc;
				}
			}
		}
		break;
	} // switch
		//
		// LOWER FLOOR ANY AMOUNT
		//
	if (datag >= 1100 && datag <= 1199) {

		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		sector[s].hitag = 0;

		daz = sector[s].floorz + (1024 * (sector[s].lotag - 1100));

		if ((j = setanimation(s, daz, speed, 0, FLOORZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1100) / 10);
		}
		sector[s].lotag = 0;
	}

	//
	// RAISE FLOOR 1-99
	//
	if (datag >= 1200 && datag <= 1299) {
		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		sector[s].hitag = 0;

		switch (sector[s].floorpicnum) {
		case LAVA:
		case ANILAVA:
		case LAVA1:
			sector[s].floorpicnum = COOLLAVA;
			break;
		case SLIME:
			sector[s].floorpicnum = DRYSLIME;
			break;
		case WATER:
		case HEALTHWATER:
			sector[s].floorpicnum = DRYWATER;
			break;
		case LAVA2:
			sector[s].floorpicnum = COOLLAVA2;
			break;
		}
		// XXX
		daz = sector[s].floorz - (1024 * (sector[s].lotag - 1200));

		if ((j = setanimation(s, daz, speed, 0, FLOORZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1200) / 10);
		}
		sector[s].lotag = 0;
	}

	if (datag >= 1300 && datag <= 1399) {
		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		sector[s].hitag = 0;

		daz = sector[s].ceilingz + (1024 * (sector[s].lotag - 1300));

		if ((j = setanimation(s, daz, speed, 0, CEILZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1300) / 10);
		}
		sector[s].lotag = 0;
	}

	// RAISE CEILING ANY AMOUNT
	if (datag >= 1400 && datag <= 1499) {
		sector[s].hitag = 0;
		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		daz = sector[s].ceilingz - (1024 * (sector[s].lotag - 1400));

		if ((j = setanimation(s, daz, speed, 0, CEILZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1400) / 10);
		}
		sector[s].lotag = 0;
	}

	/*********
		* LOWER FLOOR AND CEILING ANY AMOUNT
		*********/

	if (datag >= 1500 && datag <= 1599) {
		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		sector[s].hitag = 0;

		daz = sector[s].floorz + (1024 * (sector[s].lotag - 1500));

		setanimation(s, daz, speed, 0, FLOORZ);

		daz = sector[s].ceilingz + (1024 * (sector[s].lotag - 1500));

		if ((j = setanimation(s, daz, speed, 0, CEILZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1500) / 10);
		}
		sector[s].lotag = 0;
	}

	//
	// RAISE FLOOR AND CEILING ANY AMOUNT
	//
	if (datag >= 1600 && datag <= 1699) {

		int speed = 32;
		if (sector[s].hitag > 100)
			speed = 64;

		sector[s].hitag = 0;

		daz = sector[s].floorz - (1024 * (sector[s].lotag - 1600));

		setanimation(s, daz, speed, 0, FLOORZ);

		daz = sector[s].ceilingz - (1024 * (sector[s].lotag - 1600));

		if ((j = setanimation(s, daz, speed, 0, CEILZ)) >= 0) {
			playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1600) / 10);
		}
		sector[s].lotag = 0;
	}

	if (datag >= 1800 && datag <= 1899) {
		i = getanimationgoal(sector[s], 1);
		if (i >= 0) {
			daz = sector[s].ceilingz + (1024 * 16);

			if (gAnimationData[i].goal == daz)
				gAnimationData[i].goal = sector[nextsectorneighborz(s, sector[s].ceilingz - (1024 * 16), -1,
						-1)].floorz;
			else
				gAnimationData[i].goal = daz;
		} else {
			if (sector[s].floorz == sector[s].ceilingz + (1024 * 16))
				daz = sector[nextsectorneighborz(s, sector[s].ceilingz - (1024 * 16), -1, -1)].floorz;
			else {
				daz = sector[s].ceilingz + (1024 * 16);
			}
			if ((j = setanimation(s, daz, 32, 0, FLOORZ)) >= 0) {
				playsound(S_STONELOOP1, 0, 0, (sector[s].lotag - 1800) / 10);
			}
		}
	}

	if (datag >= 1900 && datag <= 1999) {

		sector[s].hitag = 0;
		temp1 = sector[s].lotag - 1900;
		temp2 = temp1 / 10;
		temp3 = temp1 - temp2;

		SND_Sound(S_STONELOOP1);

		switch (temp3) { // type of crush
		case 0:
			sector[s].lotag = DOORDOWNTAG;
			setanimation(s, sector[s].ceilingz, 64, 0, FLOORZ);
			break;
		case 1:
			daz = sector[s].ceilingz;
			setanimation(s, daz, 64, 0, FLOORZ);
			sector[s].lotag = 0;
//				crushsectoranim[s]=0;
//				crushsectordone[s]=0;
			break;
		case 2:
			daz = sector[s].floorz;
			setanimation(s, daz, 64, 0, CEILZ);
			sector[s].lotag = 0;
//				crushsectoranim[s]=0;
//				crushsectordone[s]=0;
			break;
		case 3:
			sector[s].lotag = 0;
//				crushsectoranim[s]=1;
//				crushsectordone[s]=1;
			break;
		case 4:
			sector[s].lotag = 0;
//				crushsectoranim[s]=2;
//				crushsectordone[s]=1;
			break;
		case 5:
			daz = (sector[s].ceilingz + sector[s].floorz) >> 1;
			setanimation(s, daz, 64, 0, FLOORZ);
			setanimation(s, daz, 64, 0, CEILZ);
			break;
		case 6:
			sector[s].lotag = 0;
//				crushsectoranim[s]=3;
//				crushsectordone[s]=1;
			break;
		}
	}

	// BRASS KEY
	// BLACK KEY
	// GLASS KEY
	// IVORY KEY

	if (datag >= 2000 && datag <= 2999) {
		Printf("WHTAG.java: 683 check this place keychecking"); //XXX
		doorkey = (sector[s].lotag - 2000) / 100;
		doorantic = (sector[s].lotag - (2000 + (doorkey * 100))) / 10;
		doortype = sector[s].lotag - (2000 + (doorkey * 100) + (doorantic * 10));
		boolean checkforkey = false;
		for (i = 0; i < MAXKEYS; i++) {
			if (plr.treasure[i] == doorkey)
				checkforkey = true;
		}
		if (checkforkey) {
			if (doorantic == 0)
				sector[s].lotag = 0;
			switch (doortype) {
			case 0: // up
				i = getanimationgoal(sector[s], 2);
				if (i >= 0) {
					goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
					if (gAnimationData[i].goal == goalz) {
						gAnimationData[i].goal = sector[s].floorz;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					} else {
						gAnimationData[i].goal = goalz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					}
				} else {
					if (sector[s].ceilingz == sector[s].floorz) {
						goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					} else {
						goalz = sector[s].floorz;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					}
					setanimation(s, goalz, DOORSPEED, 0, CEILZ);
				}
				break;
			case 1: // dn
				i = getanimationgoal(sector[s], 1);
				if (i >= 0) {
					goalz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
					if (gAnimationData[i].goal == goalz) {
						gAnimationData[i].goal = sector[s].ceilingz;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					} else {
						gAnimationData[i].goal = goalz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					}
				} else {
					if (sector[s].ceilingz == sector[s].floorz) {
						goalz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					} else {
						goalz = sector[s].ceilingz;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					}
					setanimation(s, goalz, DOORSPEED, 0, FLOORZ);
				}
				break;
			case 2: // middle
				i = getanimationgoal(sector[s], 1);
				j = getanimationgoal(sector[s], 2);
				if (i >= 0) {
					botz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
					if (gAnimationData[i].goal == botz) {
						gAnimationData[i].goal = (sector[s].ceilingz + sector[s].floorz) >> 1;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					} else {
						gAnimationData[i].goal = botz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					}
				} else {
					if (sector[s].ceilingz == sector[s].floorz) {
						botz = sector[nextsectorneighborz(s, sector[s].ceilingz, 1, 1)].floorz;
						if (doorantic == 2)
							setdelayfunc(s, DOORDELAY);
						else if (doorantic == 3)
							setdelayfunc(s, DOORDELAY << 3);
					} else {
						botz = (sector[s].ceilingz + sector[s].floorz) >> 1;
						if (doorantic == 2 || doorantic == 3)
							setdelayfunc(s, 0);
					}
					setanimation(s, botz, DOORSPEED, 0, FLOORZ);
				}
				if (j >= 0) {
					topz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
					if (gAnimationData[j].goal == topz) {
						gAnimationData[j].goal = (sector[s].ceilingz + sector[s].floorz) >> 1;
					} else {
						gAnimationData[j].goal = topz;
					}
				} else {
					if (sector[s].ceilingz == sector[s].floorz) {
						topz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
					} else {
						topz = (sector[s].ceilingz + sector[s].floorz) >> 1;
					}
					setanimation(s, topz, DOORSPEED, 0, CEILZ);
				}
				break;
			case 3: // vert
			case 4: // swing
				break;
			}
		}
	} // end of complexdoors

	if (datag == 3000) {
		for (k = 0; k < ironbarscnt; k++) {
			if (ironbarsector[k] == s) {
				ironbarsdone[k] = 1;

				switch (sprite[ironbarsanim[k]].picnum) {

				case SwingDoor:

				case SWINGDOOR2:

				case SWINGDOOR3:

				case TALLSWING:

				case TALLSWING2:
					SND_Sound(S_CREAKDOOR2);
					break;

				case SWINGGATE:
					SND_Sound(S_CREAKDOOR3);
					break;

				case SWINGHOLE:

				case SWINGGATE2:

				case ROPEDOOR:

				case GEARSSTART:

				case WOODGEARSSTART:
					SND_Sound(S_CREAKDOOR1);
					break;
				}
					
				int pic = sprite[ironbarsanim[k]].picnum;
				if(pic == SWINGGATE3 || pic == SWINGGATE4 || pic == SWINGGATE5 || pic == GEARS2START)
					SND_Sound(S_CREAKDOOR1);
			}
		}
	}

	if (datag == 4000) {
//			sector[s].lotag=0;
		for (k = 0; k < MAXSPRITES; k++) {
			if (sector[s].hitag == sprite[k].hitag && sprite[k].extra < 1) {
				newstatus(k, FLOCKSPAWN);
				if (batsnd == -1)
					batsnd = playsound(S_BATSLOOP, sprite[k].x, sprite[k].y, -1);
//					sector[s].lotag = sector[s].hitag = 0;
			}
		}
	}
	if (datag == 4001) {
//			sector[s].lotag=0;
		for (k = 0; k < MAXSPRITES; k++) {
			if (sector[s].hitag == sprite[k].hitag && sprite[k].picnum == GOBLIN) 
				newstatus(k, WAR);
		}
	}

	switch (datag) {
	case 61:
	case 62:
	case 63:
	case 64:
		if (keysok == 0)
			break;
		else {
			i = getanimationgoal(sector[s], 2);
			if (i >= 0) {
				goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
				if (gAnimationData[i].goal == goalz) {
					gAnimationData[i].goal = sector[s].floorz;
				} else {
					gAnimationData[i].goal = goalz;
				}
			} else {
				if (sector[s].ceilingz == sector[s].floorz) {
					goalz = sector[nextsectorneighborz(s, sector[s].floorz, -1, -1)].ceilingz;
				} else {
					goalz = sector[s].floorz;
				}
				setanimation(s, goalz, DOORSPEED, 0, CEILZ);
			}
			SND_Sound(S_DOOR2);
		}
	}
}

void animatetags(int nPlayer) {

	int endwall, good, j, k, oldang, startwall;
	short i, nexti;
	int dasector;

	PLAYER& plr = player[nPlayer];

	if (plr.sector != -1) {
		if (sector[plr.sector].lotag == 2) {
			for (i = 0; i < numsectors; i++)
				if (sector[i].hitag == sector[plr.sector].hitag)
					if (sector[i].lotag != 2)
						operatesector(plr, i);
			i = headspritestat[0];
			while (i != -1) {
				nexti = nextspritestat[i];
				if (sprite[i].hitag == sector[plr.sector].hitag)
					operatesprite(plr, i);
				i = nexti;
			}

			sector[plr.sector].lotag = 0;
			sector[plr.sector].hitag = 0;
		}
		if ((sector[plr.sector].lotag == 1) && (plr.sector != plr.oldsector)) {
			for (i = 0; i < numsectors; i++)
				if (sector[i].hitag == sector[plr.sector].hitag)
					if (sector[i].lotag != 2)
						operatesector(plr, i);
			i = headspritestat[0];
			while (i != -1) {
				nexti = nextspritestat[i];
				if (sprite[i].hitag == sector[plr.sector].hitag)
					operatesprite(plr, i);
				i = nexti;
			}
		}
	}

	for (i = 0; i < dragsectorcnt; i++) {

		dasector = dragsectorlist[i];

		startwall = sector[dasector].wallptr;
		endwall = startwall + sector[dasector].wallnum - 1;

		if (wall[startwall].x + dragxdir[i] < dragx1[i])
			dragxdir[i] = 16;
		if (wall[startwall].y + dragydir[i] < dragy1[i])
			dragydir[i] = 16;
		if (wall[startwall].x + dragxdir[i] > dragx2[i])
			dragxdir[i] = -16;
		if (wall[startwall].y + dragydir[i] > dragy2[i])
			dragydir[i] = -16;

		for (j = startwall; j <= endwall; j++)
			dragpoint((short) j, wall[j].x + dragxdir[i], wall[j].y + dragydir[i]);
		j = sector[dasector].floorz;
			
		game.pInt.setceilinterpolate(dasector, sector[dasector]);
		sector[dasector].floorz = dragfloorz[i] + (sintable[(lockclock << 4) & 2047] >> 3);
	
		if (plr.sector == dasector) {

			viewBackupPlayerLoc(nPlayer);

			plr.x += dragxdir[i];
			plr.y += dragydir[i];
			plr.z += (sector[dasector].floorz - j);

			// Update sprite representation of player
				
			game.pInt.setsprinterpolate(plr.spritenum, sprite[plr.spritenum]);
			setsprite(plr.spritenum, plr.x, plr.y, plr.z + (plr.height));
			sprite[plr.spritenum].ang = (short) plr.ang;
		}
	}

	for (i = 0; i < swingcnt; i++) {
		if (swingdoor[i].anginc != 0) {
			oldang = swingdoor[i].ang;
			for (j = 0; j < ((TICSPERFRAME) << 2); j++) {
				swingdoor[i].ang = ((swingdoor[i].ang + 2048 + swingdoor[i].anginc) & 2047);
				if (swingdoor[i].ang == swingdoor[i].angclosed) {
					swingdoor[i].anginc = 0;
				}
				if (swingdoor[i].ang == swingdoor[i].angopen) {
					swingdoor[i].anginc = 0;
				}
			}
			for (k = 1; k <= 3; k++) {
				Point out = rotatepoint(swingdoor[i].x[0], swingdoor[i].y[0], swingdoor[i].x[k],
						swingdoor[i].y[k], (short) swingdoor[i].ang);

				dragpoint((short)swingdoor[i].wall[k], out.getX(), out.getY());
			}
			if (swingdoor[i].anginc != 0) {
				if (plr.sector == swingdoor[i].sector) {
					good = 1;
					for (k = 1; k <= 3; k++) {
						if (clipinsidebox(plr.x, plr.y, (short) swingdoor[i].wall[k], 512) != 0) {
							good = 0;
							break;
						}
					}
					if (good == 0) {
						swingdoor[i].ang = oldang;
						for (k = 1; k <= 3; k++) {
							Point out = rotatepoint(swingdoor[i].x[0], swingdoor[i].y[0], swingdoor[i].x[k],
									swingdoor[i].y[k], (short) swingdoor[i].ang);

							dragpoint((short)swingdoor[i].wall[k], out.getX(), out.getY());
						}
						swingdoor[i].anginc = -swingdoor[i].anginc;
						break;
					}
				}
			}
		}
	}
}

void dodelayitems(int tics) {
	int cnt = delaycnt;
	for (int i = 0; i < cnt; i++) {
		if (!delayitem[i].func) {
			int j = delaycnt - 1;
			delayitem[i].memmove(delayitem[j]);
			delaycnt = j;
		}
		if (delayitem[i].timer > 0) {
			if ((delayitem[i].timer -= tics) <= 0) {
				delayitem[i].timer = 0;
				operatesector(player[pyrn], delayitem[i].item);
				delayitem[i].func = false;
			}
		}
	}
}

void setdelayfunc(int item, int delay) {
	for (int i = 0; i < delaycnt; i++) {
		if (delayitem[i].item == item && delayitem[i].func) {
			if (delay == 0)
				delayitem[i].func = false;
			delayitem[i].timer = delay;
			return;
		}
	}
	if (delay > 0) {
		delayitem[delaycnt].func = true;
		delayitem[delaycnt].item = item;
		delayitem[delaycnt].timer = delay;
		delaycnt++;
	}
}


END_WH_NS
