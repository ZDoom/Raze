#include "ns.h"
#include "wh.h"
#include "automap.h"

BEGIN_WH_NS

PLAYER player[MAXPLAYERS];
PLOCATION gPrevPlayerLoc[MAXPLAYERS];
	
short monsterangle[MAXSPRITESONSCREEN],	monsterlist[MAXSPRITESONSCREEN];
int shootgunzvel;
	
boolean justteleported;

int victor = 0;

int pyrn;
int mapon;
int damage_vel, damage_svel, damage_angvel;

void viewBackupPlayerLoc( int nPlayer )
{
	SPRITE& pSprite = sprite[player[nPlayer].spritenum];
	PLOCATION& pPLocation = gPrevPlayerLoc[nPlayer];
	pPLocation.x = pSprite.x;
	pPLocation.y = pSprite.y;
	pPLocation.z = player[nPlayer].z;
	player[nPlayer].angle.backup();
	player[nPlayer].horizon.backup();
}
	
void playerdead(PLAYER& plr) {
	if (plr.dead)
		return;

	if (plr.potion[0] > 0 && plr.spiked == 0) {
		int i = plr.currentpotion;
		plr.currentpotion = 0;
		usapotion(plr);
		plr.currentpotion = i;
		return;
	}

	plr.currspikeframe = 0;

	if (plr.spiked == 1) {
		plr.spiketics = spikeanimtics[0].daweapontics;
		spritesound(S_GORE1, plr.actor());
		SND_Sound(S_HEARTBEAT);
	}

	SND_Sound(S_PLRDIE1);

//	 	netsendmove();
	plr.dead = true;
}

void initplayersprite(PLAYER& plr) {
	if (difficulty > 1) {
		plr.currweapon = 1;
		plr.selectedgun = 1;
	} else {
		plr.currweapon = 4;
		plr.selectedgun = 4;
	}
	plr.currentpotion = 0;
	plr.helmettime = -1;
	plr.shadowtime = -1;
	plr.nightglowtime = -1;
	plr.strongtime = -1;
	plr.invisibletime = -1;
	plr.manatime = -1;
	plr.currentorb = 0;
	plr.currweaponfired = 3;
	plr.currweaponanim = 0;
	plr.currweaponattackstyle = 0;
	plr.currweaponflip = 0;

	plr.vampiretime = 0;
	plr.shieldpoints = 0;
	plr.shieldtype = 0;
	plr.dead = false;
	plr.spiked = 0;
	plr.shockme = -1;
	plr.poisoned = 0;
	plr.poisontime = -1;

	plr.oldsector = plr.sector;
	plr.horizon.horiz = q16horiz(0);
	plr.height = getPlayerHeight();
	plr.z = plr.Sector()->floorz - (plr.height << 8);

	plr.spritenum = (short) insertsprite(plr.sector, (short) 0);
	auto& spr = sprite[plr.spritenum];

	plr.onsomething = 1;

	spr.x = plr.x;
	spr.y = plr.y;
	spr.z = plr.z + (plr.height << 8);
	spr.cstat = 1 + 256;
	spr.picnum = isWh2() ? GRONSW : FRED;
	spr.shade = 0;
	spr.xrepeat = 36;
	spr.yrepeat = 36;
	spr.ang = plr.angle.ang.asbuild();
	spr.xvel = 0;
	spr.yvel = 0;
	spr.zvel = 0;
	spr.owner = (short) (4096 + myconnectindex);
	spr.lotag = 0;
	spr.hitag = 0;
	spr.pal = (short) (isWh2() ? 10 : 1);
	if(isWh2())
		spr.clipdist = 48;
		
	plr.selectedgun = 0;
		
	if(isWh2()) {
		for (int i = 0; i <= 9; i++) {
			if (i < 5) {
				plr.ammo[i] = 40;
				plr.weapon[i] = 1;
			} else {
				plr.ammo[i] = 0;
				plr.weapon[i] = 0;
			}
			if (i < 8) {
				plr.orb[i] = 0;
				plr.orbammo[i] = 0;
			}
		}
			
		if (difficulty > 1) {
			plr.weapon[0] = plr.weapon[1] = 1;
			plr.ammo[0] = 32000;
			plr.ammo[1] = 45;
		} 
			
	} else {
		
		if (difficulty > 1) {
			for (int i = 0; i <= 9; i++) {
				plr.ammo[i] = 0;
				plr.weapon[i] = 0;
				if (i < 8) {
					plr.orb[i] = 0;
					plr.orbammo[i] = 0;
				}
			}
			plr.weapon[0] = plr.weapon[1] = 1;
			plr.ammo[0] = 32000;
			plr.ammo[1] = 45;
		} else {
			for (int i = 0; i <= 9; i++) {
				plr.ammo[i] = 0;
				plr.weapon[i] = 0;
				if (i < 5) {
					plr.ammo[i] = 40;
					plr.weapon[i] = 1;
				}
				if (i < 8) {
					plr.orb[i] = 0;
					plr.orbammo[i] = 0;
				}
			}
		}
	}

	for (int i = 0; i < MAXPOTIONS; i++)
		plr.potion[i] = 0;
	for (int i = 0; i < MAXTREASURES; i++)
		plr.treasure[i] = 0;

	plr.lvl = 1;
	plr.score = 0;
	plr.health = 100;
	plr.maxhealth = 100;
	plr.armor = 0;
	plr.armortype = 0;
	plr.currentorb = 0;
	plr.currentpotion = 0;

	if (difficulty > 1)
		plr.currweapon = plr.selectedgun = 1;
	else
		plr.currweapon = plr.selectedgun = 4;

	if (isWh2()) {
		plr.potion[0] = 3;
		plr.potion[3] = 1;
		plr.currweapon = plr.selectedgun = 4;
	}

	plr.currweaponfired = 3;
	plr.currweaponflip = 0;

	for (int i = 0; i < MAXNUMORBS; i++)
		plr.orbactive[i] = -1;

	PlayClock = 0;
	playertorch = 0;

	plr.spellbookflip = 0;

	plr.invincibletime = plr.manatime = -1;
	plr.hasshot = 0;
	plr.orbshot = 0;
	plr.shadowtime = -1;
	plr.helmettime = -1;
	plr.nightglowtime = -1;
	plr.strongtime = -1;
	plr.invisibletime = -1;
	spr.backuploc();
}

void updateviewmap(PLAYER& plr) {
	int i;
	if ((i = plr.sector) > -1) {
		int wallid = sector[i].wallptr;
		show2dsector.Set(i);
		for (int j = sector[i].wallnum; j > 0; j--) {
			WALL& wal = wall[wallid++];
			i = wal.nextsector;
			if (i < 0)
				continue;
			if ((wal.cstat & 0x0071) != 0)
				continue;
			if ((wall[wal.nextwall].cstat & 0x0071) != 0)
				continue;
			if (sector[i].ceilingz >= sector[i].floorz)
				continue;
			show2dsector.Set(i);
		}
	}
}

void plruse(PLAYER& plr) {
	Neartag nt;
	neartag(plr.x, plr.y, plr.z, (short) plr.sector, plr.angle.ang.asbuild(), nt, 1024, 3);

	if (nt.tagsector >= 0) {
		if (sector[nt.tagsector].hitag == 0) {
			operatesector(plr, nt.tagsector);
		} else {
			short daang = plr.angle.ang.asbuild();
			int daz2 = -MulScale(plr.horizon.horiz.asq16(), 2000, 16);
			Hitscan pHitInfo;
			hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
					bcos(daang), // X vector of 3D ang
					bsin(daang), // Y vector of 3D ang
					daz2, // Z vector of 3D ang
					pHitInfo, CLIPMASK1);

			if (pHitInfo.hitwall >= 0) {
				if ((abs(plr.x - pHitInfo.hitx) + abs(plr.y - pHitInfo.hity) < 512)
						&& (abs((plr.z >> 8) - ((pHitInfo.hitz >> 8) - (64))) <= (512 >> 3))) {
					int pic = wall[pHitInfo.hitwall].picnum;
					if(pic == PENTADOOR1 || pic == PENTADOOR2 || (pic >= PENTADOOR3 && pic <= PENTADOOR7))
						showmessage("find door trigger", 360);
				}
			}
			spritesound(S_PUSH1 + (krand() % 2), plr.actor());
		}
	}
	if (nt.tagsprite >= 0) {
		if (sprite[nt.tagsprite].lotag == 1) {
			if(sprite[nt.tagsprite].picnum == PULLCHAIN1 || sprite[nt.tagsprite].picnum == SKULLPULLCHAIN1) {
				sprite[nt.tagsprite].lotag = 0;
				newstatus(nt.tagsprite, PULLTHECHAIN);
			} else if(sprite[nt.tagsprite].picnum == LEVERUP) {
				sprite[nt.tagsprite].lotag = 0;
				newstatus(nt.tagsprite, ANIMLEVERUP);
			}
			for (int i = 0; i < numsectors; i++)
				if (sector[i].hitag == sprite[nt.tagsprite].hitag)
					operatesector(plr, i);
		} else
			operatesprite(plr, nt.tagsprite);
	}
}

void chunksofmeat(PLAYER& plr, DWHActor* hitActor, int hitx, int hity, int hitz, short hitsect, int daang) {

	short k;
	short zgore = 0;
	int chunk = REDCHUNKSTART;
	int newchunk;

	auto& hitspr = hitActor->s();

	if (adult_lockout)
		return;

	if (hitspr.picnum == JUDY || hitspr.picnum == JUDYATTACK1
			|| hitspr.picnum == JUDYATTACK2)
		return;

	switch (plr.selectedgun) {
	case 1:
	case 2:
		zgore = 1;
		break;
	case 3:
	case 4:
		zgore = 2;
		break;
	case 5:
		zgore = 3;
		break;
	case 6:
		zgore = 1;
		break;
	case 7:
		zgore = 2;
		break;
	case 8:
	case 9:
		zgore = 3;
		break;
	}

	if (hitspr.statnum == NUKED) {
		zgore = 32;
	}

	if (hitspr.picnum == RAT)
		zgore = 1;

	if (hitspr.picnum == WILLOW || hitspr.picnum == WILLOWEXPLO
			|| hitspr.picnum == WILLOWEXPLO + 1 || hitspr.picnum == WILLOWEXPLO + 2
			|| hitspr.picnum == GUARDIAN || hitspr.picnum == GUARDIANATTACK
			|| hitspr.picnum == DEMON)
		return;

	if (hitspr.picnum == SKELETON || hitspr.picnum == SKELETONATTACK
			|| hitspr.picnum == SKELETONDIE) {
		spritesound(S_SKELHIT1 + (krand() % 2), hitActor);
	} else {
		if (krand() % 100 > 60)
			spritesound(S_GORE1 + (krand() % 4), hitActor);
	}

	if (hitActor != nullptr) {
		for (k = 0; k < zgore; k++) {
			newchunk = 0;

			auto spawnedactor = InsertActor(hitsect, CHUNKOMEAT);
			auto& spawned = spawnedactor->s();

			spawned.x = hitx;
			spawned.y = hity;
			spawned.z = hitz;
			spawned.cstat = 0;
			if (krand() % 100 > 50) {
				switch (hitspr.detail) {
				case GRONTYPE:
					chunk = REDCHUNKSTART + (krand() % 8);
					break;
				case KOBOLDTYPE:
					if (hitspr.pal == 0)
						chunk = BROWNCHUNKSTART + (krand() % 8);
					if (hitspr.pal == 4)
						chunk = GREENCHUNKSTART + (krand() % 8);
					if (hitspr.pal == 7)
						chunk = REDCHUNKSTART + (krand() % 8);
					break;
				case DRAGONTYPE:
					chunk = GREENCHUNKSTART + (krand() % 8);
					break;
				case DEVILTYPE:
					chunk = REDCHUNKSTART + (krand() % 8);
					break;
				case FREDTYPE:
					chunk = BROWNCHUNKSTART + (krand() % 8);
					break;
				case GOBLINTYPE:
				case IMPTYPE:
					if(isWh2() && (hitspr.picnum == IMP || hitspr.picnum == IMPATTACK)) {
						if (hitspr.pal == 0)
							chunk = GREENCHUNKSTART + (krand() % 8);
					} else {
						if (hitspr.pal == 0)
							chunk = GREENCHUNKSTART + (krand() % 8);
						if (hitspr.pal == 4)
							chunk = BROWNCHUNKSTART + (krand() % 8);
						if (hitspr.pal == 5)
							chunk = TANCHUNKSTART + (krand() % 8);
					}
					break;
				case MINOTAURTYPE:
					chunk = TANCHUNKSTART + (krand() % 8);
					break;
				case SPIDERTYPE:
					chunk = GREYCHUNKSTART + (krand() % 8);
					break;
				case SKULLYTYPE:
				case FATWITCHTYPE:
				case JUDYTYPE:
					chunk = REDCHUNKSTART + (krand() % 8);
					break;
				}
			} else {
				newchunk = 1;
				if (!isWh2())
					chunk = NEWCHUNK + (krand() % 9);
				else
					chunk = REDCHUNKSTART + (krand() % 8);
			}
				
			if (hitspr.detail == SKELETONTYPE)
				chunk = BONECHUNK1 + (krand() % 9);

			if (plr.weapon[2] == 3 && plr.currweapon == 2) {
				spawned.picnum = ARROWFLAME;
			} else {
				spawned.picnum = (short) chunk; // = REDCHUNKSTART + (rand() % 8);
			}

			spawned.shade = -16;
			spawned.xrepeat = 64;
			spawned.yrepeat = 64;
			spawned.clipdist = 16;
			spawned.ang = (short) (((krand() & 1023) - 1024) & 2047);
			spawned.xvel = (short) ((krand() & 1023) - 512);
			spawned.yvel = (short) ((krand() & 1023) - 512);
			spawned.zvel = (short) ((krand() & 1023) - 512);
			if (newchunk == 1)
				spawned.zvel <<= 1;
			spawned.owner = sprite[plr.spritenum].owner;
			spawned.lotag = 512;
			spawned.hitag = 0;
			spawned.pal = 0;
			movesprite(spawnedactor, (bcos(spawned.ang) * TICSPERFRAME) << 3,
					(bsin(spawned.ang) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
			spawned.backuploc();
		}
	}

}

void addhealth(PLAYER& plr, int hp) {
	if (plr.godMode && hp < 0)
		return;

	plr.health += hp;

	if (plr.health < 0)
		plr.health = 0;
}

void addarmor(PLAYER& plr, int arm) {
	plr.armor += arm;

	if (plr.armor < 0) {
		plr.armor = 0;
		plr.armortype = 0;
	}
}

void addscore(PLAYER* plr, int score) {
	if(plr == nullptr) return;
		
	plr->score += score;
	expgained += score;

	goesupalevel(*plr);
}

void goesupalevel(PLAYER& plr) {
	if (isWh2())
		goesupalevel2(plr);
	else
		goesupalevel1(plr);
}

void goesupalevel2(PLAYER& plr) {
	switch (plr.lvl) {
	case 0:
	case 1:
		if (plr.score > 9999) {
			showmessage("thou art a warrior", 360);
			plr.lvl = 2;
			plr.maxhealth = 120;
		}
		break;
	case 2:
		if (plr.score > 19999) {
			showmessage("thou art a swordsman", 360);
			plr.lvl = 3;
			plr.maxhealth = 140;
		}
		break;
	case 3:
		if (plr.score > 29999) {
			showmessage("thou art a hero", 360);
			plr.lvl = 4;
			plr.maxhealth = 160;
		}
		break;
	case 4:
		if (plr.score > 39999) {
			showmessage("thou art a champion", 360);
			plr.lvl = 5;
			plr.maxhealth = 180;
		}
		break;
	case 5:
		if (plr.score > 49999) {
			showmessage("thou art a superhero", 360);
			plr.lvl = 6;
			plr.maxhealth = 200;
		}
		break;
	case 6:
		if (plr.score > 59999) {
			showmessage("thou art a lord", 360);
			plr.lvl = 7;
		}
	}
}

void goesupalevel1(PLAYER& plr) {
	if (plr.score > 2250 && plr.score < 4499 && plr.lvl < 2) {
		showmessage("thou art 2nd level", 360);
		plr.lvl = 2;
		plr.maxhealth = 120;
	} else if (plr.score > 4500 && plr.score < 8999 && plr.lvl < 3) {
		showmessage("thou art 3rd level", 360);
		plr.lvl = 3;
		plr.maxhealth = 140;
	} else if (plr.score > 9000 && plr.score < 17999 && plr.lvl < 4) {
		showmessage("thou art 4th level", 360);
		plr.lvl = 4;
		plr.maxhealth = 160;
	} else if (plr.score > 18000 && plr.score < 35999 && plr.lvl < 5) {
		showmessage("thou art 5th level", 360);
		plr.lvl = 5;
		plr.maxhealth = 180;
	} else if (plr.score > 36000 && plr.score < 74999 && plr.lvl < 6) {
		showmessage("thou art 6th level", 360);
		plr.lvl = 6;
		plr.maxhealth = 200;
	} else if (plr.score > 75000 && plr.score < 179999 && plr.lvl < 7) {
		showmessage("thou art 7th level", 360);
		plr.lvl = 7;
	} else if (plr.score > 180000 && plr.score < 279999 && plr.lvl < 8) {
		showmessage("thou art 8th level", 360);
		plr.lvl = 8;
	} else if (plr.score > 280000 && plr.score < 379999 && plr.lvl < 9) {
		showmessage("thou art hero", 360);
		plr.lvl = 9;
	}
}

void lockon(PLAYER& plr, int numshots, int shootguntype) {
	short daang, i, k, n = 0, s;

	for (i = 0; i < tspritelistcnt && n < numshots; i++) {
		auto &spr = tspritelist[i];

		if (cansee(plr.x, plr.y, plr.z, plr.sector, spr.x, spr.y, spr.z - (tileHeight(spr.picnum) << 7),
				spr.sectnum)) {
			switch (spr.detail) {
			case KOBOLDTYPE:
			case DEVILTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SKELETONTYPE:
			case GRONTYPE:
			case DEMONTYPE:
			case GUARDIANTYPE:
			case WILLOWTYPE:
			case NEWGUYTYPE:
			case KURTTYPE:
			case GONZOTYPE:
			case KATIETYPE:
				monsterangle[n] = getangle(tspritelist[i].x - plr.x, tspritelist[i].y - plr.y);
				monsterlist[n] = i;
				n++;
				break;
			}
		}
	}

	daang = plr.angle.ang.asbuild() - ((numshots * (128 / numshots)) >> 1);
	for (k = 0, s = 0; k < numshots; k++) {
		if (n > 0) {
			auto &spr = tspritelist[monsterlist[s]];
			daang = monsterangle[s];
			shootgunzvel = ((spr.z - (48 << 8) - plr.z) << 8)
					/ ksqrt((spr.x - plr.x) * (spr.x - plr.x) + (spr.y - plr.y) * (spr.y - plr.y));
			s = (short) ((s + 1) % n);
		} else {
			daang += (128 / numshots);
		}
		shootgun(plr, daang, shootguntype);
	}
}


void dophysics(PLAYER& plr, int goalz, int flyupdn, int v) {
	if (plr.orbactive[5] > 0) {
		if (v > 0) {
			auto horiz = plr.horizon.horiz.asbuild();
			if (horiz > 25)
				plr.hvel -= (TICSPERFRAME << 8);
			else if (horiz < -25)
				plr.hvel += (TICSPERFRAME << 8);
		}
		if (flyupdn > 0) {
			plr.hvel -= (TICSPERFRAME << 7);
		}
		if (flyupdn < 0) {
			plr.hvel += (TICSPERFRAME << 7);
		}
		plr.hvel += bsin(PlayClock << 4, -6);
		plr.fallz = 0;

	}
	else if (plr.z < goalz) {
		if (isWh2())
			plr.hvel += (TICSPERFRAME * WH2GRAVITYCONSTANT);
		else
			plr.hvel += GRAVITYCONSTANT;
		plr.onsomething &= ~(GROUNDBIT | PLATFORMBIT);
		plr.fallz += plr.hvel;
	}
	else if (plr.z > goalz) {
		plr.hvel -= ((plr.z - goalz) >> 6);
		plr.onsomething |= GROUNDBIT;
		plr.fallz = 0;
	}
	else {
		plr.fallz = 0;
	}

	plr.z += plr.hvel;
	if (plr.hvel > 0 && plr.z > goalz) {
		plr.hvel >>= 2;
	}
	else if (plr.onsomething != 0) {
		if (plr.hvel < 0 && plr.z < goalz) {
			plr.hvel = 0;
			plr.z = goalz;
		}
	}

	if (plr.sector != -1) {
		if (plr.z - (plr.height >> 2) < getceilzofslope(plr.sector, plr.x, plr.y)) {
			plr.z = getceilzofslope(plr.sector, plr.x, plr.y) + (plr.height >> 2);
			plr.hvel = 0;
		}
		else {
			if (plr.orbactive[5] > 0) {
				if (plr.z + (plr.height << 7) > getflorzofslope(plr.sector, plr.x, plr.y)) {
					plr.z = getflorzofslope(plr.sector, plr.x, plr.y) - (plr.height << 7);
					plr.hvel = 0;
				}
			}
			else {
				if (plr.z + (plr.height >> 4) > getflorzofslope(plr.sector, plr.x, plr.y)) {
					plr.z = getflorzofslope(plr.sector, plr.x, plr.y) - (plr.height >> 4);
					plr.hvel = 0;
				}
			}
		}
	}
	plr.horizon.horizoff = q16horiz(-plr.hvel << 8);
}


END_WH_NS
