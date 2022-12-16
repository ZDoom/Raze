class DukeToiletWater : DukeActor
{
	default
	{
		pic "TOILETWATER";
		+FORCERUNCON;
		+NOTELEPORT;
	}
	
	override void Initialize()
	{
		self.shade = -16;
		self.changeStat(STAT_STANDABLE);
	}
}
