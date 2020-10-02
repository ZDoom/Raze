#include "ns.h"
#include "wh.h"

BEGIN_WH_NS



short skypanlist[64], skypancnt;
short lavadrylandsector[32];
short lavadrylandcnt;
short bobbingsectorlist[16], bobbingsectorcnt;

int justwarpedfx = 0;
int lastbat = -1;

short revolveclip[16];
short revolvesector[4], revolveang[4], revolvecnt;
int revolvex[4][32], revolvey[4][32];
int revolvepivotx[4], revolvepivoty[4];

static int revolvesyncstat;
static short revolvesyncang, revolvesyncrotang;
static int revolvesyncx, revolvesyncy;

int warpx, warpy, warpz, warpang;
short warpsect;

int scarytime = -1;
int scarysize = 0;

void initlava() // XXX
{

}

void movelava() {
};

void initwater() // XXX
{

}

void movewater() {
};

void skypanfx() {
	for (int i = 0; i < skypancnt; i++) {
		sector[skypanlist[i]].ceilingxpanning = (short) -((lockclock >> 2) & 255);
	}
}

void panningfx() {
	for (int i = 0; i < floorpanningcnt; i++) {
		int whichdir = sector[floorpanninglist[i]].lotag - 80;

		switch (whichdir) {
		case 0:
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		case 1:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		case 2:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			break;
		case 3:
			sector[floorpanninglist[i]].floorxpanning = (short) -((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 4:
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 5:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) -((lockclock >> 2) & 255);
			break;
		case 6:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			break;
		case 7:
			sector[floorpanninglist[i]].floorxpanning = (short) ((lockclock >> 2) & 255);
			sector[floorpanninglist[i]].floorypanning = (short) ((lockclock >> 2) & 255);
			break;
		default:
			sector[floorpanninglist[i]].floorxpanning = 0;
			sector[floorpanninglist[i]].floorypanning = 0;
			break;
		}
	}

	for (int i = 0; i < xpanningsectorcnt; i++) {
		int dasector = xpanningsectorlist[i];
		int startwall = sector[dasector].wallptr;
		int endwall = startwall + sector[dasector].wallnum - 1;
		for (int s = startwall; s <= endwall; s++)
			wall[s].xpanning = (short) ((lockclock >> 2) & 255);
	}

	for (int i = 0; i < ypanningwallcnt; i++)
		wall[ypanningwalllist[i]].ypanning = (short) ~(lockclock & 255);
}

void revolvefx() {

	short startwall, endwall;

	int dax, day;
	PLAYER& plr = player[pyrn];

	for (int i = 0; i < revolvecnt; i++) {

		startwall = sector[revolvesector[i]].wallptr;
		endwall = (short) (startwall + sector[revolvesector[i]].wallnum - 1);

		revolveang[i] = (short) ((revolveang[i] + 2048 - ((TICSPERFRAME) << 1)) & 2047);
		for (short k = startwall; k <= endwall; k++) {
			Point out = rotatepoint(revolvepivotx[i], revolvepivoty[i], revolvex[i][k - startwall],
					revolvey[i][k - startwall], revolveang[i]);
			dax = out.getX();
			day = out.getY();
			dragpoint(k, dax, day);
		}

		if (plr.sector == revolvesector[i]) {
			revolvesyncang = (short) plr.ang;
			revolvesyncrotang = 0;
			revolvesyncx = plr.x;
			revolvesyncy = plr.y;
			revolvesyncrotang = (short) ((revolvesyncrotang + 2048 - ((TICSPERFRAME) << 1)) & 2047);
			Point out = rotatepoint(revolvepivotx[i], revolvepivoty[i], revolvesyncx, revolvesyncy,
					revolvesyncrotang);
			viewBackupPlayerLoc(pyrn);
			plr.x = out.getX();
			plr.y = out.getY();
			plr.ang = ((revolvesyncang + revolvesyncrotang) & 2047);
		}
	}
}

void bobbingsector() {
	for (int i = 0; i < bobbingsectorcnt; i++) {
		short dasector = bobbingsectorlist[i];
		sector[dasector].floorz += (sintable[(lockclock << 4) & 2047] >> 6);
	}
}

void teleporter() {

	short dasector;
	short startwall, endwall;
	int i, j;
	int s;
	short daang;

	auto &plr = player[pyrn];

	for (i = 0; i < warpsectorcnt; i++) {
		dasector = warpsectorlist[i];
		j = ((lockclock & 127) >> 2);
		if (j >= 16)
			j = 31 - j;
		{
			sector[dasector].ceilingshade = (byte) j;
			sector[dasector].floorshade = (byte) j;
			startwall = sector[dasector].wallptr;
			endwall = (short) (startwall + sector[dasector].wallnum - 1);
			for (s = startwall; s <= endwall; s++)
				wall[s].shade = (byte) j;
		}
	}
	if (plr.sector == -1)
		return;

	if (sector[plr.sector].lotag == 10) {
		if (plr.sector != plr.oldsector) {
			daang = (short) plr.ang;
			warpfxsprite(plr.spritenum);
			warp(plr.x, plr.y, plr.z, daang, plr.sector);
			viewBackupPlayerLoc(pyrn);
			plr.x = warpx;
			plr.y = warpy;
			plr.z = warpz;
			daang = (short) warpang;
			plr.sector = (short) warpsect;
			warpfxsprite(plr.spritenum);
			plr.ang = (int) daang;
			justwarpedfx = 48;
			playsound_loc(S_WARP, plr.x, plr.y);
			setsprite(plr.spritenum, plr.x, plr.y, plr.z + (32 << 8));
		}
	}

	if (sector[plr.sector].lotag == 4002) {
		if (plr.sector != plr.oldsector) {
			if (plr.treasure[TPENTAGRAM] == 1) {
				plr.treasure[TPENTAGRAM] = 0;
				if (mUserFlag == UserFlag.UserMap) {
					game.changeScreen(gMenuScreen);
					return;
				}
				switch (sector[plr.sector].hitag) {
				case 1: // NEXTLEVEL
					justteleported = true;
						
					if(isWh2()) {
						gStatisticsScreen.show(plr, new Runnable() {
							@Override
							public void run() {
								mapon++;
								playsound_loc(S_CHAINDOOR1, plr.x, plr.y);
								playertorch = 0;
								playsound_loc(S_WARP, plr.x, plr.y);
								loadnewlevel(mapon);
							}
						});
						break;
					}
						
					mapon++;
					playsound_loc(S_CHAINDOOR1, plr.x, plr.y);
					playertorch = 0;
					playsound_loc(S_WARP, plr.x, plr.y);
					loadnewlevel(mapon);
					break;
				case 2: // ENDOFDEMO
					playsound_loc(S_THUNDER1, plr.x, plr.y);
					justteleported = true;
					game.changeScreen(gVictoryScreen);
					break;
				}
			} else {
				// player need pentagram to teleport
				showmessage("ITEM NEEDED", 360);
			}
		}
	}
}

void warp(int x, int y, int z, int daang, short dasector) {
	warpx = x;
	warpy = y;
	warpz = z;
	warpang = daang;
	warpsect = dasector;

	for (int i = 0; i < warpsectorcnt; i++) {
		if (sector[warpsectorlist[i]].hitag == sector[warpsect].hitag && warpsectorlist[i] != warpsect) {
			warpsect = warpsectorlist[i];
			break;
		}
	}

	// find center of sector
	int startwall = sector[warpsect].wallptr;
	int endwall = (short) (startwall + sector[warpsect].wallnum - 1);
	int dax = 0, day = 0, i = 0;
	for (int s = startwall; s <= endwall; s++) {
		dax += wall[s].x;
		day += wall[s].y;
		if (wall[s].nextsector >= 0) {
			i = s;
		}
	}
	warpx = dax / (endwall - startwall + 1);
	warpy = day / (endwall - startwall + 1);
	warpz = sector[warpsect].floorz - (32 << 8);

	warpsect = updatesector(warpx, warpy, warpsect);
	dax = ((wall[i].x + wall[wall[i].point2].x) >> 1);
	day = ((wall[i].y + wall[wall[i].point2].y) >> 1);
	warpang = getangle(dax - warpx, day - warpy);
}

void warpsprite(short spritenum) {
	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	if (monsterwarptime > 0)
		return;
	short dasectnum = sprite[spritenum].sectnum;
	warpfxsprite(spritenum);
	warp(sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z, sprite[spritenum].ang, dasectnum);
	sprite[spritenum].x = warpx;
	sprite[spritenum].y = warpy;
	sprite[spritenum].z = warpz;
	sprite[spritenum].ang = (short) warpang;
	dasectnum = (short) warpsect;

	warpfxsprite(spritenum);
	setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);

	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	monsterwarptime = 120;
}

void ironbars() {
	for (int i = 0; i < ironbarscnt; i++) {
		if (ironbarsdone[i] == 1) {
			short spritenum = ironbarsanim[i];
			switch (sprite[ironbarsanim[i]].hitag) {
			case 1:
				game.pInt.setsprinterpolate(spritenum, sprite[spritenum]);
				sprite[ironbarsanim[i]].ang += TICSPERFRAME << 1;
				if (sprite[ironbarsanim[i]].ang > 2047)
					sprite[ironbarsanim[i]].ang -= 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					sprite[ironbarsanim[i]].hitag = 2;
					ironbarsdone[i] = 0;
				}
				break;
			case 2:
				game.pInt.setsprinterpolate(spritenum, sprite[spritenum]);
				sprite[ironbarsanim[i]].ang -= TICSPERFRAME << 1;
				if (sprite[ironbarsanim[i]].ang < 0)
					sprite[ironbarsanim[i]].ang += 2047;
				ironbarsgoal[i] += TICSPERFRAME << 1;
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				if (ironbarsgoal[i] > 512) {
					ironbarsgoal[i] = 0;
					sprite[ironbarsanim[i]].hitag = 1;
					ironbarsdone[i] = 0;
				}
				break;
			case 3:
				game.pInt.setsprinterpolate(spritenum, sprite[spritenum]);
				sprite[ironbarsanim[i]].z -= TICSPERFRAME << 4;
				if (sprite[ironbarsanim[i]].z < ironbarsgoal[i]) {
					sprite[ironbarsanim[i]].z = ironbarsgoal[i];
					sprite[ironbarsanim[i]].hitag = 4;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = sprite[ironbarsanim[i]].z + 6000;
				}
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				break;
			case 4:
				game.pInt.setsprinterpolate(spritenum, sprite[spritenum]);
				sprite[ironbarsanim[i]].z += TICSPERFRAME << 4;
				if (sprite[ironbarsanim[i]].z > ironbarsgoal[i]) {
					sprite[ironbarsanim[i]].z = ironbarsgoal[i];
					sprite[ironbarsanim[i]].hitag = 3;
					ironbarsdone[i] = 0;
					ironbarsgoal[i] = sprite[ironbarsanim[i]].z - 6000;
				}
				setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
				break;
			}
		}
	}
}

void sectorsounds() {
	if (!SoundEnabled())
		return;

	PLAYER& plr = player[pyrn];

	int sec = sector[plr.sector].extra & 0xFFFF;
	if (sec != 0) {
		if ((sec & 32768) != 0) { // loop on/off sector
			if ((sec & 1) != 0) { // turn loop on if lsb is 1
				int index = (sec & ~0x8001) >> 1;
				if (index < ambsoundarray.length && ambsoundarray[index].hsound == -1)
					ambsoundarray[index].hsound = playsound(ambsoundarray[index].soundnum, 0, 0, -1);
			} else { // turn loop off if lsb is 0 and its playing
				int index = (sec & ~0x8000) >> 1;
				if (index < ambsoundarray.length && ambsoundarray[index].hsound != -1) {
					stopsound(ambsoundarray[index].hsound);
					ambsoundarray[index].hsound = -1;
				}
			}
		} else {
			if (plr.z <= sector[plr.sector].floorz - (8 << 8))
				playsound_loc(sec, plr.x, plr.y);
		}
	}
}


void scaryprocess() {
	if (krand() % 32768 > 32500 && krand() % 32768 > 32500 && scarytime < 0) {
		scarytime = 180;
		scarysize = 30;
		SND_Sound(S_SCARYDUDE);
	}

	if (scarytime >= 0) {
		scarytime -= TICSPERFRAME << 1;
		scarysize += TICSPERFRAME << 1;
	}
}

void dofx() {
	lavadryland();
	scaryprocess();
	if (revolvecnt > 0)
		revolvefx();
	panningfx();
	teleporter();
	bobbingsector();
	if (ironbarscnt > 0)
		ironbars();

	if ((gotpic[ANILAVA >> 3] & (1 << (ANILAVA & 7))) > 0) {
		gotpic[ANILAVA >> 3] &= ~(1 << (ANILAVA & 7));
//			if (waloff[ANILAVA] != nullptr)
//			movelava(waloff[ANILAVA]); XXX
	}
	if ((gotpic[HEALTHWATER >> 3] & (1 << (HEALTHWATER & 7))) > 0) {
		gotpic[HEALTHWATER >> 3] &= ~(1 << (HEALTHWATER & 7));
//			if (waloff[HEALTHWATER] != nullptr) XXX
//				movewater(waloff[HEALTHWATER]);
	}
	thesplash();
	thunder();
	cracks();
	if (isWh2()) {
		PLAYER& plr = player[0];
		if (sector[plr.sector].lotag == 50 && sector[plr.sector].hitag > 0)
			weaponpowerup(plr);
	}

	GLRenderer gl = glrender();
	if (gl != nullptr) {
		if (player[pyrn].poisoned != 0) {
			int tilt = mulscale(sintable[(3 * totalclock) & 2047], 20, 16);
			if (tilt != 0)
				gl.setdrunk(tilt);
		} else
			gl.setdrunk(0);
	}
}

static int thunderflash;
static int thundertime;

void thunder() {
	int val;

	if (thunderflash == 0) {
		visibility = 1024;
		if ((gotpic[SKY >> 3] & (1 << (SKY & 7))) > 0) {
			gotpic[SKY >> 3] &= ~(1 << (SKY & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY2 >> 3] & (1 << (SKY2 & 7))) > 0) {
			gotpic[SKY2 >> 3] &= ~(1 << (SKY2 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY3 >> 3] & (1 << (SKY3 & 7))) > 0) {
			gotpic[SKY3 >> 3] &= ~(1 << (SKY3 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY4 >> 3] & (1 << (SKY4 & 7))) > 0) {
			gotpic[SKY4 >> 3] &= ~(1 << (SKY4 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY5 >> 3] & (1 << (SKY5 & 7))) > 0) {
			gotpic[SKY5 >> 3] &= ~(1 << (SKY5 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY6 >> 3] & (1 << (SKY6 & 7))) > 0) {
			gotpic[SKY6 >> 3] &= ~(1 << (SKY6 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY7 >> 3] & (1 << (SKY7 & 7))) > 0) {
			gotpic[SKY7 >> 3] &= ~(1 << (SKY7 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY8 >> 3] & (1 << (SKY8 & 7))) > 0) {
			gotpic[SKY8 >> 3] &= ~(1 << (SKY8 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY9 >> 3] & (1 << (SKY9 & 7))) > 0) {
			gotpic[SKY9 >> 3] &= ~(1 << (SKY9 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		} else if ((gotpic[SKY10 >> 3] & (1 << (SKY10 & 7))) > 0) {
			gotpic[SKY10 >> 3] &= ~(1 << (SKY10 & 7));
			if (krand() % 32768 > 32700) {
				thunderflash = 1;
				thundertime = 120;
			}
		}
	} else {
		thundertime -= TICSPERFRAME;
		if (thundertime < 0) {
			thunderflash = 0;
			SND_Sound(S_THUNDER1 + (krand() % 4));
			visibility = 1024;
		}
	}

	if (thunderflash == 1) {
		val = krand() % 4;
		switch (val) {
		case 0:
			visibility = 2048;
			break;
		case 1:
			visibility = 1024;
			break;
		case 2:
			visibility = 512;
			break;
		case 3:
			visibility = 256;
			break;
		default:
			visibility = 4096;
			break;
		}
	}
}

void thesplash() {
	PLAYER& plr = player[pyrn];

	if (plr.sector == -1)
		return;

	if (sector[plr.sector].floorpicnum == WATER || sector[plr.sector].floorpicnum == LAVA
			|| sector[plr.sector].floorpicnum == SLIME) {
		if (plr.onsomething == 0)
			return;

		if (plr.sector != plr.oldsector) {
			switch (sector[plr.sector].floorpicnum) {
			case WATER:
				makeasplash(SPLASHAROO, plr);
				break;
			case SLIME:
				if (isWh2()) {
					makeasplash(SLIMESPLASH, plr);
				} else makeasplash(SPLASHAROO, plr);
				break;
			case LAVA:
				makeasplash(LAVASPLASH, plr);
				break;
			}
		}
	}
}

void makeasplash(int picnum, PLAYER& plr) {
	int j = insertsprite(plr.sector, MASPLASH);
	sprite[j].x = plr.x;
	sprite[j].y = plr.y;
	sprite[j].z = sector[plr.sector].floorz + (tilesizy[picnum] << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = (short) picnum;
	sprite[j].shade = 0;
	sprite[j].pal = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 8;
	sprite[j].hitag = 0;

	switch (picnum) {
	case SPLASHAROO:
	case SLIMESPLASH:
		if(!isWh2() && picnum == SLIMESPLASH)
			break;
			
		playsound_loc(S_SPLASH1 + (krand() % 3), sprite[j].x, sprite[j].y);
		break;
	case LAVASPLASH:
		break;
	}

	movesprite((short) j, ((sintable[(sprite[j].ang + 512) & 2047]) * TICSPERFRAME) << 3,
			((sintable[sprite[j].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
}

void makemonstersplash(int picnum, int i) {
	if (sprite[i].picnum == FISH)
		return;

	int j = insertsprite(sprite[i].sectnum, MASPLASH);
	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sector[sprite[i].sectnum].floorz + (tilesizy[picnum] << 8);
	sprite[j].cstat = 0; // Hitscan does not hit other bullets
	sprite[j].picnum = (short) picnum;
	sprite[j].shade = 0;

	if (sector[sprite[i].sectnum].floorpal == 9)
		sprite[j].pal = 9;
	else
		sprite[j].pal = 0;
		
	if (sprite[i].picnum == RAT) {
		sprite[j].xrepeat = 40;
		sprite[j].yrepeat = 40;
	} else {
		sprite[j].xrepeat = 64;
		sprite[j].yrepeat = 64;
	}
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 8;
	sprite[j].hitag = 0;
	game.pInt.setsprinterpolate(j, sprite[j]);

	// JSA 5/3 start
	switch (picnum) {
	case SPLASHAROO:
	case SLIME:
		if ((krand() % 2) != 0) {
			if ((gotpic[WATER >> 3] & (1 << (WATER & 7))) > 0) {
				gotpic[WATER >> 3] &= ~(1 << (WATER & 7));
					if ((krand() % 2) != 0)
						playsound_loc(S_SPLASH1 + (krand() % 3), sprite[j].x, sprite[j].y);
			}
		}
		if ((krand() % 2) != 0) {
			if ((gotpic[SLIME >> 3] & (1 << (SLIME & 7))) > 0) {
				gotpic[SLIME >> 3] &= ~(1 << (SLIME & 7));
					if ((krand() % 2) != 0)
						playsound_loc(S_SPLASH1 + (krand() % 3), sprite[j].x, sprite[j].y);
			}
		}
		break;
	case LAVASPLASH:
		break;
	}
}

void bats(PLAYER& plr, int k) {
	short j = insertsprite(sprite[k].sectnum, FLOCK);
	sprite[j].x = sprite[k].x;
	sprite[j].y = sprite[k].y;
	sprite[j].z = sprite[k].z;
	sprite[j].cstat = 0;
	sprite[j].picnum = BAT;
	sprite[j].shade = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short) ((sprite[k].ang + (krand() & 128 - 256)) & 2047);
	sprite[j].owner = (short) k;
	sprite[j].clipdist = 16;
	sprite[j].lotag = 128;
	sprite[j].hitag = (short) k;
	sprite[j].extra = 0;

	newstatus(j, FLOCK);

	if (sprite[k].extra == 1)
		lastbat = j;
}

void cracks() {

	PLAYER& plr = player[0];
	if (plr.sector == -1)
		return;

	int datag = sector[plr.sector].lotag;

	if (floorpanningcnt < 64)
		if (datag >= 3500 && datag <= 3599) {
			sector[plr.sector].hitag = 0;
			int daz = sector[plr.sector].floorz + (1024 * (sector[plr.sector].lotag - 3500));
			if ((setanimation(plr.sector, daz, 32, 0, FLOORZ)) >= 0) {
				sector[plr.sector].floorpicnum = LAVA1;
				sector[plr.sector].floorshade = -25;
				SND_Sound(S_CRACKING);
			}
			sector[plr.sector].lotag = 80;
			floorpanninglist[floorpanningcnt++] = plr.sector;
		}

	if (datag >= 5100 && datag <= 5199) {
		sector[plr.sector].hitag = 0;
		sector[plr.sector].lotag = 0;
	}

	if (datag >= 5200 && datag <= 5299) {
		sector[plr.sector].hitag = 0;
		sector[plr.sector].lotag = 0;
	}

	if (datag == 3001) {
		sector[plr.sector].lotag = 0;
		for (short k = 0; k < MAXSPRITES; k++) {
			if (sector[plr.sector].hitag == sprite[k].hitag) {
				sprite[k].lotag = 36;
				sprite[k].zvel = (short) (krand() & 1024 + 512);
				newstatus(k, SHOVE);
			}
		}
	}
}

void lavadryland() {
	PLAYER& plr = player[pyrn];

	for (int k = 0; k < lavadrylandcnt; k++) {

		int s = lavadrylandsector[k];

		if (plr.sector == s && sector[s].lotag > 0) {

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
			sector[s].lotag = 0;
		}
	}

}

void warpfxsprite(int s) {
	PLAYER& plr = player[pyrn];

	int j = insertsprite(sprite[s].sectnum, WARPFX);

	sprite[j].x = sprite[s].x;
	sprite[j].y = sprite[s].y;
	sprite[j].z = sprite[s].z - (32 << 8);

	sprite[j].cstat = 0;

	sprite[j].picnum = ANNIHILATE;
	short daang;
	if (s == plr.spritenum) {
		daang = (short) plr.ang;
		sprite[j].ang = daang;
	} else {
		daang = sprite[s].ang;
		sprite[j].ang = daang;
	}

	sprite[j].xrepeat = 48;
	sprite[j].yrepeat = 48;
	sprite[j].clipdist = 16;

	sprite[j].extra = 0;
	sprite[j].shade = -31;
	sprite[j].xvel = (short) ((krand() & 256) - 128);
	sprite[j].yvel = (short) ((krand() & 256) - 128);
	sprite[j].zvel = (short) ((krand() & 256) - 128);
	sprite[j].owner = (short) s;
	sprite[j].lotag = 12;
	sprite[j].hitag = 0;
	sprite[j].pal = 0;

	int daz = (((sprite[j].zvel) * TICSPERFRAME) >> 3);

	movesprite((short) j, ((sintable[(daang + 512) & 2047]) * TICSPERFRAME) << 3,
			((sintable[daang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);
}

void FadeInit() {
#pragma message("Fix fades")
#if 0
	Console.Println("Initializing fade effects", 0);
	registerFade("RED", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				r = 3 * (intensive + 32);
				a = 2 * (intensive + 32);
			} else
				r = a = 0;
			if (r > 255)
				r = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(r, 0, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			int multiple = intensive / 2;
			if (multiple > 170)
				multiple = 170;
			gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gl.glColor4ub(r > 0 ? multiple : 0, 0, 0, 0);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();
		}
	});

	registerFade("WHITE", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				g = r = 10 * intensive;
				a = (intensive + 32);
			} else
				g = r = a = 0;

			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(r, g, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(r > 0 ? multiple : 0, g > 0 ? multiple : 0, 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});

	registerFade("GREEN", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				g = 4 * intensive;
				a = (intensive + 32);
			} else
				g = a = 0;

			if (g > 255)
				g = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(0, g, 0, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(0, g > 0 ? multiple : 0, 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});

	registerFade("BLUE", new FadeEffect(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA) {
		private int intensive;

		@Override
		public void update(int intensive) {
			this.intensive = intensive;
			if (intensive > 0) {
				b = 4 * intensive;
				a = (intensive + 32);
			} else
				b = a = 0;

			if (b > 255)
				b = 255;
			if (a > 255)
				a = 255;
		}

		@Override
		public void draw(GL10 gl) {
			gl.glBlendFunc(sfactor, dfactor);
			gl.glColor4ub(0, 0, b, a);
			gl.glBegin(GL_TRIANGLES);
			gl.glVertex2f(-2.5f, 1.f);
			gl.glVertex2f(2.5f, 1.f);
			gl.glVertex2f(.0f, -2.5f);
			gl.glEnd();

			if (intensive > 0) {
				int multiple = intensive;
				if (multiple > 255)
					multiple = 255;
				gl.glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl.glColor4ub(0, 0, b > 0 ? multiple : 0, 0);
				gl.glBegin(GL_TRIANGLES);
				gl.glVertex2f(-2.5f, 1.f);
				gl.glVertex2f(2.5f, 1.f);
				gl.glVertex2f(.0f, -2.5f);
				gl.glEnd();
			}
		}
	});
#endif
}

void resetEffects() {
	greencount = 0;
	bluecount = 0;
	redcount = 0;
	whitecount = 0;
	updateFade("RED", 0);
	updateFade("GREEN", 0);
	updateFade("BLUE", 0);
	updateFade("WHITE", 0);
}

void weaponpowerup(PLAYER& plr) {
	showmessage("Weapons enchanted", 360);
	for (int i = 0; i < 10; i++) {
		if (plr.weapon[i] != 0 && plr.weapon[i] != 3) {
			plr.preenchantedweapon[i] = plr.weapon[i];
			plr.preenchantedammo[i] = plr.ammo[i];
			plr.weapon[i] = 3;
			switch (difficulty) {
			case 0:
				plr.ammo[i] = 25;
				break;
			case 1:
				plr.ammo[i] = 20;
				break;
			case 2:
			case 3:
				plr.ammo[i] = 10;
				break;
			}
				
			if (sector[plr.sector].hitag > 0) {
				sector[plr.sector].hitag--;
				if (sector[plr.sector].hitag == 0) {
					short j = headspritesect[plr.sector];
					while (j != -1) {
						short nextj = nextspritesect[j];
						if (sprite[j].picnum == CONE) {
							deletesprite(j);
						} else if (sprite[j].picnum == SPARKBALL) {
							deletesprite(j);
						}
						j = nextj;
					}
				}
			}
		}
	}
}

void makesparks(short i, int type) {

	int j = -1;

	switch (type) {
	case 1:
		j = insertsprite(sprite[i].sectnum, SPARKS);
		break;
	case 2:
		j = insertsprite(sprite[i].sectnum, SPARKSUP);
		break;
	case 3:
		j = insertsprite(sprite[i].sectnum, SPARKSDN);
		break;
	}

	if (j == -1)
		return;

	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sprite[i].z;
	sprite[j].cstat = 0;
	sprite[j].picnum = SPARKBALL;
	sprite[j].shade = 0;
	sprite[j].xrepeat = 24;
	sprite[j].yrepeat = 24;
	sprite[j].ang = (short) ((krand() % 2047) & 2047);
	sprite[j].owner = 0;
	sprite[j].clipdist = 16;
	sprite[j].lotag = (short) (krand() % 100);
	sprite[j].hitag = 0;
	sprite[j].extra = 0;

	sprite[j].pal = 0;
}

void shards(int i, int type) {
	short j = insertsprite(sprite[i].sectnum, SHARDOFGLASS);

	sprite[j].x = sprite[i].x + (((krand() % 512) - 256) << 2);
	sprite[j].y = sprite[i].y + (((krand() % 512) - 256) << 2);
	sprite[j].z = sprite[i].z - (getPlayerHeight() << 8) + (((krand() % 48) - 16) << 7);
	sprite[j].zvel = (short) (krand() % 256);
	sprite[j].cstat = 0;
	sprite[j].picnum = (short) (SHARD + (krand() % 3));
	sprite[j].shade = 0;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
	sprite[j].ang = (short) (sprite[i].ang + ((krand() % 512) - 256) & 2047);
	sprite[j].owner = (short) i;
	sprite[j].clipdist = 16;
	sprite[j].lotag = (short) (120 + (krand() % 100));
	sprite[j].hitag = 0;
	sprite[j].extra = (short) type;
	sprite[j].pal = 0;
}


END_WH_NS
