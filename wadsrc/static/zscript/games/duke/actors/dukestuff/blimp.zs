
class DukeBlimp : DukeActor
{
	const BLIMPRESPAWNTIME = 2048;

	default
	{
		pic "BLIMP";
		Strength 1;
		+SPAWNWEAPONDEBRIS;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}
	
}		
