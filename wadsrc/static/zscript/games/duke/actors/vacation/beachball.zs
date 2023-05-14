

class VacationBeachBall : DukeActor
{
	const BALLSTRENGTH = 20;
	default
	{
		pic "BEACHBALL";
		Strength WEAK;
	}

	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}

}