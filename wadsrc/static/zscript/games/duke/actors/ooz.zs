class DukeOoz : DukeActor
{
	default
	{
		shade -12;
		statnum STAT_ACTOR;
		pic "OOZ";
	}

	override void Initialize()
	{
		self.shade = -12;

		if (!self.mapSpawned)
		{
			if (self.actorflag2(SFLAG2_PAL8OOZ))
				self.pal = 8;
			if (!Raze.IsRR()) self.insertspriteq();
		}

		self.getglobalz();

		double z = ((self.floorz - self.ceilingz) / 128.);

		self.scale = (max(0., 0.390625 - z * 0.5), z);
		self.cstat |= randomXFlip();
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
