
class DukeFloorFlame : DukeActor
{
	default
	{
		pic "FLOORFLAME";
	}

	override void Initialize()
	{
		self.shade = -127;
		self.ChangeStat(STAT_STANDABLE);
	}
		
	override bool animate(tspritetype t)
	{
		t.shade = -127;
		return false;
	}
}

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

	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		
		if (OwnerAc && OwnerAc.statnum == STAT_PLAYER)
		{
			let p = Duke.GetViewPlayer();
			if (display_mirror == 0 && OwnerAc == p.actor && p.over_shoulder_on == 0)
				t.scale = (0, 0);
			else
			{
				// not needed.
				//t.Angle = (viewVec - t.pos.XY()).Angle();
				//t.pos.XY = OwnerAc.pos.XY + t.Angle.ToVector();
			}
		}
		t.cstat |= CSTAT_SPRITE_YCENTER;
		double d;
		if (!OwnerAc || !OwnerAc.actorflag1(SFLAG_NOFLOORFIRE))
			[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
		t.shade = -127;
		return false;
	}
}

class DukeBurning2 : DukeBurning
{
	default
	{
		pic "BURNING2";
	}
}

class RedneckFire : DukeActor
{
	default
	{
		pic "FIRE";
	}
	
	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		double d;
		if (!OwnerAc || !OwnerAc.actorflag1(SFLAG_NOFLOORFIRE))
			[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
		t.shade = -127;
		return false;
	}
}

class DukeFire : RedneckFire
{
	override bool animate(tspritetype t)
	{
		super.animate(t);
		t.cstat |= CSTAT_SPRITE_YCENTER;
		return false;
	}
}

class DukeFire2 : DukeFire
{
	default
	{
		pic "FIRE2";
	}
}

class DukeOnFire : DukeActor
{
	default
	{
		pic "FLOORFLAME";
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
	}
}
