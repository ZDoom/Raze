#include "ns.h"
#include "wh.h"

BEGIN_WH_NS


byte flashflag = 0x00;

static const int eg_onyx_effect = 1;

short torchpattern[] = { 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 4, 4, 6, 6, 4, 4, 6, 6, 4, 4, 6, 6, 4, 4, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2 };

// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
int monsterwarptime;

short adjusthp(int hp) {
#if 0
	// this doesn't do much because 'factor' will always be 0 due to integer division
	float factor = (krand() % 20) / 100;
	int howhard = difficulty;

	if (krand() % 100 > 50)
		return (short) ((hp * (factor + 1)) * howhard);
	else
		return (short) ((hp - (hp * (factor))) * howhard);
#else
	return (short)(hp * difficulty);
#endif
}

void timerprocess(PLAYER& plr) {
	if (plr.justwarpedfx > 0) {
		plr.justwarpedfx -= TICSPERFRAME;
		plr.justwarpedcnt += TICSPERFRAME << 6;

		if (plr.justwarpedfx <= 0)
			plr.justwarpedcnt = 0;
	}

	if (plr.poisoned == 1) {
		if (plr.poisontime >= 0) {
			plr.poisontime -= TICSPERFRAME;

			addhealth(plr, 0);

			if (plr.poisontime < 0) {
				startredflash(50);
				addhealth(plr, -10);
				plr.poisontime = 7200;
			}
		}
	}

	// EG 19 Aug 2017 - Try to prevent monsters teleporting back and forth wildly
	if (monsterwarptime > 0)
		monsterwarptime -= TICSPERFRAME;

	if (plr.vampiretime > 0)
		plr.vampiretime -= TICSPERFRAME;

	if (plr.shockme >= 0)
		plr.shockme -= TICSPERFRAME;

	if (plr.helmettime > 0)
		plr.helmettime -= TICSPERFRAME;

	if (plr.shadowtime >= 0)
		plr.shadowtime -= TICSPERFRAME;

	if (plr.nightglowtime >= 0) {
		plr.nightglowtime -= TICSPERFRAME;
		g_visibility = 256;
		if (plr.nightglowtime < 0)
			g_visibility = 1024;
	}

	if (plr.strongtime >= 0) {
		plr.strongtime -= TICSPERFRAME;
		whitecount = 10;
	}

	if (plr.invisibletime >= 0) {
		plr.invisibletime -= TICSPERFRAME;
	}

	if (plr.invincibletime >= 0) {
		plr.invincibletime -= TICSPERFRAME;
		whitecount = 10;
	}

	if (plr.manatime >= 0) {
		plr.manatime -= TICSPERFRAME;
		redcount = 20;
	}
	
	if(plr.spiked != 0)
		plr.spiketics -= TICSPERFRAME;
}

int getPickHeight() {
	if (isWh2())
		return PICKHEIGHT2;
	else
		return PICKHEIGHT;
}

// see if picked up any objects?
// JSA 4_27 play appropriate sounds pending object picked up

void processobjs(PLAYER& plr) {

	int dh, dx, dy, dz, i, nexti;

	if (plr.sector < 0 || plr.sector >= numsectors)
		return;

	i = headspritesect[plr.sector];
	while (i != -1) {
		nexti = nextspritesect[i];
		dx = abs(plr.x - sprite[i].x); // x distance to sprite
		dy = abs(plr.y - sprite[i].y); // y distance to sprite
		dz = abs((plr.z >> 8) - (sprite[i].z >> 8)); // z distance to sprite
		dh = tileHeight(sprite[i].picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if(isItemSprite(i)) 
				items[(sprite[i].detail & 0xFF) - ITEMSBASE].pickup(plr, (short)i);

			if (sprite[i].picnum >= EXPLOSTART && sprite[i].picnum <= EXPLOEND && sprite[i].owner != sprite[plr.spritenum].owner)
				if (plr.manatime < 1)
					addhealth(plr, -1);
		}
		i = nexti;
	}
}

void newstatus(short sn, const int seq) {
	auto& spr = sprite[sn];
	switch (seq) {
	case AMBUSH:
		changespritestat(sn, AMBUSH);
		break;
	case LAND:
		changespritestat(sn, LAND);
		break;
	case EVILSPIRIT:
		changespritestat(sn, EVILSPIRIT);
		spr.lotag = (short) (120 + (krand() & 64));
		break;
	case PATROL:
		changespritestat(sn, PATROL);
		break;
	case WARPFX:
		changespritestat(sn, WARPFX);
		spr.lotag = 12;
		break;
	case NUKED:
		changespritestat(sn, NUKED);
		if (!isWh2())
			spr.lotag = 24;
		break;
	case BROKENVASE:
		changespritestat(sn, BROKENVASE);
		switch (spr.picnum) {
		case VASEA:
			spritesound(S_GLASSBREAK1 + (krand() % 3), &spr);
			spr.picnum = SHATTERVASE;
			break;
		case VASEB:
			spritesound(S_GLASSBREAK1 + (krand() % 3), &spr);
			spr.picnum = SHATTERVASE2;
			break;
		case VASEC:
			spritesound(S_GLASSBREAK1 + (krand() % 3), &spr);
			spr.picnum = SHATTERVASE3;
			break;
		case STAINGLASS1:
		case STAINGLASS2:
		case STAINGLASS3:
		case STAINGLASS4:
		case STAINGLASS5:
		case STAINGLASS6:
		case STAINGLASS7:
		case STAINGLASS8:
		case STAINGLASS9:
			spr.picnum++;
			SND_Sound(S_BIGGLASSBREAK1 + (krand() % 3));
			break;
		case FBARRELFALL:
		case BARREL:
			spritesound(S_BARRELBREAK, &spr);
			spr.picnum = FSHATTERBARREL;
			break;
		}
		spr.lotag = 12;
		spr.cstat &= ~3;
		break;
	case DRAIN:
		changespritestat(sn, DRAIN);
		spr.lotag = 24;
		spr.pal = 7;
		break;
	case ANIMLEVERDN:
		spritesound(S_PULLCHAIN1, &spr);
		spr.picnum = LEVERUP;
		changespritestat(sn, ANIMLEVERDN);
		spr.lotag = 24;
		break;
	case ANIMLEVERUP:
		spritesound(S_PULLCHAIN1, &spr);
		spr.picnum = LEVERDOWN;
		changespritestat(sn, ANIMLEVERUP);
		spr.lotag = 24;
		break;
	case SKULLPULLCHAIN1:
	case PULLTHECHAIN:
		spritesound(S_PULLCHAIN1, &spr);
		changespritestat(sn, PULLTHECHAIN);
		SND_Sound(S_CHAIN1);
		spr.lotag = 24;
		break;
	case FROZEN:
		// JSA_NEW
		spritesound(S_FREEZE, &spr);
		changespritestat(sn, FROZEN);
		spr.lotag = 3600;
		break;
	case DEVILFIRE:
		changespritestat(sn, DEVILFIRE);
		spr.lotag = (short) (krand() & 120 + 360);
		break;
	case DRIP:
		changespritestat(sn, DRIP);
		break;
	case BLOOD:
		changespritestat(sn, BLOOD);
		break;
	case WAR:
		changespritestat(sn, WAR);
		break;
	case PAIN:
		spr.lotag = 36;
		switch (spr.detail) {
		case DEMONTYPE:
			spr.lotag = 24;
			spritesound(S_GUARDIANPAIN1 + (krand() % 2), &spr);
			spr.picnum = DEMON - 1;
			changespritestat(sn, PAIN);
			break;
		case NEWGUYTYPE:
			spr.lotag = 24;
			spr.picnum = NEWGUYPAIN;
			changespritestat(sn, PAIN);
			spritesound(S_AGM_PAIN1, &spr);
			break;

		case KURTTYPE:
			spr.lotag = 24;
			spr.picnum = GONZOCSWPAIN;
			changespritestat(sn, PAIN);
			spritesound(S_GRONPAINA + (krand() % 3), &spr);
			break;

		case GONZOTYPE:
			spr.lotag = 24;
			switch(spr.picnum)
			{
				case KURTSTAND:
				case KURTKNEE:
				case KURTAT:
				case KURTPUNCH:
				case KURTREADY:
				case KURTREADY + 1:
				case GONZOCSW:
					spr.picnum = GONZOCSWPAIN;
					spritesound(S_GRONPAINA + (krand() % 3), &spr);
					break;
				case GONZOGSW:
					spr.picnum = GONZOGSWPAIN;
					spritesound(S_GRONPAINA + (krand() % 3), &spr);
					break;
				case GONZOGHM:
					spr.picnum = GONZOGHMPAIN;
					spritesound(S_GRONPAINA + (krand() % 3), &spr);
					break;
				case GONZOGSH:
					spr.picnum = GONZOGSHPAIN;
					spritesound(S_GRONPAINA, &spr);
					break;
				default:
					changespritestat(sn, FLEE);
					break;
			}
			changespritestat(sn, PAIN);
			break;
		case KATIETYPE:
			spr.picnum = KATIEPAIN;
			changespritestat(sn, PAIN);
			break;
		case JUDYTYPE:
			spr.lotag = 24;
			spr.picnum = JUDY;
			changespritestat(sn, PAIN);
			break;
		case FATWITCHTYPE:
			spr.lotag = 24;
			spr.picnum = FATWITCHDIE;
			changespritestat(sn, PAIN);
			break;
		case SKULLYTYPE:
			spr.lotag = 24;
			spr.picnum = SKULLYDIE;
			changespritestat(sn, PAIN);
			break;
		case GUARDIANTYPE:
			spr.lotag = 24;
			// spr.picnum=GUARDIANATTACK;
			spritesound(S_GUARDIANPAIN1 + (krand() % 2), &spr);
			
			if(isWh2()) spr.picnum = GUARDIAN;
			else spr.picnum = GUARDIANCHAR;
			changespritestat(sn, PAIN);
			break;
		case GRONTYPE:
			spr.lotag = 24;
			changespritestat(sn, PAIN);
			spritesound(S_GRONPAINA + krand() % 3, &spr);
			
			if(spr.picnum == GRONHAL || spr.picnum == GRONHALATTACK)
				spr.picnum = GRONHALPAIN;
			else if(spr.picnum == GRONSW || spr.picnum == GRONSWATTACK)
				spr.picnum = GRONSWPAIN;
			else if(spr.picnum == GRONMU || spr.picnum == GRONMUATTACK)
				spr.picnum = GRONMUPAIN;
			break;
		case KOBOLDTYPE:
			spr.picnum = KOBOLDDIE;
			changespritestat(sn, PAIN);
			spritesound(S_KPAIN1 + (krand() % 2), &spr);
			break;
		case DEVILTYPE:
			spritesound(S_MPAIN1, &spr);
			spr.picnum = DEVILPAIN;
			changespritestat(sn, PAIN);
			break;
		case FREDTYPE:
			spr.picnum = FREDPAIN;
			changespritestat(sn, PAIN);
			// EG: Sounds for Fred (currently copied from ogre)
			spritesound(S_KPAIN1 + (rand() % 2), &spr);
			break;
		case GOBLINTYPE:
		case IMPTYPE:
			if (isWh2() && (spr.picnum == IMP || spr.picnum == IMPATTACK)) {
				spr.lotag = 24;
				spr.picnum = IMPPAIN;
				changespritestat(sn, PAIN);
			} else {
				spr.picnum = GOBLINPAIN;
				changespritestat(sn, PAIN);
				spritesound(S_GOBPAIN1 + (krand() % 2), &spr);
			}
			break;
		case MINOTAURTYPE:
			spr.picnum = MINOTAURPAIN;
			changespritestat(sn, PAIN);
			spritesound(S_MPAIN1, &spr);
			break;
		default:
			changespritestat(sn, FLEE);
			break;
		}
		break;
	case FLOCKSPAWN:
		spr.lotag = 36;
		spr.extra = 10;
		changespritestat(sn, FLOCKSPAWN);
		break;
	case FLOCK:
		spr.lotag = 128;
		spr.extra = 0;
		spr.pal = 0;
		changespritestat(sn, FLOCK);
		break;
	case FINDME:
		spr.lotag = 360;
		if (spr.picnum == RAT) {
			spr.ang = (short) (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			changespritestat(sn, FLEE);
		} else
			changespritestat(sn, FINDME);
		break;
	case SKIRMISH:
		spr.lotag = 60;
		if (spr.picnum == RAT) {
			spr.ang = (short) (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			changespritestat(sn, FLEE);
		} else
			changespritestat(sn, SKIRMISH);
		break;
	case CHILL:
		spr.lotag = 60;
		changespritestat(sn, CHILL);
		break;
	case WITCHSIT:
		spr.lotag = 12;
		changespritestat(sn, WITCHSIT);
		break;
	case DORMANT:
		spr.lotag = (short) (krand() & 2047 + 2047);
		break;
	case ACTIVE:
		spr.lotag = 360;
		break;
	case FLEE:
		switch (spr.detail) {
		case GONZOTYPE:
			
			switch (spr.picnum) {
			case GONZOCSWAT:
			case KURTSTAND:
			case KURTKNEE:
			case KURTAT:
			case KURTPUNCH:
				spr.picnum = GONZOCSW;
				break;
			case GONZOGSWAT:
				spr.picnum = GONZOGSW;
				break;
			case GONZOGHMAT:
				spr.picnum = GONZOGHM;
				break;
			case GONZOGSHAT:
				spr.picnum = GONZOGSH;
				break;
			}
			break;
		case NEWGUYTYPE:
			spr.picnum = NEWGUY;
			break;
		case KURTTYPE:
			spr.picnum = GONZOCSW;
			break;
		case GRONTYPE:
			if(spr.picnum == GRONHALATTACK)
				spr.picnum = GRONHAL;
			else if(spr.picnum == GRONMUATTACK)
				spr.picnum = GRONMU;
			else if(spr.picnum == GRONSWATTACK)
				spr.picnum = GRONSW;
			break;
		case DEVILTYPE:
			spr.picnum = DEVIL;
			break;
		case KOBOLDTYPE:
			spr.picnum = KOBOLD;
			break;
		case MINOTAURTYPE:
			spr.picnum = MINOTAUR;
			break;
		case SKELETONTYPE:
			spr.picnum = SKELETON;
			break;
		case FREDTYPE:
			spr.picnum = FRED;
			break;
		case GOBLINTYPE:
			spr.picnum = GOBLIN;
			break;
		}

		changespritestat(sn, FLEE);
		if (!isWh2() && spr.picnum == DEVILATTACK && spr.picnum == DEVIL)
			spr.lotag = (short) (120 + (krand() & 360));
		else
			spr.lotag = 60;
		break;
	case BOB:
		changespritestat(sn, BOB);
		break;
	case LIFTUP:
		if (soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, -1, CHAN_CART) == 0) {
			spritesound(S_CLUNK, &spr);
			spritesound(S_CHAIN1, &spr, 5, CHAN_CART);
		}
		changespritestat(sn, LIFTUP);
		break;
	case LIFTDN:
		if (soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, -1, CHAN_CART) == 0) {
			spritesound(S_CLUNK, &spr);
			spritesound(S_CHAIN1, &spr, 5, CHAN_CART);
		}
		changespritestat(sn, LIFTDN);
		break;
	case SHOVE:
		spr.lotag = 128;
		changespritestat(sn, SHOVE);
		break;
	case SHATTER:
		changespritestat(sn, SHATTER);
		switch (spr.picnum) {
		case FBARRELFALL:
			spr.picnum = FSHATTERBARREL;
			break;
		}
		break;
	case YELL:
		changespritestat(sn, YELL);
		spr.lotag = 12;
		break;
	case ATTACK2: //WH1
		if(isWh2()) break;
		spr.lotag = 40;
		spr.cstat |= 1;
		changespritestat(sn, ATTACK2);
		spr.picnum = DRAGONATTACK2;
		spritesound(S_DRAGON1 + (krand() % 3), &spr);
	case ATTACK:
		spr.lotag = 64;
		spr.cstat |= 1;
		changespritestat(sn, ATTACK);
		switch (spr.detail) {
		case NEWGUYTYPE:
			if (spr.extra > 20) {
				spr.picnum = NEWGUYCAST;
				spr.lotag = 24;
			} else if (spr.extra > 10)
				spr.picnum = NEWGUYBOW;
			else if (spr.extra > 0)
				spr.picnum = NEWGUYMACE;
			else
				spr.picnum = NEWGUYPUNCH;
			break;
		case GONZOTYPE:
		case KURTTYPE:
			switch(spr.picnum)
			{
			case GONZOCSW:
				if (spr.extra > 10)
					spr.picnum = GONZOCSWAT;
				else if (spr.extra > 0) {
					spr.picnum = KURTREADY;
					spr.lotag = 12;
				} else
					spr.picnum = KURTPUNCH;
				break;
			case GONZOGSW:
				spr.picnum = GONZOGSWAT;
				break;
			case GONZOGHM:
				spr.picnum = GONZOGHMAT;
				break;
			case GONZOGSH:
				spr.picnum = GONZOGSHAT;
				break;
			}
			break;
		case KATIETYPE:
			if ((krand() % 10) > 4) {
				spritesound(S_JUDY1, &spr);
			}
			spr.picnum = KATIEAT;
			break;
		case DEMONTYPE:
			spritesound(S_GUARDIAN1 + (krand() % 2), &spr);
			spr.picnum = DEMON;
			break;
		case GRONTYPE:
			if(spr.picnum == GRONHAL)
				spr.picnum = GRONHALATTACK;
			else if(spr.picnum == GRONMU)
				spr.picnum = GRONMUATTACK;
			else if(spr.picnum == GRONSW)
				spr.picnum = GRONSWATTACK;
			break;
		case KOBOLDTYPE:
			spr.picnum = KOBOLDATTACK;
			if (krand() % 10 > 4)
				spritesound(S_KSNARL1 + (krand() % 4), &spr);
			break;
		case DRAGONTYPE:
			if ((krand() % 10) > 3)
				spritesound(S_DRAGON1 + (krand() % 2), &spr);

			spr.picnum = DRAGONATTACK;
			break;
		case DEVILTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_DEMON1 + (krand() % 5), &spr);

			spr.picnum = DEVILATTACK;
			break;
		case FREDTYPE:
			spr.picnum = FREDATTACK;
			/* EG: Sounds for Fred (currently copied from Ogre) */
			if (rand() % 10 > 4)
				spritesound(S_KSNARL1 + (rand() % 4), &spr);
			break;
		case SKELETONTYPE:
			spr.picnum = SKELETONATTACK;
			break;
		case IMPTYPE:
			spr.lotag = 92;
			if ((krand() % 10) > 5)
				spritesound(S_IMPGROWL1 + (krand() % 3), &spr);
			spr.picnum = IMPATTACK;
			break;	
		case GOBLINTYPE:
			if ((krand() % 10) > 5)
				spritesound(S_GOBLIN1 + (krand() % 3), &spr);
			spr.picnum = GOBLINATTACK;
			break;
		case MINOTAURTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_MSNARL1 + (krand() % 3), &spr);

			spr.picnum = MINOTAURATTACK;
			break;
		case SKULLYTYPE:
			spr.picnum = SKULLYATTACK;
			break;
		case FATWITCHTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_FATLAUGH, &spr);
			spr.picnum = FATWITCHATTACK;
			break;
		case JUDYTYPE:
			// spr.cstat=0;
			if (krand() % 2 == 0)
				spr.picnum = JUDYATTACK1;
			else
				spr.picnum = JUDYATTACK2;
			break;
		case WILLOWTYPE:
			spritesound(S_WISP + (krand() % 2), &spr);
			spr.pal = 7;
			break;
		case GUARDIANTYPE:
			spritesound(S_GUARDIAN1 + (krand() % 2), &spr);
			spr.picnum = GUARDIANATTACK;
			break;
		}
		break;
	case FACE:
		changespritestat(sn, FACE);
		break;
	case STAND:
		changespritestat(sn, FACE);
		spr.lotag = 0;
		break;
	case CHASE:
		if (spr.picnum == RAT)
			changespritestat(sn, FLEE);
		else
			changespritestat(sn, CHASE);
		spr.lotag = 256;
		switch (spr.detail) {
		case NEWGUYTYPE:
			spr.picnum = NEWGUY;
			break;
		case KATIETYPE:
			spr.picnum = KATIE;
			break;
		case DEMONTYPE:
			spr.picnum = DEMON;
			break;
		case KURTTYPE:
			spr.picnum = GONZOCSW;
			break;
		case GONZOTYPE:
			switch (spr.picnum) {
			case GONZOCSWAT:
			case KURTSTAND:
			case KURTKNEE:
			case KURTAT:
			case KURTPUNCH:
				spr.picnum = GONZOCSW;
				break;
			case GONZOGSWAT:
				spr.picnum = GONZOGSW;
				break;
			case GONZOGHMAT:
				spr.picnum = GONZOGHM;
				break;
			case GONZOGSHAT:
				spr.picnum = GONZOGSH;
				break;
			}
			break;
		case GRONTYPE:
			if(spr.picnum == GRONHALATTACK) {
				if (spr.extra > 2)
					spr.picnum = GRONHAL;
				else
					spr.picnum = GRONMU;
			}
			else if(spr.picnum == GRONSWATTACK)
				spr.picnum = GRONSW;
			else if(spr.picnum == GRONMUATTACK) {
				if (spr.extra > 0)
					spr.picnum = GRONMU;
				else
					spr.picnum = GRONSW;
			}
			break;
		case KOBOLDTYPE:
			spr.picnum = KOBOLD;
			break;
		case DRAGONTYPE:
			spr.picnum = DRAGON;
			break;
		case DEVILTYPE:
			spr.picnum = DEVIL;
			break;
		case FREDTYPE:
			spr.picnum = FRED;
			break;
		case SKELETONTYPE:
			spr.picnum = SKELETON;
			break;
		case GOBLINTYPE:
		case IMPTYPE:
			if (isWh2() && spr.picnum == IMPATTACK) {
				if (krand() % 10 > 2)
					spr.picnum = IMP;
			} else {
				if (krand() % 10 > 2)
					spritesound(S_GOBLIN1 + (krand() % 3), &spr);

				spr.picnum = GOBLIN;
			}
			break;
		case MINOTAURTYPE:
			// JSA_DEMO3
			spritesound(S_MSNARL1 + (krand() % 4), &spr);
			spr.picnum = MINOTAUR;
			break;
		case SKULLYTYPE:
			spr.picnum = SKULLY;
			break;
		case FATWITCHTYPE:
			spr.picnum = FATWITCH;
			break;
		case JUDYTYPE:
			spr.picnum = JUDY;
			break;
		case GUARDIANTYPE:
			spr.picnum = GUARDIAN;
			break;
		case WILLOWTYPE:
			spr.pal = 6;
			break;
		}
		break;
	case MISSILE:
		changespritestat(sn, MISSILE);
		break;
	case CAST:
		changespritestat(sn, CAST);
		spr.lotag = 12;
		
		if(spr.picnum == GRONHALATTACK 
				|| spr.picnum == GONZOCSWAT 
				|| spr.picnum == NEWGUY)
			spr.lotag = 24;
		else if(spr.picnum == GRONMUATTACK)
			spr.lotag = 36;
		break;
	case FX:
		changespritestat(sn, FX);
		break;
	case DIE:
		if(spr.statnum == DIE || spr.statnum == DEAD) //already dying
			break;
		
		if(spr.detail != GONZOTYPE || spr.shade != 31)
			spr.cstat &= ~3;
		else spr.cstat &= ~1;
		switch (spr.detail) {
		case NEWGUYTYPE:
			spr.lotag = 20;
			spr.picnum = NEWGUYDIE;
			spritesound(S_AGM_DIE1 + (krand() % 3), &spr);
			break;
		case KURTTYPE:
		case GONZOTYPE:
			spr.lotag = 20;
			spritesound(S_GRONDEATHA + krand() % 3, &spr);
			switch (spr.picnum) {
			case KURTSTAND:
			case KURTKNEE:
			case KURTAT:
			case KURTPUNCH:
			case KURTREADY:
			case KURTREADY + 1:
			case GONZOCSW:
			case GONZOCSWAT:
			case GONZOCSWPAIN:
				spr.picnum = GONZOCSWPAIN;
				break;
			case GONZOGSW:
			case GONZOGSWAT:
			case GONZOGSWPAIN:
				spr.picnum = GONZOGSWPAIN;
				break;
			case GONZOGHM:
			case GONZOGHMAT:
			case GONZOGHMPAIN:
				spr.picnum = GONZOGHMPAIN;
				break;
			case GONZOGSH:
			case GONZOGSHAT:
			case GONZOGSHPAIN:
				spr.picnum = GONZOGSHPAIN;
				break;
			case GONZOBSHPAIN:
				spr.picnum = GONZOBSHPAIN;
				if (spr.shade > 30) {
					trailingsmoke(sn, false);
					deletesprite((short) sn);
					return;
				}
				break;
			default:
				spr.lotag = 20;
				spr.picnum = GONZOGSWPAIN;
				//System.err.println("die error " + spr.picnum);
				return;
			}
			break;
		case KATIETYPE:
			spritesound(S_JUDYDIE, &spr);
			spr.lotag = 20;
			spr.picnum = KATIEPAIN;
			break;
		case DEMONTYPE:
			spritesound(S_GUARDIANDIE, &spr);
			explosion(sn, spr.x, spr.y, spr.z, spr.owner);
			deletesprite((short) sn);
			addscore(aiGetPlayerTarget(sn), 1500);
			kills++;
			return;
		case GRONTYPE:
			spr.lotag = 20;
			spritesound(S_GRONDEATHA + krand() % 3, &spr);
			if(spr.picnum == GRONHAL || spr.picnum == GRONHALATTACK || spr.picnum == GRONHALPAIN)
				spr.picnum = GRONHALDIE;
			else if(spr.picnum == GRONSW || spr.picnum == GRONSWATTACK || spr.picnum == GRONSWPAIN)
				spr.picnum = GRONSWDIE;
			else if(spr.picnum == GRONMU || spr.picnum == GRONMUATTACK || spr.picnum == GRONMUPAIN)
				spr.picnum = GRONMUDIE;
			else {
				//System.err.println("error gron" + spr.picnum);
				spr.picnum = GRONDIE;
			}
			break;
		case FISHTYPE:
		case RATTYPE:
			spr.lotag = 20;
			break;
		case KOBOLDTYPE:
			spritesound(S_KDIE1 + (krand() % 2), &spr);
			spr.lotag = 20;
			spr.picnum = KOBOLDDIE;
			break;
		case DRAGONTYPE:
			spritesound(S_DEMONDIE1 + (krand() % 2), &spr);
			spr.lotag = 20;
			spr.picnum = DRAGONDIE;

			break;
		case DEVILTYPE:
			spritesound(S_DEMONDIE1 + (krand() % 2), &spr);
			spr.lotag = 20;
			spr.picnum = DEVILDIE;
			break;
		case FREDTYPE:
			spr.lotag = 20;
			spr.picnum = FREDDIE;
			/* EG: Sounds for Fred (currently copied from Ogre) */
			spritesound(S_KDIE1 + (rand() % 2), &spr);
			break;
		case SKELETONTYPE:
			spritesound(S_SKELETONDIE, &spr);
			spr.lotag = 20;
			spr.picnum = SKELETONDIE;
			break;
		case IMPTYPE:
			spritesound(S_IMPDIE1 + (krand() % 2), &spr);
			spr.lotag = 20;
			spr.picnum = IMPDIE;
			break;
		case GOBLINTYPE:
			spritesound(S_GOBDIE1 + (krand() % 3), &spr);
			spr.lotag = 20;
			spr.picnum = GOBLINDIE;
			break;
		case MINOTAURTYPE:
			spritesound(S_MDEATH1, &spr);
			spr.lotag = 10;
			spr.picnum = MINOTAURDIE;
			break;
		case SPIDERTYPE:
			spr.lotag = 10;
			spr.picnum = SPIDERDIE;
			break;
		case SKULLYTYPE:
			spr.lotag = 20;
			spr.picnum = SKULLYDIE;
			spritesound(S_SKULLWITCHDIE, &spr);
			break;
		case FATWITCHTYPE:
			spr.lotag = 20;
			spr.picnum = FATWITCHDIE;
			spritesound(S_FATWITCHDIE, &spr);
			break;
		case JUDYTYPE:
			spr.lotag = 20;
			if (mapon < 24) {
				for (int j = 0; j < 8; j++)
					trailingsmoke(sn, true);
				deletesprite((short) sn);
				return;
			} else {
				spr.picnum = JUDYDIE;
				spritesound(S_JUDYDIE, &spr);
			}
			break;
		case GUARDIANTYPE:
			spritesound(S_GUARDIANDIE, &spr);
			for (int j = 0; j < 4; j++)
				explosion(sn, spr.x, spr.y, spr.z, spr.owner);
			deletesprite((short) sn);
			addscore(aiGetPlayerTarget(sn), 1500);
			kills++;
			return;
		case WILLOWTYPE:
			spritesound(S_WILLOWDIE, &spr);
			spr.pal = 0;
			spr.lotag = 20;
			spr.picnum = WILLOWEXPLO;
			break;
		}
		changespritestat(sn, DIE);
		break;

	case RESURECT:
		spr.lotag = 7200;
		switch (spr.picnum) {
		case GONZOBSHDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 85);
			spr.detail = GONZOTYPE;
			break;
		case NEWGUYDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 55);
			spr.detail = NEWGUYTYPE;
			break;
		case GONZOCSWDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 55);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGSWDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 105);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGHMDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 100);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGSHDEAD:
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 110);
			spr.detail = GONZOTYPE;
			break;
		case KATIEDEAD:
			trailingsmoke(sn, true);
			spr.picnum = KATIEDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			spawnhornskull(sn);
			addscore(aiGetPlayerTarget(sn), 5000);
			spr.detail = KATIETYPE;
			break;
		case DEVILDEAD:
			trailingsmoke(sn, true);
			spr.picnum = DEVILDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 70);
			spr.detail = DEVILTYPE;
			break;	
		case IMPDEAD:
			spr.picnum = IMPDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 115);
			spr.detail = IMPTYPE;
			break;
		case KOBOLDDEAD:
			spr.picnum = KOBOLDDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			spr.detail = KOBOLDTYPE;
			if(isWh2()) {
				switch (spr.pal) {
				   case 0:
					   addscore(aiGetPlayerTarget(sn), 25);
						break;
				   case 7:
					   addscore(aiGetPlayerTarget(sn), 40);
						break;
				   }
				addscore(aiGetPlayerTarget(sn), 10);
				break;
			}
			
			
			addscore(aiGetPlayerTarget(sn), 10);
			break;
		case DRAGONDEAD:
			spr.picnum = DRAGONDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 4000);
			spr.detail = DRAGONTYPE;
			break;
		case FREDDEAD:
			spr.picnum = FREDDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 40);
			spr.detail = FREDTYPE;
			break;
		case GOBLINDEAD:
			spr.picnum = GOBLINDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 25);
			spr.detail = GOBLINTYPE;
			break;
		case MINOTAURDEAD:
			spr.picnum = MINOTAURDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), isWh2() ? 95 : 170);
			spr.detail = MINOTAURTYPE;
			break;
		case SPIDERDEAD:
			spr.picnum = SPIDERDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 5);
			spr.detail = SPIDERTYPE;
			break;
		case SKULLYDEAD:
			spr.picnum = SKULLYDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 1000);
			spr.detail = SKULLYTYPE;
			break;
		case FATWITCHDEAD:
			spr.picnum = FATWITCHDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 900);
			spr.detail = FATWITCHTYPE;
			break;
		case JUDYDEAD:
			spr.picnum = JUDYDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, RESURECT);
			addscore(aiGetPlayerTarget(sn), 7000);
			spr.detail = JUDYTYPE;
			break;
		default:
			if(spr.picnum == SKELETONDEAD) {
				spr.picnum = SKELETONDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, RESURECT);
				spr.detail = SKELETONTYPE;
				addscore(aiGetPlayerTarget(sn), isWh2() ? 20 : 10);
			} else if(spr.picnum == GRONDEAD) {
				spr.picnum = GRONDEAD;
				spr.cstat &= ~3;
				spr.extra = 3;
				spr.detail = GRONTYPE;
				changespritestat(sn, RESURECT);
				if(isWh2()) {
					switch (spr.pal) {
					   case 0:
						   addscore(aiGetPlayerTarget(sn),125);
							break;
					   case 10:
						   addscore(aiGetPlayerTarget(sn),90);
							break;
					   case 11:
						   addscore(aiGetPlayerTarget(sn),115);
							break;
					   case 12:
						   addscore(aiGetPlayerTarget(sn),65);
							break;
					   }
					} else addscore(aiGetPlayerTarget(sn), 200);
			}
			break;
		}
		break;

	case DEAD:
		spr.detail = 0;
		if(spr.picnum == SKELETONDEAD) {
			spr.picnum = SKELETONDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, DEAD);
			if(isWh2()) {
				 addscore(aiGetPlayerTarget(sn), 70);
				monsterweapon(sn);
			}
		} else if(spr.picnum == FISH || spr.picnum == RAT) {
			spr.cstat &= ~3;
			changespritestat(sn, DEAD);
			addscore(aiGetPlayerTarget(sn), 5);
		} else if(spr.picnum == GRONDEAD) {
			spr.picnum = GRONDEAD;
			spr.cstat &= ~3;
			changespritestat(sn, DEAD);
			if(isWh2()) {
				switch (spr.pal) {
				   case 0:
					   addscore(aiGetPlayerTarget(sn), 125);
						break;
				   case 10:
					   addscore(aiGetPlayerTarget(sn), 90);
						break;
				   case 11:
					   addscore(aiGetPlayerTarget(sn), 115);
						break;
				   case 12:
					   addscore(aiGetPlayerTarget(sn), 65);
						break;
				   }
			} else {
				addscore(aiGetPlayerTarget(sn), 200);
			}
			monsterweapon(sn);
		} else {
			switch (spr.picnum) {
			case GONZOBSHDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOBSHDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				if (spr.pal == 4) {
					changespritestat(sn, SHADE);
					deaddude(sn);
				} else {
					changespritestat(sn, DEAD);
					if (spr.shade < 25)
						monsterweapon(sn);
				}
				addscore(aiGetPlayerTarget(sn), 85);
				break;
			case GONZOCSWDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOCSWDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					changespritestat(sn, SHADE);
					deaddude(sn);
				} else {
					changespritestat(sn, DEAD);
					monsterweapon(sn);
				}
				addscore(aiGetPlayerTarget(sn), 55);
				break;
			case GONZOGSWDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOGSWDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					changespritestat(sn, SHADE);
					deaddude(sn);
				} else {
					changespritestat(sn, DEAD);
					monsterweapon(sn);
				}
				addscore(aiGetPlayerTarget(sn), 105);
				break;
			case GONZOGHMDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOGHMDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					changespritestat(sn, SHADE);
					deaddude(sn);
				} else {
					changespritestat(sn, DEAD);
					monsterweapon(sn);
				}
				addscore(aiGetPlayerTarget(sn), 100);
				break;
			case NEWGUYDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = NEWGUYDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				monsterweapon(sn);
				addscore(aiGetPlayerTarget(sn), 50);
				break;
			case GONZOGSHDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOGSHDEAD;
				if(spr.shade != 31)
					spr.cstat &= ~3;
				else spr.cstat &= ~1;
				if (spr.pal == 4) {
					changespritestat(sn, SHADE);
					deaddude(sn);
				} else {
					changespritestat(sn, DEAD);
					monsterweapon(sn);
				}
				addscore(aiGetPlayerTarget(sn), 110);
				break;
			case KATIEDEAD:
				if (netgame) {
					break;
				}
				trailingsmoke(sn, true);
				spr.picnum = DEVILDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				spawnhornskull(sn);
				addscore(aiGetPlayerTarget(sn), 500);
				break;
			case IMPDEAD:
				if (!isWh2())
					break;
				spr.picnum = IMPDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 115);
				monsterweapon(sn);
				break;
			case KOBOLDDEAD:
				spr.picnum = KOBOLDDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 10);
				break;
			case DRAGONDEAD:
				spr.picnum = DRAGONDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 4000);
				break;
			case DEVILDEAD:
				trailingsmoke(sn, true);
				spr.picnum = DEVILDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), isWh2() ? 70 : 50);
				if(isWh2())
					 monsterweapon(sn);
				break;
			case FREDDEAD:
				spr.picnum = FREDDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 40);
				break;
			case GOBLINDEAD:
				spr.picnum = GOBLINDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 25);
				if ((rand() % 100) > 60)
					monsterweapon(sn);
				break;
			case MINOTAURDEAD:
				spr.picnum = MINOTAURDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), isWh2() ? 95 : 70);
				if ((rand() % 100) > 60)
					monsterweapon(sn);
				break;
			case SPIDERDEAD:
				spr.picnum = SPIDERDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 5);
				break;
			case SKULLYDEAD:
				spr.picnum = SKULLYDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 100);
				break;
			case FATWITCHDEAD:
				spr.picnum = FATWITCHDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				addscore(aiGetPlayerTarget(sn), 900);
				break;
			case JUDYDEAD:
				spr.picnum = JUDYDEAD;
				spr.cstat &= ~3;
				changespritestat(sn, DEAD);
				spawnapentagram(sn);
				addscore(aiGetPlayerTarget(sn), 7000);
				break;
			case WILLOWEXPLO + 2:
				spr.pal = 0;
				spr.cstat &= ~3;
				changespritestat(sn, (short) 0);
				deletesprite(sn);
				addscore(aiGetPlayerTarget(sn), isWh2() ? 15 : 150);
				return;
			}
		}

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum,
				(spr.clipdist) << 2, CLIPMASK0);
		spr.z = zr_florz;

		if ((zr_florhit & kHitTypeMask) == kHitSector) {
			if (spr.sectnum != MAXSECTORS && (sector[spr.sectnum].floorpicnum == WATER
					|| sector[spr.sectnum].floorpicnum == SLIME)) {
				if (spr.picnum == MINOTAURDEAD) {
					spr.z += (8 << 8);
					setsprite(sn, spr.x, spr.y, spr.z);
				}
			}
			if (spr.sectnum != MAXSECTORS && (sector[spr.sectnum].floorpicnum == LAVA
					|| sector[spr.sectnum].floorpicnum == LAVA1
					|| sector[spr.sectnum].floorpicnum == LAVA2)) {
				trailingsmoke(sn, true);
				deletesprite((short) sn);
			}
		}
		break;
	}
	//
	// the control variable for monster release
	//
}

void makeafire(int i, int firetype) {
	short j = insertsprite(sprite[i].sectnum, FIRE);
	auto& spawned = sprite[j];
	
	spawned.x = sprite[i].x + (krand() & 1024) - 512;
	spawned.y = sprite[i].y + (krand() & 1024) - 512;
	spawned.z = sprite[i].z;

	spawned.cstat = 0;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;

	spawned.shade = 0;

	spawned.clipdist = 64;
	spawned.owner = sprite[i].owner;
	spawned.lotag = 2047;
	spawned.hitag = 0;
	changespritestat(j, FIRE);
}

void explosion(int i, int x, int y, int z, int owner) {
	int j = insertsprite(sprite[i].sectnum, EXPLO);
	auto& spawned = sprite[j];

	boolean isWH2 = isWh2();
	
	if(!isWH2) {
		spawned.x = x + (krand() & 1024) - 512;
		spawned.y = y + (krand() & 1024) - 512;
		spawned.z = z;
	} else {
		spawned.x = x;
		spawned.y = y;
		spawned.z = z + (16 << 8);
	}

	spawned.cstat = 0; // Hitscan does not hit smoke on wall
	spawned.cstat &= ~3;
	spawned.shade = -15;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawned.ang = (short) (krand() & 2047);
	spawned.xvel = (short) ((krand() & 511) - 256);
	spawned.yvel = (short) ((krand() & 511) - 256);
	spawned.zvel = (short) ((krand() & 511) - 256);
	spawned.owner = sprite[i].owner;
	spawned.hitag = 0;
	spawned.pal = 0;
	if(!isWH2) {
		spawned.picnum = MONSTERBALL;
		spawned.lotag = 256;
	} else {
		spawned.picnum = EXPLOSTART;
		spawned.lotag = 12;
	}
}

void explosion2(int i, int x, int y, int z, int owner) {
	int j = insertsprite(sprite[i].sectnum, EXPLO);
	auto& spawned = sprite[j];

	boolean isWH2 = isWh2();
	
	if(!isWH2) {
		spawned.x = x + (krand() & 256) - 128;
		spawned.y = y + (krand() & 256) - 128;
		spawned.z = z;
	} else {
		spawned.x = x;
		spawned.y = y;
		spawned.z = z + (16 << 8);
	}
	
	spawned.cstat = 0;
	spawned.cstat &= ~3;
	
	spawned.shade = -25;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawned.ang = (short) (krand() & 2047);
	spawned.xvel = (short) ((krand() & 256) - 128);
	spawned.yvel = (short) ((krand() & 256) - 128);
	spawned.zvel = (short) ((krand() & 256) - 128);
	spawned.owner = sprite[i].owner;
	spawned.hitag = 0;
	spawned.pal = 0;
	
	if(!isWH2) {
		spawned.picnum = MONSTERBALL;
		spawned.lotag = 128;
	} else {
		spawned.picnum = EXPLOSTART;
		spawned.lotag = 12;
	}
	
}

void trailingsmoke(int i, boolean ball) {
	int j = insertsprite(sprite[i].sectnum, SMOKE);
	auto& spawned = sprite[j];

	spawned.x = sprite[i].x;
	spawned.y = sprite[i].y;
	spawned.z = sprite[i].z;
	
	spawned.cstat = 0x03;
	spawned.cstat &= ~3;
	spawned.picnum = SMOKEFX;
	spawned.shade = 0;
	if (ball) {
		spawned.xrepeat = 128;
		spawned.yrepeat = 128;
	} else {
		spawned.xrepeat = 32;
		spawned.yrepeat = 32;
	}
	spawned.pal = 0;

	spawned.owner = sprite[i].owner;
	spawned.lotag = 256;
	spawned.hitag = 0;
}

void icecubes(int i, int x, int y, int z, int owner) {
	int j = insertsprite(sprite[i].sectnum, FX);

	sprite[j].x = x;
	sprite[j].y = y;

	sprite[j].z = sector[sprite[i].sectnum].floorz - (getPlayerHeight() << 8) + (krand() & 4096);

	sprite[j].cstat = 0; // Hitscan does not hit smoke on wall
	sprite[j].picnum = ICECUBE;
	sprite[j].shade = -16;
	sprite[j].xrepeat = 16;
	sprite[j].yrepeat = 16;

	sprite[j].ang = (short) (((krand() & 1023) - 1024) & 2047);
	sprite[j].xvel = (short) ((krand() & 1023) - 512);
	sprite[j].yvel = (short) ((krand() & 1023) - 512);
	sprite[j].zvel = (short) ((krand() & 1023) - 512);

	sprite[j].pal = 6;
	sprite[j].owner = sprite[i].owner;
	
	if(isWh2())
		sprite[j].lotag = 2048;
	else sprite[j].lotag = 999;
	sprite[j].hitag = 0;

}

boolean damageactor(PLAYER& plr, int hitobject, short i) {
	short j = (short) (hitobject & 4095); // j is the spritenum that the bullet (spritenum i) hit
	if (j == plr.spritenum && sprite[i].owner == sprite[plr.spritenum].owner)
		return false;

	if (j == plr.spritenum && sprite[i].owner != sprite[plr.spritenum].owner) {
		if (plr.invincibletime > 0 || plr.godMode) {
			deletesprite(i);
			return false;
		}
		// EG 17 Oct 2017: Mass backport of RULES.CFG behavior for resist/onyx ring
		// EG 21 Aug 2017: New RULES.CFG behavior in place of the old #ifdef
		if (plr.manatime > 0) {
			if (/* eg_resist_blocks_traps && */sprite[i].picnum != FATSPANK && sprite[i].picnum != PLASMA) {
				deletesprite(i);
				return false;
			}
			// Use "fixed" version: EXPLOSION and MONSTERBALL are the fire attacks, account
			// for animation
			// FATSPANK is right in the middle of this group of tiles, so still keep an
			// exception for it.
			else if ((sprite[i].picnum >= EXPLOSION && sprite[i].picnum <= (MONSTERBALL + 2)
					&& sprite[i].picnum != FATSPANK)) {
				deletesprite(i);
				return false;
			}
		}
		// EG 21 Aug 2017: onyx ring
		else if (plr.treasure[TONYXRING] == 1 /*&& eg_onyx_effect != 0*/
				&& ((sprite[i].picnum < EXPLOSION || sprite[i].picnum > MONSTERBALL + 2)
						&& sprite[i].picnum != PLASMA)) {
			
//				if (eg_onyx_effect == 1 || (eg_onyx_effect == 2 && ((krand() & 32) > 16))) {
				deletesprite(i);
				return false;
//				}
		}
		// EG 21 Aug 2017: Move this here so as not to make ouch sounds unless pain is
		// happening
		if ((krand() & 9) == 0)
			spritesound(S_PLRPAIN1 + (rand() % 2), &sprite[i]);

		if (isWh2() && sprite[i].picnum == DART) {
			plr.poisoned = 1;
			plr.poisontime = 7200;
			showmessage("Poisoned", 360);
		}
		 
		if (netgame) {
//						netdamageactor(j,i);
		} else {
			if (sprite[i].picnum == PLASMA)
				addhealth(plr, -((krand() & 15) + 15));
			else if (sprite[i].picnum == FATSPANK) {
				spritesound(S_GORE1A + (krand() % 3),  &sprite[plr.spritenum]);
				addhealth(plr, -((krand() & 10) + 10));
				if ((krand() % 100) > 90) {
					plr.poisoned = 1;
					plr.poisontime = 7200;
					showmessage("Poisoned", 360);
				}
			} else if (sprite[i].picnum == THROWPIKE) {
				addhealth(plr, -((krand() % 10) + 5));
			} else
				addhealth(plr, -((krand() & 20) + 5));
		}

		startredflash(10);
		/* EG 2017 - Trap fix */
		deletesprite(i);
		return true;
	}

	if (j != plr.spritenum && !netgame) // Les 08/11/95
		if (sprite[i].owner != j) {
			
//				final int DEMONTYPE = 1; XXX
//				final int DRAGONTYPE = 3;
//				final int FISHTYPE = 5;
//				final int JUDYTYPE = 12;
//				final int RATTYPE = 18;
//				final int SKULLYTYPE = 20;
			
			switch (sprite[j].detail) {
			case NEWGUYTYPE:
			case KURTTYPE:
			case GONZOTYPE:
			case KATIETYPE:
			case GRONTYPE:
			case KOBOLDTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case GOBLINTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SPIDERTYPE:
			case SKELETONTYPE:
			case FATWITCHTYPE:
			case WILLOWTYPE:
			case GUARDIANTYPE:
				if(isBlades(sprite[i].picnum)) {
					sprite[j].hitag -= 30;
					if(sprite[i].picnum == THROWPIKE) {
						if ((krand() % 2) != 0)
							spritesound(S_GORE1A + krand() % 2, &sprite[i]);
					}
				} else {
				switch (sprite[i].picnum) {
					case PLASMA:
						sprite[j].hitag -= 40;
						break;
					case FATSPANK:
						sprite[j].hitag -= 10;
						break;
					case MONSTERBALL:
						sprite[j].hitag -= 40;
						break;
					case FIREBALL:
						if(!isWh2())
							sprite[j].hitag -= 3;
						break;
					case BULLET:
						sprite[j].hitag -= 10;
						break;
					case DISTORTIONBLAST:
						sprite[j].hitag = 10;
						break;
					case BARREL:
						sprite[j].hitag -= 100;
						break;
					}
				}
				
				if (sprite[j].hitag <= 0) {
					newstatus(j, DIE);
					deletesprite(i);
				} else
					newstatus(j, PAIN);
				return true;
			}
			
			switch(sprite[j].detail) {
			case GONZOTYPE:
			case NEWGUYTYPE:
			case KATIETYPE:
			case GRONTYPE:
			case KOBOLDTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case GOBLINTYPE:
			case MINOTAURTYPE:
			case SPIDERTYPE:
			case SKELETONTYPE:
			case IMPTYPE:
				// JSA_NEW //why is this here it's in whplr
				// raf because monsters could shatter a guy thats been frozen
				if (sprite[j].pal == 6) {
					for (int k = 0; k < 32; k++)
						icecubes(j, sprite[j].x, sprite[j].y, sprite[j].z, j);
					// EG 26 Oct 2017: Move this here from medusa (anti multi-freeze exploit)
					addscore(&plr, 100);
					deletesprite(j);
				}
				return true;
			}
			
			switch (sprite[j].picnum) {
			case BARREL:
			case VASEA:
			case VASEB:
			case VASEC:
			case STAINGLASS1:
			case STAINGLASS2:
			case STAINGLASS3:
			case STAINGLASS4:
			case STAINGLASS5:
			case STAINGLASS6:
			case STAINGLASS7:
			case STAINGLASS8:
			case STAINGLASS9:
				sprite[j].hitag = 0;
				sprite[j].lotag = 0;
				newstatus(j, BROKENVASE);
				break;
			default:
				deletesprite(i);
				return true;
			}
		}

	return false;
}

int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, int cliptype) {

	int zoffs;
	int retval;
	int tempshort, dasectnum;

	SPRITE& spr = sprite[spritenum];
	if (spr.statnum == MAXSTATUS)
		return (-1);

	int dcliptype = 0;
	switch (cliptype) {
	case NORMALCLIP:
		dcliptype = CLIPMASK0;
		break;
	case PROJECTILECLIP:
		dcliptype = CLIPMASK1;
		break;
	case CLIFFCLIP:
		dcliptype = CLIPMASK0;
		break;
	}

	if ((spr.cstat & 128) == 0)
		zoffs = -((tileHeight(spr.picnum) * spr.yrepeat) << 1);
	else
		zoffs = 0;

	dasectnum = spr.sectnum; // Can't modify sprite sectors directly becuase of linked lists

	auto pos = spr.pos;
	pos.z += zoffs; // Must do this if not using the new centered centering (of course)
	retval = clipmove(&pos, &dasectnum, dx, dy, (spr.clipdist) << 2, ceildist, flordist, dcliptype);

	pushmove(&pos, &dasectnum, spr.clipdist << 2, ceildist, flordist, CLIPMASK0);
	spr.x = pos.x;
	spr.y = pos.y;
	
	if ((dasectnum != spr.sectnum) && (dasectnum >= 0))
		changespritesect(spritenum, dasectnum);

	// Set the blocking bit to 0 temporarly so getzrange doesn't pick up
	// its own sprite
	tempshort = spr.cstat;
	spr.cstat &= ~1;
	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, dcliptype);
	spr.cstat = tempshort;

	int daz = spr.z + zoffs + dz;
	if ((daz <= zr_ceilz) || (daz > zr_florz)) {
		if (retval != 0)
			return (retval);
		return (16384 | dasectnum);
	}
	spr.z = (daz - zoffs);
	return (retval);
}

void trowajavlin(int s) {
	int j = insertsprite(sprite[s].sectnum, JAVLIN);

	sprite[j].x = sprite[s].x;
	sprite[j].y = sprite[s].y;
	sprite[j].z = sprite[s].z;// - (40 << 8);
	
	sprite[j].cstat = 21;

	switch (sprite[s].lotag) {
	case 91:
		sprite[j].picnum = WALLARROW;
		sprite[j].ang = (short) (((sprite[s].ang + 2048) - 512) & 2047);
		sprite[j].xrepeat = 16;
		sprite[j].yrepeat = 48;
		sprite[j].clipdist = 24;
		break;
	case 92:
		sprite[j].picnum = DART;
		sprite[j].ang = (short) (((sprite[s].ang + 2048) - 512) & 2047);
		sprite[j].xrepeat = 64;
		sprite[j].yrepeat = 64;
		sprite[j].clipdist = 16;
		break;
	case 93:
		sprite[j].picnum = HORIZSPIKEBLADE;
		sprite[j].ang = (short) (((sprite[s].ang + 2048) - 512) & 2047);
		sprite[j].xrepeat = 16;
		sprite[j].yrepeat = 48;
		sprite[j].clipdist = 32;
		break;
	case 94:
		sprite[j].picnum = THROWPIKE;
		sprite[j].ang = (short) (((sprite[s].ang + 2048) - 512) & 2047);
		sprite[j].xrepeat = 24;
		sprite[j].yrepeat = 24;
		sprite[j].clipdist = 32;
		break;
	}

	sprite[j].extra = sprite[s].ang;
	sprite[j].shade = -15;
	sprite[j].xvel = (short) ((krand() & 256) - 128);
	sprite[j].yvel = (short) ((krand() & 256) - 128);
	sprite[j].zvel = (short) ((krand() & 256) - 128);

	sprite[j].owner = 0;
	sprite[j].lotag = 0;
	sprite[j].hitag = 0;
	sprite[j].pal = 0;
}

void spawnhornskull(short i) {
	short j = insertsprite(sprite[i].sectnum, (short) 0);
	sprite[j].x = sprite[i].x;
	sprite[j].y = sprite[i].y;
	sprite[j].z = sprite[i].z - (24 << 8);
	
	sprite[j].shade = -15;
	sprite[j].cstat = 0;
	sprite[j].cstat &= ~3;
	sprite[j].pal = 0;
	sprite[j].picnum = HORNEDSKULL;
	sprite[j].detail = HORNEDSKULLTYPE;
	sprite[j].xrepeat = 64;
	sprite[j].yrepeat = 64;
}

void spawnapentagram(int sn) {
	short j = insertsprite(sprite[sn].sectnum, (short) 0);

	sprite[j].x = sprite[sn].x;
	sprite[j].y = sprite[sn].y;
	sprite[j].z = sprite[sn].z - (8 << 8);
	
	sprite[j].xrepeat = sprite[j].yrepeat = 64;
	sprite[j].pal = 0;
	sprite[j].shade = -15;
	sprite[j].cstat = 0;
	sprite[j].clipdist = 64;
	sprite[j].lotag = 0;
	sprite[j].hitag = 0;
	sprite[j].extra = 0;
	sprite[j].picnum = PENTAGRAM;
	sprite[j].detail = PENTAGRAMTYPE;

	setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
}

END_WH_NS