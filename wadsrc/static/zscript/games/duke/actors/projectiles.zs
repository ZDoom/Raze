
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFireball : DukeProjectile // WorldTour only
{
	default
	{
		pic "FIREBALL";
		+FULLBRIGHT;
	}
	
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		// World Tour's values for angles and velocities are quite arbitrary...
		double vel, zvel;

		if (actor.extra >= 0)
			actor.shade = -96;

		pos.Z -= 2;
		if (!(actor is 'DukeBoss5'))
			vel = 840/16.;
		else {
			vel = 968/16.;
			pos.Z += 24;
		}

		if (p == null)
		{
			ang += 22.5 / 8 - frandom(0, 22.5 / 4);
			let j = actor.findplayer();
			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			zvel = ((j.actor.opos.z + j.actor.oviewzoffset - pos.Z + 3) * vel) / dist;
		}
		else
		{
			[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 49.);
			pos += (ang + 61.171875).ToVector() * (1024. / 448.);
			pos.Z += 3;
		}

		double scale = 0.28125;

		let spawned = dlevel.SpawnActor(actor.sector, pos, GetClass(), -127, (0.28125, 0.28125), ang, vel, zvel, actor, STAT_PROJECTILE);
		if (spawned)
		{
			spawned.extra += random(0, 7);
			if ((actor is 'DukeBoss5') || p)
			{
				spawned.scale = (0.625, 0.625);
			}
			//spawned.yint = p;
			spawned.cstat = CSTAT_SPRITE_YCENTER;
			spawned.clipdist = 1;
		}
		return true;
	}

	
	override bool premoveeffect()
	{
		let Owner = self.ownerActor;

		if (self.sector.lotag == 2)
		{
			self.Destroy();
			return true;
		}

		if (self.detail != 1)
		{
			if (self.counter >= 1 && self.counter < 6)
			{
				double siz = 1.0 - (self.counter * 0.2);
				DukeActor trail = self.temp_actor;
				let ball = self.spawn('DukeFireball');
				if (ball)
				{
					self.temp_actor = ball;

					ball.vel.X = self.vel.X;
					ball.vel.Z = self.vel.Z;
					ball.angle = self.angle;
					if (self.counter > 1)
					{
						if (trail)
						{
							ball.pos = trail.temp_pos;
							ball.vel = trail.temp_pos2;
						}
					}
					double scale = self.scale.X * siz;
					ball.scale = (scale, scale);
					ball.cstat = self.cstat;
					ball.extra = 0;

					ball.temp_pos = ball.pos;
					ball.temp_pos2 = ball.vel;
					ball.detail = 1;

					ball.ChangeStat(STAT_PROJECTILE);
				}
			}
			self.counter++;
		}
		if (self.vel.Z < 15000. / 256.)
			self.vel.Z += 200 / 256.;
		return false;
	}
	
	override bool weaponhitsprite_pre(DukeActor targ)
	{
		if (self.detail != 1)
			return super.weaponhitsprite_pre(targ);
		return false;
	}
	
	override bool weaponhitplayer(DukeActor targ)
	{
		let p = targ.GetPlayer();
		let Owner = self.ownerActor;

		if (p && ud.multimode >= 2 && Owner && Owner.isPlayer())
		{
			p.numloogs = -1 - self.yint;
		}
		return Super.weaponhitplayer(targ);
	}
	
	override bool weaponhitsector()
	{
		if (super.weaponhitsector()) return true;
		if (self.detail != 1)
		{
			let spawned = self.spawn('DukeLavapool');
			if (spawned)
			{
				spawned.ownerActor = self;
				spawned.hitOwnerActor = self;
				spawned.yint = self.yint;
			}
			self.Destroy();
			return true;
		}
		return false;
	}

	override void posthiteffect(CollisionData coll)
	{
		if (self.detail != 1)
		{
			let spawned = self.spawn('DukeExplosion2');
			if (spawned)
			{
				let scale = self.scale.X * 0.5;
				spawned.scale = (scale,scale);
			}
		}
		Super.postHitEffect(coll);
	}
	
	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		if (self.detail == 0) return 'DukeFlamethrowerFlame';
		return 'DukeRadiusExplosion';
	}
}

