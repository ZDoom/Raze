
class DukeBurning : DukeActor
{
	default
	{
		pic "BURNING";
		+FULLBRIGHT;
		+FORCERUNCON;
		+NOTELEPORT;
		+NOFLOORPAL;
		Strength WEAK;
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
		if (!bNOFLOORFIRE)
			[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
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
		+FULLBRIGHT;
		+NOTELEPORT;
		+NOFLOORPAL;
		Strength WEAK;
		
	}
	
	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		double d;
		[d, t.pos.Z] = t.sector.getSlopes(t.pos.XY);
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

