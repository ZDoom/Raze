class RedneckVixen : DukeActor
{
	default
	{
		pic "VIXEN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		ProjectileSpread -2.8125;
	}

	override void Initialize()
	{
		if (self.pal == 34)
		{
			self.scale = (0.34375, 0.328125);
		}
		else
		{
			self.scale = (0.3125, 0.3125);
		}
		self.setClipDistFromTile();
	}

	
	override Vector3 SpecialProjectileOffset()
	{
		return (0, 0, -12);
	}
	
}


class RedneckUWhip : DukeProjectile
{
	default
	{
		pic "UWHIP";
		+FULLBRIGHT;
		+INFLAME;
	}
	
	void shootwhip(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = actor.sector;
		double zvel;
		double vel = 300 / 16;

		if (actor.extra >= 0) actor.shade = -96;

		if (p != null)
		{
			let aimed = actor.aim(self);

			if (aimed)
			{
				double dal = ((aimed.scale.X * aimed.spriteHeight()) * 0.5);
				double dist = (p.actor.pos.XY - aimed.pos.XY).Length();
				zvel = ((aimed.pos.Z - pos.Z - dal) * vel) / dist;
				ang = (aimed.pos.XY - pos.XY).Angle();
			}
			else
				[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 49.);
		}
		else
		{
			let j = actor.findplayer();
			if (actor.projectilespread < 0)
				ang += frandom(self.projectilespread, 0);
			else
				ang += frandom(-self.projectilespread / 2, self.projectilespread / 2);

			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			zvel = ((j.actor.opos.Z + j.actor.oviewzoffset - pos.Z + 3) * vel) / dist;
		}

		double oldzvel = zvel;
		double scale = p? 0.109375 : 0.125;

		let spawned = dlevel.SpawnActor(sect, pos, self.GetClass(), -127, (scale,scale), ang, vel, zvel, actor, STAT_PROJECTILE);
		if (!spawned) return;
		spawned.extra += random(0, 7);
		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.clipdist = 1;

		ang = actor.Angle + frandom(-22.5 / 4, 22.5 / 4);
		zvel = oldzvel + frandom(-2, 2);
	}

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		pos.Z += 4;
		shootwhip(actor, p, pos, ang);
		return true;
	}
}

class RedneckOWhip : RedneckUWhip
{
	default
	{
		pic "OWHIP";
	}

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang)
	{
		pos.Z -= 15;
		shootwhip(actor, p, pos, ang);
		return true;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RedneckVixenShot : DukeActor
{
	default
	{
		pic "VIXENSHOT";
		+INFLAME;
		+FULLBRIGHT;
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		pos.Z -= 4;
		shootprojectile1(actor, p, pos, ang, 52.5, 0, 0.125);
		return true;
	}
	
}

