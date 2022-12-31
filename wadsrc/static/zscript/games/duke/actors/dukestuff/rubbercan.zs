class DukeRubberCan : DukeActor
{
	default
	{
		pic "RUBBERCAN";
		Strength WEAK;
		+FORCERUNCON;
		+CHECKSLEEP;
		+MOVEFTA_MAKESTANDABLE;
	}

	override void Initialize()
	{
		if (!self.mapSpawned)
			self.scale = (0.5, 0.5);
		self.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
		self.clipdist = 18;
		self.ChangeStat(STAT_ZOMBIEACTOR);
		self.extra = 0;
	}
	
}


