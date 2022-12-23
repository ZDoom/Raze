
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class RedneckDynamiteArrow : DukeRPG
{
	default
	{
		pic "RPG";
	}
	
	override bool weaponhitsprite_pre(DukeActor targ)
	{
		if (targ.bTRANSFERPALTOJIBS && targ.pal == 19)
		{
			self.PlayActorSound("RPG_EXPLODE");
			let spawned = self.spawn("DukeExplosion2");
			if (spawned)
				spawned.pos = oldpos;
			return true;
		}
		return Super.weaponhitsprite_pre(targ);
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.rpgexplode(coll.type, oldpos, false, -1, "RPG_EXPLODE");
		self.Destroy();
	}
	
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class RedneckChickenArrow : RedneckDynamiteArrow
{
	default
	{
		pic "RPG2";
		+FORCEAUTOAIM;
		+NOFLOORPAL;
		+ALWAYSROTATE2;
		DukeProjectile.SpawnSound "CHICKENBOW_FIRE";
	}
	
	override void Initialize()
	{
		self.hitag = 0;
		self.lotsofstuff("RedneckFeather", random(1, 4));
	}
	
	override bool premoveeffect()
	{
		// seeker handling
		self.hitag++;
		if (self.scale.X >= 0.15625 && self.sector.lotag != ST_2_UNDERWATER)
		{
			let spawned = self.spawn("DukeSmallSmoke");
			if (spawned) spawned.pos.Z += 1;
			if (random(0, 15) == 2)
			{
				self.spawn("RedneckFeather");
			}
		}
		DukeActor ts = self.seek_actor;
		if (!ts) return false;

		if (ts.extra <= 0)
			self.seek_actor = null;

		if (self.seek_actor && self.hitag > 5)
		{
			let ang = (ts.pos - self.pos).Angle();
			let ang2 = deltaangle(ang, self.angle);
			// this was quite broken in the original code. Fixed so that it seeks properly
			if (abs(ang2) < 17.5)
			{
				self.angle = ang;
			}
			else if (ang2 > 0)
			{
				self.angle -= 9;
			}
			else
				self.angle += 9;

			if (self.hitag > 180)
				if (self.vel.Z <= 0)
					self.vel.Z += 200 / 256;
		}
		return false;
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.rpgexplode(coll.type, oldpos, false, 150, "CHKBOWEX");
		self.Destroy();
	}
	
	
}

