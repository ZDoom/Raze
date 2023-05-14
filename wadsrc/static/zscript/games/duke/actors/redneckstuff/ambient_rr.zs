
class RedneckSoundFX : DukeActor
{
	default
	{
		Strength 10;
	}

	override void StaticSetup()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.detail = dlevel.addambient(self.hitag, self.lotag);
		self.lotag = self.hitag = 0;
	}
	
	// this actor needs to start on STAT_DEFAULT.
	override void Initialize(DukeActor spawner)
	{
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}

}
