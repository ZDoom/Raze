class DukeOnFire : DukeActor
{
	default
	{
		pic "FLOORFLAME";
		+FORCERUNCON;
	}

	override void  Initialize()
	{
		if (!self.mapspawned)
		{
			self.Angle = self.ownerActor.Angle;
			self.shade = -64;
			self.cstat = randomXFlip();

			double c, f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			if (self.pos.Z > f - 12)
				self.pos.Z = f - 12;
		}

		self.pos.X += frandom(-16, 16);
		self.pos.Y += frandom(-16, 16);
		self.pos.Z -= frandom(0, 40);
		self.cstat |= CSTAT_SPRITE_YCENTER;
		self.scale = (0.375, 0.375);
		self.ChangeStat(STAT_MISC);
	}
}

class DukeOnFireSmoke : DukeActor
{
	default
	{
		pic "ONFIRESMOKE";
		+FORCERUNCON;
	}
}
