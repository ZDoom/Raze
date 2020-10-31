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
			spritesound(S_GENERALMAGIC4, &sprite[plr.spritenum]);
		plr.shadowtime = ((plr.lvl + 1) * 120) << 2;
		break;
	case 1: // NIGHTVISION
		plr.nightglowtime = 3600 + (plr.lvl * 120);
		break;
	case 2: // FREEZE
		if (isWh2())
			spritesound(S_GENERALMAGIC3, &sprite[plr.spritenum]);
		else
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
		daang = plr.ang;
		shootgun(plr, daang, 6);
		break;
	case 3: // MAGIC ARROW
		if (isWh2()) {
			lockon(plr,10,2);
			spritesound(S_GENERALMAGIC2, &sprite[plr.spritenum]);
		}
		else {
			daang = BClampAngle(plr.ang - 36);
			for (k = 0; k < 10; k++) {
				daang = BClampAngle(daang + (k << 1));
				shootgun(plr, daang, 2);
			}
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
		}
		break;
	case 4: // OPEN DOORS
		daang = plr.ang;
		shootgun(plr, daang, 7);
		if (isWh2())
			spritesound(S_DOORSPELL, &sprite[plr.spritenum]);
		else
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
		break;
	case 5: // FLY
		plr.orbactive[plr.currentorb] = 3600 + (plr.lvl * 120);
		if (isWh2())
			spritesound(S_GENERALMAGIC1, &sprite[plr.spritenum]);
		else
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
		break;
	case 6: // FIREBALL
		if (isWh2()) {
			lockon(plr,3,3);
			spritesound(S_FIRESPELL, &sprite[plr.spritenum]);
		}
		else {
			daang = plr.ang;
			shootgun(plr, daang, 3);
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
		}
		break;
	case 7: // NUKE
		daang = plr.ang;
		shootgun(plr, daang, 4);
		if (isWh2())
			spritesound(S_NUKESPELL, &sprite[plr.spritenum]);
		else
			spritesound(S_SPELL1, &sprite[plr.spritenum]);
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

void nukespell(PLAYER& plr, short j) {
	if(sprite[j].detail != WILLOWTYPE && sprite[j].pal == 6) //don't nuke freezed enemies
		return;

	if (isWh2()) {
		// dont nuke a shade
		if (sprite[j].shade > 30)
			return;

		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		sprite[j].cstat &= ~3;
		sprite[j].shade = 6;
		sprite[j].lotag = 360;
		sprite[j].ang = (short) plr.ang;
		sprite[j].hitag = 0;
		addscore(&plr, 150);

		int k = insertsprite(sprite[j].sectnum, NUKED);
		sprite[k].lotag = 360;
		sprite[k].xrepeat = 30;
		sprite[k].yrepeat = 12;
		sprite[k].picnum = ZFIRE;
		sprite[k].pal = 0;
		sprite[k].ang = sprite[j].ang;
		sprite[k].x = sprite[j].x;
		sprite[k].y = sprite[j].y;
		sprite[k].z = sprite[j].z;
		sprite[k].cstat = sprite[j].cstat;

		return;
	}

	switch (sprite[j].detail) {
	case WILLOWTYPE:
	case SPIDERTYPE:
		deletesprite((short) j);
		addscore(&plr, 10);
		break;
	case KOBOLDTYPE:
		sprite[j].picnum = KOBOLDCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case DEVILTYPE:
		sprite[j].picnum = DEVILCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case GOBLINTYPE:
	case IMPTYPE:
		sprite[j].picnum = GOBLINCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case MINOTAURTYPE:
		sprite[j].picnum = MINOTAURCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case SKELETONTYPE:
		sprite[j].picnum = SKELETONCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case GRONTYPE:
		sprite[j].picnum = GRONCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case DRAGONTYPE:
		sprite[j].picnum = DRAGONCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case GUARDIANTYPE:
		sprite[j].picnum = GUARDIANCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case FATWITCHTYPE:
		sprite[j].picnum = FATWITCHCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case SKULLYTYPE:
		sprite[j].picnum = SKULLYCHAR;
		newstatus(j, NUKED);
		sprite[j].pal = 0;
		sprite[j].cstat |= 1;
		addscore(&plr, 150);
		break;
	case JUDYTYPE:
		if (mapon < 24) {
			sprite[j].picnum = JUDYCHAR;
			newstatus(j, NUKED);
			sprite[j].pal = 0;
			sprite[j].cstat |= 1;
			addscore(&plr, 150);
		}
		break;
	}
}

void medusa(PLAYER& plr, short j) {
	if(sprite[j].hitag <= 0) //don't freeze dead enemies
		return;
		
	newstatus(j, FROZEN);
	int pic = sprite[j].picnum;
	switch (sprite[j].detail) {

	case NEWGUYTYPE:
		sprite[j].picnum = NEWGUYPAIN;
		break;
	case KURTTYPE:
		sprite[j].picnum = GONZOCSWPAIN;
		break;	
	case GONZOTYPE:
		if(pic == GONZOCSW || pic == GONZOCSWAT)
			sprite[j].picnum = GONZOCSWPAIN;
		else if(pic == GONZOGSW || pic == GONZOGSWAT)
			sprite[j].picnum = GONZOGSWPAIN;
		else if(pic == GONZOGHM || pic == GONZOGHMAT 
				|| pic == GONZOGSH || pic == GONZOGSHAT)
			sprite[j].picnum = GONZOGHMPAIN;
		break;
	case KATIETYPE:
		sprite[j].picnum = KATIEPAIN;
		break;	
	case KOBOLDTYPE:
		sprite[j].picnum = KOBOLDDIE;
		break;
	case DEVILTYPE:
		sprite[j].picnum = DEVILDIE;
		break;
	case FREDTYPE:
		sprite[j].picnum = FREDDIE;
		break;
	case GOBLINTYPE:
	case IMPTYPE:
		if(isWh2()) sprite[j].picnum = IMPDIE;
		else sprite[j].picnum = GOBLINDIE;
		break;	
	case MINOTAURTYPE:
		sprite[j].picnum = MINOTAURDIE;
		break;
	case SPIDERTYPE:
		sprite[j].picnum = SPIDERDIE;
		break;
	case SKELETONTYPE:
		sprite[j].picnum = SKELETONDIE;
		break;
	case GRONTYPE:
		if(pic == GRONHAL || pic == GRONHALATTACK)
			sprite[j].picnum = (short) GRONHALDIE;
		else if(pic == GRONMU || pic == GRONMUATTACK)
			sprite[j].picnum = (short) GRONMUDIE;
		else if(pic == GRONSW || pic == GRONSWATTACK)
			sprite[j].picnum = (short) GRONSWDIE;
		break;
	}
	sprite[j].pal = 6;
	sprite[j].cstat |= 1;
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
