class DukeSmallSmoke : DukeActor
{
	default
	{
		pic "SMALLSMOKE";
		+FORCERUNCON;
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;

		if (owner && owner != self)
		{
			self.Angle = owner.Angle;
			self.cstat = randomXFlip();

			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.scale = (0.375, 0.375);
		self.ChangeStat(STAT_MISC);
	}
}

