class DukeBossBase : DukeActor
{
	const PIGCOPSTRENGTH = 100;

	default
	{
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NODAMAGEPUSH;
		+BOSS;
		+ALTHITSCANDIRECTION;

	}

	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && owner is 'DukeRespawnController')
			self.pal = owner.pal;
		
		if (self.pal != 0 && (!Raze.isWorldTour() || !(currentLevel.gameflags & MapRecord.LEVEL_WT_BOSSSPAWN) || self.pal != 22))
		{
			self.clipdist = 20;
			self.scale = (0.625, 0.625);
		}
		else
		{
			self.scale = (1.25, 1.25);
			self.clipdist = 41;
		}
	}
	
}

class DukeBoss1 : DukeBossBase
{
	const BOSS1STRENGTH = 4500;
	const BOSS1PALSTRENGTH = 1000;

	default
	{
		pic "BOSS1";
		+DONTENTERWATER;
		Strength BOSS1STRENGTH;
		
	}
	
	override void PlayFTASound(int mode)
	{
		Duke.PlaySound("BOS1_RECOG");
	}
	
	
}


class DukeBoss1Stayput : DukeBoss1
{
	default
	{
		pic "BOSS1STAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void PlayFTASound(int mode)
	{
	}
	
}
	