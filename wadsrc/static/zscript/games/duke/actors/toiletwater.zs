class DukeToiletWater : DukeActor
{
	default
	{
		pic "TOILETWATER";
	}
	
	override void Initialize()
	{
		self.shade = -16;
		self.changeStat(STAT_STANDABLE);
	}
}
