class DukeExplosion2 : DukeActor
{
	default
	{
		pic "EXPLOSION2";
		+FULLBRIGHT;
	}
	
	override bool animate(tspritetype t)
	{
		Duke.GetViewPlayer().visibility = -127;
		Duke.setlastvisinc(32);
		return false;
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
		self.cstat |= CSTAT_SPRITE_YCENTER;
		self.shade = -127;
		self.Scale = (0.75, 0.75);
		self.ChangeStat(STAT_MISC);
	}
}

class DukeExplosion2Bot : DukeExplosion2
{
	default
	{
		pic "EXPLOSION2BOT";
		+FULLBRIGHT;
	}
}

class RedneckExplosion3 : DukeExplosion2
{
	default
	{
		pic "EXPLOSION3";
	}
	
	override void Initialize()
	{
		self.scale = (2, 2);
	}
	
	override bool animate(tspritetype t)
	{
		t.shade = -127;
		return false;
	}
}
