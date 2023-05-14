class DukeShrinker : DukeActor
{
	override bool ShootThis(DukeActor shooter, DukePlayer p, Vector3 pos, double ang) const
	{
		double vel = 48.;
		double zvel;
		if (shooter.extra >= 0) shooter.shade = -96;
		if (p != null)
		{
			let aimed = IsNamWW2GI()? null : shooter.aim(self);
			if (aimed)
			{
				double dal = ((aimed.scale.X * aimed.spriteHeight()) * 0.5); // note the incorrect scale this uses!
				double dist = (p.actor.pos.XY - aimed.pos.XY).Length();
				zvel = ((aimed.pos.Z - pos.Z - dal - 4) * 48) / dist;
				ang = (aimed.pos.XY - pos.XY).Angle();
			}
			else
				[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 49.);
		}
		else if (shooter.statnum != STAT_EFFECTOR)
		{
			double x;
			DukePlayer j;
			[j, x] = shooter.findplayer();
			double dist = (j.actor.pos.XY - shooter.pos.XY).Length();
			zvel = ((j.actor.pos.Z + j.actor.viewzoffset - pos.Z) * 32) / dist;
		}
		else zvel = 0;

		let spawned = dlevel.SpawnActor(shooter.sector, pos.plusZ(2) + ang.ToVector() * 0.25, "DukeShrinkSpark", -16, (0.4375, 0.4375), ang, vel, zvel, shooter, STAT_PROJECTILE);

		if (spawned)
		{
			spawned.cstat = CSTAT_SPRITE_YCENTER;
			spawned.clipdist = 8;
		}
		return true;
	}
}
/*
class NamShrinker : DukeShrinker
{
	default
	{
		+NOAUTOAIM;
	}
}
*/

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeShrinkSpark : DukeProjectile
{
	default
	{
		spriteset "SHRINKSPARK", "SHRINKSPARK1", "SHRINKSPARK2", "SHRINKSPARK3";
		Strength SHRINKER_WEAPON_STRENGTH;
		+FULLBRIGHT;
		+MIRRORREFLECT;
		+NOFLOORPAL;
		+NOCEILINGBLAST;
		+HITRADIUS_DONTHURTSHOOTER;
		+HITRADIUS_NODAMAGE;
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.spawn('DukeShrinkerExplosion');
		self.PlayActorSound("SHRINKER_HIT");
		self.hitradius(gs.shrinkerblastradius, 0, 0, 0, 0);
		self.Destroy();
	}
	
	override bool animate(tspritetype tspr)
	{
		tspr.setSpritePic(self, (PlayClock >> 4) & 3);
		return true;
	}

	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		return 'DukeShrinkSpark';
	}
	
}


class DukeShrinkerExplosion : DukeActor
{
	default
	{
		spriteset "SHRINKEREXPLOSION";
		+FULLBRIGHT;
		+FORCERUNCON;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner != self)
		{
			self.Angle = spawner.Angle;
			self.cstat = CSTAT_SPRITE_YCENTER | randomXFlip();
			double c,f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			self.pos.Z = min(self.pos.Z, f - 12);
		}
		self.shade = -64;
		self.scale = (0.5, 0.5);
		self.ChangeStat(STAT_MISC);
	}
	
}

