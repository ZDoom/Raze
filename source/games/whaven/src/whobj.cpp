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
		return  ((hp * (factor + 1)) * howhard);
	else
		return  ((hp - (hp * (factor))) * howhard);
#else
	return (hp * difficulty);
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

	int dh, dx, dy, dz;

	if (plr.sector < 0 || plr.sector >= numsectors)
		return;

	WHSectIterator it(plr.sector);
	while (auto actor = it.Next())
	{
		SPRITE& tspr = actor->s();

		dx = abs(plr.x - tspr.x); // x distance to sprite
		dy = abs(plr.y - tspr.y); // y distance to sprite
		dz = abs((plr.z >> 8) - (tspr.z >> 8)); // z distance to sprite
		dh = tileHeight(tspr.picnum) >> 1; // height of sprite
		if (dx + dy < PICKDISTANCE && dz - dh <= getPickHeight()) {
			if(isItemSprite(actor)) 
				items[(tspr.detail & 0xFF) - ITEMSBASE].pickup(plr, actor);

			if (tspr.picnum >= EXPLOSTART && tspr.picnum <= EXPLOEND && actor->GetPlayerOwner() != plr.playerNum())
				if (plr.manatime < 1)
					addhealth(plr, -1);
		}
	}
}

void SetNewStatus(DWHActor* actor, const int seq)
{
	auto& spr = actor->s();
	switch (seq) {
	case AMBUSH:
		ChangeActorStat(actor, AMBUSH);
		break;
	case LAND:
		ChangeActorStat(actor, LAND);
		break;
	case EVILSPIRIT:
		ChangeActorStat(actor, EVILSPIRIT);
		spr.lotag =  (120 + (krand() & 64));
		break;
	case PATROL:
		ChangeActorStat(actor, PATROL);
		break;
	case WARPFX:
		ChangeActorStat(actor, WARPFX);
		spr.lotag = 12;
		break;
	case NUKED:
		ChangeActorStat(actor, NUKED);
		if (!isWh2())
			spr.lotag = 24;
		break;
	case BROKENVASE:
		ChangeActorStat(actor, BROKENVASE);
		switch (spr.picnum) {
		case VASEA:
			spritesound(S_GLASSBREAK1 + (krand() % 3), actor);
			spr.picnum = SHATTERVASE;
			break;
		case VASEB:
			spritesound(S_GLASSBREAK1 + (krand() % 3), actor);
			spr.picnum = SHATTERVASE2;
			break;
		case VASEC:
			spritesound(S_GLASSBREAK1 + (krand() % 3), actor);
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
			spritesound(S_BARRELBREAK, actor);
			spr.picnum = FSHATTERBARREL;
			break;
		}
		spr.lotag = 12;
		spr.cstat &= ~3;
		break;
	case DRAIN:
		ChangeActorStat(actor, DRAIN);
		spr.lotag = 24;
		spr.pal = 7;
		break;
	case ANIMLEVERDN:
		spritesound(S_PULLCHAIN1, actor);
		spr.picnum = LEVERUP;
		ChangeActorStat(actor, ANIMLEVERDN);
		spr.lotag = 24;
		break;
	case ANIMLEVERUP:
		spritesound(S_PULLCHAIN1, actor);
		spr.picnum = LEVERDOWN;
		ChangeActorStat(actor, ANIMLEVERUP);
		spr.lotag = 24;
		break;
	case SKULLPULLCHAIN1:
	case PULLTHECHAIN:
		spritesound(S_PULLCHAIN1, actor);
		ChangeActorStat(actor, PULLTHECHAIN);
		SND_Sound(S_CHAIN1);
		spr.lotag = 24;
		break;
	case FROZEN:
		// JSA_NEW
		spritesound(S_FREEZE, actor);
		ChangeActorStat(actor, FROZEN);
		spr.lotag = 3600;
		break;
	case DEVILFIRE:
		ChangeActorStat(actor, DEVILFIRE);
		spr.lotag =  (krand() & 120 + 360);
		break;
	case DRIP:
		ChangeActorStat(actor, DRIP);
		break;
	case BLOOD:
		ChangeActorStat(actor, BLOOD);
		break;
	case WAR:
		ChangeActorStat(actor, WAR);
		break;
	case PAIN:
		spr.lotag = 36;
		switch (spr.detail) {
		case DEMONTYPE:
			spr.lotag = 24;
			spritesound(S_GUARDIANPAIN1 + (krand() % 2), actor);
			spr.picnum = DEMON - 1;
			ChangeActorStat(actor, PAIN);
			break;
		case NEWGUYTYPE:
			spr.lotag = 24;
			spr.picnum = NEWGUYPAIN;
			ChangeActorStat(actor, PAIN);
			spritesound(S_AGM_PAIN1, actor);
			break;

		case KURTTYPE:
			spr.lotag = 24;
			spr.picnum = GONZOCSWPAIN;
			ChangeActorStat(actor, PAIN);
			spritesound(S_GRONPAINA + (krand() % 3), actor);
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
					spritesound(S_GRONPAINA + (krand() % 3), actor);
					break;
				case GONZOGSW:
					spr.picnum = GONZOGSWPAIN;
					spritesound(S_GRONPAINA + (krand() % 3), actor);
					break;
				case GONZOGHM:
					spr.picnum = GONZOGHMPAIN;
					spritesound(S_GRONPAINA + (krand() % 3), actor);
					break;
				case GONZOGSH:
					spr.picnum = GONZOGSHPAIN;
					spritesound(S_GRONPAINA, actor);
					break;
				default:
					ChangeActorStat(actor, FLEE);
					break;
			}
			ChangeActorStat(actor, PAIN);
			break;
		case KATIETYPE:
			spr.picnum = KATIEPAIN;
			ChangeActorStat(actor, PAIN);
			break;
		case JUDYTYPE:
			spr.lotag = 24;
			spr.picnum = JUDY;
			ChangeActorStat(actor, PAIN);
			break;
		case FATWITCHTYPE:
			spr.lotag = 24;
			spr.picnum = FATWITCHDIE;
			ChangeActorStat(actor, PAIN);
			break;
		case SKULLYTYPE:
			spr.lotag = 24;
			spr.picnum = SKULLYDIE;
			ChangeActorStat(actor, PAIN);
			break;
		case GUARDIANTYPE:
			spr.lotag = 24;
			// spr.picnum=GUARDIANATTACK;
			spritesound(S_GUARDIANPAIN1 + (krand() % 2), actor);
			
			if(isWh2()) spr.picnum = GUARDIAN;
			else spr.picnum = GUARDIANCHAR;
			ChangeActorStat(actor, PAIN);
			break;
		case GRONTYPE:
			spr.lotag = 24;
			ChangeActorStat(actor, PAIN);
			spritesound(S_GRONPAINA + krand() % 3, actor);
			
			if(spr.picnum == GRONHAL || spr.picnum == GRONHALATTACK)
				spr.picnum = GRONHALPAIN;
			else if(spr.picnum == GRONSW || spr.picnum == GRONSWATTACK)
				spr.picnum = GRONSWPAIN;
			else if(spr.picnum == GRONMU || spr.picnum == GRONMUATTACK)
				spr.picnum = GRONMUPAIN;
			break;
		case KOBOLDTYPE:
			spr.picnum = KOBOLDDIE;
			ChangeActorStat(actor, PAIN);
			spritesound(S_KPAIN1 + (krand() % 2), actor);
			break;
		case DEVILTYPE:
			spritesound(S_MPAIN1, actor);
			spr.picnum = DEVILPAIN;
			ChangeActorStat(actor, PAIN);
			break;
		case FREDTYPE:
			spr.picnum = FREDPAIN;
			ChangeActorStat(actor, PAIN);
			// EG: Sounds for Fred (currently copied from ogre)
			spritesound(S_KPAIN1 + (rand() % 2), actor);
			break;
		case GOBLINTYPE:
		case IMPTYPE:
			if (isWh2() && (spr.picnum == IMP || spr.picnum == IMPATTACK)) {
				spr.lotag = 24;
				spr.picnum = IMPPAIN;
				ChangeActorStat(actor, PAIN);
			} else {
				spr.picnum = GOBLINPAIN;
				ChangeActorStat(actor, PAIN);
				spritesound(S_GOBPAIN1 + (krand() % 2), actor);
			}
			break;
		case MINOTAURTYPE:
			spr.picnum = MINOTAURPAIN;
			ChangeActorStat(actor, PAIN);
			spritesound(S_MPAIN1, actor);
			break;
		default:
			ChangeActorStat(actor, FLEE);
			break;
		}
		break;
	case FLOCKSPAWN:
		spr.lotag = 36;
		spr.extra = 10;
		ChangeActorStat(actor, FLOCKSPAWN);
		break;
	case FLOCK:
		spr.lotag = 128;
		spr.extra = 0;
		spr.pal = 0;
		ChangeActorStat(actor, FLOCK);
		break;
	case FINDME:
		spr.lotag = 360;
		if (spr.picnum == RAT) {
			spr.ang =  (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			ChangeActorStat(actor, FLEE);
		} else
			ChangeActorStat(actor, FINDME);
		break;
	case SKIRMISH:
		spr.lotag = 60;
		if (spr.picnum == RAT) {
			spr.ang =  (((krand() & 512 - 256) + spr.ang + 1024) & 2047); // NEW
			ChangeActorStat(actor, FLEE);
		} else
			ChangeActorStat(actor, SKIRMISH);
		break;
	case CHILL:
		spr.lotag = 60;
		ChangeActorStat(actor, CHILL);
		break;
	case WITCHSIT:
		spr.lotag = 12;
		ChangeActorStat(actor, WITCHSIT);
		break;
	case DORMANT:
		spr.lotag =  (krand() & 2047 + 2047);
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

		ChangeActorStat(actor, FLEE);
		if (!isWh2() && spr.picnum == DEVILATTACK && spr.picnum == DEVIL)
			spr.lotag =  (120 + (krand() & 360));
		else
			spr.lotag = 60;
		break;
	case BOB:
		ChangeActorStat(actor, BOB);
		break;
	case LIFTUP:
		if (soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, -1, CHAN_CART) == 0) {
			spritesound(S_CLUNK, actor);
			spritesound(S_CHAIN1, actor, 5, CHAN_CART);
		}
		ChangeActorStat(actor, LIFTUP);
		break;
	case LIFTDN:
		if (soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, -1, CHAN_CART) == 0) {
			spritesound(S_CLUNK, actor);
			spritesound(S_CHAIN1, actor, 5, CHAN_CART);
		}
		ChangeActorStat(actor, LIFTDN);
		break;
	case SHOVE:
		spr.lotag = 128;
		ChangeActorStat(actor, SHOVE);
		break;
	case SHATTER:
		ChangeActorStat(actor, SHATTER);
		switch (spr.picnum) {
		case FBARRELFALL:
			spr.picnum = FSHATTERBARREL;
			break;
		}
		break;
	case YELL:
		ChangeActorStat(actor, YELL);
		spr.lotag = 12;
		break;
	case ATTACK2: //WH1
		if(isWh2()) break;
		spr.lotag = 40;
		spr.cstat |= 1;
		ChangeActorStat(actor, ATTACK2);
		spr.picnum = DRAGONATTACK2;
		spritesound(S_DRAGON1 + (krand() % 3), actor);
	case ATTACK:
		spr.lotag = 64;
		spr.cstat |= 1;
		ChangeActorStat(actor, ATTACK);
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
				spritesound(S_JUDY1, actor);
			}
			spr.picnum = KATIEAT;
			break;
		case DEMONTYPE:
			spritesound(S_GUARDIAN1 + (krand() % 2), actor);
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
				spritesound(S_KSNARL1 + (krand() % 4), actor);
			break;
		case DRAGONTYPE:
			if ((krand() % 10) > 3)
				spritesound(S_DRAGON1 + (krand() % 2), actor);

			spr.picnum = DRAGONATTACK;
			break;
		case DEVILTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_DEMON1 + (krand() % 5), actor);

			spr.picnum = DEVILATTACK;
			break;
		case FREDTYPE:
			spr.picnum = FREDATTACK;
			/* EG: Sounds for Fred (currently copied from Ogre) */
			if (rand() % 10 > 4)
				spritesound(S_KSNARL1 + (rand() % 4), actor);
			break;
		case SKELETONTYPE:
			spr.picnum = SKELETONATTACK;
			break;
		case IMPTYPE:
			spr.lotag = 92;
			if ((krand() % 10) > 5)
				spritesound(S_IMPGROWL1 + (krand() % 3), actor);
			spr.picnum = IMPATTACK;
			break;	
		case GOBLINTYPE:
			if ((krand() % 10) > 5)
				spritesound(S_GOBLIN1 + (krand() % 3), actor);
			spr.picnum = GOBLINATTACK;
			break;
		case MINOTAURTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_MSNARL1 + (krand() % 3), actor);

			spr.picnum = MINOTAURATTACK;
			break;
		case SKULLYTYPE:
			spr.picnum = SKULLYATTACK;
			break;
		case FATWITCHTYPE:
			if ((krand() % 10) > 4)
				spritesound(S_FATLAUGH, actor);
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
			spritesound(S_WISP + (krand() % 2), actor);
			spr.pal = 7;
			break;
		case GUARDIANTYPE:
			spritesound(S_GUARDIAN1 + (krand() % 2), actor);
			spr.picnum = GUARDIANATTACK;
			break;
		}
		break;
	case FACE:
		ChangeActorStat(actor, FACE);
		break;
	case STAND:
		ChangeActorStat(actor, FACE);
		spr.lotag = 0;
		break;
	case CHASE:
		if (spr.picnum == RAT)
			ChangeActorStat(actor, FLEE);
		else
			ChangeActorStat(actor, CHASE);
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
					spritesound(S_GOBLIN1 + (krand() % 3), actor);

				spr.picnum = GOBLIN;
			}
			break;
		case MINOTAURTYPE:
			// JSA_DEMO3
			spritesound(S_MSNARL1 + (krand() % 4), actor);
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
		ChangeActorStat(actor, MISSILE);
		break;
	case CAST:
		ChangeActorStat(actor, CAST);
		spr.lotag = 12;
		
		if(spr.picnum == GRONHALATTACK 
				|| spr.picnum == GONZOCSWAT 
				|| spr.picnum == NEWGUY)
			spr.lotag = 24;
		else if(spr.picnum == GRONMUATTACK)
			spr.lotag = 36;
		break;
	case FX:
		ChangeActorStat(actor, FX);
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
			spritesound(S_AGM_DIE1 + (krand() % 3), actor);
			break;
		case KURTTYPE:
		case GONZOTYPE:
			spr.lotag = 20;
			spritesound(S_GRONDEATHA + krand() % 3, actor);
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
					trailingsmoke(actor, false);
					DeleteActor(actor);
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
			spritesound(S_JUDYDIE, actor);
			spr.lotag = 20;
			spr.picnum = KATIEPAIN;
			break;
		case DEMONTYPE:
			spritesound(S_GUARDIANDIE, actor);
			explosion(actor, spr.x, spr.y, spr.z, 0);
			addscore(aiGetPlayerTarget(actor), 1500);
			DeleteActor(actor);
			kills++;
			return;
		case GRONTYPE:
			spr.lotag = 20;
			spritesound(S_GRONDEATHA + krand() % 3, actor);
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
			spritesound(S_KDIE1 + (krand() % 2), actor);
			spr.lotag = 20;
			spr.picnum = KOBOLDDIE;
			break;
		case DRAGONTYPE:
			spritesound(S_DEMONDIE1 + (krand() % 2), actor);
			spr.lotag = 20;
			spr.picnum = DRAGONDIE;

			break;
		case DEVILTYPE:
			spritesound(S_DEMONDIE1 + (krand() % 2), actor);
			spr.lotag = 20;
			spr.picnum = DEVILDIE;
			break;
		case FREDTYPE:
			spr.lotag = 20;
			spr.picnum = FREDDIE;
			/* EG: Sounds for Fred (currently copied from Ogre) */
			spritesound(S_KDIE1 + (rand() % 2), actor);
			break;
		case SKELETONTYPE:
			spritesound(S_SKELETONDIE, actor);
			spr.lotag = 20;
			spr.picnum = SKELETONDIE;
			break;
		case IMPTYPE:
			spritesound(S_IMPDIE1 + (krand() % 2), actor);
			spr.lotag = 20;
			spr.picnum = IMPDIE;
			break;
		case GOBLINTYPE:
			spritesound(S_GOBDIE1 + (krand() % 3), actor);
			spr.lotag = 20;
			spr.picnum = GOBLINDIE;
			break;
		case MINOTAURTYPE:
			spritesound(S_MDEATH1, actor);
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
			spritesound(S_SKULLWITCHDIE, actor);
			break;
		case FATWITCHTYPE:
			spr.lotag = 20;
			spr.picnum = FATWITCHDIE;
			spritesound(S_FATWITCHDIE, actor);
			break;
		case JUDYTYPE:
			spr.lotag = 20;
			if (mapon < 24) {
				for (int j = 0; j < 8; j++)
					trailingsmoke(actor, true);
				DeleteActor(actor);
				return;
			} else {
				spr.picnum = JUDYDIE;
				spritesound(S_JUDYDIE, actor);
			}
			break;
		case GUARDIANTYPE:
			spritesound(S_GUARDIANDIE, actor);
			for (int j = 0; j < 4; j++)
				explosion(actor, spr.x, spr.y, spr.z, 0);
			DeleteActor(actor);
			addscore(aiGetPlayerTarget(actor), 1500);
			kills++;
			return;
		case WILLOWTYPE:
			spritesound(S_WILLOWDIE, actor);
			spr.pal = 0;
			spr.lotag = 20;
			spr.picnum = WILLOWEXPLO;
			break;
		}
		ChangeActorStat(actor, DIE);
		break;

	case RESURECT:
		spr.lotag = 7200;
		switch (spr.picnum) {
		case GONZOBSHDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 85);
			spr.detail = GONZOTYPE;
			break;
		case NEWGUYDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 55);
			spr.detail = NEWGUYTYPE;
			break;
		case GONZOCSWDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 55);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGSWDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 105);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGHMDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 100);
			spr.detail = GONZOTYPE;
			break;
		case GONZOGSHDEAD:
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 110);
			spr.detail = GONZOTYPE;
			break;
		case KATIEDEAD:
			trailingsmoke(actor, true);
			spr.picnum = KATIEDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			spawnhornskull(actor);
			addscore(aiGetPlayerTarget(actor), 5000);
			spr.detail = KATIETYPE;
			break;
		case DEVILDEAD:
			trailingsmoke(actor, true);
			spr.picnum = DEVILDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 70);
			spr.detail = DEVILTYPE;
			break;	
		case IMPDEAD:
			spr.picnum = IMPDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 115);
			spr.detail = IMPTYPE;
			break;
		case KOBOLDDEAD:
			spr.picnum = KOBOLDDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			spr.detail = KOBOLDTYPE;
			if(isWh2()) {
				switch (spr.pal) {
				   case 0:
					   addscore(aiGetPlayerTarget(actor), 25);
						break;
				   case 7:
					   addscore(aiGetPlayerTarget(actor), 40);
						break;
				   }
				addscore(aiGetPlayerTarget(actor), 10);
				break;
			}
			
			
			addscore(aiGetPlayerTarget(actor), 10);
			break;
		case DRAGONDEAD:
			spr.picnum = DRAGONDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 4000);
			spr.detail = DRAGONTYPE;
			break;
		case FREDDEAD:
			spr.picnum = FREDDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 40);
			spr.detail = FREDTYPE;
			break;
		case GOBLINDEAD:
			spr.picnum = GOBLINDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 25);
			spr.detail = GOBLINTYPE;
			break;
		case MINOTAURDEAD:
			spr.picnum = MINOTAURDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), isWh2() ? 95 : 170);
			spr.detail = MINOTAURTYPE;
			break;
		case SPIDERDEAD:
			spr.picnum = SPIDERDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 5);
			spr.detail = SPIDERTYPE;
			break;
		case SKULLYDEAD:
			spr.picnum = SKULLYDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 1000);
			spr.detail = SKULLYTYPE;
			break;
		case FATWITCHDEAD:
			spr.picnum = FATWITCHDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 900);
			spr.detail = FATWITCHTYPE;
			break;
		case JUDYDEAD:
			spr.picnum = JUDYDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, RESURECT);
			addscore(aiGetPlayerTarget(actor), 7000);
			spr.detail = JUDYTYPE;
			break;
		default:
			if(spr.picnum == SKELETONDEAD) {
				spr.picnum = SKELETONDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, RESURECT);
				spr.detail = SKELETONTYPE;
				addscore(aiGetPlayerTarget(actor), isWh2() ? 20 : 10);
			} else if(spr.picnum == GRONDEAD) {
				spr.picnum = GRONDEAD;
				spr.cstat &= ~3;
				spr.extra = 3;
				spr.detail = GRONTYPE;
				ChangeActorStat(actor, RESURECT);
				if(isWh2()) {
					switch (spr.pal) {
					   case 0:
						   addscore(aiGetPlayerTarget(actor),125);
							break;
					   case 10:
						   addscore(aiGetPlayerTarget(actor),90);
							break;
					   case 11:
						   addscore(aiGetPlayerTarget(actor),115);
							break;
					   case 12:
						   addscore(aiGetPlayerTarget(actor),65);
							break;
					   }
					} else addscore(aiGetPlayerTarget(actor), 200);
			}
			break;
		}
		break;

	case DEAD:
		spr.detail = 0;
		if(spr.picnum == SKELETONDEAD) {
			spr.picnum = SKELETONDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, DEAD);
			if(isWh2()) {
				 addscore(aiGetPlayerTarget(actor), 70);
				monsterweapon(actor);
			}
		} else if(spr.picnum == FISH || spr.picnum == RAT) {
			spr.cstat &= ~3;
			ChangeActorStat(actor, DEAD);
			addscore(aiGetPlayerTarget(actor), 5);
		} else if(spr.picnum == GRONDEAD) {
			spr.picnum = GRONDEAD;
			spr.cstat &= ~3;
			ChangeActorStat(actor, DEAD);
			if(isWh2()) {
				switch (spr.pal) {
				   case 0:
					   addscore(aiGetPlayerTarget(actor), 125);
						break;
				   case 10:
					   addscore(aiGetPlayerTarget(actor), 90);
						break;
				   case 11:
					   addscore(aiGetPlayerTarget(actor), 115);
						break;
				   case 12:
					   addscore(aiGetPlayerTarget(actor), 65);
						break;
				   }
			} else {
				addscore(aiGetPlayerTarget(actor), 200);
			}
			monsterweapon(actor);
		} else {
			switch (spr.picnum) {
			case GONZOBSHDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOBSHDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				if (spr.pal == 4) {
					ChangeActorStat(actor, SHADE);
					deaddude(actor);
				} else {
					ChangeActorStat(actor, DEAD);
					if (spr.shade < 25)
						monsterweapon(actor);
				}
				addscore(aiGetPlayerTarget(actor), 85);
				break;
			case GONZOCSWDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOCSWDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					ChangeActorStat(actor, SHADE);
					deaddude(actor);
				} else {
					ChangeActorStat(actor, DEAD);
					monsterweapon(actor);
				}
				addscore(aiGetPlayerTarget(actor), 55);
				break;
			case GONZOGSWDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOGSWDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					ChangeActorStat(actor, SHADE);
					deaddude(actor);
				} else {
					ChangeActorStat(actor, DEAD);
					monsterweapon(actor);
				}
				addscore(aiGetPlayerTarget(actor), 105);
				break;
			case GONZOGHMDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = GONZOGHMDEAD;
				spr.cstat &= ~3;
				if (spr.pal == 4) {
					ChangeActorStat(actor, SHADE);
					deaddude(actor);
				} else {
					ChangeActorStat(actor, DEAD);
					monsterweapon(actor);
				}
				addscore(aiGetPlayerTarget(actor), 100);
				break;
			case NEWGUYDEAD:
				if (netgame) {
					break;
				}
				spr.picnum = NEWGUYDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				monsterweapon(actor);
				addscore(aiGetPlayerTarget(actor), 50);
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
					ChangeActorStat(actor, SHADE);
					deaddude(actor);
				} else {
					ChangeActorStat(actor, DEAD);
					monsterweapon(actor);
				}
				addscore(aiGetPlayerTarget(actor), 110);
				break;
			case KATIEDEAD:
				if (netgame) {
					break;
				}
				trailingsmoke(actor, true);
				spr.picnum = DEVILDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				spawnhornskull(actor);
				addscore(aiGetPlayerTarget(actor), 500);
				break;
			case IMPDEAD:
				if (!isWh2())
					break;
				spr.picnum = IMPDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 115);
				monsterweapon(actor);
				break;
			case KOBOLDDEAD:
				spr.picnum = KOBOLDDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 10);
				break;
			case DRAGONDEAD:
				spr.picnum = DRAGONDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 4000);
				break;
			case DEVILDEAD:
				trailingsmoke(actor, true);
				spr.picnum = DEVILDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), isWh2() ? 70 : 50);
				if(isWh2())
					 monsterweapon(actor);
				break;
			case FREDDEAD:
				spr.picnum = FREDDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 40);
				break;
			case GOBLINDEAD:
				spr.picnum = GOBLINDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 25);
				if ((rand() % 100) > 60)
					monsterweapon(actor);
				break;
			case MINOTAURDEAD:
				spr.picnum = MINOTAURDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), isWh2() ? 95 : 70);
				if ((rand() % 100) > 60)
					monsterweapon(actor);
				break;
			case SPIDERDEAD:
				spr.picnum = SPIDERDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 5);
				break;
			case SKULLYDEAD:
				spr.picnum = SKULLYDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 100);
				break;
			case FATWITCHDEAD:
				spr.picnum = FATWITCHDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				addscore(aiGetPlayerTarget(actor), 900);
				break;
			case JUDYDEAD:
				spr.picnum = JUDYDEAD;
				spr.cstat &= ~3;
				ChangeActorStat(actor, DEAD);
				spawnapentagram(actor);
				addscore(aiGetPlayerTarget(actor), 7000);
				break;
			case WILLOWEXPLO + 2:
				spr.pal = 0;
				spr.cstat &= ~3;
				ChangeActorStat(actor,  0);
				DeleteActor(actor);
				addscore(aiGetPlayerTarget(actor), isWh2() ? 15 : 150);
				return;
			}
		}

		getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum,
				(spr.clipdist) << 2, CLIPMASK0);
		spr.z = zr_florz;

		if (zr_florHit.type == kHitSector) {
			if (spr.sectnum != MAXSECTORS && (spr.sector()->floorpicnum == WATER
					|| spr.sector()->floorpicnum == SLIME)) {
				if (spr.picnum == MINOTAURDEAD) {
					spr.z += (8 << 8);
					SetActorPos(actor, &spr.pos);
				}
			}
			if (spr.sectnum != MAXSECTORS && (spr.sector()->floorpicnum == LAVA
					|| spr.sector()->floorpicnum == LAVA1
					|| spr.sector()->floorpicnum == LAVA2)) {
				trailingsmoke(actor, true);
				DeleteActor(actor);
			}
		}
		break;
	}
	//
	// the control variable for monster release
	//
}

void makeafire(DWHActor* actor, int firetype) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, FIRE);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x + (krand() & 1024) - 512;
	spawned.y = spr.y + (krand() & 1024) - 512;
	spawned.z = spr.z;

	spawned.cstat = 0;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;

	spawned.shade = 0;

	spawned.clipdist = 64;

	spawnedactor->CopyOwner(actor);
	spawned.lotag = 2047;
	spawned.hitag = 0;
	ChangeActorStat(spawnedactor, FIRE);
	spawned.backuploc();
}

void explosion(DWHActor* actor, int x, int y, int z, int ownr) {
	auto& spr = actor->s();

	auto spawnedactor = InsertActor(spr.sectnum, EXPLO);
	auto& spawned = spawnedactor->s();

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
	spawned.ang =  (krand() & 2047);
	spawned.xvel =  ((krand() & 511) - 256);
	spawned.yvel =  ((krand() & 511) - 256);
	spawned.zvel =  ((krand() & 511) - 256);
	spawnedactor->CopyOwner(actor);
	spawned.hitag = 0;
	spawned.pal = 0;
	if(!isWH2) {
		spawned.picnum = MONSTERBALL;
		spawned.lotag = 256;
	} else {
		spawned.picnum = EXPLOSTART;
		spawned.lotag = 12;
	}
	spawned.backuploc();
}

void explosion2(DWHActor* actor, int x, int y, int z, int ownr) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, EXPLO);
	auto& spawned = spawnedactor->s();

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
	spawned.ang =  (krand() & 2047);
	spawned.xvel =  ((krand() & 256) - 128);
	spawned.yvel =  ((krand() & 256) - 128);
	spawned.zvel =  ((krand() & 256) - 128);
	spawnedactor->CopyOwner(actor);
	spawned.hitag = 0;
	spawned.pal = 0;
	
	if(!isWH2) {
		spawned.picnum = MONSTERBALL;
		spawned.lotag = 128;
	} else {
		spawned.picnum = EXPLOSTART;
		spawned.lotag = 12;
	}
	spawned.backuploc();

}

void trailingsmoke(DWHActor* actor, boolean ball) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, SMOKE);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z;
	
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

	spawnedactor->CopyOwner(actor);
	spawned.lotag = 256;
	spawned.hitag = 0;
	spawned.backuploc();
}

void icecubes(DWHActor* actor, int x, int y, int z, int ownr) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, FX);
	auto& spawned = spawnedactor->s();

	spawned.x = x;
	spawned.y = y;

	spawned.z = spr.sector()->floorz - (getPlayerHeight() << 8) + (krand() & 4096);

	spawned.cstat = 0; // Hitscan does not hit smoke on wall
	spawned.picnum = ICECUBE;
	spawned.shade = -16;
	spawned.xrepeat = 16;
	spawned.yrepeat = 16;

	spawned.ang =  (((krand() & 1023) - 1024) & 2047);
	spawned.xvel =  ((krand() & 1023) - 512);
	spawned.yvel =  ((krand() & 1023) - 512);
	spawned.zvel =  ((krand() & 1023) - 512);

	spawned.pal = 6;
	spawnedactor->CopyOwner(actor);

	if(isWh2())
		spawned.lotag = 2048;
	else spawned.lotag = 999;
	spawned.hitag = 0;
	spawned.backuploc();

}

boolean damageactor(PLAYER& plr, DWHActor* hitactor, DWHActor* actor) 
{
	auto& spr = actor->s();
	auto& hitspr = hitactor->s();
	if (hitactor == plr.actor() && actor->GetPlayerOwner() == plr.playerNum())
		return false;

	if (hitactor == plr.actor() && actor->GetPlayerOwner() != plr.playerNum())
	{
		if (plr.invincibletime > 0 || plr.godMode) {
			DeleteActor(actor);
			return false;
		}
		// EG 17 Oct 2017: Mass backport of RULES.CFG behavior for resist/onyx ring
		// EG 21 Aug 2017: New RULES.CFG behavior in place of the old #ifdef
		if (plr.manatime > 0) {
			if (/* eg_resist_blocks_traps && */spr.picnum != FATSPANK && spr.picnum != PLASMA) {
				DeleteActor(actor);
				return false;
			}
			// Use "fixed" version: EXPLOSION and MONSTERBALL are the fire attacks, account
			// for animation
			// FATSPANK is right in the middle of this group of tiles, so still keep an
			// exception for it.
			else if ((spr.picnum >= EXPLOSION && spr.picnum <= (MONSTERBALL + 2)
					&& spr.picnum != FATSPANK)) {
				DeleteActor(actor);
				return false;
			}
		}
		// EG 21 Aug 2017: onyx ring
		else if (plr.treasure[TONYXRING] == 1 /*&& eg_onyx_effect != 0*/
				&& ((spr.picnum < EXPLOSION || spr.picnum > MONSTERBALL + 2)
						&& spr.picnum != PLASMA)) {
			
//				if (eg_onyx_effect == 1 || (eg_onyx_effect == 2 && ((krand() & 32) > 16))) {
				DeleteActor(actor);
				return false;
//				}
		}
		// EG 21 Aug 2017: Move this here so as not to make ouch sounds unless pain is
		// happening
		if ((krand() & 9) == 0)
			spritesound(S_PLRPAIN1 + (rand() % 2), actor);

		if (isWh2() && spr.picnum == DART) {
			plr.poisoned = 1;
			plr.poisontime = 7200;
			showmessage("Poisoned", 360);
		}
		 
		if (netgame) {
//						netdamageactor(j,i);
		} else {
			if (spr.picnum == PLASMA)
				addhealth(plr, -((krand() & 15) + 15));
			else if (spr.picnum == FATSPANK) {
				spritesound(S_GORE1A + (krand() % 3),  plr.actor());
				addhealth(plr, -((krand() & 10) + 10));
				if ((krand() % 100) > 90) {
					plr.poisoned = 1;
					plr.poisontime = 7200;
					showmessage("Poisoned", 360);
				}
			} else if (spr.picnum == THROWPIKE) {
				addhealth(plr, -((krand() % 10) + 5));
			} else
				addhealth(plr, -((krand() & 20) + 5));
		}

		startredflash(10);
		/* EG 2017 - Trap fix */
		DeleteActor(actor);
		return true;
	}

	if (hitactor == plr.actor() && !netgame) // Les 08/11/95
		if (actor->GetOwner() != hitactor) {
			
//				final int DEMONTYPE = 1; XXX
//				final int DRAGONTYPE = 3;
//				final int FISHTYPE = 5;
//				final int JUDYTYPE = 12;
//				final int RATTYPE = 18;
//				final int SKULLYTYPE = 20;
			
			switch (hitspr.detail) {
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
				if(isBlades(spr.picnum)) {
					hitspr.hitag -= 30;
					if(spr.picnum == THROWPIKE) {
						if ((krand() % 2) != 0)
							spritesound(S_GORE1A + krand() % 2, actor);
					}
				} else {
				switch (spr.picnum) {
					case PLASMA:
						hitspr.hitag -= 40;
						break;
					case FATSPANK:
						hitspr.hitag -= 10;
						break;
					case MONSTERBALL:
						hitspr.hitag -= 40;
						break;
					case FIREBALL:
						if(!isWh2())
							hitspr.hitag -= 3;
						break;
					case BULLET:
						hitspr.hitag -= 10;
						break;
					case DISTORTIONBLAST:
						hitspr.hitag = 10;
						break;
					case BARREL:
						hitspr.hitag -= 100;
						break;
					}
				}
				
				if (hitspr.hitag <= 0) {
					SetNewStatus(hitactor, DIE);
					DeleteActor(actor);
				} else
					SetNewStatus(hitactor, PAIN);
				return true;
			}
			
			switch(hitspr.detail) {
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
				if (hitspr.pal == 6) {
					for (int k = 0; k < 32; k++)
						icecubes(hitactor, hitspr.x, hitspr.y, hitspr.z, 0);
					// EG 26 Oct 2017: Move this here from medusa (anti multi-freeze exploit)
					addscore(&plr, 100);
					DeleteActor(hitactor);
				}
				return true;
			}
			
			switch (hitspr.picnum) {
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
				hitspr.hitag = 0;
				hitspr.lotag = 0;
				SetNewStatus(hitactor, BROKENVASE);
				break;
			default:
				DeleteActor(actor);
				return true;
			}
		}

	return false;
}

Collision movesprite(DWHActor* actor, int dx, int dy, int dz, int ceildist, int flordist, int cliptype) {

	Collision coll;
	int zoffs;
	int retval;
	int tempshort, dasectnum;

	SPRITE& spr = actor->s();
	if (spr.statnum == MAXSTATUS)
		return coll.setNone();

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
		changespritesect(actor->GetSpriteIndex(), dasectnum);

	// Set the blocking bit to 0 temporarly so getzrange doesn't pick up
	// its own sprite
	tempshort = spr.cstat;
	spr.cstat &= ~1;
	getzrange(spr.x, spr.y, spr.z - 1, spr.sectnum, (spr.clipdist) << 2, dcliptype);
	spr.cstat = tempshort;

	int daz = spr.z + zoffs + dz;
	if ((daz <= zr_ceilz) || (daz > zr_florz)) {
		if (retval != 0)
			return Collision(retval);
		return coll.setSector(dasectnum);
	}
	spr.z = (daz - zoffs);
	return Collision(retval);
}

void trowajavlin(DWHActor* actor) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum, JAVLIN);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z;// - (40 << 8);
	
	spawned.cstat = 21;

	switch (spr.lotag) {
	case 91:
		spawned.picnum = WALLARROW;
		spawned.ang =  (((spr.ang + 2048) - 512) & 2047);
		spawned.xrepeat = 16;
		spawned.yrepeat = 48;
		spawned.clipdist = 24;
		break;
	case 92:
		spawned.picnum = DART;
		spawned.ang =  (((spr.ang + 2048) - 512) & 2047);
		spawned.xrepeat = 64;
		spawned.yrepeat = 64;
		spawned.clipdist = 16;
		break;
	case 93:
		spawned.picnum = HORIZSPIKEBLADE;
		spawned.ang =  (((spr.ang + 2048) - 512) & 2047);
		spawned.xrepeat = 16;
		spawned.yrepeat = 48;
		spawned.clipdist = 32;
		break;
	case 94:
		spawned.picnum = THROWPIKE;
		spawned.ang =  (((spr.ang + 2048) - 512) & 2047);
		spawned.xrepeat = 24;
		spawned.yrepeat = 24;
		spawned.clipdist = 32;
		break;
	}

	spawned.extra = spr.ang;
	spawned.shade = -15;
	spawned.xvel =  ((krand() & 256) - 128);
	spawned.yvel =  ((krand() & 256) - 128);
	spawned.zvel =  ((krand() & 256) - 128);

	spawnedactor->SetOwner(nullptr);
	spawned.lotag = 0;
	spawned.hitag = 0;
	spawned.pal = 0;
	spawned.backuploc();
}

void spawnhornskull(DWHActor* actor) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum,  0);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z - (24 << 8);
	
	spawned.shade = -15;
	spawned.cstat = 0;
	spawned.cstat &= ~3;
	spawned.pal = 0;
	spawned.picnum = HORNEDSKULL;
	spawned.detail = HORNEDSKULLTYPE;
	spawned.xrepeat = 64;
	spawned.yrepeat = 64;
	spawned.backuploc();
}

void spawnapentagram(DWHActor* actor) {
	auto& spr = actor->s();
	auto spawnedactor = InsertActor(spr.sectnum,  0);
	auto& spawned = spawnedactor->s();

	spawned.x = spr.x;
	spawned.y = spr.y;
	spawned.z = spr.z - (8 << 8);
	
	spawned.xrepeat = spawned.yrepeat = 64;
	spawned.pal = 0;
	spawned.shade = -15;
	spawned.cstat = 0;
	spawned.clipdist = 64;
	spawned.lotag = 0;
	spawned.hitag = 0;
	spawned.extra = 0;
	spawned.picnum = PENTAGRAM;
	spawned.detail = PENTAGRAMTYPE;

	SetActorPos(spawnedactor, &spawned.pos);
	spawned.backuploc();
}

END_WH_NS
