
// for now only the effect is scriptified.

class DukeFireflyFlyingEffect : DukeActor
{
	default
	{
		pic "FIREFLYFLYINGEFFECT";
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.ChangeStat(STAT_MISC);
	}
	
	override void Tick()
	{
		Super.Tick();
		if (bDestroyed) return;	// killed by script.


		let Owner = self.ownerActor;
		if (!Owner || !Owner.checkType("FIREFLY"))
		{
			self.Destroy();
			return;
		}

		if (Owner.scale.X >= 0.375 || Owner.pal == 1)
			self.cstat |= CSTAT_SPRITE_INVISIBLE;
		else
			self.cstat &= ~CSTAT_SPRITE_INVISIBLE;

		let p = self.findplayer();
		let dvec = Owner.pos.XY - p.actor.pos.XY;
		double dist = dvec.Length();

		if (dist != 0.0) dvec /= dist;
		self.pos = Owner.pos + (dvec.X * -0.625, dvec.Y * -0.625, 8);

		if (Owner.extra <= 0) 
		{
			self.Destroy();
		}
	}
}
