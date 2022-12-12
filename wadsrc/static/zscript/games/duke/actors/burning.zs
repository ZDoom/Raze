
class DukeBurning : DukeActor
{
	default
	{
		pic "BURNING";
	}
	
	override void Initialize()
	{
		if (!self.mapSpawned)
		{
			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.Scale = (0.0625, 0.0625);
		self.ChangeStat(STAT_MISC);
	}
}

class DukeBurning2 : DukeBurning
{
	default
	{
		pic "BURNING2";
	}
}
