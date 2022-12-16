class DukeOoz : DukeActor
{
	default
	{
		pic "OOZ";
	}

	override void Initialize()
	{
		self.shade = -12;

		if (!self.mapSpawned)
		{
			if (self.bPAL8OOZ)
				self.pal = 8;
			if (!Raze.IsRR()) self.insertspriteq();
		}

		self.getglobalz();

		double z = ((self.floorz - self.ceilingz) / 128.);

		self.scale = (max(0., 0.390625 - z * 0.5), z);
		self.cstat |= randomXFlip();
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		self.getglobalz();

		double y = min((self.floorz - self.ceilingz) / 128, 4.);
		double x = clamp(0.390625 - y * 0.5, 0.125, 0.75);

		self.scale = (x, y);
		self.pos.Z = self.floorz;
	}
}

class DukeOoz2 : DukeActor
{
	default
	{
		pic "OOZ2";
	}
}
