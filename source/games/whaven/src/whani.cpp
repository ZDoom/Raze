#include "ns.h"
#include "wh.h"
#include "automap.h"

BEGIN_WH_NS



void animateobjs(PLAYER& plr) {

	boolean hitdamage = false;
	short osectnum = 0, hitobject;
	int dax, day, daz = 0, j, k;
	short movestat = 0;

	short startwall, endwall;

	if (plr.sector < 0 || plr.sector >= numsectors)
		return;

	if (isWh2()) {
		WHStatIterator it(SHARDOFGLASS);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			spr.lotag -= TICSPERFRAME;
			switch (spr.extra) {
			case 1:
				spr.zvel += TICSPERFRAME << 3;
				movestat = (short) movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 2:
				spr.zvel += TICSPERFRAME << 5;
				movestat = (short) movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 1,
						(bsin(spr.ang) * TICSPERFRAME) << 1, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 3:
				spr.zvel -= TICSPERFRAME << 5;
				movestat = (short) movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 2,
						(bsin(spr.ang) * TICSPERFRAME) << 2, spr.zvel, 4 << 8, 4 << 8, 0);
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

		it.Reset(SPARKSUP);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			osectnum = spr.sectnum;

			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;
			if (spr.lotag < 0) {
				daz = spr.zvel -= TICSPERFRAME << 4;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

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

		it.Reset(SPARKSDN);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			osectnum = spr.sectnum;

			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;

			if (spr.lotag < 0) {

				daz = spr.zvel += TICSPERFRAME << 4;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

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

		it.Reset(SPARKS);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			osectnum = spr.sectnum;
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < -100)
				spr.lotag = -100;

			if (spr.lotag < 0) {

				daz = 0;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);

				movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

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

		it.Reset(STONETOFLESH);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < 0) {
				switch (spr.picnum) {
				case STONEGONZOCHM:
					spr.picnum = GONZOGHM;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					spr.xrepeat = 24;
					spr.yrepeat = 24;
					spr.clipdist = 32;
					changespritestat(i, FACE);
					spr.hitag = (short) adjusthp(100);
					spr.lotag = 100;
					spr.cstat |= 0x101;
					spr.extra = 0;
					break;
				case STONEGONZOBSH:
					spr.picnum = GONZOGSH;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					spr.xrepeat = 24;
					spr.yrepeat = 24;
					spr.clipdist = 32;
					changespritestat(i, FACE);
					spr.hitag = (short) adjusthp(100);
					spr.lotag = 100;
					spr.cstat |= 0x101;
					spr.extra = 0;
					break;
				case STONEGONZOGSH:
					spr.picnum = GONZOGSH;
					enemy[GONZOTYPE].info.set(spr);
					spr.detail = GONZOTYPE;
					spr.xrepeat = 24;
					spr.yrepeat = 24;
					spr.clipdist = 32;
					changespritestat(i, FACE);
					spr.hitag = (short) adjusthp(100);
					spr.lotag = 100;
					spr.cstat |= 0x101;
					spr.extra = 0;
					break;
				case STONEGRONDOVAL:
					spr.picnum = GRONHAL;
					enemy[GRONTYPE].info.set(spr);
					spr.detail = GRONTYPE;
					spr.xrepeat = 30;
					spr.yrepeat = 30;
					spr.clipdist = 64;
					changespritestat(i, FACE);
					spr.hitag = (short) adjusthp(300);
					spr.lotag = 100;
					spr.cstat |= 0x101;
					spr.extra = 4;
					break;
				case STONEGONZOBSW2:
				case STONEGONZOBSW:
					spr.picnum = NEWGUY;
					enemy[NEWGUYTYPE].info.set(spr);
					spr.detail = NEWGUYTYPE;
					spr.xrepeat = 26;
					spr.yrepeat = 26;
					spr.clipdist = 48;
					changespritestat(i, FACE);
					spr.hitag = (short) adjusthp(100);
					spr.lotag = 100;
					spr.cstat |= 0x101;
					spr.extra = 30;
					break;
				}
			}
		}

		it.Reset(SHADE);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

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

		it.Reset(EVILSPIRIT);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			if (spr.picnum >= (GONZOBSHDEAD - 8)) {
				if (--spr.extra <= 0) {
					spr.picnum--;
					spr.extra = 12;
				}
			} else {
				j = insertsprite(spr.sectnum, FACE);
				auto& spawned = sprite[j];
				enemy[GONZOTYPE].info.set(spawned);
				spawned.x = spr.x;
				spawned.y = spr.y;
				spawned.z = spr.z;
				spawned.cstat = 0x303;
				spawned.picnum = GONZOGSH;
				spawned.shade = 31;
				spawned.pal = 0;
				spawned.xrepeat = spr.xrepeat;
				spawned.yrepeat = spr.yrepeat;
				spawned.owner = 0;
				spawned.lotag = 40;
				spawned.hitag = 0;
				spawned.detail = GONZOTYPE;
				deletesprite((short) i);
			}
		}

		it.Reset(TORCHFRONT);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			playertorch = spr.lotag -= TICSPERFRAME;
			if (plr.selectedgun > 4) {
				playertorch = spr.lotag = -1;
			}
			setsprite((short) i, plr.x, plr.y, plr.z);
			osectnum = spr.sectnum;
			j = (torchpattern[PlayClock % 38]);
			sector[osectnum].ceilingshade = (byte) ((sector[osectnum].ceilingshade + j) >> 1);
			sector[osectnum].floorshade = (byte) ((sector[osectnum].floorshade + j) >> 1);
			startwall = sector[osectnum].wallptr;
			endwall = (short) (startwall + sector[osectnum].wallnum - 1);
			for (k = startwall; k <= endwall; k++) {
				wall[k].shade = (byte) ((wall[k].shade + j) >> 1);
			}
			movestat = (short) movesprite((short) i,
					(plr.angle.ang.bcos() << TICSPERFRAME) << 8,
					(plr.angle.ang.bsin() << TICSPERFRAME) << 8, 0, 4 << 8, 4 << 8, 0);

			spr.cstat |= 0x8000;
			spr.cstat2 |= CSTAT2_SPRITE_MAPPED;

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

	WHStatIterator it(PULLTHECHAIN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum++;
			spr.lotag = 24;
			if (spr.picnum == PULLCHAIN3 || spr.picnum == SKULLPULLCHAIN3) {
				spr.lotag = 0;
				changespritestat(i, (short) 0);
			}
		}
	}

	it.Reset(ANIMLEVERDN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum++;
			spr.lotag = 24;
			if (spr.picnum == LEVERDOWN) {
				spr.lotag = 60;
				changespritestat(i, (short) 0);
			}
		}
	}

	it.Reset(ANIMLEVERUP);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum--;
			spr.lotag = 24;
			if (spr.picnum == LEVERUP) {
				spr.lotag = 1;
				changespritestat(i, (short) 0);
			}
		}
	}

	it.Reset(WARPFX);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.lotag = 12;
			spr.picnum++;
			if (spr.picnum == ANNIHILATE + 5) {
				deletesprite((short) i);
			}
		}
	}

	// FLOCKSPAWN
	it.Reset(FLOCKSPAWN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.extra--;
			spr.lotag = (short) (krand() & 48 + 24);
			bats(plr, i);
			if (spr.extra == 0)
				changespritestat(i, (short) 0);
		}
	}

	// FLOCK
	it.Reset(FLOCK);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		switch (spr.extra) {
		case 0: // going out of the cave
			if (spr.lotag < 0) {
				spr.extra = 1;
				spr.lotag = 512;
			} else {
				movestat = (short) movesprite((short) i,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, spr.x, spr.y, spr.z);
				if (movestat != 0)
					spr.ang = (short) (krand() & 2047);
			}
			break;
		case 1: // flying in circles
			if (spr.lotag < 0) {
				spr.extra = 2;
				spr.lotag = 512;
				spr.ang = (short) (((getangle(sprite[spr.hitag].x - spr.x,
						sprite[spr.hitag].y - spr.y) & 2047) - 1024) & 2047);
			} else {
				spr.z -= TICSPERFRAME << 4;
				spr.ang = (short) ((spr.ang + (TICSPERFRAME << 2)) & 2047);
				movestat = (short) movesprite((short) i,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, spr.x, spr.y, spr.z);
				if (movestat != 0)
					spr.ang = (short) (krand() & 2047);
			}
			break;
		case 2: // fly to roof and get deleted
			if (spr.lotag < 0) {
				if (i == lastbat) {
					soundEngine->StopSound(CHAN_BAT);
				}
				deletesprite((short) i);
				continue;
			} else {
				movestat = (short) movesprite((short) i,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				setsprite(i, spr.x, spr.y, spr.z);
				if ((movestat & 0xc000) == 16384) {// Hits a ceiling / floor
					if (i == lastbat) {
						soundEngine->StopSound(CHAN_BAT);
					}
					deletesprite((short) i);
					continue;
				}
				if (movestat != 0)
					spr.ang = (short) (krand() & 2047);
			}
			break;
		}
	}

	// TORCHLIGHT
	it.Reset(TORCHLIGHT);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		osectnum = spr.sectnum;
		j = (torchpattern[PlayClock % 38]);
		sector[osectnum].ceilingshade = (byte) j;
		sector[osectnum].floorshade = (byte) j;
		startwall = sector[osectnum].wallptr;
		endwall = (short) (startwall + sector[osectnum].wallnum - 1);
		for (k = startwall; k <= endwall; k++)
			wall[k].shade = (byte) j;
	}

	// GLOWLIGHT
	it.Reset(GLOWLIGHT);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		osectnum = spr.sectnum;
		j = (torchpattern[PlayClock % 38]);
		sector[osectnum].floorshade = (byte) j;
		startwall = sector[osectnum].wallptr;
		endwall = (short) (startwall + sector[osectnum].wallnum - 1);
		for (k = startwall; k <= endwall; k++)
			wall[k].shade = (byte) j;
//			startredflash(j);
	}

	// BOB
	it.Reset(BOB);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		spr.z += bsin(PlayClock << 4, -6);
	}

	// LIFT UP
	it.Reset(LIFTUP);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		switch (spr.lotag) {
		case 1821:
			
			spr.z -= (TICSPERFRAME << 6);
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z <= sector[spr.sectnum].ceilingz + 32768) {
				soundEngine->StopSound(CHAN_CART);
				spritesound(S_CLUNK, &spr);
				changespritestat(i, (short) 0);
				spr.lotag = 1820;
				spr.z = sector[spr.sectnum].ceilingz + 32768;
			}
			break;
		case 1811:
			
			spr.z -= (TICSPERFRAME << 6);
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z <= sector[spr.sectnum].ceilingz + 65536) {
				changespritestat(i, (short) 0);
				spr.lotag = 1810;
				spr.z = sector[spr.sectnum].ceilingz + 65536;
			}
			break;
		case 1801:
			
			spr.z -= (TICSPERFRAME << 6);
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z <= sector[spr.sectnum].ceilingz + 65536) {
				changespritestat(i, (short) 0);
				spr.lotag = 1800;
				spr.z = sector[spr.sectnum].ceilingz + 65536;
			}
			break;
		}
	}

	// LIFT DN
	it.Reset(LIFTDN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		int ironbarmove = 0;

		switch (spr.lotag) {
		case 1820:
			
			ironbarmove = TICSPERFRAME << 6;
			spr.z += ironbarmove;
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z >= (sector[spr.sectnum].floorz - 32768)) {
				soundEngine->StopSound(CHAN_CART);
				spritesound(S_CLUNK, &spr);
				changespritestat(i, (short) 0);
				spr.lotag = 1821;
				spr.z = sector[spr.sectnum].floorz - 32768;
			}
			break;
		case 1810:
			ironbarmove = TICSPERFRAME << 6;
			spr.z += ironbarmove;
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z >= sector[spr.sectnum].floorz) {
				changespritestat(i, (short) 0);
				spr.lotag = 1811;
				spr.z = sector[spr.sectnum].floorz;
			}
			break;
		case 1800:
			
			ironbarmove = TICSPERFRAME << 6;
			spr.z += ironbarmove;
			setsprite(i, spr.x, spr.y, spr.z);
			if (spr.z >= sector[spr.sectnum].floorz) {
				changespritestat(i, (short) 0);
				spr.lotag = 1801;
				spr.z = sector[spr.sectnum].floorz;
			}
			break;
		}
	}

	// MASPLASH
	it.Reset(MASPLASH);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		spr.z = sector[spr.sectnum].floorz + (tileHeight(spr.picnum) << 8);
		setsprite(i, spr.x, spr.y, spr.z);

		if (spr.lotag <= 0) {
			if ((spr.picnum >= SPLASHAROO && spr.picnum < LASTSPLASHAROO)
					|| (spr.picnum >= LAVASPLASH && spr.picnum < LASTLAVASPLASH)
					|| (spr.picnum >= SLIMESPLASH && spr.picnum < LASTSLIMESPLASH)) {
				spr.picnum++;
				spr.lotag = 8;
			} else {
				deletesprite((short) i);
			}
		}
	}

	// SHATTER
	it.Reset(SHATTER);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		if (spr.lotag < 0) {
			spr.picnum++;
			spr.lotag = 12;
		}
		switch (spr.picnum) {
		case FSHATTERBARREL + 2:
			changespritestat(i, (short) 0);
			break;
		}
	}

	// FIRE
	it.Reset(FIRE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		if (spr.z < sector[spr.sectnum].floorz)
			spr.z += (int) TICSPERFRAME << 8;
		if (spr.z > sector[spr.sectnum].floorz)
			spr.z = sector[spr.sectnum].floorz;

		if (spr.lotag < 0) {
			switch (spr.picnum) {
			case LFIRE:
				spr.picnum = SFIRE;
				spr.lotag = 2047;
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

	it.Reset(FALL);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2,
				CLIPMASK0);
		if (spr.z < zr_florz)
			daz = spr.zvel += (TICSPERFRAME << 9);

		hitobject = (short) movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, spr.x, spr.y, spr.z);

		if (spr.picnum == FBARRELFALL || spr.picnum >= BOULDER && spr.picnum <= BOULDER + 3
				&& (checkdist(i, plr.x, plr.y, plr.z))) {
			addhealth(plr, -50);
			startredflash(50);
		}

		if ((hitobject & 0xc0000) == 16384) {
			if (sector[spr.sectnum].floorpicnum == WATER) {
				makemonstersplash(SPLASHAROO, i);
			}
			switch (spr.picnum) {
			case FBARRELFALL:
				newstatus(i, SHATTER);
				spr.lotag = 12;
				break;
			default:
				if (spr.picnum == TORCH) {
					for (k = 0; k < 16; k++)
						makeafire(i, 0);
					deletesprite(i);
					break;
				}
				changespritestat(i, (short) 0);
				break;
			}
			spr.hitag = 0;
		}
	}

	// SHOVE
	it.Reset(SHOVE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2,
				CLIPMASK0);
		if (spr.z < zr_florz)
			daz = spr.zvel += (TICSPERFRAME << 5);

		hitobject = (short) movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, spr.x, spr.y, spr.z);

		if (spr.z >= sector[spr.sectnum].floorz) {
			if (sector[spr.sectnum].floorpicnum == WATER
					|| sector[spr.sectnum].floorpicnum == FLOORMIRROR) {
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

			if (spr.owner != hitobject) {
				hitdamage = damageactor(plr, hitobject, i);
				if (hitdamage) {
					newstatus(i, BROKENVASE);
					continue;
				}
			}

		}
	}

	// PUSH
	it.Reset(PUSH);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		osectnum = spr.sectnum;

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2,
				CLIPMASK0);
		if (spr.z < zr_florz)
			daz = spr.zvel += (TICSPERFRAME << 1);

		// clip type was 1
		hitobject = (short) movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		setsprite(i, spr.x, spr.y, spr.z);

		if (spr.lotag < 0 || (hitobject & 0xc000) == 32768) {
			spr.lotag = 0;
			changespritestat(i, (short) 0);
			if (spr.z < sector[spr.sectnum].floorz) {
				spr.zvel += 256L;
				changespritestat(i, FALL);
			}
		}
	}

	// DORMANT
	it.Reset(DORMANT);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		if (isWh2()) {
			osectnum = spr.sectnum;
			j = (torchpattern[PlayClock % 38]);
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
			spr.lotag -= TICSPERFRAME;
			spr.xrepeat = spr.yrepeat = 2;
			if (spr.lotag < 0) {
				newstatus(i, ACTIVE);
			}
		}
	}

	// ACTIVE
	it.Reset(ACTIVE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		if (isWh2()) {
			spr.lotag -= TICSPERFRAME;
			osectnum = spr.sectnum;
			j = (torchpattern[PlayClock % 38]);
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
			spr.lotag -= TICSPERFRAME;
			spr.xrepeat = 48;
			spr.yrepeat = 32;
			if (spr.lotag < 0) {
				newstatus(i, DORMANT);
			}
		}
	}

	aiProcess();

	// New missile code
	it.Reset(MISSILE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		switch (spr.picnum) {
		case WH1THROWPIKE:
		case WH2THROWPIKE:
		case FATSPANK:
		case MONSTERBALL:
		case FIREBALL:
		case PLASMA:
			if ((spr.picnum == WH1THROWPIKE && isWh2()) || (spr.picnum == WH2THROWPIKE && !isWh2()))
				break;
				
			if(!isValidSector(spr.sectnum)) {
				deletesprite((short) i);
				continue;
			}

			if (spr.picnum == MONSTERBALL && krand() % 100 > 90) {
				if (spr.lotag < 200)
					trailingsmoke(i, false);
			}
			spr.z += spr.zvel;
			if (spr.z < sector[spr.sectnum].ceilingz + (4 << 8)) {
				spr.z = sector[spr.sectnum].ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > sector[spr.sectnum].floorz - (4 << 8)) {
				spr.z = sector[spr.sectnum].floorz - (4 << 8);
				if (sector[spr.sectnum].floorpicnum == WATER || sector[spr.sectnum].floorpicnum == SLIME
						|| sector[spr.sectnum].floorpicnum == FLOORMIRROR)
					if (spr.picnum == FISH)
						spr.z = sector[spr.sectnum].floorz;
					else {
						if (krand() % 100 > 60)
							makemonstersplash(SPLASHAROO, i);
					}
					
//					if (spr.picnum != THROWPIKE) { //XXX
					deletesprite((short) i);
					continue;
//					}
			}
			dax = spr.xvel;
			day = spr.yvel;
			daz = ((((int) spr.zvel) * TICSPERFRAME) >> 3);
			break;
		case BULLET:
			dax = spr.xvel;
			day = spr.yvel;
			daz = spr.zvel;
			break;
		} // switch

		osectnum = spr.sectnum;

		if (spr.picnum == THROWPIKE) {
			spr.cstat = 0;
			hitobject = (short) movesprite((short) i,
					(bcos(spr.extra) * TICSPERFRAME) << 6,
					(bsin(spr.extra) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 1);
			spr.cstat = 21;
		} else {
			hitobject = (short) movesprite((short) i,
					(bcos(spr.ang) * TICSPERFRAME) << 6, // was 3
					(bsin(spr.ang) * TICSPERFRAME) << 6, // was 3
					daz, 4 << 8, 4 << 8, 1);
		}

		if (hitobject != 0 && spr.picnum == MONSTERBALL)
			if (spr.owner == sprite[plr.spritenum].owner) {
				explosion2(i, spr.x, spr.y, spr.z, i);
			} else {
				explosion(i, spr.x, spr.y, spr.z, i);
			}

		if ((hitobject & 0xc000) == 16384) { // Hits a ceiling / floor
			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
				changespritestat(i, (short) 0);
					
				continue;
			}
			deletesprite((short) i);
			continue;
		} else if ((hitobject & 0xc000) == 32768) { // hit a wall

			if (spr.picnum == MONSTERBALL) {
				if (spr.owner == sprite[plr.spritenum].owner)
					explosion2(i, spr.x, spr.y, spr.z, i);
				else
					explosion(i, spr.x, spr.y, spr.z, i);
			}
			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
				changespritestat(i, (short) 0);
					
				continue;
			}
			deletesprite((short) i);
			continue;
		} else if (spr.lotag < 0 && spr.picnum == PLASMA)
			hitobject = 1;

		if ((hitobject & 0xc000) == 49152) { // Bullet hit a sprite
			if (spr.picnum == MONSTERBALL) {
				if (spr.owner == sprite[plr.spritenum].owner)
					explosion2(i, spr.x, spr.y, spr.z, i);
				else
					explosion(i, spr.x, spr.y, spr.z, i);
			}

			if (spr.owner != hitobject)
				hitdamage = damageactor(plr, hitobject, i);
			if (hitdamage) {
				deletesprite((short) i);
				continue;
			}
		}

		if (hitobject != 0 || spr.lotag < 0) {
			int pic = spr.picnum;
			switch (pic) {
			case PLASMA:
			case FATSPANK:
			case MONSTERBALL:
			case FIREBALL:
			case BULLET:
			case WH1THROWPIKE:
			case WH2THROWPIKE:
				if ((spr.picnum == WH1THROWPIKE && isWh2())
						|| (spr.picnum == WH2THROWPIKE && !isWh2()))
					break;

				deletesprite((short) i);
				continue;
			}
		}
	}

	it.Reset(JAVLIN);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (isBlades(spr.picnum)) {
			spr.z -= spr.zvel;
			if (spr.z < sector[spr.sectnum].ceilingz + (4 << 8)) {
				spr.z = sector[spr.sectnum].ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > sector[spr.sectnum].floorz - (4 << 8)) {
				spr.z = sector[spr.sectnum].floorz - (4 << 8);
				if (sector[spr.sectnum].floorpicnum == WATER || sector[spr.sectnum].floorpicnum == SLIME
						|| sector[spr.sectnum].floorpicnum == FLOORMIRROR)
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				deletesprite((short) i);
				continue;
			}
			dax = spr.xvel;
			day = spr.yvel;
			if (isWh2())
				daz = spr.zvel;
			else
				daz = (((spr.zvel) * TICSPERFRAME) >> 3);
		}

		osectnum = spr.sectnum;

		spr.cstat = 0;

		hitobject = (short) movesprite(i, (bcos(spr.extra) * TICSPERFRAME) << 6,
				(bsin(spr.extra) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 0);

		if (spr.picnum == WALLARROW || spr.picnum == THROWHALBERD)
			spr.cstat = 0x11;
		else if (spr.picnum == DART)
			spr.cstat = 0x10;
		else
			spr.cstat = 0x15;

		if ((hitobject & 0xc000) == 16384) { // Hits a ceiling / floor
			// EG Bugfix 17 Aug 2014: Since the game thinks that a javlin hitting the
			// player's pike axe is a
			// floor/ceiling hit rather than a sprite hit, we'll need to check if the JAVLIN
			// is
			// actually in the floor/ceiling before going inactive.
			if (spr.z <= sector[spr.sectnum].ceilingz
					&& spr.z >= sector[spr.sectnum].floorz) {
				if (spr.picnum == THROWPIKE) {
					spr.picnum++;
					spr.detail = WALLPIKETYPE;
				}

				changespritestat(i, INACTIVE); // EG Note: RAF.H gives this a nice name, so use it
			}
			continue;
		} else if ((hitobject & 0xc000) == 32768) { // hit a wall

			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
			}

			changespritestat(i, INACTIVE);
			continue;
		}

		if ((hitobject - 49152) >= 0 || (hitobject & 0xc000) == 49152) { // Bullet hit a sprite
			j = (hitobject & 4095); // j is the spritenum that the bullet (spritenum i) hit

			hitdamage = damageactor(plr, hitobject, i);
			if (hitdamage)
				continue;

//				if (spr.owner != hitobject) {
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

	it.Reset(CHUNKOWALL);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		dax = spr.xvel >> 3;
		day = spr.yvel >> 3;
		daz = spr.zvel -= TICSPERFRAME << 2;
		movestat = (short) movesprite(i, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 1);
		setsprite(i, spr.x, spr.y, spr.z);
		if (spr.extra == 0) {
			if (spr.lotag < 0) {
				spr.lotag = 8;
				spr.picnum++;
				if (spr.picnum == SMOKEFX + 3) {
					deletesprite((short) i);
					continue;
				}
			}
		} else {
			if (spr.lotag < 0) {
				deletesprite((short) i);
			}
		}
	}

	// CHUNK O MEAT
	it.Reset(CHUNKOMEAT);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		spr.z += spr.zvel;

		daz = spr.zvel += TICSPERFRAME << 4;

		int xvel = (bcos(spr.ang) * TICSPERFRAME) << 3;
		int yvel = (bsin(spr.ang) * TICSPERFRAME) << 3;

		if (spr.picnum == BONECHUNK1 && spr.picnum == BONECHUNKEND) {
			daz >>= 1;
			xvel >>= 1;
			yvel >>= 1;
		}

		movestat = (short) movesprite((short) i, xvel, yvel, daz, 4 << 8, 4 << 8, 1);

		if ((movestat & 0xc000) == 16384) {
			if (sector[spr.sectnum].floorpicnum == WATER || sector[spr.sectnum].floorpicnum == SLIME
					|| sector[spr.sectnum].floorpicnum == FLOORMIRROR) {
				if (spr.picnum == FISH)
					spr.z = sector[spr.sectnum].floorz;
				else {
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				}
				spr.lotag = -1;
			} else {
				/* EG: Add check for parallax sky */
				if (spr.picnum >= BONECHUNK1 && spr.picnum <= BONECHUNKEND
						|| (daz >= zr_ceilz && (sector[spr.sectnum].ceilingstat & 1) != 0)) {
					deletesprite(i);
				} else {
					spr.cstat |= 0x0020;
					spr.lotag = 1200;
					newstatus(i, BLOOD);
				}
			}
		} else if ((movestat & 0xc000) == 32768) {
			if (spr.picnum >= BONECHUNK1 && spr.picnum <= BONECHUNKEND) {
				deletesprite((short) i);
			} else {
				spr.lotag = 600;
				newstatus(i, DRIP);
			}
		}
		if (spr.lotag < 0) {
			deletesprite(i);
		}
	}

	it.Reset(BLOOD);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			if (spr.z < sector[spr.sectnum].floorz) {
				spr.lotag = 600;
				spr.zvel = 0;
				newstatus(i, DRIP);
			} else {
				deletesprite(i);
			}
		}
	}

	it.Reset(DEVILFIRE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		if (plr.invisibletime < 0) {
			spr.lotag -= TICSPERFRAME;
			if (spr.lotag < 0) {
				spr.lotag = (short) (krand() & 120 + 360);
				if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y,
						spr.z - (tileHeight(spr.picnum) << 7), spr.sectnum)) {
					// JSA_NEW
					spritesound(S_FIREBALL, &spr);
					castspell(plr, i);
				}
			}
		}
	}

	it.Reset(DRIP);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		spr.z += spr.zvel;
		dax = 0;
		day = 0;
		daz = spr.zvel += TICSPERFRAME << 1;
		daz = (((spr.zvel) * TICSPERFRAME) << 1);
		movestat = (short) movesprite(i, dax, day, daz, 4 << 8, 4 << 8, 1);

		if ((movestat & 0xc000) == 16384) {
			spr.lotag = 1200;
			newstatus(i, BLOOD);
		}
		if (spr.lotag < 0) {
			deletesprite((short) i);
		}
	}

	it.Reset(SMOKE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

//			
//			spr.z -= (TICSPERFRAME << 6);

		if (spr.xrepeat > 1)
			spr.xrepeat = spr.yrepeat -= TICSPERFRAME;

//			setsprite(i, spr.x, spr.y, spr.z);
		if (spr.lotag < 0) {
			deletesprite((short) i);
		}
	}

	if (!isWh2()) {
		it.Reset(EXPLO);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

			spr.lotag -= TICSPERFRAME;
			spr.x += ((spr.xvel * TICSPERFRAME) >> 5);
			spr.y += ((spr.yvel * TICSPERFRAME) >> 5);
			spr.z -= ((spr.zvel * TICSPERFRAME) >> 6);

			spr.zvel += (TICSPERFRAME << 4);

			if (spr.z < sector[spr.sectnum].ceilingz + (4 << 8)) {
				spr.z = sector[spr.sectnum].ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > sector[spr.sectnum].floorz - (4 << 8)) {
				spr.z = sector[spr.sectnum].floorz - (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}

			spr.xrepeat += TICSPERFRAME;
			spr.yrepeat += TICSPERFRAME;

			spr.lotag -= TICSPERFRAME;

			if (krand() % 100 > 90) {
				j = insertsprite(spr.sectnum, SMOKE);

				sprite[j].x = spr.x;
				sprite[j].y = spr.y;
				sprite[j].z = spr.z;
				sprite[j].cstat = 0x03;
				sprite[j].cstat &= ~3;
				sprite[j].picnum = SMOKEFX;
				sprite[j].shade = 0;
				sprite[j].pal = 0;
				sprite[j].xrepeat = spr.xrepeat;
				sprite[j].yrepeat = spr.yrepeat;

				sprite[j].owner = spr.owner;
				sprite[j].lotag = 256;
				sprite[j].hitag = 0;
			}

			if (spr.lotag < 0) {
				deletesprite(i);
			}
		}
	} else {
		it.Reset(EXPLO);
		while (auto actor = it.Next())
		{
			SPRITE& spr = actor->s();
			int i = actor->GetSpriteIndex();

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

	it.Reset(BROKENVASE);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			spr.picnum++;
			spr.lotag = 18;

			if (spr.picnum == (SHATTERVASE + 6) || spr.picnum == (SHATTERVASE2 + 6)
					|| spr.picnum == (SHATTERVASE3 + 6))
				changespritestat(i, (short) 0);
			else {
				switch (spr.picnum) {
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
	it.Reset(FX);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;

		if (// spr.picnum == PLASMA ||
		spr.picnum == BULLET || spr.picnum == EXPLOSION || spr.picnum == FIREBALL
				|| spr.picnum == MONSTERBALL || spr.picnum == FATSPANK) {

			// spr.z+=spr.zvel;
			spr.zvel += (TICSPERFRAME << 5);
			if (spr.z < sector[spr.sectnum].ceilingz + (4 << 8)) {
				spr.z = sector[spr.sectnum].ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > sector[spr.sectnum].floorz - (4 << 8) && spr.picnum != EXPLOSION) {
				spr.z = sector[spr.sectnum].floorz - (4 << 8);
				spr.zvel = 0;
				spr.lotag = 4;
			}
			dax = ((((int) spr.xvel) * TICSPERFRAME) >> 3);
			day = ((((int) spr.yvel) * TICSPERFRAME) >> 3);
			daz = (((int) spr.zvel) * TICSPERFRAME);
			movestat = (short) movesprite((short) i, dax, day, daz, 4 << 8, 4 << 8, 1);
			setsprite(i, spr.x, spr.y, spr.z);
		}

		if (spr.picnum == ICECUBE && spr.z < sector[spr.sectnum].floorz) {
			spr.z += spr.zvel;

			daz = spr.zvel += TICSPERFRAME << 4;

			movestat = (short) movesprite((short) i, (bcos(spr.ang) * TICSPERFRAME) << 3,
					(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

		}

		if (spr.lotag < 0 || movestat != 0)
			if (spr.picnum == PLASMA || spr.picnum == EXPLOSION || spr.picnum == FIREBALL
					|| spr.picnum == MONSTERBALL || spr.picnum == FATSPANK
					|| spr.picnum == ICECUBE) {
				deletesprite(i);
				continue;
			}

		if (spr.z + (8 << 8) >= sector[spr.sectnum].floorz && spr.picnum == ICECUBE
				|| movestat != 0) {
			spr.z = sector[spr.sectnum].floorz;
			changespritestat(i, (short) 0);
			if (sector[spr.sectnum].floorpicnum == WATER || sector[spr.sectnum].floorpicnum == SLIME
					|| sector[spr.sectnum].floorpicnum == FLOORMIRROR) {
				if (spr.picnum == FISH)
					spr.z = sector[spr.sectnum].floorz;
				else {
					if (krand() % 100 > 60) {
						makemonstersplash(SPLASHAROO, i);
						deletesprite((short) i);
					}
				}
			} else {
				if (spr.lotag < 0) {
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
