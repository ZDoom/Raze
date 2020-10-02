#include "ns.h"
#include "wh.h"
#include "automap.h"

BEGIN_WH_NS



void animateobjs(PLAYER& plr) {

	boolean hitdamage = false;
	short osectnum = 0, hitobject;
	int dax, day, daz = 0, j, k;
	short movestat = 0;

	short i, nextsprite;
	short startwall, endwall;

	if (plr.sector < 0 || plr.sector >= numsectors)
		return;

	if (isWh2()) {
		for (i = headspritestat[SHARDOFGLASS]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			spr.lotag -= TICSPERFRAME;
			switch (spr.extra) {
			case 1:
				spr.zvel += TICSPERFRAME << 3;
				movestat = (short) movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 3,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 2:
				spr.zvel += TICSPERFRAME << 5;
				movestat = (short) movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 1,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 1, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 3:
				spr.zvel -= TICSPERFRAME << 5;
				movestat = (short) movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 2,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 2, spr.zvel, 4 << 8, 4 << 8, 0);
				if (spr.lotag < 0) {
					spr.lotag = 30;
					spr.extra = 2;
				}
				break;
			}
			if (spr.lotag < 0) {
				deletesprite(i);
			}
		}

		for (i = headspritestat[SPARKSUP]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];

			osectnum = spr.sectnum;

			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;
			if (spr.lotag < 0) {
				daz = spr.zvel -= TICSPERFRAME << 4;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 3,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					setsprite((short) i, spr.x, spr.y, spr.z);

				}
			}
		}

		for (i = headspritestat[SPARKSDN]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];

			osectnum = spr.sectnum;

			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;

			if (spr.lotag < 0) {

				daz = spr.zvel += TICSPERFRAME << 4;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 3,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					setsprite((short) i, spr.x, spr.y, spr.z);

				}
			}
		}

		for (i = headspritestat[SPARKS]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];

			osectnum = spr.sectnum;
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;

			if (spr.lotag < 0) {

				daz = 0;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, ((sintable[(spr.ang + 512) & 2047]) * TICSPERFRAME) << 3,
						((sintable[spr.ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					setsprite((short) i, spr.x, spr.y, spr.z);

				}
			}
		}

		for (i = headspritestat[STONETOFLESH]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < 0) {
				switch (spr.picnum) {
				case STONEGONZOCHM:
					spr.picnum = GONZOGHM;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					sprite[i].xrepeat = 24;
					sprite[i].yrepeat = 24;
					sprite[i].clipdist = 32;
					changespritestat(i, FACE);
					sprite[i].hitag = (short) adjusthp(100);
					sprite[i].lotag = 100;
					sprite[i].cstat |= 0x101;
					sprite[i].extra = 0;
					break;
				case STONEGONZOBSH:
					spr.picnum = GONZOGSH;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					sprite[i].xrepeat = 24;
					sprite[i].yrepeat = 24;
					sprite[i].clipdist = 32;
					changespritestat(i, FACE);
					sprite[i].hitag = (short) adjusthp(100);
					sprite[i].lotag = 100;
					sprite[i].cstat |= 0x101;
					sprite[i].extra = 0;
					break;
				case STONEGONZOGSH:
					spr.picnum = GONZOGSH;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					sprite[i].xrepeat = 24;
					sprite[i].yrepeat = 24;
					sprite[i].clipdist = 32;
					changespritestat(i, FACE);
					sprite[i].hitag = (short) adjusthp(100);
					sprite[i].lotag = 100;
					sprite[i].cstat |= 0x101;
					sprite[i].extra = 0;
					break;
				case STONEGRONDOVAL:
					spr.picnum = GRONHAL;
					enemy[GRONTYPE].info.set(spr);
					spr.detail = GRONTYPE;
					sprite[i].xrepeat = 30;
					sprite[i].yrepeat = 30;
					sprite[i].clipdist = 64;
					changespritestat(i, FACE);
					sprite[i].hitag = (short) adjusthp(300);
					sprite[i].lotag = 100;
					sprite[i].cstat |= 0x101;
					sprite[i].extra = 4;
					break;
				case STONEGONZOBSW2:
				case STONEGONZOBSW:
					spr.picnum = NEWGUY;
					enemy[NEWGUYTYPE].info.set(spr);
					spr.detail = NEWGUYTYPE;
					sprite[i].xrepeat = 26;
					sprite[i].yrepeat = 26;
					sprite[i].clipdist = 48;
					changespritestat(i, FACE);
					sprite[i].hitag = (short) adjusthp(100);
					sprite[i].lotag = 100;
					sprite[i].cstat |= 0x101;
					sprite[i].extra = 30;
					break;
				}
			}
		}

		for (i = headspritestat[SHADE]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			spr.lotag -= TICSPERFRAME;

			if (spr.lotag < 0) {
				spr.picnum = GONZOBSHDEAD;
				spr.shade = 31;
				spr.cstat = 0x303;
				spr.pal = 0;
				spr.extra = 12;
				newstatus((short) i, EVILSPIRIT);
			}
		}

		for (i = headspritestat[EVILSPIRIT]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			if (spr.picnum >= (GONZOBSHDEAD - 8)) {
				if (--spr.extra <= 0) {
					spr.picnum--;
					spr.extra = 12;
				}
			} else {
				j = insertsprite(spr.sectnum, FACE);
				enemy[GONZOTYPE].info.set(sprite[j]);
				sprite[j].x = spr.x;
				sprite[j].y = spr.y;
				sprite[j].z = spr.z;
				sprite[j].cstat = 0x303;
				sprite[j].picnum = GONZOGSH;
				sprite[j].shade = 31;
				sprite[j].pal = 0;
				sprite[j].xrepeat = spr.xrepeat;
				sprite[j].yrepeat = spr.yrepeat;
				sprite[j].owner = 0;
				sprite[j].lotag = 40;
				sprite[j].hitag = 0;
				sprite[j].detail = GONZOTYPE;
				deletesprite((short) i);
			}
		}

		for (i = headspritestat[TORCHFRONT]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			playertorch = spr.lotag -= TICSPERFRAME;
			if (plr.selectedgun > 4) {
				playertorch = spr.lotag = -1;
			}
			setsprite((short) i, plr.x, plr.y, plr.z);
			osectnum = spr.sectnum;
			j = (torchpattern[lockclock % 38]);
			sector[osectnum].ceilingshade = (byte) ((sector[osectnum].ceilingshade + j) >> 1);
			sector[osectnum].floorshade = (byte) ((sector[osectnum].floorshade + j) >> 1);
			startwall = sector[osectnum].wallptr;
			endwall = (short) (startwall + sector[osectnum].wallnum - 1);
			for (k = startwall; k <= endwall; k++) {
				wall[k].shade = (byte) ((wall[k].shade + j) >> 1);
			}
			movestat = (short) movesprite((short) i,
					((sintable[((short) plr.ang + 512) & 2047]) << TICSPERFRAME) << 8,
					((sintable[(short) plr.ang & 2047]) << TICSPERFRAME) << 8, 0, 4 << 8, 4 << 8, 0);

			spr.cstat |= 0x8000;
			show2dsprite.Set(i);

			if (osectnum != spr.sectnum) {
				sector[osectnum].ceilingshade = (byte) j;

				sector[osectnum].floorshade = (byte) j;
				startwall = sector[osectnum].wallptr;
				endwall = (short) (startwall + sector[osectnum].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = (byte) j;
				}
			}
			if (spr.lotag < 0) {
				deletesprite((short) i);
				// set back to normall
				sector[plr.sector].ceilingshade = ceilingshadearray[plr.sector];
				sector[plr.sector].floorshade = floorshadearray[plr.sector];
				startwall = sector[plr.sector].wallptr;
				endwall = (short) (startwall + sector[plr.sector].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = wallshadearray[k];
				}
				sector[plr.oldsector].ceilingshade = ceilingshadearray[plr.oldsector];
				sector[plr.oldsector].floorshade = floorshadearray[plr.oldsector];
				startwall = sector[plr.oldsector].wallptr;
				endwall = (short) (startwall + sector[plr.oldsector].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = wallshadearray[k];
				}
				sector[osectnum].ceilingshade = ceilingshadearray[osectnum];
				sector[osectnum].floorshade = floorshadearray[osectnum];
				startwall = sector[osectnum].wallptr;
				endwall = (short) (startwall + sector[osectnum].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = wallshadearray[k];
				}
				osectnum = plr.oldsector;
				sector[osectnum].ceilingshade = ceilingshadearray[osectnum];
				sector[osectnum].floorshade = floorshadearray[osectnum];
				startwall = sector[osectnum].wallptr;
				endwall = (short) (startwall + sector[osectnum].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = wallshadearray[k];
				}
			}
		}
	}

	for (i = headspritestat[PULLTHECHAIN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].picnum++;
			sprite[i].lotag = 24;
			if (sprite[i].picnum == PULLCHAIN3 || sprite[i].picnum == SKULLPULLCHAIN3) {
				sprite[i].lotag = 0;
				changespritestat(i, (short) 0);
			}
		}
	}

	for (i = headspritestat[ANIMLEVERDN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].picnum++;
			sprite[i].lotag = 24;
			if (sprite[i].picnum == LEVERDOWN) {
				sprite[i].lotag = 60;
				changespritestat(i, (short) 0);
			}
		}
	}

	for (i = headspritestat[ANIMLEVERUP]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].picnum--;
			sprite[i].lotag = 24;
			if (sprite[i].picnum == LEVERUP) {
				sprite[i].lotag = 1;
				changespritestat(i, (short) 0);
			}
		}
	}

	for (i = headspritestat[WARPFX]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].lotag = 12;
			sprite[i].picnum++;
			if (sprite[i].picnum == ANNIHILATE + 5) {
				deletesprite((short) i);
			}
		}
	}

	// FLOCKSPAWN
	for (i = headspritestat[FLOCKSPAWN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].extra--;
			sprite[i].lotag = (short) (krand() & 48 + 24);
			bats(plr, i);
			if (sprite[i].extra == 0)
				changespritestat(i, (short) 0);
		}
	}

	// FLOCK
	for (i = headspritestat[FLOCK]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;
		switch (sprite[i].extra) {
		case 0: // going out of the cave
			if (sprite[i].lotag < 0) {
				sprite[i].extra = 1;
				sprite[i].lotag = 512;
			} else {
				movestat = (short) movesprite((short) i,
						(((int) sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
						(((int) sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
				if (movestat != 0)
					sprite[i].ang = (short) (krand() & 2047);
			}
			break;
		case 1: // flying in circles
			if (sprite[i].lotag < 0) {
				sprite[i].extra = 2;
				sprite[i].lotag = 512;
				sprite[i].ang = (short) (((getangle(sprite[sprite[i].hitag].x - sprite[i].x,
						sprite[sprite[i].hitag].y - sprite[i].y) & 2047) - 1024) & 2047);
			} else {
				sprite[i].z -= TICSPERFRAME << 4;
				sprite[i].ang = (short) ((sprite[i].ang + (TICSPERFRAME << 2)) & 2047);
				movestat = (short) movesprite((short) i,
						(((int) sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
						(((int) sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
				if (movestat != 0)
					sprite[i].ang = (short) (krand() & 2047);
			}
			break;
		case 2: // fly to roof and get deleted
			if (sprite[i].lotag < 0) {
				if (i == lastbat && batsnd != -1) {
					stopsound(batsnd);
					batsnd = -1;
				}
				deletesprite((short) i);
				continue;
			} else {
				movestat = (short) movesprite((short) i,
						(((int) sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
						(((int) sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
				if ((movestat & 0xc000) == 16384) {// Hits a ceiling / floor
					if (i == lastbat && batsnd != -1) {
						stopsound(batsnd);
						batsnd = -1;
					}
					deletesprite((short) i);
					continue;
				}
				if (movestat != 0)
					sprite[i].ang = (short) (krand() & 2047);
			}
			break;
		}
	}

	// TORCHLIGHT
	for (i = headspritestat[TORCHLIGHT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		osectnum = sprite[i].sectnum;
		j = (torchpattern[lockclock % 38]);
		sector[osectnum].ceilingshade = (byte) j;
		sector[osectnum].floorshade = (byte) j;
		startwall = sector[osectnum].wallptr;
		endwall = (short) (startwall + sector[osectnum].wallnum - 1);
		for (k = startwall; k <= endwall; k++)
			wall[k].shade = (byte) j;
	}

	// GLOWLIGHT
	for (i = headspritestat[GLOWLIGHT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		osectnum = sprite[i].sectnum;
		j = (torchpattern[lockclock % 38]);
		sector[osectnum].floorshade = (byte) j;
		startwall = sector[osectnum].wallptr;
		endwall = (short) (startwall + sector[osectnum].wallnum - 1);
		for (k = startwall; k <= endwall; k++)
			wall[k].shade = (byte) j;
//			startredflash(j);
	}

	// BOB
	for (i = headspritestat[BOB]; i >= 0; i = nextspritestat[i]) {
		nextsprite = nextspritestat[i];
		sprite[i].z += (sintable[(lockclock << 4) & 2047] >> 6);
	}

	// LIFT UP
	for (i = headspritestat[LIFTUP]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		switch (sprite[i].lotag) {
		case 1821:
			
			sprite[i].z -= (TICSPERFRAME << 6);
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z <= sector[sprite[i].sectnum].ceilingz + 32768) {
				stopsound(cartsnd);
				cartsnd = -1;
				playsound_loc(S_CLUNK, sprite[i].x, sprite[i].y);
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1820;
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + 32768;
			}
			break;
		case 1811:
			
			sprite[i].z -= (TICSPERFRAME << 6);
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z <= sector[sprite[i].sectnum].ceilingz + 65536) {
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1810;
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + 65536;
			}
			break;
		case 1801:
			
			sprite[i].z -= (TICSPERFRAME << 6);
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z <= sector[sprite[i].sectnum].ceilingz + 65536) {
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1800;
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + 65536;
			}
			break;
		}
	}

	// LIFT DN
	for (i = headspritestat[LIFTDN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		int ironbarmove = 0;

		switch (sprite[i].lotag) {
		case 1820:
			
			ironbarmove = TICSPERFRAME << 6;
			sprite[i].z += ironbarmove;
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z >= (sector[sprite[i].sectnum].floorz - 32768)) {
				stopsound(cartsnd);
				cartsnd = -1;
				playsound_loc(S_CLUNK, sprite[i].x, sprite[i].y);
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1821;
				sprite[i].z = sector[sprite[i].sectnum].floorz - 32768;
			}
			break;
		case 1810:
			ironbarmove = TICSPERFRAME << 6;
			sprite[i].z += ironbarmove;
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z >= sector[sprite[i].sectnum].floorz) {
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1811;
				sprite[i].z = sector[sprite[i].sectnum].floorz;
			}
			break;
		case 1800:
			
			ironbarmove = TICSPERFRAME << 6;
			sprite[i].z += ironbarmove;
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
			if (sprite[i].z >= sector[sprite[i].sectnum].floorz) {
				changespritestat(i, (short) 0);
				sprite[i].lotag = 1801;
				sprite[i].z = sector[sprite[i].sectnum].floorz;
			}
			break;
		}
	}

	// MASPLASH
	for (i = headspritestat[MASPLASH]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		
		sprite[i].lotag -= TICSPERFRAME;
		sprite[i].z = sector[sprite[i].sectnum].floorz + (tileHeight(sprite[i].picnum) << 8);
		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);

		if (sprite[i].lotag <= 0) {
			if ((sprite[i].picnum >= SPLASHAROO && sprite[i].picnum < LASTSPLASHAROO)
					|| (sprite[i].picnum >= LAVASPLASH && sprite[i].picnum < LASTLAVASPLASH)
					|| (sprite[i].picnum >= SLIMESPLASH && sprite[i].picnum < LASTSLIMESPLASH)) {
				sprite[i].picnum++;
				sprite[i].lotag = 8;
			} else {
				deletesprite((short) i);
			}
		}
	}

	// SHATTER
	for (i = headspritestat[SHATTER]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;

		if (sprite[i].lotag < 0) {
			sprite[i].picnum++;
			sprite[i].lotag = 12;
		}
		switch (sprite[i].picnum) {
		case FSHATTERBARREL + 2:
			changespritestat(i, (short) 0);
			break;
		}
	}

	// FIRE
	for (i = headspritestat[FIRE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;

		if (sprite[i].z < sector[sprite[i].sectnum].floorz)
			sprite[i].z += (int) TICSPERFRAME << 8;
		if (sprite[i].z > sector[sprite[i].sectnum].floorz)
			sprite[i].z = sector[sprite[i].sectnum].floorz;

		if (sprite[i].lotag < 0) {
			switch (sprite[i].picnum) {
			case LFIRE:
				sprite[i].picnum = SFIRE;
				sprite[i].lotag = 2047;
				break;
			case SFIRE:
				deletesprite(i);
				continue;
			}
		}

		if (checkdist(i, plr.x, plr.y, plr.z)) {
			addhealth(plr, -1);
			flashflag = 1;
			startredflash(10);
		}
	}

	// FALL
	for (i = headspritestat[FALL]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		getzrange(sprite[i].x, sprite[i].y, sprite[i].z - 1, sprite[i].sectnum, (sprite[i].clipdist) << 2,
				CLIPMASK0);
		if (sprite[i].z < zr_florz)
			daz = sprite[i].zvel += (TICSPERFRAME << 9);

		hitobject = (short) movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);

		if (sprite[i].picnum == FBARRELFALL || sprite[i].picnum >= BOULDER && sprite[i].picnum <= BOULDER + 3
				&& (checkdist(i, plr.x, plr.y, plr.z))) {
			addhealth(plr, -50);
			startredflash(50);
		}

		if ((hitobject & 0xc0000) == 16384) {
			if (sector[sprite[i].sectnum].floorpicnum == WATER) {
				makemonstersplash(SPLASHAROO, i);
			}
			switch (sprite[i].picnum) {
			case FBARRELFALL:
				newstatus(i, SHATTER);
				sprite[i].lotag = 12;
				break;
			default:
				if (sprite[i].picnum == TORCH) {
					for (k = 0; k < 16; k++)
						makeafire(i, 0);
					deletesprite(i);
					break;
				}
				changespritestat(i, (short) 0);
				break;
			}
			sprite[i].hitag = 0;
		}
	}

	// SHOVE
	for (i = headspritestat[SHOVE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		getzrange(sprite[i].x, sprite[i].y, sprite[i].z - 1, sprite[i].sectnum, (sprite[i].clipdist) << 2,
				CLIPMASK0);
		if (sprite[i].z < zr_florz)
			daz = sprite[i].zvel += (TICSPERFRAME << 5);

		hitobject = (short) movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);

		if (sprite[i].z >= sector[sprite[i].sectnum].floorz) {
			if (sector[sprite[i].sectnum].floorpicnum == WATER
					|| sector[sprite[i].sectnum].floorpicnum == FLOORMIRROR) {
				makemonstersplash(SPLASHAROO, i);
			}
			newstatus(i, BROKENVASE);
			continue;
		}

		if ((hitobject & 0xc000) == 16384) {
			newstatus(i, BROKENVASE);
			continue;
		}

		if ((hitobject & 0xc000) == 49152) { // Bullet hit a sprite

			if (sprite[i].owner != hitobject) {
				hitdamage = damageactor(plr, hitobject, i);
				if (hitdamage) {
					newstatus(i, BROKENVASE);
					continue;
				}
			}

		}
	}

	// PUSH
	for (i = headspritestat[PUSH]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;

		osectnum = sprite[i].sectnum;

		getzrange(sprite[i].x, sprite[i].y, sprite[i].z - 1, sprite[i].sectnum, (sprite[i].clipdist) << 2,
				CLIPMASK0);
		if (sprite[i].z < zr_florz)
			daz = sprite[i].zvel += (TICSPERFRAME << 1);

		// clip type was 1
		hitobject = (short) movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);

		if (sprite[i].lotag < 0 || (hitobject & 0xc000) == 32768) {
			sprite[i].lotag = 0;
			changespritestat(i, (short) 0);
			if (sprite[i].z < sector[sprite[i].sectnum].floorz) {
				sprite[i].zvel += 256L;
				changespritestat(i, FALL);
			}
		}
	}

	// DORMANT
	for (i = headspritestat[DORMANT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		if (isWh2()) {
			SPRITE& spr = sprite[i];
			osectnum = spr.sectnum;
			j = (torchpattern[lockclock % 38]);
			sector[osectnum].ceilingshade = (byte) j;
			sector[osectnum].floorshade = (byte) j;
			startwall = sector[osectnum].wallptr;
			endwall = (short) (startwall + sector[osectnum].wallnum - 1);
			for (k = startwall; k <= endwall; k++)
				wall[k].shade = (byte) j;
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < 0) {
				newstatus((short) i, ACTIVE);
			}
		} else {
			sprite[i].lotag -= TICSPERFRAME;
			sprite[i].xrepeat = sprite[i].yrepeat = 2;
			if (sprite[i].lotag < 0) {
				newstatus(i, ACTIVE);
			}
		}
	}

	// ACTIVE
	for (i = headspritestat[ACTIVE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		if (isWh2()) {
			SPRITE& spr = sprite[i];
			spr.lotag -= TICSPERFRAME;
			osectnum = spr.sectnum;
			j = (torchpattern[lockclock % 38]);
			sector[osectnum].ceilingshade = (byte) j;
			sector[osectnum].floorshade = (byte) j;
			startwall = sector[osectnum].wallptr;
			endwall = (short) (startwall + sector[osectnum].wallnum - 1);
			for (k = startwall; k <= endwall; k++)
				wall[k].shade = (byte) j;
			if (spr.lotag < 0) {
				sector[osectnum].ceilingshade = ceilingshadearray[osectnum];
				sector[osectnum].floorshade = floorshadearray[osectnum];
				startwall = sector[osectnum].wallptr;
				endwall = (short) (startwall + sector[osectnum].wallnum - 1);
				for (k = startwall; k <= endwall; k++) {
					wall[k].shade = wallshadearray[k];
				}
				newstatus((short) i, DORMANT);
			}
		} else {
			sprite[i].lotag -= TICSPERFRAME;
			sprite[i].xrepeat = 48;
			sprite[i].yrepeat = 32;
			if (sprite[i].lotag < 0) {
				newstatus(i, DORMANT);
			}
		}
	}

	aiProcess();

	// New missile code
	for (i = headspritestat[MISSILE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;

		switch (sprite[i].picnum) {
		case WH1THROWPIKE:
		case WH2THROWPIKE:
		case FATSPANK:
		case MONSTERBALL:
		case FIREBALL:
		case PLASMA:
			if ((sprite[i].picnum == WH1THROWPIKE && isWh2()) || (sprite[i].picnum == WH2THROWPIKE && !isWh2()))
				break;
				
			if(!isValidSector(sprite[i].sectnum)) {
				deletesprite((short) i);
				continue;
			}

			if (sprite[i].picnum == MONSTERBALL && krand() % 100 > 90) {
				if (sprite[i].lotag < 200)
					trailingsmoke(i, false);
			}
			sprite[i].z += sprite[i].zvel;
			if (sprite[i].z < sector[sprite[i].sectnum].ceilingz + (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (4 << 8);
				sprite[i].zvel = (short) -(sprite[i].zvel >> 1);
			}
			if (sprite[i].z > sector[sprite[i].sectnum].floorz - (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].floorz - (4 << 8);
				if (sector[sprite[i].sectnum].floorpicnum == WATER || sector[sprite[i].sectnum].floorpicnum == SLIME
						|| sector[sprite[i].sectnum].floorpicnum == FLOORMIRROR)
					if (sprite[i].picnum == FISH)
						sprite[i].z = sector[sprite[i].sectnum].floorz;
					else {
						if (krand() % 100 > 60)
							makemonstersplash(SPLASHAROO, i);
					}
					
//					if (sprite[i].picnum != THROWPIKE) { //XXX
					deletesprite((short) i);
					continue;
//					}
			}
			dax = sprite[i].xvel;
			day = sprite[i].yvel;
			daz = ((((int) sprite[i].zvel) * TICSPERFRAME) >> 3);
			break;
		case BULLET:
			dax = sprite[i].xvel;
			day = sprite[i].yvel;
			daz = sprite[i].zvel;
			break;
		} // switch

		osectnum = sprite[i].sectnum;

		if (sprite[i].picnum == THROWPIKE) {
			sprite[i].cstat = 0;
			hitobject = (short) movesprite((short) i,
					((sintable[(sprite[i].extra + 512) & 2047]) * TICSPERFRAME) << 6,
					((sintable[sprite[i].extra & 2047]) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 1);
			sprite[i].cstat = 21;
		} else {
			hitobject = (short) movesprite((short) i,
					((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 6, // was 3
					((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 6, // was 3
					daz, 4 << 8, 4 << 8, 1);
		}

		if (hitobject != 0 && sprite[i].picnum == MONSTERBALL)
			if (sprite[i].owner == sprite[plr.spritenum].owner) {
				explosion2(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
			} else {
				explosion(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
			}

		if ((hitobject & 0xc000) == 16384) { // Hits a ceiling / floor
			if (sprite[i].picnum == THROWPIKE) {
				sprite[i].picnum++;
				sprite[i].detail = WALLPIKETYPE;
				changespritestat(i, (short) 0);
					
				continue;
			}
			deletesprite((short) i);
			continue;
		} else if ((hitobject & 0xc000) == 32768) { // hit a wall

			if (sprite[i].picnum == MONSTERBALL) {
				if (sprite[i].owner == sprite[plr.spritenum].owner)
					explosion2(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
				else
					explosion(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
			}
			if (sprite[i].picnum == THROWPIKE) {
				sprite[i].picnum++;
				sprite[i].detail = WALLPIKETYPE;
				changespritestat(i, (short) 0);
					
				continue;
			}
			deletesprite((short) i);
			continue;
		} else if (sprite[i].lotag < 0 && sprite[i].picnum == PLASMA)
			hitobject = 1;

		if ((hitobject & 0xc000) == 49152) { // Bullet hit a sprite
			if (sprite[i].picnum == MONSTERBALL) {
				if (sprite[i].owner == sprite[plr.spritenum].owner)
					explosion2(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
				else
					explosion(i, sprite[i].x, sprite[i].y, sprite[i].z, i);
			}

			if (sprite[i].owner != hitobject)
				hitdamage = damageactor(plr, hitobject, i);
			if (hitdamage) {
				deletesprite((short) i);
				continue;
			}
		}

		if (hitobject != 0 || sprite[i].lotag < 0) {
			int pic = sprite[i].picnum;
			switch (pic) {
			case PLASMA:
			case FATSPANK:
			case MONSTERBALL:
			case FIREBALL:
			case BULLET:
			case WH1THROWPIKE:
			case WH2THROWPIKE:
				if ((sprite[i].picnum == WH1THROWPIKE && isWh2())
						|| (sprite[i].picnum == WH2THROWPIKE && !isWh2()))
					break;

				deletesprite((short) i);
				continue;
			}
		}
	}

	for (i = headspritestat[JAVLIN]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;
		if (isBlades(sprite[i].picnum)) {
			sprite[i].z -= sprite[i].zvel;
			if (sprite[i].z < sector[sprite[i].sectnum].ceilingz + (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (4 << 8);
				sprite[i].zvel = (short) -(sprite[i].zvel >> 1);
			}
			if (sprite[i].z > sector[sprite[i].sectnum].floorz - (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].floorz - (4 << 8);
				if (sector[sprite[i].sectnum].floorpicnum == WATER || sector[sprite[i].sectnum].floorpicnum == SLIME
						|| sector[sprite[i].sectnum].floorpicnum == FLOORMIRROR)
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				deletesprite((short) i);
				continue;
			}
			dax = sprite[i].xvel;
			day = sprite[i].yvel;
			if (isWh2())
				daz = sprite[i].zvel;
			else
				daz = (((sprite[i].zvel) * TICSPERFRAME) >> 3);
		}

		osectnum = sprite[i].sectnum;

		sprite[i].cstat = 0;

		hitobject = (short) movesprite(i, ((sintable[(sprite[i].extra + 512) & 2047]) * TICSPERFRAME) << 6,
				((sintable[sprite[i].extra & 2047]) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 0);

		if (sprite[i].picnum == WALLARROW || sprite[i].picnum == THROWHALBERD)
			sprite[i].cstat = 0x11;
		else if (sprite[i].picnum == DART)
			sprite[i].cstat = 0x10;
		else
			sprite[i].cstat = 0x15;

		if ((hitobject & 0xc000) == 16384) { // Hits a ceiling / floor
			// EG Bugfix 17 Aug 2014: Since the game thinks that a javlin hitting the
			// player's pike axe is a
			// floor/ceiling hit rather than a sprite hit, we'll need to check if the JAVLIN
			// is
			// actually in the floor/ceiling before going inactive.
			if (sprite[i].z <= sector[sprite[i].sectnum].ceilingz
					&& sprite[i].z >= sector[sprite[i].sectnum].floorz) {
				if (sprite[i].picnum == THROWPIKE) {
					sprite[i].picnum++;
					sprite[i].detail = WALLPIKETYPE;
				}

				changespritestat(i, INACTIVE); // EG Note: RAF.H gives this a nice name, so use it
			}
			continue;
		} else if ((hitobject & 0xc000) == 32768) { // hit a wall

			if (sprite[i].picnum == THROWPIKE) {
				sprite[i].picnum++;
				sprite[i].detail = WALLPIKETYPE;
			}

			changespritestat(i, INACTIVE);
			continue;
		}

		if ((hitobject - 49152) >= 0 || (hitobject & 0xc000) == 49152) { // Bullet hit a sprite
			j = (hitobject & 4095); // j is the spritenum that the bullet (spritenum i) hit

			hitdamage = damageactor(plr, hitobject, i);
			if (hitdamage)
				continue;

//				if (sprite[i].owner != hitobject) {
//					hitdamage = damageactor(plr, hitobject, i);
//					continue;
//				}

			if (!hitdamage)
				if (isBlades(sprite[j].picnum)) {
					deletesprite((short) i);
					continue;
				}
		}

		if (hitobject != 0) {
			deletesprite((short) i);
		}
	}

	// CHUNK O WALL

	for (i = headspritestat[CHUNKOWALL]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		dax = sprite[i].xvel >> 3;
		day = sprite[i].yvel >> 3;
		daz = sprite[i].zvel -= TICSPERFRAME << 2;
		movestat = (short) movesprite(i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 1);
		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
		if (sprite[i].extra == 0) {
			if (sprite[i].lotag < 0) {
				sprite[i].lotag = 8;
				sprite[i].picnum++;
				if (sprite[i].picnum == SMOKEFX + 3) {
					deletesprite((short) i);
					continue;
				}
			}
		} else {
			if (sprite[i].lotag < 0) {
				deletesprite((short) i);
			}
		}
	}

	// CHUNK O MEAT
	for (i = headspritestat[CHUNKOMEAT]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;

		sprite[i].z += sprite[i].zvel;

		daz = sprite[i].zvel += TICSPERFRAME << 4;

		int xvel = ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3;
		int yvel = ((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3;

		if (sprite[i].picnum == BONECHUNK1 && sprite[i].picnum == BONECHUNKEND) {
			daz >>= 1;
			xvel >>= 1;
			yvel >>= 1;
		}

		movestat = (short) movesprite((short) i, xvel, yvel, daz, 4 << 8, 4 << 8, 1);

		if ((movestat & 0xc000) == 16384) {
			if (sector[sprite[i].sectnum].floorpicnum == WATER || sector[sprite[i].sectnum].floorpicnum == SLIME
					|| sector[sprite[i].sectnum].floorpicnum == FLOORMIRROR) {
				if (sprite[i].picnum == FISH)
					sprite[i].z = sector[sprite[i].sectnum].floorz;
				else {
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				}
				sprite[i].lotag = -1;
			} else {
				/* EG: Add check for parallax sky */
				if (sprite[i].picnum >= BONECHUNK1 && sprite[i].picnum <= BONECHUNKEND
						|| (daz >= zr_ceilz && (sector[sprite[i].sectnum].ceilingstat & 1) != 0)) {
					deletesprite(i);
				} else {
					sprite[i].cstat |= 0x0020;
					sprite[i].lotag = 1200;
					newstatus(i, BLOOD);
				}
			}
		} else if ((movestat & 0xc000) == 32768) {
			if (sprite[i].picnum >= BONECHUNK1 && sprite[i].picnum <= BONECHUNKEND) {
				deletesprite((short) i);
			} else {
				sprite[i].lotag = 600;
				newstatus(i, DRIP);
			}
		}
		if (sprite[i].lotag < 0) {
			deletesprite(i);
		}
	}

	for (i = headspritestat[BLOOD]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			if (sprite[i].z < sector[sprite[i].sectnum].floorz) {
				sprite[i].lotag = 600;
				sprite[i].zvel = 0;
				newstatus(i, DRIP);
			} else {
				deletesprite(i);
			}
		}
	}

	for (i = headspritestat[DEVILFIRE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		if (plr.invisibletime < 0) {
			sprite[i].lotag -= TICSPERFRAME;
			if (sprite[i].lotag < 0) {
				sprite[i].lotag = (short) (krand() & 120 + 360);
				if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
						sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum)) {
					// JSA_NEW
					playsound_loc(S_FIREBALL, sprite[i].x, sprite[i].y);
					castspell(plr, i);
				}
			}
		}
	}

	for (i = headspritestat[DRIP]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		sprite[i].z += sprite[i].zvel;
		dax = 0;
		day = 0;
		daz = sprite[i].zvel += TICSPERFRAME << 1;
		daz = (((sprite[i].zvel) * TICSPERFRAME) << 1);
		movestat = (short) movesprite(i, dax, day, daz, 4 << 8, 4 << 8, 1);

		if ((movestat & 0xc000) == 16384) {
			sprite[i].lotag = 1200;
			newstatus(i, BLOOD);
		}
		if (sprite[i].lotag < 0) {
			deletesprite((short) i);
		}
	}

	for (i = headspritestat[SMOKE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;

//			
//			sprite[i].z -= (TICSPERFRAME << 6);

		if (sprite[i].xrepeat > 1)
			sprite[i].xrepeat = sprite[i].yrepeat -= TICSPERFRAME;

//			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
		if (sprite[i].lotag < 0) {
			deletesprite((short) i);
		}
	}

	if (!isWh2()) {
		for (i = headspritestat[EXPLO]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			sprite[i].lotag -= TICSPERFRAME;
			sprite[i].x += ((sprite[i].xvel * TICSPERFRAME) >> 5);
			sprite[i].y += ((sprite[i].yvel * TICSPERFRAME) >> 5);
			sprite[i].z -= ((sprite[i].zvel * TICSPERFRAME) >> 6);

			sprite[i].zvel += (TICSPERFRAME << 4);

			if (sprite[i].z < sector[sprite[i].sectnum].ceilingz + (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (4 << 8);
				sprite[i].zvel = (short) -(sprite[i].zvel >> 1);
			}
			if (sprite[i].z > sector[sprite[i].sectnum].floorz - (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].floorz - (4 << 8);
				sprite[i].zvel = (short) -(sprite[i].zvel >> 1);
			}

			sprite[i].xrepeat += TICSPERFRAME;
			sprite[i].yrepeat += TICSPERFRAME;

			sprite[i].lotag -= TICSPERFRAME;

			if (krand() % 100 > 90) {
				j = insertsprite(sprite[i].sectnum, SMOKE);

				sprite[j].x = sprite[i].x;
				sprite[j].y = sprite[i].y;
				sprite[j].z = sprite[i].z;
				sprite[j].cstat = 0x03;
				sprite[j].cstat &= ~3;
				sprite[j].picnum = SMOKEFX;
				sprite[j].shade = 0;
				sprite[j].pal = 0;
				sprite[j].xrepeat = sprite[i].xrepeat;
				sprite[j].yrepeat = sprite[i].yrepeat;

				sprite[j].owner = sprite[i].owner;
				sprite[j].lotag = 256;
				sprite[j].hitag = 0;
			}

			if (sprite[i].lotag < 0) {
				deletesprite(i);
			}
		}
	} else {
		for (i = headspritestat[EXPLO]; i >= 0; i = nextsprite) {
			nextsprite = nextspritestat[i];

			SPRITE& spr = sprite[i];
			spr.lotag -= TICSPERFRAME;
			spr.picnum++;
			if (spr.lotag < 0) {
				spr.lotag = 12;
			}

			j = headspritesect[spr.sectnum];
			while (j != -1) {
				short nextj = nextspritesect[j];
				SPRITE tspr = sprite[j];
				int dx = abs(spr.x - tspr.x); // x distance to sprite
				int dy = abs(spr.y - tspr.y); // y distance to sprite
				int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
				int dh = tileHeight(tspr.picnum) >> 1; // height of sprite
				if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
					if (tspr.owner == 4096) {
						// strcpy(displaybuf,"hit player");
						// displaytime=120;
					} else {
						switch (tspr.detail) {
						case SKELETONTYPE:
						case KOBOLDTYPE:
						case IMPTYPE:
						case NEWGUYTYPE:
						case KURTTYPE:
						case GONZOTYPE:
						case GRONTYPE:
							if (tspr.hitag > 0) {
								tspr.hitag -= TICSPERFRAME << 4;
								if (tspr.hitag < 0) {
									newstatus((short) j, DIE);
								}
							}
							break;
						}
					}
				}
				j = nextj;
			}

			if (spr.picnum == EXPLOEND) {
				deletesprite((short) i);
			}
		}
	}

	for (i = headspritestat[BROKENVASE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0) {
			sprite[i].picnum++;
			sprite[i].lotag = 18;

			if (sprite[i].picnum == (SHATTERVASE + 6) || sprite[i].picnum == (SHATTERVASE2 + 6)
					|| sprite[i].picnum == (SHATTERVASE3 + 6))
				changespritestat(i, (short) 0);
			else {
				switch (sprite[i].picnum) {
				case FSHATTERBARREL + 2:
					randompotion(i);
					changespritestat(i, (short) 0);
					break;
				case STAINGLASS1 + 6:
				case STAINGLASS2 + 6:
				case STAINGLASS3 + 6:
				case STAINGLASS4 + 6:
				case STAINGLASS5 + 6:
				case STAINGLASS6 + 6:
				case STAINGLASS7 + 6:
				case STAINGLASS8 + 6:
				case STAINGLASS9 + 6:
					changespritestat(i, (short) 0);
					break;
				}
			}
		}
	}

	// Go through explosion sprites
	for (i = headspritestat[FX]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		sprite[i].lotag -= TICSPERFRAME;

		if (// sprite[i].picnum == PLASMA ||
		sprite[i].picnum == BULLET || sprite[i].picnum == EXPLOSION || sprite[i].picnum == FIREBALL
				|| sprite[i].picnum == MONSTERBALL || sprite[i].picnum == FATSPANK) {

			// sprite[i].z+=sprite[i].zvel;
			sprite[i].zvel += (TICSPERFRAME << 5);
			if (sprite[i].z < sector[sprite[i].sectnum].ceilingz + (4 << 8)) {
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (4 << 8);
				sprite[i].zvel = (short) -(sprite[i].zvel >> 1);
			}
			if (sprite[i].z > sector[sprite[i].sectnum].floorz - (4 << 8) && sprite[i].picnum != EXPLOSION) {
				sprite[i].z = sector[sprite[i].sectnum].floorz - (4 << 8);
				sprite[i].zvel = 0;
				sprite[i].lotag = 4;
			}
			dax = ((((int) sprite[i].xvel) * TICSPERFRAME) >> 3);
			day = ((((int) sprite[i].yvel) * TICSPERFRAME) >> 3);
			daz = (((int) sprite[i].zvel) * TICSPERFRAME);
			movestat = (short) movesprite((short) i, dax, day, daz, 4 << 8, 4 << 8, 1);
			setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
		}

		if (sprite[i].picnum == ICECUBE && sprite[i].z < sector[sprite[i].sectnum].floorz) {
			sprite[i].z += sprite[i].zvel;

			daz = sprite[i].zvel += TICSPERFRAME << 4;

			movestat = (short) movesprite((short) i, ((sintable[(sprite[i].ang + 512) & 2047]) * TICSPERFRAME) << 3,
					((sintable[sprite[i].ang & 2047]) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

		}

		if (sprite[i].lotag < 0 || movestat != 0)
			if (sprite[i].picnum == PLASMA || sprite[i].picnum == EXPLOSION || sprite[i].picnum == FIREBALL
					|| sprite[i].picnum == MONSTERBALL || sprite[i].picnum == FATSPANK
					|| sprite[i].picnum == ICECUBE) {
				deletesprite(i);
				continue;
			}

		if (sprite[i].z + (8 << 8) >= sector[sprite[i].sectnum].floorz && sprite[i].picnum == ICECUBE
				|| movestat != 0) {
			sprite[i].z = sector[sprite[i].sectnum].floorz;
			changespritestat(i, (short) 0);
			if (sector[sprite[i].sectnum].floorpicnum == WATER || sector[sprite[i].sectnum].floorpicnum == SLIME
					|| sector[sprite[i].sectnum].floorpicnum == FLOORMIRROR) {
				if (sprite[i].picnum == FISH)
					sprite[i].z = sector[sprite[i].sectnum].floorz;
				else {
					if (krand() % 100 > 60) {
						makemonstersplash(SPLASHAROO, i);
						deletesprite((short) i);
					}
				}
			} else {
				if (sprite[i].lotag < 0) {
					deletesprite((short) i);
				}
			}
		}
	}
}

boolean isBlades(int pic) {
	return pic == THROWPIKE || pic == WALLARROW || pic == DART || pic == HORIZSPIKEBLADE || pic == THROWHALBERD;
}

END_WH_NS
