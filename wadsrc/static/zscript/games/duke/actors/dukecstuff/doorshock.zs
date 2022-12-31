class DukeDoorShock : DukeActor
{
	default
	{
		pic "DOORSHOCK";
	}

	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.shade = -12;
		self.ChangeStat(STAT_STANDABLE);
	}


	override void Tick()
	{
		let sectp = self.sector;
		double j = abs(sectp.ceilingz - sectp.floorz) / 128.;
		self.scale = (0.25, 0.0625 + j);
		self.pos.Z = sectp.floorz;
	}
}
