#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

void activatedaorb(PLAYER& plr) {
	if (plr.orbammo[plr.currentorb] <= 0)
		return;

	switch (plr.currentorb) {
	case 0: // SCARE
		// shadowtime=1200+(plr.lvl*120);
		break;
	case 1: // NIGHT VISION
		// nightglowtime=2400+(plr.lvl*600);
		break;
	case 2: // FREEZE
		plr.orbactive[plr.currentorb] = -1;
		break;
	case 3: // MAGIC ARROW
		plr.orbactive[plr.currentorb] = -1;
		break;
	case 4: // OPEN DOORS
		plr.orbactive[plr.currentorb] = -1;
		break;
	case 5: // FLY
		// plr.orbactive[currentorb]=3600+(plr.lvl*600);
		break;
	case 6: // FIREBALL
		plr.orbactive[plr.currentorb] = -1;
		break;
	case 7: // NUKE
		plr.orbactive[plr.currentorb] = -1;
		break;
	}

	if (plr.orbammo[plr.currentorb] <= 0) {
		plr.orb[plr.currentorb] = 0;
		return;
	} else
		plr.orbammo[plr.currentorb]--;

	plr.currweaponfired = 4;
	plr.currweapontics = isWh2() ? wh2throwanimtics[plr.currentorb][0].daweapontics : throwanimtics[plr.currentorb][0].daweapontics;
}

void castaorb(PLAYER& plr) {
	int k;
	float daang;

	switch (plr.currentorb) {
	case 0: // SCARE
		if (isWh2())
			spritesound(S_GENERALMAGIC4, plr.actor());
		plr.shadowtime = ((plr.lvl + 1) * 120) << 2;
		break;
	case 1: // NIGHTVISION
		plr.nightglowtime = 3600 + (plr.lvl * 120);
		break;
	case 2: // FREEZE
		if (isWh2())
			spritesound(S_GENERALMAGIC3, plr.actor());
		else
			spritesound(S_SPELL1, plr.actor());
		daang = plr.angle.ang.asbuild();
		shootgun(plr, daang, 6);
		break;
	case 3: // MAGIC ARROW
		if (isWh2()) {
			lockon(plr,10,2);
			spritesound(S_GENERALMAGIC2, plr.actor());
		}
		else {
			daang = (float)BClampAngle(plr.angle.ang.asbuild() - 36);
			for (k = 0; k < 10; k++) {
				daang = (float)BClampAngle(int(daang) + (k << 1));
				shootgun(plr, daang, 2);
			}
			spritesound(S_SPELL1, plr.actor());
		}
		break;
	case 4: // OPEN DOORS
		daang = plr.angle.ang.asbuild();
		shootgun(plr, daang, 7);
		if (isWh2())
			spritesound(S_DOORSPELL, plr.actor());
		else
			spritesound(S_SPELL1, plr.actor());
		break;
	case 5: // FLY
		plr.orbactive[plr.currentorb] = 3600 + (plr.lvl * 120);
		if (isWh2())
			spritesound(S_GENERALMAGIC1, plr.actor());
		else
			spritesound(S_SPELL1, plr.actor());
		break;
	case 6: // FIREBALL
		if (isWh2()) {
			lockon(plr,3,3);
			spritesound(S_FIRESPELL, plr.actor());
		}
		else {
			daang = plr.angle.ang.asbuild();
			shootgun(plr, daang, 3);
			spritesound(S_SPELL1, plr.actor());
		}
		break;
	case 7: // NUKE
		daang = plr.angle.ang.asbuild();
		shootgun(plr, daang, 4);
		if (isWh2())
			spritesound(S_NUKESPELL, plr.actor());
		else
			spritesound(S_SPELL1, plr.actor());
		break;
	}
}
	
void spellswitch(PLAYER& plr, int j)
{
	int i = plr.currentorb;
	while(i >= 0 && i < MAXNUMORBS) {
		i += j;
			
		if(i == -1) i = MAXNUMORBS - 1;
        else if(i == MAXNUMORBS) i = 0;
			
		if(plr.spellbookflip != 0 || i == plr.currentorb)
			break;
			
		if (changebook(plr, i)) {
			displayspelltext(plr);
			plr.spelltime = 360;
			break;
		} 
	}
}

void bookprocess(int snum) {
	PLAYER& plr = player[snum];

	int spell = plr.spellnum;
	plr.spellnum = -1;

	if (plr.currweaponanim == 0 && plr.currweaponflip == 0) {
		if(spell != -1 && spell < 10)
		{
			if(spell != 9 && spell != 8) {
				if (changebook(plr, spell)) {
					displayspelltext(plr);
					plr.spelltime = 360;
				} else return;
			} else 
				spellswitch(plr, spell == 9 ? -1 : 1);
			plr.orbshot = 0;
		}
	}

	for (int j = 0; j < MAXNUMORBS; j++) {
		if (plr.orbactive[j] > -1) {
			plr.orbactive[j] -= TICSPERFRAME;
		}
	}
}

boolean changebook(PLAYER& plr, int i) {
	if(plr.orbammo[i] <= 0 || plr.currentorb == i)
		return false;
	plr.currentorb = i;
	if (plr.spellbookflip == 0) {
		plr.spellbook = 0;
		plr.spellbooktics = 10;
		plr.spellbookflip = 1;
		SND_Sound(S_PAGE);
		return true;
	}
	return false;
}

boolean lvlspellcheck(PLAYER& plr) {
	if(isWh2()) return true;
		
	switch (plr.currentorb) {
	case 0:
	case 1:
		return true;
	case 2:
		if (plr.lvl > 1)
			return true;
		else
			showmessage("must attain 2nd level", 360);
		break;
	case 3:
		if (plr.lvl > 1)
			return true;
		else
			showmessage("must attain 2nd level", 360);
		break;
	case 4:
		if (plr.lvl > 2)
			return true;
		else
			showmessage("must attain 3rd level", 360);
		break;
	case 5:
		if (plr.lvl > 2)
			return true;
		else
			showmessage("must attain 3rd level", 360);

		break;
	case 6:
		if (plr.lvl > 3)
			return true;
		else
			showmessage("must attain 4th level", 360);
		break;
	case 7:
		if (plr.lvl > 4)
			return true;
		else
			showmessage("must attain 5th level", 360);
		break;
	}
	return false;
}

void speelbookprocess(PLAYER& plr) {
	if (plr.spelltime > 0)
		plr.spelltime -= TICSPERFRAME;

	if (plr.spellbookflip == 1) {
		plr.spellbooktics -= TICSPERFRAME;
		if (plr.spellbooktics < 0)
			plr.spellbook++;
		if (plr.spellbook > 8)
			plr.spellbook = 8;
		if (plr.spellbook == 8)
			plr.spellbookflip = 0;
	}
}

void nukespell(PLAYER& plr, DWHActor* actor) {
	auto& spr = actor->s();

	if(spr.detail != WILLOWTYPE && spr.pal == 6) //don't nuke freezed enemies
		return;

	if (isWh2()) {
		// dont nuke a shade
		if (spr.shade > 30)
			return;

		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		spr.cstat &= ~3;
		spr.shade = 6;
		spr.lotag = 360;
		spr.ang = plr.angle.ang.asbuild();
		spr.hitag = 0;
		addscore(&plr, 150);

		auto spawnedactor = InsertActor(spr.sectnum, NUKED);
		auto& spawned = spawnedactor->s();

		spawned.lotag = 360;
		spawned.xrepeat = 30;
		spawned.yrepeat = 12;
		spawned.picnum = ZFIRE;
		spawned.pal = 0;
		spawned.ang = spr.ang;
		spawned.x = spr.x;
		spawned.y = spr.y;
		spawned.z = spr.z;
		spawned.cstat = spr.cstat;
		spawned.backuploc();

		return;
	}

	switch (spr.detail) {
	case WILLOWTYPE:
	case SPIDERTYPE:
		DeleteActor(actor);
		addscore(&plr, 10);
		break;
	case KOBOLDTYPE:
		spr.picnum = KOBOLDCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case DEVILTYPE:
		spr.picnum = DEVILCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case GOBLINTYPE:
	case IMPTYPE:
		spr.picnum = GOBLINCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case MINOTAURTYPE:
		spr.picnum = MINOTAURCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case SKELETONTYPE:
		spr.picnum = SKELETONCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case GRONTYPE:
		spr.picnum = GRONCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case DRAGONTYPE:
		spr.picnum = DRAGONCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case GUARDIANTYPE:
		spr.picnum = GUARDIANCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case FATWITCHTYPE:
		spr.picnum = FATWITCHCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case SKULLYTYPE:
		spr.picnum = SKULLYCHAR;
		SetNewStatus(actor, NUKED);
		spr.pal = 0;
		spr.cstat |= 1;
		addscore(&plr, 150);
		break;
	case JUDYTYPE:
		if (mapon < 24) {
			spr.picnum = JUDYCHAR;
			SetNewStatus(actor, NUKED);
			spr.pal = 0;
			spr.cstat |= 1;
			addscore(&plr, 150);
		}
		break;
	}
}

void medusa(PLAYER& plr, DWHActor* actor) {
	auto& spr = actor->s();

	if(spr.hitag <= 0) //don't freeze dead enemies
		return;
		
	SetNewStatus(actor, FROZEN);
	int pic = spr.picnum;
	switch (spr.detail) {

	case NEWGUYTYPE:
		spr.picnum = NEWGUYPAIN;
		break;
	case KURTTYPE:
		spr.picnum = GONZOCSWPAIN;
		break;	
	case GONZOTYPE:
		if(pic == GONZOCSW || pic == GONZOCSWAT)
			spr.picnum = GONZOCSWPAIN;
		else if(pic == GONZOGSW || pic == GONZOGSWAT)
			spr.picnum = GONZOGSWPAIN;
		else if(pic == GONZOGHM || pic == GONZOGHMAT 
				|| pic == GONZOGSH || pic == GONZOGSHAT)
			spr.picnum = GONZOGHMPAIN;
		break;
	case KATIETYPE:
		spr.picnum = KATIEPAIN;
		break;	
	case KOBOLDTYPE:
		spr.picnum = KOBOLDDIE;
		break;
	case DEVILTYPE:
		spr.picnum = DEVILDIE;
		break;
	case FREDTYPE:
		spr.picnum = FREDDIE;
		break;
	case GOBLINTYPE:
	case IMPTYPE:
		if(isWh2()) spr.picnum = IMPDIE;
		else spr.picnum = GOBLINDIE;
		break;	
	case MINOTAURTYPE:
		spr.picnum = MINOTAURDIE;
		break;
	case SPIDERTYPE:
		spr.picnum = SPIDERDIE;
		break;
	case SKELETONTYPE:
		spr.picnum = SKELETONDIE;
		break;
	case GRONTYPE:
		if(pic == GRONHAL || pic == GRONHALATTACK)
			spr.picnum = (short) GRONHALDIE;
		else if(pic == GRONMU || pic == GRONMUATTACK)
			spr.picnum = (short) GRONMUDIE;
		else if(pic == GRONSW || pic == GRONSWATTACK)
			spr.picnum = (short) GRONSWDIE;
		break;
	}
	spr.pal = 6;
	spr.cstat |= 1;
	addscore(&plr, 100);
}

void displayspelltext(PLAYER& plr) {
	switch (plr.currentorb) {
	case 0:
		showmessage("scare spell", 360);
		break;
	case 1:
		showmessage("night vision spell", 360);
		break;
	case 2:
		showmessage("freeze spell", 360);
		break;
	case 3:
		showmessage("magic arrow spell", 360);
		break;
	case 4:
		showmessage("open door spell", 360);
		break;
	case 5:
		showmessage("fly spell", 360);
		break;
	case 6:
		showmessage("fireball spell", 360);
		break;
	case 7:
		showmessage("nuke spell", 360);
		break;
	}
}

END_WH_NS
