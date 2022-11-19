class DukeDoorShock : DukeActor
{
	default
	{
		shade -12;
		statnum STAT_STANDABLE;
	}

	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}


	override void Tick()
	{
		let sectp = self.sector;
		double j = abs(sectp.ceilingz - sectp.floorz) / 128.;
		self.scale = (0.25, 0.0625 + j);
		self.pos.Z = sectp.floorz;
	}
}
