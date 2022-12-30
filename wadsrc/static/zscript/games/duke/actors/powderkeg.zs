
class RedneckPowderKeg : DukeItemBase
{
	default
	{
		pic "POWDERKEG";
		+NOFLOORPAL;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
		+INFLAME;
	}
	
	override void Tick()
	{
		let sectp = self.sector;
		if (sectp.lotag != ST_1_ABOVE_WATER && (!(ud.mapflags & MFLAG_ALLSECTORTYPES) && sectp.lotag != ST_160_FLOOR_TELEPORT))
			if (self.vel.X != 0)
			{
				movesprite((self.Angle.ToVector()* self.vel.X, self.vel.Z), CLIPMASK0);
				self.vel.X -= 1. / 16.;
			}
		Super.Tick();
	}
	
	
	override bool shootthis(DukeActor actor, DukePlayer p, Vector3 spos, double sang) const
	{
		let j = actor.spawn("RedneckPowderKeg");
		if (j)
		{
			j.vel.X = 2;
			j.Angle = actor.Angle;
			j.pos.Z -= 5;
		}
		return true;
	}
		
}
