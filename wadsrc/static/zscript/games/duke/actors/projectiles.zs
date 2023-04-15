// Note: Duke's handling is dumb enough to make it impossible for other actors than the predefined projectile type to be used as projectile - 
// even if it is given the right statnum the projectile code won't get called for it.
// So even in the future any projectile needs to inherit from this to gain the needed feature support.

extend class DukeActor
{
	// placed in DukeActor so it remains reusable.
	void bounce()
	{
		Vector3 vect = (self.angle.ToVector() * self.vel.X, self.vel.Z);
		let sectp = self.sector;

		double daang = sectp.walls[0].delta().Angle();

		double k;
		if (self.pos.Z < (self.floorz + self.ceilingz) * 0.5)
			k = sectp.ceilingheinum;
		else
			k = sectp.floorheinum;

		Vector3 davec = (sin(daang) * k, -cos(daang) * k, 4096);

		double dotp = vect dot davec;
		double l = davec.LengthSquared();

		vect -= davec * (2 * dotp / l);

		self.vel.Z = vect.Z;
		self.vel.X = vect.XY.Length();
		self.angle = vect.Angle();
	}
}

class DukeProjectile : DukeActor
{
	default
	{
		statnum STAT_PROJECTILE;
	}
	
	Vector3 oldpos;		// holds the position before the current move
	meta Sound SpawnSound;

	property SpawnSound: SpawnSound;
	
	override void Initialize()
	{
		// do not call the parent's function here.
	}
	// this large batch of subsequently called virtuals is owed to the spaghetti-like implementation of the orignal moveprojectiles function.
	
	virtual bool premoveeffect()
	{
		return false;
	}
	
	virtual bool postmoveeffect(CollisionData coll)
	{
		if (coll.type != kHitSprite)
		{
			if (self.pos.Z < self.ceilingz)
			{
				coll.setSector(self.sector);
				self.vel.Z -= 1/256.;
			}
			else if ((self.pos.Z > self.floorz && self.sector.lotag != ST_1_ABOVE_WATER) ||
					(self.pos.Z > self.floorz + 16 && self.sector.lotag == ST_1_ABOVE_WATER))
			{
				coll.setSector(self.sector);
				if (self.sector.lotag != ST_1_ABOVE_WATER)
					self.vel.Z += 1/256.;
			}
		}
		return false;
	}
	
	virtual bool weaponhitsprite_pre(DukeActor targ)
	{
		targ.OnHit(self);
		return false;
	}
	
	virtual bool weaponhitplayer(DukeActor targ)
	{
		targ.PlayActorSound("PISTOL_BODYHIT");
		return false;
	}
	
	protected bool weaponhitsprite(DukeActor targ)
	{
		if (self.weaponhitsprite_pre(targ)) return true;
		if (!targ.isPlayer()) return false;
		return self.weaponhitplayer(targ);
	}

	virtual bool weaponhitwall(walltype wal)
	{
		if (self.bMIRRORREFLECT && dlevel.isMirror(wal))
		{
			let k = wal.delta().Angle();
			self.angle = k * 2 - self.angle;
			self.ownerActor = self;
			self.spawn("DukeTransporterStar");
			return true;
		}
		else
		{
			self.SetPosition(oldpos);
			dlevel.checkhitwall(wal, self, self.pos);

			if (self.bREFLECTIVE)
			{
				if (!dlevel.isMirror(wal))
				{
					self.extra >>= 1;
					self.yint--;
				}

				let k = wal.delta().Angle();
				self.angle = k * 2 - self.angle;
				return true;
			}
		}
		return false;
	}

	virtual bool weaponhitsector()
	{
		self.SetPosition(oldpos);

		if (self.vel.Z < 0)
		{
			if ((self.sector.ceilingstat & CSTAT_SECTOR_SKY) && (self.sector.ceilingpal == 0))
			{
				self.Destroy();
				return true;
			}

			dlevel.checkhitceiling(self.sector, self);
		}
		return false;
	}
	
	virtual void posthiteffect(CollisionData coll)
	{
		self.Destroy();
	}
	
	override void Tick()
	{
		double vel = self.vel.X;
		double velz = self.vel.Z;
		oldpos = self.pos;

		int p = -1;

		if (self.bUNDERWATERSLOWDOWN && self.sector.lotag == ST_2_UNDERWATER)
		{
			vel *= 0.5;
			velz *= 0.5;
		}

		self.getglobalz();
		if (self.premoveeffect()) return;

		CollisionData coll;
		self.movesprite_ex((self.angle.ToVector() * vel, velz), CLIPMASK1, coll);

		if (!self.sector)
		{
			self.Destroy();
			return;
		}
		
		if (self.postmoveeffect(coll)) return;

		if (coll.type != 0)
		{
			if (coll.type == kHitSprite)
			{
				if (self.weaponhitsprite(DukeActor(coll.hitactor()))) return;
			}
			else if (coll.type == kHitWall)
			{
				if (weaponhitwall(coll.hitWall())) return;
			}
			else if (coll.type == kHitSector)
			{
				if (weaponhitsector()) return;
			}
			posthiteffect(coll);
		}	
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFirelaser : DukeProjectile // Liztrooper shot
{
	default
	{
		spriteset "FIRELASER", "FIRELASER2", "FIRELASER3", "FIRELASER4", "FIRELASER5", "FIRELASER6";
		+INFLAME;
		+FULLBRIGHT;
		+MIRRORREFLECT;
	}
	override bool postmoveeffect(CollisionData coll)
	{
		if (Super.postmoveeffect(coll)) return true;
		for (int k = -3; k < 2; k++)
		{
			double zAdd = k * self.vel.Z / 24;
			let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(zAdd) + self.angle.ToVector() * k * 2., 'DukeFireLaserTrail', -40 + (k << 2), self.scale, 0, 0., 0., self.ownerActor, STAT_MISC);

			if (spawned)
			{
				spawned.opos = self.opos - self.pos + spawned.pos;
				spawned.cstat = CSTAT_SPRITE_YCENTER;
				spawned.pal = self.pal;
			}
		}
		return false;
	}
	
	override bool animate(tspritetype tspr)
	{
		if (Raze.isRR()) tspr.setSpritePic(self, ((PlayClock >> 2) % 6));
		return true;
	}

}

class DukeFirelaserTrail : DukeActor
{
	default
	{
		spriteset "FIRELASER", "FIRELASER2", "FIRELASER3", "FIRELASER4", "FIRELASER5", "FIRELASER6";
		+FULLBRIGHT;
	}
	
	override void Tick()
	{
		if (self.extra == 999)
		{
			self.Destroy();
		}
	}
	
	override bool animate(tspritetype tspr)
	{
		self.extra = 999;
		if (Raze.isRR()) tspr.setSpritePic(self, ((PlayClock >> 2) % 6));
		return true;
	}
	
}

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
		+FULLBRIGHT;
		+MIRRORREFLECT;
		+NOFLOORPAL;
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
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner != self)
		{
			self.Angle = owner.Angle;
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeRPG : DukeProjectile
{
	default
	{
		pic "RPG";
		+FULLBRIGHT;
		+INFLAME;
		+UNDERWATERSLOWDOWN;
		+ALWAYSROTATE2;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+NOFLOORPAL;
		+BREAKMIRRORS;
		DukeProjectile.SpawnSound "RPG_SHOOT";
	}
	
	override bool premoveeffect()
	{
		if ((!self.ownerActor || !self.ownerActor.bNONSMOKYROCKET) && self.scale.X >= 0.15625 && self.sector.lotag != ST_2_UNDERWATER)
		{
			let spawned = self.spawn("DukeSmallSmoke");
			if (spawned) spawned.pos.Z += 1;
		}
		return super.premoveeffect();
	}
	
	override bool postmoveeffect(CollisionData coll)
	{
		Super.postmoveeffect(coll);
		if (self.temp_actor != nullptr && (self.pos.XY - self.temp_actor.pos.XY).LengthSquared() < 16 * 16)
			coll.setActor(self.temp_actor);
		return false;
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.rpgexplode(coll.type, oldpos, true, -1, "RPG_EXPLODE");
		self.Destroy();
	}
	
	void rpgexplode(int hit, Vector3 pos, bool exbottom, int newextra, Sound playsound)
	{
		let explosion = self.spawn("DukeExplosion2");
		if (!explosion) return;
		explosion.pos = pos;

		if (self.scale.X < 0.15625)
		{
			explosion.scale = (0.09375, 0.09375);
		}
		else if (hit == kHitSector)
		{
			if (self.vel.Z > 0 && exbottom)
				self.spawn("DukeExplosion2Bot");
			else
			{
				explosion.cstat |= CSTAT_SPRITE_YFLIP;
				explosion.pos.Z += 48;
			}
		}
		if (newextra > 0) self.extra = newextra;
		self.PlayActorSound(playsound);

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
	
	override void Tick()
	{
		super.Tick();
		if (self.sector && self.sector.lotag == ST_2_UNDERWATER && self.scale.X >= 0.15625 && Duke.rnd(140))
			self.spawn('DukeWaterBubble');
		
	}
	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		if (targhealth > 0) return 'DukeRPG';
		return 'DukeRadiusExplosion';
	}

}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeFreezeBlast : DukeProjectile
{
	default
	{
		pic "FREEZEBLAST";
		+FULLBRIGHT;
		+FREEZEDAMAGE;
		+REFLECTIVE;
	}
	
	override bool postmoveeffect(CollisionData coll)
	{
		return false;
	}

	override bool weaponhitsprite_pre(DukeActor targ)
	{
		if (targ.pal == 1) // is target already frozen?
		{
			if (targ.badguy() || targ.isPlayer())
			{
				let spawned = targ.spawn('DukeTransporterStar');
				if (spawned)
				{
					spawned.pal = 1;
					spawned.scale = (0.5, 0.5);
				}

				self.Destroy();
				return true;
			}
		}
		return super.weaponhitsprite_pre(targ);
	}
	
	override void Tick()
	{
		
		if (self.yint < 1 || self.extra < 2 || (self.vel.X == 0 && self.vel.Z == 0))
		{
			let star = self.spawn("DukeTransporterStar");
			if (star)
			{
				star.pal = 1;
				star.scale = (0.5, 0.5);
			}
			self.Destroy();
		}
		else
			Super.Tick();
	
	}
	
	override bool weaponhitsector()
	{
		self.bounce();
		self.doMove(CLIPMASK1);
		self.extra >>= 1;
		if (self.scale.X > 0.125 )
			self.scale.X -= 0.03125;
		if (self.scale.Y > 0.125 )
			self.scale.Y -= 0.03125;
		self.yint--;
		return true;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeSpit : DukeProjectile
{
	default
	{
		pic "SPIT";
	}

	override bool postmoveeffect(CollisionData coll)
	{
		Super.postmoveeffect(coll);
		if (self.vel.Z < 24)
			self.vel.Z += gs.gravity - 112 / 256.;
		return false;
	}
	
	override bool weaponhitplayer(DukeActor targ)
	{
		if (Super.weaponhitplayer(targ)) return true;
		
		let p = targ.GetPlayer();
		
		p.addPitch(-14.04);
		p.centerview();

		if (p.loogcnt == 0)
		{
			if (!p.actor.CheckSoundPlaying("PLAYER_LONGTERM_PAIN"))
				p.actor.PlayActorSound("PLAYER_LONGTERM_PAIN");

			int j = random(3, 7);
			p.numloogs = j;
			p.loogcnt = 24 * 4;
			for (int x = 0; x < j; x++)
			{
				p.loogie[x].X = random(0, 319);
				p.loogie[x].Y = random(0, 199);
			}
		}
		return false;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeCoolExplosion1 : DukeProjectile // octabrain shot.
{
	default
	{
		spriteset "COOLEXPLOSION1", "COOLEXPLOSION2", "COOLEXPLOSION3", "COOLEXPLOSION4", "COOLEXPLOSION5", 
			"COOLEXPLOSION6", "COOLEXPLOSION7", "COOLEXPLOSION8", "COOLEXPLOSION9", "COOLEXPLOSION10", 
			"COOLEXPLOSION11", "COOLEXPLOSION12", "COOLEXPLOSION13", "COOLEXPLOSION14", "COOLEXPLOSION15", 
			"COOLEXPLOSION16", "COOLEXPLOSION17", "COOLEXPLOSION18", "COOLEXPLOSION19", "COOLEXPLOSION20";
		+FULLBRIGHT;
		+MIRRORREFLECT;
	}
	
	override void Initialize()
	{
		self.angle = self.ownerActor.angle;
		self.shade = -64;
		self.cstat = CSTAT_SPRITE_YCENTER | self.randomXFlip();

		double c, f;
		[c, f] = self.sector.getSlopes(self.pos.XY);
		if (self.pos.Z > f - 12)
			self.pos.Z = f - 12;
	}
	
	override bool premoveeffect()
	{
		if (!self.CheckSoundPlaying("WIERDSHOT_FLY"))
			self.PlayActorSound("WIERDSHOT_FLY");
		return false;
	}
	
	override bool weaponhitsprite_pre(DukeActor targ)
	{
		if (!targ.isPlayer())
		{
			return true;
		}
		self.vel.X = self.vel.Z = 0;
		return super.weaponhitsprite_pre(targ);
	}

	override bool weaponhitwall(walltype wal)
	{
		self.vel.X = self.vel.Z = 0;
		return super.weaponhitwall(wal);
	}

	override bool weaponhitsector()
	{
		self.vel.X = self.vel.Z = 0;
		return super.weaponhitsector();
	}

	override void posthiteffect(CollisionData coll)
	{
		// don't destroy.
	}
	
	override void Tick()
	{
		Super.Tick();
		if (++self.shade >= 40)
		{
			self.Destroy();
		}
	}
	
	override bool animate(tspritetype tspr)
	{
		tspr.setSpritePic(self, clamp((self.shade >> 1), 0, 19));
		return true;
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

//---------------------------------------------------------------------------
//
// These 3 just use the base projectile code...
//
//---------------------------------------------------------------------------

class RedneckUWhip : DukeProjectile
{
	default
	{
		pic "UWHIP";
		+FULLBRIGHT;
		+INFLAME;
	}
}

class RedneckOWhip : RedneckUWhip
{
	default
	{
		pic "OWHIP";
	}
}

class RedneckVixenShot : RedneckUWhip // COOLEXPLOSION1
{
	default
	{
		pic "VIXENSHOT";
		+INFLAME;
	}
}

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class RedneckBoatGrenade : RedneckDynamiteArrow // RRRA only
{
	default
	{
		pic "BOATGRENADE";
		-DOUBLEDMGTHRUST;
		-ALWAYSROTATE2;
		DukeProjectile.SpawnSound "MORTAR";
	}
	
	override void Initialize()
	{
		
		self.extra = 10;
		self.vel.Z = -10;
		self.vel.X *= 2;
		super.Initialize();
	}

	override bool premoveeffect()
	{
		if (self.extra)
		{
			self.vel.Z = -(self.extra * 250/256.); // 250 looks like a typo...
			self.extra--;
		}
		else
			self.makeitfall();
		
		return Super.premoveeffect();
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.rpgexplode(coll.type, oldpos, false, 160, "RPG_EXPLODE");
		self.Destroy();
	}
	
	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		return 'DukeRadiusExplosion';
	}
	
}

//---------------------------------------------------------------------------
//
// this class is called shitball - but it's not just about throwing shit in the game,
// the entire logic with 4 different looks depending on the shooter is also shit...
//
//---------------------------------------------------------------------------

class RedneckShitBall : DukeSpit
{
	default
	{
		spriteset "SHITBALL", "SHITBALL2", "SHITBALL3", "SHITBALL4", 
			"FROGBALL1", "FROGBALL2", "FROGBALL3", "FROGBALL4", "FROGBALL5", "FROGBALL6", 
			"SHITBURN", "SHITBURN2", "SHITBURN3", "SHITBURN4",
			"RABBITBALL";
		+NOFLOORPAL;
	}
	
	private void rabbitguts()
	{
		self.spawnguts('RedneckRabbitJibA', 2);
		self.spawnguts('RedneckRabbitJibB', 2);
		self.spawnguts('RedneckRabbitJibC', 2);

	}
	override bool weaponhitplayer(DukeActor targ)
	{
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();

		return Super.weaponhitplayer(targ);
	}
	
	override bool weaponhitwall(walltype wal)
	{
		self.SetPosition(oldpos);
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();
		
		return super.weaponhitwall(wal);
	}	
	
	override bool weaponhitsector()
	{
		self.setPosition(oldpos);
		if (ownerActor && ownerActor.bSPAWNRABBITGUTS)
			rabbitguts();

		return super.weaponhitsector();
	}
		
	override bool animate(tspritetype tspr)
	{
		int sprite = ((PlayClock >> 4) & 3);
		if (self.ownerActor)
		{
			let OwnerAc = self.ownerActor;
			if (OwnerAc.bTRANSFERPALTOJIBS)
			{
				if (OwnerAc.pal == 8)
				{
					sprite = 4 + ((PlayClock >> 4) % 6);
				}
				else if (OwnerAc.pal == 19)
				{
					sprite = 10 + ((PlayClock >> 4) & 3);
					tspr.shade = -127;
				}
			}
			else if (OwnerAc.bSPAWNRABBITGUTS)
			{
				tspr.clipdist |= TSPR_ROTATE8FRAMES;
				sprite = 14;
			}
		}
		return true;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RedneckSawBlade : DukeProjectile
{
	default
	{
		spriteset "SAWBLADE", "SAWBLADE2", "SAWBLADE3",  "SAWBLADE4",  "SAWBLADE5",  "SAWBLADE6",  "SAWBLADE7",  "SAWBLADE8",
			"CHEERBLADE", "CHEERBLADE2", "CHEERBLADE3", "CHEERBLADE4";
	}

	override bool weaponhitwall(walltype wal)
	{
		if (dlevel.wallflags(wal, 0) & Duke.TFLAG_NOCIRCLEREFLECT)
		{
			self.Destroy();
			return true;
		}
		if (self.extra <= 0)
		{
			self.pos += self.angle.ToVector() * 8;
			let Owner = self.ownerActor;
			if (!Owner || !(Owner.bALTPROJECTILESPRITE)) // depends on the shooter. Urgh...
			{
				let j = self.spawn("RedneckCircleStuck");
				if (j)
				{
					j.scale = (0.125, 0.125);
					j.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
					j.angle += 90;
					j.clipdist = self.scale.X * self.spriteWidth() * 0.125;
				}
			}
			self.Destroy();
			return true;
		}
		if (!dlevel.isMirror(wal))
		{
			self.extra -= 20;
			self.yint--;
		}

		let k = wal.delta().Angle();
		self.angle = k * 2 - self.angle;
		return true;
	}
	
	override bool animate(tspritetype tspr)
	{
		int frame;
		if (!OwnerActor || !(OwnerActor.bALTPROJECTILESPRITE)) frame = ((PlayClock >> 4) & 7);
		else frame = 8 + ((PlayClock >> 4) & 3);
		tspr.SetSpritePic(self, frame);
		return true;
	}
}


class RedneckCircleStuck : DukeActor
{
	default
	{
		pic "CIRCLESTUCK";
	}
}