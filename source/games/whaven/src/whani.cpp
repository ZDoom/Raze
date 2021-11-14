#include "ns.h"
#include "wh.h"
#include "automap.h"

BEGIN_WH_NS



void animateobjs(PLAYER& plr) {

	boolean hitdamage = false;
	short osectnum = 0;
	int dax, day, daz = 0, j, k;
	Collision moveStat = 0;

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
				moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 2:
				spr.zvel += TICSPERFRAME << 5;
				moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 1,
						(bsin(spr.ang) * TICSPERFRAME) << 1, spr.zvel, 4 << 8, 4 << 8, 0);
				break;
			case 3:
				spr.zvel -= TICSPERFRAME << 5;
				moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 2,
						(bsin(spr.ang) * TICSPERFRAME) << 2, spr.zvel, 4 << 8, 4 << 8, 0);
				if (spr.lotag < 0) {
					spr.lotag = 30;
					spr.extra = 2;
				}
				break;
			}
			if (spr.lotag < 0) {
				DeleteActor(actor);
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

				movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					SetActorPos(actor, &spr.pos);

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

				movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					SetActorPos(actor, &spr.pos);

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

				movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

				if (osectnum != spr.sectnum) {
					spr.x = sparksx;
					spr.y = sparksy;
					spr.z = sparksz;
					spr.ang = (short) ((krand() % 2047) & 2047);
					spr.zvel = 0;
					SetActorPos(actor, &spr.pos);

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
					ChangeActorStat(actor, FACE);
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
					ChangeActorStat(actor, FACE);
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
					ChangeActorStat(actor, FACE);
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
					ChangeActorStat(actor, FACE);
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
					ChangeActorStat(actor, FACE);
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
				SetNewStatus(actor, EVILSPIRIT);
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
				auto spawnedactor = InsertActor(spr.sectnum, FACE);
				auto& spawned = spawnedactor->s();

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
				spawnedactor->SetOwner(nullptr);
				spawned.lotag = 40;
				spawned.hitag = 0;
				spawned.detail = GONZOTYPE;
				DeleteActor(actor);
				spawned.backuploc();
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
			SetActorPos(actor, plr.x, plr.y, plr.z);
			osectnum = spr.sectnum;
			j = (torchpattern[PlayClock % 38]);
			sector[osectnum].ceilingshade = (byte) ((sector[osectnum].ceilingshade + j) >> 1);
			sector[osectnum].floorshade = (byte) ((sector[osectnum].floorshade + j) >> 1);
			startwall = sector[osectnum].wallptr;
			endwall = (short) (startwall + sector[osectnum].wallnum - 1);
			for (k = startwall; k <= endwall; k++) {
				wall[k].shade = (byte) ((wall[k].shade + j) >> 1);
			}
			moveStat = movesprite(actor,
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
				DeleteActor(actor);
				// set back to normall
				plr.Sector()->ceilingshade = ceilingshadearray[plr.sector];
				plr.Sector()->floorshade = floorshadearray[plr.sector];
				startwall = plr.Sector()->wallptr;
				endwall = (short) (startwall + plr.Sector()->wallnum - 1);
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
				ChangeActorStat(actor, 0);
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
				ChangeActorStat(actor, 0);
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
				ChangeActorStat(actor, 0);
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
				DeleteActor(actor);
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
				ChangeActorStat(actor, 0);
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
				moveStat = movesprite(actor,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				SetActorPos(actor, &spr.pos);
				if (moveStat.type != kHitNone)
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
				moveStat = movesprite(actor,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				SetActorPos(actor, &spr.pos);
				if (moveStat.type != kHitNone)
					spr.ang = (short) (krand() & 2047);
			}
			break;
		case 2: // fly to roof and get deleted
			if (spr.lotag < 0) {
				if (i == lastbat) {
					soundEngine->StopSound(CHAN_BAT);
				}
				DeleteActor(actor);
				continue;
			} else {
				moveStat = movesprite(actor,
						(bcos(spr.ang) * TICSPERFRAME) << 3,
						(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
				SetActorPos(actor, &spr.pos);
				if (moveStat.type == kHitSector) {// Hits a ceiling / floor
					if (i == lastbat) {
						soundEngine->StopSound(CHAN_BAT);
					}
					DeleteActor(actor);
					continue;
				}
				if (moveStat.type != kHitNone)
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
			SetActorPos(actor, &spr.pos);
			if (spr.z <= spr.sector()->ceilingz + 32768) {
				soundEngine->StopSound(CHAN_CART);
				spritesound(S_CLUNK, actor);
				ChangeActorStat(actor, 0);
				spr.lotag = 1820;
				spr.z = spr.sector()->ceilingz + 32768;
			}
			break;
		case 1811:
			
			spr.z -= (TICSPERFRAME << 6);
			SetActorPos(actor, &spr.pos);
			if (spr.z <= spr.sector()->ceilingz + 65536) {
				ChangeActorStat(actor, 0);
				spr.lotag = 1810;
				spr.z = spr.sector()->ceilingz + 65536;
			}
			break;
		case 1801:
			
			spr.z -= (TICSPERFRAME << 6);
			SetActorPos(actor, &spr.pos);
			if (spr.z <= spr.sector()->ceilingz + 65536) {
				ChangeActorStat(actor, 0);
				spr.lotag = 1800;
				spr.z = spr.sector()->ceilingz + 65536;
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
			SetActorPos(actor, &spr.pos);
			if (spr.z >= (spr.sector()->floorz - 32768)) {
				soundEngine->StopSound(CHAN_CART);
				spritesound(S_CLUNK, actor);
				ChangeActorStat(actor, 0);
				spr.lotag = 1821;
				spr.z = spr.sector()->floorz - 32768;
			}
			break;
		case 1810:
			ironbarmove = TICSPERFRAME << 6;
			spr.z += ironbarmove;
			SetActorPos(actor, &spr.pos);
			if (spr.z >= spr.sector()->floorz) {
				ChangeActorStat(actor, 0);
				spr.lotag = 1811;
				spr.z = spr.sector()->floorz;
			}
			break;
		case 1800:
			
			ironbarmove = TICSPERFRAME << 6;
			spr.z += ironbarmove;
			SetActorPos(actor, &spr.pos);
			if (spr.z >= spr.sector()->floorz) {
				ChangeActorStat(actor, 0);
				spr.lotag = 1801;
				spr.z = spr.sector()->floorz;
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
		spr.z = spr.sector()->floorz + (tileHeight(spr.picnum) << 8);
		SetActorPos(actor, &spr.pos);

		if (spr.lotag <= 0) {
			if ((spr.picnum >= SPLASHAROO && spr.picnum < LASTSPLASHAROO)
					|| (spr.picnum >= LAVASPLASH && spr.picnum < LASTLAVASPLASH)
					|| (spr.picnum >= SLIMESPLASH && spr.picnum < LASTSLIMESPLASH)) {
				spr.picnum++;
				spr.lotag = 8;
			} else {
				DeleteActor(actor);
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
			ChangeActorStat(actor, 0);
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

		if (spr.z < spr.sector()->floorz)
			spr.z += (int) TICSPERFRAME << 8;
		if (spr.z > spr.sector()->floorz)
			spr.z = spr.sector()->floorz;

		if (spr.lotag < 0) {
			switch (spr.picnum) {
			case LFIRE:
				spr.picnum = SFIRE;
				spr.lotag = 2047;
				break;
			case SFIRE:
				DeleteActor(actor);
				continue;
			}
		}

		if (checkdist(actor, plr.x, plr.y, plr.z)) {
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

		moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		SetActorPos(actor, &spr.pos);

		if (spr.picnum == FBARRELFALL || spr.picnum >= BOULDER && spr.picnum <= BOULDER + 3
				&& (checkdist(actor, plr.x, plr.y, plr.z))) {
			addhealth(plr, -50);
			startredflash(50);
		}

		if (moveStat.type == kHitSector) {
			if (spr.sector()->floorpicnum == WATER) {
				makemonstersplash(SPLASHAROO, i);
			}
			switch (spr.picnum) {
			case FBARRELFALL:
				SetNewStatus(actor, SHATTER);
				spr.lotag = 12;
				break;
			default:
				if (spr.picnum == TORCH) {
					for (k = 0; k < 16; k++)
						makeafire(i, 0);
					DeleteActor(actor);
					break;
				}
				ChangeActorStat(actor, 0);
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

		moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		SetActorPos(actor, &spr.pos);

		if (spr.z >= spr.sector()->floorz) {
			if (spr.sector()->floorpicnum == WATER
					|| spr.sector()->floorpicnum == FLOORMIRROR) {
				makemonstersplash(SPLASHAROO, i);
			}
			SetNewStatus(actor, BROKENVASE);
			continue;
		}

		if (moveStat.type == kHitSector) {
			SetNewStatus(actor, BROKENVASE);
			continue;
		}

		if (moveStat.type == kHitSprite) { // Bullet hit a sprite

			if (actor->GetOwner() != moveStat.actor) {
				hitdamage = damageactor(plr, moveStat.actor, actor);
				if (hitdamage) {
					SetNewStatus(actor, BROKENVASE);
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
		moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 0);

		SetActorPos(actor, &spr.pos);

		if (spr.lotag < 0 || moveStat.type == kHitWall) {
			spr.lotag = 0;
			ChangeActorStat(actor, 0);
			if (spr.z < spr.sector()->floorz) {
				spr.zvel += 256L;
				ChangeActorStat(actor, FALL);
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
				SetNewStatus(actor, ACTIVE);
			}
		} else {
			spr.lotag -= TICSPERFRAME;
			spr.xrepeat = spr.yrepeat = 2;
			if (spr.lotag < 0) {
				SetNewStatus(actor, ACTIVE);
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
				SetNewStatus(actor, DORMANT);
			}
		} else {
			spr.lotag -= TICSPERFRAME;
			spr.xrepeat = 48;
			spr.yrepeat = 32;
			if (spr.lotag < 0) {
				SetNewStatus(actor, DORMANT);
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
				DeleteActor(actor);
				continue;
			}

			if (spr.picnum == MONSTERBALL && krand() % 100 > 90) {
				if (spr.lotag < 200)
					trailingsmoke(actor,false);
			}
			spr.z += spr.zvel;
			if (spr.z < spr.sector()->ceilingz + (4 << 8)) {
				spr.z = spr.sector()->ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > spr.sector()->floorz - (4 << 8)) {
				spr.z = spr.sector()->floorz - (4 << 8);
				if (spr.sector()->floorpicnum == WATER || spr.sector()->floorpicnum == SLIME
						|| spr.sector()->floorpicnum == FLOORMIRROR)
					if (spr.picnum == FISH)
						spr.z = spr.sector()->floorz;
					else {
						if (krand() % 100 > 60)
							makemonstersplash(SPLASHAROO, i);
					}
					
//					if (spr.picnum != THROWPIKE) { //XXX
					DeleteActor(actor);
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
			moveStat = movesprite(actor,
					(bcos(spr.extra) * TICSPERFRAME) << 6,
					(bsin(spr.extra) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 1);
			spr.cstat = 21;
		} else {
			moveStat = movesprite(actor,
					(bcos(spr.ang) * TICSPERFRAME) << 6, // was 3
					(bsin(spr.ang) * TICSPERFRAME) << 6, // was 3
					daz, 4 << 8, 4 << 8, 1);
		}

		if (moveStat.type != kHitNone && spr.picnum == MONSTERBALL)
			if (actor->GetPlayerOwner() == plr.playerNum()) {
				explosion2(i, spr.x, spr.y, spr.z, i);
			} else {
				explosion(i, spr.x, spr.y, spr.z, i);
			}

		if (moveStat.type == kHitSector) { // Hits a ceiling / floor
			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
				ChangeActorStat(actor, 0);
					
				continue;
			}
			DeleteActor(actor);
			continue;
		} else if (moveStat.type == kHitWall) { // hit a wall

			if (spr.picnum == MONSTERBALL) {
				if (actor->GetPlayerOwner() == plr.playerNum())
					explosion2(i, spr.x, spr.y, spr.z, i);
				else
					explosion(i, spr.x, spr.y, spr.z, i);
			}
			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
				ChangeActorStat(actor, 0);
					
				continue;
			}
			DeleteActor(actor);
			continue;
		} else if (spr.lotag < 0 && spr.picnum == PLASMA)
			moveStat.type = -1;

		if (moveStat.type == kHitSprite) { // Bullet hit a sprite
			if (spr.picnum == MONSTERBALL) {
				if (actor->GetPlayerOwner() == plr.playerNum())
					explosion2(i, spr.x, spr.y, spr.z, i);
				else
					explosion(i, spr.x, spr.y, spr.z, i);
			}

			if (actor->GetOwner() != moveStat.actor)
				hitdamage = damageactor(plr, moveStat.actor, actor);
			if (hitdamage) {
				DeleteActor(actor);
				continue;
			}
		}

		if (moveStat.type != kHitNone || spr.lotag < 0) {
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

				DeleteActor(actor);
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
			if (spr.z < spr.sector()->ceilingz + (4 << 8)) {
				spr.z = spr.sector()->ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > spr.sector()->floorz - (4 << 8)) {
				spr.z = spr.sector()->floorz - (4 << 8);
				if (spr.sector()->floorpicnum == WATER || spr.sector()->floorpicnum == SLIME
						|| spr.sector()->floorpicnum == FLOORMIRROR)
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				DeleteActor(actor);
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

		moveStat = movesprite(actor, (bcos(spr.extra) * TICSPERFRAME) << 6,
				(bsin(spr.extra) * TICSPERFRAME) << 6, daz, 4 << 8, 4 << 8, 0);

		if (spr.picnum == WALLARROW || spr.picnum == THROWHALBERD)
			spr.cstat = 0x11;
		else if (spr.picnum == DART)
			spr.cstat = 0x10;
		else
			spr.cstat = 0x15;

		if (moveStat.type == kHitSector) { // Hits a ceiling / floor
			// EG Bugfix 17 Aug 2014: Since the game thinks that a javlin hitting the
			// player's pike axe is a
			// floor/ceiling hit rather than a sprite hit, we'll need to check if the JAVLIN
			// is
			// actually in the floor/ceiling before going inactive.
			if (spr.z <= spr.sector()->ceilingz
					&& spr.z >= spr.sector()->floorz) {
				if (spr.picnum == THROWPIKE) {
					spr.picnum++;
					spr.detail = WALLPIKETYPE;
				}

				ChangeActorStat(actor, INACTIVE); // EG Note: RAF.H gives this a nice name, so use it
			}
			continue;
		} else if (moveStat.type == kHitWall) { // hit a wall

			if (spr.picnum == THROWPIKE) {
				spr.picnum++;
				spr.detail = WALLPIKETYPE;
			}

			ChangeActorStat(actor, INACTIVE);
			continue;
		}

		if (moveStat.type == kHitSprite) { // Bullet hit a sprite

			hitdamage = damageactor(plr, moveStat.actor, actor);
			if (hitdamage)
				continue;

			if (!hitdamage)
				if (isBlades(moveStat.actor->s().picnum)) {
					DeleteActor(actor);
					continue;
				}
		}

		if (moveStat.type != kHitNone) {
			DeleteActor(actor);
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
		moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
				(bsin(spr.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 1);
		SetActorPos(actor, &spr.pos);
		if (spr.extra == 0) {
			if (spr.lotag < 0) {
				spr.lotag = 8;
				spr.picnum++;
				if (spr.picnum == SMOKEFX + 3) {
					DeleteActor(actor);
					continue;
				}
			}
		} else {
			if (spr.lotag < 0) {
				DeleteActor(actor);
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

		moveStat = movesprite(actor, xvel, yvel, daz, 4 << 8, 4 << 8, 1);

		if (moveStat.type == kHitSector) {
			if (spr.sector()->floorpicnum == WATER || spr.sector()->floorpicnum == SLIME
					|| spr.sector()->floorpicnum == FLOORMIRROR) {
				if (spr.picnum == FISH)
					spr.z = spr.sector()->floorz;
				else {
					if (krand() % 100 > 60)
						makemonstersplash(SPLASHAROO, i);
				}
				spr.lotag = -1;
			} else {
				/* EG: Add check for parallax sky */
				if (spr.picnum >= BONECHUNK1 && spr.picnum <= BONECHUNKEND
						|| (daz >= zr_ceilz && (spr.sector()->ceilingstat & 1) != 0)) {
					DeleteActor(actor);
				} else {
					spr.cstat |= 0x0020;
					spr.lotag = 1200;
					SetNewStatus(actor, BLOOD);
				}
			}
		} else if (moveStat.type == kHitWall) {
			if (spr.picnum >= BONECHUNK1 && spr.picnum <= BONECHUNKEND) {
				DeleteActor(actor);
			} else {
				spr.lotag = 600;
				SetNewStatus(actor, DRIP);
			}
		}
		if (spr.lotag < 0) {
			DeleteActor(actor);
		}
	}

	it.Reset(BLOOD);
	while (auto actor = it.Next())
	{
		SPRITE& spr = actor->s();
		int i = actor->GetSpriteIndex();

		spr.lotag -= TICSPERFRAME;
		if (spr.lotag < 0) {
			if (spr.z < spr.sector()->floorz) {
				spr.lotag = 600;
				spr.zvel = 0;
				SetNewStatus(actor, DRIP);
			} else {
				DeleteActor(actor);
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
					spritesound(S_FIREBALL, actor);
					castspell(plr, actor);
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
		moveStat = movesprite(actor, dax, day, daz, 4 << 8, 4 << 8, 1);

		if (moveStat.type == kHitSector) {
			spr.lotag = 1200;
			SetNewStatus(actor, BLOOD);
		}
		if (spr.lotag < 0) {
			DeleteActor(actor);
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

//			SetActorPos(actor, &spr.pos);
		if (spr.lotag < 0) {
			DeleteActor(actor);
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

			if (spr.z < spr.sector()->ceilingz + (4 << 8)) {
				spr.z = spr.sector()->ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > spr.sector()->floorz - (4 << 8)) {
				spr.z = spr.sector()->floorz - (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}

			spr.xrepeat += TICSPERFRAME;
			spr.yrepeat += TICSPERFRAME;

			spr.lotag -= TICSPERFRAME;

			if (krand() % 100 > 90) {
				auto spawnedactor = InsertActor(spr.sectnum, SMOKE);
				auto& spawned = spawnedactor->s();

				spawned.x = spr.x;
				spawned.y = spr.y;
				spawned.z = spr.z;
				spawned.cstat = 0x03;
				spawned.cstat &= ~3;
				spawned.picnum = SMOKEFX;
				spawned.shade = 0;
				spawned.pal = 0;
				spawned.xrepeat = spr.xrepeat;
				spawned.yrepeat = spr.yrepeat;

				spawnedactor->CopyOwner(actor);
				spawned.lotag = 256;
				spawned.hitag = 0;
				spawned.backuploc();
			}

			if (spr.lotag < 0) {
				DeleteActor(actor);
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

			WHSectIterator it(spr.sectnum);
			while (auto sectactor = it.Next())
			{
				SPRITE& tspr = sectactor->s();
				int j = sectactor->GetSpriteIndex();

				int dx = abs(spr.x - tspr.x); // x distance to sprite
				int dy = abs(spr.y - tspr.y); // y distance to sprite
				int dz = abs((spr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
				int dh = tileHeight(tspr.picnum) >> 1; // height of sprite
				if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
					if (sectactor->GetPlayerOwner() == 0) {
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
									SetNewStatus(sectactor, DIE);
								}
							}
							break;
						}
					}
				}
			}

			if (spr.picnum == EXPLOEND) {
				DeleteActor(actor);
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
				ChangeActorStat(actor, 0);
			else {
				switch (spr.picnum) {
				case FSHATTERBARREL + 2:
					randompotion(actor);
					ChangeActorStat(actor, 0);
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
					ChangeActorStat(actor, 0);
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
			if (spr.z < spr.sector()->ceilingz + (4 << 8)) {
				spr.z = spr.sector()->ceilingz + (4 << 8);
				spr.zvel = (short) -(spr.zvel >> 1);
			}
			if (spr.z > spr.sector()->floorz - (4 << 8) && spr.picnum != EXPLOSION) {
				spr.z = spr.sector()->floorz - (4 << 8);
				spr.zvel = 0;
				spr.lotag = 4;
			}
			dax = ((((int) spr.xvel) * TICSPERFRAME) >> 3);
			day = ((((int) spr.yvel) * TICSPERFRAME) >> 3);
			daz = (((int) spr.zvel) * TICSPERFRAME);
			moveStat = movesprite(actor, dax, day, daz, 4 << 8, 4 << 8, 1);
			SetActorPos(actor, &spr.pos);
		}

		if (spr.picnum == ICECUBE && spr.z < spr.sector()->floorz) {
			spr.z += spr.zvel;

			daz = spr.zvel += TICSPERFRAME << 4;

			moveStat = movesprite(actor, (bcos(spr.ang) * TICSPERFRAME) << 3,
					(bsin(spr.ang) * TICSPERFRAME) << 3, daz, 4 << 8, 4 << 8, 1);

		}

		if (spr.lotag < 0 || moveStat.type != kHitNone)
			if (spr.picnum == PLASMA || spr.picnum == EXPLOSION || spr.picnum == FIREBALL
					|| spr.picnum == MONSTERBALL || spr.picnum == FATSPANK
					|| spr.picnum == ICECUBE) {
				DeleteActor(actor);
				continue;
			}

		if (spr.z + (8 << 8) >= spr.sector()->floorz && spr.picnum == ICECUBE
				|| moveStat.type != kHitNone)
		{
			spr.z = spr.sector()->floorz;
			ChangeActorStat(actor, 0);
			if (spr.sector()->floorpicnum == WATER || spr.sector()->floorpicnum == SLIME
					|| spr.sector()->floorpicnum == FLOORMIRROR) {
				if (spr.picnum == FISH)
					spr.z = spr.sector()->floorz;
				else {
					if (krand() % 100 > 60) {
						makemonstersplash(SPLASHAROO, i);
						DeleteActor(actor);
					}
				}
			} else {
				if (spr.lotag < 0) {
					DeleteActor(actor);
				}
			}
		}
	}
}

boolean isBlades(int pic) {
	return pic == THROWPIKE || pic == WALLARROW || pic == DART || pic == HORIZSPIKEBLADE || pic == THROWHALBERD;
}

END_WH_NS
