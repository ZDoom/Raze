

class DukeGenericDestructible : DukeActor
{
	TextureID spawnstate, brokenstate;
	Sound breaksound;
	bool broken;
	
	native bool SetBroken(bool bust);	// sets broken texture. Must be done natively as long as we do not have proper texture support.
	
	virtual bool DestroyAction() { return false; }	// for customized subclasses
	
	override void Initialize()
	{
		self.SetBroken(false);
	}
	
	override void OnHit(DukeActor proj)
	{
		if (broken) return;
		
		broken = true;
		bool res1 = self.SetBroken(true);
		bool res2 = DestroyAction();
		if (res1 || res2) 
			self.Destroy();
	}
}

class DukeGenericDamagingDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.hitradius(10, 0, 0, 1, 1);
		return false;
	}
}

class DukeGenericGlassSpawningDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.lotsofglass(10);
		return false;
	}
}

class DukeGenericUnblockingDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		return false;
	}
}

/*
// todo: RRRA


// RR

	case RRTILE3114:
		targ->spr.picnum = RRTILE3117;
		break;
	case RRTILE2876:
		targ->spr.picnum = RRTILE2990;
		break;
	case RRTILE3152:
		targ->spr.picnum = RRTILE3218;
		break;
	case RRTILE3153:
		targ->spr.picnum = RRTILE3219;
		break;
	case RRTILE2030:
		targ->spr.picnum = RRTILE2034;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2893:
	case RRTILE2915:
	case RRTILE3115:
	case RRTILE3171:
		switch (targ->spr.picnum)
		{
		case RRTILE2915:
			targ->spr.picnum = RRTILE2977;
			break;
		case RRTILE2893:
			targ->spr.picnum = RRTILE2978;
			break;
		case RRTILE3115:
			targ->spr.picnum = RRTILE3116;
			break;
		case RRTILE3171:
			targ->spr.picnum = RRTILE3216;
			break;
		}
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2156:
	case RRTILE2158:
	case RRTILE2160:
	case RRTILE2175:
		targ->spr.picnum++;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;

	case RRTILE2654:
	case RRTILE2656:
	case RRTILE3172:
		if (!isRRRA()) break;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		targ->Destroy();
		break;
	case GRATE1:
		targ->spr.picnum = BGRATE1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;

	case CIRCLEPANNEL:
		targ->spr.picnum = CIRCLEPANNELBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;

+
	case GRATE1:
		targ->spr.picnum = BGRATE1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;

	case CIRCLEPANNEL:
		targ->spr.picnum = CIRCLEPANNELBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;


Duke

	case GRATE1:
		targ->spr.picnum = BGRATE1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;

	case CIRCLEPANNEL:
		targ->spr.picnum = CIRCLEPANNELBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL1:
	case PANNEL2:
		targ->spr.picnum = BPANNEL1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL3:
		targ->spr.picnum = BPANNEL3;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;


	case CHAIR1:
	case CHAIR2:
		targ->spr.picnum = BROKENCHAIR;
		targ->spr.cstat = 0;
		break;
	case CHAIR3:
	case MOVIECAMERA:
	case SCALE:
	case VACUUM:
	case CAMERALIGHT:
	case IVUNIT:
	case POT1:
	case POT2:
	case POT3:
	case TRIPODCAMERA:
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);
		targ->Destroy();
		break;
*/
