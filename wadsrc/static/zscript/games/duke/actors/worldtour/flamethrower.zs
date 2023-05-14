class DukeFlamethrowerFlame : DukeActor
{
	const FLAMETHROWER_WEAPON_STRENGTH = 6;

	default
	{
		pic "FLAMETHROWERFLAME";
		+HITRADIUS_DONTHURTSPECIES;
		Strength FLAMETHROWER_WEAPON_STRENGTH;
	}

	override void Tick()
	{
		Console.Printf("ticky");
		let sectp = self.sector;
		double xx;
		Super.Tick();	// Run CON or its replacement.
		if (self.bDestroyed) return;	// killed by script.
		self.counter++;
		if (sectp.lotag == ST_2_UNDERWATER)
		{
			let spawned = self.spawn("DukeExplosion2");
			if (spawned) spawned.shade = 127;
			self.Destroy();
			return;
		}

		let dapos = self.pos;

		self.getglobalz();

		int ds = self.counter / 6;
		if (self.scale.X < 0.1250)
		{
			self.scale.X += (ds * REPEAT_SCALE);
			self.scale.Y = (self.scale.X);
		}
		self.clipdist += ds * 0.25;
		if (self.counter <= 2)
			self.temp_data[3] = random(0, 9);
		if (self.counter > 30) 
		{
			let spawned = self.spawn("DukeExplosion2");
			if (spawned) spawned.shade = 127;
			self.Destroy();
			return;
		}

		CollisionData coll;
		self.movesprite_ex((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK1, coll);

		if (self.sector == null)
		{
			self.Destroy();
			return;
		}

		if (coll.type != kHitSprite)
		{
			if (self.pos.Z < self.ceilingz)
			{
				coll.setSector(self.sector);
				self.vel.Z -= 1/256.;
			}
			else if ((self.pos.Z > self.floorz && self.sector.lotag != ST_1_ABOVE_WATER)
				|| (self.pos.Z > self.floorz + 16 && self.sector.lotag == ST_1_ABOVE_WATER))
			{
				coll.setSector(self.sector);
				if (self.sector.lotag != 1)
					self.vel.Z += 1/256.;
			}
		}

		if (coll.type != 0) 
		{
			self.vel.XY = (0, 0);
			self.vel.Z = 0;
			if (coll.type == kHitSprite)
			{
				let hitact = DukeActor(coll.hitActor());
				hitact.OnHit(self);
				if (hitact.isPlayer())
					hitact.PlayActorSound("PISTOL_BODYHIT");
			}
			else if (coll.type == kHitWall)
			{
				self.SetPosition(dapos);
				dlevel.checkhitwall(coll.hitWall(), self, self.pos);
			}
			else if (coll.type == kHitSector)
			{
				self.SetPosition(dapos);
				if (self.vel.Z < 0)
					dlevel.checkhitceiling(self.sector, self);
			}

			if (self.scale.X >= 0.15625)
			{
				int x = self.extra;
				self.hitradius(gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
			}
			else
			{
				int x = self.extra + (Duke.global_random() & 3);
				self.hitradius((gs.rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override bool shootthis(DukeActor actor, DukePlayer p, Vector3 spos, double sang) const
	{
		double vel, zvel = 0;

		if (actor.extra >= 0)
			actor.shade = -96;
		vel = 25;

		DukeActor spawned = nullptr;
		if (p == null)
		{
			double x;
			DukePlayer j;
			[j, x] = actor.findplayer();
			sang = (j.Actor.opos.XY - spos.XY).Angle();

			if (actor is 'DukeBoss5')
			{
				vel = 33;
				spos.Z += 24;
			}
			else if (actor is 'DukeBoss3')
				spos.Z -= 32;

			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			if (dist != 0)
				zvel = (((j.actor.opos.Z + j.actor.oviewzoffset - spos.Z) * vel) / dist);

			if (actor.badguy() && (actor.hitag & face_player_smart) != 0)
				sang = actor.Angle + Raze.BAngToDegree * random(-16, 15);

		}
		else
		{
			[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 40.5);
			
			// WTF???
			double myang = 90. - (180. - abs(abs((spos.XY - p.actor.pos.XY).Angle() - sang) - 180.));
			if (p.actor.vel.X != 0)
				vel = ((myang / 90.) * p.actor.vel.X) + 25;
		}

		if (actor.sector.lotag == ST_2_UNDERWATER && (random(0, 4)) == 0)
			spawned = actor.spawn("DukeWaterBubble");

		if (spawned == nullptr)
		{
			spawned = actor.spawn("DukeFlamethrowerFlame");
			if (!spawned) return true;
			spawned.vel.X = vel;
			spawned.vel.Z = zvel;
		}


		Vector3 offset;
		offset.X = cos(sang + Raze.BAngToDegree * 118) * (1024 / 448.); // Yes, these angles are really different!
		offset.Y = sin(sang + Raze.BAngToDegree * 112) * (1024 / 448.);
		offset.Z = -1;

		spawned.pos = spos + offset;
		spawned.pos.Z--;
		spawned.sector = actor.sector;
		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.Angle = sang;
		spawned.scale = (0.03125, 0.03125);
		spawned.clipdist = 10;
		spawned.yint = Duke.GetPlayerIndex(p);
		spawned.ownerActor = actor;

		if (p == null)
		{
			if (actor is 'DukeBoss5')
			{
				spawned.pos += sang.ToVector() * (128. / 7);
				spawned.scale = (0.15625, 0.15625);
			}
		}
		return true;
	}

	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		return 'DukeFlamethrowerFlame';
	}

}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLavaBubble : DukeActor // (4340)
{
	default
	{
		pic "LAVABUBBLE";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFlamethrowerSprite : DukeItemBase
{
	const FLAMETHROWERAMMOAMOUNT = 25;

	default
	{
		pic "FLAMETHROWERSPRITE";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFlamethrowerAmmo : DukeItemBase
{
	default
	{
		pic "FLAMETHROWERAMMO";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeOnFireSmoke : DukeActor
{
	default
	{
		pic "ONFIRESMOKE";
		+FORCERUNCON;
		Strength 1;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeOnFire : DukeActor
{
	default
	{
		pic "FLOORFLAME";
		+FORCERUNCON;
		Strength 0;
	}

	override void Initialize(DukeActor spawner)
	{
		if (spawner)
		{
			self.Angle = spawner.Angle;
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeBurnedCorpse : DukeActor
{
	default
	{
		pic "BURNEDCORPSE";
		+FORCERUNCON;
		Strength 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeWhispySmoke : DukeActor
{
	default
	{
		pic "WHISPYSMOKE";
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.pos.X += frandom(-8, 8);
		self.pos.Y += frandom(-8, 8);
		self.scale = (0.3125, 0.3125);
		self.ChangeStat(STAT_MISC);
	}
}		

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLavaSplash : DukeActor
{
	default
	{
		pic "LAVASPLASH";
		Strength 0;
	}
	
}		

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLavaPoolBubble : DukeActor
{
	default
	{
		pic "LAVAPOOLBUBBLE";
		statnum STAT_MISC;
		+FORCERUNCON;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		if (spawner.scale.X < 0.46875)
		{
			self.scale = (0, 0); 
			return;
		}
		self.pos.X += frandom(-16, 16);
		self.pos.Y += frandom(-16, 16);
		self.scale = (0.25, 0.25);
	}	
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLavaPool : DukeActor
{
	default
	{
		pic "LAVAPOOL";
		statnum STAT_MISC;
		+FORCERUNCON;
		+HITRADIUS_NODAMAGE;
		Strength 0;
	}
	
	override void Initialize(DukeActor spawner)
	{
		bool away = self.isAwayFromWall(6.75);
		
		if (!away)
		{
			self.scale = (0, 0); 
			return;
		}

		if (self.sector.lotag == 1)
		{
			return;
		}
	

		self.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		double c, f;
		[c, f] = self.sector.getslopes(self.pos.XY);
		self.pos.Z = f - 0.78125;
		if (!self.mapSpawned)
			self.scale = (REPEAT_SCALE, REPEAT_SCALE);
	}
	
	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		return 'DukeFlamethrowerFlame';
	}
}

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

		if (self.sector.lotag == ST_2_UNDERWATER)
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

