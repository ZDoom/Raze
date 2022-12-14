class DukeBoss1 : DukeActor
{
	default
	{
		pic "BOSS1";
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
	
	override void PlayFTASound()
	{
		Duke.PlaySound("BOS1_RECOG");
	}
}


class DukeBoss1Stayput : DukeBoss1
{
	default
	{
		pic "BOSS1STAYPUT";
	}
	
	override void PlayFTASound()
	{
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
	
}
	