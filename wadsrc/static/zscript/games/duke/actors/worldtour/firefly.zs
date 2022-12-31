
class DukeFireflyShrinkEffect : DukeActor
{
	default
	{
		pic "FIREFLYSHRINKEFFECT";
		Strength 0;
	}
	
}

class DukeFireflyGrowEffect : DukeActor
{
	default
	{
		pic "FIREFLYGROWEFFECT";
		Strength 0;
	}

}

class DukeFireflyFlyingEffect : DukeActor
{
	default
	{
		pic "FIREFLYFLYINGEFFECT";
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.ChangeStat(STAT_MISC);
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void Tick()
	{
		Super.Tick();
		if (bDestroyed) return;	// killed by script.


		let Owner = self.ownerActor;
		if (!Owner || !(Owner is 'DukeFirefly'))
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

class DukeFirefly : DukeLizTrooper // recycles part of the Liztrooper code and data
{
	const FF_STRENGTH = 50;
	const FF_SIZEX = 48;
	const FF_SIZEY = 40;
	const FF_SHRUNKSIZEX = 12;
	const FF_SHRUNKSIZEY = 10;
	
	default
	{
		pic "FIREFLY";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		-DONTENTERWATERONGROUND;
	
		Strength FF_STRENGTH;
	}
	
	override bool ShootThis(DukeActor shooter, DukePlayer p, Vector3 spos, double sang) const
	{
		let k = shooter.spawn("DukeFirefly");
		if (k)
		{
			k.sector = shooter.sector;
			k.pos = spos;
			k.Angle = sang;
			k.vel.X = 500 / 16.;
			k.vel.Z = 0;
		}
		return true;
	}
	
	override void Initialize()
	{
	}
	

}
